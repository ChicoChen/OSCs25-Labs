file build/loader.elf
gdb-remote 1234
breakpoint set -f loader_entry.c -l 36
c
