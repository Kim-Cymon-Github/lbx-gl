//---------------------------------------------------------------------------

#ifndef lbx_glH
#define lbx_glH
//---------------------------------------------------------------------------

#include "lbx.h"

#ifndef GLES
#   if defined(GLES_1_0)
#       define GLES 10
#   elif defined(GLES_1_1)
#       define GLES 11
#   elif defined(GLES_2_0)
#       define GLES 20
#   elif defined(GLES_3_0)
#       define GLES 30
#   elif defined(GLES_3_1)
#       define GLES 31
#   elif defined(GLES_3_2)
#       define GLES 32
#   endif
#endif //#ifndef GLES


#ifdef GLES
#   include <EGL/egl.h>
#   include <EGL/eglext.h>
#   if GLES < 20 // GLES 1.x
#       include <GLES/gl.h>
#   elif GLES < 30 // GLES 2.x
#       include <GLES2/gl2.h>
#   else //#elif GLES < 30 // GLES 3.x
#       include <GLES3/gl3.h>
#       if GLES >= 32 // GLES 3.2
#           include <GLES3/gl32.h>
#       elif GLES >= 31 //#if GLES >= 32
#           include <GLES3/gl31.h> // GLES 3.1
#       endif
#   endif //#else #elif GLES < 30
#   if GLES >= 20
#        include <GLES2/gl2ext.h>
#   endif
#endif //#ifdef GLES

#ifdef _WIN32
#   ifdef lbx_gl_DLL
#       define LBX_GL_EXPORT __declspec(dllexport)
#   else //#ifdef lbx_gl_DLL
#       define LBX_GL_EXPORT __declspec(dllimport)
#   endif //#else #ifdef lbx_gl_DLL
#else //#ifdef _WIN32
#   define LBX_GL_EXPORT
#endif //#else #ifdef _WIN32



#ifdef __cplusplus
extern "C" {
#endif //#ifdef __cplusplus

#define RC_FLAG_OWNS_NATIVE_WINDOW (0x00000001u)
#define RC_FLAG_OWNS_NATIVE_DISPLAY (0x00000002u)

typedef struct  {
    uint32_t flags;
    EGLNativeWindowType native_window;
    NativeDisplayType   native_display;
    EGLDisplay          egl_display;
    EGLContext          egl_context;
    EGLSurface          egl_surface;
} LBX_RENDER_CONTEXT;
static inline void RENDER_CONTEXT_Init(LBX_RENDER_CONTEXT *ctx) {memset(ctx, 0, sizeof(LBX_RENDER_CONTEXT));}

LBX_GL_EXPORT int RC_Init(LBX_RENDER_CONTEXT *ctx, EGLNativeWindowType native_window, NativeDisplayType native_display, EGLContext context_to_share, EGLint const  * attribList);
LBX_GL_EXPORT int RC_Free(LBX_RENDER_CONTEXT *ctx);

static inline bool8_t RC_MakeCurrent(const LBX_RENDER_CONTEXT *ctx) {return (bool8_t)eglMakeCurrent(ctx->egl_display, ctx->egl_surface, ctx->egl_surface, ctx->egl_context);}
static inline bool8_t RC_SwapBuffers(const LBX_RENDER_CONTEXT *ctx) {return (bool8_t)eglSwapBuffers(ctx->egl_display, ctx->egl_surface);}

LBX_GL_EXPORT const char * lbglGetErrorStr(GLenum error_code);
LBX_GL_EXPORT const char * lbeglGetErrorStr(EGLint error_code);

#if defined(ENABLE_GL_ASSERT)
//#define GL_ASSERT(aaa) aaa; {GLint _err = glGetError(); if (_err != GL_NO_ERROR) Err_("gl_assert! " #aaa " failed: %s", lbglGetErrorStr(_err));}
#define GL_CHECK_ERROR {GLint _err = glGetError(); if (_err != GL_NO_ERROR) {Err_("GL error - %s", lbglGetErrorStr(_err));}}
#define GL_ASSERT(aaa) {GLint _err = glGetError(); if (_err != GL_NO_ERROR) {Err_("Previous GL error detected - %s", lbglGetErrorStr(_err));} aaa; _err = glGetError();if (_err != GL_NO_ERROR) {Err_("gl_assert! " #aaa " failed: %s", lbglGetErrorStr(_err));}}
#else //#if defined(ENABLE_GL_ASSERT)
#define GL_CHECK_ERROR
#define GL_ASSERT(...) __VA_ARGS__
#endif //#else #if defined(ENABLE_GL_ASSERT)



#ifdef __cplusplus
}
#endif //#ifdef __cplusplus
#endif
