/**
 * Application code for iTracker 8212
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <shell/shell.h>
#include <drivers/uart.h>
#include <logging/log.h>

#include "include/bg96_at_driver.h"

LOG_MODULE_REGISTER(sample_bg96_test, LOG_LEVEL_DBG);

// Max retry period of 100 Sec
#define MAX_RETRY_PERIOD 10

static bool connected =  false;
static uint8_t retry_count = 0;

int main(void)
{
//  Initialize Component
    if(!init_bg96()){
        printk("Error! Cannot start BG96\n");
    }

    printk("BG96 Started!\n");
    printk("Sending test GSM\n");

//  Keep trying until GSM successfully connects
    while ((!connected) && (retry_count < MAX_RETRY_PERIOD)){
        if(gsm_send_test())
           connected = true;
        else{
            retry_count++;
            k_msleep(10000);
        }

    }

    return 0;
}
