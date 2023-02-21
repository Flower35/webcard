#!/bin/sh

################################################################
# Check for special argument "./install.sh -all"

INSTALL_ALL=

if [ "$1" = "-all" ]
then
    INSTALL_ALL="YES"
fi

################################################################

if [ ! "YES" = "${INSTALL_ALL}" ]
then
    clear 2>/dev/null
fi

__="--------------------------------"

printf "%s\n" "${__}"
printf "WebCard - Web Browser Extension installer\n"
printf "%s\n" "${__}"

################################################################
# Detect target OS and target CPU

    SYSTEM_TYPE=`uname -s`
    CPU=`uname -m`

    if [ "${SYSTEM_TYPE}" = "Darwin" ]
    then
        printf "Target OS: macOS X"
        HOST_OS="macOS"

        TARGET_DIR_FIREFOX="${HOME}/Library/Application Support/Mozilla/NativeMessagingHosts"
        TARGET_DIR_CHROMIUM="${HOME}/Library/Application Support/Chromium/NativeMessagingHosts"
        TARGET_DIR_GOOGLE_CHROME="${HOME}/Library/Application Support/Google/Chrome/NativeMessagingHosts"
        TARGET_DIR_MICROSOFT_EDGE="${HOME}/Library/Application Support/Microsoft Edge/NativeMessagingHosts"

        INSTALL_CHECK_BEGIN="[ -d"
        INSTALL_CHECK_END="]"

        APP_FIREFOX="/Applications/Firefox.app"
        APP_CHROMIUM="/Applications/Chromium.app"
        APP_GOOGLE_CHROME="/Applications/Google Chrome.app"
        APP_MICROSOFT_EDGE="/Applications/Microsoft Edge.app"

    elif [ "${SYSTEM_TYPE}" = "Linux" ]
    then
        printf "Target OS: Linux"
        HOST_OS="linux"

        TARGET_DIR_FIREFOX="${HOME}/.mozilla/native-messaging-hosts"
        TARGET_DIR_CHROMIUM="${HOME}/.config/chromium/NativeMessagingHosts"
        TARGET_DIR_GOOGLE_CHROME="${HOME}/.config/google-chrome/NativeMessagingHosts"
        TARGET_DIR_MICROSOFT_EDGE="${HOME}/.config/microsoft-edge/NativeMessagingHosts"

        INSTALL_CHECK_BEGIN="which"
        INSTALL_CHECK_END=""

        APP_FIREFOX="firefox"
        APP_CHROMIUM="chromium"
        APP_GOOGLE_CHROME="google-chrome"
        APP_MICROSOFT_EDGE="microsoft-edge"

    else
        printf "* Error: Unsupported Operating System \"%s\"!\n" "${SYSTEM_TYPE}"

        if [ ! "YES" = "${INSTALL_ALL}" ]
        then
            read -r _
        fi

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

        if [ ! "YES" = "${INSTALL_ALL}" ]
        then
            read -r _
        fi

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
        printf " Please (re)build the Native App executable.\n"

        if [ ! "YES" = "${INSTALL_ALL}" ]
        then
            read -r _
        fi

        exit 1
    fi

    printf "* Native App found at \"%s\".\n" "${APP_PATH}/${APP_NAME}"

