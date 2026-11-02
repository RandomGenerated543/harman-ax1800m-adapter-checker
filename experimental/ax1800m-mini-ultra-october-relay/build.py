#!/usr/bin/env python3
"""Build the MIPS relay shared object and run offline POSIX host tests."""
import os
import pathlib
import shlex
import shutil
import subprocess

ROOT = pathlib.Path(__file__).resolve().parent
BUILD = ROOT / "build"


def command(name, default):
    return shlex.split(os.environ.get(name, default))


def run(argv):
    print("+", " ".join(map(str, argv)), flush=True)
    subprocess.run([str(item) for item in argv], check=True, cwd=ROOT)


def available(executable):
    return shutil.which(executable) or pathlib.Path(executable).is_file()


def main():
    zig = command("ZIG", "zig")
    host_cc = command("HOST_CC", os.environ.get("CC", "cc"))
    if not available(zig[0]):
        raise SystemExit("Zig 0.14.1 not found; set ZIG to its executable")
    version = subprocess.check_output(zig + ["version"], text=True).strip()
    if version != "0.14.1":
        raise SystemExit(f"expected Zig 0.14.1, got {version}")
    if not available(host_cc[0]):
        raise SystemExit("POSIX host compiler not found; set HOST_CC")

    BUILD.mkdir(exist_ok=True)
    run(zig + ["cc", "-target", "mipsel-linux-gnueabihf.2.17", "-mcpu=mips32r2",
               "-shared", "-fPIC", "-Os", "-nostdlib", "-Wl,-soname,libSdHOct.so",
               "-o", BUILD / "libSdHOct.so", ROOT / "src/relay.c", "-lc", "-lpthread"])

    extra_flags = shlex.split(os.environ.get("HOST_CFLAGS", ""))
    for name in ("relay_test", "geometry_test"):
        binary = BUILD / name
        run(host_cc + ["-std=gnu11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function",
                       "-Wno-unused-variable", "-DMSG_NOSIGNAL=0", "-UNDEBUG", "-O1", "-pthread"] +
            extra_flags + [ROOT / f"tests/{name}.c", "-o", binary])
        run([binary])

    print(f"BUILD_PASS zig={version} target=mips32r2 host_tests=PASS firmware_package=NOT_BUILT")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)
