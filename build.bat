if exist GamesaveHelper.exe del GamesaveHelper.exe
if exist debug.txt del debug.txt
set PATH=C:\w64devkit\bin
make
start GamesaveHelper.exe
pause