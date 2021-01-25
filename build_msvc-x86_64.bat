@echo off

call "build_msvc.bat"

if not exist build\examples\win mkdir build\examples\win

for %%e in (123i fib chars hello) do (

    .\build\bin\basm2nasm.exe -fwin64 .\examples\%%e.basm > .\build\examples\win\%%e.asm
    
    nasm.exe -fwin64 -g .\build\examples\win\%%e.asm -o .\build\examples\win\%%e.obj

    rem MSVC linker:
    link.exe /ENTRY:_start /SUBSYSTEM:CONSOLE /MACHINE:X64 ^
        /defaultlib:Kernel32.lib /LargeAddressAware:NO /nologo ^
        /OUT:.\build\examples\win\%%e.exe .\build\examples\win\%%e.obj

    rem MinGW64 linker:
    rem ld.exe -entry=_start --subsystem=console -o .\build\examples\win\%%e.exe ^
        rem .\build\examples\win\%%e.obj %SystemRoot%\System32\kernel32.dll

    rem Go linker (http://www.godevtool.com/#linker):
    rem GoLink.exe /entry _start /console /o .\build\examples\win\%%e.exe ^
        rem .\build\examples\win\%%e.obj %SystemRoot%\System32\kernel32.dll
)
