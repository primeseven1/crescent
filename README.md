About Crescent
==============
Crescent is a 32 bit kernel written from scratch that uses the Multiboot 2 protocol to boot.

You may notice that the directory structure is very similar to the Linux kernel directory structure. I like the way they organize their files, and it's clearly worked for the Linux kernel, so i'm gonna use it too as a starting point. However, as the kernel gets more and more complex I'm sure the directory structure is gonna change quite a bit.

At the moment, I will not be accepting other contributions as I don't think the kernel is at a place to be accepting them yet. At the moment there is not even a system to print text on the screen, there is no panic function, the kernel is very very simple right now, and it doesn't really do anything (Even a simple memory error can cause a triple fault). This will be removed when I am accepting contributions.

At some point, I do plan on supporting the 64 bit architechure. It probably won't be for a while, but I do plan on having that support at some point.

Building & Running
==================
Read build.md and running.md in the Documentation folder.

compile_flags.txt
=================
This is a list of the compile flags that the clangd LSP server to understand this project a bit better which helps with code completion, errors, and more. You can also use a compile_commands.json, but I don't want to do that as a compile_flags.txt is a lot simpler

Issues
======
If you have any issues or need any help, go ahead and create an issue and I will get to it once I can.
