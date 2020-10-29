//
// Created by shreyas on 29.10.20.
//

#include "gps_at_driver.h"

#define RETRY_DELAY 60



// From Asset tracker application
// https://github.com/nrfconnect/sdk-nrf/blob/91463a03f60dfd37980cd190b34f5fde73cefc33/applications/asset_tracker/src/main.c#L648

// TODO: Fixing all fns

static void gps_handler(const struct device *dev, struct gps_event *evt)
{
    gps_last_active_time = k_uptime_get();

    switch (evt->type) {
        case GPS_EVT_SEARCH_STARTED:
            LOG_INF("GPS_EVT_SEARCH_STARTED");
            gps_control_set_active(true);
            ui_led_set_pattern(UI_LED_GPS_SEARCHING);
            gps_last_search_start_time = k_uptime_get();
            break;
        case GPS_EVT_SEARCH_STOPPED:
            LOG_INF("GPS_EVT_SEARCH_STOPPED");
            gps_control_set_active(false);
            ui_led_set_pattern(UI_CLOUD_CONNECTED);
            break;
        case GPS_EVT_SEARCH_TIMEOUT:
            LOG_INF("GPS_EVT_SEARCH_TIMEOUT");
            gps_control_set_active(false);
            LOG_INF("GPS will be attempted again in %d seconds",
                    gps_control_get_gps_reporting_interval());
            break;
        case GPS_EVT_PVT:
            /* Don't spam logs */
            break;
        case GPS_EVT_PVT_FIX:
            LOG_INF("GPS_EVT_PVT_FIX");
            gps_time_set(&evt->pvt);
            break;
        case GPS_EVT_NMEA:
            /* Don't spam logs */
            break;
        case GPS_EVT_NMEA_FIX:
            LOG_INF("Position fix with NMEA data");

            memcpy(gps_data.buf, evt->nmea.buf, evt->nmea.len);
            gps_data.len = evt->nmea.len;
            gps_cloud_data.data.buf = gps_data.buf;
            gps_cloud_data.data.len = gps_data.len;
            gps_cloud_data.ts = k_uptime_get();
            gps_cloud_data.tag += 1;

            if (gps_cloud_data.tag == 0) {
                gps_cloud_data.tag = 0x1;
            }

            int64_t gps_time_from_start_to_fix_seconds = (k_uptime_get() -
                                                          gps_last_search_start_time) / 1000;
            ui_led_set_pattern(UI_LED_GPS_FIX);
            gps_control_set_active(false);
            LOG_INF("GPS will be started in %lld seconds",
                    CONFIG_GPS_CONTROL_FIX_TRY_TIME -
                    gps_time_from_start_to_fix_seconds +
                    gps_control_get_gps_reporting_interval());

            k_work_submit_to_queue(&application_work_q,
                                   &send_gps_data_work);
            env_sensors_poll();
            break;
        case GPS_EVT_OPERATION_BLOCKED:
            LOG_INF("GPS_EVT_OPERATION_BLOCKED");
            ui_led_set_pattern(UI_LED_GPS_BLOCKED);
            break;
        case GPS_EVT_OPERATION_UNBLOCKED:
            LOG_INF("GPS_EVT_OPERATION_UNBLOCKED");
            ui_led_set_pattern(UI_LED_GPS_SEARCHING);
            break;
        case GPS_EVT_AGPS_DATA_NEEDED:
            LOG_INF("GPS_EVT_AGPS_DATA_NEEDED");
            /* Send A-GPS request with short delay to avoid LTE network-
             * dependent corner-case where the request would not be sent.
             */
            memcpy(&agps_request, &evt->agps_request, sizeof(agps_request));
            k_delayed_work_submit_to_queue(&application_work_q,
                                           &send_agps_request_work,
                                           K_SECONDS(1));
            break;
        case GPS_EVT_ERROR:
            LOG_INF("GPS_EVT_ERROR\n");
            break;
        default:
            break;
    }
}


static void gps_time_set(struct gps_pvt *gps_data)
{
    /* Change datetime.year and datetime.month to accommodate the
     * correct input format.
     */
    struct tm gps_time = {
            .tm_year = gps_data->datetime.year - 1900,
            .tm_mon = gps_data->datetime.month - 1,
            .tm_mday = gps_data->datetime.day,
            .tm_hour = gps_data->datetime.hour,
            .tm_min = gps_data->datetime.minute,
            .tm_sec = gps_data->datetime.seconds,
    };

    date_time_set(&gps_time);
}



static void send_agps_request(struct k_work *work)
{
    ARG_UNUSED(work);

#if defined(CONFIG_AGPS)
    int err;
	static int64_t last_request_timestamp;

/* Request A-GPS data no more often than every hour (time in milliseconds). */
#define AGPS_UPDATE_PERIOD (60 * 60 * 1000)

	if ((last_request_timestamp != 0) &&
	    (k_uptime_get() - last_request_timestamp) < AGPS_UPDATE_PERIOD) {
		LOG_WRN("A-GPS request was sent less than 1 hour ago");
		return;
	}

	LOG_INF("Sending A-GPS request");

	err = gps_agps_request(agps_request, GPS_SOCKET_NOT_PROVIDED);
	if (err) {
		LOG_ERR("Failed to request A-GPS data, error: %d", err);
		return;
	}

	last_request_timestamp = k_uptime_get();

	LOG_INF("A-GPS request sent");
#endif /* defined(CONFIG_AGPS) */
}

static void set_gps_enable(const bool enable)
{
    int32_t delay_ms = 0;
    bool changing = (enable != gps_control_is_enabled());
    if (changing) {
        if (enable) {
            LOG_INF("Starting GPS");
            /* GPS will be started from the device config work
             * handler AFTER the config has been sent to the cloud
             */
        } else {
            LOG_INF("Stopping GPS");
            gps_control_stop(0);
            /* Allow time for the gps to be stopped before
             * attemping to send the config update
             */
            k_sleep(K_SECONDS(RETRY_DELAY));
            //            k_delay
        }
    }

}