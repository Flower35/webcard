#!/bin/sh

# Note: This will only work with non-sourced scripts
cd -- "$(dirname -- "$0")"

################################################################
# Check for tools: "make", "gcc", "python3".

    printf "\nChecking for build tools...\n\n"

    if ! which make >/dev/null 2>&1
    then
        printf "* ERROR: Missing \"make\"!\n"
        return 1 2>/dev/null
        exit 1
    fi

    printf "* \"make\": OK\n"

    if ! which gcc >/dev/null 2>&1
    then
        printf "* ERROR: Missing \"gcc\"!\n"
        return 1 2>/dev/null
        exit 1
    fi

    printf "* \"gcc\": OK\n"

    if ! which python3 >/dev/null 2>&1
    then
        printf "* ERROR: Missing \"python3\"!\n"
        return 1 2>/dev/null
        exit 1
    fi

    printf "* \"python3\": OK\n"

    if [ ! -f "./install.sh" ]
    then
        printf "* ERROR: Missing \"./install.sh\"!\n"
        return 1 2>/dev/null
        exit 1
    fi

    printf "* \"./install.sh\": OK\n"

################################################################
# (Re)Build the Native App

    printf "\nRunning "make"...\n\n"

    (
        cd ../native
        make release -B
    )

    if [ $? -ne 0 ]
    then
        return 1 2>/dev/null
        exit 1
    fi

    printf "\n* \"make\": OK\n"

################################################################
# Calculate extension ID for "ID_CHROMIUM.txt"

    printf "\nCalculating Chromium extension ID...\n\n"

    EXTENSION_PATH="$(cd "../extension/chromium/" >/dev/null 2>&1 ; pwd -P)"

    if ! python3 -c "import sys, hashlib; print(''.join([chr(int(x, base=16) + ord('a')) for x in hashlib.sha256(sys.argv[1].encode('utf-8')).hexdigest()[:32]]))" "${EXTENSION_PATH}" 1>ID_CHROMIUM.txt
    then
        return 1 2>/dev/null
        exit 1
    fi

    printf "* \"ID_CHROMIUM.txt\": OK\n"

################################################################
# Run the installation script with special argument

    printf "\nCalling \"./install.sh -all\"...\n\n"

    if ! sh "./install.sh" -all
    then
        return 1 2>/dev/null
        exit 1
    fi

    return 0 2>/dev/null
    exit 0

################################################################
