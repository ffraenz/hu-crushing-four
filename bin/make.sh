#!/bin/bash

# Build target
gcc -o loesung -O3 -std=c11 -Wall -Werror -DNDEBUG src/main.c
