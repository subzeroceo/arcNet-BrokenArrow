#ifndef __TRACEMODELCACHE_H__
#define __TRACEMODELCACHE_H__

/*
===============================================================

	arcTraceModelCache

===============================================================
*/

#define MAX_TRACEMODEL_WATER_POINTS			64
#define MAX_TRACEMODEL_WATER_POINTS_POOL	( MAX_TRACEMODEL_WATER_POINTS * 3 )

struct traceModelWater_t {
	arcVec3				xyz;
	float				weight;
};

class arcTraceModelCache {
public:
	void							ClearTraceModelCache( void );
	size_t							TraceModelCacheSize( void );

	int								FindTraceModel( const char* fileName, bool includeBrushes );
	int								FindTraceModel( const arcTraceModel &trm, bool includeBrushes );
	int								PrecacheTraceModel( const arcTraceModel &trm, const char* fileName );
	int								AllocTraceModel( const arcTraceModel& trm, bool includeBrushes );
	void							FreeTraceModel( const int traceModelIndex );
	int								CopyTraceModel( const int traceModelIndex );

	const arcTraceModel *			GetTraceModel( const int traceModelIndex ) const;
	const traceModelWater_t*		GetWaterPoints( const int traceModelIndex ) const;
	arcCollisionModel *				GetCollisionModel( const int traceModelIndex ) const;
	float							GetVolume( const int traceModelIndex ) const;
	void							GetMassProperties( const int traceModelIndex, const float density, float &mass, arcVec3 &centerOfMass, arcMat3 &inertiaTensor ) const;

	static const arcMaterial*		TrmMaterialForName( const char* name );
	static const char*				TrmNameForMaterial( const arcMaterial* material );

	void							Write( int index, arcNetFile* fp );
	void							Read( arcTraceModel& trm, arcNetFile* fp );

private:
	void							AllocFileEntry( const char* fileName, int traceModelIndex );
	int								FindFileEntry( const char* fileName, bool includeBrushes );

	// stuff for figuring out the water points
	struct polyPoint_t {
		arcVec3						xyz;
		float						weight;
		float						squareWeight;
		idLinkList< polyPoint_t >	node;
	};

	typedef polyPoint_t*			polyPointPtr_t;

	static polyPoint_t				polyPointPool[ MAX_TRACEMODEL_WATER_POINTS_POOL ];
	static polyPoint_t*				freePolyPoints[ MAX_TRACEMODEL_WATER_POINTS_POOL ];
	static int						numFreePolyPoints;
	static bool						polyPointPoolValid;

	static polyPoint_t*				NewPolyPoint( void );
	static void						DeletePolyPoint( polyPoint_t* point );
	static void						DeletePointList( idLinkList< polyPoint_t >& points );
	static void						FindClosestPoints( idLinkList< polyPoint_t >& points, polyPointPtr_t& closePoint1, polyPointPtr_t& closePoint2 );

	struct trmCache_t {
		arcTraceModel				trm;
		int							refCount;
		float						volume;				// volume of trace model
		arcVec3						centerOfMass;		// center of mass
		arcMat3						inertiaTensor;		// inertia tensor
		arcCollisionModel *			collisionModel;		// trace model converted to a collision model
		bool						hasWater;
		bool						includesBrushes;
		traceModelWater_t			waterPoints[ MAX_TRACEMODEL_WATER_POINTS ];
	};

	struct trmFileCache_t {
		arcNetString						fileName;
		int							entryIndex;
	};

	arcNetList< trmCache_t* >			cache;
	arcBlockAlloc< trmCache_t, 64 >	allocator;

	arcNetList< trmFileCache_t >		fileCache;
	idHashIndex						nameHash;

	idHashIndex						hash;

	void							SetupWaterPoints( trmCache_t& entry );
	int								GetTraceModelHashKey( const arcTraceModel &trm );
};

ARC_INLINE const arcTraceModel *arcTraceModelCache::GetTraceModel( const int traceModelIndex ) const {
	return &cache[ traceModelIndex ]->trm;
}

ARC_INLINE arcCollisionModel *arcTraceModelCache::GetCollisionModel( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->collisionModel;
}

ARC_INLINE const traceModelWater_t* arcTraceModelCache::GetWaterPoints( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->hasWater ? cache[ traceModelIndex ]->waterPoints : NULL;
}

ARC_INLINE float arcTraceModelCache::GetVolume( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->volume;
}

#endif /* !__TRACEMODELCACHE_H__ */
