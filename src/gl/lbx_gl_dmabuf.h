//---------------------------------------------------------------------------
// DMA-BUF -> EGL external image import helpers.
//
// Up through 0.1.x, EGL/GLES2 import logic lived inside the avio-v4l2
// driver. It is now lifted out to the host side. The avio-v4l2 v0.2
// driver only exposes DMA-BUF fds and metadata via FRAME_EVENT; the
// host calls these helpers from within OnFrame to build GL textures.
//
// This location (`lbx-gl`) is temporary. Once the GL backend of `lbx-gfx`
// is ready, the implementation moves there, and a Vulkan backend
// (external image import) follows. At that point the host calls only
// the abstract API of `lbx-gfx` and this header is deprecated.
//
// The dependency on lbx-intf's LBX_DMABUF_INFO is intentionally avoided:
// the host unpacks its raw fields ({fd, offset, pitch, ...}) and passes
// them explicitly.
//---------------------------------------------------------------------------

#ifndef lbx_gl_dmabufH
#define lbx_gl_dmabufH

#include "lbx_gl.h"
#include "intf/lbx_intf_avio.h"   /* LBX_EXT_IMAGE_INTERFACE */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LBX_GLES_VERSION

/* YUV interpretation hints. Pass NULL for defaults (REC709 / NARROW) --
 * the common assumption for V4L2 camera input. Ignored for RGB formats. */
typedef struct {
    EGLint color_space;    /* EGL_ITU_REC709_EXT / EGL_ITU_REC601_EXT / EGL_ITU_REC2020_EXT */
    EGLint sample_range;   /* EGL_YUV_NARROW_RANGE_EXT / EGL_YUV_FULL_RANGE_EXT */
} LBX_GL_DMABUF_HINT;

/**
 * @brief Create an EGLImageKHR from one or more DMA-BUFs.
 *
 * @param egl_display   Current EGLDisplay (usually ctx->egl_display after RC_Init).
 * @param width         Image width in pixels.
 * @param height        Image height in pixels.
 * @param drm_fourcc    DRM_FORMAT_* fourcc. May differ from the V4L2 fourcc,
 *                      so the caller passes a pre-mapped value
 *                      (lbx-intf's LBX_DMABUF_INFO.fourcc is already mapped).
 * @param drm_modifier  DRM 64-bit modifier (DRM_FORMAT_MOD_LINEAR, etc.).
 *                      0 or DRM_FORMAT_MOD_INVALID -> modifier attributes omitted.
 * @param n_planes      Number of planes (typically 1-3; up to 4 with
 *                      EGL_EXT_image_dma_buf_import_modifiers).
 * @param fds           Per-plane DMA-BUF fd (length n_planes).
 * @param offsets       Per-plane byte offset (length n_planes).
 * @param pitches       Per-plane byte pitch (length n_planes).
 * @param hint          YUV color space / sample range hint. NULL = default.
 *
 * @return EGLImageKHR on success, EGL_NO_IMAGE_KHR on failure.
 *
 * The caller binds the resulting image to a GL texture with
 * `lbx_gl_image_target_texture` and releases it with
 * `lbx_gl_destroy_dmabuf_image` when done.
 */
LBX_GL_EXPORT EGLImageKHR lbx_gl_create_dmabuf_image(
    EGLDisplay              egl_display,
    int                     width,
    int                     height,
    u32_t                   drm_fourcc,
    u64_t                   drm_modifier,
    int                     n_planes,
    const int              *fds,
    const u32_t            *offsets,
    const u32_t            *pitches,
    const LBX_GL_DMABUF_HINT *hint);

/**
 * @brief Release an EGLImageKHR.
 */
LBX_GL_EXPORT void lbx_gl_destroy_dmabuf_image(EGLDisplay egl_display, EGLImageKHR image);

/**
 * @brief Attach an EGL image to the currently bound GL texture.
 *
 * The caller must have already called glBindTexture(target, tex_id) and
 * configured glTexParameter*(). Thin wrapper around glEGLImageTargetTexture2DOES.
 *
 * @param target  GL_TEXTURE_EXTERNAL_OES (typical for YUV camera input) or
 *                GL_TEXTURE_2D (for RGB formats supported by the extension).
 * @param image   The handle returned by lbx_gl_create_dmabuf_image.
 */
LBX_GL_EXPORT void lbx_gl_image_target_texture(GLenum target, EGLImageKHR image);

/**
 * @brief GL implementation of LBX_EXT_IMAGE_INTERFACE -- returned by value.
 *
 * The external image interface (import / update / destroy for converting
 * a driver's V4L2 DMA-BUF or CPU-memory images into GPU textures), in its
 * GL incarnation. The host assigns the returned value directly to
 * LBX_AVIO_DRIVER.ext_image -- there is no separate create/destroy step.
 * The function pointers are lbx-gl's static exports, and ctx simply holds
 * the EGLDisplay (no lifetime management needed).
 *
 * The interface is stateless -- handles live in the driver's per-buffer
 * persistent LBX_IMAGE (planes[0].texture uses a sign convention:
 * negative = external OES, positive = 2D, 0 = not created; user_data
 * holds the EGLImageKHR for the DMA-BUF path). Import creates once per
 * buffer; Update re-uploads CPU buffers (called by the driver only when
 * content has changed; the DMA-BUF path is zero-copy and a no-op);
 * Destroy is symmetric with Import.
 *
 * vtable calls must run on the render-context-current thread.
 *
 * NOTE: once lbx-gfx replaces lbx-gl, this implementation moves to lbx-gfx.
 *       The LBX_EXT_IMAGE_INTERFACE seam is backend-neutral, so the driver
 *       and host are unchanged -- only this single helper is swapped for
 *       the lbx-gfx equivalent.
 *
 * @param egl_display  Current EGLDisplay (stored as-is in the interface ctx).
 * @return The LBX_EXT_IMAGE_INTERFACE value (ctx + three function pointers).
 */
LBX_GL_EXPORT LBX_EXT_IMAGE_INTERFACE lbx_gl_external_image(EGLDisplay egl_display);

#endif /* LBX_GLES_VERSION */

#ifdef __cplusplus
}
#endif

#endif
