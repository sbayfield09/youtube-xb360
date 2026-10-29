These four public Google Trust Services root certificates were copied unchanged
from `/usr/share/ca-certificates/mozilla/` in this Debian cloud environment.
`SHA256.json` identifies the PEM inputs; `../roots.h` embeds those same PEMs.
No session/leaf certificate, private key or environment proxy CA is included.

Upstream root information: https://pki.goog/repository/
Mozilla trust-store project: https://www.mozilla.org/about/governance/policies/security-group/certs/

This deliberately small trust store may reject a valid service chain rooted
elsewhere. Such a failure requires investigating the actual chain and reviewing
a trust-store update; disabling chain verification is not a remedy.
