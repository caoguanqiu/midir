/*************************************************************************
	> File Name: local_timer.h
	> Author: 
	> Mail: 
	> Created Time: Wed 17 Feb 2016 08:15:10 PM CST
 ************************************************************************/

#ifndef _LOCAL_TIMER_H
#define _LOCAL_TIMER_H
#endif

//*****************************local_timer about*****************************************/
typedef enum
{
    CRONTAB= 0,
    ONCE_TIME
} timer_type;

typedef enum
{
    TIMER_EXE_FAILED = 0,
    TIMER_EXE_SUCCESS
} timer_exe_status_t;

typedef enum
{
    UNREPORTED = 0,
    REPORTED 
} timer_report_status_t;

typedef struct _scheduled_task_t
{
    char id_str[12];//timer_id
    timer_type type; //type of timer, crontab or once_time

    char crontab[24];//time to execute the timer, only for crontab timer
    time_t posix;//time to execute the timer, only for once_time timer

    char* command_str;
    char* params_str;

    uint8_t day_processed;//which days have been processed
    uint32_t processed_time;//last processed time
    timer_exe_status_t exe_state;//last exe_state after timer was processed
    timer_report_status_t report_state;//last report_state after timer was processed
} scheduled_task_t;
typedef scheduled_task_t* scheduled_task_p_t;

void local_timer_init(void);
void local_timer_deinit(void);
void display_all_timer_schedule(void);
int  rm_all_timer_schedule(void);
void check_schedule_and_set_timer();
