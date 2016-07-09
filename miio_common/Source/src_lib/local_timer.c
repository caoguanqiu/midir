/*************************************************************************
	> File Name: local_timer.c
	> Author:
	> Mail:
	> Created Time: Wed 17 Feb 2016 08:13:17 PM CST
 ************************************************************************/
#include <lwip/lwipopts.h>
#include <ot/d0_otd.h>
#include <lib/jsmn/jsmn_api.h>
#include <appln_dbg.h>
#include <lib/miio_up_manager.h>
#include <psm-v2.h>
#include <lib/miio_command.h>
#include <lib/local_timer.h>
#define PRINTF_DEBUG wmprintf

#define MAX_JSON_LENGTH 960 
#define MAX_KEY_LEN 32
#define MAX_VALUE_LEN 960 

#define MAX_SCHEDULED_TASK_NUM 20
#define FAIL_TIME_OF_ONCE_TIMER  5//sec

extern psm_hnd_t app_psm_hnd;
extern mum g_mum;

static scheduled_task_p_t  schedule[MAX_SCHEDULED_TASK_NUM];
static os_mutex_t g_timer_proc_mutex;

//example:
//{"id":"N10373478","ty":0,"cr":"42 0 * * 0,1,2,3,4,5,6","co":"set_power","pa":["on"],"da":8,"pr":1456314171,"ex":0,"re":0}
int parse_schedule(const char* value_str, scheduled_task_p_t schedule_task_p)
{
    int ret = STATE_OK;
    jsmntok_t tk_buf[32];

    // parsing json string
    {
        jsmn_parser psr;
        jsmn_init(&psr);
        if(JSMN_SUCCESS != (ret = jsmn_parse(&psr, value_str, strlen(value_str), tk_buf, NELEMENTS(tk_buf))))
        {
            LOG_ERROR("json parse err(%d).\r\n", ret);
            return ret;
        }
        tk_buf[psr.toknext].type= JSMN_INVALID;
    }

    //id_str
    size_t id_str_len = 0;
    jsmn_key2val_str(value_str, tk_buf, "id", schedule_task_p->id_str, sizeof(schedule_task_p->id_str), &id_str_len);
    //type
    jsmn_key2val_u8(value_str, tk_buf, "ty", (u8_t*)&schedule_task_p->type);

    //crontab or posix
    if(schedule_task_p->type == CRONTAB)
    {
        size_t crontab_len = 0;
        jsmn_key2val_str(value_str, tk_buf, "cr", schedule_task_p->crontab, sizeof(schedule_task_p->crontab), &crontab_len);
    }
    else if(schedule_task_p->type == ONCE_TIME)
    {
        jsmn_key2val_uint(value_str, tk_buf, "po", (u32_t*)&schedule_task_p->posix);
    }

    //commands
    jsmntok_t* command_tkn = NULL;
    command_tkn = jsmn_key_value(value_str,tk_buf,"co");
    size_t command_str_len = command_tkn->end - command_tkn->start + 1;

    char* command_str = (char*)malloc(command_str_len);
    if(command_str == NULL
            || STATE_OK != jsmn_key2val_str(value_str, tk_buf, "co", command_str, command_str_len, NULL))
    {
        if(command_str)
            free(command_str);
        command_str = NULL;
        LOG_ERROR("parse x.timer commands error\r\n");
        return STATE_ERROR;
    };
    schedule_task_p->command_str = command_str;

    //params
    jsmntok_t* params_tkn = NULL;
    params_tkn = jsmn_key_value(value_str,tk_buf,"pa");
    size_t params_str_len = (params_tkn->end -1) - (params_tkn->start + 1) + 1;

    char* params_str = (char*)malloc(params_str_len);
    if(params_str == NULL)
    {
        if(params_str)
            free(params_str);
        params_str = NULL;
        LOG_ERROR("parse x.timer params error\r\n");
        return STATE_ERROR;
    };
    snprintf_safe(params_str,params_str_len,"%s",value_str + params_tkn->start + 1);
    schedule_task_p->params_str = params_str;

    //day_processed
    jsmn_key2val_u8(value_str, tk_buf, "da", (u8_t*)&schedule_task_p->day_processed);
    //processed_time
    jsmn_key2val_uint(value_str, tk_buf, "pr", (u32_t*)&schedule_task_p->processed_time);
    //exe_state
    jsmn_key2val_u8(value_str, tk_buf, "ex", (u8_t*)&schedule_task_p->exe_state);
    //report_state
    jsmn_key2val_u8(value_str, tk_buf, "re", (u8_t*)&schedule_task_p->report_state);

    return ret;
}
void load_timer_schedule(void)
{
    int i = 0;
    for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
    {
//read psm
        char key_str[MAX_KEY_LEN] = {0};
        snprintf_safe(key_str, MAX_KEY_LEN, "x.timer.%d", i);
        char value_str[MAX_VALUE_LEN] = {0};

        int ret = psm_get_variable(app_psm_hnd,key_str, value_str, MAX_VALUE_LEN);
        if(ret <= 0)
        {
            LOG_INFO("read %s from psm failed\r\n",key_str);
        }
        else
        {
            LOG_INFO("read %s from psm %d bytes\r\n",key_str, ret);
            PRINTF_DEBUG("==>%s\r\n",value_str);
            scheduled_task_p_t schedule_task_p =  (scheduled_task_p_t)malloc(sizeof(scheduled_task_t));
            if(schedule_task_p == NULL)
            {
                LOG_ERROR("malloc a new timer space error\r\n");
                break;
            }
            memset(schedule_task_p, 0, sizeof(scheduled_task_t));//init

            parse_schedule(value_str,schedule_task_p);

            os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);
            schedule[i] = schedule_task_p;
            os_mutex_put(&g_timer_proc_mutex);
        }
    }
}

