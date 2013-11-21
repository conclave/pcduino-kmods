PWD=$(shell pwd)
KDIR=/usr/src/linux-headers-3.4.29+
all:
	$(MAKE) -C ${KDIR} M=$(PWD)/driver
	$(CC) regtool.c -o regtool

clean:
	$(MAKE) -C ${KDIR} M=$(PWD)/driver clean
	@rm -f regtool

