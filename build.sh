#!/usr/bin/sh

gcc $(find . -name "*.c") -Iinclude -g -o awc
