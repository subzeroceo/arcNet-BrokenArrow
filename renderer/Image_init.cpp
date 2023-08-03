#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"

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

anImageManager::anImageManager() {
	// Initialize lists and hash table
}

/*
===============
anImageManager::
===============
*/
void anImageManager::Init() {
	// Initialize image manager
}

/*
===============
anImageManager::
===============
*/
void anImageManager::Shutdown() {
	// Free images and reset
	PurgeAllImages();
}

/*
===============
anImageManager::ImageFromFile
===============
*/
anImage *anImageManager::ImageFromFile( const char *filename, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage ) {
	anImage *image = imageHash.Get( filename) ;
	if ( image ) {
		return image;
	}

	// Load image from file

	imageHash.Set( filename, image );
	images.Append( image );

	return image;
}

/*
===============
anImageManager::PreCacheImage
===============
*/
void anImageManager::PreCacheImage( const char *filename ) {
	anImage *image = ImageFromFile( filename );
}

/*
===============
anImageManager::PurgeAllImages
===============
*/
void anImageManager::PurgeAllImages() {
	// Free images
	images.DeleteContents( true );
	imageHash.Clear();
}

/*
===============
anImageManager::ResizeImage
===============
*/
void anImageManager::ResizeImage( anImage *image, int width, int height ) {
	// Resize image
}

/*
===============
anImageManager::GetImageWidth
===============
*/
int anImageManager::GetImageWidth( const anImage *image ) const {
	return image->width;
}

/*
===============
anImageManager::GetImageHeight
===============
*/
int anImageManager::GetImageHeight( const anImage *image ) const {
	return image->height;
}

