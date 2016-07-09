#include <wm_os.h>
#include <app_framework.h>
#include <wmtime.h>
#include <partition.h>
#include <appln_cb.h>
#include <appln_dbg.h>
#include <cli.h>
#include <cli_utils.h>
#include <wlan.h>
#include <rfget.h>
#include <psm.h>
#include <wmstdio.h>
#include <wmsysinfo.h>
#include <wm_net.h>
#include <mdev_aes.h>
#include <wlan.h>
#include <board.h>
#include <mdev_gpio.h>
#include <pwrmgr.h>
#include <stdlib.h>
#include <httpd.h>
#include <stdint.h>
#include <lib/lib_generic.h>
#include <lib/jsmn/jsmn_api.h>
#include <lib/cmp.h>
#include <lib/arch_os.h>
#include <wmassert.h>

#include <misc/led_indicator.h>
#include <misc/miio_cli.h>
#include <mi_smart_cfg.h>

#include <ot/lib_xobj.h>
#include <ot/lib_addon.h>
#include <ot/d0_otd.h>
#include <util/util.h>
#include <malloc.h>
#include "lib/button_detect_framework.h"
#include "util/dump_hex_info.h"
#include <lib/miio_command.h>
#include <lib/miio_chip_rpc.h>
#include <main_extension.h>
#include <lib/power_mgr_helper.h>
#include <boot_flags.h>
#include <lib/ota.h>
#include <lib/third_party_mcu_ota.h>
#include <lib/power_mgr_helper.h>
#include <miio_version.h>
#include <communication.h>

extern void (*app_thread)(void* arg);
extern void (*app_uart_thread)(void* arg);
extern void (*app_test_thread)(void* arg);
extern os_mutex_t network_state_mutex;
extern const int MIIO_APP_VERSION;

/* This function is defined for handling critical error.
 * For this application, we just stall and do nothing when
 * a critical error occurs.
 *
 */
void appln_critical_error_handler(void *data)
{
    while (1)
        api_os_tick_sleep(100);
    /* do nothing -- stall */
}