################################################################
# Detecting supported Web Browsers

    AVAIL_FIREFOX=

    if ${INSTALL_CHECK_BEGIN} "${APP_FIREFOX}" ${INSTALL_CHECK_END} >/dev/null 2>&1
    then
        printf "* \"Mozilla Firefox\" is available.\n"
        AVAIL_FIREFOX="YES"

    else
        printf "* \"Mozilla Firefox\" is NOT installed!\n"
    fi

    AVAIL_CHROMIUM=

    if ${INSTALL_CHECK_BEGIN} "${APP_CHROMIUM}" ${INSTALL_CHECK_END} >/dev/null 2>&1
    then
        printf "* \"Chromium\" is available.\n"
        AVAIL_CHROMIUM="YES"

    else
        printf "* \"Chromium\" is NOT installed!\n"
    fi

    AVAIL_GOOGLE_CHROME=

    if ${INSTALL_CHECK_BEGIN} "${APP_GOOGLE_CHROME}" ${INSTALL_CHECK_END} >/dev/null 2>&1
    then
        printf "* \"Google Chrome\" is available.\n"
        AVAIL_GOOGLE_CHROME="YES"

    else
        printf "* \"Google Chrome\" is NOT installed!\n"
    fi

    AVAIL_MICROSOFT_EDGE=

    if ${INSTALL_CHECK_BEGIN} "${APP_MICROSOFT_EDGE}" ${INSTALL_CHECK_END} >/dev/null 2>&1
    then
        printf "* \"Microsoft Edge\" is available.\n"
        AVAIL_MICROSOFT_EDGE="YES"

    else
        printf "* \"Microsoft Edge\" is NOT installed!\n"
    fi

################################################################
# Reading "Firefox" Extension ID

    # Blank space for words "Available" or "Selected "
    SEL_FIREFOX="         "

    if [ "YES" = "${AVAIL_FIREFOX}" ]
    then
        if [ -f "ID_FIREFOX.txt" ]
        then
            ID_FIREFOX=`cat "ID_FIREFOX.txt"`

            if [ ! -z "${ID_FIREFOX}" ]
            then
                SEL_FIREFOX="Available"

            else
                printf "* Warning: File \"ID_FIREFOX.txt\" is blank!\n"
                printf " Please fill it with the extension identifier.\n"
            fi
        else
            printf "* Warning: File \"ID_FIREFOX.txt\" not found!\n"
        fi
    fi

################################################################
# Reading "Chromium" Extension ID

    # Blank space for words "Available" or "Selected "
    SEL_CHROMIUM="         "
    SEL_GOOGLE_CHROME="         "
    SEL_MICROSOFT_EDGE="         "

    if  [ "YES" = "${AVAIL_CHROMIUM}" ] ||\
        [ "YES" = "${AVAIL_GOOGLE_CHROME}" ] ||\
        [ "YES" = "${AVAIL_MICROSOFT_EDGE}" ]
    then
        if [ -f "ID_CHROMIUM.txt" ]
        then
            ID_CHROMIUM=`cat "ID_CHROMIUM.txt"`

            if [ ! -z "${ID_CHROMIUM}" ]
            then
                if  [ "YES" = "${AVAIL_CHROMIUM}" ]
                then
                    SEL_CHROMIUM="Available"
                fi

                if  [ "YES" = "${AVAIL_GOOGLE_CHROME}" ]
                then
                    SEL_GOOGLE_CHROME="Available"
                fi

                if  [ "YES" = "${AVAIL_MICROSOFT_EDGE}" ]
                then
                    SEL_MICROSOFT_EDGE="Available"
                fi

            else
                printf "* Warning: File \"ID_CHROMIUM.txt\" is blank!\n"
                printf " Please fill it with the extension identifier.\n"
            fi
        else
            printf "* Warning: File \"ID_CHROMIUM.txt\" not found!\n"
        fi
    fi

