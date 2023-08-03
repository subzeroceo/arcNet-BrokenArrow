
#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"

anCVar idMegaTexture::r_megaTextureLevel( "r_megaTextureLevel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw only a specific level" );
anCVar idMegaTexture::r_showMegaTexture( "r_showMegaTexture", "0", CVAR_RENDERER | CVAR_BOOL, "display all the level images" );
anCVar idMegaTexture::r_showMegaTextureLabels( "r_showMegaTextureLabels", "0", CVAR_RENDERER | CVAR_BOOL, "draw colored blocks in each tile" );
anCVar idMegaTexture::r_skipMegaTexture( "r_skipMegaTexture", "0", CVAR_RENDERER | CVAR_INTEGER, "only use the lowest level image" );
anCVar idMegaTexture::r_terrainScale( "r_terrainScale", "3", CVAR_RENDERER | CVAR_INTEGER, "vertically scale USGS data" );

/*

allow sparse population of the upper detail tiles

*/

int RoundDownToPowerOfTwo( int num ) {
	for ( int pot = 1; ( pot * 2 ) <= num ; pot <<= 1 ) {
	}
	return pot;
}

static union {
	int		intVal;
	byte	color[4];
} fillColor;

static byte	colors[8][4] = {
	{ 0, 0, 0, 255 },
	{ 255, 0, 0, 255 },
	{ 0, 255, 0, 255 },
	{ 255, 255, 0, 255 },
	{ 0, 0, 255, 255 },
	{ 255, 0, 255, 255 },
	{ 0, 255, 255, 255 },
	{ 255, 255, 255, 255 }
};

static void R_EmptyLevelImage( anImage *image ) {
	int	c = MAX_LEVEL_WIDTH * MAX_LEVEL_WIDTH;
	byte *data = (byte *)_alloca( c*4 );

	for ( int i = 0 ; i < c ; i++ ) {
		( ( int*)data )[i] = fillColor.intVal;
	}

	// FIXME: this won't live past vid mode changes
	image->GenerateImage( data, MAX_LEVEL_WIDTH, MAX_LEVEL_WIDTH, TF_DEFAULT, false, TR_REPEAT, TD_HIGH_QUALITY );
}

/*
====================
InitFromMegaFile
====================
*/
bool idMegaTexture::InitFromMegaFile( const char *fileBase ) {
	anString	name = "megaTexture/";
	name += fileBase;
	name.StripFileExtension();
	name += ".mgat";

	fileHandle = fileSystem->OpenFileRead( name.c_str() );
	if ( !fileHandle ) {
		common->Printf( "[idMegaTexture] Failed to open %s\n", name.c_str() );
		return false;
	}

	fileHandle->Read( &header, sizeof( header ) );
	if ( header.tileSize < 64 || header.tilesWide < 1 || header.tilesHigh < 1 ) {
		common->Printf( "[idMegaTexture] bad header on %s\n", name.c_str() );
		return false;
	}

	currentTriMapping = nullptr;

	numLevels = 0;
	int width = header.tilesWide;
	int height = header.tilesHigh;

	int	tileOffset = 1;					// just past the header

	memset( levels, 0, sizeof( levels ) );
	while ( 1 ) {
		idTextureLevel *level = &levels[numLevels];

		level->mega = this;
		level->tileOffset = tileOffset;
		level->tilesWide = width;
		level->tilesHigh = height;
		level->parms[0] = -1;		// initially mask everything
		level->parms[1] = 0;
		level->parms[2] = 0;
		level->parms[3] = ( float )width / TILE_PER_LEVEL;
		level->Invalidate();

		tileOffset += level->tilesWide * level->tilesHigh;

		char str[1024];
		sprintf( str, "mgat_%s_%i", fileBase, numLevels );

		// give each level a default fill color
		for ( int i = 0 ; i < 4 ; i++ ) {
			fillColor.color[i] = colors[numLevels+1][i];
		}

		levels[numLevels].image = globalImages->ImageFromFunction( str, R_EmptyLevelImage );
		numLevels++;

		if ( width <= TILE_PER_LEVEL && height <= TILE_PER_LEVEL ) {
			break;
		}
		width = ( width + 1 ) >> 1;
		height = ( height + 1 ) >> 1;
	}

	// force first bind to load everything
	currentViewOrigin[0] = -99999999.0f;
	currentViewOrigin[1] = -99999999.0f;
	currentViewOrigin[2] = -99999999.0f;

	return true;
}

/*
===========================
rvmMegaTextureFile::LoadMegaTextureFile
===========================
*/
idMegaTextureFile *idMegaTextureFile::LoadMegaTextureFile( const char *name ) {
	int width, height;
	idMegaTextureFile *megaTextureFile = new idMegaTextureFile();

	megaTextureFile->fileHandle = fileSystem->OpenFileRead( name );
	if ( !megaTextureFile->fileHandle) {
		common->Printf( "idMegaTextureFile: failed to open %s\n", name );
		delete megaTextureFile;
		return nullptr;
	}

	megaTextureFile->fileHandle->Read( &megaTextureFile->header, sizeof (megaTextureFile->header ) );
	if ( megaTextureFile->header.tileSize < 64 || megaTextureFile->header.tilesWide < 1 || megaTextureFile->header.tilesHigh < 1 ) {
		common->Printf( "idMegaTexture: bad header on %s\n", name );
		delete megaTextureFile;
		return nullptr;
	}

	megaTextureFile->numLevels = 0;
	width = megaTextureFile->header.tilesWide;
	height = megaTextureFile->header.tilesHigh;

	int	tileOffset = 1;					// just past the header

	memset( megaTextureFile->levels, 0, sizeof( levels ));
	while ( 1 ) {
		idTextureLevel *level = &megaTextureFile->levels[megaTextureFile->numLevels];

		level->mega = megaTextureFile;
		level->tileOffset = tileOffset;
		level->tilesWide = width;
		level->tilesHigh = height;
		level->parms[0] = -1;		// initially mask everything
		level->parms[1] = 0;
		level->parms[2] = 0;
		level->parms[3] = ( float )width / ( float )TILE_PER_LEVEL;
		level->Invalidate();

		tileOffset += level->tilesWide * level->tilesHigh;

		char str[1024];
		sprintf( str, "_mega_%i", megaTextureFile->numLevels );

		// give each level a default fill color
		for ( int i = 0; i < 4; i++ ) {
			fillColor.color[i] = colors[megaTextureFile->numLevels + 1][i];
		}
		// jmarshall - adapted to idTech 5 image code from BFG
		idImageOpts opts;
		opts.format = FMT_DXT5;
		opts.colorFormat = CFM_DEFAULT;
		opts.gammaMips = 0;
		opts.width = MAX_LEVEL_WIDTH;
		opts.height = MAX_LEVEL_WIDTH;
		opts.textureType = TT_2D;
		opts.isPersistant = true;
		opts.numMSAASamples = 0;
		opts.numLevels = 1;

		anTempArray<byte>data( MAX_LEVEL_WIDTH * MAX_LEVEL_WIDTH * 4 );

		for ( inti = 0; i < MAX_LEVEL_WIDTH * MAX_LEVEL_WIDTH; i++ ) {
			( ( int*)data.Ptr())[i] = fillColor.intVal;
		}

		megaTextureFile->levels[megaTextureFile->numLevels].image = globalImages->ScratchImage( str, &opts, TF_LINEAR, TR_REPEAT, TD_DIFFUSE );
		megaTextureFile->levels[megaTextureFile->numLevels].image->UploadScratch( data.Ptr(), MAX_LEVEL_WIDTH, MAX_LEVEL_WIDTH );
		megaTextureFile->numLevels++;

		if ( width <= TILE_PER_LEVEL && height <= TILE_PER_LEVEL ) {
			break;
		}
		width = ( width + 1 ) >> 1;
		height = ( height + 1 ) >> 1;
	}

	return megaTextureFile;
}
/*
====================
SetMappingForSurface

analyzes xyz and st to create a mapping
This is not very robust, but works for rectangular grids
====================
*/
void idMegaTexture::SetMappingForSurface( const srfTriangles_t *tri ) {
	if ( tri == currentTriMapping ) {
		return;
	}
	currentTriMapping = tri;

	if ( !tri->verts ) {
		return;
	}

	anDrawVertex origin, axis[2];

	origin.st[0] = 1.0;
	origin.st[1] = 1.0;

	axis[0].st[0] = 0;
	axis[0].st[1] = 1;

	axis[1].st[0] = 1;
	axis[1].st[1] = 0;

	for ( int i = 0 ; i < tri->numVerts ; i++ ) {
		anDrawVertex *v = &tri->verts[i];
		if ( v->st[0] <= origin.st[0] && v->st[1] <= origin.st[1] ) {
			origin = *v;
		}
		if ( v->st[0] >= axis[0].st[0] && v->st[1] <= axis[0].st[1] ) {
			axis[0] = *v;
		}
		if ( v->st[0] <= axis[1].st[0] && v->st[1] >= axis[1].st[1] ) {
			axis[1] = *v;
		}
	}

	for ( int i = 0 ; i < 2 ; i++ ) {
		anVec3	dir = axis[i].xyz - origin.xyz;
		float texLen = axis[i].st[i] - origin.st[i];
		float spaceLen = ( axis[i].xyz - origin.xyz ).Length();

		float scale = texLen / ( spaceLen*spaceLen );
		dir *= scale;

		float c = origin.xyz * dir - origin.st[i];

		localViewToTextureCenter[i][0] = dir[0];
		localViewToTextureCenter[i][1] = dir[1];
		localViewToTextureCenter[i][2] = dir[2];
		localViewToTextureCenter[i][3] = -c;
	}
}

