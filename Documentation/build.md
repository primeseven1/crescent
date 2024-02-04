Building Crescent
=================
- Dependencies: i686-elf-gcc toolchain (there is a script for it in the scripts folder), grub-mkrescue (only needed for ISO's)

- First, ./run toolchain.sh in the scripts folder, this will build the gcc toolchain and place it in ~/.toolchains/gcc, this also adds it to your system PATH in your .bashrc
- A backup of your bashrc is stored in ~/.bashrc.backup
- When you run the script, you can add make flags like -j5 to make the build faster (replace 5 with how many cores you have if you decide to use that flag)

- Then run "make all" in project root to build the project
