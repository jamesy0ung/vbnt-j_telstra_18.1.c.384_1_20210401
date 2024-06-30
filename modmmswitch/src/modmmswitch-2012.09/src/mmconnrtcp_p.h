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
 * Private multimedia switch rtcp connection API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNRTCP_P_INC
#define  MMCONNRTCP_P_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include <linux/socket.h>
#include <linux/types.h>

#include "mmconnrtcpstats_p.h"
#include "mmconnrtcp.h"
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

/*
 * MmConnRtcp structure definition
 */
/**
 * Rtcp connection configuration parameters.
 */
typedef struct {
    int                     localMediaSockFd; /**< File Descriptor of local media socket, created in user space */
    int                     localRtcpSockFd;  /**< File Descriptor of local RTCP socket, created in user space */
    struct sockaddr_storage remoteMediaAddr;  /**< Remote Media Address info, generated in user space */
    struct sockaddr_storage remoteRtcpAddr;   /**< Remote RTCP Address info, generated in user space */
    MmConnPacketHeader      header;           /**<Header to add when we receive data from socket */
    unsigned int            mediaTimeout;     /**< Media timeout (ms, 0 == disable) */
    unsigned int            mediaMuteTime;    /**< Mute incoming media for x ms ( 0 == disable) */
    unsigned int            rtcpBandwidth;    /**< RTCP bandwidth */
    unsigned int            generateRtcp;     /**< Do we need to generate RTCP ? (0==FALSE, 1==TRUE) */
    unsigned int            keepStats;        /**< Do we need to keep statistics ? (0==FALSE, 1==TRUE) */
} MmConnRtcpConfig;

/**
 * Rtcp connection configuration parameters.
 */
struct MmConnRtcp {
    struct MmConn     mmConn;                   /**< Parent class */
    MmConnRtcpConfig  config;                   /**< Configuration */
    MmConnRtcpStats   stats;                    /**< Statistics */
    spinlock_t        lock;                     /**< Lock */
    MmSwitchSock      localMediaSock;           /**< local Media Socket */
    MmSwitchSock      localRtcpSock;            /**< local Rtcp Socket */
    MmSwitchTimer     mediaTimeoutTimer;        /**< Media timeout timer */
    MmSwitchTimer     rtcpTimer;                /**< RTCP transmit timer */
    int               packetCounter;            /**< Packet counter, used to detect media timeout */
    unsigned long     mediaMuteTimeout;         /**< Timeout value, used to mute first x ms of media */
    unsigned int      rtcpTimerTimeout;         /**<  RTCP transmit timer timeout value */
    void              *stackCookie;             /**< Cookie to be used by RTP/RTCP stack */
    uint64_t          sessionStart;             /**< Start of the session (jiffies), i.e. when first RTP packet is sent to the network */
};



/*#######################################################################
 #                                                                       #
 #  FUNCTION PROTOTYPES                                                  #
 #                                                                       #
 ########################################################################*/
/**
 * Set trace level of all multimedia switch rtcp connections.
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
MmPbxError mmConnRtcpSetTraceLevel(MmPbxTraceLevel level);

/**
 * Initialise mmConnRtcp component of mmswitch kernel module.
 *
 * This function initialises the mmConnRtcp component of the mmswitch kernel module.
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
MmPbxError mmConnRtcpInit(void);

/**
 * Deinitialise mmConnRtcp component of mmswitch kernel module.
 *
 * This function deinitialises the mmConnRtcp component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnRtcp component of mmswitch is deinitialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnRtcp component has been deinitialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpDeinit(void);

/**
 * Constructor of a multimedia switch rtcp connection instance.
 *
 * This function is the constructor of a multimedia switch rtcp connection instance.
 *
 * \since v1.0
 *
 * \pre \c connGr must be a valid handle.
 *
 * \post \c rtcp contains the handle of a valid multimedia switch rtcp connection instance.
 *
 * \param [in] config Configuration of rtcp connection instance of multimedia switch.
 * \param [out] rtcp Handle of relay connection instance of multimedia switch.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR A rtcp connection handle has been successfully retrieved and is not NULL.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpConstruct(MmConnRtcpConfig *config,
                               MmConnRtcpHndl   *rtcp);

/**
 * Modify Media Packet type of RTCP connection instance.
 *
 * This function allows the user to modify the media packet type of an RTCP connection instance.
 * This packet type is used as a label for all incoming traffic.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The packet type will be modified.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] type Packet type.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModMediaPacketType(MmConnRtcpHndl    rtcp,
                                        MmConnPacketType  type);

/**
 * Modify RTCP generator state of RTCP connection instance.
 *
 * This function allows the user enable/disable RTCP generation.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The state will be modified.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] generateRtcp RTCP generator state (0 == disabled, 1 == enabled)
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModGenRtcp(MmConnRtcpHndl  rtcp,
                                unsigned int    generateRtcp);

/**
 * Modify remote media socket address .
 *
 * This function allows the user to modify remote media socket address .
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The remote media socket address will be modified.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] remoteMediaAddr Remote Media socket address .
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModRemoteMediaAddr(MmConnRtcpHndl          rtcp,
                                        struct sockaddr_storage *remoteMediaAddr);

/**
 * Modify remote RTCP socket address.
 *
 * This function allows the user to modify remote RTCP socket address info.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The remote RTCP socket address info will be modified.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] remoteRtcpAddr Remote RTCP socket address info.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModRemoteRtcpAddr(MmConnRtcpHndl           rtcp,
                                       struct sockaddr_storage  *remoteRtcpAddr);

/**
 * Modify RTCP generator RTCP bandwidth.
 *
 * This function allows the user to modify the bandwidth used for RTCP.
 * RTCP bandwidth should be 5% of RTP bandwidth.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The bandwidth will be modified.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] rtcpBandwidth RTCP bandwitdh (bytes/second).
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModRtcpBandwidth(MmConnRtcpHndl  rtcp,
                                      unsigned int    rtcpBandwidth);

/**
 * Get statistics.
 *
 * This function allows the user to get the statistics.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Statistics have been successfully retrieved.
 */
MmPbxError mmConnRtcpGetStats(MmConnRtcpHndl  rtcp,
                              MmConnRtcpStats *pStats);

/**
 * Get statistics.
 *
 * This function allows the user to reset the statistics.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Statistics are successfully reset.
 */
MmPbxError mmConnRtcpResetStats(MmConnRtcpHndl rtcp);
#endif   /* ----- #ifndef MMCONNRTCP_P_INC  ----- */
