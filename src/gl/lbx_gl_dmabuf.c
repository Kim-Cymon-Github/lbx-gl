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
 * 텍스처/EGLImage 소유 — lbx-gl 가 GPU 리소스를 생성·소유·파괴한다.
 *
 * 슬롯 키는 "드라이버-소유 영속 LBX_IMAGE 주소". 드라이버는 버퍼별로
 * LBX_IMAGE 를 하나씩 영속 보유하므로(avio-file: 채널당 1개,
 * avio-v4l2: ring buffer 당 1개) 그 주소가 버퍼 정체성의 안정 키가 된다.
 *
 * 생성: planes[0].texture==0 (= 아직 안 만듦) 이고 슬롯 없으면 1회 생성.
 *       이후 같은 LBX_IMAGE 면 재사용 — DMA-BUF 는 EGLImage 가 그 버퍼
 *       dmabuf 의 영속 view 라 재타겟 불필요(V4L2 가 같은 메모리에 새
 *       프레임을 채우면 샘플링이 자동 갱신). CPU 는 sequence 바뀔 때만
 *       glTexImage2D 재업로드.
 * 파괴: lbx_gl_release_texture (버퍼/디바이스 정리 시 드라이버가 호출).
 *       EGLImage 는 버퍼 수명 끝까지 유지 → per-frame create/destroy 소멸
 *       → 임베디드 sibling-destroy 세그폴트 원천 제거.
 *       lbx_gl_release_all 은 컨텍스트 종료용 백스톱.
 *
 * 본 호출들은 update_texture 와 동일하게 렌더 컨텍스트 current 상태에서
 * 불려야 한다 (glGenTextures/glDeleteTextures/eglDestroyImageKHR).
 *---------------------------------------------------------------------------*/
typedef struct {
    LBX_IMAGE  *owner;     /* 키: 영속 LBX_IMAGE 주소. NULL = 빈 슬롯 */
    GLuint      tex;       /* lbx-gl 가 만든 GL 텍스처 id */
    EGLDisplay  disp;
    EGLImageKHR eimg;      /* DMA-BUF 경로만. 없으면 EGL_NO_IMAGE_KHR */
    int         fd0;       /* DMA-BUF 경로: 생성 당시 plane0 fd. Recover 로
                            * 버퍼 재할당되면 fd 가 바뀜 → 자가 재생성 트리거 */
    u32_t       cpu_seq;   /* CPU 경로: 마지막 업로드 sequence */
    int         is_cpu;    /* 1 = CPU 업로드 경로 */
} LbxGlTexSlot;

#define LBX_GL_TEXSLOT_MAX 64
static LbxGlTexSlot s_slot[LBX_GL_TEXSLOT_MAX];
static int          s_slot_n = 0;

static LbxGlTexSlot *slot_find_(LBX_IMAGE *img)
{
    int i;
    for (i = 0; i < s_slot_n; ++i) {
        if (s_slot[i].owner == img) return &s_slot[i];
    }
    return NULL;
}

static LbxGlTexSlot *slot_alloc_(LBX_IMAGE *img)
{
    int i;
    for (i = 0; i < s_slot_n; ++i) {        /* 해제된 슬롯 재사용 */
        if (s_slot[i].owner == NULL) break;
    }
    if (i == s_slot_n) {
        if (s_slot_n >= LBX_GL_TEXSLOT_MAX) return NULL;
        i = s_slot_n++;
    }
    s_slot[i].owner   = img;
    s_slot[i].tex     = 0;
    s_slot[i].disp    = EGL_NO_DISPLAY;
    s_slot[i].eimg    = EGL_NO_IMAGE_KHR;
    s_slot[i].fd0     = -1;
    s_slot[i].cpu_seq = 0;
    s_slot[i].is_cpu  = 0;
    return &s_slot[i];
}

/*---------------------------------------------------------------------------
 * avio v0.2 단일 진입점 — LBX_IMAGE 한 장을 GL 텍스처에 갱신.
 * host 의 OnFrame 은 이거 한 번만 부르면 끝. 텍스처 생성·부호 규약까지
 * lbx-gl 이 박는다. host 는 glGenTextures 하지 않는다.
 *---------------------------------------------------------------------------*/
