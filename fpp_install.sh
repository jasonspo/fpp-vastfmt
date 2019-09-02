#!/bin/bash

pushd $(dirname $(which $0))

# Before building, check that we have libusb-1.0-0-dev
if ! dpkg --get-selections | grep -cq libusb-1.0-0-dev; then
	sudo apt-get -q -y update
	sudo apt-get -q -y --reinstall install libusb-1.0-0-dev
fi

popd
