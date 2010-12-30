/*
 * Copyright (c) 2010 Atheros Communications Inc.
 * All rights reserved.
 * 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// Software distributed under the License is distributed on an "AS
// IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
// implied. See the License for the specific language governing
// rights and limitations under the License.
//
//
 *
 * ==============================================================================
 * Wifi Auto Configuration (WAC) User Application Implementation
 * 
 *  Author(s): ="Atheros"
 * ==============================================================================
 */
#ifdef  WAC_ENABLE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>

#include "ath_wac_lib.h"
#include "wac_defs.h"

/* defines */
#ifdef WAC_LIB_DEBUG
#define WAC_APP_DEBUG_PRINTF(args...)          printf(args);
#else
#define WAC_APP_DEBUG_PRINTF(args...)
#endif

#define WPA_CLI_PATH    "./"
#define NET_IFNAME      "eth1"

/* typedefs */
typedef struct {
    void *  wac_lib_handle;             /* handle to call ath_wac_xxx APIs */
    int     connected;
    wac_scan_bss_info_t bss_info[MAX_WAC_BSS];
    int enable;
    unsigned int period;
    unsigned int scan_thres;
    int rssi_thres;
    unsigned int wps_pin;
} wac_lib_user_private;

/* forward declarations */
void ath_wmi_connect_event(void *arg);
void ath_wmi_disconnect_event(void *arg);
int  ath_wac_scan_done_event(void *arg, wac_scan_bss_info_t *p_bss_info, unsigned int num_of_bss_info);
void ath_wac_wps_start_event(void *arg, unsigned char *bssid, unsigned int wps_pin);
void ath_wac_reconnect_start_event(void *arg, wac_scan_bss_info_t *p_bss_info);
void ath_wac_wps_reject_event(void *arg, unsigned int error_code);

/* statics */
static wac_lib_user_private *p_app_priv;

static wac_lib_user_t client = {
    WPA_CLI_PATH,   /* wpa_cli path */
    NET_IFNAME,     /* ifname */
    NULL,           /* arg will be handed over to the 1st argument of event callbacks */
    ath_wmi_connect_event,
    ath_wmi_disconnect_event,
    ath_wac_scan_done_event,
    ath_wac_wps_start_event,
    ath_wac_reconnect_start_event,
    ath_wac_wps_reject_event
};

const char *progname;
const char commands[] =
"commands:\n\
--wac <enable> <scan_period> <scan_threshold> <rssi_threshold>\n\
";

#ifndef WAC_CLI_APP
static void usage(void)
{
    printf("usage:\n%s [-i device] commands\n", progname);
    printf("%s\n", commands);
    exit(-1);
}
#endif

void ath_wmi_connect_event(void *arg)
{
    wac_lib_user_private * priv = arg;

    WAC_APP_DEBUG_PRINTF("%s\n", __func__);
    if (priv) {
        priv->connected = 1;
    }
}

void ath_wmi_disconnect_event(void *arg)
{
    wac_lib_user_private * priv = arg;

    WAC_APP_DEBUG_PRINTF("%s\n", __func__);
    if (priv) {
        priv->connected = 0;
    }
}

/*
 * The p_bss_info pointer can be invalid after ath_wac_scan_done_event returns.
 * If a user needs to save p_bss_info, save it to the user memory.
 */
int  ath_wac_scan_done_event(void *arg, wac_scan_bss_info_t *p_bss_info, unsigned int num_of_bss_info)
{
    wac_lib_user_private * priv = arg;
    int ret = 0;
    unsigned int i;
    unsigned int j = 0;
    wac_scan_bss_info_t *p;
    unsigned char snr = 0;

    WAC_APP_DEBUG_PRINTF("%s(num_of_bss_info = %d)\n", __func__, num_of_bss_info);

    /* if no WAC AP is found, disable WAC and return */
    if (!num_of_bss_info) {
        if (priv && priv->wac_lib_handle) {
            ath_wac_disable(priv->wac_lib_handle);
        }
        return -1;
    }

    for (i = 0; i < num_of_bss_info; i++) {
        p = &p_bss_info[i];
        WAC_APP_DEBUG_PRINTF("bssid %02x:%02x:%02x:%02x:%02x:%02x snr = %2d rssi = %d chan = %2d\n",
            p->bssid[0], p->bssid[1], p->bssid[2],
            p->bssid[3], p->bssid[4], p->bssid[5],
            p->snr, p->rssi, p->channel);

        /* search for the best snr index */
        if (snr <= p->snr) {
            snr = p->snr;
            j = i;
        }
    }

    if (priv && priv->wac_lib_handle) {
        if (j >= num_of_bss_info) {
            j = 0;
        }
        /* unicast probe request will be sent to the selected bssid on ath_wac_start_req */
        ret = ath_wac_start_req(priv->wac_lib_handle, &p_bss_info[j]);
    }

    return ret;
}

void ath_wac_wps_start_event(void *arg, unsigned char *bssid, unsigned int wps_pin)
{
    wac_lib_user_private * priv = arg;
    int ret;

    WAC_APP_DEBUG_PRINTF("%s\n", __func__);

    if (priv && priv->wac_lib_handle) {
        ret = ath_wac_wps_connect_start_req(priv->wac_lib_handle, bssid, wps_pin);
    }
}

/*
 * The p_bss_info pointer can be invalid after ath_wac_scan_done_event returns.
 * If a user needs to save p_bss_info, save it to the user memory.
 */
