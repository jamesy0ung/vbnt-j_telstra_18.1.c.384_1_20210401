/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2015-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **                                                                      **
*************************************************************************/

/** \file
 *      MmConnRtcpStats API
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNRTCPSTATS_P_H
#define  MMCONNRTCPSTATS_P_H

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmcommon.h"
#include "mmconn.h"

/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/
/*
 * Number of frames that need to be saved each connection and
 * per direction
 */
#define MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES    2

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/
typedef struct MmConnRtcpStatsSession  *MmConnRtcpStatsSessionHndl;

/**
 * RTCP instance statistics
 */
typedef struct {
    /* Local statisticstics, measured by the DSP or RTP instance*/
    uint64_t      sessionDuration;              /**< Session duraton in milliseconds.*/
    uint64_t      txRtpPackets;                 /**< RTP packets sent. */
    uint64_t      txRtpBytes;                   /**< RTP bytes sent. */
    uint64_t      rxRtpPackets;                 /**< RTP packets received. */
    uint64_t      rxRtpBytes;                   /**< RTP bytes received. */
    uint64_t      rxTotalPacketsLost;           /**< Total number of lost packets in RX stream. */
    uint64_t      rxTotalPacketsDiscarded;      /**< Total number of discarded packets in RX stream. */
    unsigned long rxPacketsLostRate;            /**< Packets Lost Rate */
    unsigned long rxPacketsDiscardedRate;       /**< Discarded Packets Rate */
    unsigned long signalLevel;                  /**< The Voice signal relative level in dBm */
    unsigned long noiseLevel;                   /**< The Noise level in dBm */
    unsigned long RERL;                         /**< Residual Echo Return Loss */
    unsigned long RFactor;                      /**< Voice quality metric in R-Factor */
    unsigned long externalRFactor;              /**< Voice quality metric in R-Factor for external network */
    unsigned long mosLQ;                        /**< MOS Score Line Quality (value * 10)*/
    unsigned long mosCQ;                        /**< MOS Score Conversational Quality (value * 10)*/
    unsigned long meanE2eDelay;                 /**< Mean end-to-end delay in microseconds. */
    unsigned long worstE2eDelay;                /**< Worst end-to-end delay in microseconds. */
    unsigned long currentE2eDelay;              /**< Current end-to-end delay in microseconds. */
    unsigned long rxJitter;                     /**< jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long rxMinJitter;                  /**< Min jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long rxMaxJitter;                  /**< Max jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long rxDevJitter;                  /**< Dev jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long meanRxJitter;                 /**< Mean Jitter in RX stream. \n Jitter is measred in timestamp units (0.125 ms for 8kHz codecs).*/
    unsigned long worstRxJitter;                /**< Worst Jitter in RX stream. \n Jitter is measred in timestamp units (0.125 ms for 8kHz codecs).*/
    unsigned long jitterBufferOverruns;         /**< Number of Jitter Buffer overruns */
    unsigned long jitterBufferUnderruns;        /**< Number of Jitter buffer underruns */
    /* Remote party statistics, measured by the remote party */
    uint64_t      remoteTxRtpPackets;           /**< Number of RTP packets sent. */
    uint64_t      remoteTxRtpBytes;             /**< Number of RTP bytes send.*/
    uint64_t      remoteRxTotalPacketsLost;     /**< Total number of lost packets in RX stream.   */
    unsigned long remoteRxPacketsLostRate;      /**< Lost packets rate in RX stream.   */
    unsigned long remoteRxPacketsDiscardedRate; /**< Total number of discared packets in RX stream.   */
    unsigned long remoteSignalLevel;            /**< Signal Level in dBm in RX stream. */
    unsigned long remoteNoiseLevel;             /**< Noise level in dBm  in RX stream. */
    unsigned long remoteRERL;                   /**< Residual Echo Return Loss */
    unsigned long remoteRFactor;                /**< Voice quality metric in R-Factor */
    unsigned long remoteExternalRFactor;        /**< Voice quality metric in R-Factor for external network */
    unsigned long remoteMosLQ;                  /**< MOS Score Line Quality (value * 10)*/
    unsigned long remoteMosCQ;                  /**< MOS Score Conversational Quality (value * 10)*/
    unsigned long remoteMeanE2eDelay;           /**< end-to-end delay in microseconds. */
    unsigned long remoteWorstE2eDelay;          /**< end-to-end delay in microseconds. */
    unsigned long remoteCurrentE2eDelay;        /**< end-to-end delay in microseconds. */
    unsigned long remoteRxJitter;               /**< Mean Jitter in RX stream.\n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs).*/
    unsigned long minRemoteRxJitter;            /**< Min jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long maxRemoteRxJitter;            /**< Max jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long devRemoteRxJitter;            /**< Dev jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long meanRemoteRxJitter;           /**< Mean Jitter in RX stream. \n Jitter is measred in timestamp units (0.125 ms for 8kHz codecs).*/
    unsigned long remoteRxWorstJitter;          /**< Worst jitter in RX stream.\n Measured in timestamp units (0.125 ms for 8kHz codecs).*/
    uint64_t      txRtcpXrPackets;              /**< RTCP-XR packets sent. */
    uint64_t      rxRtcpXrPackets;              /**< RTCP-XR packets sent. */
    /*AT&T extension*/
    uint64_t      txRtcpPackets;            /**< RTCP packets sent. */
    uint64_t      rxRtcpPackets;            /**< RTCP packets received. */
    unsigned long localSumFractionLoss;     /**< Sum of tx fraction loss */
    unsigned long localSumSqrFractionLoss;  /**< Sum of square of tx fraction loss */
    unsigned long remoteSumFractionLoss;    /**< Sum of tx fraction loss */
    unsigned long remoteSumSqrFractionLoss; /**< Sum of square of tx fraction loss */
    unsigned long localSumJitter;           /**< summary of tx Interarrival jitter */
    unsigned long localSumSqrJitter;        /**< Sum of square of tx Interarrival jitter */
    unsigned long remoteSumJitter;          /**< Sum of tx Interarrival jitter*/
    unsigned long remoteSumSqrJitter;       /**< summary of square of tx Interarrival jitter */
    unsigned long sumRoundTripDelay;        /**< Sum of Round Trip Delay */
    unsigned long sumSqrRoundTripDelay;     /**< Sum of Square of Max Round Trip Delay */
    unsigned long maxOneWayDelay;           /**< Max One Way Delay */
    unsigned long sumOneWayDelay;           /**< Sum of One Way Delay */
    unsigned long sumSqrOneWayDelay;        /**< Sum of Square of One Way Delay */
    /* RTCP frame buffer */
    unsigned long txRtcpFrameLength[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES];  /**< Length of tx RTCP frame*/
    uint8_t       *txRtcpFrameBuffer[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES]; /**< Buffer of tx RTCP frame*/
    unsigned long rxRtcpFrameLength[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES];  /**< Length of rx RTCP frame*/
    uint8_t       *rxRtcpFrameBuffer[MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES]; /**< Buffer of rx RTCP frame*/
} MmConnRtcpStats;

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/

MmPbxError mmConnRtcpStatsSetTraceLevel(MmPbxTraceLevel level);

MmPbxError mmConnRtcpStatsInit(void);

MmPbxError mmConnRtcpStatsDeinit(void);

MmPbxError mmConnRtcpStatsOpenSession(MmConnHndl                  mmConn,
                                      MmConnRtcpStatsSessionHndl  *session);

MmPbxError mmConnRtcpStatsCloseSession(MmConnRtcpStatsSessionHndl *session);

MmPbxError mmConnRtcpStatsResetSession(MmConnRtcpStatsSessionHndl session);

MmPbxError mmConnRtcpStatsGetSessionStats(MmConnRtcpStatsSessionHndl  session,
                                          MmConnRtcpStats             *pStats);

MmPbxError mmConnRtcpStatsOnPacketReceived(MmConnRtcpStatsSessionHndl session,
                                           MmConnPacketType           type,
                                           uint8_t                    *data,
                                           int                        len);

MmPbxError mmConnRtcpStatsOnPacketSent(MmConnRtcpStatsSessionHndl session,
                                       MmConnPacketType           type,
                                       uint8_t                    *data,
                                       int                        len);
#endif /* #ifdef MMCONNRTCPSTATS_P_H */
