#
# Copyright (C) 2006-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

LEDS_MENU:=LED modules

define KernelPackage/leds-gpio
  SUBMENU:=$(LEDS_MENU)
  TITLE:=GPIO LED support
  DEPENDS:= @GPIO_SUPPORT
  KCONFIG:=CONFIG_LEDS_GPIO
  FILES:=$(LINUX_DIR)/drivers/leds/leds-gpio.ko
  AUTOLOAD:=$(call AutoLoad,60,leds-gpio,1)
endef

define KernelPackage/leds-gpio/description
 Kernel module for LEDs on GPIO lines
endef

$(eval $(call KernelPackage,leds-gpio))

LED_TRIGGER_DIR=$(LINUX_DIR)/drivers/leds$(if $(call kernel_patchver_ge,3.10),/trigger)


define KernelPackage/ledtrig-heartbeat
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED Heartbeat Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_HEARTBEAT
  FILES:=$(LED_TRIGGER_DIR)/ledtrig-heartbeat.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-heartbeat)
endef

define KernelPackage/ledtrig-heartbeat/description
 Kernel module that allows LEDs to blink like heart beat
endef

$(eval $(call KernelPackage,ledtrig-heartbeat))


define KernelPackage/ledtrig-gpio
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED GPIO Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_GPIO
  FILES:=$(LED_TRIGGER_DIR)/ledtrig-gpio.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-gpio)
endef

define KernelPackage/ledtrig-gpio/description
 Kernel module that allows LEDs to be controlled by gpio events
endef

$(eval $(call KernelPackage,ledtrig-gpio))


define KernelPackage/ledtrig-netdev
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED NETDEV Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_NETDEV
  FILES:=$(LINUX_DIR)/drivers/leds/ledtrig-netdev.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-netdev)
endef

define KernelPackage/ledtrig-netdev/description
 Kernel module to drive LEDs based on network activity
endef

$(eval $(call KernelPackage,ledtrig-netdev))


define KernelPackage/ledtrig-netfilter
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED NetFilter Trigger
  DEPENDS:=kmod-ipt-core
  KCONFIG:=CONFIG_NETFILTER_XT_TARGET_LED
  FILES:=$(LINUX_DIR)/net/netfilter/xt_LED.ko
  AUTOLOAD:=$(call AutoLoad,50,xt_LED)
endef

define KernelPackage/ledtrig-netfilter/description
 Kernel module to flash LED when a particular packets passing through your machine.

 For example to create an LED trigger for incoming SSH traffic:
    iptables -A INPUT -p tcp --dport 22 -j LED --led-trigger-id ssh --led-delay 1000
 Then attach the new trigger to an LED on your system:
    echo netfilter-ssh > /sys/class/leds/<ledname>/trigger
endef

$(eval $(call KernelPackage,ledtrig-netfilter))


define KernelPackage/ledtrig-usbdev
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED USB device Trigger
  DEPENDS:=@USB_SUPPORT kmod-usb-core
  KCONFIG:=CONFIG_LEDS_TRIGGER_USBDEV
  FILES:=$(LINUX_DIR)/drivers/leds/ledtrig-usbdev.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-usbdev)
endef

define KernelPackage/ledtrig-usbdev/description
 Kernel module to drive LEDs based on USB device presence/activity
endef

$(eval $(call KernelPackage,ledtrig-usbdev))


define KernelPackage/ledtrig-default-on
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED Default ON Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_DEFAULT_ON
  FILES:=$(LED_TRIGGER_DIR)/ledtrig-default-on.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-default-on,1)
endef

define KernelPackage/ledtrig-default-on/description
 Kernel module that allows LEDs to be initialised in the ON state
endef

$(eval $(call KernelPackage,ledtrig-default-on))


define KernelPackage/ledtrig-timer
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED Timer Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_TIMER
  FILES:=$(LED_TRIGGER_DIR)/ledtrig-timer.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-timer,1)
endef

define KernelPackage/ledtrig-timer/description
 Kernel module that allows LEDs to be controlled by a programmable timer
 via sysfs
endef

$(eval $(call KernelPackage,ledtrig-timer))

