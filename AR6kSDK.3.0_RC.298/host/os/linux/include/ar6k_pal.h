/*------------------------------------------------------------------------------ */
/* Copyright (c) 2004-2010 Atheros Communications Inc. */
/* All rights reserved. */
/* */
/* PAL driver implementation */
/* */
/*  */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License version 2 as */
/* published by the Free Software Foundation; */
/* */
/* Software distributed under the License is distributed on an "AS */
/* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or */
/* implied. See the License for the specific language governing */
/* rights and limitations under the License. */
/* */
/* */
/* */
/* Author(s): ="Atheros" */
/*------------------------------------------------------------------------------ */

#ifndef _AR6K_PAL_H_
#define _AR6K_PAL_H_
#define HCI_GET_OP_CODE(p)          (((A_UINT16)((p)[1])) << 8) | ((A_UINT16)((p)[0]))

/* transmit packet reserve offset */
#define TX_PACKET_RSV_OFFSET        32
/* pal specific config structure */
typedef A_BOOL (*ar6k_pal_recv_pkt_t)(void *pHciPalInfo, void *skb);
typedef struct ar6k_pal_config_s
{
	ar6k_pal_recv_pkt_t fpar6k_pal_recv_pkt;
}ar6k_pal_config_t;

void register_pal_cb(ar6k_pal_config_t *palConfig_p);
#endif /* _AR6K_PAL_H_ */
