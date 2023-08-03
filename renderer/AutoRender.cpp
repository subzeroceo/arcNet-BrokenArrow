#pragma hdrstop
#include "../idlib/Lib.h"
#include "tr_local.h"

/*
====================
GL_Scissor
====================
*/
void GL_Scissor( int x, int y, int w, int h ) {
	qglScissor( x, y, w, h );
}

/*
====================
GL_Viewport
====================
*/
void GL_Viewport( int x, int y, int w, int h ) {
	qglViewport( x, y, w, h );
}

/*
====================
GL_PolygonOffset
====================
*/
void GL_PolygonOffset( float scale, float bias ) {
	backEnd.qglState.polyOfsScale = scale;
	backEnd.qglState.polyOfsBias = bias;
	if ( backEnd.qglState.glStateBits & GLS_POLYGON_OFFSET ) {
		qglPolygonOffset( scale, bias );
	}
}

/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest( const float zMin, const float zMax ) {
	if ( !glConfig.depthBoundsTestAvailable || zMin > zMax ) {
		return;
	}

	qglDepthRange( zMin, zMax );
	qglDepthFunc( GL_LEQUAL );

	if ( zMin == 0.0f && zMax == 0.0f ) {
		globalImages->blackImage->Bind();
		qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		globalImages->blackImage->Unbind();
		return;
	}

	if ( zMin == 0.0M && zMax == 0.0f ) {
		qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );
	} else {
		qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );
		qglDepthBoundsEXT( zMin, zMax );
	}
}

void GL_END( void ) {

}

/*
=================
R_CheckExtension
=================
*/
bool R_CheckExtension( const char *name ) {
	if ( !strstr( qglConfig.extensionsStr, name ) ) {
		common->Printf( "X..%s not found\n", name );
		return false;
	}

	common->Printf( "...using %s\n", name );
	return true;
}



//=========================================================================
//===========================================================================

/*
===========================================================================

===========================================================================
*/

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"
#include "DXT/DXTCodec.h" // Carl

// RB begin
#if defined(_WIN32)

// Vista OpenGL wrapper check
#include "../sys/win32/win_local.h"
#endif
// RB end

// Koz begin
#undef strncmp // Koz fixme to prevent conflict with oculus SDK.
#include "vr/Vr.h"
#ifdef USE_OVR
#include "libs\LibOVR\Include\OVR_CAPI.h"
#endif
// Koz end

// DeviceContext bypasses RenderSystem to work directly with this
idGuiModel* tr_guiModel;

// functions that are not called every frame
glconfig_t	glConfig;

