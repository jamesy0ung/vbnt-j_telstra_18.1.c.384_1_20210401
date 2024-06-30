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
 * Multimedia Switch API.
 *
 * A multimedia switch routes (multimedia) streams between all connection instances attached to the multimedia switch.
 * A multimedia switch is implemented in user space and kernel space.
 * All configuration/management is done in user space, all stream handling in kernel space.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "[MMSWITCH_NETLINK]"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/jiffies.h>
#include <net/genetlink.h>
#include <net/net_namespace.h>

#include "mmswitch.h"
#include "mmswitch_netlink_p.h"
#include "mmconn_p.h"
#include "mmconnuser_p.h"
#include "mmconnuser_netlink_p.h"
#include "mmconnkernel_p.h"
#include "mmconnmulticast_p.h"
#include "mmconnrelay_p.h"
#include "mmconnrtcp_p.h"
#include "mmconntone_p.h"
#include "mmconnrtcpstats_p.h"

/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/

/*########################################################################
#                                                                        #
#  TYPES                                                                 #
#                                                                        #
########################################################################*/

/*########################################################################
#                                                                        #
#  PRIVATE FUNCTION PROTOTYPES                                           #
#                                                                        #
########################################################################*/
static int mmSwitchFillMmConnAttr(struct sk_buff  *skb,
                                  MmConnHndl      mmConn);
static int mmSwitchFillMmConnUsrAttr(struct sk_buff *skb,
                                     MmConnUsrHndl  mmConnUsr);
static int mmSwitchParseMmConnDestruct(struct sk_buff   *skb_2,
                                       struct genl_info *info);
static int mmSwitchParseMmConnXConn(struct sk_buff    *skb_2,
                                    struct genl_info  *info);
static int mmSwitchParseMmConnDisc(struct sk_buff   *skb_2,
                                   struct genl_info *info);
static int mmSwitchParseMmConnSetTraceLevel(struct sk_buff    *skb_2,
                                            struct genl_info  *info);

static int mmSwitchParseMmConnUsrConstruct(struct sk_buff   *skb_2,
                                           struct genl_info *info);
static int mmSwitchParseMmConnUsrSync(struct sk_buff    *skb_2,
                                      struct genl_info  *info);
static int mmSwitchParseMmConnUsrDestroyGeNlFam(struct sk_buff    *skb_2,
                                                struct genl_info  *info);
static int mmSwitchParseMmConnUsrSetTraceLevel(struct sk_buff   *skb_2,
                                               struct genl_info *info);

static int mmSwitchParseMmConnKrnlConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info);
static int mmSwitchParseMmConnKrnlSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info);

static int mmSwitchParseMmConnMulticastConstruct(struct sk_buff   *skb_2,
                                                 struct genl_info *info);
static int mmSwitchParseMmConnMulticastSetTraceLevel(struct sk_buff   *skb_2,
                                                     struct genl_info *info);
static int mmSwitchParseMmConnMulticastAddSink(struct sk_buff   *skb_2,
                                               struct genl_info *info);
static int mmSwitchParseMmConnMulticastRemoveSink(struct sk_buff    *skb_2,
                                                  struct genl_info  *info);

static int mmSwitchParseMmConnToneConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info);
static int mmSwitchParseMmConnToneSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info);
static int mmSwitchParseMmConnToneSendPattern(struct sk_buff    *skb_2,
                                              struct genl_info  *info);

static int mmSwitchParseMmConnRelayConstruct(struct sk_buff   *skb_2,
                                             struct genl_info *info);
static int mmSwitchParseMmConnRelaySetTraceLevel(struct sk_buff   *skb_2,
                                                 struct genl_info *info);

static int mmSwitchParseMmConnRtcpConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info);
static int mmSwitchParseMmConnRtcpSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info);
static int mmSwitchParseMmConnRtcpModPacketType(struct sk_buff    *skb_2,
                                                struct genl_info  *info);
static int mmSwitchParseMmConnRtcpModGenRtcp(struct sk_buff   *skb_2,
                                             struct genl_info *info);
static int mmSwitchParseMmConnRtcpModRemoteMediaAddr(struct sk_buff   *skb_2,
                                                     struct genl_info *info);
static int mmSwitchParseMmConnRtcpModRemoteRtcpAddr(struct sk_buff    *skb_2,
                                                    struct genl_info  *info);
static int mmSwitchParseMmConnRtcpModRtcpBandwidth(struct sk_buff   *skb_2,
                                                   struct genl_info *info);
static int mmSwitchParseMmConnRtcpGetStats(struct sk_buff   *skb_2,
                                           struct genl_info *info);
static int mmSwitchParseMmConnRtcpResetStats(struct sk_buff   *skb_2,
                                             struct genl_info *info);
static int mmSwitchFillRtcpStats(struct sk_buff   *skb,
                                 MmConnRtcpStats  *pStats);

/*########################################################################
#                                                                        #
#  PRIVATE GLOBALS                                                       #
#                                                                        #
########################################################################*/
MMPBX_TRACEDEF(MMPBX_TRACELEVEL_ERROR);

/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h
 */
static struct nla_policy mmSwitchGeNlAttrPolicy[MMSWITCH_ATTR_MAX + 1] =
{
    [MMSWITCH_ATTR_MMCONN]                  =       { .type = NLA_NESTED },
    [MMSWITCH_ATTR_MMCONNUSR]               =       { .type = NLA_NESTED },
    [MMSWITCH_ATTR_MMCONNRTCP]              =       { .type = NLA_NESTED },
    [MMSWITCH_ATTR_MMCONNKRNL_ENDPOINT_ID]  =       { .type = NLA_U32    },
    [MMSWITCH_ATTR_MMPBX_TRACELEVEL]        =       { .type = NLA_U32    },
    [MMSWITCH_ATTR_ENCODINGNAME]            =       { .type = NLA_STRING },
#ifdef ARM64
    [MMSWITCH_ATTR_REF_MMCONN] =                    { .type = NLA_U64    },
#else
    [MMSWITCH_ATTR_REF_MMCONN] =                    { .type = NLA_UNSPEC },
#endif
    [MMSWITCH_ATTR_LOCAL_SOCKFD]                  = { .type = NLA_U32    },
    [MMSWITCH_ATTR_LOCAL_RTCP_SOCKFD]             = { .type = NLA_U32    },
    [MMSWITCH_ATTR_REMOTE_ADDR]                   = { .type = NLA_UNSPEC },
    [MMSWITCH_ATTR_REMOTE_RTCP_ADDR]              = { .type = NLA_UNSPEC },
    [MMSWITCH_ATTR_PACKET_TYPE]                   = { .type = NLA_U32    },
    [MMSWITCH_ATTR_TIMEOUT]                       = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MUTE_TIME]                     = { .type = NLA_U32    },
    [MMSWITCH_ATTR_RTCP_BANDWIDTH]                = { .type = NLA_U32    },
    [MMSWITCH_ATTR_GEN_RTCP]                      = { .type = NLA_U32    },
    [MMSWITCH_ATTR_KEEP_STATS]                    = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MMCONNTONE_ENDPOINT_ID]        = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MMCONNTONE_TYPE]               = { .type = NLA_U32    },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN]            = { .type = NLA_NESTED },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERNTABLE_SIZE]  = { .type = NLA_U32    },
};


