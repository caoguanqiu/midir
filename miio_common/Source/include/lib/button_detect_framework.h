/*! \file button_detect_framework.h
 *  \brief 按键检测识别框架
 *
 *  主要提供了按键识别逻辑,按键事件回调等。
 *
 */

#ifndef _BUTTON_DETECT_FRAMEWORK_
#define _BUTTON_DETECT_FRAMEWORK_

#include <lowlevel_drivers.h>

/** 注册按键回调handle的定义
 *  
 *  该类型用在注册按键回调的接口上
 *
 * @param[in] GPIO_IO_Type type 发生按键事件时，按键对应的pin的电平
 *
 */
typedef void (*button_press_event_handle)(GPIO_IO_Type);
typedef void (*bt_detect_Type)(int );


void button_check(os_timer_arg_t handle);

void button_debounce(int button_num,void *data);
int button_frame_work_init(void );

/** 注册按键回调接口
 *  
 *  该接口用于注册按键事件的回调接口
 *  
 * @param[in] button_num  按键的编号
 * @param[in] handle 按键的处理函数
 *
 */
void register_button_callback(int button_num,button_press_event_handle handle);

#endif // _BUTTON_DETECT_FRAMEWORK_
