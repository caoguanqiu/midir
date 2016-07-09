#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <wm_os.h>
#include <call_on_worker_thread.h>
#include <appln_dbg.h>
#include <wmlist.h>


typedef int (*general_invoke)();

typedef struct {
    general_invoke invoke;
}runtime_type_id;

typedef int (*cross_thread_task1)(void *para1);
typedef int (*cross_thread_task2)(void *para1,void *para2);
typedef int (*cross_thread_task3)(void *para1,void *para2,void* para3);
typedef int (*cross_thread_task4)(void *para1,void *para2,void* para3,void *para4);
typedef int (*cross_thread_task5)(void *para1,void *para2,void* para3,void *para4,void *para5);

typedef struct {
    runtime_type_id m_id;
    general_cross_thread_task task;
    task_result call_back;
}general_cross_thread_type;

typedef struct{
    runtime_type_id m_id;
    cross_thread_task1 task;
    task_result call_back;
    void *para1;
}cross_thread_type1;

typedef struct{
    runtime_type_id m_id;
    cross_thread_task2 task;
    task_result call_back;
    void *para1;
    void *para2;
}cross_thread_type2;

typedef struct{
    runtime_type_id m_id;
    cross_thread_task3 task;
    task_result call_back;
    void *para1;
    void *para2;
    void *para3;
}cross_thread_type3;

typedef struct{
    runtime_type_id m_id;
    cross_thread_task4 task;
    task_result call_back;
    void *para1;
    void *para2;
    void *para3;
    void *para4;
}cross_thread_type4;

/***************************************
* we may only use type5, which may waste
* memory, but reduce pointer cast operation,
* & reduce memory fragement
****************************************/
typedef struct{
    runtime_type_id m_id;
    cross_thread_task5 task;
    task_result call_back;
    void *para1;
    void *para2;
    void *para3;
    void *para4;
    void *para5;
}cross_thread_type5;



static int general_run(general_cross_thread_task task)
{
    if(!task) {
        return -100;
    }

   return (*(task))();
}

static int run1(general_cross_thread_task task,void * para1)
{   
    if(!task) {
        return -100;
    }

    return (*((cross_thread_task1)task))(para1);
}

static int run2(general_cross_thread_task task,void * para1,void *para2)
{
    if(!task) {
        return -100;
    }

    return (*((cross_thread_task2)task))(para1,para2);
}

static int run3(general_cross_thread_task task,void * para1,void *para2,void *para3)
{
    if(!task) {
        return -100;
    }

    return (*((cross_thread_task3)task))(para1,para2,para3);
}

static int run4(general_cross_thread_task task,void * para1,void *para2,void *para3,void *para4)
{
    if(!task) {
        return -100;
    }

    return (*((cross_thread_task4)task))(para1,para2,para3,para4);
}

static int run5(general_cross_thread_task task,void * para1,void *para2,void *para3,void *para4,void *para5)
{
    if(!task) {
        return -100;
    }

    return (*((cross_thread_task5)task))(para1,para2,para3,para4,para5);
}

/****************************************************
* thread func
*****************************************************/
static void worker_thread_func(cross_thread_type5 * obj)
{
    if(!obj) {
        LOG_ERROR("para error in test \r\n");
        return;
    }

    int ret;

    if(obj->m_id.invoke == general_run) {
        ret = general_run(obj->task);
    } else if(obj->m_id.invoke == run1) {
        ret = run1(obj->task,obj->para1);
    } else if(obj->m_id.invoke == run2) {
        ret = run2(obj->task,obj->para1,obj->para2);
    } else if(obj->m_id.invoke == run3) {
        ret = run3(obj->task,obj->para1,obj->para2,obj->para3);
    } else if(obj->m_id.invoke == run4) {
        ret = run4(obj->task,obj->para1,obj->para2,obj->para3,obj->para4);
    } else if(obj->m_id.invoke == run5) {
        ret = run5(obj->task,obj->para1,obj->para2,obj->para3,obj->para4,obj->para5);
    } else {
        LOG_ERROR("un recongize type\r\n");
        ret = -100;
    }
    
    if(obj->call_back) {
        if(obj->m_id.invoke == general_run) {
            (*(obj->call_back))(ret,0);
        } else if(obj->m_id.invoke == run1) {
            (*(obj->call_back))(ret,1,obj->para1);
        } else if(obj->m_id.invoke == run2) {
            (*(obj->call_back))(ret,2,obj->para1,obj->para2);
        } else if(obj->m_id.invoke == run3) {
            (*(obj->call_back))(ret,3,obj->para1,obj->para2,obj->para3);
        } else if(obj->m_id.invoke == run4) {
            (*(obj->call_back))(ret,4,obj->para1,obj->para2,obj->para3,obj->para4);
        } else if(obj->m_id.invoke == run5) {
            (*(obj->call_back))(ret,5,obj->para1,obj->para2,obj->para3,obj->para4,obj->para5);
        } else {
            (*(obj->call_back))(ret,0);
        }
    }

    free(obj);
}


