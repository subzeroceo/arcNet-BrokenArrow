#ifndef __RV_SPLINE_MOVER_H
#define __RV_SPLINE_MOVER_H

extern const anEventDef EV_SetSpline;

struct splinePState_t {
	anVec3					origin;
	anVec3					localOrigin;
	anMat3					axis;
	anMat3					localAxis;

	float					speed;
	float					idealSpeed;
	float					dist;

	float					acceleration;
	float					deceleration;

	bool					ShouldAccelerate() const;
	bool					ShouldDecelerate() const;
	
	void					ApplyAccelerationDelta( float timeStepSec );

	void					ApplyDecelerationDelta( float timeStepSec );

	void					UpdateDist( float timeStepSec );
	void					Clear();
	void					WriteToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const anBitMsgDelta &msg );
	void					Save( anSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	splinePState_t&			Assign( const splinePState_t* state );
	splinePState_t&			operator=( const splinePState_t& state );
	splinePState_t&			operator=( const splinePState_t* state );
};

//=======================================================
//
//	rvPhysics_Spline
//
//=======================================================
class rvPhysics_Spline : public idPhysics_Base {

public:

	CLASS_PROTOTYPE( rvPhysics_Spline );

							rvPhysics_Spline( void );
	virtual					~rvPhysics_Spline( void );

	void					Save( anSaveGame *savefile ) const;
	void					Event_PostRestore( void );
	void					Restore( idRestoreGame *savefile );

	void					SetSpline( anCurve_Spline<anVec3>* spline );
	const anCurve_Spline<anVec3>* GetSpline() const { return spline; }
	anCurve_Spline<anVec3>* GetSpline() { return spline; }

	void					SetSplineEntity( anSplinePath* spline );
	const anSplinePath*		GetSplineEntity() const { return splineEntity; }
	anSplinePath*			GetSplineEntity() { return splineEntity; }

	void					SetLinearAcceleration( const float accel );
	void					SetLinearDeceleration( const float decel );

	void					SetSpeed( float speed );
	float					GetSpeed( void ) const;

	virtual bool			StartingToMove( void ) const;
	virtual bool			StoppedMoving( void ) const;

	float					ComputeDecelFromSpline( void ) const;

public:	// common physics interface
	void					SetClipModel( anClipModel *model, float density, int id = 0, bool freeOld = true );
	anClipModel *			GetClipModel( int id = 0 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const anBounds &		GetBounds( int id = -1 ) const;
	const anBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	bool					EvaluateSpline( anVec3& newOrigin, anMat3& newAxis, const splinePState_t& previous );
	bool					EvaluateMaster( anVec3& newOrigin, anMat3& newAxis, const splinePState_t& previous );

	void					Activate( void );
	void					Rest( void );

	bool					IsAtRest( void ) const;
	bool					IsAtEndOfSpline( void ) const;
	bool					IsAtBeginningOfSpline( void ) const;

	virtual bool			IsPushable( void ) const;

	bool					HasValidSpline( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const anVec3 &newOrigin, int id = -1 );
	void					SetAxis( const anMat3 &newAxis, int id = -1 );

	void					Translate( const anVec3 &translation, int id = -1 );
	void					Rotate( const anRotation &rotation, int id = -1 );

	const anVec3 &			GetOrigin( int id = 0 ) const;
	const anMat3 &			GetAxis( int id = 0 ) const;

	anVec3 &				GetOrigin( int id = 0 );
	anMat3 &				GetAxis( int id = 0 );

	void					SetMaster( anEntity *master, const bool orientated );

	void					ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const;
	void					ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const;
	int						ClipContents( const anClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	void					WriteToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const anBitMsgDelta &msg );

	virtual const trace_t*	GetBlockingInfo( void ) const;
	virtual anEntity*		GetBlockingEntity( void ) const;

public:
	stateResult_t			State_Accelerating( const stateParms_t& parms );
	stateResult_t			State_Decelerating( const stateParms_t& parms );
	stateResult_t			State_Cruising( const stateParms_t& parms );

protected:
	const anVec3 &			GetLocalOrigin( int id = 0 ) const;
	const anMat3 &			GetLocalAxis( int id = 0 ) const;

	anVec3 &				GetLocalOrigin( int id = 0 );
	anMat3 &				GetLocalAxis( int id = 0 );

protected:
	splinePState_t			current;
	splinePState_t			saved;

	float					splineLength;
	anCurve_Spline<anVec3>*	spline;
	anEntityPtr<anSplinePath> splineEntity;

	trace_t					pushResults;

	anClipModel*			clipModel;

	anStateThread			accelDecelStateThread;

	CLASS_STATES_PROTOTYPE( rvPhysics_Spline );
};

extern const anEventDef EV_DoneMoving;

//=======================================================
//
//	anPhysics_SplineMover
//
//=======================================================
class anPhysics_SplineMover : public anAnimatedEntity {
	CLASS_PROTOTYPE( anPhysics_SplineMover );

public:
	void					Spawn();
	virtual					~anPhysics_SplineMover();

