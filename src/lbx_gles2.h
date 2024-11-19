//---------------------------------------------------------------------------

#ifndef lbx_gles2H
#define lbx_gles2H
//---------------------------------------------------------------------------
#include "lbx_stream.h"
#include "image/lbx_image.h"
#include "lbx_ustr.h"
#include "lbx_refl.h"
#include "lbx_core.h"
#include "lbx_gl.h"

typedef void (GL_APIENTRY * TGLGetStringFunc)(GLuint handle,  GLsizei bufSize,  GLsizei *length,  GLchar *data);
extern UString GetGLStringData(GLuint obj, i32_t length, TGLGetStringFunc func);

// Attrib Types
#define LBX_UNKNOWN     ( 0u)
#define LBX_POSITION    (10u)
#define LBX_NORMAL      (20u)
#define LBX_TANGENT     (30u)
#define LBX_TEXCOORD    (40u)
#define LBX_TEXCOORD_0  (40u)
#define LBX_TEXCOORD_1  (41u)
#define LBX_TEXCOORD_2  (42u)
#define LBX_TEXCOORD_3  (43u)
#define LBX_TEXCOORD_4  (44u)
#define LBX_TEXCOORD_5  (45u)
#define LBX_TEXCOORD_6  (46u)
#define LBX_TEXCOORD_7  (47u)
#define LBX_TEXCOORD_8  (48u)
#define LBX_TEXCOORD_9  (49u)
#define LBX_COLOR       (50u)
#define LBX_COLOR_0     (50u)
#define LBX_COLOR_1     (51u)
#define LBX_COLOR_2     (52u)
#define LBX_COLOR_3     (53u)
#define LBX_COLOR_4     (54u)
#define LBX_COLOR_5     (55u)
#define LBX_COLOR_6     (56u)
#define LBX_COLOR_7     (57u)
#define LBX_COLOR_8     (58u)
#define LBX_COLOR_9     (59u)
#define LBX_JOINTS      (60u)
#define LBX_JOINTS_0    (60u)
#define LBX_JOINTS_1    (61u)
#define LBX_JOINTS_2    (62u)
#define LBX_JOINTS_3    (63u)
#define LBX_JOINTS_4    (64u)
#define LBX_JOINTS_5    (65u)
#define LBX_JOINTS_6    (66u)
#define LBX_JOINTS_7    (67u)
#define LBX_JOINTS_8    (68u)
#define LBX_JOINTS_9    (69u)
#define LBX_WEIGHTS     (70u)
#define LBX_WEIGHTS_0   (70u)
#define LBX_WEIGHTS_1   (71u)
#define LBX_WEIGHTS_2   (72u)
#define LBX_WEIGHTS_3   (73u)
#define LBX_WEIGHTS_4   (74u)
#define LBX_WEIGHTS_5   (75u)
#define LBX_WEIGHTS_6   (76u)
#define LBX_WEIGHTS_7   (77u)
#define LBX_WEIGHTS_8   (78u)
#define LBX_WEIGHTS_9   (79u)

u8_t EstimateAttribTypeFromName(const char *name, i32_t l = -1);

//===========================================================================
class LBX_GL_EXPORT TGLObject
{
protected:
    GLuint handle;
public:
    TGLObject();
    virtual ~TGLObject();

    virtual GLuint GetHandle(void);
    inline bool Initialized(void) {return handle != 0;}
};


typedef enum {
    atUnknown = 0x00u,
    atPosition = 0x10u,
    atNormal = 0x20u,
    atTangent = 0x30u,
    atTexCoord = 0x40u,
    atColor = 0x50u,
    atJoints = 0x60u,
    atWeights = 0x70u
} TGLAttributeType;

//===========================================================================
class LBX_GL_EXPORT TGLShader : public TGLObject
{
private:
    u32_t flags;
protected:
    void CreateGLObject(GLenum type);
    i32_t GetIntParam(GLenum pname);

public:
    enum TGLShaderFlags {
        sfCompiled = 1,
    };
    TGLShader(GLenum type);
    TGLShader(GLenum type, LBX_STREAM *strm);
    TGLShader(GLenum type, const char *code, const char *hdr = NULL);
    TGLShader(GLenum type, const char *code, i32_t code_len, const char *hdr = NULL, i32_t hdr_len = -1);
    virtual ~TGLShader();
    i32_t SetSource(const char *code, const char *hdr = NULL);
    i32_t SetSource(const char *code, i32_t code_len, const char *hdr = NULL, i32_t hdr_len = -1);
    i32_t SetSource(GLint count, const char **codes, const GLint *lengths = NULL);
    i32_t SetSource(LBX_STREAM *strm);
    i32_t SetBinary(LBX_STREAM *strm);
    i32_t Load(const void *data, GLsizei length, GLenum binaryformat);

