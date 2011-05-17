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


#include "common_drv.h"
#include "bmi_msg.h"

#include "targetdef.h"
#include "hostdef.h"
#include "hif.h"


/* Target-dependent addresses and definitions */
struct targetdef_s *targetdef;
/* HIF-dependent addresses and definitions */
struct hostdef_s *hostdef;

void target_register_tbl_attach(A_UINT32 target_type)
{
    switch (target_type) {
    case TARGET_TYPE_AR6002:
        targetdef = AR6002_TARGETdef;
        break;
    case TARGET_TYPE_AR6003:
        targetdef = AR6003_TARGETdef;
        break;
    case TARGET_TYPE_MCKINLEY:
        targetdef = MCKINLEY_TARGETdef;
        break;
    default:
        break;
    }
}

void hif_register_tbl_attach(A_UINT32 hif_type)
{
    switch (hif_type) {
    case HIF_TYPE_AR6002:
        hostdef = AR6002_HOSTdef;
        break;
    case HIF_TYPE_AR6003:
        hostdef = AR6003_HOSTdef;
        break;
    case HIF_TYPE_MCKINLEY:
        hostdef = MCKINLEY_HOSTdef;
        break;
    default:
        break;
    }
}

EXPORT_SYMBOL(targetdef);
EXPORT_SYMBOL(hostdef);
