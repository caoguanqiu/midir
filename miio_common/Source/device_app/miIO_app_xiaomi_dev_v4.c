/******************************************************************************
 *
 * Model: xiaomi.dev.v1
 *
 * Property:
 *
 * Method:
 *
 * Event:
 *
 *
 ******************************************************************************
 */
#include <lwip/lwipopts.h>
#include <lwip/arch.h>
#include <wm_os.h>
#include <ot/d0_otd.h>
#include <lib/jsmn/jsmn_api.h>
#include <lib/arch_os.h>
#include <lib/arch_io.h>
#include <lib/button_detect_framework.h>
#include <ot/lib_addon.h>
#include <misc/led_indicator.h>
#include <appln_dbg.h>
#include <board.h>
#include <mdev_gpt.h>
#include <lowlevel_drivers.h>
#include <psm.h>
#include <app_framework.h>
#include <main_extension.h>
#include <lib/miio_up_manager.h>
#include <lib/uap_diagnosticate.h>
#include "../../../../sdk/src/app_framework/app_ctrl.h"
#include "mw300_uart.h"
#include <mdev_uart.h>

#include <pwrmgr.h>
#include <boot_flags.h>
#include <lib/miio_down_manager.h>
#include <lib/miio_up_manager.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <communication.h>

#define MAX_REPORT_RETRY 2 


const int MIIO_APP_VERSION = 720;

static os_semaphore_t btn_pressed_sem;
static os_timer_t g_reset_prov_timer;
static int button_pressed = 0;

//static char check[20]={0xff,0xff,0x00,0x06,0x03,0x01,0x00,0x00,0x02,0x0c};
//OTN_ONLINE(my_online_notifier);
//OTN_OFFLINE(my_offline_notifier);
/*static d0_event_ret_t my_online_notifier(struct d0_event* evt, void* ctx)
{
        LOG_DEBUG("===>my online event \r\n");
}


static d0_event_ret_t my_offline_notifier(struct d0_event* evt, void* ctx)
{
        LOG_DEBUG("===>my offline event \r\n");
}
*/

int app_reset_configured_network()
{
	return app_ctrl_notify_event(AF_EVT_NORMAL_RESET_PROV, NULL);
}

static void reset_prov(os_timer_arg_t arg)
{
    wmprintf("start deprovisioning\r\n");
    if(is_provisioned()) {
        app_reset_configured_network();
    }
    else{
        pm_reboot_soc(); //reset again,just reboot
    }
}

static int make_button_pressed_report(char *msg, int msg_max_len)
{
    return snprintf(msg, msg_max_len, "{\"retry\":0,\"method\":\"props\",\"params\":{\"button_pressed\":\"wifi_rst\"}}");
}

static int report_button_pressed_ack(jsmn_node_t* pjn, void* ctx)
{
    int retry_count = (int) ctx;

    if (pjn && pjn->js && NULL == pjn->tkn) {
        if (retry_count < MAX_REPORT_RETRY) {
            char msg[100];
            int msg_len = make_button_pressed_report(msg, sizeof(msg));
            ot_api_method(msg, msg_len, report_button_pressed_ack, (void*) (retry_count + 1));
            LOG_WARN("report button pressed fail:%s. Retry.\r\n", pjn->js);
        } else {
            LOG_ERROR("report button pressed max retry failed.\r\n");
            button_pressed = 0;
        }
    } else {
        LOG_INFO("button pressed status up success.js:%s\r\n", pjn->js);
        button_pressed = 0;
    }

    return STATE_OK;
}

static int report_button_pressed(char *msg, int msg_len)
{
    if (otn_is_online())
        ot_api_method(msg, msg_len, report_button_pressed_ack, (void*) 0);
    return STATE_OK;
}

