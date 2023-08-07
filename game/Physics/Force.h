
#ifndef __FORCE_H__
#define __FORCE_H__

/*
===============================================================================

	Force base class

	A force object applies a force to a physics object.

===============================================================================
*/
#include "Force.h"
class anEntity;
class anPhysics;
class anClass;

class anForce : public anClass; {

public:
	CLASS_PROTOTYPE( anForce );

						anForce( void );
	virtual				~anForce( void );
	static void			DeletePhysics( const anPhysics *phys );
	static void			ClearForceList( void );

public: // common force interface
						// evalulate the force up to the given time
	virtual void		Evaluate( int time );
						// removes any pointers to the physics object
	virtual void		RemovePhysics( const anPhysics *phys );

private:

	static anList<anForce *> forceList;
};


/*
===============================================================================

	Constant force

===============================================================================
*/

class anForce_Constant : public anForce {

public:
	CLASS_PROTOTYPE( anForce_Constant );

						anForce_Constant( void );
	virtual				~anForce_Constant( void );


	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

						// constant force
	void				SetForce( const anVec3 &force );
						// set force position
	void				SetPosition( anPhysics *physics, int id, const anVec3 &point );

	void				SetPhysics( anPhysics *physics );

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const anPhysics *phys );

private:
	// force properties
	anVec3				force;
	anPhysics *			physics;
	int					id;
	anVec3				point;
};

/*
===============================================================================

	Spring force

===============================================================================
*/

class anForce_Spring : public anForce {

public:
	CLASS_PROTOTYPE( anForce_Spring );

						anForce_Spring( void );
	virtual				~anForce_Spring( void );
						// initialize the spring
	void				InitSpring( float Kstretch, float Kcompress, float damping, float restLength );
						// set the entities and positions on these entities the spring is attached to
	void				SetPosition(	anPhysics *physics1, int id1, const anVec3 &p1,
										anPhysics *physics2, int id2, const anVec3 &p2 );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const anPhysics *phys );

private:

	// spring properties
	float				Kstretch;
	float				Kcompress;
	float				damping;
	float				restLength;

	// positioning
	anPhysics *			physics1;	// first physics object
	int					id1;		// clip model id of first physics object
	anVec3				p1;			// position on clip model
	anPhysics *			physics2;	// second physics object
	int					id2;		// clip model id of second physics object
	anVec3				p2;			// position on clip model

};

/*
===============================================================================

	Drag force

===============================================================================
*/

class anForce_Drag : public anForce {

public:
	CLASS_PROTOTYPE( anForce_Drag );

						anForce_Drag( void );
	virtual				~anForce_Drag( void );
						// initialize the drag force
	void				Init( float damping );
						// set physics object being dragged
	void				SetPhysics( anPhysics *physics, int id, const anVec3 &p );
						// set position to drag towards
	void				SetDragPosition( const anVec3 &pos );
						// get the position dragged towards
	const anVec3 &		GetDragPosition( void ) const;
						// get the position on the dragged physics object
	const anVec3		GetDraggedPosition( void ) const;

public: // common force interface
	virtual void		Evaluate( int time );
	virtual void		RemovePhysics( const anPhysics *phys );

private:

	// properties
	float				damping;

	// positioning
	anPhysics *			physics;		// physics object
	int					id;				// clip model id of physics object
	anVec3				p;				// position on clip model
	anVec3				dragPosition;	// drag towards this position
};

/*
===============================================================================

	Force field

===============================================================================
*/

enum forceFieldType {
	FORCEFIELD_UNIFORM,
	FORCEFIELD_EXPLOSION,
	FORCEFIELD_IMPLOSION
};

enum forceFieldApplyType {
	FORCEFIELD_APPLY_FORCE,
	FORCEFIELD_APPLY_VELOCITY,
	FORCEFIELD_APPLY_IMPULSE
};

class anForce_Field : public anForce {
public:
	CLASS_PROTOTYPE( anForce_Field );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

						anForce_Field( void );
	virtual				~anForce_Field( void );
						// uniform constant force
	void				Uniform( const anVec3 &force );
						// explosion from clip model origin
	void				Explosion( float force );
						// implosion towards clip model origin
	void				Implosion( float force );
						// add random torque
	void				RandomTorque( float force );
						// should the force field apply a force, velocity or impulse
	void				SetApplyType( const forceFieldApplyType type ) { applyType = type; }
						// make the force field only push players
	void				SetPlayerOnly( bool set ) { playerOnly = set; }
						// make the force field only push monsters
	void				SetMonsterOnly( bool set ) { monsterOnly = set; }
						// clip model describing the extents of the force field
	void				SetClipModel( anClipModel *clipModel );

	void				SetOwner( anEntity *ent );

	int					GetLastApplyTime( void ) const;


public: // common force interface
	virtual void		Evaluate( int time );

private:
	// force properties
	forceFieldType		type;
	forceFieldApplyType	applyType;
	float				magnitude;
	anVec3				dir;
	float				randomTorque;
	bool				playerOnly;
	bool				monsterOnly;
	anClipModel *		clipModel;

	int					lastApplyTime;
	anEntity*			owner;

};

inline int anForce_Field::GetLastApplyTime( void ) const {
	return lastApplyTime;
}

inline void anForce_Field::SetOwner( anEntity *ent ) {
	owner = ent;
}

#endif /* !__FORCE_H__ */
