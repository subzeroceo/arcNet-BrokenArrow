//----------------------------------------------------------------
// rvVehicleAI.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_VEHICLEAI_H__
#define __GAME_VEHICLEAI_H__

#ifndef __AI_H__
#include "AI.h"
#endif
#ifndef __GAME_VEHICLEMONSTER_H__
#include "../vehicle/VehicleMonster.h"
#endif

enum VehicleAI_Flags {
	VAIF_Chase		= 1,
	VAIF_Avoid		= 2,
	VAIF_Freeze		= 4,
};

class rvVehicleAI : public anSAAI {
	friend class rvVehicleMonster;

public:
	CLASS_PROTOTYPE( rvVehicleAI );

							rvVehicleAI				( void );
							~rvVehicleAI			( void );

	void					Spawn					( void );
	void					Think					( void );
	void					Save					( anSaveGame *savefile ) const;
	void					Restore					( anRestoreGame *savefile );

	void					SetVehicle				( rvVehicleMonster * vehicle );

	void					Random					( void );
	void					StraightToEnemy			( void );
	void					ChaseEnemy				( void );
	void					AvoidEnemy				( void );
	void					Stop					( void );
	void					Start					( void );

	int &					GetFlags				( void ) { return flags; }

	bool					IsDriving				( void ) { return driver && driver->IsDriving(); }
	rvVehicleDriver*		GetDriver				( void ) { return driver.GetEntity(); }
	const rvVehicleDriver*	GetDriver				( void ) const { return driver.GetEntity(); }

private:
	virtual void			OnWakeUp				( void );
	virtual void			CustomMove				( void );

	const anEntity *		MoveCloserTo			( const anVec3 & point, anEntity * current );
	const anEntity *		MoveAwayFrom			( const anVec3 & point, anEntity * current );
	const anEntity *		FindClosestNode			( void ) const;

	void					Event_ChoosePathTarget	( anEntity * current );

	anEntityPtr<rvVehicleDriver>	driver;
	int								flags;
};

ARC_INLINE void rvVehicleAI::Random ( void ) {
	flags = 0;
	CustomMove();
}

ARC_INLINE void rvVehicleAI::StraightToEnemy ( void ) {
	driver->ProcessEvent( &AI_ScriptedMove, enemy.ent.GetEntity(), 0.0f, 0 );
}

ARC_INLINE void rvVehicleAI::ChaseEnemy ( void ) {
	flags = VAIF_Chase;
	CustomMove();
}

ARC_INLINE void rvVehicleAI::AvoidEnemy ( void ) {
	flags = VAIF_Avoid;
	CustomMove();
}

ARC_INLINE void rvVehicleAI::Stop ( void ) {
	flags = ( flags & 0x03 ) | VAIF_Freeze;
	driver->ProcessEvent( &AI_ScriptedStop );
}

ARC_INLINE void rvVehicleAI::Start ( void ) {
	flags = ( flags & 0x03 ) & ~VAIF_Freeze;
	//driver->ProcessEvent( &EV_Activate, this );
	CustomMove();
}

#endif // __GAME_VEHICLEAI_H__
