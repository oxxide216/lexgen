#!/usr/bin/bash

CFLAGS="-Wall -Wextra -Iinclude -Ilibs"
SRC="$(find src/gen -name "*.c")"

cc -o lexgen $CFLAGS "${@:1}" $SRC
