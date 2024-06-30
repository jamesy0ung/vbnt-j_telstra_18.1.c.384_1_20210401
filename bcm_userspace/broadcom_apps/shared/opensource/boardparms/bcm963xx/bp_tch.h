#ifndef INC_BP_TCH_H
#define INC_BP_TCH_H
#include <asm/setup.h>

#define TCH_LEDS \
  bp_usGpioLedPowerRed_tch, \
  bp_usGpioLedPowerGreen_tch, \
  bp_usGpioLedPowerBlue_tch, \
  bp_usGpioLedPowerWhite_tch, \
  bp_usGpioLedBroadbandGreen_tch, \
  bp_usGpioLedBroadbandRed_tch, \
  bp_usGpioLedBroadband2Green_tch, \
  bp_usGpioLedBroadband2Red_tch, \
  bp_usGpioLedDectGreen_tch, \
  bp_usGpioLedDectBlue_tch, \
  bp_usGpioLedDectRed_tch, \
  bp_usGpioLedEthernetGreen_tch, \
  bp_usGpioLedEthernetRed_tch, \
  bp_usGpioLedEthernetBlue_tch, \
  bp_usGpioLedIPTVGreen_tch, \
  bp_usGpioLedWirelessGreen_tch, \
  bp_usGpioLedWirelessRed_tch, \
  bp_usGpioLedWirelessBlue_tch, \
  bp_usGpioLedWirelessWhite_tch, \
  bp_usGpioLedWireless5GHzGreen_tch, \
  bp_usGpioLedWireless5GHzRed_tch, \
  bp_usGpioLedInternetGreen_tch, \
  bp_usGpioLedInternetRed_tch, \
  bp_usGpioLedInternetBlue_tch, \
  bp_usGpioLedInternetWhite_tch, \
  bp_usGpioLedVoip1Green_tch, \
  bp_usGpioLedVoip1Red_tch, \
  bp_usGpioLedVoip1Blue_tch, \
  bp_usGpioLedVoip1White_tch, \
  bp_usGpioLedVoip2Green_tch, \
  bp_usGpioLedVoip2Red_tch, \
  bp_usGpioLedWPSGreen_tch, \
  bp_usGpioLedWPSRed_tch, \
  bp_usGpioLedWPSBlue_tch, \
  bp_usGpioLedUsbGreen_tch, \
  bp_usGpioLedUpgradeBlue_tch, \
  bp_usGpioLedUpgradeGreen_tch, \
  bp_usGpioLedLteBlue_tch, \
  bp_usGpioLedLteGreen_tch, \
  bp_usGpioLedLteRed_tch, \
  bp_usGpioLedLteWhite_tch, \
  bp_usGpioLedMobileBlue_tch, \
  bp_usGpioLedMobileGreen_tch, \
  bp_usGpioLedMobileRed_tch, \
  bp_usGpioLedMobileWhite_tch, \
  bp_usGpioLedMoCAGreen_tch, \
  bp_usGpioLedSFPGreen_tch, \
  bp_usGpioLedAmbient1White_tch, \
  bp_usGpioLedAmbient2White_tch, \
  bp_usGpioLedAmbient3White_tch, \
  bp_usGpioLedAmbient4White_tch, \
  bp_usGpioLedAmbient5White_tch


#define TCH_BPELEMS \
  bp_cpCustomATAG_tch, /* Custom ARM Tags */      \
  bp_cpVethPortmap_tch, \
  bp_usRxRgmiiClockDelayAtMac_tch, \
  bp_usExtSwLedCfg_tch, \
  bp_usGpioHardRST_tch, \
  bp_ulPcieOrder_tch, \
  bp_usAELinkLed_tch, \
  TCH_LEDS

#if CONFIG_ATAGS
struct custom_atag {
	struct tag_header hdr;
	union {
		/* include all tags that should be possible to specify via the
		   custom atag structure. this is used to calculate the maximum
		   size of a tag entry
		   Tags are defined in cfe/cfe/arch/arm/board/bcm63xx_ram/include/atag.h
		*/
		struct tag_rdpsize	rdpsize;
		struct tag_dhdparm      dhdparm;
	} u;
};
#define customtag_hdr(type)  { .tag = type, .size = ((sizeof(struct tag_header) + sizeof(((struct custom_atag *)0)->u)) >> 2) }
#endif /* CONFIG_ATAGS */

#endif
