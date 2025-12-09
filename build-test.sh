#!/usr/bin/bash

CFLAGS="-Wall -Wextra -Iinclude -Ilibs"
SRC="$(find test-src -name "*.c")"
RUNTIME_SRC="$(find src/runtime -name "*.c")"

./lexgen test-src/grammar.h test-src/grammar.lg
cc -o test $CFLAGS "${@:1}" $SRC $RUNTIME_SRC
