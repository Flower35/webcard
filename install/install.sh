#!/bin/sh

clear

__="--------------------------------"

printf "%s\n" "${__}"
printf "WebCard - Web Browser Extension installer\n"
printf "%s\n" "${__}"

################################################################
# Detect target OS and target CPU

    SYSTEM_TYPE="$(uname -s)"
    CPU="$(uname -m)"

    if [ "${SYSTEM_TYPE}" = "Darwin" ]
    then
        printf "Target OS: macOS X"
        HOST_OS="macOS"

        TARGET_DIR_CHROME="${HOME}/Library/Application Support/Google/Chrome/NativeMessagingHosts"
        TARGET_DIR_EDGE="${HOME}/Library/Application Support/Microsoft Edge/NativeMessagingHosts"
        TARGET_DIR_FIREFOX="${HOME}/Library/Application Support/Mozilla/NativeMessagingHosts"

    elif [ "${SYSTEM_TYPE}" = "Linux" ]
    then
        printf "Target OS: Linux"
        HOST_OS="linux"

        TARGET_DIR_CHROME="${HOME}/.config/google-chrome/NativeMessagingHosts"
        TARGET_DIR_EDGE="${HOME}/.config/microsoft-edge/NativeMessagingHosts"
        TARGET_DIR_FIREFOX="${HOME}/.mozilla/native-messaging-hosts"

    else
        printf "* Error: Unsupported Operating System \"%s\"!\n" "${SYSTEM_TYPE}"
        exit 1
    fi

    if [ "${CPU}" = "i686" ]
    then
        printf " (32-bit)\n"
        HOST_OS="${HOST_OS}32"

    elif [ "${CPU}" = "x86_64" ]
    then
        printf " (64-bit)\n"
        HOST_OS="${HOST_OS}64"

    else
        printf "\n* Error: Unsupported Processor Architecture \"%s\"!\n" "${CPU}"
        exit 1
    fi

################################################################
# NativeApp configuration

    # Executable name
    APP_NAME="webcard"

    # The path from which the Native App is copied
    APP_PATH="../native/out/${HOST_OS}"

    # The destination for Native App executable
    TARGET_PATH="${HOME}/opt/CardID/WebCard"

    # Internal name of the Native App
    HOST_NAME=org.cardid.webcard.native

    if [ ! -f "${APP_PATH}/${APP_NAME}" ]
    then
        printf "* Error: File \"%s\" not found!\n" "${APP_PATH}/${APP_NAME}"
        printf "Please (re)build the Native App executable.\n"
        exit 1
    fi

    printf "* Native App found at \"%s\".\n" "${APP_PATH}/${APP_NAME}"

################################################################
# Detecting "Google Chrome" Web Browser

    WEBCARD_CHROME=

    if which google-chrome >/dev/null 2>&1
    then
        if [ -f "ID_CHROME.txt" ]
        then
            ID_CHROME="$(cat \"ID_CHROME.txt\")"

            if [ ! -z "${ID_CHROME}" ]
            then
                printf "* \"Google Chrome\" is available.\n"
                WEBCARD_CHROME="."

            else
                printf "* Warning: File \"ID_CHROME.txt\" is blank!\n"
                printf "Please fill it with the extension identifier."
            fi
        else
            printf "* Warning: File \"ID_CHROME.txt\" not found!\n"
        fi
    else
        printf "* Warning: \"Google Chrome\" is NOT installed!\n"
    fi

################################################################
# Detecting "Microsoft Edge" Web Browser

    WEBCARD_EDGE=

    if which microsoft-edge >/dev/null 2>&1
    then
        if [ -f "ID_EDGE.txt" ]
        then
            ID_EDGE="$(cat \"ID_EDGE.txt\")"

            if [ ! -z "${ID_EDGE}" ]
            then
                printf "* \"Microsoft Edge\" is available.\n"
                WEBCARD_EDGE="."

            else
                printf "* Warning: File \"ID_EDGE.txt\" is blank!\n"
                printf "Please fill it with the extension identifier."
            fi
        else
            printf "* Warning: File \"ID_EDGE.txt\" not found!\n"
        fi
    else
        printf "* Warning: \"Microsoft Edge\" is NOT installed!\n"
    fi

