@echo off

cmake -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DINCLUDE_TEST=OFF -B build
