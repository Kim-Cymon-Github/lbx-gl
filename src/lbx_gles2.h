//---------------------------------------------------------------------------

#ifndef lbx_gles2H
#define lbx_gles2H
//---------------------------------------------------------------------------
#include "lbx_stream.h"
#include "image/lbx_image.h"
#include "lbx_ustr.h"
#include "lbx_refl.h"
#include "lbx_gl.h"

typedef void (GL_APIENTRY * TGLGetStringFunc)(GLuint handle,  GLsizei bufSize,  GLsizei *length,  GLchar *data);
extern UString GetGLStringData(GLuint obj, int length, TGLGetStringFunc func);

// Attrib Types
#define LB_UNKNOWN     ( 0u)
#define LB_POSITION    (10u)
#define LB_NORMAL      (20u)
#define LB_TANGENT     (30u)
#define LB_TEXCOORD    (40u)
#define LB_TEXCOORD_0  (40u)
#define LB_TEXCOORD_1  (41u)
#define LB_TEXCOORD_2  (42u)
#define LB_TEXCOORD_3  (43u)
#define LB_TEXCOORD_4  (44u)
#define LB_TEXCOORD_5  (45u)
#define LB_TEXCOORD_6  (46u)
#define LB_TEXCOORD_7  (47u)
#define LB_TEXCOORD_8  (48u)
#define LB_TEXCOORD_9  (49u)
#define LB_COLOR       (50u)
#define LB_COLOR_0     (50u)
#define LB_COLOR_1     (51u)
#define LB_COLOR_2     (52u)
#define LB_COLOR_3     (53u)
#define LB_COLOR_4     (54u)
#define LB_COLOR_5     (55u)
#define LB_COLOR_6     (56u)
#define LB_COLOR_7     (57u)
#define LB_COLOR_8     (58u)
#define LB_COLOR_9     (59u)
#define LB_JOINTS      (60u)
#define LB_JOINTS_0    (60u)
#define LB_JOINTS_1    (61u)
#define LB_JOINTS_2    (62u)
#define LB_JOINTS_3    (63u)
#define LB_JOINTS_4    (64u)
#define LB_JOINTS_5    (65u)
#define LB_JOINTS_6    (66u)
#define LB_JOINTS_7    (67u)
#define LB_JOINTS_8    (68u)
#define LB_JOINTS_9    (69u)
#define LB_WEIGHTS     (70u)
#define LB_WEIGHTS_0   (70u)
#define LB_WEIGHTS_1   (71u)
#define LB_WEIGHTS_2   (72u)
#define LB_WEIGHTS_3   (73u)
#define LB_WEIGHTS_4   (74u)
#define LB_WEIGHTS_5   (75u)
#define LB_WEIGHTS_6   (76u)
#define LB_WEIGHTS_7   (77u)
#define LB_WEIGHTS_8   (78u)
#define LB_WEIGHTS_9   (79u)

uint8_t EstimateAttribTypeFromName(const char *name, int l = -1);

//===========================================================================
class TGLObject
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
class TGLShader : public TGLObject
{
private:
    uint32_t flags;
protected:
    void CreateGLObject(GLenum type);
    int GetIntParam(GLenum pname);

public:
    enum TGLShaderFlags {
        sfCompiled = 1,
    };
    TGLShader(GLenum type);
    TGLShader(GLenum type, lbxSTREAM *strm);
    TGLShader(GLenum type, const char *code, const char *hdr = NULL);
    TGLShader(GLenum type, const char *code, int code_len, const char *hdr = NULL, int hdr_len = -1);
    virtual ~TGLShader();
    int SetSource(const char *code, const char *hdr = NULL);
    int SetSource(const char *code, int code_len, const char *hdr = NULL, int hdr_len = -1);
    int SetSource(GLint count, const char **codes, const GLint *lengths = NULL);
    int SetSource(lbxSTREAM *strm);
    int SetBinary(lbxSTREAM *strm);
    int Load(const void *data, GLsizei length, GLenum binaryformat);

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
class TGLVertexShader : public TGLShader
{
protected:
public:
    TGLVertexShader();
    TGLVertexShader(lbxSTREAM *strm);
    TGLVertexShader(const char *code, GLint length = -1);
    TGLVertexShader(const char *hdr, const char *body);
    virtual ~TGLVertexShader();
};

//===========================================================================
class TGLFragmentShader : public TGLShader
{
protected:
public:
    TGLFragmentShader();
    TGLFragmentShader(lbxSTREAM *strm);
    TGLFragmentShader(const char *code, GLint length = -1);
    TGLFragmentShader(const char *hdr, const char *body);
    virtual ~TGLFragmentShader();
};
//===========================================================================

class TGLBufferBase;

class TGLProgram : public TGLObject
{
protected:
    TGLShader * shaders[2];
    uint32_t flags;
    int GetIntParam(GLenum pname);

/*
    typedef struct {
        TGLAttributeType type;
        int16_t idx;
    } AttributeDescriptor;

    AttributeDescriptor * attrib_info;
    AttributeDescriptor * FindAttributeInfoByType(TGLAttributeType type, int16_t index = 0);
*/
    uint8_t *attrib_types;
    void **attrib_data;
    void Analyze(void);

