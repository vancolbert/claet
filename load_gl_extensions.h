#ifndef	_LOAD_GL_EXTENSIONS_H_
#define	_LOAD_GL_EXTENSIONS_H_

#include "platform.h"
#include <SDL_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef GL_ATI_texture_compression_3dc
#define GL_ATI_texture_compression_3dc 1

#define GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI     0x8837

#endif /* GL_ATI_texture_compression_3dc */

#ifndef GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT 0x8C72
#endif

#ifndef GL_COMPRESSED_LUMINANCE_LATC1_EXT
#define GL_COMPRESSED_LUMINANCE_LATC1_EXT  0x8C70
#endif

typedef enum {
	arb_multitexture = 0,
	arb_texture_compression = 1,
	arb_point_parameters = 2,
	arb_point_sprite = 3,
	arb_vertex_buffer_object = 4,
	arb_shadow = 5,
	arb_texture_env_combine = 6,
	arb_texture_env_crossbar = 7,
	arb_texture_env_dot3 = 8,
	arb_occlusion_query = 9,
	arb_depth_texture = 10,
	arb_fragment_program = 11,
	arb_vertex_program = 12,
	arb_fragment_shader = 13,
	arb_vertex_shader = 14,
	arb_shader_objects = 15,
	arb_shading_language_100 = 16,
	arb_texture_non_power_of_two = 17,
	ext_compiled_vertex_array = 18,
	ext_draw_range_elements = 19,
	ext_framebuffer_object = 20,
	ext_texture_compression_s3tc = 21,
	ext_texture_filter_anisotropic = 22,
	sgis_generate_mipmap = 23,
	arb_texture_mirrored_repeat = 24,
	arb_texture_rectangle = 25,
	ext_fog_coord = 26,
	ati_texture_compression_3dc = 27,
	ext_texture_compression_latc = 28,
	ext_texture_compression_rgtc = 29,
	arb_texture_cube_map = 30,
	arb_texture_float = 31,
	ext_abgr = 32,
	ext_gpu_program_parameters = 33
} extension_enum;

/*	GL_VERSION_1_2		*/
extern PFNGLCOPYTEXSUBIMAGE3DPROC ELglCopyTexSubImage3D;
extern PFNGLDRAWRANGEELEMENTSPROC ELglDrawRangeElements;
extern PFNGLTEXIMAGE3DPROC ELglTexImage3D;
extern PFNGLTEXSUBIMAGE3DPROC ELglTexSubImage3D;
/*	GL_VERSION_1_2		*/

/*	GL_VERSION_1_3		*/
extern PFNGLACTIVETEXTUREPROC ELglActiveTexture;
extern PFNGLCLIENTACTIVETEXTUREPROC ELglClientActiveTexture;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC ELglCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC ELglCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC ELglCompressedTexImage3D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC ELglCompressedTexSubImage1D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC ELglCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC ELglCompressedTexSubImage3D;
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC ELglGetCompressedTexImage;
extern PFNGLLOADTRANSPOSEMATRIXDPROC ELglLoadTransposeMatrixd;
extern PFNGLLOADTRANSPOSEMATRIXFPROC ELglLoadTransposeMatrixf;
extern PFNGLMULTTRANSPOSEMATRIXDPROC ELglMultTransposeMatrixd;
extern PFNGLMULTTRANSPOSEMATRIXFPROC ELglMultTransposeMatrixf;
extern PFNGLMULTITEXCOORD1DPROC ELglMultiTexCoord1d;
extern PFNGLMULTITEXCOORD1DVPROC ELglMultiTexCoord1dv;
extern PFNGLMULTITEXCOORD1FPROC ELglMultiTexCoord1f;
extern PFNGLMULTITEXCOORD1FVPROC ELglMultiTexCoord1fv;
extern PFNGLMULTITEXCOORD1IPROC ELglMultiTexCoord1i;
extern PFNGLMULTITEXCOORD1IVPROC ELglMultiTexCoord1iv;
extern PFNGLMULTITEXCOORD1SPROC ELglMultiTexCoord1s;
extern PFNGLMULTITEXCOORD1SVPROC ELglMultiTexCoord1sv;
extern PFNGLMULTITEXCOORD2DPROC ELglMultiTexCoord2d;
extern PFNGLMULTITEXCOORD2DVPROC ELglMultiTexCoord2dv;
extern PFNGLMULTITEXCOORD2FPROC ELglMultiTexCoord2f;
extern PFNGLMULTITEXCOORD2FVPROC ELglMultiTexCoord2fv;
extern PFNGLMULTITEXCOORD2IPROC ELglMultiTexCoord2i;
extern PFNGLMULTITEXCOORD2IVPROC ELglMultiTexCoord2iv;
extern PFNGLMULTITEXCOORD2SPROC ELglMultiTexCoord2s;
extern PFNGLMULTITEXCOORD2SVPROC ELglMultiTexCoord2sv;
extern PFNGLMULTITEXCOORD3DPROC ELglMultiTexCoord3d;
extern PFNGLMULTITEXCOORD3DVPROC ELglMultiTexCoord3dv;
extern PFNGLMULTITEXCOORD3FPROC ELglMultiTexCoord3f;
extern PFNGLMULTITEXCOORD3FVPROC ELglMultiTexCoord3fv;
extern PFNGLMULTITEXCOORD3IPROC ELglMultiTexCoord3i;
extern PFNGLMULTITEXCOORD3IVPROC ELglMultiTexCoord3iv;
extern PFNGLMULTITEXCOORD3SPROC ELglMultiTexCoord3s;
extern PFNGLMULTITEXCOORD3SVPROC ELglMultiTexCoord3sv;
extern PFNGLMULTITEXCOORD4DPROC ELglMultiTexCoord4d;
extern PFNGLMULTITEXCOORD4DVPROC ELglMultiTexCoord4dv;
extern PFNGLMULTITEXCOORD4FPROC ELglMultiTexCoord4f;
extern PFNGLMULTITEXCOORD4FVPROC ELglMultiTexCoord4fv;
extern PFNGLMULTITEXCOORD4IPROC ELglMultiTexCoord4i;
extern PFNGLMULTITEXCOORD4IVPROC ELglMultiTexCoord4iv;
extern PFNGLMULTITEXCOORD4SPROC ELglMultiTexCoord4s;
extern PFNGLMULTITEXCOORD4SVPROC ELglMultiTexCoord4sv;
extern PFNGLSAMPLECOVERAGEPROC ELglSampleCoverage;
/*	GL_VERSION_1_3		*/

