@echo off

copy /y "..\Release\tests.exe" .
tests.exe
python tests.py
