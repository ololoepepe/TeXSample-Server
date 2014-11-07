TEMPLATE = app
TARGET = texsample-server

CONFIG += console release

BEQT = core network sql
TSMP = core

isEmpty(BEQT_PREFIX) {
    mac|unix {
        BEQT_PREFIX=/usr
    } else:win32 {
        BEQT_PREFIX=$$(systemdrive)/PROGRA~1/BeQt
    }
}
include($${BEQT_PREFIX}/share/beqt/depend.pri)

isEmpty(TSMP_PREFIX) {
    mac|unix {
        TSMP_PREFIX=/usr
    } else:win32 {
        TSMP_PREFIX=$$(systemdrive)/PROGRA~1/TeXSample
    }
}
include($${TSMP_PREFIX}/share/texsample/depend.pri)

SOURCES += \
    application.cpp \
    authorityinfoprovider.cpp \
    connection.cpp \
    datasource.cpp \
    main.cpp \
    server.cpp \
    settings.cpp \
    temporarylocation.cpp \
    transactionholder.cpp \
    translator.cpp \
    authorityinfo.cpp \
    authorityresolver.cpp

HEADERS += \
    application.h \
    authorityinfoprovider.h \
    connection.h \
    datasource.h \
    server.h \
    settings.h \
    temporarylocation.h \
    transactionholder.h \
    translator.h \
    authorityinfo.h \
    authorityresolver.h

include(entity/entity.pri)
include(repository/repository.pri)
include(service/service.pri)

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

mac|unix {
    isEmpty(PREFIX):PREFIX=/usr
} else:win32 {
    isEmpty(PREFIX):PREFIX=$$(systemdrive)/PROGRA~1/TeXSample-Server
}

isEmpty(BINARY_INSTALLS_PATH):BINARY_INSTALLS_PATH=$${PREFIX}/bin
isEmpty(RESOURCES_INSTALLS_PATH):RESOURCES_INSTALLS_PATH=$${PREFIX}/share/texsample-server

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
    installsAuthDefConf.files=$$files($${PWD}/auth-def.properties)
    installsAuthDefConf.path=$${RESOURCES_INSTALLS_PATH}
    INSTALLS += installsAuthDefConf
}

} #end !contains(TSRV_CONFIG, no_install)
