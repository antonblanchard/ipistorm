obj-m := ipistorm.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.mod.c *.ko *.o .*.cmd .tmp_versions Module.markers modules.order Module.symvers
