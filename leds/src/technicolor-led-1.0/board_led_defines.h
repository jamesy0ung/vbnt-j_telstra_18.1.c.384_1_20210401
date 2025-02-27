/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          **
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

#ifndef BOARD_LED_DEFINES_H__
#define BOARD_LED_DEFINES_H__

/*
 * Defines the board led defines for each Technicolor board
 *
 * Copyright (C) 2013 Technicolor <linuxgw@technicolor.com>
 *
 */

#include <linux/types.h>
#include <linux/device.h>
#include <linux/leds.h>

#if defined(BRCM_CHIP_63138) || defined(BRCM_CHIP_63381)
#include "tch_led.h"
#define BCM_LED_FW
#endif

#if defined(BCM_LED_FW)
struct bcm_led {
	const char		*name;
	const char		*default_trigger;
	enum tch_leds		id;
};

struct bcm_led_data {
	struct led_classdev	cdev;
	unsigned		id;
	enum led_brightness	brightness;
};
#endif

struct shiftled_led {
	const char		*name;
	const char		*default_trigger;
	unsigned		bit;
	u8			active_high : 1;
};

struct shiftled_led_data {
	struct led_classdev	cdev;
	unsigned		bit;
	u8			active_high : 1;
	enum led_brightness	brightness;
};

struct aggreg_led_data {
	struct led_classdev	cdev;
	enum led_brightness	brightness;
	struct led_classdev	*led1;
	struct led_classdev	*led2;
	struct led_classdev	*led3;
};

struct aggreg_led_platform_data {
	int			num_leds;
	struct aggreg_led_data	*leds;
};

struct shiftled_led_platform_data {
	int			num_leds;
	struct shiftled_led	*leds;
	unsigned		reg_rck;    // clock out
	unsigned		reg_clk;    // clock data in
	unsigned		reg_data;   // data
	unsigned		reg_size;   // reg size
	unsigned		mask;
	struct workqueue_struct	*shift_work_queue;
};

#if defined(BCM_LED_FW)
struct bcm_led_platform_data {
	int			num_leds;
	struct bcm_led	*leds;
};
#endif

struct board {
	const char *				name;
	struct gpio_led_platform_data *		gpioleds;
	struct shiftled_led_platform_data *	shiftleds;
#if defined(BCM_LED_FW)
	struct bcm_led_platform_data *		bcmleds;
#endif
	struct aggreg_led_platform_data *	aggregleds;

};

/**
 * Retrieves the led description for a particular board
 */
struct board * get_board_description(const char * current_board);

#endif
