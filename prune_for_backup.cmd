@echo off
setlocal enabledelayedexpansion
REM
REM First: Select "Projekt -> Bereinigen" in MSVC.
REM
SET CURRENT_PATH=%~dps0
CD /D "%CURRENT_PATH%"
REM
RD /S /Q "%CURRENT_PATH%.vs" 2> NUL:
REM
RD /S /Q "%CURRENT_PATH%Debug" 2> NUL:
MD "%CURRENT_PATH%Debug" 2> NUL:
echo. 2> "%CURRENT_PATH%Debug\.gitkeep"
REM 
RD /S /Q "%CURRENT_PATH%Release" 2> NUL:
MD "%CURRENT_PATH%Release" 2> NUL:
echo. 2> "%CURRENT_PATH%Release\.gitkeep"
REM
RD /S /Q "%CURRENT_PATH%x64" 2> NUL:
REM
DEL "*.sdf" 2> NUL:
REM 
goto :eof
