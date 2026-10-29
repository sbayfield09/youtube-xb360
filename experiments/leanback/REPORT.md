# Feasibility report — 2026-10-29

**Later ELF build:** at the user's request, a separate XeLL diagnostic was
cross-compiled with checksum-verified Xenon GCC 16.2.0. It reuses the unchanged
XenonFin text presenter and endpoint parser. ELF32/big-endian/PowerPC format,
successful static linking and absence of unresolved symbols were verified.
No physical console test was performed. It contains no YouTube browser or media
player. See [ELF build/launch instructions](elf/README.md). The earlier missing
toolchain observations below describe the initial XEX investigation; the new
toolchain builds ELF, not XEX.

## Result and scope

The requested standalone Xbox 360 YouTube Leanback `.xex` is **not implemented or
shown feasible**. No verified, ready-to-integrate browser engine for that target
was identified. This is not proof that an extensive new port is impossible.
The experiment stops at concrete prerequisites rather than substituting a custom
UI, companion server, downloaded HTML screen, ELF, or desktop application.

The current source and diagnostic results are useful partial deliverables, not a
completed console prototype. Required missing capabilities are a working XEX/title
runtime build path, native verified HTTPS and DNS, a compatible browser, and a
YouTube-compatible native media pipeline. No console or XDK is attached here.
Prior chats are not accessible in this session; current source and its checked-in
hardware notes are the basis of the assessment.

## Original Xbox 360 application: evidence versus inference

Primary historical source: Google's [youtube/h5vcc](https://github.com/youtube/h5vcc)
at `7da685fe633dac18fd8db35624f7bade8619c977`. Its published description is a
minimal WebKit/Chromium build to render HTML5 video containers for game consoles.

Verified by inspecting this source:

