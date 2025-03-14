@echo off
set "ORIGINAL_DIR=%CD%"
cd /d "%~dp0"
if exist "%~dp0\tool\lbx.py" (
    python "%~dp0\tool\lbx.py" %*
) else (
    python "%~dp0\lib\lbx\tool\lbx.py" %*
)
cd /d "%ORIGINAL_DIR%"