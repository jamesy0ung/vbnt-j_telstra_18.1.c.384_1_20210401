/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          **
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

/*
 * Generic LED driver for Broadcom platforms using the BCM LED api
 *
 * Copyright (C) 2014 Technicolor <linuxgw@technicolor.com>
 *
 */
#include <linux/version.h>

#include "board_led_defines.h"

#if defined(BCM_LED_FW)

#include <linux/platform_device.h>
#include <linux/module.h>
#include "bcm_led.h"
#include "tch_led.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
#define __devinit
#define __devexit
#define __devexit_p(x) x
#endif

/* Broadcom defined LEDs */
#define BCM_MAX_LEDS 60

static struct bcm_led bcmled_dsc[BCM_MAX_LEDS] = {
	/* Will be filled in dynamically */
};

static struct bcm_led_platform_data bcmled_pd = { \
	.leds	    = bcmled_dsc
};

/* Full board LED description */
static struct board board_desc = {
	.name = "Broadcom",
	.gpioleds = NULL,
	.shiftleds = NULL,
	.bcmleds = &bcmled_pd,
	.aggregleds = NULL
};



/**
 * Adds a Broadcom LED if defined in boardparms
 */
static void bcm_led_add_ifavailable(const char * name, enum tch_leds ledId) {
	if (!tch_led_is_available(ledId)) {
		return;
	}

	if (bcmled_pd.num_leds >= BCM_MAX_LEDS) {
		printk("ERROR: Too many BCM LEDs defined\n");
		return;
	}

	bcmled_dsc[bcmled_pd.num_leds].name = name;
	bcmled_dsc[bcmled_pd.num_leds].id = ledId;
	bcmled_pd.num_leds++;
}

/**
 * Returns the number of LEDs available
 */
const struct board * bcm_led_get_board_description(void) {
	bcmled_pd.num_leds = 0;
	bcm_led_add_ifavailable("power:green", kLedPowerGreen);
	bcm_led_add_ifavailable("power:red", kLedPowerRed);
	bcm_led_add_ifavailable("power:blue", kLedPowerBlue);
	bcm_led_add_ifavailable("power:white", kLedPowerWhite);
	bcm_led_add_ifavailable("broadband:green", kLedBroadbandGreen);
	bcm_led_add_ifavailable("broadband:red", kLedBroadbandRed);
	bcm_led_add_ifavailable("broadband2:green", kLedBroadband2Green);
	bcm_led_add_ifavailable("broadband2:red", kLedBroadband2Red);
	bcm_led_add_ifavailable("dect:green", kLedDectGreen);
	bcm_led_add_ifavailable("dect:red", kLedDectRed);
	bcm_led_add_ifavailable("dect:blue",kLedDectBlue);
	bcm_led_add_ifavailable("ethernet:green", kLedEthernetGreen);
	bcm_led_add_ifavailable("ethernet:red", kLedEthernetRed);
	bcm_led_add_ifavailable("ethernet:blue", kLedEthernetBlue);
	bcm_led_add_ifavailable("iptv:green", kLedIPTVGreen);
	bcm_led_add_ifavailable("wireless:green", kLedWirelessGreen);
	bcm_led_add_ifavailable("wireless:red", kLedWirelessRed);
	bcm_led_add_ifavailable("wireless:blue", kLedWirelessBlue);
	bcm_led_add_ifavailable("wireless:white", kLedWirelessWhite);
	bcm_led_add_ifavailable("wireless_5g:green", kLedWireless5GHzGreen);
	bcm_led_add_ifavailable("wireless_5g:red", kLedWireless5GHzRed);
	bcm_led_add_ifavailable("internet:green", kLedInternetGreen);
	bcm_led_add_ifavailable("internet:red", kLedInternetRed);
	bcm_led_add_ifavailable("internet:blue", kLedInternetBlue);
	bcm_led_add_ifavailable("internet:white", kLedInternetWhite);
	bcm_led_add_ifavailable("voip:green", kLedVoip1Green);
	bcm_led_add_ifavailable("voip:red", kLedVoip1Red);
	bcm_led_add_ifavailable("voip:blue", kLedVoip1Blue);
	bcm_led_add_ifavailable("voip:white", kLedVoip1White);
	bcm_led_add_ifavailable("voip2:green", kLedVoip2Green);
	bcm_led_add_ifavailable("voip2:red", kLedVoip2Red);
	bcm_led_add_ifavailable("wps:green", kLedWPSGreen);
	bcm_led_add_ifavailable("wps:red", kLedWPSRed);
	bcm_led_add_ifavailable("usb:green", kLedUsbGreen);
	bcm_led_add_ifavailable("upgrade:blue", kLedUpgradeBlue);
	bcm_led_add_ifavailable("upgrade:green", kLedUpgradeGreen);
	bcm_led_add_ifavailable("lte:green", kLedLteGreen);
	bcm_led_add_ifavailable("lte:red", kLedLteRed);
	bcm_led_add_ifavailable("lte:blue", kLedLteBlue);
	bcm_led_add_ifavailable("lte:white", kLedLteWhite);
	// In fact, lte is only one kind of mobile technologies. So the name "mobile" is more suitable
	// lte leds above are kept just for backward compatibility
	bcm_led_add_ifavailable("mobile:green", kLedMobileGreen);
	bcm_led_add_ifavailable("mobile:red", kLedMobileRed);
	bcm_led_add_ifavailable("mobile:blue", kLedMobileBlue);
	bcm_led_add_ifavailable("mobile:white", kLedMobileWhite);
	bcm_led_add_ifavailable("sfp:green", kLedSFPGreen);
	bcm_led_add_ifavailable("moca:green", kLedMoCAGreen);
	bcm_led_add_ifavailable("ambient1:white", kLedAmbient1White);
	bcm_led_add_ifavailable("ambient2:white", kLedAmbient2White);
	bcm_led_add_ifavailable("ambient3:white", kLedAmbient3White);
	bcm_led_add_ifavailable("ambient4:white", kLedAmbient4White);
	bcm_led_add_ifavailable("ambient5:white", kLedAmbient5White);

	return &board_desc;
}



