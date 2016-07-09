/******************************************************************************
 * Model: xiaomi.demo.v1
 * Property:
 *      props temp 21 humi 23
 * Method:
 *      set_rgb
 *      get_prop
 * Event:
 *      event.button_pressed
 *
 * Author: subenchang
 * Email: subenchang@xiaomi.com
 ******************************************************************************
 */
/*
 * 黑 3.3
 * 褐 TXD
 * 橙 RXD
 * 红 GND
*/
#include <lwip/lwipopts.h>
#include <lwip/arch.h>
#include <wm_os.h>
#include <ot/d0_otd.h>
#include <lib/jsmn/jsmn_api.h>
#include <lib/arch_os.h>
#include <lib/arch_io.h>
#include <ot/lib_addon.h>
#include <lib/button_detect_framework.h>
#include <misc/led_indicator.h>
#include <appln_dbg.h>
#include <board.h>
#include <mdev_gpt.h>
//#include <lowlevel_drivers.h>
#include <mc200_pinmux.h>
#include <mc200_clock.h>
#include <app_framework.h>
#include <lib/miio_cmd.h>
#include <pwrmgr.h>
#include <boot_flags.h>
#include <main_extension.h>
#include <lib/miio_up_manager.h>
#include <lib/uap_diagnosticate.h>

#include <lib/miio_down_manager.h>
#include <lib/miio_up_manager.h>
#include <malloc.h>
#include <psm.h>
#include <stdlib.h>

const int MIIO_APP_VERSION = 2;

#include <util/rgb.h>
#include <util/dht11.h>

#define DM_PWM_FREQ	2000
#define DM_PWM_THR		255
#define TIME_PROPS_DHT 300000
#define TIME_CHECK_DHT 1000

//const int MIIO_APP_VERSION = 0;

static os_timer_t g_reset_prov_timer;
static os_semaphore_t btn_pressed_sem;
static u8_t g_button_pressed;
static mum g_mum;
int g_temp,g_humi;


//*****callback: the button has been pressed than 4s **********/
static void reset_prov(os_timer_arg_t arg)
{
    wmprintf("start deprovisioning\r\n");
    if(is_provisioned())
    {
        app_reset_configured_network();
    }
    else{
        pm_reboot_soc(); //reset again,just reboot
    }
}
//******************************************************************//

//*****button pressed handle***********/
static void button_1_press_handle(GPIO_IO_Type pin_level)
{
    if (pin_level == GPIO_IO_LOW)
    {
        LOG_INFO("button pressed\r\n");

        g_button_pressed = 1;
        os_semaphore_put(&btn_pressed_sem);
//start timer
        if(os_timer_is_running(&g_reset_prov_timer))
        {
            if(WM_SUCCESS != os_timer_deactivate(&g_reset_prov_timer))
            {
                LOG_ERROR("deactivating reset provision timer failed\r\n");
            }
        }
        if(WM_SUCCESS != os_timer_activate(&g_reset_prov_timer))
        {
            LOG_ERROR("activating reset provision timer failed\r\n");
        }
    }
    else
    {
        LOG_INFO("button released\r\n");
//stop timer
        if(os_timer_is_running(&g_reset_prov_timer))
        {
            if(WM_SUCCESS != os_timer_deactivate(&g_reset_prov_timer))
            {
                LOG_ERROR("deactivating reset provision timer failed\r\n");
            }
        }
    }
}
//*********************************************************************//

//*****check temperature and humidity: if change, props***********************//
void checkDHT()
{
    int temp,humi;
    int ret = readDHT(&temp,&humi);
    if(ret == -1)wmprintf("check sum error\r\n");
    else if(ret == -2)wmprintf("get dht timeout\r\n");
    else
    {
        char str[16];

        if(abs(temp - g_temp) >= 1)
        {
            g_temp = temp;
            snprintf(str,sizeof(str),"%d",temp);
            mum_set_property(g_mum, "temperature", str);
        }

        mum_get_property(g_mum, "humidity", str);
        int last_humi = atoi(str);
        if(abs(humi - last_humi) >= 1)
        {
            snprintf(str,sizeof(str),"%d",humi);
            mum_set_property(g_mum, "humidity",str);
        }
    }
}
//*****props temperature and humidity*****************************************//
void propsDHT()
{
    int temp,humi;
    int ret = readDHT(&temp,&humi);
    if(ret == -1)wmprintf("check sum error\r\n");
    else if(ret == -2)wmprintf("get dht timeout\r\n");
    else
    {
        char str[8];
        snprintf(str,sizeof(str),"%d",temp);
        mum_set_property(g_mum, "temperature",str);
        snprintf(str,sizeof(str),"%d",humi);
        mum_set_property(g_mum, "humidity",str);
    }
}
//*********************************************************************//