	virtual void			SetSpeed( float newSpeed );
	virtual float			GetSpeed() const;
	virtual void			SetIdealSpeed( float newIdealSpeed );
	virtual float			GetIdealSpeed() const;

	virtual void			SetSpline( anSplinePath* spline );
	virtual const anSplinePath*	GetSpline() const;
	virtual anSplinePath*	GetSpline();

	virtual void			SetAcceleration( float accel );
	virtual void			SetDeceleration( float decel );

	virtual void			CheckSplineForOverrides( const anCurve_Spline<anVec3>* spline, const anDict* args );
	virtual void			RestoreFromOverrides( const anDict* args );

	int						PlayAnim( int channel, const char* animName, int blendFrames );
	void					CycleAnim( int channel, const char* animName, int blendFrames );
	void					ClearChannel( int channel, int clearFrame );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					WriteToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const anBitMsgDelta &msg );

protected:// TramCar utility functions
	void					AddSelfToGlobalList();
	void					RemoveSelfFromGlobalList();
	bool					InGlobalList() const;
	bool					WhosVisible( const anFrustum& frustum, anList<anPhysics_SplineMover*>& list ) const;

protected:
	virtual anString			GetTrackInfo( const anSplinePath* track ) const;
	anPhysics_SplineMover*			ConvertToMover( anEntity* mover ) const;
	anSplinePath*			ConvertToSplinePath( anEntity* spline ) const;

	void					CallScriptEvents( const anSplinePath* spline, const char* prefixKey, anEntity* parm );
	virtual	void			PreBind();

	virtual void			PreDoneMoving();
	virtual void			PostDoneMoving();

protected:
	void					Event_PostSpawn();

	void					Event_SetSpline( anEntity* spline );
	void					Event_GetSpline();

	void					Event_SetAcceleration( float accel );
	void					Event_SetDeceleration( float decel );

	void					Event_OnAcceleration();
	void					Event_OnDeceleration();
	void					Event_OnCruising();

	void					Event_OnStopMoving();
	void					Event_OnStartMoving();

	void					Event_SetSpeed( float speed );
	void					Event_GetSpeed();
	void					Event_SetIdealSpeed( float speed );
	void					Event_GetIdealSpeed();
	void					Event_ApplySpeedScale( float scale );

	void					Event_SetCallBack();
	void					Event_DoneMoving();

	void					Event_GetCurrentTrackInfo();
	void					Event_GetTrackInfo( anEntity* track );

	void					Event_Activate( anEntity* activator );

	void					Event_StartSoundPeriodic( const char* sndKey, const s_channelType channel, int minDelay, int maxDelay );
	void					Event_PartBlocked( anEntity *blockingEntity );

protected:
	rvPhysics_Spline		physicsObj;

	float					idealSpeed;

	int						waitThreadId;

private:
	anLinkList<anPhysics_SplineMover>	splineMoverNode;
	static anLinkList<anPhysics_SplineMover>	splineMovers;
};

