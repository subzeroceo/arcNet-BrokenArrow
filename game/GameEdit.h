// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_EDIT_H__
#define __GAME_EDIT_H__

#include "physics/Force_Drag.h"

/*
===============================================================================

	Ingame cursor.

===============================================================================
*/

class idCursor3D : public arcEntity {
public:
	CLASS_PROTOTYPE( idCursor3D );

							idCursor3D( void );
							~idCursor3D( void );

	void					Spawn( void );
	void					Present( void );
	void					Think( void );

	arcForce_Drag			drag;
	anVec3					draggedPosition;
};


/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

class idDragEntity {
public:
							idDragEntity( void );
							~idDragEntity( void );

	void					Clear();
	void					Update( arcNetBasePlayer *player );
	void					SetSelected( arcEntity *ent );
	arcEntity *				GetSelected( void ) const { return selected.GetEntity(); }
	void					DeleteSelected( void );
	void					BindSelected( void );
	void					UnbindSelected( void );

private:
	arcEntityPtr<arcEntity>	dragEnt;			// entity being dragged
	jointHandle_t			joint;				// joint being dragged
	int						id;					// id of body being dragged
	anVec3					localEntityPoint;	// dragged point in entity space
	anVec3					localPlayerPoint;	// dragged point in player space
	anString					bodyName;			// name of the body being dragged
	idCursor3D*				cursor;				// cursor entity
	arcEntityPtr<arcEntity>	selected;			// last dragged entity

	void					StopDrag( void );
};


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/
typedef struct selectedTypeInfo_s {
	idTypeInfo *typeInfo;
	anString		textKey;
} selectedTypeInfo_t;

class idEditEntities {
public:
							idEditEntities( void );
	bool					SelectEntity( const anVec3 &origin, const anVec3 &dir, const arcEntity *skip );
	void					AddSelectedEntity( arcEntity *ent );
	void					RemoveSelectedEntity( arcEntity *ent );
	void					ClearSelectedEntities( void );
	anList<arcEntity *>&		GetSelectedEntities( void );
	void					DisplayEntities( void );
	bool					EntityIsSelectable( arcEntity *ent, anVec4 *color = nullptr, anString *text = nullptr );
private:
	int						nextSelectTime;
	anList<selectedTypeInfo_t> selectableEntityClasses;
	anList<arcEntity *>		selectedEntities;
};

#endif /* !__GAME_EDIT_H__ */
