@echo off
REM Copies the freshly built cheat DLL next to the loader resources.
REM (MSBuild pre-build event does the same automatically.)
copy /Y "..\build\RDOTerminator.dll" "res\RDOTerminator.dll"
if errorlevel 1 (
    echo Build RDOTerminator.dll first (repo root).
    pause
)