    inline GLint GetSourceLength(void) {return GetIntParam(GL_SHADER_SOURCE_LENGTH);}
    inline GLint GetInfoLogLength(void) {return GetIntParam(GL_INFO_LOG_LENGTH);}
    inline GLint GetCompileStatus(void) {return GetIntParam(GL_COMPILE_STATUS);}
    inline GLint GetDeleteStatus(void) {return GetIntParam(GL_DELETE_STATUS);}
    inline GLint GetShaderType(void) {return GetIntParam(GL_SHADER_TYPE);}
    inline UString GetSource(void) {return GetGLStringData(GetHandle(), GetSourceLength(), &glGetShaderSource);}
    inline UString GetInfoLog(void) {return GetGLStringData(GetHandle(), GetInfoLogLength(), &glGetShaderInfoLog);}

    bool IsCompiled(void);
};

//===========================================================================
class LBX_GL_EXPORT TGLVertexShader : public TGLShader
{
protected:
public:
    TGLVertexShader();
    TGLVertexShader(LBX_STREAM *strm);
    TGLVertexShader(const char *code, GLint length = -1);
    TGLVertexShader(const char *hdr, const char *body);
    virtual ~TGLVertexShader();
};

//===========================================================================
class LBX_GL_EXPORT TGLFragmentShader : public TGLShader
{
protected:
public:
    TGLFragmentShader();
    TGLFragmentShader(LBX_STREAM *strm);
    TGLFragmentShader(const char *code, GLint length = -1);
    TGLFragmentShader(const char *hdr, const char *body);
    virtual ~TGLFragmentShader();
};
//===========================================================================

class LBX_GL_EXPORT TGLBufferBase;

class LBX_GL_EXPORT TGLProgram : public TGLObject
{
protected:
    TGLShader * shaders[2];
    u32_t flags;
    i32_t GetIntParam(GLenum pname);

/*
    typedef struct {
        TGLAttributeType type;
        i16_t idx;
    } AttributeDescriptor;

    AttributeDescriptor * attrib_info;
    AttributeDescriptor * FindAttributeInfoByType(TGLAttributeType type, i16_t index = 0);
*/
    u8_t *attrib_types;
    void **attrib_data;
    void Analyze(void);

    enum TGLProgramFlags {
        pfLinked = 0x01u,
        pfOwnsFragmentShader = 0x02u,
        pfOwnsVertexShader = 0x04u,
    };

    void SetShader(i32_t index, TGLShader *shader); // 내부 목적으로만 사용
    TGLShader * CreateShader(GLenum type, const char *src, const char *hdr);

    size_t SaveToMem(void** p_svm, GLenum* bin_format = NULL);
public:
    TGLProgram();
    TGLProgram(const char *file_name, GLenum bin_format);
    TGLProgram(TGLVertexShader *v, TGLFragmentShader * f);
    TGLProgram(const char *vshader, const char *fshader, const char *hdr = NULL);
    TGLProgram(TGLVertexShader *vshader, const char *fshader, const char *hdr = NULL);
    TGLProgram(const char *vshader, TGLFragmentShader *fshader, const char *hdr = NULL);
    virtual ~TGLProgram();

    virtual GLuint GetHandle(void);

    void SetShader(TGLShader *shader);
    TGLFragmentShader * GetFragmentShader(void);
    TGLVertexShader * GetVertexShader(void);
    void SetVertexShader(TGLVertexShader *shader);
    void SetFragmentShader(TGLFragmentShader *shader);
    TGLVertexShader * CreateVertexShader(const char *src = NULL, const char *hdr = NULL);
    TGLFragmentShader * CreateFragmentShader(const char *src = NULL, const char *hdr = NULL);

    inline GLint GetLinkStatus(void) {return GetIntParam(GL_LINK_STATUS);}
    inline GLint GetInfoLogLength(void) {return GetIntParam(GL_INFO_LOG_LENGTH);}
    inline UString GetInfoLog(void) {return GetGLStringData(GetHandle(), GetInfoLogLength(), &glGetProgramInfoLog);}
    #ifdef _DEBUG
    GLint GetAttribLocation(const char *name);
    #else // #ifdef _DEBUG
    inline GLint GetAttribLocation(const char *name) {return glGetAttribLocation(GetHandle(), name);}
    #endif //#else #ifdef _DEBUG
    inline GLint GetUniformLocation(const char *name) {return glGetUniformLocation(GetHandle(), name);}

//	AttributeDescriptor GetAttributeInfoByType(TGLAttributeType type, i16_t num = 0);
    i32_t LocationOfAttribType(u8_t type);
    void SetAttributeType(i32_t location, u8_t type);
    inline void SetAttributeType(const char *attribute_name, u8_t type) { SetAttributeType(GetAttribLocation(attribute_name), type); }