static void button_1_press_handle(GPIO_IO_Type pin_level)
{
    if (pin_level == GPIO_IO_LOW) {
        LOG_INFO("button pressed\r\n");

        if (os_timer_is_running(&g_reset_prov_timer)) {
            if (WM_SUCCESS != os_timer_deactivate(&g_reset_prov_timer)) {
                LOG_ERROR("deactivating reset provision timer failed\r\n");
            }
        }
		
        if(WM_SUCCESS != os_timer_activate(&g_reset_prov_timer)) {
			LOG_ERROR("activating reset provision timer failed\r\n");
		}

    } else {
        LOG_INFO("button released\r\n");
        button_pressed = 1;
        os_semaphore_put(&btn_pressed_sem);

        if (os_timer_is_running(&g_reset_prov_timer)) {
            if (WM_SUCCESS != os_timer_deactivate(&g_reset_prov_timer)) {
                LOG_ERROR("deactivating reset provision timer failed\r\n");
            }
        }
    }
}
static void set_model() {
    int ret;
    psm_handle_t handle;
    char model[64] = { 0 };
    char argv[64] = {0};
    strcpy(argv,"supor.ricecooker.v1");
    wmprintf("model is %s\r\n",argv);
    ret = psm_open(&handle, "ot_config");
    if (ret) {
        LOG_ERROR("Error opening PSM module\r\n");
        return;
    }
    if (WM_SUCCESS == psm_get(&handle, "psm_model", model, sizeof(model))) {
        if (0 != strncmp(model, argv, sizeof(model))) {
            strcpy(cfg_struct.model, argv);
            psm_set(&handle, "psm_model", argv);
        }
    }
    psm_close(&handle);
}
void miio_app_thread(void* arg)
{
	int count = 0, len;
	char temp;


    os_timer_create(&g_reset_prov_timer, "reset-prov-timer", os_msec_to_ticks(4000), reset_prov, NULL,
            OS_TIMER_ONE_SHOT, OS_TIMER_NO_ACTIVATE);
	
    LOG_INFO("set_model.\r\n");
    set_model();
    button_frame_work_init();
    register_button_callback(0, button_1_press_handle);

    if (WM_SUCCESS != os_semaphore_create(&btn_pressed_sem, "btn_pressed_sem")) {
        LOG_ERROR("btn_pressed_sem creation failed\r\n");
    }

    miio_led_on();
	
	//while(NULL == otn_is_online()) api_os_tick_sleep(100);
	
	mum_set_property(g_mum, "start", "\"off\"");
    mum_set_property(g_mum, "ingredients", "13");
    mum_set_property(g_mum, "convenient", "1");
	mum_set_property(g_mum, "pressure_set", "60");
	mum_set_property(g_mum, "cooking_time", "255");
	mum_set_property(g_mum, "recipe_basic_fun", "1");
	mum_set_property(g_mum, "date_minutes", "1440");
    mum_set_property(g_mum, "net_recipe_num", "1234");
	mum_set_property(g_mum, "cap_status", "10");
	mum_set_property(g_mum, "pressure_now", "245");
	mum_set_property(g_mum, "pressure_time", "255");
	mum_set_property(g_mum, "cooking_countdown", "255");
	mum_set_property(g_mum, "exhaust_countdown", "255");
	mum_set_property(g_mum, "cooking_state", "0");
    mum_set_property(g_mum, "error_0", "\"off\"");
    mum_set_property(g_mum, "error_1", "\"off\"");
    mum_set_property(g_mum, "error_2", "\"off\"");
    mum_set_property(g_mum, "error_3", "\"off\"");
    mum_set_property(g_mum, "error_4", "\"off\"");
	mum_set_property(g_mum, "error_5", "\"off\"");
    mum_set_property(g_mum, "error_6", "\"off\"");
    mum_set_property(g_mum, "error_7", "\"off\"");
	LOG_INFO("miio_app_thread while loop start.\r\n");	    
    while (1) {
        supor2mi_loop();
        if (WM_SUCCESS == os_semaphore_get(&btn_pressed_sem, OS_NO_WAIT)) {
            if (button_pressed) {
                char msg[100];
                int msg_len = make_button_pressed_report(msg, sizeof(msg));			
                report_button_pressed(msg, msg_len);
            }
        }
        os_thread_sleep(os_msec_to_ticks(10));//控制两贞之间的数据间隔为10ms.
    }
     
	mum_destroy(&g_mum);
    os_timer_delete(&g_reset_prov_timer);
    os_semaphore_delete(&btn_pressed_sem);

}
void miio_app_uart_thread(void* arg)
{
  int recv_len=0;
  int i=0;
  int recv_count;
  int recv_len_temp=0; 
  uint8_t recv_frame_flag=0;
  uint8_t *indata_buffer;
  indata_buffer = malloc(UartRecvBufferLen);

  while (1) {
    os_thread_sleep(os_msec_to_ticks(2));
	if(recv_len<UartRecvBufferLen)
	{
		recv_len_temp=0;
        recv_len_temp= uart_drv_read(dev,indata_buffer+recv_len, UartRecvBufferLen-recv_len);
		recv_len +=recv_len_temp;

	}

	if (recv_len_temp <= 0)
	 {
		if(recv_count<3)
		 {
			recv_count++;
		 }
		else		
		{
		  recv_frame_flag=1;
		}
	 }
	 else
	 {
	   recv_count=0;
	   recv_frame_flag=0;
	 }
	 if((recv_frame_flag)&&(recv_len>0))//have recived a frame data 
	 {
		  recv_len = get_original_data(indata_buffer,recv_len);
		  if(recv_len>0)
		  {
			LOG_INFO("recive data:\r\n");
			for(i=0;i<recv_len;i++)
			{
				wmprintf("%02X ",indata_buffer[i] );			
			}
			wmprintf("\r\n");
            if(check_lrc(indata_buffer,recv_len))
			{
				LOG_INFO("read data len:%d\r\n", recv_len);
				device_cmd_process(indata_buffer, recv_len);							
			}
			else
		    {
				LOG_INFO("lrc check error");
			}
		  }
		  else
		  {
		    LOG_INFO("revice data error");
		  }
		   
		   *(indata_buffer+STARTFRAME0)=0;
		   *(indata_buffer+STARTFRAME1)=0;
		   recv_len=0;
		   recv_frame_flag = 0;
	}
  }
  if(indata_buffer) free(indata_buffer);

}
void miio_app_test_thread(void* arg)
{
    /*if(STATE_OK == sell_ready()){
     LOG_INFO("==================\r\n");
     LOG_INFO(" OK. Sell ready. \r\n");
     LOG_INFO("==================\r\n");
     }*/

    while (1) {
        os_thread_sleep(os_msec_to_ticks(500));
    }
}

