/*
 * GLAD OpenGL Loader
 * Generated for OpenGL 4.1 Core Profile
 * Cross-platform OpenGL function loader
 */

#ifndef __glad_h_
#define __glad_h_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef APIENTRY
#ifdef _WIN32
#define APIENTRY __stdcall
#else
#define APIENTRY
#endif
#endif

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#ifndef GLAPI
#define GLAPI extern
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

// OpenGL Constants
#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_DOUBLE                         0x140A
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400

// Error constants
#define GL_NO_ERROR                       0x0000
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505

// Additional constants for state management
#define GL_DITHER                         0x0BD0
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_PACK_ALIGNMENT                 0x0D05

// Framebuffer constants
#define GL_FRAMEBUFFER                    0x8D40
#define GL_FRAMEBUFFER_BINDING            0x8CA6
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_LINES                          0x0001
#define GL_LINE_STRIP                     0x0003
#define GL_POINTS                         0x0000
#define GL_BLEND                          0x0BE2
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DEPTH_TEST                     0x0B71
#define GL_CULL_FACE                      0x0B44
#define GL_SCISSOR_TEST                   0x0C11
#define GL_TEXTURE_2D                     0x0DE1
#define GL_RGBA                           0x1908
#define GL_RGB                            0x1907
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_STREAM_DRAW                    0x88E0
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_TEXTURE0                       0x84C0
#define GL_FRAMEBUFFER                    0x8D40
#define GL_FRAMEBUFFER_BINDING            0x8CA6
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00

// Function declarations
typedef void (APIENTRYP PFNGLCLEARPROC) (GLbitfield mask);
typedef void (APIENTRYP PFNGLCLEARCOLORPROC) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef void (APIENTRYP PFNGLVIEWPORTPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLENABLEPROC) (GLenum cap);
typedef void (APIENTRYP PFNGLDISABLEPROC) (GLenum cap);
typedef void (APIENTRYP PFNGLBLENDFUNCPROC) (GLenum sfactor, GLenum dfactor);
typedef void (APIENTRYP PFNGLSCISSORPROC) (GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLDRAWELEMENTSSPROC) (GLenum mode, GLsizei count, GLenum type, const void *indices);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRYP PFNGLUNIFORM4FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRYP PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef void (APIENTRYP PFNGLDELETETEXTURESPROC) (GLsizei n, const GLuint *textures);
typedef void (APIENTRYP PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (APIENTRYP PFNGLTEXIMAGE2DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRYP PFNGLTEXPARAMETERIPROC) (GLenum target, GLenum pname, GLint param);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
typedef void (APIENTRYP PFNGLGETINTEGERVPROC) (GLenum pname, GLint *data);
typedef GLenum (APIENTRYP PFNGLGETERRORPROC) (void);
typedef void (APIENTRYP PFNGLGETBOOLEANVPROC) (GLenum pname, GLboolean *data);
typedef void (APIENTRYP PFNGLPIXELSTOREIPROC) (GLenum pname, GLint param);
typedef void (APIENTRYP PFNGLFLUSHPROC) (void);
typedef void (APIENTRYP PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRYP PFNGLDEPTHMASKPROC) (GLboolean flag);

// Function pointers
GLAPI PFNGLCLEARPROC glClear;
GLAPI PFNGLCLEARCOLORPROC glClearColor;
GLAPI PFNGLGETERRORPROC glGetError;
GLAPI PFNGLGETBOOLEANVPROC glGetBooleanv;
GLAPI PFNGLPIXELSTOREIPROC glPixelStorei;
GLAPI PFNGLFLUSHPROC glFlush;
GLAPI PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
GLAPI PFNGLDEPTHMASKPROC glDepthMask;
GLAPI PFNGLVIEWPORTPROC glViewport;
GLAPI PFNGLENABLEPROC glEnable;
GLAPI PFNGLDISABLEPROC glDisable;
GLAPI PFNGLBLENDFUNCPROC glBlendFunc;
GLAPI PFNGLSCISSORPROC glScissor;
GLAPI PFNGLDRAWELEMENTSSPROC glDrawElements;
GLAPI PFNGLGENBUFFERSPROC glGenBuffers;
GLAPI PFNGLDELETEBUFFERSPROC glDeleteBuffers;
GLAPI PFNGLBINDBUFFERPROC glBindBuffer;
GLAPI PFNGLBUFFERDATAPROC glBufferData;
GLAPI PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
GLAPI PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
GLAPI PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
GLAPI PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
GLAPI PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
GLAPI PFNGLCREATESHADERPROC glCreateShader;
GLAPI PFNGLDELETESHADERPROC glDeleteShader;
GLAPI PFNGLSHADERSOURCEPROC glShaderSource;
GLAPI PFNGLCOMPILESHADERPROC glCompileShader;
GLAPI PFNGLGETSHADERIVPROC glGetShaderiv;
GLAPI PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
GLAPI PFNGLCREATEPROGRAMPROC glCreateProgram;
GLAPI PFNGLDELETEPROGRAMPROC glDeleteProgram;
GLAPI PFNGLATTACHSHADERPROC glAttachShader;
GLAPI PFNGLLINKPROGRAMPROC glLinkProgram;
GLAPI PFNGLGETPROGRAMIVPROC glGetProgramiv;
GLAPI PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
GLAPI PFNGLUSEPROGRAMPROC glUseProgram;
GLAPI PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
GLAPI PFNGLUNIFORM1FPROC glUniform1f;
GLAPI PFNGLUNIFORM1IPROC glUniform1i;
GLAPI PFNGLUNIFORM2FPROC glUniform2f;
GLAPI PFNGLUNIFORM4FPROC glUniform4f;
GLAPI PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
GLAPI PFNGLGENTEXTURESPROC glGenTextures;
GLAPI PFNGLDELETETEXTURESPROC glDeleteTextures;
GLAPI PFNGLBINDTEXTUREPROC glBindTexture;
GLAPI PFNGLTEXIMAGE2DPROC glTexImage2D;
GLAPI PFNGLTEXPARAMETERIPROC glTexParameteri;
GLAPI PFNGLACTIVETEXTUREPROC glActiveTexture;
GLAPI PFNGLGETINTEGERVPROC glGetIntegerv;

// Initialization
typedef void* (*GLADloadproc)(const char *name);
int gladLoadGL(void);
int gladLoadGLLoader(GLADloadproc load_proc);

#ifdef __cplusplus
}
#endif

#endif