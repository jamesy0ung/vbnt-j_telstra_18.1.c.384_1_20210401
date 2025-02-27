/*
<:copyright-BRCM:2012:DUAL/GPL:standard

   Copyright (c) 2012 Broadcom 
   All Rights Reserved

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
*/


//**************************************************************************
// File Name  : bcmgmac.c
//
// Description: This is Linux network driver for Broadcom GMAC controller
//
//**************************************************************************

#define VERSION     "0.1"
#define VER_STR     "v" VERSION

#define _BCMENET_LOCAL_

#include <linux/types.h>
#include <linux/bcm_log_mod.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include <board.h>
#include "bcmgmacctl.h"
#include "bcmgmac_4908.h"
#include "bcmmii.h"

/*----- Globals -----*/
#undef GMAC_DECL
#define GMAC_DECL(x) #x,

const char *gmacctl_ioctl_name[] =
{
    GMAC_DECL(GMACCTL_IOCTL_SYS)
    GMAC_DECL(GMACCTL_IOCTL_MAX)
};

const char *gmacctl_subsys_name[] =
{
    GMAC_DECL(GMACCTL_SUBSYS_STATUS)
    GMAC_DECL(GMACCTL_SUBSYS_MODE)
    GMAC_DECL(GMACCTL_SUBSYS_MAX)
};

const char *gmacctl_op_name[] =
{   
    GMAC_DECL(GMACCTL_OP_GET)
    GMAC_DECL(GMACCTL_OP_SET)
    GMAC_DECL(GMACCTL_OP_DUMP)
    GMAC_DECL(GMACCTL_OP_MAX)
};

DEFINE_SPINLOCK(gmac_lock_g);
EXPORT_SYMBOL(gmac_lock_g);

#if defined(CONFIG_SMP) || defined(CONFIG_PREEMPT)
#define GMAC_LOCK_BH()              spin_lock_bh( &gmac_lock_g )
#define GMAC_UNLOCK_BH()            spin_unlock_bh( &gmac_lock_g )
#else
#define GMAC_LOCK_BH()              local_bh_disable()
#define GMAC_UNLOCK_BH()            local_bh_enable()
#endif

#if defined(CONFIG_BLOG) 
extern int fcacheDrvFlushAll( void );

#if (defined(CONFIG_SMP) || defined(CONFIG_PREEMPT))
extern spinlock_t blog_lock_g;
#define BLOG_LOCK_BH()      spin_lock_bh( &blog_lock_g )
#define BLOG_UNLOCK_BH()    spin_unlock_bh( &blog_lock_g )
#else
#define BLOG_LOCK_BH()      local_bh_disable()
#define BLOG_UNLOCK_BH()    local_bh_enable()
#endif
#else
#define BLOG_LOCK_BH()      local_bh_disable()
#define BLOG_UNLOCK_BH()    local_bh_enable()
#endif

/* sets the GMAC to be active, and ROBO port to be inactive */
int gmac_set_active( void )
{
    BLOG_LOCK_BH();
    /* Now select GMAC at PHY3 */
#if defined(CONFIG_BCM963268)
    /* Disable ROBO RX */
    //enet_set_port_ctrl( GMAC_PORT_ID, 1 );

    printk("Select GMAC at Mux (set bit18=0x40000)\n" ); 
    GPIO->RoboswGphyCtrl |= GPHY_MUX_SEL_GMAC;

    printk("\tGPIORoboswGphyCtrl<0x%p>=0x%x\n", 
        &GPIO->RoboswGphyCtrl, (uint32_t) GPIO->RoboswGphyCtrl );
#endif

    /* Enable GMAC Tx and Rx */
    printk("Enable GMAC Rx & Tx (set bitMask 0x03)\n" ); 
    GMAC_MAC->Cmd.tx_ena = 1;
    GMAC_MAC->Cmd.rx_ena = 1;

    BLOG_UNLOCK_BH();

    printk("GMAC Activated \n");

    return 0;
}

