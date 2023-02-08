@ECHO OFF
CLS

SET __=----------------------------------------------------------------

ECHO %__%
ECHO WebCard - Web Browser Extension installer
ECHO %__%

SETLOCAL ENABLEEXTENSIONS 2>NUL
IF ERRORLEVEL 1 (
  ECHO * Error: "Command Extenions" are not available!
  GOTO INSTALL_FAILURE
)

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Detect target CPU

  IF NOT "%OS%"=="Windows_NT" (
    ECHO * Error: The "OS" EnvVar is not set to "Windows_NT"!
    GOTO INSTALL_FAILURE
  )

  IF "%PROCESSOR_ARCHITECTURE%"=="x86" (
    SET HOST_OS=win32
    ECHO * Target OS: Windows ^(32-bit^)
  ) ELSE IF "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
    SET HOST_OS=win64
    ECHO * Target OS: Windows ^(64-bit^)
  ) ELSE (
    ECHO * Error: Unsupported Processor Architecture "%PROCESSOR_ARCHITECTURE%"!
    GOTO INSTALL_FAILURE
  )

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ NativeApp configuration

  REM -- Executable name
  SET APP_NAME=webcard.exe

  REM -- The path from which the Native App is copied
  SET APP_PATH=..\native\out\%HOST_OS%

  REM -- The destination for Native App files (exe and manifests)
  SET TARGET_PATH=%APPDATA%\CardID\WebCard

  REM -- Internal name of the Native App
  SET HOST_NAME=org.cardid.webcard.native

  IF NOT EXIST "%APP_PATH%\%APP_NAME%" (
    ECHO * Error: File "%APP_PATH%\%APP_NAME%" not found!
    ECHO  Please ^(re^)build the Native App executable.
    GOTO INSTALL_FAILURE
  )
  ECHO * Native App found at "%APP_PATH%\%APP_NAME%".

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Detecting "Google Chrome" Web Browser

:IS_CHROME_INSTALLED
  REG QUERY "HKLM\SOFTWARE\Clients\StartMenuInternet" /f "Google Chrome" /k 1>NUL
    IF NOT ERRORLEVEL 1 GOTO CHECK_TXT_ID_CHROME
  REG QUERY "HKLM\SOFTWARE\Google\Chrome" 2>NUL
    IF NOT ERRORLEVEL 1 GOTO CHECK_TXT_ID_CHROME
  ECHO * Warning: "Google Chrome" is NOT installed!
  GOTO CHROME_UNSET

:CHECK_TXT_ID_CHROME
  IF NOT EXIST "ID_CHROME.TXT" (
    ECHO * Warning: File "ID_CHROME.TXT" not found!
    GOTO CHROME_UNSET
  )
  SET ID_CHROME=
  SET /P ID_CHROME=<"ID_CHROME.TXT"
  IF DEFINED ID_CHROME (
    ECHO * "Google Chrome" is available.
    SET "WEBCARD_CHROME=."
    GOTO IS_EDGE_INSTALLED
  ) ELSE (
    ECHO * Warning: File "ID_CHROME.TXT" is blank!
    ECHO  Please fill it with the extension identifier.
  )
:CHROME_UNSET
  SET "WEBCARD_CHROME= "

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Detecting "Microsoft Edge" Web Browser

:IS_EDGE_INSTALLED
  REG QUERY "HKLM\SOFTWARE\Clients\StartMenuInternet" /f "Microsoft Edge" /k 1>NUL
    IF NOT ERRORLEVEL 1 GOTO CHECK_TXT_ID_EDGE
  REG QUERY "HKLM\SOFTWARE\Microsoft\Edge" 2>NUL
    IF NOT ERRORLEVEL 1 GOTO CHECK_TXT_ID_EDGE
  ECHO * Warning: "Microsoft Edge" is not installed!
  GOTO EDGE_UNSET

:CHECK_TXT_ID_EDGE
  IF NOT EXIST "ID_EDGE.TXT" (
    ECHO * Warning: File "ID_EDGE.TXT" not found!
    GOTO EDGE_UNSET
  )
  SET ID_EDGE=
  SET /P ID_EDGE=<"ID_EDGE.TXT"
  IF DEFINED ID_EDGE (
    ECHO * "Microsoft Edge" is available.
    SET "WEBCARD_EDGE=."
    GOTO IS_FIREFOX_INSTALLED
  ) ELSE (
    ECHO * Warning: File "ID_EDGE.TXT" is blank!
    ECHO  Please fill it with the extension identifier.
  )
:EDGE_UNSET
  SET "WEBCARD_EDGE= "

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Detecting "Mozilla Firefox" Web Browser