/*	GL_VERSION_1_4		*/
extern PFNGLBLENDCOLORPROC ELglBlendColor;
extern PFNGLBLENDEQUATIONPROC ELglBlendEquation;
extern PFNGLBLENDFUNCSEPARATEPROC ELglBlendFuncSeparate;
extern PFNGLFOGCOORDPOINTERPROC ELglFogCoordPointer;
extern PFNGLFOGCOORDDPROC ELglFogCoordd;
extern PFNGLFOGCOORDDVPROC ELglFogCoorddv;
extern PFNGLFOGCOORDFPROC ELglFogCoordf;
extern PFNGLFOGCOORDFVPROC ELglFogCoordfv;
extern PFNGLMULTIDRAWARRAYSPROC ELglMultiDrawArrays;
extern PFNGLMULTIDRAWELEMENTSPROC ELglMultiDrawElements;
extern PFNGLPOINTPARAMETERFPROC ELglPointParameterf;
extern PFNGLPOINTPARAMETERFVPROC ELglPointParameterfv;
extern PFNGLSECONDARYCOLOR3BPROC ELglSecondaryColor3b;
extern PFNGLSECONDARYCOLOR3BVPROC ELglSecondaryColor3bv;
extern PFNGLSECONDARYCOLOR3DPROC ELglSecondaryColor3d;
extern PFNGLSECONDARYCOLOR3DVPROC ELglSecondaryColor3dv;
extern PFNGLSECONDARYCOLOR3FPROC ELglSecondaryColor3f;
extern PFNGLSECONDARYCOLOR3FVPROC ELglSecondaryColor3fv;
extern PFNGLSECONDARYCOLOR3IPROC ELglSecondaryColor3i;
extern PFNGLSECONDARYCOLOR3IVPROC ELglSecondaryColor3iv;
extern PFNGLSECONDARYCOLOR3SPROC ELglSecondaryColor3s;
extern PFNGLSECONDARYCOLOR3SVPROC ELglSecondaryColor3sv;
extern PFNGLSECONDARYCOLOR3UBPROC ELglSecondaryColor3ub;
extern PFNGLSECONDARYCOLOR3UBVPROC ELglSecondaryColor3ubv;
extern PFNGLSECONDARYCOLOR3UIPROC ELglSecondaryColor3ui;
extern PFNGLSECONDARYCOLOR3UIVPROC ELglSecondaryColor3uiv;
extern PFNGLSECONDARYCOLOR3USPROC ELglSecondaryColor3us;
extern PFNGLSECONDARYCOLOR3USVPROC ELglSecondaryColor3usv;
extern PFNGLSECONDARYCOLORPOINTERPROC ELglSecondaryColorPointer;
extern PFNGLWINDOWPOS2DPROC ELglWindowPos2d;
extern PFNGLWINDOWPOS2DVPROC ELglWindowPos2dv;
extern PFNGLWINDOWPOS2FPROC ELglWindowPos2f;
extern PFNGLWINDOWPOS2FVPROC ELglWindowPos2fv;
extern PFNGLWINDOWPOS2IPROC ELglWindowPos2i;
extern PFNGLWINDOWPOS2IVPROC ELglWindowPos2iv;
extern PFNGLWINDOWPOS2SPROC ELglWindowPos2s;
extern PFNGLWINDOWPOS2SVPROC ELglWindowPos2sv;
extern PFNGLWINDOWPOS3DPROC ELglWindowPos3d;
extern PFNGLWINDOWPOS3DVPROC ELglWindowPos3dv;
extern PFNGLWINDOWPOS3FPROC ELglWindowPos3f;
extern PFNGLWINDOWPOS3FVPROC ELglWindowPos3fv;
extern PFNGLWINDOWPOS3IPROC ELglWindowPos3i;
extern PFNGLWINDOWPOS3IVPROC ELglWindowPos3iv;
extern PFNGLWINDOWPOS3SPROC ELglWindowPos3s;
extern PFNGLWINDOWPOS3SVPROC ELglWindowPos3sv;
/*	GL_VERSION_1_4		*/

