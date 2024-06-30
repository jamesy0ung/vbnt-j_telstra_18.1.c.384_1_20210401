#
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libcli
PKG_VERSION:=1.9.7
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/dparrish/libcli
PKG_SOURCE_DIR:=$(PKG_NAME)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=ae56ebcf8189861bf75f1ad6656f0b28cacaafc8

include $(INCLUDE_DIR)/package.mk

define Package/libcli
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A Cisco-like CLI library
  URL:=http://code.google.com/p/libcli/
  DEPENDS:=+EXTERNAL_TOOLCHAIN:libc +EXTERNAL_TOOLCHAIN:libgcc
endef

define Package/libcli/description
	Libcli provides a shared library for including a Cisco-like
	command-line interface into other software. It's a telnet
	interface which supports command-line editing, history,
	authentication and callbacks for a user-definable function tree.
endef

TARGET_CFLAGS += $(FPIC)

MAKE_FLAGS += \
	DEBUG="" \
	OPTIM="$(TARGET_CFLAGS)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	PREFIX="/usr" \
	DESTDIR="$(PKG_INSTALL_DIR)" \
	all install

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libcli.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcli.so* $(1)/usr/lib/
endef

define Package/libcli/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcli.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libcli))
