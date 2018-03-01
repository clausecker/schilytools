@echo off

if exist .\psmake\smake.exe goto gopsmake
cd psmake
sh make-sh
cd ..
if exist .\psmake\smake.exe goto gopsmake

echo Cannot make .\psmake\smake.exe
goto quit

:gopsmake
.\psmake\smake.exe all

:quit
