@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64 > NUL 2> NUL
cd code
setlocal

set INCLUDE=%INCLUDE%;..\include
set LIB=%LIB%;"..\lib"

set CompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -wd4244 -wd4838 -wd4456 -wd4576 -FC -Zi -EHsc /std:c++20
set IncludeFlags= -I ..\include  
set LinkerFlags= -incremental:no -opt:ref raylib.lib gdi32.lib user32.lib msvcrt.lib winmm.lib shell32.lib 

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL

cl %CompilerFlags% -D_CRT_SECURE_NO_WARNINGS /Fe:mcubes.exe ..\code\main.cpp -Fmmain.map /link %LinkerFlags%

popd

