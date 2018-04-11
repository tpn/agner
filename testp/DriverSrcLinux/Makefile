# Make MSR driver for Linux
KERNELDIR := /lib/modules/`uname -r`/build
obj-m := MSRdrv.o
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f .MSRdrv.*
	rm -f -r .tmp_versions
	rm -f *~
	rm -f MSRdrv.ko
	rm -f MSRdrv.o
	rm -f MSRdrv.mod.*
	rm -f linux.mdl
	rm -f Modules.symvers
    
install:
	./install.sh
    