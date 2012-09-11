CONFIG += console

QT += \
    network \
    sql

SOURCES += \
    src/main.cpp \
    src/server.cpp \
    src/connection.cpp \
    src/connectionthread.cpp

HEADERS += \
    include/texsampleserver.h \
    src/server.h \
    src/connection.h \
    src/connectionthread.h

TRANSLATIONS += \
    res/translations/texsample-server_ru.ts

unix:LIBS += -lbeqtcore -lbeqtnetwork
unix:INCLUDEPATH += "/usr/include/beqt"
win32:LIBS += -L"$$(systemdrive)/Program files/BeQt/lib" -lbeqtcore0 -lbeqtnetwork0
win32:INCLUDEPATH += "$$(systemdrive)/Program files/BeQt/include"

RESOURCES += \
    texsample-server.qrc
