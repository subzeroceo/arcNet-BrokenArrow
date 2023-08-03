
#ifndef __GAME_TRIGGER_H__
#define __GAME_TRIGGER_H__

extern const anEventDef EV_Enable;
extern const anEventDef EV_Disable;

/*
===============================================================================

  Trigger base.

===============================================================================
*/

class idTrigger : public anEntity {
public:
	CLASS_PROTOTYPE( idTrigger );

	static void			DrawDebugInfo( void );

						idTrigger();
	void				Spawn( void );

	const function_t *	GetScriptFunction( void ) const;

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	virtual void		Enable( void );
	virtual void		Disable( void );

protected:

// abahr: removed const from function
	void				CallScript( anEntity* scriptEntity );


	void				Event_Enable( void );
	void				Event_Disable( void );


// abahr: changed to allow parms to be passed
	anList<rvScriptFuncUtility> scriptFunctions;
	//const function_t *	scriptFunction;

};


/*
===============================================================================

  Trigger which can be activated multiple times.

===============================================================================
*/

class idTrigger_Multi : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Multi );

						idTrigger_Multi( void );

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );
	virtual void		Think( void );

private:
	float				wait;
	float				random;
	float				delay;
	float				random_delay;
	int					nextTriggerTime;
	anString				requires;
	int					removeItem;
	bool				touchClient;
	bool				touchOther;
	bool				touchVehicle;
	bool				triggerFirst;
	bool				triggerWithSelf;
	int					buyZoneTrigger;
	int					controlZoneTrigger;
	int					prevZoneController;

	anList<anBasePlayer*>	playersInTrigger;

	bool				CheckFacing( anEntity *activator );
	void				HandleControlZoneTrigger();


// kfuller: want trigger_relays entities to be able to respond to earthquakes
	void				Event_EarthQuake				(float requiresLOS);


	void				TriggerAction( anEntity *activator );
	void				Event_TriggerAction( anEntity *activator );
	void				Event_Trigger( anEntity *activator );
	void				Event_Touch( anEntity *other, trace_t *trace );
};


/*
===============================================================================

  Trigger which can only be activated by an entity with a specific name.

===============================================================================
*/

class idTrigger_EntityName : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_EntityName );

						idTrigger_EntityName( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Spawn( void );

private:
	float				wait;
	float				random;
	float				delay;
	float				random_delay;
	int					nextTriggerTime;
	bool				triggerFirst;
	anString				entityName;

	void				TriggerAction( anEntity *activator );
	void				Event_TriggerAction( anEntity *activator );
	void				Event_Trigger( anEntity *activator );
	void				Event_Touch( anEntity *other, trace_t *trace );
};

/*
===============================================================================

  Trigger which repeatedly fires targets.

===============================================================================
*/

class idTrigger_Timer : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Timer );

						idTrigger_Timer( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Spawn( void );

	virtual void		Enable( void );
	virtual void		Disable( void );

private:
	float				random;
	float				wait;
	bool				on;
	float				delay;
	anString				onName;
	anString				offName;

	void				Event_Timer( void );
	void				Event_Use( anEntity *activator );
};


/*
===============================================================================

  Trigger which fires targets after being activated a specific number of times.

===============================================================================
*/

class idTrigger_Count : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Count );

						idTrigger_Count( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Spawn( void );

private:
	int					goal;
	int					count;
	float				delay;

	void				Event_Trigger( anEntity *activator );
	void				Event_TriggerAction( anEntity *activator );
};


/*
===============================================================================

  Trigger which hurts touching entities.

===============================================================================
*/

class idTrigger_Hurt : public idTrigger {
public:
	CLASS_PROTOTYPE( idTrigger_Hurt );

						idTrigger_Hurt( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Spawn( void );

private:
	bool				on;
	float				delay;
	int					nextTime;


// kfuller: added playeronly
	bool				playerOnly;


	void				Event_Touch( anEntity *other, trace_t *trace );
	void				Event_Toggle( anEntity *activator );
};


/*
===============================================================================

  Trigger which fades the player view.

===============================================================================
*/

class idTrigger_Fade : public idTrigger {
public:

	CLASS_PROTOTYPE( idTrigger_Fade );

private:
	void				Event_Trigger( anEntity *activator );
};


/*
===============================================================================

  Trigger which continuously tests whether other entities are touching it.

===============================================================================
*/

class idTrigger_Touch : public idTrigger {
public:

	CLASS_PROTOTYPE( idTrigger_Touch );

						idTrigger_Touch( void );
						~idTrigger_Touch( );

	void				Spawn( void );
	virtual void		Think( void );

	void				Save( anSaveGame *savefile );
	void				Restore( anRestoreGame *savefile );

	virtual void		Enable( void );
	virtual void		Disable( void );

	void				TouchEntities( void );

private:
	anClipModel *		clipModel;
	int					filterTeam;

	void				Event_Trigger( anEntity *activator );
};

#endif /* !__GAME_TRIGGER_H__ */
