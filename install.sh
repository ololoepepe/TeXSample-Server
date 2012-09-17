#!/bin/sh
echo "Installing..."
mkdir -p /usr/include/texsample-server
mkdir -p /usr/lib/texsample-server/qt4/plugins
mkdir -p /usr/lib/texsample-server/other
mkdir -p /usr/lib/texsample-server/beqt
rm /usr/lib/texsample-server/texsample-server
rm /usr/lib/texsample-server/beqt/libbeqt*
rm /usr/include/texsample-server/*.h
rm /usr/bin/texsample-server.sh
cp ./build/texsample-server /usr/lib/texsample-server
cp -P /usr/lib/libbeqtcore.* /usr/lib/texsample-server/beqt
cp -P /usr/lib/libbeqtnetwork.* /usr/lib/texsample-server/beqt
cp ./include/*.h /usr/include/texsample-server
cp ./install/texsample-server.sh /usr/bin
echo "Installation finished."
exit 0