    enum TGLProgramFlags {
        pfLinked = 0x01u,
        pfOwnsFragmentShader = 0x02u,
        pfOwnsVertexShader = 0x04u,
    };

    void SetShader(int index, TGLShader *shader); // 내부 목적으로만 사용
    TGLShader * CreateShader(GLenum type, const char *src, const char *hdr);
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

//	AttributeDescriptor GetAttributeInfoByType(TGLAttributeType type, int16_t num = 0);
    int LocationOfAttribType(uint8_t type);
    void SetAttributeType(int location, uint8_t type);
    inline void SetAttributeType(const char *attribute_name, uint8_t type) { SetAttributeType(GetAttribLocation(attribute_name), type); }


    // Attribute의 동작 모드 변경(단일값 또는 배열형)
    void EnableArrayMode(int location = -1);
    void DisableArrayMode(int location = -1);
    inline void EnableArrayMode(const char *attribute_name) {EnableArrayMode(attribute_name == NULL ? -1 : GetAttribLocation(attribute_name));}
    inline void DisableArrayMode(const char *attribute_name) {DisableArrayMode(attribute_name == NULL ? -1 : GetAttribLocation(attribute_name));}

    int Link(void);
    int Build(const char *vshader, const char *fshader, const char *hdr = NULL);
    int Build(TGLVertexShader *vshader, const char *fshader, const char *hdr = NULL);
    int Build(const char *vshader, TGLFragmentShader *fshader, const char *hdr = NULL);

    int LoadFromFile(const char *file_name, GLenum bin_format);
    int LoadFromStream(lbxSTREAM *s, int length, GLenum bin_format);
    int SaveToFile(const char *file_name);
    int SaveToStream(lbxSTREAM *s);
    int SaveToMem(void **rcm);

    int LoadBinary(const void *data, int length, GLenum bin_format);
    inline int GetBinarySize(void) {
#ifdef GL_PROGRAM_BINARY_LENGTH_OES
        return GetIntParam(GL_PROGRAM_BINARY_LENGTH_OES);
#else //#ifdef GL_PROGRAM_BINARY_LENGTH_OES
        return 0;
#endif //#else #ifdef GL_PROGRAM_BINARY_LENGTH_OES
    }
    int SaveBinary(void *dst, int dst_size, GLenum *bin_format = NULL);

    inline void Use(void) {glUseProgram(GetHandle());}

    int Uniform(GLint uniform_loc, LB_TYPE type, const void *data, int count = 1);
    int Uniform(GLint uniform_loc, const float *data, int count = 1);
    int Uniform(GLint uniform_loc, const vec2_f32 *data, int count = 1);
    int Uniform(GLint uniform_loc, const vec3_f32 *data, int count = 1);
    int Uniform(GLint uniform_loc, const vec4_f32 *data, int count = 1);
    int Uniform(GLint uniform_loc, const int *data, int count = 1);
    int Uniform(GLint uniform_loc, const vec2_i32 *data, int count = 1);
    int Uniform(GLint uniform_loc, const vec3_i32 *data, int count = 1);
    int Uniform(GLint uniform_loc, const vec4_i32 *data, int count = 1);
    int Uniform(GLint uniform_loc, const mat2_f32 *data, int count = 1);
    int Uniform(GLint uniform_loc, const mat3_f32 *data, int count = 1);
    int Uniform(GLint uniform_loc, const mat4_f32 *data, int count = 1);

