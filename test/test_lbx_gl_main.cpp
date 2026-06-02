#include <stdio.h>
#if defined(__BORLANDC__)
#   pragma hdrstop
#endif
// version.txt 를 lbx 헤더(→lbx_version.h) 보다 먼저 — Linux 산출물 버전 임베드 활성화
#define LBX_MODULE_NAME "lbx-gl-test"
#include "../src/version.txt"
#include "gl/lbx_gl_class.h"
#define IMPLEMENT_SIMPLE_CONSOLE_LOG_HANDLER
#include "system/lbx_log.h"
#include "system/lbx_console.h"
#include "lbx_core.h"
#include "system/lbx_file.h"

#include "test_lbx_gl_routines.h"

#if defined(_MSC_VER) || defined(__BORLANDC__)
#   pragma comment (lib, "lbx-core.lib")
#   pragma comment (lib, "lbx-intf.lib")
#   pragma comment (lib, "lbx-gl.lib")
#endif

// Linux 산출물에 버전 임베드 (Windows 는 .rc 담당). grep -az LBVERINFO <exe> | tr '\0' '\n'
LBX_EMBED_VERSION_INFO();

i32_t main(void)
{
#ifdef _WIN32
    system("chcp 65001");
#endif //#ifdef _WIN32
    tick_set_start(); // tick 초기화
    change_log_handler(&simple_console_log_handler); // 로그 핸들러 연결

    u32_t ver = lbx_gl_version();
    Info_("lbx-gl library function test (version " VERSION_VFMT ")", VERSION_VARG(ver));

    test();
}
