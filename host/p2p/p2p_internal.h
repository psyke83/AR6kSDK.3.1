//------------------------------------------------------------------------------
// <copyright file="p2p_internal.h" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
// 
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
//------------------------------------------------------------------------------
//==============================================================================
// This file contains internal definitions for the host P2P module.
//
// Author(s): ="Atheros"
//==============================================================================
#ifndef __P2P_INTERNAL_H__
#define __P2P_INTERNAL_H__

#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <p2p.h>

/* P2P peer device context */
struct host_p2p_dev {
    DL_LIST list;
    struct p2p_device dev;
    struct p2p_dev_ctx *p2p;
    A_UINT8 ref_cnt;
    A_UINT8 *p2p_buf;
    A_UINT32 p2p_buf_len;
    A_UINT8 persistent_grp;
/*  Persistent P2P Info*/
    A_UINT8 role_go;
    A_UINT8 ssid[WMI_MAX_SSID_LEN];
    A_UINT8 ssid_len;
};

/* Global P2P Context structure */
struct p2p_ctx {
    A_INT8 go_intent;
};

struct p2p_sd_query {
    struct p2p_sd_query *next;
    A_UINT8 peer[ETH_ALEN];
    A_UINT8 for_all_peers;
    A_UINT8 *tlvs;
    A_UINT16 tlv_len;
};

/* Device specific P2P context */
struct p2p_dev_ctx {
    struct p2p_ctx *p2p_ctx;
    DL_LIST devices;
    A_UINT8 peer_filter[ETH_ALEN];
    A_UINT8 p2p_auth_invite[ETH_ALEN];
    void *dev; /* AR6K priv context */
    struct p2p_sd_query *sd_queries;
    struct p2p_sd_query *curr_sd_query;
    A_UINT8 srv_update_indic;
};

#define P2P_MAX_GROUP_ENTRIES 10

struct p2p_client_info {
    const A_UINT8 *p2p_device_addr;
    const A_UINT8 *p2p_interface_addr;
    const A_UINT8 *pri_dev_type;
    const A_UINT8 *sec_dev_types;
    const A_CHAR *dev_name;
    A_UINT32 dev_name_len;
    A_UINT16 config_methods;
    A_UINT8 dev_capab;
    A_UINT8 num_sec_dev_types;
}; 

struct p2p_group_info {
    A_UINT32 num_clients;
    struct p2p_client_info client[P2P_MAX_GROUP_ENTRIES];
};

#define P2P_GROUP_CAPAB_PERSISTENT_GROUP (1<<1)
#define PARSE_WPS_IE 0x1
#define PARSE_P2P_IE 0x2
#endif /* __P2P_INTERNAL_H__ */
