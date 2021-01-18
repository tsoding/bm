@echo off
rem launch this from msvc-enabled console

set CFLAGS=/std:c11 /O2 /FC /W4 /WX /wd4996 /nologo
set LIBS=

mkdir build
mkdir build\bin
mkdir build\examples

cl.exe %CFLAGS% .\src\basm.c /Fe.\build\bin\basm.exe
cl.exe %CFLAGS% .\src\bme.c /Fe.\build\bin\bme.exe
cl.exe %CFLAGS% .\src\debasm.c /Fe.\build\bin\debasm.exe
cl.exe %CFLAGS% .\src\bdb.c /Fe.\build\bin\bdb.exe
cl.exe %CFLAGS% .\src\basm2nasm.c /Fe.\build\bin\basm2nasm.exe

if "%1" == "examples" setlocal EnableDelayedExpansion && for /F "tokens=*" %%e in ('dir /b .\examples\*.basm') do (
    set name=%%e
    ".\build\bin\basm.exe" -g .\examples\%%e .\build\examples\!name:~0,-4!bm
)

dir
cd build
dir
cd bin
dir
cd ..\examples
dir
