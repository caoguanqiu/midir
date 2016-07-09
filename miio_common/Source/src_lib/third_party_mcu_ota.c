#include <stdio.h>
#include <stdint.h>

#include <psm.h>
#include <httpc.h>
#include <httpc_file_download.h>
#include <ot/lib_addon.h>
#include <lib/miio_command.h>
#include <lib/arch_os.h>
#include <task.h>
#include <appln_dbg.h>
#include <lib/third_party_mcu_ota.h>
#include <lib/ota.h>
#include <mdev_uart.h>
#include <malloc.h>
#include <call_on_worker_thread.h>
#include <main_extension.h>
#include <partition.h>
#include <rfget.h>
#include <firmware_structure.h>
#include <crc32.h>
#include <pwrmgr.h>

extern uint32_t crc32_flash(mdev_t *fl_drv, uint32_t offset, uint32_t len);

static mdev_t * mdev_uart_ = NULL;
static bool config_uart1(int baud)
{
	if(WM_SUCCESS != uart_drv_init(UART1_ID, UART_8BIT)) {
       LOG_ERROR("ota uart init fail"); 
       return false;
    }
    
	mdev_uart_ = uart_drv_open(UART1_ID, baud);
    if(!mdev_uart_) {
        LOG_ERROR("open ota uart error");
        return false;
    }

    return true;
}

/**
 * config uart2 for xmodem data transfer
 *
 * @zhangyanlu
 */
static bool config_uart2(int baud)
{
    if (WM_SUCCESS != uart_drv_init(UART2_ID, UART_8BIT)) {
        LOG_ERROR("ota uart init fail");
        return false;
    }

    mdev_uart_ = uart_drv_open(UART2_ID, baud);
    if (!mdev_uart_) {
        LOG_ERROR("open ota uart error");
        return false;
    }
    return true;
}

int dl_uart_init(){
    if(mdev_uart_ != NULL){ //ota transfering
        return -1;
    }
    if(!config_uart2(/*380400*/ 230400 /* 115200*/)){
        return -1;
    }
    return 0;
}

void dl_uart_deinit(){
    if (mdev_uart_) {
        uart_drv_close(mdev_uart_);
    }
    mdev_uart_ = NULL;
    uart_drv_deinit(UART2_ID);
}



static int ota_uart_init()
{
    if (mdev_uart_ != NULL ) { //dl transfering
        return -1;
    }
    //use xmodem to transmit data
    //reinit uart1
    if(!config_uart1(115200)) {
        return -1;
    }
    
    return 0;
}

int ota_uart_byte_read(unsigned char * byte)
{
    if(!mdev_uart_) {
        return 0;
    }
    return  uart_drv_read(mdev_uart_,byte,1);
}

void ota_uart_type_write(unsigned char trychar)
{
   
    if(!mdev_uart_) {
        return;
    }

    unsigned char buf[2];
	buf[0] = trychar;
        
    uart_drv_write(mdev_uart_,buf,1);
}

