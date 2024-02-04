CONFIG_DEBUG ?= yes

CC := i686-elf-gcc
LD := i686-elf-ld

ASMFLAGS = -c -MMD -MP \
		   -D__ASSEMBLY_FILE__ \
		   -I./include
CFLAGS = -c -MMD -MP \
		 -ffreestanding -fno-stack-protector \
		 -Wall -Wextra -Wshadow -Wstrict-prototypes -Wpointer-arith \
		 -I./include -std=gnu17
LDFLAGS = -static -nostdlib

ifeq ($(CONFIG_DEBUG), yes)
	ASMFLAGS += -g -DCONFIG_DEBUG
	CFLAGS += -g -O0 -DCONFIG_DEBUG
	LDFLAGS += -O0
else
	CFLAGS += -O2
	LDFLAGS += -O2
endif

S_SOURCE_FILES := $(shell find ./ -type f -name '*.S')
S_OBJECT_FILES := $(patsubst %.S, %.o, $(S_SOURCE_FILES))
C_SOURCE_FILES := $(shell find ./ -type f -name '*.c')
C_OBJECT_FILES := $(patsubst %.c, %.o, $(C_SOURCE_FILES))
H_DEPENDENCIES := $(patsubst %.o, %.d, $(S_OBJECT_FILES) $(C_OBJECT_FILES))

LDSCRIPT := ./kernel/linker.ld
LIBGCC := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
LIBGCC_DIR := $(shell dirname $(LIBGCC))
OUTPUT := ./crescent-kernel

-include $(H_DEPENDENCIES)

.PHONY: all iso clean

all: $(OUTPUT)

iso: $(OUTPUT)
	cp ./crescent-kernel ./scripts/iso/boot
	grub-mkrescue /usr/lib/grub/i386-pc -o ./scripts/crescent.iso ./scripts/iso

$(OUTPUT): $(S_OBJECT_FILES) $(C_OBJECT_FILES) $(LDSCRIPT) $(LIBGCC)
	$(LD) $(LDFLAGS) $(S_OBJECT_FILES) $(C_OBJECT_FILES) -T$(LDSCRIPT) -L$(LIBGCC_DIR) -lgcc -o $(OUTPUT)

%.o: %.S
	$(CC) $(ASMFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(S_OBJECT_FILES) $(C_OBJECT_FILES) $(H_DEPENDENCIES) $(OUTPUT)
