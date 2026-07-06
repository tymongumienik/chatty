#!/usr/bin/env bash
set -e
mkdir -p build
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
