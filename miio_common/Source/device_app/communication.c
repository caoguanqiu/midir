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

mdev_t *dev;
cmd_set_def_t cmd_set;
mum g_mum;
xQueueHandle xQueue;
uint8_t sem_get_prop;
uint8_t semnetnotify;
uint8_t onlineflag;
device_network_state_t state_config;
static device_status_t running_status;
static uint8_t buffer[UART_BUFFER_LEN];

void supor2mi_loop(void)
{
	uint32_t len;
	cmd_set_def_t cmd_set_receive;
    portBASE_TYPE xStatus;
    
	xStatus = xQueueReceive(xQueue, &cmd_set_receive, 0);
	if(xStatus == pdPASS)
	{
		LOG_INFO("received queue  data \r\n");
		len = send_check_command_to_device(0x03,0x01,&cmd_set_receive);
	}
	if(sem_get_prop)
	{
	    LOG_INFO("received get_prop sem \r\n");
        send_check_command_to_device(0x03,0x02,NULL);
		sem_get_prop = 0;
	}
	if(semnetnotify==1)
	{
		send_check_command_to_device(0x0d,0x00,NULL);
		semnetnotify = 0;
	}
  
}

void module2dev_loop(mdev_t *dev)
{
	
}

void user_init(void)
{
  xQueue = xQueueCreate(5,sizeof(cmd_set_def_t));
  if(xQueue != NULL)
  {
	 LOG_INFO("cmd_set_def_t queue have been created\r\n");
  }
  else
  {
     LOG_INFO("creating cmd_set_def_t queue failed\r\n");
  }
  LOG_INFO("set_mum.\r\n");
  g_mum = mum_create();
  LOG_INFO("uart1 init.\r\n");
  uart_drv_init(UART1_ID, UART_8BIT);
  dev = uart_drv_open(UART1_ID,9600);
}

