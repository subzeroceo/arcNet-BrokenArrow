/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#include "../idlib/Lib.h"
#pragma hdrstop

#include "CollisionModel_local.h"

/*
===============================================================================

Retrieving contacts

===============================================================================
*/

/*
==================
anSoftBodiesPhysicsManager::Contacts
==================
*/
int anSoftBodiesPhysicsManager::Contacts( contactInfo_t *contacts, const int maxContacts, const anVec3 &start, const anVec6 &dir, const float depth,
								const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
								cmHandle_t model, const anVec3 &origin, const anMat3 &modelAxis ) {
	trace_t results;
	anVec3 end;

	// same as Translation but instead of storing the first collision we store all collisions as contacts
	anSoftBodiesPhysicsManager::getContacts = true;
	anSoftBodiesPhysicsManager::contacts = contacts;
	anSoftBodiesPhysicsManager::maxContacts = maxContacts;
	anSoftBodiesPhysicsManager::numContacts = 0;
	end = start + dir.SubVec3(0) * depth;
	anSoftBodiesPhysicsManager::Translation( &results, start, end, trm, trmAxis, contentMask, model, origin, modelAxis );
	if ( dir.SubVec3( 1 ).LengthSqr() != 0.0f ) {
		// FIXME: rotational contacts
	}
	anSoftBodiesPhysicsManager::getContacts = false;
	anSoftBodiesPhysicsManager::maxContacts = 0;

	return anSoftBodiesPhysicsManager::numContacts;
}
