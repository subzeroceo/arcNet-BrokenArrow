#ifndef __GAME_EDIT_H__
#define __GAME_EDIT_H__

#include "physics/Force_Drag.h"

/*
===============================================================================

	Ingame cursor.

===============================================================================
*/

class idCursor3D : public anEntity {
public:
	CLASS_PROTOTYPE( idCursor3D );

							idCursor3D( void );
							~idCursor3D( void );

	void					Spawn( void );
	void					Present( void );
	void					Think( void );

	anForce_Drag			drag;
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
	void					Update( anBasePlayer *player );
	void					SetSelected( anEntity *ent );
	anEntity *				GetSelected( void ) const { return selected.GetEntity(); }
	void					DeleteSelected( void );
	void					BindSelected( void );
	void					UnbindSelected( void );

private:
	anEntityPtr<anEntity>	dragEnt;			// entity being dragged
	jointHandle_t			joint;				// joint being dragged
	int						id;					// id of body being dragged
	anVec3					localEntityPoint;	// dragged point in entity space
	anVec3					localPlayerPoint;	// dragged point in player space
	anStr					bodyName;			// name of the body being dragged
	idCursor3D*				cursor;				// cursor entity
	anEntityPtr<anEntity>	selected;			// last dragged entity

	void					StopDrag( void );
};


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/
typedef struct selectedTypeInfo_s {
	idTypeInfo *typeInfo;
	anStr		textKey;
} selectedTypeInfo_t;

class idEditEntities {
public:
							idEditEntities( void );
	bool					SelectEntity( const anVec3 &origin, const anVec3 &dir, const anEntity *skip );
	void					AddSelectedEntity( anEntity *ent );
	void					RemoveSelectedEntity( anEntity *ent );
	void					ClearSelectedEntities( void );
	anList<anEntity *>&		GetSelectedEntities( void );
	void					DisplayEntities( void );
	bool					EntityIsSelectable( anEntity *ent, anVec4 *color = nullptr, anStr *text = nullptr );
private:
	int						nextSelectTime;
	anList<selectedTypeInfo_t> selectableEntityClasses;
	anList<anEntity *>		selectedEntities;
};

#endif // !__GAME_EDIT_H__
