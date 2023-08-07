#ifndef __GAME_AF_H__
#define __GAME_AF_H__

#include "physics/Physics_AF.h"
/*
===============================================================================

  Articulated figure controller.

===============================================================================
*/

class anEntity;
class anClipModel;
class anAFBody;
class anAnimator;
class anDeclAF_Body;
class anDeclAF_Constraint;

typedef struct jointConversion_s {
	int						bodyId;				// id of the body
	jointHandle_t			jointHandle;		// handle of joint this body modifies
	AFJointModType_t		jointMod;			// modify joint axis, origin or both
	anVec3					jointBodyOrigin;	// origin of body relative to joint
	anMat3					jointBodyAxis;		// axis of body relative to joint
} jointConversion_t;

typedef struct afTouch_s {
	anEntity *				touchedEnt;
	anClipModel *			touchedClipModel;
	anAFBody *				touchedByBody;
} afTouch_t;

class anAF {
public:
							anAF( void );
							~anAF( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					SetAnimator( anAnimator *a ) { animator = a; }
	bool					Load( anEntity *ent, const char *fileName );

	void					UnLoad( void ) { isLoaded = false; }
	bool					IsLoaded( void ) const { return isLoaded && self != nullptr; }

	const char *			GetName( void ) const { return name.c_str(); }
	void					SetupPose( anEntity *ent, int time );
	void					ChangePose( anEntity *ent, int time );
	int						EntitiesTouchingAF( afTouch_t touchList[ MAX_GENTITIES ] ) const;
	void					Start( void );
	void					StartFromCurrentPose( int inheritVelocityTime );
	void					Stop( void );
	void					Rest( void );

	void					SetActive( bool active ) { isActive = active; }
	bool					IsActive( void ) const { return isActive; }

	void					SetConstraintPosition( const char *name, const anVec3 &pos );

	//void					Activate();
	//void					Deactivate();

	//int					GetNbLinks() const;
	//int 					GetLinks( anAFLink **userBuffer, unsigned int bufferSize, unsigned int startIndex ) const;
	//const anAFLink*		GetLink( const char *name ) const;
	//void					AddToLinkList( anAFLink &link ) { mArticulationLinks.pushBack( &link ); }
	//bool 					RemoveLinkFromList( anAFLink &link ) { assert( mArticulationLinks.find( &link ) != mArticulationLinks.end() ); return mArticulationLinks.findAndReplaceWithLast( &link ); }
	
	//anAFLink *			GetLinks() {return mArticulationLinks.Begin(); }

	anPhysics_AF *			GetPhysics( void ) { return &physicsObj; }
	const anPhysics_AF *	GetPhysics( void ) const { return &physicsObj; }
	anBounds				GetBounds( void ) const;
	//anBounds				GetWorldBounds( float inflation = 1.01f ) const;

	bool					UpdateAnimation( void );

	void					GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis ) const;
	void					GetImpactInfo( anEntity *ent, int id, const anVec3 &point, impactInfo_t *info );
	void					ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse );
	void					AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force );
	int						BodyForClipModelId( int id ) const;

	void					SaveState( anDict &args ) const;
	void					LoadState( const anDict &args );

	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );
	void					SetSolverIterations( unsigned int positionIters, unsigned velocityIters );
	void					GetSolverIterations( unsigned int &positionIters, unsigned &velocityIters ) const;

protected:
	anStr					name;				// name of the loaded .af file
	anPhysics_AF			physicsObj;			// articulated figure physics
	anEntity *				self;				// entity using the animated model
	anAnimator *			animator;			// animator on entity
	int						modifiedAnim;		// anim to modify
	anVec3					baseOrigin;			// offset of base body relative to skeletal model origin
	anMat3					baseAxis;			// axis of base body relative to skeletal model origin
	anList<jointConversion_t>jointMods;			// list with transforms from skeletal model joints to articulated figure bodies
	anList<int>				jointBody;			// table to find the nearest articulated figure body for a joint of the skeletal model
	int						poseTime;			// last time the articulated figure was transformed to reflect the current animation pose
	int						restStartTime;		// time the articulated figure came to rest
	bool					isLoaded;			// true when the articulated figure is properly loaded
	bool					isActive;			// true if the articulated figure physics is active
	bool					hasBindConstraints;	// true if the bind constraints have been added

protected:
	void					SetBase( anAFBody *body, const anJointMat *joints );
	void					AddBody( anAFBody *body, const anJointMat *joints, const char *jointName, const AFJointModType_t mod );

	bool					LoadBody( const anDeclAF_Body *fb, const anJointMat *joints );
	bool					LoadConstraint( const anDeclAF_Constraint *fc );

	bool					TestSolid( void ) const;
};

class anAFLink : public class anAF {
public:
	CLASS_PROTOTYPE( anAFLink );

	void					Spawn( void );

protected:
	void					BuildLink( const anStr &name, const anVec3 &origin, float linkLength, float linkWidth, float density, int numLinks, bool bindToWorld = true );
};

#endif // !__GAME_AF_H__