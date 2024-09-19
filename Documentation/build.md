Configurations
==============
- CONFIG_DEBUG: Enables debugging symbols and disables optimizations.
- CONFIG_OPTIMIZATION: Set the optimization level. Only valid when CONFIG_DEBUG is not enabled. Valid options: 0, 1, 2, 3, s, z, fast
- CONFIG_E9_ENABLE: Enables outputting printk output to port E9. Useful for debugging.
- CONFIG_LLVM: Use LLVM's tools over GNU's tools for compiling the kernel. Requires clang and lld to be installed on the system. (Note: this is not fully supported)

Building
========
Operating Systems
-----------------
Windows:
- Install WSL2 (https://learn.microsoft.com/en-us/windows/wsl/install)
- Continue

MacOS:
- I have no idea, good luck

Linux:
- Continue

Compilers
---------
GCC:
- First you must install a GCC cross compiler. There is a script for building the cross compiler at scripts/gcc.sh
- Now read the script. Specifically the end of the script, and change any changes you need to make
- Reccomendations:
    * Check your gcc version (gcc --version) and binutils version (ld --version)
    * Then set the compiler version to build to either the same or one version above the current version you have (or an even earlier version, if you want to do that for some reason)
- Now run the script, the script may take a while to complete.
- After the script finished, continue onto the next step.

Clang:
- Install clang and lld on your system, and then continue.

Build
-----
- First, go to project root, and then follow the next steps
- If you're using GCC run `make kernel`
- If you're using clang, `run make kernel CONFIG_LLVM=yes`

ISO
---
- If you want to build an ISO, read Documentation/running.md

Debugging
=========
- I usually start out with optimizations turned on with CONFIG_E9_ENABLE turned on. If there is a problem I cannot see, I then enable CONFIG_DEBUG and see if the same issue is there.
- This is to catch bugs relating to optimizations. If this happens, then the code was not written properly (eg. forgetting a volatile for MMIO space).
- If you want to use a debugger like GDB, see Documentation/gdb.md.
