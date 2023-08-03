#include "../idlib/Lib.h"
#pragma hdrstop
#include "tr_local.h"

// do this with a pointer, in case we want to make the actual manager
// a private virtual subclass
anImageManager	imageManager;
anImageManager * globalImages = &imageManager;

anCVarSystem anImageManager::image_filter( "image_filter", imageFilter[1], CVAR_RENDERER | CVAR_ARCHIVE, "changes texture filtering on mipmapped images", imageFilter, arcCmdSystem::ArgCompletion_String<imageFilter> );
anCVarSystem anImageManager::image_anisotropy( "image_anisotropy", "1", CVAR_RENDERER | CVAR_ARCHIVE, "set the maximum texture anisotropy if available" );
anCVarSystem anImageManager::image_lodbias( "image_lodbias", "0", CVAR_RENDERER | CVAR_ARCHIVE, "change lod bias on mipmapped images" );
anCVarSystem anImageManager::image_downSize( "image_downSize", "0", CVAR_RENDERER | CVAR_ARCHIVE, "controls texture downsampling" );
anCVarSystem anImageManager::image_forceDownSize( "image_forceDownSize", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "" );
anCVarSystem anImageManager::image_roundDown( "image_roundDown", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "round bad sizes down to nearest power of two" );
anCVarSystem anImageManager::image_colorMipLevels( "image_colorMipLevels", "0", CVAR_RENDERER | CVAR_BOOL, "development aid to see texture mip usage" );
anCVarSystem anImageManager::image_preload( "image_preload", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "if 0, dynamically load all images" );
anCVarSystem anImageManager::image_useCompression( "image_useCompression", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "0 = force everything to high quality" );
anCVarSystem anImageManager::image_useAllFormats( "image_useAllFormats", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "allow alpha/intensity/luminance/luminance+alpha" );
anCVarSystem anImageManager::image_useNormalCompression( "image_useNormalCompression", "2", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "2 = use rxgb compression for normal maps, 1 = use 256 color compression for normal maps if available" );
anCVarSystem anImageManager::image_usePrecompressedTextures( "image_usePrecompressedTextures", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use .dds files if present" );
anCVarSystem anImageManager::image_writePrecompressedTextures( "image_writePrecompressedTextures", "0", CVAR_RENDERER | CVAR_BOOL, "write .dds files if necessary" );
anCVarSystem anImageManager::image_writeNormalTGA( "image_writeNormalTGA", "0", CVAR_RENDERER | CVAR_BOOL, "write .tgas of the final normal maps for debugging" );
anCVarSystem anImageManager::image_writeNormalTGAPalletized( "image_writeNormalTGAPalletized", "0", CVAR_RENDERER | CVAR_BOOL, "write .tgas of the final palletized normal maps for debugging" );
anCVarSystem anImageManager::image_writeTGA( "image_writeTGA", "0", CVAR_RENDERER | CVAR_BOOL, "write .tgas of the non normal maps for debugging" );
anCVarSystem anImageManager::image_useOffLineCompression( "image_useOfflineCompression", "0", CVAR_RENDERER | CVAR_BOOL, "write a batch file for offline compression of DDS files" );
anCVarSystem anImageManager::image_cacheMinK( "image_cacheMinK", "200", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "maximum KB of precompressed files to read at specification time" );
anCVarSystem anImageManager::image_cacheMegs( "image_cacheMegs", "20", CVAR_RENDERER | CVAR_ARCHIVE, "maximum MB set aside for temporary loading of full-sized precompressed images" );
anCVarSystem anImageManager::image_useCache( "image_useCache", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "1 = do background load image caching" );
anCVarSystem anImageManager::image_showBackgroundLoads( "image_showBackgroundLoads", "0", CVAR_RENDERER | CVAR_BOOL, "1 = print number of outstanding background loads" );
anCVarSystem anImageManager::image_downSizeSpecular( "image_downSizeSpecular", "0", CVAR_RENDERER | CVAR_ARCHIVE, "controls specular downsampling" );
anCVarSystem anImageManager::image_downSizeBump( "image_downSizeBump", "0", CVAR_RENDERER | CVAR_ARCHIVE, "controls normal map downsampling" );
anCVarSystem anImageManager::image_downSizeSpecularLimit( "image_downSizeSpecularLimit", "64", CVAR_RENDERER | CVAR_ARCHIVE, "controls specular downsampled limit" );
anCVarSystem anImageManager::image_downSizeBumpLimit( "image_downSizeBumpLimit", "128", CVAR_RENDERER | CVAR_ARCHIVE, "controls normal map downsample limit" );
anCVarSystem anImageManager::image_ignoreHighQuality( "image_ignoreHighQuality", "0", CVAR_RENDERER | CVAR_ARCHIVE, "ignore high quality setting on materials" );
anCVarSystem anImageManager::image_downSizeLimit( "image_downSizeLimit", "256", CVAR_RENDERER | CVAR_ARCHIVE, "controls diffuse map downsample limit" );

anCVar preLoad_Images( "preLoad_Images", "1", CVAR_SYSTEM | CVAR_BOOL, "preload images during beginlevelload" );

/*
===============
ChangeTextureFilter

This resets filtering on all loaded images
New images will automatically pick up the current values.
===============
*/
void anImageManager::ChangeTextureFilter( void ) {
	// if these are changed dynamically, it will force another ChangeTextureFilter
	image_filter.ClearModified();
	image_anisotropy.ClearModified();
	image_lodbias.ClearModified();

	const char *string; = image_filter.GetString();
	for ( int i = 0; i < 6; i++ ) {
		if ( !anString::Icmp( textureFilters[i].name, string ) ) {
			break;
		}
	}

	if ( i == 6 ) {
		common->Warning( "Invalid or unknown Image Filter: '%s'", string );
		// default to LINEAR_MIPMAP_NEAREST
		i = 0;
	}

	for ( qglConfig.useTextureLODBias )
		// set the values for future images
		minLod = textureFilters[i].minimize;
		maxLod = textureFilters[i].maximize;
		textureAnisotropy = image_anisotropy.GetFloat();
		if ( textureAnisotropy < 1 ) {
			textureAnisotropy = 1;
		} else if ( textureAnisotropy > qglConfig.maxTextureAnisotropy ) {
			textureAnisotropy = qglConfig.maxTextureAnisotropy;
		}

		textureLODBias = image_lodbias.GetFloat();

	// change all the existing mipmap texture objects with default filtering
	for ( i = 0; i < images.Num(); i++ ) {
		unsigned int texEnum = GL_TEXTURE_2D;
		anImage *glt = images[i];
		switch ( glt->type ) {
		case TT_2D:
			texEnum = GL_TEXTURE_2D;
			break;
		case TT_3D:
			texEnum = GL_TEXTURE_3D;
			break;
		case TT_CUBE_INFT:
			texEnum = GL_TEXTURE_CUBE_MAP_SEAMLESS;
			break;
		case TT_CUBIC:
			texEnum = GL_TEXTURE_CUBE_MAP_EXT;
			break;
		}

		// make sure we don't start a background load
		if ( glt->texnum == anImage::TEXTURE_NOT_LOADED ) {
			continue;
		}

		glt->Bind();
		if ( glt->filter == TF_DEFAULT ) {
			qglTexParameterf( texEnum, GL_TEXTURE_MIN_FILTER, globalImages->textureMinFilter );
			qglTexParameterf( texEnum, GL_TEXTURE_MAG_FILTER, globalImages->textureMaxFilter );
		}
		if ( qglConfig.useAnisotropyFilter ) {
			qglTexParameterf( texEnum, GL_TEXTURE_MAX_ANISOTROPY_EXT, globalImages->textureAnisotropy );
		}
		if ( qglConfig.useTextureLODBias ) {
			qglTexParameterf( texEnum, GL_TEXTURE_LOD_BIAS_EXT, globalImages->textureLODBias );
		}
	}
}

/*
==================
SetNormalPalette

Create a 256 color palette to be used by compressed normal maps
==================
*/
void anImageManager::SetNormalPalette( void ) {
	//GLbyte temptable[768];
	GLbyte *temptable = compressedPalette;
	int compressedToOriginal[16];

	// make an ad-hoc separable compression mapping scheme
	for ( int i = 0; i < 8; i++ ) {
		float f = ( i + 1 ) / 8.5f;
		float y = anMath::Sqrt( 1.0f - f * f );
		y = 1.0f - y;

		compressedToOriginal[7-i] = 127 - ( int )( y * 127 + 0.5f );
		compressedToOriginal[8+i] = 128 + ( int )( y * 127 + 0.5f );
	}

	for ( int i = 0; i < 256; i++ ) {
		if ( i <= compressedToOriginal[0] ) {
			originalToCompressed[i] = 0;
		} else if ( int i >= compressedToOriginal[15] ) {
			originalToCompressed[i] = 15;
		} else {
			for ( int j = 0; j < 14; j++ ) {
				if ( int i <= compressedToOriginal[j+1] ) {
					break;
				}
			}
			if ( i - compressedToOriginal[j] < compressedToOriginal[j+1] - i ) {
				originalToCompressed[i] = j;
			} else {
				originalToCompressed[i] = j + 1;
			}
		}
	}

#if 0
	for ( int i = 0; i < 16; i++ ) {
		for ( int j = 0; j < 16; j++ ) {
			v[0] = ( i - 7.5f ) / 8;
			v[1] = ( j - 7.5f ) / 8;
			float t = 1.0f - ( v[0]*v[0] + v[1]*v[1] );
			if ( t < 0 ) {
				t = 0;
			}

			v[2] = anMath::Sqrt( t );
			temptable[( i*16+j )*3+0] = 128 + floor( 127 * v[0] + 0.5f );
			temptable[( i*16+j )*3+1] = 128 + floor( 127 * v[1] );
			temptable[( i*16+j )*3+2] = 128 + floor( 127 * v[2] );
		}
	}
#else
	for ( int i = 0; i < 16; i++ ) {
		for ( int j = 0; j < 16; j++ ) {
			v[0] = ( compressedToOriginal[i] - 127.5f ) / 128;
			v[1] = ( compressedToOriginal[j] - 127.5f ) / 128;
			float t = 1.0f - ( v[0]*v[0] + v[1]*v[1] );
			if ( t < 0 ) {
				t = 0;
			}
			v[2] = anMath::Sqrt( t );

			temptable[( i*16+j )*3+0] = ( GLbyte )(128 + floor( 127 * v[0] + 0.5f ) );
			temptable[( i*16+j )*3+1] = ( GLbyte )(128 + floor( 127 * v[1] ) );
			temptable[( i*16+j )*3+2] = ( GLbyte )(128 + floor( 127 * v[2] ) );
		}
	}
#endif

	// color 255 will be the "nullnormal" color for no reflection
	temptable[255*3+0] =
	temptable[255*3+1] =
	temptable[255*3+2] = 128;

// this needs changing its out of date
	if ( !qglConfig.isSharedTPalette ) {
		return;
	}
	// this needs changing its out of date
	qglColorTableEXT( GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE, temptable );
	qglEnable( GL_SHARED_TEXTURE_PALETTE_EXT );
}


/*
===============
R_ReloadImages_f

Regenerate all images that came directly from files that have changed, so
any saved changes will show up in place.

New r_texturesize/r_texturedepth variables will take effect on reload

reloadImages <all>
===============
*/
void R_ReloadImages_f( const anCommandArgs &args ) {
	bool all = false;

	if ( args.Argc() == 2 ) {
		if ( !anString::Icmp( args.Argv( 1 ), "all" ) ) {
			all = true;
		} else {
			common->Printf( "USAGE: reloadImages <all>\n" );
			return;
		}
	}

	globalImages->ReloadImages( all );
}

typedef struct {
	anImage	*image;
	int		size;
	int		index;
} sortedImage_t;

/*
=======================
R_QSortImageSizes
=======================
*/
static int R_QSortImageSizes( const void *a, const void *b ) {
	const sortedImage_t	*ea, *eb;

	ea = ( sortedImage_t *)a;
	eb = ( sortedImage_t *)b;

	if ( ea->size > eb->size ) {
		return -1;
	}
	if ( ea->size < eb->size ) {
		return 1;
	}
	return anString::Icmp( ea->image->GetName(), eb->image->GetName() );
}

/*
=======================
R_QsortImageName
=======================
*/
static int R_QsortImageName( const void* a, const void* b ) {
	const sortedImage_t	*ea, *eb;

	ea = ( sortedImage_t *)a;
	eb = ( sortedImage_t *)b;

	return anString::Icmp( ea->image->GetName(), eb->image->GetName() );
}

/*
===============
R_ListImages_f
===============
*/
void R_ListImages_f( const anCommandArgs &args ) {
	int		i, partialSize;
	anImage	*image;
	int		totalSize;
	int		count = 0;
	bool	uncompressedOnly = false;
	bool	unloaded = false;
	bool	failed = false;
	bool	sorted = false;
	bool	duplicated = false;
	bool	overSized = false;
	bool	sortByName = false;

	if ( args.Argc() == 1 ) {

	} else if ( args.Argc() == 2 ) {
		if ( anString::Icmp( args.Argv( 1 ), "uncompressed" ) == 0 ) {
			uncompressedOnly = true;
		} else if ( anString::Icmp( args.Argv( 1 ), "sorted" ) == 0 ) {
			sorted = true;
		} else if ( anString::Icmp( args.Argv( 1 ), "namesort" ) == 0 ) {
			sortByName = true;
		} else if ( anString::Icmp( args.Argv( 1 ), "unloaded" ) == 0 ) {
			unloaded = true;
		} else if ( anString::Icmp( args.Argv( 1 ), "duplicated" ) == 0 ) {
			duplicated = true;
		} else if ( anString::Icmp( args.Argv( 1 ), "oversized" ) == 0 ) {
			sorted = true;
			overSized = true;
		} else {
			failed = true;
		}
	} else {
		failed = true;
	}

	if ( failed ) {
		common->Printf( "usage: listImages [ sorted | namesort | unloaded | duplicated | showOverSized ]\n" );
		return;
	}

	const char *header = "       -w-- -h-- filt -fmt-- wrap  size --name-------\n";
	common->Printf( "\n%s", header );

	totalSize = 0;

	sortedImage_t *sortedArray = ( sortedImage_t *) Mem_Alloc( sizeof( sortedImage_t ) * globalImages->images.Num() );

	for ( i = 0 ; i < globalImages->images.Num() ; i++ ) {
		image = globalImages->images[i];
		if ( uncompressedOnly ) {
			if ( image->IsCompressed() ) {
				continue;
			}
		}
		if ( unloaded == image->IsLoaded() ) {
			continue;
		}

		// only print duplicates (from mismatched wrap / clamp, etc)
		if ( duplicated ) {
			int j;
			for ( j = i+1 ; j < globalImages->images.Num() ; j++ ) {
				if ( anString::Icmp( image->GetName(), globalImages->images[ j ]->GetName() ) == 0 ) {
					break;
				}
			}
			if ( j == globalImages->images.Num() ) {
				continue;
			}
		}

		if ( sorted || sortByName ) {
			sortedArray[count].image = image;
			sortedArray[count].size = image->StorageSize();
			sortedArray[count].index = i;
		} else {
			common->Printf( "%4i:",	i );
			image->Print();
		}
		totalSize += image->StorageSize();
		count++;
	}

	if ( sorted || sortByName ) {
		if ( sortByName ) {
			qsort( sortedArray, count, sizeof( sortedImage_t ), R_QsortImageName );
		} else {
			qsort( sortedArray, count, sizeof( sortedImage_t ), R_QSortImageSizes );
		}
		partialSize = 0;
		for ( i = 0 ; i < count ; i++ ) {
			common->Printf( "%4i:",	sortedArray[i].index );
			sortedArray[i].image->Print();
			partialSize += sortedArray[i].image->StorageSize();
			if ( ( (i+1) % 10 ) == 0 ) {
				common->Printf( "-------- %5.1f of %5.1f megs --------\n",
					partialSize / ( 1024*1024.0 ), totalSize / ( 1024*1024.0 ) );
			}
		}
	}

	common->Printf( "%s", header );
	common->Printf( " %i images (%i total)\n", count, globalImages->images.Num() );
	common->Printf( " %5.1f total megabytes of images\n\n\n", totalSize / ( 1024*1024.0 ) );
}

/*
==============
AllocImage

Allocates an anImage, adds it to the list,
copies the name, and adds it to the hash chain.
==============
*/
anImage *anImageManager::AllocImage( const char *name ) {
	anImage * image = new anImage( name );
	int hash = anString( name ).FileNameHash();
	if ( !image ) {
		common->Warning( "Failed to allocate memory for image %s\n", name )
	}

	if ( strlen( name ) >= MAX_IMAGE_NAME ) {
		common->Error( "ImageManager::AllocImage: \"%s\" is too long\n", name );
	}

	imageHash.Add( hash, images.Append( image ) );

	image->hashNext = imageHashTable[hash];
	imageHashTable[hash] = image;
	image->imgName = name;

	return image;
}

/*
==============
AllocStandaloneImage

Allocates an anImage, does not add it to the list or hash chain
==============
*/
anImage *anImageManager::AllocStandaloneImage( const char *name ) {
	if  ( strlen( name ) >= MAX_IMAGE_NAME ) {
		common->Error( "ImageManager::AllocImage: \"%s\" is too long\n", name );
	}

	anImage * image = new anImage( name );

	return image;
}

/*
===============
GetImageWithParameters
==============
*/
anImage	*anImageManager::GetImageWithParameters( const char *_name, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage, cubeFiles_t cubeMap ) const {
	if ( !_name || !_name[0] || anString::Icmp( _name, "default" ) == 0 || anString::Icmp( _name, "_default" ) == 0 ) {
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}

	if ( anString::Icmpn( _name, "fonts", 5 ) == 0 || anString::Icmpn( _name, "newFonts", 8 ) == 0 ) {
		usage = TD_FONT;
	}

	if ( anString::Icmpn( _name, "lights", 6 ) == 0 ) {
		usage = TD_LIGHT;
	}

	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	anStaticString<MAX_OSPATH> name = _name;

	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	int hash = name.FileNameHash();

	for ( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) ) {
		anImage * image = images[i];
		if ( name.Icmp( image->GetName() ) == 0 ) {
			// the built in's, like _white and _flat always match the other options
			if ( name[0] == '_' ) {
				return image;
			}

			if ( image->cubeFiles != cubeMap ) {
				common->Error( "Image '%s' has been referenced with conflicting cube map states", _name );
			}

			if ( image->filter != filter || image->repeat != repeat ) {
				// we might want to have the system reset these parameters on every bind and
				// share the image data
				continue;
			}

			if ( image->usage != usage ) {
				// If an image is used differently then we need 2 copies of it because usage affects the way it's compressed and swizzled
				continue;
			}

			return image;
		}
	}

	return nullptr;
}

