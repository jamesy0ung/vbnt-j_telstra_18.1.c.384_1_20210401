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

#include "phy_drv.h"
#include "phy_bp_parsing.h"
#include "boardparms.h"

static bus_type_t bp_parse_bus_type(const ETHERNET_MAC_INFO *emac_info, uint32_t port)
{
    bus_type_t bus_type = BUS_TYPE_UNKNOWN;
    uint32_t phy_id = emac_info->sw.phy_id[port];
    uint32_t intf = phy_id & MAC_IFACE;

    switch (intf)
    {
    case MAC_IF_MII:
    case MAC_IF_GMII:
    case MAC_IF_SERDES:
    case MAC_IF_SGMII:
    case MAC_IF_HSGMII:
    {
        bus_type = BUS_TYPE_6848_INT;
        break;
    }
    default:
        break;
    }

    return bus_type;
}

void bp_parse_aux_phy(const ETHERNET_MAC_INFO *emac_info, uint32_t port, phy_dev_t *phy_dev)
{
    phy_dev_t *aux_phy;
    uint32_t phy_id = emac_info->sw.phy_id[port];
    uint32_t intf = phy_id & MAC_IFACE;

    if ((phy_id & PHY_EXTERNAL) && (intf == MAC_IF_SGMII || intf == MAC_IF_HSGMII))
    {
        aux_phy = phy_dev_add(PHY_TYPE_6848_SGMII, 6, NULL);
        aux_phy->phy_drv->bus_drv = bus_drv_get(BUS_TYPE_6848_INT);
        aux_phy->mii_type = intf == MAC_IF_SGMII ? PHY_MII_TYPE_SGMII : PHY_MII_TYPE_HSGMII;
        phy_drv_init(aux_phy->phy_drv);
        phy_dev_init(aux_phy);
        phy_dev->aux_phy = aux_phy;
    }
}

void bp_parse_phy_driver(const ETHERNET_MAC_INFO *emac_info, uint32_t port, phy_drv_t *phy_drv)
{
    bus_type_t bus_type;

    if ((bus_type = bp_parse_bus_type(emac_info, port)) != BUS_TYPE_UNKNOWN)
        phy_drv->bus_drv = bus_drv_get(bus_type);
}

phy_type_t bp_parse_phy_type(const ETHERNET_MAC_INFO *emac_info, uint32_t port)
{
    phy_type_t phy_type = PHY_TYPE_UNKNOWN;
    uint32_t phy_id = emac_info->sw.phy_id[port];
    uint32_t intf = phy_id & MAC_IFACE;
    uint32_t addr = phy_id & BCM_PHY_ID_M;

    switch (intf)
    {
    case MAC_IF_MII:
    case MAC_IF_GMII:
    {
        if (addr == 1 || addr == 2)
            phy_type = PHY_TYPE_6848_EGPHY;
        else if (addr == 3 || addr == 4)
            phy_type = PHY_TYPE_6848_EPHY;
        break;
    }
    case MAC_IF_SERDES:
    {
        phy_type = PHY_TYPE_PCS;
        break;
    }
    case MAC_IF_SGMII:
    case MAC_IF_HSGMII:
    {
        if (phy_id & PHY_EXTERNAL)
            phy_type = PHY_TYPE_EXT1;
        else
            phy_type = PHY_TYPE_6848_SGMII;
        break;
    }
    default:
        break;
    }

    return phy_type;
}

void *bp_parse_phy_priv(const ETHERNET_MAC_INFO *emac_info, uint32_t port)
{
    uint32_t priv = 0;
    uint32_t phy_id = emac_info->sw.phy_id[port];
    uint32_t intf = phy_id & MAC_IFACE;

    switch (intf)
    {
    case MAC_IF_MII:
    case MAC_IF_GMII:
    {
        /* EGPHY ports 0,1 are connected to EMAC ports 0,1 */
        if (port == 0 || port == 1)
            priv = port;

        /* EPHY ports 0,1 are connected to EMAC ports 2,3 */
        if (port == 2 || port == 3)
            priv = port - 2;

        break;
    }
    default:
        break;
    }

    return (void *)priv;
}

mac_type_t bp_parse_mac_type(const ETHERNET_MAC_INFO *emac_info, uint32_t port)
{
    if (port == 4 || port > 5)
        return MAC_TYPE_UNKNOWN;

    return MAC_TYPE_UNIMAC;
}

void *bp_parse_mac_priv(const ETHERNET_MAC_INFO *emac_info, uint32_t port)
{
    uint32_t priv = 0;

    /* Set the gmii_direct bit in the unimac cfg register */
    if (port >= 0 && port <= 3)
        priv = 1;

    return (void *)priv;
}
