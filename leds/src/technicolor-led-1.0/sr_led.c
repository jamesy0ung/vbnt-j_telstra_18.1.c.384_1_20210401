/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2017 - 2017  -  Technicolor Delivery Technologies, SAS **
** - All Rights Reserved                                                **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

/*
 * Generic LED driver for Technicolor Linux Gateway platforms.
 *
 * Copyright (C) 2017 Technicolor <linuxgw@technicolor.com>
 *
 */

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

#include "leds.h"
#include "board_led_defines.h"
#include "sr_led.h"

struct shiftled_work_struct {
	struct work_struct work;
	unsigned mask;
	struct shiftled_led_platform_data *pdata;
};

static void led_release(struct device *dev);

static struct platform_device shiftled_device = {
	.name = "shiftled-led",
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

static void led_load_shift_reg(struct work_struct *pwork)
{
	int i;
	struct shiftled_work_struct *swork = container_of(pwork, struct shiftled_work_struct, work);
	for (i = 0; i < swork->pdata->reg_size; i++) { /* LSB first */
		gpio_set_value(swork->pdata->reg_clk, 0);
		udelay(1);
		gpio_set_value(swork->pdata->reg_data, (swork->mask >> i) & 1);
		udelay(1);
		gpio_set_value(swork->pdata->reg_clk, 1);
		udelay(2);
	}
	gpio_set_value(swork->pdata->reg_rck, 1);
	udelay(2);
	gpio_set_value(swork->pdata->reg_rck, 0);
	kfree(swork);
}

static int shiftled_update(struct shiftled_led_platform_data *pdata) {
	static unsigned long last_mask = ULONG_MAX;
	if(!pdata || !pdata->shift_work_queue) {
		return -1;
	}
	if (last_mask != pdata->mask) {
		struct shiftled_work_struct *shift_work = kmalloc(sizeof(struct shiftled_work_struct), GFP_KERNEL);
		if (!shift_work) {
			return -ENOMEM;
		}
		INIT_WORK(&shift_work->work, led_load_shift_reg);
		// Make a copy of mask so we don't need to lock things
		shift_work->mask = pdata->mask;
		shift_work->pdata = pdata;
		if(queue_work(pdata->shift_work_queue, &shift_work->work) == 0) {
			printk(KERN_INFO "LED Error adding work to workqueue\n");
			kfree(shift_work);
			return -1;
		}
		last_mask = pdata->mask;
	}
	return 0;
}

static void shiftled_led_set(struct led_classdev *led_cdev,
							 enum led_brightness value)
{
	struct shiftled_led_data *led_dat =
		container_of(led_cdev, struct shiftled_led_data, cdev);
	struct shiftled_led_platform_data *pdata = dev_get_platdata(&shiftled_device.dev);

	if(value > led_cdev->max_brightness)
		value = led_cdev->max_brightness;
	led_dat->brightness = value;

	if (led_dat->active_high ^ (value == LED_OFF))
		pdata->mask |= led_dat->bit;
	else
		pdata->mask &= ~led_dat->bit;

	shiftled_update(pdata);
}

static enum led_brightness shiftled_led_get(struct led_classdev *led_cdev)
{
	struct shiftled_led_data *led_dat =
		container_of(led_cdev, struct shiftled_led_data, cdev);

