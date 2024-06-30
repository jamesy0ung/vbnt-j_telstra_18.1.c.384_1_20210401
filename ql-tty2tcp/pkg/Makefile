#
# Copyright (c) 2017 Technicolor
# All Rights Reserved
#
##                                                                      
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License version 2 as    
## published by the Free Software Foundation.                          
#
include $(TOPDIR)/rules.mk

PKG_NAME:=ql-tty2tcp
PKG_VERSION:=1.0

include $(INCLUDE_DIR)/package.mk

define Package/ql-tty2tcp
	SECTION:=ql-tty2tcp
	CATEGORY:=Technicolor
	SUBMENU:=LTE
	TITLE:=Quectel tool for making QXDM logs over the network
	MAINTAINER:=Technicolor <ltegw@technicolor.com>
	DEPENDS:=+libpthread
endef

define Package/ql-tty2tcp/description
	Quectel tool for making QXDM logs over the network
endef

define Build/Prepare
	$(CP) src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE_VARS) $(MAKE) $(MAKE_FLAGS) $(EXTRA_OPTS) -C $(PKG_BUILD_DIR)
endef

define Package/ql-tty2tcp/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ql-tty2tcp $(1)/usr/bin/ql-tty2tcp
endef

$(eval $(call BuildPackage,ql-tty2tcp))
