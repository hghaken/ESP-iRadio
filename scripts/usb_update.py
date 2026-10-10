# Sends a firmware image to the RUNNING radio over USB (see src/serialupdate.h for the protocol);
# the radio shows the progress bar on its display and restarts into the new image.
#   python scripts/usb_update.py <firmware.bin> [COM7]
# Exit codes: 0 done, 2 the firmware did not answer (use esptool instead), 1 other failure.
import hashlib
import sys
import time

import serial

CHUNK = 1024


def read_line(ser, deadline):
    buf = b''
    while time.time() < deadline:
        c = ser.read(1)
        if not c:
            continue
        if c == b'\n':
            return buf.decode('ascii', 'replace').strip()
        buf += c
    return None


def wait_for(ser, prefix, seconds):
    deadline = time.time() + seconds
    while time.time() < deadline:
        line = read_line(ser, deadline)
        if line is None:
            return None
        if line.startswith('#E'):
            raise RuntimeError('radio: ' + line[2:].strip())
        if line.startswith(prefix):
            return line
    return None


def main(path, port):
    data = open(path, 'rb').read()
    md5 = hashlib.md5(data).hexdigest()
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = port, 115200, 0.2
    ser.dtr = False  # opening the port must not reset the board (auto-program circuit)
    ser.rts = False
    ser.open()
    try:
        ser.reset_input_buffer()
        ser.write(b'\nIRADIO-UPDATE %d %s\n' % (len(data), md5.encode()))
        ready = wait_for(ser, '#R', 5)
        if not ready:
            print('the firmware did not answer on %s' % port)
            return 2
        ser.baudrate = int(ready.split()[1])
        t0 = time.time()
        done = 0
        while done < len(data):
            chunk = data[done:done + CHUNK]
            ser.write(chunk)
            ack = wait_for(ser, '#K', 15)
            if not ack:
                print('\nno acknowledgement after %d bytes' % done)
                return 1
            done = int(ack.split()[1])
            pct = done * 100 // len(data)
            sys.stdout.write('\r%3d %%  %d / %d KB' % (pct, done // 1024, len(data) // 1024))
            sys.stdout.flush()
        if not wait_for(ser, '#D', 30):
            print('\nno confirmation from the radio')
            return 1
        print('\ndone in %.1f s, the radio restarts' % (time.time() - t0))
        return 0
    except RuntimeError as e:
        print('\n%s' % e)
        return 1
    finally:
        ser.close()


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit('usage: usb_update.py <firmware.bin> [COM7]')
    sys.exit(main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else 'COM7'))
