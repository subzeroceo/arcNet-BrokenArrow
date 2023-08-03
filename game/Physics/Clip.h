
#ifndef __CLIP_H__
#define __CLIP_H__

const int CLIPSECTOR_DEPTH				= 6;
const int CLIPSECTOR_WIDTH				= 1 << CLIPSECTOR_DEPTH;

/*
===============================================================================

  Handles collision detection with the world and between physics objects.

===============================================================================
*/

#define CLIPMODEL_ID_TO_JOINT_HANDLE( id )	( ( id ) >= 0 ? INVALID_JOINT : ((jointHandle_t) ( -1 - id ) ) )
#define JOINT_HANDLE_TO_CLIPMODEL_ID( id )	( -1 - id )

class anClip;
class anClipModel;
class anEntity;


//===============================================================
//
//	anClipModel
//
//===============================================================

class anClipModel {
	friend class anClip;
public:
							anClipModel( void );
							explicit anClipModel( const char *name );
							explicit anClipModel( const anTraceModel &trm, const anMaterial *material = nullptr );
							explicit anClipModel( const int renderModelHandle );
							explicit anClipModel( const anClipModel *model );

	void					UpdateDynamicContents( void );

	bool					LoadModel( const char *name );
	void					LoadModel( const anTraceModel &trm, const anMaterial *material, bool notHashed = false );
	void					LoadModel( const int renderModelHandle );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					Link( void );				// must have been linked with an entity and id before
	void					Link( anEntity *ent, int newId, const anVec3 &newOrigin, const anMat3 &newAxis, int renderModelHandle = -1 );

	void					Unlink( void );						// unlink from sectors
	void					SetPosition( const anVec3 &newOrigin, const anMat3 &newAxis );	// unlinks the clip model
	void					Translate( const anVec3 &translation );							// unlinks the clip model
	void					Rotate( const anRotation &rotation );							// unlinks the clip model
	void					Enable( void );						// enable for clipping
	void					Disable( void );					// keep linked but disable for clipping
	void					SetContents( int newContents );		// override contents
	int						GetContents( void ) const;
	void					SetEntity( anEntity *newEntity );
	anEntity *				GetEntity( void ) const;
	void					SetId( int newId );
	int						GetId( void ) const;
	void					SetOwner( anEntity *newOwner );
	anEntity *				GetOwner( void ) const;
	const anBounds &		GetBounds( void ) const;
	const anBounds &		GetAbsBounds( void ) const;
	const anVec3 &			GetOrigin( void ) const;
	const anMat3 &			GetAxis( void ) const;
	bool					IsTraceModel( void ) const;			// returns true if this is a trace model
	bool					IsRenderModel( void ) const;		// returns true if this is a render model
	bool					IsLinked( void ) const;				// returns true if the clip model is linked
	bool					IsEnabled( void ) const;			// returns true if enabled for collision detection
	bool					IsEqual( const anTraceModel &trm ) const;
	anCollisionModel *		GetCollisionModel( void ) const;	// returns handle used to collide vs this model
	const anTraceModel *	GetTraceModel( void ) const;
	void					GetMassProperties( const float density, float &mass, anVec3 &centerOfMass, anMat3 &inertiaTensor ) const;

	static void				ClearTraceModelCache( void );
	static int				TraceModelCacheSize( void );
	static void				SaveTraceModels( anSaveGame *savefile );
	static void				RestoreTraceModels( anRestoreGame *savefile );

private:
	bool					enabled;				// true if this clip model is used for clipping

	bool					checked;				// Splash's clip model code

	anEntity *				entity;					// entity using this clip model
	int						id;						// id for entities that use multiple clip models
	anEntity *				owner;					// owner of the entity that owns this clip model
	anVec3					origin;					// origin of clip model
	anMat3					axis;					// orientation of clip model
	anBounds				bounds;					// bounds
	anBounds				absBounds;				// absolute bounds
	int						contents;				// all contents ored together
	anCollisionModel *		collisionModel;			// handle to collision model
	int						traceModelIndex;		// trace model used for collision detection
	int						renderModelHandle;		// render model def handle

	struct clipLink_s *		clipLinks;				// links into sectors
	int						touchCount;

	void					Init( void );			// initialize
	void					FreeModel( void );
	void					Link_r( struct clipSector_s *node );

	static void				CacheCollisionModels( void );
	static int				AllocTraceModel( const anTraceModel &trm, const anMaterial *material, bool notHashed = false );
	static void				ReplaceTraceModel( int index, const anTraceModel &trm, const anMaterial *material, bool notHashed = false );
	static void				FreeTraceModel( int traceModelIndex );
	static int				CopyTraceModel( const int traceModelIndex );
	static anTraceModel *	GetCachedTraceModel( int traceModelIndex );
	static anCollisionModel*GetCachedCollisionModel( int traceModelIndex );
	static int				GetTraceModelHashKey( const anTraceModel &trm );
};

