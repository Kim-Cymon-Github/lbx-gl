//---------------------------------------------------------------------------

#ifndef lbx_glH
#define lbx_glH
//---------------------------------------------------------------------------

#include "image/lbx_image.h"

#define GLES_3_2

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
#   ifdef LBXGLES_VERSION // LBX-GLES 라이브러리의 버전
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

LBX_GL_EXPORT u32_t lbx_gl_version(void);

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



/**
 * @brief Binds textures for each pixel plane in the image.
 *
 * This function iterates through the pixel planes stored in the LBX_IMAGE object in reverse order (from last to first)
 * and binds textures to the corresponding texture units if they exist.
 *
 * If a pixel plane's texture value is non-zero:
 * - A positive value binds the texture using the GL_TEXTURE_2D target.
 * - A negative value binds the texture (using its absolute value) with the GL_TEXTURE_EXTERNAL_OES target.
 *
 * The iteration stops when a pixel plane with a texture value of 0 is encountered.
 *
 * @param self Pointer to an LBX_IMAGE structure containing texture binding information.
 * @return The number of textures that were successfully bound.
 */
LBX_GL_EXPORT i32_t LBX_IMAGE_BindTextures(const LBX_IMAGE* self);

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