    inline int Uniform(const char *name, const float *data, int count = 1) {return Uniform(GetUniformLocation(name), data, count);}
    inline int Uniform(const char *name, const vec2_f32 *data, int count = 1) {return Uniform(GetUniformLocation(name), data, count);}
    inline int Uniform(const char *name, const vec3_f32 *data, int count = 1) {return Uniform(GetUniformLocation(name), data, count);}
    inline int Uniform(const char *name, const vec4_f32 *data, int count = 1) {return Uniform(GetUniformLocation(name), data, count);}

    inline int Uniform(const char *name, const mat4_f32 *data, int count = 1) {return Uniform(GetUniformLocation(name), data, count);}
    inline int Uniform(const char *name, const mat4_t *data, int count = 1) {return Uniform(GetUniformLocation(name), (mat4_f32*)data, count);}
    inline int Uniform(const char *name, int value) {return Uniform(GetUniformLocation(name), &value, 1);}

    //int Attribute(const TGLBufferBase *vbo);
    //int Attribute(void * data, int count, LB_TYPE type);

    void Attribute(GLint attrib_loc, GLint components, GLenum component_type, GLboolean normalize, int stride, const void *offset);
    inline void Attribute(const char *name, GLint components, GLenum component_type, GLboolean normalize, int stride, const void *offset) {Attribute(GetAttribLocation(name), components, component_type, normalize, stride, offset);}

    // Generic 타입으로 사용하는 경우
    inline void Attribute(GLint attrib_loc, const f32_t    *data, int stride = 0) {Attribute(attrib_loc, 1, GL_FLOAT, GL_FALSE, stride, data);}
    inline void Attribute(GLint attrib_loc, const vec2_f32 *data, int stride = 0) {Attribute(attrib_loc, 2, GL_FLOAT, GL_FALSE, stride, data);}
    inline void Attribute(GLint attrib_loc, const vec3_f32 *data, int stride = 0) {Attribute(attrib_loc, 3, GL_FLOAT, GL_FALSE, stride, data);}
    inline void Attribute(GLint attrib_loc, const vec4_f32 *data, int stride = 0) {Attribute(attrib_loc, 4, GL_FLOAT, GL_FALSE, stride, data);}
    inline void Attribute(GLint attrib_loc, const vec4_u8  *data, int stride = 0) {Attribute(attrib_loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, data);}
    inline void Attribute(const char *name, const f32_t    *data, int stride = 0) {Attribute(GetAttribLocation(name), data, stride);}
    inline void Attribute(const char *name, const vec2_f32 *data, int stride = 0) {Attribute(GetAttribLocation(name), data, stride);}
    inline void Attribute(const char *name, const vec3_f32 *data, int stride = 0) {Attribute(GetAttribLocation(name), data, stride);}
    inline void Attribute(const char *name, const vec4_f32 *data, int stride = 0) {Attribute(GetAttribLocation(name), data, stride);}
    inline void Attribute(const char *name, const vec4_u8  *data, int stride = 0) {Attribute(GetAttribLocation(name), data, stride);}

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

class TLBTexture : public TGLObject
{
private:
    typedef TGLObject inherited;
protected:
    int LoadFromFile(const char *file_name, int target = -1);
    int LoadFromStream(lbxSTREAM *s, int target = -1);

    int Load(lbxIMAGE *img);

public:
    TLBTexture();
    virtual ~TLBTexture();
    virtual int SetImage(const lbxIMAGE *img, int target = -1) = 0;

    lbxIMAGE image_format;
    virtual GLuint GetHandle(void);
};

class TGLTexture3D : public TLBTexture
{
private:
    typedef TLBTexture inherited;
protected:
public:
    TGLTexture3D();
    virtual ~TGLTexture3D();

    virtual int SetImage(const lbxIMAGE *img, int target);
    inline int LoadFromFile(const char *file_name, int target) {return inherited::LoadFromFile(file_name, target);}
    inline int LoadFromStream(lbxSTREAM *s, int target) {return inherited::LoadFromStream(s, target);}

