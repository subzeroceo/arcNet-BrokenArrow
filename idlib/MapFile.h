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

#include "Str.h"
const int OLD_MAP_VERSION					= 1;
const int CURRENT_MAP_VERSION				= 2;
const int DEFAULT_CURVE_SUBDIVISION			= 4;
const float DEFAULT_CURVE_MAX_ERROR			= 4.0f;
const float DEFAULT_CURVE_MAX_ERROR_CD		= 24.0f;
const float DEFAULT_CURVE_MAX_LENGTH		= -1.0f;
const float DEFAULT_CURVE_MAX_LENGTH_CD		= -1.0f;
const int BOT_MAP_VERSION					= 2; // bumped - actions now use idBox instead of anBounds.

class anMapPrimitive {
public: enum {
		TYPE_INVALID = -1,
		TYPE_BRUSH,
		TYPE_PATCH
	};

	anDict					epairs;

							anMapPrimitive( void ) { type = TYPE_INVALID; }
	virtual					~anMapPrimitive( void ) { }
	int						GetType( void ) const { return type; }

protected:
	int						type;
};

class anMapBrushSides {
	friend class anMapBrush;

public:
							anMapBrushSides( void );
							~anMapBrushSides( void ) { }
	const char *			GetMaterial( void ) const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	const anPlane &			GetPlane( void ) const { return plane; }
	void					SetPlane( const anPlane &p ) { plane = p; }
	void					SetTextureMatrix( const anVec3 mat[2] ) { texMat[0] = mat[0]; texMat[1] = mat[1]; }
	void					GetTextureMatrix( anVec3 &mat1, anVec3 &mat2 ) { mat1 = texMat[0]; mat2 = texMat[1]; }
	void					GetTextureVectors( anVec4 v[2] ) const;
	void					TranslateSelf( const anVec3 &translation );
protected:
	anStr					material;
	anPlane					plane;
	anVec3					texMat[2];
	anVec3					origin;
};

inline anMapBrushSides::anMapBrushSides( void ) {
	plane.Zero();
	texMat[0].Zero();
	texMat[1].Zero();
	origin.Zero();
}

inline void anMapBrushSides::TranslateSelf( const anVec3 &translation ) {
	origin += translation;
}

class anMapBrush : public anMapPrimitive {
public:
							anMapBrush( void ) { type = TYPE_BRUSH; sides.Resize( 8, 4 ); }
							~anMapBrush( void ) { sides.DeleteContents( true ); }
	static anMapBrush *		Parse( anLexer &src, const anVec3 &origin, bool newFormat = true, float version = CURRENT_MAP_VERSION );
	static anMapBrush *		ParseQ3( anLexer &src, const anVec3 &origin );
	bool					Write( anStr &buffer, int primitiveNum, const anVec3 &origin ) const;
	bool					Write( anFile *fp, int primitiveNum, const anVec3 &origin ) const;
	int						GetNumSides( void ) const { return sides.Num(); }
	int						AddSide( anMapBrushSides *side ) { return sides.Append( side ); }
	anMapBrushSides *		GetSide( int i ) const { return sides[i]; }
	unsigned int			GetGeometryCRC( void ) const;

protected:
	int						numSides;
	anList<anMapBrushSides *> sides;
};

class anMapPatch : public anMapPrimitive, public anSurface_Patch {
public:
							anMapPatch( void );
							anMapPatch( int maxPatchWidth, int maxPatchHeight );
							~anMapPatch( void ) { }
	static anMapPatch *	Parse( anLexer &src, const anVec3 &origin, bool patchDef3 = true, float version = CURRENT_MAP_VERSION );
	bool					Write( anStr &buffer, int primitiveNum, const anVec3 &origin ) const;
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
	anStr					material;
	int						horzSubdivisions;
	int						vertSubdivisions;
	bool					explicitSubdivisions;
};

inline anMapPatch::anMapPatch( void ) {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxHeight = 0;
	expanded = false;
}

inline anMapPatch::anMapPatch( int maxPatchWidth, int maxPatchHeight ) {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}

class anMapEntity {
	friend class				anMapFile;
public:
	anDict						epairs;
public:
								anMapEntity( void ) { epairs.SetHashSize( 64 ); }
								~anMapEntity( void ) { primitives.DeleteContents( true ); }

	static anMapEntity *		Parse( anLexer &src, bool worldSpawn = false, float version = CURRENT_MAP_VERSION );
	static anapEntity *			ParseActions( anLexer &src );

	bool						Write( anFile *fp, int entityNum ) const;
	int							GetNumPrimitives( void ) const { return primitives.Num(); }
	anMapPrimitive *			GetPrimitive( int i ) const { return primitives[i]; }
	void						AddPrimitive( anMapPrimitive *p ) { primitives.Append( p ); }
	unsigned int				GetGeometryCRC( void ) const;
	void						RemovePrimitiveData();

protected:
	anList<anMapPrimitive *>	primitives;
	anDeclEntityDef *			entityDef;
	int 						entityDefLine;
	anStr 						refId;		// reference map id, "" by default
	//anMapEntityEditorData *	mapEntityEditorData;
private:
	void					SetGeometryCRC( void );
};

class anMapFile {
public:
							anMapFile( void );
							~anMapFile( void ) { entities.DeleteContents( true ); }

	bool					ParseBuffer( const anStr &buffer, const anStr &name, bool moveFuncGroups = true );
	bool					WriteBuffer( anStr &buffer );

							// filename does not require an extension
							// normally this will use a .reg file instead of a .map file if it exists,
							// which is what the game and dmap want, but the editor will want to always
							// load a .map file
	bool					Parse( const char *filename, bool ignoreRegion = false, bool osPath = false );

	bool					ParseBotEntities( const char *filename );

	bool					Write( const char *fileName, const char *ext, bool fromBasePath = true );

							// get the number of entities in the map
	int						GetNumEntities( void ) const { return entities.Num(); }

							// get the specified entity
	anMapEntity *			GetEntity( int i ) const { return entities[i]; }

							// get the name without file extension
	const char *			GetName( void ) const { return name; }

							// get the file time
	ARC_TIME_T				GetFileTime( void ) const { return fileTime; }

							// get CRC for the map geometry
							// texture coordinates and entity key/value pairs are not taken into account
	unsigned int			GetGeometryCRC( void ) const { return geometryCRC; }

							// returns true if the file on disk changed
	bool					NeedsReload();

	int						AddEntity( anMapEntity *mapEntity );
	anMapEntity *			FindEntity( const char *name );
	void					RemoveEntity( anMapEntity *mapEnt );
	void					RemoveEntities( const char *classname );
	void					RemoveAllEntities();
	void					RemovePrimitiveData();
	bool					HasPrimitiveData() { return hasPrimitiveData; }
	float					GetVersion() {return version;}

protected:
	anStr 					name;
	float					version;
	ARC_TIME_T				fileTime;
	//unsigned int 			fileTime
	unsigned int			geometryCRC;
	anList<anMapEntity *>	entities;
	bool					hasPrimitiveData;
	anStr					mapModelFolder;
	//anList <anPair <anStr, anStr>, TAG_IDLIST, false> properties;
private:
	void					SetGeometryCRC( void );
};

inline anMapFile::anMapFile( void ) {
	version = CURRENT_MAP_VERSION;
	fileTime = 0;
	geometryCRC = 0;
	entities.Resize( 1024, 256 );
	hasPrimitiveData = false;
	//mapModelFolder = "maps/";
}

#endif
