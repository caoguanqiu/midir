/**************************************************************************************************
  Filename:       miio_ble.h
  Description:    This file contains the miio BLE API definitions and prototypes.
**************************************************************************************************/

//#ifndef MIIO_BLE
//#define MIIO_BLE


/*********************************************************************
 * INCLUDES
 */

/* None */

/*********************************************************************
 * CONSTANTS
 */

/*
 * Definition for BLE address length
 */
#define BLE_ADDR_LEN             6

/*
 * Definition for BLE address type
 */
#define BLE_ADDR_TYPE_PUBLIC     0x00
#define BLE_ADDR_TYPE_RANDOM     0x01

/*
 * Definition for BLE advertising type
 */
#define BLE_ADTYPE_FLAGS                        0x01 //!< Discovery Mode: @ref GAP_ADTYPE_FLAGS_MODES
#define BLE_ADTYPE_16BIT_MORE                   0x02 //!< Service: More 16-bit UUIDs available
#define BLE_ADTYPE_16BIT_COMPLETE               0x03 //!< Service: Complete list of 16-bit UUIDs
#define BLE_ADTYPE_32BIT_MORE                   0x04 //!< Service: More 32-bit UUIDs available
#define BLE_ADTYPE_32BIT_COMPLETE               0x05 //!< Service: Complete list of 32-bit UUIDs
#define BLE_ADTYPE_128BIT_MORE                  0x06 //!< Service: More 128-bit UUIDs available
#define BLE_ADTYPE_128BIT_COMPLETE              0x07 //!< Service: Complete list of 128-bit UUIDs
#define BLE_ADTYPE_LOCAL_NAME_SHORT             0x08 //!< Shortened local name
#define BLE_ADTYPE_LOCAL_NAME_COMPLETE          0x09 //!< Complete local name
#define BLE_ADTYPE_POWER_LEVEL                  0x0A //!< TX Power Level: 0xXX: -127 to +127 dBm
#define BLE_ADTYPE_OOB_CLASS_OF_DEVICE          0x0D //!< Simple Pairing OOB Tag: Class of device (3 octets)
#define BLE_ADTYPE_OOB_SIMPLE_PAIRING_HASHC     0x0E //!< Simple Pairing OOB Tag: Simple Pairing Hash C (16 octets)
#define BLE_ADTYPE_OOB_SIMPLE_PAIRING_RANDR     0x0F //!< Simple Pairing OOB Tag: Simple Pairing Randomizer R (16 octets)
#define BLE_ADTYPE_SM_TK                        0x10 //!< Security Manager TK Value
#define BLE_ADTYPE_SM_OOB_FLAG                  0x11 //!< Secutiry Manager OOB Flags
#define BLE_ADTYPE_SLAVE_CONN_INTERVAL_RANGE    0x12 //!< Min and Max values of the connection interval (2 octets Min, 2 octets Max) (0xFFFF indicates no conn interval min or max)
#define BLE_ADTYPE_SIGNED_DATA                  0x13 //!< Signed Data field
#define BLE_ADTYPE_SERVICES_LIST_16BIT          0x14 //!< Service Solicitation: list of 16-bit Service UUIDs
#define BLE_ADTYPE_SERVICES_LIST_128BIT         0x15 //!< Service Solicitation: list of 128-bit Service UUIDs
#define BLE_ADTYPE_SERVICE_DATA                 0x16 //!< Service Data
#define BLE_ADTYPE_APPEARANCE                   0x19 //!< Appearance
#define BLE_ADTYPE_MANUFACTURER_SPECIFIC        0xFF //!< Manufacturer Specific Data: first 2 octets contain the Company Identifier Code followed by the additional manufacturer specific data

/*********************************************************************
 * TYPES
 */
typedef struct {
    uint8_t  advType;
    uint8_t  intervalMin;  // *0.625 
    uint8_t  intervalMax;  // *0.625
    uint8_t  advData[31];
    uint8_t  advDataLen;
    uint8_t  scanRspData[31];
    uint8_t  scanRspLen;
} miio_ble_adv_t;


/*
 * Definition for BLE event callback prototypes
 */
typedef void (*miio_ble_initDoneCb_t)( void );
typedef void (*miio_ble_discoveryStopped_t)( void );
typedef void (*miio_ble_advReportCb_t)(uint8_t addrType,  uint8_t* addr, uint8_t rssi, uint8_t* advData, uint8_t advDataLen );


/* User need implement this callback function list */
typedef struct {
    miio_ble_initDoneCb_t       initDoneCb;           // BLE initializitino finish CB
    miio_ble_discoveryStopped_t discoveryStoppedCb;   // Triggered when device be assigned a short network address
    miio_ble_advReportCb_t      advReportCb;          // Callback function for received advertising data
} miio_ble_cbs_t;


/*********************************************************************
 * FUNCTIONS
 */

/*********************************************************************
 * @fn      miio_ble_advertising
 *
 * @brief   API to start or stop ble advertising
 *
 * @param   adv - advertising parameters
 * @param   fEnable - 1 indicating start and 0 indicating stop
 *
 * @return  None
 */
void miio_ble_advertising(miio_ble_adv_t* adv, uint32_t fEnable);

/*********************************************************************
 * @fn      miio_ble_discovery
 *
 * @brief   API to start or stop ble discovery
 *
 * @param   fEnable - 1 indicating start and 0 indicating stop
 *
 * @return  None
 */
void miio_ble_discovery(uint32_t fEnable);

/*********************************************************************
 * @fn      miio_ble_init
 *
 * @brief   Initialize BLE module
 *
 * @param   ble_cbFuncs - callback functions
 *
 * @return  None
 */
void miio_ble_init(miio_ble_cbs_t *ble_cbFuncs);

/*********************************************************************
 * @fn      adv_search_type
 *
 * @brief   Serarch specified type from advertising data
 *          e.g //02 01 05 03 02 11 22 , to search type: 02,
 *                result: 03 02 11 22
 *
 * @param   advData - The advertising data where to search
 * @param   advDataLen - The length of advertising data
 * @param   type - The specified advertising type
 *
 * @return  The segment of the specifed type, NULL means not found.
 */
uint8_t* adv_search_type(uint8_t* advData, uint8_t advDataLen, int type);

/*********************************************************************
 * @fn      strtomac
 *
 * @brief   Utility function to convert string to MAC address
 *
 * @param   params - String to contain MAC address,like [00:11:22:33:44:55]
 * @param   mac - Mac address array.
 *
 * @return  None
 */
void strtomac(char* str, uint8_t* mac);



//#endif  /* MIIO_BLE */