#include "../idlib/Lib.h"
#include "GLIncludes/gl.h"
#pragma hdrstop

#include "tr_local.h"

shaderProgram_t	interactionShader;
shaderProgram_t	shadowShader;
shaderProgram_t	defaultShader;
shaderProgram_t	depthFillShader;
shaderProgram_t cubemapShader; //k: skybox shader
shaderProgram_t reflectionCubemapShader; //k: reflection shader
shaderProgram_t	depthFillClipShader; //k: z-fill clipped shader
shaderProgram_t	fogShader; //k: fog shader
shaderProgram_t	blendLightShader; //k: blend light shader
shaderProgram_t	interactionBlinnPhongShader; //k: BLINN-PHONG lighting model interaction shader
shaderProgram_t diffuseCubemapShader; //k: diffuse cubemap shader
shaderProgram_t texgenShader; //k: texgen shader

static bool r_usePhong = true;
static float r_specularExponent = 4.0f;

struct GLSLShaderProp {
	const char *name;
	shaderProgram_t * const program;
	const char *default_vertex_shader_source;
	const char *default_fragment_shader_source;
	const char *vertex_shader_source_file;
	const char *fragment_shader_source_file;
	int cond;
};
static int R_LoadGLSLShaderProgram(
		const char *name,
		shaderProgram_t * const program,
		const char *default_vertex_shader_source,
		const char *default_fragment_shader_source,
		const char *vertex_shader_source_file,
		const char *fragment_shader_source_file,
		int cond
);

#include "glsl_shader.h"
#define HARM_INTERACTION_SHADER_PHONG "phong"
#define HARM_INTERACTION_SHADER_BLINNPHONG "blinn_phong"
const char *harm_r_lightModelArgs[]	= { HARM_INTERACTION_SHADER_PHONG, HARM_INTERACTION_SHADER_BLINNPHONG, nullptr };
static anCVarSystem harm_r_lightModel( "harm_r_lightModel", harm_r_lightModelArgs[0], CVAR_RENDERER|CVAR_ARCHIVE, "[Harmattan]: Light model when draw interactions(`phong` - Phong(default), `blinn_phong` - Blinn-Phong.)", harm_r_lightModelArgs, idCmdSystem::ArgCompletion_String<harm_r_lightModelArgs>);
static anCVarSystem harm_r_specularExponent( "harm_r_specularExponent", "4.0", CVAR_FLOAT|CVAR_RENDERER|CVAR_ARCHIVE, "[Harmattan]: Specular exponent in interaction light model(default is 4.0.)" );
#if 0
#define _GLPROGS "gl2progs"
#else
#define _GLPROGS "glslprogs"
#endif
static anCVarSystem	harm_r_shaderProgramDir( "harm_r_shaderProgramDir", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "[Harmattan]: Special external GLSL shader program directory path(default is empty, means using `" _GLPROGS "`)." );

