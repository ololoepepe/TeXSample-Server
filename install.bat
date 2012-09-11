@echo off
echo Installing...
mkdir "%programfiles%\TeXSample Server"
mkdir "%programfiles%\TeXSample Server\include"
copy .\build\release\texsample-server.exe "%programfiles%\TeXSample Server"
copy .\include\*.h "%programfiles%\TeXSample Server\include"
copy "%programfiles%\BeQt\lib\beqtcore0.dll" "%programfiles%\TeXSample Server"
copy "%programfiles%\BeQt\lib\beqtnetwork0.dll" "%programfiles%\TeXSample Server"
echo Installation finished.