i32_t lbx_gl_update_texture(EGLDisplay egl_display, LBX_IMAGE *img)
{
    LbxGlTexSlot *slot;
    i32_t w, h, n;
    i32_t fd0;

    if (img == NULL) {
        return -1;
    }
    w = img->planes[0].size.width;
    h = img->planes[0].size.height;
    n = img->plane_count ? img->plane_count : 1;
    if (n > LBX_IMAGE_MAX_PLANES) n = LBX_IMAGE_MAX_PLANES;

    fd0 = (i32_t)img->planes[0].native_handle;

    if (fd0 >= 0) {
        /* DMA-BUF 경로 (실 V4L2). 버퍼당 EGLImage+텍스처 1회 생성, 이후 재사용.
         * Recover 로 버퍼 재할당되면 fd 가 바뀜 → 자가 파괴·재생성. */
        slot = slot_find_(img);
        if (slot != NULL && slot->fd0 != fd0) {
            /* fd 변경 (Recover 등) — stale 리소스 파괴 후 슬롯 비워 재생성 */
            if (slot->eimg != EGL_NO_IMAGE_KHR) {
                lbx_gl_destroy_dmabuf_image(slot->disp, slot->eimg);
                slot->eimg = EGL_NO_IMAGE_KHR;
            }
            if (slot->tex != 0) {
                glDeleteTextures(1, &slot->tex);
                slot->tex = 0;
            }
            slot->owner = NULL;        /* 슬롯 반환 */
            slot = NULL;
        }
        if (slot == NULL) {
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
                egl_display, w, h, img->pixel_format, img->native_handle,
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

            slot = slot_alloc_(img);
            if (slot == NULL) {
                /* 슬롯 풀 소진(비정상) — 누수 방지 즉시 정리 후 실패 */
                glDeleteTextures(1, &t);
                lbx_gl_destroy_dmabuf_image(egl_display, eimg);
                return -1;
            }
            slot->tex    = t;
            slot->disp   = egl_display;
            slot->eimg   = eimg;       /* 버퍼 수명 끝까지 live */
            slot->fd0    = fd0;
            slot->is_cpu = 0;
        }
        /* 재사용: EGLImage 가 dmabuf 영속 view. 재타겟 불필요.
         * planes[0].texture 는 방어적으로 매번 세팅(드라이버가 리셋하면 안
         * 되지만, 엔진이 부호 보고 바인딩하므로 안전하게). 음수 = external OES */
        img->planes[0].texture = -(intptr_t)slot->tex;
        return 0;
    }

    if (img->planes[0].data != 0) {
        /* CPU 버퍼 경로 (avio-file). 업로드 src 포맷은 LBX_IMAGE 의
         * **정규화된** pixel_format 기준 — LBX_IMAGE_Init 이 입력 fourcc 를
         * canonical (BGRA/RGBA/ABGR/ARGB ...) 로 접어서 저장하므로 그걸로 판정.
         * (V4L2 raw fourcc AR24/RGB4/BGR4 도 driver 가 그대로 줄 수 있어 같이.)
         *   메모리 [B,G,R,A]  → GL_BGRA_EXT
         *   메모리 [R,G,B,A]  → GL_RGBA
         * GL_BGRA_EXT 없으면 GL_RGBA fallback (색 swizzle — 셰이더/원본 보정). */
        /* GLES 는 glTexImage2D 의 internalformat == format 강제.
         * GL_EXT_texture_format_BGRA8888 규약: GL_BGRA_EXT 를 format 으로 쓰면
         * internalformat 도 GL_BGRA_EXT (GL_RGBA 와 섞으면 GL_INVALID_OPERATION). */
        GLenum src  = GL_RGBA;
        GLenum ifmt = GL_RGBA;
        u32_t  fcc  = img->pixel_format;
        int    need_upload;
        if (fcc == fourcc_('B','G','R','A') ||   /* LBX_IMAGE_Init 정규화값 */
            fcc == fourcc_('A','R','2','4') ||   /* V4L2 raw (ABGR32) */
            fcc == fourcc_('R','G','B','4') ||   /* V4L2 raw (legacy ARGB) */
            fcc == fourcc_('B','G','R','4')) {   /* V4L2 raw (BGRA) */
#ifdef GL_BGRA_EXT
            src  = GL_BGRA_EXT;
            ifmt = GL_BGRA_EXT;
#endif
        }
        slot = slot_find_(img);
        if (slot == NULL) {
            GLuint t = 0;
            glGenTextures(1, &t);
            slot = slot_alloc_(img);
            if (slot == NULL) {
                glDeleteTextures(1, &t);
                return -1;
            }
            slot->tex    = t;
            slot->disp   = egl_display;
            slot->is_cpu = 1;
            slot->cpu_seq = img->sequence;
            need_upload  = 1;          /* 최초 생성 → 무조건 업로드 */
        } else {
            /* sequence 바뀔 때만 재업로드. 정지영상(avio-file)은 UI 로
             * 이미지 바꿀 때만 sequence 가 변하므로 그때만 재업로드. */
            need_upload = (slot->cpu_seq != img->sequence);
            slot->cpu_seq = img->sequence;
        }
        if (need_upload) {
            glBindTexture(GL_TEXTURE_2D, slot->tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0,
                         src, GL_UNSIGNED_BYTE, (const void *)img->planes[0].data);
        }
        img->planes[0].texture = (intptr_t)slot->tex;    /* 양수 = 2D */
        return 0;
    }

    return -1;
}

