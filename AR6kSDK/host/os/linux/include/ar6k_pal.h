/*
 * Copyright (c) 2002-2006, Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
 
#ifndef _HCI_PAL_H_
#define _HCI_PAL_H_
#define HCI_GET_OP_CODE(p)          (((A_UINT16)((p)[1])) << 8) | ((A_UINT16)((p)[0]))
#define TX_PACKET_RSV_OFFSET        32
/* pal specific config structure */
typedef A_BOOL (*ar6k_pal_recv_pkt_t)(void *pHciPalInfo, void *skb);
typedef struct ar6k_pal_config_s
{
	ar6k_pal_recv_pkt_t fpar6k_pal_recv_pkt;
}ar6k_pal_config_t;

/**********************************
 * HCI PAL private info structure
 *********************************/
typedef struct ar6k_hci_pal_info_s{

        unsigned long ulFlags;
#define HCI_NORMAL_MODE (1)
#define HCI_REGISTERED (1<<1)
        struct hci_dev *hdev;            /* BT Stack HCI dev */
        AR_SOFTC_T *ar;

}ar6k_hci_pal_info_t;

void register_pal_cb(ar6k_pal_config_t *palConfig_p);
#endif /* _HCI_PAL_H_ */