static int extract_param(cross_thread_type5 *obj,general_cross_thread_task func,task_result call_back,int para_count,va_list ap)
{
    int ret = WM_SUCCESS;

    if(para_count == 0) {

        obj->task = func;
        obj->call_back = call_back;
        obj->m_id.invoke = (general_invoke)&general_run;

    } else if(para_count == 1) {

        obj->m_id.invoke = (general_invoke)&run1;
        obj->task = func;
        obj->call_back = call_back;
        obj->para1 = va_arg(ap,void *);

    } else if(para_count == 2) {

        obj->m_id.invoke = (general_invoke)&run2;
        obj->task = func;
        obj->call_back = call_back;
        obj->para1 = va_arg(ap,void *);
        obj->para2 = va_arg(ap,void *);

    } else if(para_count == 3) {

        obj->m_id.invoke = (general_invoke)&run3;
        obj->task = func;
        obj->call_back = call_back;
        obj->para1 = va_arg(ap,void *);
        obj->para2 = va_arg(ap,void *);
        obj->para3 = va_arg(ap,void *);

    } else if(para_count == 4) {

        obj->m_id.invoke = (general_invoke)&run4;
        obj->task = func;
        obj->call_back = call_back;
        obj->para1 = va_arg(ap,void *);
        obj->para2 = va_arg(ap,void *);
        obj->para3 = va_arg(ap,void *);
        obj->para4 = va_arg(ap,void *);

    } else if(para_count == 5){
        
        obj->m_id.invoke = (general_invoke)&run5;
        obj->task = func;
        obj->para1 = va_arg(ap,void *);
        obj->para2 = va_arg(ap,void *);
        obj->para3 = va_arg(ap,void *);
        obj->para4 = va_arg(ap,void *);
        obj->para5 = va_arg(ap,void *);
    
    } else {
        ret = -1;
        wmprintf("para too much\r\n");
    }

    return ret;
}

struct {
    bool is_worker_thread_run;
} worker_thread_state = {
    .is_worker_thread_run = false,
};


static int post_task_to_worker(cross_thread_type5 * obj);
int call_on_worker_thread(general_cross_thread_task func,task_result call_back,int para_count, ...)
{
    int ret;

    if(!worker_thread_state.is_worker_thread_run) {
        return -1; 
    }

    if(!func) {
        return -1;
    }

    if((para_count < 0) || (para_count > 5)) {
        return -1; 
    }

    cross_thread_type5 *obj = (cross_thread_type5 *)malloc(sizeof(cross_thread_type5));
    if(!obj) {
        ret = -1;
        goto error_ret;
    }

    memset(obj,0,sizeof(cross_thread_type5));
    
    //extract every para
    va_list ap;
    va_start(ap,para_count);
    ret = extract_param(obj,func,call_back,para_count,ap);
    va_end(ap);

    if(WM_SUCCESS != ret) {
        free(obj);
        goto error_ret;
    }

    ret = post_task_to_worker(obj);
error_ret:
    return ret;
}


/**********************************************************
* worker thread impl
***********************************************************/
#define MAX_WORKER_TASK     (10)
static os_queue_t worker_thread_sched_queue;
static os_queue_pool_define(worker_queue_data, MAX_WORKER_TASK * sizeof(cross_thread_type5 *));

int init_worker_thread_queue(void)
{
    int ret = os_queue_create(&worker_thread_sched_queue, "worker_queue", sizeof(cross_thread_type5 *),&worker_queue_data); 
    if(ret) {
       LOG_ERROR("worker thread start fail");
    } 

    if(WM_SUCCESS == ret) {
        worker_thread_state.is_worker_thread_run = true;
    }

    return ret;
}


void worker_thread_loop(void *arg)
{
    int ret;

    cross_thread_type5 * msg = NULL;
     
    while(1) {
        ret = os_queue_recv(&worker_thread_sched_queue, &msg,OS_WAIT_FOREVER);
        if(ret) {
            LOG_ERROR("error message");
            continue;
        }
        
        worker_thread_func(msg);
    }

    os_queue_delete(&worker_thread_sched_queue);
}

static int post_task_to_worker(cross_thread_type5 * obj)
{
    int ret;

    ret = os_queue_send(&worker_thread_sched_queue, &obj, OS_NO_WAIT);
    if(WM_SUCCESS != ret) {
        LOG_ERROR("send task to worker queue error");
    }

    return ret;
}


/***************************************************************
* wait_time in milliseconds
****************************************************************/
struct delayed_task{
    list_head_t         list;
    os_timer_t          task_timer;
    cross_thread_type5 *task;
};

/**********************************************************
* init dealyed task list head
***********************************************************/
static LIST_HEAD(delayed_task_list);
static os_mutex_t delayed_task_mutex;

int init_delayed_task_mutex(void)
{
	return os_mutex_create(&delayed_task_mutex, "de_tk_mu", OS_MUTEX_INHERIT);
}

