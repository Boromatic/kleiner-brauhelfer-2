@echo off

set QT_DIR=%1
set PORTABLE=%2
set DEPLOY=%3

if "%QT_DIR%" equ "" (
  echo Usage: build_win32.cmd ^<qt-bin-directory^> [portable] [deploy]
  exit /b 1
)

if not exist "%QT_DIR%\qmake.exe" (
  echo Error: qmake.exe not found in ^<qt-bin-directory^>
  exit /b 1
)

call "%PROGRAMFILES(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" > nul 2>&1

set PRO=%~dp0kleiner-brauhelfer-2.pro
set BUILD_DIR=%~dp0build-win32

mkdir %BUILD_DIR%
cd %BUILD_DIR%
"%QT_DIR%\qmake.exe" "%PRO%" -config release
nmake
cd %~dp0

xcopy "%~dp0deployment\win\32bit\libcrypto-1_1.dll" "%BUILD_DIR%\bin" /Y
xcopy "%~dp0deployment\win\32bit\libssl-1_1.dll" "%BUILD_DIR%\bin" /Y

if "%PORTABLE%" equ "1" (
  echo. 2>"%BUILD_DIR%\bin\portable"
)

if "%DEPLOY%" equ "1" (
  "%~dp0deployment\win\32bit\deploy.cmd" "%BUILD_DIR%\bin" "%QT_DIR%"
)
