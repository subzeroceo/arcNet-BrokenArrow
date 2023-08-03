


/*===================================================================================
====================================================================================
====================================================================================
====================================================================================
====================================================================================
====================================================================================
In this updated version, the private section now includes additional variables specific to OpenGL 4.5 and other common features:

blendSrcFactor and blendDstFactor: These GLint variables represent the source and destination factors used in blending operations.
depthTestEnabled: This GLboolean variable indicates whether depth testing is enabled.
cullFaceEnabled: This GLboolean variable indicates whether face culling is enabled.
anisotropyLevel: This GLfloat variable represents the level of anisotropy for texture filtering.
These variables provide more control and flexibility for configuring the blending, depth testing,
face culling, and texture filtering settings in the backend render system.

Please note that this is just an example, and the specific values and details of these variables would
need to be set and used appropriately within the functions of the class based on what you require.
In this example, arcNet's BackendRenderSystem class incorporates the elements in the following below:.

It includes functions such as InitOpenGL() for initializing OpenGL, AllocBuffers() and PurgeBuffers() for allocating
and purging buffers,
FreeBuffers() for freeing buffers, ReloadBuffers() and ShutdownBuffers() for reloading and shutting down buffers.

It also includes functions like BeginFrame() and BeginFrameBuffers() for handling the beginning of frames,
InitAMD() for initializing AMD-specific features, and IsAMDRyzenInitiated() as a virtual boolean function
to check if AMD Ryzen is initiated.

Additionally, there are functions such as DrawRenderView() and SetupRenderView() for rendering and setting up the
 view, and ActivateATIExtensions() for activating ATI extensions.

Within the protected section, there are functions like CreateBuffers() for creating buffers,
BindVertexBuffer() and BindIndexBuffer() for binding vertex and index buffers,
SetStateBits() for setting OpenGL state bits, SetBlendingBits() for setting blending bits,
SetDepthTestBits() for setting depth test bits, SetCullingBits() for setting culling bits, and
SetAnisotrophy() for setting anisotropy.

In the private section, there are the vertexBuffer, indexBuffer, textureBuffer, and frameBuffer variables representing the OpenGL buffers.
The ClearBuffers() function is responsible for clearing the buffers, and the DrawIndexed() function is responsible for
rendering the data using indexed rendering.

Additionally, there are two randomly generated functions: DSAFeature() and GLARPB(),
which could represent features related to Direct State Access (DSA) or GL_ARB extensions.

aRcNetBackendSystem
arcNetBackendRendersystem
arcNetBackend
arcNetRender
arcNetRenderSystem
*/

BackendRenderSystem() {
	isInitisialized = false;
        // Clean up memory
    delete backendRenderer;
    return 0;
}

void arcNetRenderSystem::InitAMD() const {
    // Implementation for initializing AMD
}

 void Initialize() {
	isInitisialized = false;
	amdIsActive = false;;
	textureBuffer; // 0;
	frameBuffer; // 0;
	vertexBuffer; // 0;
	indexBuffer; // 0;
	textureBuffer; // 0;
	uniformBuffer; // 0;
	drawBuffers[2]; // 0;

    shaderProgram; // 0;
	vertexArrayObject; // 0;
	indexArrayObject; // 0;
    // Implementation for initializing AMD-specific features

 }


void arcNetRenderSystem::InitOpenGL( void )

void arcNetRenderSystem::AllocBuffers( void )

void arcNetRenderSystem::FreeBuffers( void )
void arcNetRenderSystem::ReloadBuffers( void )

void arcNetRenderSystem::ShutdownBuffers( void )

void arcNetRenderSystem::BeginFrame( int winWidth, int winHeight )
void arcNetRenderSystem::BeginFrameBuffers( void );

void arcNetRenderSystem::InitAMD( void ); const {
    // Implementation for initializing AMD
}

void arcNetRenderSystem::Shutdown( void ) const {
"Shutting down the render system"
	// set depth bounds
}
	if ( glConfig.depthBoundsTestAvailable && r_useDepthBoundsTest.GetBool() ) {
		qglDepthBoundsEXT( surf->scissorRect.zmin, surf->scissorRect.zmax );
	}

bool arcNetRenderSystem::IsAMDRyzenInitiated( void ) const {
    // Implementation for checking if AMD Ryzen is initiated
   	// Return true or false based on the condition
    return true;
}

void arcNetRenderSystem::GetGLSettings( int& width, int& height );
void arcNetRenderSystem::PrintMemInfo( MemInfo_t *mi );

void arcNetRenderSystem::DrawRenderView( void )
void arcNetRenderSystem::InitRenderView( void )
void arcNetRenderSystem::ATI_ExtensionSystems( void )

