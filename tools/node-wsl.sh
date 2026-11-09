#!/bin/sh
# WSL1 cannot directly load Ubuntu's Node binary; use its ELF interpreter.
exec /lib64/ld-linux-x86-64.so.2 /usr/bin/node "$@"