//new or update a timer_schedule to psm
int set_timer_schedule(uint8_t index)
{
    int ret = STATE_OK;
    if(index >= MAX_SCHEDULED_TASK_NUM)
    {
        LOG_INFO("Schedule index is overflow, can't set timer schedule!\r\n");
        return OT_ERR_CODE_TOO_MANY_TIMER;
    }

//construct key_str & value_str
    char key_str[MAX_KEY_LEN] = {0};
    snprintf_safe(key_str, MAX_KEY_LEN, "x.timer.%d", index);

    char value_str[MAX_VALUE_LEN] = {0};
    int value_len = 0;

    //id_str
    value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"{\"id\":\"%s\",",schedule[index]->id_str);
    //type
    value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"ty\":%d,",schedule[index]->type);
    //crontab_str or posix
    if(schedule[index]->type == CRONTAB)
        value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"cr\":\"%s\",",schedule[index]->crontab);
    else if(schedule[index]->type == ONCE_TIME)
        value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"po\":%u,",(unsigned int)schedule[index]->posix);
    //command_str
    value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"co\":\"%s\",",schedule[index]->command_str);
    //params_str
    value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"pa\":[%s],",schedule[index]->params_str);
    //day_processed
    value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"da\":%d,",schedule[index]->day_processed);
    //processed_time
    value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"pr\":%lu,",schedule[index]->processed_time);
    //exe_state
    value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"ex\":%d,",schedule[index]->exe_state);
    //report_state
    value_len += snprintf_safe(value_str + value_len, MAX_VALUE_LEN - value_len,"\"re\":%d}",schedule[index]->report_state);

// save a schedule into psm
    ret = psm_set_variable(app_psm_hnd, key_str, value_str, value_len);
    if(ret != STATE_OK)
        ret = OT_ERR_CODE_WRITE_PSM;

    return ret;
}

int rm_timer_schedule(uint8_t index)
{
    int ret = STATE_OK;

//delete in memory
    if(schedule[index])
    {
        if(schedule[index]->command_str)
            free(schedule[index]->command_str);
        schedule[index]->command_str = NULL;

        if(schedule[index]->params_str)
            free(schedule[index]->params_str);
        schedule[index]->params_str = NULL;

        free(schedule[index]);
        schedule[index] = NULL;
    }

//delete in psm
    char key_str[MAX_KEY_LEN] = {0};
    snprintf_safe(key_str, MAX_KEY_LEN, "x.timer.%d", index);
    ret = psm_object_delete(app_psm_hnd,key_str);
    if(ret != STATE_OK)
    {
        LOG_INFO("Error erase schedule in PSM\r\n");
        return OT_ERR_CODE_ERASE_PSM;
    }
    return ret;
}

