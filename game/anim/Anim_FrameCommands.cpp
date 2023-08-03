// Copyright (C) 2007 Id Software, Inc.
//

#include "../Lib.h"
#pragma hdrstop

#include "Anim_FrameCommands.h"
#include "../demos/DemoScript.h"
#include "../demos/DemoManager.h"
#include "../Player.h"
#include "../Weapon.h"

/*
===============================================================================

	sdAnimFrameCommand

===============================================================================
*/

anCVar g_debugFrameCommands( "g_debugFrameCommands", "0", CVAR_GAME | CVAR_BOOL | CVAR_CHEAT, "Prints out frame commands as they are called" );
anCVar g_debugFrameCommandsFilter( "g_debugFrameCommandsFilter", "", CVAR_GAME  | CVAR_CHEAT, "Filter the type of framecommands" );

sdAnimFrameCommand::factory_t sdAnimFrameCommand::frameCommandFactory;

#define FRAME_COMMAND_ALLOC_TYPE( type ) frameCommandFactory.RegisterType( type::TypeName(), factory_t::Allocator< type > )

/*
============
sdAnimFrameCommand::Init
============
*/
void sdAnimFrameCommand::Init( void ) {
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_ScriptFunction );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_ScriptObjectFunction );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_Event );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_Sound );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_Fade );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_Skin );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_Effect );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_FootStep );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_DemoScript );
	FRAME_COMMAND_ALLOC_TYPE( sdAnimFrameCommand_WeaponState );
}

/*
============
sdAnimFrameCommand::Shutdown
============
*/
void sdAnimFrameCommand::Shutdown( void ) {
	frameCommandFactory.Shutdown();
}

/*
============
sdAnimFrameCommand::Alloc
============
*/
sdAnimFrameCommand*	sdAnimFrameCommand::Alloc( const char* typeName ) {
	return frameCommandFactory.CreateType( typeName );
}

/*
===============================================================================

	sdAnimFrameCommand_ScriptFunction

===============================================================================
*/

/*
============
sdAnimFrameCommand_ScriptFunction::Init
============
*/
bool sdAnimFrameCommand_ScriptFunction::Init( anParser& src ) {
	anToken token;
	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_ScriptFunction::Init Unexpected end of line" );
		return false;
	}
	functionName = token;
	return true;
}

/*
============
sdAnimFrameCommand_ScriptFunction::Run
============
*/
void sdAnimFrameCommand_ScriptFunction::Run( anClass* ent ) const {
	if ( gameLocal.program == nullptr ) {
		return;
	}
	gameLocal.CallFrameCommand( gameLocal.program->FindFunction( functionName ) );
}



/*
===============================================================================

	sdAnimFrameCommand_ScriptObjectFunction

===============================================================================
*/

/*
============
sdAnimFrameCommand_ScriptObjectFunction::Init
============
*/
bool sdAnimFrameCommand_ScriptObjectFunction::Init( anParser& src ) {
	anToken token;
	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_ScriptObjectFunction::Init Unexpected end of line" );
		return false;
	}
	functionName = token;
	return true;
}

/*
============
sdAnimFrameCommand_ScriptObjectFunction::Run
============
*/
void sdAnimFrameCommand_ScriptObjectFunction::Run( anClass* ent ) const {
	if ( gameLocal.program == nullptr ) {
		return;
	}
	gameLocal.CallObjectFrameCommand( ent->GetScriptObject(), functionName, false );
}


/*
===============================================================================

	sdAnimFrameCommand_Event

===============================================================================
*/

/*
============
sdAnimFrameCommand_Event::Init
============
*/
bool sdAnimFrameCommand_Event::Init( anParser& src ) {
	anToken token;
	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Event::Init Unexpected end of line" );
		return false;
	}

	ev = arcEventDef::FindEvent( token );
	if ( !ev ) {
		src.Error( "sdAnimFrameCommand_Event::Init Event '%s' not found", token.c_str() );
		return false;
	}
	if ( ev->GetNumArgs() != 0 ) {
		src.Error( "sdAnimFrameCommand_Event::Init Event '%s' has arguments", token.c_str() );
		return false;
	}
	return true;
}

/*
============
sdAnimFrameCommand_Event::Run
============
*/
void sdAnimFrameCommand_Event::Run( anClass* ent ) const {
	if ( !ent ) {
		gameLocal.Warning( "sdAnimFrameCommand_Event::Run nullptr entity" );
		return;
	}
	ent->ProcessEvent( ev );
}


/*
===============================================================================

	sdAnimFrameCommand_Sound

===============================================================================
*/

