//
// Created by shreyas on 29.10.20.
//

#ifndef BG96_AT_DRIVER_H
#define BG96_AT_DRIVER_H
#include <include/modem/at_cmd.h>
#include "watchdog.h"
#include "gps_controller.h"
#include <date_time.h>
#include "uart.h"

#include <drivers/gsm_ppp.h>

#define GET_GPRS "CGATT";
#define OPEN_SOCKET "QIOPEN";
#define GET_SIGNAL_PRIO "CSQ";

static char recv_buf[1024];

static bool init_bg96();

static void gsm_send_command(const struct device *dev, struct gsm_command *cmd);

static void gsm_send_test(void);

static int send_uart(char* data[]);


#endif //BG96_AT_DRIVER_H
