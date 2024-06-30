# The source code form of this Open Source Project components is
# subject to the terms of the Clear BSD license.
# You can redistribute it and/or modify it under the terms of the
# Clear BSD License (http://directory.fsf.org/wiki/License:ClearBSD)
# See COPYING file/LICENSE file for more details.
# This software project does also include third party Open Source
# Software: See COPYING file/LICENSE file for more details.

include $(TOPDIR)/rules.mk

PKG_NAME:=preloader-tch
PKG_VERSION:=1.0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/preloader-tch
	SECTION:=preloader-tch
	CATEGORY:=Technicolor
	SUBMENU:=Network
	TITLE:=Technicolor preloader package
	MAINTAINER:=Thierry Du Tre <thierry.dutre@technicolor.com>
endef

define Package/preloader-tch/description
	Shared object to be used as preloader in order to support mwan and QoS functionality for host applications.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/preloader-tch/install
	$(INSTALL_DIR) $(1)/etc/
	$(INSTALL_DATA) ./files/etc/ld.so.preload $(1)/etc/
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libpreload-tch.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,preloader-tch))
