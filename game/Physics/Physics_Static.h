
#ifndef __PHYSICS_STATIC_H__
#define __PHYSICS_STATIC_H__

/*
===============================================================================

	Physics for a non moving object using at most one collision model.

===============================================================================
*/

// RAVEN BEGIN
// rjohnson: converted this from a struct to a class
class idStaticPState {

public:
	arcVec3					origin;
	arcMat3					axis;
	arcVec3					localOrigin;
	arcMat3					localAxis;

							idStaticPState( void ) { }
};
// RAVEN END

class idPhysics_Static : public idPhysics {

public:
	CLASS_PROTOTYPE( idPhysics_Static );

							idPhysics_Static( void );
							~idPhysics_Static( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

public:	// common physics interface
	void					SetSelf( idEntity *e );
//RAVEN BEGIN
// abahr: for gravity
	virtual idEntity*		GetSelf() const { return self; }
// RAVEN END

	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

// RAVEN BEGIN
// bdube: means of getting center of mass
	arcVec3					GetCenterMass( int id = -1 ) const;
// RAVEN END

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	void					SetClipMask( int mask, int id = -1 );
	int						GetClipMask( int id = -1 ) const;

	const arcBounds &		GetBounds( int id = -1 ) const;
	const arcBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const arcVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const arcVec3 &point, const arcVec3 &impulse );
	void					AddForce( const int id, const arcVec3 &point, const arcVec3 &force );
	void					Activate( void );
	void					PutToRest( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;
// RAVEN BEGIN
// bdube: water interraction
	bool					IsInWater ( void ) const;
// RAVEN END

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const arcVec3 &newOrigin, int id = -1 );
	void					SetAxis( const arcMat3 &newAxis, int id = -1 );

	void					Translate( const arcVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const arcVec3 &			GetOrigin( int id = 0 ) const;
	const arcMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const arcVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const arcVec3 &newAngularVelocity, int id = 0 );

	const arcVec3 &			GetLinearVelocity( int id = 0 ) const;
	const arcVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					SetGravity( const arcVec3 &newGravity );
	const arcVec3 &			GetGravity( void ) const;
	const arcVec3 &			GetGravityNormal( void ) const;

	void					ClipTranslation( trace_t &results, const arcVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	bool					EvaluateContacts( void );
	int						GetNumContacts( void ) const;
	const contactInfo_t &	GetContact( int num ) const;
	void					ClearContacts( void );
	void					AddContactEntity( idEntity *e );
	void					RemoveContactEntity( idEntity *e );

	bool					HasGroundContacts( void ) const;
	bool					IsGroundEntity( int entityNum ) const;
	bool					IsGroundClipModel( int entityNum, int id ) const;

	void					SetPushed( int deltaTime );
	const arcVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const arcVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated = true );

	const trace_t *			GetBlockingInfo( void ) const;
	idEntity *				GetBlockingEntity( void ) const;

	int						GetLinearEndTime( void ) const;
	int						GetAngularEndTime( void ) const;

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

protected:

	idEntity *				self;					// entity using this physics object
// RAVEN BEGIN
// rjohnson: converted this from a struct to a class
	idStaticPState			current;				// physics state
// RAVEN END
	idClipModel *			clipModel;				// collision model

	// master
	bool					hasMaster;
	bool					isOrientated;
};

#endif /* !__PHYSICS_STATIC_H__ */