/*
===============
anImageManager::R_FindImageFileExt

Finds or loads the given image.
Returns NULL if it fails, not a default image
===============
*/
anImage *anImageManager::R_FindImageFileExt( const char *name, imgType_t type, imgFlags_t flags, bool mip ) {
	image_t *image;
	int width, height;
	byte *pic;
	GLenum  picFormat;
	int picNumMips;
	long hash;
	imgFlags_t checkFlagsTrue, checkFlagsFalse;

	if ( !name ) {
		return NULL;
	}

	hash = generateHashValue( name );

	//
	// see if the image is already loaded
	//
	for ( image = hashTable[hash]; image; image = image->next ) {
		if ( !anString::icmp( name, image->imgName ) ) {
			// the white image can be used with any set of parms, but other mismatches are errors
			if ( anString::Cmp( name, "_white" ) ) {
				if ( image->flags != flags ) {
					RB_Printf( "WARNING: reused image %s with mixed flags (%i vs %i)\n", name, image->flags, flags );
				}
				if ( image->characterMIP != mip ) {
					RB_Printf( "WARNING: reused image %s with mixed characterMIP parm\n", name );
				}
			}
			return image;
		}
	}

	//
	// load the pic from disk
	//
	R_LoadImage( name, &pic, &width, &height, &picFormat, &picNumMips );
	if ( pic == NULL ) {
		return NULL;
	}

	checkFlagsTrue = IMGFLAG_PICMIP | IMGFLAG_MIPMAP | IMGFLAG_GENNORMALMAP;
	checkFlagsFalse = IMGFLAG_CUBEMAP;
	if ( r_normalMapping->integer && ( picFormat == GL_RGBA8 ) && ( type == IMGTYPE_COLORALPHA ) &&
		( ( flags & checkFlagsTrue ) == checkFlagsTrue ) && !( flags & checkFlagsFalse ) ) {
		char normalName[MAX_QPATH];
		image_t *normalImage;
		int normalWidth, normalHeight;
		imgFlags_t normalFlags;

		normalFlags = ( flags & ~IMGFLAG_GENNORMALMAP ) | IMGFLAG_NOLIGHTSCALE;

		name.StripExtension( , normalName, MAX_QPATH );
		strcat( normalName, MAX_QPATH, "_n");

		// find normalmap in case it's there
		normalImage = R_FindImageFile(normalName, IMGTYPE_NORMAL, normalFlags );

		// if not, generate it
		if ( normalImage == NULL)  {
			byte *normalPic;
			int x, y;

			normalWidth = width;
			normalHeight = height;
			normalPic = Mem_Alloc16( width * height * 4 );
			RGBAtoNormal( pic, normalPic, width, height, flags & IMGFLAG_CLAMPTOEDGE );

#if 1
			// Brighten up the original image to work with the normal map
			RGBAtoYCoCgA( pic, pic, width, height );
			for ( y = 0; y < height; y++ ) {
				byte *imgByte  = pic       + y * width * 4;
				byte *normbyte = normalPic + y * width * 4;
				for ( x = 0; x < width; x++ ) {
					int div = anMath::Max( normbyte[2] - 127.0f, 16 );
					imgByte[0] = anMath::Clamp( imgByte[0] * 128.0f / div, 0, 255 );
					imgByte  += 4;
					normbyte += 4;
				}
			}
			ConvertYCoCgAtoRGBA( pic, pic, width, height );
#else
			// Blur original image's luma to work with the normal map
			byte *blurImg;

			ConvertRGBAtoYCoCgA( pic, pic, width, height );
			blurImg = Mem_Alloc16( width * height );

			for ( y = 1; y < height - 1; y++) {
				byte *imgByte  = pic     + y * width * 4;
				byte *blurByte = blurImg + y * width;
				imgByte += 4;
				blurByte += 1;
				for ( x = 1; x < width - 1; x++ ) {
					int result =
					*( imgByte - ( width + 1 ) * 4 ) + *( imgByte - width * 4 ) + *( imgByte - ( width - 1 ) * 4 ) +
					*( imgByte -          1  * 4 ) + *( imgByte            ) + *( imgByte +          1  * 4 ) +
					*( imgByte + ( width - 1 ) * 4 ) + *( imgByte + width * 4 ) + *( imgByte + ( width + 1 ) * 4 );

					result /= 9;

					*blurByte = result;
					imgByte += 4;
					blurByte += 1;
				}
				// FIXME: do borders

				for ( y = 1; y < height - 1; y++ ) {
					byte *imgByte  = pic     + y * width * 4;
					byte *blurByte = blurImg + y * width;

					imgByte += 4;
					blurByte += 1;
					for ( x = 1; x < width - 1; x++ ) {
						imgByte[0] = *blurByte;
						imgByte += 4;
						blurByte += 1;
					}
				}

				Mem_Free( blurImg );

				ConvertYCoCgAToRGBA( pic, pic, width, height );
			}
		}
#endif

		R_CreateImage( normalName, normalPic, normalWidth, normalHeight, IMGTYPE_NORMAL, normalFlags, 0 );
		Mem_Free( normalPic );	
	}

	// force mipmaps off if image is compressed but doesn't have enough mips
	if ( ( flags & IMGFLAG_MIPMAP ) && picFormat != GL_RGBA8 && picFormat != GL_SRGB8_ALPHA8_EXT ) {
		int wh = anMath::Max( width, height );
		int neededMips = 0;
		while ( wh ) {
			neededMips++;
			wh >>= 1;
		}
		if ( neededMips > picNumMips ) {
			flags &= ~IMGFLAG_MIPMAP;
		}
	}

	image = R_CreateImageExt2( (char *) name, pic, width, height, picFormat, picNumMips, type, flags, 0, characterMIP );
	ri.Free( pic );
	return image;
}