ARC_INLINE void anClipModel::Translate( const anVec3 &translation ) {
	Unlink();
	origin += translation;
}

ARC_INLINE void anClipModel::Rotate( const anRotation &rotation ) {
	Unlink();
	origin *= rotation;
	axis *= rotation.ToMat3();
}

ARC_INLINE void anClipModel::Enable( void ) {
	enabled = true;
}

ARC_INLINE void anClipModel::Disable( void ) {
	enabled = false;
}

ARC_INLINE void anClipModel::SetContents( int newContents ) {
	contents = newContents;
	UpdateDynamicContents();
}

ARC_INLINE int anClipModel::GetContents( void ) const {
	return contents;
}

ARC_INLINE void anClipModel::SetEntity( anEntity *newEntity ) {
	entity = newEntity;
}

ARC_INLINE anEntity *anClipModel::GetEntity( void ) const {
	return entity;
}

ARC_INLINE void anClipModel::SetId( int newId ) {
	id = newId;
}

ARC_INLINE int anClipModel::GetId( void ) const {
	return id;
}

ARC_INLINE void anClipModel::SetOwner( anEntity *newOwner ) {
	owner = newOwner;
}

ARC_INLINE anEntity *anClipModel::GetOwner( void ) const {
	return owner;
}

ARC_INLINE const anBounds &anClipModel::GetBounds( void ) const {
	return bounds;
}

ARC_INLINE const anBounds &anClipModel::GetAbsBounds( void ) const {
	return absBounds;
}

ARC_INLINE const anVec3 &anClipModel::GetOrigin( void ) const {
	return origin;
}

ARC_INLINE const anMat3 &anClipModel::GetAxis( void ) const {
	return axis;
}

ARC_INLINE bool anClipModel::IsRenderModel( void ) const {
	return ( renderModelHandle != -1 );
}

ARC_INLINE bool anClipModel::IsTraceModel( void ) const {
	return ( traceModelIndex != -1 );
}

ARC_INLINE bool anClipModel::IsLinked( void ) const {
	return ( clipLinks != nullptr );
}

ARC_INLINE bool anClipModel::IsEnabled( void ) const {
	return enabled;
}

ARC_INLINE bool anClipModel::IsEqual( const anTraceModel &trm ) const {
	return ( traceModelIndex != -1 && *GetCachedTraceModel( traceModelIndex ) == trm );
}

ARC_INLINE const anTraceModel *anClipModel::GetTraceModel( void ) const {
	if ( !IsTraceModel() ) {
		return nullptr;
	}
	return anClipModel::GetCachedTraceModel( traceModelIndex );
}

//===============================================================
//
//	anClip
//
//===============================================================

class anClip {

	friend class anClipModel;

public:
							anClip( void );

	void					Init( void );
	void					Shutdown( void );


	bool					Translation( trace_t &results, const anVec3 &start, const anVec3 &end,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask, const anEntity *passEntity, const anEntity *passEntity2 = 0 );
	bool					Rotation( trace_t &results, const anVec3 &start, const anRotation &rotation,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask, const anEntity *passEntity );
	bool					Motion( trace_t &results, const anVec3 &start, const anVec3 &end, const anRotation &rotation,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask, const anEntity *passEntity );
	int						Contacts( contactInfo_t *contacts, const int maxContacts, const anVec3 &start, const anVec6 &dir, const float depth,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask, const anEntity *passEntity );
	int						Contents( const anVec3 &start,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask, const anEntity *passEntity, anEntity **touchedEntity = nullptr );

	// special case translations versus the rest of the world
	bool					TracePoint( trace_t &results, const anVec3 &start, const anVec3 &end,
								int contentMask, const anEntity *passEntity );
	bool					TraceBounds( trace_t &results, const anVec3 &start, const anVec3 &end, const anBounds &bounds,
								int contentMask, const anEntity *passEntity );

	// clip versus a specific model
	void					TranslationModel( trace_t &results, const anVec3 &start, const anVec3 &end,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask,
								anCollisionModel *model, const anVec3 &modelOrigin, const anMat3 &modelAxis );
	void					RotationModel( trace_t &results, const anVec3 &start, const anRotation &rotation,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask,
								anCollisionModel *model, const anVec3 &modelOrigin, const anMat3 &modelAxis );
	int						ContactsModel( contactInfo_t *contacts, const int maxContacts, const anVec3 &start, const anVec6 &dir, const float depth,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask,
								anCollisionModel *model, const anVec3 &modelOrigin, const anMat3 &modelAxis );
	int						ContentsModel( const anVec3 &start,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask,
								anCollisionModel *model, const anVec3 &modelOrigin, const anMat3 &modelAxis );

