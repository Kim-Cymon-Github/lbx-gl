//---------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif //#ifdef __BORLANDC__

#include "lbx_gl.h"
#include "lbx_core.h"
#include "system/lbx_log.h"

//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#pragma package(smart_init)
#endif //#ifdef __BORLANDC__

#ifdef GLES
    #ifdef IS_WINDOWS
        #pragma comment (lib, "libEGL")
        #pragma comment (lib, "libGLESv2")  // 이유는 알 수 없으나 직접 프로젝트에 추가해야 정상 동작함
    #endif //#ifdef IS_WINDOWS
    #if GLES == 20
        #define LB_RENDERABLE_TYPE EGL_OPENGL_ES2_BIT
    #elif GLES >= 30 //#if GLES == 20
        #define LB_RENDERABLE_TYPE EGL_OPENGL_ES3_BIT
    #endif //#elif GLES >= 30 #if GLES == 20
#endif


int gles_instances = 0;

typedef struct {
    GLenum code;
    const char * desc;
} LBGL_CODE_DESC;

const LBGL_CODE_DESC glerrorstrings[] = {
    {GL_NO_ERROR, "GL_NO_ERROR"},
    {GL_INVALID_ENUM, "GL_INVALID_ENUM"},
    {GL_INVALID_VALUE, "GL_INVALID_VALUE"},
    {GL_INVALID_OPERATION, "GL_INVALID_OPERATION"},
    {GL_INVALID_FRAMEBUFFER_OPERATION, "GL_INVALID_FRAMEBUFFER_OPERATION"},
    {GL_OUT_OF_MEMORY, "GL_OUT_OF_MEMORY"},
};

typedef struct {
    EGLint code;
    const char * desc;
} LBEGL_CODE_DESC;

const LBEGL_CODE_DESC eglerrorstrings[] = {
    {EGL_SUCCESS, "EGL_SUCCESS"},
    {EGL_NOT_INITIALIZED, "EGL_NOT_INITIALIZED"},
    {EGL_BAD_ACCESS, "EGL_BAD_ACCESS"},
    {EGL_BAD_ALLOC, "EGL_BAD_ALLOC"},
    {EGL_BAD_ATTRIBUTE, "EGL_BAD_ATTRIBUTE"},
    {EGL_BAD_CONTEXT, "EGL_BAD_CONTEXT"},
    {EGL_BAD_CONFIG, "EGL_BAD_CONFIG"},
    {EGL_BAD_CURRENT_SURFACE, "EGL_BAD_CURRENT_SURFACE"},
    {EGL_BAD_DISPLAY, "EGL_BAD_DISPLAY"},
    {EGL_BAD_SURFACE, "EGL_BAD_SURFACE"},
    {EGL_BAD_MATCH, "EGL_BAD_MATCH"},
    {EGL_BAD_PARAMETER, "EGL_BAD_PARAMETER"},
    {EGL_BAD_NATIVE_PIXMAP, "EGL_BAD_NATIVE_PIXMAP"},
    {EGL_BAD_NATIVE_WINDOW, "EGL_BAD_NATIVE_WINDOW"},
    {EGL_CONTEXT_LOST, "EGL_CONTEXT_LOST"},
};

const char * lbglGetErrorStr(GLenum error_code)
{
    const char *ret = NULL;
    int i;
    for (i = sizeof(glerrorstrings) / sizeof(LBGL_CODE_DESC) - 1; i >= 0; --i) {
        if (glerrorstrings[i].code == error_code) {
            ret = glerrorstrings[i].desc;
            break;
        }
    }
    return ret;
}
const char * lbeglGetErrorStr(EGLint error_code)
{
    const char *ret = NULL;
    int i;
    for (i = sizeof(eglerrorstrings) / sizeof(LBGL_CODE_DESC) - 1; i >= 0; --i) {
        if (eglerrorstrings[i].code == error_code) {
            ret = eglerrorstrings[i].desc;
            break;
        }
    }
    return ret;
}

