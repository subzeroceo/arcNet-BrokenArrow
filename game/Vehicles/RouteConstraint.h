#ifndef __GAME_VEHICLES_ROUTECONSTRAINT_H__
#define __GAME_VEHICLES_ROUTECONSTRAINT_H__

//#include "../misc/RenderEntityBundle.h"

class anRouteConstraintMarker;
class anRouteConstraintTracker;

class anRoutePoint {
public:
	static const int										MAX_NODES = 8;
	typedef anStaticList<const anRoutePoint*, MAX_NODES>	nodeList_t;
	typedef anStaticList<sdRenderEntityBundle, anRoutePoint::MAX_NODES> renderList_t;

	void							Init( anRouteConstraintMarker *marker );
	void							Clear( void );
	void							AddParent( anRoutePoint *point );
	void							AddChild( anRoutePoint *point );
	void							SetAngles( const anAngles& value ) { angles = value; }
	// RoutePoint *
	static void						Link( anparent, anRoutePoint *child );
	static anRoutePoint*			Allocate( void ) { anRoutePoint *point = s_allocator.Alloc(); point->Clear(); return point; }
	static void						Free( anRoutePoint *point ) { s_allocator.Free( point ); }

	const anStrList&				GetTargetNames( void ) const { return targetNames; }
	const char *					GetName( void ) const { return name.c_str(); }
	const anVec3 &					GetOrigin( void ) const { return origin; }
	const anAngles &				GetAngles( void ) const { return angles; }
	const nodeList_t &				GetChildren( void ) const { return children; }
	const nodeList_t &				GetParents( void ) const { return parents; }
	bool							GetAllowAirDrop( void ) const { return allowAirDrop; }

	int								OnChain( const anRoutePoint *chainEnd ) const;
	const anRoutePoint*				FindCommonParent( const anRoutePoint *other ) const;
	const anRoutePoint*				FindCommonChild( const anRoutePoint *other ) const;

private:
	const anRoutePoint*				FindCommonParent( const anRoutePoint *other, const anRoutePoint *node ) const;
	const anRoutePoint*				FindCommonChild( const anRoutePoint *other, const anRoutePoint *node ) const;

private:
	anVec3							origin;
	anAngles						angles;
	nodeList_t						parents;
	nodeList_t						children;
	anStr							name;
	anStrList						targetNames;
	bool							allowAirDrop;
	qhandle_t						maskHandle;

	static anBlockAlloc<anRoutePoint, 32>			s_allocator;
};

class anRouteConstraintController : public anEntity {
	CLASS_PROTOTYPE( anRouteConstraintController );
public:
									anRouteConstraintController( void );
									~anRouteConstraintController( void );

	void							Spawn( void );
	void							Display( anRouteConstraintTracker *tracker );
	void							SetRenderPoint( anRouteConstraintTracker *tracker, anRoutePoint *newPoint );
	void							Update( anRouteConstraintTracker *tracker );
	void							AddPoint( anRouteConstraintMarker *marker );

	const anRoutePoint*				GetStartPoint( void ) const { return startPoint; }
	const anRoutePoint*				GetEndPoint( void ) const { return endPoint; }

private:
	void							Event_Link( void );

	void							CheckForLoops( anList<const anRoutePoint *> &checkPoints, const anRoutePoint *point );
	anRoutePoint*					FindPoint( const char *name );
	anRoutePoint*					FindPoint( const anVec3 &origin );

private:
	anList<anRoutePoint *>			points;
	anRoutePoint *					startPoint;
	anRoutePoint *					endPoint;
	bool							linked;
	qhandle_t						maskHandle;
	int								warningPointDeviance;
	int								maxPointDeviance;
	idRenderModel *					directionalModel;
	const idDeclSkin *				directionalModelSkin;
	anVec3							directionalModelColor;
};

class anRouteConstraintMarker : public anEntity {
	CLASS_PROTOTYPE( anRouteConstraintMarker );
public:
	virtual void						PostMapSpawn( void );

private:
};

class anRouteConstraintTracker {
public:
												anRouteConstraintTracker( void );
	void										Init( anEntity *_entity );
	void										SetTrackerEntity( anEntity *entity );
	
	bool										IsValid( void ) const { return controller.IsValid(); }
	void										Display( void );
	void										Hide( void );
	void										Update( void );
	void										Reset( void ) { bestPoint = currentPoint; positionWarning = false; maskWarning = false; kickPlayer = false; }

	anEntity*									GetEntity( void ) const { return entity; }
	const anRoutePoint*							GetCurrentPoint( void ) const { return currentPoint; }
	const anRoutePoint*							GetBestPoint( void ) const { return bestPoint; }
	const anRoutePoint*							GetBestReserve( void ) const { return bestPointReserve; }
	const anRoutePoint*							GetRenderPoint( void ) const { return renderPoint; }
	bool										GetPositionWarning( void ) const { return positionWarning; }
	bool										GetMaskWarning( void ) const { return maskWarning; }
	bool										GetKickPlayer( void ) const { return kickPlayer; }
	float										GetKickDistance( void ) const { return kickDistance; }
	anRoutePoint::renderList_t&					GetRenderList( void ) { return markers; }
	anEntity*									GetTrackerEntity( void ) const { return controller; }

	void										SetCurrentPoint( const anRoutePoint *point ) { currentPoint = point; }
	void										SetBestPoint( const anRoutePoint *point ) { SetReserve( bestPoint ); bestPoint = point; }
	void										SetRenderPoint( const anRoutePoint *point ) { renderPoint = point; }

	void										SetPositionWarning( bool value ) { positionWarning = value; }
	void										SetMaskWarning( bool value ) { maskWarning = value; }
	void										SetKickPlayer( bool value ) { kickPlayer = value; }
	void										SetKickDistance( int value ) { kickDistance = value; }

	void										GetDropLocation( anVec3 &position, anAngles& angles ) const;

private:
	void										SetReserve( const anRoutePoint *point );

	const anRoutePoint *						currentPoint;
	const anRoutePoint *						bestPoint;
	const anRoutePoint *						bestPointReserve;
	const anRoutePoint *						renderPoint;
	anEntity *									entity;
	anEntityPtr<anRouteConstraintController>	controller;
	bool										positionWarning;
	bool										maskWarning;
	bool										kickPlayer;
	int											kickDistance;
	anRoutePoint::renderList_t					markers;
};

#endif // __GAME_VEHICLES_ROUTECONSTRAINT_H__