idCVar r_requestStereoPixelFormat( "r_requestStereoPixelFormat", "1", CVAR_RENDERER, "Ask for a stereo GL pixel format on startup" );
idCVar r_debugContext( "r_debugContext", "0", CVAR_RENDERER, "Enable various levels of context debug." );
idCVar r_glDriver( "r_glDriver", "", CVAR_RENDERER, "\"opengl32\", etc." );
idCVar r_skipIntelWorkarounds( "r_skipIntelWorkarounds", "0", CVAR_RENDERER | CVAR_BOOL, "skip workarounds for Intel driver bugs" );
// RB: disabled 16x MSAA
idCVar r_multiSamples( "r_multiSamples", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "number of antialiasing samples", 0, 8 );
// RB end
idCVar r_vidMode( "r_vidMode", "0", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_INTEGER, "fullscreen video mode number" );
idCVar r_displayRefresh( "r_displayRefresh", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_NOCHEAT, "optional display refresh rate option for vid mode", 0.0f, 240.0f );
#ifdef WIN32
idCVar r_fullscreen( "r_fullscreen", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = windowed, 1 = full screen on monitor 1, 2 = full screen on monitor 2, etc" );
#else
// DG: add mode -2 for SDL, also defaulting to windowed mode, as that causes less trouble on linux
idCVar r_fullscreen( "r_fullscreen", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "-2 = use current monitor, -1 = (reserved), 0 = windowed, 1 = full screen on monitor 1, 2 = full screen on monitor 2, etc" );
// DG end
#endif
idCVar r_customWidth( "r_customWidth", "1280", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen width. set r_vidMode to -1 to activate" );
idCVar r_customHeight( "r_customHeight", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen height. set r_vidMode to -1 to activate" );
idCVar r_windowX( "r_windowX", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowY( "r_windowY", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowWidth( "r_windowWidth", "1280", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowHeight( "r_windowHeight", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );

idCVar r_useViewBypass( "r_useViewBypass", "1", CVAR_RENDERER | CVAR_INTEGER, "bypass a frame of latency to the view" );
idCVar r_useLightPortalFlow( "r_useLightPortalFlow", "1", CVAR_RENDERER | CVAR_BOOL, "use a more precise area reference determination" );
idCVar r_singleTriangle( "r_singleTriangle", "0", CVAR_RENDERER | CVAR_BOOL, "only draw a single triangle per primitive" );
idCVar r_checkBounds( "r_checkBounds", "0", CVAR_RENDERER | CVAR_BOOL, "compare all surface bounds with precalculated ones" );
idCVar r_useConstantMaterials( "r_useConstantMaterials", "1", CVAR_RENDERER | CVAR_BOOL, "use pre-calculated material registers if possible" );
idCVar r_useSilRemap( "r_useSilRemap", "1", CVAR_RENDERER | CVAR_BOOL, "consider verts with the same XYZ, but different ST the same for shadows" );
idCVar r_useNodeCommonChildren( "r_useNodeCommonChildren", "1", CVAR_RENDERER | CVAR_BOOL, "stop pushing reference bounds early when possible" );
idCVar r_useShadowSurfaceScissor( "r_useShadowSurfaceScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor shadows by the scissor rect of the interaction surfaces" );
idCVar r_useCachedDynamicModels( "r_useCachedDynamicModels", "1", CVAR_RENDERER | CVAR_BOOL, "cache snapshots of dynamic models" );
idCVar r_useSeamlessCubeMap( "r_useSeamlessCubeMap", "1", CVAR_RENDERER | CVAR_BOOL, "use ARB_seamless_cube_map if available" );
idCVar r_useSRGB( "r_useSRGB", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 = both texture and framebuffer, 2 = framebuffer only, 3 = texture only" );
idCVar r_maxAnisotropicFiltering( "r_maxAnisotropicFiltering", "8", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "limit aniso filtering" );
idCVar r_useTrilinearFiltering( "r_useTrilinearFiltering", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "Extra quality filtering" );
// RB: not used anymore
idCVar r_lodBias( "r_lodBias", "0.5", CVAR_RENDERER | CVAR_ARCHIVE, "UNUSED: image lod bias" );
// RB end

idCVar r_useStateCaching( "r_useStateCaching", "1", CVAR_RENDERER | CVAR_BOOL, "avoid redundant state changes in GL_*() calls" );

idCVar r_znear( "r_znear", "3", CVAR_RENDERER | CVAR_FLOAT, "near Z clip plane distance", 0.001f, 200.0f );

idCVar r_ignoreGLErrors( "r_ignoreGLErrors", "0", CVAR_RENDERER | CVAR_BOOL, "ignore GL errors" );
idCVar r_swapInterval( "r_swapInterval", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = tear, 1 = swap-tear where available, 2 = always v-sync" );

idCVar r_gamma( "r_gamma", "1.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 3.0f );
idCVar r_brightness( "r_brightness", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 2.0f );

idCVar r_jitter( "r_jitter", "0", CVAR_RENDERER | CVAR_BOOL, "randomly subpixel jitter the projection matrix" );

idCVar r_skipStaticInteractions( "r_skipStaticInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip interactions created at level load" );
idCVar r_skipDynamicInteractions( "r_skipDynamicInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip interactions created after level load" );
idCVar r_skipSuppress( "r_skipSuppress", "0", CVAR_RENDERER | CVAR_BOOL, "ignore the per-view suppressions" );
idCVar r_skipPostProcess( "r_skipPostProcess", "0", CVAR_RENDERER | CVAR_BOOL, "skip all post-process renderings" );
idCVar r_skipInteractions( "r_skipInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip all light/surface interaction drawing" );
idCVar r_skipDynamicTextures( "r_skipDynamicTextures", "0", CVAR_RENDERER | CVAR_BOOL, "don't dynamically create textures" );
idCVar r_skipCopyTexture( "r_skipCopyTexture", "0", CVAR_RENDERER | CVAR_BOOL, "do all rendering, but don't actually copyTexSubImage2D" );
idCVar r_skipBackEnd( "r_skipBackEnd", "0", CVAR_RENDERER | CVAR_BOOL, "don't draw anything" );
idCVar r_skipRender( "r_skipRender", "0", CVAR_RENDERER | CVAR_BOOL, "skip 3D rendering, but pass 2D" );
// RB begin
idCVar r_skipRenderContext( "r_skipRenderContext", "0", CVAR_RENDERER | CVAR_BOOL, "DISABLED: nullptr the rendering context during backend 3D rendering" );
// RB end
idCVar r_skipTranslucent( "r_skipTranslucent", "0", CVAR_RENDERER | CVAR_BOOL, "skip the translucent interaction rendering" );
idCVar r_skipAmbient( "r_skipAmbient", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all non-interaction drawing" );
idCVar r_skipNewAmbient( "r_skipNewAmbient", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "bypasses all vertex/fragment program ambient drawing" );
idCVar r_skipBlendLights( "r_skipBlendLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all blend lights" );
idCVar r_skipFogLights( "r_skipFogLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all fog lights" );
idCVar r_skipDeforms( "r_skipDeforms", "0", CVAR_RENDERER | CVAR_BOOL, "leave all deform materials in their original state" );
idCVar r_skipFrontEnd( "r_skipFrontEnd", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all front end work, but 2D gui rendering still draws" );
idCVar r_skipUpdates( "r_skipUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't accept any entity or light updates, making everything static" );
idCVar r_skipDecals( "r_skipDecals", "0", CVAR_RENDERER | CVAR_BOOL, "skip decal surfaces" );
idCVar r_skipOverlays( "r_skipOverlays", "0", CVAR_RENDERER | CVAR_BOOL, "skip overlay surfaces" );
idCVar r_skipSpecular( "r_skipSpecular", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_CHEAT | CVAR_ARCHIVE, "use black for specular1" );
idCVar r_skipBump( "r_skipBump", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "uses a flat surface instead of the bump map" );
idCVar r_skipDiffuse( "r_skipDiffuse", "0", CVAR_RENDERER | CVAR_BOOL, "use black for diffuse" );
idCVar r_skipSubviews( "r_skipSubviews", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = don't render any gui elements on surfaces" );
idCVar r_skipGuiShaders( "r_skipGuiShaders", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all gui elements on surfaces, 2 = skip drawing but still handle events, 3 = draw but skip events", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_skipParticles( "r_skipParticles", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all particle systems", 0, 1, idCmdSystem::ArgCompletion_Integer<0, 1> );
idCVar r_skipShadows( "r_skipShadows", "0", CVAR_RENDERER | CVAR_BOOL  | CVAR_ARCHIVE, "disable shadows" );

idCVar r_useLightPortalCulling( "r_useLightPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useLightAreaCulling( "r_useLightAreaCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = off, 1 = on" );
idCVar r_useLightScissors( "r_useLightScissors", "3", CVAR_RENDERER | CVAR_INTEGER, "0 = no scissor, 1 = non-clipped scissor, 2 = near-clipped scissor, 3 = fully-clipped scissor", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_useEntityPortalCulling( "r_useEntityPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_logFile( "r_logFile", "0", CVAR_RENDERER | CVAR_INTEGER, "number of frames to emit GL logs" );
idCVar r_clear( "r_clear", "2", CVAR_RENDERER, "force screen clear every frame, 1 = purple, 2 = black, 'r g b' = custom" );

idCVar r_offsetFactor( "r_offsetfactor", "0", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
idCVar r_offsetUnits( "r_offsetunits", "-600", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );

idCVar r_shadowPolygonOffset( "r_shadowPolygonOffset", "-1", CVAR_RENDERER | CVAR_FLOAT, "bias value added to depth test for stencil shadow drawing" );
idCVar r_shadowPolygonFactor( "r_shadowPolygonFactor", "0", CVAR_RENDERER | CVAR_FLOAT, "scale value for stencil shadow drawing" );
idCVar r_subviewOnly( "r_subviewOnly", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't render main view, allowing subviews to be debugged" );
idCVar r_testGamma( "r_testGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels", 0, 195 );
idCVar r_testGammaBias( "r_testGammaBias", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
idCVar r_lightScale( "r_lightScale", "3", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "all light intensities are multiplied by this" );
idCVar r_flareSize( "r_flareSize", "1", CVAR_RENDERER | CVAR_FLOAT, "scale the flare deforms from the material def" );

idCVar r_skipPrelightShadows( "r_skipPrelightShadows", "0", CVAR_RENDERER | CVAR_BOOL, "skip the dmap generated static shadow volumes" );
idCVar r_useScissor( "r_useScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor clip as portals and lights are processed" );
idCVar r_useLightDepthBounds( "r_useLightDepthBounds", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test on lights to reduce both shadow and interaction fill" );
idCVar r_useShadowDepthBounds( "r_useShadowDepthBounds", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test on individual shadow volumes to reduce shadow fill" );
// RB begin
idCVar r_useHalfLambertLighting( "r_useHalfLambertLighting", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "use Half-Lambert lighting instead of classic Lambert, requires reloadShaders" );
// RB end

idCVar r_screenFraction( "r_screenFraction", "100", CVAR_RENDERER | CVAR_INTEGER, "for testing fill rate, the resolution of the entire screen can be changed" );
idCVar r_usePortals( "r_usePortals", "1", CVAR_RENDERER | CVAR_BOOL, " 1 = use portals to perform area culling, otherwise draw everything" );
idCVar r_singleLight( "r_singleLight", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one light" );
idCVar r_singleEntity( "r_singleEntity", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one entity" );
idCVar r_singleSurface( "r_singleSurface", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one surface on each entity" );
idCVar r_singleArea( "r_singleArea", "0", CVAR_RENDERER | CVAR_BOOL, "only draw the portal area the view is actually in" );
idCVar r_orderIndexes( "r_orderIndexes", "1", CVAR_RENDERER | CVAR_BOOL, "perform index reorganization to optimize vertex use" );
idCVar r_lightAllBackFaces( "r_lightAllBackFaces", "0", CVAR_RENDERER | CVAR_BOOL, "light all the back faces, even when they would be shadowed" );

// visual debugging info
idCVar r_showPortals( "r_showPortals", "0", CVAR_RENDERER | CVAR_BOOL, "draw portal outlines in color based on passed / not passed" );
idCVar r_showUnsmoothedTangents( "r_showUnsmoothedTangents", "0", CVAR_RENDERER | CVAR_BOOL, "if 1, put all nvidia register combiner programming in display lists" );
idCVar r_showSilhouette( "r_showSilhouette", "0", CVAR_RENDERER | CVAR_BOOL, "highlight edges that are casting shadow planes" );
idCVar r_showVertexColor( "r_showVertexColor", "0", CVAR_RENDERER | CVAR_BOOL, "draws all triangles with the solid vertex color" );
idCVar r_showUpdates( "r_showUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "report entity and light updates and ref counts" );
idCVar r_showDemo( "r_showDemo", "0", CVAR_RENDERER | CVAR_BOOL, "report reads and writes to the demo file" );
idCVar r_showDynamic( "r_showDynamic", "0", CVAR_RENDERER | CVAR_BOOL, "report stats on dynamic surface generation" );
idCVar r_showTrace( "r_showTrace", "0", CVAR_RENDERER | CVAR_INTEGER, "show the intersection of an eye trace with the world", idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_showIntensity( "r_showIntensity", "0", CVAR_RENDERER | CVAR_BOOL, "draw the screen colors based on intensity, red = 0, green = 128, blue = 255" );
idCVar r_showLights( "r_showLights", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = just print volumes numbers, highlighting ones covering the view, 2 = also draw planes of each volume, 3 = also draw edges of each volume", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showShadows( "r_showShadows", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = visualize the stencil shadow volumes, 2 = draw filled in", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showLightScissors( "r_showLightScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show light scissor rectangles" );
idCVar r_showLightCount( "r_showLightCount", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = colors surfaces based on light count, 2 = also count everything through walls, 3 = also print overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showViewEntitys( "r_showViewEntitys", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = displays the bounding boxes of all view models, 2 = print index numbers" );
idCVar r_showTris( "r_showTris", "0", CVAR_RENDERER | CVAR_INTEGER, "enables wireframe rendering of the world, 1 = only draw visible ones, 2 = draw all front facing, 3 = draw all, 4 = draw with alpha", 0, 4, idCmdSystem::ArgCompletion_Integer<0, 4> );
idCVar r_showSurfaceInfo( "r_showSurfaceInfo", "0", CVAR_RENDERER | CVAR_BOOL, "show surface material name under crosshair" );
idCVar r_showNormals( "r_showNormals", "0", CVAR_RENDERER | CVAR_FLOAT, "draws wireframe normals" );
idCVar r_showMemory( "r_showMemory", "0", CVAR_RENDERER | CVAR_BOOL, "print frame memory utilization" );
idCVar r_showCull( "r_showCull", "0", CVAR_RENDERER | CVAR_BOOL, "report sphere and box culling stats" );
idCVar r_showAddModel( "r_showAddModel", "0", CVAR_RENDERER | CVAR_BOOL, "report stats from tr_addModel" );
idCVar r_showDepth( "r_showDepth", "0", CVAR_RENDERER | CVAR_BOOL, "display the contents of the depth buffer and the depth range" );
idCVar r_showSurfaces( "r_showSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "report surface/light/shadow counts" );
idCVar r_showPrimitives( "r_showPrimitives", "0", CVAR_RENDERER | CVAR_INTEGER, "report drawsurf/index/vertex counts" );
idCVar r_showEdges( "r_showEdges", "0", CVAR_RENDERER | CVAR_BOOL, "draw the sil edges" );
idCVar r_showTexturePolarity( "r_showTexturePolarity", "0", CVAR_RENDERER | CVAR_BOOL, "shade triangles by texture area polarity" );
idCVar r_showTangentSpace( "r_showTangentSpace", "0", CVAR_RENDERER | CVAR_INTEGER, "shade triangles by tangent space, 1 = use 1st tangent vector, 2 = use 2nd tangent vector, 3 = use normal vector", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showDominantTri( "r_showDominantTri", "0", CVAR_RENDERER | CVAR_BOOL, "draw lines from vertexes to center of dominant triangles" );
idCVar r_showTextureVectors( "r_showTextureVectors", "0", CVAR_RENDERER | CVAR_FLOAT, " if > 0 draw each triangles texture (tangent) vectors" );
idCVar r_showOverDraw( "r_showOverDraw", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = geometry overdraw, 2 = light interaction overdraw, 3 = geometry and light interaction overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
// RB begin
idCVar r_showShadowMaps( "r_showShadowMaps", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_showShadowMapLODs( "r_showShadowMapLODs", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
// RB end

idCVar r_useEntityCallbacks( "r_useEntityCallbacks", "1", CVAR_RENDERER | CVAR_BOOL, "if 0, issue the callback immediately at update time, rather than defering" );

idCVar r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw the skeleton when model animates, 1 = draw model with skeleton, 2 = draw skeleton only", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_jointNameScale( "r_jointNameScale", "0.02", CVAR_RENDERER | CVAR_FLOAT, "size of joint names when r_showskel is set to 1" );
idCVar r_jointNameOffset( "r_jointNameOffset", "0.5", CVAR_RENDERER | CVAR_FLOAT, "offset of joint names when r_showskel is set to 1" );

idCVar r_debugLineDepthTest( "r_debugLineDepthTest", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "perform depth test on debug lines" );
idCVar r_debugLineWidth( "r_debugLineWidth", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "width of debug lines" );
idCVar r_debugArrowStep( "r_debugArrowStep", "120", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "step size of arrow cone line rotation in degrees", 0, 120 );
idCVar r_debugPolygonFilled( "r_debugPolygonFilled", "1", CVAR_RENDERER | CVAR_BOOL, "draw a filled polygon" );

idCVar r_materialOverride( "r_materialOverride", "", CVAR_RENDERER, "overrides all materials", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );

idCVar r_debugRenderToTexture( "r_debugRenderToTexture", "0", CVAR_RENDERER | CVAR_INTEGER, "" );

idCVar stereoRender_enable( "stereoRender_enable", "0", CVAR_INTEGER | CVAR_ARCHIVE, "1 = side-by-side compressed, 2 = top and bottom compressed, 3 = side-by-side, 4 = 720 frame packed, 5 = interlaced, 6 = OpenGL quad buffer" );
idCVar stereoRender_swapEyes( "stereoRender_swapEyes", "0", CVAR_BOOL | CVAR_ARCHIVE, "reverse eye adjustments" );
idCVar stereoRender_deGhost( "stereoRender_deGhost", "0.05", CVAR_FLOAT | CVAR_ARCHIVE, "subtract from opposite eye to reduce ghosting" );

idCVar r_useVirtualScreenResolution( "r_useVirtualScreenResolution", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "do 2D rendering at 640x480 and stretch to the current resolution" );

// RB: shadow mapping parameters
idCVar r_useShadowMapping( "r_useShadowMapping", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "use shadow mapping instead of stencil shadows" );
idCVar r_shadowMapFrustumFOV( "r_shadowMapFrustumFOV", "92", CVAR_RENDERER | CVAR_FLOAT, "oversize FOV for point light side matching" );
idCVar r_shadowMapSingleSide( "r_shadowMapSingleSide", "-1", CVAR_RENDERER | CVAR_INTEGER, "only draw a single side (0-5) of point lights" );
idCVar r_shadowMapImageSize( "r_shadowMapImageSize", "1024", CVAR_RENDERER | CVAR_INTEGER, "", 128, 2048 );
idCVar r_shadowMapJitterScale( "r_shadowMapJitterScale", "3", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter offset" );
idCVar r_shadowMapBiasScale( "r_shadowMapBiasScale", "0.0001", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter bias" );
idCVar r_shadowMapRandomizeJitter( "r_shadowMapRandomizeJitter", "1", CVAR_RENDERER | CVAR_BOOL, "randomly offset jitter texture each draw" );
idCVar r_shadowMapSamples( "r_shadowMapSamples", "1", CVAR_RENDERER | CVAR_INTEGER, "0, 1, 4, or 16" );
idCVar r_shadowMapSplits( "r_shadowMapSplits", "3", CVAR_RENDERER | CVAR_INTEGER, "number of splits for cascaded shadow mapping with parallel lights", 0, 4 );
idCVar r_shadowMapSplitWeight( "r_shadowMapSplitWeight", "0.9", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_shadowMapLodScale( "r_shadowMapLodScale", "1.4", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_shadowMapLodBias( "r_shadowMapLodBias", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_shadowMapPolygonFactor( "r_shadowMapPolygonFactor", "2", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset factor for drawing shadow buffer" );
idCVar r_shadowMapPolygonOffset( "r_shadowMapPolygonOffset", "3000", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset units for drawing shadow buffer" );
idCVar r_shadowMapOccluderFacing( "r_shadowMapOccluderFacing", "2", CVAR_RENDERER | CVAR_INTEGER, "0 = front faces, 1 = back faces, 2 = twosided" );
// RB end

const char* fileExten[3] = { "tga", "png", "jpg" };
const char* envDirection[6] = { "_nx", "_py", "_ny", "_pz", "_nz", "_px" };
const char* skyDirection[6] = { "_forward", "_back", "_left", "_right", "_up", "_down" };
/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2014 Robert Beckebans
Copyright (C) 2014 Carl Kenner

This file is part of the Doom 3 BFG Edition GPL Source Code ( "Doom 3 BFG Edition Source Code" ).

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "precompiled.h"

#include "tr_local.h"
#include "Framebuffer.h"

#include "vr/Vr.h" // Koz
#include "d3xp/Game_local.h" //Koz

idCVar r_drawEyeColor( "r_drawEyeColor", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a colored box, red = left eye, blue = right eye, grey = non-stereo" );
idCVar r_motionBlur( "r_motionBlur", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 - 5, log2 of the number of motion blur samples" );
idCVar r_forceZPassStencilShadows( "r_forceZPassStencilShadows", "0", CVAR_RENDERER | CVAR_BOOL, "force Z-pass rendering for performance testing" );
idCVar r_useStencilShadowPreload( "r_useStencilShadowPreload", "1", CVAR_RENDERER | CVAR_ARCHIVE| CVAR_BOOL, "use stencil shadow preload algorithm instead of Z-fail" );
idCVar r_skipShaderPasses( "r_skip`R_USE`ShaderPasses", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_skipInteractionFastPath( "r_skipInteractionFastPath", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useLightStencilSelect( "r_useLightStencilSelect", "0", CVAR_RENDERER | CVAR_BOOL, "use stencil select pass" );

//idCVar vr_gui2( "vr_gui2", "-.03", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "vr in game gui separation" );

extern idCVar stereoRender_swapEyes;

backEndState_t	backEnd;

/*
================
SetVertexParm
================
*/
static ARC_INLINE void SetVertexParm( renderParm_t rp, const float* value )
{
	renderProgManager.SetUniformValue( rp, value );
}

/*
================
SetVertexParms
================
*/
static ARC_INLINE void SetVertexParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		renderProgManager.SetUniformValue( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

/*
================
SetFragmentParm
================
*/
static ARC_INLINE void SetFragmentParm( renderParm_t rp, const float* value )
{
	renderProgManager.SetUniformValue( rp, value );
}

/*
================
RB_SetMVP
================
*/
void RB_SetMVP( const idRenderMatrix& mvp )
{
	SetVertexParms( RENDERPARM_MVPMATRIX_X, mvp[0], 4 );
}

/*
================
RB_SetMVPWithStereoOffset
================
*/
static void RB_SetMVPWithStereoOffset( const idRenderMatrix& mvp, const float stereoOffset )
{
	idRenderMatrix offset = mvp;
	offset[0][3] += stereoOffset;
	
	SetVertexParms( RENDERPARM_MVPMATRIX_X, offset[0], 4 );
}

static const float zero[4] = { 0, 0, 0, 0 };
static const float one[4] = { 1, 1, 1, 1 };
static const float negOne[4] = { -1, -1, -1, -1 };

/*
================
RB_SetVertexColorParms
================
*/
static void RB_SetVertexColorParms( stageVertexColor_t svc )
{
	switch ( svc )
	{
		case SVC_IGNORE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, zero );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
		case SVC_MODULATE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, one );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, zero );
			break;
		case SVC_INVERSE_MODULATE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, negOne );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
	}
}

/*
================
RB_DrawElementsWithCounters
================
*/
void RB_DrawElementsWithCounters( const drawSurf_t* surf )
{
	// get vertex buffer
	const vertCacheHandle_t vbHandle = surf->ambientCache;
	idVertexBuffer* vertexBuffer;
	if( vertexCache.CacheIsStatic( vbHandle ) )
	{
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == nullptr" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
	}
	const int vertOffset = ( int )( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	
	// get index buffer
	const vertCacheHandle_t ibHandle = surf->indexCache;
	idIndexBuffer* indexBuffer;
	if( vertexCache.CacheIsStatic( ibHandle ) )
	{
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else
	{
		const uint64 frameNum = ( int )( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == nullptr" );
			return;
		}
		indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
	}
	// RB: 64 bit fixes, changed int to GLintptr
	const GLintptr indexOffset = ( GLintptr )( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	// RB end
	
	RENDERLOG_PRINTF( "Binding Buffers: %p:%i %p:%i\n", vertexBuffer, vertOffset, indexBuffer, indexOffset );
	
	if( surf->jointCache )
	{
		// DG: this happens all the time in the erebus1 map with blendlight.vfp,
		// so don't call assert (through verify) here until it's fixed (if fixable)
		// else the game crashes on linux when using debug builds
		
		// FIXME: fix this properly if possible?
		// RB: yes but it would require an additional blend light skinned shader
		//if( !verify( renderProgManager.ShaderUsesJoints() ) )
		if( !renderProgManager.ShaderUsesJoints() )
			// DG end
		{
			return;
		}
	}
	else
	{
		if( !verify( !renderProgManager.ShaderUsesJoints() || renderProgManager.ShaderHasOptionalSkinning() ) )
		{
			return;
		}
	}
	
	
	if( surf->jointCache )
	{
		idJointBuffer jointBuffer;
		if( !vertexCache.GetJointBuffer( surf->jointCache, &jointBuffer ) )
		{
			idLib::Warning( "RB_DrawElementsWithCounters, jointBuffer == nullptr" );
			return;
		}
		assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );
		
		// RB: 64 bit fixes, changed GLuint to GLintptr
		const GLintptr ubo = reinterpret_cast< GLintptr >( jointBuffer.GetAPIObject() );
		// RB end
		
		glBindBufferRange( GL_UNIFORM_BUFFER, 0, ubo, jointBuffer.GetOffset(), jointBuffer.GetNumJoints() * sizeof( idJointMat ) );
	}
	
	renderProgManager.CommitUniforms();
	
	// RB: 64 bit fixes, changed GLuint to GLintptr
	if( backEnd.glState.currentIndexBuffer != ( GLintptr )indexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() )
	{
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ( GLintptr )indexBuffer->GetAPIObject() );
		backEnd.glState.currentIndexBuffer = ( GLintptr )indexBuffer->GetAPIObject();
	}
	
	if( ( backEnd.glState.vertexLayout != LAYOUT_DRAW_VERT ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vertexBuffer->GetAPIObject() ) || !r_useStateCaching.GetBool() )
	{
		glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer->GetAPIObject() );
		backEnd.glState.currentVertexBuffer = ( GLintptr )vertexBuffer->GetAPIObject();
		
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
		
#if defined(USE_GLES2) || defined(USE_GLES3)
		glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_XYZ_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_NORMAL_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_COLOR_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_COLOR2_OFFSET ) );
#if defined(USE_ANGLE)
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT_OES, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_ST_OFFSET ) );
#else
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_ST_OFFSET ) );
#endif
		glVertexAttribPointer( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( vertOffset + DRAWVERT_TANGENT_OFFSET ) );
		
#else
		glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), ( void* )( DRAWVERT_XYZ_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_NORMAL_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_COLOR_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_COLOR2_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_ST_OFFSET ) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), ( void* )( DRAWVERT_TANGENT_OFFSET ) );
#endif // #if defined(USE_GLES2) || defined(USE_GLES3)
		
		backEnd.glState.vertexLayout = LAYOUT_DRAW_VERT;
	}
	// RB end
	
#if defined(USE_GLES3) //defined(USE_GLES2)
	glDrawElements(	GL_TRIANGLES,
					r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
					GL_INDEX_TYPE,
					( triIndex_t* )indexOffset );
#else
	glDrawElementsBaseVertex( GL_TRIANGLES,
							  r_singleTriangle.GetBool() ? 3 : surf->numIndexes,
							  GL_INDEX_TYPE,
							  ( triIndex_t* )indexOffset,
							  vertOffset / sizeof( idDrawVert ) );
#endif
					
	// RB: added stats
	backEnd.pc.c_drawElements++;
	backEnd.pc.c_drawIndexes += surf->numIndexes;
	// RB end
}

/*
================
Koz draw triangle strips.
RB_DrawStripWithCounters
================
*/
void RB_DrawStripWithCounters( const drawSurf_t *surf ) {
	// get vertex buffer
	const vertCacheHandle_t vbHandle = surf->ambientCache;
	idVertexBuffer * vertexBuffer;
	if ( vertexCache.CacheIsStatic( vbHandle ) ) {
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	}
	else {
		const uint64 frameNum = (int)(vbHandle >> VERTCACHE_FRAME_SHIFT) & VERTCACHE_FRAME_MASK;
		if ( frameNum != ((vertexCache.currentFrame - 1) & VERTCACHE_FRAME_MASK) ) {
			idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == nullptr" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
	}
	const int vertOffset = (int)(vbHandle >> VERTCACHE_OFFSET_SHIFT) & VERTCACHE_OFFSET_MASK;

	// get index buffer
	const vertCacheHandle_t ibHandle = surf->indexCache;
	idIndexBuffer * indexBuffer;
	if ( vertexCache.CacheIsStatic( ibHandle ) ) {
		indexBuffer = &vertexCache.staticData.indexBuffer;
	}
	else {
		const uint64 frameNum = (int)(ibHandle >> VERTCACHE_FRAME_SHIFT) & VERTCACHE_FRAME_MASK;
		if ( frameNum != ((vertexCache.currentFrame - 1) & VERTCACHE_FRAME_MASK) ) {
			idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == nullptr" );
			return;
		}
		indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
	}
	const size_t indexOffset = ( size_t)(ibHandle >> VERTCACHE_OFFSET_SHIFT) & VERTCACHE_OFFSET_MASK;

	RENDERLOG_PRINTF( "Binding Buffers: %p:%i %p:%i\n", vertexBuffer, vertOffset, indexBuffer, indexOffset );

	if ( surf->jointCache ) {
		if ( !verify( renderProgManager.ShaderUsesJoints() ) ) {
			return;
		}
	}
	else {
		if ( !verify( !renderProgManager.ShaderUsesJoints() || renderProgManager.ShaderHasOptionalSkinning() ) ) {
			return;
		}
	}


	if ( surf->jointCache ) {
		idJointBuffer jointBuffer;
		if ( !vertexCache.GetJointBuffer( surf->jointCache, &jointBuffer ) ) {
			idLib::Warning( "RB_DrawElementsWithCounters, jointBuffer == nullptr" );
			return;
		}
		assert( (jointBuffer.GetOffset() & (glConfig.uniformBufferOffsetAlignment - 1)) == 0 );

		const GLintptr ubo = reinterpret_cast< GLintptr >(jointBuffer.GetAPIObject());
		glBindBufferRange( GL_UNIFORM_BUFFER, 0, ubo, jointBuffer.GetOffset(), jointBuffer.GetNumJoints() * sizeof( idJointMat ) );
	}

	renderProgManager.CommitUniforms();

	if ( backEnd.glState.currentIndexBuffer != (GLintptr)indexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() ) {
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER_ARB, (GLintptr)indexBuffer->GetAPIObject() );
		backEnd.glState.currentIndexBuffer = (GLintptr)indexBuffer->GetAPIObject();
	}

	if ( (backEnd.glState.vertexLayout != LAYOUT_DRAW_VERT) || (backEnd.glState.currentVertexBuffer != (GLintptr)vertexBuffer->GetAPIObject()) || !r_useStateCaching.GetBool() ) {
		glBindBuffer( GL_ARRAY_BUFFER_ARB, (GLintptr)vertexBuffer->GetAPIObject() );
		backEnd.glState.currentVertexBuffer = (GLintptr)vertexBuffer->GetAPIObject();

		glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_ST );
		glEnableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );

		glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof( idDrawVert ), (void *)(DRAWVERT_XYZ_OFFSET) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_NORMAL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), (void *)(DRAWVERT_NORMAL_OFFSET) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), (void *)(DRAWVERT_COLOR_OFFSET) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), (void *)(DRAWVERT_COLOR2_OFFSET) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_ST, 2, GL_HALF_FLOAT, GL_TRUE, sizeof( idDrawVert ), (void *)(DRAWVERT_ST_OFFSET) );
		glVertexAttribPointer( PC_ATTRIB_INDEX_TANGENT, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idDrawVert ), (void *)(DRAWVERT_TANGENT_OFFSET) );

		backEnd.glState.vertexLayout = LAYOUT_DRAW_VERT;
	}

	glDrawElementsBaseVertex( GL_TRIANGLES,
		surf->numIndexes,
		GL_INDEX_TYPE,
		(triIndex_t *)indexOffset,
		vertOffset / sizeof( idDrawVert ) );

}


/*
======================
RB_GetShaderTextureMatrix
======================
*/
static void RB_GetShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture, float matrix[16] )
{
	matrix[0 * 4 + 0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[1 * 4 + 0] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[2 * 4 + 0] = 0.0f;
	matrix[3 * 4 + 0] = shaderRegisters[ texture->matrix[0][2] ];
	
	matrix[0 * 4 + 1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[1 * 4 + 1] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[2 * 4 + 1] = 0.0f;
	matrix[3 * 4 + 1] = shaderRegisters[ texture->matrix[1][2] ];
	
	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if( matrix[3 * 4 + 0] < -40.0f || matrix[12] > 40.0f )
	{
		matrix[3 * 4 + 0] -= ( int )matrix[3 * 4 + 0];
	}
	if( matrix[13] < -40.0f || matrix[13] > 40.0f )
	{
		matrix[13] -= ( int )matrix[13];
	}
	
	matrix[0 * 4 + 2] = 0.0f;
	matrix[1 * 4 + 2] = 0.0f;
	matrix[2 * 4 + 2] = 1.0f;
	matrix[3 * 4 + 2] = 0.0f;
	
	matrix[0 * 4 + 3] = 0.0f;
	matrix[1 * 4 + 3] = 0.0f;
	matrix[2 * 4 + 3] = 0.0f;
	matrix[3 * 4 + 3] = 1.0f;
}

/*
======================
RB_LoadShaderTextureMatrix
======================
*/
static void RB_LoadShaderTextureMatrix( const float* shaderRegisters, const textureStage_t* texture )
{
	float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	
	if( texture->hasMatrix )
	{
		float matrix[16];
		RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );
		texS[0] = matrix[0 * 4 + 0];
		texS[1] = matrix[1 * 4 + 0];
		texS[2] = matrix[2 * 4 + 0];
		texS[3] = matrix[3 * 4 + 0];
		
		texT[0] = matrix[0 * 4 + 1];
		texT[1] = matrix[1 * 4 + 1];
		texT[2] = matrix[2 * 4 + 1];
		texT[3] = matrix[3 * 4 + 1];
		
		RENDERLOG_PRINTF( "Setting Texture Matrix\n" );
		renderLog.Indent();
		RENDERLOG_PRINTF( "Texture Matrix S : %4.3f, %4.3f, %4.3f, %4.3f\n", texS[0], texS[1], texS[2], texS[3] );
		RENDERLOG_PRINTF( "Texture Matrix T : %4.3f, %4.3f, %4.3f, %4.3f\n", texT[0], texT[1], texT[2], texT[3] );
		renderLog.Outdent();
	}
	
	SetVertexParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	SetVertexParm( RENDERPARM_TEXTUREMATRIX_T, texT );
}

/*
=====================
RB_BakeTextureMatrixIntoTexgen
=====================
*/
static void RB_BakeTextureMatrixIntoTexgen( idPlane lightProject[3], const float* textureMatrix )
{
	float genMatrix[16];
	float final[16];
	
	genMatrix[0 * 4 + 0] = lightProject[0][0];
	genMatrix[1 * 4 + 0] = lightProject[0][1];
	genMatrix[2 * 4 + 0] = lightProject[0][2];
	genMatrix[3 * 4 + 0] = lightProject[0][3];
	
	genMatrix[0 * 4 + 1] = lightProject[1][0];
	genMatrix[1 * 4 + 1] = lightProject[1][1];
	genMatrix[2 * 4 + 1] = lightProject[1][2];
	genMatrix[3 * 4 + 1] = lightProject[1][3];
	
	genMatrix[0 * 4 + 2] = 0.0f;
	genMatrix[1 * 4 + 2] = 0.0f;
	genMatrix[2 * 4 + 2] = 0.0f;
	genMatrix[3 * 4 + 2] = 0.0f;
	
	genMatrix[0 * 4 + 3] = lightProject[2][0];
	genMatrix[1 * 4 + 3] = lightProject[2][1];
	genMatrix[2 * 4 + 3] = lightProject[2][2];
	genMatrix[3 * 4 + 3] = lightProject[2][3];
	
	R_MatrixMultiply( genMatrix, textureMatrix, final );
	
	lightProject[0][0] = final[0 * 4 + 0];
	lightProject[0][1] = final[1 * 4 + 0];
	lightProject[0][2] = final[2 * 4 + 0];
	lightProject[0][3] = final[3 * 4 + 0];
	
	lightProject[1][0] = final[0 * 4 + 1];
	lightProject[1][1] = final[1 * 4 + 1];
	lightProject[1][2] = final[2 * 4 + 1];
	lightProject[1][3] = final[3 * 4 + 1];
}

/*
======================
RB_BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
static void RB_BindVariableStageImage( const textureStage_t* texture, const float* shaderRegisters )
{
	if( texture->cinematic )
	{
		cinData_t cin;
		
		if( r_skipDynamicTextures.GetBool() )
		{
			globalImages->defaultImage->Bind();
			return;
		}
		
		// offset time by shaderParm[7] (FIXME: make the time offset a parameter of the shader?)
		// We make no attempt to optimize for multiple identical cinematics being in view, or
		// for cinematics going at a lower framerate than the renderer.
		cin = texture->cinematic->ImageForTime( backEnd.viewDef->renderView.time[0] + anMath::Ftoi( 1000.0f * backEnd.viewDef->renderView.shaderParms[11] ) );
		if( cin.imageY != nullptr )
		{
			GL_SelectTexture( 0 );
			cin.imageY->Bind();
			GL_SelectTexture( 1 );
			cin.imageCr->Bind();
			GL_SelectTexture( 2 );
			cin.imageCb->Bind();
			
		}
		else if( cin.image != nullptr )
		{
			//Carl: A single RGB image works better with the FFMPEG BINK codec.
			GL_SelectTexture( 0 );
			cin.image->Bind();
			renderProgManager.BindShader_TextureVertexColor();
		}
		else
		{
			globalImages->blackImage->Bind();
			// because the shaders may have already been set - we need to make sure we are not using a bink shader which would
			// display incorrectly.  We may want to get rid of RB_BindVariableStageImage and inline the code so that the
			// SWF GUI case is handled better, too
			renderProgManager.BindShader_TextureVertexColor();
		}
	}
	else
	{
		// FIXME: see why image is invalid
		if( texture->image != nullptr )
		{
			texture->image->Bind();
		}
	}
}

/*
================
RB_PrepareStageTexturing
================
*/
static void RB_PrepareStageTexturing( const shaderStage_t* pStage,  const drawSurf_t* surf )
{
	float useTexGenParm[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	
	// set the texture matrix if needed
	RB_LoadShaderTextureMatrix( surf->shaderRegisters, &pStage->texture );
	
	// texgens
	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{
	
		// see if there is also a bump map specified
		const shaderStage_t* bumpStage = surf->material->GetBumpStage();
		if( bumpStage != nullptr )
		{
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture( 1 );
			bumpStage->texture.image->Bind();
			GL_SelectTexture( 0 );
			
			RENDERLOG_PRINTF( "TexGen: TG_REFLECT_CUBE: Bumpy Environment\n" );
			if( surf->jointCache )
			{
				renderProgManager.BindShader_BumpyEnvironmentSkinned();
			}
			else
			{
				renderProgManager.BindShader_BumpyEnvironment();
			}
		}
		else
		{
			RENDERLOG_PRINTF( "TexGen: TG_REFLECT_CUBE: Environment\n" );
			if( surf->jointCache )
			{
				renderProgManager.BindShader_EnvironmentSkinned();
			}
			else
			{
				renderProgManager.BindShader_Environment();
			}
		}
		
	}
	else if( pStage->texture.texgen == TG_SKYBOX_CUBE )
	{
	
		renderProgManager.BindShader_SkyBox();
		
	}
	else if( pStage->texture.texgen == TG_WOBBLESKY_CUBE )
	{
	
		const int* parms = surf->material->GetTexGenRegisters();
		
		float wobbleDegrees = surf->shaderRegisters[ parms[0] ] * ( anMath::PI / 180.0f );
		float wobbleSpeed = surf->shaderRegisters[ parms[1] ] * ( 2.0f * anMath::PI / 60.0f );
		float rotateSpeed = surf->shaderRegisters[ parms[2] ] * ( 2.0f * anMath::PI / 60.0f );
		
		anVec3 axis[3];
		{
			// very ad-hoc "wobble" transform
			float s, c;
			anMath::SinCos( wobbleSpeed * backEnd.viewDef->renderView.time[0] * 0.001f, s, c );
			
			float ws, wc;
			anMath::SinCos( wobbleDegrees, ws, wc );
			
			axis[2][0] = ws * c;
			axis[2][1] = ws * s;
			axis[2][2] = wc;
			
			axis[1][0] = -s * s * ws;
			axis[1][2] = -s * ws * ws;
			axis[1][1] = anMath::Sqrt( anMath::Fabs( 1.0f - ( axis[1][0] * axis[1][0] + axis[1][2] * axis[1][2] ) ) );
			
			// make the second vector exactly perpendicular to the first
			axis[1] -= ( axis[2] * axis[1] ) * axis[2];
			axis[1].Normalize();
			
			// construct the third with a cross
			axis[0].Cross( axis[1], axis[2] );
		}
		
		// add the rotate
		float rs, rc;
		anMath::SinCos( rotateSpeed * backEnd.viewDef->renderView.time[0] * 0.001f, rs, rc );
		
		float transform[12];
		transform[0 * 4 + 0] = axis[0][0] * rc + axis[1][0] * rs;
		transform[0 * 4 + 1] = axis[0][1] * rc + axis[1][1] * rs;
		transform[0 * 4 + 2] = axis[0][2] * rc + axis[1][2] * rs;
		transform[0 * 4 + 3] = 0.0f;
		
		transform[1 * 4 + 0] = axis[1][0] * rc - axis[0][0] * rs;
		transform[1 * 4 + 1] = axis[1][1] * rc - axis[0][1] * rs;
		transform[1 * 4 + 2] = axis[1][2] * rc - axis[0][2] * rs;
		transform[1 * 4 + 3] = 0.0f;
		
		transform[2 * 4 + 0] = axis[2][0];
		transform[2 * 4 + 1] = axis[2][1];
		transform[2 * 4 + 2] = axis[2][2];
		transform[2 * 4 + 3] = 0.0f;
		
		SetVertexParms( RENDERPARM_WOBBLESKY_X, transform, 3 );
		renderProgManager.BindShader_WobbleSky();
		
	}
	else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
	{
	
		useTexGenParm[0] = 1.0f;
		useTexGenParm[1] = 1.0f;
		useTexGenParm[2] = 1.0f;
		useTexGenParm[3] = 1.0f;
		
		float mat[16];
		R_MatrixMultiply( surf->space->modelViewMatrix, backEnd.viewDef->projectionMatrix, mat );
		
		RENDERLOG_PRINTF( "TexGen : %s\n", ( pStage->texture.texgen == TG_SCREEN ) ? "TG_SCREEN" : "TG_SCREEN2" );
		renderLog.Indent();
		
		float plane[4];
		plane[0] = mat[0 * 4 + 0];
		plane[1] = mat[1 * 4 + 0];
		plane[2] = mat[2 * 4 + 0];
		plane[3] = mat[3 * 4 + 0];
		SetVertexParm( RENDERPARM_TEXGEN_0_S, plane );
		RENDERLOG_PRINTF( "TEXGEN_S = %4.3f, %4.3f, %4.3f, %4.3f\n",  plane[0], plane[1], plane[2], plane[3] );
		
		plane[0] = mat[0 * 4 + 1];
		plane[1] = mat[1 * 4 + 1];
		plane[2] = mat[2 * 4 + 1];
		plane[3] = mat[3 * 4 + 1];
		SetVertexParm( RENDERPARM_TEXGEN_0_T, plane );
		RENDERLOG_PRINTF( "TEXGEN_T = %4.3f, %4.3f, %4.3f, %4.3f\n",  plane[0], plane[1], plane[2], plane[3] );
		
		plane[0] = mat[0 * 4 + 3];
		plane[1] = mat[1 * 4 + 3];
		plane[2] = mat[2 * 4 + 3];
		plane[3] = mat[3 * 4 + 3];
		SetVertexParm( RENDERPARM_TEXGEN_0_Q, plane );
		RENDERLOG_PRINTF( "TEXGEN_Q = %4.3f, %4.3f, %4.3f, %4.3f\n",  plane[0], plane[1], plane[2], plane[3] );
		
		renderLog.Outdent();
		
	}
	else if( pStage->texture.texgen == TG_DIFFUSE_CUBE )
	{
	
		// As far as I can tell, this is never used
		idLib::Warning( "Using Diffuse Cube! Please contact Brian!" );
		
	}
	else if( pStage->texture.texgen == TG_GLASSWARP )
	{
	
		// As far as I can tell, this is never used
		idLib::Warning( "Using GlassWarp! Please contact Brian!" );
	}
	
	SetVertexParm( RENDERPARM_TEXGEN_0_ENABLED, useTexGenParm );
}

/*
================
RB_FinishStageTexturing
================
*/
static void RB_FinishStageTexturing( const shaderStage_t* pStage, const drawSurf_t* surf )
{

	if( pStage->texture.cinematic )
	{
		// unbind the extra bink textures
		GL_SelectTexture( 1 );
		globalImages->BindNull();
		GL_SelectTexture( 2 );
		globalImages->BindNull();
		GL_SelectTexture( 0 );
	}
	
	if( pStage->texture.texgen == TG_REFLECT_CUBE )
	{
		// see if there is also a bump map specified
		const shaderStage_t* bumpStage = surf->material->GetBumpStage();
		if( bumpStage != nullptr )
		{
			// per-pixel reflection mapping with bump mapping
			GL_SelectTexture( 1 );
			globalImages->BindNull();
			GL_SelectTexture( 0 );
		}
		else
		{
			// per-pixel reflection mapping without bump mapping
		}
		renderProgManager.Unbind();
	}
}

// RB: moved this up because we need to call this several times for shadow mapping
static void RB_ResetViewportAndScissorToDefaultCamera( const viewDef_t* viewDef )
{
	// set the window clipping
	GL_Viewport( viewDef->viewport.x1,
				 viewDef->viewport.y1,
				 viewDef->viewport.x2 + 1 - viewDef->viewport.x1,
				 viewDef->viewport.y2 + 1 - viewDef->viewport.y1 );
				 
	// the scissor may be smaller than the viewport for subviews
	GL_Scissor( backEnd.viewDef->viewport.x1 + viewDef->scissor.x1,
				backEnd.viewDef->viewport.y1 + viewDef->scissor.y1,
				viewDef->scissor.x2 + 1 - viewDef->scissor.x1,
				viewDef->scissor.y2 + 1 - viewDef->scissor.y1 );
	backEnd.currentScissor = viewDef->scissor;
}
// RB end

/*
=========================================================================================

DEPTH BUFFER RENDERING

=========================================================================================
*/

/*
==================
RB_FillDepthBufferGeneric
==================
*/
static void RB_FillDepthBufferGeneric( const drawSurf_t* const* drawSurfs, int numDrawSurfs )
{
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* drawSurf = drawSurfs[i];
		const idMaterial* shader = drawSurf->material;
		
		// translucent surfaces don't put anything in the depth buffer and don't
		// test against it, which makes them fail the mirror clip plane operation
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}
		
		// get the expressions for conditionals / color / texcoords
		const float* regs = drawSurf->shaderRegisters;
		
		// if all stages of a material have been conditioned off, don't do anything
		int stage = 0;
		for( ; stage < shader->GetNumStages(); stage++ )
		{
			const shaderStage_t* pStage = shader->GetStage( stage );
			// check the stage enable condition
			if( regs[ pStage->conditionRegister ] != 0 )
			{
				break;
			}
		}
		if( stage == shader->GetNumStages() )
		{
			continue;
		}
		
		// change the matrix if needed
		if( drawSurf->space != backEnd.currentSpace )
		{
			RB_SetMVP( drawSurf->space->mvp );
			
			backEnd.currentSpace = drawSurf->space;
		}
		
		uint64 surfGLState = 0;
		
		// set polygon offset if necessary
		if( shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
		}
		
		// subviews will just down-modulate the color buffer
		anVec4 color;
		if( shader->GetSort() == SS_SUBVIEW )
		{
			surfGLState |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_LESS;
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
		}
		else
		{
			// others just draw black
			color[0] = 0.0f;
			color[1] = 0.0f;
			color[2] = 0.0f;
			color[3] = 1.0f;
		}
		
		renderLog.OpenBlock( shader->GetName() );
		
		bool drawSolid = false;
		if( shader->Coverage() == MC_OPAQUE )
		{
			drawSolid = true;
		}
		else if( shader->Coverage() == MC_PERFORATED )
		{
			// we may have multiple alpha tested stages
			// if the only alpha tested stages are condition register omitted,
			// draw a normal opaque surface
			bool didDraw = false;
			
			// perforated surfaces may have multiple alpha tested stages
			for( stage = 0; stage < shader->GetNumStages(); stage++ )
			{
				const shaderStage_t* pStage = shader->GetStage( stage );
				
				if( !pStage->hasAlphaTest )
				{
					continue;
				}
				
				// check the stage enable condition
				if( regs[ pStage->conditionRegister ] == 0 )
				{
					continue;
				}
				
				// if we at least tried to draw an alpha tested stage,
				// we won't draw the opaque surface
				didDraw = true;
				
				// set the alpha modulate
				color[3] = regs[ pStage->color.registers[3] ];
				
				// skip the entire stage if alpha would be black
				if( color[3] <= 0.0f )
				{
					continue;
				}
				
				uint64 stageGLState = surfGLState;
				
				// set privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
					stageGLState |= GLS_POLYGON_OFFSET;
				}
				
				GL_Color( color );
				
#ifdef USE_CORE_PROFILE
				GL_State( stageGLState );
				anVec4 alphaTestValue( regs[ pStage->alphaTestRegister ] );
				SetFragmentParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );
#else
				GL_State( stageGLState | GLS_ALPHATEST_FUNC_GREATER | GLS_ALPHATEST_MAKE_REF( anMath::Ftob( 255.0f * regs[ pStage->alphaTestRegister ] ) ) );
#endif
				
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}
				
				RB_SetVertexColorParms( SVC_IGNORE );
				
				// bind the texture
				GL_SelectTexture( 0 );
				pStage->texture.image->Bind();
				
				// set texture matrix and texGens
				RB_PrepareStageTexturing( pStage, drawSurf );
				
				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );
				
				// draw it
				RB_DrawElementsWithCounters( drawSurf );
				
				// clean up
				RB_FinishStageTexturing( pStage, drawSurf );
				
				// unset privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
				}
			}
			
			if( !didDraw )
			{
				drawSolid = true;
			}
		}
		
		// draw the entire surface solid
		if( drawSolid )
		{
			if( shader->GetSort() == SS_SUBVIEW )
			{
				renderProgManager.BindShader_Color();
				GL_Color( color );
				GL_State( surfGLState );
			}
			else
			{
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_DepthSkinned();
				}
				else
				{
					renderProgManager.BindShader_Depth();
				}
				GL_State( surfGLState | GLS_ALPHAMASK );
			}
			
			// must render with less-equal for Z-Cull to work properly
			assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );
			
			// draw it
			RB_DrawElementsWithCounters( drawSurf );
		}
		
		renderLog.CloseBlock();
	}
	
#ifdef USE_CORE_PROFILE
	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );
#endif
}

/*
=====================
RB_FillDepthBufferFast

Optimized fast path code.

If there are subview surfaces, they must be guarded in the depth buffer to allow
the mirror / subview to show through underneath the current view rendering.

Surfaces with perforated shaders need the full shader setup done, but should be
drawn after the opaque surfaces.

The bulk of the surfaces should be simple opaque geometry that can be drawn very rapidly.

If there are no subview surfaces, we could clear to black and use fast-Z rendering
on the 360.
=====================
*/
static void RB_FillDepthBufferFast( drawSurf_t** drawSurfs, int numDrawSurfs )
{
	if( numDrawSurfs == 0 )
	{
		return;
	}
	
	// if we are just doing 2D rendering, no need to fill the depth buffer
	if( backEnd.viewDef->viewEntitys == nullptr )
	{
		return;
	}
	
	renderLog.OpenMainBlock( MRB_FILL_DEPTH_BUFFER );
	renderLog.OpenBlock( "RB_FillDepthBufferFast" );
	
	GL_StartDepthPass( backEnd.viewDef->scissor );
	
	// force MVP change on first surface
	backEnd.currentSpace = nullptr;
	
	// draw all the subview surfaces, which will already be at the start of the sorted list,
	// with the general purpose path
	GL_State( GLS_DEFAULT );
	
	int	surfNum;
	for( surfNum = 0; surfNum < numDrawSurfs; surfNum++ )
	{
		if( drawSurfs[surfNum]->material->GetSort() != SS_SUBVIEW )
		{
			break;
		}
		RB_FillDepthBufferGeneric( &drawSurfs[surfNum], 1 );
	}
	
	const drawSurf_t** perforatedSurfaces = ( const drawSurf_t** )_alloca( numDrawSurfs * sizeof( drawSurf_t* ) );
	int numPerforatedSurfaces = 0;
	
	// draw all the opaque surfaces and build up a list of perforated surfaces that
	// we will defer drawing until all opaque surfaces are done
	GL_State( GLS_DEFAULT );
	
	// continue checking past the subview surfaces
	for( ; surfNum < numDrawSurfs; surfNum++ )
	{
		const drawSurf_t* surf = drawSurfs[ surfNum ];
		const idMaterial* shader = surf->material;
		
		// translucent surfaces don't put anything in the depth buffer
		if( shader->Coverage() == MC_TRANSLUCENT )
		{
			continue;
		}
		if( shader->Coverage() == MC_PERFORATED )
		{
			// save for later drawing
			perforatedSurfaces[ numPerforatedSurfaces ] = surf;
			numPerforatedSurfaces++;
			continue;
		}
		
		// set polygon offset?
		
		// set mvp matrix
		if( surf->space != backEnd.currentSpace )
		{
			RB_SetMVP( surf->space->mvp );
			backEnd.currentSpace = surf->space;
		}
		
		renderLog.OpenBlock( shader->GetName() );
		
		if( surf->jointCache )
		{
			renderProgManager.BindShader_DepthSkinned();
		}
		else
		{
			renderProgManager.BindShader_Depth();
		}
		
		// must render with less-equal for Z-Cull to work properly
		assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );
		
		// draw it solid
		RB_DrawElementsWithCounters( surf );
		
		renderLog.CloseBlock();
	}
	
	// draw all perforated surfaces with the general code path
	if( numPerforatedSurfaces > 0 )
	{
		RB_FillDepthBufferGeneric( perforatedSurfaces, numPerforatedSurfaces );
	}
	
	// Allow platform specific data to be collected after the depth pass.
	GL_FinishDepthPass();
	
	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

const int INTERACTION_TEXUNIT_BUMP			= 0;
const int INTERACTION_TEXUNIT_FALLOFF		= 1;
const int INTERACTION_TEXUNIT_PROJECTION	= 2;
const int INTERACTION_TEXUNIT_DIFFUSE		= 3;
const int INTERACTION_TEXUNIT_SPECULAR		= 4;
const int INTERACTION_TEXUNIT_SHADOWMAPS	= 5;
const int INTERACTION_TEXUNIT_JITTER		= 6;

/*
==================
RB_SetupInteractionStage
==================
*/
static void RB_SetupInteractionStage( const shaderStage_t* surfaceStage, const float* surfaceRegs, const float lightColor[4],
									  anVec4 matrix[2], float color[4] )
{

	if( surfaceStage->texture.hasMatrix )
	{
		matrix[0][0] = surfaceRegs[surfaceStage->texture.matrix[0][0]];
		matrix[0][1] = surfaceRegs[surfaceStage->texture.matrix[0][1]];
		matrix[0][2] = 0.0f;
		matrix[0][3] = surfaceRegs[surfaceStage->texture.matrix[0][2]];
		
		matrix[1][0] = surfaceRegs[surfaceStage->texture.matrix[1][0]];
		matrix[1][1] = surfaceRegs[surfaceStage->texture.matrix[1][1]];
		matrix[1][2] = 0.0f;
		matrix[1][3] = surfaceRegs[surfaceStage->texture.matrix[1][2]];
		
		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		if( matrix[0][3] < -40.0f || matrix[0][3] > 40.0f )
		{
			matrix[0][3] -= anMath::Ftoi( matrix[0][3] );
		}
		if( matrix[1][3] < -40.0f || matrix[1][3] > 40.0f )
		{
			matrix[1][3] -= anMath::Ftoi( matrix[1][3] );
		}
	}
	else
	{
		matrix[0][0] = 1.0f;
		matrix[0][1] = 0.0f;
		matrix[0][2] = 0.0f;
		matrix[0][3] = 0.0f;
		
		matrix[1][0] = 0.0f;
		matrix[1][1] = 1.0f;
		matrix[1][2] = 0.0f;
		matrix[1][3] = 0.0f;
	}
	
	if( color != nullptr )
	{
		for( int i = 0; i < 4; i++ )
		{
			// clamp here, so cards with a greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			color[i] = anMath::ClampFloat( 0.0f, 1.0f, surfaceRegs[surfaceStage->color.registers[i]] ) * lightColor[i];
		}
	}
}

/*
=================
RB_DrawSingleInteraction
=================
*/
static void RB_DrawSingleInteraction( drawInteraction_t* din )
{
	if( din->bumpImage == nullptr )
	{
		// stage wasn't actually an interaction
		return;
	}
	
	if( din->diffuseImage == nullptr || r_skipDiffuse.GetBool() )
	{
		// this isn't a YCoCg black, but it doesn't matter, because
		// the diffuseColor will also be 0
		din->diffuseImage = globalImages->blackImage;
	}
	if( din->specularImage == nullptr || r_skipSpecular.GetBool() || din->ambientLight )
	{
		din->specularImage = globalImages->blackImage;
	}
	if( r_skipBump.GetBool() )
	{
		din->bumpImage = globalImages->flatNormalMap;
	}
	
	// if we wouldn't draw anything, don't call the Draw function
	const bool diffuseIsBlack = ( din->diffuseImage == globalImages->blackImage )
								|| ( ( din->diffuseColor[0] <= 0 ) && ( din->diffuseColor[1] <= 0 ) && ( din->diffuseColor[2] <= 0 ) );
	const bool specularIsBlack = ( din->specularImage == globalImages->blackImage )
								 || ( ( din->specularColor[0] <= 0 ) && ( din->specularColor[1] <= 0 ) && ( din->specularColor[2] <= 0 ) );
	if( diffuseIsBlack && specularIsBlack )
	{
		return;
	}
	
	// bump matrix
	SetVertexParm( RENDERPARM_BUMPMATRIX_S, din->bumpMatrix[0].ToFloatPtr() );
	SetVertexParm( RENDERPARM_BUMPMATRIX_T, din->bumpMatrix[1].ToFloatPtr() );
	
	// diffuse matrix
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_S, din->diffuseMatrix[0].ToFloatPtr() );
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_T, din->diffuseMatrix[1].ToFloatPtr() );
	
	// specular matrix
	SetVertexParm( RENDERPARM_SPECULARMATRIX_S, din->specularMatrix[0].ToFloatPtr() );
	SetVertexParm( RENDERPARM_SPECULARMATRIX_T, din->specularMatrix[1].ToFloatPtr() );
	
	RB_SetVertexColorParms( din->vertexColor );
	
	SetFragmentParm( RENDERPARM_DIFFUSEMODIFIER, din->diffuseColor.ToFloatPtr() );
	SetFragmentParm( RENDERPARM_SPECULARMODIFIER, din->specularColor.ToFloatPtr() );
	
	// texture 0 will be the per-surface bump map
	GL_SelectTexture( INTERACTION_TEXUNIT_BUMP );
	din->bumpImage->Bind();
	
	// texture 3 is the per-surface diffuse map
	GL_SelectTexture( INTERACTION_TEXUNIT_DIFFUSE );
	din->diffuseImage->Bind();
	
	// texture 4 is the per-surface specular map
	GL_SelectTexture( INTERACTION_TEXUNIT_SPECULAR );
	din->specularImage->Bind();
	
	RB_DrawElementsWithCounters( din->surf );
}

/*
=================
RB_SetupForFastPathInteractions

These are common for all fast path surfaces
=================
*/
static void RB_SetupForFastPathInteractions( const anVec4& diffuseColor, const anVec4& specularColor )
{
	const anVec4 sMatrix( 1, 0, 0, 0 );
	const anVec4 tMatrix( 0, 1, 0, 0 );
	
	// bump matrix
	SetVertexParm( RENDERPARM_BUMPMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_BUMPMATRIX_T, tMatrix.ToFloatPtr() );
	
	// diffuse matrix
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_DIFFUSEMATRIX_T, tMatrix.ToFloatPtr() );
	
	// specular matrix
	SetVertexParm( RENDERPARM_SPECULARMATRIX_S, sMatrix.ToFloatPtr() );
	SetVertexParm( RENDERPARM_SPECULARMATRIX_T, tMatrix.ToFloatPtr() );
	
	RB_SetVertexColorParms( SVC_IGNORE );
	
	SetFragmentParm( RENDERPARM_DIFFUSEMODIFIER, diffuseColor.ToFloatPtr() );
	SetFragmentParm( RENDERPARM_SPECULARMODIFIER, specularColor.ToFloatPtr() );
}

/*
=============
RB_RenderInteractions

With added sorting and trivial path work.
=============
*/
static void RB_RenderInteractions( const drawSurf_t* surfList, const viewLight_t* vLight, int depthFunc, bool performStencilTest, bool useLightDepthBounds )
{
	if( surfList == nullptr )
	{
		return;
	}
	
	// change the scissor if needed, it will be constant across all the surfaces lit by the light
	if( !backEnd.currentScissor.Equals( vLight->scissorRect ) && r_useScissor.GetBool() )
	{
		GL_Scissor( backEnd.viewDef->viewport.x1 + vLight->scissorRect.x1,
					backEnd.viewDef->viewport.y1 + vLight->scissorRect.y1,
					vLight->scissorRect.x2 + 1 - vLight->scissorRect.x1,
					vLight->scissorRect.y2 + 1 - vLight->scissorRect.y1 );
		backEnd.currentScissor = vLight->scissorRect;
	}
	
	// perform setup here that will be constant for all interactions
	if( performStencilTest )
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | depthFunc | GLS_STENCIL_FUNC_EQUAL | GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );
		
	}
	else
	{
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | depthFunc | GLS_STENCIL_FUNC_ALWAYS );
	}
	
	// some rare lights have multiple animating stages, loop over them outside the surface list
	const idMaterial* lightShader = vLight->lightShader;
	const float* lightRegs = vLight->shaderRegisters;
	
	drawInteraction_t inter = {};
	inter.ambientLight = lightShader->IsAmbientLight();
	
	//---------------------------------
	// Split out the complex surfaces from the fast-path surfaces
	// so we can do the fast path ones all in a row.
	// The surfaces should already be sorted by space because they
	// are added single-threaded, and there is only a negligable amount
	// of benefit to trying to sort by materials.
	//---------------------------------
	static const int MAX_INTERACTIONS_PER_LIGHT = 1024;
	static const int MAX_COMPLEX_INTERACTIONS_PER_LIGHT = 256;
	anStaticList< const drawSurf_t*, MAX_INTERACTIONS_PER_LIGHT > allSurfaces;
	anStaticList< const drawSurf_t*, MAX_COMPLEX_INTERACTIONS_PER_LIGHT > complexSurfaces;
	for( const drawSurf_t* walk = surfList; walk != nullptr; walk = walk->nextOnLight )
	{
	
		// make sure the triangle culling is done
		if( walk->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( walk->shadowVolumeState == SHADOWVOLUME_UNFINISHED || walk->shadowVolumeState == SHADOWVOLUME_DONE );
			
			uint64 start = Sys_Microseconds();
			while( walk->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = Sys_Microseconds();
			
			backEnd.pc.shadowMicroSec += end - start;
		}
		
		const idMaterial* surfaceShader = walk->material;
		if( surfaceShader->GetFastPathBumpImage() )
		{
			allSurfaces.Append( walk );
		}
		else
		{
			complexSurfaces.Append( walk );
		}
	}
	for( int i = 0; i < complexSurfaces.Num(); i++ )
	{
		allSurfaces.Append( complexSurfaces[i] );
	}
	
	bool lightDepthBoundsDisabled = false;
	
	// RB begin
	if( r_useShadowMapping.GetBool() )
	{
		const static int JITTER_SIZE = 128;
		
		// default high quality
		float jitterSampleScale = 1.0f;
		float shadowMapSamples = r_shadowMapSamples.GetInteger();
		
		// screen power of two correction factor
		float screenCorrectionParm[4];
		screenCorrectionParm[0] = 1.0f / ( JITTER_SIZE * shadowMapSamples ) ;
		screenCorrectionParm[1] = 1.0f / JITTER_SIZE;
		screenCorrectionParm[2] = 1.0f / shadowMapResolutions[vLight->shadowLOD];
		screenCorrectionParm[3] = shadowMapSamples;
		SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
		
		float jitterTexScale[4];
		jitterTexScale[0] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;	// TODO shadow buffer size fraction shadowMapSize / maxShadowMapSize
		jitterTexScale[1] = r_shadowMapJitterScale.GetFloat() * jitterSampleScale;
		jitterTexScale[2] = -r_shadowMapBiasScale.GetFloat();
		jitterTexScale[3] = 0.0f;
		SetFragmentParm( RENDERPARM_JITTERTEXSCALE, jitterTexScale ); // rpJitterTexScale
		
		float jitterTexOffset[4];
		if( r_shadowMapRandomizeJitter.GetBool() )
		{
			jitterTexOffset[0] = ( rand() & 255 ) / 255.0;
			jitterTexOffset[1] = ( rand() & 255 ) / 255.0;
		}
		else
		{
			jitterTexOffset[0] = 0;
			jitterTexOffset[1] = 0;
		}
		jitterTexOffset[2] = 0.0f;
		jitterTexOffset[3] = 0.0f;
		SetFragmentParm( RENDERPARM_JITTERTEXOFFSET, jitterTexOffset ); // rpJitterTexOffset
		
		if( vLight->parallel )
		{
			float cascadeDistances[4];
			cascadeDistances[0] = backEnd.viewDef->frustumSplitDistances[0];
			cascadeDistances[1] = backEnd.viewDef->frustumSplitDistances[1];
			cascadeDistances[2] = backEnd.viewDef->frustumSplitDistances[2];
			cascadeDistances[3] = backEnd.viewDef->frustumSplitDistances[3];
			SetFragmentParm( RENDERPARM_CASCADEDISTANCES, cascadeDistances ); // rpCascadeDistances
		}
		
	}
	// RB end
	
	for( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ )
	{
		const shaderStage_t*	lightStage = lightShader->GetStage( lightStageNum );
		
		// ignore stages that fail the condition
		if( !lightRegs[ lightStage->conditionRegister ] )
		{
			continue;
		}
		
		const float lightScale = r_lightScale.GetFloat();
		const anVec4 lightColor(
			lightScale * lightRegs[ lightStage->color.registers[0] ],
			lightScale * lightRegs[ lightStage->color.registers[1] ],
			lightScale * lightRegs[ lightStage->color.registers[2] ],
			lightRegs[ lightStage->color.registers[3] ] );
		// apply the world-global overbright and the 2x factor for specular
		const anVec4 diffuseColor = lightColor;
		const anVec4 specularColor = lightColor * 2.0f;
		
		float lightTextureMatrix[16];
		if( lightStage->texture.hasMatrix )
		{
			RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, lightTextureMatrix );
		}
		
		// texture 1 will be the light falloff texture
		GL_SelectTexture( INTERACTION_TEXUNIT_FALLOFF );
		vLight->falloffImage->Bind();
		
		// texture 2 will be the light projection texture
		GL_SelectTexture( INTERACTION_TEXUNIT_PROJECTION );
		lightStage->texture.image->Bind();
		
		if( r_useShadowMapping.GetBool() )
		{
			// texture 5 will be the shadow maps array
			GL_SelectTexture( INTERACTION_TEXUNIT_SHADOWMAPS );
			globalImages->shadowImage[vLight->shadowLOD]->Bind();
			
			// texture 6 will be the jitter texture for soft shadowing
			GL_SelectTexture( INTERACTION_TEXUNIT_JITTER );
			if( r_shadowMapSamples.GetInteger() == 16 )
			{
				globalImages->jitterImage16->Bind();
			}
			else if( r_shadowMapSamples.GetInteger() == 4 )
			{
				globalImages->jitterImage4->Bind();
			}
			else
			{
				globalImages->jitterImage1->Bind();
			}
		}
		
		// force the light textures to not use anisotropic filtering, which is wasted on them
		// all of the texture sampler parms should be constant for all interactions, only
		// the actual texture image bindings will change
		
		//----------------------------------
		// For all surfaces on this light list, generate an interaction for this light stage
		//----------------------------------
		
		// setup renderparms assuming we will be drawing trivial surfaces first
		RB_SetupForFastPathInteractions( diffuseColor, specularColor );
		
		// even if the space does not change between light stages, each light stage may need a different lightTextureMatrix baked in
		backEnd.currentSpace = nullptr;
		
		for( int sortedSurfNum = 0; sortedSurfNum < allSurfaces.Num(); sortedSurfNum++ )
		{
			const drawSurf_t* const surf = allSurfaces[ sortedSurfNum ];
			
			// select the render prog
			if( lightShader->IsAmbientLight() )
			{
				if( surf->jointCache )
				{
					renderProgManager.BindShader_InteractionAmbientSkinned();
				}
				else
				{
					renderProgManager.BindShader_InteractionAmbient();
				}
			}
			else
			{
				if( r_useShadowMapping.GetBool() && vLight->globalShadows )
				{
					// RB: we have shadow mapping enabled and shadow maps so do a shadow compare
					
					if( vLight->parallel )
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Parallel_Skinned();
						}
						else
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Parallel();
						}
					}
					else if( vLight->pointLight )
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Point_Skinned();
						}
						else
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Point();
						}
					}
					else
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Spot_Skinned();
						}
						else
						{
							renderProgManager.BindShader_Interaction_ShadowMapping_Spot();
						}
					}
				}
				else
				{
					if( surf->jointCache )
					{
						renderProgManager.BindShader_InteractionSkinned();
					}
					else
					{
						renderProgManager.BindShader_Interaction();
					}
				}
			}
			
			const idMaterial* surfaceShader = surf->material;
			const float* surfaceRegs = surf->shaderRegisters;
			
			inter.surf = surf;
			
			// change the MVP matrix, view/light origin and light projection vectors if needed
			if( surf->space != backEnd.currentSpace )
			{
				backEnd.currentSpace = surf->space;
				
				// turn off the light depth bounds test if this model is rendered with a depth hack
				if( useLightDepthBounds )
				{
					if( !surf->space->weaponDepthHack && surf->space->modelDepthHack == 0.0f )
					{
						if( lightDepthBoundsDisabled )
						{
							GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
							lightDepthBoundsDisabled = false;
						}
					}
					else
					{
						if( !lightDepthBoundsDisabled )
						{
							GL_DepthBoundsTest( 0.0f, 0.0f );
							lightDepthBoundsDisabled = true;
						}
					}
				}
				
				// model-view-projection
				RB_SetMVP( surf->space->mvp );
				
				// RB begin
				idRenderMatrix modelMatrix;
				idRenderMatrix::Transpose( *( idRenderMatrix* )surf->space->modelMatrix, modelMatrix );
				
				SetVertexParms( RENDERPARM_MODELMATRIX_X, modelMatrix[0], 4 );
				
				// for determining the shadow mapping cascades
				idRenderMatrix modelViewMatrix, tmp;
				idRenderMatrix::Transpose( *( idRenderMatrix* )surf->space->modelViewMatrix, modelViewMatrix );
				SetVertexParms( RENDERPARM_MODELVIEWMATRIX_X, modelViewMatrix[0], 4 );
				
				anVec4 globalLightOrigin( vLight->globalLightOrigin.x, vLight->globalLightOrigin.y, vLight->globalLightOrigin.z, 1.0f );
				SetVertexParm( RENDERPARM_GLOBALLIGHTORIGIN, globalLightOrigin.ToFloatPtr() );
				// RB end
				
				// tranform the light/view origin into model local space
				anVec4 localLightOrigin( 0.0f );
				anVec4 localViewOrigin( 1.0f );
				R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, localLightOrigin.ToVec3() );
				R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewOrigin.ToVec3() );
				
				// set the local light/view origin
				SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLightOrigin.ToFloatPtr() );
				SetVertexParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );
				
				// transform the light project into model local space
				idPlane lightProjection[4];
				for( int i = 0; i < 4; i++ )
				{
					R_GlobalPlaneToLocal( surf->space->modelMatrix, vLight->lightProject[i], lightProjection[i] );
				}
				
				// optionally multiply the local light projection by the light texture matrix
				if( lightStage->texture.hasMatrix )
				{
					RB_BakeTextureMatrixIntoTexgen( lightProjection, lightTextureMatrix );
				}
				
				// set the light projection
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_S, lightProjection[0].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_T, lightProjection[1].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTPROJECTION_Q, lightProjection[2].ToFloatPtr() );
				SetVertexParm( RENDERPARM_LIGHTFALLOFF_S, lightProjection[3].ToFloatPtr() );
				
				// RB begin
				if( r_useShadowMapping.GetBool() )
				{
					if( vLight->parallel )
					{
						for( int i = 0; i < ( r_shadowMapSplits.GetInteger() + 1 ); i++ )
						{
							idRenderMatrix modelToShadowMatrix;
							idRenderMatrix::Multiply( backEnd.shadowV[i], modelMatrix, modelToShadowMatrix );
							
							idRenderMatrix shadowClipMVP;
							idRenderMatrix::Multiply( backEnd.shadowP[i], modelToShadowMatrix, shadowClipMVP );
							
							idRenderMatrix shadowWindowMVP;
							idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, shadowClipMVP, shadowWindowMVP );
							
							SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowWindowMVP[0], 4 );
						}
					}
					else if( vLight->pointLight )
					{
						for( int i = 0; i < 6; i++ )
						{
							idRenderMatrix modelToShadowMatrix;
							idRenderMatrix::Multiply( backEnd.shadowV[i], modelMatrix, modelToShadowMatrix );
							
							idRenderMatrix shadowClipMVP;
							idRenderMatrix::Multiply( backEnd.shadowP[i], modelToShadowMatrix, shadowClipMVP );
							
							idRenderMatrix shadowWindowMVP;
							idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, shadowClipMVP, shadowWindowMVP );
							
							SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X + i * 4 ), shadowWindowMVP[0], 4 );
						}
					}
					else
					{
						// spot light
						
						idRenderMatrix modelToShadowMatrix;
						idRenderMatrix::Multiply( backEnd.shadowV[0], modelMatrix, modelToShadowMatrix );
						
						idRenderMatrix shadowClipMVP;
						idRenderMatrix::Multiply( backEnd.shadowP[0], modelToShadowMatrix, shadowClipMVP );
						
						SetVertexParms( ( renderParm_t )( RENDERPARM_SHADOW_MATRIX_0_X ), shadowClipMVP[0], 4 );
						
					}
				}
				// RB end
			}
			
			// check for the fast path
			if( surfaceShader->GetFastPathBumpImage() && !r_skipInteractionFastPath.GetBool() )
			{
				renderLog.OpenBlock( surf->material->GetName() );
				
				// texture 0 will be the per-surface bump map
				GL_SelectTexture( INTERACTION_TEXUNIT_BUMP );
				surfaceShader->GetFastPathBumpImage()->Bind();
				
				// texture 3 is the per-surface diffuse map
				GL_SelectTexture( INTERACTION_TEXUNIT_DIFFUSE );
				surfaceShader->GetFastPathDiffuseImage()->Bind();
				
				// texture 4 is the per-surface specular map
				GL_SelectTexture( INTERACTION_TEXUNIT_SPECULAR );
				surfaceShader->GetFastPathSpecularImage()->Bind();
				
				RB_DrawElementsWithCounters( surf );
				
				renderLog.CloseBlock();
				continue;
			}
			
			renderLog.OpenBlock( surf->material->GetName() );
			
			inter.bumpImage = nullptr;
			inter.specularImage = nullptr;
			inter.diffuseImage = nullptr;
			inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
			inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;
			
			// go through the individual surface stages
			//
			// This is somewhat arcane because of the old support for video cards that had to render
			// interactions in multiple passes.
			//
			// We also have the very rare case of some materials that have conditional interactions
			// for the "hell writing" that can be shined on them.
			for( int surfaceStageNum = 0; surfaceStageNum < surfaceShader->GetNumStages(); surfaceStageNum++ )
			{
				const shaderStage_t*	surfaceStage = surfaceShader->GetStage( surfaceStageNum );
				
				switch ( surfaceStage->lighting )
				{
					case SL_COVERAGE:
					{
						// ignore any coverage stages since they should only be used for the depth fill pass
						// for diffuse stages that use alpha test.
						break;
					}
					case SL_AMBIENT:
					{
						// ignore ambient stages while drawing interactions
						break;
					}
					case SL_BUMP:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] )
						{
							break;
						}
						// draw any previous interaction
						if( inter.bumpImage != nullptr )
						{
							RB_DrawSingleInteraction( &inter );
						}
						inter.bumpImage = surfaceStage->texture.image;
						inter.diffuseImage = nullptr;
						inter.specularImage = nullptr;
						RB_SetupInteractionStage( surfaceStage, surfaceRegs, nullptr,
												  inter.bumpMatrix, nullptr );
						break;
					}
					case SL_DIFFUSE:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] )
						{
							break;
						}
						// draw any previous interaction
						if( inter.diffuseImage != nullptr )
						{
							RB_DrawSingleInteraction( &inter );
						}
						inter.diffuseImage = surfaceStage->texture.image;
						inter.vertexColor = surfaceStage->vertexColor;
						RB_SetupInteractionStage( surfaceStage, surfaceRegs, diffuseColor.ToFloatPtr(),
												  inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
						break;
					}
					case SL_SPECULAR:
					{
						// ignore stage that fails the condition
						if( !surfaceRegs[ surfaceStage->conditionRegister ] )
						{
							break;
						}
						// draw any previous interaction
						if( inter.specularImage != nullptr )
						{
							RB_DrawSingleInteraction( &inter );
						}
						inter.specularImage = surfaceStage->texture.image;
						inter.vertexColor = surfaceStage->vertexColor;
						RB_SetupInteractionStage( surfaceStage, surfaceRegs, specularColor.ToFloatPtr(),
												  inter.specularMatrix, inter.specularColor.ToFloatPtr() );
						break;
					}
				}
			}
			
			// draw the final interaction
			RB_DrawSingleInteraction( &inter );
			
			renderLog.CloseBlock();
		}
	}
	
	if( useLightDepthBounds && lightDepthBoundsDisabled )
	{
		GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
	}
	
	renderProgManager.Unbind();
}

