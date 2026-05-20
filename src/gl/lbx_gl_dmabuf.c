//---------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>

#if defined(__BORLANDC__)
#   pragma hdrstop
#endif

#include "lbx_gl_dmabuf.h"
#include "lbx_core.h"
#include "system/lbx_log.h"

#if defined(__BORLANDC__)
#   pragma package(smart_init)
#endif

#ifdef LBX_GLES_VERSION

/*---------------------------------------------------------------------------
 * EGL_EXT_image_dma_buf_import / _modifiers 의 PLANE attribute 들이
 * 0x3272 부터 plane 0 의 (FD, OFFSET, PITCH) 셋으로 시작해 3 씩 올라간다.
 * plane 3 (modifier 확장) 만 별도 베이스 (0x3440).
 *---------------------------------------------------------------------------*/

static EGLint plane_fd_enum_(int plane_idx)
{
    switch (plane_idx) {
    case 0: return EGL_DMA_BUF_PLANE0_FD_EXT;
    case 1: return EGL_DMA_BUF_PLANE1_FD_EXT;
    case 2: return EGL_DMA_BUF_PLANE2_FD_EXT;
#ifdef EGL_DMA_BUF_PLANE3_FD_EXT
    case 3: return EGL_DMA_BUF_PLANE3_FD_EXT;
#endif
    default: return EGL_NONE;
    }
}
static EGLint plane_offset_enum_(int plane_idx)
{
    switch (plane_idx) {
    case 0: return EGL_DMA_BUF_PLANE0_OFFSET_EXT;
    case 1: return EGL_DMA_BUF_PLANE1_OFFSET_EXT;
    case 2: return EGL_DMA_BUF_PLANE2_OFFSET_EXT;
#ifdef EGL_DMA_BUF_PLANE3_OFFSET_EXT
    case 3: return EGL_DMA_BUF_PLANE3_OFFSET_EXT;
#endif
    default: return EGL_NONE;
    }
}
static EGLint plane_pitch_enum_(int plane_idx)
{
    switch (plane_idx) {
    case 0: return EGL_DMA_BUF_PLANE0_PITCH_EXT;
    case 1: return EGL_DMA_BUF_PLANE1_PITCH_EXT;
    case 2: return EGL_DMA_BUF_PLANE2_PITCH_EXT;
#ifdef EGL_DMA_BUF_PLANE3_PITCH_EXT
    case 3: return EGL_DMA_BUF_PLANE3_PITCH_EXT;
#endif
    default: return EGL_NONE;
    }
}

#ifdef EGL_EXT_image_dma_buf_import_modifiers
static EGLint plane_modifier_lo_enum_(int plane_idx)
{
    switch (plane_idx) {
    case 0: return EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
    case 1: return EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT;
    case 2: return EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT;
#ifdef EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT
    case 3: return EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT;
#endif
    default: return EGL_NONE;
    }
}
static EGLint plane_modifier_hi_enum_(int plane_idx)
{
    switch (plane_idx) {
    case 0: return EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
    case 1: return EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT;
    case 2: return EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT;
#ifdef EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT
    case 3: return EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT;
#endif
    default: return EGL_NONE;
    }
}
#endif

/* DRM_FORMAT_MOD_INVALID — modifier 미사용 sentinel */
#ifndef LBX_GL_DRM_FORMAT_MOD_INVALID
#define LBX_GL_DRM_FORMAT_MOD_INVALID ((u64_t)0x00ffffffffffffffull)
#endif

/*---------------------------------------------------------------------------
 * 함수 포인터 — eglCreateImageKHR / eglDestroyImageKHR / glEGLImageTargetTexture2DOES
 * 는 헤더에 prototype 만 있고 실제는 extension. eglGetProcAddress 로 가져온다.
 * 첫 호출 시 한 번만 해석.
 *---------------------------------------------------------------------------*/

static PFNEGLCREATEIMAGEKHRPROC                pfn_eglCreateImageKHR_  = NULL;
static PFNEGLDESTROYIMAGEKHRPROC               pfn_eglDestroyImageKHR_ = NULL;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC     pfn_glImageTarget_      = NULL;