################################################################
# Detecting "Mozilla Firefox" Web Browser

    WEBCARD_FIREFOX=

    if which firefox >/dev/null 2>&1
    then
        if [ -f "ID_FIREFOX.txt" ]
        then
            ID_FIREFOX="$(cat \"ID_FIREFOX.txt\")"

            if [ ! -z "${ID_FIREFOX}" ]
            then
                printf "* \"Mozilla Firefox\" is available.\n"
                WEBCARD_FIREFOX="."

            else
                printf "* Warning: File \"ID_FIREFOX.txt\" is blank!\n"
                printf "Please fill it with the extension identifier."
            fi
        else
            printf "* Warning: File \"ID_FIREFOX.txt\" not found!\n"
        fi
    else
        printf "* Warning: \"Mozilla Firefox\" is NOT installed!\n"
    fi

################################################################
# Browser Selection Menu

    printf "%s\n" "${__}"
    printf "(press Enter to continue)\n"
    read -r _

    # Changing unset variables to blanks (single whitespace)

    if [ -z "${WEBCARD_CHROME}" ]
    then
        WEBCARD_CHROME=" "
    fi

    if [ -z "${WEBCARD_EDGE}" ]
    then
        WEBCARD_EDGE=" "
    fi

    if [ -z "${WEBCARD_FIREFOX}" ]
    then
        WEBCARD_FIREFOX=" "
    fi

    CHOICE=

    while [ -z "${CHOICE}" ]
    do
        clear

        printf "%s\n" "${__}"
        printf "Select Web Browsers\n"
        printf "%s\n" "${__}"
        printf "1: [%s] Google Chrome\n" "${WEBCARD_CHROME}"
        printf "2: [%s] Microsoft Edge\n" "${WEBCARD_EDGE}"
        printf "3: [%s] Mozilla Firefox\n" "${WEBCARD_FIREFOX}"
        printf "%s\nLEGEND:\n" "${__}"
        printf "  [ ] = browser unavailable\n"
        printf "  [.] = browser available, but not selected\n"
        printf "  [O] = browser selected\n"
        printf "%s\n" "${__}"

        CHOICE=
        printf "Type a number (or leave blank) and press ENTER:\n"
        read -r CHOICE

        if [ "1" = "${CHOICE}" ]
        then
            if [ "." = "${WEBCARD_CHROME}" ]
            then
                WEBCARD_CHROME="O"

            elif [ "O" = "${WEBCARD_CHROME}" ]
            then
                WEBCARD_CHROME="."
            fi

            CHOICE=

        elif [ "2" = "${CHOICE}" ]
        then
            if [ "." = "${WEBCARD_EDGE}" ]
            then
                WEBCARD_EDGE="O"

            elif [ "O" = "${WEBCARD_EDGE}" ]
            then
                WEBCARD_EDGE="."
            fi

            CHOICE=

        elif [ "3" = "${CHOICE}" ]
        then
            if [ "." = "${WEBCARD_FIREFOX}" ]
            then
                WEBCARD_FIREFOX="O"

            elif [ "O" = "${WEBCARD_FIREFOX}" ]
            then
                WEBCARD_FIREFOX="."
            fi

            CHOICE=

        else
            # "CHOICE" contains more than one number, or is blank.
            # Break out of the while-loop.
            CHOICE=" "
        fi
    done

################################################################
# Copying "Native messaging application"

    clear

    printf "%s\n" "${__}"
    printf "* Asserting that the destination directory exists...\n"

    if ! mkdir -p "${TARGET_PATH}"
    then
        exit 1
    fi

    printf "OK\n"

    printf "* Copying \"%s\"...\n" "${APP_NAME}"

    if ! cp "${APP_PATH}/${APP_NAME}" "${TARGET_PATH}/${APP_NAME}"
    then
        exit 1
    fi

    printf "OK\n"

