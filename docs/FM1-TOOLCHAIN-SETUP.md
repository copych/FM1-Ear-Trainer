# FM-1 X0X toolchain setup

Prepared 29 October 2026 for upstream revision 54194c54bb01e9d9b3fe9d641b3ddfa512ac04a3.

## Completed

- Ran `wsl.exe --install -d Ubuntu` successfully. Windows reported that a restart is required.
- Downloaded the Linux compiler archive from https://pkgman.jieliapp.com/s/linux-toolchain.
- Archive SHA-256: `a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd`.
  This records the downloaded bytes; no independently published vendor checksum was available in the repository.
- Checked out AC79 SDK tag `AC79NN_SDK_V1.2.1_2023-12-13` at commit
  `d179b4484759423312073f5fbb232501aa491047` in `.toolchains/AC79_SDK`.
- All three packaging inputs match the SHA-256 values in upstream/tools/build.py:
  uboot.boot, cfg_tool.bin and cfg/eq_cfg_hw.bin.

## After restarting Windows

Open Ubuntu and complete its first-run Linux username/password setup if prompted.
Then run in Ubuntu:

```sh
cd /mnt/c/Users/admin/Documents/ChatGPT/FM1-X0X-mod
sh tools/setup-fm1-wsl.sh
sh tools/build-fm1-wsl.sh
```

The setup script installs Ubuntu build dependencies and extracts the compiler into
the Linux home directory. It verifies that the archive still matches the recorded hash.
The build wrapper points to the local SDK and retains upstream integrity/size checks.
No global Windows PATH or Git TLS settings were changed.

## Still pending

Ubuntu first launch, Linux dependencies, compiler extraction/version verification,
target build and host tests. No firmware has been flashed. Installation is staged,
not yet verified as a working development environment.

## Follow-up after restart

WSL service is operational. Ubuntu initially failed to register under WSL2 with
`HCS_E_HYPERV_NOT_INSTALLED`; Windows reports firmware virtualization disabled.
Set the default WSL version to 1 and successfully installed Ubuntu with
`wsl --install -d Ubuntu --no-launch`. Ubuntu can execute x86-64 shell commands.
Dependency installation and compiler verification are being completed as Linux root;
the compiler will therefore reside under `/root/.jieli/toolchain`.

## Verified result

- Installed build-essential, Python 3/Pillow, xz, curl, Git and certificates.
- Extracted compiler using Python (modern tar failed on WSL1 filesystem calls).
- JieLi compiler reports clang 4.0.1; archive directory is `jieli-linux-toolchains-20260730.1`.
- Converted checked-out text files from Windows CRLF to Linux LF; source contents have no semantic changes.
- Successfully ran `sh tools/build-fm1-wsl.sh` as Ubuntu root.
- Package: `upstream/build/x0x.fwsc`, 609684 bytes, identity FM-1_900.
- Package SHA-256: `1fe42c77be438315dbf0af9c50c9c1bd5316434cbec415ef32495f26565a041a`.
- Upstream checks passed: no software-double helpers, RAM code has no calls,
  image bounds, RAM/pool bounds and register access boundaries.
- Static RAM data+bss: 97704 / 98304 bytes; pool: 335808 / 344064 bytes.
  These are linker/build budgets, not measured runtime stack or real-time headroom.
- Host sequencer and motion tests passed. Optional TB-3PO upstream reference comparison
  was not compiled; full regression suite was not run for toolchain setup.
- No hardware test or flashing performed.

From PowerShell, rebuild with:

```powershell
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-X0X-mod/tools/build-fm1-wsl.sh
```

This verified result supersedes the initial pending/restart instructions above.