/*
==============================================================================================

STENCIL SHADOW RENDERING

==============================================================================================
*/

/*
=====================
RB_StencilShadowPass

The stencil buffer should have been set to 128 on any surfaces that might receive shadows.
=====================
*/
static void RB_StencilShadowPass( const drawSurf_t* drawSurfs, const viewLight_t* vLight )
{
	if( r_skipShadows.GetBool() )
	{
		return;
	}
	
	if( drawSurfs == nullptr )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "---------- RB_StencilShadowPass ----------\n" );
	
	renderProgManager.BindShader_Shadow();
	
	GL_SelectTexture( 0 );
	globalImages->BindNull();
	
	uint64 glState = 0;
	
	// for visualizing the shadows
	if( r_showShadows.GetInteger() )
	{
		// set the debug shadow color
		SetFragmentParm( RENDERPARM_COLOR, colorMagenta.ToFloatPtr() );
		if( r_showShadows.GetInteger() == 2 )
		{
			// draw filled in
			glState = GLS_DEPTHMASK | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_LESS;
		}
		else
		{
			// draw as lines, filling the depth buffer
			glState = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_POLYMODE_LINE | GLS_DEPTHFUNC_ALWAYS;
		}
	}
	else
	{
		// don't write to the color or depth buffer, just the stencil buffer
		glState = GLS_DEPTHMASK | GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHFUNC_LESS;
	}
	
	GL_PolygonOffset( r_shadowPolygonFactor.GetFloat(), -r_shadowPolygonOffset.GetFloat() );
	
	// the actual stencil func will be set in the draw code, but we need to make sure it isn't
	// disabled here, and that the value will get reset for the interactions without looking
	// like a no-change-required
	GL_State( glState | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR |
			  GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) | GLS_POLYGON_OFFSET );
			  
	// Two Sided Stencil reduces two draw calls to one for slightly faster shadows
	GL_Cull( CT_TWO_SIDED );
	
	
	// process the chain of shadows with the current rendering state
	backEnd.currentSpace = nullptr;
	
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != nullptr; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}
		
		// make sure the shadow volume is done
		if( drawSurf->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED || drawSurf->shadowVolumeState == SHADOWVOLUME_DONE );
			
			uint64 start = Sys_Microseconds();
			while( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = Sys_Microseconds();
			
			backEnd.pc.shadowMicroSec += end - start;
		}
		
		if( drawSurf->numIndexes == 0 )
		{
			continue;	// a job may have created an empty shadow volume
		}
		
		if( !backEnd.currentScissor.Equals( drawSurf->scissorRect ) && r_useScissor.GetBool() )
		{
			// change the scissor
			GL_Scissor( backEnd.viewDef->viewport.x1 + drawSurf->scissorRect.x1,
						backEnd.viewDef->viewport.y1 + drawSurf->scissorRect.y1,
						drawSurf->scissorRect.x2 + 1 - drawSurf->scissorRect.x1,
						drawSurf->scissorRect.y2 + 1 - drawSurf->scissorRect.y1 );
			backEnd.currentScissor = drawSurf->scissorRect;
		}
		
		if( drawSurf->space != backEnd.currentSpace )
		{
			// change the matrix
			RB_SetMVP( drawSurf->space->mvp );
			
			// set the local light position to allow the vertex program to project the shadow volume end cap to infinity
			anVec4 localLight( 0.0f );
			R_GlobalPointToLocal( drawSurf->space->modelMatrix, vLight->globalLightOrigin, localLight.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLight.ToFloatPtr() );
			
			backEnd.currentSpace = drawSurf->space;
		}
		
		if( r_showShadows.GetInteger() == 0 )
		{
			if( drawSurf->jointCache )
			{
				renderProgManager.BindShader_ShadowSkinned();
			}
			else
			{
				renderProgManager.BindShader_Shadow();
			}
		}
		else
		{
			if( drawSurf->jointCache )
			{
				renderProgManager.BindShader_ShadowDebugSkinned();
			}
			else
			{
				renderProgManager.BindShader_ShadowDebug();
			}
		}
		
		// set depth bounds per shadow
		if( r_useShadowDepthBounds.GetBool() )
		{
			GL_DepthBoundsTest( drawSurf->scissorRect.zmin, drawSurf->scissorRect.zmax );
		}
		
		// Determine whether or not the shadow volume needs to be rendered with Z-pass or
		// Z-fail. It is worthwhile to spend significant resources to reduce the number of
		// cases where shadow volumes need to be rendered with Z-fail because Z-fail
		// rendering can be significantly slower even on today's hardware. For instance,
		// on NVIDIA hardware Z-fail rendering causes the Z-Cull to be used in reverse:
		// Z-near becomes Z-far (trivial accept becomes trivial reject). Using the Z-Cull
		// in reverse is far less efficient because the Z-Cull only stores Z-near per 16x16
		// pixels while the Z-far is stored per 4x2 pixels. (The Z-near coallesce buffer
		// which has 4x4 granularity is only used when updating the depth which is not the
		// case for shadow volumes.) Note that it is also important to NOT use a Z-Cull
		// reconstruct because that would clear the Z-near of the Z-Cull which results in
		// no trivial rejection for Z-fail stencil shadow rendering.
		
		const bool renderZPass = ( drawSurf->renderZFail == 0 ) || r_forceZPassStencilShadows.GetBool();
		
		
		if( renderZPass )
		{
			// Z-pass
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR );
		}
		else if( r_useStencilShadowPreload.GetBool() )
		{
			// preload + Z-pass
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_DECR, GL_DECR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR, GL_INCR );
		}
		else
		{
			// Z-fail
			// LEITH: warning this is patented by Creative Labs in US ( software patents are bad )
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_DECR, GL_KEEP );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_INCR, GL_KEEP );

		}
		
		
		// get vertex buffer
		const vertCacheHandle_t vbHandle = drawSurf->shadowCache;
		idVertexBuffer* vertexBuffer;
		if( vertexCache.CacheIsStatic( vbHandle ) )
		{
			vertexBuffer = &vertexCache.staticData.vertexBuffer;
		}
		else
		{
			const uint64 frameNum = ( int )( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
			if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
			{
				idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == nullptr" );
				continue;
			}
			vertexBuffer = &vertexCache.frameData[vertexCache.drawListNum].vertexBuffer;
		}
		const int vertOffset = ( int )( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
		
		// get index buffer
		const vertCacheHandle_t ibHandle = drawSurf->indexCache;
		idIndexBuffer* indexBuffer;
		if( vertexCache.CacheIsStatic( ibHandle ) )
		{
			indexBuffer = &vertexCache.staticData.indexBuffer;
		}
		else
		{
			const uint64 frameNum = ( int )( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
			if( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) )
			{
				idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == nullptr" );
				continue;
			}
			indexBuffer = &vertexCache.frameData[vertexCache.drawListNum].indexBuffer;
		}
		const uint64 indexOffset = ( int )( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
		
		RENDERLOG_PRINTF( "Binding Buffers: %p %p\n", vertexBuffer, indexBuffer );
		
		// RB: 64 bit fixes, changed GLuint to GLintptr
		if( backEnd.glState.currentIndexBuffer != ( GLintptr )indexBuffer->GetAPIObject() || !r_useStateCaching.GetBool() )
		{
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ( GLintptr )indexBuffer->GetAPIObject() );
			backEnd.glState.currentIndexBuffer = ( GLintptr )indexBuffer->GetAPIObject();
		}
		
		if( drawSurf->jointCache )
		{
			assert( renderProgManager.ShaderUsesJoints() );
			
			idJointBuffer jointBuffer;
			if( !vertexCache.GetJointBuffer( drawSurf->jointCache, &jointBuffer ) )
			{
				idLib::Warning( "RB_DrawElementsWithCounters, jointBuffer == nullptr" );
				continue;
			}
			assert( ( jointBuffer.GetOffset() & ( glConfig.uniformBufferOffsetAlignment - 1 ) ) == 0 );
			
			const GLintptr ubo = reinterpret_cast< GLintptr >( jointBuffer.GetAPIObject() );
			glBindBufferRange( GL_UNIFORM_BUFFER, 0, ubo, jointBuffer.GetOffset(), jointBuffer.GetNumJoints() * sizeof( idJointMat ) );
			
			if( ( backEnd.glState.vertexLayout != LAYOUT_DRAW_SHADOW_VERT_SKINNED ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vertexBuffer->GetAPIObject() ) || !r_useStateCaching.GetBool() )
			{
				glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer->GetAPIObject() );
				backEnd.glState.currentVertexBuffer = ( GLintptr )vertexBuffer->GetAPIObject();
				
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
				
#if defined(USE_GLES2) || defined(USE_GLES3)
				glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_XYZW_OFFSET ) );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_COLOR_OFFSET ) );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( vertOffset + SHADOWVERTSKINNED_COLOR2_OFFSET ) );
