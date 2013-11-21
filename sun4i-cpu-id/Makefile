obj-m += cpu_id.o

KERNEL=/usr/src/linux-headers-3.4.29+
all:
	make -C $(KERNEL) modules M=`pwd`
clean:
	make -C $(KERNEL) modules M=`pwd` clean
