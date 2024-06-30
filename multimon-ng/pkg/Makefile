#
# Copyright (C) 2018 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=multimon-ng
PKG_VERSION:=1.1.4

PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/EliasOenal/multimon-ng.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=9548e270956fd7a23fba356e14e6a02ba769a0f1
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_LICENSE:=GPL-2.0
#PKG_LICENSE_FILES:=

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/multimon-ng
  CATEGORY:=Technicolor
  SUBMENU:=utils
  TITLE:=multimon-ng
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  MENU=1
endef

define Package/multimon-ng/description
    multimon - program to decode radio transmissions
endef

define Package/multimon-ng/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/multimon-ng $(1)/usr/bin/
endef

$(eval $(call BuildPackage,multimon-ng))