void device_cmd_process(uint8_t *buf, int inlen)
{ 
  device_cmd_head_t *cmd_header;
  cmd_header = (device_cmd_head_t *)buf;
  uint8_t *p=NULL;
  char string[36];

  p = buf;
  if(((UPDEVICESTATUS == *(p+CMD))&&(*(p+LENLOW) != 5))||UPAUTOSTATUS == *(p+CMD))
  {
	  
  running_status.start.key = "start";
  if(cmd_header->start == 0)
  {
	  running_status.start.value = "\"off\"";
  }
  else if(cmd_header->start == 1)
  {
    running_status.start.value = "\"on\"";
  }
  mum_set_property(g_mum,running_status.start.key,running_status.start.value);  
 
  running_status.ingredients.key = "ingredients";
  LOG_INFO("cmd_fun ingredients:%d\r\n", cmd_header->ingredients);
  sprintf(string,"%d",cmd_header->ingredients);
  //ultoa(cmd_header->ingredients,string,10);//进制转换函数--转换一个无符号长整型数为任意进制的字符串
  LOG_INFO("cmd_fun ingredients string:%s\r\n",string);
  running_status.ingredients.value = string;
  mum_set_property(g_mum,running_status.ingredients.key,running_status.ingredients.value);
  
  running_status.convenient.key = "convenient";
  LOG_INFO("cmd_fun convenient:%d\r\n", cmd_header->convenient);
  sprintf(string,"%d",cmd_header->convenient);
  running_status.convenient.value = string;
  mum_set_property(g_mum,running_status.convenient.key,running_status.convenient.value);
  
  running_status.pressure_set.key = "pressure_set";
  LOG_INFO("cmd_fun pressure_set:%d\r\n", cmd_header->pressure_set);
  sprintf(string,"%d",cmd_header->pressure_set);
  running_status.pressure_set.value = string;
  mum_set_property(g_mum,running_status.pressure_set.key,running_status.pressure_set.value);

  running_status.cooking_time.key = "cooking_time";
  LOG_INFO("cmd_fun cooking_time:%d\r\n", cmd_header->cooking_time);
  sprintf(string,"%d",cmd_header->cooking_time);
  running_status.cooking_time.value = string;
  mum_set_property(g_mum,running_status.cooking_time.key,running_status.cooking_time.value);
 
  running_status.recipe_basic_fun.key = "recipe_basic_fun";
  LOG_INFO("cmd_fun recipe_basic_fun:%d\r\n", cmd_header->recipe_basic_fun);
  sprintf(string,"%d",cmd_header->recipe_basic_fun);
  running_status.recipe_basic_fun.value = string;
  mum_set_property(g_mum,running_status.recipe_basic_fun.key,running_status.recipe_basic_fun.value);

  running_status.date_minutes.key = "date_minutes";
  sprintf(string,"%d",((uint16_t)cmd_header->date_minutes_h)<<8 | cmd_header->date_minutes_l);
  LOG_INFO("cmd_fun date_minutes:%s\r\n", string);
  running_status.date_minutes.value = string;
  mum_set_property(g_mum,running_status.date_minutes.key,running_status.date_minutes.value);
 
  running_status.net_recipe_num.key = "net_recipe_num";
  sprintf(string,"%d",(uint32_t)cmd_header->net_recipe_num0<<24 | (uint32_t)cmd_header->net_recipe_num1<<16 | (uint32_t)cmd_header->net_recipe_num2<<8 | (uint32_t)cmd_header->net_recipe_num3);
  LOG_INFO("cmd_fun net_recipe_num:%s\r\n", string);
  running_status.net_recipe_num.value = string;
  mum_set_property(g_mum,running_status.net_recipe_num.key,running_status.net_recipe_num.value);
 
  running_status.cap_status.key = "cap_status";
  LOG_INFO("cmd_fun cap_status:%d\r\n", cmd_header->cap_status);
  sprintf(string,"%d",cmd_header->cap_status);
  running_status.cap_status.value = string;
  mum_set_property(g_mum,running_status.cap_status.key,running_status.cap_status.value);
 
 
  running_status.pressure_now.key = "pressure_now";
  LOG_INFO("cmd_fun pressure_now:%d\r\n", cmd_header->pressure_now);
  sprintf(string,"%d",cmd_header->pressure_now);
  running_status.pressure_now.value = string;
  mum_set_property(g_mum,running_status.pressure_now.key,running_status.pressure_now.value);
  
  running_status.pressure_time.key = "pressure_time";
  LOG_INFO("cmd_fun pressure_time:%d\r\n", cmd_header->pressure_time);
  sprintf(string,"%d",cmd_header->pressure_time);
  running_status.pressure_time.value = string;
  mum_set_property(g_mum,running_status.pressure_time.key,running_status.pressure_time.value);

  running_status.cooking_countdown.key = "cooking_countdown";
  LOG_INFO("cmd_fun cooking_countdown:%d\r\n", cmd_header->cooking_countdown);
  sprintf(string,"%d",cmd_header->cooking_countdown);
  running_status.cooking_countdown.value = string;
  mum_set_property(g_mum,running_status.cooking_countdown.key,running_status.cooking_countdown.value);

  running_status.exhaust_countdown.key = "exhaust_countdown";
  LOG_INFO("cmd_fun exhaust_countdown:%d\r\n", cmd_header->exhaust_countdown);
  sprintf(string,"%d",cmd_header->exhaust_countdown);
  running_status.exhaust_countdown.value = string;
  mum_set_property(g_mum,running_status.exhaust_countdown.key,running_status.exhaust_countdown.value);

  running_status.cooking_state.key = "cooking_state";
  LOG_INFO("cmd_fun cooking_state:%d\r\n", cmd_header->cooking_state);
  sprintf(string,"%d",cmd_header->cooking_state);
  running_status.cooking_state.value = string;
  mum_set_property(g_mum,running_status.cooking_state.key,running_status.cooking_state.value);

  running_status.error_0.key = "error_0";
  if(cmd_header->error&0x01)
  {
    sprintf(string,"%s","\"on\"");
  }
  else
  {
    sprintf(string,"%s","\"off\"");
  }
  LOG_INFO("cmd_fun error_0:%s\r\n", string);
  running_status.error_0.value = string;
  mum_set_property(g_mum,running_status.error_0.key,running_status.error_0.value);

  running_status.error_1.key = "error_1";
   if(cmd_header->error&0x02)
  {
    sprintf(string,"%s","\"on\"");
  }
  else
  {
    sprintf(string,"%s","\"off\"");
  }
  LOG_INFO("cmd_fun error_1:%s\r\n", string);
  running_status.error_1.value = string;
  mum_set_property(g_mum,running_status.error_1.key,running_status.error_1.value);

  running_status.error_2.key = "error_2";
   if(cmd_header->error&0x04)
  {
    sprintf(string,"%s","\"on\"");
  }
  else
  {
    sprintf(string,"%s","\"off\"");
  }
  LOG_INFO("cmd_fun error_2:%s\r\n",  string);
  running_status.error_2.value = string;
  mum_set_property(g_mum,running_status.error_2.key,running_status.error_2.value);

  running_status.error_3.key = "error_3";
  if(cmd_header->error&0x08)
  {
    sprintf(string,"%s","\"on\"");
  }
  else
  {
    sprintf(string,"%s","\"off\"");
  }
  LOG_INFO("cmd_fun error_3:%s\r\n",  string);
  running_status.error_3.value = string;
  mum_set_property(g_mum,running_status.error_3.key,running_status.error_3.value);

  running_status.error_4.key = "error_4";
   if(cmd_header->error&0x10)
  {
    sprintf(string,"%s","\"on\"");
  }
  else
  {
    sprintf(string,"%s","\"off\"");
  }
  LOG_INFO("cmd_fun error_4:%s\r\n", string);
  running_status.error_4.value = string;
  mum_set_property(g_mum,running_status.error_4.key,running_status.error_4.value);

  running_status.error_5.key = "error_5";
  if(cmd_header->error&0x20)
  {
    sprintf(string,"%s","\"on\"");
  }
  else
  {
    sprintf(string,"%s","\"off\"");
  }
  LOG_INFO("cmd_fun error_5:%s\r\n",  string);
  running_status.error_5.value = string;
  mum_set_property(g_mum,running_status.error_5.key,running_status.error_5.value);

  running_status.error_6.key = "error_6";
  if(cmd_header->error&0x40)
  {
    sprintf(string,"%s","\"on\"");
  }
  else
  {
    sprintf(string,"%s","\"off\"");
  }
  LOG_INFO("cmd_fun error_6:%s\r\n", string);
  running_status.error_6.value = string;
  mum_set_property(g_mum,running_status.error_6.key,running_status.error_6.value);

  running_status.error_7.key = "error_7";
  if(cmd_header->error&0x80)
  {
    sprintf(string,"%s","\"on\"");	
  }
  else
  {
    sprintf(string,"%s","\"off\"");
  }
  LOG_INFO("cmd_fun error_7:%s\r\n",string);
  running_status.error_7.value = string;
  mum_set_property(g_mum,running_status.error_7.key,running_status.error_7.value);
 }
	else if(( UPDEVICEINFO == *(p+CMD))&&(0x47 == *(p+LENLOW)))
	{
	   LOG_INFO("recive UPDEVICEINFO \r\n");
	}
    else if( UPHEARTBEAT == *(p+CMD))
	{
	   LOG_INFO("recive UPHEARTBEAT \r\n");
	}
	else if( UPNETCONFIG == *(p+CMD))
	{
	   send_check_command_to_device(RENETCONFIG,0,NULL);
	   LOG_INFO("recive UPNETCONFIG \r\n");


	}
	else if( UPRESETNET == *(p+CMD))
	{
     	send_check_command_to_device(RERESETNET,0,NULL);
		LOG_INFO("recive UPRESETNET \r\n");
	    wmprintf("start deprovisioning\r\n");
        if(is_provisioned()) 
	    {
			app_reset_saved_network();
		}
    }	
	else if( UPTESTMODE == *(p+CMD))
	{
	   send_check_command_to_device(RETESTMODE,0,NULL);
	   LOG_INFO("recive UPTESTMODE \r\n");

	}
	else if( UPBINDING == *(p+CMD))
	{
	   send_check_command_to_device(REBINDING,0,NULL);
	   LOG_INFO("recive UPBINDING \r\n");
	}

}


