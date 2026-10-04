#!/usr/bin/env bash

BIN=./build/minijudge
SRC=examples/ac_cpubound.cpp

for j in 1 2 4 8 12 16 20; do
    echo "===== jobs=$j ====="

    for run in 1 2 3 4 5; do
        "$BIN" -j "$j" "$SRC" > /dev/null
    done

    echo
done