#else
				glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVertSkinned ), ( void* )( SHADOWVERTSKINNED_XYZW_OFFSET ) );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( SHADOWVERTSKINNED_COLOR_OFFSET ) );
				glVertexAttribPointer( PC_ATTRIB_INDEX_COLOR2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( idShadowVertSkinned ), ( void* )( SHADOWVERTSKINNED_COLOR2_OFFSET ) );
#endif
				
				backEnd.glState.vertexLayout = LAYOUT_DRAW_SHADOW_VERT_SKINNED;
			}
			
		}
		else
		{
			if( ( backEnd.glState.vertexLayout != LAYOUT_DRAW_SHADOW_VERT ) || ( backEnd.glState.currentVertexBuffer != ( GLintptr )vertexBuffer->GetAPIObject() ) || !r_useStateCaching.GetBool() )
			{
				glBindBuffer( GL_ARRAY_BUFFER, ( GLintptr )vertexBuffer->GetAPIObject() );
				backEnd.glState.currentVertexBuffer = ( GLintptr )vertexBuffer->GetAPIObject();
				
				glEnableVertexAttribArray( PC_ATTRIB_INDEX_VERTEX );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_NORMAL );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_COLOR2 );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_ST );
				glDisableVertexAttribArray( PC_ATTRIB_INDEX_TANGENT );
				
