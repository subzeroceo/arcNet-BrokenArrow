#ifndef __WATER_H__
#define __WATER_H__

/*
===============================================================================

  Allows physics and bouyounce calculations to be performed on the fly and affect gravitational pulls
  and few more calculations i need to remember and such but ill add into the description later.

===============================================================================
*/

class anPhysics_WaterBase {
public:

private:


private:
	void			SaveEntityPosition( anEntity *ent );
	bool			RotateEntityToAxial( anEntity *ent, anVec3 rotationPoint );
#ifdef NEW_PUSH
	bool			CanEntityFloat( anEntity *ent, anEntity *pusher, anEntity *initialPusher, const int flags );
	void			AddEntityToBouyGroup( anEntity *ent, float fraction, bool waterContact );
	bool			IsFullySubmerged( anEntity *ent );

};

/*
===============================================================================

	basic spawning function for liquids

===============================================================================
*/

class anLiquid : public anEntity {
public:
	CLASS_PROTOTYPE( anLiquid );

										anLiquid( void );
	void								Spawn( void );

	virtual void						GetWaterCurrent( anVec3 &waterCurrent ) const { waterCurrent = current; }

private:
	anVec3								current;
};

#endif // __WATER_H__
