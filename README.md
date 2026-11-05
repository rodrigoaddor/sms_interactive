# sms_interactive

Experimental native NUT monitoring driver for the SMS traditional serial protocol,
intended for the SMS Interactive line, including the Gamer 2000 VA. Basic read-only monitoring has been tested on Gamer 2000 VA firmware 1.4.
Compatibility with other models is not yet established. The proprietary PowerView app is used only as static
protocol evidence and is never launched.

See [protocol notes](docs/protocol.md) for evidence and unresolved details.

## Build

Requires a C compiler, autotools, libtool, pkg-config, Python 3, and NUT sources.
The local `.cache/nut` checkout is disposable and ignored.

```sh
make test
# For a fresh workspace:
git clone --depth 1 https://github.com/networkupstools/nut.git .cache/nut
make integrate
cd .cache/nut
./autogen.sh
./configure --with-drivers=sms_interactive --without-usb --without-snmp --without-ssl --without-doc
make MAKE=/usr/bin/make -C common
make MAKE=/usr/bin/make -C drivers sms_interactive
```

To use another NUT source tree: `make integrate NUT_SOURCE=/path/to/nut`.
The integration script copies the driver and adds its automake target.
No system NUT configuration or installed binaries are changed automatically.

## Serial access without root

The NUT driver service account needs read/write access to the serial device.
Grant it membership in the device's serial group (`uucp` on this machine,
often `dialout` elsewhere). Group membership changes require a new login or
service restart. A custom udev rule is optional and is not shipped with this driver.

For temporary development access, replace `YOUR_USER` with your login name:

```sh
sudo setfacl -m u:YOUR_USER:rw /dev/ttyUSB0
```

This ACL disappears when the USB/IP device is recreated. For a stable device
path, use the adapter's existing `/dev/serial/by-id/` link in `ups.conf`.

## First hardware capture

Ensure no other program owns the serial port. This sends only the three
read-only queries identified by static analysis, stopping at the first failure:

```sh
python3 tools/probe.py /dev/ttyUSB0
```

Check USB/IP before retrying after any communication error. Save successful
JSON output as a hardware fixture for follow-up development.

Example `ups.conf` once the protocol is verified:

```ini
[gamer]
    driver = sms_interactive
    port = /dev/ttyUSB0
    pollinterval = 5
```

The driver publishes voltages, frequency, load, temperature, raw battery
indication, beeper state, status, and alarms. It marks data stale on invalid
responses. It exposes no instant commands and refuses UPS power shutdown.
Battery charge/runtime and automatic power-off are not supported yet.

## Validation in this workspace

- Protocol unit tests passed with AddressSanitizer and UndefinedBehaviorSanitizer.
- Native driver compiled and linked against the NUT revision in the protocol notes.
- Driver `-h` smoke check passed without opening the serial port.
- Read-only I/F/Q capture and native NUT `-d 1` hardware dump passed as user rodri.
- Mains-to-battery transition passed: input fell to zero, output remained 115 V, status changed to OB.
- Return to mains passed: the driver returned to OL, with input/output near 117 V.
- Control commands remain untested and unimplemented.

The build uses `MAKE=/usr/bin/make` because this host exports a `MAKE` variable
pointing at its desktop application executable, which breaks recursive make.

Historical NUT dump fixtures retain the original `sms_gamer` driver name as
captured; the current driver and build target are named `sms_interactive`.
