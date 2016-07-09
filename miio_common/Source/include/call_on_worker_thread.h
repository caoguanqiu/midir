/*! \file call_on_worker_thread.h
 *  \brief 提供将任务交给worker线程的接口
 *
 *  耗时较长可能阻塞所在线程的任务，可以传递给worker线程执行。 
 *
 */

#ifndef _CALL_ON_WORKER_THREAD_
#define _CALL_ON_WORKER_THREAD_

/** 传递给worker线程执行的函数的声明
 *
 *  该定义在接口中使用.实际代码中传递给worker线程的任务函数，
 *  参数可以有0-5个，返回值一定是int
 *  
 */
typedef int (*general_cross_thread_task)();

/** 传递给worker线程的任务函数，被执行完毕后的回调函数
 *
 *  该函数的参数为传递到worker线程执行的函数的返回值.要注意该
 *  异步回调函数是在worker线程执行的.
 *
 *  @param[in] ret: 任务函数执行后的返回值
 *  @param[in] para_count: 参数的数量,后面跟随的是每一个具体的参数，
 *              添加这些参数是为了方便业务逻辑做清除工作
 *  
 */
typedef void (*task_result)(int ret,int para_count, ...);


/** 将函数交给worker线程执行
 *
 * 该函数将参数func传递到worker线程执行，执行完毕后，func的
 * 返回值作为call_back的参数传递回来.开发者可以依据该返回值
 * 得知fun执行的成功与失败，同时也可以在该函数中释放资源。该
 * 函数可以有0-5个参数，para_count必须与参数个数对应，后面依次
 * 写入各个参数。
 *
 *
 * @param[in] func 任务函数
 * @param[in] call_back 任务执行完毕后的异步回调,
 * @param[in] para_count 后序可变参数的个数,数值为0-5
 *
 * @return 0    任务已经成功传递到worker线程
 * @return 负值 worker线程忙碌，任务传递不成功
 */
int call_on_worker_thread(general_cross_thread_task func,task_result call_back,int para_count, ...);

/** 将函数交给worker线程,但不是立即执行，而是延迟wait_time时间后执行
 *
 * 该函数将参数func传递到worker线程，worker线程会在wait_time后，
 * 执行该函数。执行完毕后，func的返回值作为call_back的参数传递回来.
 * 开发者可以依据该返回值得知fun执行的成功与失败，
 * 同时也可以在该函数中释放资源。该函数可以有0-5个参数，para_count
 * 必须与参数个数对应，后面依次写入各个参数。
 *
 *
 * @param[in] func 任务函数
 * @param[in] call_back 任务执行完毕后的异步回调
 * @param[in] wait_time 执行该函数前延迟的时间
 * @param[in] para_count 后序可变参数的个数,数值为0-5
 *
 * @return 0    任务已经成功传递到worker线程
 * @return 负值 worker线程忙碌，任务传递不成功
 */
int call_delayed_task_on_worker_thread(general_cross_thread_task func,task_result call_back,unsigned long wait_time,int para_count, ...);

/** 重置某个延迟执行任务的延迟时间，如果之前没有同样的要延迟执行的任务，
 *  则新建一个延迟执行的任务
 *
 * 该函数首先查询是否已经有一个等待执行的task的任务函数为func,
 * 如果有则将延迟时间改为wait_time,否者重新添加一个延迟时间为
 * wait_time的新任务。任务函数func执行完毕后，func的返回值作为
 * call_back的参数传递回来.开发者可以依据该返回值得知fun执行的结果，
 * 同时也可以在该函数中释放资源。该函数可以有0-5个参数，para_count
 * 必须与参数个数对应，后面依次写入各个参数。
 *
 *
 * @param[in] func 任务函数
 * @param[in] call_back 任务执行完毕后的异步回调
 * @param[in] wait_time 新的延迟时间
 * @param[in] para_count 后序可变参数的个数,数值为0-5
 *
 * @return 0    任务已经成功传递到worker线程
 * @return 负值 worker线程忙碌，任务传递不成功
 */
int restart_delayed_task_on_worker_thread(general_cross_thread_task func,task_result call_back,unsigned long wait_time,int para_count, ...);


int init_worker_thread_queue(void);
void worker_thread_loop(void *arg);
int init_delayed_task_mutex(void);

#endif //_CALL_ON_WORKER_THREAD_

