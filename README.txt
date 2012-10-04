===============================================================================
 TeXSample Server
===============================================================================

Homepage: https://github.com/TeXSample-Team/TeXSample-Server

Author: TeXSample Team

License: See COPYING.txt

===============================================================================
 Requirements
===============================================================================

To build TeXSample Server you will need the same libraries and tools as for
building any other Qt-based project.
See: http://qt-project.org/resources/getting_started for details.

Having QMYSQL plugin is required.
See: http://qt-project.org/doc/qt-4.8/sql-driver.html#qmysql

You have to use Qt libraries version 4.8.x.
Further releases of BeQt may use Qt libraries 5.0.
The easiest way to get Qt is to download the SDK.

You will also need the BeQt libraries (extension of Qt) version 1.0.x.
See: https://github.com/the-dark-angel/BeQt for details.

===============================================================================
 Building and installing
===============================================================================

All you have to do is run the followong commands:

 * qmake
 * make
   or other make command
 * make install
   You may have to run this command as a superuser on UNIX-like systems.

You may also use Qt Creator. Just open the texsample-server.pro file, and build
the application (configuration must be set to "Release"). After that, use the
"make install" command.

You should pass "BEQT_DIR=path_to_beqt" parameter to qmake. Otherwise, the
default location will be used.

If you are using the MinGW compiler, you may also pass
"MINGW_DIR=path_to_mingw" parameter to qmake, so the MinGW libraries will be
copied to the application's install directory. Otherwise, the default location
will be used.

You may also pass "PREFIX=install_path" parameter to qmake to change the
install path. On UNIX-like systems, PREFIX defaults to "/usr". On Windows it
will be "C:\PROGRA~1\TeXSample-Server".
Warning: don't use spaces. Use short names instead. For example, use "PROGRA~1"
instead of "Program files".

Qt libraries are copied automaticaly to the application's install directory.

On UNIX-like systems the headers will be located in the "PREFIX/include/beqt"
directory. On Windows the headers will be located in the "PREFIX/include"
directory.