/*
====================
BindForViewOrigin
====================
*/
void idMegaTexture::BindForViewOrigin( const anVec3 viewOrigin ) {
	SetViewOrigin( viewOrigin );

	// borderClamp image goes in texture 0
	GL_SetCurrentTextureUnit( 0 );
	globalImages->borderClampImage->Bind();

	// level images in higher textures, blurriest first
	for ( int i = 0 ; i < 7 ; i++ ) {
		GL_SetCurrentTextureUnit( 1+i );
		if ( i >= numLevels ) {
			globalImages->whiteImage->Bind();
			static float parms[4] = { -2, -2, 0, 1 };	// no contribution
			qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, i, parms );
		} else {
			idTextureLevel *level = &levels[ numLevels-1-i ];
			if ( r_showMegaTexture.GetBool() ) {
				if ( i & 1 ) {
					globalImages->blackImage->Bind();
				} else {
					globalImages->whiteImage->Bind();
				}
			} else {
				level->image->Bind();
			}
			qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, i, level->parms );
		}
	}

	float	parms[4];
	parms[0] = 0;
	parms[1] = 0;
	parms[2] = 0;
	parms[3] = 1;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 7, parms );

	parms[0] = 1;
	parms[1] = 1;
	parms[2] = r_terrainScale.GetFloat();
	parms[3] = 1;
	qglProgramLocalParameter4fvARB( GL_VERTEX_PROGRAM_ARB, 8, parms );
}

/*
====================
Unbind

This can go away once everything uses fragment programs so the enable states don't
need tracking
====================
*/
void idMegaTexture::Unbind( void ) {
	for ( int i = 0 ; i < numLevels ; i++ ) {
		GL_SetCurrentTextureUnit( 1+i );
		globalImages->BindNull();
	}
}

/*
====================
SetViewOrigin
====================
*/
void idMegaTexture::SetViewOrigin( const anVec3 viewOrig ) {
	if ( r_showMegaTextureLabels.IsModified() ) {
		r_showMegaTextureLabels.ClearModified();
		currentViewOrigin[0] = viewOrigin[0] + 0.1f;	// force a change
		for ( int i = 0; i < numLevels; i++ ) {
			levels[i].Invalidate();
		}
	}

	if ( viewOrig == currentViewOrigin || r_skipMegaTexture.GetBool() ) {
		return;
	}

	currentViewOrigin = viewOrig;

	float texCenter[2];

	// convert the viewOrigin to a texture center, which will
	// be a different conversion for each megaTexture
	for ( int i = 0; i < 2; i++ ) {
		texCenter[i] =
			viewOrigin[0] * localViewToTextureCenter[i][0] +
			viewOrigin[1] * localViewToTextureCenter[i][1] +
			viewOrigin[2] * localViewToTextureCenter[i][2] +
			localViewToTextureCenter[i][3];
	}

	for ( int i = 0; i < numLevels; i++ ) {
		levels[i].UpdateForCenter( texCenter );
	}
}
/*
========================
idMegaTextureFile::ReadTile
========================
*/
void idMegaTextureFile::ReadTile( byte *tileBuffer, int tileNum ) {
	int tileSize = TILE_SIZE * TILE_SIZE;

	fileHandle->Seek( tileNum * tileSize, FS_SEEK_SET );
	//memset(data, 128, sizeof(data));
	fileHandle->Read( tileBuffer, tileSize );
}

