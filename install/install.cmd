@ECHO OFF

SETLOCAL

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Check for special argument "INSTALL.CMD /ALL"

SET INSTALL_ALL=

SET CHOICE="%1"
SET CHOICE=%CHOICE:"=%
IF "/ALL"=="%CHOICE%" SET INSTALL_ALL=YES

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

IF NOT "YES"=="%INSTALL_ALL%" CLS

SET __=--------------------------------

ECHO %__%
ECHO WebCard - Web Browser Extension installer
ECHO %__%

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Detect target CPU

:STEP_01A
    IF "%OS%"=="Windows_NT" GOTO STEP_01B
        ECHO * Error: The "OS" EnvVar is not set to "Windows_NT"!
        GOTO INSTALL_FAILURE

:STEP_01B
    IF NOT "%PROCESSOR_ARCHITECTURE%"=="x86" GOTO STEP_01C
        SET HOST_OS=win32
        ECHO * Target OS: Windows (32-bit)
        GOTO STEP_02A

:STEP_01C
    IF NOT "%PROCESSOR_ARCHITECTURE%"=="AMD64" GOTO STEP_01D
        SET HOST_OS=win64
        ECHO * Target OS: Windows (64-bit)
        GOTO STEP_02A

:STEP_01D
    ECHO * Error: Unsupported Processor Architecture "%PROCESSOR_ARCHITECTURE%"!
    GOTO INSTALL_FAILURE

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: NativeApp configuration

:STEP_02A
    REM -- Executable name
    SET APP_NAME=webcard.exe

    REM -- The path from which the Native App is copied
    SET APP_PATH=..\native\out\%HOST_OS%

    REM -- The destination for Native App files (exe and manifests)
    SET TARGET_PATH=%APPDATA%\CardID\WebCard

    REM -- Internal name of the Native App
    SET HOST_NAME=org.cardid.webcard.native

    IF EXIST "%APP_PATH%\%APP_NAME%" GOTO STEP_02B
        ECHO * Error: File "%APP_PATH%\%APP_NAME%" not found!
        ECHO  Please (re)build the Native App executable.
        GOTO INSTALL_FAILURE

:STEP_02B
    ECHO * Native App found at "%APP_PATH%\%APP_NAME%".

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Detecting supported Web Browsers

SET AVAIL_FIREFOX=
SET AVAIL_CHROMIUM=

:STEP_03A
    REG QUERY "HKLM\SOFTWARE\Clients\StartMenuInternet" /f "Firefox" /k 1>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03A_OK
    REG QUERY "HKLM\SOFTWARE\Mozilla\Firefox" 2>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03A_OK
    REG QUERY "HKCU\SOFTWARE\Clients\StartMenuInternet" /f "Firefox" /k 1>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03A_OK
    REG QUERY "HKCU\SOFTWARE\Mozilla\Firefox" 2>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03A_OK
    ECHO * "Mozilla Firefox" is NOT installed!
    GOTO STEP_03B

:STEP_03A_OK
    ECHO * "Mozilla Firefox" is available.
    SET AVAIL_FIREFOX=YES

:STEP_03B
    REG QUERY "HKLM\SOFTWARE\Clients\StartMenuInternet" /f "Chromium" /k 1>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03B_OK
    REG QUERY "HKLM\SOFTWARE\Chromium" 2>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03B_OK
    REG QUERY "HKCU\SOFTWARE\Clients\StartMenuInternet" /f "Chromium" /k 1>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03B_OK
    REG QUERY "HKCU\SOFTWARE\Chromium" 2>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03B_OK
    GOTO STEP_03C

:STEP_03B_OK
    ECHO * "Chromium" is available.
    SET AVAIL_CHROMIUM=YES

:STEP_03C
    REG QUERY "HKLM\SOFTWARE\Clients\StartMenuInternet" /f "Google Chrome" /k 1>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03C_OK
    REG QUERY "HKLM\SOFTWARE\Google\Chrome" 2>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03C_OK
    REG QUERY "HKCU\SOFTWARE\Clients\StartMenuInternet" /f "Google Chrome" /k 1>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03C_OK
    REG QUERY "HKCU\SOFTWARE\Google\Chrome" 2>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03C_OK
    GOTO STEP_03D

:STEP_03C_OK
    ECHO * "Google Chrome" is available.
    SET AVAIL_CHROMIUM=YES

