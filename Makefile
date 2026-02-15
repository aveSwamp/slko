.PHONY: all clean load unload remove_build help


MVERMAGIC 		 := "vermagic=$(shell sudo modinfo -F vermagic $$(sudo lsmod | head -n2 | tail -n1 | cut -d" " -f1))"
MPRINTK 		 := 0x$(shell sudo grep " T _printk$$" /proc/kallsyms | cut -d" " -f1)
MPARAM_OPS_INT 	 := 0x$(shell sudo grep " D param_ops_int$$" /proc/kallsyms | cut -d" " -f1)
MPARAM_OPS_CHARP := 0x$(shell sudo grep " D param_ops_charp$$" /proc/kallsyms | cut -d" " -f1)
MKFREE 			 := 0x$(shell sudo grep " T kfree$$" /proc/kallsyms | cut -d" " -f1)
MMSLEEP			 := 0x$(shell sudo grep " T msleep$$" /proc/kallsyms | cut -d" " -f1)
MFILP_OPEN	 	 := 0x$(shell sudo grep " T filp_open$$" /proc/kallsyms | cut -d" " -f1)
MFILP_CLOSE		 := 0x$(shell sudo grep " T filp_close$$" /proc/kallsyms | cut -d" " -f1)
MKERNEL_WRITE	 := 0x$(shell sudo grep " T kernel_write$$" /proc/kallsyms | cut -d" " -f1)
MSNPRINTF	 	 := 0x$(shell sudo grep " T snprintf$$" /proc/kallsyms | cut -d" " -f1)
MKTHREAD_CREATE	 := 0x$(shell sudo grep " T kthread_create_on_node$$" /proc/kallsyms | cut -d" " -f1)
MWAKE_UP_PROCESS := 0x$(shell sudo grep " T wake_up_process$$" /proc/kallsyms | cut -d" " -f1)



CFLAGS_REMOVE_slko.o = -pg -mrecord-mcount -mfentry


ccflags-y := -DKFREE_ADDR=$(MKFREE) \
			 -DPRINTK_ADDR=$(MPRINTK) \
			 -DPARAM_OPS_INT_ADDR=$(MPARAM_OPS_INT) \
			 -DPARAM_OPS_CHARP_ADDR=$(MPARAM_OPS_CHARP) \
			 -DVERMAGIC_REVERSED='$(MVERMAGIC)' \
			 -DMSLEEP_ADDR=$(MMSLEEP) \
			 -DFILP_OPEN_ADDR=$(MFILP_OPEN) \
			 -DFILP_CLOSE_ADDR=$(MFILP_CLOSE) \
			 -DKERNEL_WRITE_ADDR=$(MKERNEL_WRITE) \
			 -DSNPRINTF_ADDR=$(MSNPRINTF) \
			 -DKTHREAD_CREATE_ADDR=$(MKTHREAD_CREATE) \
			 -DWAKE_UP_PROCESS_ADDR=$(MWAKE_UP_PROCESS)

SLKO_BUILD_DIR 	  := /lib/modules/$(shell uname -r)/slko
SLKO_SETTINGS_DIR := $(SLKO_BUILD_DIR)/slko_settings

SLKO_LOGGING_PATH := /var/tmp/test_module/

SLKO_LOGGING_FILE_PATH := $(SLKO_LOGGING_PATH)$(shell [ -f $(SLKO_SETTINGS_DIR)/slko.conf ] && cat $(SLKO_SETTINGS_DIR)/slko.conf | tail -n1 | cut -d" " -f3)
SLKO_LOGGING_TIMER := $(shell [ -f $(SLKO_SETTINGS_DIR)/slko.conf ] && cat $(SLKO_SETTINGS_DIR)/slko.conf | head -n1 | cut -d" " -f3)

obj-m += slko.o
slko-y := kernel_module/slko.o

all:
	@make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	@if [ ! -d "$(SLKO_SETTINGS_DIR)" ]; then \
		sudo mkdir -p $(SLKO_SETTINGS_DIR); \
	fi
	@sudo mv ./slko.ko $(SLKO_BUILD_DIR)
	@gcc ./module_settings/settings.c -DSLKO_CONFIG_PATH='"$(SLKO_SETTINGS_DIR)/slko.conf"' -DKERNEL_LOADED_PATH='"$(SLKO_SETTINGS_DIR)/kernel_loaded"' -o $(SLKO_SETTINGS_DIR)/settings
	@sudo ln -sf $(SLKO_SETTINGS_DIR)/settings /usr/bin/slko_settings
	@sudo mkdir -p $(SLKO_LOGGING_PATH)
	@sudo depmod -a
	@echo " "
	@echo "    [INFO]: to clean the directory of intermediate files use >make clean"
	@echo "    [INFO]: to remove builded module from system use >make remove_build"
	@echo " "
	@echo "    [WARNING]: setup parameters for slko.ko or it cant start. To set up kernel module parameters use >slko_settings"
	@echo " "


clean:
	@make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

remove_build:
	@sudo rm -rf $(SLKO_BUILD_DIR)
	@sudo rm -f /usr/bin/slko_settings
	@sudo rm -rf $(SLKO_LOGGING_PATH)

load:
	@if [ ! -f "$(SLKO_SETTINGS_DIR)/slko.conf" ]; then  \
		echo "    [ERROR]: slko.conf does not exist please use >slko_settings"; \
		exit 1; \
	fi
	@sudo insmod $(SLKO_BUILD_DIR)/slko.ko file_path=$(SLKO_LOGGING_FILE_PATH) set_timer=$(SLKO_LOGGING_TIMER)
	@echo 1 | sudo tee $(SLKO_SETTINGS_DIR)/kernel_loaded > /dev/null

unload:
	@sudo rmmod $(SLKO_BUILD_DIR)/slko.ko
	@sudo rm -f $(SLKO_SETTINGS_DIR)/kernel_loaded

help:
	@cat ./help_msg