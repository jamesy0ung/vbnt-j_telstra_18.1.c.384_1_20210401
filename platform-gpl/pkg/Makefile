########### COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE #############
## Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          ##
## All Rights Reserved                                                  ##
##                                                                      ##
## This program is free software; you can redistribute it and/or modify ##
## it under the terms of the GNU General Public License version 2 as    ##
## published by the Free Software Foundation.         
##########################################################################

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=platform-gpl
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/platform-gpl.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=fbba97b38b38973fe7838722be25701b44c3c232

PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk

define KernelPackage/platform-gpl
  SUBMENU:=Technicolor specific kernel modules
  TITLE:=Platform kernel module (GPLv2 license)
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/platform-gpl.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  AUTOLOAD:=$(call AutoLoad,51,platform-gpl)
  DEFAULT:=y if (TARGET_brcm63xx_tch)
endef

define KernelPackage/platform-gpl/description
 Kernel module to provide platform specifics like bootloader version (which needs to be GPLv2 licensed).
endef

EXTRA_KCONFIG:= \
	CONFIG_PLATFORM_GPL=m

ifneq ($(CONFIG_KPANIC_IN_PROZONE),)
EXTRA_KCONFIG += \
       CONFIG_KPANIC_IN_PROZONE=y
endif

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(KERNEL_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define KernelPackage/platform-gpl/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/platform-gpl $(1)/etc/init.d
endef

$(eval $(call KernelPackage,platform-gpl))
