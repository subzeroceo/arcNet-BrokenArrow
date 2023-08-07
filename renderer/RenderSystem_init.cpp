#include "../idlib/Lib.h"
#include <X11/Xlib.h>
#pragma hdrstop

#include "tr_local.h"

// Vista OpenGL wrapper check
#ifdef _WIN32
#include "../sys/win32/win_local.h"
#endif

// functions that are not called every frame

qglConfig_t	qglConfig;

static void GfxInfo_f( void );

const char *r_rendererArgs[] = { "best", "arb", "arb2", "Cg", "exp", "nv10", "nv20", "r200", nullptr };

anCVarSystem r_inhibitFragmentProgram( "r_inhibitFragmentProgram", "0", CVAR_RENDERER | CVAR_BOOL, "ignore the fragment program extension" );
anCVarSystem r_glDriver( "r_glDriver", "", CVAR_RENDERER, "\"opengl32\", etc." );
anCVarSystem r_useLightPortalFlow( "r_useLightPortalFlow", "1", CVAR_RENDERER | CVAR_BOOL, "use a more precise area reference determination" );
anCVarSystem r_multiSamples( "r_multiSamples", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "number of antialiasing samples" );
anCVarSystem r_mode( "r_mode", "3", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_INTEGER, "video mode number" );
anCVarSystem r_displayRefresh( "r_displayRefresh", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_NOCHEAT, "optional display refresh rate option for vid mode", 0.0f, 200.0f );
anCVarSystem r_fullscreen( "r_fullscreen", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "0 = windowed, 1 = full screen" );
//anCVarSystem r_customWidth( "r_customWidth", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen width. set r_mode to -1 to activate" );
anCVarSystem r_customWidth( "r_customWidth", "640", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen width. set r_mode to -1 to activate" );
anCVarSystem r_customHeight( "r_customHeight", "480", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen height. set r_mode to -1 to activate" );
anCVarSystem r_singleTriangle( "r_singleTriangle", "0", CVAR_RENDERER | CVAR_BOOL, "only draw a single triangle per primitive" );
anCVarSystem r_checkBounds( "r_checkBounds", "0", CVAR_RENDERER | CVAR_BOOL, "compare all surface bounds with precalculated ones" );
//anCVarSystem r_clearScreen( "r_clearScreen", "0", CVAR_RENDERER | CVAR_INTEGER, "clears the rendered screen" );
anCVarSystem r_logFile( "r_logFile", "0", CVAR_RENDERER | CVAR_INTEGER, "number of frames to emit GL logs" );
anCVarSystem r_clear( "r_clear", "2", CVAR_RENDERER, "force screen clear every frame, 1 = purple, 2 = black, 'r g b' = custom" );

anCVarSystem r_useNV20MonoLights( "r_useNV20MonoLights", "1", CVAR_RENDERER | CVAR_INTEGER, "use pass optimization for mono lights" );
anCVarSystem r_useConstantMaterials( "r_useConstantMaterials", "1", CVAR_RENDERER | CVAR_BOOL, "use pre-calculated material registers if possible" );
anCVarSystem r_useTripleTextureARB( "r_useTripleTextureARB", "1", CVAR_RENDERER | CVAR_BOOL, "cards with 3+ texture units do a two pass instead of three pass" );
anCVarSystem r_useSilRemap( "r_useSilRemap", "1", CVAR_RENDERER | CVAR_BOOL, "consider verts with the same XYZ, but different ST the same for shadows" );
anCVarSystem r_useNodeCommonChildren( "r_useNodeCommonChildren", "1", CVAR_RENDERER | CVAR_BOOL, "stop pushing reference bounds early when possible" );
anCVarSystem r_useShadowProjectedCull( "r_useShadowProjectedCull", "1", CVAR_RENDERER | CVAR_BOOL, "discard triangles outside light volume before shadowing" );
anCVarSystem r_useShadowVertexProgram( "r_useShadowVertexProgram", "1", CVAR_RENDERER | CVAR_BOOL, "do the shadow projection in the vertex program on capable cards" );
anCVarSystem r_useShadowSurfaceScissor( "r_useShadowSurfaceScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor shadows by the scissor rect of the interaction surfaces" );
anCVarSystem r_useInteractionTable( "r_useInteractionTable", "1", CVAR_RENDERER | CVAR_BOOL, "create a full entityDefs * lightDefs table to make finding interactions faster" );
anCVarSystem r_useTurboShadow( "r_useTurboShadow", "1", CVAR_RENDERER | CVAR_BOOL, "use the infinite projection with W technique for dynamic shadows" );
anCVarSystem r_useTwoSidedStencil( "r_useTwoSidedStencil", "1", CVAR_RENDERER | CVAR_BOOL, "do stencil shadows in one pass with different ops on each side" );
anCVarSystem r_useDeferredTangents( "r_useDeferredTangents", "1", CVAR_RENDERER | CVAR_BOOL, "defer tangents calculations after deform" );
anCVarSystem r_useCachedDynamicModels( "r_useCachedDynamicModels", "1", CVAR_RENDERER | CVAR_BOOL, "cache snapshots of dynamic models" );

anCVarSystem r_useVertexBuffers( "r_useVertexBuffers", "1", CVAR_RENDERER | CVAR_INTEGER, "use ARB_vertex_buffer_object for vertexes", 0, 1, arcCmdSystem::ArgCompletion_Integer<0,1>  );
anCVarSystem r_useIndexBuffers( "r_useIndexBuffers", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "use ARB_vertex_buffer_object for indexes", 0, 1, arcCmdSystem::ArgCompletion_Integer<0,1>  );

anCVarSystem r_useStateCaching( "r_useStateCaching", "1", CVAR_RENDERER | CVAR_BOOL, "avoid redundant state changes in GL_*() calls" );
anCVarSystem r_useInfiniteFarZ( "r_useInfiniteFarZ", "1", CVAR_RENDERER | CVAR_BOOL, "use the no-far-clip-plane trick" );

anCVarSystem r_znear( "r_znear", "3", CVAR_RENDERER | CVAR_FLOAT, "near Z clip plane distance", 0.001f, 200.0f );

anCVarSystem r_ignoreGLErrors( "r_ignoreGLErrors", "1", CVAR_RENDERER | CVAR_BOOL, "ignore GL errors" );
anCVarSystem r_finish( "r_finish", "0", CVAR_RENDERER | CVAR_BOOL, "force a call to glFinish() every frame" );
anCVarSystem r_swapInterval( "r_swapInterval", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "changes wglSwapIntarval" );

anCVarSystem r_gamma( "r_gamma", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 3.0f );
anCVarSystem r_brightness( "r_brightness", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 2.0f );

anCVarSystem r_renderer( "r_renderer", "best", CVAR_RENDERER | CVAR_ARCHIVE, "hardware specific renderer path to use", r_rendererArgs, arcCmdSystem::ArgCompletion_String<r_rendererArgs> );

anCVarSystem r_jitter( "r_jitter", "0", CVAR_RENDERER | CVAR_BOOL, "randomly subpixel jitter the projection matrix" );

anCVarSystem r_skipSuppress( "r_skipSuppress", "0", CVAR_RENDERER | CVAR_BOOL, "ignore the per-view suppressions" );
anCVarSystem r_skipPostProcess( "r_skipPostProcess", "0", CVAR_RENDERER | CVAR_BOOL, "skip all post-process renderings" );
anCVarSystem r_skipLightScale( "r_skipLightScale", "0", CVAR_RENDERER | CVAR_BOOL, "don't do any post-interaction light scaling, makes things dim on low-dynamic range cards" );
anCVarSystem r_skipInteractions( "r_skipInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip all light/surface interaction drawing" );
anCVarSystem r_skipDynamicTextures( "r_skipDynamicTextures", "0", CVAR_RENDERER | CVAR_BOOL, "don't dynamically create textures" );
anCVarSystem r_skipCopyTexture( "r_skipCopyTexture", "0", CVAR_RENDERER | CVAR_BOOL, "do all rendering, but don't actually copyTexSubImage2D" );
anCVarSystem r_skipBackEnd( "r_skipBackEnd", "0", CVAR_RENDERER | CVAR_BOOL, "don't draw anything" );
anCVarSystem r_skipRender( "r_skipRender", "0", CVAR_RENDERER | CVAR_BOOL, "skip 3D rendering, but pass 2D" );
anCVarSystem r_skipRenderContext( "r_skipRenderContext", "0", CVAR_RENDERER | CVAR_BOOL, "nullptr the rendering context during backend 3D rendering" );
anCVarSystem r_skipTranslucent( "r_skipTranslucent", "0", CVAR_RENDERER | CVAR_BOOL, "skip the translucent interaction rendering" );
anCVarSystem r_skipAmbient( "r_skipAmbient", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all non-interaction drawing" );
anCVarSystem r_skipNewAmbient( "r_skipNewAmbient", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "bypasses all vertex/fragment program ambient drawing" );
anCVarSystem r_skipBlendLights( "r_skipBlendLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all blend lights" );
anCVarSystem r_skipFogLights( "r_skipFogLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all fog lights" );
anCVarSystem r_skipDeforms( "r_skipDeforms", "0", CVAR_RENDERER | CVAR_BOOL, "leave all deform materials in their original state" );
anCVarSystem r_skipFrontEnd( "r_skipFrontEnd", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all front end work, but 2D gui rendering still draws" );
anCVarSystem r_skipUpdates( "r_skipUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't accept any entity or light updates, making everything static" );
anCVarSystem r_skipOverlays( "r_skipOverlays", "0", CVAR_RENDERER | CVAR_BOOL, "skip overlay surfaces" );
anCVarSystem r_skipSpecular( "r_skipSpecular", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_CHEAT | CVAR_ARCHIVE, "use black for specular1" );
anCVarSystem r_skipBump( "r_skipBump", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "uses a flat surface instead of the bump map" );
anCVarSystem r_skipDiffuse( "r_skipDiffuse", "0", CVAR_RENDERER | CVAR_BOOL, "use black for diffuse" );
anCVarSystem r_skipROQ( "r_skipROQ", "0", CVAR_RENDERER | CVAR_BOOL, "skip ROQ decoding" );

anCVarSystem r_ignore( "r_ignore", "0", CVAR_RENDERER, "used for random debugging without defining new vars" );
anCVarSystem r_ignore2( "r_ignore2", "0", CVAR_RENDERER, "used for random debugging without defining new vars" );
anCVarSystem r_usePreciseTriangleInteractions( "r_usePreciseTriangleInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "1 = do winding clipping to determine if each ambiguous tri should be lit" );
anCVarSystem r_useCulling( "r_useCulling", "2", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = sphere, 2 = sphere + box", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
anCVarSystem r_useLightCulling( "r_useLightCulling", "3", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = box, 2 = exact clip of polyhedron faces, 3 = also areas", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );
anCVarSystem r_useLightScissors( "r_useLightScissors", "1", CVAR_RENDERER | CVAR_BOOL, "1 = use custom scissor rectangle for each light" );
anCVarSystem r_useClippedLightScissors( "r_useClippedLightScissors", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = full screen when near clipped, 1 = exact when near clipped, 2 = exact always", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
anCVarSystem r_useEntityCulling( "r_useEntityCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = none, 1 = box" );
anCVarSystem r_useEntityScissors( "r_useEntityScissors", "0", CVAR_RENDERER | CVAR_BOOL, "1 = use custom scissor rectangle for each entity" );
anCVarSystem r_useInteractionCulling( "r_useInteractionCulling", "1", CVAR_RENDERER | CVAR_BOOL, "1 = cull interactions" );
anCVarSystem r_useInteractionScissors( "r_useInteractionScissors", "2", CVAR_RENDERER | CVAR_INTEGER, "1 = use a custom scissor rectangle for each shadow interaction, 2 = also crop using portal scissors", -2, 2, arcCmdSystem::ArgCompletion_Integer<-2,2> );
anCVarSystem r_useShadowCulling( "r_useShadowCulling", "1", CVAR_RENDERER | CVAR_BOOL, "try to cull shadows from partially visible lights" );
anCVarSystem r_useFrustumFarDistance( "r_useFrustumFarDistance", "0", CVAR_RENDERER | CVAR_FLOAT, "if != 0 force the view frustum far distance to this distance" );

anCVarSystem r_offsetFactor( "r_offsetfactor", "0", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
anCVarSystem r_offsetUnits( "r_offsetunits", "-600", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
anCVarSystem r_shadowPolygonOffset( "r_shadowPolygonOffset", "-1", CVAR_RENDERER | CVAR_FLOAT, "bias value added to depth test for stencil shadow drawing" );
anCVarSystem r_shadowPolygonFactor( "r_shadowPolygonFactor", "0", CVAR_RENDERER | CVAR_FLOAT, "scale value for stencil shadow drawing" );
anCVarSystem r_frontBuffer( "r_frontBuffer", "0", CVAR_RENDERER | CVAR_BOOL, "draw to front buffer for debugging" );
anCVarSystem r_skipSubviews( "r_skipSubviews", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = don't render any gui elements on surfaces" );
anCVarSystem r_skipGuiShaders( "r_skipGuiShaders", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all gui elements on surfaces, 2 = skip drawing but still handle events, 3 = draw but skip events", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );
anCVarSystem r_skipParticles( "r_skipParticles", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all particle systems", 0, 1, arcCmdSystem::ArgCompletion_Integer<0,1> );
anCVarSystem r_subviewOnly( "r_subviewOnly", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't render main view, allowing subviews to be debugged" );
anCVarSystem r_shadows( "r_shadows", "1", CVAR_RENDERER | CVAR_BOOL  | CVAR_ARCHIVE, "enable shadows" );
anCVarSystem r_testARBProgram( "r_testARBProgram", "0", CVAR_RENDERER | CVAR_BOOL, "experiment with vertex/fragment programs" );
anCVarSystem r_testGamma( "r_testGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels", 0, 195 );
anCVarSystem r_testGammaBias( "r_testGammaBias", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
anCVarSystem r_testStepGamma( "r_testStepGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
anCVarSystem r_lightScale( "r_lightScale", "2", CVAR_RENDERER | CVAR_FLOAT, "all light intensities are multiplied by this" );
anCVarSystem r_lightSourceRadius( "r_lightSourceRadius", "0", CVAR_RENDERER | CVAR_FLOAT, "for soft-shadow sampling" );
anCVarSystem r_flareSize( "r_flareSize", "1", CVAR_RENDERER | CVAR_FLOAT, "scale the flare deforms from the material def" );

anCVarSystem r_useExternalShadows( "r_useExternalShadows", "1", CVAR_RENDERER | CVAR_INTEGER, "1 = skip drawing caps when outside the light volume, 2 = force to no caps for testing", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
anCVarSystem r_useOptimizedShadows( "r_useOptimizedShadows", "1", CVAR_RENDERER | CVAR_BOOL, "use the dmap generated static shadow volumes" );
anCVarSystem r_useScissor( "r_useScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor clip as portals and lights are processed" );
anCVarSystem r_useCombinerDisplayLists( "r_useCombinerDisplayLists", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_NOCHEAT, "put all nvidia register combiner programming in display lists" );
anCVarSystem r_useDepthBoundsTest( "r_useDepthBoundsTest", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test to reduce shadow fill" );

anCVarSystem r_screenFraction( "r_screenFraction", "100", CVAR_RENDERER | CVAR_INTEGER, "for testing fill rate, the resolution of the entire screen can be changed" );
anCVarSystem r_demonstrateBug( "r_demonstrateBug", "0", CVAR_RENDERER | CVAR_BOOL, "used during development to show IHV's their problems" );
anCVarSystem r_usePortals( "r_usePortals", "1", CVAR_RENDERER | CVAR_BOOL, " 1 = use portals to perform area culling, otherwise draw everything" );
anCVarSystem r_singleLight( "r_singleLight", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one light" );
anCVarSystem r_singleEntity( "r_singleEntity", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one entity" );
anCVarSystem r_singleSurface( "r_singleSurface", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one surface on each entity" );
anCVarSystem r_singleArea( "r_singleArea", "0", CVAR_RENDERER | CVAR_BOOL, "only draw the portal area the view is actually in" );
anCVarSystem r_forceLoadImages( "r_forceLoadImages", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "draw all images to screen after registration" );
anCVarSystem r_orderIndexes( "r_orderIndexes", "1", CVAR_RENDERER | CVAR_BOOL, "perform index reorganization to optimize vertex use" );
anCVarSystem r_lightAllBackFaces( "r_lightAllBackFaces", "0", CVAR_RENDERER | CVAR_BOOL, "light all the back faces, even when they would be shadowed" );

// visual debugging info
anCVarSystem r_showPortals( "r_showPortals", "0", CVAR_RENDERER | CVAR_BOOL, "draw portal outlines in color based on passed / not passed" );
anCVarSystem r_showUnsmoothedTangents( "r_showUnsmoothedTangents", "0", CVAR_RENDERER | CVAR_BOOL, "if 1, put all nvidia register combiner programming in display lists" );
anCVarSystem r_showSilhouette( "r_showSilhouette", "0", CVAR_RENDERER | CVAR_BOOL, "highlight edges that are casting shadow planes" );
anCVarSystem r_showVertexColor( "r_showVertexColor", "0", CVAR_RENDERER | CVAR_BOOL, "draws all triangles with the solid vertex color" );
anCVarSystem r_showUpdates( "r_showUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "report entity and light updates and ref counts" );
anCVarSystem r_showDemo( "r_showDemo", "0", CVAR_RENDERER | CVAR_BOOL, "report reads and writes to the demo file" );
anCVarSystem r_showDynamic( "r_showDynamic", "0", CVAR_RENDERER | CVAR_BOOL, "report stats on dynamic surface generation" );
anCVarSystem r_showLightScale( "r_showLightScale", "0", CVAR_RENDERER | CVAR_BOOL, "report the scale factor applied to drawing for overbrights" );
anCVarSystem r_showDefs( "r_showDefs", "0", CVAR_RENDERER | CVAR_BOOL, "report the number of modeDefs and lightDefs in view" );
anCVarSystem r_showTrace( "r_showTrace", "0", CVAR_RENDERER | CVAR_INTEGER, "show the intersection of an eye trace with the world", arcCmdSystem::ArgCompletion_Integer<0,2> );
anCVarSystem r_showIntensity( "r_showIntensity", "0", CVAR_RENDERER | CVAR_BOOL, "draw the screen colors based on intensity, red = 0, green = 128, blue = 255" );
anCVarSystem r_showImages( "r_showImages", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show all images instead of rendering, 2 = show in proportional size", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
anCVarSystem r_showSmp( "r_showSmp", "0", CVAR_RENDERER | CVAR_BOOL, "show which end (front or back) is blocking" );
anCVarSystem r_showLights( "r_showLights", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = just print volumes numbers, highlighting ones covering the view, 2 = also draw planes of each volume, 3 = also draw edges of each volume", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );
anCVarSystem r_showShadows( "r_showShadows", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = visualize the stencil shadow volumes, 2 = draw filled in", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );
anCVarSystem r_showShadowCount( "r_showShadowCount", "0", CVAR_RENDERER | CVAR_INTEGER, "colors screen based on shadow volume depth complexity, >= 2 = print overdraw count based on stencil index values, 3 = only show turboshadows, 4 = only show static shadows", 0, 4, arcCmdSystem::ArgCompletion_Integer<0,4> );
anCVarSystem r_showLightScissors( "r_showLightScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show light scissor rectangles" );
anCVarSystem r_showEntityScissors( "r_showEntityScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show entity scissor rectangles" );
anCVarSystem r_showInteractionFrustums( "r_showInteractionFrustums", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show a frustum for each interaction, 2 = also draw lines to light origin, 3 = also draw entity bbox", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );
anCVarSystem r_showInteractionScissors( "r_showInteractionScissors", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show screen rectangle which contains the interaction frustum, 2 = also draw construction lines", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
anCVarSystem r_showLightCount( "r_showLightCount", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = colors surfaces based on light count, 2 = also count everything through walls, 3 = also print overdraw", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );
anCVarSystem r_showViewEntitys( "r_showViewEntitys", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = displays the bounding boxes of all view models, 2 = print index numbers" );
anCVarSystem r_showTris( "r_showTris", "0", CVAR_RENDERER | CVAR_INTEGER, "enables wireframe rendering of the world, 1 = only draw visible ones, 2 = draw all front facing, 3 = draw all", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );
anCVarSystem r_showSurfaceInfo( "r_showSurfaceInfo", "0", CVAR_RENDERER | CVAR_BOOL, "show surface material name under crosshair" );
anCVarSystem r_showNormals( "r_showNormals", "0", CVAR_RENDERER | CVAR_FLOAT, "draws wireframe normals" );
anCVarSystem r_showMemory( "r_showMemory", "0", CVAR_RENDERER | CVAR_BOOL, "print frame memory utilization" );
anCVarSystem r_showCull( "r_showCull", "0", CVAR_RENDERER | CVAR_BOOL, "report sphere and box culling stats" );
anCVarSystem r_showInteractions( "r_showInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "report interaction generation activity" );
anCVarSystem r_showDepth( "r_showDepth", "0", CVAR_RENDERER | CVAR_BOOL, "display the contents of the depth buffer and the depth range" );
anCVarSystem r_showSurfaces( "r_showSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "report surface/light/shadow counts" );
anCVarSystem r_showPrimitives( "r_showPrimitives", "0", CVAR_RENDERER | CVAR_INTEGER, "report drawsurf/index/vertex counts" );
anCVarSystem r_showEdges( "r_showEdges", "0", CVAR_RENDERER | CVAR_BOOL, "draw the sil edges" );
anCVarSystem r_showTexturePolarity( "r_showTexturePolarity", "0", CVAR_RENDERER | CVAR_BOOL, "shade triangles by texture area polarity" );
anCVarSystem r_showTangentSpace( "r_showTangentSpace", "0", CVAR_RENDERER | CVAR_INTEGER, "shade triangles by tangent space, 1 = use 1st tangent vector, 2 = use 2nd tangent vector, 3 = use normal vector", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );
anCVarSystem r_showDominantTri( "r_showDominantTri", "0", CVAR_RENDERER | CVAR_BOOL, "draw lines from vertexes to center of dominant triangles" );
anCVarSystem r_showAlloc( "r_showAlloc", "0", CVAR_RENDERER | CVAR_BOOL, "report alloc/free counts" );
anCVarSystem r_showTextureVectors( "r_showTextureVectors", "0", CVAR_RENDERER | CVAR_FLOAT, " if > 0 draw each triangles texture (tangent) vectors" );
anCVarSystem r_showOverDraw( "r_showOverDraw", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = geometry overdraw, 2 = light interaction overdraw, 3 = geometry and light interaction overdraw", 0, 3, arcCmdSystem::ArgCompletion_Integer<0,3> );

anCVarSystem r_lockSurfaces( "r_lockSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "allow moving the view point without changing the composition of the scene, including culling" );
anCVarSystem r_useEntityCallbacks( "r_useEntityCallbacks", "1", CVAR_RENDERER | CVAR_BOOL, "if 0, issue the callback immediately at update time, rather than defering" );

anCVarSystem r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw the skeleton when model animates, 1 = draw model with skeleton, 2 = draw skeleton only", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
anCVarSystem r_jointNameScale( "r_jointNameScale", "0.02", CVAR_RENDERER | CVAR_FLOAT, "size of joint names when r_showskel is set to 1" );
anCVarSystem r_jointNameOffset( "r_jointNameOffset", "0.5", CVAR_RENDERER | CVAR_FLOAT, "offset of joint names when r_showskel is set to 1" );

anCVarSystem r_cgVertexProfile( "r_cgVertexProfile", "best", CVAR_RENDERER | CVAR_ARCHIVE, "arbvp1, vp20, vp30" );
anCVarSystem r_cgFragmentProfile( "r_cgFragmentProfile", "best", CVAR_RENDERER | CVAR_ARCHIVE, "arbfp1, fp30" );

anCVarSystem r_debugLineDepthTest( "r_debugLineDepthTest", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "perform depth test on debug lines" );
anCVarSystem r_debugLineWidth( "r_debugLineWidth", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "width of debug lines" );
anCVarSystem r_debugArrowStep( "r_debugArrowStep", "120", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "step size of arrow cone line rotation in degrees", 0, 120 );
anCVarSystem r_debugPolygonFilled( "r_debugPolygonFilled", "1", CVAR_RENDERER | CVAR_BOOL, "draw a filled polygon" );

anCVarSystem r_materialOverride( "r_materialOverride", "", CVAR_RENDERER, "overrides all materials", arcCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );

anCVarSystem r_debugRenderToTexture( "r_debugRenderToTexture", "0", CVAR_RENDERER | CVAR_INTEGER, "" );

void ( APIENTRY * qglMultiTexCoord2fARB )( GLenum texture, GLfloat s, GLfloat t );
void ( APIENTRY * qglMultiTexCoord2fvARB )( GLenum texture, GLfloat *st );
void ( APIENTRY * qglActiveTextureARB )( GLenum texture );
void ( APIENTRY * qglClientActiveTextureARB )( GLenum texture );

void ( APIENTRY *qglCombinerParameterfvNV )( GLenum pname, const GLfloat *params );
void ( APIENTRY *qglCombinerParameterivNV )( GLenum pname, const GLint *params );
void ( APIENTRY *qglCombinerParameterfNV )( GLenum pname, const GLfloat param );
void ( APIENTRY *qglCombinerParameteriNV )( GLenum pname, const GLint param );
void ( APIENTRY *qglCombinerInputNV )( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
void ( APIENTRY *qglCombinerOutputNV )( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum );
void ( APIENTRY *qglFinalCombinerInputNV )( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );

void (APIENTRY *qglVertexArrayRangeNV)( GLsizei length, void *pointer );

void *(APIENTRY *qAllocateMemoryNV)( GLsizei size, float readFrequency, float writeFrequency, float priority);
void (APIENTRY *qFreeMemoryNV)( void *pointer );
#ifdef GLX_VERSION_1_1
#define Q_ALLOCATE_MEMORY_NV "glXAllocateMemoryNV"
#define Q_FREE_MEMORY_NV "glXFreeMemoryNV"
#else
#define Q_ALLOCATE_MEMORY_NV "wglAllocateMemoryNV"
#define Q_FREE_MEMORY_NV "wglFreeMemoryNV"
#endif

void (APIENTRY *qglTexImage3D)( GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
void (APIENTRY * qglColorTableEXT)( int, int, int, int, int, const void * );

// ATI_fragment_shader
PFNGLGENFRAGMENTSHADERSATIPROC			qglGenFragmentShadersATI;
PFNGLBINDFRAGMENTSHADERATIPROC			qglBindFragmentShaderATI;
PFNGLDELETEFRAGMENTSHADERATIPROC		qglDeleteFragmentShaderATI;
PFNGLBEGINFRAGMENTSHADERATIPROC			qglBeginFragmentShaderATI;
PFNGLENDFRAGMENTSHADERATIPROC			qglEndFragmentShaderATI;
PFNGLPASSTEXCOORDATIPROC				qglPassTexCoordATI;
PFNGLSAMPLEMAPATIPROC					qglSampleMapATI;
PFNGLCOLORFRAGMENTOP1ATIPROC			qglColorFragmentOp1ATI;
PFNGLCOLORFRAGMENTOP2ATIPROC			qglColorFragmentOp2ATI;
PFNGLCOLORFRAGMENTOP3ATIPROC			qglColorFragmentOp3ATI;
PFNGLALPHAFRAGMENTOP1ATIPROC			qglAlphaFragmentOp1ATI;
PFNGLALPHAFRAGMENTOP2ATIPROC			qglAlphaFragmentOp2ATI;
PFNGLALPHAFRAGMENTOP3ATIPROC			qglAlphaFragmentOp3ATI;
PFNGLSETFRAGMENTSHADERCONSTANTATIPROC	qglSetFragmentShaderConstantATI;

// EXT_stencil_two_side
PFNGLACTIVESTENCILFACEEXTPROC			qglActiveStencilFaceEXT;

// ATI_separate_stencil
PFNGLSTENCILOPSEPARATEATIPROC			qglStencilOpSeparateATI;
PFNGLSTENCILFUNCSEPARATEATIPROC			qglStencilFuncSeparateATI;

// ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC		qglCompressedTexImage2DARB;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC		qglGetCompressedTexImageARB;

// ARB_vertex_buffer_object
PFNGLBINDBUFFERARBPROC					qglBindBufferARB;
PFNGLDELETEBUFFERSARBPROC				qglDeleteBuffersARB;
PFNGLGENBUFFERSARBPROC					qglGenBuffersARB;
PFNGLISBUFFERARBPROC					qglIsBufferARB;
PFNGLBUFFERDATAARBPROC					qglBufferDataARB;
PFNGLBUFFERSUBDATAARBPROC				qglBufferSubDataARB;
PFNGLGETBUFFERSUBDATAARBPROC			qglGetBufferSubDataARB;
PFNGLMAPBUFFERARBPROC					qglMapBufferARB;
PFNGLUNMAPBUFFERARBPROC					qglUnmapBufferARB;
PFNGLGETBUFFERPARAMETERIVARBPROC		qglGetBufferParameterivARB;
PFNGLGETBUFFERPOINTERVARBPROC			qglGetBufferPointervARB;

// ARB_vertex_program / ARB_fragment_program
PFNGLVERTEXATTRIBPOINTERARBPROC			qglVertexAttribPointerARB;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC		qglEnableVertexAttribArrayARB;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC	qglDisableVertexAttribArrayARB;
PFNGLPROGRAMSTRINGARBPROC				qglProgramStringARB;
PFNGLBINDPROGRAMARBPROC					qglBindProgramARB;
PFNGLGENPROGRAMSARBPROC					qglGenProgramsARB;
PFNGLPROGRAMENVPARAMETER4FVARBPROC		qglProgramEnvParameter4fvARB;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC	qglProgramLocalParameter4fvARB;
	// GL_ARB_vertex_array_object
PFNGLGENVERTEXARRAYSPROC 				qglGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC 				qglBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC 			qglDeleteVertexArrays;
PFNGLISVERTEXARRAYPROC					qglIsVertexArray;

// GL_EXT_depth_bounds_test
PFNGLDEPTHBOUNDSEXTPROC					qglDepthBoundsEXT;
PFNGLDEPTHBOUNDSPROC					qglDepthBounds;

PFNGLARBDEPTHCLAMPPROC GL_ARB_depth_clamp;
void RB_Error( errorParm_t code, const char *fmt, ... ) {
	char buf[ 4096 ];
	va_list	argptr;
	va_start( argptr, fmt );
	anStr::vsnprintf( buf, sizeof( buf ), fmt, argptr );
	va_end( argptr );
	common->Error( code, "%s", buf );
}

void RB_Printf( const char *fmt, ... ) {
	char buf[ MAXPRINTMSG ];
	va_list	argptr;
	va_start( argptr, fmt );
	anStr::vsnprintf( buf, sizeof( buf ), fmt, argptr );
	va_end( argptr );

	common->Printf( PRINT_ALL, "%s", buf );
}

/*
========================
qglBindMultiTextureEXT

As of 2011/09/16 the Intel drivers for "Sandy-Bridge" and "Ivy-Bridge" integrated graphics
no longer support the BindMultiTexture extension.
========================
*/
void APIENTRY qglBindMultiTextureEXT( GLenum texunit, GLenum target, GLuint texture ) {
	qglActiveTextureARB( texunit );
	qglBindTexture( target, texture );
}

void GL_Scissor( GLint x, GLint y, GLsizei width, GLsizei height ) {

}

bool GL_BindTexture( char *name ) {}
bool GL_ShowTexture( char *name ) {}

void GL_Begin( GLenum mode ) {
	qglBegin( mode );
	// save the current state
	qglState.glStateVersion = stateVersion;

	// draw a 2D rect to show the texture
	qglClearColor( 0.3f, 0.3f, 0.3f, 0.0f );
	qglClear( GL_COLOR_BUFFER_BIT );

	// set the projection matrix
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, 640, 0, 480, -1, 1 );
	qglMatrixMode( GL_MODELVIEW );

	// enable 2D texturing
	qglEnable( GL_TEXTURE_2D );
	// if mode is GL_SELECT, then we need to clear the screen
	if ( mode!= GL_SELECT ) {
		qglClearColor( 0.3f, 0.3f, 0.3f, 0.0f );
		qglClear( GL_COLOR_BUFFER_BIT );
		qglLoadIdentity();
		// multi layer textures we need to render with normal map bump map specular and diffuse

	}
}
static void CALLBACK DebugCallback( unsigned int source, unsigned int type, unsigned int id, unsigned int severity, int length, const char *message, const void *userParam ) {
#if defined(_WIN32)
	OutputDebugString( message );
	OutputDebugString( "\n" );
#else
	printf( "%s\n", message );
#endif
}

/*
=================
R_CheckExtension
=================
*/
static bool R_CheckExtension( const char *name ) {
	if ( !strstr( qglConfig.extensionsStr, name ) ) {
		common->Printf( "X..%s not found\n", name );
		return false;
	}

	common->Printf( "...using %s\n", name );
	return true;
}

/*
==================
R_CheckPortableExtensions


GL_ARB_depth_clamp, GL_ARB_texture_float, drawElementsBaseVertex
GL_ARB_texture_compression_rgtc - not fully added yet just print msg atm
GL_ARB_fragment_shader - added may need more added onto it
GL_ARB_vertex_array_object - added
GL_ARB_seamless_cube_map - added

GL_EXT_direct_state_access - added / partial
	- glNamedBufferData - not yet added
	- glNamedBufferSubData - not yet added
	- glMapNamedBuffer -not yet added
	- glUnmapNamedBuffer - not yet added
	- glNamedBufferStorage - added
	- glCreateBuffers - added
	- glDeleteBuffers - added

GL_ARB_framebuffer_object - added / partial will add more
	qglGenFramebuffers; - added
	qglBindFramebuffer; - added
	qglDeleteFramebuffers; - added
	qglFramebufferTexture; - added

GL_ARB_fragment_shader - added
	glCreateShader - added
	glShaderSource - added
	glCompileShader - added
	glCreateProgram - added
	glAttachShader - added
	glAttachShader - added
	glLinkProgram - added
	glUseProgram - added
	glDeleteShader - added
	glGetUniformLocation - added
	glUniform1f - added
	glCreateShaderProgramv - added
	glBindProgramPipeline - added
	glDeleteProgram - added
GL_ARB_vertex_array_object - added / partial
qglDrawRangeElementsEXT - added
qglBlendEquationEXT - added

Heres an examples one way to check a version and string output

if ( stricmpn( qglConfig.vendor_string, "ATI Technologies",16 ) == 0
&& stricmpn( qglConfig.version_string, "1.3.3",5 ) == 0 && qglConfig.version_string[5] < '9' ) {
g_bTextureRectangleHack = true;
}
==================
*/
static void R_CheckPortableExtensions( void ) {
	if ( glSLAvailable ) {
		qglConfig.qglVersion = atof( qglConfig.qglslVersion );
		strncpyz( qglConfig.qglVersion, (const char *)qglGetString( GL_SHADING_LANGUAGE_VERSION ), sizeof( qglConfig.qglVersion ) );
		sscanf( qglConfig.qglVersion, "%d.%d", &qglConfig.versionStr, &qglConfig.qglslVersion );
		common->Printf( "...using GLSL version %s\n", qglConfig.qglVersion );
	} else {
		qglConfig.qglVersion = atof( qglConfig.versionStr );
	}

	if ( Icmpn( qglConfig.rendererStr, "ATI ", 4 ) == 0 || Icmpn( qglConfig.rendererStr, "AMD ", 4 ) == 0 ) {
		qglConfig.vendor = VEN_AMD64VEGA;
	} else if ( Icmpn( qglConfig.rendererStr, "NVIDIA", 6 ) == 0 ) {
		qglConfig.vendor = VEN_NVIDIA;
	} else if ( Icmpn( qglConfig.rendererStr, "Intel", 5 ) == 0 ) {
		qglConfig.vendor = VEN_INTEL;
	}
	if ( Icmpn( qglConfig.rendererStr, "Mesa", 4 ) == 0 || Icmpn( qglConfig.rendererStr, "X.org", 4 ) == 0 || Icmpn( qglConfig.rendererStr, "Gallium", 7 ) == 0 ) {
		qglConfig.driverType = GLDRIVER_OGL_MESA;
	}

	// GL_ARB_multitexture
	qglConfig.multitextureAvailable = R_CheckExtension( "GL_ARB_multitexture" );

	// FIXME: fix this for gl 4.6
	if ( qglConfig.multitextureAvailable ) {
		qglMultiTexCoord2fARB = (void(APIENTRY *)( GLenum, GLfloat, GLfloat) )GLimp_ExtensionPointer( "glMultiTexCoord2fARB" );
		qglMultiTexCoord2fvARB = (void(APIENTRY *)( GLenum, GLfloat *) )GLimp_ExtensionPointer( "glMultiTexCoord2fvARB" );
		qglActiveTextureARB = (void(APIENTRY *)( GLenum) )GLimp_ExtensionPointer( "glActiveTextureARB" );
		qglGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, (GLint *)&qglConfig.maxImageUnits );
		if ( qglConfig.maxImageUnits > MAX_MULTITEXTURE_UNITS ) {
			qglConfig.maxImageUnits = MAX_MULTITEXTURE_UNITS;
		}
		if ( qglConfig.maxImageUnits < 2 ) {
			qglConfig.multitextureAvailable = false;	// shouldn't ever happen
		}
		qglGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, (GLint *)&qglConfig.maxImageCoords );
		qglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, (GLint *)&qglConfig.maxTextureImageUnits );
	}
	// GL_EXT_clamp_to_edge
	//qglConfig.clampToEdgeAvailable = CR_CheckExtension( "GL_EXT_texture_edge_clamp\n" );
	//// GL_ARB_texture_env_combine
	qglConfig.textureEnvCombine = R_CheckExtension( "GL_ARB_texture_env_combine" );

	// GL_ARB_texture_cube_map and GL_ARB_seamless_cube_map
	qglConfig.useCubeMap = R_CheckExtension( "GL_ARB_texture_cube_map" );

	// GL_ARB_texture_env_dot3
	qglConfig.envDot3 = R_CheckExtension( "GL_ARB_texture_env_dot3" );

	// GL_ARB_texture_env_add
	qglConfig.textureEnvAdd = R_CheckExtension( "GL_ARB_texture_env_add" );

	// GL_ARB_texture_non_power_of_two
	qglConfig.isImageNonPO2 = R_CheckExtension( "GL_ARB_texture_non_power_of_two" );

	if ( qglConfig.qglVersion >= 3.2 || R_CheckExtension( "GL_ARB_seamless_cube_map" ) ) {
		qglConfig.useSeamlessCubeMap = R_CheckExtension( "GL_ARB_seamless_cube_map" );
		r_useSeamlessCubeMap.SetModified();
	}
	// GL_ARB_framebuffer_sRGB
	//qglConfig.sRGBFramebufferAvailable = GLEW_ARB_framebuffer_sRGB != 0;
	//r_useSRGB.SetModified();

	//qglConfig.directStateAccess = R_CheckExtension( "GL_EXT_direct_state_access" );
	// GL_EXT_direct_state_access  Check if the extension is supported
	if ( qglConfig.qglVersion >= 4.6 || R_CheckExtension( "GL_EXT_direct_state_access" ) ) {
		common->Printf( "...using %s\n", "GL_EXT_direct_state_access" );
		qglNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)GLimp_ExtensionPointer( "glNamedBufferData" );
	    qglNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)GLimp_ExtensionPointer( "glNamedBufferStorage" );
	    qglCreateBuffers = (PFNGLCREATEBUFFERSPROC)GLimp_ExtensionPointer( "glCreateBuffers" );
	    qglDeleteBuffers = (PFNGLDELETEBUFFERSPROC)GLimp_ExtensionPointer( "glDeleteBuffers" );
		qglConfig.directStateAccess = true;
	} else {
		common->Printf( "X..%s not found\n", "GL_EXT_direct_state_access" );
		qglConfig.directStateAccess = false;
	}

	// add GL_ARB_texture_compression_rgtc, GL_ARB_texture_compression_bptc
	// GL_ARB_texture_compression + GL_S3_s3tc
	// DRI drivers may have GL_ARB_texture_compression but no GL_EXT_texture_compression_s3tc
	if ( R_CheckExtension( "GL_ARB_texture_compression" ) && R_CheckExtension( "GL_EXT_texture_compression_s3tc" ) ) {
		qglConfig.textureCompression = true;
		qglCompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)GLimp_ExtensionPointer( "glCompressedTexImage2DARB" );
		qglGetCompressedTexImageARB = (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)GLimp_ExtensionPointer( "glGetCompressedTexImageARB" );
	} else {
		qglConfig.textureCompression = false;
	} else if ( R_CheckExtension( "GL_ARB_texture_compression_rgtc" ) && R_CheckExtension( "GL_ARB_texture_compression_bptc" ) ) {
		qglConfig.textureCompression = true;
		//qglCompressed = (PFNGL  ARBPROC)GLimp_ExtensionPointer( "glCompressed" );
		//qglGetCompressed = (PFNGL ARBPROC)GLimp_ExtensionPointer( "glGetCompressed" );
	} else {
		qglConfig.textureCompression = false;
	}

	// GL_EXT_texture_filter_anisotropic
	qglConfig.useAnisotropyFilter = R_CheckExtension( "GL_EXT_texture_filter_anisotropic" );
	if ( qglConfig.useAnisotropyFilter ) {
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &qglConfig.maxTextureAnisotropy );
		common->Printf( "   maxTextureAnisotropy: %f\n", qglConfig.maxTextureAnisotropy );
	} else {
		qglConfig.maxTextureAnisotropy = 1;
	}

	// GL_EXT_texture_lod_bias GL_ARB_shader_texture_lod
	// The actual extension is broken as specified, storing the state in the texture unit instead
	// of the texture object.  The behavior in GL 1.4 is the behavior we use.
	if ( qglConfig.qglVersion >= 1.4 || R_CheckExtension( "GL_EXT_texture_lod" ) ) {
		common->Printf( "...using %s\n", "GL_1.4_texture_lod_bias" );
		qglConfig.useTextureLODBias = true;
	} else {
		common->Printf( "X..%s not found\n", "GL_1.4_texture_lod_bias" );
		qglConfig.useTextureLODBias = false;
	} else if ( qglConfig.qglVersion >= 3.0 && image_lodbias && globalImages->distanceLod )
		qglConfig.distanceLod = R_CheckExtension( "GL_ARB_shader_texture_lod" );
	} else {
		common->Printf( "System does not support %s\n", "GL_ARB_shader_texture_lod" )
		globalImages->distanceLod = false;
	}

	// GL_EXT_shared_texture_palette
	qglConfig.isSharedTPalette = R_CheckExtension( "GL_EXT_shared_texture_palette" );
	if ( qglConfig.isSharedTPalette ) {
		qglColorTableEXT = ( void ( APIENTRY * ) ( int, int, int, int, int, const void * ) ) GLimp_ExtensionPointer( "glColorTableEXT" );
	}

	// GL_EXT_texture3D (not currently used for anything)
	qglConfig.use3DImageEXT = R_CheckExtension( "GL_EXT_texture3D" );
	if ( qglConfig.use3DImageEXT ) {
		qglTexImage3D =
			(void (APIENTRY *)( GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *) )
			GLimp_ExtensionPointer( "glTexImage3D" );
	}

	// qglBlendEquationEXT
	qglConfig.blendSquareAvailable = R_CheckExtension( "qglBlendEquationEXT" );
	if ( qglConfig.blendSquareAvailable ) {
		qglBlendEquationEXT = (void (APIENTRY *)( GLenum mode ) )
			GLimp_ExtensionPointer( "glCombinerParameterfvNV" );
	}

	qglConfig.drawRangeElementsAvailable = R_CheckExtension( "qglDrawRangeElementsEXT" );
	if ( qglConfig.drawRangeElementsAvailable ) {
		qglDrawRangeElementsEXT = (void (APIENTRY *)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *RESTRICT indices) )
			GLimp_ExtensionPointer( "qglDrawRangeElementsEXT" );
	}

	// EXT_stencil_wrap
	// This isn't very important, but some pathological case might cause a clamp error and give a shadow bug.
	// Nvidia also believes that future hardware may be able to run faster with this enabled to avoid the
	// serialization of clamping.
	if ( R_CheckExtension( "GL_EXT_stencil_wrap" ) ) {
		tr.stencilIncr = GL_INCR_WRAP_EXT;
		tr.stencilDecr = GL_DECR_WRAP_EXT;
	} else {
		tr.stencilIncr = GL_INCR;
		tr.stencilDecr = GL_DECR;
	}

	// GL_NV_register_combiners
	qglConfig.regCombiners = R_CheckExtension( "GL_NV_register_combiners" );
	if ( qglConfig.regCombiners ) {
		qglCombinerParameterfvNV = (void (APIENTRY *)( GLenum pname, const GLfloat *params ) )
			GLimp_ExtensionPointer( "glCombinerParameterfvNV" );
		qglCombinerParameterivNV = (void (APIENTRY *)( GLenum pname, const GLint *params ) )
			GLimp_ExtensionPointer( "glCombinerParameterivNV" );
		qglCombinerParameterfNV = (void (APIENTRY *)( GLenum pname, const GLfloat param ) )
			GLimp_ExtensionPointer( "glCombinerParameterfNV" );
		qglCombinerParameteriNV = (void (APIENTRY *)( GLenum pname, const GLint param ) )
			GLimp_ExtensionPointer( "glCombinerParameteriNV" );
		qglCombinerInputNV = (void (APIENTRY *)( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) )
			GLimp_ExtensionPointer( "glCombinerInputNV" );
		qglCombinerOutputNV = (void (APIENTRY *)( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum ) )
			GLimp_ExtensionPointer( "glCombinerOutputNV" );
		qglFinalCombinerInputNV = (void (APIENTRY *)( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage ) )
			GLimp_ExtensionPointer( "glFinalCombinerInputNV" );
	}

	// GL_EXT_stencil_two_side
	qglConfig.isDoubleEdgeStencil = R_CheckExtension( "GL_EXT_stencil_two_side" );
	if ( qglConfig.isDoubleEdgeStencil ) {
		qglActiveStencilFaceEXT = (PFNGLACTIVESTENCILFACEEXTPROC)GLimp_ExtensionPointer( "glActiveStencilFaceEXT" );
	} else {
		qglConfig.atiDoubleEdgeStencil = R_CheckExtension( "GL_ATI_separate_stencil" );
		if ( qglConfig.atiDoubleEdgeStencil && qglConfig.ATI_separateStencil ) {
			qglStencilFuncSeparateATI  = (PFNGLSTENCILFUNCSEPARATEATIPROC)GLimp_ExtensionPointer( "glStencilFuncSeparateATI" );
			qglStencilOpSeparateATI = (PFNGLSTENCILOPSEPARATEATIPROC)GLimp_ExtensionPointer( "glStencilOpSeparateATI" );
		}
	}

	// GL_ATI_fragment_shader
	qglConfig.atiShaderFragmentOn = R_CheckExtension( "GL_ATI_fragment_shader" );
	if ( ! qglConfig.atiShaderFragmentOn ) {
		// only on OSX: ATI_fragment_shader is faked through ATI_text_fragment_shader (macosx_glimp.cpp)
		qglConfig.atiShaderFragmentOn = R_CheckExtension( "GL_ATI_text_fragment_shader" );
		qglGenFragmentShadersATI = (PFNGLGENFRAGMENTSHADERSATIPROC)GLimp_ExtensionPointer( "glGenFragmentShadersATI" );
		qglBindFragmentShaderATI = (PFNGLBINDFRAGMENTSHADERATIPROC)GLimp_ExtensionPointer( "glBindFragmentShaderATI" );
		qglDeleteFragmentShaderATI = (PFNGLDELETEFRAGMENTSHADERATIPROC)GLimp_ExtensionPointer( "glDeleteFragmentShaderATI" );
		qglBeginFragmentShaderATI = (PFNGLBEGINFRAGMENTSHADERATIPROC)GLimp_ExtensionPointer( "glBeginFragmentShaderATI" );
		qglEndFragmentShaderATI = (PFNGLENDFRAGMENTSHADERATIPROC)GLimp_ExtensionPointer( "glEndFragmentShaderATI" );
		qglPassTexCoordATI = (PFNGLPASSTEXCOORDATIPROC)GLimp_ExtensionPointer( "glPassTexCoordATI" );
		qglSampleMapATI = (PFNGLSAMPLEMAPATIPROC)GLimp_ExtensionPointer( "glSampleMapATI" );
		qglColorFragmentOp1ATI = (PFNGLCOLORFRAGMENTOP1ATIPROC)GLimp_ExtensionPointer( "glColorFragmentOp1ATI" );
		qglColorFragmentOp2ATI = (PFNGLCOLORFRAGMENTOP2ATIPROC)GLimp_ExtensionPointer( "glColorFragmentOp2ATI" );
		qglColorFragmentOp3ATI = (PFNGLCOLORFRAGMENTOP3ATIPROC)GLimp_ExtensionPointer( "glColorFragmentOp3ATI" );
		qglAlphaFragmentOp1ATI = (PFNGLALPHAFRAGMENTOP1ATIPROC)GLimp_ExtensionPointer( "glAlphaFragmentOp1ATI" );
		qglAlphaFragmentOp2ATI = (PFNGLALPHAFRAGMENTOP2ATIPROC)GLimp_ExtensionPointer( "glAlphaFragmentOp2ATI" );
		qglAlphaFragmentOp3ATI = (PFNGLALPHAFRAGMENTOP3ATIPROC)GLimp_ExtensionPointer( "glAlphaFragmentOp3ATI" );
		qglSetFragmentShaderConstantATI = (PFNGLSETFRAGMENTSHADERCONSTANTATIPROC)GLimp_ExtensionPointer( "glSetFragmentShaderConstantATI" );
	}

	// GL_ARB_fragment_shader
	qglConfig.fragmentShaderAvailable = R_CheckExtension( "GL_ARB_fragment_shader" );
	if ( qglConfig.fragmentShaderAvailable ) {
	    qglCreateShader = (PFNGLCREATESHADERPROC)GLimp_ExtensionPointer( "glCreateShader" );
	    qglShaderSource = (PFNGLSHADERSOURCEPROC)GLimp_ExtensionPointer( "glShaderSource" );
	    qglCompileShader = (PFNGLCOMPILESHADERPROC)GLimp_ExtensionPointer( "glCompileShader" );
	    qglCreateProgram = (PFNGLCREATEPROGRAMPROC)GLimp_ExtensionPointer( "glCreateProgram" );
	    qglAttachShader = (PFNGLATTACHSHADERPROC)GLimp_ExtensionPointer( "glAttachShader" );
	    qglLinkProgram = (PFNGLLINKPROGRAMPROC)GLimp_ExtensionPointer( "glLinkProgram" );
	    qglUseProgram = (PFNGLUSEPROGRAMPROC)GLimp_ExtensionPointer( "glUseProgram" );
	    qglDeleteShader = (PFNGLDELETESHADERPROC)GLimp_ExtensionPointer( "glDeleteShader" );
	    qglGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GLimp_ExtensionPointer( "glGetUniformLocation" );
	    qglUniform1f = (PFNGLUNIFORM1FPROC)GLimp_ExtensionPointer( "glUniform1f" );
	    qglGenFragmentShadersARB = (PFNGLGENFRAGMENTSHADERSARBPROC)GLimp_ExtensionPointer( "glCreateShaderProgramv" );
	    qglBindFragmentShaderARB = (PFNGLBINDFRAGMENTSHADERARBPROC)GLimp_ExtensionPointer( "glBindProgramPipeline" );
		qglDeleteFragmentShaderARB = (PFNGLDELETEPROGRAMSARBPROC)GLimp_ExtensionPointer( "glDeleteProgram" );
	}

	// ARB_vertex_buffer_object
	qglConfig.ARBVertexBufferObjectAvailable = R_CheckExtension( "GL_ARB_vertex_buffer_object" );
	if ( qglConfig.ARBVertexBufferObjectAvailable ) {
		qglBindBufferARB = (PFNGLBINDBUFFERARBPROC)GLimp_ExtensionPointer( "glBindBufferARB" );
		qglDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)GLimp_ExtensionPointer( "glDeleteBuffersARB" );
		qglGenBuffersARB = (PFNGLGENBUFFERSARBPROC)GLimp_ExtensionPointer( "glGenBuffersARB" );
		qglIsBufferARB = (PFNGLISBUFFERARBPROC)GLimp_ExtensionPointer( "glIsBufferARB" );
		qglBufferDataARB = (PFNGLBUFFERDATAARBPROC)GLimp_ExtensionPointer( "glBufferDataARB" );
		qglBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)GLimp_ExtensionPointer( "glBufferSubDataARB" );
		qglGetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARBPROC)GLimp_ExtensionPointer( "glGetBufferSubDataARB" );
		qglMapBufferARB = (PFNGLMAPBUFFERARBPROC)GLimp_ExtensionPointer( "glMapBufferARB" );
		qglUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)GLimp_ExtensionPointer( "glUnmapBufferARB" );
		qglGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)GLimp_ExtensionPointer( "glGetBufferParameterivARB" );
		qglGetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARBPROC)GLimp_ExtensionPointer( "glGetBufferPointervARB" );
	}

	// GL_ARB_framebuffer_object move this upnorth outside the function.
	PFNGLGENFRAMEBUFFERSPROC qglGenFramebuffers;
	PFNGLBINDFRAMEBUFFERPROC qglBindFramebuffer;
	PFNGLDELETEFRAMEBUFFERSPROC qglDeleteFramebuffers;
	PFNGLFRAMEBUFFERTEXTUREPROC qglFramebufferTexture;
	// ... include other necessary function pointers

	// Check if the extension is supported
	if ( qglConfig.framebufferObjectAvailable ) {
		qglConfig.framebufferObjectAvailable = R_CheckExtension( "GL_ARB_framebuffer_object" );
		// Get function pointers
		qglGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GLimp_ExtensionPointer( "glGenFramebuffers" );
		qglBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GLimp_ExtensionPointer( "glBindFramebuffer" );
		qglDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)GLimp_ExtensionPointer( "glDeleteFramebuffers" );
		qglFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)GLimp_ExtensionPointer( "glFramebufferTexture" );
		// ... obtain other necessary function pointers
	} else if ( R_CheckExtension( "GLEW_EXT_framebuffer_object" ) ) {
		qglGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &qglConfig.maxRenderbufferSize );
		qglGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &qglConfig.maxColorAttachments );
		common->Printf( "...using %s\n", "GL_EXT_framebuffer_object" );
	} else {
		common->Printf( "X..%s not found\n", "GL_EXT_framebuffer_object" );
	}

	//if ( qglConfig.qglVersion >= 3.0 || R_CheckExtension( "GL_ARB_texture_float" ) ) {}
	// Check if the extension is supported
	if ( qglConfig.qglVersion >= 3.0 || R_CheckExtension( "GL_ARB_vertex_array_object" ) ) {
	    // Get function pointers
		qglGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GLimp_ExtensionPointer( "glGenVertexArrays" );
		qglBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GLimp_ExtensionPointer( "glBindVertexArray" );
		qglDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)GLimp_ExtensionPointer( "glDeleteVertexArrays" );
		qglIsVertexArray = (PFNGLISVERTEXARRAYPROC)GLimp_ExtensionPointer( "glIsVertexArray" );
	}

	// ARB_vertex_program update but keeping for testing purposes. GLSL todo
	qglConfig.ARBVertexProgramAvailable = R_CheckExtension( "GL_ARB_vertex_program" );
	if ( qglConfig.ARBVertexProgramAvailable ) {
		qglVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)GLimp_ExtensionPointer( "glVertexAttribPointerARB" );
		qglEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)GLimp_ExtensionPointer( "glEnableVertexAttribArrayARB" );
		qglDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)GLimp_ExtensionPointer( "glDisableVertexAttribArrayARB" );
		qglProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)GLimp_ExtensionPointer( "glProgramStringARB" );
		qglBindProgramARB = (PFNGLBINDPROGRAMARBPROC)GLimp_ExtensionPointer( "glBindProgramARB" );
		qglGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)GLimp_ExtensionPointer( "glGenProgramsARB" );
		qglProgramEnvParameter4fvARB = (PFNGLPROGRAMENVPARAMETER4FVARBPROC)GLimp_ExtensionPointer( "glProgramEnvParameter4fvARB" );
		qglProgramLocalParameter4fvARB = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)GLimp_ExtensionPointer( "glProgramLocalParameter4fvARB" );
	}

	// ARB_fragment_program update but keeping for testing purposes. GLSL todo
	if ( r_inhibitFragmentProgram.GetBool() ) {
		qglConfig.ARBFragmentProgramAvailable = false;
	} else {
		qglConfig.ARBFragmentProgramAvailable = R_CheckExtension( "GL_ARB_fragment_program" );
		if ( qglConfig.ARBFragmentProgramAvailable ) {
			// these are the same as ARB_vertex_program
			qglProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)GLimp_ExtensionPointer( "glProgramStringARB" );
			qglBindProgramARB = (PFNGLBINDPROGRAMARBPROC)GLimp_ExtensionPointer( "glBindProgramARB" );
			qglProgramEnvParameter4fvARB = (PFNGLPROGRAMENVPARAMETER4FVARBPROC)GLimp_ExtensionPointer( "glProgramEnvParameter4fvARB" );
			qglProgramLocalParameter4fvARB = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)GLimp_ExtensionPointer( "glProgramLocalParameter4fvARB" );
		}
	}

	// check for minimum set
	if ( !qglConfig.multitextureAvailable || !qglConfig.textureEnvCombine || !qglConfig.useCubeMap || !qglConfig.envDot3 ) {
		common->Error( common->GetLanguageDict()->GetString( "#str_06780" ) );
	}

		// OpenGL 3.2 - GL_ARB_depth_clamp
	if ( qglConfig.qglVersion >= 3.2 || R_CheckExtension( "GL_ARB_depth_clamp" ) || R_CheckExtension( "GL_ARB_seamless_cube_map" ) ) {
		qglRefConfig.depthClamp = true;
		qglConfig.seamlessCubeMap = true;
		qglConfig.depthClamp = R_CheckExtension( "GL_ARB_depth_clamp" );
		qglConfig.seamlessCubeMap = R_CheckExtension( "GL_ARB_seamless_cube_map" );
		RB_Printf( result[qglConfig.seamlessCubeMap], extension );
		Printf( result[qglConfig.depthClamp], extension );
	} else {
		Printf( result[2], extension );
	}
 	// GL_EXT_depth_bounds_test
 	qglConfig.depthBoundsTest = R_CheckExtension( "EXT_depth_bounds_test" );
 	if ( qglConfig.depthBoundsTest ) {
		qglDepthBounds = (PFNGLDEPTHBOUNDSEXTPROC)GLimp_ExtensionPointer( "glDepthBounds" );
	} else {
 		qglDepthBoundsEXT = (PFNGLDEPTHBOUNDSEXTPROC)GLimp_ExtensionPointer( "glDepthBoundsEXT" );
	}

	// GLSL, core in OpenGL > 2.0
	//glConfig.glslAvailable = ( glConfig.glVersion >= 2.0f );

	// GL_ARB_uniform_buffer_object
	/*qglConfig.uniformBufferAvailable = R_CheckExtension( "GL_ARB_uniform_buffer_object" );
	if ( qglConfig.uniformBufferAvailable ) {
		qglGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint *)&qglConfig.uniformBufferOffsetAlignment );
		if ( qglConfig.uniformBufferOffsetAlignment < 256 ) {
			qglConfig.uniformBufferOffsetAlignment = 256;
		}

	}
	qglConfig.floatBufferAvailable = qglConfig.uniformBufferAvailable && ( qglConfig.driverType == GLDRV_OGL3X || qglConfig.driverType == GLDRV_OGL32_CORE_PROFILE || qglConfig.driverType == GLDRV_OGL32_PROFILE );*/
	// GL_ARB_occlusion_query
	qglConfig.occlusionQueryAvailable = GLEW_ARB_occlusion_query != 0;

	// GL_ARB_timer_query
	qglConfig.timerQueryAvailable = ( GLEW_ARB_timer_query != 0 || GLEW_EXT_timer_query != 0 ) && ( qglConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() ) && qglConfig.driverType != GLDRV_OGL_MESA;

	// GL_ARB_debug_output
	qglConfig.debugOutputAvailable = R_CheckExtension( "GLEW_ARB_debug_output" );
	if ( qglConfig.debugOutputAvailable ) {
		if ( r_debugContext.GetInteger() >= 1 ) {
			qglDebugMessageCallbackARB( ( GLDEBUGPROCARB ) DebugCallback, nullptr );
			if ( r_debugContext.GetInteger() >= 2 ) {
				// force everything to happen in the main thread instead of in a separate driver thread
				qglEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
			}
		if ( r_debugContext.GetInteger() >= 3 ) {
			// enable all the low priority messages
			qglDebugMessageControlARB( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, nullptr, true );
		}
	}
	// generate one global Vertex Array Object (VAO)
	qglGenVertexArrays( 1, &qglConfig.global_vao );
	qglBindVertexArray( qglConfig.global_vao );
}

static bool r_initialized = false;

/*
=============================
R_IsInitialized
=============================
*/
bool R_IsInitialized() {
	return r_initialized;
}

/*
====================
R_GetModeInfo

r_mode is normally a small non-negative integer that
looks resolutions up in a table, but if it is set to -1,
the values from r_customWidth, amd r_customHeight
will be used instead.
====================
*/
typedef struct vidmode_s {
    const char *description;
    int         width, height;
} vidmode_t;

vidmode_t r_vidModes[] = {
    { "Mode  0: 320x240",		320,	240 },
    { "Mode  1: 400x300",		400,	300 },
    { "Mode  2: 512x384",		512,	384 },
    { "Mode  3: 640x480",		640,	480 },
    { "Mode  4: 800x600",		800,	600 },
    { "Mode  5: 1024x768",		1024,	768 },
    { "Mode  6: 1152x864",		1152,	864 },
    { "Mode  7: 1280x1024",		1280,	1024 },
    { "Mode  8: 1600x1200",		1600,	1200 },
};
static int	s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

#if MACOS_X
bool R_GetModeInfo( int *width, int *height, int mode )
#else
static bool R_GetModeInfo( int *width, int *height, int mode )
#endif
{
	vidmode_t	*vm;

    if ( mode < -1 ) {
        return false;
	}
	if ( mode >= s_numVidModes ) {
		return false;
	}

	if ( mode == -1 ) {
		*width = r_customWidth.GetInteger();
		*height = r_customHeight.GetInteger();
		return true;
	}

	vm = &r_vidModes[mode];

	if ( width ) {
		*width  = vm->width;
	}
	if ( height ) {
		*height = vm->height;
	}

    return true;
}

/*
==================
R_InitOpenGL

This function is responsible for initializing a valid OpenGL subsystem
for rendering.  This is done by calling the system specific GLimp_Init,
which gives us a working OGL subsystem, then setting all necessary openGL
state, including images, vertex programs, and display lists.

Changes to the vertex cache size or smp state require a vid_restart.

If qglConfig.isInitialized is false, no rendering can take place, but
all renderSystem functions will still operate properly, notably the material
and model information functions.
==================
*/
void R_InitOpenGL( void ) {
	glimpParms_t	parms;

	common->Printf( "----- R_InitOpenGL -----\n" );

	if ( qglConfig.isInitialized ) {
		common->FatalError( "R_InitOpenGL called while active" );
	}

	// in case we had an error while doing a tiled rendering
	tr.viewportOffset[0] = 0;
	tr.viewportOffset[1] = 0;

	//
	// initialize OS specific portions of the renderSystem
	//
	for ( int i = 0; i < 2; i++ ) {
		// set the parameters we are trying
		R_GetModeInfo( &qglConfig.vidWidth, &qglConfig.vidHeight, r_mode.GetInteger() );

		parms.width = qglConfig.vidWidth;
		parms.height = qglConfig.vidHeight;
		parms.fullScreen = r_fullscreen.GetBool();
		parms.displayHz = r_displayRefresh.GetInteger();
		parms.multiSamples = r_multiSamples.GetInteger();
		parms.stereo = false;

		if ( GLimp_Init( parms ) ) {
			// it worked
			break;
		}

		if ( i == 1 ) {
			common->FatalError( "Unable to initialize OpenGL" );
		}

		// if we failed, set everything back to "safe mode"
		// and try again
		r_mode.SetInteger( 3 );
		r_fullscreen.SetInteger( 1 );
		r_displayRefresh.SetInteger( 0 );
		r_multiSamples.SetInteger( 0 );
	}

	// input and sound systems need to be tied to the new window
	Sys_InitInput();
	soundSystem->InitHW();

	// get our config strings
	qglConfig.vendorStr = (const char *)qglGetString( GL_VENDOR );
	qglConfig.rendererStr = (const char *)qglGetString( GL_RENDERER );
	qglConfig.versionStr = (const char *)qglGetString( GL_VERSION );
	qglConfig.shaderLangVerson = ( const char *)qglGetString( GL_SHADING_LANGUAGE_VERSION );
	qglConfig.extensionsStr = (const char *)qglGetString( GL_EXTENSIONS );
	if ( qglConfig.extensionsStr == nullptr ) {
		// As of OpenGL 3.2, qglGetStringi is required to obtain the available extensions
		//qglGetStringi = ( PFNGLGETSTRINGIPROC )GLimp_ExtensionPointer( "glGetStringi" );
		// Build the extensions string
		GLint numExtensions;
		qglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		extensionsStr.Clear();
		for ( int i = 0; i < numExtensions; i++ ) {
			extensionsStr.Append( (const char *)qglGetStringi( GL_EXTENSIONS, i ) );
			// the now deprecated glGetString method usaed to create a single string with each extension separated by a space
			if ( i < numExtensions - 1 ) {
				extensionsStr.Append( ' ' );
			}
		}
		qglConfig.extensionsStr = extensionsStr.c_str();
	}

	qglConfig.qglVersion = atof( qglConfig.version_string );
	qglConfig.qglslVersion = atof( qglConfig.shaderLangVerson );
	common->Printf( "OpenGL Version  : %3.1f\n", qglConfig.qglVersion );
	common->Printf( "OpenGL Vendor   : %s\n", qglConfig.vendor_string );
	common->Printf( "OpenGL Renderer : %s\n", qglConfig.rendererStr );
	common->Printf( "OpenGL GLSL     : %3.1f\n", qglConfig.qglslVersion );

	// OpenGL driver constants
	GLint temp;
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
	qglConfig.maxImageSize = temp;

	// stubbed or broken drivers may have reported 0...
	if ( qglConfig.maxImageSize <= 0 ) {
		qglConfig.maxImageSize = 256;
	}

	qglConfig.isInitialized = true;

	// recheck all the extensions (FIXME: this might be dangerous)
	R_CheckPortableExtensions();

	// parse our vertex and fragment programs, possibly disably support for
	// one of the paths if there was an error
	R_NV10_Init();
	R_NV20_Init();
	R_R200_Init();
	R_ARB2_Init();

	cmdSystem->AddCommand( "reloadARBprograms", R_ReloadARBPrograms_f, CMD_FL_RENDERER, "reloads ARB programs" );
	R_ReloadARBPrograms_f( anCommandArgs() );

	// allocate the vertex array range or vertex objects
	vertexCache.Init();

	// select which renderSystem we are going to use
	r_renderer.SetModified();
	tr.SetBackEndRenderer();

	// allocate the frame data, which may be more if smp is enabled
	R_InitFrameData();

	// Reset our gamma
	R_SetColorMappings();

#ifdef _WIN32
	static bool glCheck = false;
	if ( !glCheck && win32.osversion.dwMajorVersion == 6 ) {
		glCheck = true;
		if ( !anStr::Icmp( qglConfig.vendorStr, "Microsoft" ) && anStr::FindText( qglConfig.rendererStr, "OpenGL-D3D" ) != -1 ) {
			if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
				Sys_GrabMouseCursor( false );
			}
			int ret = MessageBox( nullptr, "Please install OpenGL drivers from your graphics hardware vendor to run " GAME_NAME ".\nYour OpenGL functionality is limited.",
				"Insufficient OpenGL capabilities", MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL );
			if ( ret == IDCANCEL ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
				cmdSystem->ExecuteCommandBuffer();
			}
			if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
			}
		}
	}
#endif
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
    GLenum	err;
    char	stack[64];

	if ( !r_ignoreGLErrors.GetBool() ) {
		//common->Printf( "GL_CheckErrors: %s\n", stack );
		return false;
	}

	// check for up to 10 errors pending
	bool error = false;

	for ( int i = 0; i < 10; i++ ) {
		err = qglGetError();
		if ( err == GL_NO_ERROR ) {
			break;
		}
		error = true;
		switch ( err ) {
			case GL_INVALID_ENUM:
				strncpy( stack, "GL_INVALID_ENUM" );
				break;
			case GL_INVALID_VALUE:
				strncpy( stack, "GL_INVALID_VALUE" );
				break;
			case GL_INVALID_OPERATION:
				strncpy( stack, "GL_INVALID_OPERATION" );
				break;
			case GL_STACK_OVERFLOW:
				strncpy( stack, "GL_STACK_OVERFLOW" );
				break;
			case GL_STACK_UNDERFLOW:
				strncpy( stack, "GL_STACK_UNDERFLOW" );
				break;
			case GL_OUT_OF_MEMORY:
				strncpy( stack, "GL_OUT_OF_MEMORY" );
				break;
			default:
				anStr::snPrintf( stack, sizeof( stack ), "%i", err );
				break;
		}
		common->Printf( "caught OpenGL error: %s in file %s line %i\n", stack, filename, line );
	}
	return error;
}

/*
=====================
R_ReloadSurface_f

Reload the material displayed by r_showSurfaceInfo
=====================
*/
static void R_ReloadSurface_f( const anCommandArgs &args ) {
	modelTrace_t mt;
	anVec3 start, end;

	// start far enough away that we don't hit the player model
	start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewAxis[0] * 16;
	end = start + tr.primaryView->renderView.viewAxis[0] * 1000.0f;
	if ( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false ) ) {
		return;
	}

	common->Printf( "Reloading %s\n", mt.material->GetName() );

	// reload the decl
	mt.material->base->Reload();

	// reload any images used by the decl
	mt.material->ReloadImages( false );
}

/*
==============
R_ListModes_f
==============
*/
static void R_ListModes_f( const anCommandArgs &args ) {
	int i;

	common->Printf( "\n" );
	for ( i = 0; i < s_numVidModes; i++ ) {
		common->Printf( "%s\n", r_vidModes[i].description );
	}
	common->Printf( "\n" );
}

/*
=============
R_TestImage_f

Display the given image centered on the screen.
testimage <number>
testimage <filename>
=============
*/
void R_TestImage_f( const anCommandArgs &args ) {
	int imageNum;

	if ( tr.testVideo ) {
		delete tr.testVideo;
		tr.testVideo = nullptr;
	}
	tr.testImage = nullptr;

	if ( args.Argc() != 2 ) {
		return;
	}

	if ( anStr::IsNumeric( args.Argv(1 ) ) ) {
		imageNum = atoi( args.Argv(1 ) );
		if ( imageNum >= 0 && imageNum < globalImages->images.Num() ) {
			tr.testImage = globalImages->images[imageNum];
		}
	} else {
		tr.testImage = globalImages->ImageFromFile( args.Argv( 1 ), TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT );
	}
}

static int R_QSortSurfaceAreas( const void *a, const void *b ) {
	const anMaterial	*ea, *eb;
	int	ac, bc;

	ea = *( anMaterial ** )a;
	if ( !ea->EverReferenced() ) {
		ac = 0;
	} else {
		ac = ea->GetSurfaceArea();
	}

	eb = *( anMaterial ** ) b;

	if ( !eb->EverReferenced() ) {
		bc = 0;
	} else {
		bc = eb->GetSurfaceArea();
	}

	if ( ac < bc ) {
		return -1;
	}
	if ( ac > bc ) {
		return 1;
	}

	return anStr::Icmp( ea->GetName(), eb->GetName() );
}

/*
===================
R_ReportSurfaceAreas_f

Prints a list of the materials sorted by surface area
===================
*/
void R_ReportSurfaceAreas_f( const anCommandArgs &args ) {
	int		i, count;
	anMaterial	**list;

	count = declManager->GetNumDecls( DECL_MATERIAL );
	list = ( anMaterial ** )_alloca( count * sizeof( *list ) );

	for ( i = 0; i < count; i++ ) {
		list[i] = ( anMaterial * )declManager->DeclByIndex( DECL_MATERIAL, i, false );
	}

	qsort( list, count, sizeof( list[0] ), R_QSortSurfaceAreas );

	// skip over ones with 0 area
	for ( i = 0; i < count; i++ ) {
		if ( list[i]->GetSurfaceArea() > 0 ) {
			break;
		}
	}

	for (; i < count; i++ ) {
		// report size in "editor blocks"
		int	blocks = list[i]->GetSurfaceArea() / 4096.0;
		common->Printf( "%7i %s\n", blocks, list[i]->GetName() );
	}
}

/*
================
R_ExtractTGA_f
================
*/
void R_ExtractTGA_f( const anCommandArgs &args ) {
	anStr relativePath;
	anStr extension;
	//anFileList *fileList;
	int imageNum;
	anImage	*img = nullptr;
	//anDxtDecoder dxt;

	if ( args.Argc() != 2 ) {
		common->Printf( "usage: ExtractTGA <image path or image number>\n" );
		return;
	}

	if ( anStr::IsNumeric( args.Argv( 1 ) ) ) {
		imageNum = atoi( args.Argv( 1 ) );
		if ( imageNum >= 0 && imageNum < globalImages->images.Num() ) {
			img = globalImages->images[imageNum];
		}
	} else {
		img = globalImages->ImageFromFile( args.Argv( 1 ), TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
	}
	if ( !img ) {
		common->Warning( "Image '%s' not found.\n", args.Argv( 1 ) );
		return;
	}
	common->Printf( "Saving image\n" );
	img->ActuallySaveImage();
}

/*
===================
R_ReportImageDuplication_f

Checks for images with the same hash value and does a better comparison
===================
*/
void R_ReportImageDuplication_f( const anCommandArgs &args ) {
	int	count = 0;

	common->Printf( "Images with duplicated contents:\n" );

	for ( int i = 0; i < globalImages->images.Num(); i++ ) {
		anImage	*image1 = globalImages->images[i];
		if ( image1->isPartialImage ) {
			// ignore background loading stubs
			continue;
		}
		if ( image1->generatorFunction ) {
			// ignore procedural images
			continue;
		}
		if ( image1->cubeFiles != CF_2D ) {
			// ignore cube maps
			continue;
		}
		if ( image1->defaulted ) {
			continue;
		}
		byte *data1;

		R_LoadImageProgram( image1->imgName, &data1, &w1, &h1, nullptr );

		for ( int j = 0; j < i; j++ ) {
			anImage *image2 = globalImages->images[j];
			if ( image2->isPartialImage || image2->generatorFunction ) {
				continue;
			}
			if ( image2->cubeFiles != CF_2D || image2->defaulted ) {
				continue;
			}
			if ( image1->imageHash != image2->imageHash ||  image2->uploadWidth != image1->uploadWidth || image2->uploadHeight != image1->uploadHeight ) {
				continue;
			}
			if ( !anStr::Icmp( image1->imgName, image2->imgName ) ) {
				// ignore same image-with-different-parms
				continue;
			}

			byte	*data2;
			int		w2, h2;

			R_LoadImageProgram( image2->imgName, &data2, &w2, &h2, nullptr );

			if ( w2 != w1 || h2 != h1 ) {
				R_StaticFree( data2 );
				continue;
			}

			if ( memcmp( data1, data2, w1*h1*4 ) ) {
				R_StaticFree( data2 );
				continue;
			}

			R_StaticFree( data2 );

			common->Printf( "%s == %s\n", image1->imgName.c_str(), image2->imgName.c_str() );
			session->UpdateScreen( true );
			count++;
			break;
		}

		R_StaticFree( data1 );
	}
	common->Printf( "%i / %i collisions\n", count, globalImages->images.Num() );
}

/*
==============================================================================

						THROUGHPUT BENCHMARKING

==============================================================================
*/

/*
================
R_RenderingFPS
================
*/
static float R_RenderingFPS( const renderView_t *renderView ) {
	qglFinish();

	int		start = Sys_Milliseconds();
	static const int SAMPLE_MSEC = 1000;
	int		end;
	int		count = 0;

	while ( 1 ) {
		// render
		renderSystem->BeginFrame( qglConfig.vidWidth, qglConfig.vidHeight );
		tr.primaryWorld->RenderScene( renderView );
		renderSystem->EndFrame( nullptr, nullptr );
		qglFinish();
		count++;
		end = Sys_Milliseconds();
		if ( end - start > SAMPLE_MSEC ) {
			break;
		}
	}

	float fps = count * 1000.0 / ( end - start );

	return fps;
}

/*
================
R_Benchmark_f
================
*/
void R_Benchmark_f( const anCommandArgs &args ) {
	if ( !tr.primaryView ) {
		common->Printf( "No primaryView for benchmarking\n" );
		return;
	}
	renderView_t view = tr.primaryRenderView;

	for ( int size = 100; size >= 10; size -= 10 ) {
		r_screenFraction.SetInteger( size );
		float fps = R_RenderingFPS( &view );
		int	kpix = qglConfig.vidWidth * qglConfig.vidHeight * ( size * 0.01 ) * ( size * 0.01 ) * 0.001;
		float mSec = 1000.0 / fps;
		common->Printf( "kpix: %4i  mSec:%5.1f fps:%5.1f\n", kpix, mSec, fps );
	}

	// enable r_singleTriangle 1 while r_screenFraction is still at 10
	r_singleTriangle.SetBool( 1 );
	float fps = R_RenderingFPS( &view );
	float mSec = 1000.0 / fps;
	common->Printf( "single tri  mSec:%5.1f fps:%5.1f\n", mSec, fps );
	r_singleTriangle.SetBool( 0 );
	r_screenFraction.SetInteger( 100 );

	// enable r_skipRenderContext 1
	r_skipRenderContext.SetBool( true );
	float fps = R_RenderingFPS( &view );
	float mSec = 1000.0 / fps;
	common->Printf( "no context  mSec:%5.1f fps:%5.1f\n", mSec, fps );
	r_skipRenderContext.SetBool( false );
}

/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
====================
R_ReadTiledPixels

Allows the rendering of an image larger than the actual window by
tiling it into window-sized chunks and rendering each chunk separately

If ref isn't specified, the full session UpdateScreen will be done.
====================
*/
void R_ReadTiledPixels( int width, int height, byte *buffer, renderView_t *ref = nullptr ) {
	// include extra space for OpenGL padding to word boundaries
	byte *temp = (byte *)R_StaticAlloc( ( qglConfig.vidWidth+3 ) * qglConfig.vidHeight * 3 );

	int	oldWidth = qglConfig.vidWidth;
	int oldHeight = qglConfig.vidHeight;

	tr.tiledViewport[0] = width;
	tr.tiledViewport[1] = height;

	// disable scissor, so we don't need to adjust all those rects
	r_useScissor.SetBool( false );

	for ( int xo = 0; xo < width; xo += oldWidth ) {
		for ( int yo = 0; yo < height; yo += oldHeight ) {
			tr.viewportOffset[0] = -xo;
			tr.viewportOffset[1] = -yo;
			if ( ref ) {
				tr.BeginFrame( oldWidth, oldHeight );
				tr.primaryWorld->RenderScene( ref );
				tr.EndFrame( nullptr, nullptr );
			} else {
				session->UpdateScreen();
			}

			int w = oldWidth;
			if ( xo + w > width ) {
				w = width - xo;
			}
			int h = oldHeight;
			if ( yo + h > height ) {
				h = height - yo;
			}

			qglReadBuffer( GL_FRONT );
			qglReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, temp );

			int	row = ( w * 3 + 3 ) & ~3;		// OpenGL pads to dword boundaries

			for ( int y = 0; y < h; y++ ) {
				memcpy( buffer + ( ( yo + y )* width + xo ) * 3,
					temp + y * row, w * 3 );
			}
		}
	}

	r_useScissor.SetBool( true );

	tr.viewportOffset[0] = 0;
	tr.viewportOffset[1] = 0;
	tr.tiledViewport[0] = 0;
	tr.tiledViewport[1] = 0;

	R_StaticFree( temp );

	qglConfig.vidWidth = oldWidth;
	qglConfig.vidHeight = oldHeight;
}

/*
==================
TakeScreenshot

Move to tr_imagefiles.c...

Will automatically tile render large screen shots if necessary
Downsample is the number of steps to mipmap the image before saving it
If ref == nullptr, session->updateScreen will be used
==================
*/
void anRenderSystemLocal::TakeScreenshot( int width, int height, const char *fileName, int blends, renderView_t *ref ) {
	//takingScreenshot = true;

	int	pix = width * height;

	byte *buffer = (byte *)R_StaticAlloc( pix*3 + 18 );
	memset ( buffer, 0, 18 );

	if ( blends <= 1 ) {
		R_ReadTiledPixels( width, height, buffer + 18, ref );
	} else {
		unsigned short *shortBuffer = (unsigned short *)R_StaticAlloc( pix*2*3 );
		memset ( shortBuffer, 0, pix*2*3);

		// enable anti-aliasing jitter
		r_jitter.SetBool( true );

		for ( int i = 0; i < blends; i++ ) {
			R_ReadTiledPixels( width, height, buffer + 18, ref );
			for ( int  j = 0; j < pix*3; j++ ) {
				shortBuffer[j] += buffer[18+j];
			}
		}

		// divide back to bytes
		for ( int i = 0; i < pix*3; i++ ) {
			buffer[18+i] = shortBuffer[i] / blends;
		}

		R_StaticFree( shortBuffer );
		r_jitter.SetBool( false );
	}

	// fill in the header (this is vertically flipped, which qglReadPixels emits)
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr
	int c = 18 + width * height * 3;
	for ( int i = 18; i < c; i += 3 ) {
		int temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	if ( strstr( fileName, "removeme" ) ) {
		fileSystem->WriteFile( fileName, buffer, c, "fs_cdpath" );
	} else {
		fileSystem->WriteFile( fileName, buffer, c );
	}

	R_StaticFree( buffer );

	takingScreenshot = false;
}

/*
==================
R_ScreenshotFilename

Returns a filename with digits appended
if we have saved a previous screenshot, don't scan
from the beginning, because recording demo avis can involve
thousands of shots
==================
*/
void R_ScreenshotFilename( int &lastNumber, const char *base, anStr &fileName ) {
	bool restrict = cvarSystem->GetCVarBool( "fs_restrict" );
	cvarSystem->SetCVarBool( "fs_restrict", false );

	lastNumber++;
	if ( lastNumber > 99999 ) {
		lastNumber = 99999;
	}
	for (; lastNumber < 99999; lastNumber++ ) {
		int	frac = lastNumber;

		int	a = frac / 10000;
		frac -= a*10000;
		int	b = frac / 1000;
		frac -= b*1000;
		int	c = frac / 100;
		frac -= c*100;
		int	d = frac / 10;
		frac -= d*10;
		int	e = frac;

		sprintf( fileName, "%s%i%i%i%i%i.tga", base, a, b, c, d, e );
		if ( lastNumber == 99999 ) {
			break;
		}
		int len = fileSystem->ReadFile( fileName, nullptr, nullptr );
		if ( len <= 0 ) {
			break;
		}
		// check again...
	}
	cvarSystem->SetCVarBool( "fs_restrict", restrict );
}

/*
==================
R_BlendedScreenShot

screenshot
screenshot [filename]
screenshot [width] [height]
screenshot [width] [height] [samples]
==================
*/
#define	MAX_BLENDS	256	// to keep the accumulation in shorts
void R_ScreenShot_f( const anCommandArgs &args ) {
	static int lastNumber = 0;
	anStr checkname;

	int width = qglConfig.vidWidth;
	int height = qglConfig.vidHeight;
	int	x = 0;
	int y = 0;
	int	blends = 0;

	switch ( args.Argc() ) {
	case 1:
		width = qglConfig.vidWidth;
		height = qglConfig.vidHeight;
		blends = 1;
		R_ScreenshotFilename( lastNumber, "screenshots/shot", checkname );
		break;
	case 2:
		width = qglConfig.vidWidth;
		height = qglConfig.vidHeight;
		blends = 1;
		checkname = args.Argv( 1 );
		break;
	case 3:
		width = atoi( args.Argv( 1 ) );
		height = atoi( args.Argv( 2 ) );
		blends = 1;
		R_ScreenshotFilename( lastNumber, "screenshots/shot", checkname );
		break;
	case 4:
		width = atoi( args.Argv( 1 ) );
		height = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
		if ( blends < 1 ) {
			blends = 1;
		}
		if ( blends > MAX_BLENDS ) {
			blends = MAX_BLENDS;
		}
		R_ScreenshotFilename( lastNumber, "screenshots/shot", checkname );
		break;
	default:
		common->Printf( "usage: screenshot\n       screenshot <filename>\n       screenshot <width> <height>\n       screenshot <width> <height> <blends>\n" );
		return;
	}

	// put the console away
	console->Close();

	tr.TakeScreenshot( width, height, checkname, blends, nullptr );

	common->Printf( "Wrote %s\n", checkname.c_str() );
}

/*
===============
R_StencilShot
Save out a screenshot showing the stencil buffer expanded by 16x range
===============
*/
void R_StencilShot( void ) {
	int	width = tr.GetScreenWidth();
	int	height = tr.GetScreenHeight();

	int	pix = width * height;

	int	c = pix * 3 + 18;
	byte *buffer = (byte *)Mem_Alloc( c );
	memset (buffer, 0, 18);

	byte *byteBuffer = (byte *)Mem_Alloc( pix );

	qglReadPixels( 0, 0, width, height, GL_STENCIL_INDEX , GL_UNSIGNED_BYTE, byteBuffer );

	for ( int i = 0; i < pix; i++ ) {
		buffer[18+i*3] =
		buffer[18+i*3+1] =
			//buffer[18+i*3+2] = ( byteBuffer[i] & 15 ) * 16;
		buffer[18+i*3+2] = byteBuffer[i];
	}

	// fill in the header (this is vertically flipped, which qglReadPixels emits)
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	fileSystem->WriteFile( "screenshots/stencilShot.tga", buffer, c, "fs_savepath" );

	Mem_Free( buffer );
	Mem_Free( byteBuffer );
}

/*
==================
R_EnvShot_f

envshot <basename>

Saves out env/<basename>_ft.tga, etc
==================
*/
void R_EnvShot_f( const anCommandArgs &args ) {
	anStr fullname;
	anMat3 axis[6];
	char *extensions[6] =  { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga",
		"_pz.tga", "_nz.tga" };

	if ( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 ) {
		common->Printf( "USAGE: envshot <basename> [size] [blends]\n" );
		return;
	}
	const char	*baseName = args.Argv( 1 );

	int blends = 1;
	if ( args.Argc() == 4 ) {
		int size = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
	} else if ( args.Argc() == 3 ) {
		int size = atoi( args.Argv( 2 ) );
		blends = 1;
	} else {
		int size = 256;
		blends = 1;
	}

	if ( !tr.primaryView ) {
		common->Printf( "No primary view.\n" );
		return;
	}

	viewDef_t primary = *tr.primaryView;

	memset( &axis, 0, sizeof( axis ) );
	axis[0][0][0] = 1;
	axis[0][1][2] = 1;
	axis[0][2][1] = 1;

	axis[1][0][0] = -1;
	axis[1][1][2] = -1;
	axis[1][2][1] = 1;

	axis[2][0][1] = 1;
	axis[2][1][0] = -1;
	axis[2][2][2] = -1;

	axis[3][0][1] = -1;
	axis[3][1][0] = -1;
	axis[3][2][2] = 1;

	axis[4][0][2] = 1;
	axis[4][1][0] = -1;
	axis[4][2][1] = 1;

	axis[5][0][2] = -1;
	axis[5][1][0] = 1;
	axis[5][2][1] = 1;

	for ( int i = 0; i < 6; i++ ) {
		renderView_t ref = primary.renderView;
		ref.x = ref.y = 0;
		ref.fov_x = ref.fov_y = 90;
		ref.width = qglConfig.vidWidth;
		ref.height = qglConfig.vidHeight;
		ref.viewAxis = axis[i];
		sprintf( fullname, "env/%s%s", baseName, extensions[i] );
		tr.TakeScreenshot( size, size, fullname, blends, &ref );
	}

	common->Printf( "Wrote %s, etc\n", fullname.c_str() );
}
//============================================================================

static anMat3 cubeAxis[6];

/*
==================
R_SampleCubeMap
==================
*/
void R_SampleCubeMap( const anVec3 &dir, int size, byte *buffers[6], byte result[4] ) {
	float	adir[3];

	adir[0] = fabs(dir[0] );
	adir[1] = fabs(dir[1] );
	adir[2] = fabs(dir[2] );

	if ( dir[0] >= adir[1] && dir[0] >= adir[2] ) {
		int axis = 0;
	} else if ( -dir[0] >= adir[1] && -dir[0] >= adir[2] ) {
		int axis = 1;
	} else if ( dir[1] >= adir[0] && dir[1] >= adir[2] ) {
		int axis = 2;
	} else if ( -dir[1] >= adir[0] && -dir[1] >= adir[2] ) {
		int axis = 3;
	} else if ( dir[2] >= adir[1] && dir[2] >= adir[2] ) {
		int axis = 4;
	} else {
		int axis = 5;
	}

	float fx = (dir * cubeAxis[axis][1] ) / (dir * cubeAxis[axis][0] );
	float fy = (dir * cubeAxis[axis][2] ) / (dir * cubeAxis[axis][0] );

	fx = -fx;
	fy = -fy;
	int x = size * 0.5 * (fx + 1 );
	int y = size * 0.5 * (fy + 1 );
	if ( x < 0 ) {
		x = 0;
	} else if ( x >= size ) {
		x = size-1;
	}
	if ( y < 0 ) {
		y = 0;
	} else if ( y >= size ) {
		y = size-1;
	}

	result[0] = buffers[axis][( y * size + x )*4+0];
	result[1] = buffers[axis][( y * size + x )*4+1];
	result[2] = buffers[axis][( y * size + x )*4+2];
	result[3] = buffers[axis][( y * size + x )*4+3];
}


void R_SampleCubeMap( const anVec3 &dir, int size, byte *buffers[6], byte result[4] ) {
    int axis = anMath::Max( dir.Begin(), dir.End() ) - dir.Begin();

    float fx = -( dir * cubeAxis[axis][1]) / ( dir * cubeAxis[axis][0] );
    float fy = -( dir * cubeAxis[axis][2]) / ( dir * cubeAxis[axis][0] );

    int x = anMath::Clamp( static_cast<int>( size * 0.5f * ( fx + 1 ) ), 0, size - 1 );
    int y = anMath::Clamp( static_cast<int>( size * 0.5f * ( fy + 1 ) ) , 0, size - 1 );

    int index = ( y * size + x ) * 4;
    memcpy( result, &buffers[axis][index], sizeof( byte ) * 4 );
}


void R_MakeAmbientMap_f( const anCommandArgs &args ) {
	const char *extensions[6] = { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga", "_pz.tga", "_nz.tga" };
	byte *buffers[6];
	int width, height;

	if ( args.Argc() != 2 && args.Argc() != 3 ) {
		common->Printf( "USAGE: ambientshot <basename> [size]\n" );
		return;
	}
	const char *baseName = args.Argv( 1 );

	int downSample = 0;
	if ( args.Argc() == 3 ) {
		int outSize = atoi( args.Argv( 2 ) );
	} else {
		int outSize = 32;
	}

	memset( &cubeAxis, 0, sizeof( cubeAxis ) );
	cubeAxis[0][0][0] = 1;
	cubeAxis[0][1][2] = 1;
	cubeAxis[0][2][1] = 1;

	cubeAxis[1][0][0] = -1;
	cubeAxis[1][1][2] = -1;
	cubeAxis[1][2][1] = 1;

	cubeAxis[2][0][1] = 1;
	cubeAxis[2][1][0] = -1;
	cubeAxis[2][2][2] = -1;

	cubeAxis[3][0][1] = -1;
	cubeAxis[3][1][0] = -1;
	cubeAxis[3][2][2] = 1;

	cubeAxis[4][0][2] = 1;
	cubeAxis[4][1][0] = -1;
	cubeAxis[4][2][1] = 1;

	cubeAxis[5][0][2] = -1;
	cubeAxis[5][1][0] = 1;
	cubeAxis[5][2][1] = 1;

	// read all of the images
	for ( int i = 0 ; i < 6 ; i++ ) {
		anStr fullname;
		common->Sprintf( fullname, "env/%s%s", baseName, extensions[i] );
		common->Printf( "loading %s\n", fullname.c_str() );
		session->UpdateScreen();
		R_LoadImage( fullname, &buffers[i], &width, &height, nullptr, true );
		if ( !buffers[i] ) {
			common->Printf( "failed.\n" );
			for ( int i-- ; i >= 0 ; i-- ) {
				Mem_Free( buffers[i] );
			}
			return;
		}
	}

	// resample with hemispherical blending
	int	samples = 1000;

	byte *outBuffer = (byte *)_alloca( outSize * outSize * 4 );

	for ( int map = 0 ; map < 2 ; map++ ) {
		for ( int i = 0 ; i < 6 ; i++ ) {
			for ( int x = 0 ; x < outSize ; x++ ) {
				for ( int y = 0 ; y < outSize ; y++ ) {
					float total[3];
					anVec3 dir = cubeAxis[i][0] + -( -1 + 2.0 *x/ ( outSize-1 ) ) * cubeAxis[i][1] + -( -1 + 2.0 *y/ ( outSize-1 ) ) * cubeAxis[i][2];
					dir.Normalize();
					total[0] = total[1] = total[2] = 0;
					float limit = map ? 0.95 : 0.25;		// small for specular, almost hemisphere for ambient
					for ( int s = 0 ; s < samples ; s++ ) {
						// pick a random direction vector that is inside the unit sphere but not behind dir,
						// which is a robust way to evenly sample a hemisphere
						while( 1 ) {
							for ( int j = 0 ; j < 3 ; j++ ) {
								anVec3 test[j] = -1 + 2 * ( rand() & 0x7fff )/( float )0x7fff;
							}
							if ( test.Length() > 1.0 ) {
								continue;
							}
							test.Normalize();
							if ( test * dir > limit ) {	// don't do a complete hemisphere
								break;
							}
						}
						byte result[4];
						R_SampleCubeMap( test, width, buffers, result );
						total[0] += result[0];
						total[1] += result[1];
						total[2] += result[2];
					}
					outBuffer[(y*outSize+x)*4+0] = total[0] / samples;
					outBuffer[(y*outSize+x)*4+1] = total[1] / samples;
					outBuffer[(y*outSize+x)*4+2] = total[2] / samples;
					outBuffer[(y*outSize+x)*4+3] = 255;
				}
			}

			if ( map == 0 ) {
				common->Sprintf( fullname, "env/%s_amb%s", baseName, extensions[i] );
			} else {
				common->Sprintf( fullname, "env/%s_spec%s", baseName, extensions[i] );
			}
			common->Printf( "writing %s\n", fullname.c_str() );
			session->UpdateScreen();
			R_WriteTGA( fullname, outBuffer, outSize, outSize );
		}
	}

	for ( int i = 0 ; i < 6 ; i++ ) {
		if ( buffers[i] ) {
			Mem_Free( buffers[i] );
		}
	}
}
/*
==================
R_MakeAmbientMap_f

R_MakeAmbientMap_f <basename> [size]

Saves out env/<basename>_amb_ft.tga, etc
==================
*/
void R_MakeAmbientMap_f( const anCommandArgs &args ) {
	anStr fullname;
	char *extensions[6] =  { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga", "_pz.tga", "_nz.tga" };

	if ( args.Argc() != 2 && args.Argc() != 3 ) {
		common->Printf( "USAGE: ambientshot <basename> [size]\n" );
		return;
	}
	const char *bbaseName = args.Argv( 1 );

	intdownSample = 0;
	if ( args.Argc() == 3 ) {
		int outSize = atoi( args.Argv( 2 ) );
	} else {
		int outSize = 32;
	}

	memset( &cubeAxis, 0, sizeof( cubeAxis ) );
	cubeAxis[0][0][0] = 1;
	cubeAxis[0][1][2] = 1;
	cubeAxis[0][2][1] = 1;

	cubeAxis[1][0][0] = -1;
	cubeAxis[1][1][2] = -1;
	cubeAxis[1][2][1] = 1;

	cubeAxis[2][0][1] = 1;
	cubeAxis[2][1][0] = -1;
	cubeAxis[2][2][2] = -1;

	cubeAxis[3][0][1] = -1;
	cubeAxis[3][1][0] = -1;
	cubeAxis[3][2][2] = 1;

	cubeAxis[4][0][2] = 1;
	cubeAxis[4][1][0] = -1;
	cubeAxis[4][2][1] = 1;

	cubeAxis[5][0][2] = -1;
	cubeAxis[5][1][0] = 1;
	cubeAxis[5][2][1] = 1;

	byte *buffers[6];
	int width, height;

	// read all of the images
	for ( int i = 0; i < 6; i++ ) {
		sprintf( fullname, "env/%s%s", baseName, extensions[i] );
		common->Printf( "loading %s\n", fullname.c_str() );
		session->UpdateScreen();
		R_LoadImage( fullname, &buffers[i], &width, &height, nullptr, true );
		if ( !buffers[i] ) {
			common->Printf( "failed.\n" );
			for ( int i--; i >= 0; i-- ) {
				Mem_Free( buffers[i] );
			}
			return;
		}
	}

	// resample with hemispherical blending
	int	samples = 1000;

	byte *outBuffer = (byte *)_alloca( outSize * outSize * 4 );

	for ( int map = 0; map < 2; map++ ) {
		for ( int i = 0; i < 6; i++ ) {
			for ( int x = 0; x < outSize; x++ ) {
				for ( int y = 0; y < outSize; y++ ) {
					float total[3];

					anVec3 dir = cubeAxis[i][0] + -( -1 + 2.0f*x/( outSize-1 ) ) * cubeAxis[i][1] + -( -1 + 2.0f*y/( outSize-1 ) ) * cubeAxis[i][2];
					dir.Normalize();
					total[0] = total[1] = total[2] = 0;
	//samples = 1;
					float limit = map ? 0.95f : 0.25f;		// small for specular, almost hemisphere for ambient

					for ( int s = 0; s < samples; s++ ) {
						// pick a random direction vector that is inside the unit sphere but not behind dir,
						// which is a robust way to evenly sample a hemisphere
						anVec3	test;
						while ( 1 ) {
							for ( int j = 0; j < 3; j++ ) {
								test[j] = -1 + 2 * (rand()&0x7fff)/( float )0x7fff;
							}
							if ( test.Length() > 1.0f ) {
								continue;
							}
							test.Normalize();
							if ( test * dir > limit ) {	// don't do a complete hemisphere
								break;
							}
						}
						byte	result[4];
	//test = dir;
						R_SampleCubeMap( test, width, buffers, result );
						total[0] += result[0];
						total[1] += result[1];
						total[2] += result[2];
					}
					outBuffer[(y*outSize+x)*4+0] = total[0] / samples;
					outBuffer[(y*outSize+x)*4+1] = total[1] / samples;
					outBuffer[(y*outSize+x)*4+2] = total[2] / samples;
					outBuffer[(y*outSize+x)*4+3] = 255;
				}
			}

			if ( map == 0 ) {
				sprintf( fullname, "env/%s_amb%s", baseName, extensions[i] );
			} else {
				sprintf( fullname, "env/%s_spec%s", baseName, extensions[i] );
			}
			common->Printf( "writing %s\n", fullname.c_str() );
			session->UpdateScreen();
			R_WriteTGA( fullname, outBuffer, outSize, outSize );
		}
	}

	for ( i = 0; i < 6; i++ ) {
		if ( buffers[i] ) {
			Mem_Free( buffers[i] );
		}
	}
}

void R_TransformCubemap( const char *orgDirection[6], const char *orgDir, const char *destDirection[6], const char *destDir, const char *baseName ) {
	anStr	 fullname;
	bool        errorInOriginalImages = false;
	byte *		buffers[6];
	int			width = 0, height = 0;

	for ( int i = 0; i < 6; i++ ) {
		fullname.Format( "%s/%s%s.%s", orgDir, baseName, orgDirection[i], fileExten [TGA] );
		common->Printf( "loading %s\n", fullname.c_str() );
		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );
		R_LoadImage( fullname, &buffers[i], &width, &height, nullptr, true );
		// check for errors in the buffer
		if ( !buffers[i] ) {
			common->Printf( "failed.\n" );
			errorInOriginalImages = true;
		} else if ( width != height ) {
			common->Printf( "wrong size pal!\n\n\nget your shit together and set the size according to your images!\n\n\ninept programmers are inept!\n" );
			errorInOriginalImages = true;
		} else {
			errorInOriginalImages = false;
		}

		if ( errorInOriginalImages ) {
			errorInOriginalImages = false;
			for ( int i--; i >= 0; i-- ) {
				// clean up every buffer from this stage down
				Mem_Free( buffers[i] );
			}

			return;
		}

		// apply rotations and flips
		R_ApplyCubeMapTransforms( i, buffers[i], width );

		// save the images with the appropiate skybox naming convention
		fullname.Format( "%s/%s/%s%s.%s", destDir, baseName, baseName, destDirection[i], fileExten [TGA] );
		common->Printf( "writing %s\n", fullname.c_str() );
		common->UpdateScreen( false );
		R_WriteTGA( fullname, buffers[i], width, width );
	}

	for ( int i = 0; i < 6; i++ ) {
		if ( buffers[i] ) {
			Mem_Free( buffers[i] );
		}
	}
}

/*
==================
R_TransformEnvToSkybox_f

R_TransformEnvToSkybox_f <basename>

transforms env textures (of the type px, py, pz, nx, ny, nz)
to skybox textures ( forward, back, left, right, up, down)
==================
*/
void R_TransformEnvToSkybox_f( const anCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "USAGE: envToSky <basename>\n" );
		return;
	}

	R_TransformCubemap( envDirection, "env", skyDirection, "skybox", args.Argv( 1 ) );
}

/*
==================
R_TransformSkyboxToEnv_f

R_TransformSkyboxToEnv_f <basename>

transforms skybox textures ( forward, back, left, right, up, down)
to env textures (of the type px, py, pz, nx, ny, nz)
==================
*/

void R_TransformSkyboxToEnv_f( const anCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "USAGE: skyToEnv <basename>\n" );
		return;
	}

	R_TransformCubemap( skyDirection, "skybox", envDirection, "env", args.Argv( 1 ) );
}

//============================================================================

/*
================
GfxInfo_f
================
*/
void GfxInfo_f( const anCommandArgs &args ) {
	const char *fsstrings[] = {"windowed""fullscreen" };
	common->Printf( "CPU: %s\n", Sys_GetProcessorString() );

	common->Printf( "\nGL_VENDOR: %s\n", qglConfig.vendorStr );
	common->Printf( "GL_RENDERER: %s\n", qglConfig.rendererStr );
	common->Printf( "GL_VERSION: %s\n", qglConfig.versionStr );
	common->Printf( "GL_EXTENSIONS: %s\n", qglConfig.extensionsStr );
	if ( qglConfig.wglExtensionsStr ) {
		common->Printf( "WGL_EXTENSIONS: %s\n", qglConfig.wglExtensionsStr );
	}
	common->Printf( "GL_MAX_TEXTURE_SIZE: %d\n", qglConfig.maxImageSize );
	common->Printf( "GL_MAX_TEXTURE_UNITS_ARB: %d\n", qglConfig.maxImageUnits );
	common->Printf( "GL_MAX_TEXTURE_COORDS_ARB: %d\n", qglConfig.maxImageCoords );
	common->Printf( "GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %d\n", qglConfig.maxTextureImageUnits );
	common->Printf( "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", qglConfig.colorBits, qglConfig.depthBits, qglConfig.stencilBits );
	common->Printf( "MODE: %d, %d x %d %s hz:", r_mode.GetInteger(), qglConfig.vidWidth, qglConfig.vidHeight, fsstrings[r_fullscreen.GetBool()] );

	common->Printf( "%i multisamples\n", qglConfig.multisamples );

	if ( qglConfig.displayFrequency ) {
		common->Printf( "%d\n", qglConfig.displayFrequency );
	} else {
		common->Printf( "N/A\n" );
	}

	const char *active[2] = { "", " (ACTIVE)" };
	common->Printf( "ARB path ENABLED%s\n", active[tr.backEndRenderer == BE_ARB] );

	if ( qglConfig.allowNV10Path ) {
		common->Printf( "NV10 path ENABLED%s\n", active[tr.backEndRenderer == BE_NV10] );
	} else {
		common->Printf( "NV10 path disabled\n" );
	}

	if ( qglConfig.allowNV20Path ) {
		common->Printf( "NV20 path ENABLED%s\n", active[tr.backEndRenderer == BE_NV20] );
	} else {
		common->Printf( "NV20 path disabled\n" );
	}

	if ( qglConfig.allowR200Path ) {
		common->Printf( "R200 path ENABLED%s\n", active[tr.backEndRenderer == BE_R200] );
	} else {
		common->Printf( "R200 path disabled\n" );
	}

	if ( qglConfig.allowARB2Path ) {
		common->Printf( "ARB2 path ENABLED%s\n", active[tr.backEndRenderer == BE_ARB2] );
	} else {
		common->Printf( "ARB2 path disabled\n" );
	}

	common->Printf( "-------\n" );

	if ( r_finish.GetBool() ) {
		common->Printf( "Forcing qglFinish\n" );
	} else {
		common->Printf( "qglFinish not forced\n" );
	}

#ifdef _WIN32
// WGL_EXT_swap_interval
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) ( int interval );
extern	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
	if ( r_swapInterval.GetInteger() && wglSwapIntervalEXT ) {
		common->Printf( "Forcing swapInterval %i\n", r_swapInterval.GetInteger() );
	} else {
		common->Printf( "swapInterval not forced\n" );
	}
#endif

	bool tss = qglConfig.isDoubleEdgeStencil || qglConfig.atiDoubleEdgeStencil;

	if ( !r_useTwoSidedStencil.GetBool() && tss ) {
		common->Printf( "Two sided stencil available but disabled\n" );
	} else if ( !tss ) {
		common->Printf( "Two sided stencil not available\n" );
	} else if ( tss ) {
		common->Printf( "Using two sided stencil\n" );
	}

	if ( vertexCache.IsFast() ) {
		common->Printf( "Vertex cache is fast\n" );
	} else {
		common->Printf( "Vertex cache is SLOW\n" );
	}
	//if ( qglConfig.gpuSkinningAvailable ) {
		//common->Printf( S_COLOR_GREEN "GPU skeletal animation available\n" );
	//Color/} else {
		//common->Printf( S_COLOR_RED "GPU skeletal animation not available ( slower CPU path active)\n" );
	//}
}

/*
=================
R_VidRestart_f
=================
*/
void R_VidRestart_f( const anCommandArgs &args ) {
	// if OpenGL isn't started, do nothing
	if ( !qglConfig.isInitialized ) {
		return;
	}

	bool full = true;
	bool forceWindow = false;
	for ( int i = 1; i < args.Argc(); i++ ) {
		if ( anStr::Icmp( args.Argv( i ), "partial" ) == 0 ) {
			full = false;
			continue;
		}
		if ( anStr::Icmp( args.Argv( i ), "windowed" ) == 0 ) {
			forceWindow = true;
			continue;
		}
	}

	// this could take a while, so give them the cursor back ASAP
	Sys_GrabMouseCursor( false );

	// dump ambient caches
	renderModelManager->FreeModelVertexCaches();

	// free any current world interaction surfaces and vertex caches
	R_FreeDerivedData();

	// make sure the defered frees are actually freed
	R_ToggleSmpFrame();
	R_ToggleSmpFrame();

	// free the vertex caches so they will be regenerated again
	vertexCache.PurgeAll();

	// sound and input are tied to the window we are about to destroy
	if ( full ) {
		// free all of our texture numbers
		//soundSystem->ShutdownHW();
		Sys_ShutdownInput();
		globalImages->PurgeAllImages();
		// free the context and close the window
		GLimp_Shutdown();
		qglConfig.isInitialized = false;

		// create the new context and vertex cache
		bool latch = cvarSystem->GetCVarBool( "r_fullscreen" );
		if ( forceWindow ) {
			cvarSystem->SetCVarBool( "r_fullscreen", false );
		}
		R_InitOpenGL();
		cvarSystem->SetCVarBool( "r_fullscreen", latch );

		// regenerate all images
		globalImages->ReloadAllImages();
	} else {
		glimpParms_t	parms;
		parms.width = qglConfig.vidWidth;
		parms.height = qglConfig.vidHeight;
		parms.fullScreen = ( forceWindow ) ? false : r_fullscreen.GetBool();
		parms.displayHz = r_displayRefresh.GetInteger();
		parms.multiSamples = r_multiSamples.GetInteger();
		parms.stereo = false;
		GLimp_SetScreenParms( parms );
	}

	// make sure the regeneration doesn't use anything no longer valid
	tr.viewCount++;
	tr.viewDef = nullptr;

	// regenerate all necessary interactions
	R_RegenerateWorld_f( anCommandArgs() );

	// check for problems
	inr err = qglGetError();
	if ( err != GL_NO_ERROR ) {
		common->Printf( "glGetError() = 0x%x\n", err );
	}

	// start sound playing again
	//soundSystem->SetMute( false );
}

/*
=================
R_InitMaterials
=================
*/
void R_InitMaterials( void ) {
	tr.defaultMaterial = declManager->FindMaterial( "_default", false );
	if ( !tr.defaultMaterial ) {
		common->FatalError( "_default material not found" );
	}
	declManager->FindMaterial( "_default", false );

	// needed by R_DeriveLightData
	declManager->FindMaterial( "lights/defaultPointLight" );
	declManager->FindMaterial( "lights/defaultProjectedLight" );
	declManager->whiteMaterial = declManager->FindMaterial( "_white" );
	declManager->charSetMaterial = declManager->FindMaterial( "textures/bigchars" );
}

/*
=================
R_SizeUp_f

Keybinding command
=================
*/
static void R_SizeUp_f( const anCommandArgs &args ) {
	if ( r_screenFraction.GetInteger() + 10 > 100 ) {
		r_screenFraction.SetInteger( 100 );
	} else {
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() + 10 );
	}
}

