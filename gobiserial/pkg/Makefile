#
# Copyright (c) 2012 Technicolor
# All Rights Reserved
#
# it under the terms of the GNU General Public License version 2 as    
# published by the Free Software Foundation.                           
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gobiserial
PKG_VERSION:=2.25

include $(INCLUDE_DIR)/package.mk

define KernelPackage/gobiserial
  SUBMENU:=USB Support
  TITLE:=Sierra Wireless GobiSerial driver
  FILES:=$(PKG_BUILD_DIR)/GobiSerial.ko
  KCONFIG:=
  DEPENDS:=+kmod-usb-serial
  AUTOLOAD:=$(call AutoLoad,80,GobiSerial)
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

EXTRA_KCONFIG:= \
	CONFIG_GOBISERIAL=m

define KernelPackage/gobiserial/description
 	Kernel module that provides access to Sierra Wireless Gobi based devices
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(PKG_EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
	$(MAKE_OPTS) \
	modules
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
	@if [ -d ./patches -a "$$$$(ls ./patches | wc -l)" -gt 0 ]; then \
		$(KPATCH) $(PKG_BUILD_DIR) ./patches; \
	fi
endef

$(eval $(call KernelPackage,gobiserial))