define KernelPackage/ledtrig-bundletimer
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED Bundle Timer Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_BUNDLETIMER
  FILES:=$(LED_TRIGGER_DIR)/ledtrig-bundletimer.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-bundletimer,1)
endef

define KernelPackage/ledtrig-bundletimer/description
 Kernel module that allows LEDs to be controlled by a programmable timer
 via sysfs.
endef

$(eval $(call KernelPackage,ledtrig-bundletimer))

define KernelPackage/ledtrig-pattern
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED Pattern Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_PATTERN
  FILES:=$(LED_TRIGGER_DIR)/ledtrig-pattern.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-pattern,1)
endef

define KernelPackage/ledtrig-pattern/description
 Kernel module that allows LEDs to be configured for
 a specific blink pattern.
endef

$(eval $(call KernelPackage,ledtrig-pattern))

define KernelPackage/ledtrig-transient
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED Transient Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_TRANSIENT
  FILES:=$(LED_TRIGGER_DIR)/ledtrig-transient.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-transient,1)
endef

define KernelPackage/ledtrig-transient/description
 Kernel module that allows LEDs one time activation of a transient state.
endef

$(eval $(call KernelPackage,ledtrig-transient))


define KernelPackage/ledtrig-oneshot
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LED One-Shot Trigger
  KCONFIG:=CONFIG_LEDS_TRIGGER_ONESHOT
  FILES:=$(LED_TRIGGER_DIR)/ledtrig-oneshot.ko
  AUTOLOAD:=$(call AutoLoad,50,ledtrig-oneshot)
endef

define KernelPackage/ledtrig-oneshot/description
 Kernel module that allows LEDs to be triggered by sporadic events in
 one-shot pulses
endef

$(eval $(call KernelPackage,ledtrig-oneshot))


define KernelPackage/leds-pca963x
  SUBMENU:=$(LEDS_MENU)
  TITLE:=PCA963x LED support
  DEPENDS:=+kmod-i2c-core
  KCONFIG:=CONFIG_LEDS_PCA963X
  FILES:=$(LINUX_DIR)/drivers/leds/leds-pca963x.ko
  AUTOLOAD:=$(call AutoLoad,60,leds-pca963x,1)
endef

define KernelPackage/leds-pca963x/description
 Driver for the NXP PCA963x I2C LED controllers.
endef

$(eval $(call KernelPackage,leds-pca963x))


define KernelPackage/leds-tlc59116
  SUBMENU:=$(LEDS_MENU)
  TITLE:=TLC59116 LED support
  DEPENDS:=@TARGET_mvebu +kmod-i2c-core +kmod-regmap
  KCONFIG:=CONFIG_LEDS_TLC59116
  FILES:=$(LINUX_DIR)/drivers/leds/leds-tlc59116.ko
  AUTOLOAD:=$(call AutoLoad,60,leds-tlc59116,1)
endef

define KernelPackage/leds-tlc59116/description
 Kernel module for LEDs on TLC59116
endef

$(eval $(call KernelPackage,leds-tlc59116))


define KernelPackage/leds-lp55xx-common
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LP555xx common LED support
  DEPENDS:=+kmod-i2c-core
  KCONFIG:=CONFIG_LEDS_LP55XX_COMMON
  FILES:=$(LINUX_DIR)/drivers/leds/leds-lp55xx-common.ko
  AUTOLOAD:=$(call AutoLoad,60,leds-lp55xx-common,1)
endef

define KernelPackage/leds-lp55xx/description
 Common kernel module for LEDs on LP55xx
endef

$(eval $(call KernelPackage,leds-lp55xx-common))


define KernelPackage/leds-lp5523
  SUBMENU:=$(LEDS_MENU)
  TITLE:=LP5523/55231 LED support
  DEPENDS:=+kmod-i2c-core +kmod-leds-lp55xx-common
  KCONFIG:=CONFIG_LEDS_LP5523
  FILES:=$(LINUX_DIR)/drivers/leds/leds-lp5523.ko
  AUTOLOAD:=$(call AutoLoad,61,leds-lp5523,1)
endef

define KernelPackage/leds-lp5523/description
 Kernel module for LEDs on LP5523
endef

$(eval $(call KernelPackage,leds-lp5523))
