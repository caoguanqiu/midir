#include "wm_os.h"

#include "lib/arch_os.h"
#include <string.h>
#include <stdlib.h>
#include <wmtime.h>



#define SECONDS_IN_365_DAY_YEAR  (31536000)
#define SECONDS_IN_A_DAY         (86400)
#define SECONDS_IN_A_HOUR        (3600)
#define SECONDS_IN_A_MINUTE      (60)

static const uint32_t secondsPerMonth[12] =
{
    31*SECONDS_IN_A_DAY,
    28*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
    30*SECONDS_IN_A_DAY,
    31*SECONDS_IN_A_DAY,
};



/*
	tick机制：
	通过tick_high 记录溢出
*/

static uint16_t tick_high = 0;	//tick高位 添加高位用于扩展计数范围
static uint8_t tick_half_flag = FALSE;		//tick值过半标记 超过0x7FFFFFFF 便设置为1

uint32_t arch_os_tick_now( void )
{
	uint32_t tick = xTaskGetTickCount();

	if(tick > 0x7FFFFFFF)
		tick_half_flag = TRUE;	//过半标记

	else if(TRUE == tick_half_flag){	//若溢出 还没有标记
		tick_half_flag = FALSE;
		tick_high++;
	}

	return tick;
}


void  arch_os_tick_sleep( uint32_t tick )
{
	vTaskDelay( tick );
}

uint32_t arch_os_tick_elapsed(uint32_t last_tick)
{
	uint32_t now = arch_os_tick_now();
	
	if(last_tick  < now)
		return now - last_tick;
	else		//可能溢出
		return 0xFFFFFFFF - last_tick + now + 1;
}



/*
	time机制：
	以秒为单位，从开机为0计时.
	通过 time_tick_diff 记录实际一秒和系统tick值的差值；
	可以提供 校准之后的 秒时间
	可以设置 秒时间校准

*/

static int32_t time_tick_diff = 0;	//本地时间和实际时间的差值 正数表示表快 反之表慢


//读取校准过的 秒时间值 系统开启时间 参考为0
uint32_t arch_os_time_now(void)
{
	uint32_t t = 0;

	//若tick有溢出
	if(tick_high > 0){
		t = 0xFFFFFFFF/(configTICK_RATE_HZ + time_tick_diff);
		t *= tick_high;
	}
	t += arch_os_tick_now()/(configTICK_RATE_HZ + time_tick_diff);
	return t;
}


//设置时间(用于调节系统时间)
void arch_os_time_tune(uint32_t std_time)
{
	uint32_t now = arch_os_time_now();
	int32_t diff = 0;

	if(std_time > now){	//系统时间过慢
		uint32_t slow = std_time - now;		//计算 在now时间段之内 慢了多少秒
		diff = (slow * configTICK_RATE_HZ / now) * (-1);
		if(diff == 0)diff = -2;
	}
	else{	//系统时间过快
		uint32_t quick = now - std_time;
		diff = (quick * configTICK_RATE_HZ / now);
		if(diff == 0)diff = 2;
	}

	time_tick_diff += diff/2;

}



/*
	utc机制:
	基于time机制 和 utc_offset 本地维持一个utc时钟
	设置utc 时间将用于校准time

*/

int32_t utc_offset = 0;

uint32_t arch_os_utc_now(void)
{
	return utc_offset + arch_os_time_now();
}


//通过utc标准时间 校准系统，
//返回系统时间偏移 (正数 表示系统时间快过UTC时间值)
int arch_os_utc_set(uint32_t utc)
{
	int ret = 0;
	uint32_t time_now = arch_os_time_now();

	
	if(utc_offset != 0){		//非第一次读取标准时间

		ret = utc_offset + time_now - utc;		//本地时间 和 标准时间 差

		//暂时不调整斜率，保持简单策略
		//arch_os_time_tune(utc - utc_offset);	//用标准时间间隔 调整系统时间

		utc_offset = utc - time_now;		//再一次设置标准时间
	}
	else
		utc_offset = utc - time_now;		//第一次设置标准时间


	utc_offset = (utc - time_now + utc_offset)/2;	//Here may be verbose .....  kill it be better.

	// set mc200 RTC to local time
	wmtime_time_set_posix(arch_os_utc_now());

	return ret;
}


