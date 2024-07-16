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
        #define LBX_RENDERABLE_TYPE EGL_OPENGL_ES2_BIT
    #elif GLES >= 30 //#if GLES == 20
        #define LBX_RENDERABLE_TYPE EGL_OPENGL_ES3_BIT
    #endif //#elif GLES >= 30 #if GLES == 20
#endif

#ifdef _WIN32
static NativeDisplayType g_desktop_dc = 0;
#endif //#ifdef _WIN32

i32_t gles_instances = 0;

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
#ifdef GL_STACK_UNDERFLOW
	{GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW"},
#endif //#ifdef GL_STACK_UNDERFLOW
#ifdef GL_STACK_OVERFLOW
	{GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW"},
#endif //#ifdef GL_STACK_OVERFLOW
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
    i32_t i;
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
    i32_t i;
    for (i = sizeof(eglerrorstrings) / sizeof(LBGL_CODE_DESC) - 1; i >= 0; --i) {
        if (eglerrorstrings[i].code == error_code) {
            ret = eglerrorstrings[i].desc;
            break;
        }
    }
    return ret;
}

void LBX_RENDER_CONTEXT_Init(LBX_RENDER_CONTEXT* self, EGLNativeWindowType native_window)
{ 
    self->native_window = native_window;
    self->native_display = EGL_DEFAULT_DISPLAY;
    self->egl_display = EGL_NO_DISPLAY;
    self->egl_context = EGL_NO_CONTEXT;
    self->egl_surface = EGL_NO_SURFACE;
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

i32_t RC_Init(LBX_RENDER_CONTEXT *ctx, EGLContext context_to_share, EGLint const * attrib_list)
{
    EGLint selected_config = -1;
    EGLint numConfigs = 0;
    EGLint majorVersion = 0;
    EGLint minorVersion = 0;

    EGLint const default_egl_config_attribs[] = {

        EGL_BUFFER_SIZE,             EGL_DONT_CARE,
        EGL_RED_SIZE,                8,
        EGL_GREEN_SIZE,              8,
        EGL_BLUE_SIZE,               8,
        EGL_ALPHA_SIZE,              8,
        EGL_COLOR_BUFFER_TYPE,       EGL_RGB_BUFFER,
        EGL_RENDERABLE_TYPE,         LBX_RENDERABLE_TYPE,     //
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
    bool is_first = (interlocked_increment(&gles_instances) == 1);
    EGLint err = EGL_SUCCESS;

#ifdef GLES
    Log_("Initializing OpenGL ES %d.%d", GLES/10, GLES%10);
#endif //ifdef GLES

    // Log_("  native_window=0x%X, display=0x%X", native_window, native_display );

    if (ctx == NULL) {
        Err_("ctx cannot be NULL");
        return -101;
    }

    if (ctx->egl_display == EGL_NO_DISPLAY) {
        // 외부에서 egl_display를 제공하지 않았으므로 기본적인 방법으로 취득을 시도한다.
#ifdef _WIN32
		if (g_desktop_dc == 0) {
			g_desktop_dc = GetDC(NULL);
            if (NULL == g_desktop_dc) {
                Err_("GetDC(NULL) failed");
                return EGL_BAD_DISPLAY;
            }
        }
		if (EGL_DEFAULT_DISPLAY == ctx->native_display) { // 별도로 native_display를 지정하지 않았으면
        	ctx->native_display = g_desktop_dc; // 멀티윈도우 지원을 위해 EGL_DEFAULT_DISPLAY를 사용하지 않고 Desktop window를 사용함
        }
#endif //#ifdef _WIN32
        ctx->egl_display = eglGetDisplay(ctx->native_display);
        if (EGL_NO_DISPLAY != ctx->egl_display) {
            Log_("egl_display = %p", ctx->egl_display);
        } else {
            err = eglGetError();
            Err_("eglGetDisplay(%p) failed: %s", ctx->native_display, lbeglGetErrorStr(err));
            return err;
        }
    }

    if (is_first) {
        // Initialize EGL
        if (EGL_TRUE == eglInitialize(ctx->egl_display, &majorVersion, &minorVersion)) {
            Log_("EGL %d.%d initialized successfully", majorVersion, minorVersion);
        } else {
            err = eglGetError();
            Err_("eglInitialize() failed: %s", lbeglGetErrorStr(err));
            return err;
        }
    }

    if (err == EGL_SUCCESS && ctx->egl_surface == EGL_NO_SURFACE) {
        EGLConfig* configs = NULL;
        // Get configs
        if (EGL_TRUE == eglGetConfigs(ctx->egl_display, NULL, 0, &numConfigs)) {
            Log_("%d configs are available", numConfigs);
            configs = (EGLConfig*)alloc_memory(sizeof(EGLConfig) * numConfigs);
        } else {
            err = eglGetError();
            Err_("eglGetConfigs failed: %s", lbeglGetErrorStr(err));
        }

        if (err == EGL_SUCCESS) {
            // Choose config
            if (NULL == attrib_list) {
                // 기본 설정 사용
                attrib_list = default_egl_config_attribs;
            }

            if (EGL_FALSE == eglChooseConfig(ctx->egl_display, attrib_list, configs, numConfigs, &numConfigs)) {
                err = eglGetError();
                Err_("eglChooseConfig failed: %s", lbeglGetErrorStr(err));
            } else {
                Log_("%d configs are matching", numConfigs);
            }
        }

        if (err == EGL_SUCCESS && configs) {
            for (selected_config = 0; selected_config < numConfigs; selected_config++) {
                // Create a surface
                ctx->egl_surface = eglCreateWindowSurface(ctx->egl_display, configs[selected_config], ctx->native_window, NULL);
                err = eglGetError();
                if (EGL_NO_SURFACE != ctx->egl_surface) {
                    Log_("EGL surface %p created based on EGLConfig %d (idx=%d)", ctx->egl_surface, configs[selected_config], selected_config);
                    break;
                } else {
                    Log_("  EGLConfig %d failed - %s", selected_config, lbeglGetErrorStr(err));
                }
            }

            if (EGL_NO_SURFACE == ctx->egl_surface) {
                Err_("eglCreateWindowSurface() failed: %s", lbeglGetErrorStr(err));
            }
        }

        // Create a GL context
        if (err == EGL_SUCCESS && configs) {
            ctx->egl_context = eglCreateContext(ctx->egl_display, configs[selected_config], context_to_share, contextAttribs);
            if (EGL_NO_CONTEXT != ctx->egl_context) {
                Log_("EGL context created: 0x%x", ctx->egl_context);
            } else {
                err = eglGetError();
                Err_("eglCreateContext failed: %s", lbeglGetErrorStr(err));
            }
        }
        free_memory(configs);
    }


//	Log_("eglMakeCurrent");
    // Make the context current
    if (err == EGL_SUCCESS) {
        if (!RC_MakeCurrent(ctx)) {
            err = eglGetError();
            Err_("eglMakeCurrent failed: %s", lbeglGetErrorStr(err));
        }
    }

    if (err == EGL_SUCCESS && is_first) {
        #ifdef MAX_PERFORMANCE_CHECK
            #define LBX_SWAP_INTERVAL 0
        #else //#ifdef MAX_PERFORMANCE_CHECK
            #define LBX_SWAP_INTERVAL 1
        #endif //#else #ifdef MAX_PERFORMANCE_CHECK

    //	Log_("eglSwapInterval(%d)", LBX_SWAP_INTERVAL);
        if (!eglSwapInterval(ctx->egl_display, LBX_SWAP_INTERVAL)) {
            err = eglGetError();
            Err_("eglSwapInterval(%d) failed: %s", LBX_SWAP_INTERVAL, lbeglGetErrorStr(err));
        }
        
    }



//	glClearColor(1,0,0,1);
//	glClear(GL_COLOR_BUFFER_BIT);
//	RC_SwapBuffers(ctx);

    return err;
}


i32_t RC_Free(LBX_RENDER_CONTEXT *ctx)
{
    i32_t ret = 0;
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
		if (g_desktop_dc != 0) {
    		ReleaseDC(NULL, g_desktop_dc);
		}
#endif //#ifdef _WIN32
    }

    Log_("RC_Free done");
//	}
    // 현재 Render Context Free 시 남은 인스턴스 갯수를 반환
    // 멀티윈도우 대응 및 싱글 윈도우 대응 코드
    return ret;
}
