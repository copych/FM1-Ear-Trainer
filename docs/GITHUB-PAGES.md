# GitHub Pages publishing

Project site: https://copych.github.io/FM1-Ear-Trainer/

- Emulator: `emu/`
- Web installer: `webapp/installer/`
- Guide: `webapp/manual/`
- Backup/editor: `webapp/editor/`
- Exact package/resource/WASM hashes, source commit and workflow run: `build-info.json`

`.github/workflows/pages.yml` runs on main pushes or manual dispatch. It calls
the mandated `zvenson/fm1-build/.github/workflows/build.yml@v1` to build target
firmware and Emscripten 3.1.69 emulator from that commit. Native game, upstream UI,
MIDI/flash, WASM and package/recovery checks block publication. The site is tested
under `/FM1-Ear-Trainer/` locally so installer URLs retain the project prefix.

The shared workflow attests the package, paired resources, source recovery ZIP,
and complete site archive. Deployment checks their SHA-256 and attestations,
requires the reviewed signer workflow and current source digest, then checks that
the extracted site's firmware/resources match the attested files byte for byte.
Only then does the official Pages action publish the site. Actions are pinned to
commit SHAs; the reviewed shared build remains at v1. No tag/GitHub Release is
created by this workflow.

The published recovery ZIP includes matching development images, original RAM
loader, recovery Python sources and licenses; it is not a backup of your device
or a standalone executable. Establish and verify your own recovery route first.
Web MIDI installation requires a supported Chrome/Edge browser, USB MIDI/SysEx
permission and a physical connection. Page viewing does not write a device.

Packages use the dedicated identity from `project.json` (currently FM-1_327008).
Earlier development packages used FM-1_900. Attestation proves build/source
provenance, not hardware safety. Both physical revisions, worst-case real-time
headroom and recovery remain UNVERIFIED. Existing native/WASM validation still
does not replace device tests. No new firmware/DSP code is introduced by Pages.

## Scope and validation

Build/ABI, audio timing and memory semantics are unchanged; the CI target build
retains upstream checks. Ownership/synchronization unchanged (no runtime code
changes). Dependencies reuse official toolchain/SDK, Emscripten and GitHub Actions;
licenses/attribution retained. Package/recovery integrity and host/web gates run
before deployment. Hardware validation remains UNVERIFIED. Full upstream sound
goldens, analog output, physical flashing/recovery and hardware soak are outside
this hosting change. Actual workflow/deployment results are recorded in GitHub
Actions and the published manifest.