:IS_FIREFOX_INSTALLED
  REG QUERY "HKLM\SOFTWARE\Clients\StartMenuInternet" /f "Firefox" /k 1>NUL
    IF NOT ERRORLEVEL 1 GOTO CHECK_TXT_ID_FIREFOX
  REG QUERY "HKLM\SOFTWARE\Mozilla\Firefox" 2>NUL
    IF NOT ERRORLEVEL 1 GOTO CHECK_TXT_ID_FIREFOX
  ECHO * Warning: "Mozilla Firefox" is not installed!
  GOTO FIREFOX_UNSET

:CHECK_TXT_ID_FIREFOX
  IF NOT EXIST "ID_FIREFOX.TXT" (
    ECHO * Warning: File "ID_FIREFOX.TXT" not found!
    GOTO FIREFOX_UNSET
  )
  SET ID_FIREFOX=
  SET /P ID_FIREFOX=<"ID_FIREFOX.TXT"
  IF DEFINED ID_FIREFOX (
    ECHO * "Mozilla Firefox" is available.
    SET "WEBCARD_FIREFOX=."
    GOTO BROWSER_DETECTION_SUMMARY
  ) ELSE (
    ECHO * Warning: File "ID_FIREFOX.TXT" is blank!
    ECHO  Please fill it with the extension identifier.
  )
:FIREFOX_UNSET
  SET "WEBCARD_FIREFOX= "

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Browser Selection Menu

:BROWSER_DETECTION_SUMMARY
  ECHO %__%
  ECHO ^(press Any Key to continue^)
  PAUSE>NUL

:BROWSER_SELECTION_MENU
  CLS
  ECHO %__%
  ECHO Select Web Browsers
  ECHO %__%
  ECHO 1: [%WEBCARD_CHROME%] Google Chrome
  ECHO 2: [%WEBCARD_EDGE%] Microsoft Edge
  ECHO 3: [%WEBCARD_FIREFOX%] Mozilla Firefox
  ECHO %__%
  ECHO LEGEND:
  ECHO   [ ] = browser unavailable
  ECHO   [.] = browser available, but not selected
  ECHO   [O] = browser selected
  ECHO %__%
  REM -- Reset CHOICE before asking for any input
  SET CHOICE=
  SET /P "CHOICE=Type a number (or leave blank) and press ENTER: "

IF NOT "%CHOICE%"=="1" GOTO SELECTION_MENU_CHECK_2
  IF "%WEBCARD_CHROME%"=="." GOTO SELECTION_MENU_SET_1
  IF "%WEBCARD_CHROME%"=="O" GOTO SELECTION_MENU_UNSET_1
  GOTO BROWSER_SELECTION_MENU
:SELECTION_MENU_SET_1
  SET "WEBCARD_CHROME=O"
  GOTO BROWSER_SELECTION_MENU
:SELECTION_MENU_UNSET_1
  SET "WEBCARD_CHROME=."
  GOTO BROWSER_SELECTION_MENU

:SELECTION_MENU_CHECK_2
IF NOT "%CHOICE%"=="2" GOTO SELECTION_MENU_CHECK_3
  IF "%WEBCARD_EDGE%"=="." GOTO SELECTION_MENU_SET_2
  IF "%WEBCARD_EDGE%"=="O" GOTO SELECTION_MENU_UNSET_2
  GOTO BROWSER_SELECTION_MENU
:SELECTION_MENU_SET_2
  SET "WEBCARD_EDGE=O"
  GOTO BROWSER_SELECTION_MENU
:SELECTION_MENU_UNSET_2
  SET "WEBCARD_EDGE=."
  GOTO BROWSER_SELECTION_MENU

:SELECTION_MENU_CHECK_3
IF NOT "%CHOICE%"=="3" GOTO INSTALLATION_01
  IF "%WEBCARD_FIREFOX%"=="." GOTO SELECTION_MENU_SET_3
  IF "%WEBCARD_FIREFOX%"=="O" GOTO SELECTION_MENU_UNSET_3
  GOTO BROWSER_SELECTION_MENU
:SELECTION_MENU_SET_3
  SET "WEBCARD_FIREFOX=O"
  GOTO BROWSER_SELECTION_MENU
:SELECTION_MENU_UNSET_3
  SET "WEBCARD_FIREFOX=."
  GOTO BROWSER_SELECTION_MENU

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Copying "Native messaging application"

:INSTALLATION_01
  CLS
  ECHO %__%
  ECHO * Asserting that the destination directory exists...
  IF NOT EXIST %TARGET_PATH%\ (
    MD %TARGET_PATH%
    IF NOT ERRORLEVEL 1 GOTO INSTALLATION_02
    ECHO * Error: Could not create the "%TARGET_PATH%" directories!
    GOTO INSTALL_FAILURE
  )
  ECHO OK

