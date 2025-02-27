/*
   Copyright (c) 2015 Broadcom Corporation
   All Rights Reserved

    <:label-BRCM:2015:DUAL/GPL:standard

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

/*
 *  Created on: Dec 2015
 *      Author: yuval.raviv@broadcom.com
 */

/*
 * Phy driver for 6838/6848 AE PCS
 */

#include "bus_drv.h"
#include "phy_drv.h"
#include "phy_drv_mii.h"

#define PCS_XCTL1           0x10
#define PCS_XCTL2           0x11
#define PCS_XCTL3           0x12
#define PCS_XCTL4           0x13
#define PCS_XSTAT1          0x14
#define PCS_XSTAT2          0x15
#define PCS_XSTAT3          0x16

static int pcs_read_status(phy_dev_t *phy_dev)
{
    uint16_t val;
    int ret, speed, duplex;

    phy_dev->link = 0;
    phy_dev->speed = PHY_SPEED_UNKNOWN;
    phy_dev->duplex = PHY_DUPLEX_UNKNOWN;

    if ((ret = phy_bus_read(phy_dev, PCS_XSTAT1, &val)))
        return ret;

    phy_dev->link = ((val >> 1) & 0x1);

    if (!phy_dev->link)
        return 0;

    speed = ((val >> 3) & 0x3);
    duplex = ((val >> 2) & 0x1);

    if (speed == 0)
        phy_dev->speed = PHY_SPEED_10;
    else if (speed == 1)
        phy_dev->speed = PHY_SPEED_100;
    else if (speed == 2)
        phy_dev->speed = PHY_SPEED_1000;

    phy_dev->duplex = duplex ? PHY_DUPLEX_FULL : PHY_DUPLEX_HALF;

    return 0;
}

static int pcs_speed_set(phy_dev_t *phy_dev, phy_speed_t speed, phy_duplex_t duplex)
{
    udelay(10000);
    phy_dev_read_status(phy_dev);

    return 0;
}

static int pcs_init(phy_dev_t *phy_dev)
{
    uint16_t val;
    int ret;

    /* Reset phy */
    if ((ret = phy_bus_write(phy_dev, MII_BMCR, BMCR_RESET)))
        goto Exit;

    /* autodet_en */
    val = 0x4100;
    if (phy_dev->mii_type == PHY_MII_TYPE_SGMII)
        val |= 0x10; /* SGMII master mode */
    else
        val |= 0x01; /* Fiber mode (1000X) */

    if ((ret = phy_bus_write(phy_dev, PCS_XCTL1, val)))
        goto Exit;

    if ((ret = phy_bus_read(phy_dev, PCS_XCTL4, &val)))
        goto Exit;

    /* Turn on support of 100/10 */
    if ((ret = phy_bus_write(phy_dev, PCS_XCTL4, val | 0x0800)))
        goto Exit;

    /* Now finish to configure BMCR, different if SGMII phy is connected */
    val = BMCR_FULLDPLX | BMCR_SPEED1000;
    if (phy_dev->mii_type == PHY_MII_TYPE_SGMII)
        val |= BMCR_ANENABLE;

    if ((ret = phy_bus_write(phy_dev, MII_BMCR, val)))
        goto Exit;

Exit:
    return ret;
}

phy_drv_t phy_drv_pcs =
{
    .phy_type = PHY_TYPE_PCS,
    .name = "PCS",
    .power_set = mii_power_set,
    .read_status = pcs_read_status,
    .speed_set = pcs_speed_set,
    .init = pcs_init,
};