/*	GL_VERSION_1_5		*/
extern PFNGLBEGINQUERYPROC ELglBeginQuery;
extern PFNGLBINDBUFFERPROC ELglBindBuffer;
extern PFNGLBUFFERDATAPROC ELglBufferData;
extern PFNGLBUFFERSUBDATAPROC ELglBufferSubData;
extern PFNGLDELETEBUFFERSPROC ELglDeleteBuffers;
extern PFNGLDELETEQUERIESPROC ELglDeleteQueries;
extern PFNGLENDQUERYPROC ELglEndQuery;
extern PFNGLGENBUFFERSPROC ELglGenBuffers;
extern PFNGLGENQUERIESPROC ELglGenQueries;
extern PFNGLGETBUFFERPARAMETERIVPROC ELglGetBufferParameteriv;
extern PFNGLGETBUFFERPOINTERVPROC ELglGetBufferPointerv;
extern PFNGLGETBUFFERSUBDATAPROC ELglGetBufferSubData;
extern PFNGLGETQUERYOBJECTIVPROC ELglGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUIVPROC ELglGetQueryObjectuiv;
extern PFNGLGETQUERYIVPROC ELglGetQueryiv;
extern PFNGLISBUFFERPROC ELglIsBuffer;
extern PFNGLISQUERYPROC ELglIsQuery;
extern PFNGLMAPBUFFERPROC ELglMapBuffer;
extern PFNGLUNMAPBUFFERPROC ELglUnmapBuffer;
/*	GL_VERSION_1_5		*/

