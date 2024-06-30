/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2017 - 2017  -  Technicolor Delivery Technologies, SAS **
** - All Rights Reserved                                                **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **

*************************************************************************/

#ifndef SR_LED_DEFINES_H__
#define SR_LED_DEFINES_H__

#include "board_led_defines.h"

//const struct board * sr_led_get_board_description(void);
int sr_led_init(const struct board * board_desc);
void sr_led_exit(void);

#endif /* SR_LED_DEFINES_H__ */