#if defined(USE_GLES2) || defined(USE_GLES3)
				glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVert ), ( void* )( vertOffset + SHADOWVERT_XYZW_OFFSET ) );
#else
				glVertexAttribPointer( PC_ATTRIB_INDEX_VERTEX, 4, GL_FLOAT, GL_FALSE, sizeof( idShadowVert ), ( void* )( SHADOWVERT_XYZW_OFFSET ) );
#endif
				
				backEnd.glState.vertexLayout = LAYOUT_DRAW_SHADOW_VERT;
			}
		}
		// RB end
		
		renderProgManager.CommitUniforms();
		
		if( drawSurf->jointCache )
		{
#if defined(USE_GLES3) //defined(USE_GLES2)
			glDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
#else
			glDrawElementsBaseVertex( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVertSkinned ) );
#endif
		}
		else
		{
#if defined(USE_GLES3)
			glDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
#else
			glDrawElementsBaseVertex( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVert ) );
#endif
		}
		
		// RB: added stats
		backEnd.pc.c_shadowElements++;
		backEnd.pc.c_shadowIndexes += drawSurf->numIndexes;
		// RB end
		
		if( !renderZPass && r_useStencilShadowPreload.GetBool() )
		{
			// render again with Z-pass
			glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR );
			glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR );
			
			if( drawSurf->jointCache )
			{
#if defined(USE_GLES3)
				glDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
#else
				glDrawElementsBaseVertex( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVertSkinned ) );
#endif
			}
			else
			{
#if defined(USE_GLES3)
				glDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset );
#else
				glDrawElementsBaseVertex( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : drawSurf->numIndexes, GL_INDEX_TYPE, ( triIndex_t* )indexOffset, vertOffset / sizeof( idShadowVert ) );
#endif
			}
			
			// RB: added stats
			backEnd.pc.c_shadowElements++;
			backEnd.pc.c_shadowIndexes += drawSurf->numIndexes;
			// RB end
		}
	}
	
	// cleanup the shadow specific rendering state
	
	GL_Cull( CT_FRONT_SIDED );
	
	// reset depth bounds
	if( r_useShadowDepthBounds.GetBool() )
	{
		if( r_useLightDepthBounds.GetBool() )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}
		else
		{
			GL_DepthBoundsTest( 0.0f, 0.0f );
		}
	}
}

/*
==================
RB_StencilSelectLight

Deform the zeroOneCubeModel to exactly cover the light volume. Render the deformed cube model to the stencil buffer in
such a way that only fragments that are directly visible and contained within the volume will be written creating a
mask to be used by the following stencil shadow and draw interaction passes.
==================
*/
static void RB_StencilSelectLight( const viewLight_t* vLight )
{
	renderLog.OpenBlock( "Stencil Select" );
	
	// enable the light scissor
	if( !backEnd.currentScissor.Equals( vLight->scissorRect ) && r_useScissor.GetBool() )
	{
		GL_Scissor( backEnd.viewDef->viewport.x1 + vLight->scissorRect.x1,
					backEnd.viewDef->viewport.y1 + vLight->scissorRect.y1,
					vLight->scissorRect.x2 + 1 - vLight->scissorRect.x1,
					vLight->scissorRect.y2 + 1 - vLight->scissorRect.y1 );
		backEnd.currentScissor = vLight->scissorRect;
	}
	
	// clear stencil buffer to 0 (not drawable)
	uint64 glStateMinusStencil = GL_GetCurrentStateMinusStencil();
	GL_State( glStateMinusStencil | GLS_STENCIL_FUNC_ALWAYS | GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );	// make sure stencil mask passes for the clear
	GL_Clear( false, false, true, 0, 0.0f, 0.0f, 0.0f, 0.0f );	// clear to 0 for stencil select
	
	// set the depthbounds
	GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
	
	
	GL_State( GLS_COLORMASK | GLS_ALPHAMASK | GLS_DEPTHMASK | GLS_DEPTHFUNC_LESS | GLS_STENCIL_FUNC_ALWAYS | GLS_STENCIL_MAKE_REF( STENCIL_SHADOW_TEST_VALUE ) | GLS_STENCIL_MAKE_MASK( STENCIL_SHADOW_MASK_VALUE ) );
	GL_Cull( CT_TWO_SIDED );
	
	renderProgManager.BindShader_Depth();
	
	// set the matrix for deforming the 'zeroOneCubeModel' into the frustum to exactly cover the light volume
	idRenderMatrix invProjectMVPMatrix;
	idRenderMatrix::Multiply( backEnd.viewDef->worldSpace.mvp, vLight->inverseBaseLightProject, invProjectMVPMatrix );
	RB_SetMVP( invProjectMVPMatrix );
	
	// two-sided stencil test
	glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_REPLACE, GL_ZERO );
	glStencilOpSeparate( GL_BACK, GL_KEEP, GL_ZERO, GL_REPLACE );
	
	RB_DrawElementsWithCounters( &backEnd.zeroOneCubeSurface );
	
	// reset stencil state
	
	GL_Cull( CT_FRONT_SIDED );
	
	renderProgManager.Unbind();
	
	
	// unset the depthbounds
	GL_DepthBoundsTest( 0.0f, 0.0f );
	
	renderLog.CloseBlock();
}

/*
==============================================================================================

SHADOW MAPS RENDERING

==============================================================================================
*/

/*
same as D3DXMatrixOrthoOffCenterRH

http://msdn.microsoft.com/en-us/library/bb205348(VS.85).aspx
*/
static void MatrixOrthogonalProjectionRH( float m[16], float left, float right, float bottom, float top, float zNear, float zFar )
{
	m[0] = 2 / ( right - left );
	m[4] = 0;
	m[8] = 0;
	m[12] = ( left + right ) / ( left - right );
	m[1] = 0;
	m[5] = 2 / ( top - bottom );
	m[9] = 0;
	m[13] = ( top + bottom ) / ( bottom - top );
	m[2] = 0;
	m[6] = 0;
	m[10] = 1 / ( zNear - zFar );
	m[14] = zNear / ( zNear - zFar );
	m[3] = 0;
	m[7] = 0;
	m[11] = 0;
	m[15] = 1;
}

void MatrixCrop( float m[16], const anVec3 mins, const anVec3 maxs )
{
	float			scaleX, scaleY, scaleZ;
	float			offsetX, offsetY, offsetZ;
	
	scaleX = 2.0f / ( maxs[0] - mins[0] );
	scaleY = 2.0f / ( maxs[1] - mins[1] );
	
	offsetX = -0.5f * ( maxs[0] + mins[0] ) * scaleX;
	offsetY = -0.5f * ( maxs[1] + mins[1] ) * scaleY;
	
	scaleZ = 1.0f / ( maxs[2] - mins[2] );
	offsetZ = -mins[2] * scaleZ;
	
	m[ 0] = scaleX;
	m[ 4] = 0;
	m[ 8] = 0;
	m[12] = offsetX;
	m[ 1] = 0;
	m[ 5] = scaleY;
	m[ 9] = 0;
	m[13] = offsetY;
	m[ 2] = 0;
	m[ 6] = 0;
	m[10] = scaleZ;
	m[14] = offsetZ;
	m[ 3] = 0;
	m[ 7] = 0;
	m[11] = 0;
	m[15] = 1;
}

void MatrixLookAtRH( float m[16], const anVec3& eye, const anVec3& dir, const anVec3& up )
{
	anVec3 dirN;
	anVec3 upN;
	anVec3 sideN;
	
	sideN = dir.Cross( up );
	sideN.Normalize();
	
	upN = sideN.Cross( dir );
	upN.Normalize();
	
	dirN = dir;
	dirN.Normalize();
	
	m[ 0] = sideN[0];
	m[ 4] = sideN[1];
	m[ 8] = sideN[2];
	m[12] = -( sideN * eye );
	m[ 1] = upN[0];
	m[ 5] = upN[1];
	m[ 9] = upN[2];
	m[13] = -( upN * eye );
	m[ 2] = -dirN[0];
	m[ 6] = -dirN[1];
	m[10] = -dirN[2];
	m[14] = ( dirN * eye );
	m[ 3] = 0;
	m[ 7] = 0;
	m[11] = 0;
	m[15] = 1;
}

