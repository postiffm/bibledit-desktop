@echo off
set LANGUAGE=fr
echo %LANGUAGE%
echo "Param1=%1"
echo "Param2=%2"
echo "Param3=%3"
#pause
start "Bibledit" "bibledit-gtk.exe" %1 %2 %3
