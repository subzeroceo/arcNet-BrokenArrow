/*
================

AI_Util.h

================
*/

#ifndef __AI_UTIL__
#define __AI_UTIL__

const float AI_TETHER_MINRANGE			= 8.0f;

/*
===============================================================================
								rvAITrigger
===============================================================================
*/

class rvAITrigger : public anEntity {
public:
	CLASS_PROTOTYPE ( rvAITrigger );

	rvAITrigger ( void );

	void			Spawn					( void );
	void			Save					( anSaveGame *savefile ) const;
	void			Restore					( anRestoreGame *savefile );
	virtual void	Think					( void );

	virtual void	FindTargets				( void );

protected:

	anList< anEntityPtr<anSAAI> >			testAI;
	anList< anEntityPtr<rvSpawner> >	testSpawner;

	bool								conditionDead;
	bool								conditionTether;
	bool								conditionStop;

	int									wait;
	int									nextTriggerTime;

	float								percent;

private:

	void			Event_Activate			( anEntity *activator );
	void			Event_PostRestore		( void );

	void			Event_AppendFromSpawner	( rvSpawner* spawner, anEntity *spawned );
};

/*
===============================================================================
								anSAAITether
===============================================================================
*/

class anSAAITether : public anEntity {
public:
	CLASS_PROTOTYPE ( anSAAITether );

	anSAAITether ( void );

	void			Spawn						( void );
	void			Save						( anSaveGame *savefile ) const;
	void			Restore						( anRestoreGame *savefile );
	void			InitNonPersistentSpawnArgs	( void );

	virtual bool	ValidateAAS				( anSAAI* ai );
	virtual bool	ValidateDestination		( anSAAI* ai, const anVec3 &dest );
	virtual bool	ValidateBounds			( const anBounds& bounds );

	virtual bool	FindGoal				( anSAAI* ai, seasGoal_t& goal );
	virtual float	GetOriginReachedRange	( void ) {return AI_TETHER_MINRANGE;}

	virtual void	DebugDraw				( void );

	bool			CanBreak				( void ) const;
	bool			IsAutoBreak				( void ) const;

	anList<int>		areaNum;

	bool			IsWalkForced			( void ) const;
	bool			IsRunForced				( void ) const;

protected:

	anEntityPtr<idLocationEntity>	location;

	struct tetherFlags_s {
		bool		canBreak			:1;			// Temporarily break when enemy is within tether
		bool		autoBreak			:1;			// Break when the ai gets within the tether
		bool		forceRun			:1;			// Alwasy run when heading towards tether
 		bool		forceWalk			:1;			// Alwasy walk when heading towards tether
		bool		becomeAggressive	:1;			//
		bool		becomePassive		:1;
	} tfl;

private:

	void			Event_Activate				( anEntity *activator );
	void			Event_TetherSetupLocation	( void );
	void			Event_TetherGetLocation		( void );
};

inline bool anSAAITether::CanBreak ( void ) const {
	return tfl.canBreak;
}

inline bool anSAAITether::IsWalkForced ( void ) const {
	return tfl.forceWalk;
}

inline bool anSAAITether::IsRunForced ( void ) const {
	return tfl.forceRun;
}

inline bool anSAAITether::IsAutoBreak ( void ) const {
	return tfl.autoBreak;
}

/*
===============================================================================
								anSAAITetherBehind
===============================================================================
*/

class anSAAITetherBehind : public anSAAITether {
public:
	CLASS_PROTOTYPE ( anSAAITetherBehind );

					anSAAITetherBehind( void ) { range = 0.0f; }

	void			Spawn						( void );
	void			Save						( anSaveGame *savefile ) const { }
	void			Restore						( anRestoreGame *savefile );
	void			InitNonPersistentSpawnArgs	( void );

	virtual bool	ValidateDestination			( anSAAI* ai, const anVec3 &dest );
	virtual bool	ValidateBounds				( const anBounds& bounds );
	virtual void	DebugDraw					( void );

protected:

	float	range;
};

/*
===============================================================================
								anSAAITetherRadius
===============================================================================
*/

class anSAAITetherRadius : public anSAAITether {
public:
	CLASS_PROTOTYPE ( anSAAITetherRadius );

					anSAAITetherRadius( void ) { radiusSqr = 0.0f; }

	void			Spawn						( void );
	void			Save						( anSaveGame *savefile ) const { }
	void			Restore						( anRestoreGame *savefile );
	void			InitNonPersistentSpawnArgs	( void );

	virtual bool	ValidateDestination			( anSAAI* ai, const anVec3 &dest );
	virtual bool	ValidateBounds				( const anBounds& bounds );
	virtual void	DebugDraw					( void );

	/*
	virtual float	GetOriginReachedRange		( void )
	{
		float rad = sqrt(radiusSqr);
		float halfRad = rad/2.0f;
		if ( rad < AI_TETHER_MINRANGE )
		{
			return rad;
		}
		return (halfRad<AI_TETHER_MINRANGE)?AI_TETHER_MINRANGE:halfRad;
	}
	*/

protected:

	float	radiusSqr;
};

/*
===============================================================================
								anSAAITetherRadius
===============================================================================
*/

class anSAAITetherClear : public anSAAITether {
public:
	CLASS_PROTOTYPE ( anSAAITetherClear );
};

/*
===============================================================================
								rvAIBecomePassive
===============================================================================
*/

class rvAIBecomePassive : public anEntity {
public:
	CLASS_PROTOTYPE ( rvAIBecomePassive );

private:

	void			Event_Activate			( anEntity *activator );
};

/*
===============================================================================
								rvAIBecomeAggressive
===============================================================================
*/

class rvAIBecomeAggressive : public anEntity {
public:
	CLASS_PROTOTYPE ( rvAIBecomeAggressive );

private:

	void			Event_Activate			( anEntity *activator );
};

#endif // __AI_UTIL__

