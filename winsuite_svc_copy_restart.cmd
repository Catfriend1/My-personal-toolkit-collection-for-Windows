@echo off

SET SCRIPT_PATH=%~dps0

net stop "windows manager suite"
taskkill /f /im winsuite.exe 2> NUL:

copy /y "%SCRIPT_PATH%Release\Winsuite.exe" "%windir%\winsuite.exe"

net start "windows manager suite"
start "WinSuite" "winsuite.exe" /usermode

timeout 3