	void					TranslationEntities( trace_t &results, const anVec3 &start, const anVec3 &end,
								const anClipModel *mdl, const anMat3 &trmAxis, int contentMask, const anEntity *passEntity, const anEntity *passEntity2 = 0 );
	// get a contact feature
	bool					GetModelContactFeature( const contactInfo_t &contact, const anClipModel *clipModel, anFixedWinding &winding ) const;

	// get entities/clip models within or touching the given bounds
	int						EntitiesTouchingBounds( const anBounds &bounds, int contentMask, anEntity **entityList, int maxCount ) const;
	int						ClipModelsTouchingBounds( const anBounds &bounds, int contentMask, anClipModel **clipModelList, int maxCount ) const;

	int						PlayersTouchingBounds( const anBounds &bounds, int contentMask, anBasePlayer **entityList, int maxCount ) const;

	const anBounds &		GetWorldBounds( void ) const;
	anCollisionModel *		GetWorldCollisionModel( void ) const { return world; }

	static anClipModel *	DefaultClipModel( void );
	static void				FreeDefaultClipModel( void );

							// stats and debug drawing
	void					PrintStatistics( void );
	void					DrawClipModels( const anVec3 &eye, const float radius, const anEntity *passEntity, const idTypeInfo* type = nullptr );
	bool					DrawModelContactFeature( const contactInfo_t &contact, const anClipModel *clipModel, int lifetime ) const;

	void					DebugHudStatistics( void );
	void					ClearStatistics( void );
	void					CoordsForBounds( int* coords, anBounds& bounds ) const;
	void					DrawClipSectors( void ) const;
	void					DrawAreaClipSectors( float range ) const;
	static void				UpdateDynamicContents( struct clipSector_s* sector );
	static void				UpdateDynamicContents( anClipModel* clipModel );

private:
	anVec3					nodeScale;
	anVec3					nodeOffset;
	anVec3					nodeOffsetVisual;
	static anClipModel		defaultClipModel;
	struct clipSector_s *	clipSectors;
	anCollisionModel *		world;
	anBounds				worldBounds;
	anClipModel				temporaryClipModel;

	mutable int				touchCount;
							// statistics
	int						numTranslations;
	int						numRotations;
	int						numMotions;
	int						numRenderModelTraces;
	int						numContents;
	int						numContacts;

private:
	struct clipSector_s *	CreateClipSectors_r( const int depth, const anBounds &bounds, anVec3 &maxSector );
	void					ClipModelsTouchingBounds_r( const struct clipSector_s *node, struct listParms_s &parms ) const;
	const anTraceModel *	TraceModelForClipModel( const anClipModel *mdl ) const;
	int						GetTraceClipModels( const anBounds &bounds, int contentMask, const anEntity *passEntity, anClipModel **clipModelList, const anEntity *passEntity2 = 0 ) const;

	void					TraceRenderModel( trace_t &trace, const anVec3 &start, const anVec3 &end, const float radius, const anMat3 &axis, anClipModel *touch ) const;
	void					GetClipSectorsStaticContents( void );
};

ARC_INLINE bool anClip::TracePoint( trace_t &results, const anVec3 &start, const anVec3 &end, int contentMask, const anEntity *passEntity ) {
	Translation( results, start, end, nullptr, mat3_identity, contentMask, passEntity );
	return ( results.fraction < 1.0f );
}

ARC_INLINE bool anClip::TraceBounds( trace_t &results, const anVec3 &start, const anVec3 &end, const anBounds &bounds, int contentMask, const anEntity *passEntity ) {
	temporaryClipModel.LoadModel( anTraceModel( bounds ), nullptr, true );
	Translation( results, start, end, &temporaryClipModel, mat3_identity, contentMask, passEntity );
	return ( results.fraction < 1.0f );
}

ARC_INLINE const anBounds & anClip::GetWorldBounds( void ) const {
	return worldBounds;
}

ARC_INLINE void anClip::CoordsForBounds( int* coords, anBounds& bounds ) const {
	float fCoords[ 4 ];

	fCoords[ 0 ] = ( bounds[ 0 ].x - nodeOffset.x ) * nodeScale.x;
	fCoords[ 1 ] = ( bounds[ 0 ].y - nodeOffset.y ) * nodeScale.y;
	fCoords[ 2 ] = ( bounds[ 1 ].x - nodeOffset.x ) * nodeScale.x;
	fCoords[ 3 ] = ( bounds[ 1 ].y - nodeOffset.y ) * nodeScale.y;

	int i;
	for ( i = 0; i < 4; i++ ) {

		coords[i] = anMath::FtoiFast( fCoords[i] );

		if ( coords[i] < 0 ) {
			coords[i] = 0;
		} else if ( coords[i] > CLIPSECTOR_WIDTH - 1 ) {
			coords[i] = CLIPSECTOR_WIDTH - 1;
		}
	}
	coords[ 2 ]++; coords[ 3 ]++;
}

#endif // !__CLIP_H__
