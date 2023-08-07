
#ifndef __GAME_BRITTLEFRACTURE_H__
#define __GAME_BRITTLEFRACTURE_H__


/*
===============================================================================

B-rep Brittle Fracture - Static entity using the boundary representation
of the render model which can fracture.

===============================================================================
*/

typedef struct shard_s {
	anClipModel *				clipModel;
	anFixedWinding				winding;
	anList<anFixedWinding *>	decals;
	anList<bool>				edgeHasNeighbour;
	anList<struct shard_s *>	neighbours;
	anPhysics_RigidBody			physicsObj;
	int							droppedTime;
	bool						atEdge;
	int							islandNum;
} shard_t;


class idBrittleFracture : public anEntity {

public:
	CLASS_PROTOTYPE( idBrittleFracture );

								idBrittleFracture( void );
	virtual						~idBrittleFracture( void );

	void						Save( anSaveGame *savefile ) const;
	void						Restore( anRestoreGame *savefile );

	void						Spawn( void );

	virtual void				Present( void );
	virtual void				Think( void );
	virtual void				ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse, bool splash = false );
	virtual void				AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force );
	virtual void				AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity *inflictor );
	virtual void				Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );

	void						ProjectDecal( const anVec3 &point, const anVec3 &dir, const int time, const char *damageDefName );
	bool						IsBroken( void ) const;

	enum {
		EVENT_PROJECT_DECAL = anEntity::EVENT_MAXEVENTS,
		EVENT_SHATTER,
		EVENT_MAXEVENTS
	};

	virtual void				ClientPredictionThink( void );
	virtual bool				ClientReceiveEvent( int event, int time, const anBitMsg &msg );

private:
	// setttings
	const anMaterial *			material;
	const anMaterial *			decalMaterial;
	float						decalSize;
	float						maxShardArea;
	float						maxShatterRadius;
	float						minShatterRadius;
	float						linearVelocityScale;
	float						angularVelocityScale;
	float						shardMass;
	float						density;
	float						friction;
	float						bouncyness;
	anStr						fxFracture;

	// state
	anPhysics_StaticMulti		physicsObj;
	anList<shard_t *>			shards;
	anBounds					bounds;
	bool						disableFracture;

	// for rendering
	mutable int					lastRenderEntityUpdate;
	mutable bool				changed;

	bool						UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const;
	static bool					ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );

	void						AddShard( anClipModel *clipModel, anFixedWinding &w );
	void						RemoveShard( int index );
	void						DropShard( shard_t *shard, const anVec3 &point, const anVec3 &dir, const float impulse, const int time );
	void						Shatter( const anVec3 &point, const anVec3 &impulse, const int time );
	void						DropFloatingIslands( const anVec3 &point, const anVec3 &impulse, const int time );
	void						Break( void );
	void						Fracture_r( anFixedWinding &w );
	void						CreateFractures( const anRenderModel *renderModel );
	void						FindNeighbours( void );

	void						Event_Activate( anEntity *activator );
	void						Event_Touch( anEntity *other, trace_t *trace );
};

#endif /* !__GAME_BRITTLEFRACTURE_H__ */
