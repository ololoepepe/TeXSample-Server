CONFIG += console

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

RESOURCES += \
    texsample-server.qrc

TRANSLATIONS += \
    res/translations/texsample-server_ru.ts

unix:LIBS += -lbeqtcore -lbeqtnetwork
unix:INCLUDEPATH += "/usr/include/beqt"
win32:LIBS += -L"$$(systemdrive)/Program files/BeQt/lib" -lbeqtcore1 -lbeqtnetwork1
win32:INCLUDEPATH += "$$(systemdrive)/Program files/BeQt/include"