uint32_t send_check_command_to_device(uint8_t cmd,uint8_t action,cmd_set_def_t *cmd_receive)
{

  uint8_t *p;
  uint8_t temp=0;
  int i=0;
  device_cmd_head_t *p_out;
  p = (char *) buffer;//(device_cmd_head_t *)buffer;
  if(cmd == CHECKCTRSTATUS)
  {
   if(action == 0x01)
   {
      *p= 23;
	  *(p+1)= 0xFF;
	  *(p+2)= 0xFF;
	  *(p+3) = 0x00;
	  *(p+4) = 0x13;
	  *(p+5) = cmd;
	  *(p+6) = 0x01;
	  *(p+7) = 0x00;
	  *(p+8) = 0x00;
	  *(p+9) = action;
	 // LOG_INFO("cmd_receive->attr_flags:%d\r\n",cmd_receive->attr_flags);
	  *(p+10) = (uint8_t)((cmd_receive->attr_flags&0x00000001 | (cmd_receive->attr_flags&0x00000004)>>1|(cmd_receive->attr_flags&0x00000010)>>2 |
		  (cmd_receive->attr_flags&0x00000020)>>2 | (cmd_receive->attr_flags&0x00000040)>>2 | (cmd_receive->attr_flags&0x00000400)>>5 | (cmd_receive->attr_flags&0x00000800)>>5 | (cmd_receive->attr_flags&0x00002000)>>6));
	  *(p+11) = (uint8_t)cmd_receive->start;	  
	  *(p+12) = (uint8_t)cmd_receive->ingredients;	  
	  *(p+13) = (uint8_t)cmd_receive->convenient;	
	  *(p+14) = (uint8_t)cmd_receive->pressure_set;
	  *(p+15) = (uint8_t)cmd_receive->cooking_time;
	  *(p+16) = (uint8_t)cmd_receive->recipe_basic_fun;
	  *(p+17) = (uint8_t)((cmd_receive->date_minutes&0x0000FF00)>>8);
	  *(p+18) = (uint8_t)(cmd_receive->date_minutes&0x000000FF);
	  *(p+19) = (uint8_t)((cmd_receive->net_recipe_num&0xFF000000)>>24);
	  *(p+20) = (uint8_t)((cmd_receive->net_recipe_num&0x00FF0000)>>16);
	  *(p+21) = (uint8_t)((cmd_receive->net_recipe_num&0x0000FF00)>>8);
	  *(p+22) = (uint8_t)(cmd_receive->net_recipe_num&0x000000FF);
	  *(p+23) = calculate_lrc(p+3,*p-3);//SUM

   }
   else if(action == 0x02)
   {
	  *p= 10;
	  *(p+1)= 0xFF;
	  *(p+2)= 0xFF;
	  *(p+3) = 0x00;
	  *(p+4) = 0x06;
	  *(p+5) = cmd;
	  *(p+6) = 0x01;
	  *(p+7) = 0x00;
	  *(p+8) = 0x00;
	  *(p+9) = action;
	  *(p+10) = calculate_lrc(p+3,*p-3);  
	}
  }
  else if(cmd == CHECKDEVICEINFO)
  {
	
  }
  else if(cmd == REAUTOSTATUS)
  {
	
  }
  else if(cmd == HEARTBEAT)
  {
	
  }
  else if(cmd == RENETCONFIG)
  {
	*p= 9;
	*(p+1)= 0xFF;
	*(p+2)= 0xFF;
	*(p+3) = 0x00;
	*(p+4) = 0x05;
	*(p+5) = cmd;
    *(p+6) = 0x01;
	*(p+7) = 0x00;
	*(p+8) = 0x00;
    *(p+9) = calculate_lrc(p+3,*p-3);
  }
  else if(cmd == RERESETNET)
  {
	*p= 9;
	*(p+1)= 0xFF;
	*(p+2)= 0xFF;
	*(p+3) = 0x00;
	*(p+4) = 0x05;
	*(p+5) = cmd;
    *(p+6) = 0x01;
	*(p+7) = 0x00;
	*(p+8) = 0x00;
    *(p+9) = calculate_lrc(p+3,*p-3);

  }
  else if(cmd == WIFISTATUS)
  {
	 *p= 11;
	 *(p+1)= 0xFF;
	 *(p+2)= 0xFF;
	 *(p+3) = 0x00;
	 *(p+4) = 0x07;
	 *(p+5) = cmd;
	 *(p+6) = 0x01;
	 *(p+7) = 0x00;
	 *(p+8) = 0x00;
	if((state_config==WAIT_SMT_TRIGGER)||(state_config==UAP_WAIT_SMT))
	{
		temp &= 0x8F;
		temp |= 0x00;
	}
	else if(state_config==SMT_CONNECTING)
	{
		temp &= 0x8F;
		temp |= 0x10;
	}
	else if(state_config==GOT_IP_NORMAL_WORK)
	{
		temp &= 0x8F;
		temp |= 0x20;
		if(onlineflag==1)
	    {
		  temp |= 0x30;
	    }
	}
	else if(state_config==SMT_CFG_STOP)
	{
		temp &= 0x8F;
		temp |= 0x40;
	}
	

	 *(p+9) = 0x00;
	 *(p+10) = temp;
	 *(p+11) = calculate_lrc(p+3,*p-3);
  }
  else if(cmd == RETESTMODE)
  {
	*p= 9;
	*(p+1)= 0xFF;
	*(p+2)= 0xFF;
	*(p+3) = 0x00;
	*(p+4) = 0x05;
	*(p+5) = cmd;
    *(p+6) = 0x01;
	*(p+7) = 0x00;
	*(p+8) = 0x00;
    *(p+9) = calculate_lrc(p+3,*p-3);
  }
  else if(cmd == REBINDING)
  {
	*p= 9;
	*(p+1)= 0xFF;
	*(p+2)= 0xFF;
	*(p+3) = 0x00;
	*(p+4) = 0x05;
	*(p+5) = cmd;
    *(p+6) = 0x01;
	*(p+7) = 0x00;
	*(p+8) = 0x00;
    *(p+9) = calculate_lrc(p+3,*p-3);
  }
  
  buffer[0] = get_final_data(&buffer[1],buffer[0]);
  LOG_INFO("transmit data:\r\n");
  for(i=0;i<buffer[0];i++)
  {	 
	wmprintf("%02X ",buffer[i+1]);
  }
  wmprintf("\r\n");
  return uart_drv_write(dev, (uint8_t *)&buffer[1], buffer[0]);
}