/*	GL_VERSION_2_0		*/
extern PFNGLATTACHSHADERPROC ELglAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC ELglBindAttribLocation;
extern PFNGLBLENDEQUATIONSEPARATEPROC ELglBlendEquationSeparate;
extern PFNGLCOMPILESHADERPROC ELglCompileShader;
extern PFNGLCREATEPROGRAMPROC ELglCreateProgram;
extern PFNGLCREATESHADERPROC ELglCreateShader;
extern PFNGLDELETEPROGRAMPROC ELglDeleteProgram;
extern PFNGLDELETESHADERPROC ELglDeleteShader;
extern PFNGLDETACHSHADERPROC ELglDetachShader;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC ELglDisableVertexAttribArray;
extern PFNGLDRAWBUFFERSPROC ELglDrawBuffers;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC ELglEnableVertexAttribArray;
extern PFNGLGETACTIVEATTRIBPROC ELglGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC ELglGetActiveUniform;
extern PFNGLGETATTACHEDSHADERSPROC ELglGetAttachedShaders;
extern PFNGLGETATTRIBLOCATIONPROC ELglGetAttribLocation;
extern PFNGLGETPROGRAMINFOLOGPROC ELglGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC ELglGetProgramiv;
extern PFNGLGETSHADERINFOLOGPROC ELglGetShaderInfoLog;
extern PFNGLGETSHADERSOURCEPROC ELglGetShaderSource;
extern PFNGLGETSHADERIVPROC ELglGetShaderiv;
extern PFNGLGETUNIFORMLOCATIONPROC ELglGetUniformLocation;
extern PFNGLGETUNIFORMFVPROC ELglGetUniformfv;
extern PFNGLGETUNIFORMIVPROC ELglGetUniformiv;
extern PFNGLGETVERTEXATTRIBPOINTERVPROC ELglGetVertexAttribPointerv;
extern PFNGLGETVERTEXATTRIBDVPROC ELglGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVPROC ELglGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVPROC ELglGetVertexAttribiv;
extern PFNGLISPROGRAMPROC ELglIsProgram;
extern PFNGLISSHADERPROC ELglIsShader;
extern PFNGLLINKPROGRAMPROC ELglLinkProgram;
extern PFNGLSHADERSOURCEPROC ELglShaderSource;
extern PFNGLSTENCILFUNCSEPARATEPROC ELglStencilFuncSeparate;
extern PFNGLSTENCILMASKSEPARATEPROC ELglStencilMaskSeparate;
extern PFNGLSTENCILOPSEPARATEPROC ELglStencilOpSeparate;
extern PFNGLUNIFORM1FPROC ELglUniform1f;
extern PFNGLUNIFORM1FVPROC ELglUniform1fv;
extern PFNGLUNIFORM1IPROC ELglUniform1i;
extern PFNGLUNIFORM1IVPROC ELglUniform1iv;
extern PFNGLUNIFORM2FPROC ELglUniform2f;
extern PFNGLUNIFORM2FVPROC ELglUniform2fv;
extern PFNGLUNIFORM2IPROC ELglUniform2i;
extern PFNGLUNIFORM2IVPROC ELglUniform2iv;
extern PFNGLUNIFORM3FPROC ELglUniform3f;
extern PFNGLUNIFORM3FVPROC ELglUniform3fv;
extern PFNGLUNIFORM3IPROC ELglUniform3i;
extern PFNGLUNIFORM3IVPROC ELglUniform3iv;
extern PFNGLUNIFORM4FPROC ELglUniform4f;
extern PFNGLUNIFORM4FVPROC ELglUniform4fv;
extern PFNGLUNIFORM4IPROC ELglUniform4i;
extern PFNGLUNIFORM4IVPROC ELglUniform4iv;
extern PFNGLUNIFORMMATRIX2FVPROC ELglUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX3FVPROC ELglUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX4FVPROC ELglUniformMatrix4fv;
extern PFNGLUSEPROGRAMPROC ELglUseProgram;
extern PFNGLVALIDATEPROGRAMPROC ELglValidateProgram;
extern PFNGLVERTEXATTRIB1DPROC ELglVertexAttrib1d;
extern PFNGLVERTEXATTRIB1DVPROC ELglVertexAttrib1dv;
extern PFNGLVERTEXATTRIB1FPROC ELglVertexAttrib1f;
extern PFNGLVERTEXATTRIB1FVPROC ELglVertexAttrib1fv;
extern PFNGLVERTEXATTRIB1SPROC ELglVertexAttrib1s;
extern PFNGLVERTEXATTRIB1SVPROC ELglVertexAttrib1sv;
extern PFNGLVERTEXATTRIB2DPROC ELglVertexAttrib2d;
extern PFNGLVERTEXATTRIB2DVPROC ELglVertexAttrib2dv;
extern PFNGLVERTEXATTRIB2FPROC ELglVertexAttrib2f;
extern PFNGLVERTEXATTRIB2FVPROC ELglVertexAttrib2fv;
extern PFNGLVERTEXATTRIB2SPROC ELglVertexAttrib2s;
extern PFNGLVERTEXATTRIB2SVPROC ELglVertexAttrib2sv;
extern PFNGLVERTEXATTRIB3DPROC ELglVertexAttrib3d;
extern PFNGLVERTEXATTRIB3DVPROC ELglVertexAttrib3dv;
extern PFNGLVERTEXATTRIB3FPROC ELglVertexAttrib3f;
extern PFNGLVERTEXATTRIB3FVPROC ELglVertexAttrib3fv;
extern PFNGLVERTEXATTRIB3SPROC ELglVertexAttrib3s;
extern PFNGLVERTEXATTRIB3SVPROC ELglVertexAttrib3sv;
extern PFNGLVERTEXATTRIB4NBVPROC ELglVertexAttrib4Nbv;
extern PFNGLVERTEXATTRIB4NIVPROC ELglVertexAttrib4Niv;
extern PFNGLVERTEXATTRIB4NSVPROC ELglVertexAttrib4Nsv;
extern PFNGLVERTEXATTRIB4NUBPROC ELglVertexAttrib4Nub;
extern PFNGLVERTEXATTRIB4NUBVPROC ELglVertexAttrib4Nubv;
extern PFNGLVERTEXATTRIB4NUIVPROC ELglVertexAttrib4Nuiv;
extern PFNGLVERTEXATTRIB4NUSVPROC ELglVertexAttrib4Nusv;
extern PFNGLVERTEXATTRIB4BVPROC ELglVertexAttrib4bv;
extern PFNGLVERTEXATTRIB4DPROC ELglVertexAttrib4d;
extern PFNGLVERTEXATTRIB4DVPROC ELglVertexAttrib4dv;
extern PFNGLVERTEXATTRIB4FPROC ELglVertexAttrib4f;
extern PFNGLVERTEXATTRIB4FVPROC ELglVertexAttrib4fv;
extern PFNGLVERTEXATTRIB4IVPROC ELglVertexAttrib4iv;
extern PFNGLVERTEXATTRIB4SPROC ELglVertexAttrib4s;
extern PFNGLVERTEXATTRIB4SVPROC ELglVertexAttrib4sv;
extern PFNGLVERTEXATTRIB4UBVPROC ELglVertexAttrib4ubv;
extern PFNGLVERTEXATTRIB4UIVPROC ELglVertexAttrib4uiv;
extern PFNGLVERTEXATTRIB4USVPROC ELglVertexAttrib4usv;
extern PFNGLVERTEXATTRIBPOINTERPROC ELglVertexAttribPointer;
/*	GL_VERSION_2_0		*/

