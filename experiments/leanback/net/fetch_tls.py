#!/usr/bin/env python3
"""Fetch exact upstream TLS source. Does not alter an existing checkout."""
from pathlib import Path
import argparse
import subprocess

COMMIT="068ff080b369adfac81509f9b57b2afabaf82dc5"
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("destination",type=Path)
    a=p.parse_args()
    if not a.destination.exists():
        subprocess.run(["git","clone","--depth","1","--branch","mbedtls-3.6.7",
            "https://github.com/Mbed-TLS/mbedtls.git",str(a.destination)],check=True)
    head=subprocess.check_output(["git","-C",str(a.destination),"rev-parse","HEAD"],text=True).strip()
    if head!=COMMIT:
        raise SystemExit("TLS source is not the pinned 3.6.7 revision; refusing to reset existing files")
    dirty=subprocess.check_output(["git","-C",str(a.destination),"status","--porcelain","--untracked-files=no"],text=True)
    if dirty.strip():raise SystemExit("TLS source has tracked changes; refusing to use it")
    print("Verified Mbed TLS 3.6.7 source revision",COMMIT)

if __name__=="__main__":main()
