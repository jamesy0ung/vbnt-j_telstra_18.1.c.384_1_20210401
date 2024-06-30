/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          **
** All Rights Reserved                                                  **
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
#include <linux/types.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/version.h>

#include "leds.h"
#include "board_led_defines.h"
#include "aggr_leds.h"

/* color combination handling */
#define COLOR_NONE 0x0
#define COLOR_RED 0x1
#define COLOR_GREEN 0x2
#define COLOR_BLUE 0x4
#define COLOR_WHITE 0x8

static const char * const color_sufix[] = {"red", "green", "blue", "white"};
#define NUM_COLORS (sizeof(color_sufix)/sizeof(color_sufix[0]))
#define IDX_RED 0
#define IDX_GREEN 1
#define IDX_BLUE 2
#define IDX_WHITE 3
/*                                                 R      G        R+G       B       R+B        G+B     R+G+B   */
static const char * const combined_colors[] = {"", "red", "green", "orange", "blue", "magenta", "cyan", "white"};
/* index of combined colors */
#define IDX_C_OFF 0
#define IDX_C_RED 1
#define IDX_C_GREEN 2
#define IDX_C_YELLOW 3
#define IDX_C_ORANGE 3
#define IDX_C_BLUE 4
#define IDX_C_MAGENTA 5
#define IDX_C_CYAN 6
#define IDX_C_WHITE 7

#define MAX_LEN_LEDNAME 32
#define MAX_LEDS 60
#define MAX_AGGR_LEDS 30

#define MAX(a,b)     ((a) >= (b)? (a):(b))
#define MAX3(a,b,c)  MAX(MAX(a,b),c)

typedef struct {
	char name[MAX_LEN_LEDNAME]; /* base led name with color part removed */
	struct led_classdev * color_cdev[NUM_COLORS];
	uint8_t color_mask;
} name_color_t;

static int num_multi_color_led_names = 0; /* leds that have recognised color sufix */
static name_color_t leds_with_color[MAX_LEDS];

static char aggr_led_names[MAX_AGGR_LEDS][MAX_LEN_LEDNAME];
static int num_aggr_led_names = 0;

static struct aggreg_led_platform_data aggreg_led_pd = {
	.num_leds = 0,
	.leds = NULL
};

static void led_set3(struct led_classdev *led_cdev,
                     enum led_brightness value)
{
	struct aggreg_led_data *led_dat = container_of(led_cdev, struct aggreg_led_data, cdev);

	if(value > led_cdev->max_brightness)
		value = led_cdev->max_brightness;
	led_dat->brightness = value;
	led_set_brightness(led_dat->led1, value);
	led_set_brightness(led_dat->led2, value);
	led_set_brightness(led_dat->led3, value);
}

static void led_set2(struct led_classdev *led_cdev,
                     enum led_brightness value)
{
	struct aggreg_led_data *led_dat = container_of(led_cdev, struct aggreg_led_data, cdev);

	if(value > led_cdev->max_brightness)
		value = led_cdev->max_brightness;
	led_dat->brightness = value;
	led_set_brightness(led_dat->led1, value);
	led_set_brightness(led_dat->led2, value);
}

static enum led_brightness led_get(struct led_classdev *led_cdev)
{
	struct aggreg_led_data *led_dat = container_of(led_cdev, struct aggreg_led_data, cdev);
	return led_dat->brightness;
}

/* returns index of existing led name (with color stripped) or creates a new one */
static int find_idx_led_with_color_name(int num_names, const char * name)
{
	int idx;

	for (idx=0; idx<num_names; idx++) {
		if (0==strcmp(name, leds_with_color[idx].name)) {
			return idx;
		}
	}
	if (num_names < MAX_LEDS) { /* construct a new entry */
		leds_with_color[num_names].color_mask = COLOR_NONE;
		strncpy(leds_with_color[num_names].name, name, MAX_LEN_LEDNAME);
		for (idx=0; idx<NUM_COLORS; idx++) {
			leds_with_color[num_names].color_cdev[idx] = NULL;
		}
		num_multi_color_led_names++;
	} else {
		num_names = -1;
	}
	return num_names;
}