/*	GL_VERSION_2_1		*/
extern PFNGLUNIFORMMATRIX2X3FVPROC ELglUniformMatrix2x3fv;
extern PFNGLUNIFORMMATRIX2X4FVPROC ELglUniformMatrix2x4fv;
extern PFNGLUNIFORMMATRIX3X2FVPROC ELglUniformMatrix3x2fv;
extern PFNGLUNIFORMMATRIX3X4FVPROC ELglUniformMatrix3x4fv;
extern PFNGLUNIFORMMATRIX4X2FVPROC ELglUniformMatrix4x2fv;
extern PFNGLUNIFORMMATRIX4X3FVPROC ELglUniformMatrix4x3fv;
/*	GL_VERSION_2_1		*/

/*	GL_ARB_multitexture	*/
extern PFNGLACTIVETEXTUREARBPROC ELglActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC ELglClientActiveTextureARB;
extern PFNGLMULTITEXCOORD1DARBPROC ELglMultiTexCoord1dARB;
extern PFNGLMULTITEXCOORD1DVARBPROC ELglMultiTexCoord1dvARB;
extern PFNGLMULTITEXCOORD1FARBPROC ELglMultiTexCoord1fARB;
extern PFNGLMULTITEXCOORD1FVARBPROC ELglMultiTexCoord1fvARB;
extern PFNGLMULTITEXCOORD1IARBPROC ELglMultiTexCoord1iARB;
extern PFNGLMULTITEXCOORD1IVARBPROC ELglMultiTexCoord1ivARB;
extern PFNGLMULTITEXCOORD1SARBPROC ELglMultiTexCoord1sARB;
extern PFNGLMULTITEXCOORD1SVARBPROC ELglMultiTexCoord1svARB;
extern PFNGLMULTITEXCOORD2DARBPROC ELglMultiTexCoord2dARB;
extern PFNGLMULTITEXCOORD2DVARBPROC ELglMultiTexCoord2dvARB;
extern PFNGLMULTITEXCOORD2FARBPROC ELglMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD2FVARBPROC ELglMultiTexCoord2fvARB;
extern PFNGLMULTITEXCOORD2IARBPROC ELglMultiTexCoord2iARB;
extern PFNGLMULTITEXCOORD2IVARBPROC ELglMultiTexCoord2ivARB;
extern PFNGLMULTITEXCOORD2SARBPROC ELglMultiTexCoord2sARB;
extern PFNGLMULTITEXCOORD2SVARBPROC ELglMultiTexCoord2svARB;
extern PFNGLMULTITEXCOORD3DARBPROC ELglMultiTexCoord3dARB;
extern PFNGLMULTITEXCOORD3DVARBPROC ELglMultiTexCoord3dvARB;
extern PFNGLMULTITEXCOORD3FARBPROC ELglMultiTexCoord3fARB;
extern PFNGLMULTITEXCOORD3FVARBPROC ELglMultiTexCoord3fvARB;
extern PFNGLMULTITEXCOORD3IARBPROC ELglMultiTexCoord3iARB;
extern PFNGLMULTITEXCOORD3IVARBPROC ELglMultiTexCoord3ivARB;
extern PFNGLMULTITEXCOORD3SARBPROC ELglMultiTexCoord3sARB;
extern PFNGLMULTITEXCOORD3SVARBPROC ELglMultiTexCoord3svARB;
extern PFNGLMULTITEXCOORD4DARBPROC ELglMultiTexCoord4dARB;
extern PFNGLMULTITEXCOORD4DVARBPROC ELglMultiTexCoord4dvARB;
extern PFNGLMULTITEXCOORD4FARBPROC ELglMultiTexCoord4fARB;
extern PFNGLMULTITEXCOORD4FVARBPROC ELglMultiTexCoord4fvARB;
extern PFNGLMULTITEXCOORD4IARBPROC ELglMultiTexCoord4iARB;
extern PFNGLMULTITEXCOORD4IVARBPROC ELglMultiTexCoord4ivARB;
extern PFNGLMULTITEXCOORD4SARBPROC ELglMultiTexCoord4sARB;
extern PFNGLMULTITEXCOORD4SVARBPROC ELglMultiTexCoord4svARB;
/*	GL_ARB_multitexture	*/

/*	GL_EXT_compiled_vertex_array	*/
extern PFNGLLOCKARRAYSEXTPROC ELglLockArraysEXT;
extern PFNGLUNLOCKARRAYSEXTPROC ELglUnlockArraysEXT;
/*	GL_EXT_compiled_vertex_array	*/

/*	GL_EXT_draw_range_elements	*/
extern PFNGLDRAWRANGEELEMENTSEXTPROC ELglDrawRangeElementsEXT;
/*	GL_EXT_draw_range_elements	*/

/*	GL_ARB_point_parameters		*/
extern PFNGLPOINTPARAMETERFARBPROC ELglPointParameterfARB;
extern PFNGLPOINTPARAMETERFVARBPROC ELglPointParameterfvARB;
/*	GL_ARB_point_parameters		*/

