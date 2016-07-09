#include <stdio.h>
#include <stdint.h>
#include <appln_dbg.h>
#include <malloc.h>
#include <wm_utils.h>
#include <flash.h>
#include <lowlevel_drivers.h>

#define PSM_SIGNATURE "PSM1"
#define PRODUCT_NAME "WMC"
#define PSM_PROGRAM_VERSION 1
#define PARTITION_SIZE (1 << 12) /* 4KB */

//FIXME: fixed 4 * 4k = 16 k content
#define FLASH_16K_BLOCK_SIZE       0x4000   /*!< 16KB */
#define FLASH_LAST_16K_BLOCK       ((FLASH_CHIP_SIZE/FLASH_16K_BLOCK_SIZE) - 1)
#define FLASH_LAST_16K_START       (FLASH_LAST_16K_BLOCK * FLASH_16K_BLOCK_SIZE)

#define TRY_WRITE_COUNT (3)

/** The psm in-memory buffer of one partition */
typedef struct psm_partition_cache {
	/** Holds partition data */
	char buffer[PARTITION_SIZE];
	/** Partition number */
	int partition_num;
} psm_partition_cache_t;

typedef struct psm_metadata {
	/** Signature of metadata */
	unsigned int signature;
	/** CRC of metadata  */
	unsigned int crc;
	/** version of metadata */
	unsigned int version;
	#define METADATA_ENV_SIZE (PARTITION_SIZE - 3*sizeof(unsigned int))
	/** Holds contents of metadata blocks */
	unsigned char vars[METADATA_ENV_SIZE];
} psm_metadata_t;

typedef struct psm_var_block {
	/** CRC of variable header of PSM  */
	unsigned int crc;
	#define ENV_SIZE (PARTITION_SIZE - sizeof(unsigned int))
	/** Holds environment variables of the PSM block */
	unsigned char vars[ENV_SIZE];
} psm_var_block_t;


typedef uint16_t objectid_t;
typedef uint16_t object_type_t;

#define OBJECT_TYPE_INVALID (object_type_t)0x0000
#define OBJECT_TYPE_SMALL_BINARY (object_type_t)0xAA55
#define OBJECT_TYPE_SMALL_BINARY_MAX_SIZE (uint16_t)(~0)
/* Every PSM object has this structure on the flash */
PACK_START typedef struct {
	object_type_t object_type;
	/** Flags:
	 * [7-3]: Reserved
	 * [2]  : Index request
	 * [1]  : Cache request
	 * [0]  : Active status
	 */
	uint8_t flags;
	/** CRC32 of entire psm object including metadata */
	uint32_t crc32;
	/** Length of the data field */
	uint16_t data_len;

	/** Object ID: A unique id for the object name ASCII string */
	objectid_t obj_id;
	/** Length of object name. If this is zero the object has been
	 * seen before and obj_id->name mapping is present. X-Ref: macro
	 * PSM_MAX_OBJNAME_LEN
	 */
	uint8_t name_len;
	/** Variable length object name string. */
	/* uint8_t name[<name_len>]; */
	/** Binary data here. Length == psm_object.data_len member above */
	/* uint8_t bin_data[<len>]; */
} PACK_END psm_object_t;

//global variable
static mdev_t * fl_dev = NULL;
static flash_desc_t psm_fl;
static psm_partition_cache_t * psm_read_cache = NULL;

static int psm_crc_verify(int partition_id, char *buffer)
{
	psm_metadata_t *metadata = (psm_metadata_t *) buffer;
	psm_var_block_t *var_block = (psm_var_block_t *) buffer;
	unsigned int crc;

	if (partition_id == 0) {
		crc = soft_crc32(metadata->vars, METADATA_ENV_SIZE, 0);
		if (crc != metadata->crc) {
			LOG_ERROR("CRC mismatch while reading partition %d. Expected: %x, Got: %x \r\n",partition_id, metadata->crc, crc);
			return -WM_FAIL;
		}
	} else {
		crc = soft_crc32(var_block->vars, ENV_SIZE, 0);
		if (crc != var_block->crc) {
			LOG_ERROR("CRC mismatch while reading partition %d. Expected: %x, Got: %x \r\n",partition_id, var_block->crc, crc);
			return -WM_FAIL;
		}
	}

	return WM_SUCCESS;

}


static int is_psm1_format(psm_partition_cache_t *psm_read_cache)
{
    int ret;

	psm_metadata_t *psm_metadata = (psm_metadata_t *) psm_read_cache->buffer;
    //do this check before, to avoid useless crc check operation
    if(0 != memcmp(&psm_metadata->signature, PSM_SIGNATURE, sizeof(PSM_SIGNATURE) - 1)) {
        return -WM_FAIL;
    }

    //treated as psm 1 format,
	ret = psm_crc_verify(0, psm_read_cache->buffer);
	if (!ret && (0 == memcmp(&psm_metadata->signature, PSM_SIGNATURE, sizeof(PSM_SIGNATURE) - 1))
            && (PSM_PROGRAM_VERSION == psm_metadata->version)) {

        psm_read_cache->partition_num = 0;
        return WM_SUCCESS;
    }

    psm_read_cache->partition_num = -1;
    return -WM_FAIL;
}

