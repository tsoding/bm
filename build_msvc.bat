@echo off
rem launch this from msvc-enabled console

set CFLAGS=/std:c11 /O2 /FC /W4 /WX /wd4996 /nologo /Fe.\build\bin\ /Fo.\build\bin\
set LIBS=

if not exist build\bin mkdir build\bin
if not exist build\examples mkdir build\examples

cl.exe %CFLAGS% .\src\basm.c
cl.exe %CFLAGS% .\src\bme.c
cl.exe %CFLAGS% .\src\debasm.c
cl.exe %CFLAGS% .\src\bdb.c
cl.exe %CFLAGS% .\src\basm2nasm.c

setlocal EnableDelayedExpansion
for /F "tokens=*" %%e in ('dir /b .\examples\*.basm') do (
    set name=%%e
    ".\build\bin\basm.exe" -g .\examples\%%e .\build\examples\!name:~0,-4!bm
)
