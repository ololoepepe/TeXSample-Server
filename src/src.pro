TEMPLATE = app
TARGET = texsample-server

CONFIG += release

QT = core network sql
BEQT = core network

isEmpty(BEQT_PREFIX) {
    #TODO: Add MacOS support
    mac|unix {
        BEQT_PREFIX=/usr/share/beqt
    } else:win32 {
        BEQT_PREFIX=$$(systemdrive)/PROGRA~1/BeQt
    }
}
include($${BEQT_PREFIX}/depend.pri)

isEmpty(TSMP_PREFIX) {
    #TODO: Add MacOS support
    mac|unix {
        TSMP_PREFIX=/usr/share/texsample
    } else:win32 {
        TSMP_PREFIX=$$(systemdrive)/PROGRA~1/TeXSample
    }
}
include($${TSMP_PREFIX}/depend.pri)

SOURCES += \
    main.cpp \
    terminaliohandler.cpp \
    server.cpp \
    connection.cpp \
    database.cpp \
    sqlqueryresult.cpp \
    registrationserver.cpp \
    registrationconnection.cpp \
    global.cpp \
    storage.cpp

HEADERS += \
    terminaliohandler.h \
    server.h \
    connection.h \
    database.h \
    sqlqueryresult.h \
    registrationserver.h \
    registrationconnection.h \
    global.h \
    storage.h

TRANSLATIONS += \
    ../translations/texsample-server_ru.ts

#Gets a file name
#Returns the given file name.
#On Windows slash characters will be replaced by backslashes
defineReplace(nativeFileName) {
    fileName=$${1}
    win32:fileName=$$replace(fileName, "/", "\\")
    return($${fileName})
}

translationsTs=$$files($${PWD}/../translations/*.ts)
for(fileName, translationsTs) {
    system(lrelease $$nativeFileName($${fileName}))
}

contains(CONFIG, builtin_resources) {
    RESOURCES += \
        texsample_server.qrc \
        ../translations/texsample_server_translations.qrc
    DEFINES += BUILTIN_RESOURCES
}
