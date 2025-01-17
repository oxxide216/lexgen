#!/usr/bin/sh

CFLAGS="-Wall -Wextra"
SRC="$(find src -name "*.c")"

cc -o lexgen $CFLAGS "${@:1}" $SRC
