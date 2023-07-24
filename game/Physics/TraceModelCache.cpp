// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "TraceModelCache.h"

#define TRM_HEADER	"TRM"
#define TRM_VERSION	"1.2"

/*
===============
arcTraceModelCache::ClearTraceModelCache
===============
*/
void arcTraceModelCache::ClearTraceModelCache( void ) {
	for ( int i = 0; i < cache.Num(); i++ ) {
		collisionModelManager->FreeModel( cache[ i ]->collisionModel );
		cache[ i ]->collisionModel = NULL;
	}
	cache.Clear();
	fileCache.Clear();
	allocator.Shutdown();
	hash.Free();
	nameHash.Free();
}

/*
===============
arcTraceModelCache::TraceModelCacheSize
===============
*/
size_t arcTraceModelCache::TraceModelCacheSize( void ) {
	return cache.Size();
}

/*
===============
arcTraceModelCache::FindTraceModel
===============
*/
int arcTraceModelCache::FindTraceModel( const arcTraceModel& trm, bool includeBrushes ) {
	int hashKey = GetTraceModelHashKey( trm );
	for ( int i = hash.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = hash.GetNext( i ) ) {
		if ( cache[ i ]->trm != trm ) {
			continue;
		}

		if ( cache[ i ]->includesBrushes != includeBrushes ) {
			continue;
		}

		return i;
	}
	return -1;
}

/*
===============
arcTraceModelCache::PrecacheTraceModel
===============
*/
int arcTraceModelCache::PrecacheTraceModel( const arcTraceModel &trm, const char* fileName ) {
	int index = AllocTraceModel( trm, false );
	cache[index]->refCount--;
	if ( fileName != NULL ) {
		AllocFileEntry( fileName, index );
	}
	return index;
}

/*
===============
arcTraceModelCache::AllocTraceModel
===============
*/
int arcTraceModelCache::AllocTraceModel( const arcTraceModel &trm, bool includeBrushes ) {
	int i, hashKey, traceModelIndex;

	hashKey = GetTraceModelHashKey( trm );
	for ( i = hash.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = hash.GetNext( i ) ) {
		if ( cache[ i ]->trm == trm ) {
			cache[ i ]->refCount++;
			return i;
		}
	}

	traceModelIndex = cache.Num();
	hash.Add( hashKey, traceModelIndex );

	trmCache_t& entry = *( cache.Alloc() = allocator.Alloc() );
	entry.trm = trm;
	entry.trm.ClearUnused();
	entry.trm.GetMassProperties( 1.0f, entry.volume, entry.centerOfMass, entry.inertiaTensor );
	entry.refCount = 1;
	entry.collisionModel = collisionModelManager->ModelFromTrm( gameLocal.GetMapName(), va( "traceModel%d", traceModelIndex ), trm, includeBrushes );
	entry.hasWater = false;
	entry.includesBrushes = includeBrushes;
	SetupWaterPoints( entry );

	return traceModelIndex;
}

/*
===============
arcTraceModelCache::FreeTraceModel
===============
*/
void arcTraceModelCache::FreeTraceModel( const int traceModelIndex ) {
	if ( traceModelIndex < 0 || traceModelIndex >= cache.Num() || cache[ traceModelIndex ]->refCount <= 0 ) {
		gameLocal.Warning( "arcClipModel::FreeTraceModel: tried to free uncached trace model" );
		return;
	}
	cache[ traceModelIndex ]->refCount--;
}

/*
===============
arcTraceModelCache::CopyTraceModel
===============
*/
int arcTraceModelCache::CopyTraceModel( const int traceModelIndex ) {
	if ( traceModelIndex < 0 || traceModelIndex >= cache.Num() || cache[ traceModelIndex ]->refCount <= 0 ) {
		gameLocal.Warning( "arcTraceModelCache::CopyTraceModel: tried to copy an uncached trace model" );
		return -1;
	}
	cache[ traceModelIndex ]->refCount++;
	return traceModelIndex;
}

