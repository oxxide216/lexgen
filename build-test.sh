#!/usr/bin/sh

CFLAGS="-Wall -Wextra"
SRC="$(find test-src -name "*.c")"
LIB_SRC="$(find lib-src -name "*.c")"

cc -o test $CFLAGS "${@:1}" $SRC $LIB_SRC
