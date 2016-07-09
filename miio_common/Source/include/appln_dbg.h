/*
 *  Copyright (C) 2008-2013, Marvell International Ltd.
 *  All Rights Reserved.
 */

#ifndef __APPLN_DBG_H__
#define __APPLN_DBG_H__

#include <wmlog.h>

void appln_print_time(void);
int appln_cli_init(void);

typedef enum {
	PRINT_LEVEL_INFO = 0,
	PRINT_LEVEL_WARNING,
	PRINT_LEVEL_ERROR,
	PRINT_LEVEL_FATAL,
	PRINT_LEVEL_DEBUG
} e_print_level_t;

extern e_print_level_t print_level;

#if APPCONFIG_DEBUG_ENABLE
    #define LOG_INFO(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_INFO){appln_print_time();wmprintf("[I] "_fmt_, ##__VA_ARGS__);}}while(0)
    #define LOG_WARN(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_WARNING){appln_print_time();wmprintf("[W] "_fmt_, ##__VA_ARGS__);}}while(0)
    #define LOG_ERROR(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_ERROR){appln_print_time();wmprintf("[E] "_fmt_, ##__VA_ARGS__);}}while(0)
    #define LOG_FATAL(_fmt_, ...)	do{if(print_level <= PRINT_LEVEL_FATAL){appln_print_time();wmprintf("[F] "_fmt_, ##__VA_ARGS__);}}while(0)
    #define LOG_DEBUG(_fmt_, ...)	do{appln_print_time();wmprintf("[D] "_fmt_, ##__VA_ARGS__);}while(0)
    #define PRINTF(_fmt_, ...)	do{wmprintf(_fmt_, ##__VA_ARGS__);}while(0)
#else
    #define dbg(...)
    #define LOG_DEBUG(...)	
    #define LOG_INFO(...)	
    #define LOG_WARN(...)	
    #define LOG_ERROR(...)	
    #define LOG_FATAL(...)	
    #define PRINTF(_fmt_, ...)
#endif /* APPCONFIG_DEBUG_ENABLE */


#endif /* ! __APPLN_DBG_H__ */