static struct nla_policy mmSwitchGeNlAttrMmConnPolicy[MMSWITCH_ATTR_MMCONN_MAX + 1] =
{
#ifdef ARM64
    [MMSWITCH_ATTR_MMCONN_REF] = { .type = NLA_U64    },
#else
    [MMSWITCH_ATTR_MMCONN_REF] = { .type = NLA_UNSPEC },
#endif
};

static struct nla_policy mmSwitchGeNlAttrMmConnTonePatternPolicy[MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX + 1] =
{
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_ID]               = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_ON]               = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ1]            = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ2]            = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ3]            = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ4]            = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER1]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER2]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER3]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER4]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_DURATION]         = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTID]           = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAXLOOPS]         = { .type = NLA_U32 },
    [MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTIDAFTERLOOPS] = { .type = NLA_U32 },
};


/* family definition */
static struct genl_family MmSwitchGeNlFamily =
{
    .id       = GENL_ID_GENERATE,       //genetlink should generate an id
    .hdrsize  =                 0,
    .name     = MMSWITCH_GENL_FAMILY,                               //the name of this family, used by userspace application
    .version  = MMSWITCH_GENL_FAMILY_VERSION,                       //version number
    .maxattr  = MMSWITCH_ATTR_MAX,
};

/* commands: mapping between the command enumeration and the actual function*/
static struct genl_ops mmSwitchGeNlOps[] =
{
    {
        .cmd    = MMSWITCH_CMD_MMCONN_DESTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnDestruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONN_XCONN,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnXConn,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONN_DISC,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnDisc,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONN_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNUSR_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnUsrConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNUSR_SYNC,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnUsrSync,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNUSR_DESTROY_GENL_FAM,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnUsrDestroyGeNlFam,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNUSR_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnUsrSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNKRNL_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnKrnlConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNKRNL_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnKrnlSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNMULTICAST_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnMulticastConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNMULTICAST_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnMulticastSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNMULTICAST_ADD_SINK,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnMulticastAddSink,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNMULTICAST_REMOVE_SINK,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnMulticastRemoveSink,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNTONE_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnToneConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNTONE_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnToneSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNTONE_SEND_PATTERN,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnToneSendPattern,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRELAY_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRelayConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRELAY_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRelaySetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_CONSTRUCT,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpConstruct,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_SET_TRACE_LEVEL,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpSetTraceLevel,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_PACKET_TYPE,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModPacketType,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_GEN_RTCP,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModGenRtcp,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_REMOTE_MEDIA_ADDR,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModRemoteMediaAddr,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_REMOTE_RTCP_ADDR,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModRemoteRtcpAddr,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_MOD_RTCPBANDWIDTH,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpModRtcpBandwidth,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_GET_STATS,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpGetStats,
        .dumpit = NULL,
    },
    {
        .cmd    = MMSWITCH_CMD_MMCONNRTCP_RESET_STATS,
        .flags  = 0,
        .policy = mmSwitchGeNlAttrPolicy,
        .doit   = mmSwitchParseMmConnRtcpResetStats,
        .dumpit = NULL,
    },
};

/*########################################################################
#                                                                        #
#   PUBLIC FUNCTION DEFINITIONS                                          #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmSwitchGeNlSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_DEBUG("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmSwitchGeNlInit(void)
{
    MmPbxError  err = MMPBX_ERROR_NOERROR;
    int         ret = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31)
    int i;
#endif

    do {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        /*register new family and commands (functions) of the new family*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
        ret = genl_register_family_with_ops(&MmSwitchGeNlFamily, mmSwitchGeNlOps);
#else
        ret = genl_register_family_with_ops(&MmSwitchGeNlFamily, mmSwitchGeNlOps, ARRAY_SIZE(mmSwitchGeNlOps));
#endif
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family_with_ops failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }
#else
        ret = genl_register_family(&MmSwitchGeNlFamily);
        if (ret != 0) {
            MMPBX_TRC_ERROR("genl_register_family failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        for (i = 0; i < ARRAY_SIZE(mmSwitchGeNlOps); i++) {
            ret = genl_register_ops(&MmSwitchGeNlFamily, &mmSwitchGeNlOps[i]);
            if (ret != 0) {
                MMPBX_TRC_ERROR("genl_register_ops failed\n");
                err = MMPBX_ERROR_INTERNALERROR;
                break;
            }
        }
#endif
    } while (0);

    if (err != MMPBX_ERROR_NOERROR) {
        genl_unregister_family(&MmSwitchGeNlFamily);
    }
    return err;
}

/**
 *
 */
MmPbxError mmSwitchGeNlDeinit(void)
{
    MmPbxError  err = MMPBX_ERROR_NOERROR;
    int         ret = 0;


    do {
        /*unregister the family*/
        ret = genl_unregister_family(&MmSwitchGeNlFamily);
        if (ret != 0) {
            MMPBX_TRC_ERROR("unregister family failed with: %i\n", ret);
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }
    } while (0);

    return err;
}

/*
 * @brief Craft a netlink reponse to an MMSWITCH_CMD_MMCONNUSR_DESTROY_GENL_FAM request
 */
MmPbxError mmSwitchMmConnUsrSendDestoyGenlFamReply(MmConnUsrHndl mmConnUsr, u32 geNlSeq, u32 geNlPid)
{
    MmPbxError      err       = MMPBX_ERROR_NOERROR;
    struct sk_buff  *skb      = NULL;
    int             rc        = 0;
    void            *msg_head = NULL;

    do {
        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            err = MMPBX_ERROR_NORESOURCES;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, geNlSeq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNUSR_DESTROY_GENL_FAM);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnUsr);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, geNlPid);
#else
        rc = genlmsg_unicast(skb, mmConnUsr->geNlPid);
#endif
        if (rc != 0) {
            err = MMPBX_ERROR_INTERNALERROR;
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
    } while (0);

    if ((err != MMPBX_ERROR_NOERROR) && (skb != NULL)) {
        genlmsg_cancel(skb, msg_head);
        nlmsg_free(skb);
    }

    return err;
}

/*
 * @brief Craft a netlink reponse to an MMSWITCH_CMD_MMCONNUSR_CONSTRUCT request
 */
MmPbxError mmSwitchMmConnUsrSendConstructReply(MmConnUsrHndl mmConnUsr, u32 geNlSeq, u32 geNlPid)
{
    MmPbxError      err       = MMPBX_ERROR_NOERROR;
    struct sk_buff  *skb      = NULL;
    int             rc        = 0;
    void            *msg_head = NULL;

    do {
        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            err = MMPBX_ERROR_NORESOURCES;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, geNlSeq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNUSR_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnUsr);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, geNlPid);
#else
        rc = genlmsg_unicast(skb, geNlPid);
#endif
        if (rc != 0) {
            err = MMPBX_ERROR_INTERNALERROR;
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
    } while (0);

    if ((err != MMPBX_ERROR_NOERROR) && (skb != NULL)) {
        genlmsg_cancel(skb, msg_head);
        nlmsg_free(skb);
    }

    return err;
}