/*	GL_ARB_vertex_buffer_object	*/
extern PFNGLBINDBUFFERARBPROC ELglBindBufferARB;
extern PFNGLBUFFERDATAARBPROC ELglBufferDataARB;
extern PFNGLBUFFERSUBDATAARBPROC ELglBufferSubDataARB;
extern PFNGLDELETEBUFFERSARBPROC ELglDeleteBuffersARB;
extern PFNGLGENBUFFERSARBPROC ELglGenBuffersARB;
extern PFNGLGETBUFFERPARAMETERIVARBPROC ELglGetBufferParameterivARB;
extern PFNGLGETBUFFERPOINTERVARBPROC ELglGetBufferPointervARB;
extern PFNGLGETBUFFERSUBDATAARBPROC ELglGetBufferSubDataARB;
extern PFNGLISBUFFERARBPROC ELglIsBufferARB;
extern PFNGLMAPBUFFERARBPROC ELglMapBufferARB;
extern PFNGLUNMAPBUFFERARBPROC ELglUnmapBufferARB;
/*	GL_ARB_vertex_buffer_object	*/

/*	GL_EXT_framebuffer_object	*/
extern PFNGLBINDFRAMEBUFFEREXTPROC ELglBindFramebufferEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC ELglBindRenderbufferEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC ELglCheckFramebufferStatusEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC ELglDeleteFramebuffersEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC ELglDeleteRenderbuffersEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC ELglFramebufferRenderbufferEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC ELglFramebufferTexture1DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC ELglFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC ELglFramebufferTexture3DEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC ELglGenFramebuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC ELglGenRenderbuffersEXT;
extern PFNGLGENERATEMIPMAPEXTPROC ELglGenerateMipmapEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC ELglGetFramebufferAttachmentParameterivEXT;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC ELglGetRenderbufferParameterivEXT;
extern PFNGLISFRAMEBUFFEREXTPROC ELglIsFramebufferEXT;
extern PFNGLISRENDERBUFFEREXTPROC ELglIsRenderbufferEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC ELglRenderbufferStorageEXT;
/*	GL_EXT_framebuffer_object	*/

/*	GL_ARB_texture_compression	*/
extern PFNGLCOMPRESSEDTEXIMAGE1DARBPROC ELglCompressedTexImage1DARB;
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC ELglCompressedTexImage2DARB;
extern PFNGLCOMPRESSEDTEXIMAGE3DARBPROC ELglCompressedTexImage3DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC ELglCompressedTexSubImage1DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC ELglCompressedTexSubImage2DARB;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC ELglCompressedTexSubImage3DARB;
extern PFNGLGETCOMPRESSEDTEXIMAGEARBPROC ELglGetCompressedTexImageARB;
/*	GL_ARB_texture_compression	*/

/*	GL_ARB_occlusion_query		*/
extern PFNGLBEGINQUERYARBPROC ELglBeginQueryARB;
extern PFNGLDELETEQUERIESARBPROC ELglDeleteQueriesARB;
extern PFNGLENDQUERYARBPROC ELglEndQueryARB;
extern PFNGLGENQUERIESARBPROC ELglGenQueriesARB;
extern PFNGLGETQUERYOBJECTIVARBPROC ELglGetQueryObjectivARB;
extern PFNGLGETQUERYOBJECTUIVARBPROC ELglGetQueryObjectuivARB;
extern PFNGLGETQUERYIVARBPROC ELglGetQueryivARB;
extern PFNGLISQUERYARBPROC ELglIsQueryARB;
/*	GL_ARB_occlusion_query		*/

