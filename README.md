# Harman AX1800M adapter checker


[Download the checker ZIP](https://github.com/RandomGenerated543/harman-ax1800m-adapter-checker/raw/refs/heads/main/Harman-AX1800M-readonly-probe-v0.1.zip)


Read-only diagnostics for the AX1800M / Suding adapter with system `202603120956CA`, protocol `20260109175612`, and product `v851se-yunlian-cp`. This is not a firmware update or a wireless cockpit fix.


## Run


1. Download and unzip the ZIP.
2. Power the adapter from USB and connect your computer to its Wi-Fi.
3. In the extracted folder, run `py probe-adapter.py` on Windows, or `python3 probe-adapter.py` on Mac/Linux. Python 3 is required.
4. Send back the generated `harman-adapter-probe` folder privately.


Default address: `192.168.50.100`. Use `--host LOCAL_IP` to override it. Expected duration: about 10–30 seconds.


The checker reads identity and the web page and checks existing SSH/Telnet ports for banners. It does not attempt logins, upload firmware, execute updater commands, reboot, or change settings. Five host tests passed; hardware execution remains untested.


See [detailed instructions](./PROBE-README.md). Experimental software; use at your own risk. Do not publish diagnostic captures without reviewing them for private information.

