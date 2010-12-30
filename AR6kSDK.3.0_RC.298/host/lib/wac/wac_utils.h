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
 * Wifi Auto Configuration (WAC) Utility Functions Header
 * 
 *  Author(s): ="Atheros"
 * ==============================================================================
 */
#ifdef  WAC_ENABLE

#ifndef _WAC_UTILS_H
#define _WAC_UTILS_H

/* convert hex in char to int */
int htoi(char c);

/* convert a string of hex to actual hex numbers */
void str2hex(A_UINT8 *hex, char *str);

/* generate 8-digit random PIN */
unsigned int wac_generate_random_pin(char *my_pin);

/* run system command and capture result */
int exec_cmd(void *handle, char *cmd_to_exec, char *result);

/* set ap_scan flag for supplicant */
int wpa_ap_scan_set(void *handle, int enable);

/* check if supplicant has stored network profile */
int wpa_supplicant_profile_check(void *handle, wac_scan_bss_info_t *bss_info);


/* flag for network profile existence */
#define PROFILE_NONE        0x0
#define PROFILE_FOUND       0x1
#define PROFILE_MATCHED     0x2


#ifdef WAC_LIB_DEBUG
#define WAC_LIB_DEBUG_PRINTF(args...)        printf(args);
#else
#define WAC_LIB_DEBUG_PRINTF(args...)
#endif

/*
 * The memory area pointed by g_wac_lib_priv needs to be protected by mutex.
 * However, in order to avoid deadlock, code needs to be written carefully.
 */
#ifdef WAC_LIB_USE_MUTEX
#define WAC_LIB_MUTEX_LOCK()        pthread_mutex_lock(&g_wac_lib_mutex)
#define WAC_LIB_MUTEX_UNLOCK()      pthread_mutex_unlock(&g_wac_lib_mutex)
#else
#define WAC_LIB_MUTEX_LOCK()
#define WAC_LIB_MUTEX_UNLOCK()
#endif

#endif  /* _WAC_UTILS_H */

#endif /* WAC_ENABLE */