int arch_os_utc_parse(uint32_t ts, uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	uint32_t time = ts;

	/* Calculate year */
	uint32_t tmp = 1970 + time/SECONDS_IN_365_DAY_YEAR;
	uint32_t number_of_leap_years = (tmp - 1968 - 1)/4;
	time = time - ((tmp-1970)*SECONDS_IN_365_DAY_YEAR);
	if ( time > (number_of_leap_years*SECONDS_IN_A_DAY))
	{
		time = time - (number_of_leap_years*SECONDS_IN_A_DAY);
	}
	else
	{
		time = time - (number_of_leap_years*SECONDS_IN_A_DAY) + SECONDS_IN_365_DAY_YEAR;
		--tmp;
	}

	if(year)
		*year = tmp;

	/* Remember if the current year is a leap year */
	uint8_t  is_a_leap_year = ((tmp - 1968)%4 == 0);

	/* Calculate month */
	tmp = 1;
	int a;
	for (a = 0; a < 12; ++a){

		uint32_t seconds_per_month = secondsPerMonth[a];
		/* Compensate for leap year */
		if ((a == 1) && is_a_leap_year)
			seconds_per_month += SECONDS_IN_A_DAY;

		if (time > seconds_per_month){
			time -= seconds_per_month;
			tmp += 1;
		}else break;
	}

	if(month)	*month = tmp;

	/* Calculate day */
	tmp = time/SECONDS_IN_A_DAY;
	time = time - (tmp*SECONDS_IN_A_DAY);
	++tmp;
	if(day)*day = tmp;


	/* Calculate hour */
	tmp = time/SECONDS_IN_A_HOUR;
	time = time - (tmp*SECONDS_IN_A_HOUR);
	if(hour)*hour = tmp;

	/* Calculate minute */
	tmp = time/SECONDS_IN_A_MINUTE;
	time = time - (tmp*SECONDS_IN_A_MINUTE);
	if(min)*min = tmp;

	/* Calculate seconds */
	if(sec)*sec = time;

	return STATE_OK;
}


