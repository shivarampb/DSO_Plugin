# =============================================================================
#  ScopeCore - the framework core library (CScopeManager + ScopeError).
#  Mirrors ELoadCore.pro.
# =============================================================================
QT       -= gui
QT       += core

TEMPLATE  = lib
CONFIG   += c++11
DEFINES  += SCOPECORE_LIBRARY

TARGET    = ScopeCore
DESTDIR   = ../lib
INCLUDEPATH += ../include

HEADERS += \
    ../include/IScopePlugin.h \
    ../include/ScopeManager.h \
    ../include/ScopeError.h \
    ../include/ScopeTypes.h \
    ../include/VisaHelper.h \
    ../include/visa.h

SOURCES += \
    ScopeManager.cpp \
    ScopeError.cpp \
    VisaHelper.cpp
