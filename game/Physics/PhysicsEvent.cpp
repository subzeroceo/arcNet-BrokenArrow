#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "PhysicsEvent.h"

/*
===============================================================================

	arcPhysicsEvent

===============================================================================
*/

/*
================
arcPhysicsEvent::arcPhysicsEvent
================
*/
arcPhysicsEvent::arcPhysicsEvent( nodeType_t &list ) : _creationTime( gameLocal.time ) {
	_node.SetOwner( this );
	_node.AddToEnd( list );
}

/*
===============================================================================

	arcPhysicsEvent_RadiusPush

===============================================================================
*/

/*
================
sdPhysicsEvent_RadiusPush::sdPhysicsEvent_RadiusPush
================
*/
arcPhysicsEvent_RadiusPush::arcPhysicsEvent_RadiusPush( nodeType_t &list, const arcVec3 &origin, float radius, const arcDeclDamage *damageDecl, float push, const arcEntity *inflictor, const arcEntity *ignore, int flags ) : arcPhysicsEvent( list ) {
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
arcPhysicsEvent_RadiusPush::Apply
================
*/
void arcPhysicsEvent_RadiusPush::Apply( void ) const {
	gameLocal.RadiusPush( _origin, _radius, _damageDecl, _push, _inflictor, _ignore, _flags, false );
}
