/*
 *  Copyright (C) 2008-2015, Marvell International Ltd.
 *  All Rights Reserved.
 */

/* System includes */
#include <wmstdio.h>
#include <string.h>
#include <wmtime.h>
#include <wm_os.h>
#include <wlan.h>
#include <pwrmgr.h>
#include <appln_dbg.h>
#include <app_framework.h>
#include "lib/power_mgr_helper.h"

/* When CPU is expected to be idle for more than this time (millisecond) in
 * future, low power state is entered.
 */
#define THRESHOLD_TIME 10

static unsigned int pm_cb_handle;

/*
 * Once the user has pressed the wake key this variable remembers this
 * event. The other modules of the application refer to this before
 * deciding to enable low modes.
 */
static bool wake_status;

static void hp_pm_cb(power_save_event_t event, void *data)
{
//if use this reature,wifi will never sleep after wakeup by wakeup pin
#if 0
	/* If Wake up source is external GPIO wake up key then we disable
	 * power manager framework and IEEE power save.
	 */
	if (pm_wakeup_source() & EGPIO0) {
		wake_status = true;
		pm_mcu_cfg(false, PM1, 0);
		pm_ieeeps_hs_cfg(false, 0);
	}
#endif 
}

bool hp_pm_get_wake_status(void)
{
	return wake_status;
}

void hp_pm_reset_wake_status()
{
	wake_status = false;
}

void hp_pm_set_wake_status()
{
	wake_status = true;
}

int hp_pm_init()
{
#if defined(CONFIG_CPU_MC200)
	/*
	 * GPIO pin 28 is used as connection indicator
	 * It is in IO domain D1.
	 * This call makes power management (PM2) entry  code to
	 * keep IO domain D1 on when MC200 enters PM2.
	 */
	pm_mcu_io_cfg(IO_DOMAIN_1);
#endif
	uint8_t mcu_state;
#if CONFIG_ENABLE_MCU_PM3
	mcu_state = PM3;
#else
	mcu_state = PM1;//PM2; FIXME: should enable PM2 in near future
#endif /* CONFIG_ENABLE_MCU_PM3 */

	pm_mcu_cfg(true, mcu_state, THRESHOLD_TIME);

	pm_cb_handle = pm_register_cb(ACTION_EXIT_PM2 | ACTION_EXIT_PM3,
			hp_pm_cb, NULL);
	return WM_SUCCESS;
}

void hp_pm_wifi_ps_enable()
{
	/* if power management is enabled enable IEEE Power save for
	 * WIFI card.
	 * IEEEPS and host sleep configuration needs to be send on
	 * every connection since it is lost on station gets
	 * disconnected.
	 */
	if (!hp_pm_get_wake_status() && is_sta_connected()) {
		LOG_DEBUG("Configuring IEEEPS");
		pm_ieeeps_hs_cfg(true, WAKE_ON_ARP_BROADCAST
				 | WAKE_ON_UNICAST
				 | WAKE_ON_MULTICAST
				 | WAKE_ON_MAC_EVENT
                 | WAKE_ON_ALL_BROADCAST
                 );
	}
}

void hp_pm_wifi_ps_disable()
{
	pm_ieeeps_hs_cfg(false, 0);
}