    // Attribute의 동작 모드 변경(단일값 또는 배열형)
    void EnableArrayMode(i32_t location = -1);
    void DisableArrayMode(i32_t location = -1);
    inline void EnableArrayMode(const char *attribute_name) {EnableArrayMode(attribute_name == NULL ? -1 : GetAttribLocation(attribute_name));}
    inline void DisableArrayMode(const char *attribute_name) {DisableArrayMode(attribute_name == NULL ? -1 : GetAttribLocation(attribute_name));}

    i32_t Link(void);
    i32_t Build(const char *vshader, const char *fshader, const char *hdr = NULL);
    i32_t Build(TGLVertexShader *vshader, const char *fshader, const char *hdr = NULL);
    i32_t Build(const char *vshader, TGLFragmentShader *fshader, const char *hdr = NULL);

    i64_t LoadFromFile(const char *file_name, GLenum bin_format);
    i64_t LoadFromStream(LBX_STREAM *s, size_t size, GLenum bin_format);

    i64_t LoadFromFile(const char* file_name);
    i64_t LoadFromStream(LBX_STREAM* s);

    i64_t SaveToFile(const char *file_name, GLenum* bin_format = NULL);
    i64_t SaveToStream(LBX_STREAM *s, GLenum* bin_format = NULL);

    i32_t LoadBinary(const void *data, size_t size, GLenum bin_format);
    inline i32_t GetBinarySize(void) {
#ifdef GL_PROGRAM_BINARY_LENGTH_OES
        return GetIntParam(GL_PROGRAM_BINARY_LENGTH_OES);
#else //#ifdef GL_PROGRAM_BINARY_LENGTH_OES
        return 0;
#endif //#else #ifdef GL_PROGRAM_BINARY_LENGTH_OES
    }
    i32_t SaveBinary(void *dst, i32_t dst_size, GLenum *bin_format = NULL);

    inline void Use(void) {glUseProgram(GetHandle());}

    i32_t Uniform(GLint uniform_loc, LBX_TYPE type, const void *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const float *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const vec2_f32 *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const vec3_f32 *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const vec4_f32 *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const i32_t *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const vec2_i32 *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const vec3_i32 *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const vec4_i32 *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const mat2_f32 *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const mat3_f32 *data, i32_t count = 1);
    i32_t Uniform(GLint uniform_loc, const mat4_f32 *data, i32_t count = 1);

    inline i32_t Uniform(const char *name, const float *data, i32_t count = 1) {return Uniform(GetUniformLocation(name), data, count);}
    inline i32_t Uniform(const char *name, const vec2_f32 *data, i32_t count = 1) {return Uniform(GetUniformLocation(name), data, count);}
    inline i32_t Uniform(const char *name, const vec3_f32 *data, i32_t count = 1) {return Uniform(GetUniformLocation(name), data, count);}
    inline i32_t Uniform(const char *name, const vec4_f32 *data, i32_t count = 1) {return Uniform(GetUniformLocation(name), data, count);}

    inline i32_t Uniform(const char *name, const mat4_f32 *data, i32_t count = 1) {return Uniform(GetUniformLocation(name), data, count);}
    inline i32_t Uniform(const char *name, const mat4_t *data, i32_t count = 1) {return Uniform(GetUniformLocation(name), (mat4_f32*)data, count);}
    inline i32_t Uniform(const char *name, i32_t value) {return Uniform(GetUniformLocation(name), &value, 1);}

    //i32_t Attribute(const TGLBufferBase *vbo);
    //i32_t Attribute(void * data, i32_t count, LBX_TYPE type);

    void Attribute(GLint attrib_loc, GLint components, GLenum component_type, GLboolean normalize, i32_t stride, const void *offset);
    inline void Attribute(const char *name, GLint components, GLenum component_type, GLboolean normalize, i32_t stride, const void *offset) {Attribute(GetAttribLocation(name), components, component_type, normalize, stride, offset);}

