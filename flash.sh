#!/bin/bash
dev_name=$1
file_name=$2

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 device_name file_name"
    echo "Example: $0 /dev/cu.wchusbserial1420 builds/airrohr-fw-0.2.0.bin"
    exit 1
fi

# Platform: macOS
esptool -vv -cd nodemcu -cb 57600 -ca 0x00000 -cp $dev_name -cf $file_name
