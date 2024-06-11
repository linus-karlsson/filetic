@echo off

del .\build\CMakeFiles\FileTic.dir\src\*.obj
del .\build\CMakeFiles\FileTic.dir\src\platform\windows\*.obj
call cmake --build build
