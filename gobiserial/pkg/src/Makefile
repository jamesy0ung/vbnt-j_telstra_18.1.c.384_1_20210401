obj-m := GobiSerial.o
KDIR := /local/mnt/workspace/bselvara/ws/qsdk/build_dir/target-arm_cortex-a7_uClibc-0.9.33.2_eabi/linux-ipq806x/linux-3.14.43
PWD := $(shell pwd)
OUTPUTDIR=/lib/modules/`uname -r`/kernel/drivers/usb/serial/

all: clean
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install: all
	mkdir -p $(OUTPUTDIR)
	cp -f GobiSerial.ko $(OUTPUTDIR)
	depmod

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.* modules.order