void arcNetRenderSystem::CreateBuffers( void )
void arcNetRenderSystem::BindVertexBuffer( void )
void arcNetRenderSystem::BindIndexBuffer( void )
void arcNetRenderSystem::BindFramebuffer()
void arcNetRenderSystem::PurgeBuffers( void )
void arcNetRenderSystem::ClearBuffers()
void arcNetRenderSystem::EndFrame( int *frontEndMsec, int *backEndMsec, int *numVerts = nullptr, int *numIndexes = nullptr ) = 0;

void arcNetRenderSystem::SetUniforms() = 0;
void arcNetRenderSystem::DrawArrays() = 0;
void arcNetRenderSystem::DrawElements() = 0;

void GL_CheckErrors( void ) {
	char s[64];

	int err = qglGetError();
	if ( err == GL_NO_ERROR ) {
		return;
	}
	if ( r_ignoreGLErrors->integer ) {
		return;
	}
	switch ( err ) {
	case GL_INVALID_ENUM:
		strcpy( s, "GL_INVALID_ENUM" );
		break;
	case GL_INVALID_VALUE:
		strcpy( s, "GL_INVALID_VALUE" );
		break;
	case GL_INVALID_OPERATION:
		strcpy( s, "GL_INVALID_OPERATION" );
		break;
	case GL_STACK_OVERFLOW:
		strcpy( s, "GL_STACK_OVERFLOW" );
		break;
	case GL_STACK_UNDERFLOW:
		strcpy( s, "GL_STACK_UNDERFLOW" );
		break;
	case GL_OUT_OF_MEMORY:
		strcpy( s, "GL_OUT_OF_MEMORY" );
		break;
	default:
		sprintf( s, sizeof( s ), "%i", err );
		break;
	}

	Error( ERR_VID_FATAL, "GL_CheckErrors: %s", s );
}


//virtual void CopyFramebufferToBackBuffer( void )
void GL_State( int stateVector ) {

}
void GetOpenGLStateBits( void } {
	void GL_SetDefaultState( void ) {
	qglClearDepth( 1.0f );

	qglCullFace( GL_FRONT );

	qglColor4f( 1,1,1,1 );

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SetCurrentTextureUnit( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TextureAnisotropy( r_textureAnisotropy->value );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SetCurrentTextureUnit( 0 );
	}

	qglEnable( GL_TEXTURE_2D );
	GL_TextureMode( r_textureMode->string );
	GL_TextureAnisotropy( r_textureAnisotropy->value );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState( GL_VERTEX_ARRAY );

	//
	// make sure our GL state vector is set correctly
	//
	qglState.qglStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );

	if ( qglPNTrianglesiATI ) {
		int maxtess;
		// get max supported tesselation
		qglGetIntegerv( GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI, (GLint*)&maxtess );

		glConfig.ATIMaxTruformTess = 7;

		// cap if necessary
		if ( r_ati_truform_tess->value > maxtess ) {
			ri.Cvar_Set( "r_ati_truform_tess", va( "%d", maxtess ) );
		}

		// set Wolf defaults
		qglPNTrianglesiATI( GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI, r_ati_truform_tess->value );
	}

	if ( glConfig.anisotropicAvailable ) {
		float maxAnisotropy;

		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
		qglConfig.maxAnisotropy = maxAnisotropy;

		// set when rendering
	   qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, qglConfig.maxAnisotropy );
	}
}
void GetGLBlendingBits( void ) {

}
void SetGLAnisotropy( void ) {
    // Declare and initialize variables
    int maxAnisotropyLevel = 16; // Maximum supported anisotropy level
    int desiredAnisotropyLevel = 8; // Desired anisotropy level (can be modified as needed)

    // Check if anisotropic filtering is supported
    GLint maxAnisotropy;
    qglGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );

    if ( maxAnisotropy > 1) {
        // Anisotropic filtering is supported, set the desired level
        GLint desiredLevel = std::min(maxAnisotropyLevel, desiredAnisotropyLevel );
        qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, desiredLevel );
    } else {
        // Anisotropic filtering is not supported, handle it accordingly
        // For example, you can disable anisotropic filtering or use a fallback method
    }

}

void ClearBuffers() {

}
void DrawIndexed() {

}

~BackendRenderSystem(

}

void BackendRenderSystem::InitAMD() const {

}

bool MyClass::IsAMDRyzenInitiated() const {
    // Implementation for checking if AMD Ryzen is initiated
    // Return true or false based on the condition
    //return true;//isInitisialized
}

   void Init() override {        // Implementation for initializing the render system
        RB_PrintF( "Initializing the render system" )
    }

    void Shutdown( void ) const {
        // Implementation for shutting down the render system
        printf( "Shutting down the render system" );
    }

    void FlushLevelImages() override {
        // Implementation for flushing level images
       printf( "Flushing level images" );
    }

    // Implement other functions of the RenderSystem interface
    // ...
};