/*########################################################################
#                                                                        #
#   PRIVATE FUNCTION DEFINITIONS                                         #
#                                                                        #
########################################################################*/

/**
 *
 */
static int mmSwitchFillMmConnAttr(struct sk_buff  *skb,
                                  MmConnHndl      mmConn)
{
    struct nlattr *nest;

#ifdef ARM64
    uint64_t tmp = 0;
#endif


    do {
        nest = nla_nest_start(skb, MMSWITCH_ATTR_MMCONN);
        if (!nest) {
            break;
        }
#ifdef ARM64
        tmp = (uint64_t) mmConn;
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONN_REF, tmp) < 0) {
#else
        if (nla_put(skb, MMSWITCH_ATTR_MMCONN_REF, sizeof(void *), &mmConn) < 0) {
#endif
            break;
        }

        nla_nest_end(skb, nest);
        return 0;
    } while (0);

    MMPBX_TRC_ERROR("an error occured in %s\n", __func__);

    return -1;
}

/**
 *
 */
static int mmSwitchFillMmConnUsrAttr(struct sk_buff *skb,
                                     MmConnUsrHndl  mmConnUsr)
{
    struct nlattr *nest;


    nest = nla_nest_start(skb, MMSWITCH_ATTR_MMCONNUSR);
    do {
        if (!nest) {
            break;
        }

        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNUSR_GENL_FAMID, mmConnUsr->geNlFam.id) < 0) {
            break;
        }

        if (nla_put_string(skb, MMSWITCH_ATTR_MMCONNUSR_GENL_FAMNAME, mmConnUsr->geNlFam.name) < 0) {
            break;
        }

        nla_nest_end(skb, nest);
        return 0;
    } while (0);

    MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
    return -1;
}

static void mmConnDistructWorkFn(struct work_struct *work)
{
    struct MmConnAsyncWork *asyncWork = NULL;

    asyncWork = container_of(work, struct MmConnAsyncWork, work);
    if (mmConnDestruct(&asyncWork->mmConn) != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnDestruct failed\n");
    }
    kfree(asyncWork);
}

/*
 *
 */
static int mmSwitchParseMmConnDestruct(struct sk_buff   *skb_2,
                                       struct genl_info *info)
{
    MmConnHndl              mmConn = NULL;
    struct nlattr           *nla;
    int                     rc          = 0;
    struct MmConnAsyncWork  *asyncWork  = NULL;

    do {
        if (info == NULL) {
            MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
            rc = -1;
            break;
        }

        /*Parse attribute */
        nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
        if (!nla) {
            rc = -EINVAL;
            break;
        }

        nla_memcpy(&mmConn, nla, sizeof(void *));

        asyncWork = kmalloc(sizeof(struct MmConnAsyncWork), GFP_KERNEL);
        if (asyncWork == NULL) {
            rc = -ENOMEM;
            break;
        }

        asyncWork->mmConn = mmConn;
        mmSwitchInitWork(&asyncWork->work, mmConnDistructWorkFn);
        mmSwitchScheduleWork(&asyncWork->work);
    } while (false);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnXConn(struct sk_buff    *skb_2,
                                    struct genl_info  *info)
{
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    source  = NULL;
    MmConnHndl    target  = NULL;
    struct nlattr *parentAttr;
    struct nlattr *tb[MMSWITCH_ATTR_MAX + 1];
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMSWITCH_ATTR_MMCONN_MAX, parentAttr, mmSwitchGeNlAttrMmConnPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if (!tb[MMSWITCH_ATTR_MMCONN_REF]) {
        MMPBX_TRC_ERROR("ATTR_MCONN_REF not found !\n");
        return -EINVAL;
    }

    nla_memcpy(&source, tb[MMSWITCH_ATTR_MMCONN_REF], sizeof(void *));

    /*Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!parentAttr) {
        MMPBX_TRC_ERROR("ATTR_MCONN_REF_MMCONN not found !\n");
        return -EINVAL;
    }
    nla_memcpy(&target, parentAttr, sizeof(void *));

    err = mmConnXConn(source, target);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnXConn failed\n");
        rc = -1;
    }



    return rc;
}

static void mmConnDiscWorkFn(struct work_struct *work)
{
    int                       rc          = 0;
    struct MmConnNlAsyncReply *asyncReply = NULL;
    void                      *msg_head   = NULL;
    struct sk_buff            *skb        = NULL;

    do {
        asyncReply = container_of(work, struct MmConnNlAsyncReply, work);

        if (mmConnDisc(asyncReply->mmConn) != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnDisc failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -1;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, asyncReply->geNlSeq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONN_DISC);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONNRTCP attribute */
        rc = mmSwitchFillMmConnAttr(skb, asyncReply->mmConn);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, asyncReply->geNlPid);
#else
        rc = genlmsg_unicast(skb, asyncReply->geNlPid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
    } while (0);

    if ((rc != 0) && (skb != NULL)) {
        genlmsg_cancel(skb, msg_head);
        nlmsg_free(skb);
    }
    kfree(asyncReply);
}

int _scheduleMmConnDisc(MmConnHndl mmConn, struct genl_info *info)
{
    int                       rc          = 0;
    struct MmConnNlAsyncReply *asyncReply = NULL;

    do {
        asyncReply = kmalloc(sizeof(struct MmConnNlAsyncReply), GFP_KERNEL);
        if (asyncReply == NULL) {
            rc = -ENOMEM;
            break;
        }

        asyncReply->mmConn  = mmConn;
        asyncReply->geNlSeq = info->snd_seq;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        asyncReply->geNlPid = info->snd_portid;
#else
        asyncReply->geNlPid = info->snd_pid;
#endif
        mmSwitchInitWork(&asyncReply->work, mmConnDiscWorkFn);
        mmSwitchScheduleWork(&asyncReply->work);
    } while (0);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnDisc(struct sk_buff   *skb_2,
                                   struct genl_info *info)
{
    MmConnHndl    mmConn = NULL;
    struct nlattr *nla;
    int           rc = 0;

    do {
        if (info == NULL) {
            MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
            rc = -1;
            break;
        }

        /*Parse attribute */
        nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
        if (!nla) {
            rc = -EINVAL;
            break;
        }

        nla_memcpy(&mmConn, nla, sizeof(void *));

        if (_scheduleMmConnDisc(mmConn, info) < 0) {
            MMPBX_TRC_ERROR("_scheduleMmConnDisc failed with\n");
            rc = -1;
            break;
        }
    } while (0);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnSetTraceLevel(struct sk_buff    *skb_2,
                                            struct genl_info  *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnUsrSetTraceLevel(struct sk_buff   *skb_2,
                                               struct genl_info *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnUsrSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnUsrConstruct(struct sk_buff   *skb_2,
                                           struct genl_info *info)
{
    int           rc        = 0;
    MmPbxError    err       = MMPBX_ERROR_NOERROR;
    MmConnUsrHndl mmConnUsr = NULL;

    if (info == NULL) {
        MMPBX_TRC_ERROR("Invalid info == NULL\n");
        return -1;
    }

    do {
        err = mmConnUsrConstruct(&mmConnUsr, info);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrConstruct failed\n");
            rc = -1;
            break;
        }
    } while (0);

    return rc;
}

/*
 *  Synchronize kernel space mmConnUsr with userspace mmConnUsr
 */
static int mmSwitchParseMmConnUsrSync(struct sk_buff    *skb_2,
                                      struct genl_info  *info)
{
    MmConnHndl      mmConn = NULL;
    struct nlattr   *nla;
    struct sk_buff  *skb      = NULL;
    int             rc        = 0;
    void            *msg_head = NULL;



    if (info == NULL) {
        MMPBX_TRC_ERROR("info == NULL\n");
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        MMPBX_TRC_ERROR("Invalid nla == NULL\n");
        return -EINVAL;
    }

    nla_memcpy(&mmConn, nla, sizeof(void *));

    if (mmConn == NULL) {
        MMPBX_TRC_ERROR("Invalid mmConn (ref is NULL)\n");
        return -EINVAL;
    }

    MMPBX_TRC_DEBUG("called, mmConnUsr = %p\n", mmConn);

    do {
        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNUSR_SYNC);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONNUSR attribute */
        rc = mmSwitchFillMmConnUsrAttr(skb, (MmConnUsrHndl) mmConn);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnUsrAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }

        return rc;
    } while (0);
    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);

    return rc;
}

