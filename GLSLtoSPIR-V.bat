@echo off
setlocal enabledelayedexpansion

:: Get argument from PowerShell
set "root=%~1"

:: Check if argument is provided (default root: "shaders")
if "%root%"=="" (
    set "root=shaders"
)

:: Get list of sub-directories
for /d %%i in (%root%\*) do (
    set "subdir=%%i"
    
    :: Compile .vert files
    for %%j in (!subdir!\*.vert) do (
        glslc.exe %%j -o !subdir!\%%~nj_vert.spv
    )

    :: Compile .frag files
    for %%j in (!subdir!\*.frag) do (
        glslc.exe %%j -o !subdir!\%%~nj_frag.spv
    )
)

echo Compilation completed successfully.
exit /b 0