static int recognised_color_sufix(const char * name, char *out_name)
{
	int len;
	int idx;
	int color_len;

	len = strlen(name);
	for (idx = 0; idx < NUM_COLORS; idx++) {
		color_len = strlen(color_sufix[idx]);
		if ((len > color_len) && (0 == strcmp(&name[len-color_len], color_sufix[idx]))) {
			strncpy(out_name, name, len-color_len);
			out_name[len-color_len] = '\0';
			return idx;
		}
	}
	return -1;
}

static void populate_leds_with_color(void)
{
	int existing_led_idx;
	struct led_classdev *led_cdev;
	char led_name[MAX_LEN_LEDNAME];
	int color_idx;
	uint8_t color_mask;

	down_read(&leds_list_lock);
	list_for_each_entry(led_cdev, &leds_list, node) {
		color_idx = recognised_color_sufix(led_cdev->name, led_name);
		if (0 > color_idx) {
			continue;
		}
		existing_led_idx = find_idx_led_with_color_name(num_multi_color_led_names, led_name);
		if (existing_led_idx < 0) {
			printk("WARNING pkg/leds : populateLedsWithColor : too many candidate leds with color, %s ignored\n", led_name);
			continue;
		}
		color_mask = (uint8_t)(1 << color_idx);
		leds_with_color[existing_led_idx].color_mask |= color_mask;
		leds_with_color[existing_led_idx].color_cdev[color_idx] = led_cdev;
	}
	up_read(&leds_list_lock);
}

static void add_led_3color(const char * base, const char * new_color, struct platform_device *pdev, struct aggreg_led_data *new_led, 
		struct led_classdev *led1, struct led_classdev *led2, struct led_classdev *led3)
{

	char *aggr_name = &aggr_led_names[num_aggr_led_names][0];

	strcpy(aggr_name, base);
	strcat(aggr_name, new_color);

	new_led->cdev.name = aggr_name;
	new_led->cdev.max_brightness = MAX3(led1->max_brightness, led2->max_brightness, led3->max_brightness);
	new_led->cdev.brightness_set = led_set3;
	new_led->cdev.brightness_get = led_get;
	new_led->cdev.default_trigger = "none";
	new_led->led1 = led1;
	new_led->led2 = led2;
	new_led->led3 = led3;
	new_led->brightness = LED_OFF;
	led_classdev_register(&pdev->dev, &new_led->cdev);

	num_aggr_led_names++;
}

static void add_led_2color(const char * base, const char * new_color, struct platform_device *pdev, struct aggreg_led_data *new_led,
                struct led_classdev *led1, struct led_classdev *led2)
{
	char *aggr_name = &aggr_led_names[num_aggr_led_names][0];

	strcpy(aggr_name, base);
	strcat(aggr_name, new_color);

	new_led->cdev.name = aggr_name;
	new_led->cdev.max_brightness = MAX(led1->max_brightness, led2->max_brightness);
	new_led->cdev.brightness_set = led_set2;
	new_led->cdev.brightness_get = led_get;
	new_led->cdev.default_trigger = "none";
	new_led->led1 = led1;
	new_led->led2 = led2;
	new_led->led3 = NULL;
	new_led->brightness = LED_OFF;
	led_classdev_register(&pdev->dev, &new_led->cdev);

	num_aggr_led_names++;
}