	return led_dat->brightness;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devinit shiftled_led_probe(struct platform_device *pdev)
#else
static int shiftled_led_probe(struct platform_device *pdev)
#endif
{
	int i;
	int err;
	struct shiftled_led_data *led;
	struct shiftled_led_platform_data *pdata = dev_get_platdata(&pdev->dev);

	led = devm_kzalloc(&pdev->dev, sizeof(struct shiftled_led_data) * pdata->num_leds,
					   GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	err = gpio_request(pdata->reg_rck, "rck");
	if (err)
		goto free_led_data;

	err = gpio_request(pdata->reg_clk, "clk");
	if (err)
		goto free_sr_rck;

	err = gpio_request(pdata->reg_data, "data");
	if (err)
		goto free_sr_clk;

	gpio_direction_output(pdata->reg_rck, 1);
	gpio_direction_output(pdata->reg_clk, 1);
	gpio_direction_output(pdata->reg_data, 1);

	dev_set_drvdata(&pdev->dev, led);
	for(i = 0; i < pdata->num_leds; i++) {
		led[i].cdev.name = pdata->leds[i].name;
		led[i].cdev.max_brightness = 1;  //TODO: shift reg LEDs not YET support brightness control
		led[i].cdev.brightness_set = shiftled_led_set;
		led[i].cdev.brightness_get = shiftled_led_get;
		led[i].cdev.default_trigger = pdata->leds[i].default_trigger;

		led[i].bit = 1 << pdata->leds[i].bit;
		led[i].active_high = pdata->leds[i].active_high;
		led[i].brightness = LED_OFF;

		if (led[i].active_high == 1)
			pdata->mask &= ~led[i].bit;
		else
			pdata->mask |= led[i].bit;
	}

	err = shiftled_update(pdata);
	if (err)
		goto free_sr_data;

	for(i = 0; i < pdata->num_leds; i++) {
		err = led_classdev_register(&pdev->dev, &led[i].cdev);
		if (err)
			goto free_leds;
	}
	return 0;

free_leds:
	while( --i >= 0)
		led_classdev_unregister(&led[i].cdev);

free_sr_data:
	gpio_free(pdata->reg_data);

free_sr_clk:
	gpio_free(pdata->reg_clk);

free_sr_rck:
	gpio_free(pdata->reg_rck);

free_led_data:
	devm_kfree(&pdev->dev, led);
	return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devexit shiftled_led_remove(struct platform_device *pdev)
#else
static int shiftled_led_remove(struct platform_device *pdev)
#endif
{
	struct shiftled_led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct shiftled_led_data *led = dev_get_drvdata(&pdev->dev);
	int i;

	for(i = 0; i < pdata->num_leds; i++) {
		if (led[i].cdev.name)
			led_classdev_unregister(&led[i].cdev);
	}

	if (pdata->shift_work_queue) {
		flush_workqueue(pdata->shift_work_queue);
		destroy_workqueue(pdata->shift_work_queue);
		pdata->shift_work_queue = NULL;
	}

	devm_kfree(&pdev->dev, led);
	gpio_free(pdata->reg_data);
	gpio_free(pdata->reg_clk);
	gpio_free(pdata->reg_rck);
	return 0;
}

static struct platform_driver shiftled_driver = {
	.probe = shiftled_led_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
	.remove = __devexit_p(shiftled_led_remove),
#else
	.remove = shiftled_led_remove,
#endif

	.driver = {
		.name = "shiftled-led",
		.owner = THIS_MODULE,
	},
};

int sr_led_init(const struct board * board_desc)
{
	int err = 0;
	struct shiftled_led_platform_data *pdata;

	shiftled_device.dev.platform_data = board_desc->shiftleds;
        pdata = dev_get_platdata(&shiftled_device.dev);

        if(pdata) {
                pdata->shift_work_queue = create_singlethread_workqueue("led_shift_queue");
                if (!pdata->shift_work_queue) {
                        printk("Failed to create shiftled workqueue\n");
                        return -1;
                }
                pdata->mask = 0;
        }

	if (pdata && pdata->num_leds) {
		err = platform_driver_register(&shiftled_driver);
		if (err) {
			printk("Failed to register shiftled driver\n");
			return err;
		}

		err = platform_device_register(&shiftled_device);
		if (err) {
			printk("Failed to register shiftled device\n");
			return err;
		}
	}
	return err;
}

void sr_led_exit(void)
{
	struct shiftled_led_platform_data *pdata = dev_get_platdata(&shiftled_device.dev);

	if((NULL != pdata) && (0 < pdata->num_leds)) {
                platform_device_unregister(&shiftled_device);
                platform_driver_unregister(&shiftled_driver);
        }
}