    // Generic 타입으로 사용하는 경우
    inline void Attribute(GLint attrib_loc, const f32_t    *data, i32_t stride = 0) {Attribute(attrib_loc, 1, GL_FLOAT, GL_FALSE, stride, data);}
    inline void Attribute(GLint attrib_loc, const vec2_f32 *data, i32_t stride = 0) {Attribute(attrib_loc, 2, GL_FLOAT, GL_FALSE, stride, data);}
    inline void Attribute(GLint attrib_loc, const vec3_f32 *data, i32_t stride = 0) {Attribute(attrib_loc, 3, GL_FLOAT, GL_FALSE, stride, data);}
    inline void Attribute(GLint attrib_loc, const vec4_f32 *data, i32_t stride = 0) {Attribute(attrib_loc, 4, GL_FLOAT, GL_FALSE, stride, data);}
    inline void Attribute(GLint attrib_loc, const vec4_u8  *data, i32_t stride = 0) {Attribute(attrib_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, data);}
    inline void Attribute(const char *name, const f32_t    *data, i32_t stride = 0) {Attribute(GetAttribLocation(name), data, stride);}
    inline void Attribute(const char *name, const vec2_f32 *data, i32_t stride = 0) {Attribute(GetAttribLocation(name), data, stride);}
    inline void Attribute(const char *name, const vec3_f32 *data, i32_t stride = 0) {Attribute(GetAttribLocation(name), data, stride);}
    inline void Attribute(const char *name, const vec4_f32 *data, i32_t stride = 0) {Attribute(GetAttribLocation(name), data, stride);}
    inline void Attribute(const char *name, const vec4_u8  *data, i32_t stride = 0) {Attribute(GetAttribLocation(name), data, stride);}

    // 고정값으로 지정하는 경우
    inline void Attribute(GLint attrib_loc, f32_t    data) {glDisableVertexAttribArray(attrib_loc); glVertexAttrib1fv(attrib_loc, (float*)&data);}
    inline void Attribute(GLint attrib_loc, vec2_f32 data) {glDisableVertexAttribArray(attrib_loc); glVertexAttrib2fv(attrib_loc, (float*)&data);}
    inline void Attribute(GLint attrib_loc, vec3_f32 data) {glDisableVertexAttribArray(attrib_loc); glVertexAttrib3fv(attrib_loc, (float*)&data);}
    inline void Attribute(GLint attrib_loc, vec4_f32 data) {glDisableVertexAttribArray(attrib_loc); glVertexAttrib4fv(attrib_loc, (float*)&data);}
    inline void Attribute(const char *name, f32_t    data) {Attribute(GetAttribLocation(name), data);}
    inline void Attribute(const char *name, vec2_f32 data) {Attribute(GetAttribLocation(name), data);}
    inline void Attribute(const char *name, vec3_f32 data) {Attribute(GetAttribLocation(name), data);}
    inline void Attribute(const char *name, vec4_f32 data) {Attribute(GetAttribLocation(name), data);}
    inline void Attribute(const char *name, vec4_u8  data) {Attribute(GetAttribLocation(name), data);}
    void Attribute(GLint attrib_loc, vec4_u8 data);

    bool IsLinked(void);
};

//===========================================================================

class LBX_GL_EXPORT TLBTexture : public TGLObject
{
private:
    typedef TGLObject inherited;
protected:
    i64_t LoadFromFile(const char *file_name, i32_t target = -1);
    i64_t LoadFromStream(LBX_STREAM *s, i32_t target = -1);

    size_t Load(LBX_IMAGE *img);

public:
    TLBTexture();
    virtual ~TLBTexture();
    virtual i32_t SetImage(const LBX_IMAGE *img, i32_t target = -1) = 0;

    LBX_IMAGE image_format;
    virtual GLuint GetHandle(void);
};

class LBX_GL_EXPORT TGLTexture3D : public TLBTexture
{
private:
    typedef TLBTexture inherited;
protected:
public:
    TGLTexture3D();
    virtual ~TGLTexture3D();

    virtual i32_t SetImage(const LBX_IMAGE *img, i32_t target);
    inline i64_t LoadFromFile(const char *file_name, i32_t target) {return inherited::LoadFromFile(file_name, target);}
    inline i64_t LoadFromStream(LBX_STREAM *s, i32_t target) {return inherited::LoadFromStream(s, target);}

    inline void Bind(void) {glBindTexture(GL_TEXTURE_CUBE_MAP, GetHandle());}

};

class LBX_GL_EXPORT TGLTexture2D : public TLBTexture
{
private:
    typedef TLBTexture inherited;
protected:
public:
    TGLTexture2D();
    virtual ~TGLTexture2D();

    virtual i32_t SetImage(const LBX_IMAGE *img, i32_t target = -1);
    inline i64_t LoadFromFile(const char *file_name) {return inherited::LoadFromFile(file_name, GL_TEXTURE_2D);}
    inline i64_t LoadFromStream(LBX_STREAM *s) {return inherited::LoadFromStream(s, GL_TEXTURE_2D);}

    inline void Bind(void) {glBindTexture(GL_TEXTURE_2D, GetHandle());}
};


typedef struct {
    LBX_TYPE type;
    i32_t offset;
} LBX_GLBUFFER_MEMBER_INFO;


typedef struct {
    LBX_GLBUFFER_MEMBER_INFO vertex;
    LBX_GLBUFFER_MEMBER_INFO tex_coord;
    LBX_GLBUFFER_MEMBER_INFO color;
    i32_t stride;
} LBX_GLBUFFER_DESC;

