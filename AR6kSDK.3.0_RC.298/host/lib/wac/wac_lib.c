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
 * Wifi Auto Configuration (WAC) Library Implementation
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

#include "wac_internal.h"
#include "wac_utils.h"


/* Note for relationship between wps_pending and ap_scan
 ------------------------------------------------
   wps_start_req: wps_pending = 1, ap_scan = 1
   1st connect  : wps_pending = 1, ap_scan = 1
   disconnect   : wps_pending = 0, ap_scan = 1
   2nd connect  : wps_pending = 0, ap_scan = 0
   disconnect   : wps_pending = 0, ap_scan = 0 (if disconnected)
 ------------------------------------------------
 */

/* statics */
static wac_lib_private_t * g_wac_lib_priv = NULL;
static pthread_mutex_t     g_wac_lib_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t      g_wac_lib_cond = PTHREAD_COND_INITIALIZER;

static void wmi_connect_event_handler(A_INT8* buf);
static void wmi_disconnect_event_handler(A_INT8* buf);
static void wmi_wac_report_bss_event_handler(A_INT8* buf);
static void wmi_wac_scan_done_event_handler(A_INT8* buf);
static void wmi_wac_start_wps_event_handler(A_INT8* buf);
static void wmi_wac_reject_wps_event_handler(A_INT8* buf);

/* ----------------- WAC library static functions -------------------- */
/*
 * wac_scan_reply_cmd_req
 * cmdid >= 0 : F/W will try to do WAC connection
                tx'd unicast probe request and rx'd unicast probe response
 * cmdid == -1: F/W will try to continue WAC scan
 * cmdid == -2: F/W will try to stop WAC scan
 */
static int wac_scan_reply_cmd_req(void *handle, int cmdid)
{
    wac_lib_private_t *priv = handle;
    struct ifreq ifr;
    IOCTL_WMI_ID_CMD wmi;
    int ret = -1;
    WMI_WAC_SCAN_REPLY_CMD* cmd;

    if (g_wac_lib_priv == NULL || g_wac_lib_priv != priv) {
        return ret;
    }

    priv = g_wac_lib_priv;

    memset(&wmi, 0, sizeof(IOCTL_WMI_ID_CMD));
    memset(&ifr, 0, sizeof(struct ifreq));

    wmi.id = AR6000_XIOCTL_WAC_SCAN_REPLY;
    cmd = &wmi.u.wac_scan_reply;
    cmd->cmdid = cmdid;
    strncpy(ifr.ifr_name, priv->client.ifname, sizeof(ifr.ifr_name));
    ifr.ifr_data = &wmi;

    ret = ioctl(priv->sock, AR6000_IOCTL_EXTENDED, &ifr);
    if (ret < 0) {
        return ret;
    }

    WAC_LIB_DEBUG_PRINTF("%s(cmdid = %d) success\n", __func__, cmdid);

    return 0;
}

/*
 * This function enables ap_scan and makes wpa_supplicant save config and read config again.
 */
static void wpa_supplicant_profile_reconnect(void)
{
    int ret;
    wac_lib_private_t *priv;
    char full_cmd[128];

    WAC_LIB_DEBUG_PRINTF("%s\n", __func__);

    if (g_wac_lib_priv == NULL) {
        return;
    }

    priv = g_wac_lib_priv;

    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    ret = system(strcat(full_cmd, "wpa_cli ap_scan 1"));

    priv->ap_scan = 1;

    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    ret = system(strcat(full_cmd, "wpa_cli save_config"));

    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    ret = system(strcat(full_cmd,  "wpa_cli reconfigure"));
}

/* ----------------- WAC library event functions --------------------- */
/* ---------- CONSTANTS --------------- */
#define ATH_WE_HEADER_TYPE_NULL     0         /* Not available */
#define ATH_WE_HEADER_TYPE_CHAR     2         /* char [IFNAMSIZ] */
#define ATH_WE_HEADER_TYPE_UINT     4         /* __u32 */
#define ATH_WE_HEADER_TYPE_FREQ     5         /* struct iw_freq */
#define ATH_WE_HEADER_TYPE_ADDR     6         /* struct sockaddr */
#define ATH_WE_HEADER_TYPE_POINT    8         /* struct iw_point */
#define ATH_WE_HEADER_TYPE_PARAM    9         /* struct iw_param */
#define ATH_WE_HEADER_TYPE_QUAL     10        /* struct iw_quality */

#define ATH_WE_DESCR_FLAG_DUMP      0x0001    /* Not part of the dump command */
#define ATH_WE_DESCR_FLAG_EVENT     0x0002    /* Generate an event on SET */
#define ATH_WE_DESCR_FLAG_RESTRICT  0x0004    /* GET : request is ROOT only */
#define ATH_WE_DESCR_FLAG_NOMAX     0x0008    /* GET : no limit on request size */

#define ATH_SIOCSIWMODUL            0x8b2f
#define ATH_SIOCGIWMODUL            0x8b2f
#define ATH_WE_VERSION              (A_INT16)22
/* ---------------------------- TYPES ---------------------------- */

/*
 * standard IOCTL looks like.
 */
struct ath_ioctl_description {
    A_UINT8 header_type;    /* NULL, iw_point or other */
    A_UINT8 token_type;     /* Future */
    A_UINT16 token_size;    /* Granularity of payload */
    A_UINT16 min_tokens;    /* Min acceptable token number */
    A_UINT16 max_tokens;    /* Max acceptable token number */
    A_UINT32 flags;         /* Special handling of the request */
};


/* -------------------------- VARIABLES -------------------------- */

/*
 * Meta-data about all the standard Wireless Extension request we
 * know about.
 */
