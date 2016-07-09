#ifndef __DELAYZERO_H__
#define __DELAYZERO_H__

#include "miio_arch.h"

#include "lib_xobj.h"
#include "lib_generic.h"

struct d0_trigger;
struct d0_watcher;
struct d0_invoke;
struct d0_loop;
struct d0_timer;
struct d0_session;



/*
	ALL ~~, start from trigger....
*/

#define D0_TIGGER_PRESET(pe,h) {	struct d0_trigger*p=(struct d0_trigger*)(pe); \
									p->trig_handler = (h);}

typedef struct d0_trigger d0_trigger_t;

typedef int (*fp_d0_trigger_handler_t)(struct d0_trigger*);

struct d0_trigger{
	fp_d0_trigger_handler_t trig_handler;
};



/*
	d0_invoke Inter Watcher Call
*/


#define D0_WCALL_PRESET(pm,w,h) {	struct d0_invoke*p = (struct d0_invoke*)(pm);  \
									p->type = D0_WATCHER_CALL; p->call.watcher.dest = (w); p->call.watcher.handler = (h);}

#define D0_LCALL_PRESET(pm,l,h) {	struct d0_invoke*p = (struct d0_invoke*)(pm);  \
									p->type = D0_LOOP_CALL; p->call.loop.dest = (l); p->call.loop.handler = (h);}




typedef struct d0_invoke d0_invoke_t;

typedef enum {
	D0_INVOKE_RET_VAL_RELEASE = 0,
	D0_INVOKE_RET_VAL_DO_NOT_RELEASE = 1,
}d0_ctx_call_ret_t;

typedef d0_ctx_call_ret_t (*fp_d0_wcall_handler_t)(struct d0_watcher*, struct d0_invoke*);		//返回值0，则释放
typedef d0_ctx_call_ret_t (*fp_d0_lcall_handler_t)(struct d0_loop*, struct d0_invoke*);		//返回值0，则释放



typedef enum {
	D0_WATCHER_CALL = 0,
	D0_LOOP_CALL = 1,
}d0_invoke_type_t;

struct d0_invoke{
	struct d0_trigger trigger;
	d0_invoke_type_t type;
	union{
		struct{
			struct d0_watcher* dest;
			fp_d0_wcall_handler_t handler;
		}watcher;

		struct{
			struct d0_loop* dest;
			fp_d0_lcall_handler_t handler;
		}loop;
	}call;


	void* ctx;
};



/*
	d0_timer 
*/
#define D0_TIMER_RELEASE 0

#define D0_TIMER_PRESET(pt,n,t,h,c) {	struct d0_timer*p = (struct d0_timer*)(pt);  \
										p->next = (n); p->time = (t); p->tmr_handler = (h); p->ctx = (c); }
typedef struct d0_timer d0_timer_t;

//定时器处理函数指针类型
typedef int (* fp_d0_timer_handler)(struct d0_timer*, void *);	//返回值大于0，标识再续定时的时间，单位是ms



//定时器
struct d0_timer {
	struct d0_timer *next;
	u32_t time;
	fp_d0_timer_handler tmr_handler;
	void *ctx;
};


/*
	session
*/

struct d0_session{
	struct d0_session* next;
	struct d0_watcher *owner;
};

#define d0_session_owner(psess)	(((struct d0_session*)(psess))->owner)

/*
	Event ...
*/
struct d0_listener;
struct d0_event;

#define D0_EVENT_NAME_SIZE_MAX 32


struct d0_event{
	struct d0_invoke invk;
	char evt_txt[D0_EVENT_NAME_SIZE_MAX];
};


typedef void d0_event_ret_t;

typedef d0_event_ret_t(*fp_listener_op)(struct d0_event*, void*);

struct d0_listener{
	struct d0_listener* next;
	char evt_txt[D0_EVENT_NAME_SIZE_MAX];
	fp_listener_op handler;
};




/*
	Watcher...
*/
typedef struct d0_watcher d0_watcher_t;

#define LOOPER(w) ((struct d0_watcher*)(w))->loop


typedef int(*fp_watcher_op)(struct d0_watcher*);


typedef struct d0_watcher_if{
	fp_watcher_op enter;		//若返回小于0，则不进入主循环，直接退出，并且不执行exit
	fp_watcher_op exit;
}d0_watcher_if_t;


struct d0_watcher{
	struct d0_watcher* next;
	struct d0_loop* loop;
	//session 根部
	struct d0_session* session_hdr;

	const struct d0_watcher_if* d0_if;
};




/*
	Loop...
*/
//指示loop正在结束中...
#define D0_LOOP_FLAG_BIT_EXITING			0x80000000

//用于确定是否在退出前 销毁本进程
#define D0_LOOP_FLAG_BIT_SELF_RELEASE		0x20000000


#define D0_LOOP_FLAG_SET_SELF_RELEASE(loop) 	((struct d0_loop*)loop)->flag |= D0_LOOP_FLAG_BIT_SELF_RELEASE
#define D0_LOOP_FLAG_NOT_SELF_RELEASE(loop) 	((struct d0_loop*)loop)->flag &= ~D0_LOOP_FLAG_BIT_SELF_RELEASE
#define D0_LOOP_FLAG_IS_SELF_RELEASE(loop) 	(((struct d0_loop*)loop)->flag & D0_LOOP_FLAG_BIT_SELF_RELEASE)


