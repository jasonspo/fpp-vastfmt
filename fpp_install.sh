#!/bin/bash

if [ -n "$(which g++-4.7)" ]; then
	CXX=g++-4.7
else
	CXX=g++
fi

pushd $(dirname $(which $0))

git submodule init
git submodule update
if [ "$CXX" == "g++" ]; then
	# We don't support C++11 and need an old version of vastfmt
	pushd vastfmt
	git reset --hard 91b76c154d967ba22c5dc25831de6b888065ff87
	popd
fi
make -C vastfmt CXX=$CXX
mkdir -p bin
cp vastfmt/radio bin/rds

popd