/*
============
sdAnimFrameCommand_Sound::Init
============
*/
bool sdAnimFrameCommand_Sound::Init( anParser& src ) {
	const sdDeclStringMap* map = gameLocal.declStringMapType[ "soundChannelMap" ];
	if ( !map ) {
		gameLocal.Error( "sdAnimFrameCommand_Sound::Init stringMap 'soundChannelMap' not found" );
		return false;
	}

	anToken token;
	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Sound::Init Unexpected end of line" );
		return false;
	}

	int c;
	if ( !map->GetDict().GetInt( token, "-1", c ) ) {
		src.Error( "sdAnimFrameCommand_Sound::Init Channel '%s' not found", token.c_str() );
		return false;
	}

	soundChannel = static_cast< soundChannel_t >( c );

	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Sound::Init Unexpected end of line" );
		return false;
	}

	soundName = token;;
	return true;
}

/*
============
sdAnimFrameCommand_Sound::Run
============
*/
void sdAnimFrameCommand_Sound::Run( anClass* ent ) const {
	arcEntity* entity = ent->Cast< arcEntity >();
	if ( entity ) {
		entity->StartSound( soundName, soundChannel, 0, nullptr );
	} else {
		gameLocal.Warning( "sdAnimFrameCommand_Sound::Run Not Currently Supported on Client Entities" );
	}
}


/*
===============================================================================

	sdAnimFrameCommand_Fade

===============================================================================
*/

/*
============
sdAnimFrameCommand_Fade::Init
============
*/
bool sdAnimFrameCommand_Fade::Init( anParser& src ) {
	const sdDeclStringMap* map = gameLocal.declStringMapType[ "soundChannelMap" ];
	if ( !map ) {
		gameLocal.Error( "sdAnimFrameCommand_Fade::Init stringMap 'soundChannelMap' not found" );
		return false;
	}

	anToken token;
	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Fade::Init Unexpected end of line" );
		return false;
	}

	int c;
	if ( !map->GetDict().GetInt( token, "-1", c ) ) {
		src.Error( "sdAnimFrameCommand_Fade::Init Channel '%s' not found", token.c_str() );
		return false;
	}

	soundChannel = static_cast< soundChannel_t >( c );

	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Fade::Init Unexpected end of line" );
		return false;
	}

	to = atof( token.c_str() );

	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Fade::Init Unexpected end of line" );
		return false;
	}

	over = token.GetFloatValue();

	return true;
}

/*
============
sdAnimFrameCommand_Fade::Run
============
*/
void sdAnimFrameCommand_Fade::Run( anClass* ent ) const {
	arcEntity* entity = ent->Cast< arcEntity >();
	if ( entity ) {
		entity->FadeSound( soundChannel, to, over );
	} else {
		gameLocal.Warning( "sdAnimFrameCommand_Sound::Run Not Currently Supported on Client Entities" );
	}
}

/*
===============================================================================

	sdAnimFrameCommand_Skin

===============================================================================
*/

/*
============
sdAnimFrameCommand_Skin::Init
============
*/
bool sdAnimFrameCommand_Skin::Init( anParser& src ) {
	anToken token;
	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Skin::Init Unexpected end of line" );
		return false;
	}

	if ( !token.Icmp( "none" ) ) {
		skin = nullptr;
	} else {
		skin = gameLocal.declSkinType[ token ];
		if ( !skin ) {
			src.Error( "sdAnimFrameCommand_Skin::Init Skin '%s' not found", token.c_str() );
			return false;
		}
	}

	return true;
}

/*
============
sdAnimFrameCommand_Skin::Run
============
*/
void sdAnimFrameCommand_Skin::Run( anClass* ent ) const {
	if ( !ent ) {
		gameLocal.Warning( "sdAnimFrameCommand_Skin::Run: nullptr entity" );
		return;
	}

	ent->SetSkin( skin );
}

/*
===============================================================================

	sdAnimFrameCommand_Effect

===============================================================================
*/

/*
============
sdAnimFrameCommand_Effect::Init
============
*/
bool sdAnimFrameCommand_Effect::Init( anParser& src ) {
	anToken token;
	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Effect::Init Unexpected end of line" );
		return false;
	}

	effectName = token;

	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_Effect::Init Unexpected end of line" );
		return false;
	}

	jointName = token;

	return true;
}

/*
============
sdAnimFrameCommand_Effect::Run
============
*/
void sdAnimFrameCommand_Effect::Run( anClass* ent ) const {
	if ( !ent ) {
		gameLocal.Warning( "sdAnimFrameCommand_Effect::Run: nullptr entity" );
		return;
	}

	arcEntity *entity = ent->Cast< arcEntity >();
	if ( entity ) {
		jointHandle_t handle = entity->GetAnimator()->GetJointHandle( jointName );
		if ( handle == INVALID_JOINT ) {
			gameLocal.Warning( "sdAnimFrameCommand_Effect::Run Invalid Joint %s", jointName.c_str() );
			return;
		}
		entity->PlayEffect( effectName, colorWhite.ToVec3(), nullptr, handle );
		return;
	}

	rvClientEntity* clientEnt = ent->Cast< rvClientEntity >();
	if ( clientEnt ) {
		arcAnimator* clientAnimator = clientEnt->GetAnimator();
		clientEnt->PlayEffect( effectName, colorWhite.ToVec3(), nullptr, clientAnimator ? clientAnimator->GetJointHandle( jointName ) : INVALID_JOINT );
		return;
	}
}

