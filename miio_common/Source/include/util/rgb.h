/*************************************************************************
	> File Name: rgb.h
	> Author: subenchang
	> Mail: subenchang@xiaomi.com
	> Created Time: Sat 27 Dec 2014 04:42:58 PM CST
 ************************************************************************/

#ifndef _RGB_H
#define _RGB_H

#include <lib/arch_os.h>
#include <lib/arch_io.h>
#include <misc/led_indicator.h>
#include <appln_dbg.h>
#include <board.h>
#include <mdev_gpt.h>
#include <lowlevel_drivers.h>
#include <app_framework.h>

void setRGB(u32_t freq, u32_t thr, u8_t r,u8_t g,u8_t b);
void initRGB();
void getRGB(u8_t* r,u8_t* g,u8_t* b);
void parseRGB(char* params,u8_t* r,u8_t* g,u8_t* b);

#endif
