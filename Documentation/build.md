Windows
=======
- Install WSL2

MacOS
=====
- I have no idea, good luck

Linux
=====
- Continue

Kernel
======
GCC
---
- You need the toolchain. In order to build the toolchain, you will need to use the build script in scripts/gcc.sh. This script will build the toolchain for you. If you want to modify the script in any way, make sure you make a copy of it first, make whatever changes you need, and then run that script
- Then you can build the project by running this command: make kernel

Clang
-----
- Clang is not guarunteed to work. But nevertheless, there is a clang configuration
- All you have to do is build the project with this command in project root: make kernel CONFIG_LLVM=yes.
- You will need the LLVM linker as well (ld.lld), which is not installed on some systems even if you install clang

ISO
===
- First, there is another script (scripts/run.py). You need to execute this script with the --setup command line argument (./scripts/run.py --setup), and then it will set up the enviroment for building and running the ISO
- Then follow the build instructions for the kernel
- Then you will need to run this command in project root: make iso
- After that, the ISO will be built in TESTING/crescent.iso

Tools
=====
- First, you need to have a C compiler installed
- Then run this command in project root: make tools

Configurations
==============
- CONFIG_LLVM Uses clang instead of gcc
- CONFIG_DEBUG Enables debug mode, disables all optimizations and enables debugging symbols
- CONFIG_OPTIMIZATION Only applies if CONFIG_DEBUG is off. Affects the compiler optimization option (valid options: 0, 1, 2, 3, fast)
- CONFIG_E9_ENABLE Enables port E9 debugging (only enable for hypervisors that support it)

Notes
=====
- You can build everything by entering this command: make all
