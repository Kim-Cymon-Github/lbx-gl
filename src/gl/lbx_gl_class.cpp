//---------------------------------------------------------------------------
#include <ctype.h>

#if defined(__BORLANDC__)
#   pragma hdrstop
#endif

#include "lbx_gl_class.h"
#include "GLES2/gl2ext.h"
#include "system/lbx_log.h"
#include "system/lbx_stream_file.h"
#include "system/lbx_serialize.h"
#include "image/lbx_image.h"

//---------------------------------------------------------------------------
#if defined(__BORLANDC__)
#   pragma package(smart_init)
#endif

UString GetGLStringData(GLuint obj, i32_t length, TGLGetStringFunc func)
{
    UString r;
    if (length > 1) {
        r.SetLength(length - 1);
        func(obj,  length,  NULL,  r.c_str());
    }
    return r;
}

i32_t dump_glprogram(void* dst, i32_t dst_size, GLuint h_prog, GLenum* bin_format)
{
#ifndef GL_GLEXT_PROTOTYPES
    static PFNGLGETPROGRAMBINARYOESPROC glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)eglGetProcAddress("glGetProgramBinaryOES");
#endif //#ifndef GL_GLEXT_PROTOTYPES
    GLint pl = 0, wl = 0, currentProgram = 0;
    GL_CHECK(glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram)); // 현재 활성화된 프로그램 핸들 백업 
    GL_CHECK(glUseProgram(h_prog));
#ifdef GL_PROGRAM_BINARY_LENGTH_OES
    GL_CHECK(glGetProgramiv(h_prog, GL_PROGRAM_BINARY_LENGTH_OES, &pl));
#endif //GL_PROGRAM_BINARY_LENGTH_OES
    if (dst == NULL || pl == 0 || dst_size < pl) {
        GL_CHECK(glUseProgram(currentProgram));
        return pl;
    }
    GL_CHECK(glGetProgramBinaryOES(h_prog, pl, &wl, bin_format, dst));
    GL_CHECK(glUseProgram(currentProgram));
    return (i32_t)wl;
}


void* svec_append_glprogram(void** dst, GLuint h_prog, GLenum* bin_format)
{
    i32_t pl = dump_glprogram(NULL, 0, h_prog, bin_format);
    size_t ol = 0;
    if (pl > 0) {
        void* p;
        ol = svec_inc_length(dst, pl, 1);
        p = (u8_t*)(*dst) + ol;
        i32_t wl = dump_glprogram(p, pl, h_prog, bin_format);
        if (wl == pl) {
            Dbg_("Saved program binary (%d bytes, format=0x%X)", wl, *bin_format);
            return p;
        } else {
            Warn_("Binary size mismatch - %d/%d bytes written", wl, pl);
            svec_set_length(dst, ol, 1);
        }
    }
    return NULL;
}

size_t stream_write_glprogram(LBX_STREAM* s, GLuint h_prog, GLenum *bin_format)
{
    GLenum bf = 0;
    void *buf = svec_from_glprogram(h_prog, &bf);
    size_t r = stream_write_box_header(s, bf, svec_size(buf));
    r += stream_write(s, buf, svec_size(buf));
    svec_free(&buf);
    if (bin_format) {
        *bin_format = bf;
    }
    return r;
}

size_t stream_read_glprogram(LBX_STREAM *s, GLuint h_prog, GLenum* bin_format)
{
    #ifndef GL_GLEXT_PROTOTYPES
    static PFNGLPROGRAMBINARYOESPROC glProgramBinaryOES = (PFNGLPROGRAMBINARYOESPROC)eglGetProcAddress("glProgramBinaryOES");
    #endif //#ifndef GL_GLEXT_PROTOTYPES
    size_t r, payload_size;
    u64_t bf;
    //GLenum err = GL_NO_ERROR;
    r = stream_read_box_header(s, &bf, &payload_size);
    if (r > 0) {
        void * tmp = svec_alloc(payload_size, 1);
        r += stream_read(s, tmp, payload_size);
        GL_CHECK(glProgramBinaryOES(h_prog, (GLenum)bf, tmp, (GLint)payload_size));
        if (bin_format) {
            *bin_format = (GLenum)bf;
        }
    }
    return r;
}





/////////////////////////////////////////////////////////////////////////////
TGLObject::TGLObject()
    : handle(0)
{

}
//---------------------------------------------------------------------------
TGLObject::~TGLObject()
{

}
//---------------------------------------------------------------------------
GLuint TGLObject::GetHandle(void)
{
    return handle;
}


/////////////////////////////////////////////////////////////////////////////
TLBTexture::TLBTexture()
/////////////////////////////////////////////////////////////////////////////
    : keep_image_data(false), inherited()
{
    LBX_IMAGE_Init(&image_format, 0, 0, fourcc_('R','G','B','A'), NULL);
}
//---------------------------------------------------------------------------
TLBTexture::~TLBTexture()
{
    LBX_IMAGE_Free(&image_format);
}
//---------------------------------------------------------------------------
i64_t TLBTexture::LoadFromFile(const char *file_name, i32_t target)
{
    i64_t ret = 0;
    LBX_STREAM st = {0,};
    stream_open_file(&st, file_name, "rb");
    ret = LoadFromStream(&st, target);
    stream_close(&st);
    return ret;
}
//---------------------------------------------------------------------------
i64_t TLBTexture::LoadFromStream(LBX_STREAM *s, i32_t target)
{
    i64_t ret = 0;
    LBX_IMAGE_Free(&image_format);
    if (LBX_IMAGE_LoadFromStream(&image_format, s) > 0) {
        ret = SetImage(&image_format, (i32_t)target);
        if (keep_image_data) {
        } else {
            LBX_IMAGE_Free(&image_format);
            LBX_IMAGE_DataToOffset(&image_format);
        }
    }
    return ret;
}
//---------------------------------------------------------------------------
size_t TLBTexture::Load(LBX_IMAGE *img)
{
    return 0;
}
//---------------------------------------------------------------------------
GLuint TLBTexture::GetHandle(void)
{
    if (handle == 0) {
        GL_CHECK_READY;
        GL_CHECK(glGenTextures(1, &handle));
    }
    return handle;
}

GLenum get_gl_format(fourcc_t pixel_format)
{
    GLenum fmt = 0;
    switch (pixel_format) {
        case fourcc_('R','G','B',' '):
        case fourcc_('R','G','B','3'):
        case fourcc_('R','G','B','8'):
            fmt = GL_RGB;
            break;
        case fourcc_('R','G','B','4'):
        case fourcc_('R','G','B','A'):
        case fourcc_('A','B','2','4'):
            fmt = GL_RGBA;
            break;
        case fourcc_('Y','8',' ',' '):
        case fourcc_('G','R','A','Y'):
        case fourcc_('G','R','E','Y'):
            fmt = GL_LUMINANCE;
            break;
        case fourcc_('Y','A',' ',' '):
            fmt = GL_LUMINANCE_ALPHA;
            break;
        default:
            Err_("Pixel format " FOURCC_VFMT " is not supported", FOURCC_VARG(pixel_format));
            break;
    }
    return fmt;
}

