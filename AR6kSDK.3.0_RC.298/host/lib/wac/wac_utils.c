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
 * Wifi Auto Configuration (WAC) Utility Functions Implementation
 * 
 *  Author(s): ="Atheros"
 * ==============================================================================
 */
#ifdef  WAC_ENABLE

#include <stdio.h>
#include <string.h>
#include "a_config.h"
#include "a_osapi.h"
#include "a_types.h"
#include "athdefs.h"
#include "wac_internal.h"
#include "wac_utils.h"

/* ----------------- WAC utility static functions -------------------- */
static unsigned int calc_checksum(unsigned int pin)
{
	unsigned int tmp = 0;
	while (pin) {
		tmp += 3 * (pin % 10);
		pin /= 10;
		tmp += pin % 10;
		pin /= 10;
	}

	return (10 - tmp % 10) % 10;
}

int htoi(char c)
{
    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10);
    }

    if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10);
    }

    if (c >= '0' && c <= '9') {
        return (c - '0');
    }

    return 0;
}

void str2hex(A_UINT8 *hex, char *str)
{
    int i;
    int len = strlen(str);

    for (i = 2; i < len; i += 2) {
        hex[(i - 2) / 2] = htoi(str[i]) * 16 + htoi(str[i + 1]);
    }
}

unsigned int wac_generate_random_pin(char *my_pin)
{
	FILE *fHandle;
	unsigned int buf;
	char pin[9];

	fHandle = fopen("/dev/urandom", "rb");
	if (NULL == fHandle) {
		printf("Could not open /dev/urandom.\n");
		return -1;
	}

	fread(&buf, 1, sizeof(unsigned int), fHandle);
	fclose(fHandle);
	
	buf %= 10000000;
    
    buf = buf * 10 + calc_checksum(buf);

	printf("Random PIN = %d\n", buf);

	snprintf(pin, 9, "%08d", buf);
    memcpy(my_pin, pin, 8);

	return buf;
}

/*
 * AP scanning/selection
 * By default, wpa_supplicant requests driver to perform AP scanning and then
 * uses the scan results to select a suitable AP. Another alternative is to
 * allow the driver to take care of AP scanning and selection and use
 * wpa_supplicant just to process EAPOL frames based on IEEE 802.11 association
 * information from the driver.
 * 1: wpa_supplicant initiates scanning and AP selection
 * 0: driver takes care of scanning, AP selection, and IEEE 802.11 association
 *    parameters (e.g., WPA IE generation); this mode can also be used with
 *    non-WPA drivers when using IEEE 802.1X mode; do not try to associate with
 *    APs (i.e., external program needs to control association). This mode must
 *    also be used when using wired Ethernet drivers.
 * 2: like 0, but associate with APs using security policy and SSID (but not
 *    BSSID); this can be used, e.g., with ndiswrapper and NDIS drivers to
 *    enable operation with hidden SSIDs and optimized roaming; in this mode,
 *    the network blocks in the configuration file are tried one by one until
 *    the driver reports successful association; each network block should have
 *    explicit security policy (i.e., only one option in the lists) for
 *    key_mgmt, pairwise, group, proto variables
 */
int wpa_ap_scan_set(void *handle, int enable)
{
    int ret = -1;
    wac_lib_private_t *priv = (wac_lib_private_t *)handle;
    char full_cmd[128];

    WAC_LIB_DEBUG_PRINTF("%s(enable = %d)\n", __func__, enable);

    if (NULL == priv) {
        return ret;
    }

    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    if (enable) {
        ret = system(strcat(full_cmd, "wpa_cli ap_scan 1"));
        priv->ap_scan = 1;
    } else {
        ret = system(strcat(full_cmd, "wpa_cli ap_scan 0"));
        priv->ap_scan = 0;
    }
    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    ret = system(strcat(full_cmd, "wpa_cli save_config"));

    return ret;
}

/* run system command and capture result */
int exec_cmd(void *handle, char *cmd_to_exec, char *result)
{
    char full_cmd[128];
    FILE* fp;
    wac_lib_private_t *priv = (wac_lib_private_t *)handle;

    if (NULL == cmd_to_exec) {
        return -1;
    }

    memset(full_cmd, '\0', 128);
    strcpy(full_cmd, priv->client.wpa_cli_path);
    strcat(full_cmd, cmd_to_exec);

    fp = popen(full_cmd, "r");
    if (fp == NULL) {
        return -1;
    }

    if (NULL != result) {
        /* read the result buffer twice because the output of wpa_cli contains
         * info about the eth interface in the first line ended with '\n', so
         * throw it away by the first fgets
         * The actual result is from the second fgets
         */
        fgets(result, 128, fp);
        fgets(result, 128, fp);
    }

    pclose(fp);

    return 0;
}

/*
 * This function checks the supplicant has a stored network profile
 * Returns a flag that contains a bit mask:
 * PROFILE_NONE     -  no stored profile
 * PROFILE_FOUND    -  stored profile exists, but not matched
 * PROFILE_MATCHED  -  stored profile is matched
 *
 * How it works:
 * 1. use wpa_cli to query if a stored network profile exists
 *      - add_network and remove_network find out the network id of the stored profile
 *      - add_network returns the next id to be added, so current profile is id-1
 *      - id of 0 means no stored profile
 * 2. if network profile exists, use get_network to query its bssid and ssid
 * 3. check if the BSSID matched
 * 4. check if the SSID matched with scan result. If not, replace the network profile
 *    SSID with the scan SSID using set_network
 *
 */
