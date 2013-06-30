TEMPLATE = app
TARGET = texsample-server

CONFIG += console release

QT = core network sql
BEQT = core network sql

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
    storage.cpp \
    application.cpp

HEADERS += \
    terminaliohandler.h \
    server.h \
    connection.h \
    database.h \
    sqlqueryresult.h \
    registrationserver.h \
    registrationconnection.h \
    global.h \
    storage.h \
    application.h

TRANSLATIONS += \
    ../translations/texsample-server_ru.ts

##############################################################################
################################ Generating translations #####################
##############################################################################

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

contains(TSRV_CONFIG, builtin_resources) {
    RESOURCES += \
        texsample_server.qrc \
        ../translations/texsample_server_translations.qrc
    DEFINES += BUILTIN_RESOURCES
}

##############################################################################
################################ Installing ##################################
##############################################################################

!contains(TSRV_CONFIG, no_install) {

#mac {
    #isEmpty(PREFIX):PREFIX=/Library
    #TODO: Add ability to create bundles
#} else:unix:!mac {
#TODO: Add MacOS support
mac|unix {
    isEmpty(PREFIX):PREFIX=/usr
    equals(PREFIX, "/")|equals(PREFIX, "/usr")|equals(PREFIX, "/usr/local") {
        isEmpty(BINARY_INSTALLS_PATH):BINARY_INSTALLS_PATH=$${PREFIX}/bin
        isEmpty(RESOURCES_INSTALLS_PATH):RESOURCES_INSTALLS_PATH=$${PREFIX}/share/texsample-server
    } else {
        isEmpty(BINARY_INSTALLS_PATH):BINARY_INSTALLS_PATH=$${PREFIX}
        isEmpty(RESOURCES_INSTALLS_PATH):RESOURCES_INSTALLS_PATH=$${PREFIX}
    }
} else:win32 {
    isEmpty(PREFIX):PREFIX=$$(systemdrive)/PROGRA~1/TeXSample-Server
    isEmpty(BINARY_INSTALLS_PATH):BINARY_INSTALLS_PATH=$${PREFIX}
}

##############################################################################
################################ Binaries ####################################
##############################################################################

target.path = $${BINARY_INSTALLS_PATH}
INSTALLS = target

##############################################################################
################################ Translations ################################
##############################################################################

!contains(TSRV_CONFIG, builtin_resources) {
    installsTranslations.files=$$files($${PWD}/../translations/*.qm)
    installsTranslations.path=$${RESOURCES_INSTALLS_PATH}/translations
    INSTALLS += installsTranslations
}

##############################################################################
################################ Other resources #############################
##############################################################################

!contains(TSRV_CONFIG, builtin_resources) {
    installsDb.files=$$files($${PWD}/db/*)
    installsDb.path=$${RESOURCES_INSTALLS_PATH}/db
    INSTALLS += installsDb
    installsTsfv.files=$$files($${PWD}/texsample-framework/*.sty)
    installsTsfv.files+=$$files($${PWD}/texsample-framework/*.tex)
    installsTsfv.path=$${RESOURCES_INSTALLS_PATH}/texsample-framework
    INSTALLS += installsTsfv
}

} #end !contains(TSRV_CONFIG, no_install)