void (*app_thread)(void* arg) = miio_app_thread;
void (*app_uart_thread)(void* arg) = miio_app_uart_thread;
void (*app_test_thread)(void* arg) = miio_app_test_thread;


/*
{
    "method":"get_prop", //get_prop是必须实现的方法，用于查询属性。在开放平台可以下行发送这个命令
    "params":["rgb","temperature","humidity"] // 按照规范 RPC 参数为一个简单数组
}
*/


int JSON_DELEGATE(get_prop)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    jsmn_node_t jn;
    char js_ret[220] = "";
    int param_idx = 0;
    char param_key[26];
    char param_value[26];
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
	sem_get_prop = 1;
    return ack(&jn, ctx);
}

USER_FSYM_EXPORT(JSON_DELEGATE(get_prop)); //注册方法

/*
{
    "method":"get_prop", //get_prop是必须实现的方法，用于查询属性。在开放平台可以下行发送这个命令
    "params":["rgb","temperature","humidity"] // 按照规范 RPC 参数为一个简单数组
}
*/

int JSON_DELEGATE(cmd_fun_setting)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    char js_ret[220] = "";
    int param_idx = 0;
    u32_t param_key;
    jsmntok_t *tkn = NULL;

    jsmn_node_t jn;
    jn.js = js_ret;
    jn.tkn = NULL;
    jn.js_len = 0;

	char param_str[26];
	size_t param_str_len;

	portBASE_TYPE xStatus;

    while (NULL != (tkn = jsmn_array_value(pjn->js, pjn->tkn, param_idx)))
    {  
		
		if(param_idx == 0)
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
		   cmd_set.attr_flags = param_key;
		   LOG_INFO("cmd_fun_setting return 1:%02X \r\n",cmd_set.attr_flags);
		}
		else if((param_idx == 1)&&(cmd_set.attr_flags&0x00000001))
		{
		     jsmn_tkn2val_str(pjn->js, tkn, param_str, sizeof(param_str), &param_str_len);
     		 
			 if( 0 ==strcmp(param_str,"on"))
             {
				     cmd_set.start = 1;
                     LOG_INFO("cmd_fun_setting return 2:%s\r\n", param_str);
             }
			 else if( 0 ==strcmp(param_str,"off"))
			 {
				     cmd_set.start = 0;
				     LOG_INFO("cmd_fun_setting return 2:%s\r\n", param_str);
			 }		              
		}
		else if((param_idx == 2)&&(cmd_set.attr_flags&0x00000004))
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
           
		   cmd_set.ingredients = param_key;

		   LOG_INFO("cmd_fun_setting return 3:%02X \r\n", cmd_set.ingredients);
		}
		else if((param_idx == 3)&&(cmd_set.attr_flags&0x00000010))
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
		   		   cmd_set.convenient = param_key;
		   		   LOG_INFO("cmd_fun_setting return 4:%02X \r\n", cmd_set.convenient);
		}
		else if((param_idx == 4)&&(cmd_set.attr_flags&0x00000020))
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
 		   cmd_set.pressure_set = param_key;
				LOG_INFO("cmd_fun_setting return 5:%02X \r\n", cmd_set.pressure_set);
		}
		else if((param_idx == 5)&&(cmd_set.attr_flags&0x00000040))
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
		   		   	   cmd_set.cooking_time = param_key;
					   LOG_INFO("cmd_fun_setting return 6:%02X \r\n", cmd_set.cooking_time);
		}
		else if((param_idx == 6)&&(cmd_set.attr_flags&0x00000400))
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
		   		   		   cmd_set.recipe_basic_fun = param_key;
						   LOG_INFO("cmd_fun_setting return 7:%02X \r\n", cmd_set.recipe_basic_fun);
		}
		else if((param_idx == 7)&&(cmd_set.attr_flags&0x00000800))
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
		   cmd_set.date_minutes = param_key;
		   LOG_INFO("cmd_fun_setting return 8:%02X \r\n", cmd_set.date_minutes);
		}
		else if((param_idx == 8)&&(cmd_set.attr_flags&0x00002000))
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
		   cmd_set.net_recipe_num = param_key;
		  // LOG_INFO("cmd_fun_setting return 9:%d\r\n",cmd_set.net_recipe_num);
		}
		else if((param_idx == 9)&&(cmd_set.attr_flags&0x00002000))
		{
		   jsmn_tkn2val_uint(pjn->js, tkn, &param_key);
		   cmd_set.net_recipe_num = (cmd_set.net_recipe_num<<16)+param_key;
		   LOG_INFO("cmd_fun_setting return 9:%02X \r\n",cmd_set.net_recipe_num);
		}		
