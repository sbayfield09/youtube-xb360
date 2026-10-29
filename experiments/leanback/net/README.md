# Native direct-HTTPS diagnostic 0.2

This replaces the ELF's parser-only page 2 with a real direct-network test. It is
**not yet a browser or YouTube player**. The user confirmed v0.1 boot/display and
reported its page-3 controller test works; the v0.2 networking/storage path has
not yet been verified on a physical console.

## Prepare and test on console

1. Keep the working v0.1 ELF. Download the new `dist/youtube-leanback-xell.elf`,
   copy it to your FAT32 USB root as `xenon.elf`, and leave the USB inserted.
2. **Once for this console**, generate a private seed on that USB with Python 3:

   ```sh
   python prepare_usb.py /path/to/USB
   # Windows example: py prepare_usb.py E:/
   ```

   Use the helper from this directory or the source bundle. It uses your
   computer's OS CSPRNG to create exactly 64 bytes in `leanback-seed.bin`, without
   printing them. This is initial provisioning, not a running companion service.
   Never upload, share, clone between consoles or restore an old copy of this
   file. Keep the updated file: the console refreshes it before releasing TLS
   randomness. If the seed is lost, provision a fresh one. No seed is in the
   release/bundle. The console refuses a missing, zero or wrong-sized seed.
3. Set the console dashboard date/time correctly. The diagnostic reads the SMC
   clock through libXenon and displays UTC after initialization. It refuses an
   implausible year outside 2026–2036; that range alone cannot establish correct
   time. **Check the displayed date/time yourself.** No unauthenticated network
   time or frozen build timestamp substitutes for the certificate clock.
4. Connect Ethernet, boot using your existing XeLL setup, navigate to page 2,
   and press A. It starts DHCP and, after a verified lease, automatically runs
   seed/clock setup → DNS → TCP :443 → TLS → HTTPS GET `/tv`.
5. Photograph page 2 when it finishes or fails. Include stage, detail, IP, TLS
   error, verification flags, HTTP status, byte counts and displayed UTC.
   X cancels; A retries. B goes back, or requests exit from page 1.

DNS uses **8.8.8.8** explicitly by default because the pinned lwIP library has
DNS disabled and does not retain DHCP DNS options. To use your router or another
resolver, create `leanback-dns.txt` on the same USB containing its dotted IPv4
address, e.g. `192.168.1.1`. The selected resolver is displayed while querying.
The console requires DHCP; there is no static-IP fallback or proxy. The public
DNS resolver is a DNS service, not a companion media server. IPv6 is not supported.

## Interpret results precisely

- **DHCP pending/timeout:** no verified lease yet. Check Ethernet/router; no TLS
  connection has happened.
- **USB seed / clock / Entropy failed:** TLS will not run. The USB must be writable;
  the refreshed seed is flushed, unmounted/remounted and read back before use.
  Physical cache/power-loss behavior is unverified; preserve the current seed
  rather than restoring filesystem snapshots. This design relies on the initial
  private seed, not a hardware RNG; it cannot recover secrecy after seed exposure
  without fresh provisioning.
- **DNS failure:** the fixed-host, A-record resolver validates source IP/port,
  randomized transaction ID, question name/type, compressed names and bounded
  CNAME chains. It rejects malformed/truncated replies, non-public A answers,
  more than 16 answers or no answer. Three queries, 3s apart. No DNSSEC or DNS/TCP
  fallback is implemented. TLS independently verifies the resolved service.
- **TCP timeout/error:** couldn't connect to resolved public IPv4 port 443 within
  15s, or the transport subsequently failed.
- **TLS failed:** exact Mbed TLS error and verification flags are displayed. No
  HTTP request is sent until required chain, hostname and time checks pass.
  `Certificate verified: NO` is authoritative even if flags are zero before the
  handshake. `ffffffff` may mean verification never completed. The TLS client
  supports TLS 1.2 ECDHE-RSA/ECDHE-ECDSA with AES-GCM, not TLS 1.3. Unsupported
  suites, CA chains or algorithms produce a failure, never an insecure fallback.
- **HTTP status N:** received from the origin over verified TLS, not synthesized
  by the UI. Redirects are reported but not followed. HTTP framing errors and
  non-200 bodies can stop the existing parser with their status still displayed.
  HTTP error status does not invalidate the earlier TLS result.
- **HTTPS complete / HTTP 200:** a complete bounded response body was retrieved.
  It is saved as `leanback-page.html` on the seed's USB volume. A save failure is
  reported separately. This proves neither TV-client acceptance nor a working
  web application: admission pages can return 200. No resources, JS, CSS, DOM,
  controller-to-DOM events or media are executed in this diagnostic.

Bounds: 64 KiB encrypted receive ring; 16 KiB TLS records; 8 KiB HTTP headers;
2 MiB page body; 90s overall/30s idle timeout after seed setup. A TCP EOF without
complete HTTP framing or authenticated TLS close-notify is a truncation failure.
Response-body data is not printed to serial or uploaded. There is no sign-in,
cookie store, account token or application credential in this build.

## Build and host tests

```sh
python3 net/fetch_tls.py /workspace/setup/mbedtls-3.6.7
make -f elf.mk -j2
make test
python3 net/tests/run.py
```

Run from `experiments/leanback`. Existing Free60 toolchain and unchanged XenonFin
sources are prerequisites, as described in `../elf/README.md`. For the extracted
source bundle set `MBEDTLS` to `source/mbedtls` and `XENONFIN` to `source/XenonFin`
(absolute paths); `DEVKITXENON` selects the toolchain. The bundle contains all TLS
library/include sources needed by this build. No upstream generated-file step or
framework checkout is required by these custom makefiles.

Host tests need a C compiler, make, Python 3, OpenSSL **3.5+** for explicit fixture
certificate dates, and permission to open local test sockets. They use ephemeral
keys/certificates/seeds in a temporary directory, removed afterward. They run the
**actual probe state machine, TLS configuration and HTTP parser** with a POSIX
socket adapter that fragments traffic; raw lwIP and console hardware are not
emulated. Test roots exist only in that test executable. The production ELF
always embeds public GTS roots.

Passed cases: verified HTTPS GET/body save and NV-seed refresh; wrong hostname;
untrusted root; expired certificate; truncated response; report-only redirect;
missing seed; failed seed update. DNS A/CNAME and malformed/compressed-name tests
include 20,000 deterministic malformed packets under ASan/UBSan. Actual platform
seed/clock logic is tested with mocked FAT/SMC calls. Neither those mocks nor the
TLS fixtures prove physical NIC, USB flush, clock, big-endian crypto or real
YouTube compatibility. [Validation evidence](validation.json) records the results.

## Provenance and remaining work

Mbed TLS **3.6.7**, upstream commit
`068ff080b369adfac81509f9b57b2afabaf82dc5`, dual Apache-2.0 OR GPL-2.0-or-later.
This linked combination uses GPL-2.0-or-later. No TLS verification is disabled.
The four embedded GTS roots came from the Debian Mozilla CA store; their public
PEMs and hashes are in `certs/`. New CA chains may require an audited trust-store
update. No cloud proxy CA is embedded. libXenon NIC initialization and poll/driver
guards are reused unchanged from the pinned XenonFin checkout; its audio/video
code is untouched and not linked into this diagnostic.

The next gate is a photo/log of this test on the console. If HTTPS succeeds,
inspect the saved page and its required resources. Browser-engine portability and
native codec/MSE work remain separate unresolved gates. This remains ELF-only;
the XEX title-runtime requirement has not been solved.
