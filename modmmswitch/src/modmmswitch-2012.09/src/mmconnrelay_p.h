/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

/** \file
 * Private multimedia switch relay connection API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNRELAY_P_INC
#define  MMCONNRELAY_P_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include <linux/socket.h>

#include "mmconnrtcpstats_p.h"
#include "mmswitch_p.h"
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

/**
 * Relay connection configuration parameters.
 */
typedef struct {
    /**< File Descriptor of local socket, created in user space */
    int                     localSockFd;
    struct sockaddr_storage remoteAddr;                   /**< Remote Address info, generated in user space */
    MmConnPacketHeader      header;                       /**<Header to add when we receive data from socket */
    int                     mediaTimeout;                 /**< Media timeout (ms, 0 == disable) */
    /**
     * Only applicable if the connection is a Control Connection, otherwise it has value NULL
     */
    MmConnHndl              mainConn;
    /**
     * Do we need to keep statistics ? (0==FALSE, 1==TRUE)
     * Only applicable if the connection is a Control Connection
     */
    unsigned int            keepStats;
} MmConnRelayConfig;

/*
 * MmConnRelay structure definition
 */
struct MmConnRelay {
    struct MmConn               mmConn;                 /**< Parent class */
    spinlock_t                  lock;                   /**< Object lock */
    MmConnRelayConfig           config;                 /**< Configuration */
    MmSwitchSock                localSock;              /**< local Socket */
    MmSwitchTimer               mmSwitchTimer;          /**< Timer info */
    int                         packetsSent;            /**< Counter of sent packets for statistics purposes */
    int                         packetsReceived;        /**< Counter of received packets for tracking media time-out */
    MmConnRtcpStatsSessionHndl  statsSession;           /**< Statistics session (only applicable in case of a control connection and keepStats == TRUE) */
};

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/
/**
 * Set trace level of all multimedia switch relay connections.
 *
 * This function makes it possible to modify the trace level of all relay connections. This trace level is also dependant on the trace level which was used to compile the code.
 *
 * \since v1.0
 *
 * \pre none.
 *
 * \post The trace level will be the requested tracelevel if it not violates with the compile time trace level.
 *
 * \param [in] level Trace level.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The tracelevel has been  successfully set.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRelaySetTraceLevel(MmPbxTraceLevel level);

/**
 * Initialise mmConnRelay component of mmswitch kernel module.
 *
 * This function initialises the mmConnRelay component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnRelay component of mmswitch is initialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnRelay component has been initialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRelayInit(void);

/**
 * Deinitialise mmConnRelay component of mmswitch kernel module.
 *
 * This function deinitialises the mmConnRelay component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnRelay component of mmswitch is deinitialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnRelay component has been deinitialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRelayDeinit(void);

/**
 * Constructor of a multimedia switch relay connection instance.
 *
 * This function is the constructor of a multimedia switch relay connection instance.
 *
 * \since v1.0
 *
 * \pre \c connGr must be a valid handle.
 *
 * \post \c relay contains the handle of a valid multimedia switch relay connection instance.
 *
 * \param [in] config Configuration of relay connection instance of multimedia switch.
 * \param [out] relay Handle of relay connection instance of multimedia switch.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR A relay connection handle has been successfully retrieved and is not NULL.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRelayConstruct(MmConnRelayConfig *config,
                                MmConnRelayHndl   *relay);

/**
 * Modify remote socket address.
 *
 * This function allows the user to modify remote socket address info.
 *
 * \since v1.0
 *
 * \pre \c relay must be a valid handle.
 *
 * \post The remote socket address info will be modfied.
 *
 * \param [in] relay Handle of connection instance.
 * \param [in] remoteAddr Remote socket address info.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 */
MmPbxError mmConnRelayModRemoteAddr(MmConnRelayHndl         relay,
                                    struct sockaddr_storage *remoteAddr);

/**
 * Reset statistics.
 *
 * This function allows the user to reset the statistics.
 *
 * \since v1.0
 *
 * \pre \c relay must be a valid handle.
 *
 * \post The statistics are reset.
 *
 * \param [in] relay Handle of connection instance.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Statistics successfully reset.
 */
MmPbxError mmConnRelayResetStats(MmConnRelayHndl relay);

/**
 * Get statistics.
 *
 * This function allows the user to get the statistics.
 *
 * \since v1.0
 *
 * \pre \c relay must be a valid handle.
 *
 * \param [in] relay Handle of connection instance.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Statistics successfully retrieved.
 */
MmPbxError mmConnRelayGetStats(MmConnRelayHndl  relay,
                               MmConnRtcpStats  *stats);
#endif   /* ----- #ifndef MMCONNRELAY_P_INC  ----- */
