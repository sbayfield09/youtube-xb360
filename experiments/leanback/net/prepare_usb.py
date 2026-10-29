#!/usr/bin/env python3
"""Provision a private TLS NV seed on the user's USB drive, once per console.

This is setup only: no server runs on the computer. Never publish/copy the seed.
Do not restore an old seed: the console advances it before using TLS randomness.
"""
import argparse
import os
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("usb_root",type=Path)
    a=p.parse_args()
    if not a.usb_root.is_dir():
        p.error("USB root must be an existing directory")
    dest=a.usb_root/"leanback-seed.bin"
    if dest.exists():
        p.error("Seed already exists; preserve the console's updated seed")
    fd=os.open(dest,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
    with os.fdopen(fd,"wb") as f:
        f.write(os.urandom(64))
        f.flush()
        os.fsync(f.fileno())
    print("Created private leanback-seed.bin. Keep it on this console's USB drive.")
    print("Do not upload, share, clone or restore this file. No seed value was printed.")

if __name__=="__main__":main()
