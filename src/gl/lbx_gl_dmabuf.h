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
 * @brief LBX_IMAGE 한 장을 GL 텍스처에 갱신 — avio v0.2 단일 진입점.
 *
 * 텍스처 생성·소유·파괴는 lbx-gl 책임이다 (VK 백엔드도 자기 리소스로
 * 동일 패턴). host 는 glGenTextures 하지 않는다.
 *
 * 슬롯 키 = 드라이버-소유 영속 LBX_IMAGE 주소. 드라이버는 버퍼별로
 * LBX_IMAGE 를 영속 보유해야 한다(avio-file: 채널당 1개,
 * avio-v4l2: ring buffer 당 1개). planes[0].texture 는 lbx-gl 소유 —
 * 0 이면 최초 1회 생성, 드라이버는 이 필드를 리셋하지 않는다.
 *
 * 내부 동작:
 *   - planes[0].native_handle >= 0 (DMA-BUF fd, 실 V4L2):
 *       버퍼당 EGLImage+텍스처 1회 생성(GL_TEXTURE_EXTERNAL_OES). EGLImage 는
 *       그 버퍼 dmabuf 의 영속 view 라 이후 재타겟 불필요(V4L2 가 같은
 *       메모리에 새 프레임 채우면 샘플링 자동 갱신). 파괴는
 *       lbx_gl_release_texture 까지 미룸 → per-frame create/destroy 소멸 →
 *       임베디드 sibling-destroy 세그폴트 원천 제거.
 *       → planes[0].texture = -(tex)  (음수 = external OES)
 *   - native_handle < 0 (CPU 버퍼, avio-file): 텍스처 1회 생성,
 *       img->sequence 가 바뀔 때만 glTexImage2D 재업로드(정지영상은
 *       UI 로 이미지 바꿀 때만 sequence 변경 → 그때만 재업로드).
 *       → planes[0].texture = +(tex)  (양수 = 2D)
 *
 * 부호 규약을 lbx-gl 이 박으므로 host 는 분기 불요. 엔진의
 * LBX_IMAGE_BindTextures 가 부호 보고 알맞은 타겟에 바인딩.
 *
 * 렌더 컨텍스트 current 상태에서 호출돼야 한다.
 *
 * @param egl_display  현재 EGLDisplay
 * @param img          driver-소유 영속 LBX_IMAGE
 * @return 0 = 성공, <0 = 실패
 */
LBX_GL_EXPORT i32_t lbx_gl_update_texture(EGLDisplay egl_display, LBX_IMAGE *img);

/**
 * @brief 대칭 해제 — 드라이버가 버퍼/디바이스 정리(Close/Recover/REQBUFS(0))
 *        시 호출. 텍스처+EGLImage 파괴, 슬롯 비움, planes[0].texture=0 리셋
 *        (다음 update 때 재생성 → Recover 시 fd 변경 자동 대응).
 *        렌더 컨텍스트 current 상태에서 호출돼야 한다.
 */
LBX_GL_EXPORT void lbx_gl_release_texture(EGLDisplay egl_display, LBX_IMAGE *img);

/**
 * @brief 백스톱 — 렌더 컨텍스트 종료 시 1회. 드라이버가 release_texture 를
 *        빠뜨려도 lbx-gl 가 만든 모든 텍스처/EGLImage 회수.
 *        렌더 컨텍스트 current 상태에서 호출돼야 한다.
 */
LBX_GL_EXPORT void lbx_gl_release_all(EGLDisplay egl_display);

#endif /* LBX_GLES_VERSION */

#ifdef __cplusplus
}
#endif

#endif