int rm_all_timer_schedule(void)
{
    int ret = STATE_OK;

    int i;
    for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
    {
        os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);
        if(schedule[i] != NULL)//
        {
            if(OT_ERR_CODE_ERASE_PSM == rm_timer_schedule(i))
            {
                LOG_INFO("erase x.timer.%s from psm failed\r\n", schedule[i]->id_str);
                ret = OT_ERR_CODE_ERASE_PSM;
            }
        }
        os_mutex_put(&g_timer_proc_mutex);
    }
    return ret;
}

void  display_all_timer_schedule(void)
{
    int i;
    for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
    {
        os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);
        if(schedule[i] != NULL)
        {
            LOG_INFO("[x.timer.%d]\"x.timer.%s\" = ", i,schedule[i]->id_str);
            if(schedule[i]->type == CRONTAB)
                PRINTF_DEBUG("[\"%s\",",schedule[i]->crontab);
            else
                PRINTF_DEBUG("[%d,",schedule[i]->posix);
            PRINTF_DEBUG("[\"%s\"",schedule[i]->command_str);
            if(strlen(schedule[i]->params_str) != 0)
                PRINTF_DEBUG(",[%s]",schedule[i]->params_str);
            PRINTF_DEBUG("]]\r\n");
        }
        os_mutex_put(&g_timer_proc_mutex);
    }
}

//parse a crontab timer,get hour,minute,and which days should exe the timer
//"02 17 * * 0,1,2,3,4,5,6"==>wdays = 0x7F,hour = 17,minute = 02
void  crontab_parse(char *crontab_str , uint8_t *hour, uint8_t *minute, uint8_t *wdays)
{
    int i;

    char *p = NULL,*p1 = NULL,*p2 = NULL,*p3 = NULL,*p4 = NULL;

    char hour_str[5];
    char minute_str[5];

    memset(hour_str,0,sizeof(hour_str));
    memset(minute_str,0,sizeof(minute_str));

    p = crontab_str;
    p1 = strchr(crontab_str, ' ');
    p2 = strstr(crontab_str, " * * ");
    p3 = strchr(crontab_str, '-');
    p4 = strchr(crontab_str, ',');

    if((p != NULL)&&(p1 != NULL))
    {
        memcpy(minute_str,p,p1-p);
        *minute = atoi(minute_str);
    }
    else
    {
        return ;
    }

    if(p2 != NULL)
    {
        memcpy(hour_str,p1,p2-p1);
        *hour = atoi(hour_str);
    }
    else
    {
        return ;
    }

    if(p3 != NULL)//0-6
    {
        for(i=(*(p3-1)-'0'); i<=(*(p3+1)-'0'); i++)
        {
            *wdays |= (1<<i);
        }
    }
    else if(p4 != NULL)//0,1,2,3,4,5,6
    {
        if(*(p4+2) == '\0')
        {
            *wdays |= (1<<(*(p4-1)-'0'));
            *wdays |= (1<<(*(p4+1)-'0'));
        }
        else
        {
            *wdays |= (1<<(*(p4-1)-'0'));
            while(*(p4+1) != '\0')
            {
                if(*p4 == ',')
                {
                    *wdays |= (1<<(*(p4+1)-'0'));
                }
                p4++;
            }
        }
    }
    else if(*(p2+5) != '\0')//only one day ,eg.3
    {
        *wdays |= (1<<(*(p2+5)-'0'));
    }
}

void timer_executed_event_report(uint8_t index)
{
    int js_len = 0;
    char js_ret[MAX_JSON_LENGTH] = {0}; 

    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "\"x.timer.%s\",", schedule[index]->id_str);
    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "\"%s\",", (schedule[index]->exe_state?"success":"fail"));
    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "%ld", schedule[index]->processed_time);

    mum_set_event(g_mum, "timer_executed", js_ret);
}

void timer_executed_log_report(uint8_t index)
{
    int js_len = 0;
    char js_ret[MAX_JSON_LENGTH] = {0}; 

    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "{\"timer_info\":[\"x.timer.%s\",[", schedule[index]->id_str);
    if(schedule[index]->type == CRONTAB)
        js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "\"%s\",", schedule[index]->crontab);
    else
        js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "\"%d\",", (int)schedule[index]->posix);

    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "[\"%s\",", schedule[index]->command_str);
    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "%s]]],", schedule[index]->params_str);

    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "\"exe_state\":\"%s\",", (schedule[index]->exe_state?"success":"fail"));
    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "\"exe_result\":\"%s\",", (schedule[index]->exe_state?"success":"fail"));
    js_len += snprintf_safe(js_ret + js_len, sizeof(js_ret) - js_len, "\"exe_time\":\"%ld\"}", schedule[index]->processed_time);

    mum_set_log(g_mum,"timer_executed",js_ret);
}

