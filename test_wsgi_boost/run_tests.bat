@echo off

copy /y "..\Release\tests.exe" .
tests.exe
IF ERRORLEVEL 1 exit /b 1
python tests.py
