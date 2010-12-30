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
 * Wifi Auto Configuration (WAC) Library APIs Header
 * 
 *  Author(s): ="Atheros"
 * ==============================================================================
 */
#ifdef  WAC_ENABLE

#ifndef _ATH_WAC_LIB_H_
#define _ATH_WAC_LIB_H_

#include "wac_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WAC_LIB_DEBUG
/* #define WAC_LIB_USE_MUTEX */

#define MAX_WAC_BSS     10  /* This number should be match with F/W */

/* The WAC BSS info structure used between wac_lib_user and wac_lib */
typedef struct wac_scan_bss_info {
    unsigned short channel;
    unsigned char  snr;
    int            rssi;         /* calculated from snr */
    unsigned char  bssid[6];
    unsigned char  ssid[32 + 1]; /* null-terminated, unused */
    unsigned char ssid_len;
} wac_scan_bss_info_t;

/*
 * All p_bss_info pointers in event callbacks can be invalid after 
 * ath_wac_scan_done_event returns.
 * If a user needs to save p_bss_info, save it to the user memory.
 */
typedef struct wac_lib_user {
    /* config */
    char wpa_cli_path[128]; /* The path to run wpa_cli, for example, "./"      */
    char ifname[16];        /* The network interface name, for example, "eth1" */
    void *arg;              /* The arg to be handed over to the 1st argument of event callbacks */
                            /* wac_lib doesn't use arg. Set NULL if arg is not used */
    /* event callbacks */
    void (*wmi_connect_event)(void *arg);
    void (*wmi_disconnect_event)(void *arg);

    int  (*wac_scan_done_event)(void *arg, wac_scan_bss_info_t *p_bss_info, unsigned int num_of_bss_info);
    /* unsigned char bssid[6], wps_pin = 12345670 */
    void (*wac_wps_start_event)(void *arg, unsigned char *bssid, unsigned int wps_pin);
    void (*wac_reconnect_start_event)(void *arg, wac_scan_bss_info_t *p_bss_info);
    void (*wac_wps_reject_event)(void *arg, unsigned int error_code);
} wac_lib_user_t;

/*
 * Function to register wac_lib client
 *
 * memory allocated and thread created to receive wireless event on ath_wac_lib_register call
 * In order to terminate thread and free the allocated momory, ath_wac_lib_unregister must be called.
 *
 * ARGUMENTS
 *  client->wpa_cli_path : The path to run wpa_cli
 *  client->ifname : network interface name
 *  client->arg : arg to be handed over to the 1st argument of event callbacks 
 *  client->wmi_connect_event
 *  client->wmi_disconnect_event
 *  client->wac_scan_done_event
 *  client->wac_wps_start_event
 *  client->wac_reconnect_start_event
 *
 * RETURN VALUE:
 *  The value returned is NULL on error, NON-NULL handle on success
 *  This handle should be used to use all ath_wac_xxx APIs except ath_wac_lib_register.
 */
void *ath_wac_lib_register(wac_lib_user_t *client);

/*
 * Function to unregister wac_lib client
 *
 * ARGUMENTS
 *  handle : the handle returned on ath_wac_lib_register() call
 *
 * RETURN VALUE:
 *      The value returned is 0 on success, -1 on error
 *
 */
int   ath_wac_lib_unregister(void *handle);

/*
 * Function to enable or disable the WAC feature. This is the entry point to WAC when the
 * user pushed a button on the remote
 * Input arguments:
 *      handle - the handle returned on ath_wac_lib_register() call
 *      enable - flag to enable/disable WAC. 1 = enable; 0 = disable
 *               for disablethe remaining arguments are don't cares
 *      period - time in milliseconds between consecutive scans when WAC is enabled
 *      scan_thres - number of scan retries before the STA gave up on looking for WAC AP
 *      rssi_thres - RSSI threshold the STA will check in beacon or probe response frames
 *                   to qualify a WAC AP. This is absolute value of the signal strength in dBm
 *      wps_pin = 8 digit pin with correct checksum
 * Return value:
 *      0  = success;   -1 = failure
 *
 * Examples:
 * To enable WAC:  ath_wac_enable(handle, 1, 2000, 3, 25, 12345670)
 *      - WAC enabled with 2-second interval between scans for up to 3 tries. RSSI threshold is -25dBm
 * To disable WAC: ath_wac_enable(handle, 0, 0, 0, 0, 0) or ath_wac_disable(handle)
 *      - When the flag is disable, the rest of arguments are don't cares
 */
int ath_wac_enable(void *handle,
                   int enable,
                   unsigned int period,
                   unsigned int scan_thres,
                   int rssi_thres,
                   unsigned int wps_pin);

/* 
 * Function to disable wac with ifname only and without handle
 * This can be called without calling ath_wac_lib_register() 
 */               
int ath_wac_disable(void *handle);

/*
 * Function to disable the WAC feature.
 *  Since WAC is disabled when STA is connected to an WAC AP automatically,
 *  there is no need to call ath_wac_disable.
 */                   
int ath_wac_disable_ioctl(const char *ifname);

