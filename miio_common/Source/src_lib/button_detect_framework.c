#include <lowlevel_drivers.h>
#include <wm_os.h>
#include <board.h>
#include "lib/button_detect_framework.h"
#include "appln_dbg.h"
#include "mdev_gpio.h"
#include "wmassert.h"
#include <pwrmgr.h>
#include <lib/power_mgr_helper.h>

extern int board_buttons[BUTTON_COUNT];
extern GPIO_Int_Type buttons_int_edge[BUTTON_COUNT];
extern GPIO_PinMuxFunc_Type buttons_pin_mux[BUTTON_COUNT];

#define DEBOUNCE_TICKS os_msec_to_ticks(20)

struct {
    os_timer_t debounce_timer;
    button_press_event_handle button_cb; 
    bool active;
}button_struct[BUTTON_COUNT];


void button_check(os_timer_arg_t handle)
{
    int button_num = (int) os_timer_get_context(&handle);
   
    if(button_num >= BUTTON_COUNT) {
        LOG_ERROR("get error button num");
    }

    bool button_pressed = false;

    GPIO_IO_Type pin_level = GPIO_ReadPinLevel(board_buttons[button_num]);
    //check pin level
    switch (buttons_int_edge[button_num])
    {
    case GPIO_INT_RISING_EDGE:
        if(pin_level == GPIO_IO_HIGH) {
            button_pressed = true;
        }
        break;
    case GPIO_INT_FALLING_EDGE:
        if(pin_level == GPIO_IO_LOW) {
            button_pressed = true;
        }
        break;
    case GPIO_INT_BOTH_EDGES:
        //we think hardware is roburest enough
        button_pressed = true;
        break;
    default:
        LOG_ERROR("params error \r\n");
    }

    if(button_pressed) {
        button_struct[button_num].button_cb(pin_level);
    } 

    if(button_struct[button_num].active) {
        button_struct[button_num].active = false;
    }

}

void button_debounce(int pin_num,void *data)
{
    int button_num;
    
    int i = 0;
    for(i = 0; i< BUTTON_COUNT;++i) {
        if(board_buttons[i] == pin_num){
            break;
        }
    }

    if(i>= BUTTON_COUNT) {
        LOG_ERROR("pin is not a button \r\n");
        return;
    }

    button_num = i;

    //timer already active 
    if(button_struct[button_num].active) {
        return;
    }
    
    os_timer_deactivate(&(button_struct[button_num].debounce_timer));
    
    if(WM_SUCCESS == os_timer_activate(&(button_struct[button_num].debounce_timer))) {
        button_struct[button_num].active = true;
     }
}   


int button_frame_work_init(void )
{
    memset(&button_struct,0,sizeof(button_struct));

    int i = 0;
    for(i = 0 ;i <BUTTON_COUNT;++i) {
        button_struct[i].debounce_timer = xTimerCreate((const char *)"debunce_timer", DEBOUNCE_TICKS,
				      OS_TIMER_ONE_SHOT, (void *)i, button_check);

        gpio_drv_set_cb(NULL, board_buttons[i],buttons_int_edge[i],NULL,button_debounce);

        // gpio _config
        GPIO_PinMuxFun(board_buttons[i], buttons_pin_mux[i]);
	    GPIO_SetPinDir(board_buttons[i], GPIO_INPUT);

        button_struct[i].active = false;
    }

    return 0;
}

void register_button_callback(int button_num,button_press_event_handle handle)
{
    ASSERT(button_num < BUTTON_COUNT);

    if(button_num >= BUTTON_COUNT)
    	return;

    if(handle) {
        button_struct[button_num].button_cb = handle;   
    }
}
