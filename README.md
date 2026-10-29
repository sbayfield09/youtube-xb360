# Xbox 360 YouTube Leanback feasibility experiment

See [prototype, build instructions and test procedure](experiments/leanback/README.md)
and [findings and limitations](experiments/leanback/REPORT.md).

The required end product is a standalone **`.xex`**, connecting directly to
YouTube without a companion server. This repository currently contains research,
host diagnostics, a tested input adapter and unbuilt XDK diagnostic source.
It does **not** yet contain a working YouTube application or an XEX binary.

A standalone **XeLL HTTPS diagnostic 0.2** is now built: [download](dist/youtube-leanback-xell.elf).
It tests DHCP, DNS and a certificate-verified direct HTTPS request to YouTube.
It does not render Leanback or play media. A private USB seed needs one-time
provisioning; see [setup and test instructions](experiments/leanback/net/README.md).
The user confirmed v0.1 boot/display and controller behavior; v0.2's network
and storage paths still need physical console testing.
See [ELF build and launch instructions](experiments/leanback/elf/README.md).
