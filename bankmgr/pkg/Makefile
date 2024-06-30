#
###########COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE############
# Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          
# All Rights Reserved                                                  
#                                                                     
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License version 2 as    `
# published by the Free Software Foundation.                           


include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bankmgr
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/bankmgr.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=aede602ae6bf2adf76f20d50ec410311c46c05f7


PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk




define KernelPackage/bankmgr
  SUBMENU:=Technicolor specific kernel modules
  TITLE:=Bankmanager kernel module
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/bankmgr.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  AUTOLOAD:=$(call AutoLoad,43,bankmgr,1)
  DEFAULT:=
endef

define KernelPackage/bankmgr/description
 Kernel module that provides access to the banktable via a proc interface.
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,bankmgr))
