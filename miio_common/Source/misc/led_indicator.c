/*
 *  Copyright (C) 2008-2013, Marvell International Ltd.
 *  All Rights Reserved.
 */

#include <wmstdio.h>
#include <wm_os.h>
#include <board.h>
#include <misc/led_indicator.h>
#include <mi_smart_cfg.h>
#include <psm.h>
#include <main_extension.h>
#include <appln_dbg.h>

enum led_action{
	LED_ACTION_OFF = 0,
	LED_ACTION_ON,
	LED_ACTION_FADE_IN,
	LED_ACTION_FADE_OUT
};

/*-----------------------Global declarations----------------------*/
struct led_private_data {
	uint8_t led_no;
	uint8_t curr_state;
	os_timer_t timer;
	int on_duty_cycle;
	int off_duty_cycle;
	int locked;
	int super_locked;

	int action_num;
	int cur_action;
	uint8_t action[8];
	uint16_t duration[8];
} led_data[LED_COUNT];

static int decide_led_array_index(int led_no);

static void led_cb1(os_timer_arg_t handle)
{
	int tid = (int) os_timer_get_context(&handle);
	int cur;
	if (tid >= LED_COUNT) {
		return;
	}

	cur = (++led_data[tid].cur_action);
	if(cur >= led_data[tid].action_num)
		cur = led_data[tid].cur_action = 0;

	switch(led_data[tid].action[cur]) {
	case LED_ACTION_OFF:
		board_led_off(led_data[tid].led_no);
		break;
	case LED_ACTION_ON:
		board_led_on(led_data[tid].led_no);
		break;
	case LED_ACTION_FADE_IN:
		break;
	case LED_ACTION_FADE_OUT:
		break;
	default:
		break;
	}

	os_timer_change(&led_data[tid].timer,
			led_data[tid].duration[cur], -1);
	os_timer_activate(&led_data[tid].timer);
}

static void led_triple_blink(int led_no)
{
    int idx = decide_led_array_index(led_no);
    int err;

    if (idx == -WM_FAIL)
        return;

    if (led_data[idx].locked)
        return;

    if (led_data[idx].super_locked)
        return;

    led_data[idx].action_num = 6;
    led_data[idx].action[0] = LED_ON;
    led_data[idx].action[1] = LED_OFF;
    led_data[idx].action[2] = LED_ON;
    led_data[idx].action[3] = LED_OFF;
    led_data[idx].action[4] = LED_ON;
    led_data[idx].action[5] = LED_OFF;
    led_data[idx].duration[0] = 100;
    led_data[idx].duration[1] = 200;
    led_data[idx].duration[2] = 100;
    led_data[idx].duration[3] = 200;
    led_data[idx].duration[4] = 100;
    led_data[idx].duration[5] = 800;

    if (os_timer_is_running(&led_data[idx].timer)) {
        err = os_timer_delete(&led_data[idx].timer);
        if (err != WM_SUCCESS) {
            return;
        }
    }

    err = os_timer_create(&led_data[idx].timer,
                  "led-timer",
                  os_msec_to_ticks(led_data[idx].duration[0]),
                  led_cb1,
                  (void *)idx,
                  OS_TIMER_ONE_SHOT,
                  OS_TIMER_AUTO_ACTIVATE);
    if (err != WM_SUCCESS) {
        return;
    }
}

static void led_double_blink(int led_no)
{
	int idx = decide_led_array_index(led_no);
	int err;

	if (idx == -WM_FAIL)
		return;

    if (led_data[idx].locked)
        return;

    if (led_data[idx].super_locked)
        return;

	led_data[idx].action_num = 4;
	led_data[idx].action[0] = LED_ON;
	led_data[idx].action[1] = LED_OFF;
	led_data[idx].action[2] = LED_ON;
	led_data[idx].action[3] = LED_OFF;
	led_data[idx].duration[0] = 100;
	led_data[idx].duration[1] = 200;
	led_data[idx].duration[2] = 100;
	led_data[idx].duration[3] = 800;

	if (os_timer_is_running(&led_data[idx].timer)) {
		err = os_timer_delete(&led_data[idx].timer);
		if (err != WM_SUCCESS) {
			return;
		}
	}

	err = os_timer_create(&led_data[idx].timer,
			      "led-timer",
			      os_msec_to_ticks(led_data[idx].duration[0]),
			      led_cb1,
			      (void *)idx,
			      OS_TIMER_ONE_SHOT,
			      OS_TIMER_AUTO_ACTIVATE);
	if (err != WM_SUCCESS) {
		return;
	}
}

static int decide_led_array_index(int led_no)
{
	int i;

	if(led_no < 0)
		return -WM_FAIL;

	for (i = 0; i < LED_COUNT; i++) {
		if (led_data[i].led_no == led_no) {
			return i;
		} else if (led_data[i].led_no == 0) {
			led_data[i].led_no = led_no;
			return i;
		}
	}
	return -WM_FAIL;
}

void led_on(int led_no)
{
	int idx = decide_led_array_index(led_no);
	int ret;

	if (idx == -WM_FAIL)
		return;

	if (led_data[idx].locked)
		return;

	if (led_data[idx].super_locked)
		return;

	if (os_timer_is_running(&led_data[idx].timer)) {
		ret = os_timer_delete(&led_data[idx].timer);
		if (ret != WM_SUCCESS) {
			wmprintf("Unable to delete LED timer\n\r");
			return;
		}
	}
	led_data[idx].curr_state = LED_ON;
	board_led_on(led_no);
}

