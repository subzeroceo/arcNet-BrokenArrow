
#ifndef __PHYSICS_ACTOR_H__
#define __PHYSICS_ACTOR_H__

/*
===================================================================================

	Actor physics base class

	An actor typically uses one collision model which is aligned with the gravity
	direction. The collision model is usually a simple box with the origin at the
	bottom center.

===================================================================================
*/

class anPhysics_Actor : public anPhysics_Base {

public:
	CLASS_PROTOTYPE( anPhysics_Actor );

							anPhysics_Actor( void );
							~anPhysics_Actor( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

							// get delta yaw of master
	float					GetMasterDeltaYaw( void ) const;
							// returns the ground entity
	anEntity *				GetGroundEntity( void ) const;
							// align the clip model with the gravity direction
	void					SetClipModelAxis( void );

public:	// common physics interface
	void					SetClipModel( anClipModel *model, float density, int id = 0, bool freeOld = true );
	anClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const anBounds &		GetBounds( int id = -1 ) const;
	const anBounds &		GetAbsBounds( int id = -1 ) const;

	bool					IsPushable( void ) const;

	const anVec3 &			GetOrigin( int id = 0 ) const;
	const anMat3 &			GetAxis( int id = 0 ) const;

	void					SetGravity( const anVec3 &newGravity );

// abahr: made virtual
	virtual const anMat3&	GetGravityAxis( void ) const;


	void					ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const;
	void					ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const;
	int						ClipContents( const anClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	bool					EvaluateContacts( void );

protected:
	anClipModel *			clipModel;			// clip model used for collision detection
	anMat3					clipModelAxis;		// axis of clip model aligned with gravity direction

	// derived properties
	float					mass;
	float					invMass;

	// master
	anEntity *				masterEntity;
	float					masterYaw;
	float					masterDeltaYaw;

	// results of last evaluate
	anEntityPtr<anEntity>	groundEntityPtr;
};

#endif /* !__PHYSICS_ACTOR_H__ */
