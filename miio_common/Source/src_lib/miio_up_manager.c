#include <wmlist.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <appln_dbg.h>
#include <ot/d0_otd.h>
#include <lib/jsmn/jsmn_api.h>
#include <lib/arch_os.h>
#include <lib/miio_up_manager.h>
#include <lib/miio_up_manager_priv.h>

#define PROPERTY_UP_COOLDOWN_TIME (1000)
#define EVENT_UP_COOLDOWN_TIME    (1000)
#define EVENT_TIMEOUT_MS          (1000*30)
#define MAX_KEY_LEN      (32)
#define MAX_VALUE_LEN    (900)
#define MAX_UP_JSON_LEN  (1024)
#define MAX_EVENT_LIST_LEN (8)
#define MAX_METHOD_NAME_LEN (32)


enum item_status {
    SENT = 0, DIRTY, SENDING
};

struct property_entry {
    struct list_head list;
    char key[MAX_KEY_LEN];
    char * value;
    uint32_t value_buf_len;
    uint32_t status;
    uint32_t session;
    uint32_t last_sent_tick;
};

struct event_entry {
    struct list_head list;
    char key[MAX_KEY_LEN];
    char * value;
    uint32_t value_buf_len;
    //uint32_t status;
    //uint32_t session;
    uint32_t occur_tick;
};

struct common_entry {
    struct list_head list;
    char key[MAX_KEY_LEN];
    char * value;
    uint32_t value_buf_len;
    uint32_t status;
    uint32_t session;
    uint32_t occur_tick;
    char method_name[MAX_METHOD_NAME_LEN];
};


typedef struct mum_struct {
    list_head_t property_list;
    list_head_t event_list;
    list_head_t common_list;
    os_mutex_vt* prop_mutex;
    os_mutex_vt* evnt_mutex;
    os_mutex_vt* common_data_mutex;
    uint32_t session_id;
    char js[MAX_UP_JSON_LEN];
    os_mutex_vt* js_buf_mutex;
    os_thread_vt *up_task;
    os_semaphore_t up_sem;
}*mum;

int mum_send_common_data(mum self);
static int mum_up_task(void * arg)
{
    mum handle = (mum)arg;

    while(1) {
        if (WM_SUCCESS == os_semaphore_get(&handle->up_sem, OS_WAIT_FOREVER)) {
            mum_send_property(handle);
            arch_os_tick_sleep(500);
            mum_send_one_event_with_retry(handle, 3);
            arch_os_tick_sleep(500);
            mum_send_common_data(handle);
            arch_os_tick_sleep(500);
        }
    }

    return 0;
}

mum mum_create(void)
{
    mum h = mum_create2();

    if(h) {
        int ret = os_semaphore_create(&h->up_sem, "up_sem");
        if(ret != WM_SUCCESS) {
            mum_destroy(&h);
            return NULL;
        }
        arch_os_thread_create(&h->up_task, "up_task", mum_up_task, 1024, (void*)h);
    }
    return h;
}

void mum_destroy(mum *self)
{
    if (*self) {
        arch_os_thread_delete((*self)->up_task);
        os_semaphore_delete(&(*self)->up_sem);
        (*self)->up_sem = NULL;
        mum_destroy2(self);
    }
}


mum mum_create2(void)
{
    mum h = NULL;
    h = malloc(sizeof(*h));
    if (!h)
        return NULL;

    arch_os_mutex_create(&h->prop_mutex);
    arch_os_mutex_create(&h->evnt_mutex);
    arch_os_mutex_create(&h->common_data_mutex);
    arch_os_mutex_create(&h->js_buf_mutex);
    INIT_LIST_HEAD(&h->property_list);
    INIT_LIST_HEAD(&h->event_list);
    INIT_LIST_HEAD(&h->common_list);
    h->session_id = 0;
    h->up_sem = NULL;

    return h;
}

