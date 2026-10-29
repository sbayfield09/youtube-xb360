#!/usr/bin/env python3
"""Host-only bounded HTTP diagnostic. Never a renderer or console transport."""
import argparse
import datetime
import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import subprocess
from urllib.parse import urljoin, urlsplit

UA = {
    "desktop": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/154.0.0.0 Safari/537.36",
    # Diagnostic identity only; not a claim that a Cobalt engine is present.
    "tv": "Mozilla/5.0 (X11; Linux x86_64) Cobalt/27.lts.3.1040976-gold (unlike Gecko) v8/13.8.258.54-jit gles Starboard/18, LeanbackProbe_DESKTOP_x8664_2026/0.1",
}

class Resources(HTMLParser):
    def __init__(self):
        super().__init__()
        self.urls = []

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        u = a.get("src") if tag == "script" else a.get("href") if tag == "link" and a.get("rel") == "stylesheet" else None
        if u:
            self.urls.append(u)

def fetch(url, path, ua):
    # curl preserves inherited proxy/CA settings and validates hostname/certificates.
    p = subprocess.run(["curl", "--silent", "--show-error", "--location",
        "--proto", "=https", "--proto-redir", "=https", "--max-redirs", "5",
        "--connect-timeout", "10", "--max-time", "30", "--max-filesize", "8388608",
        "--compressed", "--user-agent", ua, "--output", str(path),
        "--write-out", "%{json}", url], capture_output=True, text=True)
    try:
        meta = json.loads(p.stdout)
    except ValueError:
        meta = {}
    data = path.read_bytes() if path.exists() else b""
    # No cookies, response headers or signed URLs in the report.
    return {"curl_exit": p.returncode, "http_status": meta.get("http_code", 0),
        "proxy_connect_status": meta.get("http_connect", 0),
        "tls_established": meta.get("time_appconnect", 0) > 0,
        "tls_verify_result": meta.get("ssl_verify_result"),
        "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest(),
        "complete": p.returncode == 0 and 200 <= meta.get("http_code", 0) < 300,
        "final_origin": "{}://{}".format(urlsplit(meta.get("url_effective", url)).scheme, urlsplit(meta.get("url_effective", url)).hostname),
        "content_type": meta.get("content_type"), "redirects": meta.get("num_redirects", 0)}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--profile", choices=UA, default="tv")
    p.add_argument("--output", type=Path, required=True, help="New private output directory")
    a = p.parse_args()
    a.output.mkdir(parents=True, mode=0o700, exist_ok=False)
    root = "https://www.youtube.com/tv"
    page = fetch(root, a.output / "page.html", UA[a.profile])
    report = {"utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "platform": "host-only", "profile": a.profile, "page": page, "resources": [],
        "rendering_tested": False, "playback_tested": False,
        "scope": "At most 16 static script/stylesheet references, not dynamic JS requests"}
    if page["complete"]:
        parser = Resources()
        parser.feed((a.output / "page.html").read_text(errors="replace"))
        for u in list(dict.fromkeys(parser.urls))[:16]:
            u = urljoin(root, u)
            host = urlsplit(u).hostname or ""
            if urlsplit(u).scheme != "https" or not any(host == d or host.endswith("."+d) for d in ("youtube.com", "ytimg.com", "gstatic.com")):
                report["resources"].append({"skipped": "non-approved resource origin", "host": host})
                continue
            r = fetch(u, a.output / ("resource-%02d" % len(report["resources"])), UA[a.profile])
            r["host"] = host
            report["resources"].append(r)
    (a.output / "report.json").write_text(json.dumps(report, indent=2)+"\n")
    print(json.dumps(report, indent=2))
    return 0 if page["complete"] and all(r.get("complete", False) for r in report["resources"]) else 2

if __name__ == "__main__":
    raise SystemExit(main())
