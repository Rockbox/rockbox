@echo off
FOR /F "tokens=1,2* delims=, " %%i in (targets.txt) do @mingw32-make MODEL=%%j TARGET=%%i build 2>nul
echo Copying files...
xcopy /I /Y *.dll ..\gui\bin
