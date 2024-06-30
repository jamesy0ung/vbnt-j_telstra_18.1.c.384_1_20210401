########### COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE #############
## Copyright (c) 2016-2017 - Technicolor Delivery Technologies, SAS     ##
## All Rights Reserved                                                  ##
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License version 2 as    
# published by the Free Software Foundation.                           
##########################################################################

include $(TOPDIR)/rules.mk

PKG_NAME:=dhcpsnooper-tch
PKG_VERSION:=1.1
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/dhcpsnooper.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=460e75e87bc79adb0485605520adfa3eaaf178e8
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_CONFIG_DEPENDS :=

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/dhcpsnooper-tch
  SECTION:=dhcpsnooper
  CATEGORY:=Technicolor
  SUBMENU:=Network
  TITLE:=DHCP snooper
  DEPENDS:=+libubox +libuci +libubus +libnetfilter-queue +iptables-mod-nfqueue
  MAINTAINER:=Alin Nastac <alin.nastac@technicolor.com>
endef

define Package/dhcpsnooper-tch/description
  Daemon implementing layer 2 DHCP snooping.
endef

define Package/dhcpsnooper-tch/conffiles
  /etc/config/dhcpsnooping
endef

define Package/dhcpsnooper-tch/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/dhcpsnooper $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(PKG_NAME).init $(1)/etc/init.d/dhcpsnooper
	$(INSTALL_DIR) $(1)/lib/functions
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_DATA) ./files/$(PKG_NAME).defaults $(1)/etc/uci-defaults/70-dhcpsnooper
	$(INSTALL_BIN) ./files/firewall.include $(1)/lib/functions/firewall-dhcpsnooper.sh
endef

define Package/dhcpsnooper-tch/postinst
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Enabling DHCP snooper at start-up"
        /etc/init.d/dhcpsnooper enable
	[ -f /etc/config/dhcpsnooping ] || touch /etc/config/dhcpsnooping 
fi
exit 0
endef

define Package/dhcpsnooper-tch/prerm
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Disabling DHCP snooper at start-up"
        /etc/init.d/dhcpsnooper disable
fi
exit 0
endef

$(eval $(call BuildPackage,dhcpsnooper-tch))