/*
===============
arcTraceModelCache::GetMassProperties
===============
*/
void arcTraceModelCache::GetMassProperties( const int traceModelIndex, const float density, float &mass, arcVec3 &centerOfMass, arcMat3 &inertiaTensor ) const {
	if ( traceModelIndex < 0 || traceModelIndex >= cache.Num() || cache[ traceModelIndex ]->refCount <= 0 ) {
		assert( 0 );
		gameLocal.Warning( "arcTraceModelCache::GetMassProperties: tried to use an uncached trace model" );
		inertiaTensor.Identity();
		centerOfMass.Zero();
		mass = 1.0f;
		return;
	}

	const trmCache_t* entry = cache[ traceModelIndex ];

	mass = entry->volume * density;
	centerOfMass = entry->centerOfMass;
	inertiaTensor = density * entry->inertiaTensor;
}

/*
===============
arcTraceModelCache::GetTraceModelHashKey
===============
*/
int arcTraceModelCache::GetTraceModelHashKey( const arcTraceModel &trm ) {
	const arcVec3 &v = trm.bounds[0];
	return ( trm.type << 8 ) ^ ( trm.numVerts << 4 ) ^ ( trm.numEdges << 2 ) ^ ( trm.numPolys << 0 ) ^ arcMath::FloatHash( v.ToFloatPtr(), v.GetDimension() );
}


/*
============
NewPolyPoint
============
*/
arcTraceModelCache::polyPoint_t	arcTraceModelCache::polyPointPool[ MAX_TRACEMODEL_WATER_POINTS_POOL ];
arcTraceModelCache::polyPoint_t*	arcTraceModelCache::freePolyPoints[ MAX_TRACEMODEL_WATER_POINTS_POOL ];
int								arcTraceModelCache::numFreePolyPoints = 0;
bool							arcTraceModelCache::polyPointPoolValid = false;

arcTraceModelCache::polyPoint_t* arcTraceModelCache::NewPolyPoint( void ) {
/*	polyPoint_t* point = new polyPoint_t;
	numPolyPoints++;
	if ( numPolyPoints > maxNumPolyPoints ) {
		maxNumPolyPoints = numPolyPoints;
		gameLocal.Printf( "***************** maxNumPolyPoints = %i\n", maxNumPolyPoints );
	}*/

	if ( !polyPointPoolValid ) {
		for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS_POOL; i++ ) {
			freePolyPoints[ i ] = &polyPointPool[ i ];
		}
		polyPointPoolValid = true;
		numFreePolyPoints = MAX_TRACEMODEL_WATER_POINTS_POOL;
	}

	if ( numFreePolyPoints == 0 ) {
		return NULL;
	}

	numFreePolyPoints--;
	polyPoint_t* point = freePolyPoints[ numFreePolyPoints ];
	return point;
}

/*
============
DeletePolyPoint
============
*/
void arcTraceModelCache::DeletePolyPoint( polyPoint_t* point ) {
/*	delete point;
	numPolyPoints--;*/
	for ( int i = 0; i < numFreePolyPoints; i++ ) {
		if ( freePolyPoints[ i ] == point ) {
			// WTF - shouldn't happen!
			int poo = 3;
		}
	}

	freePolyPoints[ numFreePolyPoints++ ] = point;
	point->node.Clear();
}

/*
============
DeletePointList
============
*/
void arcTraceModelCache::DeletePointList( idLinkList< polyPoint_t >& points ) {
	while ( polyPoint_t* next = points.Next() ) {
		DeletePolyPoint( next );
	}
}

/*
============
FindClosestPoints
============
*/
void arcTraceModelCache::FindClosestPoints( idLinkList< polyPoint_t >& points, polyPointPtr_t& closePoint1, polyPointPtr_t& closePoint2 ) {
	closePoint1 = NULL;
	closePoint2 = NULL;
	float closeDist	= arcMath::INFINITY;

	for ( polyPoint_t* next = points.Next(); next; next = next->node.Next() ) {
		for ( polyPoint_t* next2 = next->node.Next(); next2; next2 = next2->node.Next() ) {

			float dist = ( next->xyz - next2->xyz ).LengthSqr() * next->squareWeight * next2->squareWeight;
			if ( dist < closeDist ) {
				closeDist	= dist;
				closePoint1 = next;
				closePoint2 = next2;
			}
		}
	}
}