#define D0_LOOP_FLAG_EXITING_SET(loop)	((struct d0_loop*)loop)->flag |= D0_LOOP_FLAG_BIT_EXITING
#define D0_LOOP_FLAG_EXITING_RESET(loop)	((struct d0_loop*)loop)->flag &= ~D0_LOOP_FLAG_BIT_EXITING
#define D0_LOOP_FLAG_IS_EXITING(loop)	(((struct d0_loop*)loop)->flag & D0_LOOP_FLAG_BIT_EXITING)


struct d0_loop {
	xobj_info_t xobj;
	os_thread_vt* thread;

	//标志
	u32_t flag;

	//邮箱
	os_mbox_vt* mbox;

	//watcher 列表
	struct d0_watcher* watcher_hdr;

	//定时器链表
	struct d0_timer* timer_hdr;	//初始化为0

	//事件处理链表
	struct d0_listener* listener_hdr;
};



struct d0_trigger* d0_trigger_alloc(fp_d0_trigger_handler_t hdlr, u16_t ext_len);
void* d0_trigger_ext(struct d0_trigger* trig);
int d0_trigger_pull(struct d0_trigger*trig);

struct d0_invoke* d0_invoke_watcher_alloc(struct d0_watcher* w, fp_d0_wcall_handler_t hdlr, u16_t ext_len);
struct d0_invoke* d0_invoke_loop_alloc(struct d0_loop* l, fp_d0_lcall_handler_t hdlr, u16_t ext_len);
void d0_invoke_watcher_preset(struct d0_invoke* invk, struct d0_watcher* w, fp_d0_wcall_handler_t hdlr);
void d0_invoke_loop_preset(struct d0_invoke* invk, struct d0_loop* l, fp_d0_lcall_handler_t hdlr);
void* d0_invoke_ext(struct d0_invoke* m);
int d0_invoke_start(struct d0_invoke* m, void* msg);
d0_invoke_type_t d0_invoke_type(struct d0_invoke* invk);
void* d0_invoke_dest(struct d0_invoke* invk);

struct d0_timer *d0_timer_attach(struct d0_loop* loop, fp_d0_timer_handler handler, u32_t time, void* ctx, size_t ext_len);
struct d0_timer *d0_timer_restart(struct d0_loop* loop, fp_d0_timer_handler handler, u32_t time);
int d0_timer_delete(struct d0_loop* loop, fp_d0_timer_handler handler);
struct d0_timer *d0_timer_restart_by_ctx(struct d0_loop* loop, void* ctx, u32_t time);
int d0_timer_delete_by_ctx(struct d0_loop* loop, void* ctx);
void* d0_timer_ext(struct d0_timer* t);
struct d0_timer *d0_timer_find(struct d0_loop* loop, fp_d0_timer_handler handler);


struct d0_watcher* d0_watcher_alloc(const struct d0_watcher_if* d0_if, u16_t ext_len);

void d0_sess_attach_insert(struct d0_watcher* w, struct d0_session* psess);
void d0_sess_attach_append(struct d0_watcher* w, struct d0_session* psess);
void d0_sess_detach(struct d0_watcher* w, struct d0_session* psess);
int d0_sess_count(struct d0_watcher* w);
struct d0_session* d0_sess_exist(struct d0_watcher* w, struct d0_session* psess);
void* d0_sess_opt(struct d0_session* psess);
struct d0_session* d0_sess_alloc(struct d0_watcher* w, u16_t opt);	//同时申请opt指定长度
void d0_sess_preset(struct d0_watcher* w, struct d0_session* p);
void d0_sess_free(struct d0_session* psess);

struct d0_watcher* d0_sess_owner(struct d0_session* psess);
//迭代处理各个session，返回session数量(如果迭代函数结果非零 则跳出)
int d0_sess_foreach(struct d0_watcher* w, fp_iterator func, void* args);

int d0_event_emit(struct d0_loop* l, const char* txt, void* ctx);
int d0_event_on(struct d0_loop* loop, const char* txt, fp_listener_op handler);
int d0_event_off(struct d0_loop* loop, const char* txt, fp_listener_op handler);

void* d0_watcher_ext(struct d0_watcher* w);
int d0_watcher_attach(struct d0_loop* l, struct d0_watcher* w);
int d0_watcher_detach(struct d0_watcher* w);
char* d0_watcher_name(struct d0_watcher* w);
int d0_watcher_preset(struct d0_watcher* w, const struct d0_watcher_if* d0_if);

int d0_loop_exit_req(struct d0_loop* l);
void d0_loop_preset(struct d0_loop* l, char* name);
struct d0_loop* d0_loop_alloc(char* name, u16_t ext_len);
int d0_start(struct d0_loop* l);
int d0_start_thread(struct d0_loop* l, u32_t stacksize);
struct d0_loop* d0_api_find(const char* d0_name);


#endif /* __DELAYZERO_H__ */