int arch_os_iso8601_time(iso8601_time_t* iso8601_time)
{
	uint32_t time = arch_os_utc_now();


	/* Calculate year */
	uint32_t year = 1970 + time/SECONDS_IN_365_DAY_YEAR;
	uint32_t number_of_leap_years = (year - 1968 - 1)/4;
	time = time - ((year-1970)*SECONDS_IN_365_DAY_YEAR);
	if ( time > (number_of_leap_years*SECONDS_IN_A_DAY))
	{
		time = time - (number_of_leap_years*SECONDS_IN_A_DAY);
	}
	else
	{
		time = time - (number_of_leap_years*SECONDS_IN_A_DAY) + SECONDS_IN_365_DAY_YEAR;
		--year;
	}

	/* Remember if the current year is a leap year */
	uint8_t  is_a_leap_year = ((year - 1968)%4 == 0);

	/* Calculate month */
	uint32_t month = 1;
	int a;
	for (a = 0; a < 12; ++a)
	{
		uint32_t seconds_per_month = secondsPerMonth[a];
		/* Compensate for leap year */
		if ((a == 1) && is_a_leap_year)
		{
			seconds_per_month += SECONDS_IN_A_DAY;
		}
		if (time > seconds_per_month)
		{
			time -= seconds_per_month;
			month += 1;
		}
		else
		{
			break;
		}
	}

	/* Calculate day */
	uint32_t day = time/SECONDS_IN_A_DAY;
	time = time - (day*SECONDS_IN_A_DAY);
	++day;

	/* Calculate hour */
	uint32_t hour = time/SECONDS_IN_A_HOUR;
	time = time - (hour*SECONDS_IN_A_HOUR);

	/* Calculate minute */
	uint32_t minute = time/SECONDS_IN_A_MINUTE;
	time = time - (minute*SECONDS_IN_A_MINUTE);

	/* Calculate seconds */
	uint32_t second = time;

	/* Write iso8601 time */
	char tmp[8];
	snprintf(tmp, sizeof(tmp), "%04lu",year);
	memcpy(iso8601_time->year, tmp, sizeof(iso8601_time->year));

	snprintf(tmp, sizeof(tmp), "%02lu",month);
	memcpy(iso8601_time->month, tmp, sizeof(iso8601_time->month));

	snprintf(tmp, sizeof(tmp), "%02lu",day);
	memcpy(iso8601_time->day, tmp, sizeof(iso8601_time->day));

	snprintf(tmp, sizeof(tmp), "%02lu",hour);
	memcpy(iso8601_time->hour, tmp, sizeof(iso8601_time->hour));

	snprintf(tmp, sizeof(tmp), "%02lu",minute);
	memcpy(iso8601_time->minute, tmp, sizeof(iso8601_time->minute));

	snprintf(tmp, sizeof(tmp), "%02lu",second);
	memcpy(iso8601_time->second, tmp, sizeof(iso8601_time->second));

	/* coverity[bad_memset] */ /* Coverity issues warning about using character '0' rather than 0 */
	memset(iso8601_time->sub_second, '0', 6);
	iso8601_time->T	  = 'T';
	iso8601_time->Z	  = 'Z';
	iso8601_time->colon1 = ':';
	iso8601_time->colon2 = ':';
	iso8601_time->dash1  = '-';
	iso8601_time->dash2  = '-';
	iso8601_time->decimal = '.';

	iso8601_time->end = '\0';
	return STATE_OK;
}

/*************************************************
* remove mutex & semphre api
**************************************************/

int arch_os_thread_create(os_thread_vt** thread, const char* name, fp_thread_code_t function, uint32_t stack_size, void* arg )
{
	/*int temp = priority;

	 Limit priority to default lib priority 
	if( temp >= (int)configMAX_PRIORITIES )
		priority = priority%10;*/

	if( pdPASS == xTaskCreate( (pdTASK_CODE)function, (const char*)name, (unsigned short) (stack_size/sizeof( portSTACK_TYPE )), arg, WICED_PRIORITY_TO_NATIVE_PRIORITY(ARCH_OS_PRIORITY_DEFAULT), thread ) )
		return STATE_OK;
	else
		return STATE_ERROR;
}

int arch_os_thread_delete(os_thread_vt* thread)
{
    vTaskDelete(thread );
	return STATE_OK;
}

void arch_os_thread_finish(void)
{
	vTaskDelete(NULL);
}


void arch_os_enter_critical(void)
{
	vPortEnterCritical();
}
void arch_os_exit_critical(void)
{
	vPortExitCritical();
}



void arch_os_schedule_stop(void)
{
	vTaskSuspendAll ();
}

void arch_os_schedule_resume(void)
{
	xTaskResumeAll ();
}



int arch_os_mbox_new(os_mbox_vt**mbox, int size )
{
	*mbox = xQueueCreate(size, (uint32_t)sizeof(void *) );
	if(NULL != *mbox )
		return STATE_OK;
	else
		return STATE_ERROR;
}

int arch_os_mbox_free(os_mbox_vt *mbox )
{
	ARCH_ASSERT("", NULL != mbox);
	if ( uxQueueMessagesWaiting(mbox ) != 0 )
		BREAKPOINT();		// Line for breakpoint.  Should never break here!

	vQueueDelete(mbox);
	return STATE_OK;
}