void mum_destroy2(mum *self)
{
    if (*self) {
        struct property_entry *prop, *temp_prop;
        arch_os_mutex_get((*self)->prop_mutex, OS_WAIT_FOREVER);
        list_for_each_entry_safe(prop, temp_prop, &(*self)->property_list, list) {
            list_del(&prop->list);
            if(prop->value)
                free(prop->value);
            if(prop)
                free(prop);
        }

        struct event_entry *event,
        *temp_event;
        arch_os_mutex_get((*self)->evnt_mutex, OS_WAIT_FOREVER);
        list_for_each_entry_safe(event, temp_event, &(*self)->event_list, list)
        {
            list_del(&event->list);
            if (event->value)
                free(event->value);
            if (event)
                free(event);
        }

        struct event_entry *common_data,*temp_common_data;
        arch_os_mutex_get((*self)->common_data_mutex, OS_WAIT_FOREVER);
        list_for_each_entry_safe(common_data, temp_common_data, &(*self)->common_list, list)
        {
            list_del(&common_data->list);
            if (common_data->value)
                free(common_data->value);
            if (common_data)
                free(common_data);
        }

        arch_os_mutex_put((*self)->prop_mutex);
        arch_os_mutex_put((*self)->evnt_mutex);
        arch_os_mutex_put((*self)->common_data_mutex);

        arch_os_mutex_delete((*self)->prop_mutex);
        arch_os_mutex_delete((*self)->evnt_mutex);
        arch_os_mutex_delete((*self)->common_data_mutex);
        arch_os_mutex_delete((*self)->js_buf_mutex);

        free(*self);
        *self = NULL;
    }
}

static struct property_entry * get_prop_by_name(mum self, const char * key)
{
    struct property_entry *prop, *ret = NULL;
    arch_os_mutex_get(self->prop_mutex, OS_WAIT_FOREVER);
    list_for_each_entry(prop, &self->property_list, list)
    {
        if (strcmp(key, prop->key) == 0) {
            ret = prop;
            break;
        }
    }
    arch_os_mutex_put(self->prop_mutex);
    return ret;
}

static int update_prop(struct property_entry *p, const char * value, bool need_report)
{
    p->status = need_report? DIRTY : SENT;
    p->session = 0;
    if (strlen(value) + 1 > p->value_buf_len) {
        p->value = realloc(p->value, strlen(value) + 1);
    }
    if (p->value) {
        p->value_buf_len = strlen(value) + 1;
        strcpy(p->value, value);
        return MUM_OK;
    } else {
        return -MUM_FAIL;
    }
}

static int new_prop(mum self, const char * key, const char * value, bool need_report)
{
    char * value_buf = NULL;
    struct property_entry *p = NULL;
    size_t value_len = strlen(value);
    if (value_len >= MAX_VALUE_LEN)
        goto err_exit;

    value_buf = malloc(value_len + 1);
    if (!value_buf)
        goto err_exit;

    p = malloc(sizeof(*p));
    if (!p)
        goto err_exit;

    INIT_LIST_HEAD(&p->list);
    strncpy(p->key, key, sizeof(p->key));
    p->value = value_buf;
    p->value_buf_len = value_len + 1;
    p->last_sent_tick = arch_os_tick_now() - PROPERTY_UP_COOLDOWN_TIME - 1;
    update_prop(p, value, need_report);
    arch_os_mutex_get(self->prop_mutex, OS_WAIT_FOREVER);
    list_add(&p->list, &self->property_list);
    arch_os_mutex_put(self->prop_mutex);
    return MUM_OK;

err_exit:
    if (value_buf)
        free(value_buf);
    if (p)
        free(p);
    return -MUM_FAIL;
}

static const char *skip(const char *in)
{
    while (in && (unsigned char)*in <= 32)
        in++;
    return in;
}

static const char *rev_skip(const char *in)
{
    while (in && (unsigned char)*in <= 32)
        in--;
    return in;
}

static int verify_as_number(const char * str, size_t len)
{
    const char * str_head = skip(str);
    const char * str_tail = rev_skip(str + len - 1);
    const char * p = str_head + 1;

    if(str_tail < str_head)
        return -MUM_VALUE_INVALID;

    if(!((*str_head >= '0' && *str_head <= '9') || *str_head == '-'))
        return -MUM_VALUE_INVALID;

    while(p <= str_tail) {
        if((*p < '0' || *p > '9') && *p != '.')
            return -MUM_VALUE_INVALID;
        p++;
    }

    return MUM_OK;
}