/*
===============
ImageFromFile

Finds or loads the given image, always returning a valid image pointer.
Loading of the image may be deferred for dynamic loading.
==============
*/
anImage *anImageManager::ImageFromFile( const char *_name, textureFilter_t filter, bool allowDownSize, textureRepeat_t repeat, textureDepth_t depth, cubeFiles_t cubeMap ) {
	if ( !_name || !_name[0] || anString::Icmp( _name, "default" ) == 0 || anString::Icmp( _name, "_default" ) == 0 ) {
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}

	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	anString name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	//
	// see if the image is already loaded, unless we
	// are in a reloadImages call
	//
	int hash = name.FileNameHash();
	for ( anImage * image = imageHashTable[hash]; image; image = image->hashNext ) {
		if ( name.Icmp( image->imgName ) == 0 ) {
			// the built in's, like _white and _flat always match the other options
			if ( name[0] == '_' ) {
				return image;
			}
			if ( image->cubeFiles != cubeMap ) {
				common->Error( "Image '%s' has been referenced with conflicting cube map states", _name );
			}

			if ( image->filter != filter || image->repeat != repeat ) {
				// we might want to have the system reset these parameters on every bind and
				// share the image data
				continue;
			}

			if ( image->allowDownSize == allowDownSize && image->depth == depth ) {
				// note that it is used this level load
				image->levelLoadReferenced = true;
				if ( image->partialImage != nullptr ) {
					image->partialImage->levelLoadReferenced = true;
				}
				return image;
			}

			// the same image is being requested, but with a different allowDownSize or depth
			// so pick the highest of the two and reload the old image with those parameters
			if ( !image->allowDownSize ) {
				allowDownSize = false;
			}
			if ( image->depth > depth ) {
				depth = image->depth;
			}
			if ( image->allowDownSize == allowDownSize && image->depth == depth ) {
				// the already created one is already the highest quality
				image->levelLoadReferenced = true;
				if ( image->partialImage != nullptr ) {
					image->partialImage->levelLoadReferenced = true;
				}
				return image;
			}

			image->allowDownSize = allowDownSize;
			image->depth = depth;
			image->levelLoadReferenced = true;
			if ( image->partialImage != nullptr ) {
				image->partialImage->levelLoadReferenced = true;
			}
			if ( image_preload.GetBool() && !insideLevelLoad ) {
				image->referencedOutsideLevelLoad = true;
				image->ActuallyLoadImage( true, false );	// check for precompressed, load is from front end
				declManager->MediaPrint( "%ix%i %s (reload for mixed referneces)\n", image->uploadWidth, image->uploadHeight, image->imgName.c_str() );
			}
			return image;
		}
	}

	//
	// create a new image
	//
	image = AllocImage( name );

	// HACK: to allow keep fonts from being mip'd, as new ones will be introduced with localization
	// this keeps us from having to make a material for each font tga
	if ( name.Find( "fontImage_" ) >= 0 ) {
		allowDownSize = false;
	}

	image->allowDownSize = allowDownSize;
	image->repeat = repeat;
	image->depth = depth;
	image->type = TT_2D;
	image->cubeFiles = cubeMap;
	image->filter = filter;

	image->levelLoadReferenced = true;

	// also create a shrunken version if we are going to dynamically cache the full size image
	if ( image->ShouldImageBePartialCached() ) {
		// if we only loaded part of the file, create a new anImage for the shrunken version
		image->partialImage = new anImage;
		image->partialImage->allowDownSize = allowDownSize;
		image->partialImage->repeat = repeat;
		image->partialImage->depth = depth;
		image->partialImage->type = TT_2D;
		image->partialImage->cubeFiles = cubeMap;
		image->partialImage->filter = filter;
		image->partialImage->levelLoadReferenced = true;

		// we don't bother hooking this into the hash table for lookup, but we do add it to the manager
		// list for listImages
		globalImages->images.Append( image->partialImage );
		image->partialImage->imgName = image->imgName;
		image->partialImage->isPartialImage = true;

		// let the background file loader know that we can load
		image->precompressedFile = true;

		if ( image_preload.GetBool() && !insideLevelLoad ) {
			image->partialImage->ActuallyLoadImage( true, false );	// check for precompressed, load is from front end
			declManager->MediaPrint( "%ix%i %s\n", image->partialImage->uploadWidth, image->partialImage->uploadHeight, image->imgName.c_str() );
		} else {
			declManager->MediaPrint( "%s\n", image->imgName.c_str() );
		}
		return image;
	}

	// load it if we aren't in a level preload
	if ( image_preload.GetBool() && !insideLevelLoad ) {
		image->referencedOutsideLevelLoad = true;
		image->ActuallyLoadImage( true, false );	// check for precompressed, load is from front end
		declManager->MediaPrint( "%ix%i %s\n", image->uploadWidth, image->uploadHeight, image->imgName.c_str() );
	} else {
		declManager->MediaPrint( "%s\n", image->imgName.c_str() );
	}

	return image;
}

