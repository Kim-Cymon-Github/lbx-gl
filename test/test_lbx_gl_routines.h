#ifndef test_lbx_gl_routinesH
#define test_lbx_gl_routinesH

#include "lbx_type.h"
#include "gl/lbx_gl.h"
#include "intf/lbx_intf.h"

#if defined(_MSC_VER) || defined(__BORLANDC__)
#pragma comment (lib, "lbx-intf")
#endif //#if defined(_MSC_VER) || defined(__BORLANDC__)

i32_t test(void)
{
#ifdef _WIN32
    const char plat_drv[] = "plat-glwin";
    system("chcp 65001"); // 콘솔의 Code Page를 UTF-8로 변경
    SetProcessDPIAware(); // 윈도우 스케일 무시
#else
    const char plat_drv[] = "plat-glfb";
#endif //#ifdef _WIN32

    LBX_PLATFORM_DRIVER pd = LBX_PLATFORM_DRIVER_(NULL);
    if (LBX_PLATFORM_DRIVER_Open(&pd, plat_drv, "lbx_plat_entry", NULL) != LBX_NO_ERROR) {
        return -1;
    }

    LBX_WINDOW win;
    LBX_WINDOW_Init(&win, fourcc_from_str("win0"), NULL, NULL);
    win.desc = ustr_("/dev/fb0");
    win.area = xywh_f32_(0.0f, 0.0f, 1280.0f, 720.0f);
    pd.OpenWindow(&win, "/dev/fb0", NULL);

    LBX_RENDER_CONTEXT* rc = new LBX_RENDER_CONTEXT();
    LBX_RENDER_CONTEXT_Init(rc, (EGLNativeWindowType)win.framework_window_handle);
    RC_Init(rc, NULL, NULL);

    pd.CloseWindow(&win);
    LBX_WINDOW_Free(&win);

    RC_Free(rc);
    delete rc;
    LBX_PLATFORM_DRIVER_Close(&pd, NULL);

    return 0;
}

#endif //#ifndef test_lbx_gl_routinesH