/*
============
arcTraceModelCache::SetupWaterPoints
============
*/
void arcTraceModelCache::SetupWaterPoints( trmCache_t& entry ) {
	arcTraceModel& trm = entry.trm;
	if ( !trm.bounds.GetVolume() || !trm.isConvex ) {
		return;
	}

	numFreePolyPoints = MAX_TRACEMODEL_WATER_POINTS_POOL;
	idLinkList< polyPoint_t > points;

	const int numPoints = MAX_TRACEMODEL_WATER_POINTS;

	int count;
	float startScale = 1.f;
	while ( true ) {
		count = 0;
		float numPerSide = pow( numPoints * startScale, 1.f / 3.f );
		arcVec3 spacing = ( trm.bounds.GetMaxs() - trm.bounds.GetMins() ) * ( 1 / numPerSide );
		if ( spacing.FixDenormals( 0.00001f ) ) {
			// one of the values is too small!
			return;
		}

		for ( float x = trm.bounds.GetMins().x; x < trm.bounds.GetMaxs().x; x+= spacing.x ) {
			for ( float y = trm.bounds.GetMins().y; y < trm.bounds.GetMaxs().y; y+= spacing.y ) {
				for ( float z = trm.bounds.GetMins().z; z < trm.bounds.GetMaxs().z; z+= spacing.z ) {
					arcVec3 xyz( x, y, z );
					xyz += spacing * 0.5f;

					if ( !trm.ContainsPoint( xyz ) ) {
						continue;
					}
					polyPoint_t* point = NewPolyPoint();
					if ( point == NULL ) {
						continue;
					}

					point->node.SetOwner( point );
					point->weight = 1.f;
					point->xyz = xyz;

					point->node.AddToEnd( points );
					count++;
				}
			}
		}

		if ( count < numPoints ) {
			DeletePointList( points );
		} else {
			break;
		}

		startScale *= 2.f;
		if ( startScale > 16.f ) {
			return;
		}
	}

	for ( polyPoint_t* next = points.Next(); next; next = next->node.Next() ) {
		next->weight /= count;
		next->squareWeight = Square( next->weight );
	}

	while ( count > numPoints ) {
		polyPoint_t* point1;
		polyPoint_t* point2;

		FindClosestPoints( points, point1, point2 );

		point1->xyz		= ( point1->xyz + point2->xyz ) * 0.5f;
		point1->weight	+= point2->weight;
		point1->squareWeight = Square( point1->weight );
		DeletePolyPoint( point2 );
		count--;
	}

	int i = 0;
	for ( polyPoint_t* next = points.Next(); next; next = next->node.Next() ) {
		entry.waterPoints[ i ].weight = next->weight;
		entry.waterPoints[ i ].xyz	= next->xyz;
		i++;
	}
	entry.hasWater = true;

	DeletePointList( points );
}

/*
============
arcTraceModelCache::Write
============
*/
void arcTraceModelCache::Write( int index, arcNetFile* fp ) {
	if ( index < 0 || index >= cache.Num() ) {
		gameLocal.Warning( "arcClipModel::Write: tried to write uncached trace model" );
		return;
	}

	trmCache_t& entry = *cache[index];

	fp->WriteString( TRM_HEADER );
	fp->WriteString( TRM_VERSION );

	entry.trm.Write( fp, &TrmNameForMaterial );
	fp->WriteFloat( entry.volume );
	fp->WriteVec3( entry.centerOfMass );
	fp->WriteMat3( entry.inertiaTensor );
	fp->WriteBool( entry.includesBrushes );
	fp->WriteBool( entry.hasWater );
	if ( entry.hasWater ) {
		for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
			fp->WriteVec3( entry.waterPoints[ i ].xyz );
			fp->WriteFloat( entry.waterPoints[ i ].weight );
		}
	}
}

/*
============
arcTraceModelCache::TrmMaterialForName
============
*/
const arcMaterial* arcTraceModelCache::TrmMaterialForName( const char* name ) {
	if ( *name == '\0' ) {
		return NULL;
	}
	return gameLocal.declMaterialType[ name ];
}

