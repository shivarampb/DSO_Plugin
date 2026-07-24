# =============================================================================
#  GuiTester - dark-themed tabbed "Scope Plugin Test" application (Qt Widgets).
# =============================================================================
QT       += core gui widgets
CONFIG   += c++11
CONFIG   -= app_bundle

TEMPLATE  = app
TARGET    = GuiTester
DESTDIR   = ../bin

INCLUDEPATH += ../include ../Common

LIBS += -L../lib -lScopeCore

HEADERS += \
    ScopeTesterWindow.h \
    WaveformPlot.h \
    TesterCommon.h

SOURCES += \
    main.cpp \
    ScopeTesterWindow.cpp \
    WaveformPlot.cpp
