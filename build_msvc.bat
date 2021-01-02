@echo off
rem launch this from msvc-enabled console


set CFLAGS=/std:c11 /O2 /FC /W4 /WX /wd4996 /nologo
set LIBS=

for /f "delims=" %%i in ('findstr "EXAMPLES=" "Makefile"') do (
    set result=%%i
)
set EXAMPLES=%result:~9%

cl.exe %CFLAGS% ./src/basm.c

cl.exe %CFLAGS% ./src/bme.c

cl.exe %CFLAGS% ./src/debasm.c

if "%1" == "examples" setlocal EnableDelayedExpansion && for %%e in (%EXAMPLES%) do (
    set name=%%e
    "./basm.exe" !name:~0,-2!basm %%e
)