int execute_timer(uint8_t index)
{
    char miio_commands_str[MAX_VALUE_LEN];
    snprintf_safe(miio_commands_str,MAX_VALUE_LEN,"%s %s",schedule[index]->command_str,schedule[index]->params_str);
    LOG_INFO("MIIO_COMMANDS:%s\r\n",miio_commands_str);
#ifdef MIIO_COMMANDS
    mcmd_enqueue_raw(miio_commands_str);
#endif
    return TIMER_EXE_SUCCESS;
}

void crontab_proc_timer(uint8_t index, uint8_t hour, uint8_t minute, uint8_t wdays)
{
    time_t curtime = arch_os_utc_now() + 8 * 60 * 60;
    struct tm date_time;
    gmtime_r((const time_t *) &curtime, &date_time);

    uint8_t  day_val;//weekday,0~6,evey bit means one day
    day_val = 1 << date_time.tm_wday;//today's bit is 1

    if((wdays & day_val) == day_val)//today is the day
    {
        if((schedule[index]->day_processed & day_val) == 0)//have not processed
        {
            if(date_time.tm_hour < hour
                    || (date_time.tm_hour == hour && date_time.tm_min < minute))
            {
                return;//time has not come, do nothing
            }
            else if((date_time.tm_hour == hour) && (date_time.tm_min == minute))//time is the time
            {
                schedule[index]->exe_state = execute_timer(index);
            }
            else if((date_time.tm_hour > hour)
                    ||(date_time.tm_hour == hour && date_time.tm_min > minute))//time has passed
            {
                schedule[index]->exe_state = TIMER_EXE_FAILED;
            }

            schedule[index]->day_processed |= day_val;//today has processed
            schedule[index]->processed_time = curtime;
            if(otn_is_online())//
            {
                timer_executed_event_report(index);
                timer_executed_log_report(index);
                schedule[index]->report_state = REPORTED;
            }
            set_timer_schedule(index);
        }
        else if(schedule[index]->report_state == UNREPORTED && otn_is_online())//have processed,but havn't reported
        {
            timer_executed_event_report(index);
            timer_executed_log_report(index);
            schedule[index]->report_state = REPORTED;
            set_timer_schedule(index);
        }
    }
    else //today is not the day
    {
        if(schedule[index]->day_processed != 0
                || schedule[index]->report_state != UNREPORTED)//avoid to write psm repeatly
        {
            schedule[index]->day_processed = 0;//all weekdays' processed_flag clear
            schedule[index]->report_state = UNREPORTED;
            schedule[index]->processed_time = 0;
            set_timer_schedule(index);
        }
    }
}

void once_time_proc_timer(uint8_t index)
{
    time_t curtime = arch_os_utc_now();
    if(curtime < schedule[index]->posix)//time has not come, do nothing
        return;

    if(schedule[index]->day_processed == 0)//havn't processed
    {
        if(curtime - schedule[index]->posix < FAIL_TIME_OF_ONCE_TIMER)//time has come, and in FAIL_TIME
            schedule[index]->exe_state = execute_timer(index);
        else //time has come,but out of FAIL_TIME
            schedule[index]->exe_state = TIMER_EXE_FAILED;

        schedule[index]->processed_time = curtime;
        schedule[index]->day_processed = 1;//has processed
        set_timer_schedule(index);
    }

    if(otn_is_online())
    {
        timer_executed_event_report(index);
        timer_executed_log_report(index);
        rm_timer_schedule(index);
    }
}

void check_schedule_and_set_timer()
{
//    display_all_timer_schedule();//for debug

    int i;
    for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
    {
        uint8_t hour,minute;
        uint8_t wdays = 0;//0~6,every bit means one day

        os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);
        if(schedule[i] != NULL)
        {
            if(schedule[i]->type == CRONTAB)
            {
                crontab_parse(schedule[i]->crontab, &hour, &minute, &wdays);//TODO:what if the format is wrong
                crontab_proc_timer(i,hour, minute, wdays);
            }
            else if(schedule[i]->type == ONCE_TIME)
            {
                once_time_proc_timer(i);
            }
        }
        os_mutex_put(&g_timer_proc_mutex);
    }
}

