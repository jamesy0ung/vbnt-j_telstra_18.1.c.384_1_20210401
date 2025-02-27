/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2015-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

/** \file
 * Handling the RTCP statistics
 *
 * Parses the sent and received RTCP(XR) frames to collect the necessary
 * statistics.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONNRTCPSTATS"

/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
/*
 * limits.h is included due to a bug in libubox's json_object_get_int
 * where uint32 are incorrectly returned if their value is
 * higher than INT_MAX
 */
#include <linux/limits.h>

#include "mmswitch_p.h"
#include "mmconn_p.h"
#include "mmconntypes.h"
#include "mmconnrtcp.h"
#include "mmconnrtcpstatsdefs_p.h"
#include "mmconnrtcpstats_p.h"

/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/
//#define DEBUG

#define MAX_STATS_SESSION    7  // TODO: set at init time instead of hard-coded

/*########################################################################
#                                                                        #
#  TYPES                                                                 #
#                                                                        #
########################################################################*/
/*
 * mmConnStats structure definition
 */
/* Per session control block */
typedef struct RTPHANDLE {
    unsigned long ssrc;                         /* SSRC value of the session */
    unsigned long essrc;                        /* SSRC value of the egress stream */
    /* RTCP & RTCP-XR statistics */
    RTCPSTAT      rtcpStat;                     /* RTCP statistics */
    RTCPXRSTAT    localRtcpXrStat;              /* Detail report statistics */
    RTCPXRSTAT    remoteRtcpXrStat;             /* Detail report statistics */
    /* Saving the last RTCP reports  */
    RTCPBUFF      rxRtcpBuf[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES];
    int           lastRxRtcpBufIdx;             /* Index of the Last rtcpBuf RTCP buffer storages */
    RTCPBUFF      txRtcpBuf[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES];
    int           lastTxRtcpBufIdx;             /* Index of the Last rtcpBuf RTCP buffer storages */
} RTPHANDLE;

struct MmConnRtcpStatsSession {
    struct MmConn *mmConn;          /**< Parent class */
    int           session;          /**< Session number */
    uint64_t      start;            /**< Start of the session (jiffies) */
    uint64_t      end;              /**< End of the session (jiffies) */
    RTPHANDLE     rtpHandle;        /* The RTCP Statistics handle */
};

/*
 * RTP/RTCP (XR) statistics
 */
typedef struct {/* Connection statistics */
    /* connection statistics from the local endpoint's perspective */
    unsigned long localPktSent;               /* Packets sent */
    unsigned long localOctetSent;             /* Octets sent */
    unsigned long localPktRecvd;              /* Packets received */
    unsigned long localOctetRecvd;            /* Octets received */
    unsigned long localPktLost;               /* Packet lost */
    unsigned long localPktLostRate;           /* Packet loss rate */
    unsigned long localPktDiscarded;          /* Packet discarded */
    unsigned long localPktDiscardedRate;      /* Packet discarded Rate */
    unsigned long localJitter;                /* Interarrival jitter estimate (in ms) */
    unsigned long localMinJitter;             /* Minimum Interarrival jitter estimate (in ms) */
    unsigned long localMaxJitter;             /* Maximum Interarrival jitter estimate (in ms) */
    unsigned long localDevJitter;             /* Standard Deviation Interarrival jitter estimate (in ms) */
    unsigned long localJitterBufferOverruns;  /* Overrun count */
    unsigned long localJitterBufferUnderruns; /* Underrun count */
    unsigned long localLatency;               /* Round Trip Delay (in ms) */
    unsigned long locaMaxLatency;             /* Max Round Trip Delay (in ms) */
    unsigned long localSignalLevel;           /* Signal Level in dBm */
    unsigned long localNoiseLevel;            /* Noise level in dBm */
    unsigned long localRERL;                  /* Residual Echo Return Loss */
    unsigned long localRFactor;               /* R-Factor in voice quality */
    unsigned long localExternalRFactor;       /* External R-Factor quality from external network */
    unsigned long localMosLQ;                 /* Mean Option Score for Listening Quality, value * 10 */
    unsigned long localMosCQ;                 /* Mean Option Score for Conversational Quality, value * 10 */
    unsigned long localJitterBuffer;          /* Current Jitter Buffer Nominal Delay, in msecs */
    unsigned long localMaxJitterBuffer;       /* Maximum Jitter Buffer Nominal Delay, in msecs */
    unsigned long localWorstJitterBuffer;     /* Worst case Jitter Buffer Nominal Delay, in msecs */

    unsigned long localAverageJitter;         /* Average local interarrival jitter estimate (in ms) */
    unsigned long localAverageLatency;        /* Average local avg tx delay (in ms) */

    /* connection statistics from the remote endpoint's perspective */
    unsigned long remotePktSent;            /* Packets sent */
    unsigned long remoteOctetSent;          /* Octets sent */
    unsigned long remotePktRecvd;           /* Packets received */
    unsigned long remoteOctetRecvd;         /* Octets received */
    unsigned long remotePktLost;            /* Packet lost */
    unsigned long remotePktLostRate;        /* Packet loss rate */
    unsigned long remotePktDiscardedRate;   /* Packet discarded Rate */
    unsigned long remoteJitter;             /* Interarrival jitter estimate (in ms) */
    unsigned long remoteMinJitter;          /* Minimum Interarrival jitter estimate (in ms) */
    unsigned long remoteMaxJitter;          /* Maximum Interarrival jitter estimate (in ms) */
    unsigned long remoteDevJitter;          /* Standard Deviation Interarrival jitter estimate (in ms) */
    unsigned long remoteLatency;            /* Round Trip Delay (in ms) */
    unsigned long remoteMaxLatency;         /* Max Round Trip Delay (in ms) */
    unsigned long remoteSignalLevel;        /* Signal Level in dBm */
    unsigned long remoteNoiseLevel;         /* Noise level in dBm */
    unsigned long remoteRERL;               /* Residual Echo Return Loss */
    unsigned long remoteRFactor;            /* R-Factor in voice quality */
    unsigned long remoteExternalRFactor;    /* External R-Factor quality from external network */
    unsigned long remoteMosLQ;              /* Mean Option Score for Listening Quality, value * 10 */
    unsigned long remoteMosCQ;              /* Mean Option Score for Conversational Quality, value * 10 */
    unsigned long remoteJitterBuffer;       /* Current Jitter Buffer Nominal Delay, in msecs */
    unsigned long remoteMaxJitterBuffer;    /* Maximum Jitter Buffer Nominal Delay, in msecs */
    unsigned long remoteWorstJitterBuffer;  /* Worst case Jitter Buffer Nominal Delay, in msecs */

    unsigned long remoteAverageJitter;        /* Average local interarrival jitter estimate (in ms) */
    unsigned long remoteAverageLatency;       /* Average local avg tx delay (in ms) */

    unsigned long rtcpPktsReceived;           /* Number of RTCP packets received */
    unsigned long rtcpXrPktsReceived;         /* Number of RTCP-XR packets received */
    unsigned long rtcpPktsSent;               /* Number of RTCP packets Sent */
    unsigned long rtcpXrPktsSent;             /* Number of RTCP-XR packets sent */

    /*AT&T extension*/
    unsigned long localSumFractionLoss;     /*summary of tx fraction loss */
    unsigned long localSumSqrFractionLoss;  /*summary of square of tx fraction loss */
    unsigned long remoteSumFractionLoss;    /*summary of tx fraction loss */
    unsigned long remoteSumSqrFractionLoss; /*summary of square of tx fraction loss */
    unsigned long localSumJitter;           /*summary of tx Interarrival jitter */
    unsigned long localSumSqrJitter;        /*Sum of Square of tx Interarrival jitter */
    unsigned long remoteSumJitter;          /*Sum of tx Interarrival jitter*/
    unsigned long remoteSumSqrJitter;       /*Sum of square of tx Interarrival jitter */
    unsigned long sumRoundTripDelay;        /*Sum of Round Trip Delay */
    unsigned long sumSqrRoundTripDelay;     /*Sum of Square of Max Round Trip Delay */
    unsigned long maxOneWayDelay;           /*Max One Way Delay */
    unsigned long sumOneWayDelay;           /*Sum of One Way Delay */
    unsigned long sumSqrOneWayDelay;        /*Sum of Square of One Way Delay */

    /* RTCP frame buffer */
    unsigned long txRtcpFrameLength[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES];  /**<Saved sent RTCP frame buffer length */
    uint8_t       *txRtcpFrameBuffer[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES]; /**<Saved sent RTCP frame buffer content */
    unsigned long rxRtcpFrameLength[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES];  /**<Saved received RTCP frame buffer length */
    uint8_t       *rxRtcpFrameBuffer[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES]; /**<Saved received RTCP frame buffer content */
} MmConnRtcpCnxStats;

/*########################################################################
#                                                                        #
#  PRIVATE DATA MEMBERS                                                  #
#                                                                        #
########################################################################*/
static int  _traceLevel   = MMPBX_TRACELEVEL_NONE;
static int  _initialised  = FALSE;

static const int  MIN_RTCPSR  = sizeof(RTCPCOMMON) + sizeof(RTCPSR) - sizeof(RTCPRR);
static const int  MIN_RTCPRR  = sizeof(RTCPCOMMON) + sizeof(unsigned int);   /* Header + SSRC */

static MmConnRtcpStatsSessionHndl sessions[MAX_STATS_SESSION];
static DEFINE_RWLOCK(_rtcpBufferLock);

/*########################################################################
#                                                                       #
#  PRIVATE FUNCTION PROTOTYPES                                          #
#                                                                       #
########################################################################*/
static int isInitialised(void);
static MmPbxError validateSession(MmConnRtcpStatsSessionHndl mmConnStats);
static bool doesSessionExist(MmConnHndl mmConn);

static MmPbxError updateSession(MmConnRtcpStatsSessionHndl  session,
                                uint8_t                     *buff,
                                int                         len,
                                int                         Incoming);
static void rtcpProcessRR(int         handle,
                          RTCPPACKET  *rtcpbuf,
                          int         bIncoming);
static void rtcpProcessSR(int         handle,
                          RTCPPACKET  *rtcpbuf,
                          int         bIncoming);
static void rtcpProcessXR(int         handle,
                          RTCPPACKET  *rtcpBuf,
                          //uint8_t     *buff,
                          int         bIncoming);
static void rtcpProcessBye(int        handle,
                           RTCPPACKET *rtcpbuf,
                           int        bIncoming);
static void rtcpRecordRRInfo(int    handle,
                             int    count,
                             RTCPRR *rrlst,
                             int    bIncoming);
static void networkToHostRTCPPACKET(void        *buff,
                                    RTCPPACKET  **rtcpbuf);
static void networkToHostRTCPXR(void    *buff,
                                RTCPXR  **pXr);
static void networkToHostRTCPXR_BT1(RTCPXR_BT1  *buff,
                                    RTCPXR_BT1  **pBt);
static void networkToHostRTCPXR_BT3(RTCPXR_BT3  *buff,
                                    RTCPXR_BT3  **pBt);
static void networkToHostRTCPXR_BT4(RTCPXR_BT4  *buff,
                                    RTCPXR_BT4  **pBt);
static void networkToHostRTCPXR_BT6(RTCPXR_BT6  *buff,
                                    RTCPXR_BT6  **pBt);
static void networkToHostRTCPXR_BT7(RTCPXR_BT7  *buff,
                                    RTCPXR_BT7  **pBt);


static void clearRtcpCnxStats(RTCPXRSTAT *pXrStat);
static void rtcpSaveFrame(int     handle,
                          uint8_t *buff,
                          int     len,
                          int     bIncoming);
static void rtcpUpdateInfo(int                        handle,
                           MmConnDspRtpStatsParmHndl  epRtpStats,
                           MmConnRtcpCnxStats         *rtcpCnxStats);
static void rtcpXrUpdateInfo(int                handle,
                             MmConnRtcpCnxStats *rtcpCnxStats);
static void rtcpUpdateBufferedFrame(RTPHANDLE           *handle,
                                    MmConnRtcpCnxStats  *rtcpCnxStats);