    inline void Bind(void) {glBindTexture(GL_TEXTURE_CUBE_MAP, GetHandle());}

};

class TGLTexture2D : public TLBTexture
{
private:
    typedef TLBTexture inherited;
protected:
public:
    TGLTexture2D();
    virtual ~TGLTexture2D();

    virtual int SetImage(const lbxIMAGE *img, int target = -1);
    inline int LoadFromFile(const char *file_name) {return inherited::LoadFromFile(file_name, GL_TEXTURE_2D);}
    inline int LoadFromStream(lbxSTREAM *s) {return inherited::LoadFromStream(s, GL_TEXTURE_2D);}

    inline void Bind(void) {glBindTexture(GL_TEXTURE_2D, GetHandle());}
};


typedef struct {
    LB_TYPE type;
    int offset;
} LB_GLBUFFER_MEMBER_INFO;


typedef struct {
    LB_GLBUFFER_MEMBER_INFO vertex;
    LB_GLBUFFER_MEMBER_INFO tex_coord;
    LB_GLBUFFER_MEMBER_INFO color;
    int stride;
} LB_GLBUFFER_DESC;

class TGlyph
{
protected:
    rect_i16 scr_boundary;
    rect_f32 boundary;
    rect_f32 *fit;
    rect_f32 *border;
    vec2_f32 scale;
    uint32_t flags;

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
    inline void SetBoundary(int left, int top, int right, int bottom) {SetBoundary(rect_i16_(left, top, right, bottom));}
    inline void SetBoundaryXYWH(int x, int y, int width, int height) {SetBoundary(rect_i16_(x, y, x+width, y+height));}
    void FitTo(rect_i16 rect);
    inline void FitTo(xywh_i16 xywh) {FitTo(rect_i16_(xywh.x, xywh.y, xywh.x + xywh.width, xywh.y + xywh.height));}
    inline void FitTo(int left, int top, int right, int bottom) {FitTo(rect_i16_(left, top, right, bottom));}
    inline void FitToXYWH(int x, int y, int width, int height) {FitTo(rect_i16_(x, y, x+width, y+height));}
    void SetBorder(rect_i16 rect, bool perforated = false);
    inline void SetBorder(xywh_f32 xywh, bool perforated = false) {SetBorder(rect_i16_((int16_t)(xywh.x), (int16_t)(xywh.y), (int16_t)(xywh.x + xywh.width), (int16_t)(xywh.y + xywh.height)), perforated);}
    inline void SetBorder(int left, int top, int right, int bottom, bool perforated = false) {SetBorder(rect_i16_(left, top, right, bottom), perforated);}
    inline void SetBorderXYWH(int x, int y, int width, int height, bool perforated = false) {SetBorder(rect_i16_(x, y, x+width, y+height), perforated);}
    void ClearBase(void);
    void ClearBorder(void);

    int FillVertexList(vec2_f32 *target, rect_f32 area, int target_stride = 0);

    int BuildDrawList(void *buffer, const LB_GLBUFFER_DESC *buffer_info, int16_t *indice, rect_f32 target);

    const uint8_t *GetDrawIndice(int *count = NULL);
    inline const vec2_f32 * GetTexCoords(void) {return tex_coord;}
    int GetVertexCount(void);

    inline vec2_f32 GetScale(void) {return scale;}
    inline void SetScale(vec2_f32 value) {scale = value;}

    inline void Bind(void) {texture->Bind();}

