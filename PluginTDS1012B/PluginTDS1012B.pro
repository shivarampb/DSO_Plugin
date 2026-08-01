QT -= gui

# Tektronix TDS1012B (3 Series MDO) oscilloscope

TEMPLATE = lib
CONFIG += plugin c++11

# Export only the Qt plugin entry points (no internal symbol leakage).
unix: QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden

TARGET = PluginTDS1012B
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
    TDS1012BPlugin.h

SOURCES += \
    TDS1012BPlugin.cpp
