/**
 * @file engine/render/gl_loader.cpp
 * @brief OpenGL 函数加载实现
 */

#include "engine/render/gl_loader.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <cstdlib>

namespace engine::render::gl {

// =========================================================================
// 函数指针定义
// =========================================================================

// Shader
PFNGLCREATESHADERPROC          CreateShader        = nullptr;
PFNGLSHADERSOURCEPROC          ShaderSource        = nullptr;
PFNGLCOMPILESHADERPROC         CompileShader       = nullptr;
PFNGLGETSHADERIVPROC           GetShaderiv         = nullptr;
PFNGLGETSHADERINFOLOGPROC      GetShaderInfoLog    = nullptr;
PFNGLCREATEPROGRAMPROC         CreateProgram       = nullptr;
PFNGLATTACHSHADERPROC          AttachShader        = nullptr;
PFNGLLINKPROGRAMPROC           LinkProgram         = nullptr;
PFNGLGETPROGRAMIVPROC          GetProgramiv        = nullptr;
PFNGLGETPROGRAMINFOLOGPROC     GetProgramInfoLog   = nullptr;
PFNGLUSEPROGRAMPROC            UseProgram          = nullptr;
PFNGLDELETESHADERPROC          DeleteShader        = nullptr;
PFNGLDELETEPROGRAMPROC         DeleteProgram       = nullptr;

// VAO
PFNGLGENVERTEXARRAYSPROC       GenVertexArrays     = nullptr;
PFNGLBINDVERTEXARRAYPROC       BindVertexArray     = nullptr;
PFNGLDELETEVERTEXARRAYSPROC    DeleteVertexArrays  = nullptr;

// VBO
PFNGLGENBUFFERSPROC            GenBuffers          = nullptr;
PFNGLBINDBUFFERPROC            BindBuffer          = nullptr;
PFNGLBUFFERDATAPROC            BufferData          = nullptr;
PFNGLDELETEBUFFERSPROC         DeleteBuffers       = nullptr;

// Vertex Attrib
PFNGLVERTEXATTRIBPOINTERPROC   VertexAttribPointer   = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray = nullptr;

// Texture
PFNGLACTIVETEXTUREPROC         ActiveTexture       = nullptr;
PFNGLGENTEXTURESPROC           GenTextures         = nullptr;
PFNGLBINDTEXTUREPROC           BindTexture         = nullptr;
PFNGLTEXIMAGE2DPROC            TexImage2D          = nullptr;
PFNGLTEXPARAMETERIPROC         TexParameteri       = nullptr;
PFNGLDELETETEXTURESPROC        DeleteTextures      = nullptr;

// Uniform
PFNGLUNIFORMMATRIX4FVPROC      UniformMatrix4fv    = nullptr;

// Draw
PFNGLDRAWARRAYSPROC            DrawArrays          = nullptr;

// =========================================================================
// 宏：加载函数指针
// =========================================================================
// glfwGetProcAddress 需要完整的 OpenGL 函数名（含 "gl" 前缀）
#define GL_LOAD_PROC(func) \
    do { \
        func = reinterpret_cast<decltype(func)>(glfwGetProcAddress("gl" #func)); \
        if (!func) { \
            spdlog::warn("gl::load: failed to load gl{}", #func); \
        } \
    } while(0)

// =========================================================================
// load
// =========================================================================
bool load(GLFWwindow* window)
{
    spdlog::info("gl::load: loading OpenGL functions...");

    glfwMakeContextCurrent(window);

    // Shader
    GL_LOAD_PROC(CreateShader);
    GL_LOAD_PROC(ShaderSource);
    GL_LOAD_PROC(CompileShader);
    GL_LOAD_PROC(GetShaderiv);
    GL_LOAD_PROC(GetShaderInfoLog);
    GL_LOAD_PROC(CreateProgram);
    GL_LOAD_PROC(AttachShader);
    GL_LOAD_PROC(LinkProgram);
    GL_LOAD_PROC(GetProgramiv);
    GL_LOAD_PROC(GetProgramInfoLog);
    GL_LOAD_PROC(UseProgram);
    GL_LOAD_PROC(DeleteShader);
    GL_LOAD_PROC(DeleteProgram);

    // VAO
    GL_LOAD_PROC(GenVertexArrays);
    GL_LOAD_PROC(BindVertexArray);
    GL_LOAD_PROC(DeleteVertexArrays);

    // VBO
    GL_LOAD_PROC(GenBuffers);
    GL_LOAD_PROC(BindBuffer);
    GL_LOAD_PROC(BufferData);
    GL_LOAD_PROC(DeleteBuffers);

    // Vertex Attrib
    GL_LOAD_PROC(VertexAttribPointer);
    GL_LOAD_PROC(EnableVertexAttribArray);

    // Texture
    GL_LOAD_PROC(ActiveTexture);
    GL_LOAD_PROC(GenTextures);
    GL_LOAD_PROC(BindTexture);
    GL_LOAD_PROC(TexImage2D);
    GL_LOAD_PROC(TexParameteri);
    GL_LOAD_PROC(DeleteTextures);

    // Uniform
    GL_LOAD_PROC(UniformMatrix4fv);

    // Draw
    GL_LOAD_PROC(DrawArrays);

    if (!CreateProgram || !UseProgram || !GenVertexArrays || !GenBuffers)
    {
        spdlog::critical("gl::load: critical functions missing. GPU may not support OpenGL 3.3.");
        return false;
    }

    spdlog::info("gl::load: OpenGL functions loaded successfully.");
    return true;
}

#undef GL_LOAD_PROC

} // namespace engine::render::gl
