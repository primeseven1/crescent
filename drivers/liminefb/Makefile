ifeq ($(CONFIG_DRIVER_LIMINEFB), y)

# Since flanterm uses standard headers, we want to disable the standard includes create our own headers to avoid conflicts
LIMINEFB_CFLAGS := -nostdinc -I./drivers/liminefb/include
FLANTERM_CFLAGS := -DFLANTERM_FB_DISABLE_BUMP_ALLOC \
		   -DFLANTERM_FB_WIDTH_LIMIT=0xffffffff \
		   -DFLANTERM_FB_HEIGHT_LIMIT=0xffffffff \
		   -Wno-implicit-fallthrough -Wno-missing-prototypes
C_SOURCE_FILES += $(shell find ./drivers/liminefb/flanterm/src -type f -name '*.c')
C_SOURCE_FILES += ./drivers/liminefb/main.c ./drivers/liminefb/term.c

./drivers/liminefb/%.o: ./drivers/liminefb/%.c
	$(CC) $(CFLAGS) $(LIMINEFB_CFLAGS) $< -o $@

./drivers/liminefb/flanterm/src/%.o: ./drivers/liminefb/flanterm/src/%.c
	$(CC) $(CFLAGS) $(LIMINEFB_CFLAGS) $(FLANTERM_CFLAGS) $< -o $@

endif
