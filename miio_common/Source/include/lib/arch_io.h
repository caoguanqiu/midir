#ifndef ARCH_IO_H_
#define ARCH_IO_H_

#include <lowlevel_drivers.h> 

int api_io_init(GPIO_NO_Type gpio, GPIO_Dir_Type dir);
int api_io_high(GPIO_NO_Type gpio);
int api_io_low(GPIO_NO_Type gpio);
int api_io_toggle(GPIO_NO_Type gpio);
int api_io_get(GPIO_NO_Type gpio);

#endif /* ARCH_IO_H_ */