void miio_app_thread(void* arg)
{
    int ret;
    psm_handle_t handle;
    char model[64] = "xiaomi.demo.v1";
    //char model[64] = "midea.aircondition.v1";

    ret = psm_open(&handle, "ot_config");
    if (ret)
    {
        LOG_ERROR("Error opening PSM module\r\n");
        return;
    }
    psm_set(&handle, "psm_model", model);
    if (WM_SUCCESS != psm_get(&handle, "psm_model", model, sizeof(model)))
    {
        model[0] = '\0';
    }
    LOG_INFO("model:%s\r\n",model);
    psm_close(&handle);



    initRGB();
    g_mum = mum_create();
    mum_set_property(g_mum, "rgb","0");
    mum_set_property(g_mum, "temperature","-1");
    mum_set_property(g_mum, "humidity","-1");

    button_frame_work_init();
    register_button_callback(0, button_1_press_handle);
    os_timer_create(&g_reset_prov_timer, "reset-prov-timer", os_msec_to_ticks(4000),reset_prov, NULL, OS_TIMER_ONE_SHOT, OS_TIMER_NO_ACTIVATE);

    if(WM_SUCCESS != os_semaphore_create(&btn_pressed_sem, "btn_pressed_sem"))
    {
        LOG_ERROR("btn_pressed_sem creation failed\r\n");
    }

    miio_led_on();
    LOG_INFO("miio_app_thread while loop start.\r\n");

    while(NULL == otn_is_online())api_os_tick_sleep(100);

    u32_t startTime = api_os_tick_now();
    u32_t startTime2 = api_os_tick_now();
    while(1)
    {
        //propsDHT every TIME_PROPS_DHT
        u32_t currentTime = api_os_tick_now();
        if(currentTime - startTime > TIME_PROPS_DHT)
        {
            startTime = currentTime;
            propsDHT();
        }

        //checkDHT every TIME_CHECK_DHT
        u32_t currentTime2 = api_os_tick_now();
        if(currentTime2 - startTime2 > TIME_CHECK_DHT)
        {
            startTime2 = currentTime2;
            checkDHT();
        }

        //check button event
        if(WM_SUCCESS == os_semaphore_get(&btn_pressed_sem, OS_NO_WAIT))
        {
            if(g_button_pressed)
            {
                mum_set_event(g_mum, "button_pressed","\"wifi\"");
                g_button_pressed = 0;
            }
        }

        api_os_tick_sleep(20);
    }

    mum_destroy(&g_mum);
    os_timer_delete(&g_reset_prov_timer);
    os_semaphore_delete(&btn_pressed_sem);

}

/*
{
	"method":"get_prop",
	"params":["rgb","temperature","humidity"]
}
*/
int JSON_DELEGATE(get_prop)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    jsmn_node_t jn;
    char js_ret[120] = "";
    int param_idx = 0;
    char param_key[16];
    char param_value[16];
    size_t param_str_len;
    jsmntok_t *tkn = NULL;

    jn.js = js_ret;
    jn.tkn = NULL;
    jn.js_len = 0;


    jn.js_len += snprintf(js_ret + jn.js_len, sizeof(js_ret), "{\"result\":[");

    while (NULL != (tkn = jsmn_array_value(pjn->js, pjn->tkn, param_idx)))
    {
        jsmn_tkn2val_str(pjn->js, tkn, param_key, sizeof(param_key), &param_str_len);

        if (param_idx > 0)
            jn.js_len += snprintf(js_ret + jn.js_len, sizeof(js_ret), ",");

        mum_get_property(g_mum, param_key, param_value);
        jn.js_len += snprintf(js_ret + jn.js_len, sizeof(js_ret), "%s", param_value);

        param_idx++;
    }
    jn.js_len += snprintf(js_ret + jn.js_len, sizeof(js_ret), "]}");

    LOG_INFO("get_prop return:%s\r\n", js_ret);

    return ack(&jn, ctx);
}
USER_FSYM_EXPORT(JSON_DELEGATE(get_prop));
/*
{
	"method":"set_rgb",
	"params":[int(rgb)]
}
*/
int JSON_DELEGATE(set_rgb)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    char js_ret[120] = "";
    int param_idx = 0;
    unsigned int param_key;
    jsmntok_t *tkn = NULL;

    jsmn_node_t jn;
    jn.js = js_ret;
    jn.tkn = NULL;
    jn.js_len = 0;


    while (NULL != (tkn = jsmn_array_value(pjn->js, pjn->tkn, param_idx)))
    {
        jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
        param_idx++;
    }

    if(param_idx == 1)
    {
        u8_t r = (param_key >> 16) & 0x0000FF;
        u8_t g = (param_key >> 8) & 0x0000FF;
        u8_t b = (param_key) & 0x0000FF;
        setRGB(DM_PWM_FREQ,DM_PWM_THR,r,g,b);

        char str[8];

        snprintf(str,sizeof(str),"%d",param_key);
        mum_set_property(g_mum, "rgb",str);

        jn.js_len += snprintf(js_ret + jn.js_len, sizeof(js_ret), "{\"result\":[\"ok\"]}");
    }
    else
    {
        jn.js_len += snprintf(js_ret + jn.js_len, sizeof(js_ret), "{\"error\":{\"code\":-5001,\"message\":\"params error\"}}");
    }

    LOG_INFO("set_rgb return:%s\r\n", js_ret);

    return ack(&jn, ctx);
}
USER_FSYM_EXPORT(JSON_DELEGATE(set_rgb));


void miio_app_test_thread(void* arg)
{

    /*if(STATE_OK == sell_ready()){
        LOG_INFO("==================\r\n");
        LOG_INFO(" OK. Sell ready. \r\n");
        LOG_INFO("==================\r\n");
    }*/

    while(1)
    {
        os_thread_sleep(os_msec_to_ticks(500));
    }
}

void (*app_thread)(void* arg) = miio_app_thread;
void (*app_test_thread)(void* arg) = miio_app_test_thread;


// TO override the pin start mode in mc200_startup.c, so that LED are off at startup
void SetPinStartMode(void)
{
    GPIO_PinModeConfig(board_led_1().gpio, PINMODE_PULLUP);
    GPIO_PinModeConfig(board_led_2().gpio, PINMODE_PULLUP);
}


void product_set_wifi_cal_data(void)
{

}