class LBX_GL_EXPORT TGlyph
{
protected:
    rect_i16 scr_boundary;
    rect_f32 boundary;
    rect_f32 *fit;
    rect_f32 *border;
    vec2_f32 scale;
    u32_t flags;

    void Update(void);


    enum TGlyphFlags {
        gfHasHole = 0x01,
        gfHasAlign = 0x02,
        gfHasBorder = 0x04
    };
    vec2_f32 *tex_coord;
public:
    TGlyph(TGLTexture2D *tex);
    ~TGlyph();
    TGLTexture2D *texture;
    void SetBoundary(rect_i16 rect);
    inline void SetBoundary(xywh_i16 xywh) {SetBoundary(rect_i16_(xywh.x, xywh.y, xywh.x + xywh.width, xywh.y + xywh.height));}
    inline void SetBoundary(i32_t left, i32_t top, i32_t right, i32_t bottom) {SetBoundary(rect_i16_(left, top, right, bottom));}
    inline void SetBoundaryXYWH(i32_t x, i32_t y, i32_t width, i32_t height) {SetBoundary(rect_i16_(x, y, x+width, y+height));}
    void FitTo(rect_i16 rect);
    inline void FitTo(xywh_i16 xywh) {FitTo(rect_i16_(xywh.x, xywh.y, xywh.x + xywh.width, xywh.y + xywh.height));}
    inline void FitTo(i32_t left, i32_t top, i32_t right, i32_t bottom) {FitTo(rect_i16_(left, top, right, bottom));}
    inline void FitToXYWH(i32_t x, i32_t y, i32_t width, i32_t height) {FitTo(rect_i16_(x, y, x+width, y+height));}
    void SetBorder(rect_i16 rect, bool perforated = false);
    inline void SetBorder(xywh_f32 xywh, bool perforated = false) {SetBorder(rect_i16_((i16_t)(xywh.x), (i16_t)(xywh.y), (i16_t)(xywh.x + xywh.width), (i16_t)(xywh.y + xywh.height)), perforated);}
    inline void SetBorder(i32_t left, i32_t top, i32_t right, i32_t bottom, bool perforated = false) {SetBorder(rect_i16_(left, top, right, bottom), perforated);}
    inline void SetBorderXYWH(i32_t x, i32_t y, i32_t width, i32_t height, bool perforated = false) {SetBorder(rect_i16_(x, y, x+width, y+height), perforated);}
    void ClearBase(void);
    void ClearBorder(void);

    i32_t FillVertexList(vec2_f32 *target, rect_f32 area, i32_t target_stride = 0);

    i32_t BuildDrawList(void *buffer, const LBX_GLBUFFER_DESC *buffer_info, i16_t *indice, rect_f32 target);

    const u8_t *GetDrawIndice(i32_t *count = NULL);
    inline const vec2_f32 * GetTexCoords(void) {return tex_coord;}
    i32_t GetVertexCount(void);

    inline vec2_f32 GetScale(void) {return scale;}
    inline void SetScale(vec2_f32 value) {scale = value;}

    inline void Bind(void) {texture->Bind();}

    inline rect_f32 GetBoundary(void) const {return boundary;}
};

//===========================================================================
class LBX_GL_EXPORT TGLBufferBase : public TGLObject
{
protected:
    void * local_data;
    i32_t Upload(GLenum target, const void *data, i32_t size, GLenum usage);
    i32_t Upload(GLenum target, GLenum usage);
    i32_t GetIntParam(GLenum target, GLenum pname);
    void Bind(GLenum target);
public:
    TGLBufferBase();
    TGLBufferBase(GLenum target);
    virtual ~TGLBufferBase();
    virtual GLuint GetHandle(void);
    inline void * GetLocalData(void) {return local_data;}
    void * AppendLocalData(i32_t count, i32_t elem_size);
    void * InsertLocalData(i32_t index, i32_t count, i32_t elem_size);
    inline void ClearLocalData(void) {SVEC_FREE(&local_data);}
    inline i32_t GetElemSize(void) {return svec_elem_size(local_data);}
};

class LBX_GL_EXPORT TGLIndexBuffer : public TGLBufferBase
{
protected:

public:
    TGLIndexBuffer();
    virtual ~TGLIndexBuffer();
    inline void Bind(void) {TGLBufferBase::Bind(GL_ELEMENT_ARRAY_BUFFER);}
    inline i32_t Upload(const void *data, i32_t size, GLenum usage = GL_STATIC_DRAW) {return TGLBufferBase::Upload(GL_ELEMENT_ARRAY_BUFFER, data, size, usage);}
    inline i32_t Upload(GLenum usage = GL_STATIC_DRAW) {return TGLBufferBase::Upload(GL_ELEMENT_ARRAY_BUFFER, usage);}
    i32_t Size(void);
    inline GLenum Usage(void) {return GetIntParam(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_USAGE);}
    inline void Unbind(void) {glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);}
};