/*
==================
ImageFromFunction

Images that are procedurally generated are allways specified
with a callback which must work at any time, allowing the OpenGL
system to be completely regenerated if needed.
==================
*/
anImage *anImageManager::ImageFromFunction( const char *_name, void (*generatorFunction)( anImage *image ) ) {
	if ( !name ) {
		common->FatalError( "ImageManager::ImageFromFunction: nullptr name" );
	}

	// strip any .tga file extensions from anywhere in the _name
	anString name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	// see if the image already exists
	int hash = name.FileNameHash();
	for ( anImage * image = imageHashTable[hash]; image; image = image->hashNext ) {
		if ( name.Icmp( image->imgName ) == 0 ) {
			if ( image->generatorFunction != generatorFunction ) {
				common->DPrintf( "WARNING: reused image %s with mixed generators\n", name.c_str() );
			}
			return image;
		}
	}

	// create the image and issue the callback
	anImage *image = AllocImage( name );

	image->generatorFunction = generatorFunction;

	if ( image_preload.GetBool() ) {
		// check for precompressed, load is from the front end
		image->referencedOutsideLevelLoad = true;
		image->ActuallyLoadImage( true, false );
	}

	return image;
}

idMegaTexture *anImageManager::MegaTextureFromFile( const char *name ) {
	megaTexture->InitFromMegaFile( name );
}