#ifdef DEBUG
static void dumpDspStats(MmConnDspRtpStatsParmHndl pStats);
#endif

/*########################################################################
#                                                                        #
#  FUNCTION DEFINITIONS                                                  #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmConnRtcpStatsInit(void)
{
    MmPbxError  ret = MMPBX_ERROR_NOERROR;
    int         i   = 0;

    for (i = 0; i < MAX_STATS_SESSION; i++) {
        sessions[i] = NULL;
    }

    _initialised = TRUE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpStatsDeinit(void)
{
    MmPbxError  ret = MMPBX_ERROR_NOERROR;
    int         i   = 0;

    for (i = 0; i < MAX_STATS_SESSION; i++) {
        if (sessions[i] != NULL) {
            mmConnRtcpStatsCloseSession(&sessions[i]);
        }
    }

    _initialised = FALSE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpStatsSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));

    return MMPBX_ERROR_NOERROR;
}

MmPbxError mmConnRtcpStatsOpenSession(MmConnHndl mmConn, MmConnRtcpStatsSessionHndl *session)
{
    MmPbxError                  ret         = MMPBX_ERROR_NOERROR;
    MmConnRtcpStatsSessionHndl  tempSession = NULL;
    int                         i           = 0;

    MMPBX_TRC_DEBUG("Creating Statistics session: %p\n\r", mmConn);

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_ERROR("Stats Handle not yet initialized error: %s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }
    if (doesSessionExist(mmConn) == true) {   /* Found: Double allocation */
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("Stats session with connection %p already opened: %s\n\r", mmConn, mmPbxGetErrorString(ret));
        return ret;
    }

    do {
        /* Search for unassigned handle */
        for (i = 0; i < MAX_STATS_SESSION; i++) {
            if (sessions[i] == NULL) {
                break;
            }
        }

        if (i == MAX_STATS_SESSION) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
            break;
        }
        tempSession = kmalloc(sizeof(struct MmConnRtcpStatsSession), GFP_KERNEL);
        if (tempSession == NULL) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
            break;
        }
        memset(tempSession, 0, sizeof(struct MmConnRtcpStatsSession));

        tempSession->mmConn   = mmConn;
        tempSession->session  = i;

        sessions[i] = tempSession;

        /* Update the handle and callbacks */
        *session = tempSession;
    } while (0);

    if (ret != MMPBX_ERROR_NOERROR) {
        if (tempSession != NULL) {
            kfree(tempSession);
        }
        MMPBX_TRC_ERROR("fail to create RTP session, error: %s\n\r", mmPbxGetErrorString(ret));
    }
    return ret;
}

/**
 *
 */
MmPbxError mmConnRtcpStatsCloseSession(MmConnRtcpStatsSessionHndl *session)
{
    MmPbxError  ret = MMPBX_ERROR_NOERROR;
    int         i;

    if ((session == NULL) || (*session == NULL)) {
        return MMPBX_ERROR_INVALIDPARAMS;
    }

    MMPBX_TRC_DEBUG("Closing session: %p\n\r", *session);

    ret = validateSession(*session);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
        return ret;
    }

    /* Clear the Local info */
    clearRtcpCnxStats(&((*session)->rtpHandle.localRtcpXrStat));
    /* Clear the remote info */
    clearRtcpCnxStats(&((*session)->rtpHandle.remoteRtcpXrStat));

    /* Clear the stored RTCP buffers */
    write_lock(&_rtcpBufferLock);
    for (i = 0; i < MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES; i++) {
        if ((*session)->rtpHandle.txRtcpBuf[i].buf != NULL) {
            kfree((*session)->rtpHandle.txRtcpBuf[i].buf);
        }
        if ((*session)->rtpHandle.rxRtcpBuf[i].buf != NULL) {
            kfree((*session)->rtpHandle.rxRtcpBuf[i].buf);
        }
    }
    write_unlock(&_rtcpBufferLock);
    /* Decouple the handler */
    sessions[(*session)->session] = NULL;

    /* Delete the statistics session */
    kfree(*session);
    *session = NULL;

    return ret;
}

/*
 * *****************************************************************************
 * ** FUNCTION:   mmConnStatsResetSession
 * **
 * ** PURPOSE:    This function reset the RTCP statistics
 * **
 * ** PARAMETERS: handle - [IN] RTP session handle
 * **
 * ** RETURNS:    None
 * **
 * ** NOTE:
 * *****************************************************************************
 * */

MmPbxError mmConnRtcpStatsResetSession(MmConnRtcpStatsSessionHndl session)
{
    MmPbxError  ret         = MMPBX_ERROR_NOERROR;
    RTPHANDLE   *rtpHandle  = NULL;
    RTCPBUFF    *pBuf       = NULL;
    int         i           = 0;

    MMPBX_TRC_DEBUG("Reset Statistics session: %p\n\r", session);

    ret = validateSession(session);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
        return ret;
    }

    session->start  = get_jiffies_64();
    session->end    = 0;

    (void)mmConnDspControl(session->mmConn, NULL, MMCONN_DSP_RESET_STATS);

    rtpHandle = &session->rtpHandle;

    /* Reset RTCP statistics / info block */
    memset(&rtpHandle->rtcpStat, 0, sizeof(RTCPSTAT));

    /* Reset RTCP-XR statistics / info block */
    clearRtcpCnxStats(&rtpHandle->localRtcpXrStat);
    memset(&rtpHandle->localRtcpXrStat, 0, sizeof(RTCPXRSTAT));

    clearRtcpCnxStats(&rtpHandle->remoteRtcpXrStat);
    memset(&rtpHandle->remoteRtcpXrStat, 0, sizeof(RTCPXRSTAT));

    /* Reset RTCP packet Tx buffer */
    write_lock(&_rtcpBufferLock);
    for (i = 0; i < MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES; i++) {
        pBuf = &rtpHandle->txRtcpBuf[i];
        if (pBuf->buf != NULL) {
            kfree(pBuf->buf);
        }
    }
    memset(&rtpHandle->txRtcpBuf[0], 0, sizeof(rtpHandle->txRtcpBuf));

    /* Reset RTCP packet Rx buffer */
    for (i = 0; i < MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES; i++) {
        pBuf = &rtpHandle->rxRtcpBuf[i];
        if (pBuf->buf != NULL) {
            kfree(pBuf->buf);
        }
    }
    memset(&rtpHandle->rxRtcpBuf[0], 0, sizeof(rtpHandle->rxRtcpBuf));

    rtpHandle->lastRxRtcpBufIdx = rtpHandle->lastTxRtcpBufIdx = 0;
    write_unlock(&_rtcpBufferLock);

    return ret;
}

MmPbxError mmConnRtcpStatsGetSessionStats(MmConnRtcpStatsSessionHndl session, MmConnRtcpStats *pStats)
{
    MmPbxError                    ret       = MMPBX_ERROR_NOERROR;
    MmConnRtcpCnxStats            cnxStats  = { 0 };
    struct MmConnDspRtpStatsParm  dspStats  = { 0 };
    int                           i         = 0;

    do {
        ret = validateSession(session);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
            break;
        }
        if (pStats == NULL) {
            ret = MMPBX_ERROR_INVALIDPARAMS;
            MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
            break;
        }

        if (session->end == 0) {
            session->end = get_jiffies_64();
        }

        pStats->sessionDuration = jiffies_to_msecs(session->end - session->start);

        /* First get DSP statistics and update the delta */
        if ((mmConnDspControl(session->mmConn, &dspStats, MMCONN_DSP_GET_STATS)) == MMPBX_ERROR_NOERROR) {
#ifdef DEBUG
            dumpDspStats(&dspStats);
#endif
            /* Update RTCP Connection Statistics with DSP statistics */
            rtcpUpdateInfo(session->session, &dspStats, &cnxStats);
        }
        else {
            /* Update RTCP Connection Statistics with DSP statistics */
            rtcpUpdateInfo(session->session, NULL, &cnxStats);
        }

        /* Update RTCP Connection Statistics with RTCP-XR Statistics */
        rtcpXrUpdateInfo(session->session, &cnxStats);
        pStats->rxTotalPacketsLost      = cnxStats.localPktLost;
        pStats->rxTotalPacketsDiscarded = cnxStats.localPktDiscarded;
        pStats->rxPacketsLostRate       = cnxStats.localPktLostRate;
        pStats->rxPacketsDiscardedRate  = cnxStats.localPktDiscardedRate;
        pStats->signalLevel             = cnxStats.localSignalLevel;
        pStats->noiseLevel            = cnxStats.localNoiseLevel;
        pStats->RERL                  = cnxStats.localRERL;
        pStats->RFactor               = cnxStats.localRFactor;
        pStats->externalRFactor       = cnxStats.localExternalRFactor;
        pStats->mosLQ                 = cnxStats.localMosLQ;
        pStats->mosCQ                 = cnxStats.localMosCQ;
        pStats->meanE2eDelay          = cnxStats.localAverageLatency;
        pStats->worstE2eDelay         = cnxStats.locaMaxLatency;
        pStats->currentE2eDelay       = cnxStats.localLatency;
        pStats->rxJitter              = cnxStats.localJitter;
        pStats->rxMinJitter           = cnxStats.localMinJitter;
        pStats->rxMaxJitter           = cnxStats.localMaxJitter;
        pStats->rxDevJitter           = cnxStats.localDevJitter;
        pStats->meanRxJitter          = cnxStats.localAverageJitter;
        pStats->worstRxJitter         = cnxStats.localWorstJitterBuffer ? cnxStats.localWorstJitterBuffer : cnxStats.localMaxJitter;
        pStats->jitterBufferOverruns  = cnxStats.localJitterBufferOverruns;
        pStats->jitterBufferUnderruns = cnxStats.localJitterBufferUnderruns;

        pStats->remoteTxRtpPackets            = cnxStats.remotePktSent;
        pStats->remoteTxRtpBytes              = cnxStats.remoteOctetSent;
        pStats->remoteRxTotalPacketsLost      = cnxStats.remotePktLost;
        pStats->remoteRxPacketsLostRate       = cnxStats.remotePktLostRate;
        pStats->remoteRxPacketsDiscardedRate  = cnxStats.remotePktDiscardedRate;
        pStats->remoteSignalLevel             = cnxStats.remoteSignalLevel;
        pStats->remoteNoiseLevel              = cnxStats.remoteNoiseLevel;
        pStats->remoteRERL            = cnxStats.remoteRERL;
        pStats->remoteRFactor         = cnxStats.remoteRFactor;
        pStats->remoteExternalRFactor = cnxStats.remoteExternalRFactor;
        pStats->remoteMosLQ           = cnxStats.remoteMosLQ;
        pStats->remoteMosCQ           = cnxStats.remoteMosCQ;
        pStats->remoteMeanE2eDelay    = cnxStats.remoteAverageLatency;
        pStats->remoteWorstE2eDelay   = cnxStats.remoteMaxLatency;
        pStats->remoteCurrentE2eDelay = cnxStats.remoteLatency;
        pStats->remoteRxJitter        = cnxStats.remoteJitter;
        pStats->minRemoteRxJitter     = cnxStats.remoteMinJitter;
        pStats->maxRemoteRxJitter     = cnxStats.remoteMaxJitter;
        pStats->devRemoteRxJitter     = cnxStats.remoteDevJitter;
        pStats->meanRemoteRxJitter    = cnxStats.remoteAverageJitter;
        pStats->remoteRxWorstJitter   = cnxStats.remoteWorstJitterBuffer ? cnxStats.remoteWorstJitterBuffer : cnxStats.remoteMaxJitter;
        pStats->txRtcpXrPackets       = cnxStats.rtcpXrPktsSent;
        pStats->rxRtcpXrPackets       = cnxStats.rtcpXrPktsReceived;

        /*extension for AT&T*/
        pStats->txRtcpPackets = cnxStats.rtcpPktsSent;
        pStats->rxRtcpPackets = cnxStats.rtcpPktsReceived;

        pStats->localSumFractionLoss      = cnxStats.localSumFractionLoss;
        pStats->localSumSqrFractionLoss   = cnxStats.localSumSqrFractionLoss;
        pStats->remoteSumFractionLoss     = cnxStats.remoteSumFractionLoss;
        pStats->remoteSumSqrFractionLoss  = cnxStats.remoteSumSqrFractionLoss;
        pStats->localSumJitter            = cnxStats.localSumJitter;
        pStats->localSumSqrJitter         = cnxStats.localSumSqrJitter;
        pStats->remoteSumJitter           = cnxStats.remoteSumJitter;
        pStats->remoteSumSqrJitter        = cnxStats.remoteSumSqrJitter;

        pStats->sumRoundTripDelay     = cnxStats.sumRoundTripDelay;
        pStats->sumSqrRoundTripDelay  = cnxStats.sumSqrRoundTripDelay;
        pStats->maxOneWayDelay        = cnxStats.maxOneWayDelay;
        pStats->sumOneWayDelay        = cnxStats.sumOneWayDelay;
        pStats->sumSqrOneWayDelay     = cnxStats.sumSqrOneWayDelay;

        rtcpUpdateBufferedFrame(&session->rtpHandle, &cnxStats);
        for ( ; i < MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES; i++) {
            pStats->rxRtcpFrameLength[i]  = cnxStats.rxRtcpFrameLength[i];
            pStats->txRtcpFrameLength[i]  = cnxStats.txRtcpFrameLength[i];
            pStats->rxRtcpFrameBuffer[i]  = cnxStats.rxRtcpFrameBuffer[i];
            pStats->txRtcpFrameBuffer[i]  = cnxStats.txRtcpFrameBuffer[i];
        }
    } while (0);

    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("Error getting statistics: %s\n\r", mmPbxGetErrorString(ret));
    }

    return ret;
}