/*
===========================
idMegaTextureFile::UpdateForCenter
===========================
*/
void idMegaTextureFile::UpdateForCenter( float texCenter[2] ) {
	for ( int i = 0; i < numLevels; i++ ) {
		levels[i].UpdateForCenter( texCenter );
	}
}
/*
====================
UpdateTile

//int	byteSize = size * 4;
A local tile will only be mapped to globalTile[ localTile + X * TILE_PER_LEVEL ] for some x
====================
*/
void idTextureLevel::UpdateTile( int localX, int localY, int globalX, int globalY ) {
	idTextureTile *tile = &tileMap[localX][localY];

	if ( tile->x == globalX && tile->y == globalY ) {
		return;
	}

    if ( ( globalX & static_cast<int>( TILE_PER_LEVEL - 1 ) ) != localX || ( globalY & static_cast<int>( TILE_PER_LEVEL - 1 ) ) != localY ) {
	//if ( ( globalX & ( TILE_PER_LEVEL-1 ) ) != localX || ( g lobalY & ( TILE_PER_LEVEL-1 ) ) != localY ) {
		common->Error( "idTextureLevel::UpdateTile: bad coordinate mod" );
	}

	tile->x = globalX;
	tile->y = globalY;

	byte data[ TILE_SIZE * TILE_SIZE * 4 ];

	if ( globalX >= tilesWide || globalX < 0 || globalY >= tilesHigh || globalY < 0 ) {
		// off the map
		memset( data, 0, sizeof( data ) );
	} else {
		// extract the data from the full image (FIXME: background load from disk)
		int tileNum = tileOffset + tile->y * tilesWide + tile->x;
		int tileSize = TILE_SIZE * TILE_SIZE * 4;
		mega->fileHandle->Seek( tileNum * tileSize, FS_SEEK_SET );
		memset( data, 128, sizeof( data ) );
		mega->fileHandle->Read( data, tileSize );
		if ( idMegaTexture::r_showMegaTextureLabels.GetBool() ) {
			// put a color marker in it
			byte color[4] = { static_cast<byte>( 255 * localX / TILE_PER_LEVEL ), static_cast<byte>( 255 * localY / TILE_PER_LEVEL ), 0, 0};
			for ( int x = 0; x < 8; x++ ) {
				for ( int y = 0; y < 8; y++ ) {
					*( int*) & data[ ( ( y + TILE_SIZE / 2 - 4 ) * TILE_SIZE + x + TILE_SIZE / 2 - 4 ) * 4 ] = *( int*)color;
				}
			}
		}
	}

	// upload all the mip-map levels
	int	level = 0;
	int size = TILE_SIZE;
	while ( size > 0 ) {
		qglTexSubImage2D( GL_TEXTURE_2D, level, localX * size, localY * size, size, size, GL_RGBA, GL_UNSIGNED_BYTE, data );
		size >>= 1;
		level++;
		if ( size == 0 ) {
			break;
		}
	}

	/*if ( r_useOptimizedCode.GetBool() )
	for ( int y = 0; y < size; y++ ) {
		byte *in = data + y * size * 16;
		byte *in2 = in + size * 8;
		byte *out = data + y * size * 4;
		for ( int x = 0; x < size; x++ ) {
			for ( int c = 0; c < 4; c++ ) {
				int sum = 0;
				sum += in[x * 8 + c];
				sum += in[x * 8 + 4 + c];
				sum += in2[x * 8 + c];
				sum += in2[x * 8 + 4 + c];
				out[x * 4 + c] = sum >> 2;
		}*/
		// mip-map in place
	for ( int y = 0; y < size; y++ ) {
		byte *in, *in2, *out;
		in = data + y * size * 16;
		in2 = in + size * 8;
		out = data + y * size * 4;
		for ( int x = 0; x < size; x++ ) {
			out[x*4+0] = ( in[x*8+0] + in[x*8+4+0] + in2[x*8+0] + in2[x*8+4+0] ) >> 2;
			out[x*4+1] = ( in[x*8+1] + in[x*8+4+1] + in2[x*8+1] + in2[x*8+4+1] ) >> 2;
			out[x*4+2] = ( in[x*8+2] + in[x*8+4+2] + in2[x*8+2] + in2[x*8+4+2] ) >> 2;
			out[x*4+3] = ( in[x*8+3] + in[x*8+4+3] + in2[x*8+3] + in2[x*8+4+3] ) >> 2;
		}
	}
}
/*void idTextureLevel::UpdateTile( intlocalX, int localY, int globalX, int globalY) {
    idTextureTile *tile = &tileMap[localX][localY];

    if (tile->x == globalX && tile->y == globalY) {
        return;
    }

    if ((globalX & (TILE_PER_LEVEL - 1)) != localX || (globalY & (TILE_PER_LEVEL - 1)) != localY) {
        common->Error( "idTextureLevel::UpdateTile: bad coordinate mod" );
    }

    tile->x = globalX;
    tile->y = globalY;

    byte data[TILE_SIZE * TILE_SIZE * 4];

    if (globalX >= tilesWide || globalX < 0 || globalY >= tilesHigh || globalY < 0) {
        memset(data, 0, sizeof(data));
    } else {
        int tileNum = tileOffset + tile->y * tilesWide + tile->x;
        int tileSize = TILE_SIZE * TILE_SIZE * 4;

        mega->fileHandle->Seek(tileNum * tileSize, FS_SEEK_SET);
        memset(data, 128, sizeof(data));
        mega->fileHandle->Read(data, tileSize);
    }

    if (idMegaTexture::r_showMegaTextureLabels.GetBool()) {
        byte color[4] = {255 * localX / TILE_PER_LEVEL, 255 * localY / TILE_PER_LEVEL, 0, 0};
        for ( intx = 0; x < 8; x++ ) {
            for ( inty = 0; y < 8; y++ ) {
                *( int*)&data[((y + TILE_SIZE / 2 - 4) * TILE_SIZE + x + TILE_SIZE / 2 - 4) * 4] = *( int*)color;
            }
        }
    }

    int level = 0;
    int size = TILE_SIZE;
    while ( size > 0) {
        qglTexSubImage2D(GL_TEXTURE_2D, level, localX * size, localY * size, size, size, GL_RGBA, GL_UNSIGNED_BYTE, data);
        size >>= 1;
        level++;

        int byteSize = size * 4;
        for ( inty = 0; y < size; y++ ) {
            byte *in = data + y * size * 16;
            byte *in2 = in + size * 8;
            byte *out = data + y * size * 4;
            for ( intx = 0; x < size; x++ ) {
                out[x * 4 + 0] = (in[x * 8 + 0] + in[x * 8 + 4 + 0] + in2[x * 8 + 0] + in2[x * 8 + 4 + 0]) >> 2;
                out[x * 4 + 1] = (in[x * 8 + 1] + in[x * 8 + 4 + 1] + in2[x * 8 + 1] + in2[x * 8 + 4 + 1]) >> 2;
                out[x * 4 + 2] = (in[x * 8 + 2] + in[x * 8 + 4 + 2] + in2[x * 8 + 2] + in2[x * 8 + 4 + 2]) >> 2;
                out[x * 4 + 3] = (in[x * 8 + 3] + in[x * 8 + 4 + 3] + in2[x * 8 + 3] + in2[x * 8 + 4 + 3])
*/
/*
====================
UpdateForCenter

Center is in the 0.0 to 1.0 range
====================
*/
void idTextureLevel::UpdateForCenter( float center[2] ) {
	int globalTileCorner[2];
	int localTileOffset[2];

	if ( tilesWide <= TILE_PER_LEVEL && tilesHigh <= TILE_PER_LEVEL ) {
		globalTileCorner[0] = 0;
		globalTileCorner[1] = 0;
		localTileOffset[0] = 0;
		localTileOffset[1] = 0;
		// orient the mask so that it doesn't mask anything at all
		parms[0] = 0.25;
		parms[1] = 0.25;
		parms[3] = 0.25;
	} else {
		for ( int i = 0 ; i < 2 ; i++ ) {
			float global[2];

			// this value will be outside the 0.0 to 1.0 range unless
			// we are in the corner of the megaTexture
			global[i] = ( center[i] * parms[3] - 0.5f ) * TILE_PER_LEVEL;
			globalTileCorner[i] = (int)( global[i] + 0.5f );
			localTileOffset[i] = globalTileCorner[i] & ( TILE_PER_LEVEL-1 );

			// scaling for the mask texture to only allow the proper window
			// of tiles to show through
			parms[i] = -globalTileCorner[i] / ( float )TILE_PER_LEVEL;
		}
	}

	image->Bind();

	for ( int x = 0 ; x < TILE_PER_LEVEL ; x++ ) {
		for ( int y = 0 ; y < TILE_PER_LEVEL ; y++ ) {
			int globalTile[2];

			globalTile[0] = globalTileCorner[0] + ( ( x - localTileOffset[0] ) & ( TILE_PER_LEVEL-1 ) );
			globalTile[1] = globalTileCorner[1] + ( ( y - localTileOffset[1] ) & ( TILE_PER_LEVEL-1 ) );

			UpdateTile( x, y, globalTile[0], globalTile[1] );
		}
	}
}