struct LBX_BUFFER_DESCRIPTOR {
    u8_t attrib_type;
    LBX_TYPE data_type;
    u16_t offset;
};

struct LBX_GL_TYPE {
    GLint components;
    GLenum component_type;
    GLboolean normalize;
};

LBX_GL_EXPORT LBX_GL_TYPE GetGLTypeInfo(LBX_TYPE type);


class LBX_GL_EXPORT TGLAttribBuffer : public TGLBufferBase
{
protected:
//	const LBX_REFL_STRUCT_INFO *ti;
    LBX_BUFFER_DESCRIPTOR *bd;
public:
    TGLAttribBuffer();
    TGLAttribBuffer(void *data, i32_t size, GLenum usage = GL_STATIC_DRAW);
    virtual ~TGLAttribBuffer();
    inline void Bind(void) {TGLBufferBase::Bind(GL_ARRAY_BUFFER);}
    inline i32_t Upload(const void *data, i32_t size, GLenum usage = GL_STATIC_DRAW) {return TGLBufferBase::Upload(GL_ARRAY_BUFFER, data, size, usage);}
    inline i32_t Upload(GLenum usage = GL_STATIC_DRAW) {return TGLBufferBase::Upload(GL_ARRAY_BUFFER, usage);}
    inline i32_t Size(void) {return GetIntParam(GL_ARRAY_BUFFER, GL_BUFFER_SIZE);}
    inline GLenum Usage(void) {return GetIntParam(GL_ARRAY_BUFFER, GL_BUFFER_USAGE);}

//	SetTypeInfo(const LBX_REFL_STRUCT_INFO *typeinfo);

    i32_t Register(const LBX_REFL_STRUCT_INFO *rtti);
    TGLAttribBuffer & Register(char attrib_type, LBX_TYPE data_type, u16_t offset);

    i32_t BindTo(TGLProgram *program, i32_t elem_size);
    inline void Unbind(void) {glBindBuffer(GL_ARRAY_BUFFER, 0);}
};



struct LBX_BUFFER_INFO {
    GLuint buffer_id;
    u8_t *data;
    i32_t stride;
};

struct LBX_ATTRIB_BINDING {
    GLint attrib_loc;
    GLint components;
    GLenum component_type;
    GLboolean normalize;
    i32_t offset;
};


/*
struct LBX_UNIFORM_BINDING {
    GLint uniform_loc;
    LBX_TYPE type;
    void *data;
};
*/

/*
struct LBX_ATTRIB_BIND_INFO {
    GLuint buffer_id; // 0이면 local buffer를 사용한다는 의미
    GLint attrib_loc;
    GLint components;
    GLenum component_type;
    GLboolean normalize;
    GLint stride;
    void * pointer;

    LBX_ATTRIB_BIND_INFO();
    LBX_ATTRIB_BIND_INFO(LBX_TYPE type, i32_t stride = 0);
    void SetType(LBX_TYPE type);
    void SetOffset(GLuint buffer_id, i32_t offset);
    void SetPointer(void *address, i32_t offset);
};
*/

class LBX_GL_EXPORT TGLAttribBinder {
protected:
    TGLProgram *p;
    LBX_BUFFER_INFO **bi;  // [LBX_BUFFER_INFO *] [LBX_ATTRIB_BINDING] [LBX_ATTRIB_BINDING] ...
    inline i32_t GetBufferSubCount(LBX_BUFFER_INFO *buffer_info) {
        return (buffer_info == NULL) ? 0 : (i32_t)((svec_length(buffer_info) - sizeof(LBX_BUFFER_INFO)) / sizeof(LBX_ATTRIB_BINDING));
    }
    inline LBX_ATTRIB_BINDING * GetBufferSub(LBX_BUFFER_INFO *buffer_info) {
        return (buffer_info == NULL) ? NULL : (LBX_ATTRIB_BINDING *)(buffer_info + 1);
    }
    LBX_BUFFER_INFO * add_buffer(void);
    LBX_BUFFER_INFO ** find_buffer(LBX_BUFFER_INFO*);


public:
    TGLAttribBinder();
    TGLAttribBinder(TGLProgram *program, TGLAttribBuffer *buffer);
    TGLAttribBinder(TGLProgram *program, const LBX_REFL_STRUCT_INFO *typeinfo);
    ~TGLAttribBinder();

    inline i32_t GetBufferInfoCount(void) {return svec_len32(bi);}

    void SetProgram(TGLProgram * program);
    inline TGLProgram * GetProgram(void) {return p;}