static bool R_CreateShaderProgram( shaderProgram_t *shaderProgram, const char *vert, const char *frag , const char *name);

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
==================
RB_GLSL_DrawInteraction
==================
*/
void RB_GLSL_DrawInteraction(const drawInteraction_t *din) {
	static const float zero[4] = { 0, 0, 0, 0 };
	static const float one[4] = { 1, 1, 1, 1 };
	static const float negOne[4] = { -1, -1, -1, -1 };

	// load all the vertex program parameters
	GL_UniformMatrix4fv( offsetof( shaderProgram_t, textureMatrix), mat4_identity.ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, localLightOrigin), din->localLightOrigin.ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, localViewOrigin), din->localViewOrigin.ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, lightProjectionS), din->lightProjection[0].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, lightProjectionT), din->lightProjection[1].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, lightProjectionQ), din->lightProjection[2].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, lightFalloff), din->lightProjection[3].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, bumpMatrixS), din->bumpMatrix[0].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, bumpMatrixT), din->bumpMatrix[1].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, diffuseMatrixS), din->diffuseMatrix[0].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, diffuseMatrixT), din->diffuseMatrix[1].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, specularMatrixS), din->specularMatrix[0].ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, specularMatrixT), din->specularMatrix[1].ToFloatPtr() );

	switch (din->vertexColor) {
		case SVC_MODULATE:
			GL_Uniform4fv( offsetof( shaderProgram_t, colorModulate), one);
			GL_Uniform4fv( offsetof( shaderProgram_t, colorAdd), zero);
			break;
		case SVC_INVERSE_MODULATE:
			GL_Uniform4fv( offsetof( shaderProgram_t, colorModulate), negOne);
			GL_Uniform4fv( offsetof( shaderProgram_t, colorAdd), one);
			break;
		case SVC_IGNORE:
		default:
			GL_Uniform4fv( offsetof( shaderProgram_t, colorModulate), zero);
			GL_Uniform4fv( offsetof( shaderProgram_t, colorAdd), one);
			break;
	}

	// set the constant colors
	GL_Uniform4fv( offsetof( shaderProgram_t, diffuseColor), din->diffuseColor.ToFloatPtr() );
	GL_Uniform4fv( offsetof( shaderProgram_t, specularColor), din->specularColor.ToFloatPtr() );

	// material may be nullptr for shadow volumes
	GL_Uniform1fv( offsetof( shaderProgram_t, specularExponent), &r_specularExponent);

	// set the textures

	// texture 0 will be the per-surface bump map
	GL_SelectTexture(0);
	din->bumpImage->Bind();

	// texture 1 will be the light falloff texture
	GL_SelectTexture( 1 );
	din->lightFalloffImage->Bind();

	// texture 2 will be the light projection texture
	GL_SelectTexture(2);
	din->lightImage->Bind();

	// texture 3 is the per-surface diffuse map
	GL_SelectTexture(3);
	din->diffuseImage->Bind();

	// texture 4 is the per-surface specular map
	GL_SelectTexture(4);
	din->specularImage->Bind();

	GL_SelectTexture(0); //k2023

	// draw it
	RB_DrawElementsWithCounters(din->surf->geo);
}


/*
=============
RB_GLSL_CreateDrawInteractions

=============
*/
void RB_GLSL_CreateDrawInteractions( const drawSurf_t *surf ) {
	if ( !surf)  {
		return;
	}

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc);

	// bind the vertex and fragment shader
	if ( r_usePhong ) {
		GL_UseProgram( &interactionShader );
	} else {
		GL_UseProgram( &interactionBlinnPhongShader );
	}
	// enable the vertex arrays
	GL_EnableVertexAttribArray( offsetof( shaderProgram_t, attr_TexCoord ) );
	GL_EnableVertexAttribArray( offsetof( shaderProgram_t, attr_Tangent ) ) ;
	GL_EnableVertexAttribArray( offsetof( shaderProgram_t, attr_Bitangent ) );
	GL_EnableVertexAttribArray( offsetof( shaderProgram_t, attr_Normal ) );
	GL_EnableVertexAttribArray( offsetof( shaderProgram_t, attr_Vertex) );
	GL_EnableVertexAttribArray( offsetof( shaderProgram_t, attr_Color ) );

	// texture 5 is the specular lookup table
	GL_SelectTexture(5);
	globalImages->specularTableImage->Bind();

	backEnd.currentSpace = nullptr; //k2023

	for (; surf ; surf=surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes

		// set the modelview matrix for the viewer
		/*float   mat[16];
		myGlMultMatrix( surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat );
		GL_UniformMatrix4fv( offsetof( shaderProgram_t, modelViewProjectionMatrix), mat);*/ //k2023

		// set the vertex pointers
		anDrawVertex	*ac = (anDrawVertex *)vertexCache.Position( surf->geo->ambientCache );

		GL_VertexAttribPointer( offsetof( shaderProgram_t, attr_Normal), 3, GL_FLOAT, false, sizeof( anDrawVertex ), ac->normal.ToFloatPtr() );
		GL_VertexAttribPointer( offsetof( shaderProgram_t, attr_Bitangent), 3, GL_FLOAT, false, sizeof( anDrawVertex ), ac->tangents[1].ToFloatPtr() );
		GL_VertexAttribPointer( offsetof( shaderProgram_t, attr_Tangent), 3, GL_FLOAT, false, sizeof( anDrawVertex), ac->tangents[0].ToFloatPtr() );
		GL_VertexAttribPointer( offsetof( shaderProgram_t, attr_TexCoord), 2, GL_FLOAT, false, sizeof( anDrawVertex ), ac->st.ToFloatPtr() );

		GL_VertexAttribPointer( offsetof( shaderProgram_t, attr_Vertex), 3, GL_FLOAT, false, sizeof( anDrawVertex ), ac->xyz.ToFloatPtr() );
		GL_VertexAttribPointer( offsetof( shaderProgram_t, attr_Color), 4, GL_UNSIGNED_BYTE, false, sizeof( anDrawVertex ), ac->color);

		// this may cause RB_GLSL_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf, RB_GLSL_DrawInteraction);
	}

	backEnd.currentSpace = nullptr; //k2023

	GL_DisableVertexAttribArray( offsetof( shaderProgram_t, attr_TexCoord ) );
	GL_DisableVertexAttribArray( offsetof( shaderProgram_t, attr_Tangent ) );
	GL_DisableVertexAttribArray( offsetof( shaderProgram_t, attr_Bitangent) );
	GL_DisableVertexAttribArray( offsetof( shaderProgram_t, attr_Normal ) );
	GL_DisableVertexAttribArray( offsetof( shaderProgram_t, attr_Vertex ) );	// gl_Vertex
	GL_DisableVertexAttribArray( offsetof( shaderProgram_t, attr_Color ) );	// gl_Color

	// disable features
	GL_SelectTexture(5);
	globalImages->BindNull();

	GL_SelectTexture(4);
	globalImages->BindNull();

	GL_SelectTexture(3);
	globalImages->BindNull();

	GL_SelectTexture(2);
	globalImages->BindNull();

	GL_SelectTexture( 1 );
	globalImages->BindNull();

	backEnd.qglState.currenttmu = -1;
	GL_SetCurrentTextureUnit(0);

	GL_UseProgram(nullptr );
}

