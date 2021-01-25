@echo off

call "build_msvc.bat"

if not exist build\examples\win mkdir build\examples\win

for %%e in (123i fib chars hello) do (

    .\build\bin\basm2nasm.exe -fwin64 .\examples\%%e.basm > .\build\examples\%%e.asm
    
    nasm.exe -fwin64 -g .\build\examples\%%e.asm -o .\build\examples\win\%%e.obj
    
    ld.exe -entry=_start --subsystem=console -o .\build\examples\win\%%e.exe .\build\examples\win\%%e.obj %SystemRoot%\System32\kernel32.dll

    rem Instead of ld you can use MSVC linker:
    rem link.exe /ENTRY:_start /SUBSYSTEM:CONSOLE /MACHINE:X64 ^
        rem /defaultlib:Kernel32.lib /LargeAddressAware:NO /nologo ^
        rem /OUT:.\build\examples\%%e.exe .\build\examples\%%e.obj

    rem Instead of ld you can use Go linker (http://www.godevtool.com/#linker):
    rem GoLink.exe /entry _start /console /o .\build\examples\%%e.exe ^
        rem .\build\examples\%%e.obj %SystemRoot%\System32\kernel32.dll
)