    LBX_BUFFER_INFO * SetBuffer(GLint buffer_id, i32_t stride);
    LBX_BUFFER_INFO * SetBuffer(void * buffer_addr, i32_t stride);
    LBX_ATTRIB_BINDING * AddBinding(LBX_BUFFER_INFO *buffer_info = NULL);
    LBX_ATTRIB_BINDING * AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLint offset, LBX_BUFFER_INFO * buffer_info = NULL);
    LBX_ATTRIB_BINDING * AddBinding(GLint attrib_loc, LBX_TYPE type, GLint offset, LBX_BUFFER_INFO * buffer_info = NULL);
    inline LBX_ATTRIB_BINDING * AddBinding(const char *attrib_name, LBX_TYPE type, GLint offset, LBX_BUFFER_INFO * buffer_info = NULL) {
        return AddBinding(p->GetAttribLocation(attrib_name), type, offset, buffer_info);
    }
    inline LBX_ATTRIB_BINDING * AddBinding(const char *attrib_name, GLint comp_count, GLenum comp_type, GLint offset, LBX_BUFFER_INFO * buffer_info = NULL) {
        return AddBinding(p->GetAttribLocation(attrib_name), comp_count, comp_type, offset, buffer_info);
    }

    i32_t AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLboolean normalize, GLint stride, GLint buffer_id, GLint offset);
    i32_t AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLboolean normalize, GLint stride, void * pointer);
    i32_t AddBinding(GLint attrib_loc, const LBX_REFL_STRUCT_INFO *ti, const char *member_name, GLboolean normalize);
    i32_t AddBinding(const LBX_BUFFER_INFO *bind_info);


    i32_t AddUniform(GLint uniform_loc, LBX_TYPE type, void *data);


    //void glVertexAttribPointer(GLuint index,  GLint size,  GLenum type,  GLboolean normalized,  GLsizei stride,  const GLvoid * pointer);
//	i32_t AddBinding(const char *attrib_name, GLenum comp_type, GLint comp_count, GLint stride, GLint offset, GLboolean normalize = false) {
//		return AddBinding(p->GetAttribLocation(attrib_name), comp_type, comp_count, stride, offset, normalize);
//	}

    inline i32_t AddBinding(const char *attrib_name, const LBX_REFL_STRUCT_INFO *ti, const char *member_name, GLboolean normalize = false) {
        return AddBinding(p->GetAttribLocation(attrib_name), ti, member_name, normalize);
    }
//	inline i32_t AddBinding_vec4_f32(const char *attrib_name, GLint stride = sizeof(vec4_f32), GLint offset = 0, GLboolean normalize = false) {
//		return AddBinding(attrib_name, GL_FLOAT, 4, stride, offset, normalize);
//	}
//	inline i32_t AddBinding_vec3_f32(const char *attrib_name, GLint stride = sizeof(vec3_f32), GLint offset = 0, GLboolean normalize = false) {
//		return AddBinding(attrib_name, GL_FLOAT, 3, stride, offset, normalize);
//	}
//	inline i32_t AddBinding_vec2_f32(const char *attrib_name, GLint stride = sizeof(vec2_f32), GLint offset = 0, GLboolean normalize = false) {
//		return AddBinding(attrib_name, GL_FLOAT, 2, stride, offset, normalize);
//	}
//	inline i32_t AddBinding_vec4u32(const char *attrib_name, GLint stride = sizeof(vec4u32), GLint offset = 0, GLboolean normalize = true) {
//		return AddBinding(attrib_name, GL_UNSIGNED_BYTE, 4, stride, offset, normalize);
//	}

    void Clear(void);

    i32_t Enable(void);
    i32_t Disable(void);
};


struct LBX_RENDER_COMMAND {
    GLenum command;
    GLuint buffer_id;
    GLenum type;
    void *indice;
    i32_t first;
    i32_t count;
    bool *visible;
};

class LBX_GL_EXPORT TGLCommandList {
protected:
    LBX_RENDER_COMMAND *list;
public:
    TGLCommandList();
    ~TGLCommandList();

    TGLAttribBinder *ab;

    LBX_RENDER_COMMAND * Add(GLenum method, bool *visible = NULL);
    LBX_RENDER_COMMAND * Add(GLenum method, LBX_TYPE type, i32_t count, bool *visible = NULL);
    LBX_RENDER_COMMAND * Add(GLenum method, GLuint indice_buffer_id, GLenum elem_type, i32_t count, bool *visible = NULL);
    LBX_RENDER_COMMAND * Add(GLenum method, const u16_t *indice, i32_t count, bool *visible = NULL);
    LBX_RENDER_COMMAND * Add(GLenum method, const u8_t *indice, i32_t count, bool *visible = NULL);

    LBX_RENDER_COMMAND * AddArrays(GLenum method, i32_t first, i32_t count, bool *visible = NULL);