/*
=====================
Invalidate

Forces all tiles to be regenerated
=====================
*/
void idTextureLevel::Invalidate() {
	for ( int x = 0 ; x < TILE_PER_LEVEL ; x++ ) {
		for ( int y = 0 ; y < TILE_PER_LEVEL ; y++ ) {
			tileMap[x][y].x =
			tileMap[x][y].y = -99999;
		}
	}
}

//===================================================================================================

typedef struct _TargaHeader {
	unsigned char 	idLength, colorMapType, imageType;
	unsigned short	colorMapIndex, colormapLength;
	unsigned char	colorMapSize;
	unsigned short	xOrigin, yOrigin, width, height;
	unsigned char	pixelSize, attributes;
	unsigned char	pixSize;
} TargaHeader;


static byte ReadByte( anFile *f ) {
	byte b;
	f->Read( &b, 1 );
	return b;
}

static short ReadShort( anFile *f ) {
	byte b[2];
	f->Read( &b, 2 );
	return b[0] + ( b[1] << 8 );
}

/*
====================
GenerateMegaMipMaps
====================
*/
void idMegaTexture::GenerateMegaMipMaps( megaTextureHeader_t *header, anFile *outFile ) {
	outFile->Flush();

	// out fileSystem doesn't allow read / write access...
	anFile	*inFile = fileSystem->OpenFileRead( outFile->GetName() );

	int	tileOffset = 1;
	int	width = header->tilesWide, height = header->tilesHigh;

	int tileSize = header->tileSize * header->tileSize * 4;
	byte *oldBlock = (byte *)_alloca( tileSize );
	byte *newBlock = (byte *)_alloca( tileSize );

	while ( width > 1 || height > 1 ) {
		int newHeight = int newWidth = ( ( height + 1 ) >> 1 ), ( ( width + 1 ) >> 1 ) ;
		if ( newHeight < 1 || newWidth < 1 ) {
			newHeight = anMath::Max( newHeight, 1 );
			newWidth = anMath::Max( newWidth, 1 );
		}
		common->Printf( "generating %i x %i block mip level\n", newWidth, newHeight );

		int tileNum;

		for ( int y = 0 ; y < newHeight ; y++ ) {
			common->Printf( "row %i\n", y );
			//session->UpdateScreen();
			for ( int x = 0 ; x < newWidth ; x++ ) {
				// mip map four original blocks down into a single new block
				for ( int yy = 0 ; yy < 2 ; yy++ ) {
					for ( int xx = 0 ; xx< 2 ; xx++ ) {
						int	tx = x*2 + xx, ty = y*2 + yy;
						if ( tx > width || ty > height ) {
							// off edge, zero fill
							memset( newBlock, 0, sizeof( newBlock ) );
						} else {
							tileNum = tileOffset + ty * width + tx;
							inFile->Seek( tileNum * tileSize, FS_SEEK_SET );
							inFile->Read( oldBlock, tileSize );
						}
						// mip map the new pixels
						for ( int yyy = 0 ; yyy < TILE_SIZE / 2 ; yyy++ ) {
							for ( int xxx = 0 ; xxx < TILE_SIZE / 2 ; xxx++ ) {
								byte *in = &oldBlock[ ( yyy * 2 * TILE_SIZE + xxx * 2 ) * 4 ];
								byte *out = &newBlock[ ( ( ( TILE_SIZE/2 * yy ) + yyy ) * TILE_SIZE + ( TILE_SIZE/2 * xx ) + xxx ) * 4 ];
								out[0] = ( in[0] + in[4] + in[0+TILE_SIZE*4] + in[4+TILE_SIZE*4] ) >> 2;
								out[1] = ( in[1] + in[5] + in[1+TILE_SIZE*4] + in[5+TILE_SIZE*4] ) >> 2;
								out[2] = ( in[2] + in[6] + in[2+TILE_SIZE*4] + in[6+TILE_SIZE*4] ) >> 2;
								out[3] = ( in[3] + in[7] + in[3+TILE_SIZE*4] + in[7+TILE_SIZE*4] ) >> 2;
							}
						}

						// write the block out
						tileNum = tileOffset + width * height + y * newWidth + x;
						outFile->Seek( tileNum * tileSize, FS_SEEK_SET );
						outFile->Write( newBlock, tileSize );

					}
				}
			}
		}
		tileOffset += width * height;
		width = newWidth;
		height = newHeight;
	}

	delete inFile;
}

/*
====================
GenerateMegaPreview

Make a 2k x 2k preview image for a mega texture that can be used in modeling programs
====================
*/
void idMegaTexture::GenerateMegaPreview( const char *fileName ) {
	// open the file
	anFile	*fileHandle = fileSystem->OpenFileRead( fileName );
	if ( !fileHandle ) {
		common->Printf( "idMegaTexture: failed to open %s\n", fileName );
		return;
	}

	anString	outName = fileName;
	outName.StripFileExtension();
	outName += "_preview.tga";

	common->Printf( "Creating %s.\n", outName.c_str() );

	// read the header
	megaTextureHeader_t header;

	fileHandle->Read( &header, sizeof( header ) );
	if ( header.tileSize < 64 || header.tilesWide < 1 || header.tilesHigh < 1 ) {
		common->Printf( "idMegaTexture: bad header on %s\n", fileName );
		return;
	}

	// find the level that fits
	int	tileSize = header.tileSize;
	int	width = header.tilesWide, height = header.tilesHigh;
	int	tileOffset = 1;
	int	tileBytes = tileSize * tileSize * 4;

	// find the level that fits
	while ( width * tileSize > 2048 || height * tileSize > 2048 ) {
		tileOffset += width * height;
		if ( newWidth < 1 || newHeight < 1 ) {
			width = 1;
			width >>= 1;
			height = 1;
			 height >>= 1;
			//newHeight = anMath::Max( newHeight, 1 );
			//newWidth = anMath::Max( newWidth, 1 );
		}
	}

	byte *pic = (byte *)R_StaticAlloc( width * height * tileBytes );
	byte *oldBlock = (byte *)_alloca( tileBytes );
	for ( int y = 0 ; y < height ; y++ ) {
		for ( int x = 0 ; x < width ; x++ ) {
			int tileNum = tileOffset + y * width + x;
			fileHandle->Seek( tileNum * tileBytes, FS_SEEK_SET );
			fileHandle->Read( oldBlock, tileBytes );
			for ( int yy = 0 ; yy < tileSize ; yy++ ) {
				memcpy( pic + ( ( y * tileSize + yy ) * width * tileSize + x * tileSize  ) * 4,
					oldBlock + yy * tileSize * 4, tileSize * 4 );
			}
		}
	}

	R_WriteTGA( outName.c_str(), pic, width * tileSize, height * tileSize, false );
	R_WriteTGA( fileName, pic, width * tileSize, height * tileSize, false );
	R_StaticFree( pic );

	delete fileHandle;
}

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

