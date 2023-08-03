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
	void			SaveEntityPosition( arcEntity *ent );
	bool			RotateEntityToAxial( arcEntity *ent, anVec3 rotationPoint );
#ifdef NEW_PUSH
	bool			CanEntityFloat( arcEntity *ent, arcEntity *pusher, arcEntity *initialPusher, const int flags );
	void			AddEntityToBouyGroup( arcEntity *ent, float fraction, bool waterContact );
	bool			IsFullySubmerged( arcEntity *ent );

};

#endif // __WATER_H__
