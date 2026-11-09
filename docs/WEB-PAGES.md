# Local emulator and installer

Run `tools/start-web.ps1` from PowerShell. It starts a hidden Node server,
bound only to 127.0.0.1 on port 8010 (`-Port` chooses another port).

- Home: http://127.0.0.1:8010/
- Emulator: http://127.0.0.1:8010/emu/
- Installer: http://127.0.0.1:8010/webapp/installer/
- Installer simulation: http://127.0.0.1:8010/webapp/installer/?demo
- Backup/editor: http://127.0.0.1:8010/webapp/editor/

In the emulator, press **Switch on**, then **Start training**. Dedicated Retry,
Next and Exit buttons call guarded game actions using the same game handlers as
hardware. They do not synthesize HOME/PLAY gestures. Start becomes Restart while
active, and Retry/Next/Exit are disabled outside training. Mouse,
touch, computer-keyboard notes and the existing MIDI keyboard selector remain.
No firmware or preset changes are made by these web controls.

## Build

The built site is in `build/site/`. Rebuild after changing the firmware/emulator:

```powershell
wsl -d Ubuntu -u root --exec sh /mnt/c/Users/admin/Documents/ChatGPT/FM1-Ear-Trainer/tools/build-melodee.sh site
node tools/check-site.cjs
```

`site` needs the target and browser builds first. It uses Melodee's original
`web/make_site.py` and updater modules, retaining package/bootloader checks,
backup editor, preflight, update transfer and resource pairing. It supplies an
Ear Trainer guide instead of generating the unrelated full instrument manual.
Original license/attribution files remain present. The bundled bootloader's
LICENSE was restored to original LF line endings so its pinned SHA-256 matches;
the check itself was not changed or bypassed.

The local site offers the development prototype as 0.0.8; this is not a stable
or attested release. Firmware still reports the inherited Melodee development
identity FM-1_900. That identity also appears in Melodee development builds, so
it alone cannot prove a connected unit already runs Ear Trainer. Select the
reinstall path when replacing another development build. Assign a distinct
firmware identity before a public release.

The installer fetches **local** `firmware/ear-trainer-dev.fwsc` and its paired
`ear-trainer-dev-res.bin`, not the original Melodee release. `build-info.json`
records identity and SHA-256 for the package, resources and browser binary.
The package hash matches the target build documented in PROTOTYPE.md.

## Verified on 2026-11-08

- Local HTTP pages and assets return successfully, WASM has the proper MIME type,
  and the generated scripts parse.
- Served package/resource/WASM hashes match the manifest; the installer's
  embedded metadata points to the local package and its correct SHA-256.
- Browser UI: Switch on and Start training reach the actual Ear Trainer screen.
- Installer demo: simulated V16 unit detected, preflight shown, fake installation
  and fonts/factory resource stage completed. No real USB device was accessed.
- Preview screenshot: `build/emulator-page.png`.

No public hosting/deployment was created. Actual installation/recovery, SysEx
permissions in the user's Chrome/Edge, hardware timing and both physical FM-1
revisions remain unverified. Opening pages does not write a device. The real
installer writes only when the user connects a device and confirms installation.