/*
==================
RB_GLSL_DrawInteractions
==================
*/
void RB_GLSL_DrawInteractions( void ) {
	viewLight_t		*vLight;
	const anMaterial *lightShader;

	//GL_SetCurrentTextureUnit(0); //k2023

	//
	// for each light, perform adding and shadowing
	//
	for ( vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next ) {
		backEnd.vLight = vLight;
		// do fogging later
		if ( vLight->lightShader->IsFogLight() ) {
			continue;
		}

		if ( vLight->lightShader->IsBlendLight() ) {
			continue;
		}

		if ( !vLight->localInteractions && !vLight->globalInteractions
		    && !vLight->translucentInteractions) {
			continue;
		}

		lightShader = vLight->lightShader;

		// clear the stencil buffer if needed
		if ( ( vLight->globalShadows || vLight->localShadows)&&(r_shadows.GetBool() ) ) {
			backEnd.currentScissor = vLight->scissorRect;
			if (r_useScissor.GetBool() ) {
				qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
				          backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				          backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				          backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
			}
			qglClear( GL_STENCIL_BUFFER_BIT);
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			if (r_shadows.GetBool() )
			qglStencilFunc( GL_ALWAYS, 128, 255);
		}

		RB_StencilShadowPass( vLight->globalShadows );
		RB_GLSL_CreateDrawInteractions(vLight->localInteractions );

		RB_StencilShadowPass( vLight->localShadows );
		RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );
		//k GL_UseProgram(nullptr );	// if there weren't any globalInteractions, it would have stayed on
		// translucent surfaces never get stencil shadowed
		if (r_skipTranslucent.GetBool() ) {
			continue;
		}
		if (r_shadows.GetBool() )
		qglStencilFunc( GL_ALWAYS, 128, 255 );

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_GLSL_CreateDrawInteractions( vLight->translucentInteractions );

		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	if (r_shadows.GetBool() )
	qglStencilFunc( GL_ALWAYS, 128, 255);

	//GL_SetCurrentTextureUnit(0); //k2023
}

//===================================================================================


