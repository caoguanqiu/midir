#include <lowlevel_drivers.h>
#include <lib/arch_io.h>

int api_io_init(GPIO_NO_Type gpio, GPIO_Dir_Type dir) {
	GPIO_PinMuxFun(gpio, PINMUX_FUNCTION_0);
	GPIO_SetPinDir(gpio, dir);
	return 0;
}

int api_io_high(GPIO_NO_Type gpio) {

	GPIO_SetPinDir(gpio, GPIO_OUTPUT);
	GPIO_WritePinOutput(gpio, GPIO_IO_HIGH);
	return 0;
}

int api_io_low(GPIO_NO_Type gpio) {

	GPIO_SetPinDir(gpio, GPIO_OUTPUT);
	GPIO_WritePinOutput(gpio, GPIO_IO_LOW);
	return 0;
}

int api_io_toggle(GPIO_NO_Type gpio) {

	GPIO_SetPinDir(gpio, GPIO_OUTPUT);
	if(GPIO_IO_LOW == GPIO_ReadPinLevel(gpio))
		GPIO_WritePinOutput(gpio, GPIO_IO_HIGH);
	else
		GPIO_WritePinOutput(gpio, GPIO_IO_LOW);
	return 0;
}

int api_io_get(GPIO_NO_Type gpio) {
	if(GPIO_IO_LOW == GPIO_ReadPinLevel(gpio))
		return GPIO_IO_LOW;
	else
		return GPIO_IO_HIGH;
}
