KERNEL_VER :=$(shell uname -r)
KERNEL_DIR = '/lib/modules/$(KERNEL_VER)/build'

obj-m := dht_dev.o

PWD := $(shell pwd)

all :
	make -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