/*
===============
R_LoadImage


Loads any of the supported image types into a cannonical
32 bit format.

Automatically attempts to load .jpg files if .tga files fail to load.

*pic is null if the load fails.

Anything going to make this into a texture would use
makePowerOf2 = true, but loading an image as a lookup
table would leave it in identity state.

It's important to do this at image load time instead of texture load
time for bump maps.

Timestamp is null if the value is ignored

If pic is null, the image won't actually load, it will just locate the
timeStamp.
===============
*/
void anImageManager::LoadImage( const char *filename, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp, bool makePowerOf2 ) {
	anString name = filename;
		// Initialize variables
	*pic = nullptr;
	*timeStamp = 0xFFFFFFFF;
	*width = 0;
	*height = 0;

	// Process (parse? =:) the image name
	name.DefaultFileExtension( ".tga" );

	if ( name.Length() < 5 ) {
		return;
	}

	name.ToLower();
	anString ext;
	name.ExtractFileExtension( ext );

	// ArC-Net added //
	// If compressed textures are enabled, try loading a DDS first, it'll load fastest
	if ( r_ext_compressed_textures.GetInteger() ) {
		anString ddsName;
		ddsName.StripFileExtension();
		ddsName.DefaultFileExtension( ".dds" );
		LoadDDS( ddsName, pic, width, height, picFormat, numMips );
		// If loaded, we're done.
		if ( *pic ) {
			return;
		}
	} 
	// ArC-Net end -- hate dds but it should come first if the user has enabled compression //

	// Load the image based on the file extension
	if ( ext == "tga" ) {
		LoadTGA( name.c_str(), pic, width, height, timeStamp );				  // try tga first
		if ( ( pic && *pic == 0 ) || ( timeStamp && *timeStamp == -1 ) ) {
			name.StripFileExtension();
			name.DefaultFileExtension( ".jpg" );
			LoadJPG( name.c_str(), pic, width, height, timeStamp );
		}
	} else if ( ext == "pcx" ) {
		LoadPCX32( name.c_str(), pic, width, height, timeStamp );
	} else if ( ext == "bmp" ) {
		LoadBMP( name.c_str(), pic, width, height, timeStamp );
	} else if ( ext == "jpg" ) {
		LoadJPG( name.c_str(), pic, width, height, timeStamp );
	} else if ( ext -- "png" ) {
		LoadPNG( name.c_str(), pic, width, height, timeStamp );
	}

	// Check the loaded image dimensions
	if ( ( width && *width < 1 ) || ( height && *height < 1 ) ) {
		if ( pic && *pic ) {
			R_StaticFree( *pic );
			*pic = 0;//nullptr
		}
	}

	// convert to exact power of 2 sizes
	if ( pic && *pic && makePowerOf2 ) {
		int w = *width, h = *height, scaledWidth = 1, scaledHeight = 1;
		while ( scaledWidth < w || scaledHeight < h ) {
			scaledWidth <<= 1;
			scaledHeight <<= 1;
			if ( scaledWidth != w || scaledHeight != h ) {
				if ( globalImages->image_roundDown.GetBool() && scaledWidth > w
				|| globalImages->image_roundDown.GetBool() && scaledHeight > h ) {
					scaledWidth >>= 1;
					scaledHeight >>= 1;
				}
				byte *resampledBuffer = R_ResampleTexture( *pic, w, h, scaledWidth, scaledHeight );
				R_StaticFree( *pic );
				*pic = resampledBuffer;
				*width = scaledWidth;
				*height = scaledHeight;
			}
		}
	}
}
/*
===============
anImageManager::LoadBinaryTGA
===============
*/
anImage *anImageManager::LoadBinaryTGA( const char *filename ) {
	// Open file in binary mode
	// Read TGA header
	// Load binary pixel data into image
	return loadedImage;
}

