@echo off
rem ==========================================================================
rem  package_deliverables.bat - Windows/MinGW build + assemble deliverables\.
rem
rem  deliverables\ = include + lib + plugins + bin + examples + docs
rem  Leak guard: abort if an internal header or model/mock source would ship.
rem
rem  Usage:  scripts\package_deliverables.bat [release^|debug]
rem ==========================================================================
setlocal enabledelayedexpansion
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=release

set ROOT=%~dp0..
set BUILD=%ROOT%\build-pkg
set OUT=%ROOT%\deliverables

echo == Scope packaging (%BUILD_TYPE%) ==

if exist "%BUILD%" rmdir /s /q "%BUILD%"
mkdir "%BUILD%"
pushd "%BUILD%"
qmake "CONFIG+=%BUILD_TYPE%" "%ROOT%\Scope.pro" || goto :fail
mingw32-make -j4 || goto :fail
popd

if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%OUT%\include" "%OUT%\lib" "%OUT%\plugins" "%OUT%\bin" "%OUT%\examples" "%OUT%\docs"

for %%H in (ScopeManager.h IScopePlugin.h ScopeError.h ScopeTypes.h VisaHelper.h visa.h) do (
    copy /y "%ROOT%\include\%%H" "%OUT%\include\" >nul
)
xcopy /y /e "%BUILD%\lib\*"     "%OUT%\lib\"     >nul 2>&1
xcopy /y /e "%BUILD%\plugins\*" "%OUT%\plugins\" >nul 2>&1
xcopy /y /e "%BUILD%\bin\*"     "%OUT%\bin\"     >nul 2>&1
xcopy /y /e "%ROOT%\examples\*" "%OUT%\examples\" >nul 2>&1
xcopy /y /e "%ROOT%\docs\*"     "%OUT%\docs\"    >nul 2>&1

rem ---- leak guard --------------------------------------------------------
set FAIL=0
if exist "%OUT%\include\S_ScopeLimits.h"      ( echo LEAK: S_ScopeLimits.h shipped & set FAIL=1 )
if exist "%OUT%\include\CMockScopeEmulator.h" ( echo LEAK: CMockScopeEmulator.h shipped & set FAIL=1 )
for /r "%OUT%\plugins" %%F in (*.cpp) do ( echo LEAK: source %%F & set FAIL=1 )

if "%FAIL%"=="1" goto :fail
echo == packaging OK -^> %OUT% ==
echo Reminder: run "objdump -p" on each plugin DLL to confirm exports.
exit /b 0

:fail
echo == packaging FAILED ==
exit /b 1
