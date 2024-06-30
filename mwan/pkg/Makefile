#
# Copyright (c) 2017 Technicolor
# All Rights Reserved
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mwan
PKG_VERSION:=4.1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/mwan
  SECTION:=mwan
  CATEGORY:=Technicolor
  SUBMENU:=Network
  TITLE:=Multiwan package
  DEPENDS:=+iptables +ip6tables +ip +iptables-mod-conntrack-extra +iptables-mod-ipopt +iptables-mod-extra +busybox +libnetfilter-queue +libpthread +libubus +libblobmsg-json +libuci
  DEPENDS+=+preloader-tch
  MAINTAINER:=Hans Dedecker <hans.dedecker@technicolor.com>
endef

define Package/mwan/description
	Script which allows to configure policy based traffic routing based on traffic rules in case multiple WAN interfaces are present
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/mwan/conffiles
	/etc/config/mwan
endef

define Package/mwan/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/mwan $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/pre-mwan $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_DATA) ./files/etc/hotplug.d/iface/15-mwan $(1)/etc/hotplug.d/iface
	$(INSTALL_DIR) $(1)/usr/lib/mwan
	$(INSTALL_BIN) ./files/usr/lib/mwan/setup.sh $(1)/usr/lib/mwan
	$(INSTALL_BIN) ./files/usr/lib/mwan/functions.sh $(1)/usr/lib/mwan
	$(INSTALL_DIR) $(1)/etc/config
	touch $(1)/etc/config/mwan
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/load_balancer $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/lib/mwan/load_balancer.sh $(1)/usr/lib/mwan
endef

$(eval $(call BuildPackage,mwan))
