@echo off

set old_dir=%cd%

cd %~dp0

REM create build directory
if not exist build (
	mkdir build
	if "%errorlevel%" NEQ "0" goto end
)

REM go to build
cd build
if "%errorlevel%" NEQ "0" goto end

REM cmake
set flagdebug="off"
if "%1%"=="debug" set flagdebug="on"
if "%2%"=="debug" set flagdebug="on"
if %flagdebug%=="on" (
	set flagdebug="-DCMAKE_BUILD_TYPE=Debug"
) else (
	set flagdebug="-DCMAKE_BUILD_TYPE=Release"
)

echo 'cmake .. -G "Ninja" %flagdebug%
cmake .. -G "Ninja" %flagdebug%
if "%errorlevel%" NEQ "0" goto end

REM make
ninja
if "%errorlevel%" NEQ "0" goto end

REM install
mkdir bin
copy src\ibrio\ibrio.exe bin\
copy src\ibrio\*.dll bin\

echo %%~dp0\ibrio.exe console %%* > bin\ibrio-cli.bat
echo %%~dp0\ibrio.exe -daemon %%* > bin\ibrio-server.bat

echo 'Installed to build\bin\'
echo ''
echo 'Usage:'
echo 'Run build\bin\ibrio.exe to launch ibrio'
echo 'Run build\bin\ibrio-cli.bat to launch ibrio RPC console'
echo 'Run build\bin\ibrio-server.bat to launch ibrio server on background'
echo 'Run build\test\test_big.exe to launch test program.'
echo 'Default `-datadir` is the same folder of ibrio.exe'

echo 'Installed to build\bin\'
echo ''
echo 'Usage:'
echo 'Run build\bin\ibrio.exe to launch ibrio'
echo 'Run build\bin\ibrio-console.bat to launch ibrio-console'
echo 'Run build\bin\ibrio-server.bat to launch ibrio server on background'
echo 'Run build\test\test_big.exe to launch test program.'

:end

cd %old_dir%