/*
=================
R_LoadGLSLShader

loads GLSL vertex or fragment shaders
=================
*/
static void R_LoadGLSLShader( const char *name, shaderProgram_t *shaderProgram, GLenum type) {
#if 0
	anString	fullPath = "gl2progs/";
	fullPath += name;
#else
	anString	fullPath = cvarSystem->GetCVarString( "r_tst_shaderProgramDir" );
	if (fullPath.IsEmpty() )
		fullPath = _GLPROGS;
	fullPath.AppendPath(name);
#endif
	char	*fileBuffer;
	char	*buffer;

	if ( !qglConfig.isInitialized) {
		return;
	}

	// load the program even if we don't support it, so
	// fs_copyfiles can generate cross-platform data dumps
	fileSystem->ReadFile( fullPath.c_str(), (void **)&fileBuffer, nullptr );

	common->Printf( "Load GLSL shader file: %s -> %s\n", fullPath.c_str(), fileBuffer ? "success" : "fail" );

	if ( !fileBuffer) {
		return;
	}

	// copy to stack memory and free
	buffer = (char *)_alloca( strlen( fileBuffer ) + 1);
	strcpy( buffer, fileBuffer );
	fileSystem->FreeFile( fileBuffer );

	switch ( type ) {
		case GL_VERTEX_SHADER:
			// create vertex shader
			shaderProgram->vertexShader = qglCreateShader( GL_VERTEX_SHADER );
			qglShaderSource( shaderProgram->vertexShader, 1, (const GLchar **)&buffer, 0 );
			qglCompileShader( shaderProgram->vertexShader);
			break;
		case GL_FRAGMENT_SHADER:
			// create fragment shader
			shaderProgram->fragmentShader = qglCreateShader( GL_FRAGMENT_SHADER );
			qglShaderSource( shaderProgram->fragmentShader, 1, (const GLchar **)&buffer, 0 );
			qglCompileShader( shaderProgram->fragmentShader);
			break;
		default:
			common->Printf( "R_LoadGLSLShader: unexpected type\n" );
			return;
	}
}

/*
=================
R_LinkGLSLShader

links the GLSL vertex and fragment shaders together to form a GLSL program
=================
*/
static bool R_LinkGLSLShader( shaderProgram_t *shaderProgram, bool needsAttributes ) {
	char buf[BUFSIZ];
	int len;
	GLint status;
	GLint linked;

	shaderProgram->program = qglCreateProgram();

	qglAttachShader( shaderProgram->program, shaderProgram->vertexShader);
	qglAttachShader( shaderProgram->program, shaderProgram->fragmentShader);

	if (needsAttributes) {
		qglBindAttribLocation( shaderProgram->program, 8, "attr_TexCoord" );
		qglBindAttribLocation( shaderProgram->program, 9, "attr_Tangent" );
		qglBindAttribLocation( shaderProgram->program, 10, "attr_Bitangent" );
		qglBindAttribLocation( shaderProgram->program, 11, "attr_Normal" );
		qglBindAttribLocation( shaderProgram->program, 12, "attr_Vertex" );
		qglBindAttribLocation( shaderProgram->program, 13, "attr_Color" );
	}

	glLinkProgram( shaderProgram->program );

	glGetProgramiv( shaderProgram->program, GL_LINK_STATUS, &linked );

	if (com_developer.GetBool() ) {
		qglGetShaderInfoLog( shaderProgram->vertexShader, sizeof( buf ), &len, buf);
		common->Printf( "VS:\n%.*s\n", len, buf);
		qglGetShaderInfoLog( shaderProgram->fragmentShader, sizeof( buf ), &len, buf);
		common->Printf( "FS:\n%.*s\n", len, buf);
	}

	if ( !linked) {
		common->Printf( "R_LinkGLSLShader: program failed to link\n" );
		qglGetProgramInfoLog( shaderProgram->program, sizeof( buf ), nullptr, buf);
		common->Printf( "R_LinkGLSLShader:\n%.*s\n", len, buf);
		return false;
	}

	return true;
}

/*
=================
R_ValidateGLSLProgram

makes sure GLSL program is valid
=================
*/
static bool R_ValidateGLSLProgram( shaderProgram_t *shaderProgram ) {
	GLint validProgram;

	qglValidateProgram( shaderProgram->program);
	qglGetProgramiv( shaderProgram->program, GL_VALIDATE_STATUS, &validProgram );

	if ( !validProgram ) {
		common->Printf( "R_ValidateGLSLProgram: program invalid\n" );
		return false;
	}

	return true;
}


