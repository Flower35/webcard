@ECHO OFF
CLS

TASKKILL /F /T /IM webcard.exe
ECHO.

mingw32-make
ECHO.
 
DATE/T
TIME/T
