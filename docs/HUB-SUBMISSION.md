# Firmware Hub submission preparation

Destination: https://thefirmwarehub.com/submit/
Submission contact: zven@thefirmwarehub.com (the site's published mail link).
The site takes a submission by email; there is no account/form submission here.

The site requires public open source, a package with update loader, an identity
not used by another firmware, card metadata and real-device transition tests.
Its FAQ allows the package served by an installer page instead of a GitHub
Release. Our Pages workflow already builds and attests that package and its
resources/recovery files with the required reviewed shared workflow.

## Prepared items

- Project/owner: FM-1 Ear Trainer / copych.
- Repository: https://github.com/copych/FM1-Ear-Trainer
- GPL-3.0-only, preserving Melodee/Felucca and other component notices.
- Root `fmhub.json`: description, author, category, emulator/editor/guide links,
  card color and native 240 × 240 screen capture at `docs/card.png`.
- Root `project.json`: version 0.0.8 and dedicated `FM-1_327008` identity.
  The Hub's checker reported this free on 2026-11-09. This is a check, not a
  reservation or acceptance. The old development `FM-1_900` is shared by many
  listed firmwares and must not be submitted as our unique identity.
- The target wrapper now uses `build.sh --project ../project.json`; this validates
  the fork namespace and compiles the same identity into the app and package.
  Upstream release numbering remains available unchanged for upstream use.
- The site version reads the same manifest and refuses a stale/mismatched target.

## Required user verification before a ready-for-listing claim

Test the dedicated-identity package on a real FM-1: install from official V15,
install from another firmware, confirm it reports `FM-1_327008`, play the game,
then switch away successfully. The user has reported earlier gameplay tests,
but these particular transition/identity results have not been confirmed.
Do not write "tested: yes" in a submission until those results are known.
Both physical flash revisions and quantified real-time headroom remain unverified.

## Compliance scope

Target/build/ABI: builder changes only product/version configuration; final build
result must be checked. Audio/numerics: no synthesis or ISR logic changed.
Ownership/synchronization: unchanged. Memory/storage/I/O: no layout or save-format
change, static target budgets checked by upstream. Dependencies/licenses: native
screen capture reused, no new dependency, attribution retained. Compatibility/
package/recovery: product name now distinct, loader/SDK/layout/checks preserved;
actual install/exit transitions UNVERIFIED. Host functional checks include manifest
validation, malformed identity rejection, native/WASM game tests and offline
package/recovery validation. Hardware real-time checks remain UNVERIFIED.

Local target build PASS: app 566640 B; package 668948 B, identity FM-1_327008.
Static .data+.bss 97336/98304 B, pool 134052/357888 B, audio arena 81088 B;
static audio ISR stack audit 2980 B. Existing manifest/section/stack/size gates
and offline recovery/loader checks PASS. Firmware SHA-256:
`1d6bfefbd4d1ceec70d96df4890380130c671accc78827ca709be461af8d3896`.
No device writes performed. Card/identity/installer-gate tests and local site
script/hash checks PASS. Hardware tests and submission email remain pending.
