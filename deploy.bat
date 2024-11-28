@echo off
setlocal EnableDelayedExpansion
chcp 65001
cls
:: version 파일 경로 설정
set "VERSION_FILE=src\version.txt"
:: 현재 스크립트가 위치한 디렉토리의 경로 취득
set "SCRIPT_PATH=%~dp0"
echo %SCRIPT_PATH%
:: 디렉토리 경로에서 마지막 디렉토리 이름을 추출, MODULE_NAME으로 정함
for %%I in ("%SCRIPT_PATH%.") do set "MODULE_NAME=%%~nxI"

:: include 폴더에 서브디렉토리를 만드는 경우 이런 식으로 생성
set "SUB_DIR=%MODULE_NAME:*lbx-=%%"

:: 상태 표시
echo ### Deploying %MODULE_NAME% ... (SubDir: %SUB_DIR%)

:: include 폴더에 서브디렉토리 생성
if not exist "lib\lbx\include\%SUB_DIR%\"       mkdir "lib\lbx\include\%SUB_DIR%\"

:: 헤더파일 복사 (저장소마다 다름)
attrib /S -R "lib\lbx\include\*"
copy src\*.h "lib\lbx\include\%SUB_DIR%\"
attrib /S +R "lib\lbx\include\*"

:: LBX 경로로 이동
pushd lib\lbx

:: 최신 LBX로 업데이트함
git pull

:: 변경내용 반영
git add "include/%SUB_DIR%/"
git add lib/

:: git status로 현재 상태 점검
git status | findstr /C:"nothing to commit, working tree clean" /C:"nothing added to commit" /C:"no changes added to commit"> nul
set "GIT_RESULT=%errorlevel%"

if %GIT_RESULT% equ 0 (
    :: 변경 내용 없음
    echo ### Nothing to deploy
) else (
    :: 변경 내용이 감지되므로 일단 원래 디렉토리로 이동, 현재 버전 정보 취득 및 갱신(VERSION_PATCH 증가)
    popd
    for /f "tokens=1,2 delims=: " %%i in ('python3 lib\lbx\tool\ver_man.py "%VERSION_FILE%" VERSION_PATCH') do (
        set "%%i=%%j"
    )
    :: 커밋 메시지 생성
    set "COMMIT_MESSAGE=%MODULE_NAME% !VERSION_MAJOR!.!VERSION_MINOR!.!VERSION_PATCH! build !BUILD_NUMBER! 배포"

    :: LBX 경로로 이동하여 commit 후 push
    pushd lib\lbx
    if not "!COMMIT_MESSAGE!" == "" (
        git commit -m "!COMMIT_MESSAGE!"
        git push
        echo !COMMIT_MESSAGE!
    )
)
popd

@echo on
