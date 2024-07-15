@echo off

cd test
cmake -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -B build