static void RB_GLSL_GetUniformLocations( shaderProgram_t *shader ) {
	int	i;
	char	buffer[32];

	GL_UseProgram( shader );

	shader->localLightOrigin = qglGetUniformLocation( shader->program, "u_lightOrigin" );
	shader->localViewOrigin = qglGetUniformLocationglGetUniformLocation( shader->program, "u_viewOrigin" );
	shader->lightProjectionS = qglGetUniformLocation( shader->program, "u_lightProjectionS" );
	shader->lightProjectionT = qglGetUniformLocation( shader->program, "u_lightProjectionT" );
	shader->lightProjectionQ = qglGetUniformLocation( shader->program, "u_lightProjectionQ" );
	shader->lightFalloff = qglGetUniformLocation( shader->program, "u_lightFalloff" );
	shader->bumpMatrixS = qglGetUniformLocation( shader->program, "u_bumpMatrixS" );
	shader->bumpMatrixT = qglGetUniformLocation( shader->program, "u_bumpMatrixT" );
	shader->diffuseMatrixS = qglGetUniformLocation( shader->program, "u_diffuseMatrixS" );
	shader->diffuseMatrixT = qglGetUniformLocation( shader->program, "u_diffuseMatrixT" );
	shader->specularMatrixS = qglGetUniformLocation( shader->program, "u_specularMatrixS" );
	shader->specularMatrixT = qglGetUniformLocation( shader->program, "u_specularMatrixT" );
	shader->colorModulate = qglGetUniformLocation( shader->program, "u_colorModulate" );
	shader->colorAdd = qglGetUniformLocation( shader->program, "u_colorAdd" );
	shader->diffuseColor = qglGetUniformLocation( shader->program, "u_diffuseColor" );
	shader->specularColor = qglGetUniformLocation( shader->program, "u_specularColor" );
	shader->glColor = qglGetUniformLocation( shader->program, "u_glColor" );
	shader->alphaTest = qglGetUniformLocation( shader->program, "u_alphaTest" );
	shader->specularExponent = qglGetUniformLocation( shader->program, "u_specularExponent" );

	shader->eyeOrigin = qglGetUniformLocation( shader->program, "u_eyeOrigin" );
	shader->localEyeOrigin = qglGetUniformLocation( shader->program, "u_localEyeOrigin" );
	shader->nonPowerOfTwo = qglGetUniformLocation( shader->program, "u_nonPowerOfTwo" );
	shader->windowCoords = qglGetUniformLocation( shader->program, "u_windowCoords" );

	shader->modelViewProjectionMatrix = qglGetUniformLocation( shader->program, "u_modelViewProjectionMatrix" );

	shader->modelMatrix = qglGetUniformLocation( shader->program, "u_modelMatrix" );
	shader->textureMatrix = qglGetUniformLocation( shader->program, "u_textureMatrix" );
	// add modelView matrix uniform
	shader->modelViewMatrix = qglGetUniformLocation( shader->program, "u_modelViewMatrix" );
	// add clip plane uniform
	shader->clipPlane = qglGetUniformLocation( shader->program, "u_clipPlane" );
	// add fog matrix uniform
	shader->fogMatrix = qglGetUniformLocation( shader->program, "u_fogMatrix" );
	// add fog color uniform
	shader->fogColor = qglGetUniformLocation( shader->program, "u_fogColor" );
	// add texgen S T Q uniform
	shader->texgenS = qglGetUniformLocation( shader->program, "u_texgenS" );
	shader->texgenT = qglGetUniformLocation( shader->program, "u_texgenT" );
	shader->texgenQ = qglGetUniformLocation( shader->program, "u_texgenQ" );

	shader->attr_TexCoord = qglGetUniformLocation( shader->program, "attr_TexCoord" );
	shader->attr_Tangent = qglGetAttribLocation( shader->program, "attr_Tangent" );
	shader->attr_Bitangent = qglGetAttribLocation( shader->program, "attr_Bitangent" );
	shader->attr_Normal = qglGetAttribLocation( shader->program, "attr_Normal" );
	shader->attr_Vertex = qglGetAttribLocation( shader->program, "attr_Vertex" );
	shader->attr_Color = qglGetAttribLocation( shader->program, "attr_Color" );

	for ( i = 0; i < MAX_VERTEX_PARMS; i++ ) {
		anString::snPrintf(buffer, sizeof( buffer ), "u_vertexParm%d", i );
		shader->u_vertexParm[i] = qglGetAttribLocation( shader->program, buffer );
	}

	for ( i = 0; i < MAX_FRAGMENT_IMAGES; i++ ) {
		anString::snPrintf(buffer, sizeof( buffer ), "u_fragmentMap%d", i );
		shader->u_fragmentMap[i] = qglGetUniformLocation( shader->program, buffer );
		glUniform1i( shader->u_fragmentMap[i], i);
	}

	//k: add cubemap texture units
	for ( i = 0; i < MAX_FRAGMENT_IMAGES; i++ ) {
		anString::snPrintf( buffer, sizeof( buffer ), "u_fragmentCubeMap%d", i );
		shader->u_fragmentCubeMap[i] = qglGetUniformLocation( shader->program, buffer );
		glUniform1i( shader->u_fragmentCubeMap[i], i );
	}

	GL_CheckErrors();
	GL_UseProgram( nullptr );
}