/**********************************************
* Transfer firmware in flash to mcu
***********************************************/
int mcu_ota_from_flash(struct partition_entry *from)
{
    int ret = 0;
    int crc;
    uint32_t bytes_copied  = 0;
    unsigned char * buf = NULL;
    struct user_fw_header uf_header;
    size_t firmware_size = 0;

    mdev_t *dev_from = flash_drv_open(from->device);
    if(NULL == dev_from) {
        ret = -1;
        LOG_ERROR("open flash error \r\n");
        goto handle_mcu_ota_from_flash_error;
    }

    if(flash_drv_read(dev_from, (uint8_t*)&uf_header, sizeof(uf_header), from->start) != 0) {
    	goto handle_mcu_ota_from_flash_error4;
    }
    firmware_size = uf_header.length;

    //do basic check
    if(from->size < firmware_size) {
        LOG_ERROR("firmware_size error \r\n");
        ret = -1;
        goto handle_mcu_ota_from_flash_error;
    }

    #define FLASH_READ_BUF_LEN  (1024)
    buf = malloc(FLASH_READ_BUF_LEN);
    if(!buf) {
        ret = -1;
        LOG_ERROR("malloc buf error \r\n");
        goto handle_mcu_ota_from_flash_error1;
    }
    
    //init uart1
    ret = ota_uart_init();
    if(ret != 0) {
        goto handle_mcu_ota_from_flash_error2;
    }

    ret = xmodem_get_crc_config(&crc);
    if(ret < 0) {
        goto handle_mcu_ota_from_flash_error3;
    }

    bytes_copied = 0;
    uint32_t size_to_read = 0;
    //read add transfer
    while(bytes_copied < firmware_size) {
       size_to_read = ((firmware_size - bytes_copied) > FLASH_READ_BUF_LEN) ? FLASH_READ_BUF_LEN : (firmware_size - bytes_copied);
        
       ret = flash_drv_read(dev_from, buf, size_to_read, from->start + sizeof(uf_header) + bytes_copied);
       if(ret != 0){
           goto handle_mcu_ota_from_flash_error4;
       }
      
       bytes_copied += size_to_read;
       
       if(bytes_copied < firmware_size) {
           if((size_to_read % XMODEM_MTU) != 0) {
               ret = -1;
               LOG_ERROR("read size error \r\n");
               goto handle_mcu_ota_from_flash_error4;
           }

           ret = xmodem_transfer_data(crc,buf,size_to_read,XMODEM_MTU);
           if(ret != 0) {
               goto handle_mcu_ota_from_flash_error3;
           }

       } else if(bytes_copied == firmware_size) {
           ret = xmodem_transfer_end(crc,buf,size_to_read,XMODEM_MTU);
           if(ret != 0) {
               goto handle_mcu_ota_from_flash_error3;
           }
       } else {
           ret = -1;
           LOG_ERROR("read flash size error \r\n");
           goto handle_mcu_ota_from_flash_error4;
       }

    }

    //correct return procedure
    goto handle_mcu_ota_from_flash_error3;

handle_mcu_ota_from_flash_error4:
    cancel_xmodem_transfer();
handle_mcu_ota_from_flash_error3:
    if(mdev_uart_) {
        uart_drv_close(mdev_uart_);
    }

    mdev_uart_ = NULL;
    uart_drv_deinit(UART1_ID);
handle_mcu_ota_from_flash_error2:
    if(buf) {
        free(buf);
        buf = NULL;
    }
handle_mcu_ota_from_flash_error1:
    flash_drv_close(dev_from);
handle_mcu_ota_from_flash_error:
    return ret;
}


/*
 * 1. Open UART-1 with 115200 8N1
 * 2. Respond to "get_down" with "down update_fw"
 * 3. Respond to all others with "error"
 * 4. Listen to UART-1 recv, if "result "ready"" found, then quit listening and return success
 * 5. Close UART-1 and exit
 * 6. If doesn't succeed in 10s, return timeout failure
 */
