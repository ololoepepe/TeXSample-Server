TEMPLATE = app
TARGET = texsample-server

CONFIG += release

QT = core network sql
BEQT = core network

include(../beqt/depend.pri)

SOURCES += \
    main.cpp \
    terminaliohandler.cpp \
    server.cpp \
    connection.cpp

HEADERS += \
    terminaliohandler.h \
    server.h \
    connection.h

TRANSLATIONS += \
    translations/texsample-server_ru.ts

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
