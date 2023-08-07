

static const int TILE_PER_LEVEL = 4;
static const int MAX_MEGA_CHANNELS = 3;		// normal, diffuse, specular
static const int MAX_LEVELS = 12;
static const int MAX_LEVEL_WIDTH = 512;
static const int TILE_SIZE = MAX_LEVEL_WIDTH / TILE_PER_LEVEL;

class idMegaTexture;

typedef struct {
	int						tileSize;
	int						tilesWide;
	int						tilesHigh;
} megaTextureHeader_t;

typedef struct {
	void *					fileHandle;
	void *					currentTriMapping;
	float					origin[3];
	float					axis[4][2];
	int						level;
	textureLevel_t			levels[MAX_LEVELS];
	megaTextureHeader_t		header;
} idMegaTexture;

class idTextureTile {
public:
							~idTextureTile( void ) { x = nullptr; y = nullptr; }
	int						GetTexTilingCoordX() { return x; }
	int						GetTexTilingCoordY() { return y; }
	int						x, y;
};

class idMegaTexture {
public:

							idMegaTexture( void );
							~idMegaTexture( void );

	void					Clear( void );
	void					Init( int w, int h, byte height );

	void					Load( const char *filename );

	bool					InitFromMegaFile( const char *fileBase );
	void					SetMappingForSurface( const srfTriangles_t *tri );	// analyzes xyz and st to create a mapping
	void					BindForViewOrigin( const anVec3 origin );	// binds images and sets program parameters
	void					Unbind();								// removes texture bindings

	static void				MakeMegaTexture_f( const anCommandArgs &args );
private:
	void					SetViewOrigin( const anVec3 origin );
	static void				GenerateMegaMipMaps( megaTextureHeader_t *header, anFile *file );
	static void				GenerateMegaPreview( const char *fileName );

	void					UpdateForCenter( float center[2] );
	void					UpdateTile( int localX, int localY, int globalX, int globalY );
	void					Invalidate();
	// this does a kind-of-trace through the heightmap for really rough approximation work
	// where you don't want to do it in the full physics world
	// heightOffset acts as if you're tracing through an offset world
	float					TracePoint( const anVec3 &start, const anVec3 &end, anVec3 &result, float heightOffset, const anHeightMapScaleData &scale ) const;
	bool					IsValid( void ) const { return !data.Empty(); }
	float					GetInterpolatedHeight( const anVec3 &pos, const anHeightMapScaleData &scale ) const;
	void					GetHeight( const anBounds &pos, anVec2 &out, const anHeightMapScaleData &scale ) const;
	float					GetHeight( const anVec3 &start, const anVec3 &end, const anHeightMapScaleData &scale ) const;
	//void					ClipCoords( anVec3 &start, anVec3 &end ) const;

	anFile *				fileHandle;

	const srfTriangles_t *	currentTriMapping;

	anVec3					currentViewOrigin;

	float					localViewToTextureCenter[2][4];

	int						numLevels;
	idMegaTexture			levels[MAX_LEVELS];				// 0 is the highest resolution
	megaTextureHeader_t		header;
	idMegaTexture *			mega;

	int						tileOffset;
	int						tilesWide;
	int						tilesHigh;

	anImage *				image;
	idTextureTile			tileMap[TILE_PER_LEVEL][TILE_PER_LEVEL];

	float					parms[4];

	anVec3					mins;
	anVec3					size;
	anVec3					invSize;
	float					heightScale;
	float					heightOffset;
	anList<byte>			data;
	int						dimensions[2];

	static anCVar			r_megaTextureLevel;
	static anCVar			r_showMegaTexture;
	static anCVar			r_showMegaTextureLabels;
	static anCVar			r_skipMegaTexture;
	static anCVar			r_terrainScale;
}; extern megaTexture;

#define ID_MEGATEXTURE_SIZE( sizeof(void *) + sizeof(void *) + 3 * sizeof( float ) + \ 8 * sizeof( float ) + sizeof( int ) + \ MAX_LEVELS * sizeof( textureLevel_t ) + \ MEGATEXTUREHEADER_BYTES )
#define MEGATEXTUREHEADER_BYTES ( 3 * sizeof( int )  )

void ReadMegaTextureHeader( megaTextureHeader_t *header, unsigned char *buffer ) {
	header->tileSize = *(int *)( buffer );
	header->tilesWide = *(int *)( buffer + sizeof( int ) );
	header->tilesHigh = *(int *)( buffer + 2*sizeof( int ) );
}

void WriteMegaTextureHeader( megaTextureHeader_t *header, unsigned char *buffer) {
	*(int*)( buffer ) = header->tileSize;
	*(int*)( buffer + sizeof ( int) ) = header->tilesWide;
	*(int*)( b uffer + 2 * sizeof( int ) ) = header->tilesHigh;
}

inline void Init( const anBounds &bounds ) {
	mins = bounds.GetMins();
	size = bounds.Size();

	for ( int i = 0; i < 3; i++ ) {
		invSize[i] = 1 / size[i];
	}

	heightScale = size[2] / 256.0f;
	heightOffset = mins[2];
}

inline float GetHeight( const anVec3 &pos, const anHeightMapScaleData &scale ) const {
	int coords[2];
	coords[0] = anMath::ClampFloat( 0.0f, 1.0f, ( pos[0] - scale.mins[0] ) * scale.invSize[0] ) * ( dimensions[0] - 1 );
	coords[1] = anMath::ClampFloat( 0.0f, 1.0f, ( pos[1] - scale.mins[1] ) * scale.invSize[1] ) * ( dimensions[1] - 1 );
	return ( data[ coords[0] + ( coords[1] * dimensions[0] ) ] * scale.heightScale ) + scale.heightOffset;
}
