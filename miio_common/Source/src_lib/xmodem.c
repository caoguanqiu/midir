/* this code needs standard functions memcpy() and memset()
   and input/output functions port_inbyte() and port_outbyte().

   the prototypes of the input/output functions are:
     int port_inbyte(unsigned short timeout); // msec timeout
     void port_outbyte(int c);

 */

#include <stdint.h>
#include <mdev_uart.h>
#include <string.h>
#include <wm_os.h>
#include <FreeRTOSConfig.h>
#include <appln_dbg.h>
#include <lib/third_party_mcu_ota.h>

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAXRETRANS 25
static int last_error = 0;

struct {
    unsigned char packetno;
}packetno_wrap = {
    .packetno = 1,
};

#ifdef USE_CRC16
/* CRC16 implementation acording to CCITT standards */

static const unsigned short crc16tab[256]= {
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};
  
unsigned short crc16_ccitt(const unsigned char *buf, int len)
{
	register int counter;
	register unsigned short crc = 0;
	for( counter = 0; counter < len; counter++)
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *(char *)buf++)&0x00FF];
	return crc;
}

#else       //USE_CRC16

static unsigned short crc16_ccitt( const unsigned char *buf, int len )
{
	unsigned short crc = 0;
	while( len-- ) {
		int i;
		crc ^= *buf++ << 8;
		
		for( i = 0; i < 8; ++i ) {
			if( crc & 0x8000 )
				crc = (crc << 1) ^ 0x1021;
			else
				crc = crc << 1;
		}
	}
	return crc;
}

#endif  //USE_CRC16

/****************Portting Start *******************/

#define PORT_MSEC_TICK ((configTICK_RATE_HZ) / 1000) //1ms to ticks

static void port_outbyte(unsigned char trychar)
{
	ota_uart_type_write(trychar);
}

// time_out is millisecond time unit 
static unsigned char port_inbyte(unsigned int time_out)
{
	unsigned char ch;
	last_error = 0;
    
    portTickType end_tick = os_ticks_get() + (time_out * PORT_MSEC_TICK);

    while(os_ticks_get() < end_tick) { //ignore ticks overflow
        if( ota_uart_byte_read (&ch)== 1) 
            return ch;
        os_thread_sleep(6);
    }

	last_error = 1;
	return ch;
}
/****************Portting End*******************/

static void flushinput(void)
{
    portTickType end_tick = os_ticks_get() + ( 100 * PORT_MSEC_TICK);
    
    //no inputs within 10ms interval add total flush  no more than 100 ms
    while (!last_error && (os_ticks_get() < end_tick)) {
        port_inbyte(10);
    }

}


void ignore_prev_input()
{
    unsigned char ch;

    //if there's data in buffer,ignore
    while(ota_uart_byte_read(&ch));
}


/***********************************************
* xmodem协议开始获取crc的操作。
***********************************************/
int xmodem_get_crc_config(int *crc_ret)
{
	char c;
	int retry;
    
    //init packetno in the very begining
    packetno_wrap.packetno = 1;

    for( retry = 0; retry < 16; ++retry) 
	{
		c = port_inbyte((DLY_1S )<<1); //2S wait for crc byte,total waits time is 2 * 16 = 32s
		if (last_error == 0) 
		{
			switch (c) 
			{
				case 'C':
					* crc_ret = 1;
                    return 0;
				case NAK:
                    *crc_ret = 0;
                    return 0;
				case CAN:
					c = port_inbyte(DLY_1S);
					if (c == CAN) 
					{
						port_outbyte(ACK);
						flushinput();
						return -1; /* canceled by remote */
					}
					break;
				default:
                    LOG_ERROR("in get crc get byte %c \r\n",c);
					break;
			}
		}
	}
	port_outbyte(CAN);
	port_outbyte(CAN);
	port_outbyte(CAN);
	flushinput();
	return -2; /* no sync */

}