    inline rect_f32 GetBoundary(void) const {return boundary;}
};

//===========================================================================
class TGLBufferBase : public TGLObject
{
protected:
    void * local_data;
    int Upload(GLenum target, const void *data, int size, GLenum usage);
    int Upload(GLenum target, GLenum usage);
    int GetIntParam(GLenum target, GLenum pname);
    void Bind(GLenum target);
public:
    TGLBufferBase();
    TGLBufferBase(GLenum target);
    virtual ~TGLBufferBase();
    virtual GLuint GetHandle(void);
    inline void * GetLocalData(void) {return local_data;}
    void * AppendLocalData(int count, int elem_size);
    void * InsertLocalData(int index, int count, int elem_size);
    inline void ClearLocalData(void) {RCM_FREE(&local_data);}
    inline int GetElemSize(void) {return rcm_elem_size(local_data);}
};

class TGLIndexBuffer : public TGLBufferBase
{
protected:

public:
    TGLIndexBuffer();
    virtual ~TGLIndexBuffer();
    inline void Bind(void) {TGLBufferBase::Bind(GL_ELEMENT_ARRAY_BUFFER);}
    inline int Upload(const void *data, int size, GLenum usage = GL_STATIC_DRAW) {return TGLBufferBase::Upload(GL_ELEMENT_ARRAY_BUFFER, data, size, usage);}
    inline int Upload(GLenum usage = GL_STATIC_DRAW) {return TGLBufferBase::Upload(GL_ELEMENT_ARRAY_BUFFER, usage);}
    int Size(void);
    inline GLenum Usage(void) {return GetIntParam(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_USAGE);}
    inline void Unbind(void) {glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);}
};


struct LB_BUFFER_DESCRIPTOR {
    uint8_t attrib_type;
    LB_TYPE data_type;
    uint16_t offset;
};

struct LB_GL_TYPE {
    GLint components;
    GLenum component_type;
    GLboolean normalize;
};

LB_GL_TYPE GetGLTypeInfo(LB_TYPE type);


class TGLAttribBuffer : public TGLBufferBase
{
protected:
//	const LB_REFL_STRUCT_INFO *ti;
    LB_BUFFER_DESCRIPTOR *bd;
public:
    TGLAttribBuffer();
    TGLAttribBuffer(void *data, int size, GLenum usage = GL_STATIC_DRAW);
    virtual ~TGLAttribBuffer();
    inline void Bind(void) {TGLBufferBase::Bind(GL_ARRAY_BUFFER);}
    inline int Upload(const void *data, int size, GLenum usage = GL_STATIC_DRAW) {return TGLBufferBase::Upload(GL_ARRAY_BUFFER, data, size, usage);}
    inline int Upload(GLenum usage = GL_STATIC_DRAW) {return TGLBufferBase::Upload(GL_ARRAY_BUFFER, usage);}
    inline int Size(void) {return GetIntParam(GL_ARRAY_BUFFER, GL_BUFFER_SIZE);}
    inline GLenum Usage(void) {return GetIntParam(GL_ARRAY_BUFFER, GL_BUFFER_USAGE);}

//	SetTypeInfo(const LB_REFL_STRUCT_INFO *typeinfo);

    int Register(const lbxREFL_STRUCT_INFO *rtti);
    TGLAttribBuffer & Register(char attrib_type, LB_TYPE data_type, uint16_t offset);

    int BindTo(TGLProgram *program, int elem_size);
    inline void Unbind(void) {glBindBuffer(GL_ARRAY_BUFFER, 0);}
};



struct LB_BUFFER_INFO {
    GLuint buffer_id;
    uint8_t *data;
    int stride;
};

struct LB_ATTRIB_BINDING {
    GLint attrib_loc;
    GLint components;
    GLenum component_type;
    GLboolean normalize;
    int offset;
};


/*
struct LB_UNIFORM_BINDING {
    GLint uniform_loc;
    LB_TYPE type;
    void *data;
};
*/

/*
struct LB_ATTRIB_BIND_INFO {
    GLuint buffer_id; // 0이면 local buffer를 사용한다는 의미
    GLint attrib_loc;
    GLint components;
    GLenum component_type;
    GLboolean normalize;
    GLint stride;
    void * pointer;

    LB_ATTRIB_BIND_INFO();
    LB_ATTRIB_BIND_INFO(LB_TYPE type, int stride = 0);
    void SetType(LB_TYPE type);
    void SetOffset(GLuint buffer_id, int offset);
    void SetPointer(void *address, int offset);
};
*/

class TGLAttribBinder {
protected:
    TGLProgram *p;
    LB_BUFFER_INFO **bi;  // [LB_BUFFER_INFO *] [LB_ATTRIB_BINDING] [LB_ATTRIB_BINDING] ...
    inline int GetBufferSubCount(LB_BUFFER_INFO *buffer_info) {
        return (buffer_info == NULL) ? 0 : (rcm_length(buffer_info) - sizeof(LB_BUFFER_INFO)) / sizeof(LB_ATTRIB_BINDING);
    }
    inline LB_ATTRIB_BINDING * GetBufferSub(LB_BUFFER_INFO *buffer_info) {
        return (buffer_info == NULL) ? NULL : (LB_ATTRIB_BINDING *)(buffer_info + 1);
    }
    LB_BUFFER_INFO * add_buffer(void);
    LB_BUFFER_INFO ** find_buffer(LB_BUFFER_INFO*);


public:
    TGLAttribBinder();
    TGLAttribBinder(TGLProgram *program, TGLAttribBuffer *buffer);
    TGLAttribBinder(TGLProgram *program, const lbxREFL_STRUCT_INFO *typeinfo);
    ~TGLAttribBinder();

