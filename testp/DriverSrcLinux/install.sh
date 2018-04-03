	mknod /dev/MSRdrv c 222 0
	chmod 666 /dev/MSRdrv
	insmod -f MSRdrv.ko