int wpa_supplicant_profile_check(void *handle, wac_scan_bss_info_t *bss_info)
{
    int any_profile = PROFILE_NONE;
    int i, profile_ssid_len;
    char result[128];
    char cmd[128];
    char network_id[4];
    unsigned char bssid[6];

    WAC_LIB_DEBUG_PRINTF("%s\n", __func__);

    memset(result, '\0', 128);
    exec_cmd(handle, "wpa_cli add_network", result);
    memset(network_id, '\0', 4);
    strncpy(network_id, result, 4);
    /* returned result from wpa_cli command may have newline characters
     * replace them with NULL char so further wpa_cli commands won't be broken up
     */
    for (i = 0; i < 4; i++) {
        if ('\n' == network_id[i]) {
            network_id[i] = '\0';
        }
    }


    memset(cmd, '\0', 128);
    strcpy(cmd, "wpa_cli remove_network ");
    exec_cmd(handle, strcat(cmd, result), NULL);

    if (0 == htoi(network_id[0])) {
        WAC_LIB_DEBUG_PRINTF("Supplicant has NO stored network profile!\n");
        return any_profile;
    }
    else if (htoi(network_id[0]) > 1) {
        WAC_LIB_DEBUG_PRINTF("Supplicant has more than 1 stored network profile!\n");
        return any_profile;
    }
    else {
        /* network_id from 'add_network' is the next id to be added, so the id of 
         * the stored profile is 1 below it
         */
        network_id[0]--;
        /* we just found a profile, don't know if it's a match yet
         */
        any_profile |= PROFILE_FOUND;
        WAC_LIB_DEBUG_PRINTF("Supplicant has stored profile at network id: %d\n", htoi(network_id[0]));
    }

    /* query the stored profile's bssid by 'get_network bssid'
     * result is in the format: XX:XX:XX:XX:XX:XX
     */
    memset(result, '\0', 128);
    memset(cmd, '\0', 128);
    strcpy(cmd, "wpa_cli get_network ");
    strcat(cmd, network_id);
    exec_cmd(handle, strcat(cmd, " bssid"), result);

    WAC_LIB_DEBUG_PRINTF("profile bssid = %s\n", result);

    bssid[0] = htoi(result[0]) * 16 + htoi(result[1]);
    bssid[1] = htoi(result[3]) * 16 + htoi(result[4]);
    bssid[2] = htoi(result[6]) * 16 + htoi(result[7]);
    bssid[3] = htoi(result[9]) * 16 + htoi(result[10]);
    bssid[4] = htoi(result[12]) * 16 + htoi(result[13]);
    bssid[5] = htoi(result[15]) * 16 + htoi(result[16]);

    if (!memcmp(bss_info->bssid, bssid, sizeof(bssid))) {
        WAC_LIB_DEBUG_PRINTF("bssid MATCHED\n");
        any_profile |= PROFILE_MATCHED;
    }

    /* query the stored profile's ssid by 'get_network ssid'
     * result is in the format: "network ssid"
     */
    memset(result, '\0', 128);
    memset(cmd, '\0', 128);
    strcpy(cmd, "wpa_cli get_network ");
    strcat(cmd, network_id);
    exec_cmd(handle, strcat(cmd, " ssid"), result);
    /* returned result from wpa_cli command may have newline characters
     * replace them with NULL char so further wpa_cli commands won't be broken up
     * ssid is at 32 chars so just search past it to look for '\n'
     * so the SSID is null terminated
     */
    for (i = 0; i < 40; i++) {
        if ('\n' == result[i]) {
            network_id[i] = '\0';
        }
    }

    WAC_LIB_DEBUG_PRINTF("profile ssid = %s\n", result);
    /* adjustment of 2 is made for the double quotes enclosing the SSID
     */
    profile_ssid_len = strlen(result) - 2;

    /* if scan SSID and the profile SSID are not the same or of different lengths
     * replace the profile SSID with the scan SSID
     * profile ssid starts at result[1]; result[0] is a dobule quote
     */
    if ((strncmp((const char *)bss_info->ssid, &result[1], bss_info->ssid_len)) ||
        (profile_ssid_len != bss_info->ssid_len)) {
        WAC_LIB_DEBUG_PRINTF("profile ssid NOT matching scan result! Use ssid from scan\n");
        bss_info->ssid[bss_info->ssid_len] = '\0';

        /* set the new profile SSID with 'set_network ssid "name"'
         * then save_config
         */
        memset(cmd, '\0', 128);
        strcpy(cmd, "wpa_cli set_network ");
        strcat(cmd, network_id);
        strcat(cmd, " ssid \\\"");
        strncat(cmd, (const char *)bss_info->ssid, bss_info->ssid_len);
        exec_cmd(handle, strcat(cmd, "\\\""), NULL);

        memset(cmd, '\0', 128);
        exec_cmd(handle, strcpy(cmd, "wpa_cli save_config"), NULL);
    }
    else {
        WAC_LIB_DEBUG_PRINTF("profile ssid matching scan result\n");
    }

    return any_profile;
}

#endif /* WAC_ENABLE */
