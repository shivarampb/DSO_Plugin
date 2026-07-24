# =============================================================================
#  Scope.pro - top-level build for the Scope oscilloscope framework.
#
#  Mirrors the ELoad framework: a core library (CScopeManager) plus
#  runtime-discovered, self-contained per-model plugin DLLs, a VISA library
#  (MockVisa here; a real NI-VISA on a bench) and the tester apps.
#
#  Recommended (shadow) build:
#      mkdir build && cd build && qmake ../Scope.pro && make -j
#  Outputs land in build/lib, build/plugins and build/bin.
# =============================================================================
TEMPLATE = subdirs
CONFIG  += ordered

SUBDIRS += \
    ScopeCore \
    MockVisa \
    PluginMDO34 \
    PluginRTM3004 \
    PluginSimScope \
    AutoTest \
    GuiTester

MockVisa.depends        = ScopeCore
PluginMDO34.depends     = ScopeCore MockVisa
PluginRTM3004.depends   = ScopeCore MockVisa
PluginSimScope.depends  = ScopeCore
AutoTest.depends        = ScopeCore MockVisa PluginMDO34 PluginRTM3004 PluginSimScope
GuiTester.depends       = ScopeCore