void idMegaTexture::GenerateMegaPreview(const std::string &fileName) {
   std::ifstream fileHandle(fileName, std::ios::in | std::ios::binary);
   if ( !fileHandle) {
       std::cout << "idMegaTexture: failed to open " << fileName << std::endl;
       return;
   }

   std::string outName = fileName;
   outName.erase(outName.find_last_of( "." ) );
   outName += "_preview.tga";

   std::cout << "Creating " << outName << std::endl;

   // read the header
   megaTextureHeader_t header;
   fileHandle.read((char *)&header, sizeof(header));
   if (header.tileSize < 64 || header.tiles

/*
====================
MakeMegaTexture_f

Incrementally load a giant tga file and process into the mega texture block format
====================
*/
void idMegaTexture::MakeMegaTexture_f( const anCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "USAGE: makeMegaTexture <filebase>\n" );
		return;
	}

	anString name_s = "megaTexture/";
	name_s += args.Argv( 1 );
	name_s.StripFileExtension();
	name_s += ".tga";

	const char	*name = name_s.c_str();

	//
	// open the file
	//
	common->Printf( "Opening %s.\n", name );
	int fileSize = fileSystem->ReadFile( name, nullptr, nullptr );
	anFile	*file = fileSystem->OpenFileRead( name );

	if ( !file ) {
		common->Printf( "Couldn't open %s\n", name );
		return;
	}

	TargaHeader tgaHeader;
	tgaHeader.idLength = ReadByte( file );
	tgaHeader.colorMapType = ReadByte( file );
	tgaHeader.imageType = ReadByte( file );

	tgaHeader.colorMapIndex = ReadShort( file );
	tgaHeader.colormapLength = ReadShort( file );
	tgaHeader.colorMapSize = ReadByte( file );
	tgaHeader.xOrigin = ReadShort( file );
	tgaHeader.yOrigin = ReadShort( file );
	tgaHeader.width = ReadShort( file );
	tgaHeader.height = ReadShort( file );
	tgaHeader.pixelSize = ReadByte( file );
	tgaHeader.attributes = ReadByte( file );

	if ( tgaHeader.imageType != 2 && tgaHeader.imageType != 10 && tgaHeader.imageType != 3 ) {
		common->Error( "LoadTGA( %s ): Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n", name );
	}

	if ( tgaHeader.colorMapType != 0 ) {
		common->Error( "LoadTGA( %s ): colormaps not supported\n", name );
	}

	if ( ( tgaHeader.pixelSize != 32 && tgaHeader.pixelSize != 24 ) && tgaHeader.imageType != 3 ) {
		common->Error( "LoadTGA( %s ): Only 32 or 24 bit images supported (no colormaps)\n", name );
	}

	if ( tgaHeader.imageType == 2 || tgaHeader.imageType == 3 ) {
		int numBytes = tgaHeader.width * tgaHeader.height * ( tgaHeader.pixelSize >> 3 );
		if ( numBytes > fileSize - 18 - tgaHeader.idLength ) {
			common->Error( "LoadTGA( %s ): incomplete file\n", name );
		}
	}

	int columns = tgaHeader.width;
	int rows = tgaHeader.height;

	// skip TARGA image comment
	if ( tgaHeader.idLength != 0 ) {
		file->Seek( tgaHeader.idLength, FS_SEEK_CUR );
	}

	megaTextureHeader_t		mtHeader;

	mtHeader.tileSize = TILE_SIZE;
	mtHeader.tilesWide = RoundDownToPowerOfTwo( tgaHeader.width ) / TILE_SIZE;
	mtHeader.tilesHigh = RoundDownToPowerOfTwo( tgaHeader.height ) / TILE_SIZE;

	anString	outName = name;
	outName.StripFileExtension();
	outName += ".mgat";

	common->Printf( "Writing %i x %i size %i tiles to %s.\n", mtHeader.tilesWide, mtHeader.tilesHigh, mtHeader.tileSize, outName.c_str() );

	// open the output megatexture file
	anFile	*out = fileSystem->OpenFileWrite( outName.c_str() );

	out->Write( &mtHeader, sizeof( mtHeader ) );
	out->Seek( TILE_SIZE * TILE_SIZE * 4, FS_SEEK_SET );

	// we will process this one row of tiles at a time, since the entire thing
	// won't fit in memory
	byte *tgaRGBA = (byte *)R_StaticAlloc( TILE_SIZE * tgaHeader.width * 4 );

	int blockRowsRemaining = mtHeader.tilesHigh;
	while ( blockRowsRemaining-- ) {
		common->Printf( "%i blockRowsRemaining\n", blockRowsRemaining );
		//session->UpdateScreen();
		if ( tgaHeader.imageType == 2 || tgaHeader.imageType == 3 ) {
			// Uncompressed RGB or gray scale image
			for ( int row = 0 ; row < TILE_SIZE ; row++ ) {
				byte *pixBuf = tgaRGBA + row*columns*4;
				for ( int column = 0; column < columns; column++ ) {
					unsigned char red, green, blue, alphaByte;
					switch ( tgaHeader.pixelSize ) {
					case 8:
						blue = ReadByte( file );
						green = blue;
						red = blue;
						*pixBuf++ = red;
						*pixBuf++ = green;
						*pixBuf++ = blue;
						*pixBuf++ = 255;
						break;

					case 24:
						blue = ReadByte( file );
						green = ReadByte( file );
						red = ReadByte( file );
						*pixBuf++ = red;
						*pixBuf++ = green;
						*pixBuf++ = blue;
						*pixBuf++ = 255;
						break;
					case 32:
						blue = ReadByte( file );
						green = ReadByte( file );
						red = ReadByte( file );
						alphaByte = ReadByte( file );
						*pixBuf++ = red;
						*pixBuf++ = green;
						*pixBuf++ = blue;
						*pixBuf++ = alphaByte;
						break;
					default:
						common->Error( "LoadTGA( %s ): illegal pixel size '%d'\n", name, tgaHeader.pixelSize );
						break;
					}
				}
			}
		} else if ( tgaHeader.imageType == 10 ) {   // Runlength encoded RGB images
			unsigned char packetHeader, packetSize, j;

			unsigned char red = 0;
			unsigned char green = 0;
			unsigned char blue = 0;
			unsigned char alphaByte = 0xff;

			for ( int row = 0 ; row < TILE_SIZE ; row++ ) {
				byte *pixBuf = tgaRGBA + row*columns*4;
				for ( int column = 0; column < columns; ) {
					packetHeader = ReadByte( file );
					packetSize = 1 + ( packetHeader & 0x7f );
					if ( packetHeader & 0x80 ) {        // run-length packet
						switch ( tgaHeader.pixelSize ) {
							case 24:
									blue = ReadByte( file );
									green = ReadByte( file );
									red = ReadByte( file );
									alphaByte = 255;
									break;
							case 32:
									blue = ReadByte( file );
									green = ReadByte( file );
									red = ReadByte( file );
									alphaByte = ReadByte( file );
									break;
							default:
								common->Error( "LoadTGA( %s ): illegal pixelSize '%d'\n", name, tgaHeader.pixelSize );
								break;
						}

						for ( j = 0; j < packetSize; j++ ) {
							*pixBuf++=red;
							*pixBuf++=green;
							*pixBuf++=blue;
							*pixBuf++=alphaByte;
							column++;
							if ( column == columns ) { // run spans across rows
								common->Error( "TGA had RLE across columns, probably breaks block" );
								column = 0;
								if ( row > 0 ) {
									row--;
								}
								else {
									goto breakOut;
								}
								pixBuf = tgaRGBA + row*columns*4;
							}
						}
					} else {                            // non run-length packet
						for ( j = 0; j < packetSize; j++ ) {
							switch ( tgaHeader.pixelSize ) {
								case 24:
										blue = ReadByte( file );
										green = ReadByte( file );
										red = ReadByte( file );
										*pixBuf++ = red;
										*pixBuf++ = green;
										*pixBuf++ = blue;
										*pixBuf++ = 255;
										break;
								case 32:
										blue = ReadByte( file );
										green = ReadByte( file );
										red = ReadByte( file );
										alphaByte = ReadByte( file );
										*pixBuf++ = red;
										*pixBuf++ = green;
										*pixBuf++ = blue;
										*pixBuf++ = alphaByte;
										break;
								default:
									common->Error( "LoadTGA( %s ): illegal pixelSize '%d'\n", name, tgaHeader.pixelSize );
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
								pixBuf = tgaRGBA + row*columns*4;
							}
						}
					}
				}
				breakOut: ;
			}
		}

		//
		// write out individual blocks from the full row block buffer
		//
		for ( int rowBlock = 0 ; rowBlock < mtHeader.tilesWide ; rowBlock++ ) {
			for ( int y = 0 ; y < TILE_SIZE ; y++ ) {
				out->Write( tgaRGBA + ( y * tgaHeader.width + rowBlock * TILE_SIZE ) * 4, TILE_SIZE * 4 );
			}
		}
	}

	R_StaticFree( tgaRGBA );

	GenerateMegaMipMaps( &mtHeader, out );

	delete out;
	delete file;

	GenerateMegaPreview( outName.c_str() );
#if 0
	if ( ( tgaHeader.attributes & ( 1<<5 ) ) ) {			// image flp bit
		R_VerticalFlip( *pic, *width, *height );
	}
#endif
}

