/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          **
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/

#ifndef BCM_LED_DEFINES_H__
#define BCM_LED_DEFINES_H__

#include "board_led_defines.h"

const struct board * bcm_led_get_board_description(void);
int bcmled_driver_init(void);
void bcmled_driver_deinit(void);

#endif
