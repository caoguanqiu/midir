/*
 *  Copyright (C) 2008-2013, Marvell International Ltd.
 *  All Rights Reserved.
 */

#ifndef __LED_INDICATOR__
#define __LED_INDICATOR__

#define LED_COUNT 3
enum led_state {
	LED_OFF = 0,
	LED_ON,
};

/** Switch ON LED */
void led_on(int led_no);

/** Switch OFF LED */
void led_off(int led_no);

/** Blink the LED
 * on_duty_cycle : time in milisec for which LED will be ON in blinking cycle
 * off_duty_cycle : time in milisec for which LED will be OFF in blinking cycle
 */
void led_blink(int led_no, int on_duty_cycle, int off_duty_cycle);

/** Fast Blink the LED
 *  Blink LED with on_duty_cycle = 200ms and off_duty_cycle = 200ms
 */
static inline void led_fast_blink(int led_on)
{
	led_blink(led_on, 100, 200);
}

/** Slow Blink the LED
 *  Blink LED with on_duty_cycle = 1000ms and off_duty_cycle = 1000ms
 */
static inline void led_slow_blink(int led_on)
{
	led_blink(led_on, 200, 800);
}

void led_lock(int led_no);
void led_unlock(int led_no);
void led_super_lock(int led_no);
void led_super_unlock(int led_no);

void led_waiting_trigger(void);
void led_connecting(void);
void led_connected(void);
void led_updating_fw(void);
void led_diagnosing(void);
void led_stop(void);

void miio_led_on(void);
void miio_led_off(void);

#endif /* ! __LED_INDICATOR__ */
