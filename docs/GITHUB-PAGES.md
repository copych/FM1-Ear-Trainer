# GitHub Pages publishing

Hardware functional testing is performed by the project owner. This technical record describes the checks and measurements available when written; see [current testing status](HARDWARE-TESTING.md).

Project site: https://copych.github.io/FM1-Ear-Trainer/

- Emulator: `emu/`
- Web installer: `webapp/installer/`
- Guide: `webapp/manual/`
- Backup/editor: `webapp/editor/`
- Exact package/resource/WASM hashes, source commit and workflow run: `build-info.json`

`.github/workflows/pages.yml` runs on main and `feature-metronome-training-games`
pushes or manual dispatch. It calls
the mandated `zvenson/fm1-build/.github/workflows/build.yml@v1` to build target
firmware and Emscripten 3.1.69 emulator from that commit. Native game, upstream UI,
MIDI/flash, WASM and package/recovery checks block publication. The site is tested
under `/FM1-Ear-Trainer/` locally so installer URLs retain the project prefix.

The shared workflow attests the package, paired resources, source recovery ZIP,
and complete site archive. Deployment checks their SHA-256 and attestations,
requires the reviewed signer workflow and current source digest, then checks that
the extracted site's firmware/resources match the attested files byte for byte.
After verification it uploads a `verified-site` artifact. Only main runs deploy
that artifact to the project Pages site; feature builds never replace main.
Actions are pinned to
commit SHAs; the reviewed shared build remains at v1. No tag/GitHub Release is
created by this workflow.

## Metronome branch installer

Branch: `feature-metronome-training-games` (the earlier slash-separated name was
renamed). Local entry: `http://127.0.0.1:8012/webapp/installer/` after running
`sh tools/build-melodee.sh site` and serving `build/site`. The installer banner
and `build-info.json` identify the branch and source revision. The local
development package remains distinct from an attested hosted build.

The separate `copych/copych.github.io` repository hosts the checked branch site
under `feature-metronome-training-games/`; the installer path is
`https://copych.github.io/feature-metronome-training-games/webapp/installer/`.
The source of that directory is the `verified-site` artifact from a successful
feature-branch build and attestation verification, including paired resources,
emulator, backup editor, guide, licenses and recovery ZIP. Its source revision
and package hashes must match the verified run before publishing.

Initial hosted metronome site: source `1982e644a4330aed660e003eef2ff44106b2b3f7`,
verified [build run](https://github.com/copych/FM1-Ear-Trainer/actions/runs/43756617716),
hosting commit `32e5a7bee3ab8a6b1b10b8c05df207f8b86eae71`, and successful
[Pages run](https://github.com/copych/copych.github.io/actions/runs/43757564706).
The public installer, emulator, guide, editor, package and resource URLs were
retrieved and their manifest hashes verified after deployment. The local
installer at port 8012 was checked separately. Hosting guards were also tested
to reject damaged resources and unverified local builds. This records the
installed source snapshot; later README-only commits do not change its binary.

The user-site "Deploy all branches" workflow only exports branches of
`copych.github.io` itself. It does not fetch or build branches of FM1-Ear-Trainer.
Publishing raw firmware source therefore cannot create an installer: generated
HTML, WASM, firmware and resources must be assembled and verified first.

Hosting-only compliance: runtime DSP/ownership/ABI/storage semantics are
unchanged. The existing SDK/toolchain, host tests, package/recovery and signer
verification gates remain mandatory. Branch metadata and served package hashes
are checked by `tools/check-site.cjs`; local and hosted verification must be
reported separately. Physical flash/recovery and hardware timing were not
performed as part of this installer/hosting work.

The published recovery ZIP includes matching development images, original RAM
loader, recovery Python sources and licenses; it is not a backup of your device
or a standalone executable. Establish and verify your own recovery route first.
Web MIDI installation requires a supported Chrome/Edge browser, USB MIDI/SysEx
permission and a physical connection. Page viewing does not write a device.

Packages use the dedicated identity from `project.json` (currently FM-1_327008).
Earlier development packages used FM-1_900. Attestation proves build/source
provenance. The project owner performs functional hardware testing; specific quantitative measurements are recorded in [hardware testing status](HARDWARE-TESTING.md). No new firmware/DSP code is introduced by Pages.

## Scope and validation

Build/ABI, audio timing and memory semantics are unchanged; the CI target build
retains upstream checks. Ownership/synchronization unchanged (no runtime code
changes). Dependencies reuse official toolchain/SDK, Emscripten and GitHub Actions;
licenses/attribution retained. Package/recovery integrity and host/web gates run
before deployment. Hardware validation is verified by hand. Full upstream sound
goldens, analog output, physical flashing/recovery and hardware soak are outside
this hosting change. Actual workflow/deployment results are recorded in GitHub
Actions and the published manifest.