uint8_t str_value_tohex(u8 func, const char *valueStr)
{  
  return atoi(valueStr);
}


/**************************************************************************************************
* 函数原型 : int get_final_data(uint8_t *inbuf, uint8_t *outbuf, int inlen)
* 功    能 : 按协议定义，非帧头的数据ff后面会插入0x55 ，此处将0x55 去掉还原数据帧的本来面目
* 入口参数 : inbuf 接收到未经处理的数据缓存
   			 inlen 需要处理的数据长度
* 出口参数 : void
* 返    回 :	<0 
				>0
				
* 全局变量 : 
* 说    明 : 
**************************************************************************************************/
int get_original_data(uint8_t *inbuf, int inlen)
{
  int i=0;
  int current_cur=2; 
  int check_len=0;
  check_len=inlen-2;
  if((STARTFF == *inbuf)&&(STARTFF == *(inbuf+1)))
  {
	 for(i=0;i<check_len;i++)
	 {
        if(*(inbuf+current_cur) == STARTFF && *(inbuf+current_cur+1) == INSERTDATA)
		{
			one_data_back(inbuf+current_cur+1, check_len+2-(current_cur+1));	
			check_len--;
		}		
		current_cur++;  	
	 }
	 return check_len+2;
  }
  
  return 0;
}


