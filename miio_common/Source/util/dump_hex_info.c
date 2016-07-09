#include <stdint.h>
#include <wmstdio.h>

/*************************************************
* dump 32 bytes every line
**************************************************/
#define DUMP_LINE_LENGTH (16)
#define CAN_BE_PRINT(data)    (((data) >= 32) && ((data) < 127))

#if APPCONFIG_DEBUG_ENABLE
void xiaomi_dump_hex(uint8_t* data ,uint16_t length)
{
   if(length == 0) {
       return;
   } 

   uint16_t count = (length - 1) / DUMP_LINE_LENGTH + 1;
   uint16_t i,j;
   uint8_t hex_buf[2];

   wmprintf("%d bytes from %p\r\n", length, data);
   wmprintf("        +0          +4          +8          +c            0   4   8   c   \r\n");

   for( i = 0; i< count ;++i) {
     wmprintf("+%04x   ", i*DUMP_LINE_LENGTH);
     
     //print hex
     for(j = 0; ((i*DUMP_LINE_LENGTH + j) < length) && (j < DUMP_LINE_LENGTH); ++j) {
        
        uint8_t temp = (data[i * DUMP_LINE_LENGTH + j] >> 4) & 0x0f;
        hex_buf[0] = (temp >= 10)? ('A' + temp -10) : (temp + '0');

        temp = data[i * DUMP_LINE_LENGTH + j] & 0x0f;
        hex_buf[1] = (temp >= 10)? ('A' + temp -10) : (temp + '0');

        wmprintf("%c%c ",hex_buf[0],hex_buf[1]);
     }
    
     wmprintf("   ");

     //print ascii 
     for(j = 0; ((i*DUMP_LINE_LENGTH + j) < length) && (j < DUMP_LINE_LENGTH); ++j) {
        if(CAN_BE_PRINT(data[i * DUMP_LINE_LENGTH + j])) {
            wmprintf("%c",data[i * DUMP_LINE_LENGTH + j]);
        } else {
            wmprintf(".");
        }
     }

     wmprintf("\r\n");
   }
}

#else //APPCONFIG_DEBUG_ENABLE

void xiaomi_dump_hex(uint8_t* data ,uint16_t length)
{
    (void) data;
    (void) length;
}

#endif //APPCONFIG_DEBUG_ENABLE