int copy_psm_to_last_16kblcok()
{
    int ret;

    if(!fl_dev) {
        return -WM_FAIL;
    }

    if(!psm_read_cache) {
        return -WM_FAIL;
    }

    flash_drv_erase(fl_dev, FLASH_LAST_16K_START, FLASH_16K_BLOCK_SIZE);
    
    int i = 0;
    for(i = 0; i < (FLASH_16K_BLOCK_SIZE / PARTITION_SIZE); ++i) {
        ret = flash_drv_read(fl_dev, (uint8_t *)psm_read_cache->buffer, PARTITION_SIZE, psm_fl.fl_start + (i * PARTITION_SIZE));
        if(ret != WM_SUCCESS) {
            return -WM_FAIL;
        }

        ret = psm_crc_verify(i,psm_read_cache->buffer);
        if(ret != WM_SUCCESS) {
            LOG_ERROR("psm content error , error block id is %d\r\n",i);
            return -WM_FAIL;
        }

        int j = 0;
        for(j = 0; j < TRY_WRITE_COUNT; ++j) {
            ret = flash_drv_write(fl_dev, (const uint8_t *)psm_read_cache->buffer,PARTITION_SIZE, FLASH_LAST_16K_START + (i * PARTITION_SIZE));
            if(ret != WM_SUCCESS) {
                return -WM_FAIL;
            }
            
            ret = flash_drv_read(fl_dev, (uint8_t *)psm_read_cache->buffer, PARTITION_SIZE, FLASH_LAST_16K_START + (i * PARTITION_SIZE));
            if((WM_SUCCESS == ret) && WM_SUCCESS == psm_crc_verify(i,psm_read_cache->buffer)) {
                //copy success
                break;
            }
        }
    }

    LOG_DEBUG("copy psm content success \r\n");
    return WM_SUCCESS;

}

int convert_psm1_2_psm2()
{
    int ret = WM_SUCCESS;
    int i = 1;


    if(!fl_dev) {
        return -WM_FAIL;
    }

    if(!psm_read_cache) {
        return -WM_FAIL;
    }

    char * psm_write_cache = (char *)malloc(PARTITION_SIZE);
    if(!psm_write_cache) {
        return -WM_FAIL;
    }
    
    memset(psm_write_cache,0xff,PARTITION_SIZE);

    //record the write position in 4k buffer
    int write_addr = 0;
    objectid_t obj_id = 0;

    //erase psm contents
	ret = flash_drv_erase(fl_dev, psm_fl.fl_start, psm_fl.fl_size);
    if(ret != WM_SUCCESS) {
        goto error_handle;
    }

    //convert last 16k content to psm2 format
    for(i = 1; i < (FLASH_16K_BLOCK_SIZE / PARTITION_SIZE); ++i) {
        ret = flash_drv_read(fl_dev, (uint8_t *)psm_read_cache->buffer, PARTITION_SIZE, FLASH_LAST_16K_START + (i *PARTITION_SIZE));
        if((ret == WM_SUCCESS) && (WM_SUCCESS == psm_crc_verify(i,psm_read_cache->buffer))) {
            psm_var_block_t *var_block = (psm_var_block_t *) psm_read_cache->buffer;
            char * data = (char *)var_block->vars;
            
            char patten[5];
            strncpy(patten,PRODUCT_NAME,sizeof(patten));
            psm_object_t psm_obj;

            while((data[0] == patten[0]) && (data[1] == patten[1]) &&  (data[2] == patten[2]) && (data[3] == '.')) {
                data += 4; // strlen("WMC.")

                memset(&psm_obj,0xff,sizeof(psm_object_t));
                //init psm object
                psm_obj.object_type = OBJECT_TYPE_SMALL_BINARY;
                psm_obj.flags = 0xff;
                obj_id ++;
                psm_obj.obj_id = obj_id;

                int temp = write_addr;
                write_addr += sizeof(psm_object_t);

                int leng_count = 0;
                //copy name
                while((*data != '=') && ((uint32_t)(data - (char *)(var_block->vars)) < (PARTITION_SIZE - sizeof(int))) && (write_addr < PARTITION_SIZE)) {
                    psm_write_cache[write_addr++] = *data++;
                    leng_count++;
                }

                if(*data != '=') {
                    write_addr = temp; //rollback
                    continue;
                }
                    
                //init leng_count
                psm_obj.name_len = leng_count;

                data++; //skip '='
                leng_count = 0;
                //copy data
                while((*data != 0) && ((uint32_t)(data - (char *)(var_block->vars)) < (PARTITION_SIZE - sizeof(int))) && (write_addr < PARTITION_SIZE)) {
                    psm_write_cache[write_addr++] = *data++;
                    leng_count++;
                }

                if(*data != 0) {
                    write_addr = temp; //rollback
                    continue;
                }

                data++; //skip '0'
                psm_obj.data_len = leng_count;

                //calculate crc
                uint32_t temp_crc;
                //calc name
                temp_crc = soft_crc32(psm_write_cache + temp + sizeof(psm_object_t), psm_obj.name_len,0);
                //calc value
                temp_crc = soft_crc32(psm_write_cache + temp + sizeof(psm_object_t) + psm_obj.name_len,psm_obj.data_len,temp_crc);
                //calc metadata
                psm_obj.crc32 = soft_crc32(&psm_obj,sizeof(psm_object_t),temp_crc);
                //copy psm_obj to write buffer
                memcpy(psm_write_cache+temp,&psm_obj,sizeof(psm_object_t));
            }
        }
    }

    for(i = 0; i < TRY_WRITE_COUNT; ++i) {
        //write whole buffer to psm area
        ret = flash_drv_write(fl_dev,(const uint8_t *)psm_write_cache,PARTITION_SIZE,psm_fl.fl_start);
        if(ret == WM_SUCCESS) {
            ret = flash_drv_read(fl_dev, (uint8_t *)psm_read_cache->buffer, PARTITION_SIZE, psm_fl.fl_start);
            if((ret == WM_SUCCESS) && (0 == memcmp(psm_write_cache,psm_read_cache->buffer,PARTITION_SIZE))) {
                LOG_DEBUG("update psm2 success \r\n");
                flash_drv_erase(fl_dev, FLASH_LAST_16K_START, FLASH_16K_BLOCK_SIZE);
                break;
            }
        }
    }

error_handle:
    if(psm_write_cache) {
        free(psm_write_cache);
        psm_write_cache = NULL;
    }

    return ret;

}

