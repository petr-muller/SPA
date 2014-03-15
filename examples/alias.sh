#!/usr/bin/env bash

if [ -z $1 ]; then
    file=test1;
else
    file=$1;
fi
echo
echo
echo
echo
echo ---Source---
echo
cat $file.c
echo
echo ---Compilation---
echo
clang -g3 -gcolumn-info -emit-llvm -c -o $file.bc $file.c -O0
echo
echo ---LLVM code---
echo
llvm-dis $file.bc -o -
echo
echo ---Alias Analysis---
echo
opt -disable-output -basicaa --aa-eval -print-all-alias-modref-info $file.bc