/*
=====================
RB_ShadowMapPass
=====================
*/
static void RB_ShadowMapPass( const drawSurf_t* drawSurfs, const viewLight_t* vLight, int side )
{
	if( r_skipShadows.GetBool() )
	{
		return;
	}
	
	if( drawSurfs == nullptr )
	{
		return;
	}
	
	RENDERLOG_PRINTF( "---------- RB_ShadowMapPass( side = %i ) ----------\n", side );
	
	renderProgManager.BindShader_Depth();
	
	GL_SelectTexture( 0 );
	globalImages->BindNull();
	
	uint64 glState = 0;
	
	// the actual stencil func will be set in the draw code, but we need to make sure it isn't
	// disabled here, and that the value will get reset for the interactions without looking
	// like a no-change-required
	GL_State( glState | GLS_POLYGON_OFFSET );
	
	switch ( r_shadowMapOccluderFacing.GetInteger() )
	{
		case 0:
			GL_Cull( CT_FRONT_SIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;
			
		case 1:
			GL_Cull( CT_BACK_SIDED );
			GL_PolygonOffset( -r_shadowMapPolygonFactor.GetFloat(), -r_shadowMapPolygonOffset.GetFloat() );
			break;
			
		default:
			GL_Cull( CT_TWO_SIDED );
			GL_PolygonOffset( r_shadowMapPolygonFactor.GetFloat(), r_shadowMapPolygonOffset.GetFloat() );
			break;
	}
	
	idRenderMatrix lightProjectionRenderMatrix;
	idRenderMatrix lightViewRenderMatrix;
	
	
	if( vLight->parallel && side >= 0 )
	{
		assert( side >= 0 && side < 6 );
		
		// original light direction is from surface to light origin
		anVec3 lightDir = -vLight->lightCenter;
		if( lightDir.Normalize() == 0.0f )
		{
			lightDir[2] = -1.0f;
		}
		
		idMat3 rotation = lightDir.ToMat3();
		//idAngles angles = lightDir.ToAngles();
		//idMat3 rotation = angles.ToMat3();
		
		const anVec3 viewDir = backEnd.viewDef->renderView.viewaxis[0];
		const anVec3 viewPos = backEnd.viewDef->renderView.vieworg;
		
#if 1
		idRenderMatrix::CreateViewMatrix( backEnd.viewDef->renderView.vieworg, rotation, lightViewRenderMatrix );
#else
		float lightViewMatrix[16];
		MatrixLookAtRH( lightViewMatrix, viewPos, lightDir, viewDir );
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightViewMatrix, lightViewRenderMatrix );
#endif
		
		anBounds lightBounds;
		lightBounds.Clear();
		
		ALIGNTYPE16 frustumCorners_t corners;
		idRenderMatrix::GetFrustumCorners( corners, vLight->inverseBaseLightProject, bounds_zeroOneCube );
		
		anVec4 point, transf;
		for( int j = 0; j < 8; j++ )
		{
			point[0] = corners.x[j];
			point[1] = corners.y[j];
			point[2] = corners.z[j];
			point[3] = 1;
			
			lightViewRenderMatrix.TransformPoint( point, transf );
			transf[0] /= transf[3];
			transf[1] /= transf[3];
			transf[2] /= transf[3];
			
			lightBounds.AddPoint( transf.ToVec3() );
		}
		
		float lightProjectionMatrix[16];
		MatrixOrthogonalProjectionRH( lightProjectionMatrix, lightBounds[0][0], lightBounds[1][0], lightBounds[0][1], lightBounds[1][1], -lightBounds[1][2], -lightBounds[0][2] );
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );
		
		
		// 	'frustumMVP' goes from global space -> camera local space -> camera projective space
		// invert the MVP projection so we can deform zero-to-one cubes into the frustum pyramid shape and calculate global bounds
		
		idRenderMatrix splitFrustumInverse;
		if( !idRenderMatrix::Inverse( backEnd.viewDef->frustumMVPs[FRUSTUM_CASCADE1 + side], splitFrustumInverse ) )
		{
			idLib::Warning( "splitFrustumMVP invert failed" );
		}
		
		// splitFrustumCorners in global space
		ALIGNTYPE16 frustumCorners_t splitFrustumCorners;
		idRenderMatrix::GetFrustumCorners( splitFrustumCorners, splitFrustumInverse, bounds_unitCube );
		
#if 0
		anBounds splitFrustumBounds;
		splitFrustumBounds.Clear();
		for( int j = 0; j < 8; j++ )
		{
			point[0] = splitFrustumCorners.x[j];
			point[1] = splitFrustumCorners.y[j];
			point[2] = splitFrustumCorners.z[j];
			
			splitFrustumBounds.AddPoint( point.ToVec3() );
		}
		
		anVec3 center = splitFrustumBounds.GetCenter();
		float radius = splitFrustumBounds.GetRadius( center );
		
		//ALIGNTYPE16 frustumCorners_t splitFrustumCorners;
		splitFrustumBounds[0] = anVec3( -radius, -radius, -radius );
		splitFrustumBounds[1] = anVec3( radius, radius, radius );
		splitFrustumBounds.TranslateSelf( viewPos );
		anVec3 splitFrustumCorners2[8];
		splitFrustumBounds.ToPoints( splitFrustumCorners2 );
		
		for( int j = 0; j < 8; j++ )
		{
			splitFrustumCorners.x[j] = splitFrustumCorners2[j].x;
			splitFrustumCorners.y[j] = splitFrustumCorners2[j].y;
			splitFrustumCorners.z[j] = splitFrustumCorners2[j].z;
		}
#endif
		
		
		idRenderMatrix lightViewProjectionRenderMatrix;
		idRenderMatrix::Multiply( lightProjectionRenderMatrix, lightViewRenderMatrix, lightViewProjectionRenderMatrix );
		
		// find the bounding box of the current split in the light's clip space
		anBounds cropBounds;
		cropBounds.Clear();
		for( int j = 0; j < 8; j++ )
		{
			point[0] = splitFrustumCorners.x[j];
			point[1] = splitFrustumCorners.y[j];
			point[2] = splitFrustumCorners.z[j];
			point[3] = 1;
			
			lightViewRenderMatrix.TransformPoint( point, transf );
			transf[0] /= transf[3];
			transf[1] /= transf[3];
			transf[2] /= transf[3];
			
			cropBounds.AddPoint( transf.ToVec3() );
		}
		
		// don't let the frustum AABB be bigger than the light AABB
		if( cropBounds[0][0] < lightBounds[0][0] )
		{
			cropBounds[0][0] = lightBounds[0][0];
		}
		
		if( cropBounds[0][1] < lightBounds[0][1] )
		{
			cropBounds[0][1] = lightBounds[0][1];
		}
		
		if( cropBounds[1][0] > lightBounds[1][0] )
		{
			cropBounds[1][0] = lightBounds[1][0];
		}
		
		if( cropBounds[1][1] > lightBounds[1][1] )
		{
			cropBounds[1][1] = lightBounds[1][1];
		}
		
		cropBounds[0][2] = lightBounds[0][2];
		cropBounds[1][2] = lightBounds[1][2];
		
		//float cropMatrix[16];
		//MatrixCrop(cropMatrix, cropBounds[0], cropBounds[1]);
		
		//idRenderMatrix cropRenderMatrix;
		//idRenderMatrix::Transpose( *( idRenderMatrix* )cropMatrix, cropRenderMatrix );
		
		//idRenderMatrix tmp = lightProjectionRenderMatrix;
		//idRenderMatrix::Multiply( cropRenderMatrix, tmp, lightProjectionRenderMatrix );
		
		MatrixOrthogonalProjectionRH( lightProjectionMatrix, cropBounds[0][0], cropBounds[1][0], cropBounds[0][1], cropBounds[1][1], -cropBounds[1][2], -cropBounds[0][2] );
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );
		
		backEnd.shadowV[side] = lightViewRenderMatrix;
		backEnd.shadowP[side] = lightProjectionRenderMatrix;
	}
	else if( vLight->pointLight && side >= 0 )
	{
		assert( side >= 0 && side < 6 );
		
		// FIXME OPTIMIZE no memset
		
		float	viewMatrix[16];
		
		anVec3	vec;
		anVec3	origin = vLight->globalLightOrigin;
		
		// side of a point light
		memset( viewMatrix, 0, sizeof( viewMatrix ) );
		switch ( side )
		{
			case 0:
				viewMatrix[0] = 1;
				viewMatrix[9] = 1;
				viewMatrix[6] = -1;
				break;
			case 1:
				viewMatrix[0] = -1;
				viewMatrix[9] = -1;
				viewMatrix[6] = -1;
				break;
			case 2:
				viewMatrix[4] = 1;
				viewMatrix[1] = -1;
				viewMatrix[10] = 1;
				break;
			case 3:
				viewMatrix[4] = -1;
				viewMatrix[1] = -1;
				viewMatrix[10] = -1;
				break;
			case 4:
				viewMatrix[8] = 1;
				viewMatrix[1] = -1;
				viewMatrix[6] = -1;
				break;
			case 5:
				viewMatrix[8] = -1;
				viewMatrix[1] = 1;
				viewMatrix[6] = -1;
				break;
		}
		
		viewMatrix[12] = -origin[0] * viewMatrix[0] + -origin[1] * viewMatrix[4] + -origin[2] * viewMatrix[8];
		viewMatrix[13] = -origin[0] * viewMatrix[1] + -origin[1] * viewMatrix[5] + -origin[2] * viewMatrix[9];
		viewMatrix[14] = -origin[0] * viewMatrix[2] + -origin[1] * viewMatrix[6] + -origin[2] * viewMatrix[10];
		
		viewMatrix[3] = 0;
		viewMatrix[7] = 0;
		viewMatrix[11] = 0;
		viewMatrix[15] = 1;
		
		// from world space to light origin, looking down the X axis
		float	unflippedLightViewMatrix[16];
		
		// from world space to OpenGL view space, looking down the negative Z axis
		float	lightViewMatrix[16];
		
		static float	s_flipMatrix[16] =
		{
			// convert from our coordinate system (looking down X)
			// to OpenGL's coordinate system (looking down -Z)
			0, 0, -1, 0,
			-1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 1
		};
		
		memcpy( unflippedLightViewMatrix, viewMatrix, sizeof( unflippedLightViewMatrix ) );
		R_MatrixMultiply( viewMatrix, s_flipMatrix, lightViewMatrix );
		
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightViewMatrix, lightViewRenderMatrix );
		
		
		
		
		// set up 90 degree projection matrix
		const float zNear = 4;
		const float	fov = r_shadowMapFrustumFOV.GetFloat();
		
		float ymax = zNear * tan( fov * anMath::PI / 360.0f );
		float ymin = -ymax;
		
		float xmax = zNear * tan( fov * anMath::PI / 360.0f );
		float xmin = -xmax;
		
		const float width = xmax - xmin;
		const float height = ymax - ymin;
		
		// from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
		float lightProjectionMatrix[16];
		
		lightProjectionMatrix[0 * 4 + 0] = 2.0f * zNear / width;
		lightProjectionMatrix[1 * 4 + 0] = 0.0f;
		lightProjectionMatrix[2 * 4 + 0] = ( xmax + xmin ) / width;	// normally 0
		lightProjectionMatrix[3 * 4 + 0] = 0.0f;
		
		lightProjectionMatrix[0 * 4 + 1] = 0.0f;
		lightProjectionMatrix[1 * 4 + 1] = 2.0f * zNear / height;
		lightProjectionMatrix[2 * 4 + 1] = ( ymax + ymin ) / height;	// normally 0
		lightProjectionMatrix[3 * 4 + 1] = 0.0f;
		
		// this is the far-plane-at-infinity formulation, and
		// crunches the Z range slightly so w=0 vertexes do not
		// rasterize right at the wraparound point
		lightProjectionMatrix[0 * 4 + 2] = 0.0f;
		lightProjectionMatrix[1 * 4 + 2] = 0.0f;
		lightProjectionMatrix[2 * 4 + 2] = -0.999f; // adjust value to prevent imprecision issues
		lightProjectionMatrix[3 * 4 + 2] = -2.0f * zNear;
		
		lightProjectionMatrix[0 * 4 + 3] = 0.0f;
		lightProjectionMatrix[1 * 4 + 3] = 0.0f;
		lightProjectionMatrix[2 * 4 + 3] = -1.0f;
		lightProjectionMatrix[3 * 4 + 3] = 0.0f;
		
		idRenderMatrix::Transpose( *( idRenderMatrix* )lightProjectionMatrix, lightProjectionRenderMatrix );
		
		backEnd.shadowV[side] = lightViewRenderMatrix;
		backEnd.shadowP[side] = lightProjectionRenderMatrix;
	}
	else
	{
		lightViewRenderMatrix.Identity();
		lightProjectionRenderMatrix = vLight->baseLightProject;
		
		backEnd.shadowV[0] = lightViewRenderMatrix;
		backEnd.shadowP[0] = lightProjectionRenderMatrix;
	}
	
	
	
	globalFramebuffers.shadowFBO[vLight->shadowLOD]->Bind();
	
	if( side < 0 )
	{
		globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImageDepthLayer( globalImages->shadowImage[vLight->shadowLOD], 0 );
	}
	else
	{
		globalFramebuffers.shadowFBO[vLight->shadowLOD]->AttachImageDepthLayer( globalImages->shadowImage[vLight->shadowLOD], side );
	}
	
    int status = globalFramebuffers.shadowFBO[vLight->shadowLOD]->Check();
	globalFramebuffers.shadowFBO[vLight->shadowLOD]->Error( status ); // Koz 

	GL_ViewportAndScissor( 0, 0, shadowMapResolutions[vLight->shadowLOD], shadowMapResolutions[vLight->shadowLOD] );
	
	glClear( GL_DEPTH_BUFFER_BIT );
	
	// process the chain of shadows with the current rendering state
	backEnd.currentSpace = nullptr;
	
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != nullptr; drawSurf = drawSurf->nextOnLight )
	{
	
#if 1
		// make sure the shadow occluder geometry is done
		if( drawSurf->shadowVolumeState != SHADOWVOLUME_DONE )
		{
			assert( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED || drawSurf->shadowVolumeState == SHADOWVOLUME_DONE );
			
			uint64 start = Sys_Microseconds();
			while( drawSurf->shadowVolumeState == SHADOWVOLUME_UNFINISHED )
			{
				Sys_Yield();
			}
			uint64 end = Sys_Microseconds();
			
			backEnd.pc.shadowMicroSec += end - start;
		}
#endif
		
		if( drawSurf->numIndexes == 0 )
		{
			continue;	// a job may have created an empty shadow geometry
		}
		
		if( drawSurf->space != backEnd.currentSpace )
		{
			idRenderMatrix modelRenderMatrix;
			idRenderMatrix::Transpose( *( idRenderMatrix* )drawSurf->space->modelMatrix, modelRenderMatrix );
			
			idRenderMatrix modelToLightRenderMatrix;
			idRenderMatrix::Multiply( lightViewRenderMatrix, modelRenderMatrix, modelToLightRenderMatrix );
			
			idRenderMatrix clipMVP;
			idRenderMatrix::Multiply( lightProjectionRenderMatrix, modelToLightRenderMatrix, clipMVP );
			
			if( vLight->parallel )
			{
				idRenderMatrix MVP;
				idRenderMatrix::Multiply( renderMatrix_clipSpaceToWindowSpace, clipMVP, MVP );
				
				RB_SetMVP( clipMVP );
			}
			else if( side < 0 )
			{
				// from OpenGL view space to OpenGL NDC ( -1 : 1 in XYZ )
				idRenderMatrix MVP;
				idRenderMatrix::Multiply( renderMatrix_windowSpaceToClipSpace, clipMVP, MVP );
				
				RB_SetMVP( MVP );
			}
			else
			{
				RB_SetMVP( clipMVP );
			}
			
			// set the local light position to allow the vertex program to project the shadow volume end cap to infinity
			/*
			anVec4 localLight( 0.0f );
			R_GlobalPointToLocal( drawSurf->space->modelMatrix, vLight->globalLightOrigin, localLight.ToVec3() );
			SetVertexParm( RENDERPARM_LOCALLIGHTORIGIN, localLight.ToFloatPtr() );
			*/
			
			backEnd.currentSpace = drawSurf->space;
		}
		
		bool didDraw = false;
		
		const idMaterial* shader = drawSurf->material;
		
		// get the expressions for conditionals / color / texcoords
		const float* regs = drawSurf->shaderRegisters;
		anVec4 color( 0, 0, 0, 1 );
		
		uint64 surfGLState = 0;
		
		// set polygon offset if necessary
		if( shader && shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			surfGLState |= GLS_POLYGON_OFFSET;
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
		}
		
#if 1
		if( shader && shader->Coverage() == MC_PERFORATED )
		{
			// perforated surfaces may have multiple alpha tested stages
			for( int stage = 0; stage < shader->GetNumStages(); stage++ )
			{
				const shaderStage_t* pStage = shader->GetStage( stage );
				
				if( !pStage->hasAlphaTest )
				{
					continue;
				}
				
				// check the stage enable condition
				if( regs[ pStage->conditionRegister ] == 0 )
				{
					continue;
				}
				
				// if we at least tried to draw an alpha tested stage,
				// we won't draw the opaque surface
				didDraw = true;
				
				// set the alpha modulate
				color[3] = regs[ pStage->color.registers[3] ];
				
				// skip the entire stage if alpha would be black
				if( color[3] <= 0.0f )
				{
					continue;
				}
				
				uint64 stageGLState = surfGLState;
				
				// set privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
					stageGLState |= GLS_POLYGON_OFFSET;
				}
				
				GL_Color( color );
				
#ifdef USE_CORE_PROFILE
				GL_State( stageGLState );
				anVec4 alphaTestValue( regs[ pStage->alphaTestRegister ] );
				SetFragmentParm( RENDERPARM_ALPHA_TEST, alphaTestValue.ToFloatPtr() );
#else
				GL_State( stageGLState | GLS_ALPHATEST_FUNC_GREATER | GLS_ALPHATEST_MAKE_REF( anMath::Ftob( 255.0f * regs[ pStage->alphaTestRegister ] ) ) );
#endif
				
				if( drawSurf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}
				
				RB_SetVertexColorParms( SVC_IGNORE );
				
				// bind the texture
				GL_SelectTexture( 0 );
				pStage->texture.image->Bind();
				
				// set texture matrix and texGens
				RB_PrepareStageTexturing( pStage, drawSurf );
				
				// must render with less-equal for Z-Cull to work properly
				assert( ( GL_GetCurrentState() & GLS_DEPTHFUNC_BITS ) == GLS_DEPTHFUNC_LESS );
				
				// draw it
				RB_DrawElementsWithCounters( drawSurf );
				
				// clean up
				RB_FinishStageTexturing( pStage, drawSurf );
				
				// unset privatePolygonOffset if necessary
				if( pStage->privatePolygonOffset )
				{
					GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
				}
			}
		}
#endif
		
		if( !didDraw )
		{
			if( drawSurf->jointCache )
			{
				renderProgManager.BindShader_DepthSkinned();
			}
			else
			{
				renderProgManager.BindShader_Depth();
			}
			
			RB_DrawElementsWithCounters( drawSurf );
		}
	}
	
	// cleanup the shadow specific rendering state
	
	Framebuffer::BindDefault();
	renderProgManager.Unbind();
	
	GL_State( GLS_DEFAULT );
	GL_Cull( CT_FRONT_SIDED );
	
#ifdef USE_CORE_PROFILE
	SetFragmentParm( RENDERPARM_ALPHA_TEST, vec4_zero.ToFloatPtr() );
#endif
}

