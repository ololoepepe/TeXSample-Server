#!/bin/sh
export LD_LIBRARY_PATH="/usr/lib/texsample-server/qt4:/usr/lib/texsample-server/beqt:/usr/lib/texsample-server/other"
exec /usr/lib/texsample-server/texsample-server "$@"
