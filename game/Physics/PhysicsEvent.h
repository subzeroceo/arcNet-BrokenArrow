#ifndef __PHYSICS_EVENT_H__
#define __PHYSICS_EVENT_H__

class anPhysicsEvent {
public:
	typedef arcLinkList< anPhysicsEvent > nodeType_t;

								anPhysicsEvent( nodeType_t& list );
	virtual						~anPhysicsEvent( void ) { ; }

	int							GetCreationTime( void ) const { return _creationTime; }

	const nodeType_t &		GetNode( void ) const { return _node; }

	virtual void				Apply( void ) const = 0;

private:
	int							_creationTime;
	nodeType_t					_node;
};

class anPhysicsEvent_RadiusPush : public anPhysicsEvent {
public:
								anPhysicsEvent_RadiusPush( nodeType_t &list, const anVec3 &origin, float radius, const arcDeclDamage *damageDecl, float push, const arcEntity *inflictor, const arcEntity *ignore, int flags );

	void						Apply( void ) const;

private:
	anVec3						_origin;
	float						_radius;
	float						_push;
	arcEntityPtr< arcEntity >		_inflictor;
	arcEntityPtr< arcEntity >		_ignore;
	int							_flags;
	//const arcDeclDamage*			_damageDecl;
};

#endif // __PHYSICS_EVENT_H__
