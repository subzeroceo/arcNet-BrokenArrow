/*
================

Spawner.h

================
*/

#ifndef __GAME_SPAWNER_H__
#define __GAME_SPAWNER_H__

const int MAX_SPAWN_TYPES	= 32;

class rvSpawner;

typedef void (*spawnerCallbackProc_t) ( rvSpawner* spawner, anEntity *spawned, int userdata );

typedef struct {
	anEntityPtr<anEntity>	ent;
	anStr					event;
} spawnerCallback_t;

/*
===============================================================================

  rvSpawner

===============================================================================
*/
class rvSpawner : public anEntity {
public:
	CLASS_PROTOTYPE( rvSpawner );

	void				Spawn					( void );
	void				Think					( void );

	void				Attach					( anEntity *ent );
	void				Detach					( anEntity *ent );

	void				Save					( anSaveGame *savefile ) const;
	void				Restore					( anRestoreGame *savefile );

	void				AddSpawnPoint			( anEntity *point );
	void				RemoveSpawnPoint		( anEntity *point );

	int					GetNumSpawnPoints		( void ) const;
	int					GetNumActive			( void ) const;
	int					GetMaxActive			( void ) const;
	anEntity*			GetSpawnPoint			( int index );

	virtual void		FindTargets				( void );
	bool				ActiveListChanged		( void );

	void				CallScriptEvents		( const char *prefixKey, anEntity *parm );

	void				AddCallback				( anEntity *owner, const anEventDef* ev );

protected:

	int								numSpawned;
	int								maxToSpawn;
	float							nextSpawnTime;
	int								maxActive;
	anList< anEntityPtr<anEntity> >	currentActive;
	int								spawnWaves;
	int								spawnDelay;
	bool							skipVisible;
	anStringList						spawnTypes;

	anList< anEntityPtr<anEntity> >	spawnPoints;

	anList< spawnerCallback_t >		callbacks;

	// Check to see if its time to spawn
	void				CheckSpawn				( void );

	// Spawn a new entity
	bool				SpawnEnt				( void );

	// Populate the spawnType list with the available spawn types
	void				FindSpawnTypes			( void );

	// Get a random spawnpoint to spawn at
	anEntity*			GetSpawnPoint			( void );

	// Get a random spawn type
	const char*			GetSpawnType			( anEntity *spawnPoint );

	// Validate the given spawn point for spawning
	bool				ValidateSpawnPoint		( const anVec3 origin, const anBounds &bounds );

	// Copy key/values from the given entity to the given dictionary using the specified prefix
	void				CopyPrefixedSpawnArgs	( anEntity *src, const char *prefix, anDict &args );

private:

	void				Event_Activate			( anEntity *activator );
	void				Event_RemoveNullActiveEntities( void );
	void				Event_NumActiveEntities	( void );
	void				Event_GetActiveEntity	( int index );
};


inline int rvSpawner::GetNumSpawnPoints( void ) const {
	return spawnPoints.Num();
}

inline anEntity *rvSpawner::GetSpawnPoint( int index ) {
	return spawnPoints[index];
}

inline int rvSpawner::GetNumActive( void ) const {
	return currentActive.Num();
}

inline int rvSpawner::GetMaxActive( void ) const {
	return maxActive;
}

#endif // __GAME_SPAWNER_H__