int arch_os_mbox_post( os_mbox_vt *mbox, void *msg, uint8_t from_isr)
{
	int res;
	ARCH_ASSERT("", NULL != mbox);

	if(from_isr){
			signed portBASE_TYPE xHigherPriorityTaskWoken;


		res = xQueueSendFromISR(mbox, &msg,  &xHigherPriorityTaskWoken);

				/* If xSemaphoreGiveFromISR() unblocked a task, and the unblocked task has
			 * a higher priority than the currently executing task, then
			 * xHigherPriorityTaskWoken will have been set to pdTRUE and this ISR should
			 * return directly to the higher priority unblocked task. 
			 */
			portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );


		if(pdPASS == res)
			return STATE_OK;
		else
			return STATE_ERROR;

	}
	else{

		res = xQueueSend(mbox, &msg, 0);
		if(pdPASS == res)
			return STATE_OK;
		else
			return STATE_ERROR;

	}
}

//wait = 0 会立即返回
int arch_os_mbox_fetch(os_mbox_vt *mbox, void **msg, uint32_t wait)
{
	int ret;
	void *dummy_ptr;
	void ** tmp_ptr = msg;

	ARCH_ASSERT("", NULL != mbox);

	if ( msg == NULL )
		tmp_ptr = &dummy_ptr;

	ret = xQueueReceive(mbox, tmp_ptr, wait );

	if (pdPASS == ret )
		return STATE_OK;
	else if(wait == 0)
		return STATE_EMPTY;
	else
		return STATE_TIMEOUT;
}

int arch_os_mutex_create(os_mutex_vt** handle)
{
	*handle = xSemaphoreCreateMutex();
	if(*handle)
		return STATE_OK;
	else
		return STATE_ERROR;
}

int arch_os_mutex_delete(os_mutex_vt* handle)
{
	vSemaphoreDelete(handle);
	return STATE_OK;
}

int arch_os_mutex_put(os_mutex_vt* handle)
{
	int ret;
	signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	if (__get_IPSR() != 0) {
		/* This call is from Cortex-M3 handler mode, i.e. exception
		 * context, hence use FromISR FreeRTOS APIs.
		 */
		ret = xSemaphoreGiveFromISR(handle,
					    &xHigherPriorityTaskWoken);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	} else
		ret = xSemaphoreGive(handle);

	return ret == pdTRUE ? STATE_OK : STATE_ERROR;
}

int arch_os_mutex_get(os_mutex_vt* handle, uint32_t wait)
{
	int ret = xSemaphoreTake(handle, wait);
	return ret == pdTRUE ? STATE_OK : STATE_ERROR;
}

int arch_os_queue_create(os_queue_t *qhandle, uint32_t q_len, uint32_t item_size)
{
    *qhandle = xQueueCreate(q_len, item_size);
    if(*qhandle)
        return STATE_OK;
    else
        return STATE_ERROR;
}

int arch_os_queue_delete(os_queue_t *qhandle)
{
    vQueueDelete(*qhandle);
    qhandle = NULL;
    return STATE_OK;
}

int arch_os_queue_send(os_queue_t *qhandle, const void *msg, uint32_t wait)
{
    int ret;
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    if (__get_IPSR() != 0) {
        /* This call is from Cortex-M3 handler mode, i.e. exception
         * context, hence use FromISR FreeRTOS APIs.
         */
        ret = xQueueSendToBackFromISR(*qhandle, msg,
                          &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    } else
        ret = xQueueSendToBack(*qhandle, msg, wait);

    return ret == pdTRUE ? STATE_OK : STATE_ERROR;
}

int arch_os_queue_recv(os_queue_t *qhandle, void *msg, uint32_t wait)
{
    int ret = xQueueReceive(*qhandle, msg, wait);
    return ret == pdTRUE ? STATE_OK : STATE_ERROR;
}

int arch_os_queue_get_msgs_waiting(os_queue_t *qhandle)
{
    int nmsg = 0;
    nmsg = uxQueueMessagesWaiting(*qhandle);
    return nmsg;
}

