GDB
===
- First you need to have to have the kernel's tools built (run `make tools` in project root to build them)
- Now you will need to open up a separate terminal, and cd into the scripts folder
- Then in that separate terminal, run `./run.py --debug`
- Now in your previous terminal, make sure you are in the project root directory. If not, you **will** have issues later on.
- Execute `gdb` in that terminal
- Now in gdb, execute this command: `source scripts/run.py`
- Then execute this command: `python load()`
- After that, you'll need to execute this command: `target remote localhost:1234`
- Then you'll want to set up any breakpoints you want in gdb, and when you are ready, execute this command:`continue`
- Select the boot option that says `no KASLR`, as not selecting it will result in addresses being randomized (which you don't want in this case)
