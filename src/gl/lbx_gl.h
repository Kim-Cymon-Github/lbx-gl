//---------------------------------------------------------------------------

#ifndef lbx_glH
#define lbx_glH
//---------------------------------------------------------------------------

#include "image/lbx_image.h"

#ifdef LBX_GLES_VERSION
#   include <EGL/egl.h>
#   include <EGL/eglext.h>
#   if LBX_GLES_VERSION < 20 // GLES 1.x
#       include <GLES/gl.h>
#   elif LBX_GLES_VERSION < 30 // GLES 2.x
#       include <GLES2/gl2.h>
#   else //#elif LBX_GLES_VERSION < 30 // GLES 3.x
#       include <GLES3/gl3.h>
#       if LBX_GLES_VERSION >= 32 // GLES 3.2
#           include <GLES3/gl32.h>
#       elif LBX_GLES_VERSION >= 31 //#if LBX_GLES_VERSION >= 32
#           include <GLES3/gl31.h> // GLES 3.1
#       endif
#   endif //#else #elif LBX_GLES_VERSION < 30
#   if LBX_GLES_VERSION >= 20
#        include <GLES2/gl2ext.h>
#   endif
#else
#   include <gl/gl.h>  // 일반 OpenGL
#endif //#ifdef LBX_GLES_VERSION

#ifdef _WIN32
#   ifdef LBX_GL_DLL // LBX-GLES 라이브러리의 버전
#       define LBX_GL_EXPORT __declspec(dllexport)
#   else //#ifdef LBX_GL_DLL
#       define LBX_GL_EXPORT __declspec(dllimport)
#   endif //#else #ifdef LBX_GL_DLL
#else //#ifdef _WIN32
#   define LBX_GL_EXPORT
#endif //#else #ifdef _WIN32


#ifdef __cplusplus
extern "C" {
#endif //#ifdef __cplusplus

LBX_GL_EXPORT u32_t lbx_gl_version(void);
LBX_GL_EXPORT const char* lbx_gl_version_str(void);

#define RC_FLAG_OWNS_NATIVE_WINDOW (0x00000001u)
#define RC_FLAG_OWNS_NATIVE_DISPLAY (0x00000002u)

#define LBX_GL_COLOR_BITS (8)
#define LBX_GL_DEPTH_BITS (16)
#if LBX_GL_DEPTH_BITS > 16
#   define LBX_GL_DEPTH_COMPONENT  GL_DEPTH_COMPONENT
#else
#   define LBX_GL_DEPTH_COMPONENT  GL_DEPTH_COMPONENT16
#endif

typedef struct  {
    EGLNativeWindowType  native_window;
    EGLNativeDisplayType native_display;
    EGLDisplay           egl_display;
    EGLContext           egl_context;
    EGLSurface           egl_surface;
} LBX_RENDER_CONTEXT;
LBX_GL_EXPORT void LBX_RENDER_CONTEXT_Init(LBX_RENDER_CONTEXT* self, EGLNativeWindowType native_window);

LBX_GL_EXPORT i32_t RC_Init(LBX_RENDER_CONTEXT *ctx, EGLContext context_to_share, EGLint const  * attrib_list);
LBX_GL_EXPORT i32_t RC_Free(LBX_RENDER_CONTEXT *ctx);

lbx_inline bool RC_MakeCurrent(const LBX_RENDER_CONTEXT* ctx) { return eglGetCurrentContext() == ctx->egl_context ? true : (bool)eglMakeCurrent(ctx->egl_display, ctx->egl_surface, ctx->egl_surface, ctx->egl_context); }
lbx_inline bool RC_SwapBuffers(const LBX_RENDER_CONTEXT *ctx) { return (bool)eglSwapBuffers(ctx->egl_display, ctx->egl_surface);}

LBX_GL_EXPORT const char * lbxGlGetErrorStr(GLenum error_code);
LBX_GL_EXPORT const char*  lbxEglGetErrorStr(EGLint error_code);

#if defined(ENABLE_GL_CHECK)
//#define GL_CHECK(aaa) aaa; {GLint _err = glGetError(); if (_err != GL_NO_ERROR) Err_("gl_assert! " #aaa " failed: %s", lbglGetErrorStr(_err));}
#define GL_CHECK_READY do { \
    GLint _err = glGetError();  \
    if (_err != GL_NO_ERROR) { \
        Err_("GL_CHECK - %s", lbxGlGetErrorStr(_err)); \
    } \
} while (0)

#define GL_CHECK(...) do { \
    GLint _err; \
    __VA_ARGS__; \
    _err = glGetError(); \
    if (_err != GL_NO_ERROR) { \
        Err_("GL_CHECK(" #__VA_ARGS__ ") failed: %s", lbxGlGetErrorStr(_err)); \
    } \
} while (0)

#else //#if defined(ENABLE_GL_CHECK)
#define GL_CHECK_READY do {} while(0)
#define GL_CHECK(...) __VA_ARGS__
#endif //#else #if defined(ENABLE_GL_CHECK)

/**
 * @brief Saves current buffer binding and binds the new buffer
 * @return Previously bound buffer
 */
LBX_GL_EXPORT GLuint lbxGlBindBuffer(GLenum target, GLuint buffer);

/**
 * @brief Restores previous buffer binding if needed
 */
LBX_GL_EXPORT void lbxGlRestoreBufferBinding(GLenum target, GLuint previous_binding, GLuint current_binding);



/*
 * NOTE: LBX_IMAGE_BindTextures was removed in lbx-gfx migration (see
 * lbx-gfx/.omc/plan.md sec. 3.4). Its body read planes[].texture as a
 * signed GLuint (sign-trick encoding), which is incompatible with the
 * new (intptr_t)GFX_TEXTURE_2D* wire format. Use gfx_image_bind_textures
 * (lbx-gfx, src/gfx/lbx_gfx_external.h) instead.
 */

#ifdef __cplusplus
}
#endif //#ifdef __cplusplus
#endif


#ifdef __cplusplus
#ifndef lbx_glHPP
#define lbx_glHPP

class LbxRenderContext : public LBX_RENDER_CONTEXT
{
public:
    inline LbxRenderContext(EGLNativeWindowType native_window) { LBX_RENDER_CONTEXT_Init(this, native_window); }
    inline LbxRenderContext() { LBX_RENDER_CONTEXT_Init(this, NULL); }
    inline ~LbxRenderContext() { RC_Free(this); }
    inline i32_t Init(EGLContext context_to_share = NULL, EGLint const* attrib_list = NULL) { return RC_Init(this, context_to_share, attrib_list); }
};
#endif //#ifndef lbx_glHPP
#endif //#ifdef __cplusplus

