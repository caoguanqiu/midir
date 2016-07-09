/**
 * @file			addif_os_export.h
 * @brief			Operation system APIs
 * @author			dy
*/

typedef struct
{
	char year[4];
	char dash1;
	char month[2];
	char dash2;
	char day[2];
	char T;
	char hour[2];
	char colon1;
	char minute[2];
	char colon2;
	char second[2];
	char decimal;
	char sub_second[6];
	char Z; // UTC timezone
	char end;
} iso8601_time_t;


#define OS_NEVER_TIMEOUT OS_WAIT_FOREVER

#define BEIJING_UTC_SECOND_OFFSET 8*3600


typedef int (*fp_thread_code_t)(void* arg);


