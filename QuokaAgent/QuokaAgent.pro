QT       += core network

TARGET = QuokaAgent
TEMPLATE = lib
CONFIG += plugin

DESTDIR = $PWD/../build

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        quokaagentplugin.cpp

HEADERS += \
        quokaagentplugin.h \
        searchagentinterface.h

DISTFILES += QuokaAgent.json 

unix {
    target.path = /usr/lib
    INSTALLS += target
}
