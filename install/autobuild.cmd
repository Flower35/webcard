@ECHO OFF

SETLOCAL
CD "%~dp0%"

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check for tools: "mingw32-make", "gcc", "python3".

ECHO.
ECHO Checking for build tools...
ECHO.

:STEP_01A
    WHERE "mingw32-make.exe">NUL 2>&1
    IF NOT ERRORLEVEL 1 GOTO STEP_01B
        ECHO * ERROR: Missing "mingw32-make.exe"!
        EXIT /B 1

:STEP_01B
    ECHO * "mingw32-make.exe": OK

    WHERE "gcc.exe">NUL 2>&1
    IF NOT ERRORLEVEL 1 GOTO STEP_01C
        ECHO * ERROR: Missing "gcc.exe"!
        EXIT /B 1

:STEP_01C
    ECHO * "gcc.exe": OK

    WHERE "python.exe">NUL 2>&1
    IF NOT ERRORLEVEL 1 GOTO STEP_01D
        ECHO * ERROR: Missing "python.exe"!
        EXIT /B 1

:STEP_01D
    python.exe -c "import sys;exit(sys.version_info[0])"
    IF ERRORLEVEL 3 IF NOT ERRORLEVEL 4 GOTO STEP_01E
        ECHO * ERROR: Invalid python version (Python3 required)!
        EXIT /B 1

:STEP_01E
    ECHO * "Python3": OK

    IF EXIST ".\INSTALL.CMD" GOTO STEP_01F
        ECHO * ERROR: Missing ".\INSTALL.CMD"!
        EXIT /B 1

:STEP_01F
    ECHO * ".\INSTALL.CMD": OK

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: (Re)Build the Native App

ECHO.
ECHO Running "make"...
ECHO.

:STEP_02A
    SETLOCAL
        CD ..\native
        mingw32-make.exe release -B
    ENDLOCAL

    IF NOT ERRORLEVEL 1 GOTO STEP_02B
        EXIT /B 1

:STEP_02B
    ECHO.
    ECHO * "make": OK

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Calculate extension ID for "ID_CHROMIUM.txt"

ECHO.
ECHO Calculating Chromium extension ID...
ECHO.

:STEP_03A
    FOR %%a IN ("..\extension\chromium\") DO SET EXTENSION_PATH=%%~dpa
    FOR %%a IN ("%EXTENSION_PATH%.") DO SET EXTENSION_PATH=%%~fa

    python.exe -c "import sys, hashlib; print(''.join([chr(int(x, base=16) + ord('a')) for x in hashlib.sha256(sys.argv[1].encode('UTF-16LE')).hexdigest()[:32]]))" "%EXTENSION_PATH%" 1>ID_CHROMIUM.txt
    IF NOT ERRORLEVEL 1 GOTO STEP_03B
        EXIT /B 1

:STEP_03B
    ECHO * "ID_CHROMIUM.txt": OK

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Run the installation script with special argument

ECHO.
ECHO Calling ".\INSTALL.CMD /ALL"...
ECHO.

:STEP_04A
    CALL ".\INSTALL.CMD" /ALL
    IF NOT ERRORLEVEL 1 GOTO STEP_04B
        EXIT /B 1

:STEP_04B
    EXIT /B 0

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
