# =============================================================================
#  AutoTest - console end-to-end suite (exit code = number of failures).
#  Zero hardware: SimScope needs no VISA, real plugins route to MockVisa.
# =============================================================================
QT       += core
QT       -= gui
CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE  = app
TARGET    = AutoTest
DESTDIR   = ../bin

INCLUDEPATH += ../include ../Common

LIBS += -L../lib -lScopeCore -lvisa

SOURCES += \
    main.cpp