/*
===============
anImageManager::GetImage
===============
*/
anImage *anImageManager::GetImage( const char *_name ) const {
	if ( !_name || !_name[0] || anString::Icmp( _name, "default" ) == 0 || anString::Icmp( _name, "_default" ) == 0 ) {
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}

	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	anString name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	//
	// look in loaded images
	//
	int hash = name.FileNameHash();
	for ( anImage *image = imageHashTable[hash]; image; image = image->hashNext ) {
		if ( name.Icmp( image->imgName ) == 0 ) {
			return image;
		}
	}

	return nullptr;
}

/*
===============
PurgeAllImages
===============
*/
void anImageManager::PurgeAllImages() {
	for ( int i = 0; i < images.Num(); i++ ) {
		anImage *image = images[i];
		image->PurgeImage();
	}
}

/*
===============
ReloadAllImages
===============
*/
void anImageManager::ReloadAllImages() {
	anCommandArgs args;
	// build the compressed normal map palette
	SetNormalPalette();
	args.TokenizeString( "reloadImages reload", false );
	R_ReloadImages_f( args );
}

/*
===============
ReloadImages
===============
*/
void anImageManager::ReloadImages( bool all ) {
	for ( int i = 0 ; i < globalImages->images.Num() ; i++ ) {
		globalImages->images[i]->Reload( all );
	}
}