/*
============
arcTraceModelCache::TrmNameForMaterial
============
*/
const char* arcTraceModelCache::TrmNameForMaterial( const arcMaterial* material ) {
	if ( material == NULL ) {
		return "";
	}
	return material->GetName();
}

/*
============
arcTraceModelCache::Read
============
*/
void arcTraceModelCache::Read( arcTraceModel& trm, arcNetFile* fp ) {
	arcNetString header;
	fp->ReadString( header );

	if ( header != TRM_HEADER ) {
		gameLocal.Warning( "arcTraceModelCache::Read File is not a Trace Model '%s'", fp->GetName() );
		trm.SetupBox( 8.f );
		AllocFileEntry( fp->GetName(), AllocTraceModel( trm, false ) );
		return;
	}

	arcNetString version;
	fp->ReadString( version );
	if ( version != TRM_VERSION ) {
		gameLocal.Warning( "arcTraceModelCache::Read File has Wrong Version '%s'", fp->GetName() );
		trm.SetupBox( 8.f );
		AllocFileEntry( fp->GetName(), AllocTraceModel( trm, false ) );
		return;
	}

	trm.Read( fp, &TrmMaterialForName );

	trmCache_t entry;
	entry.trm = trm;
	entry.trm.ClearUnused();
	fp->ReadFloat( entry.volume );
	fp->ReadVec3( entry.centerOfMass );
	fp->ReadMat3( entry.inertiaTensor );
	fp->ReadBool( entry.includesBrushes );
	fp->ReadBool( entry.hasWater );
	if ( entry.hasWater ) {
		for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
			fp->ReadVec3( entry.waterPoints[ i ].xyz );
			fp->ReadFloat( entry.waterPoints[ i ].weight );
		}
	}

	int index = FindTraceModel( trm, entry.includesBrushes );
	if ( index != -1 ) {
		AllocFileEntry( fp->GetName(), index );
		return;
	}

	if ( FindTraceModel( fp->GetName(), entry.includesBrushes ) != -1 ) {
		gameLocal.Error( "arcTraceModelCache::Tried to Cache an Already Cached File'%s'", fp->GetName() );
	}

	int traceModelIndex = cache.Num();

	trmCache_t* cachedEntry = allocator.Alloc();
	cache.Alloc() = cachedEntry;
	*cachedEntry = entry;
	cachedEntry->refCount = 0;
	cachedEntry->collisionModel = collisionModelManager->ModelFromTrm( gameLocal.GetMapName(), va( "traceModel%d", traceModelIndex ), trm, entry.includesBrushes );

	int hashKey = GetTraceModelHashKey( trm );
	hash.Add( hashKey, traceModelIndex );

	AllocFileEntry( fp->GetName(), traceModelIndex );
}

/*
============
arcTraceModelCache::FindTraceModel
============
*/
int arcTraceModelCache::FindTraceModel( const char* fileName, bool includeBrushes ) {
	int index = FindFileEntry( fileName, includeBrushes );
	if ( index == -1 ) {
		return -1;
	}

	return fileCache[index].entryIndex;
}

/*
============
arcTraceModelCache::AllocFileEntry
============
*/
void arcTraceModelCache::AllocFileEntry( const char* fileName, int traceModelIndex ) {
	int hashKey = arcNetString::Hash( fileName );

	int cacheIndex = fileCache.Num();
	trmFileCache_t& cache = fileCache.Alloc();
	cache.fileName		= fileName;
	cache.entryIndex	= traceModelIndex;

	nameHash.Add( hashKey, cacheIndex );
}

/*
============
arcTraceModelCache::FindFileEntry
============
*/
int arcTraceModelCache::FindFileEntry( const char* fileName, bool includeBrushes ) {
	int hashKey = arcNetString::Hash( fileName );

	for ( int i = nameHash.GetFirst( hashKey ); i != idHashIndex::NULL_INDEX; i = nameHash.GetNext( i ) ) {
		if ( fileCache[ i ].fileName.Cmp( fileName ) != 0 ) {
			continue;
		}

		if ( cache[ fileCache[ i ].entryIndex ]->includesBrushes != includeBrushes ) {
			continue;
		}

		return i;
	}

	return -1;
}
