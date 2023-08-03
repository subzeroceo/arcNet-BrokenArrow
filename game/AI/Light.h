#ifndef __GAME_LIGHT_H__
#define __GAME_LIGHT_H__

/*
===============================================================================

  Generic light.

===============================================================================
*/

extern const anEventDef EV_Light_GetLightParm;
extern const anEventDef EV_Light_SetLightParm;
extern const anEventDef EV_Light_SetLightParms;

class idLight : public anEntity {
public:
	CLASS_PROTOTYPE( idLight );

					idLight();
					~idLight();

	void			Spawn( void );

	void			Save( anSaveGame *savefile ) const;					// archives object for save game file
	void			Restore( anRestoreGame *savefile );					// unarchives object from save game file

	virtual void	UpdateChangeableSpawnArgs( const anDict *source );
	virtual void	Think( void );
	virtual void	FreeLightDef( void );
	virtual bool	GetPhysicsToSoundTransform( anVec3 &origin, anMat3 &axis );
	void			Present( void );

	void			SaveState( anDict *args );
	virtual void	SetColor( float red, float green, float blue );
	virtual void	SetColor( const anVec4 &color );
	virtual void	GetColor( anVec3 &out ) const;
	virtual void	GetColor( anVec4 &out ) const;
	const anVec3 &	GetBaseColor( void ) const { return baseColor; }
	void			SetShader( const char *shadername );
	void			SetLightParm( int parmnum, float value );
	void			SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			SetRadiusXYZ( float x, float y, float z );
	void			SetRadius( float radius );
	void			On( void );
	void			Off( void );
	void			Fade( const anVec4 &to, float fadeTime );
	void			FadeOut( float time );
	void			FadeIn( float time );
	void			Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );
	void			BecomeBroken( anEntity *activator );
	qhandle_t		GetLightDefHandle( void ) const { return lightDefHandle; }
	void			SetLightParent( anEntity *lparent ) { lightParent = lparent; }
	void			SetLightLevel( void );


// jshepard: other entities ( speakers) need access to the refSound of a light object
	void			SetRefSound( int rSound ) { refSound.referenceSoundHandle = rSound;}
// ddynerman: sometimes the game needs to know if this light is ambient
	bool			IsAmbient( void ) { return renderLight.shader->IsAmbientLight(); }

	virtual void	ShowEditingDialog( void );

	enum {
		EVENT_BECOMEBROKEN = anEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual void	ClientPredictionThink( void );
	virtual void	WriteToSnapshot( anBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const anBitMsgDelta &msg );
	virtual bool	ClientReceiveEvent( int event, int time, const anBitMsg &msg );

private:
	renderLight_t	renderLight;				// light presented to the renderer
	anVec3			localLightOrigin;			// light origin relative to the physics origin
	anMat3			localLightAxis;				// light axis relative to physics axis
	qhandle_t		lightDefHandle;				// handle to renderer light def
	anString			brokenModel;
	int				levels;
	int				currentLevel;
	anVec3			baseColor;
	bool			breakOnTrigger;
	int				count;
	int				triggercount;
	anEntity *		lightParent;
	anVec4			fadeFrom;
	anVec4			fadeTo;
	int				fadeStart;
	int				fadeEnd;

// bdube: light gui
	anEntityPtr<anEntity>	lightGUI;
// abahr:
	float			wait;
	float			random;


private:
	bool			soundWasPlaying;

	void			PresentLightDefChange( void );
	void			PresentModelDefChange( void );


// jscott: added events for light level
private:
	void			Event_SetCurrentLightLevel ( int in );
	void			Event_SetMaxLightLevel ( int in );
	void			Event_IsOn( void );
	void			Event_Break( anEntity *activator, float turnOff );
	void			Event_DoneBlinking( void );
	void			Event_DoneBlinkingOff( void );
	void			Event_EarthQuake( float requiresLOS );
	void			Event_Timer( void );


private:
	void			Event_SetShader( const char *shadername );
	void			Event_GetLightParm( int parmnum );
	void			Event_SetLightParm( int parmnum, float value );
	void			Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			Event_SetRadiusXYZ( float x, float y, float z );
	void			Event_SetRadius( float radius );
	void			Event_Hide( void );
	void			Event_Show( void );
	void			Event_On( void );
	void			Event_Off( void );
	void			Event_ToggleOnOff( anEntity *activator );
	void			Event_SetSoundHandles( void );
	void			Event_FadeOut( float time );
	void			Event_FadeIn( float time );

// bdube: set light gui
	void			Event_SetLightGUI( const char* gui );

};


// bdube: externed events
extern const anEventDef EV_Light_SetCurrentLightLevel;
extern const anEventDef EV_Light_SetMaxLightLevel;
extern const anEventDef EV_Light_SetRadius;


#endif /* !__GAME_LIGHT_H__ */