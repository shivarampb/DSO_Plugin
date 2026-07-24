QT -= gui

# Rohde & Schwarz RTM3004 (RTM3000) oscilloscope

TEMPLATE = lib
CONFIG += plugin c++11

TARGET = PluginRTM3004
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
    RTM3004Plugin.h

SOURCES += \
    RTM3004Plugin.cpp
