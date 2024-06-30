#
# COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE
# Copyright (c) 2014 - Technicolor Delivery Technologies, SAS          
# All Rights Reserved
#                                                                      
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License version 2 as    
# published by the Free Software Foundation.                           
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk


PKG_NAME:=weburl-nfq
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/urlfilterd-nfq
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=3f9b0a1400792968a77b361bba0f47d4f205f789
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/weburl-nfq
  SECTION:=Firewall
  CATEGORY:=Technicolor
  SUBMENU:=Network
  TITLE:=Technicolor WEB URL filter for parental control
  DEPENDS:=+liburcu +libnetfilter-queue +libnetfilter-conntrack +libubox +libuci +libubus +libblobmsg-json +iptables-mod-conntrack-extra
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef


define Package/weburl-nfq/description
  Technicolor WEB URL Filter for parental control site blocking.
endef

define Package/weburl-nfq/install
	$(INSTALL_DIR) $(1)/etc/init.d $(1)/usr/sbin
	$(INSTALL_BIN) ./files/weburl.init $(1)/etc/init.d/weburl

	$(INSTALL_DATA) ./files/weburl_cfg_tmpl $(1)/etc
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/urlfilterd $(1)/usr/sbin/
endef

CMAKE_OPTIONS += -DLINUX_VERSION=$(LINUX_VERSION)

$(eval $(call BuildPackage,weburl-nfq))