/*
==============================================================================================

DRAW INTERACTIONS

==============================================================================================
*/
/*
==================
RB_DrawInteractions
==================
*/
static void RB_DrawInteractions( const viewDef_t* viewDef )
{
	if( r_skipInteractions.GetBool() )
	{
		return;
	}
	
	renderLog.OpenMainBlock( MRB_DRAW_INTERACTIONS );
	renderLog.OpenBlock( "RB_DrawInteractions" );
	
	GL_SelectTexture( 0 );
	
	
	const bool useLightDepthBounds = r_useLightDepthBounds.GetBool() && !r_useShadowMapping.GetBool();
	
	//
	// for each light, perform shadowing and adding
	//
	for( const viewLight_t* vLight = backEnd.viewDef->viewLights; vLight != nullptr; vLight = vLight->next )
	{
		// do fogging later
		if( vLight->lightShader->IsFogLight() )
		{
			continue;
		}
		if( vLight->lightShader->IsBlendLight() )
		{
			continue;
		}
		
		if( vLight->localInteractions == nullptr && vLight->globalInteractions == nullptr && vLight->translucentInteractions == nullptr )
		{
			continue;
		}
		
		const idMaterial* lightShader = vLight->lightShader;
		renderLog.OpenBlock( lightShader->GetName() );
		
		// set the depth bounds for the whole light
		if( useLightDepthBounds )
		{
			GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		}
		
		// RB: shadow mapping
		if( r_useShadowMapping.GetBool() )
		{
			int	side, sideStop;
			
			if( vLight->parallel )
			{
				side = 0;
				sideStop = r_shadowMapSplits.GetInteger() + 1;
			}
			else if( vLight->pointLight )
			{
				if( r_shadowMapSingleSide.GetInteger() != -1 )
				{
					side = r_shadowMapSingleSide.GetInteger();
					sideStop = side + 1;
				}
				else
				{
					side = 0;
					sideStop = 6;
				}
			}
			else
			{
				side = -1;
				sideStop = 0;
			}
			
			for( ; side < sideStop ; side++ )
			{
				RB_ShadowMapPass( vLight->globalShadows, vLight, side );
			}
			
			// go back from light view to default camera view
			RB_ResetViewportAndScissorToDefaultCamera( viewDef );
			
			if( vLight->localInteractions != nullptr )
			{
				renderLog.OpenBlock( "Local Light Interactions" );
				RB_RenderInteractions( vLight->localInteractions, vLight, GLS_DEPTHFUNC_EQUAL, false, useLightDepthBounds );
				renderLog.CloseBlock();
			}
			
			if( vLight->globalInteractions != nullptr )
			{
				renderLog.OpenBlock( "Global Light Interactions" );
				RB_RenderInteractions( vLight->globalInteractions, vLight, GLS_DEPTHFUNC_EQUAL, false, useLightDepthBounds );
				renderLog.CloseBlock();
			}
		}
		else
		{
			// only need to clear the stencil buffer and perform stencil testing if there are shadows
			const bool performStencilTest = ( vLight->globalShadows != nullptr || vLight->localShadows != nullptr ) && !r_useShadowMapping.GetBool();
			
			// mirror flips the sense of the stencil select, and I don't want to risk accidentally breaking it
			// in the normal case, so simply disable the stencil select in the mirror case
			const bool useLightStencilSelect = ( r_useLightStencilSelect.GetBool() && backEnd.viewDef->isMirror == false );
			
			if( performStencilTest )
			{
				if( useLightStencilSelect )
				{
					// write a stencil mask for the visible light bounds to hi-stencil
					RB_StencilSelectLight( vLight );
				}
				else
				{
					// always clear whole S-Cull tiles
					idScreenRect rect;
					rect.x1 = ( vLight->scissorRect.x1 +  0 ) & ~15;
					rect.y1 = ( vLight->scissorRect.y1 +  0 ) & ~15;
					rect.x2 = ( vLight->scissorRect.x2 + 15 ) & ~15;
					rect.y2 = ( vLight->scissorRect.y2 + 15 ) & ~15;
					
					if( !backEnd.currentScissor.Equals( rect ) && r_useScissor.GetBool() )
					{
						GL_Scissor( backEnd.viewDef->viewport.x1 + rect.x1,
									backEnd.viewDef->viewport.y1 + rect.y1,
									rect.x2 + 1 - rect.x1,
									rect.y2 + 1 - rect.y1 );
						backEnd.currentScissor = rect;
					}
					GL_State( GLS_DEFAULT );	// make sure stencil mask passes for the clear
					GL_Clear( false, false, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f );
				}
			}
			
			if( vLight->globalShadows != nullptr )
			{
				renderLog.OpenBlock( "Global Light Shadows" );
				RB_StencilShadowPass( vLight->globalShadows, vLight );
				renderLog.CloseBlock();
			}
			
			if( vLight->localInteractions != nullptr )
			{
				renderLog.OpenBlock( "Local Light Interactions" );
				RB_RenderInteractions( vLight->localInteractions, vLight, GLS_DEPTHFUNC_EQUAL, performStencilTest, useLightDepthBounds );
				renderLog.CloseBlock();
			}
			
			if( vLight->localShadows != nullptr )
			{
				renderLog.OpenBlock( "Local Light Shadows" );
				RB_StencilShadowPass( vLight->localShadows, vLight );
				renderLog.CloseBlock();
			}
			
			if( vLight->globalInteractions != nullptr )
			{
				renderLog.OpenBlock( "Global Light Interactions" );
				RB_RenderInteractions( vLight->globalInteractions, vLight, GLS_DEPTHFUNC_EQUAL, performStencilTest, useLightDepthBounds );
				renderLog.CloseBlock();
			}
		}
		// RB end
		
		if( vLight->translucentInteractions != nullptr && !r_skipTranslucent.GetBool() )
		{
			renderLog.OpenBlock( "Translucent Interactions" );
			
			// Disable the depth bounds test because translucent surfaces don't work with
			// the depth bounds tests since they did not write depth during the depth pass.
			if( useLightDepthBounds )
			{
				GL_DepthBoundsTest( 0.0f, 0.0f );
			}
			
			// The depth buffer wasn't filled in for translucent surfaces, so they
			// can never be constrained to perforated surfaces with the depthfunc equal.
			
			// Translucent surfaces do not receive shadows. This is a case where a
			// shadow buffer solution would work but stencil shadows do not because
			// stencil shadows only affect surfaces that contribute to the view depth
			// buffer and translucent surfaces do not contribute to the view depth buffer.
			
			RB_RenderInteractions( vLight->translucentInteractions, vLight, GLS_DEPTHFUNC_LESS, false, false );
			
			renderLog.CloseBlock();
		}
		
		renderLog.CloseBlock();
	}
	
	// disable stencil shadow test
	GL_State( GLS_DEFAULT );
	
	// unbind texture units
	for( int i = 0; i < 5; i++ )
	{
		GL_SelectTexture( i );
		globalImages->BindNull();
	}
	GL_SelectTexture( 0 );
	
	// reset depth bounds
	if( useLightDepthBounds )
	{
		GL_DepthBoundsTest( 0.0f, 0.0f );
	}
	
	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

/*
=============================================================================================

NON-INTERACTION SHADER PASSES

=============================================================================================
*/

/*
=====================
RB_DrawShaderPasses

Draw non-light dependent passes

If we are rendering Guis, the drawSurf_t::sort value is a depth offset that can
be multiplied by guiEye for polarity and screenSeparation for scale.
=====================
*/
static int RB_DrawShaderPasses( const drawSurf_t* const* const drawSurfs, const int numDrawSurfs,
								float guiStereoScreenOffset, const int stereoEye )
{
	
	float guiOffset = 0.0f;
	float guiSort = 0.0f;
	
	// only obey skipAmbient if we are rendering a view
	if( backEnd.viewDef->viewEntitys && r_skipAmbient.GetBool() )
	{
		return numDrawSurfs;
	}
	
	renderLog.OpenBlock( "RB_DrawShaderPasses" );
	
	GL_SelectTexture( 1 );
	globalImages->BindNull();
	
	GL_SelectTexture( 0 );
	
	backEnd.currentSpace = ( const viewEntity_t* )1;	// using nullptr makes /analyze think surf->space needs to be checked...
	float currentGuiStereoOffset = 0.0f;
	
	int i = 0;
	for( ; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* surf = drawSurfs[i];
		const idMaterial* shader = surf->material;
		
		if( !shader->HasAmbient() )
		{
			continue;
		}
		
		if( shader->IsPortalSky() )
		{
			continue;
		}
		
		// some deforms may disable themselves by setting numIndexes = 0
		if( surf->numIndexes == 0 )
		{
			continue;
		}
		
		if( shader->SuppressInSubview() )
		{
			continue;
		}
		
		if( backEnd.viewDef->isXraySubview && surf->space->entityDef )
		{
			if( surf->space->entityDef->parms.xrayIndex != 2 )
			{
				continue;
			}
		}
		
		// we need to draw the post process shaders after we have drawn the fog lights
		if( shader->GetSort() >= SS_POST_PROCESS && !backEnd.currentRenderCopied )
		{
			break;
		}
		
		// if we are rendering a 3D view and the surface's eye index doesn't match
		// the current view's eye index then we skip the surface
		// if the stereoEye value of a surface is 0 then we need to draw it for both eyes.
		const int shaderStereoEye = shader->GetStereoEye();
		const bool isEyeValid = stereoRender_swapEyes.GetBool() ? ( shaderStereoEye == stereoEye ) : ( shaderStereoEye != stereoEye );
		if( ( stereoEye != 0 ) && ( shaderStereoEye != 0 ) && ( isEyeValid ) )
		{
			continue;
		}
		
		float sOffset[4] = { 1, 1, 1, 1 };
				
		SetVertexParm( RENDERPARM_STEREO_CORRECTION, sOffset );

		renderLog.OpenBlock( shader->GetName() );

			
		// determine the stereoDepth offset
		// guiStereoScreenOffset will always be zero for 3D views, so the !=
		// check will never force an update due to the current sort value.
		
		
		// Koz begin
		// this is more of the gross hack to add some depth to in game guis
		guiOffset = guiStereoScreenOffset;
		guiSort = surf->sort;
		
		
		if ( guiSort >= 1000 ) // 1000 added to the depth marks this as an in game gui, so a different separation value can be used 
		{
			//guiOffset = vr_gui2.GetFloat() * -stereoEye;
			guiOffset = stereoEye * .03; //.03 is the stereo separation to use on in game guis
			guiSort -= 1000; 
			
		}
		
		//const float thisGuiStereoOffset = guiStereoScreenOffset * surf->sort;
		float thisGuiStereoOffset = guiOffset * guiSort ;
		// Koz end



		// change the matrix and other space related vars if needed
		if( surf->space != backEnd.currentSpace || thisGuiStereoOffset != currentGuiStereoOffset )
		{
						
			backEnd.currentSpace = surf->space;
			currentGuiStereoOffset = thisGuiStereoOffset;
	
			const viewEntity_t* space = backEnd.currentSpace;
			
			//if( guiStereoScreenOffset != 0.0f  
			if ( guiOffset != 0.0f ) // Koz
			{
				RB_SetMVPWithStereoOffset( space->mvp, currentGuiStereoOffset );
			}
			else
			{
				RB_SetMVP( space->mvp );
			}
			
			// set eye position in local space
			anVec4 localViewOrigin( 1.0f );

			
			R_GlobalPointToLocal( space->modelMatrix, backEnd.viewDef->renderView.vieworg, localViewOrigin.ToVec3() );
						
			SetVertexParm( RENDERPARM_LOCALVIEWORIGIN, localViewOrigin.ToFloatPtr() );
			
			// set model Matrix
			float modelMatrixTranspose[16];
			R_MatrixTranspose( space->modelMatrix, modelMatrixTranspose );
			SetVertexParms( RENDERPARM_MODELMATRIX_X, modelMatrixTranspose, 4 );
			
			// Set ModelView Matrix
			float modelViewMatrixTranspose[16];
			R_MatrixTranspose( space->modelViewMatrix, modelViewMatrixTranspose );
			SetVertexParms( RENDERPARM_MODELVIEWMATRIX_X, modelViewMatrixTranspose, 4 );
		}
		
		// change the scissor if needed
		if( !backEnd.currentScissor.Equals( surf->scissorRect ) && r_useScissor.GetBool() )
		{
			GL_Scissor( backEnd.viewDef->viewport.x1 + surf->scissorRect.x1,
						backEnd.viewDef->viewport.y1 + surf->scissorRect.y1,
						surf->scissorRect.x2 + 1 - surf->scissorRect.x1,
						surf->scissorRect.y2 + 1 - surf->scissorRect.y1 );
			backEnd.currentScissor = surf->scissorRect;
		}
		
				// get the expressions for conditionals / color / texcoords
		const float*	regs = surf->shaderRegisters;
		
		// set face culling appropriately
		if( surf->space->isGuiSurface )
		{
			GL_Cull( CT_TWO_SIDED );
		}
		else
		{
			GL_Cull( shader->GetCullType() );
		}
		
		uint64 surfGLState = surf->extraGLState;
		
		// set polygon offset if necessary
		if( shader->TestMaterialFlag( MF_POLYGONOFFSET ) )
		{
			GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
			surfGLState = GLS_POLYGON_OFFSET;
		}
		
		for( int stage = 0; stage < shader->GetNumStages(); stage++ )
		{
			const shaderStage_t* pStage = shader->GetStage( stage );
			
			// check the enable condition
			if( regs[ pStage->conditionRegister ] == 0 )
			{
				continue;
			}
			
			// skip the stages involved in lighting
			if( pStage->lighting != SL_AMBIENT )
			{
				continue;
			}
			
			uint64 stageGLState = surfGLState;
			if( ( surfGLState & GLS_OVERRIDE ) == 0 )
			{
				stageGLState |= pStage->drawStateBits;
			}
			
			// skip if the stage is ( GL_ZERO, GL_ONE ), which is used for some alpha masks
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE ) )
			{
				continue;
			}
			
			
			// see if we are a new-style stage
			newShaderStage_t* newStage = pStage->newStage;
			if( newStage != nullptr )
			{
				//--------------------------
				//
				// new style stages
				//
				//--------------------------
				if( r_skipNewAmbient.GetBool() )
				{
					continue;
				}
				renderLog.OpenBlock( "New Shader Stage" );
				
				GL_State( stageGLState );
				
				// RB: CRITICAL BUGFIX: changed newStage->glslProgram to vertexProgram and fragmentProgram
				// otherwise it will result in an out of bounds crash in RB_DrawElementsWithCounters
				renderProgManager.BindShader( newStage->glslProgram, newStage->vertexProgram, newStage->fragmentProgram, false );
				// RB end
				

				for( int j = 0; j < newStage->numVertexParms; j++ )
				{
					float parm[4];
					parm[0] = regs[ newStage->vertexParms[j][0] ];
					parm[1] = regs[ newStage->vertexParms[j][1] ];
					parm[2] = regs[ newStage->vertexParms[j][2] ];
					parm[3] = regs[ newStage->vertexParms[j][3] ];
					SetVertexParm( ( renderParm_t )( RENDERPARM_USER + j ), parm );
				}
				
				// set rpEnableSkinning if the shader has optional support for skinning
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					const anVec4 skinningParm( 1.0f );
					SetVertexParm( RENDERPARM_ENABLE_SKINNING, skinningParm.ToFloatPtr() );
				}
				
				// bind texture units
				for( int j = 0; j < newStage->numFragmentProgramImages; j++ )
				{
					anImage* image = newStage->fragmentProgramImages[j];
					if( image != nullptr )
					{
						GL_SelectTexture( j );
						image->Bind();
					}
				}
				
				// draw it
				RB_DrawElementsWithCounters( surf );
				
				// unbind texture units
				for( int j = 0; j < newStage->numFragmentProgramImages; j++ )
				{
					anImage* image = newStage->fragmentProgramImages[j];
					if( image != nullptr )
					{
						GL_SelectTexture( j );
						globalImages->BindNull();
					}
				}
				
				// clear rpEnableSkinning if it was set
				if( surf->jointCache && renderProgManager.ShaderHasOptionalSkinning() )
				{
					const anVec4 skinningParm( 0.0f );
					SetVertexParm( RENDERPARM_ENABLE_SKINNING, skinningParm.ToFloatPtr() );
				}
				
				GL_SelectTexture( 0 );
				renderProgManager.Unbind();
				
				renderLog.CloseBlock();
				continue;
			}
			
			//--------------------------
			//
			// old style stages
			//
			//--------------------------
			
			// set the color
			anVec4 color;
			color[0] = regs[ pStage->color.registers[0] ];
			color[1] = regs[ pStage->color.registers[1] ];
			color[2] = regs[ pStage->color.registers[2] ];
			color[3] = regs[ pStage->color.registers[3] ];
			
			// skip the entire stage if an add would be black
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE )
					&& color[0] <= 0 && color[1] <= 0 && color[2] <= 0 )
			{
				continue;
			}
			
			// skip the entire stage if a blend would be completely transparent
			if( ( stageGLState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) == ( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA )
					&& color[3] <= 0 )
			{
				continue;
			}
			
			stageVertexColor_t svc = pStage->vertexColor;
			
			renderLog.OpenBlock( "Old Shader Stage" );
			GL_Color( color );
			
			if( surf->space->isGuiSurface )
			{
				// Force gui surfaces to always be SVC_MODULATE
				svc = SVC_MODULATE;
				
				// use special shaders for bink cinematics
				if( pStage->texture.cinematic )
				{
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renederProg
						renderProgManager.BindShader_BinkGUI();
					}
					else
					{
						renderProgManager.BindShader_Bink();
					}
				}
				else
				{
					if( ( stageGLState & GLS_OVERRIDE ) != 0 )
					{
						// This is a hack... Only SWF Guis set GLS_OVERRIDE
						// Old style guis do not, and we don't want them to use the new GUI renderProg
						renderProgManager.BindShader_GUI();
					}
					else
					{
						if( surf->jointCache )
						{
							renderProgManager.BindShader_TextureVertexColorSkinned();
						}
						else
						{
							renderProgManager.BindShader_TextureVertexColor();
						}
					}
				}
			}
			else if( ( pStage->texture.texgen == TG_SCREEN ) || ( pStage->texture.texgen == TG_SCREEN2 ) )
			{
				renderProgManager.BindShader_TextureTexGenVertexColor();
			}
			else if( pStage->texture.cinematic )
			{
				renderProgManager.BindShader_Bink();
			}
			else
			{
				if( surf->jointCache )
				{
					renderProgManager.BindShader_TextureVertexColorSkinned();
				}
				else
				{
					renderProgManager.BindShader_TextureVertexColor();
				}
			}
			
			RB_SetVertexColorParms( svc );
			
			// bind the texture
			RB_BindVariableStageImage( &pStage->texture, regs );
			
			// set privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * pStage->privatePolygonOffset );
				stageGLState |= GLS_POLYGON_OFFSET;
			}
			
			// set the state
			GL_State( stageGLState );
			
			RB_PrepareStageTexturing( pStage, surf );
			
			// draw it
			RB_DrawElementsWithCounters( surf );
			
			RB_FinishStageTexturing( pStage, surf );
			
			// unset privatePolygonOffset if necessary
			if( pStage->privatePolygonOffset )
			{
				GL_PolygonOffset( r_offsetFactor.GetFloat(), r_offsetUnits.GetFloat() * shader->GetPolygonOffset() );
			}
			renderLog.CloseBlock();
		}
		
		renderLog.CloseBlock();
	}
	
	GL_Cull( CT_FRONT_SIDED );
	GL_Color( 1.0f, 1.0f, 1.0f );
	
	renderLog.CloseBlock();
	return i;
}

/*
=============================================================================================

BLEND LIGHT PROJECTION

=============================================================================================
*/

/*
=====================
RB_T_BlendLight
=====================
*/
static void RB_T_BlendLight( const drawSurf_t* drawSurfs, const viewLight_t* vLight ) {
	backEnd.currentSpace = nullptr;
	
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != nullptr; drawSurf = drawSurf->nextOnLight ) {
		if( drawSurf->scissorRect.IsEmpty() ) {
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}
		
		if( !backEnd.currentScissor.Equals( drawSurf->scissorRect ) && r_useScissor.GetBool() ) {
			// change the scissor
			GL_Scissor( backEnd.viewDef->viewport.x1 + drawSurf->scissorRect.x1,
						backEnd.viewDef->viewport.y1 + drawSurf->scissorRect.y1,
						drawSurf->scissorRect.x2 + 1 - drawSurf->scissorRect.x1,
						drawSurf->scissorRect.y2 + 1 - drawSurf->scissorRect.y1 );
			backEnd.currentScissor = drawSurf->scissorRect;
		}
		
		if( drawSurf->space != backEnd.currentSpace ) {
			// change the matrix
			RB_SetMVP( drawSurf->space->mvp );
			// change the light projection matrix
			anPlane	lightProjectInCurrentSpace[4];
			for( int i = 0; i < 4; i++ ) {
				R_GlobalPlaneToLocal( drawSurf->space->modelMatrix, vLight->lightProject[i], lightProjectInCurrentSpace[i] );
			}
			
			SetVertexParm( RENDERPARM_TEXGEN_0_S, lightProjectInCurrentSpace[0].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_T, lightProjectInCurrentSpace[1].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_Q, lightProjectInCurrentSpace[2].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_S, lightProjectInCurrentSpace[3].ToFloatPtr() );	// falloff
			
			backEnd.currentSpace = drawSurf->space;
		}
		
		RB_DrawElementsWithCounters( drawSurf );
	}
}

/*
=====================
RB_BlendLight

Dual texture together the falloff and projection texture with a blend
mode to the framebuffer, instead of interacting with the surface texture
=====================
*/
static void RB_BlendLight( const drawSurf_t* drawSurfs, const drawSurf_t* drawSurfs2, const viewLight_t* vLight )
{
	if( drawSurfs == nullptr )
	{
		return;
	}
	if( r_skipBlendLights.GetBool() )
	{
		return;
	}
	renderLog.OpenBlock( vLight->lightShader->GetName() );
	
	const idMaterial* lightShader = vLight->lightShader;
	const float*	 regs = vLight->shaderRegisters;
	
	// texture 1 will get the falloff texture
	GL_SelectTexture( 1 );
	vLight->falloffImage->Bind();
	
	// texture 0 will get the projected texture
	GL_SelectTexture( 0 );
	
	renderProgManager.BindShader_BlendLight();
	
	for( int i = 0; i < lightShader->GetNumStages(); i++ )
	{
		const shaderStage_t*	stage = lightShader->GetStage( i );
		
		if( !regs[ stage->conditionRegister ] )
		{
			continue;
		}
		
		GL_State( GLS_DEPTHMASK | stage->drawStateBits | GLS_DEPTHFUNC_EQUAL );
		
		GL_SelectTexture( 0 );
		stage->texture.image->Bind();
		
		if( stage->texture.hasMatrix )
		{
			RB_LoadShaderTextureMatrix( regs, &stage->texture );
		}
		
		// get the modulate values from the light, including alpha, unlike normal lights
		anVec4 lightColor;
		lightColor[0] = regs[ stage->color.registers[0] ];
		lightColor[1] = regs[ stage->color.registers[1] ];
		lightColor[2] = regs[ stage->color.registers[2] ];
		lightColor[3] = regs[ stage->color.registers[3] ];
		GL_Color( lightColor );
		
		RB_T_BlendLight( drawSurfs, vLight );
		RB_T_BlendLight( drawSurfs2, vLight );
	}
	
	GL_SelectTexture( 1 );
	globalImages->BindNull();
	
	GL_SelectTexture( 0 );
	
	renderProgManager.Unbind();
	renderLog.CloseBlock();
}

/*
=========================================================================================================

FOG LIGHTS

=========================================================================================================
*/

/*
=====================
RB_T_BasicFog
=====================
*/
static void RB_T_BasicFog( const drawSurf_t* drawSurfs, const idPlane fogPlanes[4], const idRenderMatrix* inverseBaseLightProject )
{
	backEnd.currentSpace = nullptr;
	
	for( const drawSurf_t* drawSurf = drawSurfs; drawSurf != nullptr; drawSurf = drawSurf->nextOnLight )
	{
		if( drawSurf->scissorRect.IsEmpty() )
		{
			continue;	// !@# FIXME: find out why this is sometimes being hit!
			// temporarily jump over the scissor and draw so the gl error callback doesn't get hit
		}
		
		if( !backEnd.currentScissor.Equals( drawSurf->scissorRect ) && r_useScissor.GetBool() )
		{
			// change the scissor
			GL_Scissor( backEnd.viewDef->viewport.x1 + drawSurf->scissorRect.x1,
						backEnd.viewDef->viewport.y1 + drawSurf->scissorRect.y1,
						drawSurf->scissorRect.x2 + 1 - drawSurf->scissorRect.x1,
						drawSurf->scissorRect.y2 + 1 - drawSurf->scissorRect.y1 );
			backEnd.currentScissor = drawSurf->scissorRect;
		}
		
		if( drawSurf->space != backEnd.currentSpace )
		{
			idPlane localFogPlanes[4];
			if( inverseBaseLightProject == nullptr )
			{
				RB_SetMVP( drawSurf->space->mvp );
				for( int i = 0; i < 4; i++ )
				{
					R_GlobalPlaneToLocal( drawSurf->space->modelMatrix, fogPlanes[i], localFogPlanes[i] );
				}
			}
			else
			{
				idRenderMatrix invProjectMVPMatrix;
				idRenderMatrix::Multiply( backEnd.viewDef->worldSpace.mvp, *inverseBaseLightProject, invProjectMVPMatrix );
				RB_SetMVP( invProjectMVPMatrix );
				for( int i = 0; i < 4; i++ )
				{
					inverseBaseLightProject->InverseTransformPlane( fogPlanes[i], localFogPlanes[i], false );
				}
			}
			
			SetVertexParm( RENDERPARM_TEXGEN_0_S, localFogPlanes[0].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_0_T, localFogPlanes[1].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_T, localFogPlanes[2].ToFloatPtr() );
			SetVertexParm( RENDERPARM_TEXGEN_1_S, localFogPlanes[3].ToFloatPtr() );
			
			backEnd.currentSpace = ( inverseBaseLightProject == nullptr ) ? drawSurf->space : nullptr;
		}
		
		if( drawSurf->jointCache )
		{
			renderProgManager.BindShader_FogSkinned();
		}
		else
		{
			renderProgManager.BindShader_Fog();
		}
		
		RB_DrawElementsWithCounters( drawSurf );
	}
}

