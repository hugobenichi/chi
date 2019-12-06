#!/bin/bash

make 77>&1
#make run 77>&1

gcc -o1 -fsanitize=address -fno-omit-frame-pointer -g -o build/nav src/navigation.c -I./src
