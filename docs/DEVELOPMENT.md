# Development

Player instructions: [README](../README.md) and [guide](https://copych.github.io/FM1-Ear-Trainer/webapp/manual/).

- [Toolchain, SDK and build commands](BUILDING.md)
- [GitHub Pages publication and provenance](GITHUB-PAGES.md)
- [Rating, unordered matching and persistence](RATING-UNORDERED-ANSWERS.md)
- [Highlighted-note handling and documentation update](HIGHLIGHTED-NOTES.md)
- [Metronome, rhythm games and validation](METRONOME.md)
- [Porting boundaries](PORTING.md)
- [Hardware testing and documentation status](HARDWARE-TESTING.md)

The vendored Melodee baseline is 165d177a425944f3d30b0b669255a6ed0050c3a5. Preserve the component notices and follow melodee/AGENTS.md for source attribution and attested release packaging. Public packages use the reviewed shared builder, blocking host/web tests, checksums, paired resources and recovery files; local builds are development evidence.

Build outputs, toolchains and device backups are excluded from Git. For a local site after building, use tools/start-web.ps1 or node tools/serve-site.cjs 8010. Technical reports in this directory retain development history; old workstation paths and experiments are historical.

Training loads the selected sound into track 1 and temporarily mutes other parts. Track 1 retains that sound on exit: preserve a project before saving over its previous sound. Arbitrary keys/scales, relative-pitch matching and detailed progress statistics remain possible extensions.


## Ear Trainer releases

The root `.github/workflows/release.yml` publishes this repository's releases;
the workflow under `melodee/.github` is retained upstream source and is not
active here. Keep `project.json`, `MELODEE_VERSION` and the recovery tool's
version aligned, add `docs/RELEASE_<version>.md`, then push `v<version>` on main.
The required `zvenson/fm1-build@v1` builder runs blocking host/web tests and
attests the firmware, app, resources and source archive. Four native recovery
jobs test/bundle their USB backend independently. Publication verifies the
attestations and complete matching/checksummed asset set before making the
draft public; existing public tags are never overwritten. The Ear Trainer
product identity stays `FM-1_327008` and does not use Melodee's version-derived
product IDs. Release notes must state remaining hardware-validation limits.