/*
 *  Destroy mmConnUsr generic netlink family
 */
static int mmSwitchParseMmConnUsrDestroyGeNlFam(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err       = MMPBX_ERROR_NOERROR;
    MmConnUsrHndl mmConnUsr = NULL;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }

    nla_memcpy(&mmConnUsr, nla, sizeof(void *));

    if (mmConnUsr == NULL) {
        MMPBX_TRC_ERROR("Invalid mmConnUsr (ref is NULL)\n");
        return -EINVAL;
    }

    err = mmConnUsrGeNlDestroyFam((MmConnUsrHndl) mmConnUsr, info);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnDestruct failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnKrnlConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info)
{
    MmPbxError        err         = MMPBX_ERROR_NOERROR;
    MmConnKrnlHndl    mmConnKrnl  = NULL;
    struct nlattr     *nla        = NULL;
    struct sk_buff    *skb        = NULL;
    int               rc          = 0;
    void              *msg_head   = NULL;
    MmConnKrnlConfig  config;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    memset(&config, 0, sizeof(MmConnKrnlConfig));

    /*Parse  attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMCONNKRNL_ENDPOINT_ID];
    if (!nla) {
        return -EINVAL;
    }
    config.endpointId = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_MMCONNKRNL_RTCP_SUPPORT];
    if (!nla) {
        return -EINVAL;
    }
    config.rtcpSupport = (nla_get_u32(nla) == 0) ? false : true;

    do {
        err = mmConnKrnlConstruct(&config, &mmConnKrnl);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNKRNL_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnKrnl);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnKrnl != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnKrnl);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrDestruct failed\n");
        }
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnKrnlSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnKrnlSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnMulticastConstruct(struct sk_buff   *skb_2,
                                                 struct genl_info *info)
{
    MmPbxError          err             = MMPBX_ERROR_NOERROR;
    MmConnMulticastHndl mmConnMulticast = NULL;
    struct sk_buff      *skb            = NULL;
    void                *msg_head       = NULL;
    int                 rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    do {
        err = mmConnMulticastConstruct(&mmConnMulticast);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnMulticastConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNMULTICAST_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnMulticast);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnMulticast != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnMulticast);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnMulticastDestruct failed\n");
        }
    }
    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnMulticastSetTraceLevel(struct sk_buff   *skb_2,
                                                     struct genl_info *info)
{
    int           rc  = 0;
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnMulticastSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnMulticastSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnMulticastAddSink(struct sk_buff   *skb_2,
                                               struct genl_info *info)
{
    MmPbxError    ret     = MMPBX_ERROR_NOERROR;
    MmConnHndl    source  = NULL;
    MmConnHndl    sink    = NULL;
    struct nlattr *parentAttr;
    struct nlattr *tb[MMSWITCH_ATTR_MAX + 1];
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMSWITCH_ATTR_MMCONN_MAX, parentAttr, mmSwitchGeNlAttrMmConnPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if (!tb[MMSWITCH_ATTR_MMCONN_REF]) {
        return -EINVAL;
    }

    nla_memcpy(&source, tb[MMSWITCH_ATTR_MMCONN_REF], sizeof(void *));

    /*Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    nla_memcpy(&sink, parentAttr, sizeof(void *));

    ret = mmConnMulticastAddSink((MmConnMulticastHndl)source, (MmConnMulticastHndl) sink);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnMulticastAddSink failed with error: %s\n", mmPbxGetErrorString(ret));
        rc = -1;
    }


    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnMulticastRemoveSink(struct sk_buff    *skb_2,
                                                  struct genl_info  *info)
{
    MmPbxError    ret     = MMPBX_ERROR_NOERROR;
    MmConnHndl    source  = NULL;
    MmConnHndl    sink    = NULL;
    struct nlattr *parentAttr;
    struct nlattr *tb[MMSWITCH_ATTR_MAX + 1];
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMSWITCH_ATTR_MMCONN_MAX, parentAttr, mmSwitchGeNlAttrMmConnPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if (!tb[MMSWITCH_ATTR_MMCONN_REF]) {
        return -EINVAL;
    }

    nla_memcpy(&source, tb[MMSWITCH_ATTR_MMCONN_REF], sizeof(void *));

    /*Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!parentAttr) {
        return -EINVAL;
    }
    nla_memcpy(&sink, parentAttr, sizeof(void *));

    ret = mmConnMulticastRemoveSink((MmConnMulticastHndl)source, (MmConnMulticastHndl) sink);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnMulticastRemoveSink failed with error: %s\n", mmPbxGetErrorString(ret));
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnToneConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info)
{
    MmPbxError        err         = MMPBX_ERROR_NOERROR;
    MmConnToneHndl    mmConnTone  = NULL;
    struct nlattr     *nla        = NULL;
    struct sk_buff    *skb        = NULL;
    int               rc          = 0;
    void              *msg_head   = NULL;
    MmConnToneConfig  config;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    memset(&config, 0, sizeof(MmConnToneConfig));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_MMCONNTONE_ENDPOINT_ID];
    if (!nla) {
        return -EINVAL;
    }
    config.endpointId = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_MMCONNTONE_TYPE];
    if (!nla) {
        return -EINVAL;
    }
    config.type = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_MMCONNTONE_PATTERNTABLE_SIZE];
    if (!nla) {
        return -EINVAL;
    }
    config.toneTable.size = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_ENCODINGNAME];
    if (!nla) {
        return -EINVAL;
    }
    strncpy(config.encodingName, nla_data(nla), ENCODING_NAME_LENGTH);

    nla = info->attrs[MMSWITCH_ATTR_PACKETPERIOD];
    if (!nla) {
        return -EINVAL;
    }
    config.packetPeriod = nla_get_u32(nla);

    do {
        err = mmConnToneConstruct(&config, &mmConnTone);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnToneConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNTONE_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnTone);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnTone != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnTone);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnDestruct failed\n");
        }
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnToneSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnToneSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnToneSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnToneSendPattern(struct sk_buff    *skb_2,
                                              struct genl_info  *info)
{
    MmPbxError        err     = MMPBX_ERROR_NOERROR;
    MmConnHndl        mmConn  = NULL;
    MmConnTonePattern pattern;
    struct nlattr     *parentAttr;
    struct nlattr     *tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX + 1];
    int               rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse REF_MMCONN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!parentAttr) {
        MMPBX_TRC_ERROR("missing MMSWITCH_ATTR_REF_MMCONN\n");
        return -EINVAL;
    }

    nla_memcpy(&mmConn, parentAttr, sizeof(void *));

    /*Parse MMCONNTONE_PATTERN attribute */
    parentAttr = info->attrs[MMSWITCH_ATTR_MMCONNTONE_PATTERN];
    if (!parentAttr) {
        return -EINVAL;
    }
    rc = nla_parse_nested(tb, MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX, parentAttr, mmSwitchGeNlAttrMmConnTonePatternPolicy);
    if (rc < 0) {
        MMPBX_TRC_ERROR("nla_parse_nested failed\n");
        return rc;
    }

    if ((!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_ID]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_ON]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ1]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ2]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ3]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ4]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER1]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER2]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER3]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER4]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_DURATION]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTID]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAXLOOPS]) ||
        (!tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTIDAFTERLOOPS])
        ) {
        MMPBX_TRC_ERROR("missing pattern parameters\n");
        return -EINVAL;
    }

    pattern.id                = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_ID]);
    pattern.on                = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_ON]);
    pattern.freq1             = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ1]);
    pattern.freq2             = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ2]);
    pattern.freq3             = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ3]);
    pattern.freq4             = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ4]);
    pattern.power1            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER1]);
    pattern.power2            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER2]);
    pattern.power3            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER3]);
    pattern.power4            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER4]);
    pattern.duration          = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_DURATION]);
    pattern.nextId            = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTID]);
    pattern.maxLoops          = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAXLOOPS]);
    pattern.nextIdAfterLoops  = nla_get_u32(tb[MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTIDAFTERLOOPS]);


    err = mmConnToneSavePattern((MmConnToneHndl) mmConn, &pattern);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnToneSavePattern failed with error: %s\n", mmPbxGetErrorString(err));
        rc = -1;
    }



    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRelayConstruct(struct sk_buff   *skb_2,
                                             struct genl_info *info)
{
    MmPbxError        err         = MMPBX_ERROR_NOERROR;
    MmConnRelayHndl   mmConnRelay = NULL;
    struct nlattr     *nla        = NULL;
    struct sk_buff    *skb        = NULL;
    int               rc          = 0;
    void              *msg_head   = NULL;
    MmConnRelayConfig config;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    memset(&config, 0, sizeof(MmConnRelayConfig));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_LOCAL_SOCKFD];
    if (!nla) {
        return -EINVAL;
    }
    config.localSockFd = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&config.remoteAddr, nla, sizeof(struct sockaddr_storage));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_PACKET_TYPE];
    if (!nla) {
        return -EINVAL;
    }
    config.header.type = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_TIMEOUT];
    if (!nla) {
        return -EINVAL;
    }
    config.mediaTimeout = nla_get_u32(nla);

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&config.mainConn, nla, sizeof(void *));

    MMPBX_TRC_INFO("Constructing relay connection with main connection %p\r\n", config.mainConn);
    do {
        err = mmConnRelayConstruct(&config, &mmConnRelay);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRelayConstruct failed\n");
            rc = -1;
            break;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNRELAY_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, (MmConnHndl) mmConnRelay);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConnRelay != NULL) {
        err = mmConnDestruct((MmConnHndl *)&mmConnRelay);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRelayDestruct failed\n");
        }
    }

    MMPBX_TRC_INFO("Constructed relay connection %p\r\n", mmConnRelay);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRelaySetTraceLevel(struct sk_buff   *skb_2,
                                                 struct genl_info *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnRelaySetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpConstruct(struct sk_buff    *skb_2,
                                            struct genl_info  *info)
{
    MmPbxError        err           = MMPBX_ERROR_NOERROR;
    MmConnHndl        mmConn        = NULL;
    struct nlattr     *nla          = NULL;
    struct sk_buff    *skb          = NULL;
    int               rc            = 0;
    void              *msg_head     = NULL;
    MmConnRtcpConfig  config        = { 0 };
    MmConnRtcpHndl    mmConnRtcp    = NULL;
    MmConnRelayHndl   rtpRelayConn  = NULL;
    MmConnRelayHndl   rtcpRelayConn = NULL;
    MmConnRelayConfig relayConfig   = { 0 };


    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    memset(&config, 0, sizeof(MmConnRtcpConfig));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_LOCAL_SOCKFD];
    if (!nla) {
        return -EINVAL;
    }
    config.localMediaSockFd = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&config.remoteMediaAddr, nla, sizeof(struct sockaddr_storage));

    nla = info->attrs[MMSWITCH_ATTR_LOCAL_RTCP_SOCKFD];
    if (!nla) {
        return -EINVAL;
    }
    config.localRtcpSockFd = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_RTCP_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&config.remoteRtcpAddr, nla, sizeof(struct sockaddr_storage));

    /*Parse  attributes */
    nla = info->attrs[MMSWITCH_ATTR_PACKET_TYPE];
    if (!nla) {
        return -EINVAL;
    }
    config.header.type = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_TIMEOUT];
    if (!nla) {
        return -EINVAL;
    }
    config.mediaTimeout = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_MUTE_TIME];
    if (!nla) {
        return -EINVAL;
    }
    config.mediaMuteTime = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_RTCP_BANDWIDTH];
    if (!nla) {
        return -EINVAL;
    }
    config.rtcpBandwidth = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_GEN_RTCP];
    if (!nla) {
        return -EINVAL;
    }
    config.generateRtcp = nla_get_u32(nla);

    nla = info->attrs[MMSWITCH_ATTR_KEEP_STATS];
    if (!nla) {
        return -EINVAL;
    }
    config.keepStats = nla_get_u32(nla);

    do {
        if (config.generateRtcp == 0) {
            MMPBX_TRC_DEBUG("RTCP generation not required: constructing 2 relay connections\r\n");
            /* Create first relay connection for RTP */
            relayConfig.localSockFd = config.localMediaSockFd;
            memcpy(&relayConfig.remoteAddr, (void *)&config.remoteMediaAddr, sizeof(struct sockaddr_storage));
            relayConfig.header.type   = MMCONN_PACKET_TYPE_RTP;
            relayConfig.mediaTimeout  = config.mediaTimeout;
            err = mmConnRelayConstruct(&relayConfig, &rtpRelayConn);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRelayConstruct failed with: %s\n", mmPbxGetErrorString(err));
                break;
            }
            mmConn = (MmConnHndl)rtpRelayConn;

            /* Create second relay connection for RTCP */
            memset(&relayConfig, 0, sizeof(MmConnRelayConfig));

            relayConfig.localSockFd = config.localRtcpSockFd;
            memcpy(&relayConfig.remoteAddr, (void *)&config.remoteRtcpAddr, sizeof(struct sockaddr_storage));
            relayConfig.header.type   = MMCONN_PACKET_TYPE_RTCP;
            relayConfig.mediaTimeout  = 0;
            relayConfig.mainConn      = (MmConnHndl)rtpRelayConn;
            relayConfig.keepStats     = config.keepStats;
            err = mmConnRelayConstruct(&relayConfig, &rtcpRelayConn);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRelayConstruct failed with: %s\n", mmPbxGetErrorString(err));
                break;
            }

            /* Store it as a control connection to mmConn */
            err = mmConnSetConnControl((MmConnHndl)rtpRelayConn, (MmConnHndl)rtcpRelayConn);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
                break;
            }
        }
        else {
            MMPBX_TRC_DEBUG("RTCP generation required: constructing RTCP connection\r\n");
            err = mmConnRtcpConstruct(&config, &mmConnRtcp);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRtcpConstruct failed\n");
                rc = -1;
                break;
            }
            mmConn = (MmConnHndl)mmConnRtcp;
        }

        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNRTCP_CONSTRUCT);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* add MMCONN attribute */
        rc = mmSwitchFillMmConnAttr(skb, mmConn);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillMmConnAttr failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif
        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }
        return rc;
    } while (0);

    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);
    if (mmConn != NULL) {
        err = mmConnDestruct(&mmConn);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnDestruct failed\n");
        }
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpSetTraceLevel(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err = MMPBX_ERROR_NOERROR;
    struct nlattr *nla;
    int           rc = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_MMPBX_TRACELEVEL];
    if (!nla) {
        return -EINVAL;
    }

    err = mmConnRtcpSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpSetTraceLevel failed\n");
        rc = -1;
    }

    err = mmConnRtcpStatsSetTraceLevel(nla_get_u32(nla));
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpStatsSessionSetTraceLevel failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModPacketType(struct sk_buff    *skb_2,
                                                struct genl_info  *info)
{
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    mmConn  = NULL;
    struct nlattr *nla;
    int           rc    = 0;
    int           type  = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_PACKET_TYPE];
    if (!nla) {
        return -EINVAL;
    }

    type = nla_get_u32(nla);

    switch (mmConn->type) {
        case MMCONN_TYPE_RTCP:
            err = mmConnRtcpModMediaPacketType((MmConnRtcpHndl) mmConn, type);
            break;

        case MMCONN_TYPE_RELAY:
            MMPBX_TRC_DEBUG("Not implemented for connection of type %s\n", mmConnGetTypeString(mmConn->type));
            break;

        case MMCONN_TYPE_USER:
        case MMCONN_TYPE_KERNEL:
        case MMCONN_TYPE_TONE:
        case MMCONN_TYPE_NULL:
        case MMCONN_TYPE_MULTICAST:
            MMPBX_TRC_ERROR("Invalid connection type: %s\n", mmConnGetTypeString(mmConn->type));
            rc = -1;
            break;
    }

    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpModPacketType failed\n");
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModGenRtcp(struct sk_buff   *skb_2,
                                             struct genl_info *info)
{
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    mmConn  = NULL;
    struct nlattr *nla;
    int           rc      = 0;
    int           rtcpGen = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_GEN_RTCP];
    if (!nla) {
        return -EINVAL;
    }

    rtcpGen = nla_get_u32(nla);

    switch (mmConn->type) {
        case MMCONN_TYPE_RTCP:
            err = mmConnRtcpModGenRtcp((MmConnRtcpHndl) mmConn, rtcpGen);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRtcpModGenRtcp failed: %s\n", mmPbxGetErrorString(err));
            }
            break;

        case MMCONN_TYPE_RELAY:
            MMPBX_TRC_DEBUG("Not implemented for connection of type %s\n", mmConnGetTypeString(mmConn->type));
            break;

        case MMCONN_TYPE_USER:
        case MMCONN_TYPE_KERNEL:
        case MMCONN_TYPE_TONE:
        case MMCONN_TYPE_NULL:
        case MMCONN_TYPE_MULTICAST:
            MMPBX_TRC_ERROR("Invalid connection type: %s\n", mmConnGetTypeString(mmConn->type));
            rc = -1;
            break;
    }

    if (err != MMPBX_ERROR_NOERROR) {
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModRemoteMediaAddr(struct sk_buff   *skb_2,
                                                     struct genl_info *info)
{
    MmPbxError              err     = MMPBX_ERROR_NOERROR;
    MmConnHndl              mmConn  = NULL;
    struct nlattr           *nla;
    int                     rc = 0;
    struct sockaddr_storage remoteAddr;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attributes */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&remoteAddr, nla, sizeof(struct sockaddr_storage));

    switch (mmConn->type) {
        case MMCONN_TYPE_RTCP:
            err = mmConnRtcpModRemoteMediaAddr((MmConnRtcpHndl) mmConn, &remoteAddr);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRtcpModRemoteMediaAddr failed: %s\n", mmPbxGetErrorString(err));
            }
            break;

        case MMCONN_TYPE_RELAY:
            err = mmConnRelayModRemoteAddr((MmConnRelayHndl) mmConn, &remoteAddr);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRelayModRemoteAddr failed: %s\n", mmPbxGetErrorString(err));
            }
            break;

        case MMCONN_TYPE_USER:
        case MMCONN_TYPE_KERNEL:
        case MMCONN_TYPE_TONE:
        case MMCONN_TYPE_NULL:
        case MMCONN_TYPE_MULTICAST:
            MMPBX_TRC_ERROR("Invalid connection type: %s\n", mmConnGetTypeString(mmConn->type));
            rc = -1;
            break;
    }

    if (err != MMPBX_ERROR_NOERROR) {
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModRemoteRtcpAddr(struct sk_buff    *skb_2,
                                                    struct genl_info  *info)
{
    MmPbxError              err     = MMPBX_ERROR_NOERROR;
    MmConnHndl              mmConn  = NULL;
    struct nlattr           *nla;
    int                     rc = 0;
    struct sockaddr_storage remoteRtcpAddr;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attributes */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_REMOTE_RTCP_ADDR];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&remoteRtcpAddr, nla, sizeof(struct sockaddr_storage));

    switch (mmConn->type) {
        case MMCONN_TYPE_RTCP:
            err = mmConnRtcpModRemoteRtcpAddr((MmConnRtcpHndl)mmConn, &remoteRtcpAddr);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRtcpModRemoteRtcpAddr failed: %s\n", mmPbxGetErrorString(err));
            }
            break;

        case MMCONN_TYPE_RELAY:
            err = mmConnGetConnControl(mmConn, &mmConn);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnGetConnControl failed\n");
                break;
            }
            err = mmConnRelayModRemoteAddr((MmConnRelayHndl)mmConn, &remoteRtcpAddr);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRelayModRemoteAddr failed: %s\n", mmPbxGetErrorString(err));
            }
            break;

        case MMCONN_TYPE_USER:
        case MMCONN_TYPE_KERNEL:
        case MMCONN_TYPE_TONE:
        case MMCONN_TYPE_NULL:
        case MMCONN_TYPE_MULTICAST:
            MMPBX_TRC_ERROR("Invalid connection type: %s\n", mmConnGetTypeString(mmConn->type));
            rc = -1;
            break;
    }

    if (err != MMPBX_ERROR_NOERROR) {
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpModRtcpBandwidth(struct sk_buff   *skb_2,
                                                   struct genl_info *info)
{
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    mmConn  = NULL;
    struct nlattr *nla;
    int           rc            = 0;
    int           rtcpBandwidth = 0;

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    nla = info->attrs[MMSWITCH_ATTR_RTCP_BANDWIDTH];
    if (!nla) {
        return -EINVAL;
    }

    rtcpBandwidth = nla_get_u32(nla);

    switch (mmConn->type) {
        case MMCONN_TYPE_RTCP:
            err = mmConnRtcpModRtcpBandwidth((MmConnRtcpHndl) mmConn, rtcpBandwidth);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRtcpModRtcpBandwidth failed: %s\n", mmPbxGetErrorString(err));
            }
            break;

        case MMCONN_TYPE_RELAY:
            MMPBX_TRC_DEBUG("Not implemented for connection of type %s\n", mmConnGetTypeString(mmConn->type));
            break;

        case MMCONN_TYPE_USER:
        case MMCONN_TYPE_KERNEL:
        case MMCONN_TYPE_TONE:
        case MMCONN_TYPE_NULL:
        case MMCONN_TYPE_MULTICAST:
            MMPBX_TRC_ERROR("Invalid connection type: %s\n", mmConnGetTypeString(mmConn->type));
            rc = -1;
            break;
    }

    if (err != MMPBX_ERROR_NOERROR) {
        rc = -1;
    }

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpGetStats(struct sk_buff   *skb_2,
                                           struct genl_info *info)
{
    MmPbxError      err     = MMPBX_ERROR_NOERROR;
    MmConnHndl      mmConn  = NULL;
    struct nlattr   *nla;
    int             rc        = 0;
    struct sk_buff  *skb      = NULL;
    void            *msg_head = NULL;
    MmConnRtcpStats stats     = { 0 };

    MMPBX_TRC_DEBUG("Entering\n");

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    if (mmConn == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -EINVAL;
    }

    switch (mmConn->type) {
        case MMCONN_TYPE_RTCP:
            err = mmConnRtcpGetStats((MmConnRtcpHndl)mmConn, &stats);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRtcpGetStats failed\n");
            }
            break;

        case MMCONN_TYPE_RELAY:
            err = mmConnGetConnControl(mmConn, &mmConn);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnGetConnControl failed\n");
                break;
            }
            err = mmConnRelayGetStats((MmConnRelayHndl)mmConn, &stats);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRelayGetStats failed\n");
                break;
            }
            break;

        case MMCONN_TYPE_USER:
        case MMCONN_TYPE_KERNEL:
        case MMCONN_TYPE_TONE:
        case MMCONN_TYPE_NULL:
        case MMCONN_TYPE_MULTICAST:
            MMPBX_TRC_ERROR("Invalid connection type: %s\n", mmConnGetTypeString(mmConn->type));
            rc = -1;
            break;
    }
    if (err != MMPBX_ERROR_NOERROR) {
        rc = -1;
    }
    if (rc != 0) {
        return rc;
    }

    do {
        /* send a message back*/
        /* allocate some memory, since the size is not yet known use NLMSG_GOODSIZE*/
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
        if (skb == NULL) {
            MMPBX_TRC_ERROR("genlmsg_new failed\n");
            rc = -ENOMEM;
            break;
        }

        /* create the message headers */
        /* arguments of genlmsg_put:
           struct sk_buff *,
           int (sending) pid,
           int sequence number,
           struct genl_family *,
           int flags,
           u8 command index (why do we need this?)
         */
        msg_head = genlmsg_put(skb, 0, info->snd_seq, &MmSwitchGeNlFamily, 0, MMSWITCH_CMD_MMCONNRTCP_GET_STATS);
        if (msg_head == NULL) {
            MMPBX_TRC_ERROR("genlmsg_put failed\n");
            rc = -ENOMEM;
            break;
        }

        /* Fill message with statistics */
        rc = mmSwitchFillRtcpStats(skb, &stats);
        if (rc != 0) {
            MMPBX_TRC_ERROR("mmSwitchFillRtcpStats failed\n");
            rc = -ENOMEM;
            break;
        }

        /* finalize the message */
        genlmsg_end(skb, msg_head);

        /* send the message back */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
        rc = genlmsg_unicast(&init_net, skb, info->snd_portid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
        rc = genlmsg_unicast(&init_net, skb, info->snd_pid);
#else
        rc = genlmsg_unicast(skb, info->snd_pid);
#endif

        if (rc != 0) {
            MMPBX_TRC_ERROR("genlmsg_unicast failed\n");
            break;
        }

        return rc;
    } while (0);
    genlmsg_cancel(skb, msg_head);
    nlmsg_free(skb);

    return rc;
}

