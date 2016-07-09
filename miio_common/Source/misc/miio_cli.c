#include <cli.h>
#include <cli_utils.h>
#include <wmstdio.h>
#include <misc/miio_cli.h>
#include <pwrmgr.h>
#include <boot_flags.h>
#include <psm.h>
#include <appln_dbg.h>

static uint32_t cmd_factory_mode_enable  __attribute__((section(".nvram_uninit")));
static uint32_t cmd_factory_mode_enable_flag = 0;

void cmd_enter_factory_mode(int argc, char **argv) {
    cmd_factory_mode_enable = FACTORY_MAGIC;
    pm_reboot_soc();
}

uint32_t get_factory_mode_enable(void){
    return cmd_factory_mode_enable_flag;
}

uint32_t is_factory_mode_enable(void){
    return cmd_factory_mode_enable_flag == FACTORY_MAGIC ? 1 : 0;
}

/*
for factory use only. not currently implemented
#define STR_BUF_SZ (120)
set_did 8CBEBE000001|0000000011223344|abcdeabcdeabcdefabcdeabcdeabcdef
void cmd_set_did(int argc, char ** argv) {
    if (argc == 2 && strlen(argv[1]) == 62 && argv[1][12] == '|' && argv[1][29] == '|' ) {

        char strmac[13] = { 0 };
        char strkey[33] = { 0 };
        char strdid[17] = { 0 };

        int ret = 0;
        psm_handle_t handle;
        char strval[STR_BUF_SZ] = { '\0' };

        memcpy(strmac, argv[1], 12);
        memcpy(strdid, argv[1] + 13, 16);
        memcpy(strkey, argv[1] + 30 , 32 );

        ret = psm_open(&handle, "ot_config");
        if(ret){
            return;
        }
        ret = psm_get(&handle,"psm_mac",strval,STR_BUF_SZ);
        if(ret){
            LOG_ERROR("Error get psm mac\r\n");
            goto error_psm;
        }
        if(0 != strcmp(strval,"000000000000")){//only zero mac can be write only once
            goto error_psm;
        }

        ret = psm_set(&handle,"psm_did",strdid);
        if(ret){
            LOG_ERROR("Error writing psm did\r\n");
            goto error_psm;
        }
        ret = psm_set(&handle,"psm_key",strkey);
        if (ret) {
            LOG_ERROR("Error writing psm key\r\n");
            goto error_psm;
        }
        ret = psm_set(&handle,"psm_mac",strmac);
        if (ret) {
            LOG_ERROR("Error writing psm mac\r\n");
            goto error_psm;
        }
        wmprintf(FACTORY_SET_DID);
error_psm:
        psm_close(&handle);

    }
}

*/

static void showmem(){
    const heapAllocatorInfo_t *hI = getheapAllocInfo();
        wmprintf("mem=%d/%d\r\n", hI->freeSize, hI->heapSize);
}
static struct cli_command cmd_factory_mode_tests[] = { { "miio-test",
        "(enter factory mode)", cmd_enter_factory_mode },
        { "mem",
                "(show mem)", showmem },
};

int factory_cli_init(void) {
    if( PMU_CM3_SYSRESETREQ != boot_reset_cause()){
        cmd_factory_mode_enable = 0;//none software reboot,clear flag,normal boot
    }
    cmd_factory_mode_enable_flag = cmd_factory_mode_enable; //store this boot

    cmd_factory_mode_enable = 0;//next boot normal
    return cli_register_commands(cmd_factory_mode_tests,
            sizeof(cmd_factory_mode_tests) / sizeof(struct cli_command));
}
