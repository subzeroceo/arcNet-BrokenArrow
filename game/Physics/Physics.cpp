
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

ABSTRACT_DECLARATION( anClass, anPhysics )
END_CLASS


/*
================
anPhysics::~anPhysics
================
*/
anPhysics::~anPhysics( void ) {
}

/*
================
anPhysics::Save
================
*/
void anPhysics::Save( anSaveGame *savefile ) const {
}

/*
================
anPhysics::Restore
================
*/
void anPhysics::Restore( anRestoreGame *savefile ) {
}

/*
================
anPhysics::SetClipBox
================
*/
void anPhysics::SetClipBox( const anBounds &bounds, float density ) {
	SetClipModel( new anClipModel( anTraceModel( bounds ) ), density );
}

/*
================
anPhysics::SnapTimeToPhysicsFrame
================
*/
int anPhysics::SnapTimeToPhysicsFrame( int t ) {
	int s;

// bdube: use GetMSec access rather than USERCMD_TIME
	s = t + gameLocal.GetMSec() - 1;
	return ( s - s % gameLocal.GetMSec() );

}