void local_timer_init(void)
{
#if 0
    //*********for test***************************//
    mum_set_property(g_mum,"power","\"on\"");
    mum_set_property(g_mum,"temperature","\"on\"");
    mum_set_property(g_mum,"wifi_led","\"on\"");
    mum_set_property(g_mum,"enable_recovery","\"on\"");
    //********************************************//
#endif

    if (WM_SUCCESS != os_mutex_create(&g_timer_proc_mutex, "g_timer_proc_mutex",OS_MUTEX_INHERIT))
    {
        LOG_ERROR("g_timer_proc_mutex creation failed\r\n");
    }

    os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);
    int i = 0;
    for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
        schedule[i] = NULL;
    os_mutex_put(&g_timer_proc_mutex);

    load_timer_schedule();

    if(!is_provisioned())
        rm_all_timer_schedule();
}

void local_timer_deinit(void)
{
    os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);
    int i = 0;
    for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
    {
        if(schedule[i])
        {
            if(schedule[i]->command_str)
                free(schedule[i]->command_str);
            schedule[i]->command_str = NULL;

            if(schedule[i]->params_str)
                free(schedule[i]->params_str);
            schedule[i]->params_str = NULL;

            free(schedule[i]);
            schedule[i] = NULL;
        }
    }
    os_mutex_put(&g_timer_proc_mutex);

    os_mutex_delete(&g_timer_proc_mutex);
}
//****************************local_timer about end****************************************/

// {"method":"miIO.xset","params":[["x.timer.1",["01 17 * * 0-6",["set_power","on"]]],["x.timer.2",["02 17 * * 0,1,2,3,4,5,6",["set_power","off"]]]]}
// or
// {"method":"miIO.xset","params":[["x.timer.3",[1442575980,["set_power","on"]]],["x.timer.4",[1442576040,["set_power","off"]]]]}
int find_the_schedule_index(const char* id_str)//find the exist index
{
    int i = 0;
    for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
    {
        if(schedule[i] != NULL
                && 0 == strncmp(id_str, schedule[i]->id_str,strlen(id_str)))
            return i;
    }
    return  i;
}

int get_a_schedule_index(const char* id_str)//find the exist index or get a new unused index
{
    bool find_unused_index = false;
    int first_unused_index = MAX_SCHEDULED_TASK_NUM;

    int i = 0;
    for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
    {
        if(schedule[i] != NULL
                && 0 == strncmp(id_str, schedule[i]->id_str,strlen(id_str)))
            return i;
        else if(!find_unused_index && schedule[i]  == NULL)
        {
            find_unused_index = true;
            first_unused_index = i;
        }
    }

    return first_unused_index;
}