//		snprintf(str,sizeof(str),"%d",param_key);
        param_idx++;
    }
    if(param_idx == 10)
    {      
//        char str[8];

        //mum_set_property(g_mum, "ingredients",str);
         
        xStatus = xQueueSendToBack(xQueue , &cmd_set, 0);
		if(xStatus == pdPASS)
		{
		   LOG_INFO("send one frame data to queue\r\n");		
		}
        jn.js_len += snprintf(js_ret + jn.js_len, sizeof(js_ret), "{\"result\":[\"ok\"]}");
    }
    else
    {
        jn.js_len += snprintf(js_ret + jn.js_len, sizeof(js_ret), "{\"error\":{\"code\":-5001,\"message\":\"params error\"}}");
    }

    LOG_INFO("cmd_fun_setting return:%s\r\n", js_ret);

    return ack(&jn, ctx);
}
USER_FSYM_EXPORT(JSON_DELEGATE(cmd_fun_setting)); //注册方法


// TO override the pin start mode in mc200_startup.c, so that LED are off at startup
void SetPinStartMode(void)
{
    GPIO_PinModeConfig(board_led_1().gpio, PINMODE_PULLUP);
    GPIO_PinModeConfig(board_led_2().gpio, PINMODE_PULLUP);
}

void product_set_wifi_cal_data(void)
{

}
