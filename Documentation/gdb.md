GDB
===
- First you need to have to have the tools installed. You can do this by running this command: make tools
- Then you need to run the scripts/run.py script with the --debug option.
- Run the gdb command within project root. Not doing so will cause problems.
- Now you need to run these commands in GDB:
* source scripts/run.py
* python load()
* target remote localhost:1234
- After running those commands, you can set breakpoints, or do whatever you need to do, and then run this gdb command: continue
