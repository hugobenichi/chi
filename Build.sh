#!/bin/bash

make 77>&1
#make run 77>&1

gcc -g -o build/nav src/navigation.c -I./src