/*---------------------------------------------------------------------------
 * 대칭 해제 — 드라이버가 버퍼/디바이스 정리 시 호출. 텍스처+EGLImage 파괴,
 * 슬롯 비움, planes[0].texture=0 리셋(다음 update 때 재생성됨 → Recover 대응).
 *---------------------------------------------------------------------------*/
void lbx_gl_release_texture(EGLDisplay egl_display, LBX_IMAGE *img)
{
    LbxGlTexSlot *slot;
    (void)egl_display;
    if (img == NULL) return;
    slot = slot_find_(img);
    if (slot == NULL) {
        img->planes[0].texture = 0;
        return;
    }
    if (slot->eimg != EGL_NO_IMAGE_KHR) {
        lbx_gl_destroy_dmabuf_image(slot->disp, slot->eimg);
        slot->eimg = EGL_NO_IMAGE_KHR;
    }
    if (slot->tex != 0) {
        glDeleteTextures(1, &slot->tex);
        slot->tex = 0;
    }
    slot->owner   = NULL;          /* 슬롯 반환 (slot_alloc_ 가 재사용) */
    slot->disp    = EGL_NO_DISPLAY;
    slot->cpu_seq = 0;
    slot->is_cpu  = 0;
    img->planes[0].texture = 0;
}

/*---------------------------------------------------------------------------
 * 백스톱 — 렌더 컨텍스트 종료 시 1회. 드라이버가 어느 정리 경로에서
 * release_texture 를 빠뜨려도 누수 없게 전부 회수.
 *
 * 주의: 이 시점엔 드라이버 device/LBX_IMAGE 가 이미 해제됐을 수 있다
 * (host 가 Close 후 호출하는 백스톱). 그래서 owner 를 절대 역참조하지
 * 않는다 — GPU 리소스만 회수. 종료라 드라이버 texture 필드 리셋 무의미.
 *---------------------------------------------------------------------------*/
void lbx_gl_release_all(EGLDisplay egl_display)
{
    int i;
    (void)egl_display;
    for (i = 0; i < s_slot_n; ++i) {
        if (s_slot[i].owner == NULL) continue;
        if (s_slot[i].eimg != EGL_NO_IMAGE_KHR) {
            lbx_gl_destroy_dmabuf_image(s_slot[i].disp, s_slot[i].eimg);
            s_slot[i].eimg = EGL_NO_IMAGE_KHR;
        }
        if (s_slot[i].tex != 0) {
            glDeleteTextures(1, &s_slot[i].tex);
            s_slot[i].tex = 0;
        }
        s_slot[i].owner = NULL;        /* owner 역참조 금지 (freed 가능) */
    }
    s_slot_n = 0;
}

#endif /* LBX_GLES_VERSION */
