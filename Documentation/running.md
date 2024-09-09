Running
=======
ISO
---
- There is a script in scripts/run.py.
- cd into the scripts folder, and run `./run.py --setup`
- Now you will need to install a tool called xorriso
- After you've done that, go into project root and run `make iso`
- You will now have an ISO called crescent.iso in the TESTING folder

Virtual Machine
---------------
- Install qemu
- Now cd into the scripts folder, and run `./run.py`
- This will pull up qemu, and you will have 2 options when booting. crescent, and crescent (no KASLR). It does not matter which one you pick

Real Hardware (Not recommended)
-------------------------------
- After building the ISO, you can either burn it to an actual CD, or copy it to a USB drive.
- You can use any tool you want to copy it to a usb drive (eg. dd, balena etcher).
- Now you will have a bootable usb drive you can use to boot on real hardware.
