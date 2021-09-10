@echo off
net stop "windows manager suite"
taskkill /f /im winsuite.exe 2> NUL:
timeout 3

