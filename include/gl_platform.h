#pragma once
/**
 * @file gl_platform.h
 * @brief Cross-platform OpenGL 3.3 Core / OpenGL ES 3.0 function loader
 */

#ifdef __ANDROID__
    #include <GLES3/gl3.h>
    #include <GLES3/gl3ext.h>
    inline bool InitGLPlatform() { return true; }
#else
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <GL/gl.h>
    #include <cstddef>
    #include <iostream>

    // GL constants not in standard Win32 GL/gl.h
    #define GL_ARRAY_BUFFER                   0x8892
    #define GL_ELEMENT_ARRAY_BUFFER           0x8893
    #define GL_STATIC_DRAW                    0x88E4
    #define GL_DYNAMIC_DRAW                   0x88E8
    #define GL_FRAGMENT_SHADER                0x8B30
    #define GL_VERTEX_SHADER                  0x8B31
    #define GL_COMPILE_STATUS                 0x8B81
    #define GL_LINK_STATUS                    0x8B82
    #define GL_INFO_LOG_LENGTH                0x8B84

    typedef ptrdiff_t GLsizeiptr;
    typedef ptrdiff_t GLintptr;
    typedef char GLchar;

    // Function pointers
    typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
    typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
    typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
    typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);

    typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
    typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint array);
    typedef void (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);

    typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
    typedef void (APIENTRY *PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint index);
    typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);

    typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
    typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
    typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
    typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
    typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint shader);

    typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
    typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
    typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint program);
    typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
    typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
    typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
    typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint program);

    typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
    typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
    typedef void (APIENTRY *PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    typedef void (APIENTRY *PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

    extern PFNGLGENBUFFERSPROC glGenBuffers;
    extern PFNGLBINDBUFFERPROC glBindBuffer;
    extern PFNGLBUFFERDATAPROC glBufferData;
    extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;

    extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
    extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
    extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;

    extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
    extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

    extern PFNGLCREATESHADERPROC glCreateShader;
    extern PFNGLSHADERSOURCEPROC glShaderSource;
    extern PFNGLCOMPILESHADERPROC glCompileShader;
    extern PFNGLGETSHADERIVPROC glGetShaderiv;
    extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
    extern PFNGLDELETESHADERPROC glDeleteShader;

    extern PFNGLCREATEPROGRAMPROC glCreateProgram;
    extern PFNGLATTACHSHADERPROC glAttachShader;
    extern PFNGLLINKPROGRAMPROC glLinkProgram;
    extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
    extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    extern PFNGLUSEPROGRAMPROC glUseProgram;
    extern PFNGLDELETEPROGRAMPROC glDeleteProgram;

    extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
    extern PFNGLUNIFORM1FPROC glUniform1f;
    extern PFNGLUNIFORM3FPROC glUniform3f;
    extern PFNGLUNIFORM4FPROC glUniform4f;

    inline bool InitGLPlatform() {
        #define LOAD_PROC(type, name) \
            name = (type)wglGetProcAddress(#name); \
            if (!name) { std::cerr << "Failed to load GL function: " << #name << "\n"; return false; }

        LOAD_PROC(PFNGLGENBUFFERSPROC, glGenBuffers);
        LOAD_PROC(PFNGLBINDBUFFERPROC, glBindBuffer);
        LOAD_PROC(PFNGLBUFFERDATAPROC, glBufferData);
        LOAD_PROC(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);

        LOAD_PROC(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays);
        LOAD_PROC(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
        LOAD_PROC(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays);

        LOAD_PROC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
        LOAD_PROC(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
        LOAD_PROC(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);

        LOAD_PROC(PFNGLCREATESHADERPROC, glCreateShader);
        LOAD_PROC(PFNGLSHADERSOURCEPROC, glShaderSource);
        LOAD_PROC(PFNGLCOMPILESHADERPROC, glCompileShader);
        LOAD_PROC(PFNGLGETSHADERIVPROC, glGetShaderiv);
        LOAD_PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
        LOAD_PROC(PFNGLDELETESHADERPROC, glDeleteShader);

        LOAD_PROC(PFNGLCREATEPROGRAMPROC, glCreateProgram);
        LOAD_PROC(PFNGLATTACHSHADERPROC, glAttachShader);
        LOAD_PROC(PFNGLLINKPROGRAMPROC, glLinkProgram);
        LOAD_PROC(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
        LOAD_PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
        LOAD_PROC(PFNGLUSEPROGRAMPROC, glUseProgram);
        LOAD_PROC(PFNGLDELETEPROGRAMPROC, glDeleteProgram);

        LOAD_PROC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
        LOAD_PROC(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
        LOAD_PROC(PFNGLUNIFORM1FPROC, glUniform1f);
        LOAD_PROC(PFNGLUNIFORM3FPROC, glUniform3f);
        LOAD_PROC(PFNGLUNIFORM4FPROC, glUniform4f);

        #undef LOAD_PROC
        return true;
    }
#endif
