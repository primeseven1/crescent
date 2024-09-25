#!/usr/bin/env python

import argparse
import subprocess
import shutil
import sys
import os

def do_testing_setup(path: str):
    os.makedirs(f"{path}/iso_root/boot/limine", exist_ok=True)
    os.makedirs(f"{path}/iso_root/EFI/BOOT", exist_ok=True)
    shutil.copy(f"{path}/../limine.conf", f"{path}/iso_root/boot/limine/limine.conf")

    url: str = "https://github.com/limine-bootloader/limine.git"
    branch: str = "v8.x-binary"
    result = subprocess.run(f"git clone {url} --branch={branch} --depth=1",
                            shell=True, stderr=subprocess.PIPE, text=True)
    if result.returncode:
        print(f"Setup failed (failed to clone limine): {result.stderr}", file=sys.stderr)

    result = subprocess.run("make -C limine", shell=True,
                            stderr=subprocess.PIPE, text=True)
    if result.returncode:
        print(f"Setup failed (failed to build limine): {result.stderr}", file=sys.stderr)

    for file in ["BOOTX64.EFI", "BOOTIA32.EFI"]:
        shutil.copy(f"./limine/{file}", f"{path}/iso_root/EFI/BOOT/{file}")
    for file in ["limine-bios.sys", "limine-bios-cd.bin", "limine-uefi-cd.bin"]:
        shutil.copy(f"./limine/{file}", f"{path}/iso_root/boot/limine/{file}")
    shutil.copy("./limine/limine", f"{path}/limine")
    shutil.copy("./limine/LICENSE", f"{path}/LICENSE")

    shutil.rmtree("./limine")

def parse_cmdline_args_x86_64(iso: str) -> str:
    qemu_args: str = f"-cdrom {iso} -net none -debugcon stdio"

    parser = argparse.ArgumentParser()
    parser.add_argument("--cpus", "-c", type=int, default=2,
                        help="The number of logical cores the VM should have")
    parser.add_argument("--debug", "-d", action="store_true",
                        help="Add flags that allow attaching a debugger")
    parser.add_argument("--setup", "-s", action="store_true",
                        help="Set up the testing enviroment")
    parser.add_argument("--efi", "-e", action="store_true",
                        help="Start qemu with OVMF instead of seabios")
    parser.add_argument("--ovmf-path", "-o", type=str,
                        default="/usr/share/edk2/x64",
                        help="Folder with the OVMF files")
    parser.add_argument("--memory", "-m", type=str, default="128M",
                        help="The amount of memory the VM should have")

    args = parser.parse_args()
    if args.setup:
        script_directory: str = os.path.dirname(os.path.abspath(__file__))
        do_testing_setup(f"{script_directory}/../TESTING")
        print("Setup done")
        sys.exit(0)

    if args.debug:
        qemu_args += " -S -s"
    if args.efi:
        ovmf_code = os.path.join(args.ovmf_path, "OVMF_CODE.fd")
        ovmf_vars = os.path.join(args.ovmf_path, "OVMF_VARS.fd")
        qemu_args += f" -drive if=pflash,unit=0,file={ovmf_code},readonly=on"
        qemu_args += f" -drive if=pflash,unit=1,file={ovmf_vars},readonly=on"

    qemu_args += f" -m {args.memory}"
    qemu_args += f" -smp cpus={args.cpus}"

    return qemu_args

def run_x86_64(qemu_args: str) -> int:
    ret = subprocess.run(f"qemu-system-x86_64 {qemu_args}", shell=True,
                         stderr=subprocess.PIPE, text=True)
    if ret.returncode:
        print(f"Error when running qemu: {ret.stderr}", file=sys.stderr)

    return ret.returncode

def main() -> int:
    script_directory: str = os.path.dirname(os.path.abspath(__file__))
    iso: str = f"{script_directory}/../TESTING/crescent.iso"
    qemu_args: str = parse_cmdline_args_x86_64(iso)
    return run_x86_64(qemu_args)

if __name__ == "__main__":
    sys.exit(main())
