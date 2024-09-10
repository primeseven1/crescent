KERNEL_FILE := ./crescent-kernel

CONFIG_LLVM ?= no
CONFIG_DEBUG ?= no
CONFIG_OPTIMIZATION ?= 2
CONFIG_E9_ENABLE ?= no

KERNEL_CONFIGS = CONFIG_LLVM=$(CONFIG_LLVM) CONFIG_DEBUG=$(CONFIG_DEBUG) CONFIG_OPTIMIZATION=$(CONFIG_OPTIMIZATION) \
				 CONFIG_E9_ENABLE=$(CONFIG_E9_ENABLE)

KERNEL_CONFIGS_FILE = ./configs.mk
-include $(KERNEL_CONFIGS_FILE)

.PHONY: kernel tools iso clean


all: iso tools

kernel: $(KERNEL_CONFIGS_FILE)
	make -C kernel all $(KERNEL_CONFIGS)

$(KERNEL_CONFIGS_FILE):
	@echo "KERNEL_CONFIGS = $(KERNEL_CONFIGS)" > $(KERNEL_CONFIGS_FILE)

tools:
	make -C tools all

iso: kernel
	cp -v $(KERNEL_FILE) ./TESTING/iso_root
	xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin -no-emul-boot \
		-no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot boot/limine/limine-uefi-cd.bin \
		--efi-boot-part --efi-boot-image --protective-msdos-label ./TESTING/iso_root -o ./TESTING/crescent.iso
	./TESTING/limine bios-install ./TESTING/crescent.iso

clean:
	make -C kernel clean
	make -C tools clean
	rm -f $(KERNEL_CONFIGS_FILE)
