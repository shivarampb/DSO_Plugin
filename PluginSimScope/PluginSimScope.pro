QT -= gui

# SimScope - virtual oscilloscope (no hardware, no VISA)

TEMPLATE = lib
CONFIG += plugin c++11

TARGET = PluginSimScope
DESTDIR = ../plugins

INCLUDEPATH += ../include ../Common

LIBS += -L../lib -lScopeCore

HEADERS += \
    SimScopePlugin.h

SOURCES += \
    SimScopePlugin.cpp
