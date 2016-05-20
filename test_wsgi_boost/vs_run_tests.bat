@echo off

copy /y "..\Release\wsgi_boost.pyd" ".."
cd ..\test_wsgi_boost\
python tests.py