    void Clear(void);

    inline i32_t Count(void) {return svec_len32(list);}
    inline LBX_RENDER_COMMAND * Items(i32_t index) {return list + adjust_index(index, svec_length(list));}

    TGLProgram * GetProgram(void);
};

LBX_GL_EXPORT i32_t DrawCommandList(TGLCommandList *list);

typedef struct {
    vec2_f32 vtx;
    vec2_f32 txc;
} V2T;

typedef struct {
    vec3_f32 vtx;
    vec2_f32 txc;
} V3T;

typedef struct {
    vec4_f32 vtx;
    vec2_f32 txc;
} V4T;

typedef struct {
    vec2_f32 vtx;
    u32_t col;
    vec2_f32 txc;
} V2CT;

typedef struct {
    vec3_f32 vtx;
    u32_t col;
    vec2_f32 txc;
} V3CT;

typedef struct {
    vec4_f32 vtx;
    u32_t col;
    vec2_f32 txc;
} V4CT;

typedef struct {
    vec2_f32 vtx;
    f32_t col;
    vec2_f32 txc;
} V212;

typedef struct {
    vec3_f32 vtx;
    f32_t col;
    vec2_f32 txc;
} V312;

typedef struct {
    vec4_f32 vtx;
    f32_t col;
    vec2_f32 txc;
} V412;


typedef struct {
    vec2_f32 vtx;
    vec3_f32 col;
    vec2_f32 txc;
} V232;

typedef struct {
    vec3_f32 vtx;
    vec3_f32 col;
    vec2_f32 txc;
} V332;

typedef struct {
    vec4_f32 vtx;
    vec3_f32 col;
    vec2_f32 txc;
} V432;

typedef struct {
    vec2_f32 vtx;
    vec4_f32 col;
    vec2_f32 txc;
} V242;


typedef struct {
    vec3_f32 vtx;
    vec4_f32 col;
    vec2_f32 txc;
} V342;

typedef struct {
    vec4_f32 vtx;
    vec4_f32 col;
    vec2_f32 txc;
} V442;


class LBX_GL_EXPORT TGLDrawList
{
protected:
    TGLAttribBinder ab; // attribute binder
    bool modified;
public:
    TGLDrawList(TGLProgram *program);
    ~TGLDrawList();

    TGLAttribBuffer bf; // buffer
    TGLCommandList  cl; // command list

    LBX_ATTRIB_BINDING * SetBinding(const char *attrib_name, LBX_TYPE type, GLint offset);

    void FillBuffer(V2CT *dst, TGlyph *glyph, rect_f32 area, u32_t color);

    i32_t AddGlyph(TGlyph *glyph, rect_f32 area, u32_t color);
    inline i32_t AddGlyph(TGlyph *glyph, xywh_f32 area, u32_t color) {
            return AddGlyph(glyph, rect_f32_(area.x, area.y, area.x + area.width, area.y + area.height), color
        );}
    i32_t AddLines(vec2_f32 *lines_vertice, i32_t line_count, u32_t color);

    i32_t Draw(void);
    void ClearCommandList(void);
//    inline void Touch(void) {modified = true;}
};




/*
class TGLCommandList2 {
protected:
public:
    void ChangeProgram(const char *program_name);
    void SetBuffer(void);
    void AddPolyLine

};
*/



template <class T>
class TGLBuffer : public TGLBufferBase
{
public:
    TGLBuffer();
    ~TGLBuffer();
    T * data;
    inline i32_t GetElemSize(void) {return (i32_t)sizeof(T);}
};

template <class T>
TGLBuffer<T>::TGLBuffer()
    : TGLBufferBase(), data(NULL)
{
}

template <class T>
TGLBuffer<T>::~TGLBuffer()
{
    svec_free((void**)&data);
}


class LBX_GL_EXPORT TGLFrameBufferObject : public TGLObject
{
private:
    typedef TGLObject inherited;
    enum {
        owns_texture = 1
    };
protected:
    u32_t flags;
    TGLTexture2D *tex;
//	GLuint tex;
    GLuint depth;
    size2_i16 sz;
public:
    TGLFrameBufferObject();
    TGLFrameBufferObject(i32_t width, i32_t height);
    virtual ~TGLFrameBufferObject();
    i32_t SetSize(i32_t width, i32_t height);
    inline i32_t SetSize(size2_i16 size) { return SetSize(size.width, size.height); }
    void Bind(void);
    void Release(void);
//	inline GLuint GetTexture() {return tex;}
    i32_t SetTexture(GLuint texture_handle);

    virtual GLuint GetHandle(void);
    inline GLuint GetTexHandle(void) {return tex->GetHandle();}
    inline size2_i16 GetSize(void) {return sz;}
};


#endif