:STEP_03D
    REG QUERY "HKLM\SOFTWARE\Clients\StartMenuInternet" /f "Microsoft Edge" /k 1>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03D_OK
    REG QUERY "HKLM\SOFTWARE\Microsoft\Edge" 2>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03D_OK
    REG QUERY "HKCU\SOFTWARE\Clients\StartMenuInternet" /f "Microsoft Edge" /k 1>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03D_OK
    REG QUERY "HKCU\SOFTWARE\Microsoft\Edge" 2>NUL
        IF NOT ERRORLEVEL 1 GOTO STEP_03D_OK
    GOTO STEP_03E

:STEP_03D_OK
    ECHO * "Microsoft Edge" is available.
    SET AVAIL_CHROMIUM=YES

:STEP_03D
    IF "YES"=="%AVAIL_CHROMIUM%" GOTO STEP_04A
        ECHO * NO "Chromium"-based web browser installed!

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Reading "Firefox" Extension ID

:STEP_04A
    REM -- Blank space for words "Available" or "Selected "
    SET "SEL_FIREFOX=         "

    IF NOT "YES"=="%AVAIL_FIREFOX%" GOTO STEP_05A

    IF EXIST "ID_FIREFOX.TXT" GOTO STEP_04B
        ECHO * Warning: File "ID_FIREFOX.TXT" not found!
        GOTO STEP_05A

:STEP_04B
    SET /P ID_FIREFOX=<"ID_FIREFOX.TXT"
    IF NOT ""=="%ID_FIREFOX%" GOTO STEP_04C
        ECHO * Warning: File "ID_FIREFOX.TXT" is blank!
        ECHO  Please fill it with the extension identifier.
        GOTO STEP_05A

:STEP_04C
    SET "SEL_FIREFOX=Available"

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Reading "Chromium" Extension ID

:STEP_05A
    REM -- Blank space for words "Available" or "Selected "
    SET "SEL_CHROMIUM=         "

    IF NOT "YES"=="%AVAIL_CHROMIUM%" GOTO STEP_06A

    IF EXIST "ID_CHROMIUM.TXT" GOTO STEP_05B
        ECHO * Warning: File "ID_CHROMIUM.TXT" not found!
        GOTO STEP_06A

:STEP_05B
    SET /P ID_CHROMIUM=<"ID_CHROMIUM.TXT"
    IF NOT ""=="%ID_CHROMIUM%" GOTO STEP_05C
        ECHO * Warning: File "ID_CHROMIUM.TXT" is blank!
        ECHO  Please fill it with the extension identifier.
        GOTO STEP_06A

:STEP_05C
    SET "SEL_CHROMIUM=Available"

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Browser Selection Menu

:STEP_06AA
    IF NOT "YES"=="%INSTALL_ALL%" GOTO STEP_06AB
        IF "Available"=="%SEL_FIREFOX%"  SET "SEL_FIREFOX=Selected "
        IF "Available"=="%SEL_CHROMIUM%" SET "SEL_CHROMIUM=Selected "
        GOTO STEP_07A

:STEP_06AB
    ECHO %__%
    ECHO (press Any Key to continue)
    PAUSE>NUL

