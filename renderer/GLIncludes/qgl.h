#include "../../idlib/Lib.h"
#ifndef __QGL_H__
#define __QGL_H__
#include "qgldefines.h"
// non-windows systems will just redefine qgl* to gl*
//defined( ID_GL_HARDLINK )
#include "qgl_linked.h"
//#else


//#if defined( _WIN32 )
#include "gl.h"
#include <stdio.h>
#include <float.h>
#include <string.h>
#elif defined( MACOS_X )
// magic flag to keep tiger gl.h from loading glext.h
#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>
#elif defined( __linux__ ) || defined( __FreeBSD__ )
#include <dlfcn.h>
// using our local glext.h
#define GL_GLEXT_LEGACY
#define GLX_GLXEXT_LEGACY
#include <GL/gl.h>
#include <GL/glx.h>
#else
#include <gl.h>
#endif
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef WINAPI
#define WINAPI
#endif

// only use local glext.h if we are not using the system one already
// http://oss.sgi.com/projects/ogl-sample/ABI/
#ifndef GL_GLEXT_VERSION
#include "glext.h"
#endif

typedef void (*GLExtension_t)( void );

#ifdef __cplusplus
	extern "C" {
#endif

GLExtension_t GLimp_ExtensionPointer( const char *name );

#ifdef __cplusplus
}
#endif

#if defined( _WIN32 )
#pragma warning (disable : 4113 4133 4047 4018 )
int ( WINAPI * qwglChoosePixelFormat )( HDC, CONST PIXELFORMATDESCRIPTOR * );
int ( WINAPI * qwglDescribePixelFormat )( HDC, int, UINT, LPPIXELFORMATDESCRIPTOR );
int ( WINAPI * qwglGetPixelFormat )( HDC );
bool ( WINAPI * qwglSetPixelFormat )( HDC, int, CONST PIXELFORMATDESCRIPTOR * );
bool ( WINAPI * qwglSwapBuffers )( HDC );

bool ( WINAPI * qwglCopyContext )( HGLRC, HGLRC, UINT );
HGLRC ( WINAPI * qwglCreateContext )( HDC );
HGLRC ( WINAPI * qwglCreateLayerContext )( HDC, int );
bool ( WINAPI * qwglDeleteContext )( HGLRC );
HGLRC ( WINAPI * qwglGetCurrentContext )( VOID );
HDC ( WINAPI * qwglGetCurrentDC )( VOID );
PROC ( WINAPI * qwglGetProcAddress )( LPCSTR );
bool ( WINAPI * qwglMakeCurrent )( HDC, HGLRC );
bool ( WINAPI * qwglShareLists )( HGLRC, HGLRC );
bool ( WINAPI * qwglUseFontBitmaps )( HDC, DWORD, DWORD, DWORD );

bool ( WINAPI * qwglUseFontOutlines )( HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT );

bool ( WINAPI * qwglDescribeLayerPlane )( HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR );
int ( WINAPI * qwglSetLayerPaletteEntries )( HDC, int, int, int, CONST COLORREF * );
int ( WINAPI * qwglGetLayerPaletteEntries )( HDC, int, int, int, COLORREF * );
bool ( WINAPI * qwglRealizeLayerPalette )( HDC, int, bool );
bool ( WINAPI * qwglSwapLayerBuffers )( HDC, UINT );

bool ( WINAPI * qwglGetDeviceGammaRampEXT )( unsigned char *, unsigned char *, unsigned char * );
bool ( WINAPI * qwglSetDeviceGammaRampEXT )( const unsigned char *, const unsigned char *, const unsigned char * );
bool ( WINAPI * qwglSwapIntervalEXT )( int interval );
#endif

