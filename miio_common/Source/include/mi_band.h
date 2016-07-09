/**************************************************************************************************
  Filename:       mi_band.h
  Description:    This file contains the mi band related API definitions and prototypes.
**************************************************************************************************/

#ifndef __MI_BAND__
#define __MI_BAND__


/*********************************************************************
 * INCLUDES
 */

/* None */

/*********************************************************************
 * CONSTANTS
 */

/*
 * Definition for max number of supported xiaomi band.
 */
#define MAX_MI_BAND_NUM        10


/*
 * @brief Default RSSI threshold of band 
 */
#define DFLT_BAND_CLOSE_THRESHOLD       90
#define DFLT_BAND_AWAY_THERSHOLD        70

/*
 * @brief Default RSSI threshold of band 
 */
#define MI_BAND_USER_STATUS_RFU         0x00
#define MI_BAND_USER_STATUS_SLEEPING    0x01
#define MI_BAND_USER_STATUS_UNDEFINED   0xFF


/*********************************************************************
 * TYPES
 */

typedef enum {
    MI_BAND_SUCC = 0,
    MI_BAND_ERR_TABLE_FULL = 1,
    MI_BAND_ERR_EXISTED = 2,
    MI_BAND_ERR_NOT_FOUND = 3,
    MI_BAND_ERR_INIT_FAIL = 4,
    MI_BAND_ERR_INVALID_PARA = 5,
} mi_band_sts_t;


/*
 * Definition for BLE event callback prototypes
 */
typedef void (*mi_band_close)( uint8_t* mac, int8_t rssi);
typedef void (*mi_band_away) ( uint8_t* mac );
typedef void (*mi_band_status_changed) ( uint8_t* mac, uint8_t newStatus);


/* User need implement this callback function list */
typedef struct {
    mi_band_close        bandCloseCb;           // Triggered when a band from away to close
    mi_band_away         bandAwayCb;            // Triggered when a band from close to away
    mi_band_status_changed bandStatusChangedCb;   // Triggered when a band status changed
} mi_band_cbs_t;



/*********************************************************************
 * FUNCTIONS
 */
void mi_band_reset(void); 
mi_band_sts_t mi_band_init(mi_band_cbs_t* band_cbFuncs);
mi_band_sts_t mi_band_add(uint8_t* mac);
mi_band_sts_t mi_band_del(uint8_t* mac);
mi_band_sts_t mi_band_get_entry(uint8_t index, uint8_t * addr, int8_t* rssi, uint8_t *status);
uint8_t mi_band_get_curNum(void);


#endif  /* __MI_BAND__ */

