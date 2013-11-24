#!/bin/bash

pushd $(dirname $(which $0))

git submodule init
git submodule update
make -C vastfmt
mkdir -p bin
cp vastfmt/radio bin/rds

popd
