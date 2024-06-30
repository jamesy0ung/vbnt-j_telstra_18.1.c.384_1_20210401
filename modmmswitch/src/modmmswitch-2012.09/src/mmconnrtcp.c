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
 * Multimedia switch RTCP connection API.
 *
 * A RTCP connectione creates/terminates an rtcp stream and relays RTP/UDPTL.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONNRTCP"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/net.h>
#include <linux/file.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include <linux/module.h>

#include "mmconnrtcp_p.h"
#include "mmswitch_p.h"
#include "mmconn_p.h"
#include "mmconn_netlink_p.h"

/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/
#define PACKET_TMR_RESTART_COUNT    24

/*########################################################################
#                                                                        #
#  TYPES                                                                 #
#                                                                        #
########################################################################*/

/*########################################################################
#                                                                        #
#  PRIVATE DATA MEMBERS                                                  #
#                                                                        #
########################################################################*/

static int  _traceLevel   = MMPBX_TRACELEVEL_NONE;
static int  _initialised  = FALSE;

static MmConnRtcpStackCreateSessionsCb    _mmConnRtcpStackCreateSessionsCb    = NULL; /**< RTP/RTCP stack interface */
static MmConnRtcpStackDestroySessionsCb   _mmConnRtcpStackDestroySessionsCb   = NULL; /**< RTP/RTCP stack interface */
static MmConnRtcpStackModRtcpBandwidthCb  _mmConnRtcpStackModRtcpBandwidthCb  = NULL; /**< RTP/RTCP stack interface */
static MmConnRtcpStackRtpSendCb           _mmConnRtcpStackRtpSendCb           = NULL; /**< RTP/RTCP stack interface */
static MmConnRtcpStackRtpReceiveCb        _mmConnRtcpStackRtpReceiveCb        = NULL; /**< RTP/RTCP stack interface */
static MmConnRtcpStackRtcpReceiveCb       _mmConnRtcpStackRtcpReceiveCb       = NULL; /**< RTP/RTCP stack interface */
static MmConnRtcpStackRtcpTimerCb         _mmConnRtcpStackRtcpTimerCb         = NULL; /**< RTP/RTCP stack interface */

/*########################################################################
#                                                                       #
#  PRIVATE FUNCTION PROTOTYPES                                          #
#                                                                       #
########################################################################*/

static void mediaTimeoutTimerCb(MmConnHndl mmConn);

static void rtcpTimerCb(MmConnHndl mmConn);

static int isInitialised(void);
static void objLock(MmConnRtcpHndl mmConnRtcp);
static void objRelease(MmConnRtcpHndl mmConnRtcp);
static MmPbxError mmConnChildWriteCb(MmConnHndl         conn,
                                     MmConnPacketHeader *header,
                                     uint8_t            *buff,
                                     unsigned int       bytes);

static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn);
static MmPbxError mmConnChildXConnCb(MmConnHndl mmConn);
static MmPbxError mmConnChildDiscCb(MmConnHndl mmConn);
static MmPbxError mmSwitchSocketCb(MmConnHndl         mmConn,
                                   void               *data,
                                   MmConnPacketHeader *header,
                                   unsigned int       bytes);

static void mmConnRtcpGetRemoteMediaAddr(MmConnRtcpHndl           mmConnRtcp,
                                         struct sockaddr_storage  *remoteMediaAddr);

static void mmConnRtcpGetRemoteRtcpAddr(MmConnRtcpHndl          mmConnRtcp,
                                        struct sockaddr_storage *remoteRtcpAddr);