:INSTALLATION_02
  ECHO * Copying "%APP_NAME%"...
  COPY /Y /B "%APP_PATH%\%APP_NAME%" "%TARGET_PATH%\%APP_NAME%"
    IF NOT ERRORLEVEL 1 GOTO INSTALLATION_03
    GOTO INSTALL_FAILURE

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Generating "Native messaging host manifest" (JSON files)

:INSTALLATION_03
  IF "%WEBCARD_CHROME%"=="O" GOTO GENERATE_CHROMIUM_MANIFEST
  IF "%WEBCARD_EDGE%"=="O" GOTO GENERATE_CHROMIUM_MANIFEST
  GOTO INSTALLATION_04

:GENERATE_CHROMIUM_MANIFEST
  SET JSON_FILE="%TARGET_PATH%\%HOST_NAME%.chromium.json"
  ECHO * Generating "Native messaging host manifest" ^(Chromium^)...
  ECHO {>"%JSON_FILE%"
  ECHO   "name": "%HOST_NAME%",>>"%JSON_FILE%"
  ECHO   "description": "WebCard Native Helper App",>>"%JSON_FILE%"
  ECHO   "path": "webcard.exe",>>"%JSON_FILE%"
  ECHO   "type": "stdio",>>"%JSON_FILE%"
  ECHO   "allowed_origins": [>>"%JSON_FILE%"
  IF "%WEBCARD_CHROME%"=="O" (
    IF "%WEBCARD_EDGE%"=="O" (
      ECHO     "chrome-extension://%ID_CHROME%/",>>"%JSON_FILE%"
      ECHO     "chrome-extension://%ID_EDGE%/">>"%JSON_FILE%"
    ) ELSE (
      ECHO     "chrome-extension://%ID_CHROME%/">>"%JSON_FILE%"
    )
  ) ELSE (
    ECHO     "chrome-extension://%ID_EDGE%/">>"%JSON_FILE%"
  )
  ECHO   ]>>"%JSON_FILE%"
  ECHO }>>"%JSON_FILE%"
  ECHO OK

  IF "%WEBCARD_CHROME%"=="O" (
    ECHO * Adding new entry to registry ^(Google Chrome^)
    REG ADD "HKCU\Software\Google\Chrome\NativeMessagingHosts\%HOST_NAME%" /ve /t REG_SZ /d "%JSON_FILE%" /f
    IF ERRORLEVEL 1 (
      GOTO INSTALL_FAILURE
    )
  )
  IF "%WEBCARD_EDGE%"=="O" (
    ECHO * Adding new entry to registry ^(Microsoft Edge^)
    REG ADD "HKCU\Software\Microsoft\Edge\NativeMessagingHosts\%HOST_NAME%" /ve /t REG_SZ /d "%JSON_FILE%" /f
    IF ERRORLEVEL 1 (
      GOTO INSTALL_FAILURE
    )
  )

:INSTALLATION_04
  IF "%WEBCARD_FIREFOX%"=="O" GOTO GENERATE_FIREFOX_MANIFEST
  GOTO INSTALLATION_05

:GENERATE_FIREFOX_MANIFEST
  SET JSON_FILE="%TARGET_PATH%\%HOST_NAME%.firefox.json"
  ECHO * Generating "Native messaging host manifest" ^(Firefox^)...
  ECHO {>"%JSON_FILE%""
  ECHO   "name": "%HOST_NAME%",>>"%JSON_FILE%"
  ECHO   "description": "WebCard Native Helper App",>>"%JSON_FILE%"
  ECHO   "path": "webcard.exe",>>"%JSON_FILE%"
  ECHO   "type": "stdio",>>"%JSON_FILE%"
  ECHO   "allowed_extensions": [>>"%JSON_FILE%"
  ECHO     "%ID_FIREFOX%">>"%JSON_FILE%"
  ECHO   ]>>"%JSON_FILE%"
  ECHO }>>"%JSON_FILE%"
  ECHO OK

  ECHO * Adding new entry to registry ^(Mozilla Firefox^)
  REG ADD "HKCU\SOFTWARE\Mozilla\NativeMessagingHosts\%HOST_NAME%" /ve /t REG_SZ /d "%JSON_FILE%" /f
  IF ERRORLEVEL 1 (
    GOTO INSTALL_FAILURE
  )

:INSTALLATION_05
  GOTO INSTALL_SUCCESS

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Installation Failure

:INSTALL_FAILURE
  ECHO.
  ECHO %__%
  ECHO Installation Failure... ^(press Any Key to exit^)
  PAUSE>NUL

  REM -- Returns ERRORLEVEL=1
  EXIT /B 1
  GOTO END

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
REM @ Installation Success

  :INSTALL_SUCCESS
  ECHO.
  ECHO %__%
  ECHO Installation Success! ^(press Any Key to exit^)
  PAUSE>NUL

  REM -- Returns ERRORLEVEL=0
  EXIT /B 0

REM @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

:END
