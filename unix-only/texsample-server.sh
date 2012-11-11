#!/bin/sh
export LD_LIBRARY_PATH="/usr/lib/texsample-server"
exec /usr/lib/texsample-server/texsample-server "$@"
