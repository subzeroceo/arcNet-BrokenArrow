#ifndef __TRACEMODELCACHE_H__
#define __TRACEMODELCACHE_H__

/*
===============================================================

	anTraceModelCache

===============================================================
*/

#define MAX_TRACEMODEL_WATER_POINTS			64
#define MAX_TRACEMODEL_WATER_POINTS_POOL	( MAX_TRACEMODEL_WATER_POINTS * 3 )

struct traceModelWater_t {
	anVec3				xyz;
	float				weight;
};

class anTraceModelCache {
public:
	void							ClearTraceModelCache( void );
	size_t							TraceModelCacheSize( void );

	int								FindTraceModel( const char *fileName, bool includeBrushes );
	int								FindTraceModel( const anTraceModel &trm, bool includeBrushes );
	int								PrecacheTraceModel( const anTraceModel &trm, const char *fileName );
	int								AllocTraceModel( const anTraceModel& trm, bool includeBrushes );
	void							FreeTraceModel( const int traceModelIndex );
	int								CopyTraceModel( const int traceModelIndex );

	const anTraceModel *			GetTraceModel( const int traceModelIndex ) const;
	const traceModelWater_t*		GetWaterPoints( const int traceModelIndex ) const;
	anCollisionModel *				GetCollisionModel( const int traceModelIndex ) const;
	float							GetVolume( const int traceModelIndex ) const;
	void							GetMassProperties( const int traceModelIndex, const float density, float &mass, anVec3 &centerOfMass, anMat3 &inertiaTensor ) const;

	static const anMaterial*		TrmMaterialForName( const char *name );
	static const char*				TrmNameForMaterial( const anMaterial* material );

	void							Write( int index, anFile *fp );
	void							Read( anTraceModel& trm, anFile *fp );

private:
	void							AllocFileEntry( const char *fileName, int traceModelIndex );
	int								FindFileEntry( const char *fileName, bool includeBrushes );

	// stuff for figuring out the water points
	struct polyPoint_t {
		anVec3						xyz;
		float						weight;
		float						squareWeight;
		anLinkList<polyPoint_t>	node;
	};

	typedef polyPoint_t*			polyPointPtr_t;

	static polyPoint_t				polyPointPool[MAX_TRACEMODEL_WATER_POINTS_POOL];
	static polyPoint_t*				freePolyPoints[MAX_TRACEMODEL_WATER_POINTS_POOL];
	static int						numFreePolyPoints;
	static bool						polyPointPoolValid;

	static polyPoint_t*				NewPolyPoint( void );
	static void						DeletePolyPoint( polyPoint_t *point );
	static void						DeletePointList( anLinkList<polyPoint_t> &points );
	static void						FindClosestPoints( anLinkList<polyPoint_t> &points, polyPointPtr_t &closePoint1, polyPointPtr_t &closePoint2 );

	struct trmCache_t {
		anTraceModel				trm;
		int							refCount;
		float						volume;				// volume of trace model
		anVec3						centerOfMass;		// center of mass
		anMat3						inertiaTensor;		// inertia tensor
		anCollisionModel *			collisionModel;		// trace model converted to a collision model
		bool						hasWater;
		bool						includesBrushes;
		traceModelWater_t			waterPoints[MAX_TRACEMODEL_WATER_POINTS];
	};

	struct trmFileCache_t {
		anStr						fileName;
		int							entryIndex;
	};

	anList<trmCache_t *>			cache;
	anBlockAlloc<trmCache_t, 64>	allocator;

	anList<trmFileCache_t>		fileCache;
	anHashIndex						nameHash;

	anHashIndex						hash;

	void							SetupWaterPoints( trmCache_t& entry );
	int								GetTraceModelHashKey( const anTraceModel &trm );
};

inline const anTraceModel *anTraceModelCache::GetTraceModel( const int traceModelIndex ) const {
	return &cache[ traceModelIndex ]->trm;
}

inline anCollisionModel *anTraceModelCache::GetCollisionModel( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->collisionModel;
}

inline const traceModelWater_t* anTraceModelCache::GetWaterPoints( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->hasWater ? cache[ traceModelIndex ]->waterPoints : nullptr;
}

inline float anTraceModelCache::GetVolume( const int traceModelIndex ) const {
	return cache[ traceModelIndex ]->volume;
}

#endif // !__TRACEMODELCACHE_H__