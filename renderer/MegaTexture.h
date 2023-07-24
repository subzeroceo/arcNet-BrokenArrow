class idTextureTile {
public:
	int		x, y;
};

static const int TILE_PER_LEVEL = 4;
static const int MAX_MEGA_CHANNELS = 3;		// normal, diffuse, specular
static const int MAX_LEVELS = 12;
static const int MAX_LEVEL_WIDTH = 512;
static const int TILE_SIZE = MAX_LEVEL_WIDTH / TILE_PER_LEVEL;

class	idMegaTexture;

class idTextureLevel {
public:
	idMegaTexture	*mega;

	int				tileOffset;
	int				tilesWide;
	int				tilesHigh;

	arcImage			*image;
	idTextureTile	tileMap[TILE_PER_LEVEL][TILE_PER_LEVEL];

	float			parms[4];

	void			UpdateForCenter( float center[2] );
	void			UpdateTile( int localX, int localY, int globalX, int globalY );
	void			Invalidate();
};

typedef struct {
	int		tileSize;
	int		tilesWide;
	int		tilesHigh;
} megaTextureHeader_t;

class idMegaTexture {
public:
	bool	InitFromMegaFile( const char *fileBase );
	void	SetMappingForSurface( const srfTriangles_t *tri );	// analyzes xyz and st to create a mapping
	void	BindForViewOrigin( const arcVec3 origin );	// binds images and sets program parameters
	void	Unbind();								// removes texture bindings

	static	void MakeMegaTexture_f( const arcCommandArgs &args );
private:
friend class idTextureLevel;
	void	SetViewOrigin( const arcVec3 origin );
	static void	GenerateMegaMipMaps( megaTextureHeader_t *header, arcNetFile *file );
	static void	GenerateMegaPreview( const char *fileName );

	arcNetFile			*fileHandle;

	const srfTriangles_t *currentTriMapping;

	arcVec3			currentViewOrigin;

	float			localViewToTextureCenter[2][4];

	int				numLevels;
	idTextureLevel	levels[MAX_LEVELS];				// 0 is the highest resolution
	megaTextureHeader_t	header;

	static idCVar	r_megaTextureLevel;
	static idCVar	r_showMegaTexture;
	static idCVar	r_showMegaTextureLabels;
	static idCVar	r_skipMegaTexture;
	static idCVar	r_terrainScale;
};


class arcHeightMapScaleData {
public:
	void Init( const arcBounds &bounds ) {
		mins = bounds.GetMins();
		size = bounds.Size();

		for ( int i = 0; i < 3; i++ ) {
			invSize[i] = 1 / size[i];
		}

		heightScale		= size[2] / 256.0f;
		heightOffset	= mins[2];
	}

	arcVec3							mins;
	arcVec3							size;
	arcVec3							invSize;
	float							heightScale;
	float							heightOffset;
};

class arcHeightMap {
public:
									arcHeightMap( void );
									~arcHeightMap( void );

	void							Clear( void );
	void							Init( int w, int h, byte height );

	void							Load( const char *filename );

	float GetHeight( const arcVec3 &pos, const arcHeightMapScaleData &scale ) const {
		int coords[ 2 ];
		coords[ 0 ] = arcMath::ClampFloat( 0.f, 1.f, ( pos[ 0 ] - scale.mins[ 0 ] ) * scale.invSize[ 0 ] ) * ( dimensions[ 0 ] - 1 );
		coords[ 1 ] = arcMath::ClampFloat( 0.f, 1.f, ( pos[ 1 ] - scale.mins[ 1 ] ) * scale.invSize[ 1 ] ) * ( dimensions[ 1 ] - 1 );
		return ( data[ coords[ 0 ] + ( coords[ 1 ] * dimensions[ 0 ] ) ] * scale.heightScale ) + scale.heightOffset;
	}

	float							GetInterpolatedHeight( const arcVec3 &pos, const arcHeightMapScaleData &scale ) const;
	void							GetHeight( const arcBounds &pos, arcVec2 &out, const arcHeightMapScaleData &scale ) const;
	float							GetHeight( const arcVec3 &start, const arcVec3 &end, const arcHeightMapScaleData &scale ) const;
	//void							ClipCoords( arcVec3 &start, arcVec3 &end ) const;

	// this does a kind-of-trace through the heightmap for really rough approximation work
	// where you don't want to do it in the full physics world
	// heightOffset acts as if you're tracing through an offset world
	float							TracePoint( const arcVec3 &start, const arcVec3 &end, arcVec3 &result, float heightOffset, const arcHeightMapScaleData &scale ) const;

	bool							IsValid( void ) const { return !data.Empty(); }

private:
	arcNetList<byte>					data;
	int								dimensions[ 2 ];
};