/*
 *
 */
static int mmSwitchParseMmConnRtcpResetStats(struct sk_buff   *skb_2,
                                             struct genl_info *info)
{
    int           rc      = 0;
    MmPbxError    err     = MMPBX_ERROR_NOERROR;
    MmConnHndl    mmConn  = NULL;
    struct nlattr *nla;


    MMPBX_TRC_DEBUG("Called\n");

    if (info == NULL) {
        MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
        return -1;
    }

    /*Parse attribute */
    nla = info->attrs[MMSWITCH_ATTR_REF_MMCONN];
    if (!nla) {
        return -EINVAL;
    }
    nla_memcpy(&mmConn, nla, sizeof(void *));

    if (mmConn == NULL) {
        return -EINVAL;
    }

    switch (mmConn->type) {
        case MMCONN_TYPE_RTCP:
            err = mmConnRtcpResetStats((MmConnRtcpHndl)mmConn);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRtcpResetStats failed: %s\n", mmPbxGetErrorString(err));
                rc = -1;
            }
            break;

        case MMCONN_TYPE_RELAY:
            err = mmConnGetConnControl(mmConn, &mmConn);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnGetConnControl failed\n");
                rc = -1;
                break;
            }
            err = mmConnRelayResetStats((MmConnRelayHndl)mmConn);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnRelayResetStats failed: %s\n", mmPbxGetErrorString(err));
                rc = -1;
                break;
            }
            break;

        case MMCONN_TYPE_USER:
        case MMCONN_TYPE_KERNEL:
        case MMCONN_TYPE_TONE:
        case MMCONN_TYPE_NULL:
        case MMCONN_TYPE_MULTICAST:
            MMPBX_TRC_ERROR("Invalid connection type: %s\n", mmConnGetTypeString(mmConn->type));
    }

    return rc;
}

