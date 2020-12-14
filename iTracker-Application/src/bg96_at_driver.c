//
// Created by shreyas on 29.10.20.
//

#include "bg96_at_driver.h"

#define RETRY_DELAY 60
#define PWR_GPRS_ON 6 // GPIO P0.06 ( Chip pin 8)
#define GPRS_PWR_KEY  14 // GPIO P0.14 ( Chip pin 17)

static bool init_bg96() {

    int ret;

//  Turn on Power supply to BG96
    gpio_device = device_get_binding("GPIO_0");
    ret = gpio_pin_configure(dev, PWR_GPRS_ON, GPIO_OUTPUT_ACTIVE);
    ret &= gpio_pin_configure(dev, GPRS_PWR_KEY, GPIO_OUTPUT_ACTIVE);

    if (!ret) return false;

//    Init UART
    uart_device = device_get_binding("UART_0");
    uart_callback_set(uart_device, uart_data_handler, NULL);
    uart_rx_enable(uart_device_0, recv_buf, sizeof(recv_buf), 120);

//    Turn on other modules/sensors

    return true;
}


static int send_uart(char *data[]) {

//    Ref: test/drivers/uart/
    /* Verify uart_irq_tx_ready() */

    /*
    * Note that TX IRQ may be disabled, but uart_irq_tx_ready() may
    * still return true when ISR is called for another UART interrupt,
    * hence additional check for i < DATA_SIZE.
    */


    if (uart_irq_tx_ready(dev) && tx_data_idx < DATA_SIZE) {
        /* We arrive here by "tx ready" interrupt, so should always
         * be able to put at least one byte into a FIFO. If not,
         * well, we'll fail test.
         */
        if (uart_fifo_fill(dev,
                           (uint8_t * ) & fifo_data[tx_data_idx++], 1) > 0) {
            data_transmitted = true;
            char_sent++;
        }

        if (tx_data_idx == DATA_SIZE) {
            /* If we transmitted everything, stop IRQ stream,
             * otherwise main app might never run.
             */
            uart_irq_tx_disable(dev);
        }
    }

}


static void gsm_send_command(const struct device *dev, struct gsm_command *cmd) {

    switch (cmd->type) {
        case GPS_EVT_SEARCH_STARTED:
            LOG_INF("GPS_EVT_SEARCH_STARTED");
            gps_control_set_active(true);gps_last_search_start_time = k_uptime_get();
            break;
        case GPS_EVT_SEARCH_STOPPED:
            LOG_INF("GPS_EVT_SEARCH_STOPPED");
            gps_control_set_active(false);
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
            gps_time_set(&cmd->pvt);
            break;
        case GPS_EVT_NMEA:
            /* Don't spam logs */
            break;
        case GPS_EVT_NMEA_FIX:
            LOG_INF("Position fix with NMEA data");

            memcpy(gps_data.buf, cmd->nmea.buf, cmd->nmea.len);
            gps_data.len = cmd->nmea.len;
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
            memcpy(&agps_request, &cmd->agps_request, sizeof(agps_request));
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


bool gsm_send_test(void) {

//  TODO: Implement logic which checks return of each command from UART and returns if not okay.

    send_uart("ATI");

//  TODO: AT+QCFG for config
//  Source https://github.com/RAKWireless/RUI_Platform_Firmware_GCC/blob/master/RUI/Source/driver/bg96.c

    send_uart("AT+QISEND=TEST_DATA");
    send_uart("AT+QISEND=1,75");
    send_uart("$GPGGA,134303.00,3418.040101,N,10855.904676,E,1,07,1.0,418.5,M,-28.0,M,,*4A");
    send_uart("AT+QISEND=1,170");
}
