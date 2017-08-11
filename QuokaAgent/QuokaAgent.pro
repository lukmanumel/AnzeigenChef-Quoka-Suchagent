QT       += core network

TARGET = QuokaAgent
TEMPLATE = lib
CONFIG += plugin

# This Part is only for debugging on our development
CONFIG(release, debug|release) {
    macx{
        DESTDIR = $$PWD/../build/release/macos
    }

    win32{
        DESTDIR = $$PWD\\..\\build\\release\\windows
    }

    linux{
        DESTDIR = $$PWD/../build/release/linux
    }
}

CONFIG(debug, debug|release) {
    macx{
        DESTDIR = $$PWD/../build/debug/macos
    }

    win32{
        DESTDIR = $$PWD\\..\\build\\debug\\windows
    }

    linux{
        DESTDIR = $$PWD/../build/debug/linux
    }
}


DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        quokaagentplugin.cpp

HEADERS += \
        quokaagentplugin.h \
        searchagentinterface.h

DISTFILES += QuokaAgent.json