static bool RB_GLSL_InitShaders( void ) {
	const GLSLShaderProp Props[] = {
			{ "interaction", &interactionShader, INTERACTION_VERT, INTERACTION_FRAG, "interaction.vert", "interaction.frag", 1 },

			{ "shadow", &shadowShader, SHADOW_VERT, SHADOW_FRAG, "shadow.vert", "shadow.frag", 0 },
			{ "default", &defaultShader, DEFAULT_VERT, DEFAULT_FRAG, "default.vert", "default.frag", 0 },

			{ "zfill", &depthFillShader, ZFILL_VERT, ZFILL_FRAG, "zfill.vert", "zfill.frag", 0 },
			{ "zfillClip", &depthFillClipShader, ZFILLCLIP_VERT, ZFILLCLIP_FRAG, "zfillClip.vert", "zfillClip.frag", 0 },

			{ "cubemap", &cubemapShader, CUBEMAP_VERT, CUBEMAP_FRAG, "cubemap.vert", "cubemap.frag", 0 },
			{ "reflectionCubemap", &reflectionCubemapShader, REFLECTION_CUBEMAP_VERT, CUBEMAP_FRAG, "reflectionCubemap.vert", "reflectionCubemap.frag", 0 },
			{ "fog", &fogShader, FOG_VERT, FOG_FRAG, "fog.vert", "fog.frag", 0 },
			{ "blendLight", &blendLightShader, BLENDLIGHT_VERT, FOG_FRAG, "blendLight.vert", "blendLight.frag", 0 },

			{ "interaction_blinn_phong", &interactionBlinnPhongShader, INTERACTION_BLINNPHONG_VERT, INTERACTION_BLINNPHONG_FRAG, "interaction_blinnphong.vert", "interaction_blinnphong.frag", 1 },

			{ "diffuseCubemap", &diffuseCubemapShader, DIFFUSE_CUBEMAP_VERT, CUBEMAP_FRAG, "diffuseCubemap.vert", "diffuseCubemap.frag", 0 },
			{ "texgen", &texgenShader, TEXGEN_VERT, TEXGEN_FRAG, "texgen.vert", "texgen.frag", 0 },
	};

	for ( int i = 0; i < sizeof( Props ) / sizeof( Props[0] ); i++ ) {
		const GLSLShaderProp *prop = Props + i;
		if ( R_LoadGLSLShaderProgram( prop->name, prop->program, prop->default_vertex_shader_source, prop->default_fragment_shader_source, prop->vertex_shader_source_file, prop->fragment_shader_source_file, prop->cond
				) < 0)
			return false;
	}

	return true;
}

/*
==================
R_ReloadGLSLPrograms_f
==================
*/
static void R_InitGLSLCvars( void ) {
	const char *lightModel = harm_r_lightModel.GetString();
	r_usePhong = !( lightModel && !anString::Icmp( INTERACTION_SHADER_BLINNPHONG, lightModel ) );

	float f = harm_r_specularExponent.GetFloat();
	if ( f <= 0.0f ) {
		f = 4.0f;
	}
	r_specularExponent = f;
}