/* Reads the stats from GMAC Regs */
void gmac_hw_stats( struct rtnl_link_stats64 *stats)
{
    volatile GmacMIBRegs *e = (volatile GmacMIBRegs *)GMAC_MIB;

    stats->rx_packets = e->RxPkts;
    stats->rx_bytes =  e->RxOctetsLo;
    stats->multicast = e->RxMulticastPkts;
    stats->rx_broadcast_packets = e->RxBroadcastPkts;		
    stats->rx_dropped =  (e->RxPkts - e->RxGoodPkts);
    stats->rx_errors =  
        (e->RxFCSErrs + e->RxAlignErrs + e->RxSymbolError);
        
    stats->tx_packets =  e->TxPkts;
    stats->tx_bytes =  e->TxOctetsLo;
    stats->tx_multicast_packets = e->TxMulticastPkts;
    stats->tx_broadcast_packets = e->TxBroadcastPkts;		
    stats->tx_dropped =  (e->TxPkts - e->TxGoodPkts);
}

void extsw_wreg_mmap(int page, int reg, uint8 *data_in, int len)
{
    uint32 addr = ((page << 8) + reg) << 2;
    uint32 val = 0;
    uint64 data64;
    //volatile unsigned *switch_directWriteReg  = (unsigned int *) (SWITCH_BASE + SF2_DIRECT_DATA_WRITE_REG);
    void *data = &data64;
    // Add ASSERTs if address is not aligned for the requested len.

    volatile uint32 *base = (volatile uint32 *) (SWITCH_BASE + addr);
    memcpy(data, data_in, len);

    switch (len) {
        case 1:
            val = *(uint8 *)data;
            break;
        case 2:
            val = *(uint16 *)data;
            break;
        case 4:
            val = *(uint32 *)data;
            break;
            #if 0
        case 6:
            //[] = m0m1m2m3m4m5????
            *switch_directWriteReg = (data64 >> 32) & 0xffff;
            val = (uint32 )data64;
            break;
        case 8:
            // for mac vid case, you have
            // m0m1m2m3m4m5,vidL,vidM
            *switch_directWriteReg = data64 >> 32;
            val = (uint32 )data64;
            break;
            #endif
        default:
            printk("%s: len = %d NOT Handled !! \n", __FUNCTION__, len);
            return;
    }
    *base = val;
}

void extsw_rreg_mmap(int page, int reg, uint8 *data_out, int len)
{
    uint32 addr = ((page << 8) + reg) << 2;
    uint32 val;
    uint64 data64 = 0;
    //volatile unsigned *switch_directReadReg   = (unsigned int *) (SWITCH_BASE + SF2_DIRECT_DATA_READ_REG);
    void *data = &data64;
    // Add ASSERTs if address is not aligned for the requested len.

    volatile uint32 *base = (volatile uint32 *) (SWITCH_BASE + addr);

    val = *base;

    switch (len) {
        case 1:
            *(uint32 *)data = (uint8)val;
            break;
        case 2:
            *(uint32 *)data = (uint16)val;
            break;
        case 4:
            *(uint32 *)data = val;
            break;
        case 6:
            //*(uint64 *)data    = val | ((uint64)(*switch_directReadReg & 0xffff)) << 32;
            break;
        case 8:
            //*(uint64 *)data = val | ((uint64)*switch_directReadReg) << 32;
            break;
        default:
            printk("%s: len = %d NOT Handled !! \n", __FUNCTION__, len);
            break;
    }
    memcpy(data_out, data, len);
}

