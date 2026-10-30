# Wireless CarPlay adapter checker and downloads

**HIGHLY EXPERIMENTAL. USE AT YOUR OWN RISK. NO WARRANTY.** Read the compatibility instructions before using any firmware package.

## Adapter firmware downloads

- [HW301 / CXZ521 / SEAT MU1409 v0.1 ZIP](./HW301-CXZ521-SEAT-MU1409-v0.1.zip)
- [AX1800M / Suding / Leon MU1447 / October v0.2 ZIP](./AX1800M-Suding-Leon-MU1447-October-v0.2.zip)
- [Compatibility, checksums, provenance and instructions](./ADAPTER-FIRMWARE.md)

Both packages contain candidate and rollback binaries plus their uploaders and instructions. They are not interchangeable. Neither matches system `202603120956CA` / protocol `20260109175612`; that newer adapter is covered only by the read-only checker below.

## Read-only AX1800M checker

[Download the checker ZIP](./Harman-AX1800M-readonly-probe-v0.1.zip)

Read-only diagnostics for the AX1800M / Suding adapter with system `202603120956CA`, protocol `20260109175612`, and product `v851se-yunlian-cp`. This checker is not a firmware update or a wireless cockpit fix.

1. Download and unzip the checker ZIP.
2. Power the adapter from USB and connect your computer to its Wi-Fi.
3. In the extracted folder, run `py probe-adapter.py` on Windows, or `python3 probe-adapter.py` on Mac/Linux. Python 3 is required.
4. Send back the generated diagnostic folder privately.

Default address: `192.168.50.100`. Use `--host LOCAL_IP` to override it. Expected duration: about 10–30 seconds.

The checker reads identity and the web page and checks existing SSH/Telnet ports for banners. It does not attempt logins, upload firmware, execute updater commands, reboot, or change settings. Five host tests passed; hardware execution remains untested. The separately downloaded firmware uploaders have explicit flash/rollback commands that do write to the adapter.

See [detailed checker instructions](./PROBE-README.md). Do not publish diagnostic captures without reviewing them for private information.