/*
===============
anImageManager::LoadTGA
===============
*/
anImage *anImageManager::LoadTGA( const char *filename, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp ) {
	// Load TGA file data into image
	byte *buffer;

	if ( !pic ) {
		fileSystem->ReadFile( filename, nullptr, timeStamp );
		return;	// just getting timeStamp
	}

	*pic = nullptr;

	//
	// load the file
	//
	int fileSize = fileSystem->ReadFile( filename, (void **)&buffer, timeStamp );
	if ( !buffer ) {
		return;
	}

	byte *targaBuffer = buffer;

	TargaHeader targa.idLength = *targaBuffer++;
	targa.colorMapType = *targaBuffer++;
	targa.imageType = *targaBuffer++;

	targa.colorMapIndex = LittleShort( *( short *)targaBuffer ); targaBuffer += 2;
	targa.colormapLength = LittleShort( *( short *)targaBuffer ); targaBuffer += 2;
	targa.colorMapSize = *targaBuffer++;
	targa.xOrigin = LittleShort( *( short *)targaBuffer ); targaBuffer += 2;
	targa.yOrigin = LittleShort( *( short *)targaBuffer ); targaBuffer += 2;
	targa.width = LittleShort( *( short *)targaBuffer ); targaBuffer += 2;
	targa.height = LittleShort( *( short *)targaBuffer ); targaBuffer += 2;
	targa.pixelSize = *targaBuffer++;
	targa.attributes = *targaBuffer++;

	if ( targa.imageType != 2 && targa.imageType != 10 && targa.imageType != 3 ) {
		common->Error( "LoadTGA( %s ): Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n", filename );
	}

	if ( targa.colorMapType != 0 ) {
		common->Error( "LoadTGA( %s ): colormaps not supported\n", filename );
	}

	if ( ( targa.pixelSize != 32 && targa.pixelSize != 24 ) && targa.imageType != 3 ) {
		common->Error( "LoadTGA( %s ): Only 32 or 24 bit images supported (no colormaps)\n", filename );
	}

	if ( targa.imageType == 2 || targa.imageType == 3 ) {
		int numBytes = targa.width * targa.height * ( targa.pixelSize >> 3 );
		if ( numBytes > fileSize - 18 - targa.idLength ) {
			common->Error( "LoadTGA( %s ): incomplete file\n", filename );
		}
	}

	int columns = targa.width;
	int rows = targa.height;
	int numPixels = columns * rows;

	if ( width && height ) {
		*width = columns;
		*height = rows;
	}

	//rgba = Mem_Alloc( numPixels*4 );
	byte *rgba = (byte *)R_StaticAlloc( numPixels * 4 );
	*pic = rgba;

	if ( targa.idLength != 0 ) {
		targaBuffer += targa.idLength;  // skip TARGA image comment
		if ( targa.imageType == 2 || targa.imageType == 3 ) {
			// Uncompressed RGB or gray scale image
			for ( int row = rows - 1; row >= 0; row-- ) {
				byte *pixBuf = rgba + row * columns * 4;
				for ( int column = 0; column < columns; column++ ) {
					unsigned char red, green, blue, alphaByte;
					switch ( targa.pixelSize ) {
					case 8:
						blue = *targaBuffer++;
						green = blue;
						red = blue;
						*pixBuf++ = red;
						*pixBuf++ = green;
						*pixBuf++ = blue;
						*pixBuf++ = 255;
						break;
					case 24:
						blue = *targaBuffer++;
						green = *targaBuffer++;
						red = *targaBuffer++;
						*pixBuf++ = red;
						*pixBuf++ = green;
						*pixBuf++ = blue;
						*pixBuf++ = 255;
						break;
					case 32:
						blue = *targaBuffer++;
						green = *targaBuffer++;
						red = *targaBuffer++;
						alphaByte = *targaBuffer++;
						*pixBuf++ = red;
						*pixBuf++ = green;
						*pixBuf++ = blue;
						*pixBuf++ = alphaByte;
						break;
					default:
						common->Error( "LoadTGA( %s ): Invalid unsupported pixelSize '%d'\n", filename, targa.pixelSize );
						break;
					}
				}
			}
		}
	} else if ( targa.imageType == 10 ) {   // Runlength encoded RGB images
		unsigned char packetHeader,packetSize;
		unsigned char red = 0, green = 0, blue = 0, alphaByte = 0xff;
		for ( int row = rows - 1; row >= 0; row-- ) {
			byte *pixBuf = rgba + row * columns * 4;
			for ( int column = 0; column < columns; ) {
				packetHeader= *targaBuffer++;
				packetSize = 1 + ( packetHeader & 0x7f );
				if ( packetHeader & 0x80 ) {		   // run-length packet
					switch ( targa.pixelSize ) {
						case 24:
								blue = *targaBuffer++;
								green = *targaBuffer++;
								red = *targaBuffer++;
								alphaByte = 255;
								break;
						case 32:
								blue = *targaBuffer++;
								green = *targaBuffer++;
								red = *targaBuffer++;
								alphaByte = *targaBuffer++;
								break;
						default:
							common->Error( "LoadTGA( %s ): illegal pixelSize '%d'\n", filename, targa.pixelSize );
							break;
					}

					for ( unsigned char j = 0; j < packetSize; j++ ) {
						*pixBuf++=red;
						*pixBuf++=green;
						*pixBuf++=blue;
						*pixBuf++=alphaByte;
						column++;
						if ( column == columns ) { // run spans across rows
							column = 0;
							if ( row > 0 ) {
								row--;
							} else {
								goto breakOut;
							}
							pixBuf = rgba + row * columns * 4;
						}
					}
				} else {										   // non run-length packet
					for ( unsigned char j = 0; j < packetSize; j++ ) {
						switch ( targa.pixelSize ) {
							case 24:
									blue = *targaBuffer++;
									green = *targaBuffer++;
									red = *targaBuffer++;
									*pixBuf++ = red;
									*pixBuf++ = green;
									*pixBuf++ = blue;
									*pixBuf++ = 255;
									break;
							case 32:
									blue = *targaBuffer++;
									green = *targaBuffer++;
									red = *targaBuffer++;
									alphaByte = *targaBuffer++;
									*pixBuf++ = red;
									*pixBuf++ = green;
									*pixBuf++ = blue;
									*pixBuf++ = alphaByte;
									break;
							default:
								common->Error( "LoadTGA( %s ): illegal pixelSize '%d'\n", filename, targa.pixelSize );
								break;
						}
						column++;
						if ( column == columns ) { // pixel packet run spans across rows
							column = 0;
							if ( row > 0 ) {
								row--;
							} else {
								goto breakOut;
							}
							pixBuf = rgba + row * columns * 4;
						}
					}
				}
			}
			breakOut: ;
		}
	}

	if ( ( targa.attributes & ( 1<<5 ) ) ) {			// image flp bit
		R_VerticalFlip( *pic, *width, *height );
	}
	Mem_Free( buffer );
	fileSystem->FreeFile( buffer );
}

