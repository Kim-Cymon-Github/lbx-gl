//---------------------------------------------------------------------------
// DMA-BUF → EGL external image import helpers.
//
// 0.1.x 까지는 avio-v4l2 driver 안에 직접 박혀 있던 EGL/GLES2 import 로직을
// 호스트 측으로 끌어낸 것. avio-v4l2 v0.2 의 driver 는 DMA-BUF fd 와 메타만
// FRAME_EVENT 로 노출하고, 호스트가 OnFrame 안에서 본 헬퍼를 호출해 GL
// 텍스처를 만든다.
//
// 이 위치(`lbx-gl`)는 임시. `lbx-gfx` 의 GL 백엔드가 준비되면 그쪽으로 이전
// 하고, 동시에 Vulkan 백엔드 (external image import) 모듈이 추가된다.
// 그 시점에 호스트는 `lbx-gfx` 의 추상 API 만 호출하게 되며, 본 헤더는
// deprecated 후 제거 예정.
//
// lbx-intf 의 LBX_DMABUF_INFO 에 대한 의존성은 의도적으로 피함. 호스트가
// info 의 raw 필드 ({fd, offset, pitch, ...}) 를 직접 풀어 넘기는 형태.
//---------------------------------------------------------------------------

#ifndef lbx_gl_dmabufH
#define lbx_gl_dmabufH

#include "lbx_gl.h"
#include "intf/lbx_intf_avio.h"   /* LBX_GFX (백엔드 주입 vtable) */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LBX_GLES_VERSION

/* YUV 해석 hint. NULL 로 넘기면 디폴트 (REC709 / NARROW) — V4L2 카메라
 * 입력의 일반적인 가정. RGB 포맷이면 무시됨. */
typedef struct {
    EGLint color_space;    /* EGL_ITU_REC709_EXT / EGL_ITU_REC601_EXT / EGL_ITU_REC2020_EXT */
    EGLint sample_range;   /* EGL_YUV_NARROW_RANGE_EXT / EGL_YUV_FULL_RANGE_EXT */
} LBX_GL_DMABUF_HINT;

/**
 * @brief DMA-BUF 들로 EGLImageKHR 생성.
 *
 * @param egl_display   현재 EGLDisplay (보통 RC_Init 후의 ctx->egl_display)
 * @param width         이미지 너비 (픽셀)
 * @param height        이미지 높이 (픽셀)
 * @param drm_fourcc    DRM_FORMAT_* fourcc. V4L2 fourcc 와 다를 수 있으니
 *                      호출자가 미리 매핑한 값을 넘긴다 (lbx-intf 의
 *                      LBX_DMABUF_INFO.fourcc 는 이미 매핑된 값).
 * @param drm_modifier  DRM 64-bit modifier (DRM_FORMAT_MOD_LINEAR 등).
 *                      0 또는 DRM_FORMAT_MOD_INVALID 면 modifier attribute 생략.
 * @param n_planes      plane 수 (1~3 일반적, 최대 4 까지 EGL_EXT_image_dma_buf_import_modifiers).
 * @param fds           plane 별 DMA-BUF fd (n_planes 길이).
 * @param offsets       plane 별 byte offset (n_planes 길이).
 * @param pitches       plane 별 byte pitch (n_planes 길이).
 * @param hint          YUV color space / sample range hint. NULL = 디폴트.
 *
 * @return 성공 시 EGLImageKHR, 실패 시 EGL_NO_IMAGE_KHR.
 *
 * 생성된 image 는 caller 가 `lbx_gl_image_target_texture` 로 GL 텍스처에
 * 바인딩 후, 다 쓰면 `lbx_gl_destroy_dmabuf_image` 로 해제해야 한다.
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
 * @brief EGLImageKHR 해제.
 */
LBX_GL_EXPORT void lbx_gl_destroy_dmabuf_image(EGLDisplay egl_display, EGLImageKHR image);

/**
 * @brief 현재 바인딩된 GL 텍스처에 EGL image 의 내용을 attach.
 *
 * 호출자가 사전에 glBindTexture(target, tex_id) 와 glTexParameter*() 을
 * 끝낸 상태여야 함. glEGLImageTargetTexture2DOES 의 얇은 wrapper.
 *
 * @param target  GL_TEXTURE_EXTERNAL_OES (YUV 카메라 입력 일반) 또는 GL_TEXTURE_2D
 *                (확장이 받는 RGB 포맷).
 * @param image   lbx_gl_create_dmabuf_image 가 반환한 핸들.
 */
LBX_GL_EXPORT void lbx_gl_image_target_texture(GLenum target, EGLImageKHR image);

/**
 * @brief LBX_GFX 백엔드 생성 (GL 임시 구현) — 모델 A "백엔드 주입".
 *
 * host 가 1회 생성해 Open 후 LBX_AVIO_DEVICE.gfx 에 주입한다. 드라이버가
 * 자기 버퍼 lifecycle(Import/Update/Destroy)에서 호출 — 드라이버는 GL 을
 * 모르고 LBX_GFX vtable 만 안다. host 는 백엔드 생성·주입·draw 만.
 *
 * 백엔드는 스테이트리스 — 핸들은 드라이버의 버퍼별 영속 LBX_IMAGE 가
 * 보유(planes[0].texture: 부호 규약 음수=external OES/양수=2D/0=미생성,
 * user_data: DMA-BUF 의 EGLImageKHR). Import 는 버퍼당 1회 생성,
 * Update 는 CPU 재업로드(드라이버가 내용 변경 알 때만 호출, DMA-BUF 는
 * zero-copy no-op), Destroy 는 Import 와 대칭 파괴.
 *
 * vtable 호출은 렌더 컨텍스트 current 스레드에서 이뤄져야 한다.
 *
 * NOTE: lbx-gfx 가 lbx-gl 을 대체하면 이 구현을 lbx-gfx 로 이식. LBX_GFX
 *       seam 이 백엔드 중립이라 드라이버/host 무변경(이 create 만 교체).
 *
 * @param egl_display  현재 EGLDisplay (백엔드 ctx 에 보관)
 * @return LBX_GFX* (lbx_gl_backend_destroy 로 해제), 실패 시 NULL
 */
LBX_GL_EXPORT LBX_GFX *lbx_gl_backend_create(EGLDisplay egl_display);

/**
 * @brief LBX_GFX 백엔드 해제. host 가 컨텍스트 종료 시 1회.
 *        (텍스처/EGLImage 자체는 드라이버가 Close 에서 Destroy 로 회수.)
 */
LBX_GL_EXPORT void lbx_gl_backend_destroy(LBX_GFX *gfx);

#endif /* LBX_GLES_VERSION */

#ifdef __cplusplus
}
#endif

#endif