int main() {
    MyRenderSystem renderSystem;
    renderSystem.Init();
    renderSystem.BeginFrame(800, 600);
    // Perform rendering operations
    renderSystem.EndFrame();
    renderSystem.Shutdown();

    return 0;
}

arcNetRenderSystem() {
	isInitisialized = false;
	// Clean up memory
		delete backendRenderer;
	return 0;
}


    // Query the number of supported extensions
    GLint numExtensions;
    qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
     maxExtensions;
    glGetIntegerv( GL_MAX_EXTENSIONS, &maxExtensionCount);

	RB_PrintF( "Total number of supported OpenGL extensions: ", numExtensions );
	RB_PrintF( "Maximum number of supported OpenGL extensions: ", maxExtensions );

	// not done yet i know below this is called before it exists shes not in the
	// loop i know. its ok give me a minute. ;)
	backendRs->extensionCount = numExtensions;

    // Print individual extension names (optional)
    for ( GLint i = 0; i < numExtensions; ++i ) {
        const GLubyte* extension = qglGetStringi( GL_EXTENSIONS, i );
		RB_PrintF( "Extension Formated Name:%s/n", extension );
		"Total Available Extensions:", extensionCount );
// Print individual extension names with total count
    printf( "Available OpenGL extensions:\n" );
    for ( GLint i = 0; i < numExtensions; ++i) {
        const GLubyte* extension = glGetStringi(GL_EXTENSIONS, i);
        printf( "Extension Formatted Name: %-30s", extension);
        printf( "Total Available Extensions: %d\n", numExtensions);
	}
    return 0;
}GL_AMD_performance_monitor

class arcNetRenderSystem {
public:
	virtual					~arcNetRenderSystem() {}
	virtual					arcNetRenderSystem()
								// for everything in this class for the functions to be setup and init calls
								// sets up the backend render system for the main system to give her a phone call
								// because shes lonely and wants to start up. =-)
								// NOTE: remove these comments later lol
    virtual void			Initialize();


	// Initialize the backend render system and the middle man no more in-between-system function
	// makes it possible to use the backend render system without the extra calls and jumps to try and save time and more efficiency.
	// Registers console commands and clears the list
	virtual void			InitOpenGL( void );

	virtual void			AllocBuffers( void );

							// Freeing buffers: This generally involves releasing or deallocating the memory used by the buffers,
							// making them available for other uses. It implies a more permanent action compared to clearing,
							// which is typically done within the rendering pipeline.
	virtual void			FreeBuffers( void );
	virtual void			ReloadBuffers( void );

							// Shutting down buffers: This typically implies a complete termination or closure of the buffers,
							// where they are no longer in use and any associated resources
							// are released. It may involve releasing GPU resources,
							// closing connections, or performing other cleanup tasks.
	virtual void			ShutdownBuffers( void );

    // Adds buffers to the system begin the buffers and their frames
    //virtual void AddBuffers() = 0; begin frame is essitially addbuffers to those whom dont understand
	virtual void			BeginFrame( int winWidth, int winHeight );
	virtual void			BeginFrameBuffers( void );
	virtual void			InitAMD( void );
	virtual bool			IsAMDRyzenInitiated( void );

	virtual void			GetGLSettings( int& width, int& height );
	virtual void			PrintMemInfo( MemInfo_t *mi );

	virtual void			DrawRenderView( void );
	virtual void			InitRenderView( void ) = 0;
	virtual void			ATI_ExtensionSystems( void ) = 0;

							// create the buffers for the init and rendering of the view( s)
	virtual void			CreateBuffers( void ) = 0;
	virtual void			BindVertexBuffer( void ) = 0;	// are we binding the vertex buffer? since we dont bind so much as older versions, less binding?
	virtual void			BindIndexBuffer( void ) = 0;	// are we binding the vertex buffer? since we dont bind so much as older versions, less binding?
		    void			BindFramebuffer() = 0;


						    // Frees all the buffersPurging buffers: This could mean freeing up resources associated with the buffers,
							// or freeing up memory associated with the buffers, such as deallocating memory or releasing any other system resources they hold.
							// It could also involve resetting the buffers to their initial state.
	virtual void 			PurgeBuffers( void ) = 0;
    		void 			ClearBuffers() = 0; // makes sure all the buffers for the next frame are cleared.

	// if the pointers are not nullptr, timing info will be returned and prey it ends the framebuffers
	virtual void			EndFrame( int *frontEndMsec, int *backEndMsec, int *numVerts = nullptr, int *numIndexes = nullptr ) = 0;

