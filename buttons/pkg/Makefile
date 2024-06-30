########### COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE #############
## Copyright (c) 2012-2017 - Technicolor Delivery Technologies, SAS     ##
## All Rights Reserved                                                  ##
##
## This program is free software; you can redistribute it and/or modify ##
## it under the terms of the GNU General Public License version 2 as    ##
## published by the Free Software Foundation.                           ##
##########################################################################


include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=technicolor-button
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/buttons.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=8edf33f839b0e72116a646270262a3e54cf301ca
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk

define Package/technicolor-button-userspace
  SECTION:=Button
  CATEGORY:=Technicolor
  SUBMENU:=General
  TITLE:=Technicolor Userspace Tools for Handling Button Hotplug Events
  DEPENDS:=@(TARGET_brcm63xx_tch||TARGET_brcm68xx_tch||TARGET_comcerto2000||TARGET_ar71xx_tch||TARGET_mdm9x07||TARGET_apl_alpinev2||TARGET_ipq806x_tch||TARGET_lantiq_tch) +(!TARGET_lantiq_tch):kmod-technicolor-button +kmod-button-hotplug +kmod-input-gpio-keys-polled
  DEFAULT:=y if (TARGET_brcm63xx_tch||TARGET_brcm68xx_tch||TARGET_comcerto2000||TARGET_ar71xx_tch||TARGET_mdm9x07||TARGET_apl_alpinev2||TARGET_ipq806x_tch||TARGET_lantiq_tch)
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define KernelPackage/technicolor-button
  SUBMENU:=Other modules
  TITLE:=Button support for Technicolor gateways
  DEPENDS:=@(TARGET_brcm63xx_tch||TARGET_brcm68xx_tch||TARGET_comcerto2000||TARGET_ar71xx_tch||TARGET_mdm9x07||TARGET_apl_alpinev2||TARGET_ipq806x_tch) +kmod-button-hotplug +kmod-input-gpio-keys-polled
  FILES:=$(PKG_BUILD_DIR)/technicolor_button.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  AUTOLOAD:=$(call AutoLoad,01,technicolor_button)
  DEFAULT:=y if (TARGET_brcm63xx_tch||TARGET_brcm68xx_tch||TARGET_comcerto2000||TARGET_ar71xx_tch||TARGET_mdm9x07||TARGET_apl_alpinev2||TARGET_ipq806x_tch)
endef

define Package/technicolor-button-userspace/description
  Technicolor Userspace Tools for Handling Button Hotplug Events
endef

define KernelPackage/technicolor-button/description
 Kernel module to provide button support for Technicolor gateways.
endef

EXTRA_KCONFIG:= \
	CONFIG_TECHNICOLOR_BUTTON=m

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="-DBOARD_$(SUBTARGET)"\
	$(EXTRA_KCONFIG)

define Build/Compile/technicolor-button
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define Build/Compile
	$(if $(CONFIG_PACKAGE_kmod-technicolor-button),$(Build/Compile/technicolor-button))
endef

define Package/technicolor-button-userspace/install
	$(INSTALL_DIR) $(1)/etc/hotplug.d/button
	$(INSTALL_DATA) ./files/etc/hotplug.d/button/00-button $(1)/etc/hotplug.d/button/00-button
endef

$(eval $(call BuildPackage,technicolor-button-userspace))
$(eval $(call KernelPackage,technicolor-button))
