/*************************************************************************
	> File Name: ./miio_app/app_proj/Source/util/dht11.c
	> Author: subenchang
	> Mail: subenchang@xiaomi.com
	> Created Time: Sat 27 Dec 2014 04:46:47 PM CST
 ************************************************************************/

#include <util/dht11.h>


#define TIMEOUT_DHT 5000
#define PIN_DHT   GPIO_8
//****************up:  propsDHT************************/
void mySleepUs(int us)
{
    int i = 50 * us;

    while(i--)
    {
        __asm("nop");
    }
}

static  int pin_read(u8_t* bits)
{
    u8_t cnt = 7;
    u8_t idx = 0;

    int i = 0;
    for(i = 0; i < 5; i++)
    {
        bits[i] = 0;
    }

    //request sample to DHT11  from MIIO
    api_io_init(PIN_DHT,GPIO_OUTPUT);
    api_io_low(PIN_DHT);
    api_os_tick_sleep(20);

    api_io_high(PIN_DHT);
    mySleepUs(40);
    api_io_init(PIN_DHT,GPIO_INPUT);

    //respond sample to MIIO from DHT11
    u32_t loopCnt = TIMEOUT_DHT;
    while(api_io_get(PIN_DHT) == GPIO_IO_LOW)
    {
        if(loopCnt-- == 0)return -2;
        __asm("nop");
    }

    loopCnt = TIMEOUT_DHT;
    while(api_io_get(PIN_DHT) == GPIO_IO_HIGH)
    {
        if(loopCnt-- == 0)return -2;
        __asm("nop");
    }
//get the temp and humi
    for(i = 0; i < 40 ; i++)
    {
        loopCnt = TIMEOUT_DHT;
        while(api_io_get(PIN_DHT) == GPIO_IO_LOW)
        {
            if(loopCnt-- == 0)return -2;
            __asm("nop");
        }
        mySleepUs(50);

        if(api_io_get(PIN_DHT) == GPIO_IO_HIGH)
        {
            bits[idx] |= (1 << cnt);
        }

        if(cnt == 0)
        {
            cnt = 7;
            idx++;
        }
        else cnt--;

        loopCnt = TIMEOUT_DHT;
        while(api_io_get(PIN_DHT) == GPIO_IO_HIGH)
        {
            if(loopCnt-- == 0)return -2;
            __asm("nop");
        }
    }
    return 0;
}

int readDHT(int* temp,int* humi)
{

    u8_t bits[5];
    int rv = pin_read(bits);
    if(rv != 0)return rv;

    *humi = bits[0];
    *temp = bits[2];

    u8_t sum = bits[0] + bits[2];
    if(bits[4] != sum)return -1;

    return 0;

}
