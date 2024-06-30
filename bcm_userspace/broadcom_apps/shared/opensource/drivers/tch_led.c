
#include "boardparms.h"
#include "bcm_led.h"
#include "bcm_pinmux.h"
#include "shared_utils.h"
#include "tch_led.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <bcm_map_part.h>
#include <linux/string.h>
#include <linux/spinlock.h>

/* Spinlock to prevent concurrent access to the GPIO registers */
static DEFINE_SPINLOCK(bcm63xx_tch_led_lock);


/**
 * API to set leds
 *
 *  led: ID of the TCH led
 *  state: 1 means on, 1 means off
 *
 *  returns: 0 if SUCCESS, -1 otherwise
 */
int tch_led_set(enum tch_leds led, unsigned short state) {
	unsigned short gpio;
	unsigned long flags;
#ifdef  LED_MAX_BRIGHTNESS
	unsigned short led_controller;
	short *optled_map;
#endif

	if (BpGetLedGpio_tch(led,  &gpio) != BP_SUCCESS ) {
		printk("Failed to set led %d value %d\n", led, state);
		return -1;
	}

#ifdef  LED_MAX_BRIGHTNESS
	if ((gpio & BP_LED_USE_GPIO) != BP_LED_USE_GPIO){
		led_controller = gpio & BP_GPIO_NUM_MASK;
		optled_map = bcm_led_driver_get_optled_map();
		led_controller = optled_map[led_controller];
		if(led_controller < LED_NUM_LEDS)
			LED->brightCtrl[led_controller/8] =
			  (LED->brightCtrl[led_controller/8] & ~(0xf<<(led_controller%8*4)))
			   |(state << (led_controller%8*4));
	}
#endif

	spin_lock_irqsave(&bcm63xx_tch_led_lock, flags);
	bcm_led_driver_set( gpio, state );
	spin_unlock_irqrestore(&bcm63xx_tch_led_lock, flags);
	return 0;
}
EXPORT_SYMBOL(tch_led_set);


/**
 * API to get leds supported brightness level
 *
 *  led: ID of the TCH led
 *
 *  returns:  >1 means support multi brightness level
 */
unsigned short tch_led_get_max_brightness(enum tch_leds led) {
	unsigned short gpio;
	unsigned short max_brightness = 1;

#ifdef  LED_MAX_BRIGHTNESS
	if (BpGetLedGpio_tch(led,  &gpio) == BP_SUCCESS ) {
		if ((gpio & BP_LED_USE_GPIO) != BP_LED_USE_GPIO)
			max_brightness = LED_MAX_BRIGHTNESS;
	}
#endif

	return max_brightness;
}
EXPORT_SYMBOL(tch_led_get_max_brightness);

/**
 * API to determine whether a certain LED is available on this platform
 *
 *  led: ID of the TCH led
 *
 *  returns: 1 if available, 0 otherwise
 */
int tch_led_is_available(enum tch_leds led) {
	unsigned short gpio;

	return (BpGetLedGpio_tch(led,  &gpio) == BP_SUCCESS );
}

EXPORT_SYMBOL(tch_led_is_available);

