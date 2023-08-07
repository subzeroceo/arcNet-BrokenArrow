///////////////////////////////////////////////////////////////////////////////
// SEAS_tactical
//
// This file is the interface to an SEAS Tactical Extractor, which can search
// out from a given start point and report a variety of tactically important
// objectives, including corners, walls, and pinch points.
//
// By seeing the SEAS graph as a qualitative and simplified spatial
// representation of the game world.  This representation is amply capapble of
// rendering higher level tactical data efficiently in real time.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __SEAS_TACTICAL_H__
#define __SEAS_TACTICAL_H__

///////////////////////////////////////////////////////////////////////////////
// anSEASTacticalSensor
//
// The sensor structure is the public interface to the internals of AAS
// tactical features.
struct anSEASTacticalSensor {
	// Regular Update Function
	virtual void			Update() = 0;
	virtual void			Save( anSaveGame *savefile ) = 0;
	virtual void			Restore( anRestoreGame *savefile ) = 0;
	virtual void			Clear() = 0;

	// Search
	virtual void			SearchRadius(const anVec3 &origin=vec3_origin, float rangeMin=0.0f, float rangeMax=1.0f) = 0;
	virtual void			SearchCover(float rangeMin=0.0f, float rangeMax=1.0f) = 0;
	virtual void			SearchHide( anEntity *from=0 ) = 0;
	virtual void			SearchFlank() = 0;
	virtual void			SearchAdvance() = 0;
	virtual void			SearchRetreat() = 0;
	virtual void			SearchAmbush() = 0;
	virtual void			SearchDebug() = 0;

	// Feature Testing
	virtual bool			TestValid( seasFeature_t *f, float walkDistanceToFeature) = 0;
	virtual bool			TestValidWithCurrentState( seasFeature_t *f=0 ) = 0;

	// Feature Reservation
	virtual void			Reserve( seasFeature_t *f ) = 0;

	// Access To Results
	virtual int				FeatureCount() = 0;
	virtual seasFeature_t *	Feature( int i ) = 0;
	virtual seasFeature_t *	Near() const = 0;
	virtual seasFeature_t *	Look() const = 0;
	virtual seasFeature_t *	Reserved() const = 0;
	virtual const anVec3&	ReservedOrigin() const = 0;



	// STATIC SYSTEM FUNCTIONS
	static anSEASTacticalSensor *		CREATE_SENSOR(anActor *owner);
};

#endif // !__SEAS_TACTICAL_H__
