# Xbox 360 YouTube Leanback feasibility experiment

See [prototype, build instructions and test procedure](experiments/leanback/README.md)
and [findings and limitations](experiments/leanback/REPORT.md).

The required end product is a standalone **`.xex`**, connecting directly to
YouTube without a companion server. This repository currently contains research,
host diagnostics, a tested input adapter and unbuilt XDK diagnostic source.
It does **not** yet contain a working YouTube application or an XEX binary.

A standalone **XeLL ELF diagnostic** is now built: [download](dist/youtube-leanback-xell.elf).
It displays status, exercises controller events and tests the native endpoint
parser. It does not connect to YouTube, render Leanback or play media.
See [ELF build and launch instructions](experiments/leanback/elf/README.md).