    inline int GetBufferInfoCount(void) {return rcm_length(bi);}

    void SetProgram(TGLProgram * program);
    inline TGLProgram * GetProgram(void) {return p;}

    LB_BUFFER_INFO * SetBuffer(GLint buffer_id, int stride);
    LB_BUFFER_INFO * SetBuffer(void * buffer_addr, int stride);
    LB_ATTRIB_BINDING * AddBinding(LB_BUFFER_INFO *buffer_info = NULL);
    LB_ATTRIB_BINDING * AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLint offset, LB_BUFFER_INFO * buffer_info = NULL);
    LB_ATTRIB_BINDING * AddBinding(GLint attrib_loc, LB_TYPE type, GLint offset, LB_BUFFER_INFO * buffer_info = NULL);
    inline LB_ATTRIB_BINDING * AddBinding(const char *attrib_name, LB_TYPE type, GLint offset, LB_BUFFER_INFO * buffer_info = NULL) {
        return AddBinding(p->GetAttribLocation(attrib_name), type, offset, buffer_info);
    }
    inline LB_ATTRIB_BINDING * AddBinding(const char *attrib_name, GLint comp_count, GLenum comp_type, GLint offset, LB_BUFFER_INFO * buffer_info = NULL) {
        return AddBinding(p->GetAttribLocation(attrib_name), comp_count, comp_type, offset, buffer_info);
    }

    int AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLboolean normalize, GLint stride, GLint buffer_id, GLint offset);
    int AddBinding(GLint attrib_loc, GLint comp_count, GLenum comp_type, GLboolean normalize, GLint stride, void * pointer);
    int AddBinding(GLint attrib_loc, const lbxREFL_STRUCT_INFO *ti, const char *member_name, GLboolean normalize);
    int AddBinding(const LB_BUFFER_INFO *bind_info);


    int AddUniform(GLint uniform_loc, LB_TYPE type, void *data);


    //void glVertexAttribPointer(GLuint index,  GLint size,  GLenum type,  GLboolean normalized,  GLsizei stride,  const GLvoid * pointer);
//	int AddBinding(const char *attrib_name, GLenum comp_type, GLint comp_count, GLint stride, GLint offset, GLboolean normalize = false) {
//		return AddBinding(p->GetAttribLocation(attrib_name), comp_type, comp_count, stride, offset, normalize);
//	}

    inline int AddBinding(const char *attrib_name, const lbxREFL_STRUCT_INFO *ti, const char *member_name, GLboolean normalize = false) {
        return AddBinding(p->GetAttribLocation(attrib_name), ti, member_name, normalize);
    }
//	inline int AddBinding_vec4_f32(const char *attrib_name, GLint stride = sizeof(vec4_f32), GLint offset = 0, GLboolean normalize = false) {
//		return AddBinding(attrib_name, GL_FLOAT, 4, stride, offset, normalize);
//	}
//	inline int AddBinding_vec3_f32(const char *attrib_name, GLint stride = sizeof(vec3_f32), GLint offset = 0, GLboolean normalize = false) {
//		return AddBinding(attrib_name, GL_FLOAT, 3, stride, offset, normalize);
//	}
//	inline int AddBinding_vec2_f32(const char *attrib_name, GLint stride = sizeof(vec2_f32), GLint offset = 0, GLboolean normalize = false) {
//		return AddBinding(attrib_name, GL_FLOAT, 2, stride, offset, normalize);
//	}
//	inline int AddBinding_vec4u32(const char *attrib_name, GLint stride = sizeof(vec4u32), GLint offset = 0, GLboolean normalize = true) {
//		return AddBinding(attrib_name, GL_UNSIGNED_BYTE, 4, stride, offset, normalize);
//	}

    void Clear(void);

    int Enable(void);
    int Disable(void);
};