/*
=============
anImageManager::LoadJPG
=============
*/
static void anImageManager::LoadJPG( const char *filename, unsigned char **pic, int *width, int *height, ARC_TIME_T *timeStamp ) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	if ( pic ) {
			*pic = nullptr;
		}
		anFile *f = fileSystem->OpenFileRead( filename );
		if ( !f ) {
			return;
		}
		int len = f->Length();
		if ( timeStamp ) {
			*timeStamp = f->Timestamp();
			if ( !pic ) {
				fileSystem->CloseFile( f );
				return;	// just getting timeStamp
			}
			byte *fbuffer = (byte *)Mem_ClearedAlloc( len + 4096 );
			f->Read( fbuffer, len );
			fileSystem->CloseFile( f );
	}

	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_decompress( &cinfo );
	jpeg_stdio_src( &cinfo, fbuffer );
	( void )jpeg_read_header( &cinfo, true );
	( void )jpeg_start_decompress( &cinfo );
	int row_stride = cinfo.output_width * cinfo.output_components;

	if ( cinfo.output_components != 4 ) {
		common->DWarning( "JPG %s is unsupported color depth (%d)", filename, cinfo.output_components );
	}
	unsigned char *out = (byte *)R_StaticAlloc( cinfo.output_width*cinfo.output_height*4 );

	*pic = out;
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	while ( cinfo.output_scanline < cinfo.output_height ) {
		byte *bbuf = ( ( out+( row_stride*cinfo.output_scanline ) ) );
		JSAMPARRAY buffer = &bbuf;
		( void ) jpeg_read_scanlines( &cinfo, buffer, 1 );
	} // clear all the alphas to 255
	byte *buf = *pic;

	int j = cinfo.output_width * cinfo.output_height * 4;
	for ( int i = 3; i < j; i += 4 ) {
		buf[i] = 255;
	}

	( void ) jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );
	Mem_Free( fbuffer );
}

/*
==============
anImageManager::LoadBMP
==============
*/
static voidanImageManager::LoadBMP( const char *name, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp ) {
}