int JSON_DELEGATE(xset)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    int ret = STATE_OK;

    int array_idx, param_idx, param2_idx, param3_idx;
    jsmntok_t* array_tkn, *param_tkn, *param2_tkn, *param3_tkn;

    scheduled_task_p_t schedule_task_p = NULL;
    char* command_str = NULL;
    char* params_str = NULL;

    array_idx = 0;
    while (NULL != (array_tkn = jsmn_array_value(pjn->js, pjn->tkn, array_idx)))
    {
        //array_tkn point to one timer
        os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);

        //malloc a new timer space
        schedule_task_p =  (scheduled_task_p_t)malloc(sizeof(scheduled_task_t));
        if(schedule_task_p == NULL)
        {
            LOG_ERROR("malloc a new timer space error\r\n");
            goto malloc_error_return;
        }
        memset(schedule_task_p, 0, sizeof(scheduled_task_t));//init

        int index = -1;
        param_idx = 0;
        while (NULL != (param_tkn = jsmn_array_value(pjn->js, array_tkn, param_idx)))
        {
            char key_str[MAX_KEY_LEN] = "";
            size_t key_str_len = 0;
            switch(param_idx)
            {
            case 0://timer_id
                jsmn_tkn2val_str(pjn->js, param_tkn, key_str, sizeof(key_str), &key_str_len);
                if(0 == strncmp(key_str,"x.timer.",8))//
                {
                    char* id_str = key_str + 8;
                    if(MAX_SCHEDULED_TASK_NUM == (index = get_a_schedule_index(id_str)))
                    {
                        if(schedule_task_p)
                        {
                            free(schedule_task_p);
                            schedule_task_p = NULL;
                        }
                        goto too_many_timer_error_return;
                    }
                    LOG_INFO("[x.timer.%d]\"x.timer.%s\" = ",index, id_str);
                    strncpy(schedule_task_p->id_str, id_str,sizeof(schedule_task_p->id_str));//
                }
                break;
            case 1://timer content
                param2_idx = 0;
                while (NULL != (param2_tkn = jsmn_array_value(pjn->js, param_tkn, param2_idx)))
                {
                    switch(param2_idx)
                    {
                    case 0://timer type
                        if(param2_tkn->type == JSMN_STRING)
                        {
                            schedule_task_p->type = CRONTAB;
                            size_t param_crontab_len = 0;
                            jsmn_tkn2val_str(pjn->js, param2_tkn, schedule_task_p->crontab, sizeof(schedule_task_p->crontab), &param_crontab_len);
                            PRINTF_DEBUG("[%s,",schedule_task_p->crontab);
                        }
                        else if(param2_tkn->type == JSMN_PRIMITIVE)
                        {
                            schedule_task_p->type = ONCE_TIME;
                            jsmn_tkn2val_uint(pjn->js, param2_tkn, (u32_t*)(&schedule_task_p->posix));
                            PRINTF_DEBUG("[%d,",schedule_task_p->posix);
                        }
                        break;
                    case 1://command & its params
                        param3_idx = 0;
                        while (NULL != (param3_tkn = jsmn_array_value(pjn->js, param2_tkn, param3_idx)))
                        {
                            if(param3_idx == 0) //command
                            {
                                size_t command_str_len = param3_tkn->end - param3_tkn->start + 1;
                                //LOG_INFO("command_str_len = %d\r\n",command_str_len);

                                command_str = (char*)malloc(command_str_len);
                                if(command_str == NULL
                                        || STATE_OK != jsmn_tkn2val_str(pjn->js, param3_tkn, command_str, command_str_len, NULL))
                                {
                                    if(command_str)
                                        free(command_str);
                                    command_str = NULL;
                                    LOG_ERROR("malloc space for x.timer error.\r\n");
                                    goto malloc_error_return;
                                }
                                schedule_task_p->command_str = command_str;
                                PRINTF_DEBUG("[\"%s\"",schedule_task_p->command_str);
                            }
                            else//params of commands
                            {
                                //length of all params
                                size_t params_str_len = param2_tkn->end - param3_tkn->start + 1;
                                //LOG_INFO("params_str_len = %d\r\n",params_str_len);
                                params_str = (char*)malloc(params_str_len);
                                if(params_str == NULL)
                                {
                                    if(params_str)
                                        free(params_str);
                                    params_str = NULL;
                                    LOG_ERROR("malloc space for x.timer error.\r\n");
                                    goto malloc_error_return;
                                }

                                if(param3_tkn->type == JSMN_STRING)//if first param is a string
                                    snprintf_safe(params_str,params_str_len, "\"%s",pjn->js + param3_tkn->start);
                                else
                                    snprintf_safe(params_str,params_str_len - 1, "%s",pjn->js + param3_tkn->start);

                                schedule_task_p->params_str = params_str;
                                PRINTF_DEBUG(",%s",schedule_task_p->params_str);
                                break;//all params has been parsed
                            }
                            param3_idx++;
                        }
                        PRINTF_DEBUG("]]\r\n");
                        break;
                    default:
                        break;
                    }
                    param2_idx++;
                }
                break;
            default:
                break;
            }
            param_idx++;
        }

        //set the current timer to psm
        schedule[index] = schedule_task_p;
        ret = set_timer_schedule(index);
        if(ret == OT_ERR_CODE_TOO_MANY_TIMER)
            goto too_many_timer_error_return;
        else if(ret == OT_ERR_CODE_WRITE_PSM)
            goto set_psm_error_return;

        os_mutex_put(&g_timer_proc_mutex);
        array_idx++;
    }

    char js[120]  = "";
    jsmn_node_t jn;
    jn.js = js;
    jn.tkn = NULL;
    jn.js_len = snprintf_safe(js, sizeof(js), "{\"result\":[\"ok\"]}");

    ack(&jn, ctx);
    return STATE_OK;

too_many_timer_error_return:
    os_mutex_put(&g_timer_proc_mutex);
    if (ack)
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_TOO_MANY_TIMER, OT_ERR_INFO_TOO_MANY_TIMER);//TODO:MESSAGE
    return STATE_ERROR;

