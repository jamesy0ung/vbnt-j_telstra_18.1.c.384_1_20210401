#
#########COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE##############
# Copyright (c) 2017 - Technicolor Delivery Technologies, SAS          
# All Rights Reserved                                                  
#                                                                      
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License version 2 as    
# published by the Free Software Foundation.   
#

PKG_NAME:=qflash
PKG_VERSION:=1.4.0

include $(INCLUDE_DIR)/package.mk

define Package/qflash
	SECTION:=qflash
	CATEGORY:=Technicolor
	SUBMENU:=LTE
	TITLE:=Quectel firmware upgrade utility
	MAINTAINER:=Technicolor <ltegw@technicolor.com>
	DEPENDS:=+libstdcpp +librt
endef

define Package/qflash/description
	Quectel LTE module upgrade tool
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
	$(MAKE) $(MAKE_FLAGS) $(EXTRA_OPTS) -C $(PKG_BUILD_DIR)
endef

define Package/qflash/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/QFlash $(1)/usr/bin/qflash
endef

$(eval $(call BuildPackage,qflash))
