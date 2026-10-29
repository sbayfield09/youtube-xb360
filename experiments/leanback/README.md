# Xbox 360 Leanback experiment

**Current version 0.2:** [native HTTPS diagnostic and required USB preparation](net/README.md).
It now attempts DHCP/DNS/TLS/GET directly. The original parser-only native gate
and initial feasibility findings below remain available as historical evidence.

**ELF follow-up:** a standalone XeLL diagnostic is now built. See
[ELF instructions](elf/README.md). The XEX/browser/playback limitations below
remain; the ELF is a status/controller/transport-gate diagnostic only.

Status: partial feasibility prototype, **no built or tested `.xex` and no working
YouTube client**. The final deliverable must be a native dashboard-loadable XEX;
a XeLL ELF, extension rename, ELF chainloader, desktop wrapper, or companion server
does not satisfy that requirement. See [REPORT.md](REPORT.md) for evidence and blockers.

The experiment lives in the new `youtube-xb360` repository. XenonFin is a separate,
unchanged sibling checkout. No audio, video, application or build files in XenonFin
are patched. No original-app binaries, Microsoft SDK files or VacuumTube code are
redistributed here. The new diagnostic source is under [LICENSE](LICENSE).

## Reproduce the completed host checks

Use the existing isolated checkouts; do not create a worktree. Requirements: C99
compiler, make, Python 3, curl. Run from `experiments/leanback`:

```sh
make XENONFIN=/workspace/XenonFin test
./build/native-gate
# Expected diagnostic exit 2, BEFORE DNS/TCP/TLS; test target verifies that status.
```

`native-gate` links the real XenonFin endpoint parser and JSON implementation.
It proves rejection by that code on the host, not execution on console hardware.
`input-test` checks A press, held-button suppression, release/direction change and
release on disconnection. It does not prove page navigation.

## HTTP and browser reference probes

These tools are developer diagnostics, **not runtime dependencies or a companion
server for the proposed console application**. They preserve network policy and
TLS verification. Each invocation requires a new output directory. Raw HTML/assets
can contain session identifiers; outputs are ignored by Git and should remain local.

```sh
python3 probe_http.py --profile desktop --output artifacts/http-desktop-NEW
python3 probe_http.py --profile tv --output artifacts/http-tv-NEW
node browser_probe.mjs --self-test --output=artifacts/selftest-NEW
node browser_probe.mjs --output=artifacts/browser-desktop-NEW
node browser_probe.mjs --tv --output=artifacts/browser-tv-NEW
```

Node 22+ and Chromium are required for the browser probe. Set `CHROMIUM` to its
executable if needed. CDP binds to Chromium's local debug interface with a fresh
temporary profile; do not expose its port. Use the normal Chromium sandbox.
`--container-no-sandbox` is an explicit optional diagnostic flag for disposable
containers where Chromium cannot initialize its sandbox; never use that flag in
the console design. A sandbox startup failure is a host harness failure, not a
Leanback JavaScript failure.

HTTP probe exit 2 means incomplete/non-2xx page or static-resource retrieval.
Browser probe exit 2 means a harness/navigation/network failure. Exit 0 alone
never means working Leanback: inspect `browser.json`, UI and playback manually.
The browser probe records uncaught exceptions, HTTP errors, loading failures,
feature availability and codec capability answers. It deliberately leaves
`renderingVerified` and `playbackVerified` false. It does not drive sign-in or play
a video. Static extraction covers at most 16 script/stylesheet references;
dynamic resources require the browser. A TV user agent is only an identity hint;
these probes do not replicate VacuumTube's full client-hint and platform shims.

Cloud YouTube access currently needs the saved domain settings applied. A proxy
CONNECT 403 precedes origin TLS: it is neither a YouTube HTTP response nor evidence
of unsupported JavaScript. Do not disable TLS verification or bypass the proxy.

## XEX build path — not validated in this environment

`xdk/main.cpp` is a separate minimal D3D/XInput bring-up diagnostic. It draws four
focus markers on red, emits down/up events to the debugger, and explicitly logs
that no DOM/network/media bridge exists. These markers are not the YouTube UI.
There is no audio or video decoder in this diagnostic. It is provided to make the
native XEX boundary concrete, not as a claimed completed console prototype.

This cloud machine has no licensed Xbox 360 XDK compiler, libraries, XEX image
builder, compatible open XEX toolchain, or physical console. Build instructions
below are therefore a proposed path, **not tested commands**:

1. On a machine with a legally obtained Xbox 360 XDK and its supported Visual
   Studio integration, create an empty **Xbox 360 C++ game** project (not Win32).
2. Add `xdk/main.cpp` and `input.h`, preserving their relative paths. Disable
   precompiled headers for this translation unit; link the SDK's D3D9/XInput
   libraries through its normal game project settings. Do not link libXenon.
3. Build the Xbox 360 target with the SDK's executable-to-XEX image stage. Set
   the image output to `default.xex`. Retain all compiler/image-builder logs.
   Build errors must be fixed against the actual SDK; this source has not been
   compiled against one here. Do not distribute SDK libraries or keys separately.
4. Verify that the image is an XEX2 executable using an XEX-aware inspector, then
   test it only on your already configured homebrew-capable console. Hardware
   compatibility of the SDK-produced image with that console is unverified.

A normal PowerPC ELF linker/objcopy cannot produce a working equivalent. XenonFin
uses libXenon bare-metal initialization under XeLL, whereas an XDK title uses the
Xbox kernel/title runtime. Reusing algorithms does not make those driver backends
ABI compatible. The existing sound DMA code must not simply be linked into an
XDK title and invoked alongside kernel-owned audio.

## Required console test procedure

Record console/kernel/dashboard, toolchain version, source revisions, image hash,
resolution and available memory. Keep the existing XenonFin binaries for regression
testing; this experiment must never replace them.

1. Confirm actual `default.xex` launch. For the diagnostic, expect four markers;
   check all directions, A/B press/release, held buttons, disconnect/reconnect.
   Capture debugger events. No Leanback navigation is claimed at this gate.
2. After a real title-runtime network/TLS backend exists, resolve YouTube, perform
   a verified HTTPS GET, report cert/hostname/time failures and origin status, and
   fetch referenced assets with bounded storage. Test redirects and rejection of
   invalid certificates. Never use an insecure fallback or companion service.
3. Only with a real engine integrated, load `/tv`, capture script exceptions,
   missing APIs, navigation requests, rendered frames, controller focus behavior
   and peak memory. A downloaded page or a capability listing is not a pass.
4. Log actual video MIME/container/codecs/profile/level, MSE segment sequence,
   buffer use and errors before attempting the native media bridge. Test clear
   media first; encrypted media may require a licensed CDM. No bypass is provided.
5. Verify on-screen moving video and audible synchronized sound, pause/resume,
   seeking, stop/replay, controller disconnect and network loss over five minutes.
   Measure total memory below the 512 MiB shared ceiling with headroom for kernel,
   graphics and audio; do not allocate the entire nominal RAM to a browser heap.

No console stages have been executed in this session. Do not advance the status
to a usable client until those gates pass. A custom client is not selected as a
fallback by this experiment; the narrower compatibility-layer investigation in
the report comes first.