/*
===============================================================================

	sdAnimFrameCommand_FootStep

===============================================================================
*/

/*
============
sdAnimFrameCommand_FootStep::Init
============
*/
bool sdAnimFrameCommand_FootStep::Init( anParser& src ) {
	anToken token;
	rightFoot = false;
	if ( src.ReadTokenOnLine( &token ) ) {
		bool valid = true;
		if ( !token.Icmp( "_right" ) ) {
			rightFoot = true;
			if ( !src.ReadTokenOnLine( &token ) ) {
				valid = false;
			}
		}
		if ( valid ) {
			prefix = token;
		}
	}
	return true;
}

/*
============
sdAnimFrameCommand_FootStep::Run
============
*/
void sdAnimFrameCommand_FootStep::Run( anClass* ent ) const {
	arcNetBasePlayer* player = ent->Cast< arcNetBasePlayer >();
	if ( player == nullptr ) {
		gameLocal.Warning( "sdAnimFrameCommand_FootStep::Run: Invalid Entity" );
		return;
	}

	player->PlayFootStep( prefix, rightFoot );
}

/*
===============================================================================

	sdAnimFrameCommand_DemoScript

===============================================================================
*/

/*
============
sdAnimFrameCommand_DemoScript::Init
============
*/
bool sdAnimFrameCommand_DemoScript::Init( anParser& src ) {
	src.ParseRestOfLine( command );
	return true;
}

/*
============
sdAnimFrameCommand_DemoScript::Run
============
*/
void sdAnimFrameCommand_DemoScript::Run( anClass* ent ) const {
	sdDemoScript* demoScript = sdDemoManager::GetInstance().GetScript();
	if ( !demoScript ) {
		return;
	}

	anParser src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.LoadMemory( command, command.Length(), "sdAnimFrameCommand_DemoScript::Run" );

	anToken token;
	if ( src.ExpectAnyToken( &token ) ) {
		sdDemoScript::sdEvent* event = sdDemoScript::CreateEvent( token.c_str() );

		if ( event ) {
			if ( event->Parse( src ) ) {
				event->Run( *demoScript );
			}
			delete event;
		} else {
			gameLocal.Warning( "Framecommand 'demoScriptEvent' on entity '%s' : Unsupported event type '%s'", ent->GetName(), token.c_str() );
		}
	} else {
		gameLocal.Warning( "Framecommand 'demoScriptEvent' on entity '%s' : Failed to parse initial script token ('%s')", ent->GetName(), command.c_str() );
	}
}


/*
============
sdAnimFrameCommand_WeaponState::Init
============
*/
bool sdAnimFrameCommand_WeaponState::Init( anParser& src ) {
	anToken token;
	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Error( "sdAnimFrameCommand_WeaponState::Init Unexpected end of line" );
		return false;
	}

	command = token;

	return true;
}

/*
============
sdAnimFrameCommand_WeaponState::Run
============
*/
void sdAnimFrameCommand_WeaponState::Run( anClass* ent ) const {
	arcNetBasePlayer* player = ent->Cast< arcNetBasePlayer >();
	if ( !player ) {
		gameLocal.Warning( "FrameCommand 'weaponState' on entity '%s' is only supported for players.", ent->GetName() );
		return;
	}

	if ( !command.Icmp( "hide" ) ) {
		if ( idWeapon* weapon = player->GetWeapon()) {
			weapon->HideWorldModel();
		}
	} else if ( !command.Icmp( "show" ) ) {
		if ( idWeapon* weapon = player->GetWeapon()) {
			weapon->ShowWorldModel();
			player->GetInventory().HideCurrentItem( true );
		}
	} else {
		gameLocal.Warning( "FrameCommand 'weaponState' on entity '%s': unknown command '%s'.", ent->GetName(), command.c_str() );
	}

	if ( g_debugFrameCommands.GetBool() ) {
		if ( !anString::Length( g_debugFrameCommandsFilter.GetString()) || anString::FindText( GetTypeName(), g_debugFrameCommandsFilter.GetString(), false ) != anString::INVALID_POSITION )  {
			gameLocal.Printf( "Command '%s': state '%s'\n", GetTypeName(), command.c_str() );
		}
	}
}
