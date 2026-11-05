#!/usr/bin/env python3
"""Add this driver to a NUT source tree (idempotently)."""
import pathlib, shutil, sys
root=pathlib.Path(__file__).resolve().parents[1]
nut=pathlib.Path(sys.argv[1]).resolve()
for name in ('sms_interactive.c','sms_interactive_protocol.h'):
    shutil.copy2(root/'drivers'/name,nut/'drivers'/name)
p=nut/'drivers/Makefile.am'
s=p.read_text()
if 'sms_interactive_SOURCES' not in s:
    s=s.replace('sms_ser bicker_ser', 'sms_ser sms_interactive bicker_ser')
    s+='\nsms_interactive_SOURCES = sms_interactive.c sms_interactive_protocol.h\nsms_interactive_LDADD = $(LDADD_DRIVERS_SERIAL)\n'
p.write_text(s)