set_psm_error_return:
    os_mutex_put(&g_timer_proc_mutex);
    if (ack)
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_WRITE_PSM, OT_ERR_INFO_WRITE_PSM);
    return STATE_ERROR;

malloc_error_return:
    os_mutex_put(&g_timer_proc_mutex);
    if(schedule_task_p){
        free(schedule_task_p);
        schedule_task_p = NULL;
    }
    if(command_str){
        free(command_str);
        command_str = NULL;
    }
    if(params_str){
        free(params_str);
        params_str = NULL;
    }

    if (ack)
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_WRITE_PSM, OT_ERR_INFO_WRITE_PSM);
    return STATE_ERROR;
}
FSYM_EXPORT(JSON_DELEGATE(xset));

int JSON_DELEGATE(xdel)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    int ret = STATE_OK;
    int array_idx = 0;
    jsmntok_t *array_tkn = NULL;
    while (NULL != (array_tkn = jsmn_array_value(pjn->js, pjn->tkn, array_idx)))
    {
        char key_str[MAX_KEY_LEN] = "";
        size_t key_str_len = 0;
        jsmn_tkn2val_str(pjn->js, array_tkn, key_str, sizeof(key_str), &key_str_len);
        if(0 == strncmp(key_str,"x.timer.",8))
        {
            if(0 == strncmp(key_str,"x.timer.*",9))//rm all
            {
                ret = rm_all_timer_schedule();
                if(ret == OT_ERR_CODE_ERASE_PSM)
                    goto erase_psm_error_return;
            }
            else
            {
                os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);//
                char* id_str = key_str + 8;
                int index = -1;
                if(MAX_SCHEDULED_TASK_NUM != (index = find_the_schedule_index(id_str)))
                {
                    LOG_INFO("delete [x.timer.%d]x.timer.%s\r\n",index,schedule[index]->id_str);
                    if(OT_ERR_CODE_ERASE_PSM == (ret = rm_timer_schedule(index)))
                    {
                        os_mutex_put(&g_timer_proc_mutex);
                        goto erase_psm_error_return;
                    }
                }
                os_mutex_put(&g_timer_proc_mutex);
            }
        }
        memset(key_str, 0, sizeof(key_str));
        array_idx++;
    }

    jsmn_node_t jn;
    char js[120] = "";
    jn.js = js;
    jn.js_len = snprintf_safe(js, sizeof(js), "{\"result\":[\"ok\"]}");
    jn.tkn = NULL;

    ack(&jn, ctx);
    return STATE_OK;

erase_psm_error_return:
    if (ack)
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_ERASE_PSM, OT_ERR_INFO_ERASE_PSM);
    return STATE_ERROR;
}
FSYM_EXPORT(JSON_DELEGATE(xdel));

int JSON_DELEGATE(xget)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    jsmn_node_t jn;
    char js_ret[MAX_JSON_LENGTH] = "";

    jn.js = js_ret;
    jn.tkn = NULL;
    jn.js_len = 0;
    jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "{\"result\":{\"list\":[");

    bool find_flag = false;
    int array_idx = 0;
    jsmntok_t *array_tkn = NULL;
    while (NULL != (array_tkn = jsmn_array_value(pjn->js, pjn->tkn, array_idx)))
    {
        os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);

        char key_str[MAX_KEY_LEN] = "";
        size_t key_str_len = 0;
        jsmn_tkn2val_str(pjn->js, array_tkn, key_str, sizeof(key_str), &key_str_len);

        if(0 == strncmp(key_str,"x.timer.",8))
        {
            if(0 == strncmp(key_str,"x.timer.*",9))   //get all
            {
                int i = 0;
                for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
                {
                    if(schedule[i] != NULL)
                    {
                        jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "[\"x.timer.%s\",", schedule[i]->id_str);
                        if(schedule[i]->type == CRONTAB)
                            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "[\"%s\",", schedule[i]->crontab);
                        else
                            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "[%d,", (int)schedule[i]->posix);
                        jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "[\"%s\"",schedule[i]->command_str);
                        if(strlen(schedule[i]->params_str) != 0)
                            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, ",%s",schedule[i]->params_str);
                        jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "]]],");
                        find_flag = true;
                    }
                }
            }
            else
            {
                char* id_str = key_str + 8;
                int index = -1;
                if(MAX_SCHEDULED_TASK_NUM !=  (index = find_the_schedule_index(id_str)))
                {
                    if(schedule[index] != NULL)
                    {
                        jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "[\"x.timer.%s\",", schedule[index]->id_str);
                        if(schedule[index]->type == CRONTAB)
                            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "[\"%s\",", schedule[index]->crontab);
                        else
                            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "[%d,", (int)schedule[index]->posix);
                        jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "[\"%s\"", schedule[index]->command_str);
                        if(strlen(schedule[index]->params_str) != 0)
                            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, ",%s", schedule[index]->params_str);
                        jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "]]],");
                        find_flag = true;
                    }
                }
            }
            os_mutex_put(&g_timer_proc_mutex);
        }
        array_idx ++;
        memset(key_str, 0,sizeof(key_str));
    }

    if(find_flag == false) //none is exist
        jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "]}}");
    else
        jn.js_len += snprintf_safe(js_ret + jn.js_len - 1, sizeof(js_ret) - jn.js_len, "]}}");

    LOG_INFO("get_timer result:%s\r\n", js_ret);
    return ack(&jn, ctx);
}
FSYM_EXPORT(JSON_DELEGATE(xget));