/*
 *****************************************************************************
 ** FUNCTION:   mmConnStatsOnPacketReceived (Brcm function: rtcpRecv)
 **
 ** PURPOSE:    This function processes the received RTCP control packet.  It
 **             parses RTCP packet and store packet information for future
 **             reference (e.g., SR recept time)
 **
 ** PARAMETERS: handle - [IN] Handle to the RTP session control block
 **             buf    - [IN] Buffer of the RTCP packet to process
 **             len    - [IN] Packet (i.e. header + payload) size in byte
 **
 ** RETURNS:    None
 **
 ** NOTE:       The control structure indexed by the handle is updated
 *****************************************************************************
 */
MmPbxError mmConnRtcpStatsOnPacketReceived(MmConnRtcpStatsSessionHndl session, MmConnPacketType type, uint8_t *buff, int len)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    MMPBX_TRC_DEBUG("%s packet received for session %p; frame length %d\n\r", mmConnGetPacketTypeString(type), session, len);

    switch (type) {
        case MMCONN_PACKET_TYPE_RAW:
        case MMCONN_PACKET_TYPE_UDPTL:
            return MMPBX_ERROR_INVALIDPARAMS;

        case MMCONN_PACKET_TYPE_RTP:
            /* nothing to do */
            return MMPBX_ERROR_NOERROR;

        case MMCONN_PACKET_TYPE_RTCP:
            break;
    }

    ret = validateSession(session);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
    }
    else {
        ret = updateSession(session, buff, len, TRUE);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
        }
    }

    return ret;
}

/*
 *****************************************************************************
 ** FUNCTION:   mmConnStatsOnPacketSent (Brcm function: rtcpSend)
 **
 ** PURPOSE:    This function sends a RTCP packet
 **
 ** PARAMETERS: rtpp        - [IN] Ptr to the RTP session control block
 **             rtcpPacket  - [IN] Ptr to the RTCP packet
 **             pktsize     - [IN] packet size
 **
 ** RETURNS:    None
 **
 ** NOTE:
 *****************************************************************************
 */
MmPbxError mmConnRtcpStatsOnPacketSent(MmConnRtcpStatsSessionHndl session, MmConnPacketType type, uint8_t *buff, int len)
{
    MmPbxError  ret       = MMPBX_ERROR_NOERROR;
    RTCPSTAT    *rtcpStat = &session->rtpHandle.rtcpStat;

    MMPBX_TRC_DEBUG("%s packet sent for session %p; frame length %d\n\r", mmConnGetPacketTypeString(type), session, len);

    switch (type) {
        case MMCONN_PACKET_TYPE_RAW:
        case MMCONN_PACKET_TYPE_UDPTL:
            return MMPBX_ERROR_INVALIDPARAMS;

        case MMCONN_PACKET_TYPE_RTP:
            if (session->start == 0) {
                session->start = get_jiffies_64();
                MMPBX_TRC_DEBUG("Session started\r\n");
            }
            return MMPBX_ERROR_NOERROR;

        case MMCONN_PACKET_TYPE_RTCP:
            break;
    }

    if (rtcpStat != NULL) {
        rtcpStat->rtcpPktsSent++;
    }
    ret = validateSession(session);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
    }
    else {
        ret = updateSession(session, buff, len, FALSE);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
        }
    }

    return ret;
}

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
 * Check if the mmConnStats Handler has been registered
 */

static MmPbxError validateSession(MmConnRtcpStatsSessionHndl session)
{
    int i;

    for (i = 0; i < MAX_STATS_SESSION; i++) {
        if (sessions[i] == session) {
            break;
        }
    }

    return i < MAX_STATS_SESSION ? MMPBX_ERROR_NOERROR : MMPBX_ERROR_INVALIDPARAMS;
}

/**
 * Check if the mmConn handler has been registered
 */
static bool doesSessionExist(MmConnHndl mmConn)
{
    int i = 0;

    for (i = 0; i < MAX_STATS_SESSION; i++) {
        if (sessions[i] != NULL) {
            if (sessions[i]->mmConn == mmConn) {
                break;
            }
        }
    }
    return (i < MAX_STATS_SESSION) ? true : false;
}

#ifdef DEBUG
static void dumpDspStats(MmConnDspRtpStatsParmHndl pStats)
{
    if (pStats) {
        printk("txpkts:       %8u txBytes:      %8u\n\r", pStats->txpkts, pStats->txbytes);
        printk("rxpkts:       %8u rxBytes:      %8u\n\r", pStats->rxpkts, pStats->rxbytes);
        printk("lost:         %8u discarded:    %8u\n\r", pStats->lost, pStats->discarded);
        printk("txRtcpPkts:   %8u rxRtcpPkts:   %8u\n\r", pStats->txRtcpPkts, pStats->rxRtcpPkts);
        printk("txRtcpXrPkts: %8u rxRtcpXrPkts: %8u\n\r", pStats->txRtcpXrPkts, pStats->rxRtcpXrPkts);
        printk("jbOverRuns:   %8u jbUnderRuns:  %8u\n\r", pStats->jitterBufferOverruns, pStats->jitterBufferUnderruns);
        printk("jitter:       %8u peakjitter:   %8u\n\r", pStats->jitter, pStats->peakJitter);
        printk("jbMin:        %8u jbAvg:        %8u jbMax: %8u\n\r", pStats->jitterBufferMin, pStats->jitterBufferAverage, pStats->jitterBufferMax);
        printk("latency:      %8u peaklatency:  %8u\n\r", pStats->latency, pStats->peakLatency);
        printk("MOSLQ:        %8u MOSCQ:        %8u\n\r", pStats->mosLQ, pStats->mosCQ);
    }
}
#endif

/**
 * Parse the RTCP frames and update the RTCP statistics according to the
 * frame direction
 */
static MmPbxError updateSession(MmConnRtcpStatsSessionHndl session, uint8_t *buff, int len, int incoming)
{
    MmPbxError  ret       = MMPBX_ERROR_NOERROR;
    RTCPPACKET  *rtcpbuf  = NULL;
    int         restLen   = 0;
    int         blockLen  = 0;

    MMPBX_TRC_DEBUG("Start RTCP Update Rtcp-Packet: %p\n\r", buff);
    networkToHostRTCPPACKET(buff, &rtcpbuf);
    if ((rtcpbuf == NULL) || (session == NULL)) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n\r", mmPbxGetErrorString(ret));
        return ret;
    }

#ifdef DEBUG
    printk("Rtcp-packet: %p Update Statistics session: %p Frame Length: %d direction: %s\n\r", buff, session, len, incoming ? "Incoming" : "Outgoing");
#endif

    if (rtcpbuf->common.version != RTP_VERSION) {
        MMPBX_TRC_DEBUG("RTP version %d differs from expected version %d; skipping packet.r\n", rtcpbuf->common.version, RTP_VERSION);
        return ret;
    }

    /* Validate rtcp packet */
    if ((rtcpbuf->common.packetType != RTCP_SR) && (rtcpbuf->common.packetType != RTCP_RR)) {
        MMPBX_TRC_DEBUG("Non RTCP SR/RR packet received, skipping packet.\n\r");
        return ret;
    }

    do {
        restLen = len;
        while (restLen > 0) {
            blockLen = (rtcpbuf->common.length + 1 /* common block */) * 4;
#ifdef DEBUG
            printk("Rtcp-Packet: %p rtcpBuf: %p BuffLen: %d BlockLen: %d restLen: %d BlockLen in Bytes (+hdr): %d Packet Type: %d\n\r", buff, rtcpbuf, len, rtcpbuf->common.length, restLen, blockLen, rtcpbuf->common.packetType);
#endif
            switch (rtcpbuf->common.packetType) {
                case RTCP_SR:
                    /* Validate SR packet */
                    if ((rtcpbuf->common.length < (MIN_RTCPSR / 4 - 1)) || (len < MIN_RTCPSR)) {
                        ret = MMPBX_ERROR_INVALIDTYPE;
                        /* Ensure that the SR packet has the correct size */
                        MMPBX_TRC_ERROR("ERROR rtcpRecv: Bad RTCP SR received (incorrect size); error: %s\n\r", mmPbxGetErrorString(ret));
                        break;
                    }

                    MMPBX_TRC_DEBUG("Recv RTCP SR session: %d\n\r", session->session);
                    rtcpProcessSR(session->session, rtcpbuf, incoming);
                    break;

                case RTCP_RR:
                    MMPBX_TRC_DEBUG("Recv RTCP RR; Session: %d\n\r", session->session);
                    rtcpProcessRR(session->session, rtcpbuf, incoming);
                    break;

                case RTCP_XR:
                    MMPBX_TRC_DEBUG("RTCP-XR Block; Session: %d\n\r", session->session);
                    rtcpProcessXR(session->session, rtcpbuf /*, buff*/, incoming);
                    break;

                case RTCP_BYE:
                    MMPBX_TRC_DEBUG("RTCP-BYE Block; Session: %d\n\r", session->session);
                    rtcpProcessBye(session->session, rtcpbuf, incoming);
                    break;

                default:
                    break;
            }
            restLen -= blockLen;
            if (restLen <= 0) {
                break;
            }
            networkToHostRTCPPACKET(((uint8_t *)rtcpbuf + blockLen), &rtcpbuf); /* Forward to the next report block */
            if (rtcpbuf == NULL) {
                MMPBX_TRC_ERROR("Invalid/NULL rtcp buffer \n\r");
                break;
            }
        }
    } while (0);

    if (ret == MMPBX_ERROR_NOERROR) {
        /* Save the last 2 RTCP buffers */
        rtcpSaveFrame(session->session, buff, len, incoming);
    }
    MMPBX_TRC_DEBUG("Rtcp-Packet: %p Finish Update statistics result: %s\n\r", buff, mmPbxGetErrorString(ret));

    return ret;
}

/*
 *****************************************************************************
 ** FUNCTION:   rtcpProcessRR
 **
 ** PURPOSE:    This function processes received receiver report
 **
 ** PARAMETERS: handle  - [IN] Handle to the RTP session control block
 **             rtcpbuf   - [IN] Ptr to the RTCP packet
 **             bIncoming - [IN] Indicates whether it is an incoming RTCP or
 **                              outgoing RTCP
 **
 ** RETURNS:    None
 **
 ** NOTE:       Only NTP timestamp is needed to calculate round-trip statistics
 *****************************************************************************
 */
