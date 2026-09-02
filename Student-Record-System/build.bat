@echo off
REM Build script for MinGW64 (run from VS Code terminal, in this folder)
gcc gui.c student.c fileio.c auth.c -o StudentRecordSystem.exe -lcomctl32 -mwindows
if %errorlevel%==0 (
    echo Build succeeded: StudentRecordSystem.exe
) else (
    echo Build FAILED - see errors above.
)
pause
