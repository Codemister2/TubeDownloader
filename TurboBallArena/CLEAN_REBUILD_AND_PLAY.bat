@echo off
setlocal
cd /d "%~dp0"
echo Removing old build cache...
if exist build rmdir /s /q build
call AUTO_INSTALL_AND_PLAY.bat