/***********************************************
* check flash state,
***********************************************/
int psm_format_check(flash_desc_t *fl)
{
    int ret = WM_SUCCESS;
    
    psm_fl = *fl;

    //fixed 16k space
    if(psm_fl.fl_size != FLASH_16K_BLOCK_SIZE) {
        LOG_ERROR("psm partition size error \r\n");
        return -WM_FAIL;
    }

	/* Initialise internal/external flash memory */
	flash_drv_init();

	/* Open the flash */
	fl_dev = flash_drv_open(psm_fl.fl_dev);
	if (fl_dev == NULL) {
		LOG_ERROR("Flash driver init is required before open");
		return -WM_FAIL;
	}
    
    /* read content form psm format 1, during start malloc 4k is easy*/
    psm_read_cache = (psm_partition_cache_t *)malloc(sizeof(psm_partition_cache_t));
    if(!psm_read_cache) {
        ret = -WM_FAIL;
        goto exit_handle;
    }

    //check the last 4 blocks, is there a backup ?
    ret = flash_drv_read(fl_dev, (uint8_t *)psm_read_cache->buffer, PARTITION_SIZE, FLASH_LAST_16K_START);
    if((ret == WM_SUCCESS) && (WM_SUCCESS == is_psm1_format(psm_read_cache))) {
        ret = convert_psm1_2_psm2();
        goto exit_handle;
    }

    //last blocks is not psm format,so check psm area
    ret = flash_drv_read(fl_dev, (uint8_t *)psm_read_cache->buffer, PARTITION_SIZE, psm_fl.fl_start);
	if (ret == WM_SUCCESS) {
        //Is psm-2 format?
        psm_object_t *v2_obj = (psm_object_t *)(psm_read_cache->buffer);
        if(v2_obj->object_type == OBJECT_TYPE_SMALL_BINARY) {
            //is correct psm-2 format, so do nothing
            goto exit_handle;
        }

        //check if is psm1 format
        if(WM_SUCCESS == is_psm1_format(psm_read_cache)) {
            
            //version check success so we should copy the four blocks to the end of internal flash
            ret = copy_psm_to_last_16kblcok();
            if(ret != WM_SUCCESS) {
                goto exit_handle;
            }
            //convert psm1 content to psm2 content
            ret = convert_psm1_2_psm2();
        } 
    }

exit_handle:
    if(psm_read_cache){
        free(psm_read_cache);
        psm_read_cache = NULL;
    }

    if(fl_dev) {
        flash_drv_close(fl_dev);
        fl_dev = NULL;
    }

    return ret;
}



