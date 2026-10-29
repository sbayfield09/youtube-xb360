# XeLL ELF diagnostic 0.1

**Superseded by 0.2:** current downloads include direct DHCP/DNS/verified-HTTPS
testing. Follow [the 0.2 USB setup and test procedure](../net/README.md).
The instructions below describe the preserved 0.1 rollback binary; its boot and
page-3 controller behavior have since been reported working on the console.
For rebuilding 0.2, fetch Mbed TLS using `net/fetch_tls.py` before `make -f elf.mk`.

This build was requested as an interim ELF rather than an XEX. It is a separate
application: **no YouTube UI, HTTPS request, audio or video playback is implemented**.
It shows three diagnostic pages using XenonFin's existing text compositor.
The transport page calls XenonFin's real endpoint parser and reports its HTTPS
rejection before any network request. It is not an HTTP test of YouTube.

## Launch

1. Download `dist/youtube-leanback-xell.elf` from the repository root.
2. On a FAT32 USB drive, copy it as **`xenon.elf`** at the root. Preserve any existing
   file first. This replaces neither XenonFin nor your console's boot configuration.
3. Cold-boot through your console's existing working **XeLL** setup. This is not
   directly launchable in Aurora/XeXMenu. No flashing or bootloader changes needed.
4. Expect a `YOUTUBE LEANBACK - XeLL DIAGNOSTIC 0.1` text screen. D-pad changes
   pages; A on page 2 invokes the transport gate. Page 3 shows key events.
5. B returns to page 1; B from page 1 requests exit by returning to the loader.
   Loader behavior after return and the physical boot/display/input path remain
   unverified. Capture a photo and the console/video-mode details if it fails.

## Rebuild

From an isolated checkout with the unchanged XenonFin sibling repository:

```sh
python3 /workspace/XenonFin/scripts/fetch_devkit.py /workspace/setup/devkit
cd /workspace/youtube-xb360/experiments/leanback
make -f elf.mk -j2 DEVKITXENON=/workspace/setup/devkit XENONFIN=/workspace/XenonFin
make XENONFIN=/workspace/XenonFin test
file build/elf/youtube-leanback-diagnostic.elf
/workspace/setup/devkit/bin/xenon-readelf -h -l build/elf/youtube-leanback-diagnostic.elf
/workspace/setup/devkit/bin/xenon-nm -u build/elf/youtube-leanback-diagnostic.linked
```

The fetcher checks the pinned Free60 OCI manifest and layer hashes, keeping TLS
verification enabled. It extracts the toolchain without running Docker. Existing
checkouts are used; no worktree is required. The source bundle places the needed
XenonFin sources under `XenonFin/`; set `XENONFIN` to that extracted path to rebuild
without a second Git clone. The original fetcher is included under
`XenonFin/scripts/`.

## Validation and provenance

- Built with Xenon GCC 16.2.0, `-Wall -Wextra -Werror`, pinned toolchain manifest
  `sha256:4113852fd3281b50c084f9e26cfb6cdd30b707bb2e7ee676e919dbb7cfbdd24f`.
- Static ELF32 MSB PowerPC, entry `0x3c00`, LOAD memory size `0x123bf94`
  (about 18.2 MiB; not a measured total runtime peak). No unresolved symbols.
- Uses the upstream XeLL ELF address adjustment, matching XenonFin's build method.
- Host controller assertions and the expected HTTPS rejection check pass.
- XenonFin revision `554adb8d79a251e4b502ac4a87a328cbe5055b09`; original checkout
  unchanged. No sound/DMA/video implementation edited or linked into this app.
- Physical boot, controller disconnect handling, display output and return-to-loader
  behavior are unverified. No console success, browser compatibility or playback
  success is inferred from the completed cross-build.

`main.c` and the linked XenonFin combination are GPL-2.0-or-later; see `LICENSE`
beside this file. The original portable input adapter is MIT. The binary/source
archive includes both notices and the libXenon console/font provenance. Underlying
libXenon sources are available from Free60, linked in the bundled third-party notes.