#if defined( _DEBUG ) && !defined( ARC_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
===============================================================================

	anHeightMap

===============================================================================
*/

/*
==============
anHeightMap::anHeightMap
==============
*/
anHeightMap::anHeightMap( void ) {
	Clear();
}

/*
==============
anHeightMap::~anHeightMap
==============
*/
anHeightMap::~anHeightMap( void ) {
	Clear();
}

/*
==============
anHeightMap::Clear
==============
*/
void anHeightMap::Clear( void ) {
	data.Clear();

	dimensions[0] = 0;
	dimensions[1] = 0;
}

/*
==============
anHeightMap::Init
==============
*/
void anHeightMap::Init( int w, int h, byte height ) {
	dimensions[0] = w;
	dimensions[1] = h;

	data.AssureSize( dimensions[0] * dimensions[1], 0 );
	for ( int i=0; i<w; i++ ) {
		for ( intj=0; j<h; j++ ) {
			data[ i * h + j ] = height;
		}
	}
}

/*
==============
anHeightMap::Load
==============
*/
void anHeightMap::Load( const char *filename ) {
	Clear();

	if ( !filename || !*filename ) {
		return;
	}

	byte *pic;
	unsigned timestamp;
	fileSystem->ReadTGA( filename, &pic, &dimensions[0], &dimensions[1], &timestamp );

	if ( dimensions[0] == 0 || dimensions[1] == 0 ) {
		return;
	}

	data.AssureSize( dimensions[0] * dimensions[1], 0 );

	for ( int x = 0; x < dimensions[0]; x++ ) {
		for ( int y = 0; y < dimensions[1]; y++ ) {
			data[ x + ( y * dimensions[0] ) ] = pic[ ( x + ( ( dimensions[1] - 1 - y ) * dimensions[0] ) ) * 4 ];
		}
	}

	fileSystem->FreeTGA( pic );
}

/*
==============
anHeightMap::GetInterpolatedHeight
==============
*/
float anHeightMap::GetInterpolatedHeight( const anVec3 &pos, const anHeightMapScaleData &scale ) const {
	// find which points to sample
	int minCoords[2];
	int maxCoords[2];
	float blendValues[2];

	for ( int i = 0; i < 2; i ++ ) {
		float posCoord = anMath::ClampFloat( 0.0f, 1.0f, ( pos[i] - scale.mins[i] ) * scale.invSize[i] ) * ( dimensions[i] - 1 );
		float intCoord = ( int )posCoord;
		float coordLeftover = posCoord - intCoord;

		minCoords[i] = intCoord;
		maxCoords[i] = anMath::ClampInt( 0, dimensions[i] - 1, intCoord + 1 );
		blendValues[i] = coordLeftover;
	}

	// sample points
	byte tl = data[ minCoords[0] + ( maxCoords[1] * dimensions[0] ) ];
	byte tr = data[ maxCoords[0] + ( maxCoords[1] * dimensions[0] ) ];
	byte bl = data[ minCoords[0] + ( minCoords[1] * dimensions[0] ) ];
	byte br = data[ maxCoords[0] + ( minCoords[1] * dimensions[0] ) ];

	float combinedTop = Lerp( tl, tr, blendValues[0] );
	float combinedBottom = Lerp( bl, br, blendValues[0] );
	float combined = Lerp( combinedBottom, combinedTop, blendValues[0] );

	return ( combined * scale.heightScale ) + scale.heightOffset;
}

/*
==============
anHeightMap::GetHeight
==============
*/
void anHeightMap::GetHeight( const anBounds &in, anVec2 &out, const anHeightMapScaleData &scale ) const {
	if ( data.Num() == 0 ) {
		common->Warning( "[HeightMap::GetHeight] - no heightmap data!" );
		out[0] = 0.0f;
		out[1] = 0.0f;
		return;
	}

	int minCoords[2];
	int maxCoords[2];

	minCoords[0] = ( anMath::ClampFloat( 0.0f, 1.0f, ( in.GetMins()[0] - scale.mins[0] ) * scale.invSize[0] ) * ( dimensions[0] - 1 ) );
	minCoords[1] = ( anMath::ClampFloat( 0.0f, 1.0f, ( in.GetMins()[1] - scale.mins[1] ) * scale.invSize[1] ) * ( dimensions[1] - 1 ) );

	maxCoords[0] = ( anMath::ClampFloat( 0.0f, 1.0f, ( in.GetMaxs()[0] - scale.mins[0] ) * scale.invSize[0] ) * ( dimensions[0] - 1 ) ) + 1;
	maxCoords[1] = ( anMath::ClampFloat( 0.0f, 1.0f, ( in.GetMaxs()[1] - scale.mins[1] ) * scale.invSize[1] ) * ( dimensions[1] - 1 ) ) + 1;

	int x;
	int y;
	float value;

	out[0] = anMath::INFINITY;
	out[1] = -anMath::INFINITY;

	for ( x = minCoords[0]; x < maxCoords[0]; x++ ) {
		for ( y = minCoords[1]; y < maxCoords[1]; y++ ) {
			value = ( data[ x + ( y * dimensions[0] ) ] * scale.heightScale ) + scale.heightOffset;
			if ( value < out[0] ) {
				out[0] = value;
			}
			if ( value > out[1] ) {
				out[1] = value;
			}
		}
	}
}