static int verify_as_string(const char * str, size_t len)
{
    const char * str_head = skip(str);
    const char * str_tail = rev_skip(str + len - 1);
    const char * p = str_head + 1;

    if(str_tail <= str_head)
        return -MUM_VALUE_INVALID;

    if(*str_head != '"' || *str_tail != '"')
        return -MUM_VALUE_INVALID;

    while(p < str_tail) {
        if(*p < 32 || *p > 126 || *p == '"')
            return -MUM_VALUE_INVALID;
        p++;
    }

    return MUM_OK;
}

static int verify_as_bool(const char * str, size_t len)
{
    const char * str_head = skip(str);
    const char * str_tail = rev_skip(str + len - 1);

    if(str_tail < str_head)
        return -MUM_VALUE_INVALID;

    if(str_tail - str_head == 3 && strncmp(str_head, "true", 4) == 0)
        return MUM_OK;

    if(str_tail - str_head == 4 && strncmp(str_head, "false", 5) == 0)
        return MUM_OK;

    return -MUM_VALUE_INVALID;
}

static inline int verify_as_null(const char * str, size_t len)
{
    if(len == 0 || (len == 4 && strncmp(str, "null", 4) == 0))
        return MUM_OK;
    else
        return -MUM_VALUE_INVALID;
}

static int verify_key(const char * key)
{
    if(!key || *key == '\0')
        return -MUM_KEY_INVALID;
    if (strlen(key) >= MAX_KEY_LEN)
        return -MUM_KEY_TOO_LONG;
    while(*key != '\0') {
        if(!((*key >= '0' && *key <= '9')||(*key >= 'A' && *key <= 'Z')||(*key >= 'a' && *key <= 'z')||*key == '_'))
            return -MUM_KEY_INVALID;
        key++;
    }
    return MUM_OK;
}

static int verify_prop_value(const char * value)
{
    if(!value)
        return -MUM_VALUE_INVALID;
    if (strlen(value) >= MAX_VALUE_LEN)
        return -MUM_VALUE_TOO_LONG;
    if(verify_as_string(value, strlen(value)) < 0 && verify_as_number(value, strlen(value)) < 0
            && verify_as_bool(value, strlen(value)) < 0)
        return -MUM_VALUE_INVALID;
    return MUM_OK;
}

static size_t find_value_size(const char * value)
{
    const char * p = value;
    bool in_quote = false;
    while(*p != '\0') {
        if(!in_quote && *p == ',')
            break;
        if(*p == '"')
            in_quote = !in_quote;
        p++;
    }
    return p - value;
}

static int verify_event_value(const char * value)
{
    if(!value)
        return MUM_OK;
    if (strlen(value) >= MAX_VALUE_LEN)
        return -MUM_VALUE_TOO_LONG;
    while(*value != '\0') {
        size_t sz = find_value_size(value);
        int is_null = verify_as_null(value, sz);
        int is_string = verify_as_string(value, sz);
        int is_number = verify_as_number(value, sz);
        int is_bool = verify_as_bool(value, sz);
        if(is_null < 0 && is_string < 0 && is_number < 0 && is_bool < 0)
            return -MUM_VALUE_INVALID;
        value += sz;
        if(*value == ',')
            value++;
    }
    return MUM_OK;
}

static int _mum_set_property(mum self, const char *key, const char *value, bool need_report)
{
    int ret;
    struct property_entry *prop;

    if (!self)
        return -MUM_NOT_INIT;
    ret = verify_key(key);
    if (ret < 0)
        return ret;

    ret = verify_prop_value(value);
    if (ret < 0)
        return ret;

    prop = get_prop_by_name(self, key);

    if (prop) {
        ret = update_prop(prop, value, need_report);
    } else {
        ret = new_prop(self, key, value, need_report);
    }

    if(self->up_sem)
        os_semaphore_put(&self->up_sem);


    return ret;
}

int mum_set_property(mum self, const char *key, const char *value)
{
    return _mum_set_property(self, key, value, true);
}

int mum_set_property_without_report(mum self, const char *key, const char *value)
{
    return _mum_set_property(self, key, value, false);
}

