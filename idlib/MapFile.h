#ifndef __MAPFILE_H__
#define __MAPFILE_H__

/*
===============================================================================

	Reads or writes the contents of .map files into a standard internal
	format, which can then be moved into private formats for collision
	detection, map processing, or editor use.

	No validation (duplicate planes, null area brushes, etc) is performed.
	There are no limits to the number of any of the elements in maps.
	The order of entities, brushes, and sides is maintained.

===============================================================================
*/

const int OLD_MAP_VERSION					= 1;
const int CURRENT_MAP_VERSION				= 2;
const int DEFAULT_CURVE_SUBDIVISION			= 4;
const float DEFAULT_CURVE_MAX_ERROR			= 4.0f;
const float DEFAULT_CURVE_MAX_ERROR_CD		= 24.0f;
const float DEFAULT_CURVE_MAX_LENGTH		= -1.0f;
const float DEFAULT_CURVE_MAX_LENGTH_CD		= -1.0f;


class TectMapPrim {
public: enum {
		TYPE_INVALID = -1,
		TYPE_BRUSH,
		TYPE_PATCH
	};

	arcDictionary			epairs;

							TectMapPrim( void ) { type = TYPE_INVALID; }
	virtual					~TectMapPrim( void ) { }
	int						GetType( void ) const { return type; }

protected:
	int						type;
};


class aRcMapBrushSides {
	friend class TectMapBrush;

public:
							aRcMapBrushSides( void );
							~aRcMapBrushSides( void ) { }
	const char *			GetMaterial( void ) const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	const anPlane &		GetPlane( void ) const { return plane; }
	void					SetPlane( const anPlane &p ) { plane = p; }
	void					SetTextureMatrix( const anVec3 mat[2] ) { texMat[0] = mat[0]; texMat[1] = mat[1]; }
	void					GetTextureMatrix( anVec3 &mat1, anVec3 &mat2 ) { mat1 = texMat[0]; mat2 = texMat[1]; }
	void					GetTextureVectors( anVec4 v[2] ) const;

protected:
	anString					material;
	anPlane				plane;
	anVec3				texMat[2];
	anVec3				origin;
};

ARC_INLINE aRcMapBrushSides::aRcMapBrushSides( void ) {
	plane.Zero();
	texMat[0].Zero();
	texMat[1].Zero();
	origin.Zero();
}


class TectMapBrush : public TectMapPrim {
public:
							TectMapBrush( void ) { type = TYPE_BRUSH; sides.Resize( 8, 4 ); }
							~TectMapBrush( void ) { sides.DeleteContents( true ); }
	static TectMapBrush *	Parse( anLexer &src, const anVec3 &origin, bool newFormat = true, float version = CURRENT_MAP_VERSION );
	static TectMapBrush *	ParseQ3( anLexer &src, const anVec3 &origin );
	bool					Write( anFile *fp, int primitiveNum, const anVec3 &origin ) const;
	int						GetNumSides( void ) const { return sides.Num(); }
	int						AddSide( aRcMapBrushSides *side ) { return sides.Append( side ); }
	aRcMapBrushSides *		GetSide( int i ) const { return sides[i]; }
	unsigned int			GetGeometryCRC( void ) const;

protected:
	int						numSides;
	anList<aRcMapBrushSides*> sides;
};


class aRcMapPatch : public TectMapPrim, public arcSurface_Patch {
public:
							aRcMapPatch( void );
							aRcMapPatch( int maxPatchWidth, int maxPatchHeight );
							~aRcMapPatch( void ) { }
	static aRcMapPatch *	Parse( anLexer &src, const anVec3 &origin, bool patchDef3 = true, float version = CURRENT_MAP_VERSION );
	bool					Write( anFile *fp, int primitiveNum, const anVec3 &origin ) const;
	const char *			GetMaterial( void ) const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	int						GetHorzSubdivisions( void ) const { return horzSubdivisions; }
	int						GetVertSubdivisions( void ) const { return vertSubdivisions; }
	bool					GetExplicitlySubdivided( void ) const { return explicitSubdivisions; }
	void					SetHorzSubdivisions( int n ) { horzSubdivisions = n; }
	void					SetVertSubdivisions( int n ) { vertSubdivisions = n; }
	void					SetExplicitlySubdivided( bool b ) { explicitSubdivisions = b; }
	unsigned int			GetGeometryCRC( void ) const;

protected:
	anString					material;
	int						horzSubdivisions;
	int						vertSubdivisions;
	bool					explicitSubdivisions;
};

ARC_INLINE aRcMapPatch::aRcMapPatch( void ) {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxHeight = 0;
	expanded = false;
}

ARC_INLINE aRcMapPatch::aRcMapPatch( int maxPatchWidth, int maxPatchHeight ) {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}


class aRcMapEnt {
	friend class			aRcMapFile;

public:
	arcDictionary			epairs;

public:
							aRcMapEnt( void ) { epairs.SetHashSize( 64 ); }
							~aRcMapEnt( void ) { primitives.DeleteContents( true ); }
	static aRcMapEnt *		Parse( anLexer &src, bool worldSpawn = false, float version = CURRENT_MAP_VERSION );
	bool					Write( anFile *fp, int entityNum ) const;
	int						GetNumPrimitives( void ) const { return primitives.Num(); }
	TectMapPrim *			GetPrimitive( int i ) const { return primitives[i]; }
	void					AddPrimitive( TectMapPrim *p ) { primitives.Append( p ); }
	unsigned int			GetGeometryCRC( void ) const;
	void					RemovePrimitiveData();

protected:
	anList<TectMapPrim*>	primitives;
};


class aRcMapFile {
public:
							aRcMapFile( void );
							~aRcMapFile( void ) { entities.DeleteContents( true ); }

							// filename does not require an extension
							// normally this will use a .reg file instead of a .map file if it exists,
							// which is what the game and dmap want, but the editor will want to always
							// load a .map file
	bool					Parse( const char *filename, bool ignoreRegion = false, bool osPath = false );
	bool					Write( const char *fileName, const char *ext, bool fromBasePath = true );
							// get the number of entities in the map
	int						GetNumEntities( void ) const { return entities.Num(); }
							// get the specified entity
	aRcMapEnt *			GetEntity( int i ) const { return entities[i]; }
							// get the name without file extension
	const char *			GetName( void ) const { return name; }
							// get the file time
	ARC_TIME_T					GetFileTime( void ) const { return fileTime; }
							// get CRC for the map geometry
							// texture coordinates and entity key/value pairs are not taken into account
	unsigned int			GetGeometryCRC( void ) const { return geometryCRC; }
							// returns true if the file on disk changed
	bool					NeedsReload();

	int						AddEntity( aRcMapEnt *mapEntity );
	aRcMapEnt *			FindEntity( const char *name );
	void					RemoveEntity( aRcMapEnt *mapEnt );
	void					RemoveEntities( const char *classname );
	void					RemoveAllEntities();
	void					RemovePrimitiveData();
	bool					HasPrimitiveData() { return hasPrimitiveData; }

protected:
	float					version;
	ARC_TIME_T				fileTime;
	unsigned int			geometryCRC;
	anList<aRcMapEnt *>	entities;
	anString					name;
	bool					hasPrimitiveData;

private:
	void					SetGeometryCRC( void );
};

ARC_INLINE aRcMapFile::aRcMapFile( void ) {
	version = CURRENT_MAP_VERSION;
	fileTime = 0;
	geometryCRC = 0;
	entities.Resize( 1024, 256 );
	hasPrimitiveData = false;
}

#endif