- [lb_shell_main.cc](https://github.com/youtube/h5vcc/blob/7da685fe633dac18fd8db35624f7bade8619c977/lbshell/src/lb_shell_main.cc)
  has an explicit `__LB_XB360__` branch selecting
  `lb_shell/lb_shell_webkit_init_xb360.h` and `LBShellWebKitInitXB360`.
  It initializes WebKit and has a conditional JavaScriptCore initialization path.
  That conditional alone does not identify the exact JS engine in a shipped SKU.
- [lb_shell_webkit_init.cc](https://github.com/youtube/h5vcc/blob/7da685fe633dac18fd8db35624f7bade8619c977/lbshell/src/lb_shell_webkit_init.cc)
  explicitly enables `MediaSource` and encrypted media, and constructs a
  `Chrome/... LeanbackShell/...` product identity. This was an embedded web/media
  shell, not merely an HTML downloader or the dashboard Internet Explorer app.
- [lb_graphics.h](https://github.com/youtube/h5vcc/blob/7da685fe633dac18fd8db35624f7bade8619c977/lbshell/src/lb_graphics.h)
  provides a platform player hook for extracting video textures. Native playback
  integration is a deliberate platform boundary in the architecture.
- [lbshell README](https://github.com/youtube/h5vcc/blob/7da685fe633dac18fd8db35624f7bade8619c977/lbshell/README.txt)
  documents Linux GYP/Ninja builds. The inspected `src/platform` tree exposes
  Linux, not a complete public Xbox 360 backend. An attempted raw fetch under
  `src/platform/xb360/lb_shell_webkit_init_xb360.h` returned 404; that guessed path
  alone does not prove absence from every archive or private distribution.

Supported inference: the historical Leanback shell had an Xbox 360 platform port
and is a strong architectural lead for the original app family. Not verified:
the exact source revision, engine version, rendering backend, codec accelerator,
authentication, service contract or memory budget of any particular retail XEX.
No retail package was downloaded, extracted, disassembled or executed. There is
no measured evidence here that replacing an old app's URL or UA would work today.

## Reusing original browser components

Public h5vcc shell source is a legitimate design/reference starting point;
[lbshell/COPYING](https://github.com/youtube/h5vcc/blob/7da685fe633dac18fd8db35624f7bade8619c977/lbshell/COPYING)
is Apache-2.0. That does not license every WebKit/Chromium dependency under the
same terms, nor grant rights to proprietary Xbox SDK code, retail application
assets, platform adapters, credentials or service certificates. Each component
needs its own license and dependency review before redistribution.

No separable, documented Xbox browser DLL/API with permission to redistribute it
was identified. A proprietary application binary would not automatically become
an embeddable engine: title imports, initialization, graphics/audio ownership,
network services, browser ABI and JS/media bindings would have to match. This is
an engineering assessment, not a demonstrated binary-reuse experiment or legal
opinion. No proprietary binary, certificate extraction or DRM bypass is proposed.
Even a recovered old engine would still need modern-web compatibility testing.

## VacuumTube findings

Inspected [shy1132/VacuumTube](https://github.com/shy1132/VacuumTube) at
`21ed770379deefc123c1026774b7d8fc1bc0d84b` (package 2.0.1, MIT):

- `package.json` uses Electron `^44.5.1`; `src/index.js` creates a BrowserWindow
  with a preload and loads `https://www.youtube.com/tv`. Chromium performs DOM,
  CSS, JS execution, resource loading and browser media behavior.
- `src/ua.js` presents a Cobalt 27/Starboard identity, modifies HTTP headers and
  uses DevTools user-agent/client-hint overrides. A UA string is not an engine
  compatibility layer. The host diagnostic's optional TV UA is intentionally a
  narrower admission experiment and does not reproduce all those overrides.
- `src/preload/util/controller.js` polls `navigator.getGamepads()` and translates
  standard gamepad/axis events. `controller-support.js` sends document keydown/up
  events using `32768 + standard button index`; selected buttons are remapped for
  VacuumTube volume controls. Our adapter tests the six basic A/B/D-pad edges,
  including disconnect release. It has no DOM sink yet.
- `media-capabilities.js` adds Cobalt-style validation of extra MIME parameters
  such as width, height and frame rate. `h264ify.js` filters capability answers
  for codec selection; it does not add a decoder or guarantee an H.264 rendition.
  Its settings include a YouTube low-memory hint, not a 512 MiB residency guarantee.

VacuumTube was inspected, not installed or run. No VacuumTube code was copied into
this prototype. Its wrapper approach depends on a full working browser; its
desktop binaries cannot execute natively on the Xenon/title runtime.

## Engine candidates and 512 MiB constraint

These are feasibility assessments, not console benchmarks. The Xbox 360's PowerPC
CPU and big-endian title ABI are not interchangeable with Linux ppc64le. Nominal
512 MiB shared RAM must cover runtime, networking, engine/JS heap, fonts, decoded
images, GPU textures, media buffers, video frames and sound with bounded peaks.
No measured peak memory, startup time or frame rate is available on console.

| Candidate | Useful property | Critical missing work / present conclusion |
| --- | --- | --- |
| Modern Chromium/Electron | Modern JS/DOM/MSE on supported desktops; host reference available | [Electron upstream](https://github.com/electron/electron#platform-support) lists x64/arm64 Windows/Linux, not Xbox 360. New PPC/title-runtime, rendering, threading and media ports would be needed. Not an available Xbox engine. |
| Current Cobalt | YouTube-oriented engine; [current upstream](https://github.com/youtube/cobalt/blob/bc0f8fe1d4c71c958ea32788e3d688889ad4034e/README.md) is Chromium-based | Inspected [build status](https://github.com/youtube/cobalt/blob/bc0f8fe1d4c71c958ea32788e3d688889ad4034e/cobalt/BUILD_STATUS.md) shows Android, Evergreen, Linux, tvOS, not Xbox 360. Does not establish a PPC32/XEX port or fit in this memory budget. |
| Older Cobalt / Starboard | Smaller TV-oriented abstraction is a narrower architectural lead | Would require an identified licensed release, PPC JS backend/interpreter, complete Starboard platform, graphics, networking, storage, codecs and validation against today's service. Historical compatibility does not guarantee current service support. Not built here. |
| Historical h5vcc | Explicit Xbox 360 target and useful platform/media abstractions | Missing ready public console platform build path; old DOM/JS/media behavior needs audit. Cannot assume modern syntax/APIs or secure TLS by backporting a UA. |
| WebKit / JavaScriptCore | Upstream [CPU detection](https://github.com/WebKit/WebKit/blob/main/Source/WTF/wtf/PlatformCPU.h) still recognizes big-endian PPC variants | CPU macros do not imply a supported engine port, working interpreter/JIT, GPU backend or Xbox OS API. A full browser/media embedding remains a substantial project. |
| NetSurf framebuffer port | [Upstream README](https://github.com/netsurf-browser/netsurf) points to a simple self-contained framebuffer front end | Useful for small pages; no demonstrated complete modern Leanback DOM/JS/CSS/MSE stack. A lightweight UI renderer is insufficient. Not selected as a Leanback engine. |
| QuickJS plus custom bindings | [Upstream documentation](https://github.com/bellard/quickjs/blob/master/doc/quickjs.texi) describes an interpreter and near-complete ES2025 support | Modern JS syntax alone is not a browser. No built-in DOM/CSS layout/compositor, browser fetch semantics, MSE or native YouTube media integration. PPC/XDK portability untested here. Most of the web platform would still need implementation. |

No evaluated candidate is currently available here as a demonstrated native XEX
engine executing modern Leanback. A full port might succeed, but its cost and
memory viability are unresolved; calling it impossible or compatible would both
overstate the evidence.

## XenonFin reuse audit

Inspected [XenonFin](https://github.com/sbayfield09/XenonFin) at
`554adb8d79a251e4b502ac4a87a328cbe5055b09`. Its checkout is unchanged.

| Component | Actual current boundary | Reuse assessment |
| --- | --- | --- |
| Native networking | `platform/xenon/protocol.c` rejects HTTPS; numeric IPv4 HTTP only. `native_net`/`tcp_client` use libXenon/lwIP, not desktop curl | Useful protocol/buffering patterns; no current direct YouTube HTTPS transport. Need DNS, TLS with certificate/hostname/time validation and title-compatible sockets. |
| Graphics/controller | Native libXenon framebuffer/GPU/USB code; desktop SDL is separate | Algorithms and mappings may be reused. Cannot assume bare-metal drivers can run inside an XDK title. Browser layout/compositing still absent. |
| Native video | `video_decode.c` uses pl_mpeg MPEG-1, bounded <=720x480 and <=30 fps; `video_ps` handles progressive MPEG-PS | Not a YouTube MP4/WebM/DASH/MSE player. No native H.264/VP9/AV1 decoder was identified in this implementation. |
| Native audio | MP3 decoding and checked PCM/DMA ownership, documented hardware regression history | Preserve source and working reference binaries. YouTube AAC/Opus and title-runtime audio need separate work; don't replace functioning DMA/audio implementation speculatively. |
| Desktop player | FFmpeg H.264/AAC and other desktop formats; documented exclusions for DASH/HLS/DRM | Not the native Xbox backend; host codec success would not establish console support. |
| Build/runtime | Makefile produces XeLL ELF using libXenon | No XEX generation. XEX requires a title-runtime-compatible toolchain/backends, not extension conversion. |

XenonFin's native video workflow depends on **Jellyfin server transcoding to
MPEG-1/MP3**. That does not meet the direct-YouTube/no-companion requirement. The
current YouTube rendition set has not been fetched here; compatibility cannot be
rescued by assuming YouTube serves the native MPEG-1 profile.

## Narrow compatibility layer, before considering a custom client

The useful narrow path would keep the real Leanback JS/UI in a compatible engine,
adapt platform discovery (Cobalt/h5vcc feature queries), translate controller edges
into real DOM events, and implement a truthful media capability/player bridge.
Identity hints and a small feature shim are plausible **only after** the base
engine already executes the application. They cannot replace the DOM/layout engine.

The media bridge cannot generally be `play(video.src)`: MSE often uses a `blob:`
object URL with separate audio/video `SourceBuffer` appends. An implementation
needs append queues, fMP4/WebM demux, timestamps, buffered ranges, seek/removal,
backpressure, duration/end-of-stream, error propagation and synchronized decoding.
The native MPEG-PS stream interface is not that contract. Even intercepting an
elementary stream leaves missing codecs. Filter capabilities honestly; never
advertise H.264/AAC or MSE success when the console cannot process them. EME/CDM
support may be required for protected content, but not every video needs DRM.

Proposed bounded investigation order: establish an actual XEX and verified native
HTTPS first; select and run an engine on console; retrieve the actual TV build and
trace its used APIs; test a recorded bootstrap offline for JS/DOM failures; add
only measured missing platform hooks; then attempt clear-media MSE integration.
An offline fixture is a compatibility test only, never a replacement for direct
online console playback. If the engine requires most of a modern browser to be
reimplemented, this is no longer a narrow layer. No custom YouTube client has
been substituted or recommended as a completed fallback.

## Executed checks and exact stopping points

All checks below ran on the Linux cloud host, not on Xbox hardware.

| Check | Outcome and meaning |
| --- | --- |
| Build adapter and native gate with C99, `-Wall -Wextra -Werror` | Passed. No SDK/toolchain install needed for these host checks. |
| Controller edge test | Passed press/hold/release/direction/disconnect assertions. Page navigation untested. |
| Real XenonFin endpoint parser called with `https://www.youtube.com/tv` | Expected diagnostic exit 2: `HTTPS unavailable on this native build; no insecure TLS fallback`. Stops before DNS/TCP/TLS, not an HTTP status. |
| curl TV-profile GET | Exit 56; proxy CONNECT status **403**, origin HTTP status **0**, zero body bytes. No origin TLS session established. A reported TLS verification field of 0 does not establish TLS success when CONNECT failed. |
| Chromium 154 TV-profile navigation | `net::ERR_TUNNEL_CONNECTION_FAILED`; no YouTube JS or resources loaded. Feature detection ran on Chromium's **error document**, not Leanback. |
| Chromium fixture self-test | Initially command sandbox blocked Chromium startup; normal process execution outside that command sandbox works. A data-URL fixture was blocked by administrator policy. Changed fixture setup to a local DevTools-created about:blank document; test passed, capturing the intentionally thrown `fixture exception`. No Chromium sandbox-disabling flag used. |
| Rendering / API failures in real Leanback | **Unrun** because page loading is blocked. No Leanback JS exception or missing API has been observed; don't treat absence of exceptions as success. |
| Native media codec verification against YouTube | **Unrun**; source incompatibility identified, no actual YouTube media fetched or decoded. |
| `.xex` build and console launch | **Unrun/blocked**: no licensed XDK or compatible working XEX toolchain/console. `xdk/main.cpp` is uncompiled diagnostic source. No XEX binary produced. |

Raw host diagnostics are in ignored `artifacts/`; the compact
[evidence.json](evidence.json) records the selected current-run results. Host
browser capability answers are not console capability or decoded-media evidence.
No YouTube UI frame, console video output or audible console playback was verified.

## Outstanding actions

The environment draft saves custom domain requirements for `youtube.com`,
`www.youtube.com`, `*.youtube.com`, `*.ytimg.com`, `*.googlevideo.com`,
`*.googleapis.com`, `*.gstatic.com`, `*.ggpht.com`, `consent.youtube.com` and
`accounts.google.com`, preserving package-manager presets. These are a starting
set, not a measured complete resource graph. No credentials were requested or
saved. Review and save the network settings, then publish the environment as
required by its configuration UI; draft saving alone did not change this runtime.
Retry the probes after policy propagation before asserting service access.

A legally available working XEX toolchain/title-runtime development environment
and physical console verification remain external prerequisites. Existing source
does not supply them. No install/start instructions were saved to claim a working
console environment. The build/test procedure in README is explicit about which
steps have and have not been validated.