void ath_wac_reconnect_start_event(void *arg, wac_scan_bss_info_t *p_bss_info)
{
    wac_lib_user_private * priv = arg;
    int ret;

    WAC_APP_DEBUG_PRINTF("%s\n", __func__);

    if (priv && priv->wac_lib_handle) {
        ret = ath_wac_reconnect_start_req(priv->wac_lib_handle, p_bss_info);
    }
}

void ath_wac_wps_reject_event(void *arg, unsigned int error_code)
{
    (void)arg;

    WAC_APP_DEBUG_PRINTF("%s - Probe response received with reject code %d, WPS start FAILED!\n", 
                            __func__, error_code);
}

#ifndef WAC_CLI_APP
int main (int argc, char **argv)
{
    int ret = -1;
    int i, max_count = 10;
    int val = -1;
    WAC_STATUS status = WAC_DISABLED;
    char ifname[16];
    char *ethIf;

    p_app_priv = malloc(sizeof(wac_lib_user_private));
    if (p_app_priv == NULL) {
        exit(-1);
    }

    memset(p_app_priv, 0, sizeof(wac_lib_user_private));

    p_app_priv->enable = 1;
    p_app_priv->period = 2000;
    p_app_priv->scan_thres = 3;
    p_app_priv->rssi_thres = 95;
    p_app_priv->wps_pin = 12345670;

    client.arg = p_app_priv;

    memset(ifname, '\0', sizeof(ifname));
    if ((ethIf = getenv("NETIF")) == NULL) {
        ethIf = "eth1";
    }
    strncpy(ifname, ethIf, sizeof(ifname));

    progname = argv[0];

    while (1) {
        int c;
        int option_index = 0;
        static struct option long_options[] = {
            {"wac", 1, NULL, 501},
            {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "i:", long_options, &option_index);

        if (c == -1)
        break;

        switch (c) {
        case 'i':
            memset(ifname, '\0', 16);
            strncpy(ifname, optarg, sizeof(ifname));
            strncpy(client.ifname, ifname, sizeof(ifname));
            break;

        case 501:
            if ((optind + 3) > argc) {
                usage();
                goto done;
            }

            p_app_priv->enable = atoi(argv[optind - 1]);
            if (p_app_priv->enable) {
                p_app_priv->enable = 1;
            }
            p_app_priv->period = atoi(argv[optind + 0]);
            p_app_priv->scan_thres = atoi(argv[optind + 1]);
            p_app_priv->rssi_thres = atoi(argv[optind + 2]);
            break;

        default:
            usage();
            break;
        }
    }

    p_app_priv->wac_lib_handle = ath_wac_lib_register(&client);
    if (p_app_priv->wac_lib_handle == NULL) {
        free(p_app_priv);
        exit(-1);
    }

    ret = ath_wac_enable(p_app_priv->wac_lib_handle, p_app_priv->enable, p_app_priv->period,
                   p_app_priv->scan_thres, p_app_priv->rssi_thres, p_app_priv->wps_pin);

    for (i = 0; i < max_count; i++) {
        ath_wac_control_request(p_app_priv->wac_lib_handle, WAC_GET,
                                WAC_GET_STATUS, NONE, NULL, &val, &status);

        WAC_APP_DEBUG_PRINTF("ath_wac_control_request(val = %d, status = %d)\n", val, status);
        sleep(1);
    }

    ath_wac_lib_unregister(p_app_priv->wac_lib_handle);
    p_app_priv->wac_lib_handle = NULL;

done:
    free(p_app_priv);

    exit(ret);
}
#else
int main (void)
{
    int ret = -1;
    char ifname[16];
    char *ethIf;
    unsigned char c;

    p_app_priv = malloc(sizeof(wac_lib_user_private));
    if (p_app_priv == NULL) {
        exit(-1);
    }

    memset(p_app_priv, 0, sizeof(wac_lib_user_private));

    p_app_priv->enable = 1;
    p_app_priv->period = 2000;
    p_app_priv->scan_thres = 3;
    p_app_priv->rssi_thres = 95;
    p_app_priv->wps_pin = 0;

    client.arg = p_app_priv;

    memset(ifname, '\0', sizeof(ifname));
    if ((ethIf = getenv("NETIF")) == NULL) {
        ethIf = "eth1";
    }
    strncpy(ifname, ethIf, sizeof(ifname));
    strncpy(client.ifname, ifname, sizeof(ifname));

    printf("Press\n"    \
           "'r' - register event listener\n"    \
           "'e' - enable WAC\n"     \
           "'q' or 'u' - unregister event listener and quit\n");
           
    while ((c = getchar())) {
        switch (c) {
        case 'r':
            p_app_priv->wac_lib_handle = ath_wac_lib_register(&client);
            if (p_app_priv->wac_lib_handle == NULL) {
                free(p_app_priv);
                exit(-1);
            }
            break;

        case 'e':
            ret = ath_wac_enable(p_app_priv->wac_lib_handle, p_app_priv->enable, p_app_priv->period,
                           p_app_priv->scan_thres, p_app_priv->rssi_thres, p_app_priv->wps_pin);
            break;

        case 'q':
        case 'u':
            ath_wac_lib_unregister(p_app_priv->wac_lib_handle);
            p_app_priv->wac_lib_handle = NULL;
            goto done;
            break;

        default:
            break;
        }
    }

done:
    free(p_app_priv);

    exit(ret);
}
#endif

#else  /* WAC_ENABLE */
int main(void)
{
    return 0;
}
#endif /* WAC_ENABLE */