static void rtcpProcessRR(int handle, RTCPPACKET *rtcpbuf, int bIncoming)
{
    MMPBX_TRC_DEBUG("Processing RR Packet Type of Rtcp: %p Incoming: %s\n\r", rtcpbuf, bIncoming ? "true" : "false");

    if (rtcpbuf->common.count) {
        /* Process RR info */
        rtcpRecordRRInfo(handle, rtcpbuf->common.count, &rtcpbuf->r.rr.rr[0], bIncoming);
    }
}

/*
 *****************************************************************************
 ** FUNCTION:   rtcpProcessSR
 **
 ** PURPOSE:    This function processes received sender report
 **
 ** PARAMETERS: handle  - [IN] Handle to the RTP session control block
 **             rtcpbuf - [IN] Ptr to the RTCP packet
 **             bIncoming - [IN] Indicates whether it is an incoming RTCP or
 **                              outgoing RTCP
 **
 ** RETURNS:    None
 **
 ** NOTE:       Only NTP timestamp is needed to calculate round-trip statistics
 *****************************************************************************
 */
static void rtcpProcessSR(int handle, RTCPPACKET *rtcpbuf, int bIncoming)
{
    RTCPSR                      *sr;
    MmConnRtcpStatsSessionHndl  mmStat    = sessions[handle];
    RTCPSTAT                    *rtcpStat = &mmStat->rtpHandle.rtcpStat;
    unsigned long               tmp;

    MMPBX_TRC_DEBUG("Processing SR Packet Type of Rtcp: %p Incoming: %s\n\r", rtcpbuf, bIncoming ? "true" : "false");

    sr = &rtcpbuf->r.sr;

    if (bIncoming == TRUE) {
        /* RTCP SR packet received */
        rtcpStat->rtcpSRRecv = TRUE;

        /* Record sender's NTP values for last SR timestamp (LSR) */
        rtcpStat->lastntpsec  = ntohl(sr->ntp_sec);
        rtcpStat->lastntpfrac = ntohl(sr->ntp_frac);

        MMPBX_TRC_DEBUG("last NTP SEC 0x%X FRAC 0x%X\n\r", (unsigned int)(rtcpStat->lastntpsec), (unsigned int)(rtcpStat->lastntpfrac));

        /* Update far-end delta values */
        rtcpStat->dwRemotePacketSent  += ntohl(sr->psent) - rtcpStat->dwPrevCumRemotePacketSent;
        rtcpStat->dwRemoteOctetSent   += ntohl(sr->osent) - rtcpStat->dwPrevCumRemoteOctetSent;

        /* Update far-end cumulative values */
        rtcpStat->dwPrevCumRemotePacketSent = ntohl(sr->psent);
        rtcpStat->dwPrevCumRemoteOctetSent  = ntohl(sr->osent);

        /*AT&T extension*/
        tmp = ntohl(sr->rr[0].fraction);
        rtcpStat->remoteSumFractionLoss += tmp;
        if (rtcpStat->remoteSumFractionLoss > INT_MAX) {
            rtcpStat->remoteSumFractionLoss = INT_MAX;
        }
        rtcpStat->remoteSumSqrFractionLoss += tmp * tmp;
        if (rtcpStat->remoteSumSqrFractionLoss > INT_MAX) {
            rtcpStat->remoteSumSqrFractionLoss = INT_MAX;
        }
        tmp = ntohl(sr->rr[0].jitter);
        rtcpStat->remoteSumJitter += tmp;
        if (rtcpStat->remoteSumJitter > INT_MAX) {
            rtcpStat->remoteSumJitter = INT_MAX;
        }
        rtcpStat->remoteSumSqrJitter += tmp * tmp;
        if (rtcpStat->remoteSumSqrJitter > INT_MAX) {
            rtcpStat->remoteSumSqrJitter = INT_MAX;
        }

        MMPBX_TRC_DEBUG("REMPSENT %d REMOCSENT %d RRCOUNT %d\n\r", (unsigned int)(rtcpStat->dwRemotePacketSent), (unsigned int)(rtcpStat->dwRemoteOctetSent), rtcpbuf->common.count);
    }
    else {
        /*AT&T extension*/
        tmp = ntohl(sr->rr[0].fraction);
        rtcpStat->localSumFractionLoss += tmp;
        if (rtcpStat->localSumFractionLoss > INT_MAX) {
            rtcpStat->localSumFractionLoss = INT_MAX;
        }
        rtcpStat->localSumSqrFractionLoss += tmp * tmp;
        if (rtcpStat->localSumSqrFractionLoss > INT_MAX) {
            rtcpStat->localSumSqrFractionLoss = INT_MAX;
        }
        tmp = ntohl(sr->rr[0].jitter);
        rtcpStat->localSumJitter += tmp;
        if (rtcpStat->localSumJitter > INT_MAX) {
            rtcpStat->localSumJitter = INT_MAX;
        }
        rtcpStat->localSumSqrJitter += tmp * tmp;
        if (rtcpStat->localSumSqrJitter > INT_MAX) {
            rtcpStat->localSumSqrJitter = INT_MAX;
        }
    }
}

/*
 *****************************************************************************
 ** FUNCTION:   rtcpProcessXR
 **
 ** PURPOSE:    This function process received receiver report
 **
 ** PARAMETERS: handle  - [IN] Handle to the RTP session control block
 **             count   - [IN] Number of RR
 **             rrlst   - [IN] List of RRs
 **             bIncoming - [IN] Indicates whether it is an incoming RTCP or
 **                         outgoing RTCP
 **
 ** RETURNS:    None
 **
 ** NOTE:       Only NTP timestamp is needed to calculate round-trip statistics
 **             The control structure indexed by the handle is updated
 *****************************************************************************
 */
