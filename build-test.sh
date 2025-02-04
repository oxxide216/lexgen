#!/usr/bin/sh

CFLAGS="-Wall -Wextra"
SRC="$(find test-src -name "*.c")"
RUNTIME_SRC="$(find runtime-src -name "*.c")"

./lexgen test-src/grammar.h test-src/grammar.lg
cc -o test $CFLAGS "${@:1}" $SRC $RUNTIME_SRC
