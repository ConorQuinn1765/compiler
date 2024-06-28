#!/bin/bash

clean() {
    make clean
    rm *.s a.out
}

if [ $# -eq 0 ]
then
    make
    ./parse test.bm > test.s
    gcc -Wall -Werror -pedantic test.s lib/*.c -g -lX11
elif declare -f "$1" > /dev/null
then
    "$@"
else
    echo "$1 is not a known command" >&2
    exit 1
fi
