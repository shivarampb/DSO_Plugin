# =============================================================================
#  CMakeLists.txt - CMake build for the Scope oscilloscope framework, provided
#  alongside the qmake build (Scope.pro). Mirrors the same layout: a core
#  library, a MockVisa (libvisa), self-contained per-model plugins, SimScope,
#  and the AutoTest / GuiTester apps.
#
#      cmake -S . -B build-cmake && cmake --build build-cmake -j
#      LD_LIBRARY_PATH=build-cmake/lib build-cmake/bin/AutoTest build-cmake/plugins
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(Scope VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# Output layout matches the qmake shadow build (lib / plugins / bin).
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(PLUGIN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/plugins)

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/Common)

# ---- core library ----------------------------------------------------------
# (headers with Q_OBJECT are listed so AUTOMOC generates their moc)
add_library(ScopeCore SHARED
    ScopeCore/ScopeManager.cpp
    ScopeCore/ScopeError.cpp
    ScopeCore/VisaHelper.cpp
    include/ScopeManager.h)
target_compile_definitions(ScopeCore PRIVATE SCOPECORE_LIBRARY)
target_link_libraries(ScopeCore PUBLIC Qt5::Core)

# ---- MockVisa (built as libvisa) -------------------------------------------
add_library(visa SHARED
    MockVisa/MockVisa.cpp
    MockVisa/CMockScopeEmulator.cpp)
target_link_libraries(visa PRIVATE Qt5::Core)

# ---- helper to declare a self-contained model plugin -----------------------
function(add_scope_plugin name dir src)
    string(REPLACE ".cpp" ".h" hdr ${src})
    add_library(${name} MODULE ${dir}/${src} ${dir}/${hdr})
    set_target_properties(${name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIR}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    target_link_libraries(${name} PRIVATE Qt5::Core ScopeCore ${ARGN})
endfunction()

# real model plugins (link visa)
add_scope_plugin(PluginMDO34          PluginMDO34          MDO34Plugin.cpp          visa)
add_scope_plugin(PluginRTM3004        PluginRTM3004        RTM3004Plugin.cpp        visa)
add_scope_plugin(PluginDSO7104B       PluginDSO7104B       DSO7104BPlugin.cpp       visa)
add_scope_plugin(PluginDSOS204A       PluginDSOS204A       DSOS204APlugin.cpp       visa)
add_scope_plugin(PluginDSOX2012A      PluginDSOX2012A      DSOX2012APlugin.cpp      visa)
add_scope_plugin(PluginMSO6054A       PluginMSO6054A       MSO6054APlugin.cpp       visa)
add_scope_plugin(PluginRTO2064        PluginRTO2064        RTO2064Plugin.cpp        visa)
add_scope_plugin(PluginTDS1012B       PluginTDS1012B       TDS1012BPlugin.cpp       visa)
add_scope_plugin(PluginTDS2024C       PluginTDS2024C       TDS2024CPlugin.cpp       visa)
add_scope_plugin(PluginWaveSurfer42Xs PluginWaveSurfer42Xs WaveSurfer42XsPlugin.cpp visa)
# virtual model (no visa)
add_scope_plugin(PluginSimScope       PluginSimScope       SimScopePlugin.cpp)

# ---- testers ---------------------------------------------------------------
add_executable(AutoTest AutoTest/main.cpp)
target_link_libraries(AutoTest PRIVATE Qt5::Core ScopeCore visa)

add_executable(GuiTester
    GuiTester/main.cpp
    GuiTester/ScopeTesterWindow.cpp
    GuiTester/WaveformPlot.cpp
    GuiTester/ConnectionTab.cpp
    GuiTester/VerticalTab.cpp
    GuiTester/HorizontalTriggerTab.cpp
    GuiTester/AcquisitionTab.cpp
    GuiTester/WaveformTab.cpp
    GuiTester/MeasurementCursorTab.cpp
    GuiTester/SaveTab.cpp
    GuiTester/ScopeTesterWindow.h
    GuiTester/WaveformPlot.h
    GuiTester/ConnectionTab.h
    GuiTester/VerticalTab.h
    GuiTester/HorizontalTriggerTab.h
    GuiTester/AcquisitionTab.h
    GuiTester/WaveformTab.h
    GuiTester/MeasurementCursorTab.h
    GuiTester/SaveTab.h)
target_link_libraries(GuiTester PRIVATE Qt5::Core Qt5::Widgets ScopeCore)
