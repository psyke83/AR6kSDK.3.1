//------------------------------------------------------------------------------
// <copyright file="os_wince.h" company="Atheros">
//    Copyright (c) 2007 Atheros Corporation.  All rights reserved.
// 
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================
#ifndef __OS_WINCE_H__
#define __OS_WINCE_H__

#include <stdio.h>
//#include <windows.h>
#include <ndis.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <strsafe.h>
#include <MMSystem.h>

#define SAFE_STR_CPY(d,s)  StringCbCopyA((d),STRSAFE_MAX_CCH,(s))
#define SAFE_STRN_CPY(d,s,c)  StringCbCopyA((d),(c),(s))

#define MSG_DONTWAIT        0
#define ITIMER_REAL         0

typedef unsigned long ThreadCallback(void *);
typedef ThreadCallback *thread_callback;

#define SIGALRM             0
typedef void sigfn();
typedef sigfn *sighandler;
typedef HANDLE pthread_cond_t;
typedef HANDLE pthread_mutex_t;
typedef int    pthread_attr_t;
typedef PBYTE  pthread_t;

#define SIG_IGN     ((sighandler) 1)
#define SOL_IP      IPPROTO_IP

typedef char *caddr_t;

struct itimerval
{
    struct timeval it_interval;
    struct timeval it_value;
};

struct timerhandle
{
    int enable;
    int id;
    sighandler fn;
};

void gettimeofday (struct timeval *t, int dummmy);
void usleep (int usec);
void sleep (int sec);
void bzero (char *ptr, int size);
int  setitimer (int dummy, struct itimerval *t, struct itimerval *t1);
void perror (char *msg);
int  signal (int sig, sighandler);
int  system (TCHAR *cmd);
HANDLE pthread_create (HANDLE *pThread, int *pAttr, thread_callback, void *context);
void pthread_attr_init (int *pHandle);
void pthread_cond_init (HANDLE *pHandle, TCHAR *param);
void pthread_mutex_init (HANDLE *pHandle, TCHAR *param);
void alarm (int timer_id);
void pthread_mutex_lock (HANDLE *pHandle);
void pthread_mutex_unlock (HANDLE *pHandle);
void pthread_cond_wait (HANDLE *pHandle, HANDLE *pMutex);
void pthread_cond_signal (HANDLE *pHandle);
void destroyHandle (HANDLE *pHandle);

#endif
