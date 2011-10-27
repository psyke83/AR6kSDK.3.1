//------------------------------------------------------------------------------
// <copyright file="os_wince.c" company="Atheros">
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
#include "os_wince.h"

struct timerhandle g_timer;

void usleep (int usec)
{
	Sleep (usec/1000);
    // NdisMSleep (usec); // Sleep (usec/1000);
}

void sleep (int sec)
{
    Sleep (sec*1000);
}

void bzero (char *ptr, int size)
{
    memset (ptr, 0, size);
}

void perror (char *msg)
{
    printf ("ERROR: %s : %d\n", msg, GetLastError());
}

void gettimeofday (struct timeval *t, int dummmy)
{
    DWORD time = GetTickCount();
    t->tv_sec  = time/1000;
    t->tv_usec = (time%1000)*1000;
}

void CALLBACK TimerProc (UINT uTimerID,
                         UINT uMsg,
                         DWORD_PTR dwUser,
                         DWORD_PTR dw1,
                         DWORD_PTR dw2)
{
    g_timer.id = 0;

    if (g_timer.enable)
    {
        g_timer.fn ();
    }
}

int setitimer (int dummy,
               struct itimerval *t,
               struct itimerval *t1)
{
    DWORD   timeMilliSec;

    if (g_timer.id)
    {
        UINT ret = timeKillEvent (g_timer.id);
    }

    if ((t->it_value.tv_sec == 0) && (t->it_value.tv_usec == 0) )
        return 0;

    timeMilliSec = (t->it_value.tv_sec * 1000) + (t->it_value.tv_usec / 1000);

    g_timer.id = timeSetEvent (timeMilliSec, 1, TimerProc, 0, TIME_ONESHOT|TIME_CALLBACK_FUNCTION);

    if (g_timer.id == 0)
    {
        perror("setitimer failed 2");
    }

    return 0;
}

void alarm (int timer_id)
{
    if (g_timer.id)
    {
        timeKillEvent (g_timer.id);
    }
}


int signal (int dummy, sighandler fn)
{
    if (fn == SIG_IGN)
    {
        g_timer.enable = 0;
    }
    else
    {
        g_timer.fn = fn;
        g_timer.enable = 1;
    }
    return 0;
}

int system (TCHAR *arg)
{
    if(!CreateProcess(L"wmiconfig.exe", arg, NULL, NULL,
                FALSE, 0, NULL, NULL, NULL, NULL ))
    {
        perror("Failed to run wmiconfig");
        return -1;
    }

    return 0;
}

void pthread_mutex_init (HANDLE *pHandle, TCHAR *param)
{
    *pHandle = CreateMutex (NULL,                // No security descriptor
                            FALSE,               // Mutex object not owned
                            param);              // Object name

    if (NULL == *pHandle)
    {
        printf ("Error in Mutex Creation\n");
    }
}

void pthread_cond_init (HANDLE *pHandle, TCHAR *param)
{
    *pHandle = CreateEvent (NULL,                // No security descriptor
                            FALSE,               // Mutex object not owned
                            FALSE,
                            param);             // Object name

    if (NULL == *pHandle)
    {
        printf ("Error in Event Creation\n");
    }
}

void pthread_attr_init (int *pHandle)
{
    *pHandle = 0;
}

HANDLE pthread_create (HANDLE *pThread, int *pAttr, thread_callback  TCB, void *context)
{
    HANDLE hThread = NULL;
    hThread = CreateThread (*pThread, *pAttr, TCB, context, 0, NULL);
    return hThread;
}

void pthread_mutex_lock (HANDLE *pHandle)
{
    WaitForSingleObject (*pHandle, 5000L);
}

void pthread_mutex_unlock (HANDLE *pHandle)
{
    ReleaseMutex (*pHandle);
}

void pthread_cond_wait (HANDLE *pHandle, HANDLE *pMutex)
{
    WaitForSingleObject (*pHandle, 5000L);
}

void pthread_cond_signal (HANDLE *pHandle)
{
    SetEvent (*pHandle);
}

void destroyHandle (HANDLE *pHandle)
{
    CloseHandle (*pHandle);
    *pHandle = NULL;
