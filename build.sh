#!/usr/bin/sh

CFLAGS="-Wall -Wextra -Ilibs"
SRC="$(find gen-src -name "*.c")"

cc -o lexgen $CFLAGS "${@:1}" $SRC
