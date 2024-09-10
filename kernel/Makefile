ASMFLAGS = -c -MMD -MP -D_ASSEMBLY_ \
		   -I../include
CFLAGS = -c -MMD -MP -std=c11 -I../include \
		 -ffreestanding -fno-stack-protector -fwrapv \
		 -fno-omit-frame-pointer -fno-lto -fPIE \
		 -Wall -Wextra -Wshadow -Wpointer-arith \
		 -Wstrict-prototypes -Wmissing-prototypes \
		 -Wvla -Walloca -Winline -Waggregate-return \
		 -mno-red-zone
LDFLAGS = -static -nostdlib --no-dynamic-linker \
		  -pie -ztext -zmax-page-size=0x1000

ifeq ($(CONFIG_LLVM), yes)
CC := clang
LD := ld.lld
CFLAGS += -target x86_64-pc-none-elf
else
CC := x86_64-crescent-gcc
LD := x86_64-crescent-ld
LIBGCC := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)
LIBGCC_DIR := $(shell dirname $(LIBGCC))
endif

ifeq ($(CONFIG_DEBUG), yes)
ASMFLAGS += -g -DCONFIG_DEBUG
CFLAGS += -g -O0 -DCONFIG_DEBUG
else
CFLAGS += -O$(CONFIG_OPTIMIZATION)
LDFLAGS += -O$(CONFIG_OPTIMIZATION)
endif

ifeq ($(CONFIG_E9_ENABLE), yes)
ASMFLAGS += -DCONFIG_E9_ENABLE
CFLAGS += -DCONFIG_E9_ENABLE
endif

S_SOURCE_FILES := $(shell find ./ -type f -name '*.S')
S_OBJECT_FILES := $(patsubst %.S, %.o, $(S_SOURCE_FILES))
C_SOURCE_FILES := $(shell find ./ -type f -name '*.c')
C_OBJECT_FILES := $(patsubst %.c, %.o, $(C_SOURCE_FILES))
H_DEPENDENCIES := $(patsubst %.o, %.d, $(S_OBJECT_FILES) $(C_OBJECT_FILES))

LDSCRIPT := ./core/linker.ld
OUTPUT := ../crescent-kernel

.PHONY: all clean

all: $(OUTPUT)

-include $(H_DEPENDENCIES)

ifeq ($(CONFIG_LLVM), no)
$(OUTPUT): $(S_OBJECT_FILES) $(C_OBJECT_FILES) $(LDSCRIPT) $(LIBGCC)
	$(LD) $(LDFLAGS) $(S_OBJECT_FILES) $(C_OBJECT_FILES) -T$(LDSCRIPT) -L$(LIBGCC_DIR) -lgcc -o $(OUTPUT)
else
$(OUTPUT): $(S_OBJECT_FILES) $(C_OBJECT_FILES) $(LDSCRIPT)
	$(LD) $(LDFLAGS) $(S_OBJECT_FILES) $(C_OBJECT_FILES) -T$(LDSCRIPT) -o $(OUTPUT)
endif

%.o: %.S
	$(CC) $(ASMFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(S_OBJECT_FILES) $(C_OBJECT_FILES) $(H_DEPENDENCIES) $(OUTPUT)