struct LB_RENDER_COMMAND {
    GLenum command;
    GLuint buffer_id;
    GLenum type;
    rcm_t indice;
    int first;
    int count;
    bool *visible;
};

class TGLCommandList {
protected:
    LB_RENDER_COMMAND *list;
public:
    TGLCommandList();
    ~TGLCommandList();

    TGLAttribBinder *ab;

    LB_RENDER_COMMAND * Add(GLenum method, bool *visible = NULL);
    LB_RENDER_COMMAND * Add(GLenum method, LB_TYPE type, int count, bool *visible = NULL);
    LB_RENDER_COMMAND * Add(GLenum method, GLuint indice_buffer_id, GLenum elem_type, int count, bool *visible = NULL);
    LB_RENDER_COMMAND * Add(GLenum method, const uint16_t *indice, int count, bool *visible = NULL);
    LB_RENDER_COMMAND * Add(GLenum method, const uint8_t *indice, int count, bool *visible = NULL);

    LB_RENDER_COMMAND * AddArrays(GLenum method, int first, int count, bool *visible = NULL);

    void Clear(void);

    inline int Count(void) {return rcm_length(list);}
    inline LB_RENDER_COMMAND * Items(int index) {return list + adjust_index(index, rcm_length(list));}

    TGLProgram * GetProgram(void);
};

int DrawCommandList(TGLCommandList *list);

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
    uint32_t col;
    vec2_f32 txc;
} V2CT;

typedef struct {
    vec3_f32 vtx;
    uint32_t col;
    vec2_f32 txc;
} V3CT;

typedef struct {
    vec4_f32 vtx;
    uint32_t col;
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


class TGLDrawList
{
protected:
    TGLAttribBinder ab; // attribute binder
    bool modified;
public:
    TGLDrawList(TGLProgram *program);
    ~TGLDrawList();

    TGLAttribBuffer bf; // buffer
    TGLCommandList  cl; // command list

    LB_ATTRIB_BINDING * SetBinding(const char *attrib_name, LB_TYPE type, GLint offset);

    void FillBuffer(V2CT *dst, TGlyph *glyph, rect_f32 area, uint32_t color);

    int AddGlyph(TGlyph *glyph, rect_f32 area, uint32_t color);
    inline int AddGlyph(TGlyph *glyph, xywh_f32 area, uint32_t color) {
            return AddGlyph(glyph, rect_f32_(area.x, area.y, area.x + area.width, area.y + area.height), color
        );}
    int AddLines(vec2_f32 *lines_vertice, int line_count, uint32_t color);

    int Draw(void);
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
    inline int GetElemSize(void) {return (int)sizeof(T);}
};

template <class T>
TGLBuffer<T>::TGLBuffer()
    : TGLBufferBase(), data(NULL)
{
}

template <class T>
TGLBuffer<T>::~TGLBuffer()
{
    rcm_free((void**)&data);
}


class TGLFrameBufferObject : public TGLObject
{
private:
    typedef TGLObject inherited;
    enum {
        owns_texture = 1
    };
protected:
    uint32_t flags;
    TGLTexture2D *tex;
//	GLuint tex;
    GLuint depth;
    size2_i16 sz;
public:
    TGLFrameBufferObject();
    TGLFrameBufferObject(int width, int height);
    virtual ~TGLFrameBufferObject();
    int SetSize(int width, int height);
    inline int SetSize(size2_i16 size) { return SetSize(size.width, size.height); }
    void Bind(void);
    void Release(void);
//	inline GLuint GetTexture() {return tex;}
    int SetTexture(GLuint texture_handle);

    virtual GLuint GetHandle(void);
    inline GLuint GetTexHandle(void) {return tex->GetHandle();}
    inline size2_i16 GetSize(void) {return sz;}
};


#endif