/*
==============
anImageManager::LoadPCX
==============
*/
static void voidanImageManager::LoadPCX( const char *filename, byte **pic, byte **palette, int *width, int *height, ARC_TIME_T *timeStamp ) {
	if ( !pic ) {
		fileSystem->ReadFile( filename, nullptr, timeStamp );
		return;	// just getting timeStamp
	}

	*pic = nullptr;
	*palette = nullptr;

	// load the file
	int len = fileSystem->ReadFile( filename, (void **)&raw, timeStamp );
	if ( !raw ) {
		return;
	}

	// parse the PCX file
	pcx_t *pcx = (pcx_t *)raw;
	byte *raw = &pcx->data;

  	int xMax = LittleShort( pcx->xMax );
	int yMax = LittleShort( pcx->yMax );

	if ( pcx->manufacturer != 0x0a || pcx->version != 5 || pcx->encoding != 1 || pcx->bits_per_pixel != 8 || xMax >= 1024 || yMax >= 1024 ) {
		common->Printf( "Bad pcx file %s (%i x %i) (%i x %i)\n", filename, xMax+1, yMax+1, pcx->xMax, pcx->yMax );
		return;
	}

	byte *out = (byte *)R_StaticAlloc( ( yMax+1 ) * ( xMax+1 ) );

	*pic = out;

	byte *pix = out;

	if ( palette ) {
		*palette = (byte *)R_StaticAlloc( 768 );
		memcpy( *palette, (byte *)pcx + len - 768, 768 );
	}

	if ( width && height ) {
		*width = xMax+1;
		*height = yMax+1;
	}

	// FIXME: use bytes_per_line here?
	for ( int y = 0; y <= yMax; y++, pix += xMax + 1 ) {
		for ( int x = 0; x <= xMax; ) {
			int dataByte = *raw++;
			if ( ( dataByte & 0xC0 ) == 0xC0 ) {
				int runLength = dataByte & 0x3F;
				dataByte = *raw++;
			} else {
				int runLength = 1;
			while ( int runLength -- > 0 ) {
				pix[x++] = dataByte;
			}
		}
	}

	if ( raw - (byte *)pcx > len ) {
		common->Printf( "PCX file %s was malformed", filename );
		R_StaticFree( *pic );
		*pic = nullptr;
	}
	fileSystem->FreeFile( pcx );
}

/*
==============
anImageManager::LoadPCX
==============
*/
static void anImageManager::LoadPCX32( const char *filename, byte **pic, int *width, int *height, ARC_TIME_T *timeStamp ) {
	byte	*palette;
	byte	*pic8;
	int		i, c, p;
	byte	*pic32;

	if ( !pic ) {
		fileSystem->ReadFile( filename, nullptr, timeStamp );
		return;	// just getting timeStamp
	}
	LoadPCX( filename, &pic8, &palette, width, height, timeStamp );
	if ( !pic8 ) {
		*pic = nullptr;
		return;
	}

	c = (* width) * (* height);
	pic32 = *pic = (byte *)R_StaticAlloc(4 * c );
	for ( int i = 0; i < c; i++ ) {
		p = pic8[i];
		pic32[0] = palette[p*3];
		pic32[1] = palette[p*3 + 1];
		pic32[2] = palette[p*3 + 2];
		pic32[3] = 255;
		pic32 += 4;
	}

	R_StaticFree( pic8 );
	R_StaticFree( palette );
}

/*
===============
anImageManager::WriteTGA
===============
*/
void anImageManager::WriteTGA( const anImage *image, const char *filename ) {
  // Write image data to TGA file
}

/*
===============
anImageManager::WriteBinaryTGA
===============
*/
// The binary loading/saving would skip any text formatting and read the raw pixel data directly.
void anImageManager::WriteBinaryTGA( const anImage *image, const char *fiilename ) {
	// Open file in binary mode
	// Write TGA header
	// Write binary pixel data
}

/*
===============
anImageManager::LoadVolumeTexture
===============
*/
idVolumeTexture *anImageManager::LoadVolumeTexture( const char *filename ) {
	// Load volumetric texture from file
}

/*
===============
anImageManager::UpdateVolumeTexture
===============
*/
void anImageManager::UpdateVolumeTexture( idVolumeTexture *volTex, const void *data, int size ) {
	// Update volume texture data
}

/*
===============
anImageManager::CreateVirtualTexture
===============
*/
idVirtualTexture *anImageManager::CreateVirtualTexture( int width, int height ) {
	// Allocate virtual texture
}

/*
===============
anImageManager::TileVirtualTexture
===============
*/
void anImageManager::TileVirtualTexture( idVirtualTexture *virTex ) {
	// Layout virtual texture tiles
}

/*
===============
anImageManager::DeleteTextures
===============
*/
void anImageManager::DeleteTextures( void ) {
	for ( int i = 0; i < images.Num(); i++ ) {
		qglDeleteTextures( 1, &images[i]->texnum );
	}
	memset( images, 0, sizeof( images ) );
	memset( qglState.currenttextures, 0, sizeof( qglState.currenttextures ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SetCurrentTextureUnit( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SetCurrentTextureUnit( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
	deletet images;
}

/*
===============
anImageManager::PurgeTexture
===============
*/
// PurgeTexture allows removing a specific texture from the cache instead of clearing everything.
void anImageManager::PurgeTexture( anImage *texture ) {
	// Remove references from hash table and list
	images.Remove( texture );
	imageHash.Remove( texture );

	// Delete image
	delete texture;
}

/*
===============
anImageManager::LoadRaytracedImage
===============
*/
anImage *anImageManager::LoadRaytracedImage( const char *filename ) {
	// Load completed raytraced image
}

/*
===============
anImageManager::RaytraceImage
===============
*/
void anImageManager::RaytraceImage( anImage *image ) {
	// Raytrace image using accelerator
}

/*
===============
anImageManager::LoadVolumeTexture
===============
*/
// Volumetric Textures
idVolumeTexture *LoadVolumeTexture( const char *filename ) {
}

/*
===============
anImageManager::VolumeTexture
===============
*/
void UpdateVolumeTexture( idVolumeTexture *volTex, const void *data, int size ) {
}

/*
===============
anImageManager::BuildAccelerationStructure
===============
*/
void anImageManager::BuildAccelerationStructure() {
}

/*
===============
anImageManager::RenderWithRayTracing
===============
*/
void anImageManager::RenderWithRayTracing( anImage *image ) {
}

/*
===============
anImageManager::HardwareDecode
===============
*/
anImage *anImageManager::HardwareDecode( const byte *compressedData, int dataSize ) {
}

/*
===============
anImageManager::SamplVeolumeTexture
 
In shader: Trilinear filtering
Sample volume texture 
Combine color with surface 
===============
*/
void anImageManager::SampleVolumeTexture( anVec3 pos, float *outColor ) { 
 // Clamp coordinates
 	anVec3 coord = uvw;
	coord.x = anMath::ClampFloat( 0.0f, 1.0 f );
    
	// Trilinearly interpolate voxels
	int x = coord.x * width;
	int y = coord.y * height;
	int z = coord.z * depth;
	float vx = frac( x ); 
	float vy = frac( y );
	float vz = frac( z );
    
	return sampledValue;
}

void anImageManager::ResizeImageLanczos( anImage *img, const void *data, int size ) {
	// Update volume texture data
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void RB_ShowImages( void ) {
	int i;
	image_t *image;
	int start, end;

	RB_SetGL2D();

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();


	start = Sys_Milliseconds();

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[i];

		int w = qglConfig.vidWidth / 40;
		int h = qglConfig.vidHeight / 30;

		int x = i % 40 * w;
		int y = i / 30 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
			anVec4 quadVerts[4];

			GL_BindToTMU( image, TB_COLORMAP );

			VectorSet4( quadVerts[0], x, y, 0, 1 );
			VectorSet4( quadVerts[1], x + w, y, 0, 1 );
			VectorSet4( quadVerts[2], x + w, y + h, 0, 1 );
			VectorSet4( quadVerts[3], x, y + h, 0, 1 );

			RB_InstantQuad( quadVerts );
		}
	}

	qglFinish();

	end = Sys_Milliseconds();
	RB_Printf( "%i msec to draw all images\n", end - start );
}


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

	float Sample( const idVec3 &uvw) const;

	void ApplyToSurface( idRenderEntity *entity );

private:

	int width;
	int height;
	int depth;

	byte *voxelData;

};