							// we will be using these functions to set and draw uniform gl ext
							// and Open GL Vertex Arrays and Elements. part of Render views rendering
							// and debug use as well? ATI/AMD specfic?
    		void			SetUniforms() = 0;
		    void			DrawArrays() = 0;
		    void			DrawElements() = 0;

	//virtual void			CopyFramebufferToBackBuffer( void ) = 0;
	virtual void			GL_State( int stateVector ) = 0;
	virtual void			GetOpenGLStateBits( void ) = 0;
	virtual void			GetGLBlendingBits( void ) = 0;
	virtual void			SetGLAnisotropy( void ) = 0;

							// Clearing buffers: This typically refers to clearing the content of the buffers, such as color buffers,
							//depth buffers, or stencil buffers, to prepare them for rendering a new frame or scene.
	void					ClearBuffers() = 0;
	virtual void			DrawIndexed() = 0;

/** ATI/AMD specific comprehensive list of extensions. Please Dualy NOTE: these are not implemented in the backend
they are also subject to change, and may be removed at any time.  They also may not be available or supported.
Do note that also may not be compatable with the end product. keep these in mind when using them.

    GL_AMD_conservative_depth, GL_AMD_depth_clamp_separate,
    GL_AMD_draw_buffers_blend, GL_AMD_framebuffer_multisample_advanced,
    GL_AMD_multi_draw_indirect, GL_AMD_performance_monitor,
    GL_AMD_pinned_memory, GL_AMD_query_buffer_object,
    GL_AMD_seamless_cubemap_per_texture, GL_AMD_shader_stencil_export,
    GL_AMD_shader_trinary_minmax, GL_AMD_texture_texture4,
    GL_AMD_vertex_shader_layer, GL_AMD_vertex_shader_viewport_index,
    GL_ATI_blend_equation_separate, GL_ATI_blend_equation_separate,
    GL_ATI_draw_buffers, GL_ATI_fragment_shader, GL_ATI_meminfo,
    GL_ATI_separate_stencil, GL_ATI_texture_compression_3dc,
    GL_ATI_texture_env_combine3, GL_ATI_texture_float,
    GL_ATI_texture_mirror_once, GL_EXT_EGL_image_storage, GL_EXT_EGL_sync,
    GL_AMD_framebuffer_multisample_advanced, GL_AMD_performance_monitor,
 GL_ATI_blend_equation_separate
*/GL_ATI_separate_stencil
	// which extension for DSA? for GL 4.6.
	virtual void			DSATestFeatures() = 0;
	virtual void			GLARPBImplementations() = 0;

    		void			FinalInitializationRenderView() = 0;
    		void			RenderView() = 0;

	virtual void 			RandomFunction1() = 0;
	virtual void			AMD_GpuSystems() = 0;
	virtual int				TotolSupportedExtensions( void ) = 0;

	virtual void			ToggleSmpFrame( void ) = 0;
							RendererStrLog = 0;
	virtual int				GetFrameCount() const = 0;
	virtual int				GetMaxFrameCount() const = 0;
private:
	bool					isInitialized;
	bool 					amdIsActive // amd has been detected and present.

    bool isInitialized;
    int swapBuffers;
	//anisoLevel_t anisoLevel; // need to create a table enum of sorts maybe ? idk yet if it woudl be beneficial yet or nesssesary
	ansiotrophicFilter; // not defined yet.
	GLlint maxAnsiotrophicFilter;
	GLlint minAnsiotrophicFilter;

	// no antialias yet
	// but maybe later

	// Private variables and  for OpenGL 4.6 buffers and vars
	GLuint textureBuffer;
	GLuint frameBuffer;
	GLuint vertexBuffer;
	GLuint indexBuffer;
	GLuint textureBuffer;
	GLuint uniformBuffer;
	GLenum drawBuffers[2];

	float *depthBuffer;

    GLuint shaderProgram;
	GLuint vertexArrayObject;
	GLuint indexArrayObject;

	//const glExtensions_t	*extension // extension names and or types originally for amd and ati exts, but it can branch out to other extensions
	int	maxExtensionCount, totalExtCount, extensionCount;,

	int frameCount, totalFrameCount;
}; extern arcNetRenderSystem *backendRs;


enum ansiotrophicFilter_t {
    ANISO_OFF,              // No anisotropic filtering
    ANISO_LOW,              // Low anisotropic filtering level
    ANISO_MEDIUM,           // Medium anisotropic filtering level
    ANISO_HIGH              // High anisotropic filtering level
};

typedef GLfloat ansiotrophicFilter_t; // Define an alias for the anisotropic filter type

ansiotrophicFilter_t ANISOTROPHICFILTER_OFF;    // Placeholder variable for anisotropic filtering
GLlint maxAnsiotrophicFilter;                   // Maximum anisotropic filtering value
GLlint minAnsiotrophicFilter;