static void modules_init()
{
    int ret;
    struct partition_entry *p;
    flash_desc_t fl;

    /*
     * Initialize wmstdio prints
     */
    ret = wmstdio_init(UART0_ID, 115200);
    if (ret != WM_SUCCESS) {
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    /* Initialize time subsystem.
     *
     * Initializes time to 1/1/1970 epoch 0.
     */
    ret = wmtime_init();
    if (ret != WM_SUCCESS) {
        wmprintf("Error: wmtime_init failed");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    /*
     * Initialize CLI Commands
     */
    ret = cli_init();
    if (ret != WM_SUCCESS) {
        LOG_ERROR("Error: cli_init failed");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    /* Initialize the partition module */
    ret = part_init();
    if (ret != 0) {
        LOG_ERROR("Failed to initialize partition\r\n");
        appln_critical_error_handler((void *) -WM_FAIL);
    }
    p = part_get_layout_by_id(FC_COMP_PSM, NULL);
    if (!p) {
        LOG_ERROR("Error: no psm partition found");
        appln_critical_error_handler((void *) -WM_FAIL);
    }
    part_to_flash_desc(p, &fl);

#if defined CONFIG_CPU_MC200 && defined CONFIG_WiFi_8801
    //check psm _format
    psm_format_check(&fl);
#endif
    /* Initilize psm module */
    ret = app_psm_init();
    if (ret != 0) {
        LOG_ERROR("Failed to initialize psm module\r\n");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    wmprintf("\n\r");
    wmprintf("_|      _|  _|_|_|  _|_|_|    _|_|  \n\r");
    wmprintf("_|_|  _|_|    _|      _|    _|    _|\n\r");
    wmprintf("_|  _|  _|    _|      _|    _|    _|\n\r");
    wmprintf("_|      _|    _|      _|    _|    _|\n\r");
    wmprintf("_|      _|  _|_|_|  _|_|_|    _|_|  \n\r");
    print_versions();

    read_provision_status();

#ifndef RELEASE
    /* Initilize cli for psm module */
    ret = psm_cli_init();
    if (ret != 0) {
        LOG_ERROR("Failed to register psm-cli commands\r\n");
        appln_critical_error_handler((void *) -WM_FAIL);
    }
#endif

    ret = gpio_drv_init();
    if (ret != WM_SUCCESS) {
        LOG_ERROR("Error: gpio_drv_init failed");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    ret = aes_drv_init();
    if (ret != WM_SUCCESS) {
        LOG_ERROR("Error: aes drv init failed");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    ret = factory_cli_init();
    if (ret != 0) {
        LOG_ERROR("Error: factory_cli_init failed");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    ret = miio_chip_rpc_init();
    if (ret != 0) {
        LOG_ERROR("Error: miio_chip_rpc_cli_init failed");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

#ifndef RELEASE
    ret = appln_cli_init();
    if (ret != 0) {
        LOG_ERROR("Error: appln init failed");
        appln_critical_error_handler((void *) -WM_FAIL);
    }
#endif

    /* init add on interface */
    init_addon_interface();

    ret = ota_init();
    if (ret != 0) {
        LOG_ERROR("Error: ota init failed");
        appln_critical_error_handler((void *) -WM_FAIL);
    }

#ifdef MIIO_COMMANDS
    ret = mcmd_create(UART1_ID);
    if (ret < 0) {
        LOG_ERROR("Error: miio command init failed(%d)\r\n", ret);
        appln_critical_error_handler((void *) -WM_FAIL);
    }
#endif

    /*
	* Initialize Power Management Subsystem
	*/
	ret = pm_init();
	if (ret != WM_SUCCESS) {
		LOG_ERROR("Error: pm_init failed");
		appln_critical_error_handler((void *) -WM_FAIL);
	}

}

int main()
{
    int ret;

    	
	modules_init();//模块的初始化
    
	user_init();
    /* system tag data  */
    store_or_retrive_taged_config();
    
    get_mcu_version_from_psm();

    LOG_DEBUG("rst cause is %x\r\n", boot_reset_cause());

#ifdef MIIO_COMMANDS
    mcmd_enqueue_raw("MIIO_model_req");
    mcmd_enqueue_raw("MIIO_mcu_version_req");
#endif

    ret = os_mutex_create(&network_state_mutex,"network_state",OS_MUTEX_INHERIT);
    if(WM_SUCCESS != ret) {
        LOG_ERROR("create network_state_mutex fail");
        goto main_error;
    }

    register_device_network_state_observer(led_network_state_observer);
    
    /*set wifi calbration data*/
    product_set_wifi_cal_data();

    /* Start the application framework */
    if ((ret = app_framework_start(common_event_handler)) != WM_SUCCESS) {
        LOG_ERROR("Failed to start application framework.\r\n");
#if defined(CONFIG_CPU_MC200)
        if (-WLAN_ERROR_FW_DNLD_FAILED == ret || -WLAN_ERROR_FW_NOT_DETECTED == ret
                || -WLAN_ERROR_FW_NOT_READY == ret) {
            LOG_WARN("Wifi firmware broken, trying to recover.\r\n");
            if (recover_wifi_fw() != WM_SUCCESS) {
                LOG_FATAL("Recovering wifi fw failed!!!\r\n");
            } else {
                LOG_WARN("Recovering wifi fw success.\r\n");
                pm_reboot_soc();
            }
        }
#endif
        appln_critical_error_handler((void *) -WM_FAIL);
    }

    ret = is_factory_mode_enable();
    LOG_DEBUG("factory mode magic is %x\r\n" ,get_factory_mode_enable());
    if (0 == ret) {
        // user mode
        static xTaskHandle task_handle;
        if (app_thread) {
            xTaskCreate(app_thread, (signed char*) "app", 2000, NULL, APPLICATION_PRIORITY,
                    &task_handle);
            xTaskCreate(app_uart_thread, (signed char*) "app_uart", 2000, NULL, APPLICATION_PRIORITY,
                    &task_handle);
            LOG_INFO("App thread and App uart thread started.\r\n");
        }
    } else {
        // factory mode
        static xTaskHandle task_handle;
        if (app_test_thread) {
            xTaskCreate(app_test_thread, (signed char*) "test", 2000, NULL, 2,
                    &task_handle);
            LOG_INFO("Test thread started.\r\n");
        }
    }
    
    // Initialize power management
	hp_pm_init();
    
    //enter main loop, main loop will be used as a worker thread
    main_loop_run();

    //main loop return means error
main_error: 
    pm_reboot_soc(); //error occure on main thread , reboot
	return 1;
}




