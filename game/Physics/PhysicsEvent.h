#ifndef __PHYSICS_EVENT_H__
#define __PHYSICS_EVENT_H__

class arcPhysicsEvent {
public:
	typedef arcLinkList< arcPhysicsEvent > nodeType_t;

								arcPhysicsEvent( nodeType_t& list );
	virtual						~arcPhysicsEvent( void ) { ; }

	int							GetCreationTime( void ) const { return _creationTime; }

	const nodeType_t &		GetNode( void ) const { return _node; }

	virtual void				Apply( void ) const = 0;

private:
	int							_creationTime;
	nodeType_t					_node;
};

class arcPhysicsEvent_RadiusPush : public arcPhysicsEvent {
public:
								arcPhysicsEvent_RadiusPush( nodeType_t &list, const arcVec3 &origin, float radius, const arcDeclDamage *damageDecl, float push, const arcEntity *inflictor, const arcEntity *ignore, int flags );

	void						Apply( void ) const;

private:
	arcVec3						_origin;
	float						_radius;
	float						_push;
	arcEntityPtr< arcEntity >		_inflictor;
	arcEntityPtr< arcEntity >		_ignore;
	int							_flags;
	//const arcDeclDamage*			_damageDecl;
};

#endif // __PHYSICS_EVENT_H__
