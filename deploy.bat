@echo off
chcp 65001

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

set "commit_message=%VERSION_MAJOR%.%VERSION_MINOR%.%VERSION_PATCH% build %BUILD_NUMBER% 배포"

if not exist lib\lbx\include\gl\       mkdir lib\lbx\include\gl\
attrib -r /s /d lib\lbx\include\gl
copy src\*.h lib\lbx\include\gl\

pushd lib\lbx
git pull
git add include/gl/*
git add lib/*

if not commit_message == "" (
git commit -m "%commit_message%"
git push
)
popd
lib\lbx\tools\inc VERSION_PATCH "%VERSION_FILE%"
