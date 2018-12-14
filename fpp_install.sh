#!/bin/bash

pushd $(dirname $(which $0))

git submodule init
git submodule update

# Before building, check that we have libusb-1.0-0-dev
if ! dpkg --get-selections | grep -cq libusb-1.0-0-dev; then
	sudo apt-get -q -y update
	sudo apt-get -q -y --reinstall install libusb-1.0-0-dev
fi

make -C vastfmt

mkdir -p bin
cp vastfmt/radio bin/rds

if ! dpkg --get-selections | grep -cq "^jq"; then
	sudo apt-get -q -y update
	sudo apt-get -q -y install jq
fi

popd