################################################################
# Generating "Native messaging host manifest" (JSON files)

    if [ "O" = "${WEBCARD_CHROME}" ]
    then
        JSON_FILE="${TARGET_DIR_CHROME}/${HOST_NAME}.json"
        printf "* Generating \"Native messaging host manifest\" (Google Chrome)...\n"

        if ! mkdir -p "${TARGET_DIR_CHROME}"
        then
            exit 1
        fi

        printf "{\n" >"${JSON_FILE}"
        printf "    \"name\": \"%s\",\n" "${HOST_NAME}" >>"${JSON_FILE}"
        printf "    \"description\": \"WebCard Native Helper App\",\n" >>"${JSON_FILE}"
        printf "    \"path\": \"%s\",\n" "${TARGET_PATH}/${APP_NAME}" >>"${JSON_FILE}"
        printf "    \"type\": \"stdio\",\n" >>"${JSON_FILE}"
        printf "    \"allowed_origins\": [\n" >>"${JSON_FILE}"
        printf "        \"chrome-extension://%s/\"\n" "${ID_CHROME}" >>"${JSON_FILE}"
        printf "    ]\n" >>"${JSON_FILE}"
        printf "}\n" >>"${JSON_FILE}"

        printf "OK\n"
    fi

    if [ "O" = "${WEBCARD_EDGE}" ]
    then
        JSON_FILE="${TARGET_DIR_EDGE}/${HOST_NAME}.json"
        printf "* Generating \"Native messaging host manifest\" (Microsoft Edge)...\n"

        if ! mkdir -p "${TARGET_DIR_EDGE}"
        then
            exit 1
        fi

        printf "{\n" >"${JSON_FILE}"
        printf "    \"name\": \"%s\",\n" "${HOST_NAME}" >>"${JSON_FILE}"
        printf "    \"description\": \"WebCard Native Helper App\",\n" >>"${JSON_FILE}"
        printf "    \"path\": \"%s\",\n" "${TARGET_PATH}/${APP_NAME}" >>"${JSON_FILE}"
        printf "    \"type\": \"stdio\",\n" >>"${JSON_FILE}"
        printf "    \"allowed_origins\": [\n" >>"${JSON_FILE}"
        printf "        \"chrome-extension://%s/\"\n" "${ID_EDGE}" >>"${JSON_FILE}"
        printf "    ]\n" >>"${JSON_FILE}"
        printf "}\n" >>"${JSON_FILE}"

        printf "OK\n"
    fi

    if [ "O" = "${WEBCARD_FIREFOX}" ]
    then
        JSON_FILE="${TARGET_DIR_FIREFOX}/${HOST_NAME}.json"
        printf "* Generating \"Native messaging host manifest\" (Mozilla Firefox)...\n"

        if ! mkdir -p "${TARGET_DIR_FIREFOX}"
        then
            exit 1
        fi

        printf "{\n" >"${JSON_FILE}"
        printf "    \"name\": \"%s\",\n" "${HOST_NAME}" >>"${JSON_FILE}"
        printf "    \"description\": \"WebCard Native Helper App\",\n" >>"${JSON_FILE}"
        printf "    \"path\": \"%s\",\n" "${TARGET_PATH}/${APP_NAME}" >>"${JSON_FILE}"
        printf "    \"type\": \"stdio\",\n" >>"${JSON_FILE}"
        printf "    \"allowed_extensions\": [\n" >>"${JSON_FILE}"
        printf "        \"%s\"\n" "${ID_FIREFOX}" >>"${JSON_FILE}"
        printf "    ]\n" >>"${JSON_FILE}"
        printf "}\n" >>"${JSON_FILE}"

        printf "OK\n"
    fi

################################################################
# Installation Success

    printf "\n%s\n" "${__}"
    printf "Installation Success! (press Any Key to exit)\n"

    read -r _

    exit 0

################################################################
