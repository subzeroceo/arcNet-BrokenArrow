#include "../idlib/Lib.h"

#define COPYSAMPLE(a,b) *(unsigned int *)(a) = *(unsigned int *)(b)

/*
Here are some ideas for implementing an advanced image management system in a Doom 3 code style while evolving its capabilities:
Class Definition:

Create anImageManager class header
Define class constructor/destructor
Declare public interface methods
Declare private data members
Core Functionality:

Implement Init() and Shutdown() methods
Load image files with ImageFromFile()
Write image files with WriteImage()
Manage image list and cache
Implement GetImageInfo() methods
Advanced Features:

Add hardware accelerated decoding
Implement asynchronous streaming
Add procedural texture generation
Create lightmap baking functions
Implement virtual texturing
Raytracing Support:

Integrate raytracing API (DXR)
Acceleration structure management
Load/save raytraced images
Perform raytracing in RenderImage()
Volumetric Textures:

Implement 3D texture formats
Load volume data from files
Update volume texture from voxel data
Sample and render volume textures
Class Interface:

Maintain original anImageManager class interface and method signatures
Add new advanced methods like you suggested for raytracing, volumetric textures, etc.
Keep class definitions in .h files and implementations in .cpp as idTech does
Image Formats:

Support standard formats like JPEG, PNG, TGA, HDR
Add high quality formats like EXR for HDR
Implement hardware compressed formats like ASTC, BC7 for efficiency
Allow binary loading/saving for formats like TGA as you mentioned
Features:

Hardware-accelerated decoding and encoding
Asynchronous streaming
Caching and purging system based on usage
Procedural texture generation
Texture baking and lightmap creation
Virtual texturing for large texture sets
Raytracing:

Integrate with raytracing API like DXR
Acceleration structure management
Load completed raytraced images
Raytrace images and materials
Volumetric Textures:

Support 3D texture formats
Load from voxel data
Update texture data
Sample and render volumes
Hardware-accelerated texture compression/decompression (ASTC, BC7, etc)
Asynchronous texture streaming for loading high-res textures without stalling
Virtual texturing for massive texture sets exceeding GPU limits
Texture arrays and atlases for more efficient texture management
Procedural texture generation (noise, patterns, etc)
Texture baking and lightmap generation
GPU-based image processing and effects
Multi-channel textures like normal/roughness maps
HDR texture support with floating point formats
Volumetric textures for effects like smoke, fire, liquids
Texture tessellation for continuous LODs
Ray tracing integration for material/texture effects
AI-assisted texture upscaling and enhancement------
Texture painting and editing tools
Integration with physically-based rendering pipelines
Support for VR/AR headsets and stereo rendering
Leveraging compute shaders for parallel operations
Texture streaming over network
Integration with material editor and shader graph
Texture usage analysis and optimization


dvanced Rendering:

Ray tracing support for true dynamic lighting, reflections, shadows etc. This could drastically improve visual quality.

Implement voxel cone tracing for indirect lighting and ambient occlusion.

Add a modern physically based rendering pipeline with real-time area lights, global illumination solutions like voxel GI or lightmaps.

Lumen-style fully dynamic scene lighting.

Integrate advanced post processing like volumetric fog, SSAO, motion blur.

Support for VR rendering and headsets.

Richer World:

Large open world areas instead of isolated levels.

Advanced AI and crowds to make the world more interactive and alive.

Complex physics with destructible environments.

Ecosystem simulation for wildlife, weather etc.

Expanded modding and user content support.

Multiplayer Experiences:

Shared social hub spaces for players to interact.

Competitive team-based multiplayer modes.

User-hosted servers and custom matches.

Of course, major engine changes like these would be incredibly challenging.
But with enough effort, even dated engines can be updated with new features.
The key is identifying the most impactful enhancements that could bring Doom 3 to modern standards.



*/

class anImageManager {

public:

									anImageManager() { insideLevelLoad = false;}
									~anImageManager();

	void								Init();
	void								Shutdown();

	anImage *							ImageFromFile(const char *filename, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage );

	void								PreCacheImage( const char *filename );

	void								PurgeAllImages();

	void								ResizeImage( anImage *image, int width, int height );

	int									GetImageWidth( const anImage *image ) const;
	int									GetImageHeight( const anImage *image ) const;

	anImage *							FindImageFileExt( const char *filename, imgType_t type, imgFlags_t flags, bool mip );
	anImage *							LoadImage( const char *filename, byte **pic, int *width, int *height, GLenum *picFormat, int *numMips );

	anImage *							LoadTGA( const char *filename, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp );
	anImage *							LoadPNG( const char *filename );
	anImage *							LoadBinaryTGA( const char *filename );

	static void							LoadPCX32( const char *filename, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp );
	anImage *							LoadJPEG( const char *filename );
	void								WriteTGA( const anImage *image, const char *filename );

	void 								WriteJPEG( const anImage *image, const char *fiilename );

	void								WriteBinaryTGA( const anImage *image, const char *fiilename );

	// Raytracing
	anImage *							LoadRaytracedImage( const char *filename );
	void								RaytraceImage( anImage *image );

	// Volumetric Textures
	idVolumeTexture *					LoadVolumeTexture( const char *filename );
	void								UpdateVolumeTexture( idVolumeTexture *volTex, const void *data, int size);
	void 								AddTexture( idTexture *texture );
	void 								RemoveTexture( idTexture *texture );

	// Virtual Texturing
	idVirtualTexture *					CreateVirtualTexture( int width, int height );
	void 								TileVirtualTexture( idVirtualTexture *virTex); // Layout tiles

	void 								BuildAccelerationStructure(); // Build BVH, etc
	void 								RenderWithRayTracing( anImage *image ); // Trace rays

	anImage *							HardwareDecode( const byte *compressedData, int dataSize );

	void								SampleVolumeTexture( anVec3 pos, float *outColor ); // Trilinear filtering

	void 								DeleteTextures( void );
	void								PurgeTexture( anImage *texture );

	anImage *			AllocImage( const char *name );
	void				SetNormalPalette();
	void				ChangeTextureFilter();

private:

	anList<anImage *>					images;
	anHashTable<anImage *>				imageHash;

	bool								insideLevelLoad;			// don't actually load images now

};

//	idVolumeTexture class

// Stores volume texture data
// Methods for initialization, loading data, sampling, etc.

class idVirtualTexture {

public:

	idVirtualTexture();
	~idVirtualTexture();

	void Init( const char *name, int width, int height, int depth );
	void LoadFromFile( const char *filename );

	void UpdateVoxelData( const byte *data, int size );

	float Sample( const anVec3 &uvw) const;

	void ApplyToSurface( anRenderEntity *entity );

private:

	int width;
	int height;
	int depth;

	byte *voxelData;
};