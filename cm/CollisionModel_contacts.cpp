/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#include "/idlib/Lib.h"
#pragma hdrstop

#include "CollisionModel_local.h"

/*
===============================================================================

Retrieving contacts

===============================================================================
*/

/*
==================
arcCollisionModelManagerLocal::Contacts
==================
*/
int arcCollisionModelManagerLocal::Contacts( contactInfo_t *contacts, const int maxContacts, const anVec3 &start, const anVec6 &dir, const float depth,
								const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
								cmHandle_t model, const anVec3 &origin, const anMat3 &modelAxis ) {
	trace_t results;
	anVec3 end;

	// same as Translation but instead of storing the first collision we store all collisions as contacts
	arcCollisionModelManagerLocal::getContacts = true;
	arcCollisionModelManagerLocal::contacts = contacts;
	arcCollisionModelManagerLocal::maxContacts = maxContacts;
	arcCollisionModelManagerLocal::numContacts = 0;
	end = start + dir.SubVec3(0) * depth;
	arcCollisionModelManagerLocal::Translation( &results, start, end, trm, trmAxis, contentMask, model, origin, modelAxis );
	if ( dir.SubVec3(1).LengthSqr() != 0.0f ) {
		// FIXME: rotational contacts
	}
	arcCollisionModelManagerLocal::getContacts = false;
	arcCollisionModelManagerLocal::maxContacts = 0;

	return arcCollisionModelManagerLocal::numContacts;
}
