QT       += core network

TARGET = QuokaAgent
TEMPLATE = lib
CONFIG += plugin

macx{
    DESTDIR = $$PWD/../build/release/macos
}

win32{
    DESTDIR = $$PWD\\..\\build\\release\\windows
}

linux{
    DESTDIR = $$PWD/../build/release/linux
}


DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        quokaagentplugin.cpp

HEADERS += \
        quokaagentplugin.h \
        searchagentinterface.h

DISTFILES += QuokaAgent.json
