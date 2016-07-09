/*
 * miio_cli.h
 *
 *  Created on: Aug 18, 2014
 *      Author: zhangyanlu
 */

#ifndef MIIO_CLI_H_
#define MIIO_CLI_H_

#define FACTORY_MAGIC  0x82562647
// factory log messages
#define FACTORY_NORMAL_BOOT "[MIIO-NORMAL-BOOT]\r\n" //this is the trigger of factory command miio-test
#define FACTORY_STARTUP_PASS "[MIIO-TEST-STARTUP-PASS]\r\n"
#define FACTORY_JOINAP_PASS "[MIIO-TEST-JOINAP-PASS]\r\n"
#define FACTORY_FNTEST_PASS "[MIIO-TEST-FNTEST-PASS]\r\n"
#define FACTORY_SET_DID "[MIIO-SETDID-OK]\r\n"

void cmd_enter_factory_mode(int argc, char **argv);
uint32_t is_factory_mode_enable(void);
uint32_t get_factory_mode_enable(void);

int factory_cli_init(void);

#endif /* MIIO_CLI_H_ */
