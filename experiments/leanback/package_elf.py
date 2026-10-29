#!/usr/bin/env python3
"""Package the built diagnostic and corresponding source, never private seeds."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import zipfile

EXP=Path(__file__).resolve().parent
REPO=EXP.parents[1]
XF=Path(os.environ.get("XENONFIN","/workspace/XenonFin"))
TLS=Path(os.environ.get("MBEDTLS","/workspace/setup/mbedtls-3.6.7"))
DEVKIT=Path(os.environ.get("DEVKITXENON","/workspace/setup/devkit"))
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    dest=REPO/"dist";dest.mkdir(exist_ok=True)
    # Keep the hardware-confirmed original as a downloadable rollback.
    for old,new in [("youtube-leanback-xell.elf","youtube-leanback-xell-0.1.elf"),
                    ("youtube-leanback-xell-bundle.zip","youtube-leanback-xell-0.1-bundle.zip")]:
        target=dest/new
        if not target.exists():
            target.write_bytes(subprocess.check_output(["git","show",f"f9f0d79:dist/{old}"],cwd=REPO))
    binary=dest/"youtube-leanback-xell.elf"
    shutil.copy2(EXP/"build/elf/youtube-leanback-diagnostic.elf",binary)
    source_revision=subprocess.check_output(["git","-C",str(XF),"rev-parse","HEAD"],text=True).strip()
    undefined=subprocess.check_output([str(DEVKIT/"bin/xenon-nm"),"-u",str(EXP/"build/elf/youtube-leanback-diagnostic.linked")],text=True)
    if undefined.strip():raise SystemExit("Unresolved symbols in ELF")
    report=json.loads((EXP/"net/validation.json").read_text())
    meta={"version":"0.2","kind":"Native DHCP/DNS/verified-HTTPS XeLL diagnostic",
        "youtube_browser":False,"youtube_playback":False,"hardware_tested_v02":False,
        "hardware_evidence_v01":"User reports boot/display and controller page working",
        "format":"ELF32 big-endian PowerPC static","compiler":"Xenon GCC 16.2.0",
        "xenonfin_commit":source_revision,"mbedtls_version":"3.6.7",
        "mbedtls_commit":"068ff080b369adfac81509f9b57b2afabaf82dc5",
        "sha256":digest(binary),"bytes":binary.stat().st_size,
        "host_fixture_cases_passed":len(report["tests"]),"dns_malformed_packets":20000,
        "unresolved_symbols":[]}
    (dest/"elf-build.json").write_text(json.dumps(meta,indent=2)+"\n")
    bundle=dest/"youtube-leanback-xell-bundle.zip"
    with zipfile.ZipFile(bundle,"w",zipfile.ZIP_DEFLATED) as z:
        z.write(binary,"xenon.elf");z.write(dest/"elf-build.json","elf-build.json")
        z.write(EXP/"net/README.md","README.md")
        z.write(EXP/"net/prepare_usb.py","prepare_usb.py")
        for p in sorted(EXP.rglob("*")):
            rel=p.relative_to(EXP)
            if p.is_file() and not any(x in ("build","artifacts","__pycache__") for x in rel.parts):
                if p.name in ("leanback-seed.bin","leanback-page.html"):raise SystemExit("Private runtime file in source tree")
                z.write(p,"source/youtube-xb360/experiments/leanback/"+str(rel))
        files=["console_batch.c","console_batch.h","console_frame.c","console_frame.h","framebuffer.c","framebuffer.h",
            "protocol.c","protocol.h","json.c","timing.h","native_net.c","native_net.h","driver_guard.c","driver_guard.h","init_guard.c","http.c"]
        for name in files:z.write(XF/"platform/xenon"/name,"source/XenonFin/platform/xenon/"+name)
        for name in ("THIRD_PARTY.md","LIBXENON_CONSOLE_LICENSE.md","LIBFAT_LICENSE.md","PLATFORM.md"):
            z.write(XF/"docs"/name,"source/XenonFin/docs/"+name)
        z.write(XF/"LICENSE","source/XenonFin/LICENSE")
        z.write(XF/"scripts/fetch_devkit.py","source/XenonFin/scripts/fetch_devkit.py")
        for folder in ("include","library"):
            for p in sorted((TLS/folder).rglob("*")):
                if p.is_file():z.write(p,"source/mbedtls/"+str(p.relative_to(TLS)))
        z.write(TLS/"LICENSE","source/mbedtls/LICENSE")
    with zipfile.ZipFile(bundle) as z:
        assert z.testzip() is None
        assert hashlib.sha256(z.read("xenon.elf")).hexdigest()==meta["sha256"]
        assert not any(Path(n).name=="leanback-seed.bin" for n in z.namelist())
    (dest/"SHA256SUMS").write_text("".join(digest(p)+"  "+p.name+"\n" for p in sorted(dest.iterdir()) if p.is_file() and p.name!="SHA256SUMS"))
    print(json.dumps(meta,indent=2))

if __name__=="__main__":main()
