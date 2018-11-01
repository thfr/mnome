#!/bin/bash

mkdir -p build
pushd build
cmake ..
popd

for f in src/*
do
    clang-tidy -checks="*,-llvm-header-guard" -header-filter="src/" $f -p build -fix
done