//=======================================================
//
//	anTramCar
//
//=======================================================
class anTramCar : public anPhysics_SplineMover {
	CLASS_PROTOTYPE( anTramCar );

public:
	void				Spawn();
	virtual				~anTramCar();

	virtual void		Think();
	virtual void		MidThink();

	virtual float		GetNormalSpeed() const { return spawnArgs.GetFloat( "normalSpeed" ); }

	virtual void		SetIdealTrack( int track );
	virtual int			GetIdealTrack() const;

	virtual void		AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity* inflictor );
	virtual void		Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );

	virtual float		GetDamageScale() const;
	virtual float		GetHealthScale() const;

	void				Save( anSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				WriteToSnapshot( anBitMsgDelta &msg ) const;
	void				ReadFromSnapshot( const anBitMsgDelta &msg );

public:
	stateResult_t		State_Idle( const stateParms_t& parms );
	stateResult_t		State_NormalSpeed( const stateParms_t& parms );
	stateResult_t		State_ExcessiveSpeed( const stateParms_t& parms );

	stateResult_t		State_RandomTrack( const stateParms_t& parms );
	stateResult_t		State_AssignedTrack( const stateParms_t& parms );

protected:
	void				SpawnDriver( const char* driverKey );
	void				SpawnWeapons( const char* partKey );
	void				SpawnOccupants( const char* partKey );
	anEntity*			SpawnPart( const char* partDefName, const char* subPartDefName );
	void				SpawnDoors();
	anMover*			SpawnDoor( const char* key );

	int					SortSplineTargets( anSplinePath* spline ) const;
	anEntity*			GetSplineTarget( anSplinePath* spline, int index ) const;

	void				HeadTowardsIdealTrack();
	anSplinePath*		FindSplineToTrack( anSplinePath* spline, const anString& track ) const;
	anSplinePath*		FindSplineToIdealTrack( anSplinePath* spline ) const;
	anSplinePath*		GetRandomSpline( anSplinePath* spline ) const;

	char				ConvertToTrackLetter( int trackNum ) const { return trackNum + 'A'; }
	int					ConvertToTrackNumber( char trackLetter ) const { return trackLetter - 'A'; }
	int					ConvertToTrackNumber( const anString& trackInfo ) const { return ConvertToTrackNumber(anString::ToUpper(trackInfo.Right( 1 )[0])); }
	int					GetCurrentTrack() const;

	virtual void		UpdateChannel( const s_channelType channel, const soundShaderParms_t& parms );
	virtual void		AttenuateTrackChannel( float attenuation );
	virtual void		AttenuateTramCarChannel( float attenuation );

	virtual void		RegisterStateThread( anStateThread& stateThread, const char* name );

	bool				LookForward( anList<anPhysics_SplineMover*>& list ) const;
	bool				LookLeft( anList<anPhysics_SplineMover*>& list ) const;
	bool				LookRight( anList<anPhysics_SplineMover*>& list ) const;

	bool				LookLeftForTrackChange( anList<anPhysics_SplineMover*>& list ) const;
	bool				LookRightForTrackChange( anList<anPhysics_SplineMover*>& list ) const;

	bool				Look( const anFrustum& fov, anList<anPhysics_SplineMover*>& list ) const;

	virtual void		LookAround();
	virtual bool		AdjustSpeed( anList<anPhysics_SplineMover*>& moverList );

	virtual bool		OnSameTrackAs( const anPhysics_SplineMover* tram ) const;
	virtual bool		SameIdealTrackAs( const anPhysics_SplineMover* tram ) const;

	virtual anEntity*	DriverSpeak( const char *speechDecl, bool random = false );
	virtual anEntity*	OccupantSpeak( const char *speechDecl, bool random = false );

	virtual void		PostDoneMoving();

	virtual void		DeployRamp();
	virtual void		RetractRamp();
	void				OperateRamp( const char* operation );
	void				OperateRamp( const char* operation, anMover* door );

protected:
	void				Event_DriverSpeak( const char* voKey );
	void				Event_GetDriver();
	void				Event_Activate( anEntity* activator );
	void				Event_RadiusDamage( const anVec3& origin, const char* damageDefName );
	void				Event_SetIdealTrack( const char* track );

	void				Event_OnStopMoving();
	void				Event_OnStartMoving();

	void				Event_OpenDoors();
	void				Event_CloseDoors();

	void				Event_SetHealth( float health );

protected:
	int					idealTrack;
	anString				idealTrackTag;// HACK

	mutable anFrustum	collisionFov;

	anStateThread		idealTrackStateThread;
	anStateThread		speedSoundEffectsStateThread;

	anEntityPtr<idAI>					driver;
	anList< anEntityPtr<idAI> >			occupants;
	anList< anEntityPtr<anVehicle> >	weapons;

	int									numTracksOnMap;

	anEntityPtr<anMover>				leftDoor;
	anEntityPtr<anMover>				rightDoor;

	CLASS_STATES_PROTOTYPE( anTramCar );
};

