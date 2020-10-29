#include "gsm_driver.h"

static int cmd_sample_modem_suspend(const struct shell *shell,
                                    size_t argc, char *argv[])
{
    if (!connected) {
        shell_fprintf(shell, SHELL_NORMAL, "Not connected.\n");
        return -ENOEXEC;
    }

    gsm_ppp_stop(gsm_dev);

    return 0;
}

static int cmd_sample_modem_resume(const struct shell *shell,
                                   size_t argc, char *argv[])
{
    if (starting) {
        shell_fprintf(shell, SHELL_NORMAL,
                      "Please wait for network connection.\n");
        return -ENOEXEC;
    }

    if (connected) {
        shell_fprintf(shell, SHELL_NORMAL, "Already connected.\n");
        return -ENOEXEC;
    }

    gsm_ppp_start(gsm_dev);

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sample_commands,
        SHELL_CMD(resume, NULL,
"Resume the modem\n",
cmd_sample_modem_resume),
SHELL_CMD(suspend, NULL,
"Suspend the modem\n",
cmd_sample_modem_suspend),
SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sample, &sample_commands,
"Sample application commands", NULL);


static void event_handler(struct net_mgmt_event_callback *cb,
                          uint32_t mgmt_event, struct net_if *iface)
{
    if ((mgmt_event & (NET_EVENT_L4_CONNECTED
                       | NET_EVENT_L4_DISCONNECTED)) != mgmt_event) {
        return;
    }

    starting = false;

    if (mgmt_event == NET_EVENT_L4_CONNECTED) {
        LOG_INF("Network connected");
        connected = true;
        return;
    }

    if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
        LOG_INF("Network disconnected");
        connected = false;
        return;
    }
}