/*
==============
anHeightMap::GetHeight
==============
*/
float anHeightMap::GetHeight( const anVec3 &start, const anVec3 &end, const anHeightMapScaleData &scale ) const {
	if ( data.Num() == 0 ) {
		common->Warning( "[HeightMap::TracePoint] - no heightmap data!" );
		return 0.0f;
	}

    // find the start & end points in heightmap coordinates
    int startCoords[2];
    startCoords[0] = static_cast<int>( ( start[0] - scale.mins[0] ) * ( scale.invSize[0] * (dimensions[0] - 1 ) ) );
    startCoords[1] = static_cast<int>( ( start[1] - scale.mins[1] )  * ( scale.invSize[1] * (dimensions[1] - 1 ) ) );
    int endCoords[2];
    endCoords[0] = static_cast<int>( ( end[0] - scale.mins[0] ) * ( scale.invSize[0] * ( dimensions[0] - 1 ) ) );
    endCoords[1] = static_cast<int>( ( end[1] - scale.mins[1] ) * ( scale.invSize[1] * ( dimensions[1] - 1 ) ) );

    const int left = 0;
    const int right = static_cast<int>( dimensions[0] ) - 1;
    const int top = 0;
    const int bottom = static_cast<int>( dimensions[1] ) - 1;

	// find if the trace is entirely outside the map
   if ( ( startCoords[0] < left && endCoords[0] < left ) || ( startCoords[0] > right && endCoords[0] > right ) ||
        ( startCoords[1] < top && endCoords[1] < top ) || ( startCoords[1] > bottom && endCoords[1] > bottom ) ) {
        return 0.0f;
	    }

	int startCoordsX = static_cast<int>( startCoords[0] );
	int startCoordsY = static_cast<int>( startCoords[1] );
	startCoordsX = ClipCoord( startCoordsX, left, right );
	startCoordsY = ClipCoord( startCoordsY, top, bottom );
	startCoords = anVec2( startCoordsX, startCoordsY );

	int endCoordsX = static_cast<int>( endCoords[0] );
	int endCoordsY = static_cast<int>( endCoords[1] );
	endCoordsX = ClipCoord( endCoordsX, left, right );
	endCoordsY = ClipCoord( endCoordsY, top, bottom );
	endCoords = anVec2( endCoordsX, endCoordsY );

	// clip the start & end coords to the dimensions of the map whilst keeping the same slopes
	startCoords[0] = ( startCoords[0] < left ) ? ( startCoords[0] + static_cast<int>( ( left - startCoords[0] ) / direction[0] ) * direction[0] ) : startCoords[0];
	startCoords[0] = ( startCoords[0] > right ) ? ( startCoords[0] + static_cast<int>( ( right - startCoords[0] ) / direction[0] ) * direction[0] ) : startCoords[0];
	startCoords[1] = ( startCoords[1] < top ) ? ( startCoords[1] + static_cast<int>( ( top - startCoords[1] ) / direction[1]) * direction[1] ) : startCoords[1];
	startCoords[1] = ( startCoords[1] > bottom ) ? ( startCoords[1] + static_cast<int>( ( bottom - startCoords[1] ) / direction[1] ) * direction[1] ) : startCoords[1];

	endCoords[0] = ( endCoords[0] < left ) ? ( endCoords[0] + static_cast<int>( ( left - endCoords[0] ) / direction[0] ) * direction[0] ) : endCoords[0];
	endCoords[0] = ( endCoords[0] > right ) ? ( endCoords[0] + static_cast<int>( ( right - endCoords[0] ) / direction[0] ) * direction[0] ) : endCoords[0];
	endCoords[1] = ( endCoords[1] < top ) ? ( endCoords[1] + static_cast<int>( ( top - endCoords[1] ) / direction[1] ) * direction[1] )  : endCoords[1];
	endCoords[1] = ( endCoords[1] > bottom ) ? ( endCoords[1] + static_cast<int>( ( bottom - endCoords[1] ) / direction[1] ) * direction[1] ) : endCoords[1];

	// update the coorddelta
	coordDelta = endCoords - startCoords;

	// check if the trace is a vertical one
	if ( anMath::Fabs( coordDelta[0] ) < 1.0f && anMath::Fabs( coordDelta[1] ) < 1.0f ) {
		return GetHeight( start, scale );
	}

	// find the step to go by
	int mostSignificantAxis = 0;
	if ( anMath::Fabs( direction[1] ) > anMath::Fabs( direction[0] ) ) {
		mostSignificantAxis = 1;
	}

	float axisStep = anMath::Fabs( direction[ mostSignificantAxis ] );
	anVec2 step = direction /  axisStep;
	if ( step[ mostSignificantAxis ] > 0.0f ) {
		step[ mostSignificantAxis ] = 1.0f;		// paranoid
	} else {
		step[ mostSignificantAxis ] = -1.0f;	// paranoid
	}

	int fullDistance = static_cast<int>( endCoords[mostSignificantAxis] ) - static_cast<int>( startCoords[mostSignificantAxis] );
	fullDistance = anMath::Abs( fullDistance );
	byte maxHeight = 0;

    int intTestCoords[2];
    intTestCoords[0] = static_cast<int>( startCoords[0] );
    intTestCoords[1] = static_cast<int>( startCoords[1] );

	// TODO: Could optimize this a lot by using ints only in here
	anVec2 testCoords = startCoords;
	for ( int travelled = 0; travelled < fullDistance; travelled++ ) {
		byte height = data[intTestCoords[0] + ( intTestCoords[1] * width )];
		maxHeight = ( height > maxHeight ) ? height : maxHeight;
		testCoords += step;
		intTestCoords[0] = static_cast<int>( testCoords[0] );
		intTestCoords[1] = static_cast<int>( testCoords[1] );
	}

	return ( maxHeight * scale.heightScale ) + scale.heightOffset;
}

