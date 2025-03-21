﻿#include <stdio.h>
#include "gl/lbx_gl_class.h"
#define IMPLEMENT_SIMPLE_CONSOLE_LOG_HANDLER
#include "system/lbx_log.h"
#include "system/lbx_console.h"
#include "lbx_core.h"
#include "system/lbx_file.h"

#include "lbx_gles_test_routines.h"

#pragma comment (lib, "lbx-gl.lib")
#pragma comment (lib, "lbx-core.lib")

i32_t main(void)
{
#ifdef _WIN32
    system("chcp 65001");
#endif //#ifdef _WIN32
    check_memory_usage(true); // reset
    tick_set_start(); // tick 초기화
    change_log_handler(&simple_console_log_handler); // 로그 핸들러 연결

    Log_("lbx-gl library function test");

    test();

    check_memory_usage(false); // reset
}
