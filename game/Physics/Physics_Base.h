
#ifndef __PHYSICS_BASE_H__
#define __PHYSICS_BASE_H__

/*
===============================================================================

	Physics base for a moving object using one or more collision models.

===============================================================================
*/

// RAVEN BEGIN
// jnewquist: Changed from #define to typedef to fix compiler issues
typedef idEntityPtr<idEntity> contactEntity_t;
// RAVEN END

class idPhysics_Base : public idPhysics {

public:
	CLASS_PROTOTYPE( idPhysics_Base );

							idPhysics_Base( void );
							~idPhysics_Base( void );

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
// bdube: added center mass
	arcVec3					GetCenterMass ( int id = -1 ) const;
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

// RAVEN BEGIN
// abahr: helper function used in projectiles
	virtual const arcVec3	GetContactNormal() const;
	virtual const arcVec3	GetGroundContactNormal() const;
// RAVEN END

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
	int						clipMask;				// contents the physics object collides with
	arcVec3					gravityVector;			// direction and magnitude of gravity
	arcVec3					gravityNormal;			// normalized direction of gravity
	idList<contactInfo_t>	contacts;				// contacts with other physics objects
	idList<contactEntity_t>	contactEntities;		// entities touching this physics object

protected:
							// add ground contacts for the clip model
	void					AddGroundContacts( const idClipModel *clipModel );
							// add contact entity links to contact entities
	void					AddContactEntitiesForContacts( void );
							// active all contact entities
	void					ActivateContactEntities( void );
							// returns true if the whole physics object is outside the world bounds
	bool					IsOutsideWorld( void ) const;
							// draw linear and angular velocity
	void					DrawVelocity( int id, float linearScale, float angularScale ) const;
};

#endif /* !__PHYSICS_BASE_H__ */
