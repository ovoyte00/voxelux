/*
 * GLAD OpenGL Loader Implementation
 * Generated for OpenGL 4.1 Core Profile
 * Cross-platform OpenGL function loader
 */

#include "glad/glad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Function pointers
PFNGLCLEARPROC glClear = NULL;
PFNGLCLEARCOLORPROC glClearColor = NULL;
PFNGLGETERRORPROC glGetError = NULL;
PFNGLGETBOOLEANVPROC glGetBooleanv = NULL;
PFNGLPIXELSTOREIPROC glPixelStorei = NULL;
PFNGLFLUSHPROC glFlush = NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
PFNGLDEPTHMASKPROC glDepthMask = NULL;
PFNGLVIEWPORTPROC glViewport = NULL;
PFNGLENABLEPROC glEnable = NULL;
PFNGLDISABLEPROC glDisable = NULL;
PFNGLBLENDFUNCPROC glBlendFunc = NULL;
PFNGLSCISSORPROC glScissor = NULL;
PFNGLDRAWELEMENTSSPROC glDrawElements = NULL;
PFNGLGENBUFFERSPROC glGenBuffers = NULL;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
PFNGLBINDBUFFERPROC glBindBuffer = NULL;
PFNGLBUFFERDATAPROC glBufferData = NULL;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
PFNGLCREATESHADERPROC glCreateShader = NULL;
PFNGLDELETESHADERPROC glDeleteShader = NULL;
PFNGLSHADERSOURCEPROC glShaderSource = NULL;
PFNGLCOMPILESHADERPROC glCompileShader = NULL;
PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
PFNGLATTACHSHADERPROC glAttachShader = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
PFNGLUSEPROGRAMPROC glUseProgram = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
PFNGLUNIFORM1FPROC glUniform1f = NULL;
PFNGLUNIFORM1IPROC glUniform1i = NULL;
PFNGLUNIFORM2FPROC glUniform2f = NULL;
PFNGLUNIFORM4FPROC glUniform4f = NULL;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
PFNGLGENTEXTURESPROC glGenTextures = NULL;
PFNGLDELETETEXTURESPROC glDeleteTextures = NULL;
PFNGLBINDTEXTUREPROC glBindTexture = NULL;
PFNGLTEXIMAGE2DPROC glTexImage2D = NULL;
PFNGLTEXPARAMETERIPROC glTexParameteri = NULL;
PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
PFNGLGETINTEGERVPROC glGetIntegerv = NULL;

static void* get_proc(const char* name) {
#ifdef _WIN32
    static HMODULE opengl32 = NULL;
    if (opengl32 == NULL) {
        opengl32 = LoadLibraryA("opengl32.dll");
    }
    if (opengl32 != NULL) {
        return (void*)GetProcAddress(opengl32, name);
    }
    return NULL;
#elif defined(__APPLE__)
    static void* opengl_lib = NULL;
    if (opengl_lib == NULL) {
        opengl_lib = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY);
    }
    if (opengl_lib != NULL) {
        return dlsym(opengl_lib, name);
    }
    return NULL;
#else
    static void* libgl = NULL;
    if (libgl == NULL) {
        libgl = dlopen("libGL.so.1", RTLD_LAZY);
        if (libgl == NULL) {
            libgl = dlopen("libGL.so", RTLD_LAZY);
        }
    }
    if (libgl != NULL) {
        return dlsym(libgl, name);
    }
    return NULL;
#endif
}

int gladLoadGL(void) {
    return gladLoadGLLoader(&get_proc);
}

int gladLoadGLLoader(GLADloadproc load_proc) {
    if (load_proc == NULL) return 0;
    
    glClear = (PFNGLCLEARPROC)load_proc("glClear");
    glClearColor = (PFNGLCLEARCOLORPROC)load_proc("glClearColor");
    glGetError = (PFNGLGETERRORPROC)load_proc("glGetError");
    glGetBooleanv = (PFNGLGETBOOLEANVPROC)load_proc("glGetBooleanv");
    glPixelStorei = (PFNGLPIXELSTOREIPROC)load_proc("glPixelStorei");
    glFlush = (PFNGLFLUSHPROC)load_proc("glFlush");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)load_proc("glBindFramebuffer");
    glDepthMask = (PFNGLDEPTHMASKPROC)load_proc("glDepthMask");
    glViewport = (PFNGLVIEWPORTPROC)load_proc("glViewport");
    glEnable = (PFNGLENABLEPROC)load_proc("glEnable");
    glDisable = (PFNGLDISABLEPROC)load_proc("glDisable");
    glBlendFunc = (PFNGLBLENDFUNCPROC)load_proc("glBlendFunc");
    glScissor = (PFNGLSCISSORPROC)load_proc("glScissor");
    glDrawElements = (PFNGLDRAWELEMENTSSPROC)load_proc("glDrawElements");
    glGenBuffers = (PFNGLGENBUFFERSPROC)load_proc("glGenBuffers");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)load_proc("glDeleteBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)load_proc("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)load_proc("glBufferData");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)load_proc("glGenVertexArrays");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)load_proc("glDeleteVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)load_proc("glBindVertexArray");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)load_proc("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)load_proc("glVertexAttribPointer");
    glCreateShader = (PFNGLCREATESHADERPROC)load_proc("glCreateShader");
    glDeleteShader = (PFNGLDELETESHADERPROC)load_proc("glDeleteShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)load_proc("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)load_proc("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)load_proc("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)load_proc("glGetShaderInfoLog");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)load_proc("glCreateProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)load_proc("glDeleteProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)load_proc("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)load_proc("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)load_proc("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)load_proc("glGetProgramInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)load_proc("glUseProgram");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)load_proc("glGetUniformLocation");
    glUniform1f = (PFNGLUNIFORM1FPROC)load_proc("glUniform1f");
    glUniform1i = (PFNGLUNIFORM1IPROC)load_proc("glUniform1i");
    glUniform2f = (PFNGLUNIFORM2FPROC)load_proc("glUniform2f");
    glUniform4f = (PFNGLUNIFORM4FPROC)load_proc("glUniform4f");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)load_proc("glUniformMatrix4fv");
    glGenTextures = (PFNGLGENTEXTURESPROC)load_proc("glGenTextures");
    glDeleteTextures = (PFNGLDELETETEXTURESPROC)load_proc("glDeleteTextures");
    glBindTexture = (PFNGLBINDTEXTUREPROC)load_proc("glBindTexture");
    glTexImage2D = (PFNGLTEXIMAGE2DPROC)load_proc("glTexImage2D");
    glTexParameteri = (PFNGLTEXPARAMETERIPROC)load_proc("glTexParameteri");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)load_proc("glActiveTexture");
    glGetIntegerv = (PFNGLGETINTEGERVPROC)load_proc("glGetIntegerv");
    
    return 1; // Success
}