static void rtcpProcessXR(int handle, RTCPPACKET *rtcpBuf, /*uint8_t *buff,*/ int bIncoming)
{
    MmConnRtcpStatsSessionHndl  mmStat        = sessions[handle];
    RTCPSTAT                    *rtcpStat     = &mmStat->rtpHandle.rtcpStat;
    RTCPXR                      *pXr          = NULL;
    RTCPXRSTAT                  *pXrStat      = NULL;
    int                         totBlocksLen  = 0;
    int                         blockLen      = 0;

    MMPBX_TRC_DEBUG("Processing RTCP-XR Packet Type Rtcp: %p Packet length: %d\n\r", rtcpBuf, rtcpBuf->common.length);

    totBlocksLen = ((rtcpBuf->common.length + 1 /* RTCP 207 common block */) * 4) - 8 /* RTCPCOMMON + SSRC */;

    networkToHostRTCPXR(&rtcpBuf->r.xr.xr[0], &pXr);   /* Point to the first Block Type */
    if (pXr == NULL) {
        MMPBX_TRC_ERROR("Invalid/NULL XR packet\n\r");
        return;
    }
    if (bIncoming == TRUE) {
        pXrStat = &mmStat->rtpHandle.remoteRtcpXrStat;
    }
    else {
        if (rtcpStat != NULL) {
            rtcpStat->rtcpXrPktsSent++;
        }

        pXrStat = &mmStat->rtpHandle.localRtcpXrStat;
    }

    pXrStat->rtcpXrUpdated = 1;

    while (totBlocksLen > 0) {
        blockLen = (pXr->common.blockLength + 1 /* RTCPXR common Block type header */) * 4;
#ifdef DEBUG
        printk("\n\rIncoming %s pXr: %p Block Type: %d Block Length: %d sizeof(pXr->common): %d sizeof(pXr->common.typeSpecific): %d pXr->common.typeSpecific: 0x%x Actual Block Length (incl common header): %d totBlockLen: %d\n\r",
               bIncoming ? "true" : "false", pXr, pXr->common.blockTypeId, pXr->common.blockLength, sizeof(pXr->common), sizeof(pXr->common.typeSpecific), pXr->common.typeSpecific, blockLen, totBlocksLen);
#endif
        switch (pXr->common.blockTypeId) {
            case 1: /* Loss RLE (Report Length Encoding) */
            {
                RLE_CHUNK_INFO  *pInfo        = NULL;
                unsigned short  *pReport      = NULL;
                int             chunksLength  = 0;
                int             chunk         = 0;
                RTCPXR_BT1      *pBt          = NULL;
                networkToHostRTCPXR_BT1(&pXr->blockTypes.bt1, &pBt);
                if (pBt == NULL) {
                    MMPBX_TRC_ERROR("Invalid/NULL Block buffer \n\r");
                    break;
                }


                if (pXr->common.blockLength < RTCPXR_MIN_RLE_BLOCKTYPE_SIZE) {
                    /* Block Type 1 is too small, incorrect info */
                    break;
                }
                pXrStat->rtcpXrUpdated |= (1 << pXr->common.blockTypeId);

                pXrStat->lrleStartSeq       = pBt->beginSeq;                                    /* Start of RTP transmission */
                pXrStat->lrleEndSeq         = pBt->endSeq;                                      /* The last RTP transmission */
                pXrStat->lrleThinningValue  = pXr->common.typeSpecific.thinningHeader.thinning; /* Thinning factor */
                chunksLength = pXr->common.blockLength - 2 /* SSRC & Beg+End Seq */;
                /* chunk blockLength is in 32 bits and always content 2 chunks */
                chunksLength = (chunksLength * 2);
#ifdef DEBUG
                printk("lrleStartSeq: %x lrleEndSeq: %x lrleThinningValue: %x chunksLength: %d\n\r", pXrStat->lrleStartSeq, pXrStat->lrleEndSeq, pXrStat->lrleThinningValue, chunksLength);
                printk("lrleInfoLength: %d lrleInfo: %p\n\r", pXrStat->lrleInfoLength, pXrStat->lrleInfo);
#endif


                if (pXrStat->lrleInfoLength != chunksLength) {
                    if (pXrStat->lrleInfo != NULL) {
                        kfree(pXrStat->lrleInfo);
                        pXrStat->lrleInfo       = NULL;
                        pXrStat->lrleInfoLength = 0;
                    }
                }
                /* Only allocate if there are Bit Vector Informatie */
                if (chunksLength > RTCPXR_MIN_RLE_CHUNKS_SIZE) {
                    if (pXrStat->lrleInfo == NULL) {
                        pXrStat->lrleInfo = (unsigned short *)kmalloc((chunksLength * sizeof(unsigned short)), GFP_KERNEL);
                        if (pXrStat->lrleInfo == NULL) {
                            MMPBX_TRC_ERROR("No more resources, unable to malloc lrleInfo placeholder\n\r");
                            break;
                        }
                    }
                    pXrStat->lrleInfoLength = chunksLength;
                    memset(pXrStat->lrleInfo, 0, pXrStat->lrleInfoLength * sizeof(unsigned short));
#ifdef DEBUG
                    printk("Reallocate/Reuse buffer lrleInfoLength: %d lrleInfo: %p\n\r", pXrStat->lrleInfoLength, pXrStat->lrleInfo);
#endif
                }

                pReport = pXrStat->lrleInfo;
                pInfo   = &pBt->runLengthChunk[0];
                /* Each chunk info is 16 bits */
                for (chunk = 0; chunk < chunksLength; chunk++) {
#ifdef DEBUG
                    printk("chunk: %d pReport: %p *pInfo: %x sizeof(pInfo->chunkInfo): %d\n\r", chunk, pReport, *pInfo, sizeof(pInfo->chunkInfo));
#endif
                    /* Check for NULL Terminating Chunk */
                    if (pInfo->chunkInfo.nullChunk != 0) {
                        if (pInfo->chunkInfo.runLengthChunk.chunkType == RTCPXR_RUNLENGTH_CHUNK_TYPE) {
                            pXrStat->lrleRunType    = pInfo->chunkInfo.runLengthChunk.runType;    /* Run for 0 or 1 */
                            pXrStat->lrleRunLength  = pInfo->chunkInfo.runLengthChunk.runLength;  /* Run Length */
#ifdef DEBUG
                            printk("chunk: %d pInfo->chunkInfo.runLengthChunk.chunkType: %d lrleRunType: %d lrleRunLength: %d\n\r", chunk, pInfo->chunkInfo.runLengthChunk.chunkType, pXrStat->lrleRunType, pXrStat->lrleRunLength);
#endif
                        }
                        if (pInfo->chunkInfo.bitVectorChunk.chunkType != RTCPXR_BITVECTOR_CHUNK_TYPE) {
                            if (pReport != NULL) {
                                *pReport++ = ntohs(pInfo->chunkInfo.bitVectorChunk.bitVector);
                            }
#ifdef DEBUG
                            printk("chunk: %s pReport: %p *pReport: %x pInfo: %p *pInfo: %x\n\r", chunk, (unsigned int)pReport, *pReport, (unsigned int)pInfo, *pInfo);
#endif
                        }
                    }
                    pInfo++;
                }
            }
            break;

            case 2: /* Duplicate RLE (Report Length Encoding) */
            {
                RLE_CHUNK_INFO  *pInfo        = NULL;
                unsigned short  *pReport      = NULL;
                int             chunksLength  = 0;
                int             chunk         = 0;
                RTCPXR_BT1      *pBt          = NULL;
                networkToHostRTCPXR_BT1(&pXr->blockTypes.bt2, &pBt);
                if (pBt == NULL) {
                    MMPBX_TRC_ERROR("Invalid/NULL Block buffer \n\r");
                    break;
                }

                if (pXr->common.blockLength < RTCPXR_MIN_RLE_BLOCKTYPE_SIZE) {
                    /* Block Type 2 is too small, incorrect info */
                    break;
                }

                pXrStat->rtcpXrUpdated |= (1 << pXr->common.blockTypeId);

                pXrStat->drleStartSeq       = pBt->beginSeq;                                    /* Start of RTP transmission */
                pXrStat->drleEndSeq         = pBt->endSeq;                                      /* The last RTP transmission */
                pXrStat->drleThinningValue  = pXr->common.typeSpecific.thinningHeader.thinning; /* Thinning factor */
                chunksLength = pXr->common.blockLength - 2 /* SSRC & Beg+End Seq */;
                /* chunk blockLength is in 32 bits and always content 2 chunks */
                chunksLength = (chunksLength * 2);

#ifdef DEBUG
                printk("drleStartSeq: %x drleEndSeq: %x drleThinningValue: %x chunksLength: %d\n\r", pXrStat->drleStartSeq, pXrStat->drleEndSeq, pXrStat->drleThinningValue, chunksLength);
                printk("drleInfoLength: %d drleInfo: %p\n\r", pXrStat->drleInfoLength, pXrStat->drleInfo);
#endif
                if (pXrStat->drleInfoLength != chunksLength) {
                    if (pXrStat->drleInfo != NULL) {
                        kfree(pXrStat->drleInfo);
                        pXrStat->drleInfo       = NULL;
                        pXrStat->drleInfoLength = 0;
                    }
                }
                /* Only allocate if there are Bit Vector Informatie */
                if (chunksLength > RTCPXR_MIN_RLE_CHUNKS_SIZE) {
                    if (pXrStat->drleInfo == NULL) {
                        pXrStat->drleInfo = (unsigned short *)kmalloc((chunksLength * sizeof(int)), GFP_KERNEL);
                        if (pXrStat->drleInfo == NULL) {
                            MMPBX_TRC_ERROR("No more resources, unable to malloc drleInfo placeholder\n\r");
                            break;
                        }
                    }
                    pXrStat->drleInfoLength = chunksLength;
                    memset(pXrStat->drleInfo, 0, pXrStat->drleInfoLength * sizeof(unsigned short));
#ifdef DEBUG
                    printk("drleInfoLength: %d lrleInfo: %p\n\r", pXrStat->drleInfoLength, pXrStat->drleInfo);
#endif
                }

                pReport = pXrStat->drleInfo;
                pInfo   = &pBt->runLengthChunk[0];
                /* Each chunk info is 16 bits */
                for (chunk = 0; chunk < chunksLength; chunk++) {
#ifdef DEBUG
                    printk("chunk: %d pReport: %p *pInfo: %x sizeof(pInfo->chunkInfo): %d\n\r", chunk, (unsigned int)pReport, *pInfo, sizeof(pInfo->chunkInfo));
#endif
                    /* Check for NULL Terminating Chunk */
                    if (pInfo->chunkInfo.nullChunk != 0) {
                        if (pInfo->chunkInfo.runLengthChunk.chunkType == RTCPXR_RUNLENGTH_CHUNK_TYPE) {
                            /* A Run Length Info */
                            pXrStat->drleRunType    = pInfo->chunkInfo.runLengthChunk.runType;        /* Run for 0 or 1 */
                            pXrStat->drleRunLength  = pInfo->chunkInfo.runLengthChunk.runLength;      /* Run Length */
                        }
                        if (pInfo->chunkInfo.runLengthChunk.chunkType == RTCPXR_BITVECTOR_CHUNK_TYPE) {      /* Bit Vector Chunk */
                            *pReport++ = ntohs(pInfo->chunkInfo.bitVectorChunk.bitVector);
                        }
#ifdef DEBUG
                        printk("chunk: %d pInfo->chunkInfo.runLengthChunk.chunkType: %d drleRunType: %d drleRunLength: %d\n\r", chunk, pInfo->chunkInfo.runLengthChunk.chunkType, pXrStat->drleRunType, pXrStat->drleRunLength);
#endif
                    }
                    pInfo++;
                }
            }
            break;

            case 3: /* Packet Receipt Times Report */
            {
                unsigned long *pInfo      = NULL;
                unsigned long *pReport    = NULL;
                int           infoLength  = 0;
                int           i;
                RTCPXR_BT3    *pBt = NULL;
                networkToHostRTCPXR_BT3(&pXr->blockTypes.bt3, &pBt);
                if (pBt == NULL) {
                    MMPBX_TRC_ERROR("Invalid/NULL Block buffer \n\r");
                    break;
                }

                if (pXr->common.blockLength < RTCPXR_MIN_PACKET_RECEIPT_TIME_REPORT_SIZE) {
                    MMPBX_TRC_ERROR("Lengh is less than the Minimum Packet Receipt Report size: %d\n\r", pXr->common.blockLength);
                    break;
                }

                pXrStat->rtcpXrUpdated |= (1 << pXr->common.blockTypeId);

                infoLength = pXr->common.blockLength - 2;                                         /* minus SSRC and begin & End Seq */
                pXrStat->packetThinningValue  = pXr->common.typeSpecific.thinningHeader.thinning; /* Thinning factor */
                pXrStat->packetStartSeq       = pBt->beginSeq;                                    /* Start of RTP transmission */
                pXrStat->packetEndSeq         = pBt->endSeq;                                      /* The last RTP transmission */
#ifdef DEBUG
                printk("infoLength: %d packetThinningValue: %d packetStartSeq: %d packetEndSeq: %d\n\r", infoLength, pXrStat->packetThinningValue, pXrStat->packetStartSeq, pXrStat->packetEndSeq);
                printk("pXrStat->packetsReceiptInfoLength: %d pXrStat->packetsReceiptInfo: %p\n\r", pXrStat->packetReceiptInfoLength, pXrStat->packetsReceiptInfo);
#endif
                if (infoLength > 0) {
                    if (pXrStat->packetReceiptInfoLength != infoLength) {
                        if (pXrStat->packetsReceiptInfo != NULL) {
                            kfree(pXrStat->packetsReceiptInfo);
                            pXrStat->packetsReceiptInfo       = NULL;
                            pXrStat->packetReceiptInfoLength  = 0;
                        }
                    }
                    if (pXrStat->packetsReceiptInfo == NULL) {
                        pXrStat->packetsReceiptInfo = kmalloc((infoLength * sizeof(int)), GFP_KERNEL);
                        if (pXrStat->packetsReceiptInfo == NULL) {
                            MMPBX_TRC_ERROR("No more resources, unable to malloc packetsReceiptInfo placeholder\n\r");
                            break;
                        }
                        pXrStat->packetReceiptInfoLength = infoLength;
                    }
#ifdef DEBUG
                    printk("Reallocated/Reuse Buffer infoLength: %d pXrStat->packetReceiptInfoLength: %d pXrStat->packetsReceiptInfo: %p\n\r", infoLength, pXrStat->packetReceiptInfoLength, pXrStat->packetsReceiptInfo);
#endif
                    memset(pXrStat->packetsReceiptInfo, 0, sizeof(pXrStat->packetReceiptInfoLength * sizeof(int)));

                    for (i = 0, pReport = pXrStat->packetsReceiptInfo, pInfo = &pBt->rxTime[0];
                         i < infoLength; i++, pInfo++, pReport++) {
                        *pReport = *pInfo;
#ifdef DEBUG
                        printk("Count: %d pReport: %p *pReport: %x pInfo: %p *pInfo: %x \n\r", i, pReport, *pReport, pInfo, *pInfo);
#endif
                    }
                }
            }
            break;

            case 4: /* Receiver Reference Time Report */
            {
                RTCPXR_BT4 *pBt = NULL;
                networkToHostRTCPXR_BT4(&pXr->blockTypes.bt4, &pBt);
                if (pBt == NULL) {
                    MMPBX_TRC_ERROR("Invalid/NULL Block buffer \n\r");
                    break;
                }
                pXrStat->rtcpXrUpdated |= (1 << pXr->common.blockTypeId);

                pXrStat->receiptTimeReferenceMsw  = pBt->msw;
                pXrStat->receiptTimeReferenceLsw  = pBt->lsw;
#ifdef DEBUG
                printk("receiptTimeRefMsw: %x receiptTimeRefLsw: %x\n\r", pXrStat->receiptTimeReferenceMsw, pXrStat->receiptTimeReferenceLsw);
#endif
            }
            break;

            case 5: /* DLRR Report */
            {
                RTCPXR_BT5      *pBt            = &pXr->blockTypes.bt5;
                DLRR_SUB_BLOCK  *pSubBlock      = NULL;
                DLRR_SUB_BLOCK  *pInfo          = NULL;
                int             numOfSubBlocks  = 0;
                int             i = 0;

                pXrStat->rtcpXrUpdated |= (1 << pXr->common.blockTypeId);

                numOfSubBlocks = (pXr->common.blockLength * sizeof(int)) / sizeof(DLRR_SUB_BLOCK);
#ifdef DEBUG
                printk("numOfSubBlocks: %d sizeof(DLRR_SUB_BLOCK): %d\n\r", numOfSubBlocks, sizeof(DLRR_SUB_BLOCK));
                printk("dlrrSubBlock: %p dlrrNummerOfBlocks: %d\n\r", pXrStat->dlrrSubBlock, pXrStat->dlrrNummerOfBlocks);
#endif
                if (pXrStat->dlrrNummerOfBlocks != numOfSubBlocks) {
                    if (pXrStat->dlrrSubBlock != NULL) {
                        kfree(pXrStat->dlrrSubBlock);
                        pXrStat->dlrrNummerOfBlocks = 0;
                        pXrStat->dlrrSubBlock       = NULL;
                    }
                }
                if (pXrStat->dlrrSubBlock == NULL) {
                    pXrStat->dlrrSubBlock = kmalloc((numOfSubBlocks * sizeof(DLRR_SUB_BLOCK)), GFP_KERNEL);
                    if (pXrStat->dlrrSubBlock == NULL) {
                        MMPBX_TRC_ERROR("No more resources, unable to malloc dlrrSubBlockis placeholder\n\r");
                        break;
                    }
                    pXrStat->dlrrNummerOfBlocks = numOfSubBlocks;
                }
#ifdef DEBUG
                printk("Reallocating/Reuse Buffer dlrrSubBlock: %p dlrrNummerOfBlocks: %x\n\r", pXrStat->dlrrSubBlock, pXrStat->dlrrNummerOfBlocks);
#endif

                for (i = 0, pSubBlock = pXrStat->dlrrSubBlock, pInfo = &pBt->subBlocks[0];
                     i < numOfSubBlocks; i++, pSubBlock++, pInfo++) {
#ifdef __LITTLE_ENDIAN
                    pSubBlock->ssrc = ntohl(pInfo->ssrc);
                    pSubBlock->lrr  = ntohl(pInfo->lrr);
                    pSubBlock->dlrr = ntohl(pInfo->dlrr);
#else
                    pSubBlock->ssrc = pInfo->ssrc;
                    pSubBlock->lrr  = pInfo->lrr;
                    pSubBlock->dlrr = pInfo->dlrr;
#endif
#ifdef DEBUG
                    printk("Count: %d pSubBlock: %p pInfo: %p ssrc: %x lrr: %x dlrr: %x\n\r", i, pSubBlock, pInfo, pSubBlock->ssrc, pSubBlock->lrr, pSubBlock->dlrr);
#endif
                }
            }
            break;

            case 6:
            {
                RTCPXR_BT6 *pBt = NULL;
                networkToHostRTCPXR_BT6(&pXr->blockTypes.bt6, &pBt);
                if (pBt == NULL) {
                    MMPBX_TRC_ERROR("Invalid/NULL Block buffer \n\r");
                    break;
                }

                pXrStat->rtcpXrUpdated |= (1 << pXr->common.blockTypeId);
#ifdef DEBUG
                printk("pXr->common.typeSpecific.reportHeader: %x sizeof(pXr->common.typeSpecific.reportHeader): %d \n\r", pXr->common.typeSpecific.reportHeader, sizeof(pXr->common.typeSpecific.reportHeader));
#endif

                if (pXr->common.typeSpecific.reportHeader.loss != 0) {
                    pXrStat->sumLostPackets += pBt->lostPackets;
#ifdef DEBUG
                    printk("lostPackets: increased to %d by %d\n\r", pXrStat->sumLostPackets, pBt->lostPackets);
#endif
                }
                if (pXr->common.typeSpecific.reportHeader.duplicate != 0) {
                    pXrStat->duplicatedPackets = pBt->duplicatedPackets;
#ifdef DEBUG
                    printk("dupPackets: %d\n\r", pXrStat->duplicatedPackets);
#endif
                }
                if (pXr->common.typeSpecific.reportHeader.jitter != 0) {
                    pXrStat->minJitter  = pBt->minJitter;
                    pXrStat->meanJitter = pBt->meanJitter;
                    pXrStat->maxJitter  = pBt->maxJitter;
                    pXrStat->devJitter  = pBt->devJitter;
#ifdef DEBUG
                    printk("minJitter: %d meanJitter: %d maxJitter: %d devJitter: %d\n\r", pXrStat->minJitter, pXrStat->meanJitter, pXrStat->maxJitter, pXrStat->devJitter);
#endif
                }
                if (pXr->common.typeSpecific.reportHeader.toh != 0) {
                    /* 1 = IPV4 TTL Value; 2 = IPV6 TTL Value */
                    pXrStat->ipValid  = pXr->common.typeSpecific.reportHeader.toh ? RTCPXR_TTL_IPV4 : RTCPXR_TTL_IPV6;
                    pXrStat->minToH   = pBt->minToH;
                    pXrStat->maxToH   = pBt->maxToH;
                    pXrStat->meanToH  = pBt->meanToH;
                    pXrStat->devToH   = pBt->devToH;
#ifdef DEBUG
                    printk("pValid: %d minToH: %d maxToH: %d meanToH: %d devToH: %d\n\r",
                           pXrStat->ipValid, pXrStat->minToH, pXrStat->maxToH, pXrStat->meanToH, pXrStat->devToH);
#endif
                }
            }
            break;

            case 7:
            {
                uint8_t       level;
                RTCPXRSTAT    *pOtherXrStat = !bIncoming ? &mmStat->rtpHandle.remoteRtcpXrStat : &mmStat->rtpHandle.localRtcpXrStat;
                unsigned long oneWayDelay;
                RTCPXR_BT7    *pBt = NULL;
                networkToHostRTCPXR_BT7(&pXr->blockTypes.bt7, &pBt);
                if (pBt == NULL) {
                    MMPBX_TRC_ERROR("Invalid/NULL Block buffer \n\r");
                    break;
                }


                pXrStat->rtcpXrUpdated |= (1 << pXr->common.blockTypeId);

                pXrStat->lossRate       = pBt->lossRate;
                pXrStat->discardedRate  = pBt->discardedRate;
                pXrStat->burstDensity   = pBt->burstDensity;
                pXrStat->gapDensity     = pBt->gapDensity;
                pXrStat->burstDuration  = pBt->burstDuration;
                pXrStat->gapDuration    = pBt->gapDuration;

                if (pBt->roundTripDelay != 0) {
                    /* It's observed that this value is frequently set as 0 in BRCM's RTCP-XR frame....*/
                    if (pXrStat->roundTripDelay < pBt->roundTripDelay) {
                        pXrStat->maxRoundTripDelay = pBt->roundTripDelay;
                    }
                    pXrStat->roundTripDelay = pBt->roundTripDelay;
                }
                pXrStat->sumRoundTripDelay    += pXrStat->roundTripDelay;
                pXrStat->sumSqrRoundTripDelay += pXrStat->roundTripDelay * pXrStat->roundTripDelay;

                pXrStat->endSystemDelay = pBt->endSystemDelay;

                oneWayDelay = (pXrStat->roundTripDelay + pBt->endSystemDelay + pOtherXrStat->endSystemDelay) / 2;

                if (pXrStat->maxOneWayDelay < oneWayDelay) {
                    pXrStat->maxOneWayDelay = oneWayDelay;
                }
                pXrStat->sumOneWayDelay     += oneWayDelay;
                pXrStat->sumSqrOneWayDelay  += oneWayDelay * oneWayDelay;

                /* Convert from negatief to positief in 2-complement */
                level = (pBt->signalLevel - 1);
                pXrStat->signalLevel = ~level;
                level = (pBt->noiseLevel - 1);
                pXrStat->noiseLevel       = ~level;
                pXrStat->RERL             = pBt->rerl;
                pXrStat->RFactor          = pBt->RFactor;
                pXrStat->externalRFactor  = pBt->externalRFactor;
                pXrStat->mosLQ            = pBt->mosLQ;
                pXrStat->mosCQ            = pBt->mosCQ;
                /* Get the RX Config paramaters */

                pXrStat->rxConfigPLC = RTCPXR_GET_RXCONFIG_PLC(pBt->rxconfig);

                pXrStat->rxConfigJBA = RTCPXR_GET_RXCONFIG_JBA(pBt->rxconfig);

                pXrStat->rxConfigJBRate = RTCPXR_GET_RXCONFIG_JBA_RATE(pBt->rxconfig);

                pXrStat->jitterBufferNominal    = pBt->jitterBufferNominal;
                pXrStat->jitterBufferMax        = pBt->jitterBufferMax;
                pXrStat->jitterBufferWorstCase  = pBt->jitterBufferWorstCase;

#ifdef DEBUG
                printk("lossRate: %d discardedRate: %d burstDensity: %d gapDensity: %d burstDuration: %d gapDuration: %d roundTripDelay: %d\n\r",
                       pXrStat->lossRate, pXrStat->discardedRate, pXrStat->burstDensity, pXrStat->gapDensity, pXrStat->burstDuration, pXrStat->gapDuration, pXrStat->roundTripDelay);
                printk("endSystemDelay: %d signalLevel: %d noiseLevel: %d RERL: %d RFactor: %d externalRFactor: %d mosLQ: %d mosCQ: %d\n\r",
                       pXrStat->endSystemDelay, pXrStat->signalLevel, pXrStat->noiseLevel, pXrStat->RERL, pXrStat->RFactor, pXrStat->externalRFactor, pXrStat->mosLQ, pXrStat->mosCQ);
                printk("pBt->rxconfig: %x sizeof(pBt->rx): %d\n\r", pBt->rxconfig, sizeof(pBt->rxconfig));
                printk("rxConfigPlc: %d rxConfigJba: %d rxConfigJbRate: %d\n\r", pXrStat->rxConfigPLC, pXrStat->rxConfigJBA, pXrStat->rxConfigJBRate);
                printk("jitterBufferNominal: %d jitterBufferMax: %d jitterBufferWorstCase: %d\n\r", pXrStat->jitterBufferNominal, pXrStat->jitterBufferMax, pXrStat->jitterBufferWorstCase);
#endif
            }
            break;

            default:
                MMPBX_TRC_DEBUG("Unknown Block Type: %d\n\r", pXr->common.blockTypeId);
                break;
        }
        totBlocksLen -= blockLen;
        networkToHostRTCPXR(((uint8_t *)pXr + blockLen), &pXr);   /* Point to the first Block Type */
        if (pXr == NULL) {
            MMPBX_TRC_ERROR("Invalid/NULL XR packet\n\r");
            break;
        }
    }
    MMPBX_TRC_DEBUG("Updated Blocks: %x\n\r", pXrStat->rtcpXrUpdated);
}

