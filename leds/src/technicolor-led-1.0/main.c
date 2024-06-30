/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2017 - 2017  -  Technicolor Delivery Technologies, SAS **
** - All Rights Reserved                                                **
** Technicolor hereby informs you that certain portions                 **
** of this software module and/or Work are owned by Technicolor         **
** and/or its software providers.                                       **
** Distribution copying and modification of all such work are reserved  **
** to Technicolor and/or its affiliates, and are not permitted without  **
** express written authorization from Technicolor.                      **
** Technicolor is registered trademark and trade name of Technicolor,   **
** and shall not be used in any manner without express written          **
** authorization from Technicolor                                       **
*************************************************************************/

/*
 * Generic LED driver for Technicolor Linux Gateway platforms.
 *
 * Copyright (C) 2017 Technicolor <linuxgw@technicolor.com>
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include "leds.h"
#if defined(ENABLE_GPIO_LED_DRIVER) || defined(ENABLE_SHIFT_LED_DRIVER)
#include "board_led_defines.h"
#endif /* defined(ENABLE_GPIO_LED_DRIVER) || defined(ENABLE_SHIFT_LED_DRIVER) */
#ifdef ENABLE_SHIFT_LED_DRIVER
#include "sr_led.h"
#endif /* ENABLE_SHIFT_LED_DRIVER */
#include "aggr_leds.h"
#if defined(BCM_LED_FW)
#include "bcm_led.h"
#endif
#ifdef ENABLE_GPIO_LED_DRIVER
static void led_release(struct device *dev);

static struct platform_device gpled_device = {
	.name = "leds-gpio",
	.id = 0,
	.dev = {
		.release = led_release
	}
};

static void led_release(struct device *dev)
{
	(void)dev;
	/* really nothing to be done, just avoid nagging when removing the module */
}

#endif /* ENABLE_GPIO_LED_DRIVER */

/* Prototypes. */
static int __init led_init(void);
static void __exit led_exit(void);

/***************************************************************************
 * Function Name: led_init
 * Description  : Initial function that is called when the module is loaded
 * Returns      : None.
 ***************************************************************************/
static int __init led_init(void)
{
	int err;
#ifdef ENABLE_GPIO_LED_DRIVER
	struct gpio_led_platform_data *gpled_data;
#endif
	/* The build process for these boards does not yet fill in tch_board */
#if defined(BOARD_GANTN)
	const struct board * board_desc = get_board_description("GANT-N");
#elif defined(BOARD_C2KEVM)
	const struct board * board_desc = get_board_description("C2KEVM");
#elif defined (BOARD_GANTV)
	const struct board * board_desc = get_board_description("GANT-V");
#elif defined (BOARD_GANTW)
	const struct board * board_desc = get_board_description("GANT-W");
#elif defined (BOARD_GANT5)
	const struct board * board_desc = get_board_description("GANT-5");
#elif defined (BCM_LED_FW)
	const struct board * board_desc = bcm_led_get_board_description();
#elif defined (BOARD_GBNTD)
	const struct board * board_desc = get_board_description("GBNT-D");
#elif defined(ENABLE_GPIO_LED_DRIVER) || defined(ENABLE_SHIFT_LED_DRIVER)
	const struct board * board_desc = get_board_description(tch_board);
#else
        /* board_desc not needed */
#endif

#if defined(ENABLE_GPIO_LED_DRIVER) || defined(ENABLE_SHIFT_LED_DRIVER)
	if (!board_desc) {
		printk("Could not find LED description for platform: %s\n", tch_board);
		return -1;
	}
#ifdef ENABLE_GPIO_LED_DRIVER
	gpled_device.dev.platform_data = board_desc->gpioleds;
	gpled_data = dev_get_platdata(&gpled_device.dev);

	if(gpled_data && gpled_data->num_leds > 0) {
		err = platform_device_register(&gpled_device);
		if (err) {
			printk("ERROR led_init : Failed to register GPIO led device (err %d)\n", err);
			return -1;
		}
	}
#endif /* ENABLE_GPIO_LED_DRIVER */
#if defined(BCM_LED_FW)
	err = bcmled_driver_init();
	if (err) {
		printk("ERROR led_init : bcmled_driver_init (err %d)\n", err);
		return -1;
	}
#endif
#ifdef ENABLE_SHIFT_LED_DRIVER
	err = sr_led_init(board_desc);
        if (err) {
                printk("ERROR led_init : sr_led_init (err %d)\n", err);
                return -1;
        }
#endif /* ENABLE_SHIFT_LED_DRIVER */
#endif /* defined(ENABLE_GPIO_LED_DRIVER) || defined(ENABLE_SHIFT_LED_DRIVER) */

	err = aggr_led_init();
	return err;
}

/***************************************************************************
 * Function Name: led_exit
 * Description  : Final function that is called when the module is unloaded
 * Returns      : None.
 ***************************************************************************/
static void __exit led_exit()
{
	aggr_led_exit();
#ifdef ENABLE_SHIFT_LED_DRIVER
	sr_led_exit();
#endif

#ifdef ENABLE_GPIO_LED_DRIVER
	{
	        struct gpio_led_platform_data *gpled_data = dev_get_platdata(&gpled_device.dev);

		if (gpled_data && gpled_data->num_leds) {
			platform_device_unregister(&gpled_device);
		}
	}
#endif /* ENABLE_GPIO_LED_DRIVER */
#if defined(BCM_LED_FW)
	bcmled_driver_deinit();
#endif
}
module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Generic LED support for Technicolor Linux Gateways");
MODULE_AUTHOR("Technicolor <linuxgw@technicolor.com");
