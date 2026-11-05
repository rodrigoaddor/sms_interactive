#!/usr/bin/env python3
"""One read-only I/F/Q query sequence; stop on the first communication error."""
import argparse, array, fcntl, json, os, select, termios, time
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('port', nargs='?', default='/dev/ttyUSB0')
a=p.parse_args()
fd=None
try:
    fd=os.open(a.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    fcntl.ioctl(fd, termios.TIOCEXCL)
    old=termios.tcgetattr(fd)
    cfg=termios.tcgetattr(fd)
    cfg[0]=cfg[1]=cfg[3]=0
    cfg[2]=termios.CS8 | termios.CLOCAL | termios.CREAD
    cfg[4]=cfg[5]=termios.B2400
    cfg[6][termios.VMIN]=0; cfg[6][termios.VTIME]=0
    termios.tcsetattr(fd, termios.TCSANOW, cfg)
    fcntl.ioctl(fd, termios.TIOCMBIS, array.array('i', [termios.TIOCM_RTS]))
    fcntl.ioctl(fd, termios.TIOCMBIC, array.array('i', [termios.TIOCM_DTR]))
    for cmd in b'IFQ':
        termios.tcflush(fd, termios.TCIOFLUSH)
        request=bytes([cmd,255,255,255,255,(4-cmd)&255,13])
        if os.write(fd, request) != 7: raise OSError('short write')
        reply=b''; deadline=time.monotonic()+3
        while len(reply)<18 and time.monotonic()<deadline:
            if select.select([fd],[],[], max(0,deadline-time.monotonic()))[0]:
                part=os.read(fd,18-len(reply))
                if not part: raise OSError('serial device disconnected')
                reply+=part
        print(json.dumps(dict(command=chr(cmd), request=request.hex(), response=reply.hex())), flush=True)
        if len(reply)!=18 or reply[-1]!=13 or sum(reply[:-1])&255:
            raise OSError('invalid/short response; stop and check USB/IP before retrying')
except OSError as e:
    p.exit(1, str(e)+'\n')
finally:
    if fd is not None:
        if 'old' in locals(): termios.tcsetattr(fd, termios.TCSANOW, old)
        os.close(fd)