/*
 *****************************************************************************
 ** FUNCTION:   rtcpProcessBye
 **
 ** PURPOSE:    This function processes a received/sent RTCP BYE packet
 **
 ** PARAMETERS: handle  - [IN] Handle to the RTP session control block
 **             rtcpbuf   - [IN] Ptr to the RTCP packet
 **
 ** RETURNS:    None
 *****************************************************************************
 */
static void rtcpProcessBye(int handle, RTCPPACKET *rtcpbuf, int bIncoming)
{
    MmConnRtcpStatsSessionHndl session = sessions[handle];

    MMPBX_TRC_DEBUG("Processing BYE Packet Type of Rtcp: %p Incoming: %s\n\r", rtcpbuf, bIncoming ? "true" : "false");

    session->end = get_jiffies_64();
}

/*
 *****************************************************************************
 ** FUNCTION:   rtcpRecordRRInfo
 **
 ** PURPOSE:    This function process received receiver report
 **
 ** PARAMETERS: handle  - [IN] Handle to the RTP session control block
 **             count   - [IN] Number of RR
 **             rrlst   - [IN] List of RRs
 **             bIncoming - [IN] Indicates whether it is an incoming RTCP or
 **                         outgoing RTCP
 **
 ** RETURNS:    None
 **
 ** NOTE:       Only NTP timestamp is needed to calculate round-trip statistics
 **             The control structure indexed by the handle is updated
 *****************************************************************************
 */
