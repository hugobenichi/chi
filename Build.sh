#!/bin/bash

mkdir -p build/

make 77>&1
#make run 77>&1

flags="-o1"
#flags=$flags" -g -p -no-pie"
flags=$flags" -g -p"
flags=$flags" -fsanitize=address -fno-omit-frame-pointer"

CC=g++
$CC $flags -c -o build/base.o src/base.cpp -I./src
$CC $flags -c -o build/navigation.o src/navigation.cpp -I./src
$CC $flags -o build/nav build/{base,navigation}.o