/**
 *
 */
static int mmSwitchFillRtcpStats(struct sk_buff   *skb,
                                 MmConnRtcpStats  *pStats)
{
    struct nlattr *nest;


    do {
        nest = nla_nest_start(skb, MMSWITCH_ATTR_MMCONNRTCP);
        if (!nest) {
            break;
        }

        /*Add stats to message */
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_SESSIONDURATION, pStats->sessionDuration) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_TXRTPPACKETS, pStats->txRtpPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_TXRTPBYTES, pStats->txRtpBytes) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXRTPPACKETS, pStats->rxRtpPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXRTPBYTES, pStats->rxRtpBytes) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXTOTALPACKETSLOST, pStats->rxTotalPacketsLost) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXTOTALPACKETSDISCARDED, pStats->rxTotalPacketsDiscarded) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXPACKETSLOSTRATE, pStats->rxPacketsLostRate) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXPACKETSDISCARDEDRATE, pStats->rxPacketsDiscardedRate) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SIGNALLEVEL, pStats->signalLevel) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_NOISELEVEL, pStats->noiseLevel) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RERL, pStats->RERL) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RFACTOR, pStats->RFactor) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_EXTERNALRFACTOR, pStats->externalRFactor) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_MOSLQ, pStats->mosLQ) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_MOSCQ, pStats->mosCQ) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_MEANE2EDELAY, pStats->meanE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_WORSTE2EDELAY, pStats->worstE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_CURRENTE2EDELAY, pStats->currentE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXJITTER, pStats->rxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXMINJITTER, pStats->rxMinJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXMAXJITTER, pStats->rxMaxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXDEVJITTER, pStats->rxDevJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXMEANJITTER, pStats->meanRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_RXWORSTJITTER, pStats->worstRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_JITTER_BUFFER_OVERRUNS, pStats->jitterBufferOverruns) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_JITTER_BUFFER_UNDERRUNS, pStats->jitterBufferUnderruns) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_TXRTPPACKETS, pStats->remoteTxRtpPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_TXRTPBYTES, pStats->remoteTxRtpBytes) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXTOTALPACKETSLOST, pStats->remoteRxTotalPacketsLost) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXPACKETSLOSTRATE, pStats->remoteRxPacketsLostRate) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXPACKETSDISCARDEDRATE, pStats->remoteRxPacketsDiscardedRate) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SIGNALLEVEL, pStats->remoteSignalLevel) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_NOISELEVEL, pStats->remoteNoiseLevel) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RERL, pStats->remoteRERL) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RFACTOR, pStats->remoteRFactor) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_EXTERNALRFACTOR, pStats->remoteExternalRFactor) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MOSLQ, pStats->remoteMosLQ) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MOSCQ, pStats->remoteMosCQ) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MEANE2EDELAY, pStats->remoteMeanE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_WORSTE2EDELAY, pStats->remoteWorstE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_CURRENTE2EDELAY, pStats->remoteCurrentE2eDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXJITTER, pStats->remoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMINJITTER, pStats->minRemoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMAXJITTER, pStats->maxRemoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXDEVJITTER, pStats->devRemoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMEANJITTER, pStats->meanRemoteRxJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXWORSTJITTER, pStats->remoteRxWorstJitter) < 0) {
            break;
        }

        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_TXRTCPXRPACKETS, pStats->txRtcpXrPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXRTCPXRPACKETS, pStats->rxRtcpXrPackets) < 0) {
            break;
        }

        /*AT&T extension*/
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_TXRTCPPACKETS, pStats->txRtcpPackets) < 0) {
            break;
        }
        if (nla_put_u64(skb, MMSWITCH_ATTR_MMCONNRTCP_RXRTCPPACKETS, pStats->rxRtcpPackets) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_FRACTIONLOSS, pStats->localSumFractionLoss) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_FRACTIONLOSS, pStats->localSumSqrFractionLoss) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_FRACTIONLOSS, pStats->remoteSumFractionLoss) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_SQR_FRACTIONLOSS, pStats->remoteSumSqrFractionLoss) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_JITTER, pStats->localSumJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_JITTER, pStats->localSumSqrJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_JITTER, pStats->remoteSumJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_SQR_JITTER, pStats->remoteSumSqrJitter) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_E2EDELAY, pStats->sumRoundTripDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_E2EDELAY, pStats->sumSqrRoundTripDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_MAX_ONEWAYDELAY, pStats->maxOneWayDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_ONEWAYDELAY, pStats->sumOneWayDelay) < 0) {
            break;
        }
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_ONEWAYDELAY, pStats->sumSqrOneWayDelay) < 0) {
            break;
        }
