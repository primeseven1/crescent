import gdb
import subprocess

def load():
    output: str = subprocess.run("./tools/sections/sections ./crescent-kernel",
                                 shell=True, text=True, capture_output=True)
    if output.returncode:
        gdb.write(f"Failed to load! stderr: {output.stderr.strip()}")
        return

    relocations: str = output.stdout.strip()
    gdb.execute(relocations)