static int set_mcu_ready(void)
{
    uint8_t cmd[64];
    int bp = 0;
    mdev_t *dev = NULL;
    uint32_t time;
    int ret = WM_SUCCESS;
    const uint8_t updatefw_str[] = "down update_fw\r";
    const uint8_t error_str[] = "error\r";
    const uint8_t ok_str[] = "ok\r";

    if (WM_SUCCESS != uart_drv_init(UART1_ID, UART_8BIT)) {
        LOG_ERROR("uart1 init fail\r\n");
        ret = -WM_FAIL;
        goto out;
    }

    dev = uart_drv_open(UART1_ID, 115200);
    if (!dev) {
        LOG_ERROR("open uart1 error\r\n");
        ret = -WM_FAIL;
        goto out;
    }

    time = api_os_tick_now();
    while (1) {
        int bytes;
        if (1 == (bytes = uart_drv_read(dev, &cmd[bp], 1))) {
            if ('\r' == cmd[bp]) {
                if (strncmp((char*) cmd, "get_down", 8) == 0) {
                    uart_drv_write(dev, updatefw_str, strlen((char*) updatefw_str));
                    LOG_DEBUG("updatefw sent\r\n");
                } else if (strncmp((char*) cmd, "result \"ready\"", 14) == 0) {
                    LOG_DEBUG("result \"ready\" received!\r\n");
                    uart_drv_write(dev, ok_str, strlen((char*) ok_str));
                    break;
                } else if (strncmp((char*) cmd, "result \"busy\"", 13) == 0) {
                    LOG_DEBUG("result \"busy\" received!\r\n");
                    uart_drv_write(dev, ok_str, strlen((char*) ok_str));
                    ret = -WM_FAIL;
                    break;
                } else {
                    uart_drv_write(dev, error_str, strlen((char*) error_str));
                    LOG_DEBUG("error sent\r\n");
                }
                bp = 0;
                cmd[0] = '\0';
            } else {
                bp += bytes;
                if (bp >= sizeof(cmd))
                    bp = 0;
            }
        }

        api_os_tick_sleep(1);

        if (api_os_tick_elapsed(time) > 10000) {
            LOG_ERROR("mcu ota handshake timeout\r\n");
            ret = -WM_FAIL;
            break;
        }
    }

out:
    if (dev)
        uart_drv_close(dev);
    uart_drv_deinit(UART1_ID);
    return ret;
}

int transfer_mcu_fw(struct partition_entry *p)
{
    int ret = WM_SUCCESS;

    mcmd_destroy();
    if (WM_SUCCESS != set_mcu_ready()) {
        LOG_ERROR("set mcu to ready status failed\r\n");
        ret = -WM_FAIL;
        goto err_exit;
    }

    ret = mcu_ota_from_flash(p);
    if (ret < 0) {
        LOG_ERROR("mcu ota from flash failed(%d)\r\n", ret);
        ret = -WM_FAIL;
        goto err_exit;
    }

    mdev_t *dev = flash_drv_open(p->device);
    if (NULL == dev) {
        LOG_ERROR("open flash error \r\n");
        ret = -WM_FAIL;
        goto err_exit;
    }
    flash_drv_erase(dev, p->start, 4); // to invalidate mcu fw
    flash_drv_close(dev);

err_exit:
    mcmd_create(UART1_ID);
    return ret;
}

struct partition_entry * find_valid_mcu_fw(void)
{
    struct user_fw_header uf_header;
#if defined(CONFIG_CPU_MW300)
    struct partition_entry *p = rfget_get_mcu_firmware();
#elif defined(CONFIG_CPU_MC200)
    struct partition_entry *p = rfget_get_passive_firmware();
#else
#error not supported platform
#endif

    uint32_t calculated_crc = 0;
    uint32_t recv_crc = 0;

    if(NULL == p) {
        LOG_ERROR("get passive firmware failed\r\n");
        return NULL;
    }

    mdev_t *dev = flash_drv_open(p->device);
    if (NULL == dev) {
        LOG_ERROR("open flash error \r\n");
        return NULL;
    }

    if(flash_drv_read(dev, (uint8_t*)&uf_header, sizeof(uf_header), p->start) != 0) {
        LOG_ERROR("restore_mcu_fw, read mcu fw header failed\r\n");
        goto err_exit;
    }

    if(uf_header.magic != USER_FW_MAGIC) {
        LOG_INFO("mcu fw header not found, skip restoring\r\n");
        goto err_exit;
    }

    if(uf_header.length > p->size) {
        LOG_ERROR("mcu fw header corrupted\r\n");
        goto err_exit;
    }

    calculated_crc = crc32_flash(dev, p->start + sizeof(uf_header), uf_header.length);
    flash_drv_read(dev, (uint8_t*)&recv_crc, sizeof(recv_crc), p->start + sizeof(uf_header) + uf_header.length);
    if (calculated_crc != recv_crc) {
        LOG_ERROR("mcu fw crc failed\r\n");
        goto err_exit;
    }

    return p;

err_exit:
    flash_drv_close(dev);
    return NULL;
}

