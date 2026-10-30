# Experimental wireless CarPlay adapter packages

**HIGHLY EXPERIMENTAL. USE AT YOUR OWN RISK. NO WARRANTY.** Incorrect firmware can disable the adapter. The maintainers/contributors accept no liability to the extent permitted by applicable law. These packages modify the adapter, not the MHI2 or cockpit firmware. An existing compatible wired cockpit-map implementation is required.

Both ZIPs include the candidate binary, stock rollback, uploader, instructions, source and retained validation. Hardware/platform and installed versions must match. These are separate packages and are **not interchangeable**. Do not bypass identity/hash checks.

| Package | Exact intended target | Test status |
| --- | --- | --- |
| [HW301 / CXZ521 / SEAT MU1409 v0.1](HW301-CXZ521-SEAT-MU1409-v0.1.zip) | HW301/CXZ521; reference original APP HW301_20250630126, system 20250320; custom app based on vendor v129 | One vehicle's operator reported working wireless CarPlay and cockpit maps. Not universal acceptance. |
| [AX1800M / Suding / Leon MU1447 / October v0.2](AX1800M-Suding-Leon-MU1447-October-v0.2.zip) | platform AX1800M, custom suding, product v851se-yunlian-cp, system 202504180933CA, stock protocol 20261008165747 | October candidate with r6 uploader. Installation reported complete; do not infer visible vehicle acceptance from flashing alone. |

**Neither package matches the newer AX1800M/Suding system 202603120956CA / protocol 20260109175612 covered by this repository's read-only probe.** No exact firmware fix for that newer combination is included.

## Use

Download and extract the entire matching ZIP. Read its README before connecting or flashing. Keep IMAGES.json and the firmware folder together with the uploader; keep the rollback available. Start with the local `dry-run`, then `probe` to confirm the adapter identity. Use the package's own install and rollback commands. Do not mix files from older packages.

- HW301 package: `sh adapter.sh dry-run` on Mac/Linux. Read README.md and RECOVERY.md for the exact procedure and compatibility limits.
- AX1800M October package: `py adapter-october-r6.py dry-run` on Windows, or `python3 adapter-october-r6.py dry-run` on Mac/Linux. This is the r6 October package, not the earlier September candidate.

The original read-only `probe-adapter.py` remains read-only. The separately downloaded firmware uploaders have explicit flash/rollback commands that do write to the adapter.

## Provenance

The HW301 native proxy reuses [kamgurgul/mib2q-carplay](https://github.com/kamgurgul/mib2q-carplay/tree/cf8ab2effb24292025e7648bb79244fed3805741/dongle/hw301). The AX1800M package retains its upstream source/reference and patch evidence, including [OMKE/wireless-carplay-virtual-cockpit](https://github.com/OMKE/wireless-carplay-virtual-cockpit). See the included licenses and build notes. Vendor binaries remain proprietary inputs; the project source license does not relicense them.

These download copies use neutral device labels and omit personal tester references/local account paths. Candidate and rollback firmware bytes are unchanged from the original prepared packages. Archive-level checksums differ because documentation and folder names were cleaned up; internal firmware hashes remain in IMAGES.json.

## Download checksums (SHA-256)

```text
89c936f5b6d4010d02570b069e1ee05c52b80a5e0229a3334183b95163f00579  HW301-CXZ521-SEAT-MU1409-v0.1.zip
b24600fea982667e896580b7b19e1712de96249da260cf5cb5f5f8ae33fcd1d6  AX1800M-Suding-Leon-MU1447-October-v0.2.zip
```