/*
==================
RB_FogPass
==================
*/
static void RB_FogPass( const drawSurf_t* drawSurfs,  const drawSurf_t* drawSurfs2, const viewLight_t* vLight ) {
	renderLog.OpenBlock( vLight->lightShader->GetName() );
	
	// find the current color and density of the fog
	const idMaterial* lightShader = vLight->lightShader;
	const float* regs = vLight->shaderRegisters;
	// assume fog shaders have only a single stage
	const shaderStage_t* stage = lightShader->GetStage( 0 );
	
	anVec4 lightColor;
	lightColor[0] = regs[ stage->color.registers[0] ];
	lightColor[1] = regs[ stage->color.registers[1] ];
	lightColor[2] = regs[ stage->color.registers[2] ];
	lightColor[3] = regs[ stage->color.registers[3] ];
	
	GL_Color( lightColor );
	
	// calculate the falloff planes
	float a;
	
	// if they left the default value on, set a fog distance of 500
	if( lightColor[3] <= 1.0f )
	{
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	}
	else
	{
		// otherwise, distance = alpha color
		a = -0.5f / lightColor[3];
	}
	
	// texture 0 is the falloff image
	GL_SelectTexture( 0 );
	globalImages->fogImage->Bind();
	
	// texture 1 is the entering plane fade correction
	GL_SelectTexture( 1 );
	globalImages->fogEnterImage->Bind();
	
	// S is based on the view origin
	const float s = vLight->fogPlane.Distance( backEnd.viewDef->renderView.vieworg );
	
	const float FOG_SCALE = 0.001f;
	
	idPlane fogPlanes[4];
	
	// S-0
	fogPlanes[0][0] = a * backEnd.viewDef->worldSpace.modelViewMatrix[0 * 4 + 2];
	fogPlanes[0][1] = a * backEnd.viewDef->worldSpace.modelViewMatrix[1 * 4 + 2];
	fogPlanes[0][2] = a * backEnd.viewDef->worldSpace.modelViewMatrix[2 * 4 + 2];
	fogPlanes[0][3] = a * backEnd.viewDef->worldSpace.modelViewMatrix[3 * 4 + 2] + 0.5f;
	
	// T-0
	fogPlanes[1][0] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[0*4+0];
	fogPlanes[1][1] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[1*4+0];
	fogPlanes[1][2] = 0.0f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[2*4+0];
	fogPlanes[1][3] = 0.5f;//a * backEnd.viewDef->worldSpace.modelViewMatrix[3*4+0] + 0.5f;
	
	// T-1 will get a texgen for the fade plane, which is always the "top" plane on unrotated lights
	fogPlanes[2][0] = FOG_SCALE * vLight->fogPlane[0];
	fogPlanes[2][1] = FOG_SCALE * vLight->fogPlane[1];
	fogPlanes[2][2] = FOG_SCALE * vLight->fogPlane[2];
	fogPlanes[2][3] = FOG_SCALE * vLight->fogPlane[3] + FOG_ENTER;
	
	// S-1
	fogPlanes[3][0] = 0.0f;
	fogPlanes[3][1] = 0.0f;
	fogPlanes[3][2] = 0.0f;
	fogPlanes[3][3] = FOG_SCALE * s + FOG_ENTER;
	
	// draw it
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	RB_T_BasicFog( drawSurfs, fogPlanes, nullptr );
	RB_T_BasicFog( drawSurfs2, fogPlanes, nullptr );
	
	// the light frustum bounding planes aren't in the depth buffer, so use depthfunc_less instead
	// of depthfunc_equal
	GL_State( GLS_DEPTHMASK | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_LESS );
	GL_Cull( CT_BACK_SIDED );
	
	backEnd.zeroOneCubeSurface.space = &backEnd.viewDef->worldSpace;
	backEnd.zeroOneCubeSurface.scissorRect = backEnd.viewDef->scissor;
	RB_T_BasicFog( &backEnd.zeroOneCubeSurface, fogPlanes, &vLight->inverseBaseLightProject );
	
	GL_Cull( CT_FRONT_SIDED );
	
	GL_SelectTexture( 1 );
	globalImages->BindNull();
	
	GL_SelectTexture( 0 );
	
	renderProgManager.Unbind();
	
	renderLog.CloseBlock();
}

/*
==================
RB_FogAllLights
==================
*/
static void RB_FogAllLights() {
	if ( r_skipFogLights.GetBool() || r_showOverDraw.GetInteger() != 0
			|| backEnd.viewDef->isXraySubview /* don't fog in xray mode*/ ) {
		return;
	}
	renderLog.OpenMainBlock( MRB_FOG_ALL_LIGHTS );
	renderLog.OpenBlock( "RB_FogAllLights" );
	
	// force fog plane to recalculate
	backEnd.currentSpace = nullptr;
	
	for ( viewLight_t* vLight = backEnd.viewDef->viewLights; vLight != nullptr; vLight = vLight->next ) {
		if ( vLight->lightShader->IsFogLight() ) {
			RB_FogPass( vLight->globalInteractions, vLight->localInteractions, vLight );
		} else if ( vLight->lightShader->IsBlendLight() ) {
			RB_BlendLight( vLight->globalInteractions, vLight->localInteractions, vLight );
		}
	}

	renderLog.CloseBlock();
	renderLog.CloseMainBlock();
}

/*
=========================================================================================================

BACKEND COMMANDS

=========================================================================================================
*/

/*
==================
RB_DrawViewInternal
==================
*/
void RB_DrawViewInternal( const viewDef_t* viewDef, const int stereoEye ) {
	renderLog.OpenBlock( "RB_DrawViewInternal" );

	//-------------------------------------------------
	// guis can wind up referencing purged images that need to be loaded.
	// this used to be in the gui emit code, but now that it can be running
	// in a separate thread, it must not try to load images, so do it here.
	//-------------------------------------------------
	drawSurf_t** drawSurfs = ( drawSurf_t** )&viewDef->drawSurfs[0];
	const int numDrawSurfs = viewDef->numDrawSurfs;
	
	for( int i = 0; i < numDrawSurfs; i++ )
	{
		const drawSurf_t* ds = viewDef->drawSurfs[i];
		if( ds->material != nullptr )
		{
			const_cast<idMaterial*>( ds->material )->EnsureNotPurged();
		}
	}
	
	//-------------------------------------------------
	// RB_BeginDrawingView
	//
	// Any mirrored or portaled views have already been drawn, so prepare
	// to actually render the visible surfaces for this view
	//
	// clear the z buffer, set the projection matrix, etc
	//-------------------------------------------------
	RB_ResetViewportAndScissorToDefaultCamera( viewDef );
	
	backEnd.glState.faceCulling = -1;		// force face culling to set next time
	
	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// Clear the depth buffer and clear the stencil to 128 for stencil shadows as well as gui masking
	GL_Clear( false, true, true, STENCIL_SHADOW_TEST_VALUE, 0.0f, 0.0f, 0.0f, 0.0f );

	// normal face culling
	GL_Cull( CT_FRONT_SIDED );

#if defined(USE_CORE_PROFILE) && !defined(USE_GLES2) && !defined(USE_GLES3)
	// bind one global Vertex Array Object (VAO)
	glBindVertexArray( glConfig.global_vao );
#endif

	//------------------------------------
	// sets variables that can be used by all programs
	//------------------------------------
	{
		//
		// set eye position in global space
		//
		float parm[4];
		parm[0] = backEnd.viewDef->renderView.vieworg[0];
		parm[1] = backEnd.viewDef->renderView.vieworg[1];
		parm[2] = backEnd.viewDef->renderView.vieworg[2];
		parm[3] = 1.0f;
		
		SetVertexParm( RENDERPARM_GLOBALEYEPOS, parm ); // rpGlobalEyePos
		
		// sets overbright to make world brighter
		// This value is baked into the specularScale and diffuseScale values so
		// the interaction programs don't need to perform the extra multiply,
		// but any other renderprogs that want to obey the brightness value
		// can reference this.
		float overbright = r_lightScale.GetFloat() * 0.5f;
		parm[0] = overbright;
		parm[1] = overbright;
		parm[2] = overbright;
		parm[3] = overbright;
		SetFragmentParm( RENDERPARM_OVERBRIGHT, parm );
		
		// Set Projection Matrix
		float projMatrixTranspose[16];
		R_MatrixTranspose( backEnd.viewDef->projectionMatrix, projMatrixTranspose );
		SetVertexParms( RENDERPARM_PROJMATRIX_X, projMatrixTranspose, 4 );
	}
	
	//-------------------------------------------------
	// fill the depth buffer and clear color buffer to black except on subviews
	//-------------------------------------------------
	RB_FillDepthBufferFast( drawSurfs, numDrawSurfs );
	
	//-------------------------------------------------
	// main light renderer
	//-------------------------------------------------
	RB_DrawInteractions( viewDef );
	
	//-------------------------------------------------
	// now draw any non-light dependent shading passes
	//-------------------------------------------------
	int processed = 0;
	if( !r_skipShaderPasses.GetBool() )
	{
		renderLog.OpenMainBlock( MRB_DRAW_SHADER_PASSES );
		float guiScreenOffset;
		if (  viewDef->viewEntitys != nullptr )
		{
			// guiScreenOffset will be 0 in non-gui views
			guiScreenOffset = 0.0f;
		}
		else
		{
			// Koz fixme guiScreenOffset = stereoEye * viewDef->renderView.stereoScreenSeparation;
			
			extern idCVar vr_guiSeparation;
			guiScreenOffset = stereoEye * vr_guiSeparation.GetFloat(); //viewDef->renderView.stereoScreenSeparation;

		}
		processed = RB_DrawShaderPasses( drawSurfs, numDrawSurfs, guiScreenOffset, stereoEye );
		renderLog.CloseMainBlock();
	}
	
	//-------------------------------------------------
	// fog and blend lights, drawn after emissive surfaces
	// so they are properly dimmed down
	//-------------------------------------------------
	RB_FogAllLights();
	
	//-------------------------------------------------
	// capture the depth for the motion blur before rendering any post process surfaces that may contribute to the depth
	//-------------------------------------------------
	if( r_motionBlur.GetInteger() > 0 )
	{
		const idScreenRect& viewport = backEnd.viewDef->viewport;
		globalImages->currentDepthImage->CopyDepthbuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	}
	
	//-------------------------------------------------
	// now draw any screen warping post-process effects using _currentRender
	//-------------------------------------------------
	if( processed < numDrawSurfs && !r_skipPostProcess.GetBool() )
	{
		
		int x = backEnd.viewDef->viewport.x1;
		int y = backEnd.viewDef->viewport.y1;
		int	w = backEnd.viewDef->viewport.x2 - backEnd.viewDef->viewport.x1 + 1;
		int	h = backEnd.viewDef->viewport.y2 - backEnd.viewDef->viewport.y1 + 1;
		
		RENDERLOG_PRINTF( "Resolve to %i x %i buffer\n", w, h );
		GL_SelectTexture( 0 );

		globalImages->currentRenderImage->CopyFramebuffer( x, y, w, h );
		backEnd.currentRenderCopied = true;
				

		// RENDERPARM_SCREENCORRECTIONFACTOR amd RENDERPARM_WINDOWCOORD overlap
		// diffuseScale and specularScale
		
		// screen power of two correction factor (no longer relevant now)
		float screenCorrectionParm[4];
		screenCorrectionParm[0] = 1.0f;
		screenCorrectionParm[1] = 1.0f;
		screenCorrectionParm[2] = 0.0f;
		screenCorrectionParm[3] = 1.0f;
		SetFragmentParm( RENDERPARM_SCREENCORRECTIONFACTOR, screenCorrectionParm ); // rpScreenCorrectionFactor
		
		// window coord to 0.0 to 1.0 conversion
		float windowCoordParm[4];
		windowCoordParm[0] = 1.0f / w;
		windowCoordParm[1] = 1.0f / h;
		windowCoordParm[2] = 0.0f;
		windowCoordParm[3] = 1.0f;
		SetFragmentParm( RENDERPARM_WINDOWCOORD, windowCoordParm ); // rpWindowCoord
		
		// render the remaining surfaces
		renderLog.OpenMainBlock( MRB_DRAW_SHADER_PASSES_POST );
		RB_DrawShaderPasses( drawSurfs + processed, numDrawSurfs - processed, 0.0f /* definitely not a gui */, stereoEye );
		renderLog.CloseMainBlock();
			
	}
	
	//-------------------------------------------------
	// render debug tools
	//-------------------------------------------------
	RB_RenderDebugTools( drawSurfs, numDrawSurfs );
	
	renderLog.CloseBlock();
}

/*
==================
RB_MotionBlur

Experimental feature
==================
*/
void RB_MotionBlur() {
	if( !backEnd.viewDef->viewEntitys ) {
		// 3D views only
		return;
	}
	if( r_motionBlur.GetInteger() <= 0 ) {
		return;
	}
	if( backEnd.viewDef->isSubview ) {
		return;
	}
	
	// Koz GL_CheckErrors();
	
	// clear the alpha buffer and draw only the hands + weapon into it so
	// we can avoid blurring them
	glClearColor( 0, 0, 0, 1 );
	GL_State( GLS_COLORMASK | GLS_DEPTHMASK );
	glClear( GL_COLOR_BUFFER_BIT );
	GL_Color( 0, 0, 0, 0 );
	GL_SelectTexture( 0 );
	globalImages->blackImage->Bind();
	backEnd.currentSpace = nullptr;
	
	drawSurf_t** drawSurfs = ( drawSurf_t** )&backEnd.viewDef->drawSurfs[0];
	for ( int surfNum = 0; surfNum < backEnd.viewDef->numDrawSurfs; surfNum++ ) {
		const drawSurf_t* surf = drawSurfs[ surfNum ];
		if( !surf->space->weaponDepthHack && !surf->space->skipMotionBlur && !surf->material->HasSubview() ) {
			// Apply motion blur to this object
			continue;
		}
		
		const idMaterial* shader = surf->material;
		if( shader->Coverage() == MC_TRANSLUCENT ) {
			// muzzle flash, etc
			continue;
		}

		// set mvp matrix
		if( surf->space != backEnd.currentSpace )
		{
			RB_SetMVP( surf->space->mvp );
			backEnd.currentSpace = surf->space;
		}

		// this could just be a color, but we don't have a skinned color-only prog
		if( surf->jointCache )
		{
			renderProgManager.BindShader_TextureVertexColorSkinned();
		}
		else
		{
			renderProgManager.BindShader_TextureVertexColor();
		}
		
		// draw it solid
		RB_DrawElementsWithCounters( surf );
	}
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	
	// copy off the color buffer and the depth buffer for the motion blur prog
	// we use the viewport dimensions for copying the buffers in case resolution scaling is enabled.
	const idScreenRect& viewport = backEnd.viewDef->viewport;
	globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	
	// in stereo rendering, each eye needs to get a separate previous frame mvp
	int mvpIndex = ( backEnd.viewDef->renderView.viewEyeBuffer == 1 ) ? 1 : 0;
	
	// derive the matrix to go from current pixels to previous frame pixels
	idRenderMatrix	inverseMVP;
	idRenderMatrix::Inverse( backEnd.viewDef->worldSpace.mvp, inverseMVP );
	
	idRenderMatrix	motionMatrix;
	idRenderMatrix::Multiply( backEnd.prevMVP[mvpIndex], inverseMVP, motionMatrix );
	
	backEnd.prevMVP[mvpIndex] = backEnd.viewDef->worldSpace.mvp;
	
	RB_SetMVP( motionMatrix );
	
	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	renderProgManager.BindShader_MotionBlur();
	
	// let the fragment program know how many samples we are going to use
	anVec4 samples( ( float )( 1 << r_motionBlur.GetInteger() ) );
	SetFragmentParm( RENDERPARM_OVERBRIGHT, samples.ToFloatPtr() );
	
	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();
	GL_SelectTexture( 1 );
	globalImages->currentDepthImage->Bind();
	
	RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	// Koz GL_CheckErrors();
}

/*
==================
RB_DrawView

StereoEye will always be 0 in mono modes, or -1 / 1 in stereo modes.
If the view is a GUI view that is repeated for both eyes, the viewDef.stereoEye value
is 0, so the stereoEye parameter is not always the same as that.
==================
*/
void RB_DrawView( const void* data, const int stereoEye ) {
	const drawSurfsCommand_t* cmd = ( const drawSurfsCommand_t* )data;
	
	backEnd.viewDef = cmd->viewDef;
	
	// we will need to do a new copyTexSubImage of the screen
	// when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;
	
	// if there aren't any drawsurfs, do nothing
	if( !backEnd.viewDef->numDrawSurfs ) {
		return;
	}
	
	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if( r_skipRender.GetBool() && backEnd.viewDef->viewEntitys ) {
		return;
	}
	
	// skip render context sets the wgl context to nullptr,
	// which should factor out the API cost, under the assumption
	// that all gl calls just return if the context isn't valid
	
	// RB: not really needed
	//if( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys )
	//{
	//	GLimp_DeactivateContext();
	//}
	// RB end
	
	backEnd.pc.c_surfaces += backEnd.viewDef->numDrawSurfs;
	
	RB_ShowOverdraw();
	

	
	// Koz vr right before the view is drawn, update the view with the latest pos/angles from the hmd
	//Thanks to Leyland for idea & implementation
	
	if ( game->isVR && ( !cmd->viewDef->isSubview || cmd->viewDef->isMirror ) && !( gameLocal.inCinematic && vr_cinematics.GetInteger() == 2 ) ) // dont fix up if we are projecting the cinematic into space or if this is a subview ( security cam etc. )
	{
			
		static anVec3 hmdPosDelta = vec3_zero;
		static idMat3 hmdAxisDelta = mat3_identity;
		static float ipdOffset = 0.0f;

		anVec3 &drawViewOrigin = cmd->viewDef->renderView.vieworg;
		idMat3 &drawViewAxis = cmd->viewDef->renderView.viewaxis;
		
		hmdPosDelta = ( commonVr->poseHmdAbsolutePosition - commonVr->poseLastHmdAbsolutePosition ) ; // the delta in hmd position since the frame was initialy created and now
		hmdAxisDelta = commonVr->poseHmdAngles.ToMat3() * commonVr->poseLastHmdAngles.ToMat3().Inverse();// the delta in hmd rotation since the frame was initialy created and now
			
		ipdOffset = stereoEye * -commonVr->singleEyeIPD; // adjust origin from the stereoeye position, not the center eye position

		drawViewOrigin -= ipdOffset * drawViewAxis[1];
		drawViewOrigin += hmdPosDelta;

		drawViewAxis = hmdAxisDelta * drawViewAxis;

		drawViewOrigin += ipdOffset * drawViewAxis[1];

		R_SetupViewMatrix( cmd->viewDef );
		idRenderMatrix drawViewRenderMatrix;
		idRenderMatrix::Transpose( *(idRenderMatrix*)cmd->viewDef->projectionMatrix, cmd->viewDef->projectionRenderMatrix );
		idRenderMatrix::Transpose( *(idRenderMatrix*)cmd->viewDef->worldSpace.modelViewMatrix, drawViewRenderMatrix );
		idRenderMatrix::Multiply( cmd->viewDef->projectionRenderMatrix, drawViewRenderMatrix, cmd->viewDef->worldSpace.mvp );


		//model fixup
		viewEntity_t *vEntity = cmd->viewDef->viewEntitys;
		while ( vEntity )
		//for ( viewEntity_t * vEntity = cmd->viewDef->viewEntitys; vEntity; vEntity = vEntity->next )
		{
			
			// Koz from tr_frontend_addmodels
			R_MatrixMultiply(vEntity->modelMatrix, cmd->viewDef->worldSpace.modelViewMatrix,vEntity->modelViewMatrix );
						
			idRenderMatrix viewMat;
			idRenderMatrix::Transpose( *(idRenderMatrix*) vEntity->modelViewMatrix, viewMat );
			idRenderMatrix::Multiply( cmd->viewDef->projectionRenderMatrix, viewMat,vEntity->mvp );
			if (vEntity->weaponDepthHack )
			{
				idRenderMatrix::ApplyDepthHack(vEntity->mvp );
			}
			if (vEntity->modelDepthHack != 0.0f )
			{
				idRenderMatrix::ApplyModelDepthHack( vEntity->mvp, vEntity->modelDepthHack );
			}
			vEntity = vEntity->next;
		}
	}
	
	commonVr->privateCamera = false;
	
	// Koz end

	// render the scene
	RB_DrawViewInternal( cmd->viewDef, stereoEye );
	
	RB_MotionBlur();
	
	// restore the context for 2D drawing if we were stubbing it out
	// RB: not really needed
	//if( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys )
	//{
	//	GLimp_ActivateContext();
	//	GL_SetDefaultState();
	//}
	// RB end
	
	// optionally draw a box colored based on the eye number
	if( r_drawEyeColor.GetBool() ) {
		const idScreenRect& r = backEnd.viewDef->viewport;
		GL_Scissor( ( r.x1 + r.x2 ) / 2, ( r.y1 + r.y2 ) / 2, 32, 32 );
		switch ( stereoEye ) {
			case -1:
				GL_Clear( true, false, false, 0, 1.0f, 0.0f, 0.0f, 1.0f );
				break;
			case 1:
				GL_Clear( true, false, false, 0, 0.0f, 1.0f, 0.0f, 1.0f );
				break;
			default:
				GL_Clear( true, false, false, 0, 0.5f, 0.5f, 0.5f, 1.0f );
				break;
		}
	}
}

/*
==================
RB_CopyRender

Copy part of the current framebuffer to an image
==================
*/
void RB_CopyRender( const void* data ) {
	const copyRenderCommand_t* cmd = ( const copyRenderCommand_t* )data;
	
	if( r_skipCopyTexture.GetBool() ) {
		return;
	}
	
	RENDERLOG_PRINTF( "***************** RB_CopyRender *****************\n" );
	
	if ( cmd->image ) {
		cmd->image->CopyFramebuffer( cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight );
	}
	
	if ( cmd->clearColorAfterCopy ) {
		GL_Clear( true, false, false, STENCIL_SHADOW_TEST_VALUE, 0, 0, 0, 0 );
	}
}

/*
==================
RB_PostProcess

==================
*/
extern idCVar rs_enable;
void RB_PostProcess( const void* data ) {
	// only do the post process step if resolution scaling is enabled. Prevents the unnecessary copying of the framebuffer and
	// corresponding full screen quad pass.
	if ( rs_enable.GetInteger() == 0 ) {
		return;
	}
	
	// resolve the scaled rendering to a temporary texture
	postProcessCommand_t* cmd = ( postProcessCommand_t* )data;
	const idScreenRect& viewport = cmd->viewDef->viewport;
	globalImages->currentRenderImage->CopyFramebuffer( viewport.x1, viewport.y1, viewport.GetWidth(), viewport.GetHeight() );
	
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHMASK | GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );
	
	int screenWidth = renderSystem->GetWidth();
	int screenHeight = renderSystem->GetHeight();
	
	// set the window clipping
	GL_Viewport( 0, 0, screenWidth, screenHeight );
	qglScissor( 0, 0, screenWidth, screenHeight );
	
	GL_SelectTexture( 0 );
	globalImages->currentRenderImage->Bind();

	// Draw
	RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
	
	renderLog.CloseBlock();
}
