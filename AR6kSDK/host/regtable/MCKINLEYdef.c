/*
 * Copyright (c) 2010 Atheros Communications, Inc.
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
#if defined(MCKINLEY_HEADERS_DEF) || defined(ATHR_WIN_DEF)
#define AR6002 1
#define AR6002_REV 6

#define WLAN_HEADERS 1
#include "common_drv.h"
#include "AR6002/hw6.0/hw/apb_map.h"
#include "AR6002/hw6.0/hw/gpio_reg.h"
#include "AR6002/hw6.0/hw/rtc_reg.h"
#include "AR6002/hw6.0/hw/si_reg.h"
#include "AR6002/hw6.0/hw/mbox_reg.h"
#include "AR6002/hw6.0/hw/mbox_wlan_host_reg.h"

#define SYSTEM_SLEEP_OFFSET     SOC_SYSTEM_SLEEP_OFFSET

#define MY_TARGET_DEF MCKINLEY_TARGETdef
#define MY_HOST_DEF MCKINLEY_HOSTdef
#define MY_TARGET_BOARD_DATA_SZ MCKINLEY_BOARD_DATA_SZ
#define MY_TARGET_BOARD_EXT_DATA_SZ MCKINLEY_BOARD_EXT_DATA_SZ
#include "targetdef.h"
#include "hostdef.h"
#else
#include "common_drv.h"
#include "targetdef.h"
#include "hostdef.h"
struct targetdef_s *MCKINLEY_TARGETdef=NULL;
struct hostdef_s *MCKINLEY_HOSTdef=NULL;
#endif /*MCKINLEY_HEADERS_DEF */