void led_off(int led_no)
{
	int idx = decide_led_array_index(led_no);
	int err;

	if (idx == -WM_FAIL)
		return;

	if (led_data[idx].locked)
		return;

	if (led_data[idx].super_locked)
		return;

	if (os_timer_is_running(&led_data[idx].timer)) {
		err = os_timer_delete(&led_data[idx].timer);
		if (err != WM_SUCCESS) {
			wmprintf("Unable to delete LED timer\n\r");
			return;
		}
	}
	led_data[idx].curr_state = LED_OFF;
	board_led_off(led_no);
}

static void led_cb(os_timer_arg_t handle)
{
	int tid = (int) os_timer_get_context(&handle);
	if (tid >= LED_COUNT) {
		return;
	}
	if (led_data[tid].curr_state == LED_ON) {
		board_led_off(led_data[tid].led_no);
		led_data[tid].curr_state = LED_OFF;
		os_timer_change(&led_data[tid].timer,
				led_data[tid].off_duty_cycle, -1);
		os_timer_activate(&led_data[tid].timer);
	} else {
		board_led_on(led_data[tid].led_no);
		led_data[tid].curr_state = LED_ON;
		os_timer_change(&led_data[tid].timer,
				led_data[tid].on_duty_cycle, -1);
		os_timer_activate(&led_data[tid].timer);
	}
}

void led_blink(int led_no, int on_duty_cycle, int off_duty_cycle)
{
	int err, idx;

	idx = decide_led_array_index(led_no);

	if (idx == -WM_FAIL)
		return;

	if (led_data[idx].locked)
		return;

	if (led_data[idx].super_locked)
		return;

	if (os_timer_is_running(&led_data[idx].timer)) {
		err = os_timer_delete(&led_data[idx].timer);
		if (err != WM_SUCCESS) {
			return;
		}
	}
	led_data[idx].on_duty_cycle = on_duty_cycle;
	led_data[idx].off_duty_cycle = off_duty_cycle;

	board_led_off(led_no);
	led_data[idx].curr_state = LED_OFF;

	err = os_timer_create(&led_data[idx].timer,
			      "led-timer",
			      os_msec_to_ticks(led_data[idx].on_duty_cycle),
			      led_cb,
			      (void *)idx,
			      OS_TIMER_ONE_SHOT,
			      OS_TIMER_AUTO_ACTIVATE);
	if (err != WM_SUCCESS) {
		return;
	}
}

void led_lock(int led_no)
{
	int idx = decide_led_array_index(led_no);

	if (idx == -WM_FAIL)
		return;

	led_data[idx].locked = 1;
}

void led_unlock(int led_no)
{
	int idx = decide_led_array_index(led_no);

	if (idx == -WM_FAIL)
		return;

	led_data[idx].locked = 0;
}

void led_super_lock(int led_no)
{
	int idx = decide_led_array_index(led_no);

	if (idx == -WM_FAIL)
		return;

	led_data[idx].super_locked = 1;
}

void led_super_unlock(int led_no)
{
	int idx = decide_led_array_index(led_no);

	if (idx == -WM_FAIL)
		return;

	led_data[idx].super_locked = 0;
}



void led_waiting_trigger(void)
{
	/* dual led */
	//wait for trigger, fast blink in orange
	led_fast_blink(board_led_2().gpio);
	led_off(board_led_1().gpio);

	/* single led */
	led_slow_blink(board_led_3().gpio);
}

void led_connecting(void)
{
	/* dual led */
	//connecting, fast blink in blue
	led_fast_blink(board_led_1().gpio);
	led_off(board_led_2().gpio);

	/* single led */
	led_fast_blink(board_led_3().gpio);
}

void led_connected(void)
{
	/* dual led */
	//connect success, blue
	led_on(board_led_1().gpio);
	led_off(board_led_2().gpio);

	/* single led */
	led_on(board_led_3().gpio);
}

void led_updating_fw(void)
{
    /* dual led */
    // updating firmware, slow blink in orange
    led_slow_blink(board_led_2().gpio);
    led_off(board_led_1().gpio);

    /* single led */
    led_double_blink(board_led_3().gpio);
}

void led_diagnosing(void)
{
    /* dual led */
    led_triple_blink(board_led_2().gpio);
    led_off(board_led_1().gpio);

    /* single led */
    led_triple_blink(board_led_3().gpio);
}

void miio_led_on(void)
{
	/* dual led */
	led_unlock(board_led_1().gpio);
	led_unlock(board_led_2().gpio);

	/* single led */
	led_unlock(board_led_3().gpio);
	led_network_state_observer(get_device_network_state());
}

void miio_led_off(void)
{
	/* dual led */
	led_off(board_led_1().gpio);
	led_lock(board_led_1().gpio);
	led_off(board_led_2().gpio);
	led_lock(board_led_2().gpio);

	/* single led */
	led_off(board_led_3().gpio);
	led_lock(board_led_3().gpio);
}

void led_stop()
{
    miio_led_off();
}

