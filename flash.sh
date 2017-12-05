#!/bin/bash
file_name=$1
# Platform: macOS
~/Library/Arduino15/packages/esp8266/tools/esptool/0.4.9/esptool -vv -cd nodemcu -cb 57600 -ca 0x00000 -cp /dev/cu.wchusbserial1420 -cf $file_name