// multitexture
extern	void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
extern	void ( APIENTRY * qglMultiTexCoord2fvARB )( GLenum texture, const GLfloat *st );
extern	void ( APIENTRY * qglMultiTexCoord4fARB )( GLenum texture, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
extern	void ( APIENTRY * qglMultiTexCoord4fvARB )( GLenum texture, const GLfloat *strq );
extern	void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
extern	void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

// ARB_vertex_buffer_object
extern PFNGLBINDBUFFERARBPROC 			qglBindBufferARB;
extern PFNGLDELETEBUFFERSARBPROC 		qglDeleteBuffersARB;
extern PFNGLGENBUFFERSARBPROC 			qglGenBuffersARB;
extern PFNGLISBUFFERARBPROC 			qglIsBufferARB;
extern PFNGLBUFFERDATAARBPROC 			qglBufferDataARB;
extern PFNGLBUFFERSUBDATAARBPROC 		qglBufferSubDataARB;
extern PFNGLGETBUFFERSUBDATAARBPROC 	qglGetBufferSubDataARB;
extern PFNGLMAPBUFFERARBPROC 			qglMapBufferARB;
extern PFNGLUNMAPBUFFERARBPROC 			qglUnmapBufferARB;
extern PFNGLGETBUFFERPARAMETERIVARBPROC qglGetBufferParameterivARB;
extern PFNGLGETBUFFERPOINTERVARBPROC 	qglGetBufferPointervARB;

// NV_register_combiners
extern	void ( APIENTRY *qglCombinerParameterfvNV )( GLenum pname, const GLfloat *params );
extern	void ( APIENTRY *qglCombinerParameterivNV )( GLenum pname, const GLint *params );
extern	void ( APIENTRY *qglCombinerParameterfNV )( GLenum pname, const GLfloat param );
extern	void ( APIENTRY *qglCombinerParameteriNV )( GLenum pname, const GLint param );
extern	void ( APIENTRY *qglCombinerInputNV )( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
extern	void ( APIENTRY *qglCombinerOutputNV )( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum );
extern	void ( APIENTRY *qglFinalCombinerInputNV )( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );

// NV_vertex_program / NV_fragment_program
extern void ( APIENTRY *qglBindProgramNV )(GLenum, GLuint);
extern void ( APIENTRY *qglLoadProgramNV )(GLenum, GLuint, GLsizei, const GLubyte * RESTRICT );
extern void ( APIENTRY *qglProgramParameter4fvNV )(GLenum, GLuint, const GLfloat * RESTRICT );

// 3D textures
extern void ( APIENTRY *qglTexImage3D)( GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);

// shared texture palette
extern	void ( APIENTRY *qglColorTableEXT)( int, int, int, int, int, const void * );

// EXT_draw_range_elements
extern  void ( APIENTRY * qglDrawRangeElementsEXT )(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * RESTRICT indices);

// ATI_fragment_shader
extern	PFNGLGENFRAGMENTSHADERSATIPROC	qglGenFragmentShadersATI;
extern	PFNGLBINDFRAGMENTSHADERATIPROC	qglBindFragmentShaderATI;
extern	PFNGLDELETEFRAGMENTSHADERATIPROC qglDeleteFragmentShaderATI;
extern	PFNGLBEGINFRAGMENTSHADERATIPROC qglBeginFragmentShaderATI;
extern	PFNGLENDFRAGMENTSHADERATIPROC	qglEndFragmentShaderATI;
extern	PFNGLPASSTEXCOORDATIPROC		qglPassTexCoordATI;
extern	PFNGLSAMPLEMAPATIPROC			qglSampleMapATI;
extern	PFNGLCOLORFRAGMENTOP1ATIPROC	qglColorFragmentOp1ATI;
extern	PFNGLCOLORFRAGMENTOP2ATIPROC	qglColorFragmentOp2ATI;
extern	PFNGLCOLORFRAGMENTOP3ATIPROC	qglColorFragmentOp3ATI;
extern	PFNGLALPHAFRAGMENTOP1ATIPROC	qglAlphaFragmentOp1ATI;
extern	PFNGLALPHAFRAGMENTOP2ATIPROC	qglAlphaFragmentOp2ATI;
extern	PFNGLALPHAFRAGMENTOP3ATIPROC	qglAlphaFragmentOp3ATI;
extern	PFNGLSETFRAGMENTSHADERCONSTANTATIPROC	qglSetFragmentShaderConstantATI;

// EXT_stencil_two_side
extern	PFNGLACTIVESTENCILFACEEXTPROC		qglActiveStencilFaceEXT;

// ATI_separate_stencil
extern	PFNGLSTENCILOPSEPARATEATIPROC		qglStencilOpSeparateATI;
extern	PFNGLSTENCILFUNCSEPARATEATIPROC		qglStencilFuncSeparateATI;

// ARB_texture_compression
extern	PFNGLCOMPRESSEDTEXIMAGE2DARBPROC	qglCompressedTexImage2DARB;
extern	PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC	qglCompressedTexSubImage2DARB;
extern	PFNGLGETCOMPRESSEDTEXIMAGEARBPROC	qglGetCompressedTexImageARB;

// ARB_vertex_program / ARB_fragment_program
extern PFNGLVERTEXATTRIBPOINTERARBPROC		qglVertexAttribPointerARB;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC	qglEnableVertexAttribArrayARB;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC	qglDisableVertexAttribArrayARB;
extern PFNGLVERTEXATTRIB3FVARBPROC			qglVertexAttrib3fvARB;
extern PFNGLVERTEXATTRIB4FVARBPROC			qglVertexAttrib4fvARB;
extern PFNGLGETPROGRAMIVARBPROC				qglGetProgramivARB;
extern PFNGLPROGRAMSTRINGARBPROC			qglProgramStringARB;
extern PFNGLBINDPROGRAMARBPROC				qglBindProgramARB;
extern PFNGLDELETEPROGRAMSARBPROC			qglDeleteProgramsARB;
extern PFNGLGENPROGRAMSARBPROC				qglGenProgramsARB;
extern PFNGLPROGRAMENVPARAMETER4FARBPROC	qglProgramEnvParameter4fARB;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC	qglProgramEnvParameter4fvARB;
extern PFNGLPROGRAMLOCALPARAMETER4FARBPROC	qglProgramLocalParameter4fARB;
extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC	qglProgramLocalParameter4fvARB;
extern PFNGLGETVERTEXATTRIBPOINTERVARBPROC	qglGetVertexAttribPointervARB;

// GLSL fragment
extern PFNGLCREATESHADEROBJECTARBPROC		qglCreateShaderObjectARB;
extern PFNGLDELETEOBJECTARBPROC				qglDeleteObjectARB;
extern PFNGLSHADERSOURCEARBPROC				qglShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC			qglCompileShaderARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC		qglGetObjectParameterivARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC		qglCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC				qglAttachObjectARB;
extern PFNGLDETACHOBJECTARBPROC				qglDetachObjectARB;
extern PFNGLLINKPROGRAMARBPROC				qglLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC			qglUseProgramObjectARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC		qglGetUniformLocationARB;
extern PFNGLUNIFORM1FARBPROC				qglUniform1fARB;
extern PFNGLUNIFORM1IARBPROC				qglUniform1iARB;
extern PFNGLUNIFORM1FVARBPROC				qglUniform1fvARB;
extern PFNGLUNIFORM2FVARBPROC				qglUniform2fvARB;
extern PFNGLUNIFORM3FVARBPROC				qglUniform3fvARB;
extern PFNGLUNIFORM4FVARBPROC				qglUniform4fvARB;
extern PFNGLGETINFOLOGARBPROC				qglGetInfoLogARB;

// GL_EXT_depth_bounds_test
extern PFNGLDEPTHBOUNDSEXTPROC              qglDepthBoundsEXT;

// GL_ARB_occlusion_query
extern PFNGLGENQUERIESARBPROC				qglGenQueriesARB;
extern PFNGLDELETEQUERIESARBPROC			qglDeleteQueriesARB;
extern PFNGLISQUERYARBPROC					qglIsQueryARB;
extern PFNGLBEGINQUERYARBPROC				qglBeginQueryARB;
extern PFNGLENDQUERYARBPROC					qglEndQueryARB;
extern PFNGLGETQUERYIVARBPROC				qglGetQueryivARB;
extern PFNGLGETQUERYOBJECTIVARBPROC			qglGetQueryObjectivARB;
extern PFNGLGETQUERYOBJECTUIVARBPROC		qglGetQueryObjectuivARB;

// GL_EXT_framebuffer_object
extern PFNGLISRENDERBUFFEREXTPROC						qglIsRenderbufferEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC						qglBindRenderbufferEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC					qglDeleteRenderbuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC						qglGenRenderbuffersEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC					qglRenderbufferStorageEXT;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC			qglGetRenderbufferParameterivEXT;
extern PFNGLISFRAMEBUFFEREXTPROC						qglIsFramebufferEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC						qglBindFramebufferEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC					qglDeleteFramebuffersEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC						qglGenFramebuffersEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC				qglCheckFramebufferStatusEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC					qglFramebufferTexture1DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC					qglFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC					qglFramebufferTexture3DEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC				qglFramebufferRenderbufferEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC	qglGetFramebufferAttachmentParameterivEXT;
extern PFNGLGENERATEMIPMAPEXTPROC						qglGenerateMipmapEXT;

// GL_EXT_blend_minmax
extern PFNGLBLENDEQUATIONEXTPROC			qglBlendEquationEXT;
extern	void ( APIENTRY *qglBlendEquationEXT)( GLenum mode );


// GL_ARB_multisample
extern PFNGLSAMPLECOVERAGEARBPROC			qglSampleCoverageARB;

// GL_ARB_shader_objects
extern PFNGLDELETEOBJECTARBPROC				qglDeleteObjectARB;
extern PFNGLGETHANDLEARBPROC				qglGetHandleARB;
extern PFNGLDETACHOBJECTARBPROC				qglDetachObjectARB;
extern PFNGLCREATESHADEROBJECTARBPROC		qglCreateShaderObjectARB;
extern PFNGLSHADERSOURCEARBPROC				qglShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC			qglCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC		qglCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC				qglAttachObjectARB;
extern PFNGLLINKPROGRAMARBPROC				qglLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC			qglUseProgramObjectARB;
extern PFNGLVALIDATEPROGRAMARBPROC			qglValidateProgramARB;
extern PFNGLUNIFORM1FARBPROC				qglUniform1fARB;
extern PFNGLUNIFORM2FARBPROC				qglUniform2fARB;
extern PFNGLUNIFORM3FARBPROC				qglUniform3fARB;
extern PFNGLUNIFORM4FARBPROC				qglUniform4fARB;
extern PFNGLUNIFORM1IARBPROC				qglUniform1iARB;
extern PFNGLUNIFORM2IARBPROC				qglUniform2iARB;
extern PFNGLUNIFORM3IARBPROC				qglUniform3iARB;
extern PFNGLUNIFORM4IARBPROC				qglUniform4iARB;
extern PFNGLUNIFORM1FVARBPROC				qglUniform1fvARB;
extern PFNGLUNIFORM2FVARBPROC				qglUniform2fvARB;
extern PFNGLUNIFORM3FVARBPROC				qglUniform3fvARB;
extern PFNGLUNIFORM4FVARBPROC				qglUniform4fvARB;
extern PFNGLUNIFORM1IVARBPROC				qglUniform1ivARB;
extern PFNGLUNIFORM2IVARBPROC				qglUniform2ivARB;
extern PFNGLUNIFORM3IVARBPROC				qglUniform3ivARB;
extern PFNGLUNIFORM4IVARBPROC				qglUniform4ivARB;
extern PFNGLUNIFORMMATRIX2FVARBPROC			qglUniformMatrix2fvARB;
extern PFNGLUNIFORMMATRIX3FVARBPROC			qglUniformMatrix3fvARB;
extern PFNGLUNIFORMMATRIX4FVARBPROC			qglUniformMatrix4fvARB;
extern PFNGLGETOBJECTPARAMETERFVARBPROC		qglGetObjectParameterfvARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC		qglGetObjectParameterivARB;
extern PFNGLGETINFOLOGARBPROC				qglGetInfoLogARB;
extern PFNGLGETATTACHEDOBJECTSARBPROC		qglGetAttachedObjectsARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC		qglGetUniformLocationARB;
extern PFNGLGETACTIVEUNIFORMARBPROC			qglGetActiveUniformARB;
extern PFNGLGETUNIFORMFVARBPROC				qglGetUniformfvARB;
extern PFNGLGETUNIFORMIVARBPROC				qglGetUniformivARB;
extern PFNGLGETSHADERSOURCEARBPROC			qglGetShaderSourceARB;

// GL_ARB_vertex_shader
extern PFNGLBINDATTRIBLOCATIONARBPROC		qglBindAttribLocationARB;
extern PFNGLGETACTIVEATTRIBARBPROC			qglGetActiveAttribARB;
extern PFNGLGETATTRIBLOCATIONARBPROC		qglGetAttribLocationARB;

// GL_EXT_gpu_program_parameters
extern PFNGLPROGRAMENVPARAMETERS4FVEXTPROC		qglProgramEnvParameters4fvEXT;
extern PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC	qglProgramLocalParameters4fvEXT;

// GL_EXT_timer_query
extern PFNGLGETQUERYOBJECTI64VEXTPROC		qglGetQueryObjecti64vEXT;
extern PFNGLGETQUERYOBJECTUI64VEXTPROC		qglGetQueryObjectui64vEXT;

// GREMEDY_string_marker
extern PFNGLSTRINGMARKERGREMEDYPROC			qglStringMarkerGREMEDY;
extern PFNGLSECONDARYCOLOR3BPROC			qglSecondaryColor3b;
//===========================================================================

// windows systems use a function pointer for each call so we can do our log file intercepts

extern  void ( APIENTRY * qglAccum )( GLenum op, GLfloat value);
extern  void ( APIENTRY * qglAlphaFunc )( GLenum func, GLclampf ref);
extern  GLboolean ( APIENTRY * qglAreTexturesResident )( GLsizei n, const GLuint *textures, GLboolean *residences);
extern  void ( APIENTRY * qglArrayElement )( GLint i);
extern  void ( APIENTRY * qglBegin )( GLenum mode);
extern  void ( APIENTRY * qglBindTexture )( GLenum target, GLuint texture);
extern  void ( APIENTRY * qglBitmap )( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern  void ( APIENTRY * qglBlendFunc )( GLenum sfactor, GLenum dfactor);
extern  void ( APIENTRY * qglCallList )( GLuint list);
extern  void ( APIENTRY * qglCallLists )( GLsizei n, GLenum type, const GLvoid *lists);
extern  void ( APIENTRY * qglClear )( GLbitfield mask);
extern  void ( APIENTRY * qglClearAccum )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern  void ( APIENTRY * qglClearColor )( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern  void ( APIENTRY * qglClearDepth )( GLclampd depth);
extern  void ( APIENTRY * qglClearIndex )( GLfloat c);
extern  void ( APIENTRY * qglClearStencil )( GLint s);
extern  void ( APIENTRY * qglClipPlane )( GLenum plane, const GLdouble *equation);
extern  void ( APIENTRY * qglColor3b )( GLbyte red, GLbyte green, GLbyte blue);
extern  void ( APIENTRY * qglColor3bv )(const GLbyte *v);
extern  void ( APIENTRY * qglColor3d )( GLdouble red, GLdouble green, GLdouble blue);
extern  void ( APIENTRY * qglColor3dv )(const GLdouble *v);
extern  void ( APIENTRY * qglColor3f )( GLfloat red, GLfloat green, GLfloat blue);
extern  void ( APIENTRY * qglColor3fv )(const GLfloat *v);
extern  void ( APIENTRY * qglColor3i )( GLint red, GLint green, GLint blue);
extern  void ( APIENTRY * qglColor3iv )(const GLint *v);
extern  void ( APIENTRY * qglColor3s )( GLshort red, GLshort green, GLshort blue);
extern  void ( APIENTRY * qglColor3sv )(const GLshort *v);
extern  void ( APIENTRY * qglColor3ub )( GLubyte red, GLubyte green, GLubyte blue);
extern  void ( APIENTRY * qglColor3ubv )(const GLubyte *v);
extern  void ( APIENTRY * qglColor3ui )( GLuint red, GLuint green, GLuint blue);
extern  void ( APIENTRY * qglColor3uiv )(const GLuint *v);
extern  void ( APIENTRY * qglColor3us )( GLushort red, GLushort green, GLushort blue);
extern  void ( APIENTRY * qglColor3usv )(const GLushort *v);
extern  void ( APIENTRY * qglColor4b )( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern  void ( APIENTRY * qglColor4bv )(const GLbyte *v);
extern  void ( APIENTRY * qglColor4d )( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern  void ( APIENTRY * qglColor4dv )(const GLdouble *v);
extern  void ( APIENTRY * qglColor4f )( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern  void ( APIENTRY * qglColor4fv )(const GLfloat *v);
extern  void ( APIENTRY * qglColor4i )( GLint red, GLint green, GLint blue, GLint alpha);
extern  void ( APIENTRY * qglColor4iv )(const GLint *v);
extern  void ( APIENTRY * qglColor4s )( GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern  void ( APIENTRY * qglColor4sv )(const GLshort *v);
extern  void ( APIENTRY * qglColor4ub )( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern  void ( APIENTRY * qglColor4ubv )(const GLubyte *v);
extern  void ( APIENTRY * qglColor4ui )( GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern  void ( APIENTRY * qglColor4uiv )(const GLuint *v);
extern  void ( APIENTRY * qglColor4us )( GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern  void ( APIENTRY * qglColor4usv )(const GLushort *v);
extern  void ( APIENTRY * qglColorMask )( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern  void ( APIENTRY * qglColorMaterial )( GLenum face, GLenum mode);
extern  void ( APIENTRY * qglColorPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglCopyPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern  void ( APIENTRY * qglCopyTexImage1D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern  void ( APIENTRY * qglCopyTexImage2D )( GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern  void ( APIENTRY * qglCopyTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern  void ( APIENTRY * qglCopyTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern  void ( APIENTRY * qglCullFace )( GLenum mode);
extern  void ( APIENTRY * qglDeleteLists )( GLuint list, GLsizei range);
extern  void ( APIENTRY * qglDeleteTextures )( GLsizei n, const GLuint *textures);
extern  void ( APIENTRY * qglDepthFunc )( GLenum func);
extern  void ( APIENTRY * qglDepthMask )( GLboolean flag);
extern  void ( APIENTRY * qglDepthRange )( GLclampd zNear, GLclampd zFar);
extern  void ( APIENTRY * qglDisable )( GLenum cap);
extern  void ( APIENTRY * qglDisableClientState )( GLenum array);
extern  void ( APIENTRY * qglDrawArrays )( GLenum mode, GLint first, GLsizei count);
extern  void ( APIENTRY * qglDrawBuffer )( GLenum mode);
extern  void ( APIENTRY * qglDrawElements )( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern  void ( APIENTRY * qglDrawPixels )( GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( APIENTRY * qglEdgeFlag )( GLboolean flag);
extern  void ( APIENTRY * qglEdgeFlagPointer )( GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglEdgeFlagv )(const GLboolean *flag);
extern  void ( APIENTRY * qglEnable )( GLenum cap);
extern  void ( APIENTRY * qglEnableClientState )( GLenum array);
extern  void ( APIENTRY * qglEnd )( void );
extern  void ( APIENTRY * qglEndList )( void );
extern  void ( APIENTRY * qglEvalCoord1d )( GLdouble u);
extern  void ( APIENTRY * qglEvalCoord1dv )(const GLdouble *u);
extern  void ( APIENTRY * qglEvalCoord1f )( GLfloat u);
extern  void ( APIENTRY * qglEvalCoord1fv )(const GLfloat *u);
extern  void ( APIENTRY * qglEvalCoord2d )( GLdouble u, GLdouble v);
extern  void ( APIENTRY * qglEvalCoord2dv )(const GLdouble *u);
extern  void ( APIENTRY * qglEvalCoord2f )( GLfloat u, GLfloat v);
extern  void ( APIENTRY * qglEvalCoord2fv )(const GLfloat *u);
extern  void ( APIENTRY * qglEvalMesh1 )( GLenum mode, GLint i1, GLint i2);
extern  void ( APIENTRY * qglEvalMesh2 )( GLenum mode, GLint i1, GLint i2, GLint ( j + 1 ), GLint j2);
extern  void ( APIENTRY * qglEvalPoint1 )( GLint i);
extern  void ( APIENTRY * qglEvalPoint2 )( GLint i, GLint j);
extern  void ( APIENTRY * qglFeedbackBuffer )( GLsizei size, GLenum type, GLfloat *buffer);
extern  void ( APIENTRY * qglFinish )( void );
extern  void ( APIENTRY * qglFlush )( void );
extern  void ( APIENTRY * qglFogf )( GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglFogfv )( GLenum pname, const GLfloat *params);
extern  void ( APIENTRY * qglFogi )( GLenum pname, GLint param);
extern  void ( APIENTRY * qglFogiv )( GLenum pname, const GLint *params);
extern  void ( APIENTRY * qglFrontFace )( GLenum mode);
extern  void ( APIENTRY * qglFrustum )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern  GLuint ( APIENTRY * qglGenLists )( GLsizei range);
extern  void ( APIENTRY * qglGenTextures )( GLsizei n, GLuint *textures);
extern  void ( APIENTRY * qglGetBooleanv )( GLenum pname, GLboolean *params);
extern  void ( APIENTRY * qglGetClipPlane )( GLenum plane, GLdouble *equation);
extern  void ( APIENTRY * qglGetDoublev )( GLenum pname, GLdouble *params);
extern  GLenum ( APIENTRY * qglGetError )( void );
extern  void ( APIENTRY * qglGetFloatv )( GLenum pname, GLfloat *params);
extern  void ( APIENTRY * qglGetIntegerv )( GLenum pname, GLint *params);
extern  void ( APIENTRY * qglGetLightfv )( GLenum light, GLenum pname, GLfloat *params);
extern  void ( APIENTRY * qglGetLightiv )( GLenum light, GLenum pname, GLint *params);
extern  void ( APIENTRY * qglGetMapdv )( GLenum target, GLenum query, GLdouble *v);
extern  void ( APIENTRY * qglGetMapfv )( GLenum target, GLenum query, GLfloat *v);
extern  void ( APIENTRY * qglGetMapiv )( GLenum target, GLenum query, GLint *v);
extern  void ( APIENTRY * qglGetMaterialfv )( GLenum face, GLenum pname, GLfloat *params);
extern  void ( APIENTRY * qglGetMaterialiv )( GLenum face, GLenum pname, GLint *params);
extern  void ( APIENTRY * qglGetPixelMapfv )( GLenum map, GLfloat *values);
extern  void ( APIENTRY * qglGetPixelMapuiv )( GLenum map, GLuint *values);
extern  void ( APIENTRY * qglGetPixelMapusv )( GLenum map, GLushort *values);
extern  void ( APIENTRY * qglGetPointerv )( GLenum pname, GLvoid* *params);
extern  void ( APIENTRY * qglGetPolygonStipple )( GLubyte *mask);
extern  const GLubyte * ( APIENTRY * qglGetString )( GLenum name);
extern  void ( APIENTRY * qglGetTexEnvfv )( GLenum target, GLenum pname, GLfloat *params);
extern  void ( APIENTRY * qglGetTexEnviv )( GLenum target, GLenum pname, GLint *params);
extern  void ( APIENTRY * qglGetTexGendv )( GLenum coord, GLenum pname, GLdouble *params);
extern  void ( APIENTRY * qglGetTexGenfv )( GLenum coord, GLenum pname, GLfloat *params);
extern  void ( APIENTRY * qglGetTexGeniv )( GLenum coord, GLenum pname, GLint *params);
extern  void ( APIENTRY * qglGetTexImage )( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern  void ( APIENTRY * qglGetTexLevelParameterfv )( GLenum target, GLint level, GLenum pname, GLfloat *params);
extern  void ( APIENTRY * qglGetTexLevelParameteriv )( GLenum target, GLint level, GLenum pname, GLint *params);
extern  void ( APIENTRY * qglGetTexParameterfv )( GLenum target, GLenum pname, GLfloat *params);
extern  void ( APIENTRY * qglGetTexParameteriv )( GLenum target, GLenum pname, GLint *params);
extern  void ( APIENTRY * qglHint )( GLenum target, GLenum mode);
extern  void ( APIENTRY * qglIndexMask )( GLuint mask);
extern  void ( APIENTRY * qglIndexPointer )( GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglIndexd )( GLdouble c);
extern  void ( APIENTRY * qglIndexdv )(const GLdouble *c);
extern  void ( APIENTRY * qglIndexf )( GLfloat c);
extern  void ( APIENTRY * qglIndexfv )(const GLfloat *c);
extern  void ( APIENTRY * qglIndexi )( GLint c);
extern  void ( APIENTRY * qglIndexiv )(const GLint *c);
extern  void ( APIENTRY * qglIndexs )( GLshort c);
extern  void ( APIENTRY * qglIndexsv )(const GLshort *c);
extern  void ( APIENTRY * qglIndexub )( GLubyte c);
extern  void ( APIENTRY * qglIndexubv )(const GLubyte *c);
extern  void ( APIENTRY * qglInitNames )( void );
extern  void ( APIENTRY * qglInterleavedArrays )( GLenum format, GLsizei stride, const GLvoid *pointer);
extern  GLboolean ( APIENTRY * qglIsEnabled )( GLenum cap);
extern  GLboolean ( APIENTRY * qglIsList )( GLuint list);
extern  GLboolean ( APIENTRY * qglIsTexture )( GLuint texture);
extern  void ( APIENTRY * qglLightModelf )( GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglLightModelfv )( GLenum pname, const GLfloat *params);
extern  void ( APIENTRY * qglLightModeli )( GLenum pname, GLint param);
extern  void ( APIENTRY * qglLightModeliv )( GLenum pname, const GLint *params);
extern  void ( APIENTRY * qglLightf )( GLenum light, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglLightfv )( GLenum light, GLenum pname, const GLfloat *params);
extern  void ( APIENTRY * qglLighti )( GLenum light, GLenum pname, GLint param);
extern  void ( APIENTRY * qglLightiv )( GLenum light, GLenum pname, const GLint *params);
extern  void ( APIENTRY * qglLineStipple )( GLint factor, GLushort pattern);
extern  void ( APIENTRY * qglLineWidth )( GLfloat width);
extern  void ( APIENTRY * qglListBase )( GLuint base);
extern  void ( APIENTRY * qglLoadIdentity )( void );
extern  void ( APIENTRY * qglLoadMatrixd )(const GLdouble *m);
extern  void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
extern  void ( APIENTRY * qglLoadName )( GLuint name);
extern  void ( APIENTRY * qglLogicOp )( GLenum opcode);
extern  void ( APIENTRY * qglMap1d )( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern  void ( APIENTRY * qglMap1f )( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern  void ( APIENTRY * qglMap2d )( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern  void ( APIENTRY * qglMap2f )( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern  void ( APIENTRY * qglMapGrid1d )( GLint un, GLdouble u1, GLdouble u2);
extern  void ( APIENTRY * qglMapGrid1f )( GLint un, GLfloat u1, GLfloat u2);
extern  void ( APIENTRY * qglMapGrid2d )( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern  void ( APIENTRY * qglMapGrid2f )( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern  void ( APIENTRY * qglMaterialf )( GLenum face, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglMaterialfv )( GLenum face, GLenum pname, const GLfloat *params);
extern  void ( APIENTRY * qglMateriali )( GLenum face, GLenum pname, GLint param);
extern  void ( APIENTRY * qglMaterialiv )( GLenum face, GLenum pname, const GLint *params);
extern  void ( APIENTRY * qglMatrixMode )( GLenum mode);
extern  void ( APIENTRY * qglMultMatrixd )(const GLdouble *m);
extern  void ( APIENTRY * qglMultMatrixf )(const GLfloat *m);
extern  void ( APIENTRY * qglNewList )( GLuint list, GLenum mode);
extern  void ( APIENTRY * qglNormal3b )( GLbyte nx, GLbyte ny, GLbyte nz);
extern  void ( APIENTRY * qglNormal3bv )(const GLbyte *v);
extern  void ( APIENTRY * qglNormal3d )( GLdouble nx, GLdouble ny, GLdouble nz);
extern  void ( APIENTRY * qglNormal3dv )(const GLdouble *v);
extern  void ( APIENTRY * qglNormal3f )( GLfloat nx, GLfloat ny, GLfloat nz);
extern  void ( APIENTRY * qglNormal3fv )(const GLfloat *v);
extern  void ( APIENTRY * qglNormal3i )( GLint nx, GLint ny, GLint nz);
extern  void ( APIENTRY * qglNormal3iv )(const GLint *v);
extern  void ( APIENTRY * qglNormal3s )( GLshort nx, GLshort ny, GLshort nz);
extern  void ( APIENTRY * qglNormal3sv )(const GLshort *v);
extern  void ( APIENTRY * qglNormalPointer )( GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglOrtho )( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern  void ( APIENTRY * qglPassThrough )( GLfloat token);
extern  void ( APIENTRY * qglPixelMapfv )( GLenum map, GLsizei mapsize, const GLfloat *values);
extern  void ( APIENTRY * qglPixelMapuiv )( GLenum map, GLsizei mapsize, const GLuint *values);
extern  void ( APIENTRY * qglPixelMapusv )( GLenum map, GLsizei mapsize, const GLushort *values);
extern  void ( APIENTRY * qglPixelStoref )( GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglPixelStorei )( GLenum pname, GLint param);
extern  void ( APIENTRY * qglPixelTransferf )( GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglPixelTransferi )( GLenum pname, GLint param);
extern  void ( APIENTRY * qglPixelZoom )( GLfloat xfactor, GLfloat yfactor);
extern  void ( APIENTRY * qglPointSize )( GLfloat size);
extern  void ( APIENTRY * qglPolygonMode )( GLenum face, GLenum mode);
extern  void ( APIENTRY * qglPolygonOffset )( GLfloat factor, GLfloat units);
extern  void ( APIENTRY * qglPolygonStipple )(const GLubyte *mask);
extern  void ( APIENTRY * qglPopAttrib )( void );
extern  void ( APIENTRY * qglPopClientAttrib )( void );
extern  void ( APIENTRY * qglPopMatrix )( void );
extern  void ( APIENTRY * qglPopName )( void );
extern  void ( APIENTRY * qglPrioritizeTextures )( GLsizei n, const GLuint *textures, const GLclampf *priorities);
extern  void ( APIENTRY * qglPushAttrib )( GLbitfield mask);
extern  void ( APIENTRY * qglPushClientAttrib )( GLbitfield mask);
extern  void ( APIENTRY * qglPushMatrix )( void );
extern  void ( APIENTRY * qglPushName )( GLuint name);
extern  void ( APIENTRY * qglRasterPos2d )( GLdouble x, GLdouble y);
extern  void ( APIENTRY * qglRasterPos2dv )(const GLdouble *v);
extern  void ( APIENTRY * qglRasterPos2f )( GLfloat x, GLfloat y);
extern  void ( APIENTRY * qglRasterPos2fv )(const GLfloat *v);
extern  void ( APIENTRY * qglRasterPos2i )( GLint x, GLint y);
extern  void ( APIENTRY * qglRasterPos2iv )(const GLint *v);
extern  void ( APIENTRY * qglRasterPos2s )( GLshort x, GLshort y);
extern  void ( APIENTRY * qglRasterPos2sv )(const GLshort *v);
extern  void ( APIENTRY * qglRasterPos3d )( GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglRasterPos3dv )(const GLdouble *v);
extern  void ( APIENTRY * qglRasterPos3f )( GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglRasterPos3fv )(const GLfloat *v);
extern  void ( APIENTRY * qglRasterPos3i )( GLint x, GLint y, GLint z);
extern  void ( APIENTRY * qglRasterPos3iv )(const GLint *v);
extern  void ( APIENTRY * qglRasterPos3s )( GLshort x, GLshort y, GLshort z);
extern  void ( APIENTRY * qglRasterPos3sv )(const GLshort *v);
extern  void ( APIENTRY * qglRasterPos4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern  void ( APIENTRY * qglRasterPos4dv )(const GLdouble *v);
extern  void ( APIENTRY * qglRasterPos4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern  void ( APIENTRY * qglRasterPos4fv )(const GLfloat *v);
extern  void ( APIENTRY * qglRasterPos4i )( GLint x, GLint y, GLint z, GLint w);
extern  void ( APIENTRY * qglRasterPos4iv )(const GLint *v);
extern  void ( APIENTRY * qglRasterPos4s )( GLshort x, GLshort y, GLshort z, GLshort w);
extern  void ( APIENTRY * qglRasterPos4sv )(const GLshort *v);
extern  void ( APIENTRY * qglReadBuffer )( GLenum mode);
extern  void ( APIENTRY * qglReadPixels )( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern  void ( APIENTRY * qglRectd )( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern  void ( APIENTRY * qglRectdv )(const GLdouble *v1, const GLdouble *v2);
extern  void ( APIENTRY * qglRectf )( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern  void ( APIENTRY * qglRectfv )(const GLfloat *v1, const GLfloat *v2);
extern  void ( APIENTRY * qglRecti )( GLint x1, GLint y1, GLint x2, GLint y2);
extern  void ( APIENTRY * qglRectiv )(const GLint *v1, const GLint *v2);
extern  void ( APIENTRY * qglRects )( GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern  void ( APIENTRY * qglRectsv )(const GLshort *v1, const GLshort *v2);
extern  GLint ( APIENTRY * qglRenderMode )( GLenum mode);
extern  void ( APIENTRY * qglRotated )( GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglRotatef )( GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglScaled )( GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglScalef )( GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglScissor )( GLint x, GLint y, GLsizei width, GLsizei height);
extern  void ( APIENTRY * qglSelectBuffer )( GLsizei size, GLuint *buffer);
extern  void ( APIENTRY * qglShadeModel )( GLenum mode);
extern  void ( APIENTRY * qglStencilFunc )( GLenum func, GLint ref, GLuint mask);
extern  void ( APIENTRY * qglStencilMask )( GLuint mask);
extern  void ( APIENTRY * qglStencilOp )( GLenum fail, GLenum zfail, GLenum zpass);
extern  void ( APIENTRY * qglTexCoord1d )( GLdouble s);
extern  void ( APIENTRY * qglTexCoord1dv )(const GLdouble *v);
extern  void ( APIENTRY * qglTexCoord1f )( GLfloat s);
extern  void ( APIENTRY * qglTexCoord1fv )(const GLfloat *v);
extern  void ( APIENTRY * qglTexCoord1i )( GLint s);
extern  void ( APIENTRY * qglTexCoord1iv )(const GLint *v);
extern  void ( APIENTRY * qglTexCoord1s )( GLshort s);
extern  void ( APIENTRY * qglTexCoord1sv )(const GLshort *v);
extern  void ( APIENTRY * qglTexCoord2d )( GLdouble s, GLdouble t);
extern  void ( APIENTRY * qglTexCoord2dv )(const GLdouble *v);
extern  void ( APIENTRY * qglTexCoord2f )( GLfloat s, GLfloat t);
extern  void ( APIENTRY * qglTexCoord2fv )(const GLfloat *v);
extern  void ( APIENTRY * qglTexCoord2i )( GLint s, GLint t);
extern  void ( APIENTRY * qglTexCoord2iv )(const GLint *v);
extern  void ( APIENTRY * qglTexCoord2s )( GLshort s, GLshort t);
extern  void ( APIENTRY * qglTexCoord2sv )(const GLshort *v);
extern  void ( APIENTRY * qglTexCoord3d )( GLdouble s, GLdouble t, GLdouble r);
extern  void ( APIENTRY * qglTexCoord3dv )(const GLdouble *v);
extern  void ( APIENTRY * qglTexCoord3f )( GLfloat s, GLfloat t, GLfloat r);
extern  void ( APIENTRY * qglTexCoord3fv )(const GLfloat *v);
extern  void ( APIENTRY * qglTexCoord3i )( GLint s, GLint t, GLint r);
extern  void ( APIENTRY * qglTexCoord3iv )(const GLint *v);
extern  void ( APIENTRY * qglTexCoord3s )( GLshort s, GLshort t, GLshort r);
extern  void ( APIENTRY * qglTexCoord3sv )(const GLshort *v);
extern  void ( APIENTRY * qglTexCoord4d )( GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern  void ( APIENTRY * qglTexCoord4dv )(const GLdouble *v);
extern  void ( APIENTRY * qglTexCoord4f )( GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern  void ( APIENTRY * qglTexCoord4fv )(const GLfloat *v);
extern  void ( APIENTRY * qglTexCoord4i )( GLint s, GLint t, GLint r, GLint q);
extern  void ( APIENTRY * qglTexCoord4iv )(const GLint *v);
extern  void ( APIENTRY * qglTexCoord4s )( GLshort s, GLshort t, GLshort r, GLshort q);
extern  void ( APIENTRY * qglTexCoord4sv )(const GLshort *v);
extern  void ( APIENTRY * qglTexCoordPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglTexEnvf )( GLenum target, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglTexEnvfv )( GLenum target, GLenum pname, const GLfloat *params);
extern  void ( APIENTRY * qglTexEnvi )( GLenum target, GLenum pname, GLint param);
extern  void ( APIENTRY * qglTexEnviv )( GLenum target, GLenum pname, const GLint *params);
extern  void ( APIENTRY * qglTexGend )( GLenum coord, GLenum pname, GLdouble param);
extern  void ( APIENTRY * qglTexGendv )( GLenum coord, GLenum pname, const GLdouble *params);
extern  void ( APIENTRY * qglTexGenf )( GLenum coord, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglTexGenfv )( GLenum coord, GLenum pname, const GLfloat *params);
extern  void ( APIENTRY * qglTexGeni )( GLenum coord, GLenum pname, GLint param);
extern  void ( APIENTRY * qglTexGeniv )( GLenum coord, GLenum pname, const GLint *params);
extern  void ( APIENTRY * qglTexImage1D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( APIENTRY * qglTexImage2D )( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( APIENTRY * qglTexParameterf )( GLenum target, GLenum pname, GLfloat param);
extern  void ( APIENTRY * qglTexParameterfv )( GLenum target, GLenum pname, const GLfloat *params);
extern  void ( APIENTRY * qglTexParameteri )( GLenum target, GLenum pname, GLint param);
extern  void ( APIENTRY * qglTexParameteriv )( GLenum target, GLenum pname, const GLint *params);
extern  void ( APIENTRY * qglTexSubImage1D )( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( APIENTRY * qglTexSubImage2D )( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern  void ( APIENTRY * qglTranslated )( GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglTranslatef )( GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglVertex2d )( GLdouble x, GLdouble y);
extern  void ( APIENTRY * qglVertex2dv )(const GLdouble *v);
extern  void ( APIENTRY * qglVertex2f )( GLfloat x, GLfloat y);
extern  void ( APIENTRY * qglVertex2fv )(const GLfloat *v);
extern  void ( APIENTRY * qglVertex2i )( GLint x, GLint y);
extern  void ( APIENTRY * qglVertex2iv )(const GLint *v);
extern  void ( APIENTRY * qglVertex2s )( GLshort x, GLshort y);
extern  void ( APIENTRY * qglVertex2sv )(const GLshort *v);
extern  void ( APIENTRY * qglVertex3d )( GLdouble x, GLdouble y, GLdouble z);
extern  void ( APIENTRY * qglVertex3dv )(const GLdouble *v);
extern  void ( APIENTRY * qglVertex3f )( GLfloat x, GLfloat y, GLfloat z);
extern  void ( APIENTRY * qglVertex3fv )(const GLfloat *v);
extern  void ( APIENTRY * qglVertex3i )( GLint x, GLint y, GLint z);
extern  void ( APIENTRY * qglVertex3iv )(const GLint *v);
extern  void ( APIENTRY * qglVertex3s )( GLshort x, GLshort y, GLshort z);
extern  void ( APIENTRY * qglVertex3sv )(const GLshort *v);
extern  void ( APIENTRY * qglVertex4d )( GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern  void ( APIENTRY * qglVertex4dv )(const GLdouble *v);
extern  void ( APIENTRY * qglVertex4f )( GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern  void ( APIENTRY * qglVertex4fv )(const GLfloat *v);
extern  void ( APIENTRY * qglVertex4i )( GLint x, GLint y, GLint z, GLint w);
extern  void ( APIENTRY * qglVertex4iv )(const GLint *v);
extern  void ( APIENTRY * qglVertex4s )( GLshort x, GLshort y, GLshort z, GLshort w);
extern  void ( APIENTRY * qglVertex4sv )(const GLshort *v);
extern  void ( APIENTRY * qglVertexPointer )( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern  void ( APIENTRY * qglViewport )( GLint x, GLint y, GLsizei width, GLsizei height);

extern void ( APIENTRY * qglPointParameterfEXT )( GLenum param, GLfloat value );
extern void ( APIENTRY * qglPointParameterfvEXT )( GLenum param, const GLfloat *value );
extern void ( APIENTRY * qglColorTableEXT )( int, int, int, int, int, const void * );

extern void ( APIENTRY * qglMTexCoord2fSGIS )( GLenum, GLfloat, GLfloat );
extern void ( APIENTRY * qglSelectTextureSGIS )( GLenum );

extern void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
extern void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );
extern void ( APIENTRY * qglMultiTexCoord1dARB )( GLenum target, GLdouble s );
extern void ( APIENTRY * qglMultiTexCoord1dvARB )( GLenum target, const GLdouble *v );
extern void ( APIENTRY * qglMultiTexCoord1fARB )( GLenum target, GLfloat s );
extern void ( APIENTRY * qglMultiTexCoord1fvARB )( GLenum target, const GLfloat *v );
extern void ( APIENTRY * qglMultiTexCoord1iARB )( GLenum target, GLint s );
extern void ( APIENTRY * qglMultiTexCoord1ivARB )( GLenum target, const GLint *v );
extern void ( APIENTRY * qglMultiTexCoord1sARB )( GLenum target, GLshort s );
extern void ( APIENTRY * qglMultiTexCoord1svARB )( GLenum target, const GLshort *v );
extern void ( APIENTRY * qglMultiTexCoord2dARB )( GLenum target, GLdouble s );
extern void ( APIENTRY * qglMultiTexCoord2dvARB )( GLenum target, const GLdouble *v );
extern void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum target, GLfloat s );
extern void ( APIENTRY * qglMultiTexCoord2fvARB )( GLenum target, const GLfloat *v );
extern void ( APIENTRY * qglMultiTexCoord2iARB )( GLenum target, GLint s );
extern void ( APIENTRY * qglMultiTexCoord2ivARB )( GLenum target, const GLint *v );
extern void ( APIENTRY * qglMultiTexCoord2sARB )( GLenum target, GLshort s );
extern void ( APIENTRY * qglMultiTexCoord2svARB )( GLenum target, const GLshort *v );
extern void ( APIENTRY * qglMultiTexCoord3dARB )( GLenum target, GLdouble s );
extern void ( APIENTRY * qglMultiTexCoord3dvARB )( GLenum target, const GLdouble *v );
extern void ( APIENTRY * qglMultiTexCoord3fARB )( GLenum target, GLfloat s );
extern void ( APIENTRY * qglMultiTexCoord3fvARB )( GLenum target, const GLfloat *v );
extern void ( APIENTRY * qglMultiTexCoord3iARB )( GLenum target, GLint s );
extern void ( APIENTRY * qglMultiTexCoord3ivARB )( GLenum target, const GLint *v );
extern void ( APIENTRY * qglMultiTexCoord3sARB )( GLenum target, GLshort s );
extern void ( APIENTRY * qglMultiTexCoord3svARB )( GLenum target, const GLshort *v );
extern void ( APIENTRY * qglMultiTexCoord4dARB )( GLenum target, GLdouble s );
extern void ( APIENTRY * qglMultiTexCoord4dvARB )( GLenum target, const GLdouble *v );
extern void ( APIENTRY * qglMultiTexCoord4fARB )( GLenum target, GLfloat s );
extern void ( APIENTRY * qglMultiTexCoord4fvARB )( GLenum target, const GLfloat *v );
extern void ( APIENTRY * qglMultiTexCoord4iARB )( GLenum target, GLint s );
extern void ( APIENTRY * qglMultiTexCoord4ivARB )( GLenum target, const GLint *v );
extern void ( APIENTRY * qglMultiTexCoord4sARB )( GLenum target, GLshort s );
extern void ( APIENTRY * qglMultiTexCoord4svARB )( GLenum target, const GLshort *v );

//#if defined( _WIN32 )
typedef struct tagPIXELFORMATDESCRIPTOR PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, FAR *LPPIXELFORMATDESCRIPTOR;

extern  int   ( WINAPI * qwglChoosePixelFormat )(HDC, CONST PIXELFORMATDESCRIPTOR *);
extern  int   ( WINAPI * qwglDescribePixelFormat)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
extern  int   ( WINAPI * qwglGetPixelFormat)(HDC);
extern  bool  ( WINAPI * qwglSetPixelFormat)(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
extern  bool  ( WINAPI * qwglSwapBuffers)(HDC);

typedef struct HPBUFFERARB__ *HPBUFFERARB;
extern bool  ( WINAPI * qwglBindTexImageARB)(HPBUFFERARB, int);
extern bool	 ( WINAPI * qwglChoosePixelFormatARB)(HDC hdc, const int * RESTRICT piAttribIList, const FLOAT * RESTRICT pfAttribFList, UINT nMaxFormats, int * RESTRICT piFormats, UINT * RESTRICT nNumFormats);
extern HPBUFFERARB ( WINAPI * qwglCreatePbufferARB)(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int * RESTRICT piAttribList);
extern bool  ( WINAPI * qwglDestroyPbufferARB)(HPBUFFERARB);
extern HDC	 ( WINAPI * qwglGetPbufferDCARB)(HPBUFFERARB);
extern int	 ( WINAPI * qwglReleasePbufferDCARB)(HPBUFFERARB, HDC);
extern bool  ( WINAPI * qwglReleaseTexImageARB)(HPBUFFERARB, int);
extern bool  ( WINAPI * qwglSetPbufferAttribARB)(HPBUFFERARB, const int * RESTRICT );

extern bool  ( WINAPI * qwglCopyContext)(HGLRC, HGLRC, UINT);
extern HGLRC ( WINAPI * qwglCreateContext)(HDC);
extern HGLRC ( WINAPI * qwglCreateLayerContext)(HDC, int);
extern bool  ( WINAPI * qwglDeleteContext)(HGLRC);
extern HGLRC ( WINAPI * qwglGetCurrentContext)( void );
extern HDC   ( WINAPI * qwglGetCurrentDC)( void );
extern PROC  ( WINAPI * qwglGetProcAddress)(LPCSTR);
extern bool  ( WINAPI * qwglMakeCurrent)(HDC, HGLRC);
extern bool  ( WINAPI * qwglShareLists)(HGLRC, HGLRC);
extern bool  ( WINAPI * qwglUseFontBitmaps)(HDC, DWORD, DWORD, DWORD);

extern bool  ( WINAPI * qwglUseFontOutlines)(HDC, DWORD, DWORD, DWORD, FLOAT,FLOAT, int, LPGLYPHMETRICSFLOAT);

extern bool ( WINAPI * qwglDescribeLayerPlane)(HDC, int, int, UINT,LPLAYERPLANEDESCRIPTOR);
extern int  ( WINAPI * qwglSetLayerPaletteEntries)(HDC, int, int, int,CONST COLORREF *);
extern int  ( WINAPI * qwglGetLayerPaletteEntries)(HDC, int, int, int,COLORREF *);
extern bool ( WINAPI * qwglRealizeLayerPalette)(HDC, int, bool);
extern bool ( WINAPI * qwglSwapLayerBuffers )( HDC, UINT );

extern bool ( WINAPI * qwglSwapIntervalEXT )( int interval );

extern bool ( WINAPI * qwglGetDeviceGammaRampEXT )( unsigned char *pRed, unsigned char *pGreen, unsigned char *pBlue );
extern bool ( WINAPI * qwglSetDeviceGammaRampEXT )( const unsigned char *pRed, const unsigned char *pGreen, const unsigned char *pBlue );

// GLX Functions
extern XVisualInfo * (*qglXChooseVisual)( Display *dpy, int screen, int *attribList );
extern GLXContext (*qglXCreateContext)( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
extern void (*qglXDestroyContext)( Display *dpy, GLXContext ctx );
extern Bool (*qglXMakeCurrent)( Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern void (*qglXSwapBuffers)( Display *dpy, GLXDrawable drawable );

extern GLXPixmap ( *qglXCreateGLXPixmap )( Display *dpy, XVisualInfo *visual, Pixmap pixmap );
extern void ( *qglXDestroyGLXPixmap )( Display *dpy, GLXPixmap pixmap );
extern Bool ( *qglXQueryExtension )( Display *dpy, int *errorb, int *event );
extern Bool ( *qglXQueryVersion )( Display *dpy, int *maj, int *min );
extern Bool ( *qglXIsDirect )( Display *dpy, GLXContext ctx );
extern int ( *qglXGetConfig )( Display *dpy, XVisualInfo *visual, int attrib, int *value );
extern GLXContext ( *qglXGetCurrentContext )( void );
extern GLXDrawable ( *qglXGetCurrentDrawable )( void );
extern void ( *qglXWaitGL )( void );
extern void ( *qglXWaitX )( void );
extern void ( *qglXUseXFont )( Font font, int first, int count, int list );

extern GLExtension_t (*qglXGetProcAddressARB)( const GLubyte *procname );
// make sure the code is correctly using qgl everywhere
// don't enable that when building glimp itself obviously..
#if !defined( GLIMP )
	#include "../sys/linux/qgl_enforce.h"
#endif

extern void ( APIENTRY *qgluPerspective )( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar );
extern void ( APIENTRY *qgluLookAt )( GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz );
extern const GLubyte * ( APIENTRY * qgluErrorString )(GLenum errCode );

extern void ( GLAPIENTRY *qgluBeginCurve)(GLUnurbs *nurb);
extern void ( GLAPIENTRY *qgluBeginPolygon)( GLUtesselator *tess);
extern void ( GLAPIENTRY *qgluBeginSurface)(GLUnurbs *nurb);
extern void ( GLAPIENTRY *qgluBeginTrim)(GLUnurbs *nurb);
extern GLint ( GLAPIENTRY qgluBuild1DMipmapLevels)( GLenum target, GLint internalFormat, GLsizei width, GLenum format, GLenum type, GLint level, GLint base, GLint max, const void *data);
extern GLint ( GLAPIENTRY qgluBuild1DMipmaps)( GLenum target, GLint internalFormat, GLsizei width, GLenum format, GLenum type, const void *data);
extern GLint ( GLAPIENTRY qgluBuild2DMipmapLevels)( GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint level, GLint base, GLint max, const void *data);
extern GLint ( GLAPIENTRY qgluBuild2DMipmaps)( GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data);
extern GLint ( GLAPIENTRY qgluBuild3DMipmapLevels)( GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLint level, GLint base, GLint max, const void *data);
extern GLint ( GLAPIENTRY qgluBuild3DMipmaps)( GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data);
extern GLboolean ( GLAPIENTRY qgluCheckExtension)(const GLubyte *extName, const GLubyte *extString);
extern void ( GLAPIENTRY *qgluCylinder)( GLUquadric *quad, GLdouble base, GLdouble top, GLdouble height, GLint slices, GLint stacks);
extern void ( GLAPIENTRY *qgluDeleteNurbsRenderer)(GLUnurbs *nurb);
extern void ( GLAPIENTRY *qgluDeleteQuadric)( GLUquadric *quad);
extern void ( GLAPIENTRY *qgluDeleteTess)( GLUtesselator *tess);
extern void ( GLAPIENTRY *qgluDisk)( GLUquadric *quad, GLdouble inner, GLdouble outer, GLint slices, GLint loops);
extern void ( GLAPIENTRY *qgluEndCurve)(GLUnurbs *nurb);
extern void ( GLAPIENTRY *qgluEndPolygon)( GLUtesselator *tess);
extern void ( GLAPIENTRY *qgluEndSurface)(GLUnurbs *nurb);
extern void ( GLAPIENTRY *qgluEndTrim)(GLUnurbs *nurb);
extern const GLubyte (* GLAPIENTRY qgluErrorString)( GLenum error);
extern void ( GLAPIENTRY *qgluGetNurbsProperty)(GLUnurbs *nurb, GLenum property, GLfloat* data);
extern const GLubyte (* GLAPIENTRY qgluGetString)( GLenum name);
extern void ( GLAPIENTRY *qgluGetTessProperty)( GLUtesselator *tess, GLenum which, GLdouble* data);
extern void ( GLAPIENTRY *qgluLoadSamplingMatrices)(GLUnurbs *nurb, const GLfloat *model, const GLfloat *perspective, const GLint *view);
extern void ( GLAPIENTRY *qgluLookAt)( GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble centerX, GLdouble centerY, GLdouble centerZ, GLdouble upX, GLdouble upY, GLdouble upZ);
extern GLUnurbs (* GLAPIENTRY qgluNewNurbsRenderer)( void );
extern GLUquadric (* GLAPIENTRY qgluNewQuadric)( void );
extern GLUtesselator (*GLAPIENTRY qgluNewTess)( void );
extern void ( GLAPIENTRY *qgluNextContour)( GLUtesselator *tess, GLenum type);
extern void ( GLAPIENTRY *qgluNurbsCallback)(GLUnurbs *nurb, GLenum which, _GLUfuncptr CallBackFunc);
extern void ( GLAPIENTRY *qgluNurbsCallbackData)(GLUnurbs *nurb, GLvoid* userData);
extern void ( GLAPIENTRY *qgluNurbsCallbackDataEXT)(GLUnurbs *nurb, GLvoid* userData);
extern void ( GLAPIENTRY *qgluNurbsCurve)(GLUnurbs *nurb, GLint knotCount, GLfloat *knots, GLint stride, GLfloat *control, GLint order, GLenum type);
extern void ( GLAPIENTRY *qgluNurbsProperty)(GLUnurbs *nurb, GLenum property, GLfloat value);
extern void ( GLAPIENTRY *qgluNurbsSurface)(GLUnurbs *nurb, GLint sKnotCount, GLfloat* sKnots, GLint tKnotCount, GLfloat* tKnots, GLint sStride, GLint tStride, GLfloat* control, GLint sOrder, GLint tOrder, GLenum type);
extern void ( GLAPIENTRY *qgluOrtho2D)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
extern void ( GLAPIENTRY *qgluPartialDisk)( GLUquadric *quad, GLdouble inner, GLdouble outer, GLint slices, GLint loops, GLdouble start, GLdouble sweep);
extern void ( GLAPIENTRY *qgluPerspective)( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
extern void ( GLAPIENTRY *qgluPickMatrix)( GLdouble x, GLdouble y, GLdouble delX, GLdouble delY, GLint *viewport);
GLAPI GLint ( GLAPIENTRY qgluProject)( GLdouble objX, GLdouble objY, GLdouble objZ, const GLdouble *model, const GLdouble *proj, const GLint *view, GLdouble* winX, GLdouble* winY, GLdouble* winZ);
extern void ( GLAPIENTRY *qgluPwlCurve)(GLUnurbs *nurb, GLint count, GLfloat* data, GLint stride, GLenum type);
extern void ( GLAPIENTRY *qgluQuadricCallback)( GLUquadric *quad, GLenum which, _GLUfuncptr CallBackFunc);
extern void ( GLAPIENTRY *qgluQuadricDrawStyle)( GLUquadric *quad, GLenum draw );
extern void ( GLAPIENTRY *qgluQuadricNormals)( GLUquadric *quad, GLenum normal);
extern void ( GLAPIENTRY *qgluQuadricOrientation)( GLUquadric *quad, GLenum orientation);
extern void ( GLAPIENTRY *qgluQuadricTexture)( GLUquadric *quad, GLboolean texture);
extern GLint( GLAPIENTRY qgluScaleImage)( GLenum format, GLsizei wIn, GLsizei hIn, GLenum typeIn, const void *dataIn, GLsizei wOut, GLsizei hOut, GLenum typeOut, GLvoid *dataOut );
extern void ( GLAPIENTRY *qgluSphere)( GLUquadric *quad, GLdouble radius, GLint slices, GLint stacks);
extern void ( GLAPIENTRY *qgluTessBeginContour)( GLUtesselator *tess);
extern void ( GLAPIENTRY *qgluTessBeginPolygon)( GLUtesselator *tess, GLvoid* data);
extern void ( GLAPIENTRY *qgluTessCallback)( GLUtesselator *tess, GLenum which, _GLUfuncptr CallBackFunc);
extern void ( GLAPIENTRY *qgluTessEndContour)( GLUtesselator *tess);
extern void ( GLAPIENTRY *qgluTessEndPolygon)( GLUtesselator *tess);
extern void ( GLAPIENTRY *qgluTessNormal)( GLUtesselator *tess, GLdouble valueX, GLdouble valueY, GLdouble valueZ);
extern void ( GLAPIENTRY *qgluTessProperty)( GLUtesselator *tess, GLenum which, GLdouble data);
extern void ( GLAPIENTRY *qgluTessVertex)( GLUtesselator *tess, GLdouble *location, GLvoid* data);
extern GLint ( GLAPIENTRY qgluUnProject)( GLdouble winX, GLdouble winY, GLdouble winZ, const GLdouble *model, const GLdouble *proj, const GLint *view, GLdouble *objX, GLdouble *objY, GLdouble *objZ );
extern GLint ( GLAPIENTRY qgluUnProject4)( GLdouble winX, GLdouble winY, GLdouble winZ, GLdouble clipW, const GLdouble *model, const GLdouble *proj, const GLint *view, GLdouble nearVal, GLdouble farVal, GLdouble *objX, GLdouble *objY, GLdouble *objZ, GLdouble *objW );

extern void ( APIENTRY * qglSetTexCacheDefault2DImageId )( int id);
extern void ( APIENTRY * qglSetTexCacheDefaultCubeImageId )( int id);
extern  GLboolean ( APIENTRY * qglTexImageExistsInBundles )( unsigned long texNameCRC32 );
extern  void ( APIENTRY * qglTexImageFromCache )( int id, unsigned long texNameCRC32 );

// capture backbuffer to memory
extern void qxglCapture( int width, int height, void * RESTRICT pixels );

// allow us to directly use the results of resolving from the EDRAM rendertarget as texture.
extern void ( APIENTRY * qglTexImageFromFrontBuffer )( void );

typedef byte				GLubyte;
typedef float				GLfloat;
typedef unsigned char		GLubyte8;		// 8-bit unsigned integer
typedef unsigned short		GLuint16;		// 16-bit unsigned integer
typedef unsigned int		GLuint32;		// 32-bit unsigned integer
typedef unsigned long long	GLuint64;		// 64-bit unsigned integer
typedef signed char			GLbyte;		// 8-bit signed integer
typedef signed short		GLshort16;		// 16-bit signed integer
typedef signed int			GLint32;		// 32-bit signed integer
typedef signed long long	GLint64;		// 64-bit signed integer
typedef float				GLfloat32;		// 32-bit floating-point number
typedef double				GLfloat64;		// 64-bit floating-point number
typedef unsigned int		GLenum;
typedef unsigned char		GLboolean;
typedef bool				GLbool;
typedef unsigned int		GLbitfield;
typedef void				GLvoid;
typedef signed char			GLbyte;		// 1-byte signed
typedef short				GLshort;	// 2-byte signed
typedef int					GLint;		// 4-byte signed
typedef unsigned char		GLubyte;	// 1-byte unsigne
typedef unsigned short		GLushort;	// 2-byte unsigned
typedef unsigned int		GLuint;		// 4-byte unsigned
typedef int					GLsizei;	// 4-byte signed
typedef float				GLfloat;	// single precision float
typedef float				GLclampf;	// single precision float in [0,1]
typedef double				GLdouble;	// double precision float
typedef double				GLclampd;	// double precision float in [0,1]
typedef unsigned char		byte
