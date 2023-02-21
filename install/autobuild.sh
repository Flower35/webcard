#!/bin/sh

################################################################
# Check for tools: "make", "gcc", "python3".

    printf "\nChecking for build tools...\n\n"

    if ! which make >/dev/null
    then
        printf "* ERROR: Missing \"make\"!\n"
        exit 1
    fi

    printf "* \"make\": OK\n"

    if ! which gcc >/dev/null
    then
        printf "* ERROR: Missing \"gcc\"!\n"
        exit 1
    fi

    printf "* \"gcc\": OK\n"

    if ! which python3 >/dev/null
    then
        printf "* ERROR: Missing \"python3\"!\n"
        exit 1
    fi

    printf "* \"python3\": OK\n"

    if [ ! -f "./install.sh" ]
    then
        printf "* ERROR: Missing \"install.sh\"!\n"
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
        exit 1
    fi

    printf "\n* \"make\": OK\n"

################################################################
# Calculate extension ID for "ID_CHROMIUM.txt"

    printf "\nCalculating Chromium extension ID...\n\n"

    if ! python3 -c "import os, hashlib; print(''.join([chr(int(x, base=16) + ord('a')) for x in hashlib.sha256(os.path.abspath('../extension/chromium').encode('utf-8')).hexdigest()[:32]]))" 1>ID_CHROMIUM.txt
    then
        exit 1
    fi

    printf "* \"ID_CHROMIUM.txt\": OK\n"

################################################################
# Run the installation script with special argument

    printf "\nRunning \"install.sh -all\"...\n\n"

    chmod u+x ./install.sh
    if ! sh ./install.sh -all
    then
        exit 1
    fi

    exit 0

################################################################