int mum_set_event(mum self, const char *key, const char *value)
{
    int ret;
    struct event_entry *event;
    char * value_buf = NULL;

    if (!self)
        return -MUM_NOT_INIT;

    ret = verify_key(key);
    if (ret < 0)
        return ret;

    if (NULL == value)
        value = "";

    ret = verify_event_value(value);
    if (ret < 0)
        return ret;

    if(mum_get_event_no(self) >= MAX_EVENT_LIST_LEN)
        return MUM_FAIL;

    size_t value_len = strlen(value);

    event = malloc(sizeof(*event));
    if(!event)
        goto err_exit;
    value_buf = malloc(value_len + 1);
    if(!value_buf)
        goto err_exit;
    event->value = value_buf;
    event->value_buf_len = value_len + 1;
    //event->status = DIRTY;
    //event->session = 0;
    event->occur_tick = arch_os_tick_now();

    INIT_LIST_HEAD(&event->list);
    strncpy(event->key, key, sizeof(event->key));
    strncpy(event->value, value, event->value_buf_len);
    arch_os_mutex_get(self->evnt_mutex, OS_WAIT_FOREVER);
    list_add(&event->list, &self->event_list);
    arch_os_mutex_put(self->evnt_mutex);

    if(self->up_sem)
        os_semaphore_put(&self->up_sem);

    return MUM_OK;

err_exit:
    if(value_buf)
        free(value_buf);
    if(event)
        free(event);
    return -MUM_FAIL;
}


int mum_set_common_data(mum self, char* method_name, const char *key, const char *value)
{
    int ret;
    struct common_entry *common_data;
    char * value_buf = NULL;

    if (!self)
        return -MUM_NOT_INIT;

    ret = verify_key(key);
    if (ret < 0)
        return ret;

    if (NULL == value)
        value = "";

    //ret = verify_log_value(value);
    //if (ret < 0)
    //    return ret;

    //if(mum_get_event_no(self) > MAX_EVENT_LIST_LEN)
    //    return MUM_FAIL;

    size_t value_len = strlen(value);

    common_data = malloc(sizeof(*common_data));
    if(!common_data)
        goto err_exit;
    value_buf = malloc(value_len + 1);
    if(!value_buf)
        goto err_exit;
    strncpy(common_data->method_name, method_name, sizeof(common_data->method_name));
    common_data->value = value_buf;
    common_data->value_buf_len = value_len + 1;
    common_data->status = DIRTY;
    common_data->session = 0;
    common_data->occur_tick = arch_os_tick_now();

    INIT_LIST_HEAD(&common_data->list);
    strncpy(common_data->key, key, sizeof(common_data->key));
    strncpy(common_data->value, value, common_data->value_buf_len);
    arch_os_mutex_get(self->common_data_mutex, OS_WAIT_FOREVER);
    list_add(&common_data->list, &self->common_list);
    arch_os_mutex_put(self->common_data_mutex);

    if(self->up_sem)
        os_semaphore_put(&self->up_sem);

    return MUM_OK;

err_exit:
    if(value_buf)
        free(value_buf);
    if(common_data)
        free(common_data);
    return -MUM_FAIL;
}


int mum_set_log (mum self, const char *key, const char *value)
{
    return mum_set_common_data(self, "_otc.log", key, value);
}

int mum_set_store (mum self, const char *key, const char *value)
{
    return mum_set_common_data(self, "_async.store", key, value);
}

int mum_get_property(mum self, const char *key, char *value)
{
    if (!self)
        return -MUM_NOT_INIT;

    struct property_entry *prop;
    prop = get_prop_by_name(self, key);
    if (prop) {
        strcpy(value, prop->value);
        return MUM_OK;
    } else {
        return -MUM_ITEM_NONEXIST;
    }
}

int mum_get_event(mum self, const char *key, char *value)
{
    if (!self)
        return -MUM_NOT_INIT;

    bool found = false;
    struct event_entry *event;
    arch_os_mutex_get(self->evnt_mutex, OS_WAIT_FOREVER);
    list_for_each_entry(event, &self->event_list, list)
    {
        if (strcmp(key, event->key) == 0) {
            strcpy(value, event->value);
            found = true;
            break;
        }
    }
    arch_os_mutex_put(self->evnt_mutex);

    if (found)
        return MUM_OK;
    else
        return -MUM_ITEM_NONEXIST;
}

