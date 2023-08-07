#ifndef __PHYSICS_LINEAR_H__
#define __PHYSICS_LINEAR_H__

#include "Physics_Base.h"

typedef struct linearPState_s {
	int										time;					// physics time
	int										atRest;					// set when simulation is suspended
	anVec3									origin;					// world origin
	anVec3									localOrigin;			// local origin
	anExtrapolate<anVec3>					linearExtrapolation;	// extrapolation based description of the position over time
} linearPState_t;

class anPhysics_Linear : public idPhysics_Base {
public:
	CLASS_PROTOTYPE( anPhysics_Linear );

							anPhysics_Linear( void );
							~anPhysics_Linear( void );

	void					SetPusher( int flags );
	bool					IsPusher( void ) const;

	void					SetLinearExtrapolation( extrapolation_t type, int time, int duration, const anVec3 &base, const anVec3 &speed, const anVec3 &baseSpeed );
	extrapolation_t			GetLinearExtrapolationType( void ) const { return current.linearExtrapolation.GetExtrapolationType(); }

	void					GetLocalOrigin( anVec3 &curOrigin ) const;

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

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					Activate( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const anVec3 &newOrigin, int id = -1 );
	void					SetAxis( const anMat3 &newAxis, int id = -1 );

	void					Translate( const anVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const anVec3 &			GetOrigin( int id = 0 ) const;
	const anMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const anVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const anVec3 &newAngularVelocity, int id = 0 );

	const anVec3 &			GetLinearVelocity( int id = 0 ) const;
	const anVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					UnlinkClip( void );
	void					LinkClip( void );
	void					DisableClip( bool activateContacting = true );
	void					EnableClip( void );

	void					SetMaster( anEntity *master, const bool orientated = true );

	const trace_t *			GetBlockingInfo( void ) const;
	anEntity *				GetBlockingEntity( void ) const;

	int						GetLinearEndTime( void ) const;

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

private:
	// parametric physics state
	linearPState_t			current;
	linearPState_t			saved;

	anMat3					axis;					// world axis

	// pusher
	bool					isPusher;
	anClipModel *			clipModel;
	int						pushFlags;

	// results of last evaluate
	trace_t					pushResults;
	bool					isBlocked;

	// master
	bool					hasMaster;
	bool					isOrientated;

private:
	bool					TestIfAtRest( void ) const;
	void					Rest( void );
};

#endif /* __PHYSICS_LINEAR_H__ */
