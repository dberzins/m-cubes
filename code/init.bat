@echo off

REM NOTE: Do "cl" compiler setup/configuration: 
if not exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    echo Please install Visual Studio Comunity 2022!
    goto :eof
)

call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
