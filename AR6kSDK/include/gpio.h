/*------------------------------------------------------------------------------ */
/* Copyright (c) 2005-2010 Atheros Corporation.  All rights reserved. */
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
/*------------------------------------------------------------------------------ */
/*============================================================================== */
/* Author(s): ="Atheros" */
/*============================================================================== */

#define AR6001_GPIO_PIN_COUNT 18
#define AR6002_GPIO_PIN_COUNT 18
#define AR6003_GPIO_PIN_COUNT 28
#define MCKINLEY_GPIO_PIN_COUNT 57

/*
 * Values of gpioreg_id in the WMIX_GPIO_REGISTER_SET_CMDID and WMIX_GPIO_REGISTER_GET_CMDID
 * commands come in two flavors.  If the upper bit of gpioreg_id is CLEAR, then the
 * remainder is interpreted as one of these values.  This provides platform-independent
 * access to GPIO registers.  If the upper bit (GPIO_ID_OFFSET_FLAG) of gpioreg_id is SET,
 * then the remainder is interpreted as a platform-specific GPIO register offset.
 */
#define GPIO_ID_OUT             0x00000000
#define GPIO_ID_OUT_W1TS        0x00000001
#define GPIO_ID_OUT_W1TC        0x00000002
#define GPIO_ID_ENABLE          0x00000003
#define GPIO_ID_ENABLE_W1TS     0x00000004
#define GPIO_ID_ENABLE_W1TC     0x00000005
#define GPIO_ID_IN              0x00000006
#define GPIO_ID_STATUS          0x00000007
#define GPIO_ID_STATUS_W1TS     0x00000008
#define GPIO_ID_STATUS_W1TC     0x00000009
#define GPIO_ID_PIN0            0x0000000a
#define GPIO_ID_PIN(n)          (GPIO_ID_PIN0+(n))
#define GPIO_ID_NONE            0xffffffff

#define GPIO_ID_OFFSET_FLAG     0x80000000
#define GPIO_ID_REG_MASK        0x7fffffff
#define GPIO_ID_IS_OFFSET(reg_id) (((reg_id) & GPIO_ID_OFFSET_FLAG) != 0)
