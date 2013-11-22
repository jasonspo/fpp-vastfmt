#!/bin/sh

pushd $(dirname $(which $0))

git submodule init
make -C vastfmt
mkdir -p bin
cp vastfmt/radio bin/rds

popd
