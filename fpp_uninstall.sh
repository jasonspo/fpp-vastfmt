#!/bin/bash

pushd $(dirname $(which $0))

echo -n "Uninstalling..."
sleep 1
sed -i 's/defaults.ctl.card\ [0-9]/defaults.ctl.card 0/;s/defaults.pcm.card\ [0-9]/defaults.pcm.card 0/' /usr/share/alsa/alsa.conf
sleep 1
echo "done"

popd
