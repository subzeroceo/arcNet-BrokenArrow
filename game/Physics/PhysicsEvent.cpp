#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ARC_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PhysicsEvent.h"

/*
===============================================================================

	anPhysicsEvent

===============================================================================
*/

/*
================
anPhysicsEvent::anPhysicsEvent
================
*/
anPhysicsEvent::anPhysicsEvent( nodeType_t &list ) : _creationTime( gameLocal.time ) {
	_node.SetOwner( this );
	_node.AddToEnd( list );
}

/*
===============================================================================

	anPhysicsEvent_RadiusPush

===============================================================================
*/

/*
================
sdPhysicsEvent_RadiusPush::sdPhysicsEvent_RadiusPush
================
*/
anPhysicsEvent_RadiusPush::anPhysicsEvent_RadiusPush( nodeType_t &list, const anVec3 &origin, float radius, const anDeclDamage *damageDecl, float push, const anEntity *inflictor, const anEntity *ignore, int flags ) : anPhysicsEvent( list ) {
	_origin = origin;
	_radius = radius;
	_push = push;
	_inflictor = inflictor;
	_ignore = ignore;
	_flags = flags;
	_damageDecl = damageDecl;
}

/*
================
anPhysicsEvent_RadiusPush::Apply
================
*/
void anPhysicsEvent_RadiusPush::Apply( void ) const {
	gameLocal.RadiusPush( _origin, _radius, _damageDecl, _push, _inflictor, _ignore, _flags, false );
}
