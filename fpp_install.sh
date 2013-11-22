#!/bin/sh

git submodule init
make -C vastfmt
mkdir -p bin
cp vastfmt/radio bin/rds
