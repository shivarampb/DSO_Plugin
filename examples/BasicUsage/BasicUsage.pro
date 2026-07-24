# =============================================================================
#  examples/BasicUsage - a minimal consumer built against the SDK.
#
#  Standalone build against the packaged deliverables:
#      qmake "INCLUDEPATH+=<deliverables>/include" "LIBS+=-L<deliverables>/lib -lScopeCore"
#  In-tree it links the sibling ../lib and ../include.
# =============================================================================
QT       += core
QT       -= gui
CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE  = app
TARGET    = BasicUsage

# Built standalone against the packaged deliverables; the output lands in the
# build directory. Override INCLUDEPATH/LIBS on the qmake line to point at
# deliverables/ (see docs/Integration_Guide.md and .github/workflows/ci.yml).
INCLUDEPATH += ../../include
LIBS += -L../../lib -lScopeCore

SOURCES += main.cpp