################################################################
# Browser Selection Menu

    if [ "YES" = "${INSTALL_ALL}" ]
    then
        if [ "Available" = "${SEL_FIREFOX}" ]
        then
            SEL_FIREFOX="Selected "
        fi

        if [ "Available" = "${SEL_CHROMIUM}" ]
        then
            SEL_CHROMIUM="Selected "
        fi

        if [ "Available" = "${SEL_GOOGLE_CHROME}" ]
        then
            SEL_GOOGLE_CHROME="Selected "
        fi

        if [ "Available" = "${SEL_MICROSOFT_EDGE}" ]
        then
            SEL_MICROSOFT_EDGE="Selected "
        fi

    else
        printf "%s\n" "${__}"
        printf "(press Enter to continue)\n"
        read -r _

        CHOICE=

        while [ -z "${CHOICE}" ]
        do
            clear 2>/dev/null

            printf "%s\n" "${__}"
            printf "Select Web Browsers\n"
            printf "%s\n" "${__}"
            printf "1: [ %s ] \"Mozilla Firefox\"\n" "${SEL_FIREFOX}"
            printf "2: [ %s ] \"Chromium\"\n" "${SEL_CHROMIUM}"
            printf "3: [ %s ] \"Google Chrome\"\n" "${SEL_GOOGLE_CHROME}"
            printf "4: [ %s ] \"Microsoft Edge\"\n" "${SEL_MICROSOFT_EDGE}"
            printf "%s\n" "${__}"

            CHOICE=
            printf "Type a SINGLE number (or leave a blank choice\n"
            printf " to continue), then press ENTER: "
            read -r CHOICE

            if [ "1" = "${CHOICE}" ]
            then
                if [ "Available" = "${SEL_FIREFOX}" ]
                then
                    SEL_FIREFOX="Selected "

                elif [ "Selected " = "${SEL_FIREFOX}" ]
                then
                    SEL_FIREFOX="Available"
                fi

                CHOICE=

            elif [ "2" = "${CHOICE}" ]
            then
                if [ "Available" = "${SEL_CHROMIUM}" ]
                then
                    SEL_CHROMIUM="Selected "

                elif [ "Selected " = "${SEL_CHROMIUM}" ]
                then
                    SEL_CHROMIUM="Available"
                fi

                CHOICE=

            elif [ "3" = "${CHOICE}" ]
            then
                if [ "Available" = "${SEL_GOOGLE_CHROME}" ]
                then
                    SEL_GOOGLE_CHROME="Selected "

                elif [ "Selected " = "${SEL_GOOGLE_CHROME}" ]
                then
                    SEL_GOOGLE_CHROME="Available"
                fi

                CHOICE=

            elif [ "4" = "${CHOICE}" ]
            then
                if [ "Available" = "${SEL_MICROSOFT_EDGE}" ]
                then
                    SEL_MICROSOFT_EDGE="Selected "

                elif [ "Selected " = "${SEL_MICROSOFT_EDGE}" ]
                then
                    SEL_MICROSOFT_EDGE="Available"
                fi

                CHOICE=

            else
                # "CHOICE" contains more than one number, or is blank.
                # Break out of the while-loop.
                CHOICE=" "
            fi
        done
    fi

################################################################
# Copying "Native messaging application"

    if [ ! "YES" = "${INSTALL_ALL}" ]
    then
        clear 2>/dev/null
    fi

    printf "%s\n" "${__}"
    printf "* Asserting that the destination directory exists...\n"

    if ! mkdir -p "${TARGET_PATH}"
    then
        if [ ! "YES" = "${INSTALL_ALL}" ]
        then
            read -r _
        fi

        exit 1
    fi

    printf "OK\n"

    printf "%s\n" "${__}"
    printf "* Copying \"%s\"...\n" "${APP_NAME}"

    if ! cp "${APP_PATH}/${APP_NAME}" "${TARGET_PATH}/${APP_NAME}"
    then
        if [ ! "YES" = "${INSTALL_ALL}" ]
        then
            read -r _
        fi

        exit 1
    fi

    printf "OK\n"