static void rtcpRecordRRInfo(int handle, int count, RTCPRR *rrlst, int bIncoming)
{
    int                         i;
    MmConnRtcpStatsSessionHndl  mmStat  = sessions[handle];
    RTPHANDLE                   *rtpp   = NULL;
    RTCPRR                      *rr     = NULL;

    MMPBX_TRC_DEBUG("Recording RR Packet Type of Rtcp: %p count: %d Incoming: %s\n\r", rrlst, count, bIncoming ? "true" : "false");

    rtpp = &mmStat->rtpHandle;

    /* Iterate through the RRs to find our own RR */
    for (i = 0; i < count; i++) {
        rr = &rrlst[i];
        if (bIncoming == TRUE) {
            /* Found the RR to the local RTP media */
            if (rr->ssrc == rtpp->ssrc) {
                /* Update far-end pkt lost delta */
                rtpp->rtcpStat.dwRemotePacketLost = rr->lost - rtpp->rtcpStat.dwPrevCumRemotePacketLost;
                /* Update far-end pkt lost cumulative */
                rtpp->rtcpStat.dwPrevCumRemotePacketLost = rr->lost;
                /* Update far-end jitter */
                /* Convert jitter from 8kHz samples to msec */
                rtpp->rtcpStat.dwRemoteJitter = (((unsigned long)rr->jitter) >> 3);

                MMPBX_TRC_DEBUG("REMOTELOST %d REMOTEJITTER %d FRACTION %d\n\r", (int)(rr->lost), (int)(rr->jitter), (int) rr->fraction);

                /* Calculation for local packet loss rate */
                if (rtpp->rtcpStat.bFirstRtcpReceived == TRUE) {
                    if (rtpp->rtcpStat.dwPrevRemoteLastSeqNum != rr->last_seq) {
                        rtpp->rtcpStat.wRemotePktLossRate     = 100 * (rr->lost - rtpp->rtcpStat.dwPrevRemoteNumLost) / (rr->last_seq - rtpp->rtcpStat.dwPrevRemoteLastSeqNum);
                        rtpp->rtcpStat.bCalRemotePktLossRate  = TRUE;
                    }
                }
                else {
                    rtpp->rtcpStat.bFirstRtcpReceived = TRUE;
                }

                rtpp->rtcpStat.dwPrevRemoteNumLost    = rr->lost;
                rtpp->rtcpStat.dwPrevRemoteLastSeqNum = rr->last_seq;
                break;
            }
        }
        else {
            if (rr->ssrc == rtpp->essrc) {
                /* Calculation for remote packet loss */
                if (rtpp->rtcpStat.bFirstRtcpSent == TRUE) {
                    if (rtpp->rtcpStat.dwPrevLocalLastSeqNum != rr->last_seq) {
                        rtpp->rtcpStat.wLocalPktLossRate    = 100 * (rr->lost - rtpp->rtcpStat.dwPrevLocalNumLost) / (rr->last_seq - rtpp->rtcpStat.dwPrevLocalLastSeqNum);
                        rtpp->rtcpStat.bCalLocalPktLossRate = TRUE;
                    }
                }
                else {
                    rtpp->rtcpStat.bFirstRtcpSent = TRUE;
                }

                rtpp->rtcpStat.dwPrevLocalNumLost     = rr->lost;
                rtpp->rtcpStat.dwPrevLocalLastSeqNum  = rr->last_seq;

                MMPBX_TRC_DEBUG("REMOTELOST %d LAST-SEQ %d FRACTION %d\n\r", (int)(rr->lost), (int)(rr->last_seq), (int) rr->fraction);
                break;
            }
        }
    }
}

/*
 * Clear the Content of the RTCP-XR Statistics
 */
static void clearRtcpCnxStats(RTCPXRSTAT *pXrStat)
{
    if (pXrStat == NULL) {
        return;
    }

    if (pXrStat->lrleInfo != NULL) {
        kfree(pXrStat->lrleInfo);
    }

    if (pXrStat->drleInfo != NULL) {
        kfree(pXrStat->drleInfo);
    }

    if (pXrStat->packetsReceiptInfo != NULL) {
        kfree(pXrStat->packetsReceiptInfo);
    }

    if (pXrStat->dlrrSubBlock != NULL) {
        kfree(pXrStat->dlrrSubBlock);
    }
    memset(pXrStat, 0, sizeof(RTCPXRSTAT));
}

static void rtcpSaveFrame(int handle, uint8_t *buff, int len, int bIncoming)
{
    MmConnRtcpStatsSessionHndl  mmStat      = sessions[handle];
    RTPHANDLE                   *rtpHandle  = &mmStat->rtpHandle;
    RTCPBUFF                    *pBuf       = NULL;
    int                         nextIdx     = 0;

    MMPBX_TRC_DEBUG("Entering...\n\r");

    /* Store the last RTCP frames */
#if MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES > 0
    read_lock(&_rtcpBufferLock);

    if (bIncoming) {
        nextIdx = (rtpHandle->lastRxRtcpBufIdx + 1) % MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES;
        pBuf    = &rtpHandle->rxRtcpBuf[nextIdx];
        rtpHandle->lastRxRtcpBufIdx = nextIdx;
    }
    else {
        nextIdx = (rtpHandle->lastTxRtcpBufIdx + 1) % MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES;
        pBuf    = &rtpHandle->txRtcpBuf[nextIdx];
        rtpHandle->lastTxRtcpBufIdx = nextIdx;
    }

#ifdef DEBUG
    printk("Incoming: %s len: %d nextIdx: %d pBuf->len: %d pBuf->buf: %p\n\r", bIncoming ? "true" : "false", len, nextIdx, pBuf->len, pBuf->buf);
#endif
    if (len != pBuf->len) {
        if (pBuf->buf != NULL) {
            kfree(pBuf->buf);
            pBuf->buf = NULL;
            pBuf->len = 0;
        }
    }
    if (pBuf->buf == NULL) {
        pBuf->buf = kmalloc(len, GFP_KERNEL);
        if (pBuf->buf == NULL) {
            MMPBX_TRC_DEBUG("Unable to allocate memory to store Last RTCP packet\n\r");
            read_unlock(&_rtcpBufferLock);
            return;
        }
        pBuf->len = len;
    }
#ifdef DEBUG
    printk("Reallocating/Reuse pBuf->buf: %p pBuf->len: %d\n\r", pBuf->buf, pBuf->len);
#endif
    memcpy(pBuf->buf, buff, len);       /* Copy the entire RTCP buffer */
    read_unlock(&_rtcpBufferLock);
#endif
    MMPBX_TRC_DEBUG("Finish Saving RTCP frame...\n\r");
}

/*
 *****************************************************************************
 ** FUNCTION:   rtcpUpdateInfo
 **
 ** PURPOSE:    This function prepares report statistics to the RTP client
 **
 ** PARAMETERS: handle       - [IN] RTP stack handle
 **             epRtpStats   - [IN] Endpoint statistics from RTCP DSP
 **             rtcpCnxStats - [OUT] statistics (see MmConnRtcpCnxStats)
 **
 ** RETURNS:    None
 *****************************************************************************
 */
static void rtcpUpdateInfo(int handle, MmConnDspRtpStatsParmHndl epRtpStats, MmConnRtcpCnxStats *rtcpCnxStats)
{
    MmConnRtcpStatsSessionHndl  mmStat    = sessions[handle];
    RTCPSTAT                    *rtcpStat = &mmStat->rtpHandle.rtcpStat;

    MMPBX_TRC_DEBUG("Handle %d\n", handle);

    if (rtcpCnxStats == NULL) {
        return;
    }

    rtcpStat->remoteAverageJitter = (rtcpStat->remoteAverageJitter + rtcpStat->dwRemoteJitter) / 2;
    if (epRtpStats == NULL) {
        rtcpCnxStats->localAverageJitter = rtcpStat->localAverageJitter;
    }
    else {
        /* Update average values */
        rtcpStat->localAverageJitter  = (rtcpStat->localAverageJitter + epRtpStats->jitter) / 2;
        rtcpStat->localAverageLatency = (rtcpStat->localAverageLatency + epRtpStats->latency) / 2;

        /*
         * Update the Reporting info
         */
        /* Save the connection statics from the local endpoint */
        rtcpCnxStats->localPktSent      = epRtpStats->txpkts;
        rtcpCnxStats->localOctetSent    = epRtpStats->txbytes;
        rtcpCnxStats->localPktRecvd     = epRtpStats->rxpkts;
        rtcpCnxStats->localOctetRecvd   = epRtpStats->rxbytes;
        rtcpCnxStats->localPktLost      = epRtpStats->lost;
        rtcpCnxStats->localPktDiscarded = epRtpStats->discarded;
        /* RTCP Statistics */
        rtcpCnxStats->rtcpPktsReceived    = epRtpStats->rxRtcpPkts;
        rtcpCnxStats->rtcpXrPktsReceived  = epRtpStats->rxRtcpXrPkts;

        rtcpCnxStats->localJitter                 = epRtpStats->jitter;
        rtcpCnxStats->localMaxJitter              = epRtpStats->peakJitter;
        rtcpCnxStats->localJitterBufferOverruns   = epRtpStats->jitterBufferOverruns;
        rtcpCnxStats->localJitterBufferUnderruns  = epRtpStats->jitterBufferUnderruns;
        rtcpCnxStats->localLatency                = epRtpStats->latency;
        rtcpCnxStats->localAverageLatency         = rtcpStat->localAverageLatency;

        rtcpCnxStats->localJitterBuffer     = epRtpStats->jitterBufferMin;
        rtcpCnxStats->localMaxJitterBuffer  = epRtpStats->jitterBufferMax;
        rtcpCnxStats->localAverageJitter    = rtcpStat->localAverageJitter;

        rtcpCnxStats->localMosLQ  = epRtpStats->mosLQ;
        rtcpCnxStats->localMosCQ  = epRtpStats->mosCQ;
    }

    if (rtcpStat->bCalLocalPktLossRate == TRUE) {
        rtcpCnxStats->localPktLostRate  = rtcpStat->wLocalPktLossRate;
        rtcpStat->bCalLocalPktLossRate  = FALSE;
        rtcpStat->wLocalPktLossRate     = 0;
    }

    /* Save the connection statics from the remote endpoint */
    rtcpCnxStats->remotePktSent       = rtcpStat->dwPrevCumRemotePacketSent;
    rtcpCnxStats->remoteOctetSent     = rtcpStat->dwPrevCumRemoteOctetSent;
    rtcpCnxStats->remotePktLost       = rtcpStat->dwPrevCumRemotePacketLost;
    rtcpCnxStats->remoteJitter        = rtcpStat->dwRemoteJitter;
    rtcpCnxStats->remoteAverageJitter = rtcpStat->remoteAverageJitter;

    if (rtcpStat->bCalRemotePktLossRate == TRUE) {
        rtcpCnxStats->remotePktLostRate = rtcpStat->wRemotePktLossRate;
        rtcpStat->bCalRemotePktLossRate = FALSE;
        rtcpStat->wRemotePktLossRate    = 0;
    }
    rtcpCnxStats->rtcpXrPktsSent  = rtcpStat->rtcpXrPktsSent;
    rtcpCnxStats->rtcpPktsSent    = rtcpStat->rtcpPktsSent;

    /* extension for AT&T*/
    rtcpCnxStats->localSumFractionLoss      = rtcpStat->localSumFractionLoss;
    rtcpCnxStats->localSumSqrFractionLoss   = rtcpStat->localSumSqrFractionLoss;
    rtcpCnxStats->remoteSumFractionLoss     = rtcpStat->remoteSumFractionLoss;
    rtcpCnxStats->remoteSumSqrFractionLoss  = rtcpStat->remoteSumSqrFractionLoss;
    rtcpCnxStats->remoteSumJitter           = rtcpStat->remoteSumJitter;
    rtcpCnxStats->remoteSumSqrJitter        = rtcpStat->remoteSumSqrJitter;
    rtcpCnxStats->localSumJitter            = rtcpStat->localSumJitter;
    rtcpCnxStats->localSumSqrJitter         = rtcpStat->localSumSqrJitter;
}

/*
 *****************************************************************************
 ** FUNCTION:   rtcpXrUpdateInfo
 **
 ** PURPOSE:    This function updates RTCP XR connection report statistics
 **
 ** PARAMETERS: handle       - [IN]  RTP session handle
 **             rtcpCnxStats - [OUT] statistics (see MmConnRtcpCnxStats)
 **
 ** RETURNS:    None
 **
 ** NOTE:       None
 *****************************************************************************
 */
