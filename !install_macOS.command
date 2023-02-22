#!/bin/sh
(
    cd -- "$(dirname -- "$0")"
    sh "install/autobuild.sh"
    printf "\n(press Enter to close the terminal)\n"
    read -r _
)
