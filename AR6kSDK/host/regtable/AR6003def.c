/*
 * Copyright (c) 2008 Atheros Communications, Inc.
 * All rights reserved.
 *
 *
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
 */

#if defined(AR6003_HEADERS_DEF) || defined(ATHR_WIN_DEF)
#define AR6002 1
#define AR6002_REV 4

#define WLAN_HEADERS 1
#include "common_drv.h"
#include "AR6002/hw4.0/hw/apb_map.h"
#include "AR6002/hw4.0/hw/gpio_reg.h"
#include "AR6002/hw4.0/hw/rtc_reg.h"
#include "AR6002/hw4.0/hw/si_reg.h"
#include "AR6002/hw4.0/hw/mbox_reg.h"
#include "AR6002/hw4.0/hw/mbox_wlan_host_reg.h"

#define MY_TARGET_DEF AR6003_TARGETdef
#define MY_HOST_DEF AR6003_HOSTdef
#define MY_TARGET_BOARD_DATA_SZ AR6003_BOARD_DATA_SZ
#define MY_TARGET_BOARD_EXT_DATA_SZ AR6003_BOARD_EXT_DATA_SZ
#define RTC_WMAC_BASE_ADDRESS              RTC_BASE_ADDRESS
#define RTC_SOC_BASE_ADDRESS               RTC_BASE_ADDRESS

#include "targetdef.h"
#include "hostdef.h"
#else
#include "common_drv.h"
#include "targetdef.h"
#include "hostdef.h"
struct targetdef_s *AR6003_TARGETdef=NULL;
struct hostdef_s *AR6003_HOSTdef=NULL;
#endif /*AR6003_HEADERS_DEF */
