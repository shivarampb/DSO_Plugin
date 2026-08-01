QT -= gui

# SimScope - virtual oscilloscope (no hardware, no VISA)

TEMPLATE = lib
CONFIG += plugin c++11

# Export only the Qt plugin entry points (no internal symbol leakage).
unix: QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden

TARGET = PluginSimScope
DESTDIR = ../plugins

INCLUDEPATH += ../include ../Common

LIBS += -L../lib -lScopeCore

HEADERS += \
    SimScopePlugin.h

SOURCES += \
    SimScopePlugin.cpp
