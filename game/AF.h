#ifndef __GAME_AF_H__
#define __GAME_AF_H__

/*
===============================================================================

  Articulated figure controller.

===============================================================================
*/

typedef struct jointConversion_s {
	int						bodyId;				// id of the body
	jointHandle_t			jointHandle;		// handle of joint this body modifies
	AFJointModType_t		jointMod;			// modify joint axis, origin or both
	arcVec3					jointBodyOrigin;	// origin of body relative to joint
	arcMat3					jointBodyAxis;		// axis of body relative to joint
} jointConversion_t;

typedef struct afTouch_s {
	arcEntity *				touchedEnt;
	arcClipModel *			touchedClipModel;
	arcAFBody *				touchedByBody;
} afTouch_t;

class arcAF {
public:
							arcAF( void );
							~arcAF( void );

	void					Save( arcSaveGame *savefile ) const;
	void					Restore( arcRestoreGame *savefile );

	void					SetAnimator( arcAnimator *a ) { animator = a; }
	bool					Load( arcEntity *ent, const char *fileName );
	bool					IsLoaded( void ) const { return isLoaded && self != NULL; }
	const char *			GetName( void ) const { return name.c_str(); }
	void					SetupPose( arcEntity *ent, int time );
	void					ChangePose( arcEntity *ent, int time );
	int						EntitiesTouchingAF( afTouch_t touchList[ MAX_GENTITIES ] ) const;
	void					Start( void );
	void					StartFromCurrentPose( int inheritVelocityTime );
	void					Stop( void );
	void					Rest( void );
	bool					IsActive( void ) const { return isActive; }
	void					SetConstraintPosition( const char *name, const arcVec3 &pos );

	arcPhysics_AF *			GetPhysics( void ) { return &physicsObj; }
	const arcPhysics_AF *	GetPhysics( void ) const { return &physicsObj; }
	arcBounds				GetBounds( void ) const;
	bool					UpdateAnimation( void );

	void					GetPhysicsToVisualTransform( arcVec3 &origin, arcMat3 &axis ) const;
	void					GetImpactInfo( arcEntity *ent, int id, const arcVec3 &point, impactInfo_t *info );
	void					ApplyImpulse( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &impulse );
	void					AddForce( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &force );
	int						BodyForClipModelId( int id ) const;

	void					SaveState( arcDict &args ) const;
	void					LoadState( const arcDict &args );

	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );

protected:
	idStr					name;				// name of the loaded .af file
	arcPhysics_AF			physicsObj;			// articulated figure physics
	arcEntity *				self;				// entity using the animated model
	arcAnimator *			animator;			// animator on entity
	int						modifiedAnim;		// anim to modify
	arcVec3					baseOrigin;			// offset of base body relative to skeletal model origin
	arcMat3					baseAxis;			// axis of base body relative to skeletal model origin
	arcList<jointConversion_t>jointMods;			// list with transforms from skeletal model joints to articulated figure bodies
	arcList<int>				jointBody;			// table to find the nearest articulated figure body for a joint of the skeletal model
	int						poseTime;			// last time the articulated figure was transformed to reflect the current animation pose
	int						restStartTime;		// time the articulated figure came to rest
	bool					isLoaded;			// true when the articulated figure is properly loaded
	bool					isActive;			// true if the articulated figure physics is active
	bool					hasBindConstraints;	// true if the bind constraints have been added

protected:
	void					SetBase( arcAFBody *body, const idJointMat *joints );
	void					AddBody( arcAFBody *body, const idJointMat *joints, const char *jointName, const AFJointModType_t mod );

	bool					LoadBody( const arcDeclAF_Body *fb, const idJointMat *joints );
	bool					LoadConstraint( const arcDeclAF_Constraint *fc );

	bool					TestSolid( void ) const;
};

#endif // !__GAME_AF_H__