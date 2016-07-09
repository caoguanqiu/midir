/*************************************************************************
	> File Name: dht11.h
	> Author: subenchang
	> Mail: subenchang@xiaomi.com
	> Created Time: Sat 27 Dec 2014 04:43:45 PM CST
 ************************************************************************/

#ifndef _DHT11_H
#define _DHT11_H

#include <wm_os.h>
#include <lib/arch_os.h>
#include <lib/arch_io.h>
#include <board.h>
#include <appln_dbg.h>
#include <app_framework.h>



void mySleepUs(int us);
int readDHT(int* temp,int* humi);

#endif
