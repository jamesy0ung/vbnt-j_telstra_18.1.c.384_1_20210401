#ifndef BOARD_BUTTON_DEFINES_H__
#define BOARD_BUTTON_DEFINES_H__

/*********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          **
** All Rights Reserved                                                  **
**                                                                      **
** This program is free software; you can redistribute it and/or modify **
** it under the terms of the GNU General Public License version 2 as    **
** published by the Free Software Foundation.                           **
*************************************************************************/



struct board {
	const char *				name;
	struct gpio_keys_platform_data *			buttons;
};

/**
 * Retrieves the led description for a particular board
 */
struct board * get_board_description(const char * current_board);

#endif
