/*------------------------------------------------------------------------------ */
/* Copyright (c) 2004-2010 Atheros Communications Inc. */
/* All rights reserved. */
/* */
/* This file implements the Atheros PS and patch downloaded for HCI UART  */
/* Transport driver. This file can be used for HCI SDIO transport  */
/* implementation for AR6002 with HCI_TRANSPORT_SDIO defined. */
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

#ifndef __AR3KPSCONFIG_H
#define __AR3KPSCONFIG_H

/* 
 * Define the flag HCI_TRANSPORT_SDIO and undefine HCI_TRANSPORT_UART if the transport being used is SDIO.
 */
#undef HCI_TRANSPORT_UART

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/signal.h>


#include <linux/ioctl.h>
#include <linux/firmware.h>


#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "ar3kpsparser.h"

#define FPGA_REGISTER	0x4FFC
#define BDADDR_TYPE_STRING	0
#define BDADDR_TYPE_HEX		1
#define CONFIG_PATH	  "ar3k"

#define PS_ASIC_FILE      "PS_ASIC.pst"
#define PS_FPGA_FILE      "PS_FPGA.pst"

#define PATCH_FILE      "RamPatch.txt"
#define BDADDR_FILE "ar3kbdaddr.pst"

#define ROM_VER_AR3001_3_1_0	30000
#define ROM_VER_AR3001_3_1_1	30101	


#ifndef HCI_TRANSPORT_SDIO
#define AR3K_CONFIG_INFO        struct hci_dev
extern wait_queue_head_t HciEvent;
extern wait_queue_t Eventwait;
extern A_UCHAR *HciEventpacket;
#endif /* #ifndef HCI_TRANSPORT_SDIO */

A_STATUS AthPSInitialize(AR3K_CONFIG_INFO *hdev);
A_STATUS ReadPSEvent(A_UCHAR* Data);
#endif /* __AR3KPSCONFIG_H */
