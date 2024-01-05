//---------------------------------------------------------------------------

#ifdef __BORLANDC__
#pragma hdrstop
#endif //#ifdef __BORLANDC__

#include "lbx_gles2.h"
#include "GLES2/gl2ext.h"
#include "system/lbx_log.h"
#include "system/lbx_stream_file.h"
//#include "image/lbxIMAGE.h"
//---------------------------------------------------------------------------
#ifdef __BORLANDC__
#pragma package(smart_init)
#endif //#ifdef __BORLANDC__

UString GetGLStringData(GLuint obj, int length, TGLGetStringFunc func)
{
    UString r;
    if (length > 1) {
        r.SetLength(length - 1);
        func(obj,  length,  NULL,  r.c_str());
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
    : inherited()
{

}
//---------------------------------------------------------------------------
TLBTexture::~TLBTexture()
{
}
//---------------------------------------------------------------------------
int TLBTexture::LoadFromFile(const char *file_name, int target)
{
    int ret = 0;
    lbxSTREAM st = {0,};
    stream_open_file(&st, file_name, "rb");
    ret = LoadFromStream(&st, target);
    stream_close(&st);
    return ret;
}
//---------------------------------------------------------------------------
int TLBTexture::LoadFromStream(lbxSTREAM *s, int target)
{
    int ret = 0;
/*
    if (lbxIMAGE_LoadFromStream(&image_format, s) > 0) {
        ret = SetImage(&image_format, (int)target);
        lbxIMAGE_Free(&image_format);
    }
    lbxIMAGE_DataToOffset(&image_format);
*/	return ret;
}
//---------------------------------------------------------------------------
int TLBTexture::Load(lbxIMAGE *img)
{
    return 0;
}
//---------------------------------------------------------------------------
GLuint TLBTexture::GetHandle(void)
{
    if (handle == 0) {
        GL_ASSERT(glGenTextures(1, &handle));
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
int TGLTexture3D::SetImage(const lbxIMAGE *img, int target)
{
    size2_i16 sz = img->planes[0].size;
    GLenum fmt;

    fmt = get_gl_format(img->pixel_format);
    if (fmt) {
        Bind();
        GL_ASSERT(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + target, 0, fmt, sz.width, sz.height, 0, fmt, GL_UNSIGNED_BYTE, (void*)(img->planes[0].data)));

        GL_ASSERT(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL_ASSERT(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL_ASSERT(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_ASSERT(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
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
    memset(&image_format, 0, sizeof(lbxIMAGE));
}
//---------------------------------------------------------------------------
TGLTexture2D::~TGLTexture2D()
{
    if (handle) {
        glDeleteTextures(1, &handle);
    }
}
//---------------------------------------------------------------------------
int TGLTexture2D::SetImage(const lbxIMAGE *img, int target)
{
    fourcc_t pf = img->pixel_format;
    size2_i16 sz = img->planes[0].size;
    GLenum fmt;

    GetHandle();
    if (handle == 0) {
        GL_ASSERT(glGenTextures(1, &handle));
    }
    if (target == -1) {
        target = GL_TEXTURE_2D;
    }

    fmt = get_gl_format(img->pixel_format);
    if (fmt) {
        GLenum t = (target == GL_TEXTURE_2D) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
        GL_ASSERT(glBindTexture(t, GetHandle()));
        GL_ASSERT(glTexImage2D(target, 0, fmt, sz.width, sz.height, 0, fmt, GL_UNSIGNED_BYTE, (void*)(img->planes[0].data)));

        if (t == GL_TEXTURE_2D) {
        #if 1 // Mipmap 사용
            GL_ASSERT(glTexParameteri(t, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL_ASSERT(glTexParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
            GL_ASSERT(glGenerateMipmap(t));
        #else
            GL_ASSERT(glTexParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL_ASSERT(glTexParameteri(t, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        #endif
        } else {
            GL_ASSERT(glTexParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL_ASSERT(glTexParameteri(t, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL_ASSERT(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL_ASSERT(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        }
        return 1;
    } else {
        return 0;
    }


}


static inline void xywh_to_rect(rect_f32 *dst, int x, int y, int width, int height, size2_i16 res)
{
    vec2_f32 sc = vec2_f32_(1.0f / (float)res.width, 1.0f / (float)res.height);
    dst->left = (float)x * sc.x;
    dst->top = 1.0f - ((float)y * sc.y);
    dst->right = (float)(x + width) * sc.x;
    dst->bottom = 1.0f - ((float)(y + height) * sc.y);
}

static inline void rect_normalize(rect_f32 *out, rect_i16 in, size2_i16 res)
{
    vec2_f32 sc  = vec2_f32_(1.0f / (float)res.width, 1.0f / (float)res.height);
    out->left   = (float)in.left   * sc.x;
    out->top    = (float)in.top    * sc.y;
    out->right  = (float)in.right  * sc.x;
    out->bottom = (float)in.bottom * sc.y;
}

uint8_t idx_rect_triangles[] = {0,2,1,2,3,1};
uint8_t idx_rect_triangle_strip[] = {0,2,1,3};
uint8_t idx_border_triangles[] = {0,4,1,1,4,5,1,5,2,2,5,6,2,6,3,3,6,7,4,8,5,5,8,9,6,10,7,7,10,11,8,12,9,9,12,13,9,13,10,10,13,14,10,14,11,11,14,15,5,9,6,6,9,10};
uint8_t idx_border_triangle_strip[] = {0,4,1,5,2,6,3,7,7,7,6,11,10,15,14,14,14,10,13,9,12,8,8,8,9,4,5,5,5,9,6,10};


/////////////////////////////////////////////////////////////////////////////
TGlyph::TGlyph(TGLTexture2D *tex)
    : fit(NULL), border(NULL), tex_coord(NULL)
{
    texture = tex;
    memset(&boundary, 0, sizeof(boundary));
    scale = vec2_f32_(1.0, 1.0);
}
//---------------------------------------------------------------------------
TGlyph::~TGlyph()
{
    ClearBase();
    ClearBorder();
    RCM_FREE(&tex_coord);
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
void TGlyph::SetBoundary(rect_i16 rect)
{
    size2_i16 res = texture->image_format.planes[0].size;
    scr_boundary = rect;
    rect_normalize(&boundary, rect, texture->image_format.planes[0].size);
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
        RCM_SET_LENGTH(vec2_f32, &tex_coord, 16);
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
        RCM_SET_LENGTH(vec2_f32, &tex_coord, 4);
        tex_coord[0] = vec2_f32_(boundary.left, boundary.top);
        tex_coord[1] = vec2_f32_(boundary.right, boundary.top);
        tex_coord[2] = vec2_f32_(boundary.left, boundary.bottom);
        tex_coord[3] = vec2_f32_(boundary.right, boundary.bottom);
    }
}
//---------------------------------------------------------------------------
int TGlyph::GetVertexCount(void)
{
    return (border ? 16 : 4);
}
//---------------------------------------------------------------------------
#define PTR_OFFSET(ptr, offset) ((uint8_t*)ptr + offset)
int TGlyph::FillVertexList(vec2_f32 *target, rect_f32 area, int target_stride)
{
    size2_i16 res = texture->image_format.planes[0].size;

    Update();
    if (target_stride == 0) {
        target_stride = sizeof(*target);
    }
    vec2_f32 *p[16];
    int vtx_count = (border) ? 16 : 4;
    for (int i = 0; i < vtx_count; i++) {
        p[i] = (vec2_f32*)((uint8_t*)target + target_stride * i);
    }

    // 좌상, 우하 좌표부터 먼저 계산함
    if (fit) {
        vec2_f32 sc = vec2_f32_(
                RECT_WIDTH(area) / (RECT_WIDTH(boundary) + RECT_HEIGHT(*fit)),
                RECT_HEIGHT(area) / (RECT_HEIGHT(boundary) + RECT_HEIGHT(*fit))
            );
        *p[0] = vec2_f32_(area.left - fit->left * sc.x, area.top - fit->top * sc.y);
        *p[vtx_count - 1] = vec2_f32_(area.right - fit->right * sc.x, area.bottom - fit->bottom * sc.y);
    } else {
        *p[0] = vec2_f32_(area.left, area.top);
        *p[vtx_count - 1] = vec2_f32_(area.right, area.bottom);
    }


    if (border) {
        *p[ 5] = vec2_f32_(p[ 0]->x + border->left * (float)res.width * scale.x, p[ 0]->y + border->top * (float)res.height * scale.y);
        *p[10] = vec2_f32_(p[15]->x + border->right * (float)res.width * scale.x, p[15]->y + border->bottom * (float)res.height * scale.y);

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
const uint8_t *TGlyph::GetDrawIndice(int *count)
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
int TGlyph::BuildDrawList(void *buffer, const LB_GLBUFFER_DESC *bi, int16_t *indice, rect_f32 target)
{
    int stride;
    uint8_t *b = (uint8_t *)buffer;

    if (!(LB_TYPE_ELEMTYPE(bi->vertex.type) == LB_TYPE_F32 &&
        LB_TYPE_ELEMTYPE(bi->tex_coord.type) == LB_TYPE_F32)) {
        Err_("Cannot build drawlist");
        return 0;
    }

    stride = bi->stride;
    if (border == NULL) {
        if (fit == NULL) {
            b = (uint8_t *)buffer + bi->vertex.offset;
            *(vec2_f32*)(b + stride * 0) = vec2_f32_(target.left, target.top);
            *(vec2_f32*)(b + stride * 1) = vec2_f32_(target.right, target.top);
            *(vec2_f32*)(b + stride * 2) = vec2_f32_(target.left, target.bottom);
            *(vec2_f32*)(b + stride * 3) = vec2_f32_(target.right, target.bottom);
            b = (uint8_t *)buffer + bi->tex_coord.offset;
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
    : flags(0), TGLObject()
{
    CreateGLObject(type);
/*
    char test[1024];
    GLsizei a,b;
    GL_ASSERT(glGetShaderSource(shader,  1024,  &a,  test));
*/
}
//---------------------------------------------------------------------------
TGLShader::TGLShader(GLenum type, lbxSTREAM *strm)
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
TGLShader::TGLShader(GLenum type, const char *code, int code_len, const char *hdr, int hdr_len)
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
int TGLShader::GetIntParam(GLenum pname)
{
    GLint info = 0;
    GL_ASSERT(glGetShaderiv(handle, pname, &info));
    return info;
}
//---------------------------------------------------------------------------
int TGLShader::Load(const void *data, int length, GLenum binaryformat)
{
    Log_("glShaderBinary...");
    GL_ASSERT(glShaderBinary(1, (GLuint *)&handle, binaryformat, data, length));
    if (glGetError() == GL_NO_ERROR) {
        Log_("Shader binary loaded successfully.");
        return 1;
    } else {
        Err_("Error loading shader binary: %s", GetInfoLog().c_str());
        return 0;
    }
}
//---------------------------------------------------------------------------
int TGLShader::SetBinary(lbxSTREAM *strm)
{
    int ret;
    uint8_t *b = NULL;
    int l = (int)stream_get_size(strm);
    RCM_SET_LENGTH(uint8_t, &b, l);
    stream_read(strm, b, l);
    Log_("%d bytes loaded", l);


/*
    int format_cnt = 0;
    int formats[10];
    GL_ASSERT(glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &format_cnt));
    Log_("%d formats supported", format_cnt);
    GL_ASSERT(glGetIntegerv(GL_SHADER_BINARY_FORMATS, formats));
    for (int i = 0; i < format_cnt; i++) {
        Log_("%d: %d(GL_MALI_SHADER_BINARY_ARM=%d)", i, formats[i], GL_MALI_SHADER_BINARY_ARM);
    }
*/
    ret = Load(b, l, GL_MALI_SHADER_BINARY_ARM);
    RCM_FREE(&b);
    return ret;

}
//---------------------------------------------------------------------------
int TGLShader::SetSource(GLint count, const char **codes, const GLint *lengths)
{
    if (handle == 0) {
        return 0;
    }
    GLuint shader = GetHandle();
    // Load the shader source
    GL_ASSERT(glShaderSource(shader, count, codes, lengths));
    // Compile the shader
    GL_ASSERT(glCompileShader(shader));

    if (GetCompileStatus()) {
        flags |= (uint32_t)(sfCompiled);
    } else {
        flags &= ~(uint32_t)(sfCompiled);
        Err_("Error compiling shader: %s", GetInfoLog().c_str());
        return 0;
    }
    return 1;
}
//---------------------------------------------------------------------------
int TGLShader::SetSource(const char *code, const char *hdr)
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
int TGLShader::SetSource(const char *code, int code_len, const char *hdr, int hdr_len)
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
int TGLShader::SetSource(lbxSTREAM *strm)
{
    UString code;
    int l = (int)stream_get_size(strm);
    code.SetLength(l);
    stream_read(strm, code.c_str(), l);
    return SetSource(code.c_str(), l);
}
//---------------------------------------------------------------------------
bool TGLShader::IsCompiled(void)
{
    return flags & (uint32_t)(sfCompiled);
}

//---------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
TGLVertexShader::TGLVertexShader()
    : TGLShader(GL_VERTEX_SHADER)
{

}
//---------------------------------------------------------------------------
TGLVertexShader::TGLVertexShader(lbxSTREAM *strm)
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
TGLFragmentShader::TGLFragmentShader(lbxSTREAM *strm)
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
    : TGLObject(), attrib_types(NULL), attrib_data(NULL), flags(0)
{
    shaders[0] = shaders[1] = NULL;

}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(const char *file_name, GLenum bin_format)
    : TGLObject(), attrib_types(NULL), attrib_data(NULL), flags(0)
{
    shaders[0] = shaders[1] = NULL;
    LoadFromFile(file_name, bin_format);
}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(TGLVertexShader *v, TGLFragmentShader * f)
    : TGLObject(), attrib_types(NULL), attrib_data(NULL), flags(0)
{
    shaders[0] = v;
    shaders[1] = f;
}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(const char *vshader, const char *fshader, const char *hdr)
    : TGLObject(), attrib_types(NULL), attrib_data(NULL), flags(0)
{
    Build(vshader, fshader, hdr);
}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(TGLVertexShader *vshader, const char *fshader, const char *hdr)
    : TGLObject(), attrib_types(NULL), attrib_data(NULL), flags(0)
{
    Build(vshader, fshader, hdr);
}
//---------------------------------------------------------------------------
TGLProgram::TGLProgram(const char *vshader, TGLFragmentShader *fshader, const char *hdr)
    : TGLObject(), attrib_types(NULL), attrib_data(NULL), flags(0)
{
    Build(vshader, fshader, hdr);
}
//---------------------------------------------------------------------------
TGLProgram::~TGLProgram()
{
    if (handle) {
        // glDetachShader가 자동으로 호출되는지 확인할 것
        for (int i = 0; i < 2; i++) {
            if (shaders[i]) {
                GL_ASSERT(glDetachShader(handle, shaders[i]->GetHandle()));
            }
            SetShader(i, NULL);
        }
        GL_ASSERT(glDeleteProgram(handle));
    }
    RCM_FREE(&attrib_types);
    for (int i = rcm_length(attrib_data) - 1; i >= 0; i--) {
        free_memory(attrib_data[i]);
    }
    RCM_FREE(&attrib_data);
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
void TGLProgram::SetShader(int index, TGLShader *shader)
{
    uint32_t f = ((uint32_t)(pfOwnsVertexShader) << index);
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
int TGLProgram::GetIntParam(GLenum pname)
{
    GLint info = 0;
    GL_ASSERT(glGetProgramiv(GetHandle(), pname, &info));
    return info;
}
//---------------------------------------------------------------------------
int TGLProgram::LoadFromFile(const char *file_name, GLenum bin_format)
{
    lbxSTREAM *s = new_file_stream(file_name, "rb");
    int r = LoadFromStream(s, (int)stream_get_size(s), bin_format);
    delete_stream(s);
    return r;
}
//---------------------------------------------------------------------------
int TGLProgram::LoadFromStream(lbxSTREAM *s, int length, GLenum bin_format)
{
    void * buf = alloc_memory(length);
    stream_read(s, buf, length);
    int r = LoadBinary(buf, length, bin_format);
    free_memory(buf);
    return r;
}
//---------------------------------------------------------------------------
int TGLProgram::LoadBinary(const void *data, int length, GLenum bin_format)
{
    #ifndef GL_GLEXT_PROTOTYPES
    static PFNGLPROGRAMBINARYOESPROC glProgramBinaryOES = (PFNGLPROGRAMBINARYOESPROC)eglGetProcAddress("glProgramBinaryOES");
    #endif //#ifndef GL_GLEXT_PROTOTYPES
    flags &= ~(uint32_t)pfLinked;
    glProgramBinaryOES(GetHandle(), bin_format, data, length);
    if (glGetError() == GL_NO_ERROR) {
        flags |= (uint32_t)pfLinked;
        Analyze();
        return 1;
    } else {
        return 0;
    }
}
//---------------------------------------------------------------------------
int TGLProgram::SaveBinary(void *dst, int dst_size, GLenum *bin_format)
{
    #ifndef GL_GLEXT_PROTOTYPES
    static PFNGLGETPROGRAMBINARYOESPROC glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)eglGetProcAddress("glGetProgramBinaryOES");
    #endif //#ifndef GL_GLEXT_PROTOTYPES
    if (dst == NULL || dst_size == 0) {
        return GetBinarySize();
    }
    GLsizei length;
    GLenum binfmt;
    if (bin_format == NULL) {
        bin_format = &binfmt;
    }
    GL_ASSERT(glGetProgramBinaryOES(GetHandle(), dst_size, &length, bin_format, dst));
    return length;
}
//---------------------------------------------------------------------------
int TGLProgram::SaveToMem(void **rcm)
{
    int l = GetBinarySize(), r;
    if (l > 0) {
        rcm_set_length(rcm, l, 1);
        GLenum bin_format;
        r = SaveBinary(*rcm, l, &bin_format);
        if (l != r) {
            Err_("Binary size mismatch - %d:%d", l, r);
            rcm_set_length(rcm, r, 1);
        } else {
            Log_("Saved program binary (%d bytes, format=0x%X)", r, bin_format);
        }
        return r;
    }
    return 0;
}
//---------------------------------------------------------------------------
int TGLProgram::SaveToFile(const char *file_name)
{
    lbxSTREAM *s = new_file_stream(file_name, "wb");
    int r = SaveToStream(s);
    delete_stream(s);
    return r;
}
//---------------------------------------------------------------------------
int TGLProgram::SaveToStream(lbxSTREAM *s)
{
    void *buf = NULL;
    SaveToMem(&buf);
    int r = (int)stream_write(s, buf, rcm_length(buf));
    rcm_free(&buf);
    return r;
}
//---------------------------------------------------------------------------
TGLShader * TGLProgram::CreateShader(GLenum type, const char *src, const char *hdr)
{
    TGLShader *ns = NULL;
    int i = -1;
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
            flags |= ((uint32_t)pfOwnsVertexShader << i);
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
int TGLProgram::Build(TGLVertexShader *vshader, const char *fshader, const char *hdr)
{
    SetShader(vshader);
    return CreateShader(GL_FRAGMENT_SHADER, fshader, hdr) ? Link() : 0;
}
//---------------------------------------------------------------------------
int TGLProgram::Build(const char *vshader, TGLFragmentShader *fshader, const char *hdr)
{
    SetShader(fshader);
    return CreateShader(GL_VERTEX_SHADER, vshader, hdr) ? Link() : 0;
}
//---------------------------------------------------------------------------
int TGLProgram::Build(const char *vshader, const char *fshader, const char *hdr)
{
    if (CreateShader(GL_VERTEX_SHADER, vshader, hdr) &&
        CreateShader(GL_FRAGMENT_SHADER, fshader, hdr)) {
        return Link();
    }
    return 0;
}
//---------------------------------------------------------------------------


const UString g_POSITION    = "position";
const UString g_NORMAL      = "normal";
const UString g_TANGENT     = "tangent";
const UString g_TEXCOORD    = "texcoord";
const UString g_COLOR       = "color";
const UString g_JOINT       = "joint";
const UString g_WEIGHT       = "weight";

//---------------------------------------------------------------------------
uint8_t EstimateAttribTypeFromName(const char *param_name, int l)
{
    uint8_t type = 0u;
    UString lc;
    const char *name = NULL;
    const char *found = NULL;

    if (l == -1) {
        l = (int)strlen(param_name);
    }
    lc = param_name;
    lc.LowerCase();
    name = lc.c_str();

    if ((found = strstr(name, "position")) != NULL) {
        type = LB_POSITION;
        found += 8;
    } else if ((found = strstr(name, "normal")) != NULL) {
        type = LB_NORMAL;
        found += 6;
    } else if ((found = strstr(name, "tangent")) != NULL) {
        type = LB_TANGENT;
        found += 7;
    } else if ((found = strstr(name, "color")) != NULL) {
        type = LB_COLOR;
        found += 5;
    } else if ((found = strstr(name, "coord")) != NULL && strstr(name, "tex")) {
        type = LB_TEXCOORD;
        found += 5;
    } else if ((found = strstr(name, "joint")) != NULL) {
        type = LB_JOINTS;
        found += 5;
    } else if ((found = strstr(name, "weight")) != NULL) {
        type = LB_WEIGHTS;
        found += 6;
    } else if ((found = strstr(name, "vertex")) != NULL) {
        type = LB_POSITION;
        found += 6;
    } else if ((found = strstr(name, "vtx")) != NULL) {
        type = LB_POSITION;
        found += 3;
    } else if ((found = strstr(name, "uv")) != NULL) {
        type = LB_TEXCOORD;
        found += 2;
    } else if ((found = strstr(name, "txc")) != NULL) {
        type = LB_TEXCOORD;
        found += 3;
    } else if ((found = strstr(name, "col")) != NULL) {
        type = LB_COLOR;
        found += 3;
    } else if ((found = strstr(name, "nor")) != NULL) {
        type = LB_NORMAL;
        found += 3;
    } else {
        return type;
    }

    // 번호가 있는 경우 이를 감지해서 반영
    for (; *found != 0; found++) {
        char c = *found;
        if (c >= '0' && c <= '9') {
            type |= (uint8_t)(c - '0');
        }
    }

    return type;

}

void TGLProgram::Analyze(void)
{
    int missing_count = 0;
    int attrib_count = GetIntParam(GL_ACTIVE_ATTRIBUTES);
    int max_length = GetIntParam(GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
    RCM_SET_LENGTH(uint8_t, &attrib_types, attrib_count);
    RCM_SET_LENGTH(void *, &attrib_data, attrib_count);
    memset(attrib_types, 0, sizeof(uint8_t) * attrib_count);
    memset(attrib_data, 0, sizeof(void *) * attrib_count);

    UString name;
    name.SetLength(max_length);

    for (int i = 0; i < attrib_count; i++) {
        GLenum type;
        GLint length;
        GLint size;
        glGetActiveAttrib(handle, (GLuint)i, max_length, &length, &size, &type, name.c_str());
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
TGLProgram::AttributeDescriptor * TGLProgram::FindAttributeInfoByType(TGLAttributeType type, int16_t num)
{
    int l = rcm_length(attrib_info);
    for (int i = 0; i < l; i++) {
        if (attrib_info[i].type == type && attrib_info[i].idx == num) {
            return attrib_info + i;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
*/
/*
TGLProgram::AttributeDescriptor * TGLProgram::FindAttributeInfoByLoc(TGLAttributeType type, int16_t num)
{
    int l = rcm_length(attrib_info);
    for (int i = 0; i < l; i++) {
        if (attrib_info[i].type == type && attrib_info[i].idx == num) {
            return attrib_info + i;
        }
    }
    return NULL;
}
*/
int TGLProgram::LocationOfAttribType(uint8_t type)
{
    int l = rcm_length(attrib_types);
    for (int i = 0; i < l; i++) {
        if (attrib_types[i] == type) {
            return i;
        }
    }
    return -1;
}
//---------------------------------------------------------------------------
void TGLProgram::SetAttributeType(int location, uint8_t type)
{
    int l = rcm_length(attrib_types);
    assert(location >= 0 && location < l);
    attrib_types[location] = type;
}
//---------------------------------------------------------------------------
int TGLProgram::Link(void)
{
    GLuint program = GetHandle();
    flags &= ~(uint32_t)pfLinked;
    for (int i = 0; i < 2; i++) {
        if (shaders[i]) {
            GL_ASSERT(glAttachShader(program, shaders[i]->GetHandle()));
        }
    }
    GL_ASSERT(glLinkProgram(program));

    if (GetLinkStatus()) {
        flags |= (uint32_t)pfLinked;
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
    return (flags & (uint32_t)pfLinked);
}

//---------------------------------------------------------------------------
void TGLProgram::DisableArrayMode(int location)
{
    Use();
    if (location == -1) {
        int l = rcm_length(attrib_types);
        for (int i = 0; i < l; i++) {
            glDisableVertexAttribArray(i);
        }
    } else {
        glDisableVertexAttribArray(location);
    }
}
//---------------------------------------------------------------------------
void TGLProgram::EnableArrayMode(int location)
{
    Use();
    if (location == -1) {
        int l = rcm_length(attrib_types);
        for (int i = 0; i < l; i++) {
            glEnableVertexAttribArray(i);
        }
    } else {
        glEnableVertexAttribArray(location);
    }
}
//---------------------------------------------------------------------------
void TGLProgram::Attribute(GLint attrib_loc, GLint components, GLenum component_type, GLboolean normalize, int stride, const void *offset)
{
    glEnableVertexAttribArray(attrib_loc);
    glVertexAttribPointer(attrib_loc, components, component_type, normalize, stride, offset);
}
//---------------------------------------------------------------------------
void TGLProgram::Attribute(GLint attrib_loc, vec4_u8 data)
{
    glDisableVertexAttribArray(attrib_loc);
    vec4_f32 n = vec4_f32_((float)data.x / 255.0f, (float)data.y / 255.0f, (float)data.z / 255.0f, (float)data.w / 255.0f);
    glVertexAttrib4fv(attrib_loc, (float*)&n);
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const float *data, int count)
{
    Use(); glUniform1fv(uniform_loc, count, data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const vec2_f32 *data, int count)
{
    Use(); glUniform2fv(uniform_loc, count, (float*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const vec3_f32 *data, int count)
{
    Use(); glUniform3fv(uniform_loc, count, (float*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const vec4_f32 *data, int count)
{
    Use(); glUniform4fv(uniform_loc, count, (float*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const int *data, int count)
{
    Use(); glUniform1iv(uniform_loc, count, data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const vec2_i32 *data, int count)
{
    Use(); glUniform2iv(uniform_loc, count, (int*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const vec3_i32 *data, int count)
{
    Use(); glUniform3iv(uniform_loc, count, (int*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const vec4_i32 *data, int count)
{
    Use(); glUniform4iv(uniform_loc, count, (int*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const mat2_f32 *data, int count)
{
    Use(); glUniformMatrix2fv(uniform_loc, count, GL_FALSE, (float*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const mat3_f32 *data, int count)
{
    Use(); glUniformMatrix3fv(uniform_loc, count, GL_FALSE, (float*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, const mat4_f32 *data, int count)
{
    Use(); glUniformMatrix4fv(uniform_loc, count, GL_FALSE, (float*)data); return 1;
}
//---------------------------------------------------------------------------
int TGLProgram::Uniform(GLint uniform_loc, LB_TYPE type, const void *data, int count)
{
    int dim = LB_TYPE_DIMENSION(type);
    bool is_mat = LB_TYPE_ISMATRIX(type);
    switch (LB_TYPE_ELEMTYPE(type)) {
        case LB_TYPE_F32:
            if (is_mat) {
                switch (LB_TYPE_DIMENSION(type)) {
                    case 2: return Uniform(uniform_loc, (mat2_f32*)data, count);
                    case 3: return Uniform(uniform_loc, (mat3_f32*)data, count);
                    case 4: return Uniform(uniform_loc, (mat4_f32*)data, count);
                    default: break;
                }
            } else {
                switch (LB_TYPE_DIMENSION(type)) {
                    case 1: return Uniform(uniform_loc, (float*)data, count);
                    case 2: return Uniform(uniform_loc, (vec2_f32*)data, count);
                    case 3: return Uniform(uniform_loc, (vec3_f32*)data, count);
                    case 4: return Uniform(uniform_loc, (vec4_f32*)data, count);
                    default: break;
                }
            }
            break;
        case LB_TYPE_F64:
        case LB_TYPE_U8:
        case LB_TYPE_U16:
        case LB_TYPE_U32:
        case LB_TYPE_U64:
        case LB_TYPE_I8:
        case LB_TYPE_I16:
            break;
        case LB_TYPE_I32:
            switch (LB_TYPE_DIMENSION(type)) {
                case 1: return Uniform(uniform_loc, (int*)data, count);
                case 2: return Uniform(uniform_loc, (vec2_i32*)data, count);
                case 3: return Uniform(uniform_loc, (vec3_i32*)data, count);
                case 4: return Uniform(uniform_loc, (vec4_i32*)data, count);
                default: break;
            }
            break;
        case LB_TYPE_I64:
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
        GL_ASSERT(glDeleteBuffers(1, &handle));
    }
}
//---------------------------------------------------------------------------
void TGLBufferBase::Bind(GLenum target)
{
    GL_ASSERT(glBindBuffer(target, GetHandle()));
}
//---------------------------------------------------------------------------
int TGLBufferBase::GetIntParam(GLenum target, GLenum pname)
{
    GLint v = 0;
    glBindBuffer(target, GetHandle());
    GL_ASSERT(glGetBufferParameteriv(target, pname, &v));
    return v;
}
//---------------------------------------------------------------------------
GLuint TGLBufferBase::GetHandle(void)
{
    if (handle == 0) {
        GL_ASSERT(glGenBuffers(1, &handle));
    }
    return handle;
}
//---------------------------------------------------------------------------
int TGLBufferBase::Upload(GLenum target, GLenum usage)
{
    Bind(target);
    GL_ASSERT(glBufferData(target, rcm_size(local_data), local_data, usage));
    return 1;
}
//---------------------------------------------------------------------------
int TGLBufferBase::Upload(GLenum target, const void *data, int size, GLenum usage)
{
    Bind(target);
    GL_ASSERT(glBufferData(target, size, data, usage));
    return 1;
}
//---------------------------------------------------------------------------
void * TGLBufferBase::AppendLocalData(int count, int elem_size)
{
    return rcm_append(&local_data, count, elem_size);
}
//---------------------------------------------------------------------------
void * TGLBufferBase::InsertLocalData(int index, int count, int elem_size)
{
    return rcm_insert(&local_data, index, count, elem_size);
}
//---------------------------------------------------------------------------



/*
LB_ATTRIB_BIND_INFO::LB_ATTRIB_BIND_INFO(LB_TYPE type, int a_stride)
{
    SetType(type);
    stride = (a_stride == 0) ? LB_TYPE_Size_(type) : a_stride;
}
//---------------------------------------------------------------------------
void LB_ATTRIB_BIND_INFO::void SetOffset(GLuint a_buffer_id, int offset)
{
    buffer_id = a_buffer_id;
    pointer = (void*)(uintptr_t)offset;
}
//---------------------------------------------------------------------------
void LB_ATTRIB_BIND_INFO::SetPointer(void *address, int offset)
{
    pointer = address;
}
//---------------------------------------------------------------------------
int LB_ATTRIB_BIND_INFO::SetType(LB_TYPE type)
{
    components = LB_TYPE_Dimension_(type);
    normalized = false;

    switch (LB_TYPE_ElemType_(type)) {
        case LB_TYPE_F32:
            component_type = GL_FLOAT;
            break;
        case LB_TYPE_F64:
            component_type = 0; // unsupported
            break;
        case LB_TYPE_U8:
            component_type = GL_UNSIGNED_BYTE;
            normalized = true;
            break;
        case LB_TYPE_U16:
            component_type = GL_UNSIGNED_SHORT;
            normalized = true;
            break;
        case LB_TYPE_U32:
            component_type = 0; // unsupported
            break;
        case LB_TYPE_U64:
            component_type = 0; // unsupported
            break;
        case LB_TYPE_I8:
            component_type = GL_BYTE;
            normalized = true;
            break;
        case LB_TYPE_I16:
            component_type = GL_SHORT;
            normalized = true;
            break;
        case LB_TYPE_I32:
            component_type = GL_FIXED;
            break;
        case LB_TYPE_I64:
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
int TGLIndexBuffer::Size(void)
{
    return GetIntParam(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE);
}




TGLAttribBuffer::TGLAttribBuffer()
    : TGLBufferBase(GL_ARRAY_BUFFER), bd(NULL)
{

}
TGLAttribBuffer::TGLAttribBuffer(void *data, int size, GLenum usage)
    : TGLBufferBase(GL_ARRAY_BUFFER), bd(NULL)
{
    Upload(data, size, usage);
}

TGLAttribBuffer::~TGLAttribBuffer()
{
    RCM_FREE(&bd);
}

TGLAttribBuffer & TGLAttribBuffer::Register(char attrib_type, LB_TYPE data_type, uint16_t offset)
{
    LB_BUFFER_DESCRIPTOR *p = RCM_APPEND(LB_BUFFER_DESCRIPTOR, &bd, 1);
    p->attrib_type = attrib_type;
    p->data_type = data_type;
    p->offset = offset;
    return *this;
}

int TGLAttribBuffer::Register(const lbxREFL_STRUCT_INFO *rtti)
{
    RCM_FREE(&bd);
    lbxREFL_MEMBER_ITERATOR it = refl_get_member_iterator(rtti);
    const lbxREFL_MEMBER_INFO *mi;
    int r = 0;
    while ((mi = refl_next_member(&it)) != NULL) {
        uint8_t attrib_type = EstimateAttribTypeFromName(mi->id);
        if (attrib_type) {
            Register(attrib_type, mi->type, mi->offset);
            r++;
        }
    }
    return r;
}


int TGLAttribBuffer::BindTo(TGLProgram *program, int elem_size)
{
    int i, i_end, r = 0;
    i_end = rcm_length(bd);
    program->Use();
    Bind();
    for (i = 0; i < i_end; i++) {
        LB_BUFFER_DESCRIPTOR d = bd[i];
        GLint loc = program->LocationOfAttribType(d.attrib_type);
        if (loc == -1) {
            continue;
        }
        LB_GL_TYPE glt = GetGLTypeInfo(d.data_type);
        if (glt.components == 0) {
            continue;
        }
        glBindBuffer(GL_ARRAY_BUFFER, GetHandle());
        program->EnableArrayMode(loc);

/*
            glVertexAttribPointer(si->attrib_loc, si->components,
            si->component_type, si->normalize, bi[i]->stride,
            bi[i]->data + si->offset);
*/
        glVertexAttribPointer(loc, glt.components, glt.component_type, glt.normalize, elem_size, (void*)d.offset);
        r++;
    }
    return r;
}


LB_GL_TYPE GetGLTypeInfo(LB_TYPE type)
{
    LB_GL_TYPE r = {0,};

    switch (LB_TYPE_ELEMTYPE(type)) {
        case LB_TYPE_F32:
            r.component_type = GL_FLOAT;
            break;
        case LB_TYPE_F64:
            Err_("Unsupported type");
            return r;
        case LB_TYPE_U8:
            r.component_type = GL_UNSIGNED_BYTE;
            r.normalize = true;
            break;
        case LB_TYPE_U16:
            r.component_type = GL_UNSIGNED_SHORT;
            r.normalize = true;
            break;
        case LB_TYPE_U32:
        case LB_TYPE_U64:
            Err_("Unsupported type");
            return r;
        case LB_TYPE_I8:
            r.component_type = GL_BYTE;
            r.normalize = true;
            break;
        case LB_TYPE_I16:
            r.component_type = GL_SHORT;
            r.normalize = true;
            break;
        case LB_TYPE_I32:
            r.component_type = GL_FIXED;
            break;
        case LB_TYPE_I64:
            Err_("Unsupported type");
            return r;
        default:
            break;
    }

    r.components = LB_TYPE_DIMENSION(type);
    return r;
}



//LB_ATTRIB_BIND_INFO *bi;

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
TGLAttribBinder::TGLAttribBinder(TGLProgram *program, const lbxREFL_STRUCT_INFO *typeinfo)
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
LB_BUFFER_INFO * TGLAttribBinder::add_buffer(void)
{
    LB_BUFFER_INFO ** r = RCM_APPEND(LB_BUFFER_INFO *, &bi, 1);
    *r = (LB_BUFFER_INFO *)rcm_from_length(sizeof(LB_BUFFER_INFO), 1);
    return *r;
}
//---------------------------------------------------------------------------
LB_ATTRIB_BINDING * TGLAttribBinder::AddBinding(LB_BUFFER_INFO *buffer_info)
{
    LB_BUFFER_INFO ** pbi = bi;
    int l = rcm_length(bi);
    if (l == 0) {
        return NULL;
    }
    pbi = (buffer_info == NULL) ? bi + (l - 1) : find_buffer(buffer_info);
    if (pbi) {
        return (LB_ATTRIB_BINDING *)rcm_append((void**)pbi, sizeof(LB_ATTRIB_BINDING), 1);
    } else {
        return NULL;
    }
}
//---------------------------------------------------------------------------
LB_BUFFER_INFO * TGLAttribBinder::SetBuffer(GLint buffer_id, int stride)
{
    LB_BUFFER_INFO *r = add_buffer();
    r->buffer_id = buffer_id;
    r->stride = stride;
    r->data = NULL;
    return r;
}
//---------------------------------------------------------------------------
LB_BUFFER_INFO * TGLAttribBinder::SetBuffer(void * buffer_addr, int stride)
{
    LB_BUFFER_INFO *r = add_buffer();
    r->buffer_id = 0;
    r->stride = stride;
    r->data = (uint8_t*)buffer_addr;
    return r;
}
//---------------------------------------------------------------------------
LB_BUFFER_INFO ** TGLAttribBinder::find_buffer(LB_BUFFER_INFO* to_find)
{
    LB_BUFFER_INFO ** pbi = bi;
    int l = rcm_length(bi);
    for (int i = 0; i < l; i++, pbi++) {
        if (*pbi == to_find) {
            return pbi;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
LB_ATTRIB_BINDING * TGLAttribBinder::AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLint offset, LB_BUFFER_INFO * buffer_info)
{
    LB_ATTRIB_BINDING * r = AddBinding(buffer_info);
    if (r) {
        r->attrib_loc = attrib_loc;
        r->components = comp_count;
        r->component_type = comp_type;
        r->offset = offset;
    }
    return r;
}
//---------------------------------------------------------------------------
LB_ATTRIB_BINDING * TGLAttribBinder::AddBinding(GLint attrib_loc, LB_TYPE type, GLint offset, LB_BUFFER_INFO * buffer_info)
{
    LB_ATTRIB_BINDING * r = NULL;
    GLenum comp_type = 0;
    GLboolean normalize = false;

    if (attrib_loc < 0) {
        Err_("Invalid attrib loc");
        return NULL;
    }

    switch (LB_TYPE_ELEMTYPE(type)) {
        case LB_TYPE_F32:
            comp_type = GL_FLOAT;
            break;
        case LB_TYPE_F64:
            return NULL; // unsupported
        case LB_TYPE_U8:
            comp_type = GL_UNSIGNED_BYTE;
            normalize = true;
            break;
        case LB_TYPE_U16:
            comp_type = GL_UNSIGNED_SHORT;
            normalize = true;
            break;
        case LB_TYPE_U32:
        case LB_TYPE_U64:
            return NULL; // unsupported
        case LB_TYPE_I8:
            comp_type = GL_BYTE;
            normalize = true;
            break;
        case LB_TYPE_I16:
            comp_type = GL_SHORT;
            normalize = true;
            break;
        case LB_TYPE_I32:
            comp_type = GL_FIXED;
            break;
        case LB_TYPE_I64:
            return NULL; // unsupported
        default:
            break;
    }

    r = AddBinding(buffer_info);
    if (r) {
        r->attrib_loc = attrib_loc;
        r->components = LB_TYPE_DIMENSION(type);
        r->component_type = comp_type;
        r->normalize = normalize;
        r->offset = offset;
    }
    return r;
}

int TGLAttribBinder::AddBinding(GLint attrib_loc, const lbxREFL_STRUCT_INFO *ti, const char *member_name, GLboolean normalize)
{

    return 1;
}
//---------------------------------------------------------------------------
int TGLAttribBinder::AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLboolean normalize, GLint stride, void * pointer)
{

    return 1;
}
//---------------------------------------------------------------------------
int TGLAttribBinder::AddUniform(GLint uniform_loc, LB_TYPE type, void *data)
{
    return 1;
}
//---------------------------------------------------------------------------
void TGLAttribBinder::Clear(void)
{
    int i, i_end;
    i_end = rcm_length(bi);
    for (i = 0; i < i_end; i++) {
        rcm_free((void**)(bi + i));
    }
    rcm_free((void**)&bi);
}
//---------------------------------------------------------------------------

int TGLAttribBinder::Enable(void)
{
    int i, i_end, j, j_end;
    if (p == NULL) {
        return 0;
    }
    p->Use();
    i_end = rcm_length(bi);
    for (i = 0; i < i_end; i++) {
        j_end = GetBufferSubCount(bi[i]);
        glBindBuffer(GL_ARRAY_BUFFER, bi[i]->buffer_id);
        LB_ATTRIB_BINDING * si = GetBufferSub(bi[i]);
        for (j = 0; j < j_end; j++, si++) {
            glEnableVertexAttribArray(si->attrib_loc);
            glVertexAttribPointer(si->attrib_loc, si->components,
                si->component_type, si->normalize, bi[i]->stride,
                bi[i]->data + si->offset);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return 1;
}
//---------------------------------------------------------------------------
int TGLAttribBinder::Disable(void)
{
    int i, i_end, j, j_end;
    i_end = rcm_length(bi);
    if (p == NULL) {
        return 0;
    }
    p->Use();
    for (i = 0; i < i_end; i++) {
        j_end = GetBufferSubCount(bi[i]);
        LB_ATTRIB_BINDING * si = GetBufferSub(bi[i]);
        for (j = 0; j < j_end; j++, si++) {
            glDisableVertexAttribArray(si->attrib_loc);
        }
    }
    return 1;
}
//---------------------------------------------------------------------------




TGLCommandList::TGLCommandList()
    : ab(NULL), list(NULL)
{
}
TGLCommandList::~TGLCommandList()
{
    Clear();
}
void TGLCommandList::Clear(void)
{
    int i, i_end = Count();
    for (i = 0; i < i_end; i++) {
        RCM_FREE(&(list[i].indice));
    }
    RCM_FREE(&list);
}

LB_RENDER_COMMAND * TGLCommandList::Add(GLenum method, bool *visible)
{
    LB_RENDER_COMMAND * r = RCM_APPEND(LB_RENDER_COMMAND, &list, 1);
    memset(r, 0, sizeof(LB_RENDER_COMMAND));
    r->command = method;
    r->visible = visible;
    return r;
}
LB_RENDER_COMMAND * TGLCommandList::AddArrays(GLenum method, int first, int count, bool *visible)
{
    LB_RENDER_COMMAND * r = Add(method, visible);
    r->first = first;
    r->count = count;
    return r;
}
LB_RENDER_COMMAND * TGLCommandList::Add(GLenum method, LB_TYPE type, int count, bool *visible)
{
    GLenum gltype = 0;
    switch (type) {
        case LB_TYPE_U16:
            gltype = GL_UNSIGNED_SHORT;
            break;
        case LB_TYPE_U8:
            gltype = GL_UNSIGNED_BYTE;
            break;
        default:
            return NULL;
    }
    LB_RENDER_COMMAND * r = Add(method, visible);
    r->type = gltype;
    r->count = count;
    rcm_set_length((void**)(&(r->indice)), count, LB_TYPE_ELEMSIZE(type));
    return r;
}
LB_RENDER_COMMAND * TGLCommandList::Add(GLenum method, GLuint indice_buffer_id, GLenum elem_type,  int count, bool *visible)
{
    LB_RENDER_COMMAND * r = Add(method, visible);
    r->buffer_id = indice_buffer_id;
    r->type = elem_type;
    r->count = count;
    return r;
}
LB_RENDER_COMMAND * TGLCommandList::Add(GLenum method, const uint16_t *indice, int count, bool *visible)
{
    LB_RENDER_COMMAND * r = Add(method, visible);
    rcm_set_length(&(r->indice), count, sizeof(uint16_t));
    if (indice) {
        copy_memory(r->indice, indice, sizeof(uint16_t) * count);
    }
    r->type = GL_UNSIGNED_SHORT;
    r->count = count;
    return r;
}
LB_RENDER_COMMAND * TGLCommandList::Add(GLenum method, const uint8_t *indice, int count, bool *visible)
{
    LB_RENDER_COMMAND * r = Add(method, visible);
    rcm_set_length(&(r->indice), count, sizeof(uint8_t));
    if (indice) {
        copy_memory(r->indice, indice, sizeof(int32_t) * count);
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




int DrawCommandList(TGLCommandList *list)
{
    int i_end = list->Count();
    if (list->ab) {
        list->ab->Enable();
    }
    for (int i = 0; i < i_end; i++) {
        LB_RENDER_COMMAND *c = list->Items(i);
        if (c->visible && *(c->visible) != true) {
            continue;
        }
        GL_ASSERT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, c->buffer_id));
        if (c->buffer_id == 0 && c->indice == NULL) {
            GL_ASSERT(glDrawArrays(c->command, c->first, c->count));
        } else {
            GL_ASSERT(glDrawElements(c->command, (c->indice) ? rcm_length(c->indice) : c->count, c->type, c->indice));
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
LB_ATTRIB_BINDING * TGLDrawList::SetBinding(const char *attrib_name, LB_TYPE type, GLint offset)
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
void TGLDrawList::FillBuffer(V2CT *dst, TGlyph *glyph, rect_f32 area, uint32_t color)
{
    int cnt = glyph->FillVertexList(&(dst->vtx), area, sizeof(V2CT));
    // TexCoord와 Color를 채우고
    for (int i = 0; i < cnt; i++) {
        dst[i].txc = glyph->GetTexCoords()[i];
        dst[i].col = color;
    }
/*
    uint16_t *idx = new uint16_t[icnt];
    for (int i = 0; i < icnt; i++) {
        idx[i] = base + indice[i];
    }
*/	modified = true;
}
//---------------------------------------------------------------------------
int TGLDrawList::AddGlyph(TGlyph *glyph, rect_f32 area, uint32_t color)
{
    int cnt, icnt, base;
    const uint8_t *indice = glyph->GetDrawIndice(&icnt);
    base = rcm_length(bf.GetLocalData());
    cnt = glyph->GetVertexCount();
    V2CT *v = (V2CT*)bf.AppendLocalData(cnt, sizeof(V2CT)); // 버퍼 크기를 늘리고
    // Vertex정보를 채우고
    cnt = glyph->FillVertexList(&(v->vtx), area, sizeof(V2CT));
    // TexCoord와 Color를 채우고
    for (int i = 0; i < cnt; i++) {
        v[i].txc = glyph->GetTexCoords()[i];
        v[i].col = color;
    }
    uint16_t *idx = new uint16_t[icnt];
    for (int i = 0; i < icnt; i++) {
        idx[i] = base + indice[i];
    }
    cl.Add(GL_TRIANGLES, idx, icnt);
    modified = true;
    delete [] idx;
    return 1;
}
//---------------------------------------------------------------------------
int TGLDrawList::AddLines(vec2_f32 *lines_vertice, int line_count, uint32_t color)
{
    int vtx_count = line_count * 2;
    int base = rcm_length(bf.GetLocalData());
    V2CT *v = (V2CT*)bf.AppendLocalData(vtx_count, sizeof(V2CT)); // 버퍼 크기를 늘리고
    vec2_f32 txc = vec2_f32_(1.0f / 1024.0f, 1.0f / 1024.0f);
    for (int i = 0; i < vtx_count; i++) {
        v[i].vtx = lines_vertice[i];
        v[i].col = color;
        v[i].txc = txc;
    }
    cl.AddArrays(GL_LINES, base, vtx_count);
/*
    uint16_t *idx = new uint16_t[vtx_count];
    for (int i = 0; i < vtx_count; i++) {
        idx[i] = base + i;
    }
    cl.Add(GL_LINES, idx, vtx_count);
    modified = true;
    delete [] idx;
*/
    return 1;
}

//---------------------------------------------------------------------------
int TGLDrawList::Draw(void)
{
    if (modified) {
        bf.Upload();
        modified = false;
    }
    DrawCommandList(&cl);
    return 1;
}





TGLFrameBufferObject::TGLFrameBufferObject()
    : inherited(), tex(NULL), depth(0)
{
    sz = size2_i16_(0,0);
}
TGLFrameBufferObject::TGLFrameBufferObject(int width, int height)
    : inherited(), tex(NULL), depth(0)
{
    sz = size2_i16_(width, height);
//    SetSize(width, height);
}

TGLFrameBufferObject::~TGLFrameBufferObject()
{
    if (handle) {
        glDeleteFramebuffers(1, &handle);
        handle = 0;
    }
    if (depth) {
        glDeleteRenderbuffers(1, &depth);
    }
    delete tex;
}
int TGLFrameBufferObject::SetSize(int width, int height)
{
    GLint n_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &n_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, GetHandle());

    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    tex->Bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (n_fbo != handle) {
        glBindFramebuffer(GL_FRAMEBUFFER, n_fbo);
    }
    return (int)status;
}
void TGLFrameBufferObject::Bind(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, GetHandle());
}
void TGLFrameBufferObject::Release(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint TGLFrameBufferObject::GetHandle(void)
{
    if (!handle) {
        GLint n_fbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &n_fbo);

        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);

        glGenRenderbuffers(1, &depth);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        GL_ASSERT(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, sz.width, sz.height));
        GL_ASSERT(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth));

        if (tex == NULL) {
            tex = new TGLTexture2D();
        }

//		glGenTextures(1, &tex);
//		glBindTexture(GL_TEXTURE_2D, tex);
//		GL_ASSERT(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));

        tex->Bind();
        GL_ASSERT(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL_ASSERT(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL_ASSERT(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sz.width, sz.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
        GL_ASSERT(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->GetHandle(), 0)); // 마지막은 mipmap 레벨임
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

/*
        uint32_t *buf = new uint32_t[1280*720];
        for (int i = 0; i < 1280*720; i++) {
            buf[i] = 0xff00ffff;
        }
        GL_ASSERT(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf));
        delete [] buf;
*/
        if (n_fbo != handle) {
            glBindFramebuffer(GL_FRAMEBUFFER, n_fbo);
        }
    }
    return handle;
}

