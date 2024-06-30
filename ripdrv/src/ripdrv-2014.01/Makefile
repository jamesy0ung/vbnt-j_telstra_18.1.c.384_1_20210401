ifndef KERNELRELEASE
##################################################
##################################################
#
#  BUILDING THIS CODE FOR U-BOOT
#
$(warning Compiling for u-boot)
##################################################
##################################################


UBOOT_WRAP_CFLAGS := -I$(TOPDIR)$(srctree)
UBOOT_WRAP_CFLAGS += -I$(TOPDIR)$(srctree)/rip/include
UBOOT_WRAP_CFLAGS += -I$(TOPDIR)$(srctree)/rip/target/uboot_bootloader/include
UBOOT_WRAP_CFLAGS += -I$(TOPDIR)$(srctree)/include
UBOOT_WRAP_CFLAGS += -I$(TOPDIR)$(srctree)/technicolor/rip
UBOOT_WRAP_CFLAGS += -I$(TOPDIR)$(srctree)/technicolor/otp/include
UBOOT_WRAP_CFLAGS += -I$(TOPDIR)$(srctree)/include/linux
UBOOT_WRAP_CFLAGS += -I$(TOPDIR)$(srctree)/lib/tommath
UBOOT_WRAP_CFLAGS += -I$(TOPDIR)$(srctree)/lib/tomcrypt/src/headers

UBOOT_WRAP_OBJECTS := rip2lib.o
UBOOT_WRAP_OBJECTS += crc32.o
UBOOT_WRAP_OBJECTS += efu_common.o
UBOOT_WRAP_OBJECTS += efu_privileged.o
UBOOT_WRAP_OBJECTS += target/uboot_bootloader/platform.o
UBOOT_WRAP_OBJECTS += target/uboot_bootloader/crypto_api.o
UBOOT_WRAP_OBJECTS += target/uboot_bootloader/crypto_fixup.o

ifneq ($(filter alpine_db ipq40xx_cdp,$(BOARD)),)
UBOOT_WRAP_OBJECTS += target/uboot_bootloader/$(BOARD)/crypto_bl.o
UBOOT_WRAP_OBJECTS += crypto.o
UBOOT_WRAP_CFLAGS += -DCONFIG_RIPDRV_CRYPTO_SUPPORT -DCONFIG_RIPDRV_INTEGRITY_ONLY
else
$(warning TODO - Crypto support for GRX500)
endif

UBOOT_WRAP_LIB := librip

include $(TOPDIR)$(srctree)/tch_libwrapper.mk

else # KERNELRELEASE
##################################################
##################################################
#
#  BUILDING THIS CODE FOR LINUX KERNEL MODULE
#
$(warning Compiling for kernel module)
##################################################
##################################################


obj-${CONFIG_RIPDRV} += ripdrv.o

ripdrv-y := main.o rip2lib.o rip2_cert_privkey.o target/linux/platform.o flashrip_drv.o rip_proc.o efu_privileged.o efu_common.o rip_efu_proc.o target/linux/crypto_api.o
ccflags-y += -I$(src)/include -I$(src)/target/linux/include/ -I$(src)/target/linux/${TARGET}/include/

ifneq ($(CONFIG_RIP_ON_UBI),)
ripdrv-y += target/linux/ubi.o
else
ripdrv-y += target/linux/mtd.o
endif

ifneq ($(CONFIG_RIPDRV_CRYPTO_SUPPORT),)
ccflags-y += -DCONFIG_RIPDRV_CRYPTO_SUPPORT
ripdrv-y += crypto.o target/linux/crypto_lx.o
endif

ifneq ($(CONFIG_RIPDRV_EXPOSE_PRIVATES),)
ccflags-y += -DCONFIG_RIPDRV_EXPOSE_PRIVATES
endif

ifneq ($(CONFIG_RIPDRV_INTEGRITY_ONLY),)
ccflags-y += -DCONFIG_RIPDRV_INTEGRITY_ONLY
endif

ifneq ($(CONFIG_RIPDRV_ANVIL),)
ccflags-y += -DCONFIG_RIPDRV_ANVIL
endif

ifneq ($(CONFIG_ARM),)
ccflags-y += -DCONFIG_ARM
endif

ccflags-y += -Wall -Werror

clean:
	rm -f core *.o *.a *.s *~

endif # KERNELRELEASE
