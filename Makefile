KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	rm -rf *.mod.c *.ko *.o .*.cmd .tmp_versions Module.markers modules.order Module.symvers
