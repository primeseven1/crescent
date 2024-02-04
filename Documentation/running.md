Running
=======
- In order to run the OS in a VM, first follow the instructions in build.md
- Then in the scripts folder, run "./run.py --setup" to get the directory paths set up (Make sure you run this when you are inside the scripts folder!)
- In project root, run "make iso" to build the ISO
- And then in the scripts folder, run ./run.py without --setup

Debugging
=========
- After following all the instructions under Running, you can add --debug or -d so that you can attach GDB to qemu

