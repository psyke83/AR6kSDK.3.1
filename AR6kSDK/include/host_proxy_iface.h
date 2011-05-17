/*------------------------------------------------------------------------------ */
/* Copyright (c) 2004-2011 Atheros Corporation.  All rights reserved. */
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

/*
 * This file contains the definitions of the host_proxy interface.  
 */

#ifndef _HOST_PROXY_IFACE_H_
#define _HOST_PROXY_IFACE_H_

/* Host proxy initializes shared memory with HOST_PROXY_INIT to 
 * indicate that it is ready to receive instruction */
#define HOST_PROXY_INIT         (1)
/* Host writes HOST_PROXY_NORMAL_BOOT to shared memory to 
 * indicate to host proxy that it should proceed to boot 
 * normally (bypassing BMI).
 */
#define HOST_PROXY_NORMAL_BOOT  (2)
/* Host writes HOST_PROXY_BMI_BOOT to shared memory to
 * indicate to host proxy that is should enable BMI and 
 * exit.  This allows a host to reprogram the on board
 * flash. 
 */
#define HOST_PROXY_BMI_BOOT     (3)

#endif /* _HOST_PROXY_IFACE_H_ */