static void bcmled_release(struct device *dev)
{
}

static struct platform_device bcmled_device = {
       .name                   = "bcm-led",
       .id                     = 0,
       .dev = {
		.release = bcmled_release,
	}
};

static void bcm_led_set(struct led_classdev *led_cdev,
                             enum led_brightness value)
{
	struct bcm_led_data *led_dat =
		container_of(led_cdev, struct bcm_led_data, cdev);

	if(value > led_cdev->max_brightness)
		value = led_cdev->max_brightness;
	led_dat->brightness = value;
	if (tch_led_set(led_dat->id, value) != 0) {
		printk("Failed to set led %d value %d\n", led_dat->id, value);
	}
}

static enum led_brightness bcm_led_get(struct led_classdev *led_cdev)
{
	struct bcm_led_data *led_dat =
		container_of(led_cdev, struct bcm_led_data, cdev);

	return led_dat->brightness;
}

static int __devinit bcm_led_probe(struct platform_device *pdev)
{
	int i;
	int err;
	struct bcm_led_data *led;
	struct bcm_led_platform_data *pdata = dev_get_platdata(&pdev->dev);

	led = devm_kzalloc(&pdev->dev, sizeof(struct bcm_led_data) * pdata->num_leds,
	                   GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, led);

	for(i = 0; i < pdata->num_leds; i++) {
		led[i].cdev.name = pdata->leds[i].name;
		led[i].cdev.max_brightness = tch_led_get_max_brightness(pdata->leds[i].id);
		led[i].cdev.brightness_set = bcm_led_set;
		led[i].cdev.brightness_get = bcm_led_get;
		led[i].cdev.default_trigger = pdata->leds[i].default_trigger;

		led[i].brightness = LED_OFF;
		led[i].id = pdata->leds[i].id;

	}

	for(i = 0; i < pdata->num_leds; i++) {
		err = led_classdev_register(&pdev->dev, &led[i].cdev);
		if (err)
			goto free_leds;
	}
	return 0;

free_leds:
	while( --i >= 0)
		led_classdev_unregister(&led[i].cdev);

	devm_kfree(&pdev->dev, led);
	return err;
}

static int __devexit bcm_led_remove(struct platform_device *pdev)
{
	struct bcm_led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct bcm_led_data *led = dev_get_drvdata(&pdev->dev);
	int i;

	for(i = 0; i < pdata->num_leds; i++) {
		if (led[i].cdev.name)
			led_classdev_unregister(&led[i].cdev);
	}
	devm_kfree(&pdev->dev, led);
	return 0;
}

static struct platform_driver bcmled_driver = {
	.probe = bcm_led_probe,
	.remove = __devexit_p(bcm_led_remove),
	.driver = {
		.name = "bcm-led",
		.owner = THIS_MODULE,
	},
};


int bcmled_driver_init(void ) {
	int err = 0;
	struct bcm_led_platform_data *bcmdata;

	bcmled_device.dev.platform_data = board_desc.bcmleds;
	bcmdata = dev_get_platdata(&bcmled_device.dev);

	if (!bcmdata || bcmdata->num_leds == 0) {
		/* No LEDs defined, so do not load driver */
		return 0;
	}

	err = platform_driver_register(&bcmled_driver);
	if (err) {
		printk("Failed to register Broadcom led driver\n");
		return err;
	}

	err = platform_device_register(&bcmled_device);
	if (err) {
		printk("Failed to register Broadcom led device\n");
		return err;
	}

	return err;
}

void bcmled_driver_deinit(void ) {
	struct bcm_led_platform_data *bcmdata = dev_get_platdata(&bcmled_device.dev);

	if (bcmdata && bcmdata->num_leds) {
		platform_device_unregister(&bcmled_device);
		platform_driver_unregister(&bcmled_driver);
	}
}

#endif
