#if !defined(_TCH_LED_H)
#define _TCH_LED_H

enum tch_leds {
  kLedPowerRed,
  kLedPowerGreen,
  kLedPowerBlue,
  kLedPowerWhite,
  kLedBroadbandGreen,
  kLedBroadbandRed,
  kLedBroadband2Green,
  kLedBroadband2Red,
  kLedDectGreen,
  kLedDectRed,
  kLedDectBlue,
  kLedEthernetGreen,
  kLedEthernetRed,
  kLedEthernetBlue,
  kLedIPTVGreen,
  kLedWirelessGreen,
  kLedWirelessRed,
  kLedWirelessBlue,
  kLedWirelessWhite,
  kLedWireless5GHzGreen,
  kLedWireless5GHzRed,
  kLedInternetGreen,
  kLedInternetRed,
  kLedInternetBlue,
  kLedInternetWhite,
  kLedVoip1Green,
  kLedVoip1Red,
  kLedVoip1Blue,
  kLedVoip1White,
  kLedVoip2Green,
  kLedVoip2Red,
  kLedWPSGreen,
  kLedWPSBlue,
  kLedWPSRed,
  kLedUsbGreen,
  kLedUpgradeBlue,
  kLedUpgradeGreen,
  kLedLteGreen,
  kLedLteRed,
  kLedLteBlue,
  kLedLteWhite,
  // In fact, lte is only one kind of mobile technologies. So the name "mobile" is more suitable
  // "lte" leds above are kept just for backward compatibility
  kLedMobileGreen,
  kLedMobileRed,
  kLedMobileBlue,
  kLedMobileWhite,
  kLedMoCAGreen,
  kLedSFPGreen,
  kLedAmbient1White,
  kLedAmbient2White,
  kLedAmbient3White,
  kLedAmbient4White,
  kLedAmbient5White,
};


/**
 * API to set leds
 *
 *  led: ID of the TCH led
 *  state: 1 means on, 1 means off
 *
 *  returns: 0 if SUCCESS, -1 otherwise
 */
int tch_led_set(enum tch_leds led, unsigned short state);


/**
 * API to get leds supported brightness level
 *
 *  led: ID of the TCH led
 *
 *  returns:  >1 means support multi brightness level
 */
unsigned short tch_led_get_max_brightness(enum tch_leds led);

/**
 * API to determine whether a certain LED is available on this platform
 *
 *  led: ID of the TCH led
 *
 *  returns: 1 if available, 0 otherwise
 */
int tch_led_is_available(enum tch_leds led);

#endif