void R_ReloadGLSLPrograms_f( const anCommandArgs &args ) {
	common->Printf( "----- R_ReloadGLSLPrograms -----\n" );

	if ( !RB_GLSL_InitShaders() ) {
		common->Printf( "GLSL shaders failed to init.\n" );
	}

	qglConfig.allowGLSLPath = true;

	common->Printf( "-------------------------------\n" );
}

void R_GLSL_Init( void ) {
	qglConfig.allowGLSLPath = false;

	common->Printf( "---------- R_GLSL_Init ----------\n" );

	if ( !qglConfig.GLSLAvailable ) {
		common->Printf( "Not available.\n" );
		return;
	}

	common->Printf( "Available.\n" );

	R_InitGLSLCvars();
	common->Printf( "---------------------------------\n" );
}


static void R_DeleteShaderProgram( shaderProgram_t *shaderProgram ) {
	if ( shaderProgram->program ) {
		if ( qglIsProgram( shaderProgram->program ) );
			qglDeleteProgram( shaderProgram->program );
	}

	if ( shaderProgram->vertexShader ) {
		if ( qglIsShader( shaderProgram->vertexShader ) )
			qglDeleteShader( shaderProgram->vertexShader ) ;
	}

	if ( shaderProgram->fragmentShader) {
		if ( qglIsShader( shaderProgram->fragmentShader ) )
			qglDeleteShader( shaderProgram->fragmentShader );
	}
	shaderProgram_t = shaderProgram
}

