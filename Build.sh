#!/bin/bash

make 77>&1
#make run 77>&1

flags="-o1"
#flags=$flags" -g -p -no-pie"
flags=$flags" -g -p"
flags=$flags" -fsanitize=address -fno-omit-frame-pointer"

#CC=gcc
CC=g++
$CC $flags -o build/nav src/navigation.c -I./src