static const struct ath_ioctl_description standard_ioctl_descr[] = {
    [SIOCSIWCOMMIT  - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_NULL,
    },
    [SIOCGIWNAME    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_CHAR,
        .flags      = ATH_WE_DESCR_FLAG_DUMP,
    },
    [SIOCSIWNWID    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
        .flags      = ATH_WE_DESCR_FLAG_EVENT,
    },
    [SIOCGIWNWID    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
        .flags      = ATH_WE_DESCR_FLAG_DUMP,
    },
    [SIOCSIWFREQ    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_FREQ,
        .flags      = ATH_WE_DESCR_FLAG_EVENT,
    },
    [SIOCGIWFREQ    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_FREQ,
        .flags      = ATH_WE_DESCR_FLAG_DUMP,
    },
    [SIOCSIWMODE    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_UINT,
        .flags      = ATH_WE_DESCR_FLAG_EVENT,
    },
    [SIOCGIWMODE    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_UINT,
        .flags      = ATH_WE_DESCR_FLAG_DUMP,
    },
    [SIOCSIWSENS    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCGIWSENS    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCSIWRANGE   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_NULL,
    },
    [SIOCGIWRANGE   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = sizeof(struct iw_range),
        .flags      = ATH_WE_DESCR_FLAG_DUMP,
    },
    [SIOCSIWPRIV    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_NULL,
    },
    [SIOCGIWPRIV    - SIOCIWFIRST] = { /* (handled directly by us) */
        .header_type    = ATH_WE_HEADER_TYPE_NULL,
    },
    [SIOCSIWSTATS   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_NULL,
    },
    [SIOCGIWSTATS   - SIOCIWFIRST] = { /* (handled directly by us) */
        .header_type    = ATH_WE_HEADER_TYPE_NULL,
        .flags      = ATH_WE_DESCR_FLAG_DUMP,
    },
    [SIOCSIWSPY - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = sizeof(struct sockaddr),
        .max_tokens = IW_MAX_SPY,
    },
    [SIOCGIWSPY - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = sizeof(struct sockaddr) +
                  sizeof(struct iw_quality),
        .max_tokens = IW_MAX_SPY,
    },
    [SIOCSIWTHRSPY  - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = sizeof(struct iw_thrspy),
        .min_tokens = 1,
        .max_tokens = 1,
    },
    [SIOCGIWTHRSPY  - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = sizeof(struct iw_thrspy),
        .min_tokens = 1,
        .max_tokens = 1,
    },
    [SIOCSIWAP  - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_ADDR,
    },
    [SIOCGIWAP  - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_ADDR,
        .flags      = ATH_WE_DESCR_FLAG_DUMP,
    },
    [SIOCSIWMLME    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .min_tokens = sizeof(struct iw_mlme),
        .max_tokens = sizeof(struct iw_mlme),
    },
    [SIOCGIWAPLIST  - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = sizeof(struct sockaddr) +
                  sizeof(struct iw_quality),
        .max_tokens = IW_MAX_AP,
        .flags      = ATH_WE_DESCR_FLAG_NOMAX,
    },
    [SIOCSIWSCAN    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .min_tokens = 0,
        .max_tokens = sizeof(struct iw_scan_req),
    },
    [SIOCGIWSCAN    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_SCAN_MAX_DATA,
        .flags      = ATH_WE_DESCR_FLAG_NOMAX,
    },
    [SIOCSIWESSID   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_ESSID_MAX_SIZE + 1,
        .flags      = ATH_WE_DESCR_FLAG_EVENT,
    },
    [SIOCGIWESSID   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_ESSID_MAX_SIZE + 1,
        .flags      = ATH_WE_DESCR_FLAG_DUMP,
    },
    [SIOCSIWNICKN   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_ESSID_MAX_SIZE + 1,
    },
    [SIOCGIWNICKN   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_ESSID_MAX_SIZE + 1,
    },
    [SIOCSIWRATE    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCGIWRATE    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCSIWRTS - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCGIWRTS - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCSIWFRAG    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCGIWFRAG    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCSIWTXPOW   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCGIWTXPOW   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCSIWRETRY   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCGIWRETRY   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCSIWENCODE  - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_ENCODING_TOKEN_MAX,
        .flags      = ATH_WE_DESCR_FLAG_EVENT | ATH_WE_DESCR_FLAG_RESTRICT,
    },
    [SIOCGIWENCODE  - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_ENCODING_TOKEN_MAX,
        .flags      = ATH_WE_DESCR_FLAG_DUMP | ATH_WE_DESCR_FLAG_RESTRICT,
    },
    [SIOCSIWPOWER   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCGIWPOWER   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [ATH_SIOCSIWMODUL   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    /* warning: initialized field overwritten */
#if 0
    [ATH_SIOCGIWMODUL   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
#endif
    [SIOCSIWGENIE   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_GENERIC_IE_MAX,
    },
    [SIOCGIWGENIE   - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_GENERIC_IE_MAX,
    },
    [SIOCSIWAUTH    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCGIWAUTH    - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_PARAM,
    },
    [SIOCSIWENCODEEXT - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .min_tokens = sizeof(struct iw_encode_ext),
        .max_tokens = sizeof(struct iw_encode_ext) +
                  IW_ENCODING_TOKEN_MAX,
    },
    [SIOCGIWENCODEEXT - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .min_tokens = sizeof(struct iw_encode_ext),
        .max_tokens = sizeof(struct iw_encode_ext) +
                  IW_ENCODING_TOKEN_MAX,
    },
    [SIOCSIWPMKSA - SIOCIWFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .min_tokens = sizeof(struct iw_pmksa),
        .max_tokens = sizeof(struct iw_pmksa),
    },
};
static const unsigned int standard_ioctl_num = (sizeof(standard_ioctl_descr) /
                        sizeof(struct ath_ioctl_description));

/*
 * Meta-data about all the additional standard Wireless Extension events
 */
static const struct ath_ioctl_description standard_event_descr[] = {
    [IWEVTXDROP - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_ADDR,
    },
    [IWEVQUAL   - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_QUAL,
    },
    [IWEVCUSTOM - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_CUSTOM_MAX,
    },
    [IWEVREGISTERED - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_ADDR,
    },
    [IWEVEXPIRED    - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_ADDR,
    },
    [IWEVGENIE  - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_GENERIC_IE_MAX,
    },
    [IWEVMICHAELMICFAILURE  - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = sizeof(struct iw_michaelmicfailure),
    },
    [IWEVASSOCREQIE - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_GENERIC_IE_MAX,
    },
    [IWEVASSOCRESPIE    - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = IW_GENERIC_IE_MAX,
    },
    [IWEVPMKIDCAND  - IWEVFIRST] = {
        .header_type    = ATH_WE_HEADER_TYPE_POINT,
        .token_size = 1,
        .max_tokens = sizeof(struct iw_pmkid_cand),
    },
};
static const unsigned int standard_event_num = (sizeof(standard_event_descr) /
                        sizeof(struct ath_ioctl_description));

/* Size (in bytes) of various events */
static const int event_type_size[] = {
    IW_EV_LCP_PK_LEN,   /* ATH_WE_HEADER_TYPE_NULL */
    0,
    IW_EV_CHAR_PK_LEN,  /* ATH_WE_HEADER_TYPE_CHAR */
    0,
    IW_EV_UINT_PK_LEN,  /* ATH_WE_HEADER_TYPE_UINT */
    IW_EV_FREQ_PK_LEN,  /* ATH_WE_HEADER_TYPE_FREQ */
    IW_EV_ADDR_PK_LEN,  /* ATH_WE_HEADER_TYPE_ADDR */
    0,
    IW_EV_POINT_PK_LEN, /* Without variable payload */
    IW_EV_PARAM_PK_LEN, /* ATH_WE_HEADER_TYPE_PARAM */
    IW_EV_QUAL_PK_LEN,  /* ATH_WE_HEADER_TYPE_QUAL */
};

/* Structure used for parsing event list, such as Wireless Events
 * and scan results */
typedef struct event_list {
    A_INT8* end;        /* End of the list */
    A_INT8* current;    /* Current event in list of events */
    A_INT8* value;      /* Current value in event */
} event_list;

/*------------------------------------------------------------------*/
/*
 * Extract the next event from the event list.
 */
static int app_extract_events(
    struct event_list* list,    /* list of events */
    struct iw_event* iwe        /* Extracted event */)
{
    const struct ath_ioctl_description* descr = NULL;
    int event_type = 0;
    unsigned int  event_len = 1;      /* Invalid */
    A_INT8* pointer;
    A_INT16 we_version = ATH_WE_VERSION;
    unsigned  cmd_index;

    /* Check for end of list */
    if ((list->current + IW_EV_LCP_PK_LEN) > list->end)
        return(0);

    /* Extract the event header (to get the event id).
     * Note : the event may be unaligned, therefore copy... */
    memcpy((char *) iwe, list->current, IW_EV_LCP_PK_LEN);

    /* Check invalid events */
    if (iwe->len <= IW_EV_LCP_PK_LEN)
        return(-1);

    /* Get the type and length of that event */
    if (iwe->cmd <= SIOCIWLAST) {
        cmd_index = iwe->cmd - SIOCIWFIRST;
        if (cmd_index < standard_ioctl_num)
            descr = &(standard_ioctl_descr[cmd_index]);
    } else {
        cmd_index = iwe->cmd - IWEVFIRST;
        if (cmd_index < standard_event_num)
            descr = &(standard_event_descr[cmd_index]);
    }
    if (descr != NULL)
        event_type = descr->header_type;
    /* Unknown events -> event_type=0 => IW_EV_LCP_PK_LEN */
    event_len = event_type_size[event_type];
    /* Fixup for earlier version of WE */
    if ((we_version <= 18) && (event_type == ATH_WE_HEADER_TYPE_POINT))
        event_len += IW_EV_POINT_OFF;

    /* Check if we know about this event */
    if (event_len <= IW_EV_LCP_PK_LEN) {
        /* Skip to next event */
        list->current += iwe->len;
        return(2);
    }
    event_len -= IW_EV_LCP_PK_LEN;

    /* Set pointer on data */
    if (list->value != NULL)
        pointer = list->value;          /* Next value in event */
    else
        pointer = list->current + IW_EV_LCP_PK_LEN; /* First value in event */

    /* Copy the rest of the event (at least, fixed part) */
    if ((pointer + event_len) > list->end) {
        /* Go to next event */
        list->current += iwe->len;
        return(-2);
    }
    /* Fixup for WE-19 and later : pointer no longer in the list */
    /* Beware of alignement. Dest has local alignement, not packed */
    if ((we_version > 18) && (event_type == ATH_WE_HEADER_TYPE_POINT))
        memcpy((char *) iwe + IW_EV_LCP_LEN + IW_EV_POINT_OFF, pointer, event_len);
    else
        memcpy((char *) iwe + IW_EV_LCP_LEN, pointer, event_len);

    /* Skip event in the stream */
    pointer += event_len;

    /* Special processing for iw_point events */
    if (event_type == ATH_WE_HEADER_TYPE_POINT) {
        /* Check the length of the payload */
        unsigned int  extra_len = iwe->len - (event_len + IW_EV_LCP_PK_LEN);
        if (extra_len > 0) {
            /* Set pointer on variable part (warning : non aligned) */
            iwe->u.data.pointer = pointer;

            /* Check that we have a descriptor for the command */
            if (descr == NULL)
                    /* Can't check payload -> unsafe... */
                iwe->u.data.pointer = NULL; /* Discard paylod */
            else {
                /* Those checks are actually pretty hard to trigger,
                 * because of the checks done in the kernel... */

                unsigned int  token_len = iwe->u.data.length * descr->token_size;

                /* Ugly fixup for alignement issues.
                 * If the kernel is 64 bits and userspace 32 bits,
                 * we have an extra 4+4 bytes.
                 * Fixing that in the kernel would break 64 bits userspace. */
                if ((token_len != extra_len) && (extra_len >= 4)) {
                    A_UINT16 alt_dlen = *((A_UINT16*) pointer);
                    unsigned int  alt_token_len = alt_dlen * descr->token_size;
                    if ((alt_token_len + 8) == extra_len) {
                        /* Ok, let's redo everything */
                        pointer -= event_len;
                        pointer += 4;
                        /* Dest has local alignement, not packed */
                        memcpy((char *) iwe + IW_EV_LCP_LEN + IW_EV_POINT_OFF, pointer, event_len);
                        pointer += event_len + 4;
                        iwe->u.data.pointer = pointer;
                        token_len = alt_token_len;
                    }
                }

                /* Discard bogus events which advertise more tokens than
                 * what they carry... */
                if (token_len > extra_len)
                    iwe->u.data.pointer = NULL; /* Discard paylod */
                /* Check that the advertised token size is not going to
                 * produce buffer overflow to our caller... */
                if ((iwe->u.data.length > descr->max_tokens) && !(descr->flags & ATH_WE_DESCR_FLAG_NOMAX))
                    iwe->u.data.pointer = NULL; /* Discard paylod */
                /* Same for underflows... */
                if (iwe->u.data.length < descr->min_tokens)
                    iwe->u.data.pointer = NULL; /* Discard paylod */
            }
        } else
            /* No data */
            iwe->u.data.pointer = NULL;

        /* Go to next event */
        list->current += iwe->len;
    } else {
        /* Ugly fixup for alignement issues.
         * If the kernel is 64 bits and userspace 32 bits,
         * we have an extra 4 bytes.
         * Fixing that in the kernel would break 64 bits userspace. */
        if ((list->value == NULL) && ((((iwe->len - IW_EV_LCP_PK_LEN) % event_len) == 4) ||
            ((iwe->len == 12) && ((event_type == ATH_WE_HEADER_TYPE_UINT) ||
            (event_type == ATH_WE_HEADER_TYPE_QUAL))))) {
            pointer -= event_len;
            pointer += 4;
            /* Beware of alignement. Dest has local alignement, not packed */
            memcpy((char *) iwe + IW_EV_LCP_LEN, pointer, event_len);
            pointer += event_len;
        }

        /* Is there more value in the event ? */
        if ((pointer + event_len) <= (list->current + iwe->len))
            /* Go to next value */
            list->value = pointer;
        else {
            /* Go to next event */
            list->value = NULL;
            list->current += iwe->len;
        }
    }
    return(1);
}

static void event_wireless(A_INT8* data, int len)
{
    wac_lib_private_t *priv = g_wac_lib_priv;
    A_INT8* pos, * end, * custom, * buf;
    A_UINT16 eventid;
    struct iw_event iwe;
    struct event_list list;
    int ret;

    pos = data;
    end = data + len;

    /* Cleanup */
    memset((char *) &list, '\0', sizeof(struct event_list));

    /* Set things up */
    list.current = data;
    list.end = data + len;

    do {
        /* Extract an event and print it */
        ret = app_extract_events(&list, &iwe);
        if (ret != 0) {
            /* WAC_LIB_DEBUG_PRINTF("\n cmd = %x, length = %d, ", iwe.cmd, iwe.u.data.length); */
            switch (iwe.cmd) {
            case IWEVCUSTOM:
                custom = pos + IW_EV_POINT_LEN;
                if (custom + iwe.u.data.length > end)
                    return;
                buf = malloc(iwe.u.data.length + 1);
                if (buf == NULL)
                    return;
                memcpy(buf, custom, iwe.u.data.length);
                eventid = *((A_UINT16 *) buf);

                /* WAC_LIB_DEBUG_PRINTF("eventid = %x \n", eventid); */

                switch (eventid) {
                case WMI_READY_EVENTID:
                    WAC_LIB_DEBUG_PRINTF("\nevent = Wmi Ready, len = %d\n", iwe.u.data.length);
                    break;

                case WMI_DISCONNECT_EVENTID:
                    WAC_LIB_DEBUG_PRINTF("\nevent = Wmi Disconnect, len = %d\n", iwe.u.data.length);
                    wmi_disconnect_event_handler(buf);
                    break;

                case WMI_SCAN_COMPLETE_EVENTID:
                    WAC_LIB_DEBUG_PRINTF("\nevent = Wmi Scan Complete, len = %d\n", iwe.u.data.length);
                    break;

                case WMI_WAC_REPORT_BSS_EVENTID:
                    wmi_wac_report_bss_event_handler(buf);
                    break;

                case WMI_WAC_SCAN_DONE_EVENTID:
                    wmi_wac_scan_done_event_handler(buf);
                    break;

                case WMI_WAC_START_WPS_EVENTID:
                    wmi_wac_start_wps_event_handler(buf);
                    break;

                case WMI_WAC_REJECT_WPS_EVENTID:
                    wmi_wac_reject_wps_event_handler(buf);
                    break;

                default:
                    /* WAC_LIB_DEBUG_PRINTF("Host received other event with id 0x%x\n", eventid); */
                    break;
                }
                free(buf);
                break;

            case IWEVGENIE:
                custom = pos + IW_EV_POINT_LEN;
                if (custom + iwe.u.data.length > end)
                    return;
                buf = malloc(iwe.u.data.length + 1);
                if (buf == NULL)
                    return;
                memcpy(buf, custom, iwe.u.data.length);
                eventid = *((A_UINT16 *) buf);

                switch (eventid) {
                case WMI_CONNECT_EVENTID:
                    wmi_connect_event_handler(buf);
                    break;

                case WMI_ENABLE_WAC_CMDID:
                    if (priv && priv->sock >= 0) {
                        WMI_WAC_ENABLE_CMD* cmd = (WMI_WAC_ENABLE_CMD*) (buf + 2);
                        ath_wac_enable(priv, cmd->enable, cmd->period,
                                       cmd->threshold, cmd->rssi, DEFAULT_WPS_PIN);
                    }
                    break;

                default:
                    break;
                }
                free(buf);
                break;

            default:
                /* WAC_LIB_DEBUG_PRINTF("event = Others\n"); */
                break;
            }
        }
    } while (ret > 0);
}

static void event_rtm_newlink(struct nlmsghdr* h, int len)
{
    struct ifinfomsg* ifi;
    int attrlen, nlmsg_len, rta_len;
    struct rtattr* attr;

    if (len < (int)sizeof(*ifi)) {
        perror("too short\n");
        return;
    }

    ifi = NLMSG_DATA(h);

    nlmsg_len = NLMSG_ALIGN(sizeof(struct ifinfomsg));

    attrlen = h->nlmsg_len - nlmsg_len;
    if (attrlen < 0) {
        perror("bad attren\n");
        return;
    }

    attr = (struct rtattr *) (((char *) ifi) + nlmsg_len);

    rta_len = RTA_ALIGN(sizeof(struct rtattr));
    while (RTA_OK(attr, attrlen)) {
        if (attr->rta_type == IFLA_WIRELESS) {
            event_wireless(((A_INT8 *) attr) + rta_len, attr->rta_len - rta_len);
        } else if (attr->rta_type == IFLA_IFNAME) {

        }
        attr = RTA_NEXT(attr, attrlen);
    }
}

/* ------------------- WAC Event Listener  --------------------------- */
static void *event_listener_thread(void *handle)
{
    wac_lib_private_t *priv = handle;
    struct sockaddr_nl from;
    socklen_t fromlen;
    int left;
    struct nlmsghdr* h;
    int len, plen;
    int ret;

    WAC_LIB_DEBUG_PRINTF("%s(handle = %p)\n", __func__, handle);

    if (priv == NULL) {
        pthread_exit(NULL);
    }

    ret = pthread_mutex_lock(&g_wac_lib_mutex);

    priv->event_thread_started = 1;

    ret = pthread_cond_signal(&g_wac_lib_cond);

    ret = pthread_mutex_unlock(&g_wac_lib_mutex);

    while(1) {
        fromlen = sizeof(from);
        left = recvfrom(priv->sock, &priv->hdr, sizeof(priv->buf), 0, (struct sockaddr *)&from, &fromlen);
        if (left < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                WAC_LIB_DEBUG_PRINTF("recvfrom(netlink) errno = %d", errno);
            }
            break;
        }

        h = (struct nlmsghdr *) &priv->hdr;

        while (left >= (int)sizeof(struct nlmsghdr)) {

            len = h->nlmsg_len;
            plen = len - sizeof(*h);
            if (len > left || plen < 0) {
                perror("Malformed netlink message: ");
                break;
            }

            switch (h->nlmsg_type) {
            case RTM_NEWLINK:
                event_rtm_newlink(h, plen);
                break;
            case RTM_DELLINK:
                WAC_LIB_DEBUG_PRINTF("DELLINK\n");
                break;
            default:
                WAC_LIB_DEBUG_PRINTF("OTHERS\n");
            }

            len = NLMSG_ALIGN(len);
            left -= len;
            h = (struct nlmsghdr *) ((char *) h + len);
        }
    }

    pthread_exit(NULL);

    WAC_LIB_DEBUG_PRINTF("%s() exit\n", __func__);
}
/* ------------------- WAC Event Listener  -- END ---------------------- */

/* --------------------- WAC Event Handlers ---------------------------- */
/*
 * event handlers run in event_listener_thread context.
 */
static void wmi_connect_event_handler(A_INT8* buf)
{
    wac_lib_private_t *priv = g_wac_lib_priv;

    (void)buf;

    if (priv == NULL) {
        return;
    }

    priv->connected = 1;

    if (priv->client.wmi_connect_event) {
        priv->client.wmi_connect_event(priv->client.arg);
    }

    ath_wac_disable(priv);

    if (!priv->wps_pending) {
        wpa_ap_scan_set(priv, 0);
    }
}

/* buf contains 16 bit event id and body */
static void wmi_disconnect_event_handler(A_INT8* buf)
{
    wac_lib_private_t *priv = g_wac_lib_priv;

    (void)buf;

    if (priv == NULL) {
        return;
    }

    priv->connected = 0;

    if (priv->client.wmi_disconnect_event) {
        priv->client.wmi_disconnect_event(priv->client.arg);
    }

    if (priv->wps_pending) {
        priv->wps_pending = 0;
        wpa_ap_scan_set(priv, 1);
    }
}

static void wmi_wac_report_bss_event_handler(A_INT8* buf)
{
    wac_lib_private_t *priv = g_wac_lib_priv;
    WMI_WAC_BSS_INFO_REPORT* p;

    if (priv == NULL) {
        return;
    }

    if (priv->wac_bss_info_count >= MAX_WAC_BSS) {
        WAC_LIB_DEBUG_PRINTF("Too many bss reported\n");
        return;
    }

    p = &priv->wac_bss_info[priv->wac_bss_info_count];
    memcpy(p, buf + 2, sizeof(WMI_WAC_BSS_INFO_REPORT));
    priv->wac_bss_info_count++;

    WAC_LIB_DEBUG_PRINTF("WAC_REPORT_BSS_EVENT: %02x:%02x:%02x:%02x:%02x:%02x snr = %2d chan = %2d\n",
        p->bssid[0], p->bssid[1], p->bssid[2],
        p->bssid[3], p->bssid[4], p->bssid[5],
        p->snr, p->channel);
}

static void wmi_wac_scan_done_event_handler(A_INT8* buf)
{
    wac_lib_private_t *priv = g_wac_lib_priv;
    wac_scan_bss_info_t *p_bss_info = NULL;
    wac_scan_bss_info_t *p_info;
    unsigned int num_of_bss_info = 0;
    unsigned int i, j;
    WMI_WAC_BSS_INFO_REPORT *p_info2;

    (void)buf;

    WAC_LIB_DEBUG_PRINTF("%s wac_bss_info_count = %d wac_scan_bss_info_t = %d\n",
                         __func__, priv->wac_bss_info_count, sizeof(wac_scan_bss_info_t));

    if (priv == NULL)
    {
        return;
    }

    priv->wac_scan_count++;
    if (!priv->wac_bss_info_count && 
        ((unsigned int )priv->wac_scan_count < priv->wac_scan_thres)) {
        wac_scan_reply_cmd_req(priv, -1); /* continue WAC scan */
        return;
    }

    if (priv->client.wac_scan_done_event) {
        if (priv->wac_bss_info_count > 0) {
            p_bss_info = malloc(sizeof(wac_scan_bss_info_t) * priv->wac_bss_info_count);
            if (p_bss_info == NULL) {
                return;
            }
            memset(p_bss_info, 0, sizeof(wac_scan_bss_info_t) * priv->wac_bss_info_count);

            p_info = p_bss_info;

            for (i = 0; i < priv->wac_bss_info_count; i++) {
                p_info2 = &priv->wac_bss_info[i];

                for (j = 0; j < num_of_bss_info; j++) {
                    p_info = &p_bss_info[j];
                    /* break if BSSIDs are the same */
                    if (!memcmp(p_info->bssid, p_info2->bssid, sizeof(p_info->bssid))) {
                        break;
                    }
                }

                /* copy only if BSSIDs are different */
                if (j == num_of_bss_info) {
                    memcpy(p_info->bssid, p_info2->bssid, sizeof(p_info->bssid));
                    p_info->snr = p_info2->snr;
                    p_info->channel = p_info2->channel;
                    /* The following info is not given */
                    p_info->rssi = (int)p_info->snr - 95;
                    memcpy(p_info->ssid, p_info2->ssid, p_info2->ssid_len);
                    p_info->ssid_len = p_info2->ssid_len;
                    num_of_bss_info++;
                }
            }
        }

        WAC_LIB_DEBUG_PRINTF("wac_bss_info_count = %d num_of_bss_info = %d\n",
                              priv->wac_bss_info_count, num_of_bss_info);

        priv->client.wac_scan_done_event(priv->client.arg, p_bss_info, num_of_bss_info);

        if (p_bss_info) {
            free(p_bss_info);
            p_bss_info = NULL;
        }
    }

    if (!num_of_bss_info) {
        wac_scan_reply_cmd_req(priv, -2); /* stop WAC scan */
    }
}

static void wmi_wac_start_wps_event_handler(A_INT8* buf)
{
    wac_lib_private_t *priv = g_wac_lib_priv;

    (void)buf;

    if (priv && priv->client.wac_wps_start_event) {
        priv->client.wac_wps_start_event(priv->client.arg, priv->wps_bssid, priv->wps_pin);
    } else {
        /*
         * If priv->client.wac_wps_start_eventt is NULL,
         * let's start WPS automatically
         */
        ath_wac_wps_connect_start_req(priv, priv->wps_bssid, priv->wps_pin);
    }
}

static void wmi_wac_reject_wps_event_handler(A_INT8* buf)
{
    wac_lib_private_t *priv = g_wac_lib_priv;
    unsigned int error_code;

    memcpy(&error_code, buf+2, sizeof(unsigned int));

    if (priv && priv->client.wac_wps_reject_event) {
        priv->client.wac_wps_reject_event(priv->client.arg, error_code);
    } else {
        WAC_LIB_DEBUG_PRINTF("%s - Probe response received with reject code %d, WPS start FAILED!\n", 
                                __func__, error_code);
    }
}

/* ------------------ WAC Event Handlers -- END  ------------------------ */


/* --------------------- WAC library APIs ---------------------------- */

int ath_wac_enable(void *handle,
                   int enable,
                   unsigned int period,
                   unsigned int scan_thres,
                   int rssi_thres,
                   unsigned int wps_pin)
{
    wac_lib_private_t *priv = handle;
    struct ifreq ifr;
    IOCTL_WMI_ID_CMD wmi;
    int ret = -1;
    WMI_WAC_ENABLE_CMD* cmd;
    char wps_pin_a[9];

    if (g_wac_lib_priv == NULL || g_wac_lib_priv != priv) {
        return ret;
    }

    WAC_LIB_MUTEX_LOCK();

    memset(&wmi, 0, sizeof(IOCTL_WMI_ID_CMD));
    memset(&ifr, 0, sizeof(struct ifreq));

    wmi.id = AR6000_XIOCTL_WMI_ENABLE_WAC_PARAM;
    cmd = &wmi.u.wac_enable;
    cmd->enable = enable;
    cmd->period = period;
    cmd->threshold = scan_thres;
    cmd->rssi = rssi_thres;
    if (enable) {
        if (0 == wps_pin) {
            priv->wps_pin = wac_generate_random_pin(cmd->wps_pin);
        } else {
            snprintf(wps_pin_a, 9, "%d", wps_pin);
            memcpy(cmd->wps_pin, wps_pin_a, 8);
            priv->wps_pin = wps_pin;
        }
    }

    strncpy(ifr.ifr_name, priv->client.ifname, sizeof(ifr.ifr_name));
    ifr.ifr_data = &wmi;

    ret = ioctl(priv->sock, AR6000_IOCTL_EXTENDED, &ifr);
    if (ret < 0) {
        goto done;
    }

    priv->wac_enable = enable;
    priv->wac_period = period;
    priv->wac_scan_thres = scan_thres;
    priv->wac_rssi_thres = rssi_thres;

    /* When wac is enabled, f/w will reset wac_bss_info table. */
    if (enable) {
        memset(priv->wac_bss_info, 0, sizeof(priv->wac_bss_info));
        priv->wac_bss_info_count = 0;
        priv->wac_scan_count = 0;
    }

    ret = 0;

done:
    WAC_LIB_MUTEX_UNLOCK();

    WAC_LIB_DEBUG_PRINTF("%s(%d, %d, %d, %d, %s) ret = %d\n",
                   __func__, enable, period, scan_thres, rssi_thres, wps_pin_a, ret);

    return ret;
}

int ath_wac_disable(void *handle)
{
    return ath_wac_enable(handle, 0, 0, 0, 0, 0);
}

/*
 * Function to disable wac with ifname only and without handle
 * This can be called without calling ath_wac_lib_register()
 */
int ath_wac_disable_ioctl(const char *ifname)
{
    int ret = -1;
    int sock = -1;
    struct sockaddr_nl local;
    struct ifreq ifr;
    IOCTL_WMI_ID_CMD wmi;

    sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) {
        WAC_LIB_DEBUG_PRINTF("socket(PF_NETLINK,SOCK_RAW,NETLINK_ROUTE) error\n");
        return -1;
    }

    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;
    local.nl_groups = RTMGRP_LINK;
    if (bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0) {
        WAC_LIB_DEBUG_PRINTF("bind() error\n");
        goto error;
    }

    memset(&wmi, 0, sizeof(IOCTL_WMI_ID_CMD));
    memset(&ifr, 0, sizeof(struct ifreq));

    wmi.id = AR6000_XIOCTL_WMI_ENABLE_WAC_PARAM;

    if (ifname == NULL || ifname[0] == '\0') {
        strncpy(ifr.ifr_name, DEFAULT_IFNAME, sizeof(ifr.ifr_name));
    } else {
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    }

    ifr.ifr_data = &wmi;

    ret = ioctl(sock, AR6000_IOCTL_EXTENDED, &ifr);

error:
    if (sock >= 0) {
        close(sock);
    }

    WAC_LIB_DEBUG_PRINTF("%s(ifname = %s) ret = %d\n", __func__, ifname, ret);

    return ret;
}

int ath_wac_control_request(void *handle,
                            WAC_REQUEST_TYPE req,
                            WAC_COMMAND cmd,
                            WAC_FRAME_TYPE frm,
                            char *ie,
                            int *ret_val,
                            WAC_STATUS *status)
{
    wac_lib_private_t *priv = handle;
    struct ifreq ifr;
    IOCTL_WMI_ID_CMD wmi;
    WMI_WAC_CTRL_REQ_CMD* ctrl_cmd;
    int ret = -1;

    if (g_wac_lib_priv == NULL || g_wac_lib_priv != priv) {
        return ret;
    }

    WAC_LIB_MUTEX_LOCK();

    memset(&wmi, 0, sizeof(IOCTL_WMI_ID_CMD));
    memset(&ifr, 0, sizeof(struct ifreq));

    wmi.id = AR6000_XIOCTL_WMI_WAC_CTRL_REQ;
    ctrl_cmd = &wmi.u.wac_ctrl_req;

    strncpy(ifr.ifr_name, priv->client.ifname, sizeof(ifr.ifr_name));
    ifr.ifr_data = &wmi;

    *ret_val = 0;

    if (WAC_SET == req) {
        if (PRBREQ == frm) {
            switch (cmd) {
            case WAC_ADD:
                if (0 != strncmp(ie, "0x", 2) || 0 != strncmp(ie, "0X", 2)) {
                    WAC_LIB_DEBUG_PRINTF("expect \'ie\' in hex format with 0x\n");
                    *ret_val = -1;
                    goto done;
                }

                if (0 != (strlen(ie) % 2)) {
                    WAC_LIB_DEBUG_PRINTF("expect \'ie\' to be even length\n");
                    *ret_val = -1;
                    goto done;
                }

                ctrl_cmd->req = WAC_SET;
                ctrl_cmd->cmd = WAC_ADD;
                ctrl_cmd->frame = PRBREQ;
                str2hex(ctrl_cmd->ie, ie);
                ret = ioctl(priv->sock, AR6000_IOCTL_EXTENDED, &ifr);
                if (ret < 0) {
                    WAC_LIB_DEBUG_PRINTF("%s ioctl() error = %d\n", __func__, ret);
                    *ret_val = -1;
                }
                break;

            case WAC_DEL:
                ctrl_cmd->req = WAC_SET;
                ctrl_cmd->cmd = WAC_DEL;
                ctrl_cmd->frame = PRBREQ;
                ret = ioctl(priv->sock, AR6000_IOCTL_EXTENDED, &ifr);
                if (ret < 0) {
                    WAC_LIB_DEBUG_PRINTF("%s ioctl() error = %d\n", __func__, ret);
                    *ret_val = -1;
                }
                break;

            default:
                WAC_LIB_DEBUG_PRINTF("unknown command %d\n", cmd);
                *ret_val = -1;
                break;
            }
        } else {
            WAC_LIB_DEBUG_PRINTF("unknown set frame type %d\n", frm);
            *ret_val = -1;
        }
    } else if (WAC_GET == req) {
        if (WAC_GET_STATUS == cmd) {
            /* GET STATUS */
            ctrl_cmd->req = WAC_GET;
            ctrl_cmd->cmd = WAC_GET_STATUS;

            if (ioctl(priv->sock, AR6000_IOCTL_EXTENDED, &ifr) < 0) {
                err(1, "%s", ifr.ifr_name);
                *ret_val = -1;
                goto done;
            }
            A_MEMCPY(status, ifr.ifr_data, sizeof(WAC_STATUS));
        } else if (PRBREQ == frm) {
            /* GET IE */
            ctrl_cmd->req = WAC_GET;
            ctrl_cmd->cmd = WAC_GET_IE;

            ret = ioctl(priv->sock, AR6000_IOCTL_EXTENDED, &ifr);
            if (ret < 0) {
                WAC_LIB_DEBUG_PRINTF("%s ioctl() error = %d\n", __func__, ret);
                *ret_val = -1;
                goto done;
            }

            A_MEMCPY(ie, ifr.ifr_data, sizeof(WMI_GET_WAC_INFO));
        } else {
            WAC_LIB_DEBUG_PRINTF("unknown get request\n");
            *ret_val = -1;
        }
    } else {
        WAC_LIB_DEBUG_PRINTF("unkown request type %d\n", req);
        *ret_val = -1;
    }

done:

    if (*ret_val) {
        ret = -1;
    } else {
        ret = 0;
    }

    WAC_LIB_MUTEX_UNLOCK();
    return ret;
}

int ath_wac_start_req(void *handle, wac_scan_bss_info_t *p_bss_info)
{
    wac_lib_private_t *priv = handle;
    int ret = -1;
    WMI_WAC_BSS_INFO_REPORT *p_info2;
    unsigned int i;
    int has_profile = 0;

    WAC_LIB_DEBUG_PRINTF("%s\n", __func__);

    if (g_wac_lib_priv == NULL || g_wac_lib_priv != priv) {
        return ret;
    }

    WAC_LIB_MUTEX_LOCK();

    memcpy(priv->wps_bssid, p_bss_info->bssid, sizeof(p_bss_info->bssid));

    for (i = 0; i < priv->wac_bss_info_count; i++) {
        p_info2 = &priv->wac_bss_info[i];
        /* break if BSSIDs are the same */
        if (!memcmp(priv->wps_bssid, p_info2->bssid, sizeof(priv->wps_bssid))) {
            break;
        }
    }

    has_profile = wpa_supplicant_profile_check(priv, p_bss_info);

    if (!(has_profile & PROFILE_MATCHED)) {
        if (i < priv->wac_bss_info_count) {
            ret = wac_scan_reply_cmd_req(handle, (int)i);
        }
    } else if ( (has_profile & PROFILE_MATCHED) ||
                ((has_profile & PROFILE_FOUND) &&
                 ((unsigned int )priv->wac_scan_count >= priv->wac_scan_thres)) ) {
        ret = wac_scan_reply_cmd_req(priv, -2); /* stop WAC scan */

        if (priv->client.wac_reconnect_start_event) {
            priv->wac_bss_info_user = *p_bss_info;
            priv->client.wac_reconnect_start_event(priv->client.arg, p_bss_info);
        } else {
            /*
             * If priv->client.wac_reconnect_start_event is NULL,
             * let's reconnect automatically
             */
            wpa_supplicant_profile_reconnect();
        }
        ret = 0;
    }

    WAC_LIB_MUTEX_UNLOCK();
    return ret;
}

int ath_wac_wps_connect_start_req(void *handle, unsigned char *bssid, unsigned int wps_pin)
{
    int ret = 0;
    wac_lib_private_t *priv = handle;
    char full_cmd[128];
    char wps_bssid[32];
    char wps_pin_a[9];

    if (g_wac_lib_priv == NULL || g_wac_lib_priv != priv) {
        return ret;
    }

    WAC_LIB_MUTEX_LOCK();

    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    ret = system(strcat(full_cmd, "wpa_cli ap_scan 1"));

    priv->ap_scan = 1;

    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    ret = system(strcat(full_cmd, "wpa_cli save_config"));

    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    ret = system(strcat(full_cmd, "wpa_cli reconfigure"));

    memset(wps_bssid, '\0', 32);
    snprintf(wps_bssid, sizeof(wps_bssid), "%02x:%02x:%02x:%02x:%02x:%02x",
        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

    snprintf(wps_pin_a, 9, "%d", wps_pin);

    full_cmd[0] = '\0';
    strcpy(full_cmd, priv->client.wpa_cli_path);
    strcat(full_cmd, "wpa_cli wps_pin ");
    strcat(full_cmd, wps_bssid);
    strcat(full_cmd, " ");
    strcat(full_cmd, wps_pin_a);

    priv->wps_pending = 1;

    WAC_LIB_DEBUG_PRINTF("%s: %s\n", __func__, full_cmd);

    ret = system(full_cmd);

    WAC_LIB_MUTEX_UNLOCK();

    return ret;
}

int ath_wac_reconnect_start_req(void *handle, wac_scan_bss_info_t *p_bss_info)
{
    int ret = -1;
    wac_lib_private_t *priv = handle;
    unsigned char *bssid;

    WAC_LIB_DEBUG_PRINTF("%s\n", __func__);

    if (g_wac_lib_priv == NULL || g_wac_lib_priv != priv) {
        return ret;
    }

    WAC_LIB_MUTEX_LOCK();

    /*
     * wac_bss_info_user is cached in ath_wac_start_req.
     */
    if (memcmp(p_bss_info->bssid, priv->wac_bss_info_user.bssid, sizeof(p_bss_info->bssid))) {

        bssid = p_bss_info->bssid;
        WAC_LIB_DEBUG_PRINTF("p_bss_info->bssid = %02x:%02x:%02x:%02x:%02x:%02x\n",
            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

        bssid = priv->wac_bss_info_user.bssid;
        WAC_LIB_DEBUG_PRINTF("wac_bss_info_user.bssid = %02x:%02x:%02x:%02x:%02x:%02x\n",
            bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    }

    wpa_supplicant_profile_reconnect();

    WAC_LIB_MUTEX_UNLOCK();

    return 0;
}

int ath_wac_bss_list_get(void *handle, wac_scan_bss_info_t *p_bss_info)
{
    wac_lib_private_t *priv = (wac_lib_private_t *)handle;
    WMI_WAC_BSS_INFO_REPORT *p_bss_info_report;
    wac_scan_bss_info_t *p;
    unsigned int i;

    if (NULL != p_bss_info) {
        for (i = 0; i < priv->wac_bss_info_count; i++) {
            p_bss_info_report = &priv->wac_bss_info[i];
            p = p_bss_info + i;

            memcpy(p->bssid, p_bss_info_report->bssid, 6);
            memcpy(p->ssid, p_bss_info_report->ssid, p_bss_info_report->ssid_len);
            p->ssid_len = p_bss_info_report->ssid_len;
            p->channel = p_bss_info_report->channel;
            p->snr = p_bss_info_report->snr;
        }
    }

    return priv->wac_bss_info_count;
}

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
void *ath_wac_lib_register(wac_lib_user_t *client)
{
    int ret, rc = 0;
    int sock;
    wac_lib_private_t * priv = NULL;
    struct sockaddr_nl  local;
    struct timespec     ts;
    struct timeval      tp;

    if (client == NULL || g_wac_lib_priv)
    {
        WAC_LIB_DEBUG_PRINTF("ath_wac_lib_register() already registered?\n");
        return priv;
    }

    ret = pthread_mutex_init(&g_wac_lib_mutex, NULL);
    if (ret)
    {
        WAC_LIB_DEBUG_PRINTF("pthread_mutex_init() error\n");
        return priv;
    }

    ret = pthread_cond_init(&g_wac_lib_cond, NULL);
    if (ret)
    {
        WAC_LIB_DEBUG_PRINTF("pthread_cond_init() error\n");

        ret = pthread_mutex_destroy(&g_wac_lib_mutex);
        if (ret) {
            WAC_LIB_DEBUG_PRINTF("pthread_mutex_destroy() error\n");
        }
        return priv;
    }

    priv = malloc(sizeof(wac_lib_private_t));
    if (priv == NULL)
    {
        return priv;
    }

    memset(priv, 0, sizeof(wac_lib_private_t));

    memcpy(&priv->client, client, sizeof(wac_lib_user_t));

    /*
     * Although wpa_cli_path is null, let's not set the default path.
     * wpa_cli can be somewhere in $PATH.
     */
#if 0
    if (priv->client.wpa_cli_path[0] == '\0') {
        snprintf(priv->client.wpa_cli_path, sizeof(priv->client.wpa_cli_path), DEFAULT_WPA_CLI_PATH);
    }
#endif

    if (priv->client.ifname[0] == '\0') {
        snprintf(priv->client.ifname, sizeof(priv->client.ifname), DEFAULT_IFNAME);
    }

    /* The socket is used for sending ioctl() and receiving wireless events. */
    sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) {
        WAC_LIB_DEBUG_PRINTF("socket(PF_NETLINK,SOCK_RAW,NETLINK_ROUTE) error\n");
        goto error;
    }

    memset(&local, 0, sizeof(local));
    local.nl_family = AF_NETLINK;
    local.nl_groups = RTMGRP_LINK;
    if (bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0) {
        WAC_LIB_DEBUG_PRINTF("bind() error\n");
        goto error;
    }

    priv->sock = sock;

    g_wac_lib_priv = priv;

    ret = pthread_create(&priv->event_thread_id, NULL, event_listener_thread, (void *)priv);
    if (ret) {
        WAC_LIB_DEBUG_PRINTF("pthread_create() error = %d\n", ret);
        goto error;
    }

    ret =  gettimeofday(&tp, NULL);

   /* Convert from timeval to timespec */
    ts.tv_sec  = tp.tv_sec + EVENT_THREAD_CREATE_WAIT_TIME;
    ts.tv_nsec = tp.tv_usec * 1000;

    ret = pthread_mutex_lock(&g_wac_lib_mutex);

    while(!priv->event_thread_started) {
        rc = pthread_cond_timedwait(&g_wac_lib_cond, &g_wac_lib_mutex, &ts);
        if (rc == ETIMEDOUT) {
            break;
        }
    }

    ret = pthread_mutex_unlock(&g_wac_lib_mutex);

    if (rc == ETIMEDOUT) {
        int status;
        WAC_LIB_DEBUG_PRINTF("pthread_cond_timedwait() error = ETIMEDOUT\n");
        pthread_cancel(priv->event_thread_id);
        pthread_join(priv->event_thread_id, (void **)&status);
        WAC_LIB_DEBUG_PRINTF("pthread_join status = %d\n", status);

        goto error;
    }

    return priv;

error:
    if (sock >= 0) {
        close(sock);
    }

    if (priv) {
        free(priv);
    }

    g_wac_lib_priv = NULL;

    ret = pthread_cond_destroy(&g_wac_lib_cond);
    if (ret) {
        WAC_LIB_DEBUG_PRINTF("pthread_cond_destroy() error\n");
    }

    ret = pthread_mutex_destroy(&g_wac_lib_mutex);
    if (ret) {
        WAC_LIB_DEBUG_PRINTF("pthread_mutex_destroy() error\n");
    }

    return NULL;
}

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
int ath_wac_lib_unregister(void *handle)
{
    int ret = -1;
    int status;
    wac_lib_private_t *priv = handle;
    pthread_t tid = pthread_self();

    if (g_wac_lib_priv == NULL || g_wac_lib_priv != priv) {
        return ret;
    }

    if (pthread_equal(tid, priv->event_thread_id)) {
        WAC_LIB_DEBUG_PRINTF("ath_wac_lib_unregister() must not be called in event_listener_thread context\n");
        return ret;
    }

#if 0
    if (priv->wac_enable) {
        ath_wac_disable(priv);
    }
#endif

    pthread_cancel(priv->event_thread_id);

    /* wait until event_listener_thread() is terminated. */
    pthread_join(priv->event_thread_id, (void **)&status);
    WAC_LIB_DEBUG_PRINTF("pthread_join status = %d\n", status);

    if (priv->sock >= 0) {
        close(priv->sock);
    }

    free(priv);
    g_wac_lib_priv = NULL;

    ret = pthread_cond_destroy(&g_wac_lib_cond);
    if (ret) {
        WAC_LIB_DEBUG_PRINTF("pthread_cond_destroy() error\n");
    }

    ret = pthread_mutex_destroy(&g_wac_lib_mutex);
    if (ret) {
        WAC_LIB_DEBUG_PRINTF("pthread_mutex_destroy() error\n");
    }

    return ret;
}

/* ------------------- WAC library APIs -- END -------------------------- */
#endif /* WAC_ENABLE */
