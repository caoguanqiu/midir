#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <appln_dbg.h>
#include <wmtime.h>
#include <wmstdio.h>
#include <lib/arch_os.h>
#include <cli.h>

e_print_level_t print_level = PRINT_LEVEL_INFO;

int32_t g_time_zone = 8 * 60; // time zone, offset value in minutes
void appln_print_time(void)
{
    struct tm date_time;
    time_t curtime;
    curtime = arch_os_utc_now() + g_time_zone * 60;
    gmtime_r((const time_t *) &curtime, &date_time);
    wmprintf("%02d:%02d:%02d.%03d ", date_time.tm_hour, date_time.tm_min, date_time.tm_sec, arch_os_tick_now() % 1000);
}

void cmd_set_print_level(int argc, char **argv)
{
    if (argc <= 1)
        wmprintf("%u\r\n", print_level);
    else
        print_level = atoi(argv[1]);
}

static struct cli_command appln_commands[] =
        { { "pl", "get/set print level (0~3), default is 1", cmd_set_print_level } };

int appln_cli_init(void)
{
    return cli_register_commands(appln_commands, sizeof(appln_commands) / sizeof(struct cli_command));
}