static int create_combined_colors(struct platform_device *pdev)
{
	int idx;
	uint8_t mask;
	int num_aggr_leds = 0;
	struct aggreg_led_data *composite_led_array;
	struct aggreg_led_platform_data *adata = dev_get_platdata(&pdev->dev);

	if (adata == NULL)
	{
		printk("ERROR pkg/leds: create_combined_colors : Could not get platform data!\n");
		return -1;
	}

	/* calculate the number of agregated leds that will be generated */
	for (idx=0; idx<num_multi_color_led_names; idx++) {
		mask = leds_with_color[idx].color_mask;
		switch (mask & (COLOR_RED|COLOR_GREEN|COLOR_BLUE)) {
			case (COLOR_RED|COLOR_GREEN|COLOR_BLUE) :
				/* combining 3 colors gives 4 combinations : 1*"all 3 collors" + 3*"2 out of 3 colors" */
				num_aggr_leds += 4;
				break;
			case (COLOR_RED|COLOR_GREEN) :
			case (COLOR_RED|COLOR_BLUE) :
			case (COLOR_GREEN|COLOR_BLUE) :
				/* FT intentional */
				num_aggr_leds++;
				break;
			default :
				break;
		}
	}
	if (MAX_AGGR_LEDS < num_aggr_leds) {
		/* considder incrementing MAX_AGGR_LEDS */
		printk("ERROR pkg/leds : createCombinedColors : Too many aggregated leds possible %d\n", num_aggr_leds);
		return -1;
	}

	if (num_aggr_leds == 0) {
		/* In case num_aggr_leds is 0, return immediately. */
		printk("WARNING pkg/leds : createCombinedColors : No aggregated led would be created\n");
		return num_aggr_leds;
	}

	composite_led_array = devm_kzalloc(&pdev->dev, sizeof(struct aggreg_led_data) * num_aggr_leds, GFP_KERNEL);
	if (composite_led_array == NULL)
	{
		printk("ERROR pkg/leds: create_combined_colors : Could not allocate mem for aggregate leds!\n");
		return -1;
	}
	adata->leds = composite_led_array;

	num_aggr_leds = 0;

	for (idx=0; idx<num_multi_color_led_names; idx++) {
		mask = leds_with_color[idx].color_mask;
		switch (mask & (COLOR_RED|COLOR_GREEN|COLOR_BLUE)) {
			case (COLOR_RED|COLOR_GREEN|COLOR_BLUE) :
				if (!(mask & COLOR_WHITE)) {
					add_led_3color(leds_with_color[idx].name, combined_colors[IDX_C_WHITE] , pdev, &composite_led_array[num_aggr_leds],
						leds_with_color[idx].color_cdev[IDX_RED], leds_with_color[idx].color_cdev[IDX_GREEN],
						leds_with_color[idx].color_cdev[IDX_BLUE]);
					num_aggr_leds++;
				} else
					printk("WARNING pkg/leds : createCombinedColors : There is already a real white led. No need to create an aggregated white led!\n");
				add_led_2color(leds_with_color[idx].name, combined_colors[IDX_C_YELLOW], pdev, &composite_led_array[num_aggr_leds],
					leds_with_color[idx].color_cdev[IDX_RED], leds_with_color[idx].color_cdev[IDX_GREEN]);
				num_aggr_leds++;
				add_led_2color(leds_with_color[idx].name, combined_colors[IDX_C_MAGENTA], pdev, &composite_led_array[num_aggr_leds],
					leds_with_color[idx].color_cdev[IDX_RED], leds_with_color[idx].color_cdev[IDX_BLUE]);
				num_aggr_leds++;
				add_led_2color(leds_with_color[idx].name, combined_colors[IDX_C_CYAN], pdev, &composite_led_array[num_aggr_leds],
					leds_with_color[idx].color_cdev[IDX_GREEN], leds_with_color[idx].color_cdev[IDX_BLUE]);
				num_aggr_leds++;
				break;
			case (COLOR_RED|COLOR_GREEN) :
				add_led_2color(leds_with_color[idx].name, combined_colors[IDX_C_YELLOW], pdev, &composite_led_array[num_aggr_leds],
					leds_with_color[idx].color_cdev[IDX_RED], leds_with_color[idx].color_cdev[IDX_GREEN]);
				num_aggr_leds++;
				break;
			case (COLOR_RED|COLOR_BLUE) :
				add_led_2color(leds_with_color[idx].name, combined_colors[IDX_C_MAGENTA], pdev, &composite_led_array[num_aggr_leds],
					leds_with_color[idx].color_cdev[IDX_RED], leds_with_color[idx].color_cdev[IDX_BLUE]);
				num_aggr_leds++;
				break;
			case (COLOR_GREEN|COLOR_BLUE) :
				add_led_2color(leds_with_color[idx].name, combined_colors[IDX_C_CYAN], pdev, &composite_led_array[num_aggr_leds],
					leds_with_color[idx].color_cdev[IDX_GREEN], leds_with_color[idx].color_cdev[IDX_BLUE]);
				num_aggr_leds++;
				break;
			default :
				/* no aggregation to be done for single color leds */
				break;
		}
	}
	adata->num_leds = num_aggr_leds;

	return num_aggr_leds;
}

