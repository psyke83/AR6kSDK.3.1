/*------------------------------------------------------------------------------ */
/* Copyright (c) 2010 Atheros Corporation.  All rights reserved. */
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

#ifndef __WAC_API_H__
#define __WAC_API_H__

typedef enum {
    WAC_SET,
    WAC_GET,
} WAC_REQUEST_TYPE;

typedef enum {
    WAC_ADD,
    WAC_DEL,
    WAC_GET_STATUS,
    WAC_GET_IE,
} WAC_COMMAND;

typedef enum {
    PRBREQ,
    PRBRSP,
    BEACON,
} WAC_FRAME_TYPE;

typedef enum {
    WAC_FAILED_NO_WAC_AP = -4,
    WAC_FAILED_LOW_RSSI = -3,
    WAC_FAILED_INVALID_PARAM = -2,
    WAC_FAILED_REJECTED = -1,
    WAC_SUCCESS = 0,
    WAC_DISABLED = 1,
    WAC_PROCEED_FIRST_PHASE,
    WAC_PROCEED_SECOND_PHASE,
} WAC_STATUS;


#endif
