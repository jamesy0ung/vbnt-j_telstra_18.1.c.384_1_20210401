#if defined(CONFIG_BCM_KF_EMMC)
/**************************************************************
 * sdhci-bcm63xx.c Support for SDHCI on Broadcom DSL/PON CPE SoC's
 *
 * Author: Farhan Ali <fali@broadcom.com>
 * Based on sdhci-brcmstb.c
 *
 * Copyright (c) 2014 Broadcom Corporation
 * All Rights Reserved
 *
 * <:label-BRCM:2014:DUAL/GPL:standard
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 *
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 *
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 *
 * :>
 *
 ************************************************************/
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mmc/host.h>
#include <linux/module.h>
#include <bcm_map_part.h>

#include "sdhci.h"
#include "sdhci-pltfm.h"
   
#define SDHCI_BCM63XX_FORCE_LOW_PERFORMANCE 0

static struct sdhci_pltfm_data sdhci_bcm63xx_pdata = {
#if defined(CONFIG_BCM963138)
	/* Quirks and ops defined here will be passed to sdhci_host structure */
	.quirks = 0
#if SDHCI_BCM63XX_FORCE_LOW_PERFORMANCE	
		| SDHCI_QUIRK_FORCE_1_BIT_DATA
		| SDHCI_QUIRK_BROKEN_ADMA
		| SDHCI_QUIRK_NO_MULTIBLOCK
#endif		
		,
	.quirks2 = 0
#if SDHCI_BCM63XX_FORCE_LOW_PERFORMANCE
		| SDHCI_QUIRK2_BROKEN_DDR50
#endif		
		,
#endif
};

static const struct of_device_id sdhci_bcm63xx_of_match[] = {
	{ .compatible = "brcm,bcm63xx-sdhci"},
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_bcm63xx_of_match);

#if defined(CONFIG_BCM963138)
#include <bcm_intr.h>
static struct resource bcm63138_emmc_resources[] = {
	[0] = 	{
			.start = EMMC_HOSTIF_PHYS_BASE,
			.end = EMMC_HOSTIF_PHYS_BASE + SZ_256 - 1,  /* we only need this area */
			/* the memory map actually makes SZ_4K available  */
			.flags = IORESOURCE_MEM,
		},
	[1] =	{
			.start = INTERRUPT_ID_EMMC,
			.end = INTERRUPT_ID_EMMC,
			.flags = IORESOURCE_IRQ,
		},
};
#endif

static int sdhci_bcm63xx_probe(struct platform_device *pdev)
{
	int res = 0;
	struct sdhci_host *host;
	struct sdhci_pltfm_host *pltfm_host;

	/* Force straps to enable emmc signals - must prevent platform device register if not using emmc */
	AHBSS_CTRL->ahbss_ctrl_cfg |= FORCE_EMMC_BOOT_STRAP;
	EMMC_TOP_CFG->emmc_top_cfg_sd_pin_sel = 1;
	
	/* Check if we are in normal mode, if not then force us in normal mode */
	while( EMMC_BOOT->emmc_boot_status & EMMC_BOOT_MODE_MASK )
	{
		EMMC_BOOT->emmc_boot_main_ctl &= ~EMMC_BOOT_ENABLE;		
	}
	
#if defined(CONFIG_BCM963138)
	/* For 63138 the interrupt controller is not listed in the device tree
	 * therefore all interrupt related bindings will fail. We therefore have 
	 * to manually insert the IRQs by redefining the driver resources
	 */
	pdev->resource = bcm63138_emmc_resources;
	pdev->num_resources = ARRAY_SIZE(bcm63138_emmc_resources);
#endif	

	host = sdhci_pltfm_init(pdev, &sdhci_bcm63xx_pdata, sizeof(&sdhci_bcm63xx_pdata));
	if (IS_ERR(host))
		return PTR_ERR(host);
	
	/* Get pltfm host */
	pltfm_host = sdhci_priv(host);

	res = mmc_of_parse(host->mmc);
	if (res)
		goto err_pltfm_free;

	res = sdhci_add_host(host);

err_pltfm_free:	
	if (res)
		sdhci_pltfm_free(pdev);
	
	return res;
}

static int sdhci_bcm63xx_remove(struct platform_device *pdev)
{
	int res;
	res = sdhci_pltfm_unregister(pdev);
	return res;
}

static struct platform_driver sdhci_bcm63xx_driver = {
	.driver		= {
		.name	= "sdhci-bcm63xx",
		.owner	= THIS_MODULE,
		.of_match_table = sdhci_bcm63xx_of_match,
	},
	.probe		= sdhci_bcm63xx_probe,
	.remove		= sdhci_bcm63xx_remove,
};

module_platform_driver(sdhci_bcm63xx_driver);

MODULE_DESCRIPTION("SDHCI driver for Broadcom DSL/PON CPE devices");
MODULE_AUTHOR("Farhan Ali <fali@broadcom.com>");
MODULE_LICENSE("GPL v2");
#endif /* CONFIG_BCM_KF_EMMC */
