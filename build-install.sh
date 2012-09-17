#!/bin/sh
echo "Building..."
cd build
qmake CONFIG+="release" ../texsample-server.pro
make "$@"
echo "Building finished."
cd ..
./install.sh
exit 0
