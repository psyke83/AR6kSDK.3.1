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
 * Wifi Auto Configuration (WAC) Internal Definitions Header
 * 
 *  Author(s): ="Atheros"
 * ==============================================================================
 */
#ifdef  WAC_ENABLE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <getopt.h>
#include <limits.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/types.h>
#include <linux/if.h>
#include <linux/version.h>
#ifdef ANDROID
#include "wireless_copy.h"
#else
#include <linux/wireless.h>
#endif

#include <a_config.h>
#include <a_osapi.h>
#include <a_types.h>
#include <athdefs.h>
#include <wmi.h>
#include <athdrv_linux.h>

#include "ath_wac_lib.h"

#ifndef _WAC_INTERNAL_H
#define _WAC_INTERNAL_H

/* typedefs */
typedef struct wac_lib_private {
    /* socket */
    struct nlmsghdr     hdr;
    char                buf[16384]; /* buf to receive wireless events */
    int                 sock;       /* socket to receive wireless events and send ioctl */

    /* wac_lib_user client config and callback function pointers */
    wac_lib_user_t      client;

    /* WAC SCAN */
    WMI_WAC_BSS_INFO_REPORT  wac_bss_info[MAX_WAC_BSS];
    unsigned int        wac_bss_info_count; /* reset on ath_wac_enable */

    wac_scan_bss_info_t wac_bss_info_user;  /* cached in ath_wac_start_req */

    /* WPS */
    unsigned int        wps_pending;        /* 1 during WPS operation */
    unsigned char       wps_bssid[6];       /* WPS bssid */
    unsigned int        wps_pin;            /* WPS ping  */

    /* status */
    int                 connected;      /* true or false */
    int                 ap_scan;        /* wpa_supplicant ap_scan 0, 1, or 2 */
    int                 wac_scan_count;

    /* wac_enable parameters */
    int                 wac_enable;     /* 1: enable, 0: disable */
    unsigned int        wac_period;
    unsigned int        wac_scan_thres;
    int                 wac_rssi_thres;

    /* thread */
    pthread_t           event_thread_id;
    int                 event_thread_started; /* true or false */
} wac_lib_private_t;


typedef struct {
    int                         id;
    union {
        WMI_WAC_ENABLE_CMD      wac_enable;
        WMI_WAC_CTRL_REQ_CMD    wac_ctrl_req;
        WMI_WAC_SCAN_REPLY_CMD  wac_scan_reply;
    } u;
} IOCTL_WMI_ID_CMD;

#define DEFAULT_WPA_CLI_PATH                "./"
#define DEFAULT_IFNAME                      "eth1"
#define DEFAULT_WPS_PIN                     12345670
#define EVENT_THREAD_CREATE_WAIT_TIME       5           /* seconds */


#endif /* _WAC_INTERNAL_H */

#endif /* WAC_ENABLE */
