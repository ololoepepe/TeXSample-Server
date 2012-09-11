#!/bin/sh
echo "Installing..."
mkdir -p /usr/include/texsample-server
mkdir -p /usr/lib/texsample-server/qt4/plugins
mkdir -p /usr/lib/texsample-server/other
mkdir -p /usr/lib/texsample-server/beqt
cp ./build/texsample-server /usr/lib/texsample-server
cp ./install/texsample-server.sh /usr/bin
cp ./include/* /usr/include/texsample-server
cp /usr/lib/libbeqtcore.so.0 /usr/lib/texsample-server/beqt
cp /usr/lib/libbeqtnetwork.so.0 /usr/lib/texsample-server/beqt
echo "Installation finished."
exit 0
