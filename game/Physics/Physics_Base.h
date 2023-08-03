
#ifndef __PHYSICS_BASE_H__
#define __PHYSICS_BASE_H__

/*
===============================================================================

	Physics base for a moving object using one or more collision models.

===============================================================================
*/

#define anEntityPtr <anEntity> contactEntity_t;

class anPhysics_Base {//: public anPhysics {

public:
	CLASS_PROTOTYPE( anPhysics_Base );

							anPhysics_Base( void );
							~anPhysics_Base( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

public:	// common physics interface

	void					SetSelf( anEntity *e );
	virtual anEntity *		GetSelf() const { return self; }


	void					SetClipModel( anClipModel *model, float density, int id = 0, bool freeOld = true );
	anClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	anVec3					GetCenterMass ( int id = -1 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	void					SetClipMask( int mask, int id = -1 );
	int						GetClipMask( int id = -1 ) const;

	const anBounds &		GetBounds( int id = -1 ) const;
	const anBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse );
	void					AddForce( const int id, const anVec3 &point, const anVec3 &force );
	void					Activate( void );
	void					PutToRest( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	bool					IsInWater ( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const anVec3 &newOrigin, int id = -1 );
	void					SetAxis( const anMat3 &newAxis, int id = -1 );

	void					Translate( const anVec3 &translation, int id = -1 );
	void					Rotate( const anRotation &rotation, int id = -1 );

	const anVec3 &			GetOrigin( int id = 0 ) const;
	const anMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const anVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const anVec3 &newAngularVelocity, int id = 0 );

	const anVec3 &			GetLinearVelocity( int id = 0 ) const;
	const anVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					SetGravity( const anVec3 &newGravity );
	const anVec3 &			GetGravity( void ) const;
	const anVec3 &			GetGravityNormal( void ) const;

	void					ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const;
	void					ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const;
	int						ClipContents( const anClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	bool					EvaluateContacts( void );
	int						GetNumContacts( void ) const;
	const contactInfo_t &	GetContact( int num ) const;
	void					ClearContacts( void );
	void					AddContactEntity( anEntity *e );
	void					RemoveContactEntity( anEntity *e );

							// helper function used in projectiles
	virtual const anVec3	GetContactNormal() const;
	virtual const anVec3	GetGroundContactNormal() const;

	bool					HasGroundContacts( void ) const;
	bool					IsGroundEntity( int entityNum ) const;
	bool					IsGroundClipModel( int entityNum, int id ) const;

	void					SetPushed( int deltaTime );
	const anVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const anVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( anEntity *master, const bool orientated = true );

	const trace_t *			GetBlockingInfo( void ) const;
	anEntity *				GetBlockingEntity( void ) const;

	int						GetLinearEndTime( void ) const;
	int						GetAngularEndTime( void ) const;

	void					WriteToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const anBitMsgDelta &msg );

protected:
	anEntity *				self;					// entity using this physics object
	int						clipMask;				// contents the physics object collides with
	anVec3					gravityVector;			// direction and magnitude of gravity
	anVec3					gravityNormal;			// normalized direction of gravity
	anList<contactInfo_t>	contacts;				// contacts with other physics objects
	anList<contactEntity_t>	contactEntities;		// entities touching this physics object

protected:
							// add ground contacts for the clip model
	void					AddGroundContacts( const anClipModel *clipModel );
							// add contact entity links to contact entities
	void					AddContactEntitiesForContacts( void );
							// active all contact entities
	void					ActivateContactEntities( void );
							// returns true if the whole physics object is outside the world bounds
	bool					IsOutsideWorld( void ) const;
							// draw linear and angular velocity
	void					DrawVelocity( int id, float linearScale, float angularScale ) const;
};

#endif // !__PHYSICS_BASE_H__
