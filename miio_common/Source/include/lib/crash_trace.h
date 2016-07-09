/*
 * crash_trace.h
 *
 *  Created on: 2014年9月15日
 *      Author: liuxin
 */

#ifndef CRASH_TRACE_H_
#define CRASH_TRACE_H_

#if defined(CONFIG_CPU_MW300)
    #include <core_cm4.h>
#elif defined(CONFIG_CPU_MC200)
    #include <core_cm3.h>
#else 
#error not supported platform
#endif

// this is copied from mc200_startup.c, keep synced!
typedef struct stframe {
	int r0;
	int r1;
	int r2;
	int r3;
	int r12;
	int lr;
	int pc;
	int psr;
} stframe;

#define CRASH_TRACE_VALID_MAGIC (('C' << 24)|('T' << 16)|('V' << 8)|('M' << 0))

typedef struct crash_trace_t
{
	uint32_t valid;
	signed char task_name[16];
	unsigned long *pmsp;
	unsigned long *ppsp;
	stframe msp;
	stframe psp;
	SCB_Type scb;
	uint32_t stack_dump_size;
	uint8_t stack_dump[400]; // (1000-100)/4*3-240 = 435, 1000 is max json len, 100 is json overhead, 240 is other part len of crash trace
} crash_trace_t;


#define ASSERT_TRACE_VALID_MAGIC (('R' << 24)|('C' << 16)|('V' << 8)|('M' << 0))
#define MAX_ASSET_TRACE_DATA (sizeof(crash_trace_t) - sizeof(assert_trace))
typedef struct assert_trace_t
{
    uint32_t valid;
    char data[0]; //store assert context text, share same location with crash_trace_t
} assert_trace;

void save_crash_trace(unsigned long *pmsp, unsigned long *ppsp, SCB_Type *scb, const char *task_name);
int crash_assert_check(void);
#endif /* CRASH_TRACE_H_ */