/*	GL_ARB_vertex_program		*/
extern PFNGLBINDPROGRAMARBPROC ELglBindProgramARB;
extern PFNGLDELETEPROGRAMSARBPROC ELglDeleteProgramsARB;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC ELglDisableVertexAttribArrayARB;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC ELglEnableVertexAttribArrayARB;
extern PFNGLGENPROGRAMSARBPROC ELglGenProgramsARB;
extern PFNGLGETPROGRAMENVPARAMETERDVARBPROC ELglGetProgramEnvParameterdvARB;
extern PFNGLGETPROGRAMENVPARAMETERFVARBPROC ELglGetProgramEnvParameterfvARB;
extern PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC ELglGetProgramLocalParameterdvARB;
extern PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC ELglGetProgramLocalParameterfvARB;
extern PFNGLGETPROGRAMSTRINGARBPROC ELglGetProgramStringARB;
extern PFNGLGETPROGRAMIVARBPROC ELglGetProgramivARB;
extern PFNGLGETVERTEXATTRIBPOINTERVARBPROC ELglGetVertexAttribPointervARB;
extern PFNGLGETVERTEXATTRIBDVARBPROC ELglGetVertexAttribdvARB;
extern PFNGLGETVERTEXATTRIBFVARBPROC ELglGetVertexAttribfvARB;
extern PFNGLGETVERTEXATTRIBIVARBPROC ELglGetVertexAttribivARB;
extern PFNGLISPROGRAMARBPROC ELglIsProgramARB;
extern PFNGLPROGRAMENVPARAMETER4DARBPROC ELglProgramEnvParameter4dARB;
extern PFNGLPROGRAMENVPARAMETER4DVARBPROC ELglProgramEnvParameter4dvARB;
extern PFNGLPROGRAMENVPARAMETER4FARBPROC ELglProgramEnvParameter4fARB;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC ELglProgramEnvParameter4fvARB;
extern PFNGLPROGRAMLOCALPARAMETER4DARBPROC ELglProgramLocalParameter4dARB;
extern PFNGLPROGRAMLOCALPARAMETER4DVARBPROC ELglProgramLocalParameter4dvARB;
extern PFNGLPROGRAMLOCALPARAMETER4FARBPROC ELglProgramLocalParameter4fARB;
extern PFNGLPROGRAMLOCALPARAMETER4FVARBPROC ELglProgramLocalParameter4fvARB;
extern PFNGLPROGRAMSTRINGARBPROC ELglProgramStringARB;
extern PFNGLVERTEXATTRIB1DARBPROC ELglVertexAttrib1dARB;
extern PFNGLVERTEXATTRIB1DVARBPROC ELglVertexAttrib1dvARB;
extern PFNGLVERTEXATTRIB1FARBPROC ELglVertexAttrib1fARB;
extern PFNGLVERTEXATTRIB1FVARBPROC ELglVertexAttrib1fvARB;
extern PFNGLVERTEXATTRIB1SARBPROC ELglVertexAttrib1sARB;
extern PFNGLVERTEXATTRIB1SVARBPROC ELglVertexAttrib1svARB;
extern PFNGLVERTEXATTRIB2DARBPROC ELglVertexAttrib2dARB;
extern PFNGLVERTEXATTRIB2DVARBPROC ELglVertexAttrib2dvARB;
extern PFNGLVERTEXATTRIB2FARBPROC ELglVertexAttrib2fARB;
extern PFNGLVERTEXATTRIB2FVARBPROC ELglVertexAttrib2fvARB;
extern PFNGLVERTEXATTRIB2SARBPROC ELglVertexAttrib2sARB;
extern PFNGLVERTEXATTRIB2SVARBPROC ELglVertexAttrib2svARB;
extern PFNGLVERTEXATTRIB3DARBPROC ELglVertexAttrib3dARB;
extern PFNGLVERTEXATTRIB3DVARBPROC ELglVertexAttrib3dvARB;
extern PFNGLVERTEXATTRIB3FARBPROC ELglVertexAttrib3fARB;
extern PFNGLVERTEXATTRIB3FVARBPROC ELglVertexAttrib3fvARB;
extern PFNGLVERTEXATTRIB3SARBPROC ELglVertexAttrib3sARB;
extern PFNGLVERTEXATTRIB3SVARBPROC ELglVertexAttrib3svARB;
extern PFNGLVERTEXATTRIB4NBVARBPROC ELglVertexAttrib4NbvARB;
extern PFNGLVERTEXATTRIB4NIVARBPROC ELglVertexAttrib4NivARB;
extern PFNGLVERTEXATTRIB4NSVARBPROC ELglVertexAttrib4NsvARB;
extern PFNGLVERTEXATTRIB4NUBARBPROC ELglVertexAttrib4NubARB;
extern PFNGLVERTEXATTRIB4NUBVARBPROC ELglVertexAttrib4NubvARB;
extern PFNGLVERTEXATTRIB4NUIVARBPROC ELglVertexAttrib4NuivARB;
extern PFNGLVERTEXATTRIB4NUSVARBPROC ELglVertexAttrib4NusvARB;
extern PFNGLVERTEXATTRIB4BVARBPROC ELglVertexAttrib4bvARB;
extern PFNGLVERTEXATTRIB4DARBPROC ELglVertexAttrib4dARB;
extern PFNGLVERTEXATTRIB4DVARBPROC ELglVertexAttrib4dvARB;
extern PFNGLVERTEXATTRIB4FARBPROC ELglVertexAttrib4fARB;
extern PFNGLVERTEXATTRIB4FVARBPROC ELglVertexAttrib4fvARB;
extern PFNGLVERTEXATTRIB4IVARBPROC ELglVertexAttrib4ivARB;
extern PFNGLVERTEXATTRIB4SARBPROC ELglVertexAttrib4sARB;
extern PFNGLVERTEXATTRIB4SVARBPROC ELglVertexAttrib4svARB;
extern PFNGLVERTEXATTRIB4UBVARBPROC ELglVertexAttrib4ubvARB;
extern PFNGLVERTEXATTRIB4UIVARBPROC ELglVertexAttrib4uivARB;
extern PFNGLVERTEXATTRIB4USVARBPROC ELglVertexAttrib4usvARB;
extern PFNGLVERTEXATTRIBPOINTERARBPROC ELglVertexAttribPointerARB;
/*	GL_ARB_vertex_program		*/

