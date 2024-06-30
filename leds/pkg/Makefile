########### COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE #############
## Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     ##
## All Rights Reserved                                                  ##
##                                                                      
## This program is free software; you can redistribute it and/or modify 
## it under the terms of the GNU General Public License version 2 as    
## published by the Free Software Foundation.                           
##########################################################################

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=technicolor-led
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/leds.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=8043df2e89d794fb87c3840f8b7a92abb2428d67
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk


define KernelPackage/technicolor-led
  SUBMENU:=LED modules
  TITLE:=LED support for Technicolor Linux Gateways
  DEPENDS:=+kmod-leds-gpio
  FILES:=$(PKG_BUILD_DIR)/technicolor_led.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  AUTOLOAD:=
  DEFAULT:=y
endef

define KernelPackage/technicolor-led/description
 Kernel module to provide LED support for Technicolor Gateways.
endef

EXTRA_KCONFIG:= \
	CONFIG_TECHNICOLOR_LED=m

# platform specific flags
ifeq ($(CONFIG_TARGET_lantiq_tch),y)
	PS_CFLAGS :=
else
ifeq ($(CONFIG_TARGET_brcm63xx_tch),y)
	PS_CFLAGS := -DBRCM_CHIP_$(BRCM_CHIP) -I$(LINUX_DIR)/../../shared/opensource/include/bcm963xx
	PS_CFLAGS += -DENABLE_GPIO_LED_DRIVER -DENABLE_SHIFT_LED_DRIVER -DENABLE_BCM_DRIVER
else
ifeq ($(CONFIG_TARGET_brcm68xx_tch),y)
	PS_CFLAGS := -DBRCM_CHIP_$(BRCM_CHIP) -I$(LINUX_DIR)/../../shared/opensource/include/bcm963xx
	PS_CFLAGS += -DENABLE_GPIO_LED_DRIVER -DENABLE_SHIFT_LED_DRIVER -DENABLE_BCM_DRIVER
else
	PS_CFLAGS := -DENABLE_GPIO_LED_DRIVER -DENABLE_SHIFT_LED_DRIVER
endif
endif
endif

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	EXTRA_CFLAGS="-DBOARD_$(SUBTARGET) $(PS_CFLAGS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,technicolor-led))
