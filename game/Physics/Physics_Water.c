#include "../../idlib/Lib.h"
#pragma hdrstop
#include "../Game_local.h"


/*
===============================================================================

	Liquid this is so small added it with the physics to keep organized.

===============================================================================
*/


/*
================
anLiquid::anLiquid
================
*/
anLiquid::anLiquid( void ) {
}

/*
================
anLiquid::Spawn
================
*/
void anLiquid::Spawn( void ) {
	current = spawnArgs.GetVector( "current" );
}
