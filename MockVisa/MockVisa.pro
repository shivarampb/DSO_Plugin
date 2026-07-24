# =============================================================================
#  MockVisa - a fake VISA library (libvisa) for hardware-free dev/CI.
#  Model plugins link -lvisa; on a real bench this is the vendor VISA instead.
# =============================================================================
QT       -= gui
QT       += core

TEMPLATE  = lib
CONFIG   += plugin c++11        # 'plugin' => clean libvisa.so (no version symlinks)

TARGET    = visa
DESTDIR   = ../lib

INCLUDEPATH += ../include ../Common

HEADERS += \
    CMockScopeEmulator.h

SOURCES += \
    CMockScopeEmulator.cpp \
    MockVisa.cpp