static void delayed_task_call_back(os_timer_arg_t handle)
{
    struct delayed_task * task = os_timer_get_context(&handle);
    if(!task) {
        return;
    }

    //delete item from list
    os_mutex_get(&delayed_task_mutex,OS_WAIT_FOREVER);
    
    if(task->task)
        post_task_to_worker(task->task);
    
    if(task->task_timer)
        os_timer_delete(&task->task_timer);

    list_head_t * iter;
    list_for_each(iter,&delayed_task_list) {
        if(&task->list == iter) {
            list_del(&task->list);
            break;
        }
    }
    os_mutex_put(&delayed_task_mutex);
    
    free(task);
}


int call_delayed_task_on_worker_thread(general_cross_thread_task func,task_result call_back,unsigned long wait_time,int para_count, ...)
{
    int ret = 0;
    
    if(!worker_thread_state.is_worker_thread_run) {
        return -1; 
    }

    if(!func) {
        return -1;
    }

    if((para_count < 0) || (para_count > 5)) {
        return -1; 
    }
    
    struct delayed_task * task = malloc(sizeof(struct delayed_task));
    if(!task) {
        return -1;
    }

    memset(task,0,sizeof(struct delayed_task));
    //init list
    INIT_LIST_HEAD(&task->list);

    cross_thread_type5 *obj = (cross_thread_type5 *)malloc(sizeof(cross_thread_type5));
    if(!obj) {
        goto delayed_task_error_handle1;
    }
    
    memset(obj,0,sizeof(cross_thread_type5));
    task->task = obj;
    
    //extract every para
    va_list ap;
    va_start(ap,para_count);
    ret = extract_param(obj,func,call_back,para_count,ap);
    va_end(ap);
    
    if(WM_SUCCESS != ret) {
        goto delayed_task_error_handle2;
    
    }

    ret = os_timer_create(&task->task_timer,"task_timer",
                          os_msec_to_ticks(wait_time),
                          delayed_task_call_back,task,
                          OS_TIMER_ONE_SHOT,OS_TIMER_AUTO_ACTIVATE);

    if(WM_SUCCESS == ret) {
        //add task to list
        os_mutex_get(&delayed_task_mutex, OS_WAIT_FOREVER);
        list_add(&task->list,&delayed_task_list);
        os_mutex_put(&delayed_task_mutex);

        return WM_SUCCESS;
    }

delayed_task_error_handle2:
    if(obj) {
        free(obj);
    }

    if(task) 
        task->task = NULL;
delayed_task_error_handle1:
    if(task) {
        free(task);
    }
    task = NULL;
    return -1;

}


int restart_delayed_task_on_worker_thread(general_cross_thread_task func,task_result call_back,unsigned long wait_time,int para_count, ...)
{
    int ret;
    bool found = false;
    va_list ap;
    struct delayed_task * task;

    if(!worker_thread_state.is_worker_thread_run) {
        return -1; 
    }

    if(!func) {
        return -1;
    }

    if((para_count < 0) || (para_count > 5)) {
        return -1; 
    }

    os_mutex_get(&delayed_task_mutex,OS_WAIT_FOREVER);
    list_for_each_entry(task,&delayed_task_list,list){
        if(task && task->task && (func == task->task->task)) {
            found = true;
            LOG_INFO("find timer and will update \r\n");

            if(task->task_timer) {
                os_timer_change(&task->task_timer,os_msec_to_ticks(wait_time),0);
                //update para 
                va_start(ap,para_count); 
                extract_param(task->task,func,call_back,para_count,ap);
                va_end(ap);
            }
        }     
    }
    os_mutex_put(&delayed_task_mutex);
   
    if(found) {
       return WM_SUCCESS; 
    }

    LOG_INFO("not find timer will add\r\n");
    //add task to delayed_task list
    task = malloc(sizeof(struct delayed_task));
    if(!task) {
        return -1;
    }

    memset(task,0,sizeof(struct delayed_task));
    //init list
    INIT_LIST_HEAD(&task->list);

    cross_thread_type5 *obj = (cross_thread_type5 *)malloc(sizeof(cross_thread_type5));
    if(!obj) {
        goto restart_delayed_task_error_handle1;
    }
    
    memset(obj,0,sizeof(cross_thread_type5));
    task->task = obj;
    
    //extract every para
    va_start(ap,para_count);
    ret = extract_param(obj,func,call_back,para_count,ap);
    va_end(ap);
    
    if(WM_SUCCESS != ret) {
        goto restart_delayed_task_error_handle2;
    
    }

    ret = os_timer_create(&task->task_timer,"task_timer",
                          os_msec_to_ticks(wait_time),
                          delayed_task_call_back,task,
                          OS_TIMER_ONE_SHOT,OS_TIMER_AUTO_ACTIVATE);

    if(WM_SUCCESS == ret) {
        //add task to list
        os_mutex_get(&delayed_task_mutex, OS_WAIT_FOREVER);
        list_add(&task->list,&delayed_task_list);
        os_mutex_put(&delayed_task_mutex);

        return WM_SUCCESS;
    }

restart_delayed_task_error_handle2:
    if(obj) {
        free(obj);
    }

    if(task) 
        task->task = NULL;
restart_delayed_task_error_handle1:
    if(task) {
        free(task);
    }
    task = NULL;
    return -1;
}
