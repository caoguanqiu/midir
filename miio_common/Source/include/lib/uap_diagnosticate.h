#ifndef _UAP_DIAGNOSTICATE_H_
#define _UAP_DIAGNOSTICATE_H_

#include <stdint.h>
#include <stdbool.h>

#define ENABLE_UAP  (0x5a5a5a5a)
#define DISABLE_UAP (0x00000000)

uint32_t get_trigger_uap_info();
void set_trigger_uap_info(uint32_t value);

#endif 