/*
 * Function for I/O Control Request for Samsung IE as specified version 1.2 protocol
 * Input arguments:
 *      req - request type takes the possible values:
 *              WAC_GET
 *              WAC_SET
 *      cmd - command takes the possible values:
 *              WAC_ADD
 *              WAC_DEL
 *              WAC_GET_STATUS
 *      frm - frame type takes the possible values:
 *              PRBREQ (for STA)
 *              PRBRSP (for AP)
 *              BEACON (for AP)
 *      ie - pointer to char string that contains the IE to set or get
 *      ret_val - indicates whether the control request is successful
 *              0  = success; -1 = failure
 *      status - status code returned by STA that takes possible values:
 *              WAC_FAILED_NO_WAC_AP
 *              WAC_FAILED_LOW_RSSI
 *              WAC_FAILED_INVALID_PARAM
 *              WAC_FAILED_REJECTED
 *              WAC_SUCCESS
 *              WAC_PROCEED_FIRST_PHASE
 *              WAC_PROCEED_SECOND_PHASE
 *              WAC_DISABLED
 * Examples:
 * To insert an IE into the probe request frame:
 *      ath_wac_control_request(handle, WAC_SET, WAC_ADD, PRBREQ,
 *                          "0x0012fb0100010101083132333435363730", val, status)
 * To query the WAC status from STA:
 *      ath_wac_control_request(handle, WAC_GET, WAC_GET_STATUS, NULL, NULL, val, status)
 */
int ath_wac_control_request(void *handle,
                            WAC_REQUEST_TYPE req,
                            WAC_COMMAND cmd,
                            WAC_FRAME_TYPE frm,
                            char *ie,
                            int *ret_val,
                            WAC_STATUS *status);

/*
 * The function to start WAC connection.
 *  F/W will send unicast probe request with WPS PIN to the selected bssid WAC AP
 *  WAC AP will send unicast probe response with status code.
 */
int ath_wac_start_req(void *handle, wac_scan_bss_info_t *p_bss_info);

/*
 * The function to start WPS connection.
 */
int ath_wac_wps_connect_start_req(void *handle, unsigned char *bssid, unsigned int wps_pin);

/*
 * The function to start wpa_supplicant profile connection.
 */
int ath_wac_reconnect_start_req(void *handle, wac_scan_bss_info_t *p_bss_info);

/* 
 * Function to query the number of WAC BSS or the list of BSS
 * When p_bss_info == NULL, it returns the number of WAC BSS
 * When p_bss_info != NULL, it copies the BSS info to the memory pointed to by 
 * p_bss_info and returns of the number of WAC BSS
 * User should query the number of BSS first by using p_bss_info == NULL to 
 * ensure enough memory to store the BSS info
 */
int ath_wac_bss_list_get(void *handle, wac_scan_bss_info_t *p_bss_info);

/* WAC connection flow

 1. 1st connection

    wac_lib_user   ----  ath_wac_lib_register          ----> wac_lib
      memory alloc, thread creation to receive wireless events
                       return non-zero handle

    wac_lib_user   ----  ath_wac_enable                ----> wac_lib

    wac_lib_user   <---- wac_scan_done_event           ----  wac_lib

    wac_lib_user   ----  ath_wac_start_req             ----> wac_lib

                    < If profile doesn't exist >
             < AR6000_XIOCTL_WAC_SCAN_REPLY with cmdid >= 0 >

                 < tx'd unicast probe request with wps PIN >
              < rx'd unicast probe response with status code 0>

    wac_lib_user   <---- wac_wps_start_event           ----  wac_lib

    wac_lib_user   ----  ath_wac_wps_connect_start_req ----> wac_lib

    wac_lib_user   <---- wmi_connect_event             ----  wac_lib

                     < WAC disabled automatically >
                        
                        < WPS message exchange >

    wac_lib_user   <---- wmi_disconnect_event          ----  wac_lib

    wac_lib_user   <---- wmi_connect_event             ----  wac_lib

            < WPA-PSK 4 way handshake message exchange >

    wac_lib_user   ----  ath_wac_lib_unregister        ----> wac_lib

 ---------------------------------------------------------------------
 
 2. reconnection

    wac_lib_user   ----  ath_wac_lib_register          ----> wac_lib
      memory alloc, thread creation to receive wireless events
                       return non-zero handle

    wac_lib_user   ----  ath_wac_enable                ----> wac_lib

    wac_lib_user   <---- wac_scan_done_event           ----  wac_lib

    wac_lib_user   ----  ath_wac_start_req             ----> wac_lib

                   < If profile already exists >
    < AR6000_XIOCTL_WAC_SCAN_REPLY with cmdid = -2 to stop WAC scan >
                         
    wac_lib_user   <---- wac_reconnect_start_event     ----  wac_lib

    wac_lib_user   ----  ath_wac_reconnect_start_req   ----> wac_lib

    wac_lib_user   <---- wmi_connect_event             ----  wac_lib

            < WPA-PSK 4 way handshake message exchange >

    wac_lib_user   ----  ath_wac_lib_unregister        ----> wac_lib

 */


#ifdef __cplusplus
}
#endif

#endif  /* _ATH_WAC_LIB_H_ */

#endif /* WAC_ENABLE */