static int create_aggregated_leds(struct platform_device *pdev)
{
	int num_leds;

	populate_leds_with_color();
	num_leds = create_combined_colors(pdev);
	return num_leds;
}

static void aled_release(struct device *dev)
{
}

static struct platform_device aggregled_device = {
	.name                   = "aggreg-led",
	.id                     = 0,
	.dev = {
		.release = aled_release
	}
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devinit aggreg_led_probe(struct platform_device *pdev)
#else
static int aggreg_led_probe(struct platform_device *pdev)
#endif
{
	/* no probe action needed, since HW is initialised in the LED driver */
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devexit aggreg_led_remove(struct platform_device *pdev)
#else
static int aggreg_led_remove(struct platform_device *pdev)
#endif
{
	struct aggreg_led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct aggreg_led_data *led = dev_get_drvdata(&pdev->dev);
	int i;

	for(i = 0; i < pdata->num_leds; i++) {
		if (led[i].cdev.name)
			led_classdev_unregister(&led[i].cdev);
	}
	devm_kfree(&pdev->dev, led);
	return 0;
}

static struct platform_driver aggregled_driver = {
	.probe = aggreg_led_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
	.remove = __devexit_p(aggreg_led_remove),
#else
	.remove = aggreg_led_remove,
#endif
	.driver = {
		.name = "aggreg-led",
		.owner = THIS_MODULE,
	},
};

/***************************************************************************
 * Function Name: led_init
 * Description  : Initial function that is called when the module is loaded
 * Returns      : None.
 ***************************************************************************/
/***************************************************************************
 * adds led color aggregation based on led names found in the class leds
 * assumes that the individual leds have been probed and are operational
 ***************************************************************************/
int aggr_led_init(void)
{
	int err = 0;
	int num_leds;

	aggregled_device.dev.platform_data = &aggreg_led_pd;

	err = platform_driver_register(&aggregled_driver);
	if (err) {
		printk("ERROR pkg/leds : Failed to register aggregated led driver\n");
		return -2;
	}

	err = platform_device_register(&aggregled_device);
	if (err) {
		printk("ERROR pkg/leds : Failed to register aggregated led device\n");
		return -3;
	}

	num_leds = create_aggregated_leds(&aggregled_device);
	if (0 > num_leds) {
		printk("ERROR pkg/leds : Failed to create aggregated led devices\n");
		return -4;
	}

	return err;
}

/***************************************************************************
 * Function Name: led_exit
 * Description  : Final function that is called when the module is unloaded
 * Returns      : None.
 ***************************************************************************/
void aggr_led_exit(void)
{
	struct aggreg_led_platform_data *adata = dev_get_platdata(&aggregled_device.dev);

	if ((NULL != adata) && (0 < adata->num_leds)) {
		platform_device_unregister(&aggregled_device);
		platform_driver_unregister(&aggregled_driver);
	}
}