static int resolve_egl_procs_(void)
{
    if (pfn_eglCreateImageKHR_ == NULL) {
        pfn_eglCreateImageKHR_ =
            (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    }
    if (pfn_eglDestroyImageKHR_ == NULL) {
        pfn_eglDestroyImageKHR_ =
            (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    }
    if (pfn_glImageTarget_ == NULL) {
        pfn_glImageTarget_ =
            (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
                "glEGLImageTargetTexture2DOES");
    }
    if (pfn_eglCreateImageKHR_ == NULL ||
        pfn_eglDestroyImageKHR_ == NULL ||
        pfn_glImageTarget_ == NULL) {
        Err_("EGL DMA-BUF import extensions are not available");
        return 0;
    }
    return 1;
}

/*---------------------------------------------------------------------------
 * V4L2 fourcc → DRM fourcc 매핑.
 *
 * EGL_LINUX_DRM_FOURCC_EXT 는 DRM fourcc 를 요구한다. V4L2 의 멀티플레인
 * 변종 (NM12=NV12M, YM12 등) 은 DRM 에 없으므로 동등 DRM 포맷으로 접는다.
 * DRM fourcc 코드는 곧 4-char 코드라 drm_fourcc.h 없이 fourcc_() 로 표현
 * 가능. (avio-v4l2 0.1.x 의 egl_create_pixmap switch 에서 이식.)
 *
 * 멀티플레인 (NM12 등) 의 per-plane fd / offset / pitch 는 v0.2 driver 가
 * FRAME_EVENT.dmabuf 에 이미 정확히 채워 넘기므로 (분리 fd, offset 0,
 * pitch=stride), 여기서는 fourcc 만 접으면 충분하다.
 *---------------------------------------------------------------------------*/
static u32_t v4l2_to_drm_fourcc_(u32_t f)
{
    switch (f) {
    /* RGBA 계열 — V4L2 RGB4(ARGB) 는 실제로 BGRA 바이트 순서로 들어옴 */
    case fourcc_('R','G','B','4'):           /* V4L2 ARGB → DRM ARGB8888 */
    case fourcc_('B','G','R','4'):           /* deprecated V4L2 BGRA */
    case fourcc_('A','R','2','4'):           /* V4L2_PIX_FMT_ABGR32 (mem: BGRA) */
        return fourcc_('A','R','2','4');     /* DRM_FORMAT_ARGB8888 */
    case fourcc_('A','B','2','4'):           /* V4L2_PIX_FMT_RGBA32 (mem: RGBA) */
        return fourcc_('A','B','2','4');     /* DRM_FORMAT_ABGR8888 */
    case fourcc_('X','R','2','4'):
        return fourcc_('X','R','2','4');     /* DRM_FORMAT_XRGB8888 */
    case fourcc_('R','G','B','3'):
        return fourcc_('B','G','2','4');     /* DRM_FORMAT_BGR888 */
    case fourcc_('B','G','R','3'):
        return fourcc_('R','G','2','4');     /* DRM_FORMAT_RGB888 */
    case fourcc_('R','G','B','P'):
        return fourcc_('R','G','1','6');     /* DRM_FORMAT_RGB565 */
    case fourcc_('A','Y','U','V'):
        return fourcc_('A','Y','U','V');
    /* packed YUV — DRM 동일 코드 */
    case fourcc_('Y','U','Y','V'):
    case fourcc_('Y','V','Y','U'):
    case fourcc_('U','Y','V','Y'):
    case fourcc_('V','Y','U','Y'):
        return f;
    /* semi-planar — NV12 / NM12(NV12M) 둘 다 DRM_FORMAT_NV12 */
    case fourcc_('N','V','1','2'):
    case fourcc_('N','M','1','2'):
        return fourcc_('N','V','1','2');
    case fourcc_('N','V','2','1'):
    case fourcc_('N','M','2','1'):
        return fourcc_('N','V','2','1');
    case fourcc_('N','V','1','6'):
        return fourcc_('N','V','1','6');
    case fourcc_('N','V','6','1'):
        return fourcc_('N','V','6','1');
    /* planar 4:2:0 — YU12/YM12 → YUV420, YV12/YM21 → YVU420 */
    case fourcc_('Y','U','1','2'):
    case fourcc_('Y','M','1','2'):
        return fourcc_('Y','U','1','2');     /* DRM_FORMAT_YUV420 */
    case fourcc_('Y','V','1','2'):
    case fourcc_('Y','M','2','1'):
        return fourcc_('Y','V','1','2');     /* DRM_FORMAT_YVU420 */
    default:
        return f;   /* 이미 DRM fourcc 라고 가정하고 그대로 통과 */
    }
}

/* YUV fourcc 빠른 판정 — color_space / sample_range hint 를 attribute 에
 * 추가할지 결정하기 위해. RGB 만 들어오면 hint 가 무의미해 생략. */
static int is_yuv_fourcc_(u32_t fourcc)
{
    /* DRM fourcc 의 마지막 자리만 보고 판정하면 false positive 있어 풀 매칭. */
    switch (fourcc) {
    case fourcc_('N','V','1','2'):
    case fourcc_('N','V','2','1'):
    case fourcc_('N','V','1','6'):
    case fourcc_('N','V','6','1'):
    case fourcc_('Y','U','Y','V'):
    case fourcc_('Y','V','Y','U'):
    case fourcc_('U','Y','V','Y'):
    case fourcc_('V','Y','U','Y'):
    case fourcc_('Y','U','1','2'):  /* YUV420 */
    case fourcc_('Y','V','1','2'):  /* YVU420 */
    case fourcc_('A','Y','U','V'):
    case fourcc_('Y','M','1','2'):
    case fourcc_('Y','M','2','1'):
    case fourcc_('N','M','1','2'):
    case fourcc_('N','M','2','1'):
        return 1;
    default:
        return 0;
    }
}

EGLImageKHR lbx_gl_create_dmabuf_image(
    EGLDisplay              egl_display,
    int                     width,
    int                     height,
    u32_t                   drm_fourcc,
    u64_t                   drm_modifier,
    int                     n_planes,
    const int              *fds,
    const u32_t            *offsets,
    const u32_t            *pitches,
    const LBX_GL_DMABUF_HINT *hint)
{
    if (egl_display == EGL_NO_DISPLAY) {
        Err_("egl_display is EGL_NO_DISPLAY");
        return EGL_NO_IMAGE_KHR;
    }
    if (width <= 0 || height <= 0) {
        Err_("invalid size %dx%d", width, height);
        return EGL_NO_IMAGE_KHR;
    }
    if (n_planes <= 0 || n_planes > 4 || fds == NULL || offsets == NULL || pitches == NULL) {
        Err_("invalid plane data n=%d", n_planes);
        return EGL_NO_IMAGE_KHR;
    }
    if (!resolve_egl_procs_()) {
        return EGL_NO_IMAGE_KHR;
    }

    /* 호출자가 V4L2 fourcc (NM12 등) 를 넘겨도 DRM 으로 접어준다. */
    drm_fourcc = v4l2_to_drm_fourcc_(drm_fourcc);

    /* attribute 버퍼 — 안전 여유 충분히. base 8개 + plane 당 (FD,OFFSET,PITCH)
     * + modifier 사용 시 plane 당 (MOD_LO, MOD_HI) + YUV hint 4개 + terminator. */
    EGLint attribs[64];
    int    n = 0;

    attribs[n++] = EGL_WIDTH;                 attribs[n++] = (EGLint)width;
    attribs[n++] = EGL_HEIGHT;                attribs[n++] = (EGLint)height;
    attribs[n++] = EGL_LINUX_DRM_FOURCC_EXT;  attribs[n++] = (EGLint)drm_fourcc;

    const int use_modifier =
        (drm_modifier != 0 && drm_modifier != LBX_GL_DRM_FORMAT_MOD_INVALID);

    for (int p = 0; p < n_planes; ++p) {
        attribs[n++] = plane_fd_enum_(p);      attribs[n++] = (EGLint)fds[p];
        attribs[n++] = plane_offset_enum_(p);  attribs[n++] = (EGLint)offsets[p];
        attribs[n++] = plane_pitch_enum_(p);   attribs[n++] = (EGLint)pitches[p];
#ifdef EGL_EXT_image_dma_buf_import_modifiers
        if (use_modifier) {
            attribs[n++] = plane_modifier_lo_enum_(p);
            attribs[n++] = (EGLint)(drm_modifier & 0xFFFFFFFFu);
            attribs[n++] = plane_modifier_hi_enum_(p);
            attribs[n++] = (EGLint)((drm_modifier >> 32) & 0xFFFFFFFFu);
        }
#else
        (void)use_modifier;
#endif
    }

    if (is_yuv_fourcc_(drm_fourcc)) {
        EGLint color_space  = (hint && hint->color_space)  ? hint->color_space  : EGL_ITU_REC709_EXT;
        EGLint sample_range = (hint && hint->sample_range) ? hint->sample_range : EGL_YUV_NARROW_RANGE_EXT;
        attribs[n++] = EGL_YUV_COLOR_SPACE_HINT_EXT;  attribs[n++] = color_space;
        attribs[n++] = EGL_SAMPLE_RANGE_HINT_EXT;     attribs[n++] = sample_range;
    } else {
        (void)hint;
    }

    attribs[n++] = EGL_NONE;

    EGLImageKHR image = pfn_eglCreateImageKHR_(
        egl_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
    if (image == EGL_NO_IMAGE_KHR) {
        Err_("eglCreateImageKHR failed: %s (fourcc=" FOURCC_VFMT
             " %dx%d planes=%d fd0=%d)",
             lbxEglGetErrorStr(eglGetError()), FOURCC_VARG(drm_fourcc),
             width, height, n_planes, fds[0]);
    }
    return image;
}

void lbx_gl_destroy_dmabuf_image(EGLDisplay egl_display, EGLImageKHR image)
{
    if (image == EGL_NO_IMAGE_KHR) return;
    if (!resolve_egl_procs_()) return;
    if (pfn_eglDestroyImageKHR_(egl_display, image) != EGL_TRUE) {
        Err_("eglDestroyImageKHR failed: %s", lbxEglGetErrorStr(eglGetError()));
    }
}

void lbx_gl_image_target_texture(GLenum target, EGLImageKHR image)
{
    if (image == EGL_NO_IMAGE_KHR) return;
    if (!resolve_egl_procs_()) return;
    pfn_glImageTarget_(target, (GLeglImageOES)image);
    GL_CHECK_READY;
}

/*---------------------------------------------------------------------------
 * LBX_EXT_IMAGE_INTERFACE 의 GL 구현
 *
 * 외부 이미지 인터페이스 (도메인 표준 — EGL OES_EGL_image_external,
 * VK KHR_external_memory 호응) 의 GL backend. driver 의 V4L2 DMA-BUF
 * 또는 CPU 메모리 이미지를 GPU 텍스처로 import/update/destroy.
 *
 * GPU 리소스 수명 = 버퍼 수명. 버퍼 수명은 driver 가 안다(REQBUFS/Recover/
 * Close). 그래서 driver 가 자기 버퍼 lifecycle 에서 본 인터페이스의
 * Import/Update/Destroy 를 호출 (driver 는 GL 모름, vtable 만).
 *
 * 스테이트리스 — 전역 슬롯 테이블 없음. 핸들은 driver 의 버퍼별 영속
 * LBX_IMAGE 가 보유:
 *   img->planes[0].texture : GL 텍스처 (부호 규약: 음수 external OES /
 *                            양수 2D, 0 = 미생성) — GL backend 내부 약속
 *   img->user_data         : DMA-BUF 경로의 EGLImageKHR (CPU 는 0)
 *
 * 객체 생성·파괴 없음. lbx_gl_external_image() 가 vtable + ctx 묶음 값을
 * 돌려주고 host 가 그 값을 LBX_AVIO_DRIVER.ext_image 슬롯에 직접 대입한다.
 * ctx 는 EGLDisplay 그대로 (별도 packing 불요).
 *
 * 렌더 컨텍스트 current 스레드에서 호출돼야 한다 (driver 의 Open/Grab/
 * Recover/Close 를 host 가 자기 GL 스레드에서 부른다는 가정 — dispatcher
 * 구조와 동일).
 *
 * NOTE(이식): lbx-gfx 가 lbx-gl 을 대체할 때 본 구현을 lbx-gfx 로 옮긴다.
 * LBX_EXT_IMAGE_INTERFACE seam 이 백엔드 중립이라 driver/host 는 무변경.
 *---------------------------------------------------------------------------*/

/* CPU 업로드 src/internalformat 결정 — 정규화된 fourcc 기준.
 * GLES 는 glTexImage2D 의 internalformat == format 강제(GL_BGRA_EXT 규약). */
static void cpu_fmt_(u32_t fcc, GLenum *src, GLenum *ifmt)
{
    *src  = GL_RGBA;
    *ifmt = GL_RGBA;
    if (fcc == fourcc_('B','G','R','A') ||   /* LBX_IMAGE_Init 정규화값 */
        fcc == fourcc_('A','R','2','4') ||   /* V4L2 raw (ABGR32) */
        fcc == fourcc_('R','G','B','4') ||   /* V4L2 raw (legacy ARGB) */
        fcc == fourcc_('B','G','R','4')) {   /* V4L2 raw (BGRA) */
#ifdef GL_BGRA_EXT
        *src  = GL_BGRA_EXT;
        *ifmt = GL_BGRA_EXT;
#endif
    }
}

static i32_t LBX_API ext_image_import_(LBX_EXT_IMAGE_INTERFACE *self, LBX_IMAGE *img)
{
    EGLDisplay disp;
    i32_t w, h, n, fd0;

    if (self == NULL || img == NULL) {
        return -1;
    }
    disp = (EGLDisplay)self->ctx;
    w    = img->planes[0].size.width;
    h    = img->planes[0].size.height;
    n    = img->plane_count ? img->plane_count : 1;
    if (n > LBX_IMAGE_MAX_PLANES) n = LBX_IMAGE_MAX_PLANES;
    fd0  = (i32_t)img->planes[0].native_handle;

    if (fd0 >= 0) {
        /* DMA-BUF 경로 (실 V4L2). EGLImage+텍스처 생성, 핸들은 img 가 보유. */
        int   fds[LBX_IMAGE_MAX_PLANES];
        u32_t offsets[LBX_IMAGE_MAX_PLANES];
        u32_t pitches[LBX_IMAGE_MAX_PLANES];
        EGLImageKHR eimg;
        GLuint t = 0;
        i32_t p;
        for (p = 0; p < n; ++p) {
            fds[p]     = (int)img->planes[p].native_handle;
            offsets[p] = 0;
            pitches[p] = (u32_t)img->planes[p].stride.y;
        }
        eimg = lbx_gl_create_dmabuf_image(
            disp, w, h, img->pixel_format, img->native_handle,
            n, fds, offsets, pitches, NULL);
        if (eimg == EGL_NO_IMAGE_KHR) {
            return -1;
        }
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, t);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        lbx_gl_image_target_texture(GL_TEXTURE_EXTERNAL_OES, eimg);
        /* EGLImage 는 버퍼 수명 끝까지 live (Destroy 때 해제) →
         * per-frame create/destroy 소멸 → 임베디드 sibling 세그폴트 원천 제거 */
        img->user_data        = (intptr_t)eimg;
        img->planes[0].texture = -(intptr_t)t;   /* 음수 = external OES */
        return 0;
    }

    if (img->planes[0].data != 0) {
        /* CPU 버퍼 경로 (avio-file). 텍스처 생성 + 초기 업로드. */
        GLenum src, ifmt;
        GLuint t = 0;
        cpu_fmt_(img->pixel_format, &src, &ifmt);
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0,
                     src, GL_UNSIGNED_BYTE, (const void *)img->planes[0].data);
        img->user_data         = 0;             /* CPU 는 EGLImage 없음 */
        img->planes[0].texture = (intptr_t)t;   /* 양수 = 2D */
        return 0;
    }

    return -1;
}

static i32_t LBX_API ext_image_update_(LBX_EXT_IMAGE_INTERFACE *self, LBX_IMAGE *img)
{
    GLenum src, ifmt;
    intptr_t tex;
    i32_t w, h;
    (void)self;
    if (img == NULL) {
        return -1;
    }
    tex = img->planes[0].texture;
    /* DMA-BUF(음수) 는 zero-copy — 재업로드 불요. 미생성(0) 도 무시. */
    if (tex <= 0) {
        return 0;
    }
    /* CPU 재업로드. driver 가 "내용 실제 변경" 일 때만 호출하므로
     * 백엔드는 무조건 업로드 (게이팅 책임은 driver). */
    w = img->planes[0].size.width;
    h = img->planes[0].size.height;
    cpu_fmt_(img->pixel_format, &src, &ifmt);
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0,
                 src, GL_UNSIGNED_BYTE, (const void *)img->planes[0].data);
    return 0;
}

static void LBX_API ext_image_destroy_(LBX_EXT_IMAGE_INTERFACE *self, LBX_IMAGE *img)
{
    EGLDisplay disp;
    intptr_t tex;
    EGLImageKHR eimg;
    if (self == NULL || img == NULL) {
        return;
    }
    disp = (EGLDisplay)self->ctx;
    tex  = img->planes[0].texture;
    if (tex < 0) tex = -tex;
    if (tex != 0) {
        GLuint t = (GLuint)tex;
        glDeleteTextures(1, &t);
    }
    eimg = (EGLImageKHR)img->user_data;
    if (eimg != EGL_NO_IMAGE_KHR) {
        lbx_gl_destroy_dmabuf_image(disp, eimg);
    }
    img->planes[0].texture = 0;
    img->user_data         = 0;
}

LBX_EXT_IMAGE_INTERFACE lbx_gl_external_image(EGLDisplay egl_display)
{
    LBX_EXT_IMAGE_INTERFACE r;
    r.ctx     = (void *)egl_display;
    r.Import  = ext_image_import_;
    r.Update  = ext_image_update_;
    r.Destroy = ext_image_destroy_;
    return r;
}

#endif /* LBX_GLES_VERSION */
