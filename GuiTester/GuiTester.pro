# =============================================================================
#  GuiTester - the "Scope Plugin Test" application. A dark-themed Qt Widgets
#  tester with a tab per function group (Connection / Vertical / Horizontal &
#  Trigger / Acquisition / Waveform / Measurements & Cursors / Save), driving
#  one instrument through the deliverable CScopeManager API. Plugins are
#  discovered at runtime. Mirrors the ELoad GuiTester.pro.
# =============================================================================
QT       += core gui widgets

TARGET    = GuiTester
TEMPLATE  = app
CONFIG   += c++11

DESTDIR   = $$OUT_PWD/../bin

INCLUDEPATH += ../include

LIBS += -L$$OUT_PWD/../lib -lScopeCore
unix: QMAKE_RPATHDIR += $$OUT_PWD/../lib

HEADERS += \
    TesterCommon.h \
    ScopeTesterWindow.h \
    WaveformPlot.h \
    ConnectionTab.h \
    VerticalTab.h \
    HorizontalTriggerTab.h \
    AcquisitionTab.h \
    WaveformTab.h \
    MeasurementCursorTab.h \
    SaveTab.h

SOURCES += \
    main.cpp \
    ScopeTesterWindow.cpp \
    WaveformPlot.cpp \
    ConnectionTab.cpp \
    VerticalTab.cpp \
    HorizontalTriggerTab.cpp \
    AcquisitionTab.cpp \
    WaveformTab.cpp \
    MeasurementCursorTab.cpp \
    SaveTab.cpp