/////////////////////////////////////////////////////////////////////////////
TGLTexture3D::TGLTexture3D()
/////////////////////////////////////////////////////////////////////////////
    : inherited()
{

}
//---------------------------------------------------------------------------
TGLTexture3D::~TGLTexture3D()
{

}
//---------------------------------------------------------------------------
i32_t TGLTexture3D::SetImage(const LBX_IMAGE *img, i32_t target)
{
    size2_i16 sz = img->planes[0].size;
    GLenum fmt;

    fmt = get_gl_format(img->pixel_format);
    if (fmt) {
        Bind();
        GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + target, 0, fmt, sz.width, sz.height, 0, fmt, GL_UNSIGNED_BYTE, (void*)(img->planes[0].data)));

        GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        return 1;
    } else {
        return 0;
    }
}
//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
TGLTexture2D::TGLTexture2D()
    : inherited()
{
    memset(&image_format, 0, sizeof(LBX_IMAGE));
}
//---------------------------------------------------------------------------
TGLTexture2D::~TGLTexture2D()
{
    if (handle) {
        glDeleteTextures(1, &handle);
    }
}
//---------------------------------------------------------------------------
i32_t TGLTexture2D::SetImage(const LBX_IMAGE *img, i32_t target)
{
    //fourcc_t pf = img->pixel_format;
    size2_i16 sz = img->planes[0].size;
    GLenum fmt;

    GetHandle();
    if (handle == 0) {
        GL_CHECK(glGenTextures(1, &handle));
    }
    if (target == -1) {
        target = GL_TEXTURE_2D;
    }

    fmt = get_gl_format(img->pixel_format);
    if (fmt) {
        GLenum t = (target == GL_TEXTURE_2D) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
        GL_CHECK(glBindTexture(t, GetHandle()));
        GL_CHECK(glTexImage2D(target, 0, fmt, sz.width, sz.height, 0, fmt, GL_UNSIGNED_BYTE, (void*)(img->planes[0].data)));

        if (t == GL_TEXTURE_2D) {
        #if 1 // Mipmap 사용
            GL_CHECK(glTexParameteri(t, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL_CHECK(glTexParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
            GL_CHECK(glGenerateMipmap(t));
        #else
            GL_CHECK(glTexParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL_CHECK(glTexParameteri(t, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        #endif
        } else {
            GL_CHECK(glTexParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL_CHECK(glTexParameteri(t, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        }
        return 1;
    } else {
        return 0;
    }


}


static inline void xywh_to_rect(rect_f32 *dst, i32_t x, i32_t y, i32_t width, i32_t height, size2_i16 res)
{
    vec2_f32 sc = vec2_f32_(1.0f / (f32_t)res.width, 1.0f / (f32_t)res.height);
    dst->left = (f32_t)x * sc.x;
    dst->top = 1.0f - ((f32_t)y * sc.y);
    dst->right = (f32_t)(x + width) * sc.x;
    dst->bottom = 1.0f - ((f32_t)(y + height) * sc.y);
}

static inline void rect_normalize(rect_f32 *out, rect_i16 in, size2_i16 res)
{
    vec2_f32 sc  = vec2_f32_(1.0f / (f32_t)res.width, 1.0f / (f32_t)res.height);
    out->left   = (f32_t)in.left   * sc.x;
    out->top    = (f32_t)in.top    * sc.y;
    out->right  = (f32_t)in.right  * sc.x;
    out->bottom = (f32_t)in.bottom * sc.y;
}

u8_t idx_rect_triangles[] = {0,2,1,2,3,1};
u8_t idx_rect_triangle_strip[] = {0,2,1,3};
u8_t idx_border_triangles[] = {0,4,1,1,4,5,1,5,2,2,5,6,2,6,3,3,6,7,4,8,5,5,8,9,6,10,7,7,10,11,8,12,9,9,12,13,9,13,10,10,13,14,10,14,11,11,14,15,5,9,6,6,9,10};
u8_t idx_border_triangle_strip[] = {0,4,1,5,2,6,3,7,7,7,6,11,10,15,14,14,14,10,13,9,12,8,8,8,9,4,5,5,5,9,6,10};


/////////////////////////////////////////////////////////////////////////////
TGlyph::TGlyph(TGLTexture2D *tex)
    : fit(NULL), border(NULL), flags(0), tex_coord(NULL)
{
    texture = tex;
    memset(&scr_boundary, 0, sizeof(scr_boundary));
    memset(&boundary, 0, sizeof(boundary));
    scale = vec2_f32_(1.0, 1.0);
}
//---------------------------------------------------------------------------
TGlyph::~TGlyph()
{
    ClearBase();
    ClearBorder();
    SVEC_FREE(&tex_coord);
}
//---------------------------------------------------------------------------
void TGlyph::ClearBase(void)
{
    delete fit;
    fit = NULL;
}
//---------------------------------------------------------------------------
void TGlyph::ClearBorder(void)
{
    delete border;
    border = NULL;
}
//---------------------------------------------------------------------------
void TGlyph::SetSourceRegion(rect_i16 rect)
{
    size2_i16 res = texture->image_format.planes[0].size;
    scr_boundary = rect;
    rect_normalize(&boundary, rect, res);
}
//---------------------------------------------------------------------------
void TGlyph::FitTo(rect_i16 rect)
{
    if (NULL == fit) {
        fit = new rect_f32();
    }
    rect_normalize(fit, rect, texture->image_format.planes[0].size);
    rect_f32_subeq(fit, boundary);
}
//---------------------------------------------------------------------------
void TGlyph::SetBorder(rect_i16 rect, bool perforated)
{
    if (NULL == border) {
        border = new rect_f32();
    }
    rect_normalize(border, rect, texture->image_format.planes[0].size);
    rect_f32_subeq(border, boundary);
    if (perforated) {
        flags |= gfHasHole;
    } else {
        flags &= ~gfHasHole;
    }
}
//---------------------------------------------------------------------------
void TGlyph::Update(void)
{
    if (border) {
        SVEC_SET_LENGTH(vec2_f32, &tex_coord, 16);
        tex_coord[ 0] = vec2_f32_(boundary.left, boundary.top);
        tex_coord[15] = vec2_f32_(boundary.right, boundary.bottom);
        tex_coord[ 5] = vec2_f32_(tex_coord[ 0].x + border->left, tex_coord[ 0].y + border->top);
        tex_coord[10] = vec2_f32_(tex_coord[15].x + border->right, tex_coord[15].y + border->bottom);

        tex_coord[ 1] = vec2_f32_(tex_coord[ 5].x, tex_coord[ 0].y);
        tex_coord[ 2] = vec2_f32_(tex_coord[10].x, tex_coord[ 0].y);
        tex_coord[ 3] = vec2_f32_(tex_coord[15].x, tex_coord[ 0].y);

        tex_coord[ 4] = vec2_f32_(tex_coord[ 0].x, tex_coord[ 5].y);
        tex_coord[ 6] = vec2_f32_(tex_coord[10].x, tex_coord[ 5].y);
        tex_coord[ 7] = vec2_f32_(tex_coord[15].x, tex_coord[ 5].y);

        tex_coord[ 8] = vec2_f32_(tex_coord[ 0].x, tex_coord[10].y);
        tex_coord[ 9] = vec2_f32_(tex_coord[ 5].x, tex_coord[10].y);
        tex_coord[11] = vec2_f32_(tex_coord[15].x, tex_coord[10].y);

        tex_coord[12] = vec2_f32_(tex_coord[ 0].x, tex_coord[15].y);
        tex_coord[13] = vec2_f32_(tex_coord[ 5].x, tex_coord[15].y);
        tex_coord[14] = vec2_f32_(tex_coord[10].x, tex_coord[15].y);
    } else {
        SVEC_SET_LENGTH(vec2_f32, &tex_coord, 4);
        tex_coord[0] = vec2_f32_(boundary.left, boundary.top);
        tex_coord[1] = vec2_f32_(boundary.right, boundary.top);
        tex_coord[2] = vec2_f32_(boundary.left, boundary.bottom);
        tex_coord[3] = vec2_f32_(boundary.right, boundary.bottom);
    }
}
//---------------------------------------------------------------------------
i32_t TGlyph::GetVertexCount(void)
{
    return (border ? 16 : 4);
}
//---------------------------------------------------------------------------
#define PTR_OFFSET(ptr, offset) ((u8_t*)ptr + offset)
i32_t TGlyph::FillVertexList(vec2_f32 *target, rect_f32 area, i32_t target_stride)
{
    size2_i16 res = texture->image_format.planes[0].size;

    Update();
    if (target_stride == 0) {
        target_stride = sizeof(*target);
    }
    vec2_f32 *p[16];
    i32_t vtx_count = (border) ? 16 : 4;
    for (i32_t i = 0; i < vtx_count; i++) {
        p[i] = (vec2_f32*)((u8_t*)target + target_stride * i);
    }

    // 좌상, 우하 좌표부터 먼저 계산함
    if (fit) {
        vec2_f32 sc = vec2_f32_(
                RECT_WIDTH(area) / (RECT_WIDTH(boundary) + RECT_WIDTH(*fit)),
                RECT_HEIGHT(area) / (RECT_HEIGHT(boundary) + RECT_HEIGHT(*fit))
            );
        *p[0] = vec2_f32_(area.left - fit->left * sc.x, area.top - fit->top * sc.y);
        *p[vtx_count - 1] = vec2_f32_(area.right - fit->right * sc.x, area.bottom - fit->bottom * sc.y);
    } else {
        *p[0] = vec2_f32_(area.left, area.top);
        *p[vtx_count - 1] = vec2_f32_(area.right, area.bottom);
    }


    if (border) {
        *p[ 5] = vec2_f32_(p[ 0]->x + border->left * (f32_t)res.width * scale.x, p[ 0]->y + border->top * (f32_t)res.height * scale.y);
        *p[10] = vec2_f32_(p[15]->x + border->right * (f32_t)res.width * scale.x, p[15]->y + border->bottom * (f32_t)res.height * scale.y);

        *p[ 1] = vec2_f32_(p[ 5]->x, p[0]->y);
        *p[ 2] = vec2_f32_(p[10]->x, p[0]->y);
        *p[ 3] = vec2_f32_(p[15]->x, p[0]->y);

        *p[ 4] = vec2_f32_(p[ 0]->x, p[5]->y);
        *p[ 6] = vec2_f32_(p[10]->x, p[5]->y);
        *p[ 7] = vec2_f32_(p[15]->x, p[5]->y);

        *p[ 8] = vec2_f32_(p[ 0]->x, p[10]->y);
        *p[ 9] = vec2_f32_(p[ 5]->x, p[10]->y);
        *p[11] = vec2_f32_(p[15]->x, p[10]->y);

        *p[12] = vec2_f32_(p[ 0]->x, p[15]->y);
        *p[13] = vec2_f32_(p[ 5]->x, p[15]->y);
        *p[14] = vec2_f32_(p[10]->x, p[15]->y);

        return 16;
    } else {
        *p[1] = vec2_f32_(p[3]->x, p[0]->y);
        *p[2] = vec2_f32_(p[0]->x, p[3]->y);
    }
    return 4;
}
//---------------------------------------------------------------------------
const u8_t *TGlyph::GetDrawIndice(i32_t *count)
{
    if (border) {
        if (count) {
            *count = sizeof(idx_border_triangles) / sizeof(*idx_border_triangles);
            if (flags & gfHasHole) {
                *count -= 6;
            }
        }
        return idx_border_triangles;
    } else {
        if (count) {
            *count = sizeof(idx_rect_triangles) / sizeof(*idx_border_triangles);
        }
        return idx_rect_triangles;
    }
}
//---------------------------------------------------------------------------
i32_t TGlyph::BuildDrawList(void *buffer, const LBX_GLBUFFER_DESC *bi, i16_t *indice, rect_f32 target)
{
    i32_t stride;
    u8_t *b = (u8_t *)buffer;

    if (!(LBX_TYPE_ELEMTYPE(bi->vertex.type) == LBX_TYPE_F32 &&
        LBX_TYPE_ELEMTYPE(bi->tex_coord.type) == LBX_TYPE_F32)) {
        Err_("Cannot build drawlist");
        return 0;
    }

    stride = bi->stride;
    if (border == NULL) {
        if (fit == NULL) {
            b = (u8_t *)buffer + bi->vertex.offset;
            *(vec2_f32*)(b + stride * 0) = vec2_f32_(target.left, target.top);
            *(vec2_f32*)(b + stride * 1) = vec2_f32_(target.right, target.top);
            *(vec2_f32*)(b + stride * 2) = vec2_f32_(target.left, target.bottom);
            *(vec2_f32*)(b + stride * 3) = vec2_f32_(target.right, target.bottom);
            b = (u8_t *)buffer + bi->tex_coord.offset;
/*
            *(vec2_f32*)(b + stride * 0) = vec2_f32_(target.left, target.top);
            *(vec2_f32*)(b + stride * 1) = vec2_f32_(target.right, target.top);
            *(vec2_f32*)(b + stride * 2) = vec2_f32_(target.left, target.bottom);
            *(vec2_f32*)(b + stride * 3) = vec2_f32_(target.right, target.bottom);
*/
            indice[0] = 0;
            indice[1] = 2;
            indice[2] = 1;
            indice[3] = 2;
            indice[4] = 3;
            indice[5] = 1;
        } else {
        }
    } else {

    }

    return 1;
}
//---------------------------------------------------------------------------






/////////////////////////////////////////////////////////////////////////////
TGLShader::TGLShader(GLenum type)
    : TGLObject(), flags(0)
{
    CreateGLObject(type);
/*
    char test[1024];
    GLsizei a,b;
    GL_CHECK(glGetShaderSource(shader,  1024,  &a,  test));
*/
}
//---------------------------------------------------------------------------
TGLShader::TGLShader(GLenum type, LBX_STREAM *strm)
{
    CreateGLObject(type);
    SetSource(strm);
}
//---------------------------------------------------------------------------
TGLShader::TGLShader(GLenum type, const char *code, const char *hdr)
{
    CreateGLObject(type);
    SetSource(code, hdr);
}
//---------------------------------------------------------------------------
TGLShader::TGLShader(GLenum type, const char *code, i32_t code_len, const char *hdr, i32_t hdr_len)
{
    CreateGLObject(type);
    SetSource(code, code_len, hdr, hdr_len);
}
//---------------------------------------------------------------------------
TGLShader::~TGLShader()
{
    glDeleteShader(handle);
}
//---------------------------------------------------------------------------
void TGLShader::CreateGLObject(GLenum type)
{
    if (handle == 0) {
        handle = glCreateShader(type);
    }
}
//---------------------------------------------------------------------------
i32_t TGLShader::GetIntParam(GLenum pname)
{
    GLint info = 0;
    GL_CHECK(glGetShaderiv(handle, pname, &info));
    return info;
}
//---------------------------------------------------------------------------
i32_t TGLShader::Load(const void *data, i32_t length, GLenum binaryformat)
{
    Log_("glShaderBinary...");
    GL_CHECK(glShaderBinary(1, (GLuint *)&handle, binaryformat, data, length));
    if (glGetError() == GL_NO_ERROR) {
        Log_("Shader binary loaded successfully.");
        return 1;
    } else {
        Err_("Error loading shader binary: %s", GetInfoLog().c_str());
        return 0;
    }
}
//---------------------------------------------------------------------------
i32_t TGLShader::SetBinary(LBX_STREAM *strm)
{
    i32_t ret;
    u8_t *b = NULL;
    i32_t l = (i32_t)stream_get_size(strm);
    SVEC_SET_LENGTH(u8_t, &b, l);
    stream_read(strm, b, l);
    Log_("%d bytes loaded", l);


/*
    i32_t format_cnt = 0;
    i32_t formats[10];
    GL_CHECK(glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &format_cnt));
    Log_("%d formats supported", format_cnt);
    GL_CHECK(glGetIntegerv(GL_SHADER_BINARY_FORMATS, formats));
    for (i32_t i = 0; i < format_cnt; i++) {
        Log_("%d: %d(GL_MALI_SHADER_BINARY_ARM=%d)", i, formats[i], GL_MALI_SHADER_BINARY_ARM);
    }
*/
    ret = Load(b, l, GL_MALI_SHADER_BINARY_ARM);
    SVEC_FREE(&b);
    return ret;

}
//---------------------------------------------------------------------------
i32_t TGLShader::SetSource(GLint count, const char **codes, const GLint *lengths)
{
    if (handle == 0) {
        return 0;
    }
    GLuint shader = GetHandle();
    // Load the shader source
    GL_CHECK(glShaderSource(shader, count, codes, lengths));
    // Compile the shader
    GL_CHECK(glCompileShader(shader));

    if (GetCompileStatus()) {
        flags |= (u32_t)(sfCompiled);
    } else {
        flags &= ~(u32_t)(sfCompiled);
        Err_("Error compiling shader: %s", GetInfoLog().c_str());
        return 0;
    }
    return 1;
}
//---------------------------------------------------------------------------
i32_t TGLShader::SetSource(const char *code, const char *hdr)
{
    if (code == NULL) {
        return 0;
    }
    if (hdr == NULL) {
        return SetSource(1, &code, NULL);
    } else {
        const char * s[2];
        s[0] = hdr;
        s[1] = code;
        return SetSource(2, s, NULL);
    }
}
//---------------------------------------------------------------------------
i32_t TGLShader::SetSource(const char *code, i32_t code_len, const char *hdr, i32_t hdr_len)
{
    if (code == NULL) {
        return 0;
    }

    const char *s[2];
    GLint l[2];

    s[0] = code;
    l[0] = (code_len == -1) ? (GLint)strlen(code) : code_len;

    if (hdr) {
        s[1] = hdr;
        l[1] = (hdr_len == -1) ? (GLint)strlen(hdr) : hdr_len;
        return SetSource(2, s, l);
    }

    return SetSource(1, s, l);
}
//---------------------------------------------------------------------------
i32_t TGLShader::SetSource(LBX_STREAM *strm)
{
    UString code;
    i32_t l = (i32_t)stream_get_size(strm);
    code.SetLength(l);
    stream_read(strm, code.c_str(), l);
    return SetSource(code.c_str(), l);
}
//---------------------------------------------------------------------------
bool TGLShader::IsCompiled(void)
{
    return flags & (u32_t)(sfCompiled);
}

//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
TGLVertexShader::TGLVertexShader()
    : TGLShader(GL_VERTEX_SHADER)
{

}
//---------------------------------------------------------------------------
TGLVertexShader::TGLVertexShader(LBX_STREAM *strm)
    : TGLShader(GL_VERTEX_SHADER, strm)
{

}
//---------------------------------------------------------------------------
TGLVertexShader::TGLVertexShader(const char *code, GLint length)
    : TGLShader(GL_VERTEX_SHADER, code, length)
{

}
//---------------------------------------------------------------------------
TGLVertexShader::TGLVertexShader(const char *hdr, const char *body)
    : TGLShader(GL_VERTEX_SHADER, hdr, body)
{

}
//---------------------------------------------------------------------------
TGLVertexShader::~TGLVertexShader()
{

}
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
TGLFragmentShader::TGLFragmentShader()
    : TGLShader(GL_FRAGMENT_SHADER)
{

}
//---------------------------------------------------------------------------
TGLFragmentShader::TGLFragmentShader(LBX_STREAM *strm)
    : TGLShader(GL_FRAGMENT_SHADER, strm)
{

}
//---------------------------------------------------------------------------
TGLFragmentShader::TGLFragmentShader(const char *code, GLint length)
    : TGLShader(GL_FRAGMENT_SHADER, code, length)
{

}
//---------------------------------------------------------------------------
TGLFragmentShader::TGLFragmentShader(const char *hdr, const char *body)
    : TGLShader(GL_FRAGMENT_SHADER, hdr, body)
{

}
//---------------------------------------------------------------------------
TGLFragmentShader::~TGLFragmentShader()
{

}
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
TGLProgram::TGLProgram()
    : TGLObject(), flags(0), attrib_types(NULL), attrib_data(NULL)
{
    shaders[0] = shaders[1] = NULL;

}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(const char *file_name, GLenum bin_format)
    : TGLObject(), flags(0), attrib_types(NULL), attrib_data(NULL)
{
    shaders[0] = shaders[1] = NULL;
    LoadFromFile(file_name, bin_format);
}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(TGLVertexShader *v, TGLFragmentShader * f)
    : TGLObject(), flags(0), attrib_types(NULL), attrib_data(NULL)
{
    shaders[0] = v;
    shaders[1] = f;
}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(const char *vshader, const char *fshader, const char *hdr)
    : TGLObject(), flags(0), attrib_types(NULL), attrib_data(NULL)
{
    Build(vshader, fshader, hdr);
}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(TGLVertexShader *vshader, const char *fshader, const char *hdr)
    : TGLObject(), flags(0), attrib_types(NULL), attrib_data(NULL)
{
    Build(vshader, fshader, hdr);
}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(const char *vshader, TGLFragmentShader *fshader, const char *hdr)
    : TGLObject(), flags(0), attrib_types(NULL), attrib_data(NULL)
{
    Build(vshader, fshader, hdr);
}
//---------------------------------------------------------------------------
TGLProgram::~TGLProgram()
{
    if (handle) {
        // glDetachShader가 자동으로 호출되는지 확인할 것
        for (i32_t i = 0; i < 2; i++) {
            if (shaders[i]) {
                GL_CHECK(glDetachShader(handle, shaders[i]->GetHandle()));
            }
            SetShader(i, NULL);
        }
        GL_CHECK(glDeleteProgram(handle));
    }
    SVEC_FREE(&attrib_types);
    for (i32_t i = svec_len32(attrib_data) - 1; i >= 0; i--) {
        free_memory(attrib_data[i]);
    }
    SVEC_FREE(&attrib_data);
}
//---------------------------------------------------------------------------
GLuint TGLProgram::GetHandle(void)
{
    if (handle == 0) {
        handle = glCreateProgram();
    }
    return handle;
}
//---------------------------------------------------------------------------
void TGLProgram::SetShader(i32_t index, TGLShader *shader)
{
    u32_t f = ((u32_t)(pfOwnsVertexShader) << index);
    if (flags & f) {
        delete shaders[index];
        flags &= ~f;
    }
    shaders[index] = shader;
}
//---------------------------------------------------------------------------
void TGLProgram::SetShader(TGLShader *shader)
{
    if (shader) {
        GLenum shader_type = shader->GetShaderType();
        if (shader_type == GL_VERTEX_SHADER) {
            SetShader(0, shader);
        } else if (shader_type == GL_FRAGMENT_SHADER) {
            SetShader(1, shader);
        }
    }
}
//---------------------------------------------------------------------------
void TGLProgram::SetVertexShader(TGLVertexShader *shader)
{
    if (shader == NULL || shader->GetShaderType() == GL_VERTEX_SHADER) {
        SetShader(0, shader);
    }
}
//---------------------------------------------------------------------------
void TGLProgram::SetFragmentShader(TGLFragmentShader *shader)
{
    if (shader == NULL || shader->GetShaderType() == GL_FRAGMENT_SHADER) {
        SetShader(1, shader);
    }
}
//---------------------------------------------------------------------------
TGLVertexShader * TGLProgram::GetVertexShader(void)
{
    return (TGLVertexShader *)(shaders[0]);
}
//---------------------------------------------------------------------------
TGLFragmentShader * TGLProgram::GetFragmentShader(void)
{
    return (TGLFragmentShader *)(shaders[1]);
}
//---------------------------------------------------------------------------
i32_t TGLProgram::GetIntParam(GLenum pname)
{
    GLint info = 0;
    GL_CHECK(glGetProgramiv(GetHandle(), pname, &info));
    return info;
}
//---------------------------------------------------------------------------
i64_t TGLProgram::LoadFromFile(const char* file_name)
{
    LBX_STREAM* s = new_file_stream(file_name, "rb");
    i64_t r = LoadFromStream(s);
    delete_stream(s);
    return r;
}
//---------------------------------------------------------------------------
i64_t TGLProgram::LoadFromFile(const char *file_name, GLenum bin_format)
{
    LBX_STREAM *s = new_file_stream(file_name, "rb");
    i64_t r = LoadFromStream(s, static_cast<i32_t>(stream_get_size(s)), bin_format);
    delete_stream(s);
    return r;
}
//---------------------------------------------------------------------------
i64_t TGLProgram::LoadFromStream(LBX_STREAM* s)
{
    size_t sz = 0;
    u64_t bf = 0;
    i64_t r = stream_read_box_header(s, &bf, &sz);
    if (r > 0) {
        void* buf = alloc_memory(sz);
        stream_read(s, buf, sz);
        r = LoadBinary(buf, sz, (GLenum)bf);
        free_memory(buf);
    }
    return r;
}
//---------------------------------------------------------------------------
i64_t TGLProgram::LoadFromStream(LBX_STREAM *s, size_t size, GLenum bin_format)
{
    void * buf = alloc_memory(size);
    stream_read(s, buf, size);
    i64_t r = LoadBinary(buf, size, bin_format);
    free_memory(buf);
    return r;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::LoadBinary(const void *data, size_t size, GLenum bin_format)
{
    #ifndef GL_GLEXT_PROTOTYPES
    static PFNGLPROGRAMBINARYOESPROC glProgramBinaryOES = (PFNGLPROGRAMBINARYOESPROC)eglGetProcAddress("glProgramBinaryOES");
    #endif //#ifndef GL_GLEXT_PROTOTYPES
    flags &= ~(u32_t)pfLinked;
    glProgramBinaryOES(GetHandle(), bin_format, data, (GLint)size);
    if (glGetError() == GL_NO_ERROR) {
        flags |= (u32_t)pfLinked;
        Analyze();
        return 1;
    } else {
        return 0;
    }
}
//---------------------------------------------------------------------------
i32_t TGLProgram::SaveBinary(void *dst, i32_t dst_size, GLenum *bin_format)
{
    return dump_glprogram(dst, dst_size, GetHandle(), bin_format);
}
//---------------------------------------------------------------------------
size_t TGLProgram::SaveToMem(void **p_svec, GLenum *bin_format)
{
    void *p = svec_append_glprogram(p_svec, GetHandle(), bin_format);
    return svec_size(*p_svec) - ((u8_t*)p - (u8_t*)(*p_svec));
}
//---------------------------------------------------------------------------
i64_t TGLProgram::SaveToFile(const char *file_name, GLenum* bin_format)
{
    LBX_STREAM *s = new_file_stream(file_name, "wb");
    i64_t r = SaveToStream(s, bin_format);
    delete_stream(s);
    return r;
}
//---------------------------------------------------------------------------
i64_t TGLProgram::SaveToStream(LBX_STREAM *s, GLenum* bin_format)
{
    stream_write_glprogram(s, GetHandle(), bin_format);
    void *buf = NULL;
    GLenum bf = 0;
    i64_t r = 0;

    r = SaveToMem(&buf, &bf);
    if (r > 0) {
        if (bin_format) {
            *bin_format = bf;
        }
        r = stream_write_box_header(s, bf, r);
        r += stream_write(s, buf, svec_size(buf));
    }

    svec_free(&buf);
    return r;
}
//---------------------------------------------------------------------------
TGLShader * TGLProgram::CreateShader(GLenum type, const char *src, const char *hdr)
{
    TGLShader *ns = NULL;
    i32_t i = -1;
    if (type == GL_VERTEX_SHADER) {
        i = 0;
        ns = new TGLVertexShader(src, hdr);
    } else if (type == GL_FRAGMENT_SHADER) {
        i = 1;
        ns = new TGLFragmentShader(src, hdr);
    }

    if (ns) {
        SetShader(i, NULL);
        if (ns->IsCompiled()) {
            shaders[i] = ns;
            flags |= ((u32_t)pfOwnsVertexShader << i);
        } else {
            delete ns;
            ns = NULL;
        }
    }

    return ns;
}
//---------------------------------------------------------------------------
TGLVertexShader * TGLProgram::CreateVertexShader(const char *src, const char *hdr)
{
    return (TGLVertexShader *)CreateShader(GL_VERTEX_SHADER, src, hdr);
}
//---------------------------------------------------------------------------
TGLFragmentShader * TGLProgram::CreateFragmentShader(const char *src, const char *hdr)
{
    return (TGLFragmentShader *)CreateShader(GL_FRAGMENT_SHADER, src, hdr);
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Build(TGLVertexShader *vshader, const char *fshader, const char *hdr)
{
    SetShader(vshader);
    return CreateShader(GL_FRAGMENT_SHADER, fshader, hdr) ? Link() : 0;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Build(const char *vshader, TGLFragmentShader *fshader, const char *hdr)
{
    SetShader(fshader);
    return CreateShader(GL_VERTEX_SHADER, vshader, hdr) ? Link() : 0;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Build(const char *vshader, const char *fshader, const char *hdr)
{
    if (CreateShader(GL_VERTEX_SHADER, vshader, hdr) &&
        CreateShader(GL_FRAGMENT_SHADER, fshader, hdr)) {
        return Link();
    }
    return 0;
}
//---------------------------------------------------------------------------

/*
const UString g_POSITION    = "position";
const UString g_NORMAL      = "normal";
const UString g_TANGENT     = "tangent";
const UString g_TEXCOORD    = "texcoord";
const UString g_COLOR       = "color";
const UString g_JOINT       = "joint";
const UString g_WEIGHT       = "weight";
*/

//---------------------------------------------------------------------------
u8_t EstimateAttribTypeFromName(const char *param_name, i32_t l)
{
    u8_t type = 0u;
    UString lc;
    const char *name = NULL;
    const char *found = NULL;

    if (l == -1) {
        l = static_cast<i32_t>(strlen(param_name));
    }

    lc = ustr_(param_name, l);
    name = lc.LowerCase().c_str();

    if ((found = strstr(name, "position")) != NULL) {
        type = LBX_POSITION;
        found += 8;
    } else if ((found = strstr(name, "normal")) != NULL) {
        type = LBX_NORMAL;
        found += 6;
    } else if ((found = strstr(name, "tangent")) != NULL) {
        type = LBX_TANGENT;
        found += 7;
    } else if ((found = strstr(name, "color")) != NULL) {
        type = LBX_COLOR;
        found += 5;
    } else if ((found = strstr(name, "coord")) != NULL && strstr(name, "tex")) {
        type = LBX_TEXCOORD;
        found += 5;
    } else if ((found = strstr(name, "joint")) != NULL) {
        type = LBX_JOINTS;
        found += 5;
    } else if ((found = strstr(name, "weight")) != NULL) {
        type = LBX_WEIGHTS;
        found += 6;
    } else if ((found = strstr(name, "vertex")) != NULL) {
        type = LBX_POSITION;
        found += 6;
    } else if ((found = strstr(name, "vtx")) != NULL) {
        type = LBX_POSITION;
        found += 3;
    } else if ((found = strstr(name, "uv")) != NULL) {
        type = LBX_TEXCOORD;
        found += 2;
    } else if ((found = strstr(name, "txc")) != NULL) {
        type = LBX_TEXCOORD;
        found += 3;
    } else if ((found = strstr(name, "col")) != NULL) {
        type = LBX_COLOR;
        found += 3;
    } else if ((found = strstr(name, "nor")) != NULL) {
        type = LBX_NORMAL;
        found += 3;
    } else {
        return type;
    }

    // 번호가 있는 경우 이를 감지해서 반영
    for (; *found != 0; found++) {
        char c = *found;
        if (c >= '0' && c <= '9') {
            type |= (u8_t)(c - '0');
        }
    }

    return type;

}

void TGLProgram::Analyze(void)
{
    //i32_t missing_count = 0;
    i32_t attrib_count = GetIntParam(GL_ACTIVE_ATTRIBUTES);
    i32_t max_length = GetIntParam(GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
    SVEC_SET_LENGTH(u8_t, &attrib_types, attrib_count);
    SVEC_SET_LENGTH(void *, &attrib_data, attrib_count);
    memset(attrib_types, 0, sizeof(u8_t) * attrib_count);
    memset(attrib_data, 0, sizeof(void *) * attrib_count);

    UString name;
    name.SetLength(max_length);

    for (i32_t i = 0; i < attrib_count; i++) {
        GLenum type;
        GLint length;
        GLint size;
        GL_CHECK(glGetActiveAttrib(handle, (GLuint)i, max_length, &length, &size, &type, name.c_str()));
        GLint loc = GetAttribLocation(name.c_str());
        if (loc >= 0 && loc < attrib_count) {
            attrib_types[loc] = EstimateAttribTypeFromName(name.c_str(), length);
            Log_("Attribute #%d Type: %u Name: %s, Size=%d, Loc=%d, AttribType=%d", i, type, name.c_str(), size, loc, attrib_types[loc]);
        } else {
            Err_("Attribute #%d location %d is out of range", i, loc);
        }
    }


}
//---------------------------------------------------------------------------
/*
TGLProgram::AttributeDescriptor * TGLProgram::FindAttributeInfoByType(TGLAttributeType type, i16_t num)
{
    i32_t l = svec_len32(attrib_info);
    for (i32_t i = 0; i < l; i++) {
        if (attrib_info[i].type == type && attrib_info[i].idx == num) {
            return attrib_info + i;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
*/
/*
TGLProgram::AttributeDescriptor * TGLProgram::FindAttributeInfoByLoc(TGLAttributeType type, i16_t num)
{
    i32_t l = svec_len32(attrib_info);
    for (i32_t i = 0; i < l; i++) {
        if (attrib_info[i].type == type && attrib_info[i].idx == num) {
            return attrib_info + i;
        }
    }
    return NULL;
}
*/
i32_t TGLProgram::LocationOfAttribType(u8_t type)
{
    i32_t l = svec_len32(attrib_types);
    for (i32_t i = 0; i < l; i++) {
        if (attrib_types[i] == type) {
            return i;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
void TGLProgram::SetAttributeType(i32_t location, u8_t type)
{
    i32_t l = svec_len32(attrib_types);
    assert(location >= 0 && location < l);
    attrib_types[location] = type;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Link(void)
{
    GLuint program = GetHandle();
    flags &= ~(u32_t)pfLinked;
    for (i32_t i = 0; i < 2; i++) {
        if (shaders[i]) {
            GL_CHECK(glAttachShader(program, shaders[i]->GetHandle()));
        }
    }
    GL_CHECK(glLinkProgram(program));

    if (GetLinkStatus()) {
        flags |= (u32_t)pfLinked;
        Analyze();
    } else {
        Err_("Error linking program: %s", GetInfoLog().c_str());
        return 0;
    }
    return 1;
}
//---------------------------------------------------------------------------
bool TGLProgram::IsLinked(void)
{
    return (flags & (u32_t)pfLinked);
}

//---------------------------------------------------------------------------
void TGLProgram::DisableArrayMode(i32_t location)
{
    Use();
    if (location == -1) {
        i32_t l = svec_len32(attrib_types);
        for (i32_t i = 0; i < l; i++) {
            GL_CHECK(glDisableVertexAttribArray(i));
        }
    } else {
        GL_CHECK(glDisableVertexAttribArray(location));
    }
}
//---------------------------------------------------------------------------
void TGLProgram::EnableArrayMode(i32_t location)
{
    Use();
    if (location == -1) {
        i32_t l = svec_len32(attrib_types);
        for (i32_t i = 0; i < l; i++) {
            GL_CHECK(glEnableVertexAttribArray(i));
        }
    } else {
        GL_CHECK(glEnableVertexAttribArray(location));
    }
}
//---------------------------------------------------------------------------
void TGLProgram::Attribute(GLint attrib_loc, GLint components, GLenum component_type, GLboolean normalize, i32_t stride, const void *offset)
{
    GL_CHECK(glEnableVertexAttribArray(attrib_loc));
    GL_CHECK(glVertexAttribPointer(attrib_loc, components, component_type, normalize, stride, offset));
}
//---------------------------------------------------------------------------
void TGLProgram::Attribute(GLint attrib_loc, vec4_u8 data)
{
    GL_CHECK(glDisableVertexAttribArray(attrib_loc));
    vec4_f32 n = vec4_f32_((f32_t)data.x / 255.0f, (f32_t)data.y / 255.0f, (f32_t)data.z / 255.0f, (f32_t)data.w / 255.0f);
    GL_CHECK(glVertexAttrib4fv(attrib_loc, (f32_t*)&n));
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const f32_t *data, i32_t count)
{
    Use(); GL_CHECK(glUniform1fv(uniform_loc, count, data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const vec2_f32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniform2fv(uniform_loc, count, (f32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const vec3_f32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniform3fv(uniform_loc, count, (f32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const vec4_f32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniform4fv(uniform_loc, count, (f32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const i32_t *data, i32_t count)
{
    Use(); GL_CHECK(glUniform1iv(uniform_loc, count, data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const vec2_i32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniform2iv(uniform_loc, count, (i32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const vec3_i32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniform3iv(uniform_loc, count, (i32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const vec4_i32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniform4iv(uniform_loc, count, (i32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const mat2_f32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniformMatrix2fv(uniform_loc, count, GL_FALSE, (f32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const mat3_f32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniformMatrix3fv(uniform_loc, count, GL_FALSE, (f32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, const mat4_f32 *data, i32_t count)
{
    Use(); GL_CHECK(glUniformMatrix4fv(uniform_loc, count, GL_FALSE, (f32_t*)data)); return 1;
}
//---------------------------------------------------------------------------
i32_t TGLProgram::Uniform(GLint uniform_loc, LBX_TYPE type, const void *data, i32_t count)
{
    //i32_t dim = LBX_TYPE_DIMENSION(type);
    bool is_mat = LBX_TYPE_ISMATRIX(type);
    switch (LBX_TYPE_ELEMTYPE(type)) {
        case LBX_TYPE_F32:
            if (is_mat) {
                switch (LBX_TYPE_DIMENSION(type)) {
                    case 2: return Uniform(uniform_loc, (mat2_f32*)data, count);
                    case 3: return Uniform(uniform_loc, (mat3_f32*)data, count);
                    case 4: return Uniform(uniform_loc, (mat4_f32*)data, count);
                    default: break;
                }
            } else {
                switch (LBX_TYPE_DIMENSION(type)) {
                    case 1: return Uniform(uniform_loc, (f32_t*)data, count);
                    case 2: return Uniform(uniform_loc, (vec2_f32*)data, count);
                    case 3: return Uniform(uniform_loc, (vec3_f32*)data, count);
                    case 4: return Uniform(uniform_loc, (vec4_f32*)data, count);
                    default: break;
                }
            }
            break;
        case LBX_TYPE_F64:
        case LBX_TYPE_U8:
        case LBX_TYPE_U16:
        case LBX_TYPE_U32:
        case LBX_TYPE_U64:
        case LBX_TYPE_I8:
        case LBX_TYPE_I16:
            break;
        case LBX_TYPE_I32:
            switch (LBX_TYPE_DIMENSION(type)) {
                case 1: return Uniform(uniform_loc, (i32_t*)data, count);
                case 2: return Uniform(uniform_loc, (vec2_i32*)data, count);
                case 3: return Uniform(uniform_loc, (vec3_i32*)data, count);
                case 4: return Uniform(uniform_loc, (vec4_i32*)data, count);
                default: break;
            }
            break;
        case LBX_TYPE_I64:
            break;
        default:
            break;
    }
    return 0;
}
#ifdef _DEBUG
GLint TGLProgram::GetAttribLocation(const char *name)
{
    GLint r = glGetAttribLocation(GetHandle(), name);
    if (r == -1) {
        Err_("Cannot find attribute named \"%s\"", name);
    }
    return r;
}
#endif //#ifdef _DEBUG

/////////////////////////////////////////////////////////////////////////////
TGLBufferBase::TGLBufferBase()
    : TGLObject(), local_data(NULL)
{

}
//---------------------------------------------------------------------------
TGLBufferBase::TGLBufferBase(GLenum target)
    : TGLObject(), local_data(NULL)
{

}
//---------------------------------------------------------------------------
TGLBufferBase::~TGLBufferBase()
{
    if (handle) {
        GL_CHECK(glDeleteBuffers(1, &handle));
    }
}
//---------------------------------------------------------------------------
void TGLBufferBase::Bind(GLenum target)
{
    GL_CHECK(glBindBuffer(target, GetHandle()));
}
//---------------------------------------------------------------------------
i32_t TGLBufferBase::GetIntParam(GLenum target, GLenum pname)
{
    GLint v = 0;
    glBindBuffer(target, GetHandle());
    GL_CHECK(glGetBufferParameteriv(target, pname, &v));
    return v;
}
//---------------------------------------------------------------------------
GLuint TGLBufferBase::GetHandle(void)
{
    if (handle == 0) {
        GL_CHECK(glGenBuffers(1, &handle));
    }
    return handle;
}
//---------------------------------------------------------------------------
i32_t TGLBufferBase::Upload(GLenum target, GLenum usage)
{
    Bind(target);
    GL_CHECK(glBufferData(target, svec_size(local_data), local_data, usage));
    return 1;
}
//---------------------------------------------------------------------------
i32_t TGLBufferBase::Upload(GLenum target, const void *data, i32_t size, GLenum usage)
{
    Bind(target);
    GL_CHECK(glBufferData(target, size, data, usage));
    return 1;
}
//---------------------------------------------------------------------------
void * TGLBufferBase::AppendLocalData(i32_t count, i32_t elem_size)
{
    return svec_append(&local_data, count, elem_size);
}
//---------------------------------------------------------------------------
void * TGLBufferBase::InsertLocalData(i32_t index, i32_t count, i32_t elem_size)
{
    return svec_insert(&local_data, index, count, elem_size);
}
//---------------------------------------------------------------------------



/*
LBX_ATTRIB_BIND_INFO::LBX_ATTRIB_BIND_INFO(LBX_TYPE type, i32_t a_stride)
{
    SetType(type);
    stride = (a_stride == 0) ? LBX_TYPE_Size_(type) : a_stride;
}
//---------------------------------------------------------------------------
void LBX_ATTRIB_BIND_INFO::void SetOffset(GLuint a_buffer_id, i32_t offset)
{
    buffer_id = a_buffer_id;
    pointer = (void*)(uintptr_t)offset;
}
//---------------------------------------------------------------------------
void LBX_ATTRIB_BIND_INFO::SetPointer(void *address, i32_t offset)
{
    pointer = address;
}
//---------------------------------------------------------------------------
i32_t LBX_ATTRIB_BIND_INFO::SetType(LBX_TYPE type)
{
    components = LBX_TYPE_Dimension_(type);
    normalized = false;

    switch (LBX_TYPE_ElemType_(type)) {
        case LBX_TYPE_F32:
            component_type = GL_FLOAT;
            break;
        case LBX_TYPE_F64:
            component_type = 0; // unsupported
            break;
        case LBX_TYPE_U8:
            component_type = GL_UNSIGNED_BYTE;
            normalized = true;
            break;
        case LBX_TYPE_U16:
            component_type = GL_UNSIGNED_SHORT;
            normalized = true;
            break;
        case LBX_TYPE_U32:
            component_type = 0; // unsupported
            break;
        case LBX_TYPE_U64:
            component_type = 0; // unsupported
            break;
        case LBX_TYPE_I8:
            component_type = GL_BYTE;
            normalized = true;
            break;
        case LBX_TYPE_I16:
            component_type = GL_SHORT;
            normalized = true;
            break;
        case LBX_TYPE_I32:
            component_type = GL_FIXED;
            break;
        case LBX_TYPE_I64:
            component_type = 0; // unsupported
            break;
        default:
            break;
    }
    return component_type;
}
*/

TGLIndexBuffer::TGLIndexBuffer()
    : TGLBufferBase(GL_ELEMENT_ARRAY_BUFFER)
{

}
TGLIndexBuffer::~TGLIndexBuffer()
{

}
i32_t TGLIndexBuffer::Size(void)
{
    return GetIntParam(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE);
}




TGLAttribBuffer::TGLAttribBuffer()
    : TGLBufferBase(GL_ARRAY_BUFFER), bd(NULL)
{

}
TGLAttribBuffer::TGLAttribBuffer(void *data, i32_t size, GLenum usage)
    : TGLBufferBase(GL_ARRAY_BUFFER), bd(NULL)
{
    Upload(data, size, usage);
}

TGLAttribBuffer::~TGLAttribBuffer()
{
    SVEC_FREE(&bd);
}

bool TGLAttribBuffer::Register(const char* struct_id)
{
    i32_t i, len = (i32_t)strlen(struct_id);
    u8_t attrib_type = LBX_POSITION;
    LBX_TYPE data_type = LBX_TYPE_VEC4_F32;
    i32_t size = 0;
    u16_t offset = 0;

    SVEC_FREE(&bd);
    
    const char* s = struct_id;
    for (i = 0; i < len; ++i) {
        switch (s[i]) {
        case 'V':
        case 'v':
            attrib_type = LBX_POSITION;
            data_type = LBX_TYPE_VEC4_F32;
            size = (i32_t)sizeof(vec4_f32);
            if (isdigit(s[i + 1])) {
                switch (s[i + 1]) {
                case '2': 
                    data_type = LBX_TYPE_VEC2_F32;
                    size = (i32_t)sizeof(vec2_f32);
                    break;
                case '3':
                    data_type = LBX_TYPE_VEC3_F32;
                    size = (i32_t)sizeof(vec3_f32);
                    break;
                case '4':
                    break;
                default:
                    size = -1;
                    break;
                }
            }
            break;
        }
        if (size == -1) {
            return false;
        } else {
            LBX_BUFFER_DESCRIPTOR* p = SVEC_APPEND(LBX_BUFFER_DESCRIPTOR, &bd, 1);
            p->attrib_type = attrib_type;
            p->data_type = data_type;
            p->offset = offset;

            offset += size;


        }
    }
    return true;;
}

TGLAttribBuffer & TGLAttribBuffer::Register(char attrib_type, LBX_TYPE data_type, u16_t offset)
{
    LBX_BUFFER_DESCRIPTOR *p = SVEC_APPEND(LBX_BUFFER_DESCRIPTOR, &bd, 1);
    p->attrib_type = attrib_type;
    p->data_type = data_type;
    p->offset = offset;
    return *this;
}

i32_t TGLAttribBuffer::Register(const LBX_REFL_STRUCT_INFO *rtti)
{
    SVEC_FREE(&bd);
    LBX_REFL_MEMBER_ITERATOR it = refl_get_member_iterator(rtti);
    const LBX_REFL_MEMBER_INFO *mi;
    i32_t r = 0;
    while ((mi = refl_next_member(&it)) != NULL) {
        u8_t attrib_type = EstimateAttribTypeFromName(mi->id);
        if (attrib_type) {
            Register(attrib_type, mi->type, mi->offset);
            r++;
        }
    }
    return r;
}


i32_t TGLAttribBuffer::BindTo(TGLProgram *program, i32_t elem_size)
{
    i32_t i, i_end, r = 0;
    i_end = svec_len32(bd);
    if (i_end == 0) {
        Err_("No Buffer Descriptor was registered to the TGLAttribBuffer instatnce");
        return r;
    }
    program->Use();
    Bind();
    for (i = 0; i < i_end; i++) {
        LBX_BUFFER_DESCRIPTOR d = bd[i];
        GLint loc = program->LocationOfAttribType(d.attrib_type);
        if (loc == -1) {
            continue;
        }
        LBX_GL_TYPE glt = GetGLTypeInfo(d.data_type);
        if (glt.components == 0) {
            continue;
        }
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, GetHandle()));
        program->EnableArrayMode(loc);

/*
            glVertexAttribPointer(si->attrib_loc, si->components,
            si->component_type, si->normalize, bi[i]->stride,
            bi[i]->data + si->offset);
*/
        GL_CHECK(glVertexAttribPointer(loc, glt.components, glt.component_type, glt.normalize, elem_size, (void*)(uintptr_t)d.offset));
        r++;
    }
    return r;
}


LBX_GL_TYPE GetGLTypeInfo(LBX_TYPE type)
{
    LBX_GL_TYPE r = {0,};

    switch (LBX_TYPE_ELEMTYPE(type)) {
        case LBX_TYPE_F32:
            r.component_type = GL_FLOAT;
            break;
        case LBX_TYPE_F64:
            Err_("Unsupported type");
            return r;
        case LBX_TYPE_U8:
            r.component_type = GL_UNSIGNED_BYTE;
            r.normalize = true;
            break;
        case LBX_TYPE_U16:
            r.component_type = GL_UNSIGNED_SHORT;
            r.normalize = true;
            break;
        case LBX_TYPE_U32:
        case LBX_TYPE_U64:
            Err_("Unsupported type");
            return r;
        case LBX_TYPE_I8:
            r.component_type = GL_BYTE;
            r.normalize = true;
            break;
        case LBX_TYPE_I16:
            r.component_type = GL_SHORT;
            r.normalize = true;
            break;
        case LBX_TYPE_I32:
            r.component_type = GL_FIXED;
            break;
        case LBX_TYPE_I64:
            Err_("Unsupported type");
            return r;
        default:
            break;
    }

    r.components = LBX_TYPE_DIMENSION(type);
    return r;
}



//LBX_ATTRIB_BIND_INFO *bi;

TGLAttribBinder::TGLAttribBinder()
    : p(NULL), bi(NULL)
{

}
//---------------------------------------------------------------------------
TGLAttribBinder::TGLAttribBinder(TGLProgram *program, TGLAttribBuffer *buffer)
    : p(program), bi(NULL)
{
}
//---------------------------------------------------------------------------
TGLAttribBinder::TGLAttribBinder(TGLProgram *program, const LBX_REFL_STRUCT_INFO *typeinfo)
    : p(program), bi(NULL)
{

}
//---------------------------------------------------------------------------
TGLAttribBinder::~TGLAttribBinder()
{
    Clear();
}
//---------------------------------------------------------------------------
void TGLAttribBinder::SetProgram(TGLProgram * program)
{
    Clear();
    p = program;
}
//---------------------------------------------------------------------------
LBX_BUFFER_INFO * TGLAttribBinder::add_buffer(void)
{
    LBX_BUFFER_INFO ** r = SVEC_APPEND(LBX_BUFFER_INFO *, &bi, 1);
    *r = (LBX_BUFFER_INFO *)svec_alloc(sizeof(LBX_BUFFER_INFO), 1);
    return *r;
}
//---------------------------------------------------------------------------
LBX_ATTRIB_BINDING * TGLAttribBinder::AddBinding(LBX_BUFFER_INFO *buffer_info)
{
    LBX_BUFFER_INFO ** pbi = bi;
    i32_t l = svec_len32(bi);
    if (l == 0) {
        return NULL;
    }
    pbi = (buffer_info == NULL) ? bi + (l - 1) : find_buffer(buffer_info);
    if (pbi) {
        return (LBX_ATTRIB_BINDING *)svec_append((void**)pbi, sizeof(LBX_ATTRIB_BINDING), 1);
    } else {
        return NULL;
    }
}
//---------------------------------------------------------------------------
LBX_BUFFER_INFO * TGLAttribBinder::SetBuffer(GLint buffer_id, i32_t stride)
{
    LBX_BUFFER_INFO *r = add_buffer();
    r->buffer_id = buffer_id;
    r->stride = stride;
    r->data = NULL;
    return r;
}
//---------------------------------------------------------------------------
LBX_BUFFER_INFO * TGLAttribBinder::SetBuffer(void * buffer_addr, i32_t stride)
{
    LBX_BUFFER_INFO *r = add_buffer();
    r->buffer_id = 0;
    r->stride = stride;
    r->data = (u8_t*)buffer_addr;
    return r;
}
//---------------------------------------------------------------------------
LBX_BUFFER_INFO ** TGLAttribBinder::find_buffer(LBX_BUFFER_INFO* to_find)
{
    LBX_BUFFER_INFO ** pbi = bi;
    i32_t l = svec_len32(bi);
    for (i32_t i = 0; i < l; i++, pbi++) {
        if (*pbi == to_find) {
            return pbi;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
LBX_ATTRIB_BINDING * TGLAttribBinder::AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLint offset, LBX_BUFFER_INFO * buffer_info)
{
    LBX_ATTRIB_BINDING * r = AddBinding(buffer_info);
    if (r) {
        r->attrib_loc = attrib_loc;
        r->components = comp_count;
        r->component_type = comp_type;
        r->offset = offset;
    }
    return r;
}
//---------------------------------------------------------------------------
LBX_ATTRIB_BINDING * TGLAttribBinder::AddBinding(GLint attrib_loc, LBX_TYPE type, GLint offset, LBX_BUFFER_INFO * buffer_info)
{
    LBX_ATTRIB_BINDING * r = NULL;
    GLenum comp_type = 0;
    GLboolean normalize = false;

    if (attrib_loc < 0) {
        Err_("Invalid attrib loc");
        return NULL;
    }

    switch (LBX_TYPE_ELEMTYPE(type)) {
        case LBX_TYPE_F32:
            comp_type = GL_FLOAT;
            break;
        case LBX_TYPE_F64:
            return NULL; // unsupported
        case LBX_TYPE_U8:
            comp_type = GL_UNSIGNED_BYTE;
            normalize = true;
            break;
        case LBX_TYPE_U16:
            comp_type = GL_UNSIGNED_SHORT;
            normalize = true;
            break;
        case LBX_TYPE_U32:
        case LBX_TYPE_U64:
            return NULL; // unsupported
        case LBX_TYPE_I8:
            comp_type = GL_BYTE;
            normalize = true;
            break;
        case LBX_TYPE_I16:
            comp_type = GL_SHORT;
            normalize = true;
            break;
        case LBX_TYPE_I32:
            comp_type = GL_FIXED;
            break;
        case LBX_TYPE_I64:
            return NULL; // unsupported
        default:
            break;
    }

    r = AddBinding(buffer_info);
    if (r) {
        r->attrib_loc = attrib_loc;
        r->components = LBX_TYPE_DIMENSION(type);
        r->component_type = comp_type;
        r->normalize = normalize;
        r->offset = offset;
    }
    return r;
}

i32_t TGLAttribBinder::AddBinding(GLint attrib_loc, const LBX_REFL_STRUCT_INFO *ti, const char *member_name, GLboolean normalize)
{

    return 1;
}
//---------------------------------------------------------------------------
i32_t TGLAttribBinder::AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLboolean normalize, GLint stride, void * pointer)
{

    return 1;
}
//---------------------------------------------------------------------------
i32_t TGLAttribBinder::AddUniform(GLint uniform_loc, LBX_TYPE type, void *data)
{
    return 1;
}
//---------------------------------------------------------------------------
void TGLAttribBinder::Clear(void)
{
    i32_t i, i_end;
    i_end = svec_len32(bi);
    for (i = 0; i < i_end; i++) {
        svec_free((void**)(bi + i));
    }
    svec_free((void**)&bi);
}
//---------------------------------------------------------------------------

i32_t TGLAttribBinder::Enable(void)
{
    i32_t i, i_end, j, j_end;
    if (p == NULL) {
        return 0;
    }
    p->Use();
    i_end = svec_len32(bi);
    for (i = 0; i < i_end; i++) {
        j_end = GetBufferSubCount(bi[i]);
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, bi[i]->buffer_id));
        LBX_ATTRIB_BINDING * si = GetBufferSub(bi[i]);
        for (j = 0; j < j_end; j++, si++) {
            GL_CHECK(glEnableVertexAttribArray(si->attrib_loc));
            GL_CHECK(glVertexAttribPointer(si->attrib_loc, si->components,
                si->component_type, si->normalize, bi[i]->stride,
                bi[i]->data + si->offset));
        }
    }
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    return 1;
}
//---------------------------------------------------------------------------
i32_t TGLAttribBinder::Disable(void)
{
    i32_t i, i_end, j, j_end;
    i_end = svec_len32(bi);
    if (p == NULL) {
        return 0;
    }
    p->Use();
    for (i = 0; i < i_end; i++) {
        j_end = GetBufferSubCount(bi[i]);
        LBX_ATTRIB_BINDING * si = GetBufferSub(bi[i]);
        for (j = 0; j < j_end; j++, si++) {
            glDisableVertexAttribArray(si->attrib_loc);
        }
    }
    return 1;
}
//---------------------------------------------------------------------------




TGLCommandList::TGLCommandList()
    : list(NULL), ab(NULL)
{
}
TGLCommandList::~TGLCommandList()
{
    Clear();
}
void TGLCommandList::Clear(void)
{
    i32_t i, i_end = Count();
    for (i = 0; i < i_end; i++) {
        SVEC_FREE(&(list[i].indice));
    }
    SVEC_FREE(&list);
}

LBX_RENDER_COMMAND * TGLCommandList::Add(GLenum method, bool *visible)
{
    LBX_RENDER_COMMAND * r = SVEC_APPEND(LBX_RENDER_COMMAND, &list, 1);
    memset(r, 0, sizeof(LBX_RENDER_COMMAND));
    r->command = method;
    r->visible = visible;
    return r;
}
LBX_RENDER_COMMAND * TGLCommandList::AddArrays(GLenum method, i32_t first, i32_t count, bool *visible)
{
    LBX_RENDER_COMMAND * r = Add(method, visible);
    r->first = first;
    r->count = count;
    return r;
}
LBX_RENDER_COMMAND * TGLCommandList::Add(GLenum method, LBX_TYPE type, i32_t count, bool *visible)
{
    GLenum gltype = 0;
    switch (type) {
        case LBX_TYPE_U16:
            gltype = GL_UNSIGNED_SHORT;
            break;
        case LBX_TYPE_U8:
            gltype = GL_UNSIGNED_BYTE;
            break;
        default:
            return NULL;
    }
    LBX_RENDER_COMMAND * r = Add(method, visible);
    r->type = gltype;
    r->count = count;
    svec_set_length((void**)(&(r->indice)), count, (u16_t)LBX_TYPE_ELEM_SIZE(type));
    return r;
}
LBX_RENDER_COMMAND * TGLCommandList::Add(GLenum method, GLuint indice_buffer_id, GLenum elem_type,  i32_t count, bool *visible)
{
    LBX_RENDER_COMMAND * r = Add(method, visible);
    r->buffer_id = indice_buffer_id;
    r->type = elem_type;
    r->count = count;
    return r;
}
LBX_RENDER_COMMAND * TGLCommandList::Add(GLenum method, const u16_t *indice, i32_t count, bool *visible)
{
    LBX_RENDER_COMMAND * r = Add(method, visible);
    svec_set_length(&(r->indice), count, sizeof(u16_t));
    if (indice) {
        copy_memory(r->indice, indice, sizeof(u16_t) * count);
    }
    r->type = GL_UNSIGNED_SHORT;
    r->count = count;
    return r;
}
LBX_RENDER_COMMAND * TGLCommandList::Add(GLenum method, const u8_t *indice, i32_t count, bool *visible)
{
    LBX_RENDER_COMMAND * r = Add(method, visible);
    svec_set_length(&(r->indice), count, sizeof(u8_t));
    if (indice) {
        copy_memory(r->indice, indice, sizeof(i32_t) * count);
    }
    r->type = GL_UNSIGNED_BYTE;
    r->count = count;
    return r;
}
TGLProgram * TGLCommandList::GetProgram(void)
{
    if (ab) {
        return ab->GetProgram();
    }
    return NULL;
}




i32_t DrawCommandList(TGLCommandList *list)
{
    i32_t i_end = list->Count();
    if (list->ab) {
        list->ab->Enable();
    }
    for (i32_t i = 0; i < i_end; i++) {
        LBX_RENDER_COMMAND *c = list->Items(i);
        if (c->visible && *(c->visible) != true) {
            continue;
        }
        GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, c->buffer_id));
        if (c->buffer_id == 0 && c->indice == NULL) {
            GL_CHECK(glDrawArrays(c->command, c->first, c->count));
        } else {
            GL_CHECK(glDrawElements(c->command, (c->indice) ? (GLsizei)svec_length(c->indice) : (GLsizei)c->count, c->type, c->indice));
        }
    }
    if (list->ab) {
        list->ab->Disable();
    }
    return 1;
}



//---------------------------------------------------------------------------
TGLDrawList::TGLDrawList(TGLProgram *program)
{
    ab.SetProgram(program);
    cl.ab = &ab;
    ab.SetBuffer(bf.GetHandle(), sizeof(V2CT));
    modified = true;
}
//---------------------------------------------------------------------------
TGLDrawList::~TGLDrawList()
{

}
//---------------------------------------------------------------------------
LBX_ATTRIB_BINDING * TGLDrawList::SetBinding(const char *attrib_name, LBX_TYPE type, GLint offset)
{
    modified = true;
    return ab.AddBinding(attrib_name, type, offset);
}
//---------------------------------------------------------------------------
void TGLDrawList::ClearCommandList(void) {
    bf.ClearLocalData();
    cl.Clear();
    modified = true;
}
//---------------------------------------------------------------------------
void TGLDrawList::FillBuffer(V2CT *dst, TGlyph *glyph, rect_f32 area, u32_t color)
{
    i32_t cnt = glyph->FillVertexList(&(dst->vtx), area, sizeof(V2CT));
    // TexCoord와 Color를 채우고
    for (i32_t i = 0; i < cnt; i++) {
        dst[i].txc = glyph->GetTexCoords()[i];
        dst[i].col = color;
    }
/*
    u16_t *idx = new u16_t[icnt];
    for (i32_t i = 0; i < icnt; i++) {
        idx[i] = base + indice[i];
    }
*/    modified = true;
}
//---------------------------------------------------------------------------
i32_t TGLDrawList::AddGlyph(TGlyph *glyph, rect_f32 area, u32_t color)
{
    i32_t cnt, icnt, base;
    const u8_t *indice = glyph->GetDrawIndice(&icnt);
    base = svec_len32(bf.GetLocalData());
    cnt = glyph->GetVertexCount();
    V2CT *v = (V2CT*)bf.AppendLocalData(cnt, sizeof(V2CT)); // 버퍼 크기를 늘리고
    // Vertex정보를 채우고
    cnt = glyph->FillVertexList(&(v->vtx), area, sizeof(V2CT));
    // TexCoord와 Color를 채우고
    for (i32_t i = 0; i < cnt; i++) {
        v[i].txc = glyph->GetTexCoords()[i];
        v[i].col = color;
    }
    u16_t *idx = new u16_t[icnt];
    for (i32_t i = 0; i < icnt; i++) {
        idx[i] = base + indice[i];
    }
        cl.Add(GL_TRIANGLES, idx, icnt);
    modified = true;
    delete [] idx;
    return 1;
}
//---------------------------------------------------------------------------
i32_t TGLDrawList::AddLines(vec2_f32 *lines_vertice, i32_t line_count, u32_t color)
{
    i32_t vtx_count = line_count * 2;
    i32_t base = svec_len32(bf.GetLocalData());
    V2CT *v = (V2CT*)bf.AppendLocalData(vtx_count, sizeof(V2CT)); // 버퍼 크기를 늘리고
    vec2_f32 txc = vec2_f32_(1.0f / 1024.0f, 1.0f / 1024.0f);
    for (i32_t i = 0; i < vtx_count; i++) {
        v[i].vtx = lines_vertice[i];
        v[i].col = color;
        v[i].txc = txc;
    }
    cl.AddArrays(GL_LINES, base, vtx_count);
/*
    u16_t *idx = new u16_t[vtx_count];
    for (i32_t i = 0; i < vtx_count; i++) {
        idx[i] = base + i;
    }
    cl.Add(GL_LINES, idx, vtx_count);
    modified = true;
    delete [] idx;
*/
    return 1;
}

//---------------------------------------------------------------------------
i32_t TGLDrawList::Draw(void)
{
    if (modified) {
        bf.Upload();
        modified = false;
    }
    DrawCommandList(&cl);
    return 1;
}





TGLFrameBufferObject::TGLFrameBufferObject()
    : inherited(), flags(0), tex(NULL), depth(0), samples(1)
{
    sz = size2_i16_(0,0);
}
TGLFrameBufferObject::TGLFrameBufferObject(i32_t width, i32_t height)
    : inherited(), flags(0), tex(NULL), depth(0)
{
    sz = size2_i16_(width, height);
//    SetSize(width, height);
}

TGLFrameBufferObject::~TGLFrameBufferObject()
{
    if (handle) {
        GL_CHECK(glDeleteFramebuffers(1, &handle));
        handle = 0;
    }
    if (depth) {
        GL_CHECK(glDeleteRenderbuffers(1, &depth));
    }
    delete tex;
}
/*
GLenum TGLFrameBufferObject::SetSize(i32_t width, i32_t height)
{
    return Update(size2_i16_(width, height));
    GLint n_fbo;
    bool modified = false;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &n_fbo);

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, GetHandle()));
    if (width != sz.width || height != sz.height) {
        sz = size2_i16_(width, height);
        // 먼저 detach 하고 
        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0));
        // 새로운 storage 할당:
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, depth));
        GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, LBX_GL_DEPTH_COMPONENT, width, height));
        // 다시 연결
        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth));
    }
#if 1
    tex->Bind();
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
    if (samples > 1) {
        static PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress("glFramebufferTexture2DMultisampleEXT");
        if (glFramebufferTexture2DMultisampleEXT) {
            GL_CHECK(glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                            GL_TEXTURE_2D, tex->GetHandle(), 0, 4));
            GL_CHECK(glBlitFramebufferEXT())
        } else {
            Err_("Cannot find glFramebufferTexture2DMultisampleEXT function, multi-sampling disabled");
            samples = 1;
        }
    }
#else
    GL_CHECK(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex->GetHandle()));
    static PFNGLTEXSTORAGE2DMULTISAMPLEPROC glTexImage2DMultisample = 
        (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)eglGetProcAddress("glTexImage2DMultisample");
    if (glTexImage2DMultisample) {
        GL_CHECK(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, width, height, GL_TRUE));
    } else {
        Err_("Cannot find glTexImage2DMultisample function, multi-sampling disabled");
        samples = 1;
    }
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex->GetHandle(), 0)); // 마지막은 mipmap 레벨임
#endif
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (n_fbo != handle) {
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, n_fbo));
    }
    return (i32_t)status;
}
*/
void TGLFrameBufferObject::Bind(void)
{
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, GetHandle()));
}
void TGLFrameBufferObject::Release(void)
{
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}
GLenum TGLFrameBufferObject::Update(size2_i16 new_size, i32_t new_samples)
{
    GLint n_fbo;
#if GLES > 20
    GLenum status = GL_FRAMEBUFFER_UNDEFINED;
#else
    GLenum status = GL_FRAMEBUFFER_UNSUPPORTED;
#endif
    bool size_changed = false, sample_changed = false;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &n_fbo);

    if (!handle) {
        GL_CHECK(glGenFramebuffers(1, &handle)); // Frame buffer 할당
        GL_CHECK(glGenRenderbuffers(1, &depth)); // Render buffer 할당
        sz = new_size;
        size_changed = (new_size.width * new_size.height != 0);
        samples = new_samples;
        sample_changed = true;
    } else {
        // 렌더버퍼가 이미 연결돼 있을 수 있으니 먼저 현 렌더버퍼를 분리
        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0));
        // 사이즈가 같으면 아무것도 하지 않는다
        if (sz.width != new_size.width || sz.height != new_size.height) {
            size_changed = true;
            sz = new_size;
        }
        if (samples != new_samples) {
            sample_changed = true;
            samples = new_samples;
        }
    }
    if (!handle || !depth) {
        // 프레임버퍼나 렌더버퍼 생성 실패이므로 바로 오류 처리
        return -1;
    }

    // 현 Framebuffer를 바인딩하고
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, handle));

    if (size_changed || sample_changed) {
        // 렌더버퍼(깊이버퍼)를 바인딩하고
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, depth));
#if GLES > 20
        if (samples > 1) {
            // 렌더버퍼(깊이버퍼)의 용량을 재조정 하고
            GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, LBX_GL_DEPTH_COMPONENT, sz.width, sz.height));
        } else {
            // 렌더버퍼(깊이버퍼)의 용량을 재조정 하고
#endif
            GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, LBX_GL_DEPTH_COMPONENT, sz.width, sz.height));
#if GLES > 20
        }
#endif
        // 다시 연결해준다.
        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth));

        // 텍스쳐 객체가 없는 경우엔 새로 생성
        if (tex == NULL) {
            tex = new TGLTexture2D();
            if (tex == NULL) {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
            Info_("Texture Created: %d", tex->GetHandle());
        }
        tex->Bind();
#if GLES > 20
        if (samples == 1) {
#endif
            // 텍스쳐를 바인딩하고
            // 파라미터 설정
            GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            // 텍스쳐 크기 변경
            GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sz.width, sz.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
            GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->GetHandle(), 0)); // 마지막은 mipmap 레벨임
#if GLES > 20
        } else {
            GL_CHECK(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex->GetHandle()));
            // 파라미터 설정
            //GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            //GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

            static PFNGLTEXSTORAGE2DMULTISAMPLEPROC glTexStorage2DMultisample =
                (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)eglGetProcAddress("glTexStorage2DMultisample");
            GL_CHECK(glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, sz.width, sz.height, GL_TRUE));
            GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex->GetHandle(), 0));
        }
#endif
    }
    // 프레임버퍼 상태 점검
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    // Framebuffer 바인딩을 원복함
    if (n_fbo != (GLint)handle) {
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, n_fbo));
    }
    return status;
}

GLuint TGLFrameBufferObject::GetHandle(void)
{
    if (!handle) {
        Update(sz, samples);
    }
    return handle;
}