/*########################################################################
#                                                                        #
#  FUNCTION DEFINITIONS                                                  #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmConnRtcpInit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    do {
        ret = mmConnRtcpStatsInit();
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRtcpStatsInit failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        _initialised = TRUE;
    } while (0);

    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpDeinit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    _initialised = FALSE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));

    return MMPBX_ERROR_NOERROR;
}

MmPbxError mmConnRtcpConstruct(MmConnRtcpConfig *config,
                               MmConnRtcpHndl   *rtcp)
{
    MmPbxError          ret               = MMPBX_ERROR_NOERROR;
    MmConnRtcpHndl      mmConnRtcpTemp    = NULL;
    MmConnPacketHeader  mmConnRtcpHeader  = { .type = MMCONN_PACKET_TYPE_RTCP };

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (config == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (rtcp == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    *rtcp = NULL;

    do {
        /* Try to allocate another mmConnRtcp object instance */
        mmConnRtcpTemp = kmalloc(sizeof(struct MmConnRtcp), GFP_KERNEL);
        if (mmConnRtcpTemp == NULL) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
            break;
        }

        memset(mmConnRtcpTemp, 0, sizeof(struct MmConnRtcp));

        /* Prepare connection for usage */
        ret = mmConnPrepare((MmConnHndl)mmConnRtcpTemp, MMCONN_TYPE_RTCP);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnPrepare failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Register Child Destruct callback, will be called before destruct of object */
        ret = mmConnRegisterChildDestructCb((MmConnHndl)mmConnRtcpTemp, mmConnChildDestructCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDestructCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child cross-connect callback, will be called after cross-connect */
        ret = mmConnRegisterChildXConnCb((MmConnHndl)mmConnRtcpTemp, mmConnChildXConnCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildXConnCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child disconnect callback, will be called after disconnect */
        ret = mmConnRegisterChildDiscCb((MmConnHndl)mmConnRtcpTemp, mmConnChildDiscCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDiscCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child write callback, will be called to push data into mmConn */
        ret = mmConnRegisterChildWriteCb((MmConnHndl)mmConnRtcpTemp, mmConnChildWriteCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildWriteCb failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }
        mmConnRtcpTemp->config.localMediaSockFd = config->localMediaSockFd;
        memcpy(&mmConnRtcpTemp->config.remoteMediaAddr, (void *)&config->remoteMediaAddr, sizeof(struct sockaddr_storage));
        mmConnRtcpTemp->config.localRtcpSockFd = config->localRtcpSockFd;
        memcpy(&mmConnRtcpTemp->config.remoteRtcpAddr, (void *)&config->remoteRtcpAddr, sizeof(struct sockaddr_storage));

        MMPBX_TRC_INFO("mediaMuteTime: %d\n", config->mediaMuteTime);
        mmConnRtcpTemp->config.mediaMuteTime = config->mediaMuteTime;

        mmConnRtcpTemp->config.rtcpBandwidth  = config->rtcpBandwidth;
        mmConnRtcpTemp->config.generateRtcp   = config->generateRtcp;
        mmConnRtcpTemp->config.keepStats      = config->keepStats;

        /*Prepare sockets */
        ret = mmSwitchPrepareSocket(&mmConnRtcpTemp->localMediaSock, config->localMediaSockFd);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchPrepareSockets (media) failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        ret = mmSwitchPrepareSocket(&mmConnRtcpTemp->localRtcpSock, config->localRtcpSockFd);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchPrepareSockets (media) failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        memcpy(&mmConnRtcpTemp->config.header, (void *)&config->header, sizeof(MmConnPacketHeader));
        ret = mmSwitchRegisterSocketCb(&mmConnRtcpTemp->localMediaSock, mmSwitchSocketCb, (MmConnHndl) mmConnRtcpTemp, &config->header);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchRegisterSocketCb (media) failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        ret = mmSwitchRegisterSocketCb(&mmConnRtcpTemp->localRtcpSock, mmSwitchSocketCb, (MmConnHndl) mmConnRtcpTemp, &mmConnRtcpHeader);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchRegisterSocketCb (media) failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        MMPBX_TRC_INFO("mediaTimeout: %d\n", config->mediaTimeout);
        mmConnRtcpTemp->config.mediaTimeout = config->mediaTimeout;

        /* Prepare timer for usage (if needed) */
        if ((config->mediaTimeout) > 0) {
            ret = mmSwitchPrepareTimer(&mmConnRtcpTemp->mediaTimeoutTimer, mediaTimeoutTimerCb, (MmConnHndl)mmConnRtcpTemp, FALSE);
            if (ret != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmSwitchPrepareTimer failed with: %s\n", mmPbxGetErrorString(ret));
                break;
            }
        }

        /*Create RTCP transmit timer, not periodic, because period can change */
        ret = mmSwitchPrepareTimer(&mmConnRtcpTemp->rtcpTimer, rtcpTimerCb, (MmConnHndl)mmConnRtcpTemp, FALSE);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchPrepareTimer failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Create RTP & RTCP session, this will return RTCP transmit timeout value */
        if (_mmConnRtcpStackCreateSessionsCb != NULL) {
            ret = _mmConnRtcpStackCreateSessionsCb(mmConnRtcpTemp, mmConnRtcpTemp->config.rtcpBandwidth, &mmConnRtcpTemp->stackCookie, &mmConnRtcpTemp->rtcpTimerTimeout);
            if (ret != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("_mmConnRtcpStackCreateSessionsCb failed with: %s\n", mmPbxGetErrorString(ret));
                break;
            }
        }

        /* Initialise mmConnRtcp specific lock */
        spin_lock_init(&mmConnRtcpTemp->lock);

        *rtcp = mmConnRtcpTemp;
        MMPBX_TRC_INFO("mmConnRtcp = %p\n", *rtcp);
    } while (0);

    if (ret != MMPBX_ERROR_NOERROR) {
        if (mmConnRtcpTemp != NULL) {
            mmConnDestruct((MmConnHndl *)&mmConnRtcpTemp);
        }
    }

    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpModMediaPacketType(MmConnRtcpHndl    rtcp,
                                        MmConnPacketType  type)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (rtcp == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }


    /* Modify packet type in kernel space */
    rtcp->config.header.type = type;

    /*Update SocketCb to apply new header */
    ret = mmSwitchRegisterSocketCb(&rtcp->localMediaSock, mmSwitchSocketCb, (MmConnHndl) rtcp, &rtcp->config.header);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmSwitchRegisterSocketCb (media) failed with: %s\n", mmPbxGetErrorString(ret));
    }


    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpModGenRtcp(MmConnRtcpHndl  rtcp,
                                unsigned int    generateRtcp)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (rtcp == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    MMPBX_TRC_INFO("generateRtcp: %d\n", generateRtcp);

    rtcp->config.generateRtcp = generateRtcp;

    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpModRemoteMediaAddr(MmConnRtcpHndl          rtcp,
                                        struct sockaddr_storage *remoteMediaAddr)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (rtcp == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (remoteMediaAddr == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }


    MMPBX_TRC_INFO("called\n");

    objLock(rtcp);
    memcpy(&rtcp->config.remoteMediaAddr, (void *)remoteMediaAddr, sizeof(struct sockaddr_storage));
    objRelease(rtcp);

    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpModRemoteRtcpAddr(MmConnRtcpHndl           rtcp,
                                       struct sockaddr_storage  *remoteRtcpAddr)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (rtcp == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (remoteRtcpAddr == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    MMPBX_TRC_INFO("called\n");

    objLock(rtcp);
    memcpy(&rtcp->config.remoteRtcpAddr, (void *)remoteRtcpAddr, sizeof(struct sockaddr_storage));
    objRelease(rtcp);

    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpModRtcpBandwidth(MmConnRtcpHndl  rtcp,
                                      unsigned int    rtcpBandwidth)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (rtcp == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    MMPBX_TRC_INFO("rtcpBandwidth: %d\n", rtcpBandwidth);

    objLock(rtcp);

    rtcp->config.rtcpBandwidth = rtcpBandwidth;

    objRelease(rtcp);

    /* Notify RTP/RTCP stack (which will send RTCP) */
    if (_mmConnRtcpStackModRtcpBandwidthCb != NULL) {
        ret = _mmConnRtcpStackModRtcpBandwidthCb(rtcp, rtcp->stackCookie, rtcpBandwidth);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("_mmConnRtcpStackModRtcpBandwidthCb failed with error: %s\n", mmPbxGetErrorString(ret));
        }
    }

    return ret;
}

MmPbxError mmConnRtcpGetStats(MmConnRtcpHndl  rtcp,
                              MmConnRtcpStats *pStats)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (rtcp == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    rtcp->stats.sessionDuration = jiffies_to_msecs(get_jiffies_64() - rtcp->sessionStart);
    memcpy(pStats, &rtcp->stats, sizeof(MmConnRtcpStats));

    return err;
}

MmPbxError mmConnRtcpResetStats(MmConnRtcpHndl rtcp)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (rtcp == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    rtcp->sessionStart = 0;
    memset(&rtcp->stats, 0, sizeof(MmConnRtcpStats));

    return err;
}

/**
 *
 */

MmPbxError mmConnRtcpRegisterStackCreateSessionsCb(MmConnRtcpStackCreateSessionsCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackCreateSessionsCb = cb;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpUnregisterStackCreateSessionsCb(MmConnRtcpStackCreateSessionsCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackCreateSessionsCb = NULL;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpRegisterStackDestroySessionsCb(MmConnRtcpStackDestroySessionsCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackDestroySessionsCb = cb;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpUnregisterStackDestroySessionsCb(MmConnRtcpStackDestroySessionsCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackDestroySessionsCb = NULL;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpRegisterStackModRtcpBandwidthCb(MmConnRtcpStackModRtcpBandwidthCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackModRtcpBandwidthCb = cb;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpUnregisterStackModRtcpBandwidthCb(MmConnRtcpStackModRtcpBandwidthCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackModRtcpBandwidthCb = NULL;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpRegisterStackRtpSendCb(MmConnRtcpStackRtpSendCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackRtpSendCb = cb;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpUnregisterStackRtpSendCb(MmConnRtcpStackRtpSendCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackRtpSendCb = NULL;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpRegisterStackRtpReceiveCb(MmConnRtcpStackRtpReceiveCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackRtpReceiveCb = cb;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpUnregisterStackRtpReceiveCb(MmConnRtcpStackRtpReceiveCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackRtpReceiveCb = NULL;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpRegisterStackRtcpReceiveCb(MmConnRtcpStackRtcpReceiveCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackRtcpReceiveCb = cb;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpUnregisterStackRtcpReceiveCb(MmConnRtcpStackRtcpReceiveCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackRtcpReceiveCb = NULL;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpRegisterStackRtcpTimerCb(MmConnRtcpStackRtcpTimerCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackRtcpTimerCb = cb;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpUnregisterStackRtcpTimerCb(MmConnRtcpStackRtcpTimerCb cb)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (cb == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    _mmConnRtcpStackRtcpTimerCb = NULL;

    return err;
}

/**
 *
 */
MmPbxError mmConnRtcpSendRtcp(MmConnRtcpHndl  mmConnRtcp,
                              uint8_t         *buff,
                              unsigned int    bytes)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    struct sockaddr_storage dst;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (mmConnRtcp == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (buff == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    mmConnRtcpGetRemoteRtcpAddr(mmConnRtcp, &dst);
    if (dst.ss_family == AF_UNSPEC) {
        //Remote address not known, discard write
        return err;
    }

    err = mmSwitchWriteSocket(&mmConnRtcp->localRtcpSock, (struct sockaddr *) &dst, buff, bytes);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmSwitchWriteSocket (RTCP) failed with error: %s\n", mmPbxGetErrorString(err));
    }

    return err;
}

MmPbxError mmConnRtcpUpdateStackStats(MmConnRtcpHndl        mmConnRtcp,
                                      MmConnRtcpStackStats  *stats)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (mmConnRtcp == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (stats == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    mmConnRtcp->stats.meanE2eDelay              = stats->meanE2eDelay;
    mmConnRtcp->stats.worstE2eDelay             = stats->worstE2eDelay;
    mmConnRtcp->stats.rxJitter                  = stats->rxJitter;
    mmConnRtcp->stats.rxTotalPacketsLost        = stats->rxTotalPacketsLost;
    mmConnRtcp->stats.remoteTxRtpPackets        = stats->remoteTxRtpPackets;
    mmConnRtcp->stats.remoteTxRtpBytes          = stats->remoteTxRtpBytes;
    mmConnRtcp->stats.remoteRxTotalPacketsLost  = stats->remoteRxTotalPacketsLost;
    mmConnRtcp->stats.remoteRxJitter            = stats->remoteRxJitter;
    mmConnRtcp->stats.remoteRxWorstJitter       = stats->remoteRxWorstJitter;
    mmConnRtcp->stats.currentE2eDelay           = stats->currentE2eDelay;
    mmConnRtcp->stats.meanRemoteRxJitter        = stats->meanRemoteRxJitter;
    mmConnRtcp->stats.meanRxJitter              = stats->meanRxJitter;

    return err;
}

/*########################################################################
#                                                                        #
#   EXPORTS                                                              #
#                                                                        #
########################################################################*/
EXPORT_SYMBOL(mmConnRtcpRegisterStackCreateSessionsCb);
EXPORT_SYMBOL(mmConnRtcpUnregisterStackCreateSessionsCb);
EXPORT_SYMBOL(mmConnRtcpRegisterStackDestroySessionsCb);
EXPORT_SYMBOL(mmConnRtcpUnregisterStackDestroySessionsCb);
EXPORT_SYMBOL(mmConnRtcpRegisterStackRtpSendCb);
EXPORT_SYMBOL(mmConnRtcpUnregisterStackRtpSendCb);
EXPORT_SYMBOL(mmConnRtcpRegisterStackRtpReceiveCb);
EXPORT_SYMBOL(mmConnRtcpUnregisterStackRtpReceiveCb);
EXPORT_SYMBOL(mmConnRtcpRegisterStackRtcpReceiveCb);
EXPORT_SYMBOL(mmConnRtcpUnregisterStackRtcpReceiveCb);
EXPORT_SYMBOL(mmConnRtcpRegisterStackRtcpTimerCb);
EXPORT_SYMBOL(mmConnRtcpUnregisterStackRtcpTimerCb);
EXPORT_SYMBOL(mmConnRtcpRegisterStackModRtcpBandwidthCb);
EXPORT_SYMBOL(mmConnRtcpUnregisterStackModRtcpBandwidthCb);
EXPORT_SYMBOL(mmConnRtcpSendRtcp);
EXPORT_SYMBOL(mmConnRtcpUpdateStackStats);

/*########################################################################
#                                                                        #
#   PRIVATE FUNCTION DEFINITIONS                                         #
#                                                                        #
########################################################################*/

/**
 *
 */
static int isInitialised(void)
{
    return _initialised;
}

/**
 *  Lock mmConnRtcp specific part of object
 */
static void objLock(MmConnRtcpHndl mmConnRtcp)
{
    spin_lock_bh(&mmConnRtcp->lock);
}

/**
 * Release mmConnRtcp specific part of object
 */
static void objRelease(MmConnRtcpHndl mmConnRtcp)
{
    spin_unlock_bh(&mmConnRtcp->lock);
}

/**
 *
 */
static void mediaTimeoutTimerCb(MmConnHndl mmConn)
{
    MmPbxError  ret = MMPBX_ERROR_NOERROR;
    MmConnEvent event;

    event.type  = MMCONN_EV_INGRESS_MEDIA_TIMEOUT;
    event.parm  = 0;

    /*Send event to user space*/
    ret = mmConnTriggerEvent(mmConn, &event);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnTriggerEvent failed with error: %s\n", mmPbxGetErrorString(ret));
    }
}

/**
 *
 */
static void rtcpTimerCb(MmConnHndl mmConn)
{
    MmPbxError      ret         = MMPBX_ERROR_NOERROR;
    MmConnRtcpHndl  mmConnRtcp  = (MmConnRtcpHndl) mmConn;


    /*Only send RTCP if we are allowed*/
    if (mmConnRtcp->config.generateRtcp != 0) {
        /* Notify RTP/RTCP stack (which will send RTCP) */
        if (_mmConnRtcpStackRtcpTimerCb != NULL) {
            ret = _mmConnRtcpStackRtcpTimerCb(mmConnRtcp, mmConnRtcp->stackCookie, &mmConnRtcp->rtcpTimerTimeout);
            if (ret != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRtcpStackRtcpTimerCb failed with error: %s\n", mmPbxGetErrorString(ret));
            }
        }
    }

    ret = mmSwitchStartTimer(&mmConnRtcp->rtcpTimer, mmConnRtcp->rtcpTimerTimeout);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmSwitchReStartTimer (RTCP) failed with: %s\n", mmPbxGetErrorString(ret));
    }
}

/**
 *
 */
static MmPbxError mmConnChildWriteCb(MmConnHndl         conn,
                                     MmConnPacketHeader *header,
                                     uint8_t            *buff,
                                     unsigned int       bytes)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    MmConnRtcpHndl          mmConnRtcp  = (MmConnRtcpHndl) conn;
    struct sockaddr_storage dst;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (mmConnRtcp == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (buff == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (header == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    MMPBX_TRC_DEBUG("%s packet sent for RTCP conn %p\r\n", mmConnGetPacketTypeString(header->type), conn);

    mmConnRtcpGetRemoteMediaAddr(mmConnRtcp, &dst);
    if (dst.ss_family == AF_UNSPEC) {
        //Remote address not known, discard write
        return err;
    }

    if (header->type == MMCONN_PACKET_TYPE_RTP) {
        if (mmConnRtcp->stats.txRtpPackets == 0) {
            MMPBX_TRC_DEBUG("Packet type RTP; Initializing callStartJiffies\n\r");
            /* Initialise callStartTime */
            mmConnRtcp->sessionStart = get_jiffies_64();
        }

        /* Notify RTP/RTCP stack */
        if (_mmConnRtcpStackRtpSendCb != NULL) {
            err = _mmConnRtcpStackRtpSendCb(mmConnRtcp, mmConnRtcp->stackCookie, buff, bytes);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("_mmConnRtcpStackRtpSendCb failed with error: %s\n", mmPbxGetErrorString(err));
            }
        }
        /* Update Statistics */
        mmConnRtcp->stats.txRtpPackets++;
        mmConnRtcp->stats.txRtpBytes += bytes;
    }

    err = mmSwitchWriteSocket(&mmConnRtcp->localMediaSock, (struct sockaddr *) &dst, buff, bytes);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmSwitchWriteSocket failed with error: %s\n", mmPbxGetErrorString(err));
    }

    return err;
}

/**
 *
 */
static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn)
{
    MmPbxError      err         = MMPBX_ERROR_NOERROR;
    MmConnRtcpHndl  mmConnRtcp  = (MmConnRtcpHndl) mmConn;

    MMPBX_TRC_INFO("called\n");

    if (mmConnRtcp->config.mediaTimeout > 0) {
        err = mmSwitchDestroyTimer(&mmConnRtcp->mediaTimeoutTimer);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchDestroyTimer (mediaTimeout) failed with: %s\n", mmPbxGetErrorString(err));
        }
    }
    err = mmSwitchCleanupSocket(&mmConnRtcp->localMediaSock);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmSwitchCleanupSocket failed with: %s\n", mmPbxGetErrorString(err));
    }

    err = mmSwitchCleanupSocket(&mmConnRtcp->localRtcpSock);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmSwitchCleanupSocket failed with: %s\n", mmPbxGetErrorString(err));
    }

    /* Destroy RTP and RTCP session associated with this mmConnRtcp */
    if (_mmConnRtcpStackDestroySessionsCb != NULL) {
        err = _mmConnRtcpStackDestroySessionsCb(mmConnRtcp, mmConnRtcp->stackCookie);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("_mmConnRtcpStackDestroySessionsCb failed with: %s\n", mmPbxGetErrorString(err));
        }
    }

    return err;
}

/**
 * Function to handle received data from socket.
 */
static MmPbxError mmSwitchSocketCb(MmConnHndl         mmConn,
                                   void               *data,
                                   MmConnPacketHeader *header,
                                   unsigned int       bytes)
{
    MmPbxError      err         = MMPBX_ERROR_NOERROR;
    MmConnRtcpHndl  mmConnRtcp  = (MmConnRtcpHndl) mmConn;

    if (header->type == MMCONN_PACKET_TYPE_RTCP) {
        /* Notify RTP/RTCP stack */
        if (_mmConnRtcpStackRtcpReceiveCb != NULL) {
            err = _mmConnRtcpStackRtcpReceiveCb(mmConnRtcp, mmConnRtcp->stackCookie, (uint8_t *) data, bytes);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("_mmConnRtcpStackRtcpReceiveCb failed with: %s\n", mmPbxGetErrorString(err));
            }
        }
        /* We do not need to forward RTCP, so just return */
        return err;
    }

    MMPBX_TRC_DEBUG("%s packet received for RTCP conn %p\r\n", mmConnGetPacketTypeString(header->type), mmConn);


    /* Mute initial incoming media */
    if (mmConnRtcp->config.mediaMuteTime != 0) {
        /* Initialise timeout value when receiving first packet */
        if (mmConnRtcp->mediaMuteTimeout == 0) {
            mmConnRtcp->mediaMuteTimeout = jiffies + HZ * (mmConnRtcp->config.mediaMuteTime) / 1000;
            MMPBX_TRC_INFO("mmConnRtcp: %p, Mute timeout initialised\n", mmConnRtcp);
        }

        if (time_before(jiffies, mmConnRtcp->mediaMuteTimeout)) {
            MMPBX_TRC_INFO("Packet Dropped\n");
            return err;
        }
    }

    if (header->type == MMCONN_PACKET_TYPE_RTP) {
        /* Notify RTP/RTCP stack */
        if (_mmConnRtcpStackRtpReceiveCb != NULL) {
            err = _mmConnRtcpStackRtpReceiveCb(mmConnRtcp, mmConnRtcp->stackCookie, data, bytes);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("_mmConnRtcpStackRtpReceiveCb failed with error: %s\n", mmPbxGetErrorString(err));
                /* There was something wrong with received RTP packet so just return */
                return err;
            }
        }
        /* Update Statistics */
        mmConnRtcp->stats.rxRtpPackets++;
        mmConnRtcp->stats.rxRtpBytes += bytes;
    }

    /* Write */
    err = mmConnWrite(mmConn, header, data, bytes);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnWrite failed with: %s\n", mmPbxGetErrorString(err));
    }

    /* Media timeout detection */
    if (mmConnRtcp->config.mediaTimeout > 0) {
        if (mmConnRtcp->packetCounter == 0) {
            err = mmSwitchRestartTimer(&mmConnRtcp->mediaTimeoutTimer, mmConnRtcp->config.mediaTimeout);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmSwitchRestartTimer failed with: %s\n", mmPbxGetErrorString(err));
            }
        }
        mmConnRtcp->packetCounter++;
        if (mmConnRtcp->packetCounter > PACKET_TMR_RESTART_COUNT) {
            mmConnRtcp->packetCounter = 0;
        }
    }
    return err;
}

/**
 *
 */
static void mmConnRtcpGetRemoteMediaAddr(MmConnRtcpHndl           mmConnRtcp,
                                         struct sockaddr_storage  *remoteMediaAddr)
{
    objLock(mmConnRtcp);

    memcpy(remoteMediaAddr, &mmConnRtcp->config.remoteMediaAddr, sizeof(struct sockaddr_storage));

    objRelease(mmConnRtcp);
}

/**
 *
 */
static void mmConnRtcpGetRemoteRtcpAddr(MmConnRtcpHndl          mmConnRtcp,
                                        struct sockaddr_storage *remoteRtcpAddr)
{
    objLock(mmConnRtcp);

    memcpy(remoteRtcpAddr, &mmConnRtcp->config.remoteRtcpAddr, sizeof(struct sockaddr_storage));

    objRelease(mmConnRtcp);
}

/**
 *
 */
static MmPbxError mmConnChildXConnCb(MmConnHndl mmConn)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    MmConnRtcpHndl mmConnRtcp = (MmConnRtcpHndl) mmConn;

    if (mmConnRtcp == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(err));
        return err;
    }

    MMPBX_TRC_INFO("called, mmConnRtcp: %p\n", mmConnRtcp);

    /* Start RTCP transmit timer */
    if (mmConnRtcp->rtcpTimerTimeout != 0) {
        err = mmSwitchStartTimer(&mmConnRtcp->rtcpTimer, mmConnRtcp->rtcpTimerTimeout);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchStartTimer (RTCP) failed with: %s\n", mmPbxGetErrorString(err));
        }
    }

    return err;
}

/**
 *
 */
static MmPbxError mmConnChildDiscCb(MmConnHndl mmConn)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    MmConnRtcpHndl mmConnRtcp = (MmConnRtcpHndl) mmConn;

    MMPBX_TRC_INFO("called, mmConnRtcp: %p\n", mmConnRtcp);
    err = mmSwitchDestroyTimer(&mmConnRtcp->rtcpTimer);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmSwitchDestroyTimer (RTCP) failed with: %s\n", mmPbxGetErrorString(err));
    }

    return err;
}