#define extsw_rreg_wrap(a, b, v, l) extsw_rreg_mmap(a, b, (uint8*)v, (int)l)
static int sf2_bcmsw_dump_mib_ext(int port, int type)
{
    unsigned int v32, errcnt;
    //uint8 data[8] = {0};

    /* Display Tx statistics */
    printk("External Switch Stats : Port# %d\n",port);
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXUPKTS, &v32, 4);  // Get TX unicast packet count
    printk("TxUnicastPkts:          %10u \n", v32);
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXMPKTS, &v32, 4);  // Get TX multicast packet count
    printk("TxMulticastPkts:        %10u \n",  v32);
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXBPKTS, &v32, 4);  // Get TX broadcast packet count
    printk("TxBroadcastPkts:        %10u \n", v32);
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXDROPS, &v32, 4);
    printk("TxDropPkts:             %10u \n", v32);

    if (type)
    {
    /*
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXOCTETS, data, DATA_TYPE_MIB_COUNT|8);
        v32 = (((uint32*)data)[0]);
        printk("TxOctetsLo:             %10u \n", v32);
        v32 = (((uint32*)data)[1]);
        printk("TxOctetsHi:             %10u \n", v32);
        */
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TX64OCTPKTS, &v32, 4);
        printk("TxPkts64Octets:         %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TX127OCTPKTS, &v32, 4);
        printk("TxPkts65to127Octets:    %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TX255OCTPKTS, &v32, 4);
        printk("TxPkts128to255Octets:   %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TX511OCTPKTS, &v32, 4);
        printk("TxPkts256to511Octets:   %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TX1023OCTPKTS, &v32, 4);
        printk("TxPkts512to1023Octets:  %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXMAXOCTPKTS, &v32, 4);
        printk("TxPkts1024OrMoreOctets: %10u \n", v32);
//
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXQ0PKT, &v32, 4);
        printk("TxQ0Pkts:               %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXQ1PKT, &v32, 4);
        printk("TxQ1Pkts:               %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXQ2PKT, &v32, 4);
        printk("TxQ2Pkts:               %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXQ3PKT, &v32, 4);
        printk("TxQ3Pkts:               %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXQ4PKT, &v32, 4);
        printk("TxQ4Pkts:               %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXQ5PKT, &v32, 4);
        printk("TxQ5Pkts:               %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXQ6PKT, &v32, 4);
        printk("TxQ6Pkts:               %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXQ7PKT, &v32, 4);
        printk("TxQ7Pkts:               %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXCOL, &v32, 4);
        printk("TxCol:                  %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXSINGLECOL, &v32, 4);
        printk("TxSingleCol:            %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXMULTICOL, &v32, 4);
        printk("TxMultipleCol:          %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXDEFERREDTX, &v32, 4);
        printk("TxDeferredTx:           %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXLATECOL, &v32, 4);
        printk("TxLateCol:              %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXEXCESSCOL, &v32, 4);
        printk("TxExcessiveCol:         %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXFRAMEINDISC, &v32, 4);
        printk("TxFrameInDisc:          %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXPAUSEPKTS, &v32, 4);
        printk("TxPausePkts:            %10u \n", v32);
    }
    else
    {
        errcnt=0;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXCOL, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXSINGLECOL, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXMULTICOL, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXDEFERREDTX, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXLATECOL, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXEXCESSCOL, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_TXFRAMEINDISC, &v32, 4);
        errcnt += v32;
        printk("TxOtherErrors:          %10u \n", errcnt);
    }

    /* Display Rx statistics */
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXUPKTS, &v32, 4);  // Get RX unicast packet count
    printk("RxUnicastPkts:          %10u \n", v32);
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXMPKTS, &v32, 4);  // Get RX multicast packet count
    printk("RxMulticastPkts:        %10u \n",v32);
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXBPKTS, &v32, 4);  // Get RX broadcast packet count
    printk("RxBroadcastPkts:        %10u \n",v32);
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXDROPS, &v32, 4);
    printk("RxDropPkts:             %10u \n",v32);
    extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXDISCARD, &v32, 4);
    printk("RxDiscard:              %10u \n", v32);

    if (type)
    {
    /*
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXOCTETS, data, DATA_TYPE_MIB_COUNT|8);
        v32 = (((uint32*)data)[0]);
        printk("RxOctetsLo:             %10u \n", v32);
        v32 = (((uint32*)data)[1]);
        printk("RxOctetsHi:             %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXGOODOCT, data, DATA_TYPE_MIB_COUNT|8);
        v32 = (((uint32*)data)[0]);
        printk("RxGoodOctetsLo:         %10u \n", v32);
        v32 = (((uint32*)data)[1]);
        */
        printk("RxGoodOctetsHi:         %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXJABBERS, &v32, 4);
        printk("RxJabbers:              %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXALIGNERRORS, &v32, 4);
        printk("RxAlignErrs:            %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXFCSERRORS, &v32, 4);
        printk("RxFCSErrs:              %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXFRAGMENTS, &v32, 4);
        printk("RxFragments:            %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXOVERSIZE, &v32, 4);
        printk("RxOversizePkts:         %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXUNDERSIZEPKTS, &v32, 4);
        printk("RxUndersizePkts:        %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXPAUSEPKTS, &v32, 4);
        printk("RxPausePkts:            %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXSACHANGES, &v32, 4);
        printk("RxSAChanges:            %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXSYMBOLERRORS, &v32, 4);
        printk("RxSymbolError:          %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RX64OCTPKTS, &v32, 4);
        printk("RxPkts64Octets:         %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RX127OCTPKTS, &v32, 4);
        printk("RxPkts65to127Octets:    %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RX255OCTPKTS, &v32, 4);
        printk("RxPkts128to255Octets:   %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RX511OCTPKTS, &v32, 4);
        printk("RxPkts256to511Octets:   %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RX1023OCTPKTS, &v32, 4);
        printk("RxPkts512to1023Octets:  %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXMAXOCTPKTS, &v32, 4);
        printk("RxPkts1024OrMoreOctets: %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXJUMBOPKT , &v32, 4);
        printk("RxJumboPkts:            %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXOUTRANGEERR, &v32, 4);
        printk("RxOutOfRange:           %10u \n", v32);
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXINRANGEERR, &v32, 4);
        printk("RxInRangeErr:           %10u \n", v32);
    }
    else
    {
        errcnt=0;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXJABBERS, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXALIGNERRORS, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXFCSERRORS, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXFRAGMENTS, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXOVERSIZE, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXUNDERSIZEPKTS, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXSYMBOLERRORS, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXOUTRANGEERR, &v32, 4);
        errcnt += v32;
        extsw_rreg_wrap(PAGE_MIB_P0 + (port), SF2_REG_MIB_P0_RXINRANGEERR, &v32, 4);
        errcnt += v32;
        printk("RxOtherErrors:          %10u \n", errcnt);

    }

    return 0;
}

void switch_dump(int type)
{
    //sf2_bcmsw_dump_mib_ext(1, 1);
    sf2_bcmsw_dump_mib_ext(2, type);
    //sf2_bcmsw_dump_mib_ext(3, 1);
    sf2_bcmsw_dump_mib_ext(8, type);
}

/* Dumps the MIB from GMAC MIB Regs */
int gmac_dump_mib(int type)
{
    volatile GmacMIBRegs *e = (volatile GmacMIBRegs *)GMAC_MIB;

    switch_dump(type);
    /* Display Tx statistics */
    printk("\n");
    printk("TxUnicastPkts:          %10u \n", e->TxUnicastPkts);
    printk("TxMulticastPkts:        %10u \n",  e->TxMulticastPkts);
    printk("TxBroadcastPkts:        %10u \n", e->TxBroadcastPkts);
    printk("TxDropPkts:             %10u \n", (e->TxPkts - e->TxGoodPkts));

    /* Display remaining tx stats only if requested */
    if (type) {
        printk("TxOctetsLo:             %10u \n", e->TxOctetsLo);
        printk("TxOctetsHi:             %10u \n", 0);
        printk("TxQoSPkts:              %10u \n", e->TxGoodPkts);
        printk("TxCol:                  %10u \n", e->TxCol);
        printk("TxSingleCol:            %10u \n", e->TxSingleCol);
        printk("TxMultipleCol:          %10u \n", e->TxMultipleCol);
        printk("TxDeferredTx:           %10u \n", e->TxDeferredTx);
        printk("TxLateCol:              %10u \n", e->TxLateCol);
        printk("TxExcessiveCol:         %10u \n", e->TxExcessiveCol);
        printk("TxFrameInDisc:          %10u \n", 0);
        printk("TxPausePkts:            %10u \n", e->TxPausePkts);
        printk("TxQoSOctetsLo:          %10u \n", e->TxOctetsLo);
        printk("TxQoSOctetsHi:          %10u \n", 0);
    }

    /* Display Rx statistics */
    printk("\n");
    printk("RxUnicastPkts:          %10u \n", e->RxUnicastPkts);
    printk("RxMulticastPkts:        %10u \n", e->RxMulticastPkts);
    printk("RxBroadcastPkts:        %10u \n", e->RxBroadcastPkts);
    printk("RxDropPkts:             %10u \n", (e->RxPkts - e->RxGoodPkts));

    /* Display remaining rx stats only if requested */
    if (type) {
        printk("RxJabbers:              %10u \n", e->RxJabbers);
        printk("RxAlignErrs:            %10u \n", e->RxAlignErrs);
        printk("RxFCSErrs:              %10u \n", e->RxFCSErrs);
        printk("RxFragments:            %10u \n", e->RxFragments);
        printk("RxOversizePkts:         %10u \n", e->RxOversizePkts);
        printk("RxExcessSizeDisc:       %10u \n", e->RxExcessSizeDisc);
        printk("RxOctetsLo:             %10u \n", e->RxOctetsLo);
        printk("RxOctetsHi:             %10u \n", 0);
        printk("RxUndersizePkts:        %10u \n", e->RxUndersizePkts);
        printk("RxPausePkts:            %10u \n", e->RxPausePkts);
        printk("RxGoodOctetsLo:         %10u \n", e->RxOctetsLo);
        printk("RxGoodOctetsHi:         %10u \n", 0);
        printk("RxSAChanges:            %10u \n", 0);
        printk("RxSymbolError:          %10u \n", e->RxSymbolError);
        printk("RxQoSPkts:              %10u \n", e->RxGoodPkts);
        printk("RxQoSOctetsLo:          %10u \n", e->RxOctetsLo);
        printk("RxQoSOctetsHi:          %10u \n", 0);
        printk("RxPkts64Octets:         %10u \n", e->Pkts64Octets);
        printk("RxPkts65to127Octets:    %10u \n", e->Pkts65to127Octets);
        printk("RxPkts128to255Octets:   %10u \n", e->Pkts128to255Octets);
        printk("RxPkts256to511Octets:   %10u \n", e->Pkts256to511Octets);
        printk("RxPkts512to1023Octets:  %10u \n", e->Pkts512to1023Octets);
        printk("RxPkts1024to1522Octets: %10u \n", 
            (e->Pkts1024to1518Octets + e->Pkts1519to1522));
        printk("RxPkts1523to2047:       %10u \n", e->Pkts1523to2047);
        printk("RxPkts2048to4095:       %10u \n", e->Pkts2048to4095);
        printk("RxPkts4096to8191:       %10u \n", e->Pkts4096to8191);
        printk("RxPkts8192to9728:       %10u \n", 0);
    }
    return 0;
}


/* Resets the GMAC MIB Regs */
void gmac_reset_mib( void )
{
    /* clear the MIB */
    GMAC_INTF->MibCtrl.clrMib = 1;
    GMAC_INTF->MibCtrl.clrMib = 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: gmac_drv_ioctl
 * Description  : Main entry point to handle user applications IOCTL requests
 *                GMAC Utility.
 * Returns      : 0 - success or error
 *------------------------------------------------------------------------------
 */
static int gmac_drv_ioctl(struct inode *inode, struct file *filep,
                       unsigned int command, unsigned long arg)
{
    gmacctl_ioctl_t cmd;
    gmacctl_data_t gmac;
    gmacctl_data_t *gmac_p = &gmac;
    int ret = GMAC_SUCCESS;

    if ( command > GMACCTL_IOCTL_MAX )
        cmd = GMACCTL_IOCTL_MAX;
    else
        cmd = (gmacctl_ioctl_t)command;

    copy_from_user( gmac_p, (uint8_t *) arg, sizeof(gmac) );

    printk(
        "cmd<%d>%s subsys<%d>%s op<%d>%s arg<0x%lx>\n",
        command, gmacctl_ioctl_name[command], 
        gmac_p->subsys, gmacctl_subsys_name[gmac_p->subsys],
        gmac_p->op, gmacctl_op_name[gmac_p->op], arg );

    GMAC_LOCK_BH();

    if (cmd == GMACCTL_IOCTL_SYS)
    {
        switch (gmac_p->subsys)
        {
            case GMACCTL_SUBSYS_MIB:
            {
                switch (gmac_p->op)
                {
                    case GMACCTL_OP_DUMP:
                        gmac_dump_mib(gmac_p->mib);
                        break;

                    default:
                        printk(
                            "Invalid op[%u]", gmac_p->op );
                        ret = GMAC_ERROR;
                }
                break;
            }

            default:
                printk(
                    "Invalid subsys[%u]", gmac_p->subsys );
                ret = GMAC_ERROR;
        }
    }
    else
    {
        printk( "Invalid cmd[%u]", command );
        ret = GMAC_ERROR;
    }

    GMAC_UNLOCK_BH();

    return ret;

} /* gmac_drv_ioctl */

static DEFINE_MUTEX(gmacIoctlMutex);

static long gmac_drv_ioctl_unlocked(struct file *filep, unsigned int cmd, 
                               unsigned long arg)
{
    struct inode *inode;
    long rt;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    inode = filep->f_dentry->d_inode;
#else
    inode = file_inode(filep);
#endif


    mutex_lock(&gmacIoctlMutex);
    rt = gmac_drv_ioctl(inode, filep, cmd, arg);
    mutex_unlock(&gmacIoctlMutex);

    return rt;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: gmac_drv_open
 * Description  : Called when a user application opens this device.
 * Returns      : 0 - success
 *------------------------------------------------------------------------------
 */
static int gmac_drv_open(struct inode *inode, struct file *filep)
{
    printk("Access GMAC Char Device\n" );
    return GMAC_SUCCESS;
} /* gmac_drv_open */

/* Global file ops */
static struct file_operations gmac_fops =
{
    .unlocked_ioctl = gmac_drv_ioctl_unlocked,
    .open   = gmac_drv_open,
};

/*
 *------------------------------------------------------------------------------
 * Function Name: gmac_drv_construct
 * Description  : Initial function that is called at system startup that
 *                registers this device.
 * Returns      : None.
 *------------------------------------------------------------------------------
 */

static int gmac_drv_construct(void)
{
    if ( register_chrdev( GMAC_DRV_MAJOR, GMAC_DRV_NAME, &gmac_fops ) )
    {
        printk( "%s Unable to get major number <%d>\n", __FUNCTION__, GMAC_DRV_MAJOR);
        return GMAC_ERROR;
    }

    printk( " GMAC Char Driver Registered\n");

    return GMAC_DRV_MAJOR;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: gmac_drv_destruct
 * Description  : Final function that is called when the module is unloaded.
 * Returns      : None.
 *------------------------------------------------------------------------------
 */
static void gmac_drv_destruct(void)
{
    unregister_chrdev( GMAC_DRV_MAJOR, GMAC_DRV_NAME );

    printk( " GMAC Char Driver Unregistered\n");
}

#if defined(CONFIG_BCM963268)
static void gmac_init_63268( void )
{
    /* Set bits [14:12] to 0xD to achieve 1G over iuDMA */
    GPIO->RoboswSwitchCtrl = 
        (GPIO->RoboswSwitchCtrl & ~RSW_IUDMA_CLK_FREQ_MASK) |
        (3<<RSW_IUDMA_CLK_FREQ_SHIFT);

    printk(
        "Set bits[14:12]=0x3 RSW IUDMA CLK Freq<0x%p> = 0x%x\n", 
        &GPIO->RoboswSwitchCtrl, (unsigned int) GPIO->RoboswSwitchCtrl );

    /* Enable GMAC clock */
    PERF->blkEnables |= GMAC_CLK_EN;

    printk(
        "Enable GMAC clock (bit19=0x80000) blkEnables<0x%p>=0x%x\n", 
        &PERF->blkEnables, (unsigned int) PERF->blkEnables );

    MISC->miscIddqCtrl &= ~MISC_IDDQ_CTRL_GMAC;

    printk(
        "Cleared IDDQ bit (0x40000) miscIddqCtrl<0x%p> = 0x%x\n", 
        &MISC->miscIddqCtrl, (unsigned int) MISC->miscIddqCtrl );
}
#endif

#if defined(CONFIG_BCM94908)
static void gmac_init_default(void)
{
    /* Reset GMAC */
    GMAC_MAC->Cmd.sw_reset = 1;
    //usleep(20);
    GMAC_MAC->Cmd.sw_reset = 0;
   
    GMAC_INTF->Flush.txfifo_flush = 1;
    GMAC_INTF->Flush.rxfifo_flush = 1;

    GMAC_INTF->Flush.txfifo_flush = 0;
    GMAC_INTF->Flush.rxfifo_flush = 0;

    GMAC_INTF->MibCtrl.clrMib = 1;
    GMAC_INTF->MibCtrl.clrMib = 0;

    /* default CMD configuration */
    GMAC_MAC->Cmd.eth_speed = CMD_ETH_SPEED_1000;

    /* Disable Tx and Rx */
    GMAC_MAC->Cmd.tx_ena = 0;
    GMAC_MAC->Cmd.rx_ena = 0;

    GMAC_INTF->GmacStatus.auto_cfg_en = 1;
    GMAC_INTF->GmacStatus.hd = 0;
    GMAC_INTF->GmacStatus.eth_speed = 2;
    GMAC_INTF->GmacStatus.link_up = 1;

    return;
}
#endif /* 4908 */
#if defined(CONFIG_BCM963268)
void gmac_init_default( void )
{
    /* Reset GMAC */
    printk(
        "Toggling Cmd to SW Reset (clear bitMask=0x2000)\n" );
    GMAC_MAC->Cmd.sw_reset = 1;
    GMAC_MAC->Cmd.sw_reset = 0;

    printk(
        "\tCmd<0x%p>=0x%x\n", 
        &GMAC_MAC->Cmd.word, (int) GMAC_MAC->Cmd.word ); 

    printk(
        "Toggling MacSwReset (clear bitMask=0x07)\n" );
    GMAC_INTF->MacSwReset.txfifo_flush = 1;
    GMAC_INTF->MacSwReset.rxfifo_flush = 1;
    GMAC_INTF->MacSwReset.mac_sw_reset = 1;

    GMAC_INTF->MacSwReset.txfifo_flush = 0;
    GMAC_INTF->MacSwReset.rxfifo_flush = 0;
    GMAC_INTF->MacSwReset.mac_sw_reset = 0;

    printk(
        "\tMacSwReset<0x%p>=0x%x\n", 
        &GMAC_INTF->MacSwReset.word, (uint32_t) GMAC_INTF->MacSwReset.word );

    gmac_reset_mib();

    /* default CMD configuration */
    GMAC_MAC->Cmd.runt_filt_dis = 0;
    GMAC_MAC->Cmd.txrx_en_cfg = 0;
    GMAC_MAC->Cmd.tx_pause_ign = 0;
    GMAC_MAC->Cmd.rmt_loop_ena = 0;
    GMAC_MAC->Cmd.len_chk_dis = 1;
    GMAC_MAC->Cmd.ctrl_frm_ena = 0;
    GMAC_MAC->Cmd.ena_ext_cfg = 0;
    GMAC_MAC->Cmd.lcl_loop_ena = 0;
    GMAC_MAC->Cmd.hd_ena = 0;
    GMAC_MAC->Cmd.tx_addr_ins = 0;
    GMAC_MAC->Cmd.rx_pause_ign = 0;
    GMAC_MAC->Cmd.pause_fwd = 1;
    GMAC_MAC->Cmd.crc_fwd = 1;
    GMAC_MAC->Cmd.pad_rem_en = 0;
    GMAC_MAC->Cmd.promis_en = 1;
    GMAC_MAC->Cmd.eth_speed = CMD_ETH_SPEED_1000;

    /* Disable Tx and Rx */
    printk(
        "Disable Rx & Tx (set bitMask 0x00)\n" ); 
    GMAC_MAC->Cmd.tx_ena = 0;
    GMAC_MAC->Cmd.rx_ena = 0;

    printk("\tCmd<0x%p>=0x%x\n", 
        &GMAC_MAC->Cmd.word, (uint32_t) GMAC_MAC->Cmd.word ); 
}
#endif /* 63268 */

int gmac_init( void )
{
    if (gmac_drv_construct() == GMAC_ERROR)
        return GMAC_ERROR;
    printk(
        "============ GMAC Init Begins ============\n" ); 

#if defined(CONFIG_BCM963268)
    gmac_init_63268();
#elif defined(CONFIG_BCM94908)
#else
#error "ERROR - GMAC driver not supported for this chip"
#endif 

    gmac_init_default();

    printk(
        "=========== GMAC Init Ends ==============\n"); 
    
    printk( GMAC_MODNAME " Driver " GMAC_VER_STR " Initialized\n" ); 
    return GMAC_SUCCESS;
}

/*
 *------------------------------------------------------------------------------
 * Function     : gmac_exit
 * Description  : Destruction of GMAC driver.
 *------------------------------------------------------------------------------
 */
void gmac_exit(void)
{
    gmac_drv_destruct();
    printk( GMAC_MODNAME " Driver " GMAC_VER_STR " Uninitialized\n" ); 
}


EXPORT_SYMBOL( gmac_set_active );
EXPORT_SYMBOL( gmac_hw_stats );
EXPORT_SYMBOL( gmac_dump_mib );
EXPORT_SYMBOL( gmac_reset_mib );
EXPORT_SYMBOL( gmac_init );