/*
==============
anHeightMap::TracePoint
==============
*/
float anHeightMap::TracePoint( const anVec3& start, const anVec3& end, anVec3& result, float heightOffset, const anHeightMapScaleData& scale ) const {
	result = end;
	if ( data.Num() == 0 ) {
		gameLocal.Warning( "[HeightMap::TracePoint] - no heightmap data!" );
		return 1.0f;
	}

	// find the start & end points in heightmap coordinates
	anVec3 startCoords, unclippedStartCoords;
	anVec3 endCoords, unclippedEndCoords;
	startCoords[0] = ( start[0] - scale.mins[0] ) * ( scale.invSize[0] * ( dimensions[0] - 1 ) );
	startCoords[1] = ( start[1] - scale.mins[1] ) * ( scale.invSize[1] * ( dimensions[1] - 1 ) );
	startCoords[2] = ( ( start[2] - heightOffset ) - scale.heightOffset ) / scale.heightScale;
	endCoords[0] = ( end[0] - scale.mins[0] ) * ( scale.invSize[0] * ( dimensions[0] - 1 ) );
	endCoords[1] = ( end[1] - scale.mins[1] ) * ( scale.invSize[1] * ( dimensions[1] - 1 ) );
	endCoords[2] = ( ( end[2] - heightOffset ) - scale.heightOffset ) / scale.heightScale;

	unclippedStartCoords = startCoords;
	unclippedEndCoords = endCoords;

	const float left = 0.0f;
	const float right = dimensions[0] - 1.0f;
	const float top = 0.0f;
	const float bottom = dimensions[1] - 1.0f;

	// find if the trace is entirely outside the map
	if ( ( startCoords[0] < left && endCoords[0] < left ) || ( startCoords[0] > right && endCoords[0] > right ) ) {
		return 1.0f;
	}
	if ( ( startCoords[1] < top && endCoords[1] < top ) || ( startCoords[1] > bottom && endCoords[1] > bottom ) ) {
		return 1.0f;
	}
	if ( ( startCoords[2] < 0.0f && endCoords[2] < 0.0f ) || ( startCoords[2] > 255.0f && endCoords[2] > 255.0f ) ) {
		return 1.0f;
	}

	anVec3 coordDelta = endCoords - startCoords;
	anVec3 direction = coordDelta;
	direction.Normalize();

	// clip the start & end coords to the dimensions of the map whilst keeping the same slopes
	// yes, ugly formatting, but it stops it being huuuuge
	if ( startCoords[0] < left ) startCoords += ( ( left - startCoords[0] ) / direction[0] ) * direction;
	if ( startCoords[0] > right )	startCoords += ( ( right - startCoords[0] ) / direction[0] ) * direction;
	if ( startCoords[1] < top ) startCoords += ( ( top - startCoords[1] ) / direction[1] ) * direction;
	if ( startCoords[1] > bottom ) startCoords += ( ( bottom - startCoords[1] ) / direction[1] ) * direction;
	if ( startCoords[2] < 0.0f ) startCoords += ( ( -startCoords[2] ) / direction[2] ) * direction;
	if ( startCoords[2] > 255.0f ) startCoords += ( ( 255.0f - startCoords[2] ) / direction[2] ) * direction;

	if ( endCoords[0] < left ) endCoords += ( ( left - endCoords[0] ) / direction[0] ) * direction;
	if ( endCoords[0] > right ) endCoords += ( ( right - endCoords[0] ) / direction[0] ) * direction;
	if ( endCoords[1] < top ) endCoords += ( ( top - endCoords[1] ) / direction[1] ) * direction;
	if ( endCoords[1] > bottom ) endCoords += ( ( bottom - endCoords[1] ) / direction[1] ) * direction;
	if ( endCoords[2] < 0.0f ) endCoords += ( ( -endCoords[2] ) / direction[2] ) * direction;
	if ( endCoords[2] > 255.0f ) endCoords += ( ( 255.0f - endCoords[2] ) / direction[2] ) * direction;

	// update the coorddelta
	coordDelta = endCoords - startCoords;

	// check if the trace is a vertical one
	if ( anMath::Fabs( coordDelta[0] ) < 1.0f && anMath::Fabs( coordDelta[1] ) < 1.0f ) {
		// not tracing far enough to do anything
		if ( anMath::Fabs( coordDelta[2] ) < 1.0f ) {
			return 1.0f;
		}

		// tracing up can never return a hit
		if ( endCoords[2] > startCoords[2] ) {
			return 1.0f;
		}

		int intStart[2];
		intStart[0] = startCoords[0];
		intStart[1] = startCoords[1];
		float height = data[ intStart[0] + ( intStart[1] * dimensions[0] ) ];

		if ( height < startCoords[2] ) {
			float fraction = ( unclippedStartCoords[2] - height ) / ( unclippedStartCoords[2] - unclippedEndCoords[2] );
			if ( fraction > 1.0f ) {
				fraction = 1.0f;
			}
			result = Lerp( start, end, fraction );
			return fraction;
		}
	}

	// Ok, clipped to the heightmap
	// loop along the path & find where this crosses the map
	int mostSignificantAxis = 0;
	int mostSignificantFractionAxis = 0;
	if ( anMath::Fabs( direction[1] ) > anMath::Fabs( direction[0] ) ) {
		mostSignificantAxis = 1;
		mostSignificantFractionAxis = 1;
	}
	if ( anMath::Fabs( direction[2] ) > anMath::Fabs( direction[ mostSignificantFractionAxis ] ) ) {
		mostSignificantFractionAxis = 2;
	}

	// find the step to go by
	float axisStep = anMath::Fabs( direction[ mostSignificantAxis ] );
	anVec3 step = direction /  axisStep;
	if ( step[ mostSignificantAxis ] > 0.0f ) {
		step[ mostSignificantAxis ] = 1.0f;		// paranoid
	} else {
		step[ mostSignificantAxis ] = -1.0f;	// paranoid
	}

	bool first = true;
	bool wasAbove;
	int travelled = 0;
	int fullDistance = ( int )endCoords[ mostSignificantAxis ] - ( int )startCoords[ mostSignificantAxis ];
	fullDistance = anMath::Abs( fullDistance );

	// TODO: Could optimize this a lot by using ints only in here
	anVec3 testCoords = startCoords;
	for ( int travelled = 0; travelled < fullDistance; travelled++ ) {
		int intTestCoords[ 3 ];
		intTestCoords[0] = testCoords[0];
		intTestCoords[1] = testCoords[1];

		float height = data[ intTestCoords[0] + ( intTestCoords[1] * dimensions[0] ) ];
		intTestCoords[2] = height;
		bool above = testCoords[2] >= height;
		if ( first ) {
			wasAbove = above;
			first = false;
		} else {
			if ( !above && wasAbove ) {
				// it crossed through the heightmap
				float fraction = intTestCoords[ mostSignificantFractionAxis ] - unclippedStartCoords[ mostSignificantFractionAxis ];
				fraction /= unclippedEndCoords[ mostSignificantFractionAxis ] - unclippedStartCoords[ mostSignificantFractionAxis ];
				result = Lerp( start, end, fraction );

				return fraction;
			}
		}

		testCoords += step;
	}

	return 1.0f;
}

/*
===============================================================================

	sdDeclHeightMap

===============================================================================
*/

/*
================
sdDeclHeightMap::sdDeclHeightMap
================
*/
sdDeclHeightMap::sdDeclHeightMap( void ) {
	FreeData();
}

/*
================
sdDeclHeightMap::~sdDeclHeightMap
================
*/
sdDeclHeightMap::~sdDeclHeightMap( void ) {
	FreeData();
}

/*
================
sdDeclHeightMap::DefaultDefinition
================
*/
const char* sdDeclHeightMap::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclHeightMap::Parse
================
*/
bool sdDeclHeightMap::Parse( const char *text, const int textLength ) {
	anToken token;
	anParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	anDict temp;
	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			return false;
		}
		if ( !token.Icmp( "data" ) )
			if ( !temp.Parse( src ) ) {
				return false;
			}
			game->CacheDictionaryMedia( temp );
			data.Copy( temp );
		if ( !token.Icmp( "megatexturehm" ) ) {
			if ( !src.ReadToken( &token ) ) {
				src.Error( "DeclHeightMap::ParseLevel Missing Parm for 'heightmap'" );
				return false;
			}
			heightMap.Load( token );
		} else if ( !token.Cmp( "}" ) ) {
			break;
		} else {
			src.Error( "DeclHeightMap::Parse Invalid Token '%s' in heightmap def '%s'", token.c_str(), base->GetName() );
			return false;
		}
	}
	heightMap					= data.GetString( "megatexturehm" );
	location					= data.GetVec2( "location", anVec2( SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f ).ToString() );
	serverShot					= data.GetString( "mtr_serverShot", "levelshots/generic" );

	return true;
}

/*
================
sdDeclHeightMap::FreeData
================
*/
void sdDeclHeightMap::FreeData( void ) {
	heightMap.Clear();
	megatextureMaterials.Clear();
	data.Clear();
	location.Set( SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f );
	mapShot = "maploadicon/generic";
}

/*
================
sdDeclHeightMap::CacheFromDict
================
*/
void sdDeclHeightMap::CacheFromDict( const anDict& dict ) {
	const anKeyValue *kv;
	kv = nullptr;
	while( kv = dict.MatchPrefix( "hm_", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declHeightMapType[ kv->GetValue() ];
		}
	}
}

/*
===============================================================================

	HeightMapInstance

===============================================================================
*/

/*
================
sdHeightMapInstance::Init
================
*/
void anHeightMapInstance::Init( const char *declName, const anBounds &bounds ) {
	const anHeightMap *declHeightMap = declHeightMapType[ declName ];
	if ( heightMap == nullptr ) {
		arcLib::Error( "HeightMapInstance::Init Invalid Heightmap '%s'", declName );
	}

	heightMap = &declHeightMap->GetHeightMap();
	heightMapData.Init( bounds );
}

void anHeightMapInstance::Init( const sdHeightMap *map, const anBounds& bounds ) {
	heightMap = map;
	heightMapData.Init( bounds );
}