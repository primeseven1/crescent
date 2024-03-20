Folders
=======
boot: Early boot code, should only contain assembly code, since we don't want gcc to make a call to something mapped to the higher half
include/crescent/asm: Guarunteed to be able to be included by assembly code
include/crescent/cpu: C wrappers for common assembly instructions
init: initalization code
kernel: General kernel stuff
lib: Common libc functions, or other stuff that may not be apart of the kernel nessecarily, but is used nevertheless
mm: Memory management
