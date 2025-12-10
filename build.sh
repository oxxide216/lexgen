#!/usr/bin/bash

CFLAGS="-Wall -Wextra -Iinclude -Ilibs"
SRC="$(find src/gen src/common -name "*.c")"

cc -o lexgen $CFLAGS "${@:1}" $SRC