static void rtcpXrUpdateInfo(int handle, MmConnRtcpCnxStats *rtcpCnxStats)
{
    MmConnRtcpStatsSessionHndl  mmStat          = sessions[handle];
    RTCPXRSTAT                  *pXrLocalStat   = NULL;
    RTCPXRSTAT                  *pXrRemoteStat  = NULL;

    MMPBX_TRC_DEBUG("Handle %d\n", handle);

    pXrLocalStat = &mmStat->rtpHandle.localRtcpXrStat;    /* Local RTCP-XR statistics */

    /* If RTCP-XR packets are captured */
    if (pXrLocalStat->rtcpXrUpdated != 0) {
        /* Populate the Local connection statistics */
        if (RTCPXR_BT_UPDATED(pXrLocalStat->rtcpXrUpdated, 6)) {
            rtcpCnxStats->localJitter         = pXrLocalStat->meanJitter;
            rtcpCnxStats->localMinJitter      = pXrLocalStat->minJitter;
            rtcpCnxStats->localMaxJitter      = pXrLocalStat->maxJitter;
            rtcpCnxStats->localDevJitter      = pXrLocalStat->devJitter;
            rtcpCnxStats->localAverageJitter  = pXrLocalStat->meanJitter;
            rtcpCnxStats->localPktLost        = pXrLocalStat->sumLostPackets;
        }
        if (RTCPXR_BT_UPDATED(pXrLocalStat->rtcpXrUpdated, 7)) {
            rtcpCnxStats->localPktLostRate      = pXrLocalStat->lossRate;
            rtcpCnxStats->localPktDiscardedRate = pXrLocalStat->discardedRate;

            rtcpCnxStats->localLatency    = pXrLocalStat->roundTripDelay;
            rtcpCnxStats->locaMaxLatency  = pXrLocalStat->maxRoundTripDelay;

            rtcpCnxStats->localAverageJitter  = (rtcpCnxStats->localAverageJitter + rtcpCnxStats->localJitter) / 2;
            rtcpCnxStats->localAverageLatency = (rtcpCnxStats->localAverageLatency + rtcpCnxStats->localLatency) / 2;

            rtcpCnxStats->localSignalLevel      = pXrLocalStat->signalLevel;
            rtcpCnxStats->localNoiseLevel       = pXrLocalStat->noiseLevel;
            rtcpCnxStats->localRERL             = pXrLocalStat->RERL;
            rtcpCnxStats->localRFactor          = pXrLocalStat->RFactor;
            rtcpCnxStats->localExternalRFactor  = pXrLocalStat->externalRFactor;
            rtcpCnxStats->localMosLQ            = pXrLocalStat->mosLQ;
            rtcpCnxStats->localMosCQ            = pXrLocalStat->mosCQ;

            rtcpCnxStats->localJitterBuffer       = pXrLocalStat->jitterBufferNominal;
            rtcpCnxStats->localMaxJitterBuffer    = pXrLocalStat->jitterBufferMax;
            rtcpCnxStats->localWorstJitterBuffer  = pXrLocalStat->jitterBufferWorstCase;

            /*AT&T extension*/
            rtcpCnxStats->sumRoundTripDelay     = pXrLocalStat->sumRoundTripDelay;
            rtcpCnxStats->sumSqrRoundTripDelay  = pXrLocalStat->sumSqrRoundTripDelay;
            rtcpCnxStats->maxOneWayDelay        = pXrLocalStat->maxOneWayDelay;
            rtcpCnxStats->sumOneWayDelay        = pXrLocalStat->sumOneWayDelay;
            rtcpCnxStats->sumSqrOneWayDelay     = pXrLocalStat->sumSqrOneWayDelay;
        }
    }

    pXrRemoteStat = &mmStat->rtpHandle.remoteRtcpXrStat;  /* Remote RTCP-XR statistics */

    /* If RTCP-XR packets are captured */
    if (pXrRemoteStat->rtcpXrUpdated != 0) {
        /* Populate the Remote connection statistics */
        if (RTCPXR_BT_UPDATED(pXrRemoteStat->rtcpXrUpdated, 6)) {
            rtcpCnxStats->remoteJitter        = pXrRemoteStat->meanJitter;
            rtcpCnxStats->remoteMinJitter     = pXrRemoteStat->minJitter;
            rtcpCnxStats->remoteMaxJitter     = pXrRemoteStat->maxJitter;
            rtcpCnxStats->remoteDevJitter     = pXrRemoteStat->devJitter;
            rtcpCnxStats->remoteAverageJitter = pXrRemoteStat->meanJitter;
            rtcpCnxStats->remotePktLost       = pXrRemoteStat->sumLostPackets;
        }
        if (RTCPXR_BT_UPDATED(pXrRemoteStat->rtcpXrUpdated, 7)) {
            rtcpCnxStats->remotePktLostRate       = pXrRemoteStat->lossRate;
            rtcpCnxStats->remotePktDiscardedRate  = pXrRemoteStat->discardedRate;

            rtcpCnxStats->remoteLatency     = pXrRemoteStat->roundTripDelay;
            rtcpCnxStats->remoteMaxLatency  = pXrRemoteStat->maxRoundTripDelay;

            rtcpCnxStats->remoteAverageJitter   = (rtcpCnxStats->remoteAverageJitter + rtcpCnxStats->remoteJitter) / 2;
            rtcpCnxStats->remoteAverageLatency  = (rtcpCnxStats->remoteAverageLatency + rtcpCnxStats->remoteLatency) / 2;

            rtcpCnxStats->remoteSignalLevel     = pXrRemoteStat->signalLevel;
            rtcpCnxStats->remoteNoiseLevel      = pXrRemoteStat->noiseLevel;
            rtcpCnxStats->remoteRERL            = pXrRemoteStat->RERL;
            rtcpCnxStats->remoteRFactor         = pXrRemoteStat->RFactor;
            rtcpCnxStats->remoteExternalRFactor = pXrRemoteStat->externalRFactor;
            rtcpCnxStats->remoteMosLQ           = pXrRemoteStat->mosLQ;
            rtcpCnxStats->remoteMosCQ           = pXrRemoteStat->mosCQ;

            rtcpCnxStats->remoteJitterBuffer      = pXrRemoteStat->jitterBufferNominal;
            rtcpCnxStats->remoteMaxJitterBuffer   = pXrRemoteStat->jitterBufferMax;
            rtcpCnxStats->remoteWorstJitterBuffer = pXrRemoteStat->jitterBufferWorstCase;
        }
    }
}

/*
 *****************************************************************************
 ** FUNCTION:   rtcpUpdateBufferedFrame
 **
 ** PURPOSE:    This function add buffered RTCP frames to report statistics
 **
 ** PARAMETERS: handle       - [IN]  RTP  handle
 **             rtcpCnxStats - [OUT] statistics (see MmConnRtcpCnxStats)
 **
 ** RETURNS:    None
 **
 ** NOTE:       None
 *****************************************************************************
 */
static void rtcpUpdateBufferedFrame(RTPHANDLE *handle, MmConnRtcpCnxStats *rtcpCnxStats)
{
    int i = 0;

    write_lock(&_rtcpBufferLock);
    for ( ; i < MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES; i++) {
        int index = (handle->lastRxRtcpBufIdx + i) % MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES;
        rtcpCnxStats->rxRtcpFrameLength[i] = handle->rxRtcpBuf[index].len;
        if (handle->rxRtcpBuf[index].buf != NULL) {
            rtcpCnxStats->rxRtcpFrameBuffer[i] = kmalloc(rtcpCnxStats->rxRtcpFrameLength[i], GFP_KERNEL);
            if (rtcpCnxStats->rxRtcpFrameBuffer[i] == NULL) {
                MMPBX_TRC_DEBUG("Unable to allocate memory to dump RTCP packet\n\r");
                rtcpCnxStats->rxRtcpFrameLength[i] = 0;
            }
            else {
                memcpy(rtcpCnxStats->rxRtcpFrameBuffer[i], handle->rxRtcpBuf[index].buf, rtcpCnxStats->rxRtcpFrameLength[i]); /* Copy the entire RTCP buffer */
            }
        }

        rtcpCnxStats->txRtcpFrameLength[i] = handle->txRtcpBuf[index].len;
        if (handle->txRtcpBuf[index].buf != NULL) {
            rtcpCnxStats->txRtcpFrameBuffer[i] = kmalloc(rtcpCnxStats->txRtcpFrameLength[i], GFP_KERNEL);
            if (rtcpCnxStats->txRtcpFrameBuffer[i] == NULL) {
                MMPBX_TRC_DEBUG("Unable to allocate memory to dump RTCP packet\n\r");
                rtcpCnxStats->txRtcpFrameLength[i] = 0;
            }
            else {
                memcpy(rtcpCnxStats->txRtcpFrameBuffer[i], handle->txRtcpBuf[index].buf, rtcpCnxStats->txRtcpFrameLength[i]);     /* Copy the entire RTCP buffer */
            }
        }
    }
    write_unlock(&_rtcpBufferLock);
    return;
}

static void networkToHostRTCPPACKET(void *buff, RTCPPACKET **rtcpbuf)
{
    if ((buff != NULL) && (rtcpbuf != NULL)) {
        (*rtcpbuf) = (RTCPPACKET *)buff;
#ifdef __LITTLE_ENDIAN
        (*rtcpbuf)->common.length = ntohs((*rtcpbuf)->common.length);
#endif
    }
}

static void networkToHostRTCPXR(void *buff, RTCPXR **pXr)
{
    if ((buff != NULL) && (pXr != NULL)) {
        (*pXr) = (RTCPXR *)buff;
#ifdef __LITTLE_ENDIAN
        (*pXr)->common.blockLength = ntohs((*pXr)->common.blockLength);
#endif
    }
}

static void networkToHostRTCPXR_BT1(RTCPXR_BT1 *buff, RTCPXR_BT1 **pBt)
{
    if ((buff != NULL) && (pBt != NULL)) {
        (*pBt) = buff;
#ifdef __LITTLE_ENDIAN
        (*pBt)->beginSeq  = ntohs((*pBt)->beginSeq);
        (*pBt)->endSeq    = ntohs((*pBt)->endSeq);
        (*pBt)->ssrc      = ntohl((*pBt)->ssrc);
#endif
    }
}

static void networkToHostRTCPXR_BT3(RTCPXR_BT3 *buff, RTCPXR_BT3 **pBt)
{
    if ((buff != NULL) && (pBt != NULL)) {
        (*pBt) = buff;
#ifdef __LITTLE_ENDIAN
        (*pBt)->ssrc      = ntohl((*pBt)->ssrc);
        (*pBt)->beginSeq  = ntohs((*pBt)->beginSeq);
        (*pBt)->endSeq    = ntohs((*pBt)->endSeq);
#endif
    }
}

static void networkToHostRTCPXR_BT4(RTCPXR_BT4 *buff, RTCPXR_BT4 **pBt)
{
    if ((buff != NULL) && (pBt != NULL)) {
        (*pBt) = buff;
#ifdef __LITTLE_ENDIAN
        (*pBt)->msw = ntohl((*pBt)->msw);
        (*pBt)->lsw = ntohl((*pBt)->lsw);
#endif
    }
}

static void networkToHostRTCPXR_BT6(RTCPXR_BT6 *buff, RTCPXR_BT6 **pBt)
{
    if ((buff != NULL) && (pBt != NULL)) {
        (*pBt) = buff;
#ifdef __LITTLE_ENDIAN
        (*pBt)->ssrc              = ntohl((*pBt)->ssrc);
        (*pBt)->beginSeq          = ntohs((*pBt)->beginSeq);
        (*pBt)->endSeq            = ntohs((*pBt)->endSeq);
        (*pBt)->lostPackets       = ntohl((*pBt)->lostPackets);
        (*pBt)->duplicatedPackets = ntohl((*pBt)->duplicatedPackets);
        (*pBt)->minJitter         = ntohl((*pBt)->minJitter);
        (*pBt)->maxJitter         = ntohl((*pBt)->maxJitter);
        (*pBt)->meanJitter        = ntohl((*pBt)->meanJitter);
        (*pBt)->devJitter         = ntohl((*pBt)->devJitter);
#endif
    }
}

static void networkToHostRTCPXR_BT7(RTCPXR_BT7 *buff, RTCPXR_BT7 **pBt)
{
    if ((buff != NULL) && (pBt != NULL)) {
        (*pBt) = buff;
#ifdef __LITTLE_ENDIAN
        (*pBt)->ssrc                  = ntohl((*pBt)->ssrc);
        (*pBt)->burstDuration         = ntohs((*pBt)->burstDuration);
        (*pBt)->gapDuration           = ntohs((*pBt)->gapDuration);
        (*pBt)->roundTripDelay        = ntohs((*pBt)->roundTripDelay);
        (*pBt)->endSystemDelay        = ntohs((*pBt)->endSystemDelay);
        (*pBt)->jitterBufferNominal   = ntohs((*pBt)->jitterBufferNominal);
        (*pBt)->jitterBufferMax       = ntohs((*pBt)->jitterBufferMax);
        (*pBt)->jitterBufferWorstCase = ntohs((*pBt)->jitterBufferWorstCase);
#endif
    }
}
