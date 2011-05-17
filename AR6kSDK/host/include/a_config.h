/*------------------------------------------------------------------------------ */
/* <copyright file="a_config.h" company="Atheros"> */
/*    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved. */
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
/* This file contains software configuration options that enables */
/* specific software "features" */
/* */
/* Author(s): ="Atheros" */
/*============================================================================== */
#ifndef _A_CONFIG_H_
#define _A_CONFIG_H_

#ifdef ATHR_WM_NWF
#include "../os/windows/include/config.h"
#endif

#ifdef ATHR_CE_LEGACY
#include "../os/windows/include/config.h"
#endif

#if defined(__linux__) && !defined(LINUX_EMULATION)
#include "../os/linux/include/config_linux.h"
#endif

#ifdef REXOS
#include "../os/rexos/include/common/config_rexos.h"
#endif

#ifdef ATHR_WIN_NWF
#include "../os/windows/include/config.h"
#pragma warning( disable:4242)
#pragma warning( disable:4100)
#pragma warning( disable:4189)
#pragma warning( disable:4244)
#pragma warning( disable:4701)
#pragma warning( disable:4389)
#pragma warning( disable:4057)
#pragma warning( disable:28193)
#endif

#endif
