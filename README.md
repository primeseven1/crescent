About
=====
Crescent is a 64 bit hobby operating system.

The bootloader that's used is limine, and it uses the limine specification to boot. I chose this bootloader and protocol since it moves some responsibility of the kernel to the bootloader (eg. Setting up SMP, switching the CPU's into 64 bit mode, and setting up initial page tables).

At the moment, this project is in it's very early stages. At the moment this is able to run on real hardware, but that's not recommended at the moment. If you want to run this project (either in an emulator or on real hardware), read Documentation/build.md and Documentation/running.md to get started. 

If you don't want to build a whole gcc toolchain for this project, you can use LLVM to build it.

Though this project doesn't support multithreading quite yet, I am pretending that it exists to avoid a bunch of rewriting later.
