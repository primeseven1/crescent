#!/usr/bin/python

import os
import sys
import subprocess


def run_qemu(qemu_args: str):
    result = subprocess.run(f"qemu-system-x86_64 {qemu_args}", shell=True, 
                            stderr=subprocess.PIPE, text=True)
    
    if result.returncode != 0:
        print(f"Error when running qemu: {result.stderr}")


def do_setup(path: str):
    os.makedirs(f"{path}/boot/grub")

    with open(f"{path}/boot/grub/grub.cfg", "w") as grub_config:
        file_data = [ 
            "set default=0\nset timeout=0\n\n", 
            "menuentry \"Crescent\" {\n", 
            "    insmod all_video\n",
            "    multiboot2 /boot/crescent-kernel\n", 
            "    boot\n"
            "}\n"
        ]

        grub_config.write("".join(file_data))


def main():
    argv: list[str] = sys.argv
    crescent_iso: str = "./crescent.iso"

    if "-s" in argv or "--setup" in argv:
        do_setup("./iso")
        print("Setup done")
        exit(0)

    if not os.path.isfile(crescent_iso):
        print(f"Error: {crescent_iso} does not exist")
        sys.exit(1)

    qemu_args: str = f"-m 512M -cdrom {crescent_iso} -net none -serial stdio"

    if "-d" in argv or "--debug" in argv:
        qemu_args += " -s -S"
    if "-e" in argv or "--efi" in argv:
        qemu_args += " -bios /usr/share/edk2/x64/OVMF.fd"

    run_qemu(qemu_args)

if __name__ == "__main__":
    main()
    
