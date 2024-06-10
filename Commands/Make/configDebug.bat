@echo off

cmake -S . 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug -DINCLUDE_TEST=OFF -B build
