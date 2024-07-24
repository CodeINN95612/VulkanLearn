@echo off
echo Generating Visual Studio 2022 project files...
external\premake5.exe vs2022
if %errorlevel% neq 0 (
    echo Failed to generate project files.
    pause
    exit /b %errorlevel%
)
echo Project files generated successfully.
pause