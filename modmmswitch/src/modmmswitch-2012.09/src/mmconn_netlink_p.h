/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

/** \file
 * Multimedia connection generic netlink private API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef __MMCONN_NETLINK_P
#define __MMCONN_NETLINK_P

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmconn_netlink.h"
#include "mmcommon.h"
#include "mmconn_p.h"
/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/


/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/
MmPbxError mmConnGeNlSetTraceLevel(MmPbxTraceLevel level);
MmPbxError mmConnGeNlInit(void);
MmPbxError mmConnGeNlDeinit(void);
MmPbxError mmConnGeNlPrepare(MmConnHndl mmConn);
MmPbxError mmConnGeNlDestroy(MmConnHndl mmConn);
MmPbxError mmConnSendEvent(MmConnHndl   mmConn,
                           MmConnEvent  *event);
#endif /* __MMCONN_NETLINK_P */
