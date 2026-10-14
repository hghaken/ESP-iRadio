// Firmware update over the USB serial port, done by the running firmware (like the WiFi
// update): the display shows the progress bar. The PC side is scripts/usb_update.py (called by
// scripts/flash_usb.ps1, which falls back to esptool when the firmware does not answer).
//
// Protocol on UART0 (115200 baud, switched to 921600 for the data):
//   PC -> radio  "IRADIO-UPDATE <size> <md5>\n"
//   radio -> PC  "#R 921600\n"            then both sides switch the baud rate
//   PC -> radio  1024-byte chunks, one at a time (stop-and-wait: nothing arrives while the radio
//                writes flash, when the UART interrupt can be held off)
//   radio -> PC  "#K <bytes so far>\n"     after each chunk is written (or, after 1 s
//                without the rest of a chunk, the old count again: the PC then resends it)
//   radio -> PC  "#D\n" (then restart) or "#E <reason>\n" (back to 115200, playback resumes)
// Other output on the port (log lines) is ignored by the PC side; the radio's own log lines to
// the serial port are muted during the transfer.
#pragma once

void serialUpdateLoop();  // call from loop(): watches the serial port for the update command
