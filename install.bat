@echo off
echo Installing...
mkdir "%programfiles%\TeXSample Server"
mkdir "%programfiles%\TeXSample Server\include"
del "%programfiles%\TeXSample Server\texsample-server.exe"
del "%programfiles%\TeXSample Server\include\*.h"
del "%programfiles%\TeXSample Server\beqt*"
copy ".\build\release\texsample-server.exe" "%programfiles%\TeXSample Server"
copy ".\include\*.h" "%programfiles%\TeXSample Server\include"
copy "%programfiles%\BeQt\lib\beqtcore*" "%programfiles%\TeXSample Server"
copy "%programfiles%\BeQt\lib\beqtnetwork*" "%programfiles%\TeXSample Server"
echo Installation finished.
