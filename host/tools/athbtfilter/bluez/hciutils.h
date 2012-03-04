//------------------------------------------------------------------------------
// <copyright file="hciutils.h" company="Atheros">
//    Copyright (c) 2011 Atheros Corporation.  All rights reserved.
// 
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
//------------------------------------------------------------------------------
//==============================================================================

#ifndef __HCI_UTILS_H__
#define __HCI_UTILS_H__


//
// Return codes
//
typedef enum 
{
    HCIUTILS_SUCCESS,               // Success
    HCIUTILS_FAILURE,               // Generic Failure
    HCIUTILS_ALREADY_REGISTERED,    // Filter already registered
    HCIUTILS_OUT_OF_MEMORY          //out of memory
}tHCIUTILS_STATUS;

//
// Notification Type - Or to get notification for both types
//
typedef enum 
{
    HCIUTILS_COMMAND=0x1,
    HCIUTILS_EVENT =0x2
}tHCIUTILS_NOTIFICATION_TYPE;

//
// OpCode to get notifications for all hci commands and events
//
#define HCIUTILS_NOTIFICATION_OPCODE_ALL        0xffff

typedef struct 
{    
    tHCIUTILS_NOTIFICATION_TYPE     tType;
    unsigned short                  nOpCode;
    void *                          p_notification_data_buf;
    int                             n_data_length;
    void *                          p_appdata;
}tHCIUTILS_NOTIFICATION;

typedef void ( *tHCIUTILS_EVENT_CALLBACK) ( tHCIUTILS_NOTIFICATION * pEvent);

tHCIUTILS_STATUS HCIUTILS_RegisterHCINotification
    (
        tHCIUTILS_NOTIFICATION_TYPE t_type, 
        unsigned short              nOpCode,
        tHCIUTILS_EVENT_CALLBACK    eventCallback ,
        void *                      p_appdata
    );

void HCIUTILS_UnRegisterHCINotification
    (
        tHCIUTILS_NOTIFICATION_TYPE t_type, 
        unsigned short              nOpCode
    );

typedef enum
{
    HCIUTILS_SET_AFH_CHANNELS = 0x01,
    HCIUTILS_SET_AFH_CHANNEL_ASSESSMENT

}tHCIUTILS_HCI_CMD;

typedef struct 
{
    unsigned char       first;
    unsigned char       last;
}tHCIUTILS_HCICMD_SET_AFH_CHANNELS;

typedef struct
{
    int                 enable_or_disable;
}tHCIUTILS_HCICMD_SET_AFH_CHANNEL_ASSESSMENT;


void HCIUTILS_SendCmd
    (
        tHCIUTILS_HCI_CMD   tCmd,
        void *              p_hci_cmd_params
    );


#endif //__HCI_UTILS_H__

