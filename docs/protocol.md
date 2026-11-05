# Protocol evidence and open questions

Static inspection only: the PowerView application and its classes were never executed.
`javap -c -p` disassembled the serial library. No vendor code is linked into the driver.

Sources in the supplied installation:

- `powerview.jar!/protocolos/monofasico.xml`: I, F, Q query layouts and response fields.
- `Comunicacao-1.9.jar`, `PortaSerial`: 2400 baud, 8N1, RTS asserted, DTR cleared.
- `UtilSerial`: binary transmission, CR terminator.
- Upstream NUT `drivers/sms_ser.c`, inspected at commit
  `1c4ab78c615a7b2b8231bdb023d553abd64b541d`, corroborates frame layout.

Requests are seven bytes: command, ff ff ff ff, negative sum of the
preceding five bytes modulo 256, 0d. Only I/F/Q are allowed by our encoder.
Responses are expected to be 18 bytes, including checksum and CR. The
negative-sum response checksum rule was confirmed on I, F, and Q hardware replies.
Read fixed lengths: CR may occur inside binary payloads.

Q responses: `<`, `=`, or `>`; seven big endian 16-bit quantities;
one status byte; checksum; CR. Quantities are last input voltage, input voltage,
output voltage, output load, output frequency, battery indication, temperature.
All except the battery indication use tenths according to PowerView XML.
PowerView leaves battery indication unscaled, whereas upstream sms_ser divides
it by ten. Our driver preserves the raw value and does not invent battery charge,
voltage, or runtime until this discrepancy is resolved on the Gamer.

Status bits from least significant to most: beeper enabled, shutdown pending,
test active, UPS OK, boost, bypass, low battery, on battery. XML indexes refer
to positions in a binary string; upstream sms_ser corroborates this bit order.
UPS fault is exposed as an alarm; it does not establish battery replacement.

I: header, 12 model characters, 3 firmware characters, checksum, CR.
F: header, 7 voltage-range characters, 3 current-range characters,
3 nominal battery voltage characters, 2 frequency characters, checksum, CR.
F is collected by the probe but not published by the initial driver.

Basic monitoring on Gamer 2000 VA firmware 1.4 is confirmed below;
compatibility with other models remains unverified.
Other PowerView protocols (Voltronic, Upsilon, three-phase) are distinct and
must not be assumed compatible. No automatic protocol scanning is performed.

SHA256 of inspected files:

- powerview.jar: f667629ccd2044065ef56c4375d55a0abac273fbe2f330e0adda04229fc142b7
- Comunicacao-1.9.jar: 1fdeb3b5f68523f72b8f2f42a1acb91bb628ea79e4b97ab9413a0133c1e62e2f

Next validation: resolve battery units and observe OB and return to OL with
the user's assistance. Low battery,
self-test, shutdown, and load controls have not been tested or enabled.

## Hardware validation on 2026-11-05

Non-root read-only I/F/Q queries succeeded over USB/IP at `/dev/ttyUSB0`.
Model `GAM2000BiBiW`, firmware `1.4`. Raw replies are retained in
`tests/fixtures/gam2000bibiw-1.4-mains.json` and covered by parser tests.
The compiled NUT driver also completed a `-d 1` data dump successfully without
root. It reported `OL BYPASS`, voltages, frequency, temperature, and raw battery.
No communication errors occurred in these tests.

The initial Q capture decoded as 119.1 V input, 119.0 V output, 59.9 Hz,
40.9 degrees C, load 0.1, battery raw 290, status 0x28 (UPS OK + bypass).
Later voltage samples varied; physical voltage accuracy has not been measured.
F returns nominal battery text ` 24` and frequency text `60`.

Further static analysis of `br.com.sms.powerview.protocolo.a.a.a` shows that
PowerView divides the battery field by ten unless firmware contains `SE`.
It then uses that number with a configurable low-battery threshold and calls
it a battery percentage. This establishes software scaling, but does not prove
physical units on this firmware: 290 could plausibly encode 29.0 V for a 24 V
pack or another indication. Do not publish it as charge or voltage yet.

Battery-mode transition validated with the user disconnecting only mains:
Q reply `3d00000000047e0002025803e8019ba8b60d` reports input 0 V,
output 115.0 V, 60.0 Hz, battery raw 1000, and status 0xa8. The compiled
driver reports `OB BYPASS`. Thus bit 7 changes with loss of mains while bit 5
remains set in both modes; its physical meaning on this model needs further
investigation despite PowerView naming it bypass. Battery raw changed from
290 on mains to 1000 on battery; do not assume one simple voltage conversion.
Return to mains was also validated: Q reply
`3d0000046304620000025603b6019c28200d` has status 0x28 again, battery raw
950, and normal input/output. A subsequent compiled-driver sample reported
117.4 V input and 117.3 V output with `OL BYPASS`. Voltage varies between
queries. Both raw captures and native NUT dumps are retained under
`tests/fixtures/`. All captured frames passed checksums.

The battery field transitions 290 -> 1000 -> 950. PowerView's scaling would
produce 29.0 -> 100.0 -> 95.0. Its physical meaning and initial low value
remain unresolved; the driver deliberately continues exposing the raw field.
No self-test, beeper control, output switching, or shutdown commands were sent.
