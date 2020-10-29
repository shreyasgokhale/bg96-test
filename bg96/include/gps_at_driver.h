//
// Created by shreyas on 29.10.20.
//

#ifndef ZEPHYR_GPS_AT_DRIVER_H
#define ZEPHYR_GPS_AT_DRIVER_H
#include <modem/at_cmd.h>
#include "watchdog.h"
#include "gps_controller.h"
#include <date_time.h>

#include <drivers/gsm_ppp.h>
#include <include/at_parser.h>
static void set_gps_enable(const bool enable);


static struct gps_nmea gps_data;
static struct gps_agps_request agps_request;
static void send_agps_request(struct k_work *work);
static void gps_time_set(struct gps_pvt *gps_data);
static void gps_handler(const struct device *dev, struct gps_event *evt);
#endif //ZEPHYR_GPS_AT_DRIVER_H
