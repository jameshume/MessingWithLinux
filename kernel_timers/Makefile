obj-m += kernel_timers_play.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: all
all:
	$(MAKE) M=$(PWD) -C $(KDIR) modules V=1

.PHONY: clean
clean:
	$(MAKE) M=$(PWD) -C $(KDIR) clean V=1
