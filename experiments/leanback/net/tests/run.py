#!/usr/bin/env python3
"""Local-only fixture tests of actual probe.c, TLS config and HTTP parser.
The raw networking and FAT/SMC interfaces are host adapters, not console evidence.
"""
import json
import os
from pathlib import Path
import socket
import ssl
import subprocess
import tempfile
import threading

EXP=Path(__file__).resolve().parents[2]
TLS=Path(os.environ.get("MBEDTLS","/workspace/setup/mbedtls-3.6.7"))
XF=Path(os.environ.get("XENONFIN","/workspace/XenonFin"))
TEST=EXP/"net/tests"

def cmd(*args,cwd=None):
    r=subprocess.run(list(map(str,args)),cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if r.returncode:raise RuntimeError(r.stderr.decode(errors="replace"))

def main():
    subprocess.run(["make","-f","net/host.mk","-j2",f"MBEDTLS={TLS}"],cwd=EXP,check=True,stdout=subprocess.DEVNULL)
    records=[]
    with tempfile.TemporaryDirectory(prefix="leanback-tls-test-") as tmp:
        root=Path(tmp)
        def issue(name,host,days):
            cmd("openssl","req","-newkey","rsa:2048","-nodes","-keyout",root/f"{name}.key","-out",root/f"{name}.csr","-subj",f"/CN={host}")
            (root/f"{name}.ext").write_text(f"subjectAltName=DNS:{host}\nbasicConstraints=critical,CA:FALSE\nkeyUsage=critical,digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth\n")
            dates=["-not_before","20200101000000Z","-not_after","20200102000000Z"] if days<0 else ["-days",str(days)]
            cmd("openssl","x509","-req","-in",root/f"{name}.csr","-CA",root/"ca.crt","-CAkey",root/"ca.key","-CAcreateserial",*dates,"-sha256","-extfile",root/f"{name}.ext","-out",root/f"{name}.crt")
        cmd("openssl","req","-x509","-newkey","rsa:2048","-nodes","-keyout",root/"ca.key","-out",root/"ca.crt","-days","3","-subj","/CN=Local test CA","-addext","basicConstraints=critical,CA:TRUE")
        issue("good","www.youtube.com",2);issue("wrong","wrong.example",2);issue("expired","www.youtube.com",-1)
        cmd("openssl","req","-x509","-newkey","rsa:2048","-nodes","-keyout",root/"untrusted.key","-out",root/"untrusted.crt","-days","2","-subj","/CN=www.youtube.com","-addext","subjectAltName=DNS:www.youtube.com")
        (root/"ca.h").write_text("static const char lb_roots[]="+json.dumps((root/"ca.crt").read_text())+";\n")
        exe=root/"probe"
        cmd("cc","-std=c99","-g","-O1","-Wall","-Wextra","-Werror","-Wl,--gc-sections",
            '-DMBEDTLS_CONFIG_FILE="tls_config.h"',f'-DLB_TEST_CA_HEADER="{root}/ca.h"',
            "-I"+str(TEST/"shim"),"-I"+str(EXP/"net"),"-I"+str(TLS/"include"),"-I"+str(XF/"platform/xenon"),
            EXP/"net/probe.c",EXP/"net/dns_wire.c",TEST/"raw_posix.c",TEST/"host_platform.c",TEST/"host_main.c",
            XF/"platform/xenon/http.c",EXP/"build/host-tls/tls.a","-o",exe)
        (root/"fixture:").mkdir()
        cases=[("valid","good",b"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello"),
            ("wrong-host","wrong",b""),("untrusted","untrusted",b""),("expired","expired",b""),
            ("truncated","good",b"HTTP/1.1 200 OK\r\nContent-Length: 50\r\n\r\nhello"),
            ("redirect","good",b"HTTP/1.1 302 Found\r\nContent-Length: 0\r\nLocation: https://example.com/\r\n\r\n"),
            ("seed-write-failure","good",b""),("missing-seed","good",b"")]
        for name,cert,response in cases:
            seed=os.urandom(64);(root/"seed.bin").write_bytes(seed)
            if name=="missing-seed":(root/"seed.bin").unlink()
            listener=socket.socket();listener.bind(("127.0.0.1",0));listener.listen();listener.settimeout(.2)
            port=listener.getsockname()[1];stop=threading.Event();requests=[]
            ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.minimum_version=ssl.TLSVersion.TLSv1_2;ctx.maximum_version=ssl.TLSVersion.TLSv1_2
            ctx.load_cert_chain(root/f"{cert}.crt",root/f"{cert}.key")
            def serve():
                while not stop.is_set():
                    try:connection,_=listener.accept()
                    except socket.timeout:continue
                    except OSError:return
                    try:
                        connection.settimeout(4)
                        with ctx.wrap_socket(connection,server_side=True) as tls:
                            request=b""
                            while b"\r\n\r\n" not in request:
                                data=tls.recv(1024)
                                if not data:break
                                request+=data
                            requests.append(request)
                            if response:tls.sendall(response)
                            try:tls.unwrap().close()
                            except (OSError,ssl.SSLError):pass
                    except (OSError,ssl.SSLError):connection.close()
                    return
            thread=threading.Thread(target=serve,daemon=True);thread.start()
            env=os.environ.copy();env["LB_TEST_PORT"]=str(port)
            if name=="seed-write-failure":env["LB_TEST_SEED_WRITE_FAIL"]="1"
            try:r=subprocess.run([str(exe)],cwd=root,env=env,capture_output=True,text=True,check=True,timeout=15)
            finally:stop.set();listener.close();thread.join(timeout=5)
            fields=dict(line.split("=",1) for line in r.stdout.splitlines())
            if name=="valid":
                assert fields["verified"]=="1" and fields["complete"]=="1" and fields["status"]=="200" and fields["body"]=="5",fields
                assert requests[0].startswith(b"GET /tv HTTP/1.1\r\n")
                assert (root/"fixture:/leanback-page.html").read_bytes()==b"hello"
                assert (root/"seed.bin").read_bytes()!=seed
            elif name in ("wrong-host","untrusted","expired"):
                assert fields["verified"]=="0" and fields["status"]=="0" and int(fields["flags"])!=0 and not requests,fields
            elif name=="truncated":assert fields["verified"]=="1" and fields["complete"]=="0",fields
            elif name=="redirect":assert fields["verified"]=="1" and fields["status"]=="302" and fields["complete"]=="1" and len(requests)==1,fields
            else:assert fields["stage"]=="Entropy failed" and fields["verified"]=="0" and not requests,fields
            records.append({"case":name,"passed":True,**fields})
            print("PASS:",name,fields["stage"])
        dns=root/"dns-test"
        cmd("cc","-std=c99","-g","-Wall","-Wextra","-Werror","-fsanitize=address,undefined","-I"+str(EXP/"net"),TEST/"dns_test.c",EXP/"net/dns_wire.c","-o",dns)
        subprocess.run([str(dns)],check=True)
        platform=root/"platform-test"
        for name in ("uda:","udb:","udc:"):(root/name).mkdir()
        cmd("cc","-std=c99","-g","-Wall","-Wextra","-Werror","-Wl,--gc-sections","-Wl,--wrap=gettimeofday",
            '-DMBEDTLS_CONFIG_FILE="tls_config.h"',"-I"+str(TEST/"shim"),"-I"+str(EXP/"net"),"-I"+str(TLS/"include"),
            TEST/"platform_test.c",EXP/"net/platform.c",EXP/"build/host-tls/tls.a","-o",platform)
        subprocess.run([str(platform)],cwd=root,check=True)
    path=EXP/"artifacts/net-host-tests.json";path.parent.mkdir(exist_ok=True)
    path.write_text(json.dumps({"scope":"Host adapters; actual probe/TLS/parser; no console validation","tests":records,"dns_malformed_packets":20000,"platform_logic_test":"passed; FAT/SMC mocked"},indent=2)+"\n")

if __name__=="__main__":main()
