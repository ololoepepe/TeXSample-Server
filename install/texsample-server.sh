#!/bin/sh
LD_LIBRARY_PATH="/usr/lib/texsample-server/qt4:/usr/lib/texsample-server/beqt:/usr/lib/texsample-server/other"
export LD_LIBRARY_PATH
/usr/lib/texsample-server/texsample-server "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"
