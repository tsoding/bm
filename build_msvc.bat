@echo off
rem launch this from msvc-enabled console


set CFLAGS=/std:c11 /O2 /FC /W4 /WX /wd4996 /nologo
set LIBS=

cl.exe %CFLAGS% ./src/basm.c

cl.exe %CFLAGS% ./src/bme.c

cl.exe %CFLAGS% ./src/debasm.c

rem TODO(#82): bdb is not built on Windows

if "%1" == "examples" setlocal EnableDelayedExpansion && for /F "tokens=*" %%e in ('dir /b .\examples\*.basm') do (
    set name=%%e
    "./basm.exe" .\examples\%%e .\examples\!name:~0,-4!bm
)