int get_final_data(uint8_t *inbuf, int inlen)
{
  int i=0;
  int pre_cur=2;
  int current_cur=3; 
  int check_len=0;
  check_len=inlen-2;
  if((STARTFF == *inbuf)&&(STARTFF == *(inbuf+1)))
  {
	for(i=0;i<check_len;i++)
	{
		if(*(inbuf+2+i)==STARTFF)
		{
			insert_toback(inbuf+2+i,check_len-1-i,INSERTDATA);
			check_len++;
		}
	}
	return check_len+2;
  }
    return 0;
}

uint8_t calculate_lrc(uint8_t *inbuf,int inlen)
{
   uint8_t temp=0;
   int   icount=0;
   for(icount=0;icount<inlen;icount++)
   {
      temp += *(inbuf+icount);
   }
   return temp;
}

int check_lrc(uint8_t *inbuf,int inlen)
{
	int ret;  
	if(calculate_lrc(&inbuf[2], inlen-3) == *(inbuf+inlen-1))
	{
	  ret = 1;
	}
	else
	{
		ret =0;
	}

	return ret;
}


void one_data_back(uint8_t *inbuf,int len)
{
	int i=0;

    for(i=0;i<len;i++)
	{
	  *(inbuf+i) = *(inbuf+i+1);
	}	
}
/**************************************************************************************************
* 函数原型 : int insert_toback(uint8_t *pos,int move_data_len,uint8_t data)
* 功    能 : 在指定位置的后面插入数据0x55
* 入口参数 : pos 需要插入数据的位置
   			 move_data_len 插入数据出到数据结尾的长度
* 出口参数 : 
* 返    回 :	<0 
				>0
				
* 全局变量 : 
* 说    明 : 
**************************************************************************************************/
void insert_toback(uint8_t *pos,int move_data_len,uint8_t data)
{
	int i=0;
	for(i=move_data_len;i>0;i--)
	{
		*(pos+i+1) = *(pos+i);
	}

	*(pos+1)=data;    
}
