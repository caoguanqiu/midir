/*************************************************************************
	> File Name: ./miio_app/app_proj/Source/util/rgb.c
	> Author: subenchang
	> Mail: subenchang@xiaomi.com
	> Created Time: Sat 27 Dec 2014 04:45:34 PM CST
 ************************************************************************/


#include <util/rgb.h>
#include <stdlib.h>
#define PIN_RED   GPIO_9
#define PIN_GREEN GPIO_10
#define PIN_BLUE  GPIO_11

static u8_t glb_r,glb_b,glb_g;

void setRGB(u32_t freq, u32_t thr, u8_t r,u8_t g,u8_t b)
{
	int gpt_clk;
	int x,y;
	int freq_count;
	mdev_t* gpt_dev;

	if((r>thr)||(g>thr)||(b>thr))
		wmprintf("%s,the r:%d,g:%d,b:%d is outsize of threshold\r\n",r,g,b);

    gpt_drv_init(GPT3_ID);
    gpt_dev = gpt_drv_open(GPT3_ID);

	gpt_clk = board_main_xtal();//you should confirm used CLK_MAINXTAL as GPT_CLK
	freq_count = gpt_clk/freq;

	//set r
	x = freq_count * r / thr;
	y = freq_count -x;
	gpt_drv_pwm(gpt_dev,GPT_CH_3,x,y);

	//set g
	x = freq_count * g / thr;
	y = freq_count -x;
	gpt_drv_pwm(gpt_dev,GPT_CH_4,x,y);

	//set b
	x = freq_count * b / thr;
	y = freq_count -x;
	gpt_drv_pwm(gpt_dev,GPT_CH_5,x,y);

    gpt_drv_close(gpt_dev);

    glb_r = r;
    glb_g = g;
    glb_b = b;
    wmprintf("setRGB(%d,%d,%d,%d,%d)\r\n",freq,thr,r,g,b);
}

void initRGB()
{

    mdev_t* gpt_dev;

    gpt_drv_init(GPT3_ID);
    gpt_dev = gpt_drv_open(GPT3_ID);

    GPIO_PinMuxFun(PIN_RED,GPIO9_GPT3_CH3);
    GPIO_PinMuxFun(PIN_GREEN,GPIO10_GPT3_CH4);
    GPIO_PinMuxFun(PIN_BLUE,GPIO11_GPT3_CH5);

    CLK_GPTInternalClkSrc(CLK_GPT_ID_3,CLK_MAINXTAL);
    CLK_ModuleClkDivider(CLK_GPT3,0);

    gpt_drv_set(gpt_dev,0x51EB851);

    gpt_drv_pwm(gpt_dev,GPT_CH_3,0,255);
    gpt_drv_pwm(gpt_dev,GPT_CH_4,0,255);
    gpt_drv_pwm(gpt_dev,GPT_CH_5,0,255);
    gpt_drv_start(gpt_dev);
    gpt_drv_close(gpt_dev);

    setRGB(1000,255,0,0,0);
}

void de_initRGB()
{
    GPIO_PinMuxFun(PIN_RED,GPIO9_GPIO9);
    GPIO_PinMuxFun(PIN_GREEN,GPIO27_GPIO27);
    GPIO_PinMuxFun(PIN_BLUE,GPIO11_GPIO11);
}



void getRGB(u8_t* r,u8_t* g,u8_t* b)
{

    *r = glb_r;
    *g = glb_g;
    *b = glb_b;
}

void parseRGB(char* params,u8_t* r,u8_t* g,u8_t* b)
{
    if(params == NULL)
    {
        LOG_ERROR("params error.\r\n");
        return;
    }
    //get r
    char* head = params;
    char* tail = strchr(head,',');
    if(tail == NULL)
    {
        LOG_ERROR("params error.\r\n");
        return;
    }
    *tail = '\0';
    *r = atoi(head);

    //get g
    head = tail + 1;
    tail = strchr(head,',');
    if(tail == NULL)
    {
        LOG_ERROR("params error.\r\n");
        return;
    }
    *tail = '\0';
    *g = atoi(head);

    //get b
    head = tail + 1;
    if(head == NULL)
    {
        LOG_ERROR("params error.\r\n");
        return;
    }
    *b = atoi(head);
}

