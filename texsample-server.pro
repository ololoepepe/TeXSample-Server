CONFIG += console release

QT -= gui
QT += \
    network \
    sql

SOURCES += \
    src/main.cpp \
    src/userconnection.cpp \
    src/userworker.cpp \
    src/userserver.cpp \
    src/databaseinteractor.cpp

HEADERS += \
    include/texsampleserver.h \
    src/userconnection.h \
    src/userworker.h \
    src/userserver.h \
    src/databaseinteractor.h

TRANSLATIONS += \
    translations/texsample-server_ru.ts

unix:LIBS += -lbeqtcore -lbeqtnetwork
unix:INCLUDEPATH += "/usr/include/beqt"
win32:LIBS += -L"$$(systemdrive)/BeQt/lib" -lbeqtcore1 -lbeqtnetwork1
win32:INCLUDEPATH += "$$(systemdrive)/BeQt/include"

builddir = .build

MOC_DIR = $$builddir
OBJECTS_DIR = $$builddir
RCC_DIR = $$builddir

unix {
### Target ###
target.path = /usr/lib/texsample-server
INSTALLS = target
### Includes ###
includes.files = include/*.h
includes.path = /usr/include/texsample-server
INSTALLS += includes
### Translations ###
trans.files = translations/*.qm
trans.path = /usr/share/texsample-server/translations
INSTALLS += trans
### BeQt translations ###
beqttrans.files = /usr/share/beqt/translations/*.qm
beqttrans.path = /usr/share/texsample-server/translations
INSTALLS += beqttrans
### Unix sh ###
unixsh.files = unix-only/texsample-server.sh
unixsh.path = /usr/bin
INSTALLS += unixsh
}
win32 {
isEmpty(PREFIX) {
    PREFIX = $$(systemdrive)/TeXSample-Server
}
### Target ###
target.path = $$PREFIX
INSTALLS = target
### Includes ###
includes.files = include/*.h
includes.path = $$PREFIX/include
INSTALLS += includes
### Translations ###
trans.files = translations/*.qm
trans.path = $$PREFIX/translations
INSTALLS += trans
### BeQt translations ###
beqttrans.files = /usr/share/beqt/translations/*.qm
beqttrans.path = $$PREFIX/translations
INSTALLS += beqttrans
}