#if MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES > 0
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_FIRST_RX_RTCP_LENGTH, pStats->rxRtcpFrameLength[0]) < 0) {
            break;
        }
        if (pStats->rxRtcpFrameLength[0] > 0) {
            nla_put(skb, MMSWITCH_ATTR_MMCONNRTCP_FIRST_RX_RTCP_BUFFER, pStats->rxRtcpFrameLength[0], pStats->rxRtcpFrameBuffer[0]);
            kfree(pStats->rxRtcpFrameBuffer[0]);
            pStats->rxRtcpFrameBuffer[0] = NULL;
        }

        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_FIRST_TX_RTCP_LENGTH, pStats->txRtcpFrameLength[0]) < 0) {
            break;
        }

        if (pStats->txRtcpFrameLength[0] > 0) {
            nla_put(skb, MMSWITCH_ATTR_MMCONNRTCP_FIRST_TX_RTCP_BUFFER, pStats->txRtcpFrameLength[0], pStats->txRtcpFrameBuffer[0]);
            kfree(pStats->txRtcpFrameBuffer[0]);
            pStats->txRtcpFrameBuffer[0] = NULL;
        }

#if MMCONNRTCPSTATS_NUMBER_OF_SAVED_FRAMES > 1
        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SECOND_RX_RTCP_LENGTH, pStats->rxRtcpFrameLength[1]) < 0) {
            break;
        }
        if (pStats->rxRtcpFrameLength[1] > 0) {
            nla_put(skb, MMSWITCH_ATTR_MMCONNRTCP_SECOND_RX_RTCP_BUFFER, pStats->rxRtcpFrameLength[1], pStats->rxRtcpFrameBuffer[1]);
            kfree(pStats->rxRtcpFrameBuffer[1]);
            pStats->rxRtcpFrameBuffer[1] = NULL;
        }


        if (nla_put_u32(skb, MMSWITCH_ATTR_MMCONNRTCP_SECOND_TX_RTCP_LENGTH, pStats->txRtcpFrameLength[1]) < 0) {
            break;
        }
        if (pStats->txRtcpFrameLength[1] > 0) {
            nla_put(skb, MMSWITCH_ATTR_MMCONNRTCP_SECOND_TX_RTCP_BUFFER, pStats->txRtcpFrameLength[1], pStats->txRtcpFrameBuffer[1]);
            kfree(pStats->txRtcpFrameBuffer[1]);
            pStats->txRtcpFrameBuffer[1] = NULL;
        }
#endif
#endif

        nla_nest_end(skb, nest);

        return 0;
    } while (0);

    MMPBX_TRC_ERROR("an error occured in %s\n", __func__);
    return -1;
}
