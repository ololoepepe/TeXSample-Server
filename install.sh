#!/bin/sh
echo "Installing..."
mkdir -p /usr/include/texsample-server
mkdir -p /usr/lib/texsample-server/qt4/plugins
mkdir -p /usr/lib/texsample-server/other
mkdir -p /usr/lib/texsample-server/beqt
mkdir -p /usr/share/texsample-server/translations
rm /usr/lib/texsample-server/texsample-server
rm /usr/lib/texsample-server/beqt/libbeqt*
rm /usr/include/texsample-server/*.h
rm /usr/bin/texsample-server.sh
cp ./build/texsample-server /usr/lib/texsample-server
cp -P /usr/lib/libbeqtcore.* /usr/lib/texsample-server/beqt
cp -P /usr/lib/libbeqtnetwork.* /usr/lib/texsample-server/beqt
cp /usr/share/beqt/translations/*.qm /usr/share/texsample-server/translations
cp ./include/*.h /usr/include/texsample-server
cp ./install/texsample-server.sh /usr/bin
cp ./install/translations/*.qm /usr/share/texsample-server/translations
cp ./translations/*.qm /usr/share/texsample-server/translations
echo "Installation finished."
exit 0
