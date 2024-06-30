obj-m := GobiNet.o
GobiNet-objs := GobiUSBNet.o QMIDevice.o QMI.o usbnet_2_6_32.o usbnet_3_0_6.o \
		usbnet_2_6_35.o usbnet_3_10_21.o usbnet_3_12_xx.o
KDIR := /local/mnt/workspace/bselvara/ws/qsdk/build_dir/target-arm_cortex-a7_uClibc-0.9.33.2_eabi/linux-ipq806x/linux-3.14.43
PWD := $(shell pwd)
OUTPUTDIR=/lib/modules/`uname -r`/kernel/drivers/net/usb/
#KBUILD_CFLAGS += -DQOS_SIMULATE
#KBUILD_CFLAGS += -DTX_XMIT_SIERRA -DTX_URB_MONITOR
TX_URB_MONITOR := 0
ifeq ($(TX_URB_MONITOR), 1)
	ccflags-y:=-DTX_URB_MONITOR
endif

RAWIP := 0
ifeq ($(RAWIP), 1)
	ccflags-y:=-DDATA_MODE_RP
endif

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules


install: all
	mkdir -p $(OUTPUTDIR)
	cp -f GobiNet.ko $(OUTPUTDIR)
	depmod

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.* modules.order


