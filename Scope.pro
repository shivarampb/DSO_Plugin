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
    PluginDSO7104B \
    PluginDSOS204A \
    PluginDSOX2012A \
    PluginMSO6054A \
    PluginRTO2064 \
    PluginTDS1012B \
    PluginTDS2024C \
    PluginWaveSurfer42Xs \
    PluginSimScope \
    AutoTest \
    GuiTester

MockVisa.depends             = ScopeCore
PluginMDO34.depends          = ScopeCore MockVisa
PluginRTM3004.depends        = ScopeCore MockVisa
PluginDSO7104B.depends       = ScopeCore MockVisa
PluginDSOS204A.depends       = ScopeCore MockVisa
PluginDSOX2012A.depends      = ScopeCore MockVisa
PluginMSO6054A.depends       = ScopeCore MockVisa
PluginRTO2064.depends        = ScopeCore MockVisa
PluginTDS1012B.depends       = ScopeCore MockVisa
PluginTDS2024C.depends       = ScopeCore MockVisa
PluginWaveSurfer42Xs.depends = ScopeCore MockVisa
PluginSimScope.depends       = ScopeCore
AutoTest.depends             = ScopeCore MockVisa PluginMDO34 PluginRTM3004 PluginSimScope
GuiTester.depends            = ScopeCore
