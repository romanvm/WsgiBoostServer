@echo off

copy /y "..\Release\wsgi_boost.pyd" "..\wsgi_boost\"
cd ..\test_wsgi_boost\
python tests.py
