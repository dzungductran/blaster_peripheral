#!/bin/bash
# Start file transfer service
hciconfig hci0 sspmode 0
hciconfig hci0 piscan
hciconfig -a
sdptool add --channel=22 SP
sdptool add OPUSH
sdptool add FTP
/usr/lib/bluez/test/simple-agent &
systemctl start obex
systemctl status obex