/*
===============
anImageManager::UnbindAll
===============
*/
void anImageManager::UnbindAll() {
	int oldtmu = backEnd.qglState.currenttmu;
	for ( int i = 0; i < MAX_PROG_TEXTURE_PARMS; ++i ) {
		backEnd.qglState.currenttmu = i;
		BindNull();
	}
	backEnd.qglState.currenttmu = oldtmu;
}

void idImageManager::BindNull() {
	RB_Printf( "[Image Manager] Bind Null\n" )
}

/*
==================
anImage::StartBackgroundImageLoad
==================
*/
void anImage::StartBackgroundImageLoad() {
	if ( imageManager.numActiveBackgroundImageLoads >= anImageManager::MAX_BACKGROUND_IMAGE_LOADS ) {
		return;
	}
	if ( globalImages->image_showBackgroundLoads.GetBool() ) {
		common->Printf( "anImage::StartBackgroundImageLoad: %s\n", imgName.c_str() );
	}
	backgroundLoadInProgress = true;

	if ( !precompressedFile ) {
		common->Warning( "anImageManager::StartBackgroundImageLoad: %s wasn't a precompressed file", imgName.c_str() );
		return;
	}

	bglNext = globalImages->backgroundImageLoads;
	globalImages->backgroundImageLoads = this;

	char	filename[MAX_IMAGE_NAME];
	ImageProgramStringToCompressedFileName( imgName, filename );

	bgl.completed = false;
	bgl.f = fileSystem->OpenFileRead( filename );
	if ( !bgl.f ) {
		common->Warning( "anImageManager::StartBackgroundImageLoad: Couldn't load %s", imgName.c_str() );
		return;
	}
	bgl.file.position = 0;
	bgl.file.length = bgl.f->Length();
	if ( bgl.file.length < sizeof( ddsFileProperties_t ) ) {
		common->Warning( "anImageManager::StartBackgroundImageLoad: %s had a bad file length", imgName.c_str() );
		return;
	}

	bgl.file.buffer = R_StaticAlloc( bgl.file.length );

	fileSystem->BackgroundDownload( &bgl );

	imageManager.numActiveBackgroundImageLoads++;

	// purge some images if necessary
	int		totalSize = 0;
	for ( anImage *check = globalImages->cacheLRU.cacheUsageNext; check != &globalImages->cacheLRU; check = check->cacheUsageNext ) {
		totalSize += check->StorageSize();
	}
	int	needed = this->StorageSize();

	while ( ( totalSize + needed ) > globalImages->image_cacheMegs.GetFloat() * 1024 * 1024 ) {
		// purge the least recently used
		anImage	*check = globalImages->cacheLRU.cacheUsagePrev;
		if ( check->texnum != TEXTURE_NOT_LOADED ) {
			totalSize -= check->StorageSize();
			if ( globalImages->image_showBackgroundLoads.GetBool() ) {
				common->Printf( "purging %s\n", check->imgName.c_str() );
			}
			check->PurgeImage();
		}
		// remove it from the cached list
		check->cacheUsageNext->cacheUsagePrev = check->cacheUsagePrev;
		check->cacheUsagePrev->cacheUsageNext = check->cacheUsageNext;
		check->cacheUsageNext = nullptr;
		check->cacheUsagePrev = nullptr;
	}
}

