
// Copyright (C) 2007 Id Software, Inc.
//
#ifndef __ANIM_FRAMECOMMANDS_H__
#define __ANIM_FRAMECOMMANDS_H__

extern anCVar g_debugFrameCommands;
extern anCVar g_debugFrameCommandsFilter;


class sdAnimFrameCommand {
private:
	typedef sdFactory< sdAnimFrameCommand > factory_t;

public:
								sdAnimFrameCommand( void ) : refCount( 1 ) { ; }
	virtual						~sdAnimFrameCommand( void ) { ; }

	static void					Init( void );
	static void					Shutdown( void );
	static sdAnimFrameCommand*	Alloc( const char *typeName );

	virtual const char*			GetTypeName( void ) const = 0;
	virtual bool				Init( anParser& src ) = 0;
	virtual void				Run( anClass* ent ) const = 0;

	void						IncRef( void ) { refCount++; }
	void						DecRef( void ) { refCount--; if ( refCount == 0 ) { delete this; } }

private:
	sdAnimFrameCommand( const sdAnimFrameCommand& rhs );
	sdAnimFrameCommand& operator =( const sdAnimFrameCommand& rhs );

private:
	static factory_t			frameCommandFactory;

	int							refCount;
};

class sdAnimFrameCommand_ScriptFunction : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_ScriptFunction( void ) { ; }
								sdAnimFrameCommand_ScriptFunction( const anStr& _functionName ) : functionName( _functionName ) { ; }
								~sdAnimFrameCommand_ScriptFunction( void ) { ; }

	static const char*			TypeName( void ) { return "call"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	anStr						functionName;
};

class sdAnimFrameCommand_ScriptObjectFunction : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_ScriptObjectFunction( void ) { ; }
								sdAnimFrameCommand_ScriptObjectFunction( const anStr& _functionName ) : functionName( _functionName ) { ; }
								~sdAnimFrameCommand_ScriptObjectFunction( void ) { ; }

	static const char*			TypeName( void ) { return "object_call"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	anStr						functionName;
};

class sdAnimFrameCommand_Event : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_Event( void ) : ev( nullptr ) { ; }
								sdAnimFrameCommand_Event( const anEventDef* _ev ) : ev( _ev ) { ; }
								~sdAnimFrameCommand_Event( void ) { ; }

	static const char*			TypeName( void ) { return "event"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	const anEventDef*			ev;
};


class sdAnimFrameCommand_Sound : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_Sound( void ) { ; }
								sdAnimFrameCommand_Sound( const anStr& _soundName, soundChannel_t _soundChannel ) : soundName( _soundName ), soundChannel( _soundChannel ) { ; }
								~sdAnimFrameCommand_Sound( void ) { ; }

	static const char*			TypeName( void ) { return "sound_channel"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	anStr						soundName;
	soundChannel_t				soundChannel;
};

class sdAnimFrameCommand_Fade : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_Fade( void ) { ; }
								sdAnimFrameCommand_Fade( float _to, float _over, soundChannel_t _soundChannel ) : to( _to ), over( _over ), soundChannel( _soundChannel ) { ; }
								~sdAnimFrameCommand_Fade( void ) { ; }

	static const char*			TypeName( void ) { return "fade_channel"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	float						to;
	float						over;
	soundChannel_t				soundChannel;
};



class sdAnimFrameCommand_Skin : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_Skin( void ) : skin( nullptr ) { ; }
								sdAnimFrameCommand_Skin( const anDeclSkin* _skin ) : skin( _skin ){ ; }
								~sdAnimFrameCommand_Skin( void ) { ; }

	static const char*			TypeName( void ) { return "skin"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	const anDeclSkin*			skin;
};

class sdAnimFrameCommand_Effect : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_Effect( void ) { ; }
								sdAnimFrameCommand_Effect( const anStr& _effectName, const anStr& _jointName ) : effectName( _effectName ), jointName( _jointName ) { ; }
								~sdAnimFrameCommand_Effect( void ) { ; }

	static const char*			TypeName( void ) { return "effect"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	anStr						effectName;
	anStr						jointName;
};

class sdAnimFrameCommand_FootStep : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_FootStep( void ) { ; }
								~sdAnimFrameCommand_FootStep( void ) { ; }

	static const char*			TypeName( void ) { return "footstep"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

	bool						rightFoot;
	anStr						prefix;
};

class sdAnimFrameCommand_DemoScript : public sdAnimFrameCommand {
public:
								sdAnimFrameCommand_DemoScript( void ) { ; }
								sdAnimFrameCommand_DemoScript( const anStr& _command ) : command( _command ) { ; }
								~sdAnimFrameCommand_DemoScript( void ) { ; }

	static const char*			TypeName( void ) { return "demoScriptEvent"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	anStr						command;
};

class sdAnimFrameCommand_WeaponState : public sdAnimFrameCommand {
public:
	sdAnimFrameCommand_WeaponState( void ) { ; }
	sdAnimFrameCommand_WeaponState( const anStr& _command ) : command( _command ) { ; }
	~sdAnimFrameCommand_WeaponState( void ) { ; }

	static const char*			TypeName( void ) { return "weaponState"; }
	virtual const char*			GetTypeName( void ) const { return TypeName(); }

	virtual bool				Init( anParser& src );
	virtual void				Run( anClass* ent ) const;

private:
	anStr						command;
};

#endif // __ANIM_FRAMECOMMANDS_H__