//=======================================================
//
//	anTramCar_Marine
//
//=======================================================
class anTramCar_Marine : public anTramCar {
	CLASS_PROTOTYPE( anTramCar_Marine );

public:
	void				Spawn();

	virtual void		MidThink();

	void				Save( anSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

public:
	stateResult_t		State_Occupied( const stateParms_t& parms );
	stateResult_t		State_NotOccupied( const stateParms_t& parms );

	stateResult_t		State_UsingMountedGun( const stateParms_t& parms );
	stateResult_t		State_NotUsingMountedGun( const stateParms_t& parms );

protected:
	virtual void		Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );

	virtual void		LookAround();

	bool				LookOverLeftShoulder( anList<anPhysics_SplineMover*>& list );
	bool				LookOverRightShoulder( anList<anPhysics_SplineMover*>& list );

	bool				EntityIsInside( const anEntity* entity ) const;
	bool				PlayerIsInside() const { return EntityIsInside(gameLocal.GetLocalPlayer()); }

	void				ActivateTramHud( anBasePlayer* player );
	void				DeactivateTramHud( anBasePlayer* player );
	void				UpdateTramHud( anBasePlayer* player );

	virtual void		DeployRamp();
	virtual void		RetractRamp();

	void				UseMountedGun( anBasePlayer* player );

	void				Event_UseMountedGun( anEntity* ent );
	void				Event_SetPlayerDamageEntity(float f);	

protected:
	anList< anEntityPtr<anPhysics_SplineMover> > visibleEnemies;

	anStateThread		playerOccupationStateThread;
	anStateThread		playerUsingMountedGunStateThread;

	int						maxHealth;
	int						lastHeal;
	int						healDelay;
	int						healAmount;

	CLASS_STATES_PROTOTYPE( anTramCar_Marine );
};

//=======================================================
//
//	anTramCar_Strogg
//
//=======================================================
class anTramCar_Strogg : public anTramCar {
	CLASS_PROTOTYPE( anTramCar_Strogg );

public:
	void				Spawn();

	virtual void		SetTarget( anEntity* newTarget );
	virtual const anPhysics_SplineMover*	GetTarget() const;

	void				Save( anSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				WriteToSnapshot( anBitMsgDelta &msg ) const;
	void				ReadFromSnapshot( const anBitMsgDelta &msg );

public:
	stateResult_t		State_LookingForTarget( const stateParms_t& parms );
	stateResult_t		State_TargetInSight( const stateParms_t& parms );

protected:	
	bool				TargetIsToLeft();
	bool				TargetIsToRight();

	virtual void		LookAround();

protected:
	void				Event_PostSpawn();

protected:
	anEntityPtr<anPhysics_SplineMover> target;

	anStateThread		targetSearchStateThread;

	CLASS_STATES_PROTOTYPE( anTramCar_Strogg );
};

#endif
