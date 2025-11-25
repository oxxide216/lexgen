#!/usr/bin/bash

CFLAGS="-Wall -Wextra -Ilibs"
SRC="$(find gen-src -name "*.c")"

cc -o lexgen $CFLAGS "${@:1}" $SRC
