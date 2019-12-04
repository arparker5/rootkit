ifneq ($(KERNELRELEASE),)
	obj-m := lkmr.o
 CFLAGS_[filename].o := -DDEBUG

else
	KERNELDIR ?=/lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	INC=-I /usr/include -I /usr/include/x86_64-linux-gnu/

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules $(INC)

endif


clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean $(INC)
