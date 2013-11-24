#!/bin/bash

pushd $(dirname $(which $0))

echo -n "Uninstalling..."
sleep 1
echo "done"

popd