/*
====================
anImageManager::Preload

rid of it soon?
====================
*/
void anImageManager::Preload(  const bool & mapPreload ) {
	if ( preLoad_Images.GetBool() ) {
		// preload this levels images
		common->Printf( "Preloading images...\n" );
		preloadingMapImages = mapPreload;
		int	start = Sys_Milliseconds();
		int numLoaded = 0;
		int	end = Sys_Milliseconds();
		common->Printf( "%05d images preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
		common->Printf( "----------------------------------------\n" );
		preloadingMapImages = false;
	}
}

/*
==================
R_CompleteBackgroundImageLoads

Do we need to worry about vid_restarts here?
==================
*/
void anImageManager::CompleteBackgroundImageLoads() {
	anImage *remainingList = nullptr;
	anImage *next;

	for ( anImage *image = backgroundImageLoads; image; image = next ) {
		next = image->bglNext;
		if ( image->bgl.completed ) {
			numActiveBackgroundImageLoads--;
			fileSystem->CloseFile( image->bgl.f );
			// upload the image
			image->UploadPrecompressedImage( (GLbyte *)image->bgl.file.buffer, image->bgl.file.length );
			R_StaticFree( image->bgl.file.buffer );
			if ( image_showBackgroundLoads.GetBool() ) {
				common->Printf( "R_CompleteBackgroundImageLoad: %s\n", image->imgName.c_str() );
			}
		} else {
			image->bglNext = remainingList;
			remainingList = image;
		}
	}
	if ( image_showBackgroundLoads.GetBool() ) {
		static int prev;
		if ( numActiveBackgroundImageLoads != prev ) {
			prev = numActiveBackgroundImageLoads;
			common->Printf( "background Loads: %i\n", numActiveBackgroundImageLoads );
		}
	}

	backgroundImageLoads = remainingList;
}

/*
===============
CheckCvars
===============
*/
void anImageManager::CheckCvars() {
	// textureFilter stuff
	if ( image_filter.IsModified() || image_anisotropy.IsModified() || image_lodbias.IsModified() ) {
		ChangeTextureFilter();
		image_filter.ClearModified();
		image_anisotropy.ClearModified();
		image_lodbias.ClearModified();
	}
}

/*
===============
SumOfUsedImages
===============
*/
int anImageManager::SumOfUsedImages() {
	int total = 0;
	for ( int i = 0; i < images.Num(); i++ ) {
		anImage *image = images[i];
		if ( image->frameUsed == backEnd.frameCount ) {
			total += image->StorageSize();
		} else {
			total += image->uploadWidth * image->uploadHeight;
		}
		commonh->Printf( " total..%d\n", total, image->frameUsed );
	}

	return total;
}

/*
===============
BindNull
===============
*/
void anImageManager::BindNull() {
	tmu_t tmu = &backEnd.qglState.tmu[backEnd.qglState.currenttmu];

	RB_LogComment( "BindNull()\n" );
	if ( tmu->textureType == TT_CUBIC ) {
		qglDisable( GL_TEXTURE_CUBE_MAP_EXT );
	} else if ( tmu->textureType == TT_CUBE_INFT ) {
		qglDisable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
	} else if ( tmu->textureType == TT_3D ) {
		qglDisable( GL_TEXTURE_3D );
	} else if ( tmu->textureType == TT_2D ) {
		qglDisable( GL_TEXTURE_2D );
	} else if ( tmu->textureType == TT_RECT ) {
		qglDisable( GL_TEXTURE_RECTANGLE_EXT );
	}
	tmu->textureType = TT_DISABLED;
}

/*
===============
Init
===============
*/
void anImageManager::Init() {
	images.Resize( 1024, 1024 );
	imageHashTable.ResizeIndex( 1024 );

	// clear the cached LRU
	cacheLRU.cacheUsageNext = &cacheLRU;
	cacheLRU.cacheUsagePrev = &cacheLRU;

	// set default texture filter modes
	ChangeTextureFilter();

	// create built in images
	defaultImage = ImageFromFunction( "_default", R_DefaultImage );
	whiteImage = ImageFromFunction( "_white", R_WhiteImage );
	blackImage = ImageFromFunction( "_black", R_BlackImage );
	borderClampImage = ImageFromFunction( "_borderClamp", R_BorderClampImage );
	flatNormalMap = ImageFromFunction( "_flat", R_FlatNormalImage );
	ambientNormalMap = ImageFromFunction( "_ambient", R_AmbientNormalImage );
	specularTableImage = ImageFromFunction( "_specularTable", R_SpecularTableImage );
	specular2DTableImage = ImageFromFunction( "_specular2DTable", R_Specular2DTableImage );
	rampImage = ImageFromFunction( "_ramp", R_RampImage );
	alphaRampImage = ImageFromFunction( "_alphaRamp", R_RampImage );
	alphaNotchImage = ImageFromFunction( "_alphaNotch", R_AlphaNotchImage );
	fogImage = ImageFromFunction( "_fog", R_FogImage );
	fogEnterImage = ImageFromFunction( "_fogEnter", R_FogEnterImage );
	normalCubeMapImage = ImageFromFunction( "_normalCubeMap", MakeNormalizeVectorCubeMap );
	noFalloffImage = ImageFromFunction( "_noFalloff", R_CreateNoFalloffImage );
	ImageFromFunction( "_quadratic", R_QuadraticImage );

	// cinematicImage is used for cinematic drawing
	// scratchImage is used for screen wipes/doublevision etc..
	cinematicImage = ImageFromFunction( "_cinematic", R_RGBA8Image );
	scratchImage = ImageFromFunction( "_scratch", R_RGBA8Image );
	scratchImage2 = ImageFromFunction( "_scratch2", R_RGBA8Image );
	accumImage = ImageFromFunction( "_accum", R_RGBA8Image );
	scratchCubeMapImage = ImageFromFunction( "_scratchCubeMap", MakeNormalizeVectorCubeMap );
	currentRenderImage = ImageFromFunction( "_currentRender", R_RGBA8Image );

	cmdSystem->AddCommand( "reloadImages", R_ReloadImages_f, CMD_FL_RENDERER, "reloads images" );
	cmdSystem->AddCommand( "listImages", R_ListImages_f, CMD_FL_RENDERER, "lists images" );
	cmdSystem->AddCommand( "combineCubeImages", R_CombineCubeImages_f, CMD_FL_RENDERER, "combines six images for roq compression" );

	// should forceLoadImages be here?
}

/*
===============
Shutdown
===============
*/
void anImageManager::Shutdown() {
	images.DeleteContents( true );
	imageHash.Clear();
	//imageHashTable.Clear();
}

/*
====================
anImageManager::ExcludePreloadImage
====================
*/
bool anImageManager::ExcludePreloadImage( const char *name ) {
	anStaticString<MAX_OSPATH> imgName = name;
	imgName.ToLower();
	if ( imgName.Find( "newfonts/", false ) >= 0 ) {
		return true;
	}
	if ( imgName.Find( "generated/", false ) >= 0 ) {
		return true;
	}
	if ( imgName.Find( "/loadscreens/", false ) >= 0 ) {
		return true;
	}
	return false;
}

/*
===============
anImageManager::LoadLevelImages

i dont think were gonna use this combine with begin level load?
===============
*/
int anImageManager::LoadLevelImages( bool pacifier ) {
	int	loadCount = 0;
	for ( int i = 0; i < images.Num(); i++ ) {
		//if ( pacifier ) {
			//common->UpdateLevelLoadPacifier();}

		anImage	*image = images[i];
		if ( image->generatorFunction ) {
			continue;
		}
		if ( image->levelLoadReferenced && !image->IsLoaded() ) {
			loadCount++;
			image->ActuallyLoadImage( false );
		}
	}
	return loadCount;
}

/*
====================
BeginLevelLoad

Mark all file based images as currently unused,
but don't free anything.  Calls to ImageFromFile() will
either mark the image as used, or create a new image without
loading the actual data.
====================
*/
void anImageManager::BeginLevelLoad() {//( bool pacifier ) {
	insideLevelLoad = true;
	int	loadCount = 0;
	for ( int i = 0; i < images.Num(); i++ ) {
		anImage *image = images[i];
		//if ( pacifier ) {
			//common->UpdateLevelLoadPacifier();}
		// generator function images are always kept around
		if ( image->generatorFunction ) {
			continue;
		}

		if ( !image->referencedOutsideLevelLoad && image->IsLoaded() ) {
			if ( com_purgeAll.GetBool() ) {
				image->PurgeImage();
			}
			if ( image->levelLoadReferenced && !image->IsLoaded() ) {
				loadCount++;
				image->ActuallyLoadImage( false );
			}
		}
		return loadCount;
	}
}

/*
===============
anImageManager::EndLevelLoad
===============
*/
void anImageManager::EndLevelLoad() {
	insideLevelLoad = false;
	int start = Sys_Milliseconds();
	int	loadCount = LoadLevelImages( true );
	int purgeCount = 0;
	int keepCount = 0;

	common->Printf( "-------[DeInitiating arC-Net]------\n" );
	common->Printf( "---------- [LevelLoad] ------------\n" );
	common->Printf( "---------[Image Manager]-------\n" );

	if ( int i = 0; 1 < images.Num(); i++ ) {
		anImage *image = images[i];
		if ( image->generatorFunction ) {
			continue;
		}
		if ( image->levelLoadReference && !image->referencedOutsideLevelLoad ) {
			common->Printf( "Purging %s \n", image->imgName.c_str() );
			purgeCount++;
			image->PurgeImage();
		} else if ( !texnum != anImage::TEXTURE_NOT_LOADED ) {
			common->Printf( "Not purging  %s \n", image->imgName.c_str() );
			keepCount++
			if ( int i = 0; i < images.Num(); i++ ) {
				anImage image = images[i];
				if ( image->generatorFunction ) {
					continue;
				}
				if ( image->levelLoadReference && images->texnum == anImage::TEXTURE_NOT_LOADED && !image->isPartial ) {
					common->Printf( "Loading %s\n", image->imgName.c_str() );
					loadCount++;
					image->ActuallyLoadImage( true, false );
					// this has to go its for seessions online dont need it
					// later on we will re implement networking codes.
					if ( loadCount & 15 ) {
						UpdateLevelLoadPacifier();
					}
				}
			}
		}
	}

	int	end = Sys_Milliseconds();
	common->Printf( "%5i purged from previous\n", purgeCount );
	common->Printf( "%5i kept from previous\n", keepCount );
	common->Printf( "%5i new loaded\n", loadCount );
	common->Printf( "%5i images loaded in %5.1f seconds\n", loadCount, (end-start) * 0.001 );
	common->Printf( "----------------------------------------\n" );
	//R_ListImages_f( anCommandArgs( "sorted sorted", false ) );
}

/*
===============
anImageManager::StartBuild
===============
*/
void anImageManager::StartBuild() {
	ddsList.Clear();
	ddsHash.Free();
}

/*
===============
anImageManager::FinishBuild
===============
*/
void anImageManager::FinishBuild( bool removeDups ) {
	anFile *batchFile;
	if ( removeDups ) {
		ddsList.Clear();
		if ( const char *buffer = nullptr; fileSystem->ReadFile( "makedds.bat", reinterpret_cast<void **>( &buffer ) ) ) {
			anString str = buffer;
			while ( str.Length() ) {
				int n = str.Find( '\n' );
				if ( n > 0 ) {
					anString line = str.Left( n + 1 );
					anString right;
					str.Right( str.Length() - n - 1, right );
					str = right;
					ddsList.AddUnique( line );
				} else {
					break;
				}
			}
		}
	}

    if ( anFile *batchFile = fileSystem->OpenFileWrite( ( removeDups ) ? "makedds2.bat" : "makedds.bat" ) ) {
		int ddsNum = ddsList.Num();
		for ( int i = 0; i < ddsNum; i++ ) {
			batchFile->WriteFloatString( "%s", ddsList[i].c_str() );
			batchFile->Printf( "@echo Finished compressing %d of %d.  %.1f percent done.\n", i+1, ddsNum, ( ( float )(i+1)/( float )ddsNum)*100.f );
		}
		fileSystem->CloseFile( batchFile );
	}
	ddsList.Clear();
	ddsHash.Free();
}

/*
===============
anImageManager::PrintMemInfo
===============
*/
void anImageManager::PrintMemInfo( MemInfo_t *mi ) {
	int total = 0;
	anFile *f = fileSystem->OpenFileWrite( mi->filebase + "_images.txt" );
	if ( !f ) {
		return;
	}

	// sort first
	int *sortIndex = new int[images.Num()];

	for ( i = 0; i < images.Num(); i++ ) {
		sortIndex[i] = i;
	}

	for ( int i = 0; i < images.Num() - 1; i++ ) {
		for ( int j = i + 1; j < images.Num(); j++ ) {
			if ( images[sortIndex[i]]->StorageSize() < images[sortIndex[j]]->StorageSize() ) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for ( int i = 0; i < images.Num(); i++ ) {
		anImage *im = images[sortIndex[i]];
		int size = im->StorageSize();
		total += size;

		f->Printf( "%s %3i %s\n", anString::FormatNumber( size ).c_str(), im->refCount, im->GetName() );
	}

	delete [] sortIndex;
	mi->imageAssetsTotal = total;

	f->Printf( "\nTotal image bytes allocated: %s\n", anString::FormatNumber( total ).c_str() );
	fileSystem->CloseFile( f );
}


/*
===============
anImageManager::AddDDSCommand
===============
*/
void anImageManager::AddDDSCommand( const char *cmd ) {
	int i, key;

	if ( !( cmd && *cmd ) ) {
		return;
	}

	key = ddsHash.GenerateKey( cmd, false );
	for ( i = ddsHash.First( key ); i != -1; i = ddsHash.Next( i ) ) {
		if ( ddsList[i].Icmp( cmd ) == 0 ) {
			break;
		}
	}

	if ( i == -1 ) {
		ddsList.Append( cmd );
	}
}