/*
static void * lbglSetWindowStyles(NativeWindowType h_wnd)
{
#ifdef _WIN32
    LONG_PTR lpStyle = GetWindowLongPtrW(h_wnd, GWL_STYLE);
    SetWindowLongPtrW(h_wnd, GWL_STYLE, lpStyle | CS_HREDRAW | CS_VREDRAW | CS_OWNDC);
    return (void *)lpStyle;
#else //#ifdef _WIN32
    return NULL;
#endif // #else #ifdef _WIN32
}
*/
int RC_Init(LBX_RENDER_CONTEXT *ctx, EGLNativeWindowType native_window,
    NativeDisplayType native_display, EGLContext context, EGLint const * attribList)
{
    EGLint selected_config = -1;
    EGLint numConfigs = 0;
    EGLint majorVersion = 0;
    EGLint minorVersion = 0;
    EGLDisplay display = 0;
    EGLSurface surface = EGL_NO_SURFACE;

    EGLint const default_egl_config_attribs[] = {

        EGL_BUFFER_SIZE,             EGL_DONT_CARE,
        EGL_RED_SIZE,                8,
        EGL_GREEN_SIZE,              8,
        EGL_BLUE_SIZE,               8,
        EGL_ALPHA_SIZE,              8,
        EGL_COLOR_BUFFER_TYPE,       EGL_RGB_BUFFER,
        EGL_RENDERABLE_TYPE,         LB_RENDERABLE_TYPE,     //
//		EGL_CONFIG_CAVEAT,           EGL_DONT_CARE,
//		EGL_CONFIG_ID,               EGL_DONT_CARE,

        EGL_DEPTH_SIZE,              16,

        EGL_LEVEL,                   0,
        EGL_MAX_SWAP_INTERVAL,       EGL_DONT_CARE,
        EGL_MIN_SWAP_INTERVAL,       EGL_DONT_CARE,
        EGL_NATIVE_RENDERABLE,       EGL_DONT_CARE,
//		EGL_NATIVE_VISUAL_TYPE,      EGL_DONT_CARE, // MALI Emulator에서 오류 발생
        EGL_SAMPLE_BUFFERS,          1, //0, // 1
        EGL_SAMPLES,                 4, //0, // 4
        EGL_STENCIL_SIZE,            0, //0
        EGL_SURFACE_TYPE,            EGL_WINDOW_BIT, //EGL_PIXMAP_BIT,
        EGL_TRANSPARENT_TYPE,        EGL_NONE,
        EGL_TRANSPARENT_RED_VALUE,   EGL_DONT_CARE,
        EGL_TRANSPARENT_GREEN_VALUE, EGL_DONT_CARE,
        EGL_TRANSPARENT_BLUE_VALUE,  EGL_DONT_CARE,
        EGL_NONE,                    EGL_NONE
    };
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
    EGLConfig *configs = NULL;
    bool is_first = (interlocked_increment(&gles_instances) == 1);

#ifdef GLES
    Log_("Initializing OpenGL ES %d.%d", GLES/10, GLES%10);
#endif //ifdef GLES
    Log_("  native_window=0x%X, display=0x%X", native_window, native_display );

    if (ctx == NULL) {
        Err_("ctx cannot be NULL");
    }
    ctx->flags = 0;
    ctx->native_window = native_window;
//	lbglSetWindowStyles(native_window);
    ctx->native_display = native_display;
    if (native_display == 0) {
#ifdef _WIN32
        //		native_display = GetDC(native_window);

        static NativeDisplayType g_native_display = 0;
        if (is_first) {
            g_native_display = GetDC(NULL);
        }
        ctx->native_display = g_native_display;
        if (ctx->native_display == NULL) {
            Err_("Cannot get DC");
        } else {
            ctx->flags |= RC_FLAG_OWNS_NATIVE_DISPLAY;
        }
#else  //#ifdef _WIN32
        ctx->native_display = EGL_DEFAULT_DISPLAY;
#endif //#else #ifdef _WIN32
    }
    display = eglGetDisplay(ctx->native_display);

    if (EGL_NO_DISPLAY != display) {
        Log_("egl_display = %p", display);
        ctx->egl_display = display;
    } else {
        Err_("eglGetDisplay(%p) returned EGL_NO_DISPLAY", ctx->native_display);
        return 0;
    }

    if (is_first) {
        // Initialize EGL
        if (EGL_TRUE == eglInitialize(display, &majorVersion, &minorVersion)) {
            Log_("EGL %d.%d initialized successfully", majorVersion, minorVersion);
        } else {
            Err_("eglInitialize failed: %s", lbeglGetErrorStr(eglGetError()));
            return 0;
        }
    }

    // Get configs
    if (EGL_TRUE == eglGetConfigs(display, NULL, 0, &numConfigs)) {
        Log_("%d configs are available", numConfigs);
    } else {
        Err_("eglGetConfigs failed: %s", lbeglGetErrorStr(eglGetError()));
        return 0;
    }

    configs = (EGLConfig*)malloc(sizeof(EGLConfig) * numConfigs);

    // Choose config
    if (NULL == attribList) {
        // 기본 설정 사용
        attribList = default_egl_config_attribs;
    }

    if (EGL_FALSE == eglChooseConfig(display, attribList, configs, numConfigs, &numConfigs) ) {
        Err_("eglChooseConfig failed: %s", lbeglGetErrorStr(eglGetError()));
        return 0;
    } else {
        Log_("%d configs are matching", numConfigs);
    }


    for (selected_config = 0; selected_config < numConfigs; selected_config++) {
        // Create a surface
        surface = eglCreateWindowSurface(display, configs[selected_config], native_window, NULL);
        if (EGL_NO_SURFACE != surface) {
            ctx->egl_surface = surface;
            Log_("EGL surface %p created based on EGLConfig %d (idx=%d)", surface, configs[selected_config], selected_config);
            break;
        } else {
            Log_("EGLConfig %d failed - %s", selected_config, lbeglGetErrorStr(eglGetError()));
        }
    }

    if (EGL_NO_SURFACE == surface) {
        Err_("eglCreateWindowSurface failed: %s", lbeglGetErrorStr(eglGetError()));
        return 0;
    }

    // Create a GL context
    context = eglCreateContext(display, configs[selected_config], context, contextAttribs );
    if ( context != EGL_NO_CONTEXT ) {
        ctx->egl_context = context;
        Log_("EGL context created: 0x%x", context);
    } else {
        Err_("eglCreateContext failed: %s", lbeglGetErrorStr(eglGetError()));
        return 0;
    }

//	Log_("freeing configs");
    free(configs);


//	Log_("eglMakeCurrent");
    // Make the context current
    if ( !eglMakeCurrent(display, surface, surface, context)) {
        Err_("eglMakeCurrent failed: %s", lbeglGetErrorStr(eglGetError()));
        return 0;
    }

    if (is_first) {
        #ifdef MAX_PERFORMANCE_CHECK
            #define LBX_SWAP_INTERVAL 0
        #else //#ifdef MAX_PERFORMANCE_CHECK
            #define LBX_SWAP_INTERVAL 1
        #endif //#else #ifdef MAX_PERFORMANCE_CHECK

    //	Log_("eglSwapInterval(%d)", LBX_SWAP_INTERVAL);
        eglSwapInterval(display, LBX_SWAP_INTERVAL);
    }



//	glClearColor(1,0,0,1);
//	glClear(GL_COLOR_BUFFER_BIT);
//	RC_SwapBuffers(ctx);

    return 1;
}