int mum_get_property_no(mum self)
{
    if (!self)
        return 0;

    int count = 0;
    list_head_t *iter;
    arch_os_mutex_get(self->prop_mutex, OS_WAIT_FOREVER);
    list_for_each(iter, &self->property_list)
    {
        count++;
    }
    arch_os_mutex_put(self->prop_mutex);
    return count;
}

int mum_get_event_no(mum self)
{
    if (!self)
        return 0;

    int count = 0;
    list_head_t *iter;
    arch_os_mutex_get(self->evnt_mutex, OS_WAIT_FOREVER);
    list_for_each(iter, &self->event_list)
    {
        count++;
    }
    arch_os_mutex_put(self->evnt_mutex);

    return count;
}

int mum_get_specific_event_no(mum self, const char *key)
{
    if (!self)
        return 0;

    int count = 0;
    struct event_entry *event;
    arch_os_mutex_get(self->evnt_mutex, OS_WAIT_FOREVER);
    list_for_each_entry(event, &self->event_list, list)
    {
        if (strcmp(key, event->key) == 0)
            count++;
    }
    arch_os_mutex_put(self->evnt_mutex);

    return count;
}

static int alloc_session_id(mum self)
{
    self->session_id++;
    if (self->session_id == 0)
        self->session_id = 1;
    return self->session_id;
}

uint32_t mum_make_json_property(mum self, char * js, uint32_t *js_len, uint32_t *session)
{
    if (!self)
        return 0;

    int n = 0, prop_count = 0, js_buf_len = *js_len;
    n += snprintf_safe(js + n, js_buf_len - n, "{\"retry\":3,\"timeout\":2000,\"method\":\"props\",\"params\":{");

    struct property_entry *prop;
    arch_os_mutex_get(self->prop_mutex, OS_WAIT_FOREVER);
    *session = alloc_session_id(self);
    list_for_each_entry(prop, &self->property_list, list)
    {
        if (prop->status == DIRTY && strlen(prop->key) + strlen(prop->value) + 5 < js_buf_len - n
        && arch_os_tick_elapsed(prop->last_sent_tick) > PROPERTY_UP_COOLDOWN_TIME) {
            if (prop_count++ > 0)
                n += snprintf_safe(js + n, js_buf_len - n, ",");
            n += snprintf_safe(js + n, js_buf_len - n, "\"%s\":%s", prop->key, prop->value);
            prop->status = SENDING;
            prop->session = *session;
        }
    }
    arch_os_mutex_put(self->prop_mutex);

    n += snprintf_safe(js + n, js_buf_len - n, "}}");

    *js_len = n;

    return prop_count;
}

struct up_ack_ctx {
    mum handle;
    uint32_t session;
    uint32_t retry;
};

int mum_send_property_ack(jsmn_node_t* pjn, void* ctx)
{
    struct up_ack_ctx * ack_ctx = ctx;
    mum self = ack_ctx->handle;
    uint32_t session = ack_ctx->session;
    int status = (pjn && pjn->js && NULL == pjn->tkn) ? DIRTY : SENT;
    uint32_t retry = ack_ctx->retry;

    if (ctx)
        free(ctx);

    if (!self)
        return 0;

    struct property_entry *prop;
    arch_os_mutex_get(self->prop_mutex, OS_WAIT_FOREVER);
    list_for_each_entry(prop, &self->property_list, list)
    {
        if (prop->status == SENDING && prop->session == session) {
            prop->status = status;
            if (status == SENT) {
                prop->last_sent_tick = arch_os_tick_now();
            }
        }
    }
    arch_os_mutex_put(self->prop_mutex);

    // try until success if retry is not used up
    if(retry > 0 && status != SENT)
        mum_send_property_with_retry(self, retry - 1);

    return 0;
}