:STEP_06AC
    CLS
    ECHO %__%
    ECHO Select Web Browsers
    ECHO %__%
    ECHO 1: [ %SEL_FIREFOX% ] "Mozilla Firefox"
    ECHO 2: [ %SEL_CHROMIUM% ] "Chromium", "Google Chrome", "Microsoft Edge"
    ECHO %__%

    REM -- Reset CHOICE before asking for any input
    SET CHOICE=
    ECHO Type a SINGLE number (or leave a blank choice
    SET /P CHOICE="to continue), then press ENTER: "
    SET CHOICE="%CHOICE%"
    SET CHOICE=%CHOICE:"=%

:STEP_06BA
    IF NOT "1"=="%CHOICE%" GOTO STEP_06CA
        IF "Available"=="%SEL_FIREFOX%" GOTO STEP_06BB
        IF "Selected "=="%SEL_FIREFOX%" GOTO STEP_06BC
        GOTO STEP_06AC
:STEP_06BB
    SET "SEL_FIREFOX=Selected "
    GOTO STEP_06AC
:STEP_06BC
    SET "SEL_FIREFOX=Available"
    GOTO STEP_06AC

:STEP_06CA
    IF NOT "2"=="%CHOICE%" GOTO STEP_07A
        IF "Available"=="%SEL_CHROMIUM%" GOTO STEP_06CB
        IF "Selected "=="%SEL_CHROMIUM%" GOTO STEP_06CC
        GOTO STEP_06AC
:STEP_06CB
    SET "SEL_CHROMIUM=Selected "
    GOTO STEP_06AC
:STEP_06CC
    SET "SEL_CHROMIUM=Available"
    GOTO STEP_06AC

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Copying "Native messaging application"

:STEP_07A
    IF NOT "YES"=="%INSTALL_ALL%" CLS
    ECHO %__%
    ECHO * Asserting that the destination directory exists...
    IF EXIST "%TARGET_PATH%\" GOTO STEP_07B
        MD "%TARGET_PATH%"
        IF NOT ERRORLEVEL 1 GOTO STEP_07B
            GOTO INSTALL_FAILURE

:STEP_07B
    ECHO OK

    ECHO %__%
    ECHO * Copying "%APP_NAME%"...
    COPY /Y /B "%APP_PATH%\%APP_NAME%" "%TARGET_PATH%\%APP_NAME%"
        IF NOT ERRORLEVEL 1 GOTO STEP_08A
            GOTO INSTALL_FAILURE

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Generating "Native messaging host manifest" (JSON files)

:STEP_08A
    if "Selected "=="%SEL_FIREFOX%" GOTO STEP_08D
:STEP_08B
    if "Selected "=="%SEL_CHROMIUM%" GOTO STEP_08E
:STEP_08C
    GOTO INSTALL_SUCCESS

:STEP_08D
    SET JSON_FILE="%TARGET_PATH%\%HOST_NAME%.firefox.json"
    ECHO %__%
    ECHO * Generating "Native messaging host manifest" (Firefox)...

    ECHO {>"%JSON_FILE%"
    ECHO     "name": "%HOST_NAME%",>>"%JSON_FILE%"
    ECHO     "description": "WebCard Native Helper App",>>"%JSON_FILE%"
    ECHO     "path": "%APP_NAME%",>>"%JSON_FILE%"
    ECHO     "type": "stdio",>>"%JSON_FILE%"
    ECHO     "allowed_extensions": [>>"%JSON_FILE%"
    ECHO         "%ID_FIREFOX%">>"%JSON_FILE%"
    ECHO     ]>>"%JSON_FILE%"
    ECHO }>>"%JSON_FILE%"

    ECHO OK

    ECHO %__%
    ECHO * Adding a registry key under "Mozilla\NativeMessagingHosts"...
    REG ADD "HKCU\SOFTWARE\Mozilla\NativeMessagingHosts\%HOST_NAME%" /ve /t REG_SZ /d "%JSON_FILE%" /f
        IF NOT ERRORLEVEL 1 GOTO STEP_08B
        GOTO INSTALL_FAILURE

:STEP_08E
    SET JSON_FILE="%TARGET_PATH%\%HOST_NAME%.chromium.json"
    ECHO %__%
    ECHO * Generating "Native messaging host manifest" (Chromium)...

    ECHO {>"%JSON_FILE%"
    ECHO     "name": "%HOST_NAME%",>>"%JSON_FILE%"
    ECHO     "description": "WebCard Native Helper App",>>"%JSON_FILE%"
    ECHO     "path": "%APP_NAME%",>>"%JSON_FILE%"
    ECHO     "type": "stdio",>>"%JSON_FILE%"
    ECHO     "allowed_origins": [>>"%JSON_FILE%"
    ECHO         "chrome-extension://%ID_CHROMIUM%/">>"%JSON_FILE%"
    ECHO     ]>>"%JSON_FILE%"
    ECHO }>>"%JSON_FILE%"

    ECHO OK

    ECHO %__%
    ECHO * Adding a registry key under "Google\Chrome\NativeMessagingHosts"...
    REG ADD "HKCU\Software\Google\Chrome\NativeMessagingHosts\%HOST_NAME%" /ve /t REG_SZ /d "%JSON_FILE%" /f
        IF NOT ERRORLEVEL 1 GOTO STEP_08C
        GOTO INSTALL_FAILURE

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Installation Failure

:INSTALL_FAILURE
    ECHO.
    ECHO %__%

    IF "YES"=="%INSTALL_ALL%" GOTO INSTALL_FAILURE_SHORT
        ECHO Installation Failure... (press Any Key to exit)
        PAUSE>NUL
        CLS

        EXIT /B 1

:INSTALL_FAILURE_SHORT
    ECHO Installation Failure...
    EXIT /B 1

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Installation Success

:INSTALL_SUCCESS
    ECHO.
    ECHO %__%

    IF "YES"=="%INSTALL_ALL%" GOTO INSTALL_SUCCESS_SHORT
        ECHO Installation Success! (press Any Key to exit)
        PAUSE>NUL
        CLS

        EXIT /B 0

:INSTALL_SUCCESS_SHORT
    ECHO Installation Success!
    EXIT /B 0

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