#define LOG_LEN 1024
static GLint R_CreateShader( GLenum type, const char *source) {
	GLint shader = 0;
	GLint status;

	shader = glCreateShader(type);
	if ( shader == 0) {
		common->Error( "%s::glCreateShader(%s) error!\n", __func__, type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER" );
		return 0;
	}

	qglShaderSource( shader, 1, (const GLchar **)&source, 0 );
	qglCompileShader( shader);

	qglGetShaderiv( shader, GL_COMPILE_STATUS, &status );
	if ( !status ) {
		GLchar log[LOG_LEN];
		qglGetShaderInfoLog( shader, sizeof( GLchar) * LOG_LEN, nullptr, log );
		common->Error( "%s::glCompileShader(%s) -> %s!\n", __func__, type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : "GL_FRAGMENT_SHADER", log);
		qglDeleteShader( shader);
		shader = 0;
	}

	return shader;
}

static GLint R_CreateProgram( GLint vertShader, GLint fragShader, bool needsAttributes = true) {
	GLint program = 0;
	GLint result;

	program = glCreateProgram();
	if (program == 0) {
		common->Error( "%s::glCreateProgram() error!\n", __func__);
		return 0;
	}

	qglAttachShader(program, vertShader);
	qglAttachShader(program, fragShader);

	if (needsAttributes) {
		qglBindAttribLocation(program, 8, "attr_TexCoord" );
		qglBindAttribLocation(program, 9, "attr_Tangent" );
		qglBindAttribLocation(program, 10, "attr_Bitangent" );
		qglBindAttribLocation(program, 11, "attr_Normal" );
		qglBindAttribLocation(program, 12, "attr_Vertex" );
		qglBindAttribLocation(program, 13, "attr_Color" );
	}

	qglLinkProgram( program );
	qglGetProgramiv(program, GL_LINK_STATUS, &result);
	if ( !result) {
		GLchar log[LOG_LEN];
		qglGetProgramInfoLog(program, sizeof( GLchar) * LOG_LEN, nullptr, log );
		common->Error( "%s::glLinkProgram() -> %s!\n", __func__, log );
		qglDeleteProgram( program );
		program = 0;
	}

	qglValidateProgram( program );
	qglGetProgramiv( program, GL_VALIDATE_STATUS, &result);
	if ( !result ) {
		GLchar log[LOG_LEN];
		qglGetProgramInfoLog( program, sizeof( GLchar) * LOG_LEN, nullptr, log );
		common->Error( "[Harmattan]: %s::glValidateProgram() -> %s!\n", __func__, log );
		qglDeleteProgram( program );
		program = 0;
	}

	return program;
}

bool R_CreateShaderProgram( shaderProgram_t *shaderProgram, const char *vert, const char *frag , const char *name) {
	R_DeleteShaderProgram( shaderProgram);
	shaderProgram->vertexShader = R_CreateShader( GL_VERTEX_SHADER, vert);
	if ( shaderProgram->vertexShader == 0)
		return false;

	shaderProgram->fragmentShader = R_CreateShader( GL_FRAGMENT_SHADER, frag);
	if ( shaderProgram->fragmentShader == 0)
	{
		R_DeleteShaderProgram( shaderProgram);
		return false;
	}

	shaderProgram->program = R_CreateProgram( shaderProgram->vertexShader, shaderProgram->fragmentShader);
	if ( shaderProgram->program == 0)
	{
		R_DeleteShaderProgram( shaderProgram);
		return false;
	}

	RB_GLSL_GetUniformLocations( shaderProgram);
#ifdef _HARM_SHADER_NAME
	strncpy( shaderProgram->name, name, sizeof( shaderProgram->name));
#else
	( void )(name);
#endif

	return true;
}

int R_LoadGLSLShaderProgram( const char *name, shaderProgram_t *const program, const char *default_vertex_shader_source, const char *default_fragment_shader_source, const char *vertex_shader_source_file, const char *fragment_shader_source_file, int cond ) {
	shaderProgram_t *program = new  static_cast<shaderProgram_t *>( sprogram );
	//shaderProgram_t *program = new program;

	common->Printf( " Load GLSL shader program: %s\n", name );
	common->Printf( " Load external shader source: Vertex(%s), Fragment(%s)\n", vertex_shader_source_file, fragment_shader_source_file );
	R_LoadGLSLShader( vertex_shader_source_file, program, GL_VERTEX_SHADER );
	R_LoadGLSLShader( fragment_shader_source_file, program, GL_FRAGMENT_SHADER );

	if ( !R_LinkGLSLShader( program, true ) && !R_ValidateGLSLProgram( program ) ) {
		common->Printf( " Load internal shader source\n\n" );
		if ( !R_CreateShaderProgram( program, default_vertex_shader_source, default_fragment_shader_source, name ) ) {
			common->Error( "[ERROR]: Load internal shader program fail!" );
			return -1;
		} else {
			return 2;
		}
	} else {
		RB_GLSL_GetUniformLocations( program );

#ifdef _HARM_SHADER_NAME
		strncpy( program->name, name, sizeof(program->name ) );
#endif
	delete [] program;
		return 1;
	}
}

int LoadGLSLShaderProgram( const char *name, shaderProgram_t *program, const char *defaultVertexShaderSource, const char *defaultFragmentShaderSource, const char *vertexShaderSourceFile, const char *fragmentShaderSourceFile, int cond ) {
	//memset( program, 0, sizeof(  ) );
	//shaderProgram_t *program = new static_cast<shaderProgram_t *>( program );

	common->Printf( "Loading GLSL shader vertex/fragment program: %s\n", name );

	R_LoadExternalShaderSource( vertexShaderSourceFile, program, fragmentShaderSourceFile );

	if ( !R_LinkGLSLShader( program, true ) &&
		 !R_ValidateGLSLProgram( program ) ) {
		if ( !R_CreateShaderProgram( program, defaultVertexShaderSource, defaultFragmentShaderSource, name ) ) {
			common->Error( "[ERROR] Failed to Load GL Shader Program %s\n", program->name );
			return -1;
		} else {
			return 2;
		}
	} else {
		RB_GLSL_GetUniformLocations( program );
		common->Printf( "[R_LoadGLSLShaderProgram] %d\n\n", name );
		strncpy( program->name, name, sizeof( program->name ) );
	return 1;
	}
}

void R_CheckGLSLCvars( void ) {
	if ( r_tst_lightModel.IsModified() ) {
		const char *lightModel = r_tst_lightModel.GetString();
		r_usePhong = !( lightModel && !anString::Icmp(HARM_INTERACTION_SHADER_BLINNPHONG, lightModel ) );
		r_tst_lightModel.ClearModified();
	}

	if ( r_tst_specularExponent.IsModified() ) {
		float f = r_tst_specularExponent.GetFloat();
		if (f <= 0.0f)
			f = 4.0f;
		r_specularExponent = f;
		r_tst_specularExponent.ClearModified();
	}
}