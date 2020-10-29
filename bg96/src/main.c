/*
 * Copyright (c) 2020, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Modified Code from GSM example

#include <zephyr.h>
#include <sys/printk.h>
#include <shell/shell.h>
#include <drivers/uart.h>
#include <net/net_mgmt.h>
#include <net/net_event.h>
#include <net/net_conn_mgr.h>
#include <modem/at_cmd_parser.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(sample_bg96_test, LOG_LEVEL_DBG);


// If we are using a GSM driver or AT Command Sender
// #define USE_DRIVER


static const struct device *gsm_dev;
static struct net_mgmt_event_callback mgmt_cb;
static bool connected;
static bool starting = true;



int main(void)
{
	const struct device *uart_dev =
				device_get_binding(CONFIG_MODEM_GSM_UART_NAME);

	gsm_dev = device_get_binding(GSM_MODEM_DEVICE_NAME);

	LOG_INF("Board '%s' APN '%s' UART '%s' device %p (%s)",
		CONFIG_BOARD, CONFIG_MODEM_GSM_APN,
		CONFIG_MODEM_GSM_UART_NAME, uart_dev, GSM_MODEM_DEVICE_NAME);

#ifdef USE_DRIVER
//	Process based on GSM driver in zephyr
#else
//Use gps At command Driver
// For now, send a request every 1 min
    set_gps_enable(true);
    while (1){
        send_agps_request();
        delay_ms = 5 * MSEC_PER_SEC;

    }

	return 0;
}