int mum_send_property_with_retry(mum self, uint32_t retry)
{
    if (!self)
        return -MUM_NOT_INIT;

    if(NULL == otn_is_online())
        return 0;

    uint32_t js_len = MAX_UP_JSON_LEN;
    uint32_t session = 0;
    uint32_t prop_count = 0;

    arch_os_mutex_get(self->js_buf_mutex, OS_WAIT_FOREVER);
    prop_count = mum_make_json_property(self, self->js, &js_len, &session);

    if (prop_count > 0) {
        struct up_ack_ctx *ctx = malloc(sizeof(struct up_ack_ctx));
        ctx->handle = self;
        ctx->session = session;
        ctx->retry = retry;
        ot_api_method(self->js, js_len, mum_send_property_ack, (void*) ctx);
        LOG_INFO("up:%s\r\n", self->js);
    }
    arch_os_mutex_put(self->js_buf_mutex);

    return prop_count;
}

int mum_send_property(mum self)
{
    return mum_send_property_with_retry(self, 0);
}

uint32_t mum_get_dirty_property_no(mum self)
{
    if (!self)
        return 0;

    int count = 0;
    struct property_entry *prop;
    arch_os_mutex_get(self->prop_mutex, OS_WAIT_FOREVER);
    list_for_each_entry(prop, &self->property_list, list)
    {
        if (prop->status == DIRTY)
            count++;
    }
    arch_os_mutex_put(self->prop_mutex);
    return count;
}

uint32_t mum_get_sending_property_no(mum self)
{
    if (!self)
        return 0;

    int count = 0;
    struct property_entry *prop;
    arch_os_mutex_get(self->prop_mutex, OS_WAIT_FOREVER);
    list_for_each_entry(prop, &self->property_list, list)
    {
        if (prop->status == SENDING)
            count++;
    }
    arch_os_mutex_put(self->prop_mutex);
    return count;
}

uint32_t mum_get_sent_property_no(mum self)
{
    if (!self)
        return 0;

    int count = 0;
    struct property_entry *prop;
    arch_os_mutex_get(self->prop_mutex, OS_WAIT_FOREVER);
    list_for_each_entry(prop, &self->property_list, list)
    {
        if (prop->status == SENT)
            count++;
    }
    arch_os_mutex_put(self->prop_mutex);
    return count;
}

int mum_send_one_event_with_retry(mum self, uint32_t retry)
{
    if (!self)
        return 0;

    if(NULL == otn_is_online())
        return 0;

    int js_len = 0;
    int evnt_count = 0;
    struct event_entry *evnt, *tmp;
    arch_os_mutex_get(self->evnt_mutex, OS_WAIT_FOREVER);
    arch_os_mutex_get(self->js_buf_mutex, OS_WAIT_FOREVER);
    list_for_each_entry_safe(evnt, tmp, &self->event_list, list)
    {
        if (arch_os_tick_elapsed(evnt->occur_tick) < EVENT_TIMEOUT_MS) {
            /* construct json */
            js_len = snprintf_safe(self->js, MAX_UP_JSON_LEN,
                    "{\"retry\":%lu,\"timeout\":2000,\"method\":\"event.%s\",\"params\":[%s]}",
                    retry, evnt->key, evnt->value);

            /* send json */
            ot_api_method(self->js, js_len, mum_send_event_ack, NULL);
            evnt_count++;
            LOG_INFO("up:%s\r\n", self->js);
        }

        list_del(&evnt->list);
        if(evnt->value)
            free(evnt->value);
        if(evnt)
            free(evnt);

        if(evnt_count > 0)
            break;
    }
    arch_os_mutex_put(self->js_buf_mutex);
    arch_os_mutex_put(self->evnt_mutex);

    return evnt_count;
}