################################################################
# Generating "Native messaging host manifest" (JSON files)

    if [ "Selected " = "${SEL_FIREFOX}" ]
    then
        JSON_FILE="${TARGET_DIR_FIREFOX}/${HOST_NAME}.json"
        printf "%s\n" "${__}"
        printf "* Generating \"Native messaging host manifest\" (Mozilla Firefox)...\n"

        if ! mkdir -p "${TARGET_DIR_FIREFOX}"
        then
            if [ ! "YES" = "${INSTALL_ALL}" ]
            then
                read -r _
            fi

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

    if [ "Selected " = "${SEL_CHROMIUM}" ]
    then
        JSON_FILE="${TARGET_DIR_CHROMIUM}/${HOST_NAME}.json"
        printf "%s\n" "${__}"
        printf "* Generating \"Native messaging host manifest\" (Chromium)...\n"

        if ! mkdir -p "${TARGET_DIR_CHROMIUM}"
        then
            if [ ! "YES" = "${INSTALL_ALL}" ]
            then
                read -r _
            fi

            exit 1
        fi

        printf "{\n" >"${JSON_FILE}"
        printf "    \"name\": \"%s\",\n" "${HOST_NAME}" >>"${JSON_FILE}"
        printf "    \"description\": \"WebCard Native Helper App\",\n" >>"${JSON_FILE}"
        printf "    \"path\": \"%s\",\n" "${TARGET_PATH}/${APP_NAME}" >>"${JSON_FILE}"
        printf "    \"type\": \"stdio\",\n" >>"${JSON_FILE}"
        printf "    \"allowed_origins\": [\n" >>"${JSON_FILE}"
        printf "        \"chrome-extension://%s/\"\n" "${ID_CHROMIUM}" >>"${JSON_FILE}"
        printf "    ]\n" >>"${JSON_FILE}"
        printf "}\n" >>"${JSON_FILE}"

        printf "OK\n"
    fi

    if [ "Selected " = "${SEL_GOOGLE_CHROME}" ]
    then
        JSON_FILE="${TARGET_DIR_GOOGLE_CHROME}/${HOST_NAME}.json"
        printf "%s\n" "${__}"
        printf "* Generating \"Native messaging host manifest\" (Google Chrome)...\n"

        if ! mkdir -p "${TARGET_DIR_GOOGLE_CHROME}"
        then
            if [ ! "YES" = "${INSTALL_ALL}" ]
            then
                read -r _
            fi

            exit 1
        fi

        printf "{\n" >"${JSON_FILE}"
        printf "    \"name\": \"%s\",\n" "${HOST_NAME}" >>"${JSON_FILE}"
        printf "    \"description\": \"WebCard Native Helper App\",\n" >>"${JSON_FILE}"
        printf "    \"path\": \"%s\",\n" "${TARGET_PATH}/${APP_NAME}" >>"${JSON_FILE}"
        printf "    \"type\": \"stdio\",\n" >>"${JSON_FILE}"
        printf "    \"allowed_origins\": [\n" >>"${JSON_FILE}"
        printf "        \"chrome-extension://%s/\"\n" "${ID_CHROMIUM}" >>"${JSON_FILE}"
        printf "    ]\n" >>"${JSON_FILE}"
        printf "}\n" >>"${JSON_FILE}"

        printf "OK\n"
    fi

    if [ "Selected " = "${SEL_MICROSOFT_EDGE}" ]
    then
        JSON_FILE="${TARGET_DIR_MICROSOFT_EDGE}/${HOST_NAME}.json"
        printf "%s\n" "${__}"
        printf "* Generating \"Native messaging host manifest\" (Microsoft Edge)...\n"

        if ! mkdir -p "${TARGET_DIR_MICROSOFT_EDGE}"
        then
            if [ ! "YES" = "${INSTALL_ALL}" ]
            then
                read -r _
            fi

            exit 1
        fi

        printf "{\n" >"${JSON_FILE}"
        printf "    \"name\": \"%s\",\n" "${HOST_NAME}" >>"${JSON_FILE}"
        printf "    \"description\": \"WebCard Native Helper App\",\n" >>"${JSON_FILE}"
        printf "    \"path\": \"%s\",\n" "${TARGET_PATH}/${APP_NAME}" >>"${JSON_FILE}"
        printf "    \"type\": \"stdio\",\n" >>"${JSON_FILE}"
        printf "    \"allowed_origins\": [\n" >>"${JSON_FILE}"
        printf "        \"chrome-extension://%s/\"\n" "${ID_CHROMIUM}" >>"${JSON_FILE}"
        printf "    ]\n" >>"${JSON_FILE}"
        printf "}\n" >>"${JSON_FILE}"

        printf "OK\n"
    fi

################################################################
# Installation Success

    printf "\n%s\n" "${__}"

    if [ "YES" = "${INSTALL_ALL}" ]
    then
        printf "Installation Success!\n"

    else
        printf "Installation Success! (press Any Key to exit)\n"

        read -r _
        clear 2>/dev/null
    fi

    exit 0

################################################################
