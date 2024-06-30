#
# Copyright (c) 2017 Technicolor
# All Rights Reserved
#
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License version 2 as    
# published by the Free Software Foundation.                           
#
include $(TOPDIR)/rules.mk

PKG_NAME:=qmbnflash
PKG_VERSION:=1.0

include $(INCLUDE_DIR)/package.mk

define Package/qmbnflash
	SECTION:=qmbnflash
	CATEGORY:=Technicolor
	SUBMENU:=LTE
	TITLE:=Quectel MBN upgrade tool
	MAINTAINER:=Technicolor <ltegw@technicolor.com>
	DEPENDS:=+libpthread
endef

define Package/qmbnflash/description
	Quectel MBN upgrade tool
endef

define Build/Prepare
	$(CP) src/* $(PKG_BUILD_DIR)/
	@if [ -d ./patches -a "$$$$(ls ./patches | wc -l)" -gt 0 ]; then \
		$(KPATCH) $(PKG_BUILD_DIR) ./patches; \
	fi
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE_VARS) \
	$(MAKE) $(MAKE_FLAGS) $(EXTRA_OPTS) -C $(PKG_BUILD_DIR) linux
endef

define Package/qmbnflash/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/qflash $(1)/usr/bin/qmbnflash
endef

$(eval $(call BuildPackage,qmbnflash))
