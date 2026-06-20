/**
 * @file engine/render/gl_loader.h
 * @brief 最小化 OpenGL 3.3 Core 函数加载器
 *
 * Windows SDK 10.0.26100+ 移除了 GL/glext.h，因此本文件自行定义
 * 所需的 OpenGL 函数指针类型，不依赖 glext.h。
 *
 * 使用方式：
 *   1. 在 OpenGL 上下文创建后调用 gl::load(window)
 *   2. 通过 gl::FuncName() 调用 OpenGL 函数
 *
 * 后续 Phase 切换 bgfx 时，此文件可完全移除。
 */

#pragma once

// ---------------------------------------------------------------------------
// Windows SDK 基本 GL 类型（APIENTRY, GLenum 等）
// ---------------------------------------------------------------------------
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/gl.h>

// ---------------------------------------------------------------------------
// 补充 OpenGL 类型定义（原 glext.h 中的内容）
// ---------------------------------------------------------------------------
typedef char           GLchar;
typedef ptrdiff_t      GLsizeiptr;
typedef ptrdiff_t      GLintptr;
typedef ptrdiff_t      GLintptrARB;
typedef ptrdiff_t      GLsizeiptrARB;
typedef unsigned char  GLboolean;

#ifndef GL_FRAGMENT_SHADER
#  define GL_FRAGMENT_SHADER          0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#  define GL_VERTEX_SHADER            0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#  define GL_COMPILE_STATUS           0x8B81
#endif
#ifndef GL_LINK_STATUS
#  define GL_LINK_STATUS              0x8B82
#endif
#ifndef GL_ARRAY_BUFFER
#  define GL_ARRAY_BUFFER             0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#  define GL_ELEMENT_ARRAY_BUFFER     0x8893
#endif
#ifndef GL_STATIC_DRAW
#  define GL_STATIC_DRAW              0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#  define GL_DYNAMIC_DRAW             0x88E8
#endif
#ifndef GL_FLOAT
#  define GL_FLOAT                    0x1406
#endif
#ifndef GL_FALSE
#  define GL_FALSE                    0
#endif
#ifndef GL_TRUE
#  define GL_TRUE                     1
#endif
#ifndef GL_TEXTURE0
#  define GL_TEXTURE0                 0x84C0
#endif
#ifndef GL_TEXTURE_2D
#  define GL_TEXTURE_2D               0x0DE1
#endif
#ifndef GL_RGBA
#  define GL_RGBA                     0x1908
#endif
#ifndef GL_RGB
#  define GL_RGB                      0x1907
#endif
#ifndef GL_UNSIGNED_BYTE
#  define GL_UNSIGNED_BYTE            0x1401
#endif
#ifndef GL_LINEAR
#  define GL_LINEAR                   0x2601
#endif
#ifndef GL_NEAREST
#  define GL_NEAREST                  0x2600
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#  define GL_TEXTURE_MIN_FILTER       0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#  define GL_TEXTURE_MAG_FILTER       0x2800
#endif
#ifndef GL_TEXTURE_WRAP_S
#  define GL_TEXTURE_WRAP_S           0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#  define GL_TEXTURE_WRAP_T           0x2803
#endif
#ifndef GL_CLAMP_TO_EDGE
#  define GL_CLAMP_TO_EDGE            0x812F
#endif
#ifndef GL_TRIANGLES
#  define GL_TRIANGLES                0x0004
#endif
#ifndef GL_INFO_LOG_LENGTH
#  define GL_INFO_LOG_LENGTH          0x8B84
#endif
#ifndef GL_DEBUG_OUTPUT
#  define GL_DEBUG_OUTPUT             0x92E0
#endif

struct GLFWwindow;

