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

CONFIG += console release

builddir = .build

MOC_DIR = $$builddir
OBJECTS_DIR = $$builddir
RCC_DIR = $$builddir

###############################################################################
# PREFIX and BEQT_DIR
###############################################################################

unix:isEmpty(PREFIX):PREFIX = /usr
win32:PREFIX = TeXSample-Server

win32 {
isEmpty(BEQT_DIR) {
    BEQT_DIR = $$quote($$(systemdrive)/Program files/BeQt)
    warning(BeQt dir not specified; trying "$$BEQT_DIR")
}
isEmpty(MINGW_DIR) {
    MINGW_DIR = $$quote($$(systemdrive)/MinGW)
    warning(MinGW dir not specified; trying "$$MINGW_DIR")
}
}

###############################################################################
# LIBS and INCLUDEPATH
###############################################################################

unix {
LIBS += -lbeqtcore -lbeqtnetwork
INCLUDEPATH += $$PREFIX/include/beqt
}
win32 {
LIBS += -L"$$BEQT_DIR/lib" -lbeqtcore1 -lbeqtnetwork1
INCLUDEPATH += "$$BEQT_DIR/include"
}

###############################################################################
# INSTALLS.files
###############################################################################

I_HEADERS.files = include/*.h
I_TRANSLATIONS.files = translations/*.qm
unix:I_TRANSLATIONS.files += $$PREFIX/share/beqt/translations/*.qm
win32:I_TRANSLATIONS.files += $$BEQT_DIR/translations/*.qm
I_TRANSLATIONS.files += $$(QTDIR)/translations/qt_??.qm
I_TRANSLATIONS.files += $$(QTDIR)/translations/qt_??_??.qm
### unix-only ###
unix {
I_SCRIPTS.files = unix-only/texsample-server.sh
}
### win32-only ###
win32 {
I_LIBS.files += \
    $$BEQT_DIR/lib/beqtcore1.dll \
    $$BEQT_DIR/lib/beqtnetwork1.dll \
    $$(QTDIR)/lib/QtCore4.dll \
    $$(QTDIR)/lib/QtNetwork4.dll \
    $$(QTDIR)/lib/QtSql4.dll \
    $$MINGW_DIR/libgcc_s_dw2-1.dll \
    $$MINGW_DIR/mingwm10.dll
}

###############################################################################
# INSTALLS.path
###############################################################################

unix {
target.path = $$PREFIX/lib/texsample-server
I_HEADERS.path = $$PREFIX/include/texsample-server
I_TRANSLATIONS.path = $$PREFIX/share/texsample-server/translations
I_LIBS.path = $$PREFIX/lib/tex-creator
### unix-only ###
I_SCRIPTS.path = $$PREFIX/bin
}
win32 {
target.path = $$PREFIX
I_HEADERS.path = $$PREFIX/include
I_TRANSLATIONS.path = $$PREFIX/translations
I_LIBS.path = $$PREFIX
}

###############################################################################
# INSTALLS.extra
###############################################################################

### unix-only ###
unix {
I_LIBS.extra = \
    cp -P $$PREFIX/lib/libbeqtcore.so* $$PREFIX/lib/texsample-server; \
    cp -P $$PREFIX/lib/libbeqtnetwork.so* $$PREFIX/lib/texsample-server; \
    cp -P $$(QTDIR)/lib/libQtCore.so* $$PREFIX/lib/texsample-server; \
    cp -P $$(QTDIR)/lib/libQtNetwork.so* $$PREFIX/lib/texsample-server; \
    cp -P $$(QTDIR)/lib/libQtSql.so* $$PREFIX/lib/texsample-server
}

###############################################################################
# INSTALLS
###############################################################################

INSTALLS = target
INSTALLS += I_HEADERS
INSTALLS += I_TRANSLATIONS
INSTALLS += I_LIBS
### unix-only ###
unix {
INSTALLS += I_SCRIPTS
}