/*
=================
R_SizeDown_f

Keybinding command
=================
*/
static void R_SizeDown_f( const anCommandArgs &args ) {
	if ( r_screenFraction.GetInteger() - 10 < 10 ) {
		r_screenFraction.SetInteger( 10 );
	} else {
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() - 10 );
	}
}

/*
===============
TouchGui_f

this is called from the main thread
===============
*/
void R_TouchGui_f( const anCommandArgs &args ) {
	const char	*gui = args.Argv( 1 );

	if ( !gui[0] ) {
		common->Printf( "USAGE: touchGui <guiName>\n" );
		return;
	}

	common->Printf( "touchGui %s\n", gui );
	session->UpdateScreen();
	uiManager->Touch( gui );
}

/*
=================
R_InitCvars
=================
*/
void R_InitCvars( void ) {
	// update latched cvars here
}

/*
=================
R_InitCommands
=================
*/
void R_InitCommands( void ) {
	cmdSystem->AddCommand( "MakeMegaTexture", idMegaTexture::MakeMegaTexture_f, CMD_FL_RENDERER|CMD_FL_CHEAT, "processes giant images" );
	cmdSystem->AddCommand( "sizeUp", R_SizeUp_f, CMD_FL_RENDERER, "makes the rendered view larger" );
	cmdSystem->AddCommand( "sizeDown", R_SizeDown_f, CMD_FL_RENDERER, "makes the rendered view smaller" );
	cmdSystem->AddCommand( "reloadGuis", R_ReloadGuis_f, CMD_FL_RENDERER, "reloads guis" );
	cmdSystem->AddCommand( "listGuis", R_ListGuis_f, CMD_FL_RENDERER, "lists guis" );
	cmdSystem->AddCommand( "touchGui", R_TouchGui_f, CMD_FL_RENDERER, "touches a gui" );
	cmdSystem->AddCommand( "screenshot", R_ScreenShot_f, CMD_FL_RENDERER, "takes a screenshot" );
	cmdSystem->AddCommand( "envshot", R_EnvShot_f, CMD_FL_RENDERER, "takes an environment shot" );
	cmdSystem->AddCommand( "makeAmbientMap", R_MakeAmbientMap_f, CMD_FL_RENDERER|CMD_FL_CHEAT, "makes an ambient map" );
	cmdSystem->AddCommand( "benchmark", R_Benchmark_f, CMD_FL_RENDERER, "benchmark" );
	cmdSystem->AddCommand( "gfxInfo", GfxInfo_f, CMD_FL_RENDERER, "show graphics info" );
	cmdSystem->AddCommand( "modulateLights", R_ModulateLights_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "modifies shader parms on all lights" );
	cmdSystem->AddCommand( "testImage", R_TestImage_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given image centered on screen", arcCmdSystem::ArgCompletion_ImageName );
	cmdSystem->AddCommand( "testVideo", R_TestVideo_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given cinematic", arcCmdSystem::ArgCompletion_VideoName );
	cmdSystem->AddCommand( "reportSurfaceAreas", R_ReportSurfaceAreas_f, CMD_FL_RENDERER, "lists all used materials sorted by surface area" );
	cmdSystem->AddCommand( "reportImageDuplication", R_ReportImageDuplication_f, CMD_FL_RENDERER, "checks all referenced images for duplications" );
	cmdSystem->AddCommand( "regenerateWorld", R_RegenerateWorld_f, CMD_FL_RENDERER, "regenerates all interactions" );
	cmdSystem->AddCommand( "showInteractionMemory", R_ShowInteractionMemory_f, CMD_FL_RENDERER, "shows memory used by interactions" );
	cmdSystem->AddCommand( "showTriSurfMemory", R_ShowTriSurfMemory_f, CMD_FL_RENDERER, "shows memory used by triangle surfaces" );
	cmdSystem->AddCommand( "vid_restart", R_VidRestart_f, CMD_FL_RENDERER, "restarts renderSystem" );
	cmdSystem->AddCommand( "listRenderEntityDefs", R_ListRenderEntityDefs_f, CMD_FL_RENDERER, "lists the entity defs" );
	cmdSystem->AddCommand( "listRenderLightDefs", R_ListRenderLightDefs_f, CMD_FL_RENDERER, "lists the light defs" );
	cmdSystem->AddCommand( "listModes", R_ListModes_f, CMD_FL_RENDERER, "lists all video modes" );
	cmdSystem->AddCommand( "reloadSurface", R_ReloadSurface_f, CMD_FL_RENDERER, "reloads the decl and images for selected surface" );
}

/*
===============
anRenderSystemLocal::Clear
===============
*/
void anRenderSystemLocal::Clear( void ) {
	registered = false;
	frameCount = 0;
	viewCount = 0;
	staticAllocCount = 0;
	frameShaderTime = 0.0f;
	viewportOffset[0] = 0;
	viewportOffset[1] = 0;
	tiledViewport[0] = 0;
	tiledViewport[1] = 0;
	backEndRenderer = BE_BAD;
	backEndRendererHasVertexPrograms = false;
	backEndRendererMaxLight = 1.0f;
	ambientLightVector.Zero();
	sortOffset = 0;
	worlds.Clear();
	primaryWorld = nullptr;
	memset( &primaryRenderView, 0, sizeof( primaryRenderView ) );
	primaryView = nullptr;
	defaultMaterial = nullptr;
	testImage = nullptr;
	ambientCubeImage = nullptr;
	viewDef = nullptr;
	memset( &pc, 0, sizeof( pc ) );
	memset( &lockSurfacesCmd, 0, sizeof( lockSurfacesCmd ) );
	memset( &identitySpace, 0, sizeof( identitySpace ) );
	logFile = nullptr;
	stencilIncr = 0;
	stencilDecr = 0;
	memset( renderCrops, 0, sizeof( renderCrops ) );
	currentRenderCrop = 0;
	guiRecursionLevel = 0;
	guiModel = nullptr;
	memset( gammaTable, 0, sizeof( gammaTable ) );
	takingScreenshot = false;
	if ( unitSquareTriangles != nullptr ) {
		Mem_Free( unitSquareTriangles );
		unitSquareTriangles = nullptr;
	}

	if ( zeroOneCubeTriangles != nullptr ) {
		Mem_Free( zeroOneCubeTriangles );
		zeroOneCubeTriangles = nullptr;
	}

	if ( testImageTriangles != nullptr ) {
		Mem_Free( testImageTriangles );
		testImageTriangles = nullptr;
	}
	//if ( hudTriangles != nullptr ) {
		//Mem_Free( hudTriangles );
		//hudTriangles = nullptr;
	//}
}

void R_InitMaterials() {
	tr.defaultMaterial = declManager->FindMaterial( "_default", false );
	if ( !tr.defaultMaterial ) {
		common->FatalError( "_default material not found" );
	}
	tr.defaultPointLight = declManager->FindMaterial( "lights/defaultPointLight" );
	tr.defaultProjectedLight = declManager->FindMaterial( "lights/defaultProjectedLight" );
	tr.whiteMaterial = declManager->FindMaterial( "_white" );
	tr.charSetMaterial = declManager->FindMaterial( "textures/bigchars" );
}

/*
=============
R_MakeFullScreenTris
=============
*/
static srfTriangles_t* R_MakeFullScreenTris() {
	// copy verts and indexes
	srfTriangles_t* tri = ( srfTriangles_t*)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = (anDrawVert *)Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	anDrawVert* verts = tri->verts;

	triIndex_t tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy( tri->indexes, tempIndexes, indexSize );

	verts[0].xyz[0] = -1.0f;
	verts[0].xyz[1] = 1.0f;
	verts[0].SetTexCoord( 0.0f, 1.0f );

	verts[1].xyz[0] = 1.0f;
	verts[1].xyz[1] = 1.0f;
	verts[1].SetTexCoord( 1.0f, 1.0f );

	verts[2].xyz[0] = 1.0f;
	verts[2].xyz[1] = -1.0f;
	verts[2].SetTexCoord( 1.0f, 0.0f );

	verts[3].xyz[0] = -1.0f;
	verts[3].xyz[1] = -1.0f;
	verts[3].SetTexCoord( 0.0f, 0.0f );

	for( int i = 0; i < 4; i++ ) {
		verts[i].SetColor( 0xffffffff );
	}

	return tri;
}
/*
=============
R_MakeZeroOneCubeTris
=============
*/
static srfTriangles_t* R_MakeZeroOneCubeTris() {
	srfTriangles_t* tri = ( srfTriangles_t *)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numVerts = 8;
	tri->numIndexes = 36;

	const int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	const int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	const int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	const int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = ( anDrawVert* )Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	anDrawVert *verts = tri->verts;

	const float low = 0.0f;
	const float high = 1.0f;

	anVec3 center( 0.0f );
	anVec3 mx( low, 0.0f, 0.0f );
	anVec3 px( high, 0.0f, 0.0f );
	anVec3 my( 0.0f,  low, 0.0f );
	anVec3 py( 0.0f, high, 0.0f );
	anVec3 mz( 0.0f, 0.0f,  low );
	anVec3 pz( 0.0f, 0.0f, high );

	verts[0].xyz = center + mx + my + mz;
	verts[1].xyz = center + px + my + mz;
	verts[2].xyz = center + px + py + mz;
	verts[3].xyz = center + mx + py + mz;
	verts[4].xyz = center + mx + my + pz;
	verts[5].xyz = center + px + my + pz;
	verts[6].xyz = center + px + py + pz;
	verts[7].xyz = center + mx + py + pz;

	// bottom
	tri->indexes[ 0 * 3 + 0] = 2;
	tri->indexes[ 0 * 3 + 1] = 3;
	tri->indexes[ 0 * 3 + 2] = 0;
	tri->indexes[ 1 * 3 + 0] = 1;
	tri->indexes[ 1 * 3 + 1] = 2;
	tri->indexes[ 1 * 3 + 2] = 0;
	// back
	tri->indexes[ 2 * 3 + 0] = 5;
	tri->indexes[ 2 * 3 + 1] = 1;
	tri->indexes[ 2 * 3 + 2] = 0;
	tri->indexes[ 3 * 3 + 0] = 4;
	tri->indexes[ 3 * 3 + 1] = 5;
	tri->indexes[ 3 * 3 + 2] = 0;
	// left
	tri->indexes[ 4 * 3 + 0] = 7;
	tri->indexes[ 4 * 3 + 1] = 4;
	tri->indexes[ 4 * 3 + 2] = 0;
	tri->indexes[ 5 * 3 + 0] = 3;
	tri->indexes[ 5 * 3 + 1] = 7;
	tri->indexes[ 5 * 3 + 2] = 0;
	// right
	tri->indexes[ 6 * 3 + 0] = 1;
	tri->indexes[ 6 * 3 + 1] = 5;
	tri->indexes[ 6 * 3 + 2] = 6;
	tri->indexes[ 7 * 3 + 0] = 2;
	tri->indexes[ 7 * 3 + 1] = 1;
	tri->indexes[ 7 * 3 + 2] = 6;
	// front
	tri->indexes[ 8 * 3 + 0] = 3;
	tri->indexes[ 8 * 3 + 1] = 2;
	tri->indexes[ 8 * 3 + 2] = 6;
	tri->indexes[ 9 * 3 + 0] = 7;
	tri->indexes[ 9 * 3 + 1] = 3;
	tri->indexes[ 9 * 3 + 2] = 6;
	// top
	tri->indexes[10 * 3 + 0] = 4;
	tri->indexes[10 * 3 + 1] = 7;
	tri->indexes[10 * 3 + 2] = 6;
	tri->indexes[11 * 3 + 0] = 5;
	tri->indexes[11 * 3 + 1] = 4;
	tri->indexes[11 * 3 + 2] = 6;

	for( int i = 0 ; i < 4 ; i++ ) {
		verts[i].SetColor( 0xffffffff );
	}

	return tri;
}

/*
================
R_MakeTestImageTriangles

Initializes the Test Image Triangles
================
*/
srfTriangles_t* R_MakeTestImageTriangles() {
	srfTriangles_t *tri = ( srfTriangles_t *)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = (anrawVert *)Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	ALIGNTYPE16 triIndex_t tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy( tri->indexes, tempIndexes, indexSize );

	anDrawVert* tempVerts = tri->verts;
	tempVerts[0].xyz[0] = 0.0f;
	tempVerts[0].xyz[1] = 0.0f;
	tempVerts[0].xyz[2] = 0;
	tempVerts[0].SetTexCoord( 0.0, 0.0f );

	tempVerts[1].xyz[0] = 1.0f;
	tempVerts[1].xyz[1] = 0.0f;
	tempVerts[1].xyz[2] = 0;
	tempVerts[1].SetTexCoord( 1.0f, 0.0f );

	tempVerts[2].xyz[0] = 1.0f;
	tempVerts[2].xyz[1] = 1.0f;
	tempVerts[2].xyz[2] = 0;
	tempVerts[2].SetTexCoord( 1.0f, 1.0f );

	tempVerts[3].xyz[0] = 0.0f;
	tempVerts[3].xyz[1] = 1.0f;
	tempVerts[3].xyz[2] = 0;
	tempVerts[3].SetTexCoord( 0.0f, 1.0f );

	for( int i = 0; i < 4; i++ ) {
		tempVerts[i].SetColor( 0xFFFFFFFF );
	}
	return tri;
}

/*
===============
anRenderSystemLocal::Init
===============
*/
void anRenderSystemLocal::Init( void ) {
	common->Printf( "------- Initializing Teck RenderSystem --------\n" );

	// clear all our internal state
	viewCount = 1;		// so cleared structures never match viewCount
	// we used to memset tr, but now that it is a class, we can't, so
	// there may be other state we need to reset

	ambientLightVector[0] = 0.5f;
	ambientLightVector[1] = 0.5f - 0.385f;
	ambientLightVector[2] = 0.8925f;
	ambientLightVector[3] = 1.0f;

	//memset( &backEnd, 0, sizeof( backEnd ) );

	R_InitCvars();
	R_InitCommands();

	guiModel = new anInteractiveGuiModel;
	guiModel->Clear();
	tr_guiModel = guiModel;	// for DeviceContext fast path
	renderCache->Init();
	R_InitTriSurfData();

	globalImages->Init();

	// build brightness translation tables
	R_SetColorMappings();

	R_InitMaterials();

	renderModelManager->Init();

	// set the identity space
	identitySpace.modelMatrix[0*4+0] = 1.0f;
	identitySpace.modelMatrix[1*4+1] = 1.0f;
	identitySpace.modelMatrix[2*4+2] = 1.0f;

	// make sure the tr.unitSquareTriangles data is current in the vertex / index cache
	if ( unitSquareTriangles == nullptr || zeroOneCubeTriangles == nullptr || testImageTriangles == nullptr ) {
		unitSquareTriangles = R_CreateFullScreenTris();
		zeroOneCubeTriangles = R_CreateZeroOneCubeTris();
		testImageTriangles = R_CreateTestImageTris();
	}
	// make sure the tr.zeroOneCubeTriangles data is current in the vertex / index cache
	/*if (  ) {
		zeroOneCubeTriangles = R_CreateZeroOneCubeTris();
	}
	// make sure the tr.testImageTriangles data is current in the vertex / index cache
	if (  )  {
		testImageTriangles = R_CreateTestImageTris();
	}*/

	//if ( hudTriangles == nullptr )  {
	//	hudTriangles = R_MakeHUDTriangles();
	//}
	// determine which back end we will use
	// ??? this is invalid here as there is not enough information to set it up correctly
	SetBackEndRenderer();

	common->Printf( "Teck RenderSystem Initialized.\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
===============
anRenderSystemLocal::Shutdown
===============
*/
void anRenderSystemLocal::Shutdown( void ) {
	common->Printf( "Teck RenderSystem Shutdown\n" );

	R_DoneFreeType();

	if ( qglConfig.isInitialized && R_IsInitialized() ) {
		globalImages->PurgeAllImages();
	}

	fonts.DeleteContents();
	renderModelManager->Shutdown();
	globalImages->Shutdown();
	renderCache->Shutdown();

	// close the r_logFile
	if ( logFile ) {
		FPrintf( logFile, "*** CLOSING LOG ***\n" );
		fclose( logFile );
		logFile = 0;
	}

	// free frame memory
	R_ShutdownFrameData();

	// free the vertex cache, which should have nothing allocated now
	vertexCache.Shutdown();

	R_ShutdownTriSurfData();

	RB_ShutdownDebugTools();

	delete guiModel;

	Clear();

	ShutdownOpenGL();
	qglConfig.isInitialized = false;
}

/*
========================
idRenderSystemLocal::ResetGuiModels
========================
*/
void idRenderSystemLocal::ResetGuiModels() {
	delete guiModel;
	guiModel = new( TAG_RENDER ) arcGuiModel;
	guiModel->Clear();
	guiModel->BeginFrame();
	tr_guiModel = guiModel;	// for DeviceContext fast path
}

/*
========================
anRenderSystemLocal::BeginLevelLoad
========================
*/
void anRenderSystemLocal::BeginLevelLoad( void ) {
	renderModelManager->BeginLevelLoad();
	globalImages->BeginLevelLoad();
	// Re-Initialize the Default Materials if needed.
	R_InitMaterials();
}

/*
========================
anRenderSystemLocal::LoadLevelImages
========================
*/
void anRenderSystemLocal::LoadLevelImages() {
	globalImages->LoadLevelImages( false );
}

/*
========================
anRenderSystemLocal::Preload
========================
*/
///void anRenderSystemLocal::Preload( const idPreloadManifest &manifest, const char *mapName ) {
	//globalImages->Preload( manifest, true );
	//uiManager->Preload( mapName );
	//renderModelManager->Preload( manifest );
//}

/*
========================
anRenderSystemLocal::EndLevelLoad
========================
*/
void anRenderSystemLocal::EndLevelLoad( void ) {
	renderModelManager->EndLevelLoad();
	globalImages->EndLevelLoad();
	if ( r_forceLoadImages.GetBool() ) {
		RB_ShowImages();
	}
}

/*
============
RegisterFont

Loads 3 point sizes, 12, 24, and 48
============
*/
bool anRenderSystemLocal::RegisterFont( const char *fontName, fontInfoEx_t &font ) {
	void *faceData;
	ARC_TIME_T ftime;
	int i, len, fontCount;
	char name[1024];

	int pointSize = 12;
/*	if ( registeredFontCount >= MAX_FONTS ) {
		common->Warning( "RegisterFont: Too many fonts registered already." );
		return false;
	}

	int pointSize = 12;
	anStr::snPrintf( name, sizeof( name ), "%s/fontImage_%i.dat", fontName, pointSize );
	for ( i = 0; i < registeredFontCount; i++ ) {
		if ( anStr::Icmp(name, registeredFont[i].fontInfoS.name) == 0 ) {
			memcpy( &font, &registeredFont[i], sizeof( fontInfoEx_t ) );
			return true;
		}
	}*/

	memset( &font, 0, sizeof( font ) );

	for ( fontCount = 0; fontCount < 3; fontCount++ ) {
		if ( fontCount == 0 ) {
			pointSize = 12;
		} else if ( fontCount == 1 ) {
			pointSize = 24;
		} else {
			pointSize = 48;
		}
		// we also need to adjust the scale based on point size relative to 48 points as the ui scaling is based on a 48 point font
		float glyphScale = 1.0f; 		// change the scale to be relative to 1 based on 72 dpi ( so dpi of 144 means a scale of .5 )
		glyphScale *= 48.0f / pointSize;

		anStr::snPrintf( name, sizeof( name ), "%s/fontImage_%i.dat", fontName, pointSize );

		fontInfo_t *outFont;
		if ( fontCount == 0 ) {
			outFont = &font.fontInfoS;
		} else if ( fontCount == 1 ) {
			outFont = &font.fontInfoM;
		} else {
			outFont = &font.fontInfoXL;
		}

		anStr::Copynz( outFont->name, name, sizeof( outFont->name ) );

		len = fileSystem->ReadFile( name, nullptr, &ftime );
		if ( len != sizeof( fontInfo_t ) ) {
			common->Warning( "RegisterFont: couldn't find font: '%s'", name );
			return false;
		}

		fileSystem->ReadFile( name, &faceData, &ftime );
		fdOffset = 0;
		fdFile = reinterpret_cast<unsigned char*>(faceData);
		for ( i = 0; i < GLYPHS_PER_FONT; i++ ) {
			outFont->glyphs[i].height		= readInt();
			outFont->glyphs[i].top			= readInt();
			outFont->glyphs[i].bottom		= readInt();
			outFont->glyphs[i].pitch		= readInt();
			outFont->glyphs[i].xSkip		= readInt();
			outFont->glyphs[i].imageWidth	= readInt();
			outFont->glyphs[i].imageHeight	= readInt();
			outFont->glyphs[i].s			= readFloat();
			outFont->glyphs[i].t			= readFloat();
			outFont->glyphs[i].s2			= readFloat();
			outFont->glyphs[i].t2			= readFloat();
			int junk /* font.glyphs[i].glyph */		= readInt();
			//FIXME: the +6, -6 skips the embedded fonts/
			memcpy( outFont->glyphs[i].shaderName, &fdFile[fdOffset + 6], 32 - 6 );
			fdOffset += 32;
		}
		outFont->glyphScale = readFloat();

		int mw = 0;
		int mh = 0;
		for ( i = GLYPH_START; i < GLYPH_END; i++ ) {
			anStr::snPrintf(name, sizeof( name ), "%s/%s", fontName, outFont->glyphs[i].shaderName);
			outFont->glyphs[i].glyph = declManager->FindMaterial( name );
			outFont->glyphs[i].glyph->SetSort( SS_GUI );
			if (mh < outFont->glyphs[i].height) {
				mh = outFont->glyphs[i].height;
			}
			if (mw < outFont->glyphs[i].xSkip) {
				mw = outFont->glyphs[i].xSkip;
			}
		}
		if (fontCount == 0 ) {
			font.maxHeightS = mw;
			font.maxHeightS = mh;
		} else if (fontCount == 1 ) {
			font.maxWidthMedium = mw;
			font.maxHeightMedium = mh;
		} else {
			font.maxWidthXL = mw;
			font.maxHeighXL = mh;
		}
		fileSystem->FreeFile( faceData );
	}

	//memcpy( &registeredFont[registeredFontCount++], &font, sizeof( fontInfoEx_t ) );
	return true;
}

/*
========================
anRenderSystemLocal::ResetFonts
========================
*/
void anRenderSystemLocal::ResetFonts() {
	fonts.DeleteContents( true );
}

/*
========================
anRenderSystemLocal::InitOpenGL
========================
*/
void anRenderSystemLocal::InitOpenGL( void ) {
	// if OpenGL isn't started, start it now
	if ( !qglConfig.isInitialized && !R_IsInitialized() ) {
		R_InitOpenGL();
		// Reloading images here causes the rendertargets to get deleted
		globalImages->ReloadAllImages();

		int err = qglGetError();
		if ( err != GL_NO_ERROR ) {
			common->Printf( "qglGetError() = 0x%x\n", err );
		}
	}
}

/*
========================
anRenderSystemLocal::ShutdownOpenGL
========================
*/
void anRenderSystemLocal::ShutdownOpenGL( void ) {
	// free the context and close the window
	R_ShutdownFrameData();
	GLimp_Shutdown();
	qglConfig.isInitialized = false;
	//r_initialized = false;
}

/*
========================
anRenderSystemLocal::IsOpenGLRunning
========================
*/
bool anRenderSystemLocal::IsOpenGLRunning( void ) const {
	//return R_IsInitialized();
	if ( !qglConfig.isInitialized ) {
		return false;
	}

	return true;
}

/*
========================
anRenderSystemLocal::IsFullScreen
========================
*/
bool anRenderSystemLocal::IsFullScreen( void ) const {
	return qglConfig.isFullscreen;// != 0;
}

/*
========================
anRenderSystemLocal::GetScreenWidth
========================
*/
int anRenderSystemLocal::GetScreenWidth() const {
	return qglConfig.vidWidth;
}

/*
========================
anRenderSystemLocal::GetScreenHeight
========================
*/
int anRenderSystemLocal::GetScreenHeight() const {
	return qglConfig.vidHeight;
}

/*
========================
anRenderSystemLocal::GetCardCaps
========================
*/
void anRenderSystemLocal::GetCardCaps( bool &oldCard, bool &nv10or20 ) {
	nv10or20 = ( tr.backEndRenderer == BE_NV10 || tr.backEndRenderer == BE_NV20 );
	oldCard = ( tr.backEndRenderer == BE_ARB || tr.backEndRenderer == BE_R200 || tr.backEndRenderer == BE_NV10 || tr.backEndRenderer == BE_NV20 );
}
