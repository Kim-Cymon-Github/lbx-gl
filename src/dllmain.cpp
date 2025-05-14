// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN 
#include <windows.h>
#if defined(__BORLANDC__)
#   pragma hdrstop
#endif
#if defined(_MSC_VER) || defined(__BORLANDC__)
#   pragma comment (lib, "lbx-core")
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