int JSON_DELEGATE(xgetKeys)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    jsmn_node_t jn;
    char js_ret[MAX_JSON_LENGTH] = "";
    jn.js = js_ret;
    jn.tkn = NULL;
    jn.js_len = 0;
    jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "{\"result\":[");

    int array_idx = 0;
    jsmntok_t* array_tkn = NULL;
    while (NULL != (array_tkn = jsmn_array_value(pjn->js, pjn->tkn, array_idx)))
    {
        os_mutex_get(&g_timer_proc_mutex, OS_WAIT_FOREVER);
        char key_str[MAX_KEY_LEN] = "";
        jsmn_tkn2val_str(pjn->js, array_tkn, key_str, sizeof(key_str), NULL);
        if(0 == strncmp(key_str,"x.timer",7))
        {
            int i = 0;
            for(i = 0; i < MAX_SCHEDULED_TASK_NUM; i++)
            {
                if(schedule[i] != NULL)
                {
                    if(i > 0)
                        jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, ",");
                    jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "\"x.timer.%s\"", schedule[i]->id_str);
                }
            }
        }
        os_mutex_put(&g_timer_proc_mutex);
        memset(key_str,0,sizeof(key_str));
        array_idx ++;
    }

    jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "]}");
    LOG_INFO("miIO.xgetkeys result:%s\r\n", js_ret);
    return ack(&jn, ctx);
}
FSYM_EXPORT(JSON_DELEGATE(xgetKeys));
#if 0
//**********************for test*************************************************//
int JSON_DELEGATE(set_power)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    jsmn_node_t jn;
    char js[120] = "";

    jn.js = js;
    jn.tkn = NULL;
    jn.js_len = snprintf_safe(js, sizeof(js), "{\"result\":[\"ok\"]}");

    LOG_INFO("ret: %s\r\n",jn.js);
    return ack(&jn, ctx);
}
USER_FSYM_EXPORT(JSON_DELEGATE(set_power));

int JSON_DELEGATE(get_prop)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    jsmn_node_t jn;
    char js_ret[120] = "";
    int param_idx = 0;
    char param_key[16];
    char param_value[16];
    size_t param_str_len;
    jsmntok_t *tkn = NULL;

    jn.js = js_ret;
    jn.tkn = NULL;
    jn.js_len = 0;

    jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "{\"result\":[");

    while (NULL != (tkn = jsmn_array_value(pjn->js, pjn->tkn, param_idx)))
    {
        int ret;

        jsmn_tkn2val_str(pjn->js, tkn, param_key, sizeof(param_key), &param_str_len);

        if (param_idx > 0)
            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, ",");

        ret = mum_get_property(g_mum, param_key, param_value);

        if(MUM_OK == ret)
        {
            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "%s", param_value);
        }
        else if(-MUM_ITEM_NONEXIST == ret)
        {
            jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "null");
        }

        param_idx++;
    }

    jn.js_len += snprintf_safe(js_ret + jn.js_len, sizeof(js_ret) - jn.js_len, "]}");

    LOG_INFO("get_prop result:%s\r\n", js_ret);
    return ack(&jn, ctx);
}
USER_FSYM_EXPORT(JSON_DELEGATE(get_prop));
//**********************for test end*************************************************//
#endif
