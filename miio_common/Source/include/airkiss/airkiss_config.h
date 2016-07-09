#ifndef __AP_CONFIG_H__
#define __AP_CONFIG_H__

#define SAND_DBG(_fmt_, ...)  \
	wmprintf("[sand debug] "_fmt_"\n\r", ##__VA_ARGS__)

#define SAND_LOG(_fmt_, ...)  \
	wmprintf("[sand] "_fmt_"\n\r", ##__VA_ARGS__ )

void airkiss_config_with_sniffer(char * ssid);

#endif
