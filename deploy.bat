@echo off
chcp 65001
@echo off
cls
rem 현재 스크립트가 위치한 디렉토리의 경로 취득
set "SCRIPT_PATH=%~dp0"
echo %SCRIPT_PATH%
rem 디렉토리 경로에서 마지막 디렉토리 이름을 추출
for %%I in ("%SCRIPT_PATH%.") do set "MODULE_NAME=%%~nxI"

set "SUB_DIR=%MODULE_NAME:*lbx-=%%"

echo ### Deploying %MODULE_NAME% ... (SubDir: %SUB_DIR%)
rem 전달된 파일이 존재하는지 확인합니다.
set "VERSION_FILE=build\src\version.h"
if exist "%VERSION_FILE%" (
    for /f "tokens=3" %%i in ('findstr /C:"define VERSION_MAJOR" "%VERSION_FILE%"') do (
        set "VERSION_MAJOR=%%i"
    )
    for /f "tokens=3" %%i in ('findstr /C:"define VERSION_MINOR" "%VERSION_FILE%"') do (
        set "VERSION_MINOR=%%i"
    )
    for /f "tokens=3" %%i in ('findstr /C:"define VERSION_PATCH" "%VERSION_FILE%"') do (
        set "VERSION_PATCH=%%i"
    )
    for /f "tokens=3" %%i in ('findstr /C:"define BUILD_NUMBER" "%VERSION_FILE%"') do (
        set "BUILD_NUMBER=%%i"
    )
)

set "COMMIT_MESSAGE=%MODULE_NAME% %VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH% build %BUILD_NUMBER% 배포"

if not exist "lib\lbx\include\%SUB_DIR%\"       mkdir "lib\lbx\include\%SUB_DIR%\"
attrib /S -R "lib\lbx\include\*"
copy src\*.h "lib\lbx\include\%SUB_DIR%\"
attrib /S +R "lib\lbx\include\*"

pushd lib\lbx
git pull
git add "include/%SUB_DIR%/*"
git add lib/*

git status | findstr /C:"nothing to commit, working tree clean" > nul
set "GIT_RESULT=%errorlevel%"
if not %GIT_RESULT% equ 0 (
    rem 변경 내용이 감지됨
    if not COMMIT_MESSAGE == "" (
        git commit -m "%COMMIT_MESSAGE%"
        git push
    ) 
)
popd

if %GIT_RESULT% equ 0 (
    echo ### Nothing to deploy
) else (
    lib\lbx\tools\inc VERSION_PATCH "%VERSION_FILE%"
    echo ### %COMMIT_MESSAGE%
)

@echo on
