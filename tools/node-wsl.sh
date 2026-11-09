#!/bin/sh
# Prefer normal Node (including GitHub's toolcache). Only WSL1 needs the loader.
if command -v node >/dev/null 2>&1 && node --version >/dev/null 2>&1; then
    exec node "$@"
fi
exec /lib64/ld-linux-x86-64.so.2 /usr/bin/node "$@"