/*	GL_ARB_vertex_shader		*/
extern PFNGLBINDATTRIBLOCATIONARBPROC ELglBindAttribLocationARB;
extern PFNGLGETACTIVEATTRIBARBPROC ELglGetActiveAttribARB;
extern PFNGLGETATTRIBLOCATIONARBPROC ELglGetAttribLocationARB;
/*	GL_ARB_vertex_shader		*/

/*	GL_ARB_shader_objects		*/
extern PFNGLATTACHOBJECTARBPROC ELglAttachObjectARB;
extern PFNGLCOMPILESHADERARBPROC ELglCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC ELglCreateProgramObjectARB;
extern PFNGLCREATESHADEROBJECTARBPROC ELglCreateShaderObjectARB;
extern PFNGLDELETEOBJECTARBPROC ELglDeleteObjectARB;
extern PFNGLDETACHOBJECTARBPROC ELglDetachObjectARB;
extern PFNGLGETACTIVEUNIFORMARBPROC ELglGetActiveUniformARB;
extern PFNGLGETATTACHEDOBJECTSARBPROC ELglGetAttachedObjectsARB;
extern PFNGLGETHANDLEARBPROC ELglGetHandleARB;
extern PFNGLGETINFOLOGARBPROC ELglGetInfoLogARB;
extern PFNGLGETOBJECTPARAMETERFVARBPROC ELglGetObjectParameterfvARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC ELglGetObjectParameterivARB;
extern PFNGLGETSHADERSOURCEARBPROC ELglGetShaderSourceARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC ELglGetUniformLocationARB;
extern PFNGLGETUNIFORMFVARBPROC ELglGetUniformfvARB;
extern PFNGLGETUNIFORMIVARBPROC ELglGetUniformivARB;
extern PFNGLLINKPROGRAMARBPROC ELglLinkProgramARB;
extern PFNGLSHADERSOURCEARBPROC ELglShaderSourceARB;
extern PFNGLUNIFORM1FARBPROC ELglUniform1fARB;
extern PFNGLUNIFORM1FVARBPROC ELglUniform1fvARB;
extern PFNGLUNIFORM1IARBPROC ELglUniform1iARB;
extern PFNGLUNIFORM1IVARBPROC ELglUniform1ivARB;
extern PFNGLUNIFORM2FARBPROC ELglUniform2fARB;
extern PFNGLUNIFORM2FVARBPROC ELglUniform2fvARB;
extern PFNGLUNIFORM2IARBPROC ELglUniform2iARB;
extern PFNGLUNIFORM2IVARBPROC ELglUniform2ivARB;
extern PFNGLUNIFORM3FARBPROC ELglUniform3fARB;
extern PFNGLUNIFORM3FVARBPROC ELglUniform3fvARB;
extern PFNGLUNIFORM3IARBPROC ELglUniform3iARB;
extern PFNGLUNIFORM3IVARBPROC ELglUniform3ivARB;
extern PFNGLUNIFORM4FARBPROC ELglUniform4fARB;
extern PFNGLUNIFORM4FVARBPROC ELglUniform4fvARB;
extern PFNGLUNIFORM4IARBPROC ELglUniform4iARB;
extern PFNGLUNIFORM4IVARBPROC ELglUniform4ivARB;
extern PFNGLUNIFORMMATRIX2FVARBPROC ELglUniformMatrix2fvARB;
extern PFNGLUNIFORMMATRIX3FVARBPROC ELglUniformMatrix3fvARB;
extern PFNGLUNIFORMMATRIX4FVARBPROC ELglUniformMatrix4fvARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC ELglUseProgramObjectARB;
extern PFNGLVALIDATEPROGRAMARBPROC ELglValidateProgramARB;
/*	GL_ARB_shader_objects		*/

/*	GL_EXT_fog_coord		*/
extern PFNGLFOGCOORDPOINTEREXTPROC ELglFogCoordPointerEXT;
extern PFNGLFOGCOORDDEXTPROC ELglFogCoorddEXT;
extern PFNGLFOGCOORDDVEXTPROC ELglFogCoorddvEXT;
extern PFNGLFOGCOORDFEXTPROC ELglFogCoordfEXT;
extern PFNGLFOGCOORDFVEXTPROC ELglFogCoordfvEXT;
/*	GL_EXT_fog_coord		*/

/*	GL_EXT_gpu_program_parameters	*/
extern PFNGLPROGRAMENVPARAMETERS4FVEXTPROC ELglProgramEnvParameters4fvEXT;
extern PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC ELglProgramLocalParameters4fvEXT;
/*	GL_EXT_gpu_program_parameters	*/

extern void init_opengl_extensions();
extern Uint32 have_extension(extension_enum extension);
extern Uint32 get_texture_units();

extern const char* get_gl_version_str();
extern GLboolean supports_gl_version(Uint8 major, Uint8 minor);

#ifdef __cplusplus
}
#endif

#endif	//_LOAD_GL_EXTENSIONS_H_