int RC_Free(LBX_RENDER_CONTEXT *ctx)
{
    int ret = 0;
//	if (EGL_NO_DISPLAY != ctx->egl_display) {
//		if (!RC_MakeCurrent(ctx)) {
//			Err_("eglMakeCurrent failed");
//		}
    ret = interlocked_decrement(&gles_instances);
    
    if (EGL_NO_CONTEXT != ctx->egl_context) {
        if (EGL_TRUE == eglDestroyContext(ctx->egl_display, ctx->egl_context)) {
            ctx->egl_context = EGL_NO_CONTEXT;
        } else {
            Err_("eglDestroyContext failed: %s", lbeglGetErrorStr(eglGetError()));
        }
    }

    if (EGL_NO_SURFACE != ctx->egl_surface) {
        if (EGL_TRUE == eglDestroySurface(ctx->egl_display, ctx->egl_surface)) {
            ctx->egl_surface = EGL_NO_SURFACE;
        } else {
            Err_("eglDestroySurface failed: %s", lbeglGetErrorStr(eglGetError()));
        }
    }

    if (0 == ret) {
        if (EGL_TRUE == eglTerminate(ctx->egl_display)) {
            ctx->egl_display = EGL_NO_DISPLAY;
        } else {
            Err_("eglTerminate failed: %s", lbeglGetErrorStr(eglGetError()));
        }

#ifdef _WIN32
        if (ctx->flags & RC_FLAG_OWNS_NATIVE_DISPLAY) {
            ctx->flags &= ~RC_FLAG_OWNS_NATIVE_DISPLAY;
            ReleaseDC(NULL, ctx->native_display);
        }
#endif //#ifdef _WIN32
    }

    Log_("RC_Free done");
//	}
    // 현재 Render Context Free 시 남은 인스턴스 갯수를 반환
    // 멀티윈도우 대응 및 싱글 윈도우 대응 코드
    return ret;
}


