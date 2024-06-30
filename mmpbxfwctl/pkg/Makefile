#
# Copyright (c) 2018 Technicolor
# All Rights Reserved
#                                                                       
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License version 2 as    
# published by the Free Software Foundation.                           
#
include $(TOPDIR)/rules.mk

PKG_NAME:=mmpbxfwctl
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/mmpbxfwctl.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=d65d2c6a1bfb18cbfbe9693c40550634df33b527
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_CONFIG_DEPENDS :=

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/mmpbxfwctl
	SECTION:=mmpbxfwctl
	CATEGORY:=Technicolor
	SUBMENU:=Voice
	TITLE:=mmpbx firewall controller
	DEPENDS:=+libip4tc
	MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define Package/mmpbxfwctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mmpbxfwctl $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/mmpbxfwctl $(1)/etc/init.d/
endef


$(eval $(call BuildPackage,mmpbxfwctl))