int mum_send_event_with_retry(mum self, uint32_t retry)
{
    if (!self)
        return 0;

    if(NULL == otn_is_online())
        return 0;

    int js_len = 0;
    int evnt_count = 0;
    struct event_entry *evnt, *tmp;
    arch_os_mutex_get(self->evnt_mutex, OS_WAIT_FOREVER);
    arch_os_mutex_get(self->js_buf_mutex, OS_WAIT_FOREVER);
    list_for_each_entry_safe(evnt, tmp, &self->event_list, list)
    {
        if (arch_os_tick_elapsed(evnt->occur_tick) < EVENT_TIMEOUT_MS) {
            /* construct json */
            js_len = snprintf_safe(self->js, MAX_UP_JSON_LEN,
                    "{\"retry\":%lu,\"timeout\":2000,\"method\":\"event.%s\",\"params\":[%s]}",
                    retry, evnt->key, evnt->value);

            /* send json */
            ot_api_method(self->js, js_len, mum_send_event_ack, NULL);
            evnt_count++;
            LOG_INFO("up:%s\r\n", self->js);
        }

        list_del(&evnt->list);
        if(evnt->value)
            free(evnt->value);
        if(evnt)
            free(evnt);
    }
    arch_os_mutex_put(self->js_buf_mutex);
    arch_os_mutex_put(self->evnt_mutex);

    return evnt_count;
}

int mum_send_event(mum self)
{
    return mum_send_event_with_retry(self, 3);
}

int mum_send_event_ack(jsmn_node_t* pjn, void* ctx)
{
    // do nothing
    return 0;
}

int mum_send_common_data(mum self)
{
    if (!self)
        return 0;

    if(NULL == otn_is_online())
        return 0;

    int js_len = 0;
    int data_count = 0;
    struct common_entry *common_data, *tmp;
    arch_os_mutex_get(self->common_data_mutex, OS_WAIT_FOREVER);
    arch_os_mutex_get(self->js_buf_mutex, OS_WAIT_FOREVER);
    list_for_each_entry_safe(common_data, tmp, &self->common_list, list)
    {
        if (arch_os_tick_elapsed(common_data->occur_tick) > EVENT_TIMEOUT_MS) {
            list_del(&common_data->list);
            if(common_data->value)
                free(common_data->value);
            if(common_data)
                free(common_data);
            continue;
        }
        if (common_data->status == DIRTY) {
            /* construct json */
            js_len = snprintf_safe(self->js, MAX_UP_JSON_LEN, "{\"retry\":0,\"method\":\"%s\",\"params\":{\"%s\":%s}}", 
                common_data->method_name, common_data->key, common_data->value);
            common_data->status = SENDING;
            common_data->session = alloc_session_id(self);

            /* send json */
            struct up_ack_ctx *ctx = malloc(sizeof(struct up_ack_ctx));
            ctx->handle = self;
            ctx->session = common_data->session;
            ot_api_method(self->js, js_len, mum_send_common_data_ack, (void*) ctx);
            data_count++;
            LOG_INFO("up:%s\r\n", self->js);
        }
    }
    arch_os_mutex_put(self->js_buf_mutex);
    arch_os_mutex_put(self->common_data_mutex);

    return data_count;
}

int mum_send_common_data_ack(jsmn_node_t* pjn, void* ctx)
{
    struct up_ack_ctx * ack_ctx = ctx;
    mum self = ack_ctx->handle;
    uint32_t session = ack_ctx->session;
    int status = (pjn && pjn->js && NULL == pjn->tkn) ? DIRTY : SENT;
    
    if (ctx)
        free(ctx);

    if (!self)
        return 0;

    struct common_entry *common_data, *tmp;
    arch_os_mutex_get(self->common_data_mutex, OS_WAIT_FOREVER);
    list_for_each_entry_safe(common_data, tmp, &self->common_list, list)
    {
        if (common_data->status == SENDING && common_data->session == session) {
            common_data->status = status;
            if (common_data->status == SENT) {
                list_del(&common_data->list);
                if(common_data->value)
                    free(common_data->value);
                if(common_data)
                    free(common_data);
            }
        }
    }
    arch_os_mutex_put(self->common_data_mutex);

    return 0;
}



#ifdef MIIO_COMMANDS
extern mum mcmd_mum;
extern int mcmd_enqueue_raw(const char *buf);


d0_event_ret_t mum_offline_notifier(struct d0_event* evt, void* ctx)
{
        mcmd_enqueue_raw("MIIO_net_change local");
}

d0_event_ret_t mum_online_notifier(struct d0_event* evt, void* ctx)
{
        mcmd_enqueue_raw("MIIO_net_change cloud");
        if(mcmd_mum->up_sem)
            os_semaphore_put(&mcmd_mum->up_sem);
}



#endif