namespace engine::render::gl {

// =========================================================================
// 初始化
// =========================================================================
bool load(GLFWwindow* window);

// =========================================================================
// OpenGL 1.1 函数 — 直接转发（无需动态加载）
// =========================================================================
inline void Viewport(GLint x, GLint y, GLsizei w, GLsizei h) { glViewport(x, y, w, h); }
inline void Clear(GLbitfield mask)      { glClear(mask); }
inline void ClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { glClearColor(r, g, b, a); }
inline void Enable(GLenum cap)          { glEnable(cap); }
inline void Disable(GLenum cap)         { glDisable(cap); }
inline void BlendFunc(GLenum sf, GLenum df) { glBlendFunc(sf, df); }
inline const GLubyte* GetString(GLenum name) { return glGetString(name); }
inline GLenum GetError()                { return glGetError(); }

// =========================================================================
// OpenGL 2.0+ 函数指针类型定义
// =========================================================================

// --- Shader ---
using PFNGLCREATESHADERPROC         = GLuint  (APIENTRY *)(GLenum);
using PFNGLSHADERSOURCEPROC         = void    (APIENTRY *)(GLuint, GLsizei, const GLchar**, const GLint*);
using PFNGLCOMPILESHADERPROC        = void    (APIENTRY *)(GLuint);
using PFNGLGETSHADERIVPROC          = void    (APIENTRY *)(GLuint, GLenum, GLint*);
using PFNGLGETSHADERINFOLOGPROC     = void    (APIENTRY *)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFNGLCREATEPROGRAMPROC        = GLuint  (APIENTRY *)();
using PFNGLATTACHSHADERPROC         = void    (APIENTRY *)(GLuint, GLuint);
using PFNGLLINKPROGRAMPROC          = void    (APIENTRY *)(GLuint);
using PFNGLGETPROGRAMIVPROC         = void    (APIENTRY *)(GLuint, GLenum, GLint*);
using PFNGLGETPROGRAMINFOLOGPROC    = void    (APIENTRY *)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFNGLUSEPROGRAMPROC           = void    (APIENTRY *)(GLuint);
using PFNGLDELETESHADERPROC         = void    (APIENTRY *)(GLuint);
using PFNGLDELETEPROGRAMPROC        = void    (APIENTRY *)(GLuint);

// --- VAO ---
using PFNGLGENVERTEXARRAYSPROC      = void    (APIENTRY *)(GLsizei, GLuint*);
using PFNGLBINDVERTEXARRAYPROC      = void    (APIENTRY *)(GLuint);
using PFNGLDELETEVERTEXARRAYSPROC   = void    (APIENTRY *)(GLsizei, const GLuint*);

// --- VBO ---
using PFNGLGENBUFFERSPROC           = void    (APIENTRY *)(GLsizei, GLuint*);
using PFNGLBINDBUFFERPROC           = void    (APIENTRY *)(GLenum, GLuint);
using PFNGLBUFFERDATAPROC           = void    (APIENTRY *)(GLenum, GLsizeiptr, const void*, GLenum);
using PFNGLDELETEBUFFERSPROC        = void    (APIENTRY *)(GLsizei, const GLuint*);

// --- Vertex Attrib ---
using PFNGLVERTEXATTRIBPOINTERPROC  = void    (APIENTRY *)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void (APIENTRY *)(GLuint);

// --- Texture ---
using PFNGLACTIVETEXTUREPROC        = void    (APIENTRY *)(GLenum);
using PFNGLGENTEXTURESPROC          = void    (APIENTRY *)(GLsizei, GLuint*);
using PFNGLBINDTEXTUREPROC          = void    (APIENTRY *)(GLenum, GLuint);
using PFNGLTEXIMAGE2DPROC           = void    (APIENTRY *)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
using PFNGLTEXPARAMETERIPROC        = void    (APIENTRY *)(GLenum, GLenum, GLint);
using PFNGLDELETETEXTURESPROC       = void    (APIENTRY *)(GLsizei, const GLuint*);

// --- Uniform ---
using PFNGLUNIFORMMATRIX4FVPROC     = void    (APIENTRY *)(GLint, GLsizei, GLboolean, const GLfloat*);

// --- Draw ---
using PFNGLDRAWARRAYSPROC           = void    (APIENTRY *)(GLenum, GLint, GLsizei);

// =========================================================================
// 函数指针声明
// =========================================================================
extern PFNGLCREATESHADERPROC          CreateShader;
extern PFNGLSHADERSOURCEPROC          ShaderSource;
extern PFNGLCOMPILESHADERPROC         CompileShader;
extern PFNGLGETSHADERIVPROC           GetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC      GetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC         CreateProgram;
extern PFNGLATTACHSHADERPROC          AttachShader;
extern PFNGLLINKPROGRAMPROC           LinkProgram;
extern PFNGLGETPROGRAMIVPROC          GetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC     GetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC            UseProgram;
extern PFNGLDELETESHADERPROC          DeleteShader;
extern PFNGLDELETEPROGRAMPROC         DeleteProgram;

extern PFNGLGENVERTEXARRAYSPROC       GenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC       BindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC    DeleteVertexArrays;

extern PFNGLGENBUFFERSPROC            GenBuffers;
extern PFNGLBINDBUFFERPROC            BindBuffer;
extern PFNGLBUFFERDATAPROC            BufferData;
extern PFNGLDELETEBUFFERSPROC         DeleteBuffers;

extern PFNGLVERTEXATTRIBPOINTERPROC   VertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;

extern PFNGLACTIVETEXTUREPROC         ActiveTexture;
extern PFNGLGENTEXTURESPROC           GenTextures;
extern PFNGLBINDTEXTUREPROC           BindTexture;
extern PFNGLTEXIMAGE2DPROC            TexImage2D;
extern PFNGLTEXPARAMETERIPROC         TexParameteri;
extern PFNGLDELETETEXTURESPROC        DeleteTextures;

extern PFNGLUNIFORMMATRIX4FVPROC      UniformMatrix4fv;

extern PFNGLDRAWARRAYSPROC            DrawArrays;

} // namespace engine::render::gl