/*********************************************
* srcsz must be integral multiples of 128 
**********************************************/
int xmodem_transfer_data(int crc,unsigned char *src, int srcsz,int xmodem_mtu)
{
    unsigned char xbuff[1030]; /* 128 data  + 3 head chars + 2 crc + nul */
	int bufsz;
	int i, c, len = 0;
	int retry;

    unsigned char length_byte;

    if(xmodem_mtu == 128) {
        length_byte = SOH;
    } else if(xmodem_mtu == 1024) {
        length_byte = STX;
    } else {
        LOG_ERROR("mtu error\r\n");
        return -1;
    }

    for(;;) 
	{
	start_trans:
		xbuff[0] = length_byte; bufsz = xmodem_mtu;
		xbuff[1] = packetno_wrap.packetno;
		xbuff[2] = ~packetno_wrap.packetno;
		c = srcsz - len;
		if (c >= bufsz) c = bufsz;
        
        //one http package tranfer complete
        if(c == 0) {
            return 0;
        }
        
		memset (&xbuff[3], 0, bufsz);
	    memcpy (&xbuff[3], &src[len], c);

		if (crc) 
		{
			unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
			xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
			xbuff[bufsz+4] = ccrc & 0xFF;
		}
		else 
		{
			unsigned char ccks = 0;
			for (i = 3; i < bufsz+3; ++i) 
			{
				ccks += xbuff[i];
			}
			xbuff[bufsz+3] = ccks;
		}
		for (retry = 0; retry < MAXRETRANS; ++retry) 
		{
		    taskENTER_CRITICAL();
			for (i = 0; i < bufsz+4+(crc?1:0); ++i) 
			{
				port_outbyte(xbuff[i]);
			}
			taskEXIT_CRITICAL();
            
            ignore_prev_input();
			
            c = port_inbyte(DLY_1S);
			if (last_error == 0 ) 
			{
				switch (c) 
				{
					case ACK:
						++packetno_wrap.packetno;
						len += bufsz;
						goto start_trans;
					case CAN:
						c = port_inbyte(DLY_1S);
						if ( c == CAN) 
						{
							port_outbyte(ACK);
							flushinput();
							return -1; /* canceled by remote */
						}
						break;
					case NAK:
					default:
						break;
				}
			}
		}
		port_outbyte(CAN);
		port_outbyte(CAN);
		port_outbyte(CAN);
		flushinput();
		return -4; /* xmit error */
	}
}

/************************************************************
* transfer last http package
**************************************************************/
int xmodem_transfer_end(int crc ,unsigned char *src, int srcsz,int xmodem_mtu)
{
    unsigned char xbuff[1030]; /* 1024 data  + 3 head chars + 2 crc + nul */
	int bufsz;
	int i, c, len = 0;
	int retry;
    
    unsigned char length_byte;

    if(xmodem_mtu == 128) {
        length_byte = SOH;
    } else if(xmodem_mtu == 1024) {
        length_byte = STX;
    } else {
        LOG_ERROR("mtu error\r\n");
        return -1;
    }


    for(;;) 
	{
	start_trans:
		xbuff[0] = length_byte; bufsz = xmodem_mtu;
		xbuff[1] = packetno_wrap.packetno;
		xbuff[2] = ~packetno_wrap.packetno;
		c = srcsz - len;
        if (c > bufsz) c = bufsz;

        if (c >= 0) 
		{
            //for write flash, so pad oxff
			memset (&xbuff[3], 0xFF, bufsz);
			if (c == 0) 
			{
				xbuff[3] = CTRLZ;
			}
			else 
			{
				memcpy (&xbuff[3], &src[len], c);
				if (c < bufsz) xbuff[3+c] = CTRLZ;
			}
			if (crc) 
			{
				unsigned short ccrc = crc16_ccitt(&xbuff[3], bufsz);
				xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
				xbuff[bufsz+4] = ccrc & 0xFF;
			}
			else 
			{
				unsigned char ccks = 0;
				for (i = 3; i < bufsz+3; ++i) 
				{
					ccks += xbuff[i];
				}
				xbuff[bufsz+3] = ccks;
			}
			for (retry = 0; retry < MAXRETRANS; ++retry) 
			{
			    taskENTER_CRITICAL();
				for (i = 0; i < bufsz+4+(crc?1:0); ++i) 
				{
					port_outbyte(xbuff[i]);
				}
				taskEXIT_CRITICAL();

                ignore_prev_input();

				c = port_inbyte(DLY_1S);
				if (last_error == 0 ) 
				{
					switch (c) 
					{
						case ACK:
							++packetno_wrap.packetno;
							len += bufsz;
							goto start_trans;
						case CAN:
							c = port_inbyte(DLY_1S);
							if ( c == CAN) 
							{
								port_outbyte(ACK);
								flushinput();
								return -1; /* canceled by remote */
							}
							break;
						case NAK:
						default:
							break;
					}
				}
			}
			port_outbyte(CAN);
			port_outbyte(CAN);
			port_outbyte(CAN);
			flushinput();
			return -4; /* xmit error */
		}
		else 
		{
			for (retry = 0; retry < 10; ++retry) 
			{
				port_outbyte(EOT);
				c = port_inbyte((DLY_1S)<<1);
				if (c == ACK) break;
			}
			flushinput();
            
            if(c == ACK) {
                LOG_INFO("********all data transfer complete***\r\n");    
            }
			return (c == ACK)?0:-5;
		}
	}
}


void cancel_xmodem_transfer()
{
    port_outbyte(CAN);
    port_outbyte(CAN);
    port_outbyte(CAN);
    flushinput();
}
