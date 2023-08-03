/*
================

SEAS_Find.h

================
*/

#ifndef __SEAS_FIND__
#define __SEAS_FIND__

class anSAAI;
class anSAAIHelper;

/*
===============================================================================
							anSEAS Tactical Manager
						Dully Note that this is also apart of the
				- Spatially Aware AI System - and -
			- Spatially Aware Tactical AI System-

===============================================================================
*/

class anSEASTacticalManager : public anSEASCallback {
public:
	anSEASTacticalManager( const anVec3 &hideFromPos );
	~anSEASTacticalManager( void );

protected:

	virtual bool		TestArea( class anSEAS *aas, int areaNum, const seasArea_t& area );

private:

	pvsHandle_t			hidePVS;
	int					PVSAreas[ anEntity::MAX_PVS_AREAS ];
};

/*
===============================================================================
								anSEASFindAreaOutOfRange
===============================================================================
*/

class anAASTargetOutOfRange : public anSEASCallback {
public:

	anAASTargetOutOfRange( anSAAI *_owner );

protected:

	virtual bool		TestPoint( class anSEAS *aas, const anVec3& point, const float zAllow=0.0f );

private:

	anSAAI *			owner;
};

/*
===============================================================================
								anSEASFindAttackPosition
===============================================================================
*/

class anAASHolstileCoordnation : public anSEASCallback {
public:
	anAASHolstileCoordnation( anSAAI *self );
	~anAASHolstileCoordnation( void );


	bool				TestCachedGoals( int count, seasGoal_t& goal );

	virtual void		Init( void );
	virtual void		Finish( void );

private:

	virtual bool		TestArea( class anSEAS *aas, int areaNum, const seasArea_t& area );
	virtual bool		TestPoint( class anSEAS *aas, const anVec3& point, const float zAllow=0.0f );

	bool				TestCachedGoal( int index );

	anSAAI*				owner;

  	pvsHandle_t			targetPVS;
  	int					PVSAreas[ anEntity::MAX_PVS_AREAS ];

	anList<seasGoal_t>	cachedGoals;
	int					cachedIndex;
	int					cachedAreaNum;
};

/*
===============================================================================
							SEASTegtherObjective
===============================================================================
*/

class SEASTegtherObjective : public anSEASCallback {
public:
	SEASTegtherObjective( anSAAI* owner, anSAAITether* helper );
	~SEASTegtherObjective( void );

protected:

	virtual bool		TestArea( class anSEAS *aas, int areaNum, const seasArea_t& area );
	virtual bool		TestPoint( class anSEAS* aas, const anVec3& pos, const float zAllow=0.0f );

private:

	anSAAI *				owner;
	anSAAITether *		tether;
};

#endif // __SEAS_FIND__
