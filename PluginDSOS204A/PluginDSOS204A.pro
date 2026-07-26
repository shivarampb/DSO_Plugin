QT -= gui

# Keysight (Agilent) DSOS204A InfiniiVision 2000 X-Series oscilloscope

TEMPLATE = lib
CONFIG += plugin c++11

# Export only the Qt plugin entry points (no internal symbol leakage).
unix: QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden

TARGET = PluginDSOS204A
DESTDIR = ../plugins

INCLUDEPATH += ../include ../Common

LIBS += -L../lib -lScopeCore

# VISA library
win32 {
    INCLUDEPATH += "C:/Program Files/IVI Foundation/VISA/Win64/Include"
    LIBS += -L"C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc" -lvisa64
}
unix {
    LIBS += -L../lib -lvisa
}

HEADERS += \
    DSOS204APlugin.h

SOURCES += \
    DSOS204APlugin.cpp
