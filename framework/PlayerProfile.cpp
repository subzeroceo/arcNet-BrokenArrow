#pragma hdrstop
#include "/idlib/precompiled.h"
#pragma hdrstop
#include "PlayerProfile.h"

// After releasing a version to the market, here are limitations for compatibility:
//	- the major version should not ever change
//	- always add new items to the bottom of the save/load routine
//	- never remove or change the size of items, just stop using them or add new items to the end
//
// The biggest reason these limitations exist is because if a newer profile is created and then loaded with a GMC
// version, we have to support it.
const int16		PROFILE_TAG					= ( 'D' << 8 ) | '3';
const int8		PROFILE_VER_MAJOR			= 10;	// If this is changed, you should reset the minor version and remove all backward compatible code
const int8		PROFILE_VER_MINOR			= 0;	// Within each major version, minor versions can be supported for backward compatibility

class arcNetBasePlayerProfileLocal : public arcNetBasePlayerProfile {
}; arcNetBasePlayerProfileLocal playerProfiles[MAX_INPUT_DEVICES];

/*
========================
Contains data that needs to be saved out on a per player profile basis, global for the lifetime of the player so
the data can be shared across computers.
- HUD tint colors
- key bindings
- etc...
========================
*/

/*
========================
arcNetBasePlayerProfile * CreatePlayerProfile
========================
*/
arcNetBasePlayerProfile * arcNetBasePlayerProfile::CreatePlayerProfile( int deviceIndex ) {
}

/*
========================
arcNetBasePlayerProfile::arcNetBasePlayerProfile
========================
*/
arcNetBasePlayerProfile::arcNetBasePlayerProfile() {
	SetDefaults();

	// Don't have these in SetDefaults because they're used for state management and SetDefaults is called when
	// loading the profile
	state				= IDLE;
	requestedState		= IDLE;
	deviceNum			= -1;
}

/*
========================
arcNetBasePlayerProfile::SetDefaults
========================
*/
void arcNetBasePlayerProfile::SetDefaults() {
	stats.SetNum( MAX_PLAYER_PROFILE_STATS );
	for ( int i = 0; i < MAX_PLAYER_PROFILE_STATS; ++i ) {
		stats[i].i = 0;
	}

	customConfig = false;
	configSet = 0;
}

/*
========================
arcNetBasePlayerProfile::~arcNetBasePlayerProfile
========================
*/
arcNetBasePlayerProfile::~arcNetBasePlayerProfile() {
}

/*
========================
arcNetBasePlayerProfile::Serialize
========================
*/
bool arcNetBasePlayerProfile::Serialize( idSerializer & ser ) {
	// NOTE:
	// See comments at top of file on versioning rules
	// Default to current tag/version
	int32 magicNumber = 0;
	magicNumber += PROFILE_TAG << 16;
	magicNumber += PROFILE_VER_MAJOR << 8;
	magicNumber += PROFILE_VER_MINOR;

	// Serialize version
	ser.SerializePacked( magicNumber );
	int16 tag = ( magicNumber >> 16 ) & 0xffff;
	int8 majorVersion = ( magicNumber >> 8 ) & 0xff;
	int8 minorVersion = magicNumber & 0xff; minorVersion;

	if ( tag != PROFILE_TAG ) {
		return false;
	}

	if ( majorVersion != PROFILE_VER_MAJOR ) {
		return false;
	}

	// Archived cvars (all the menu settings for Doom3 are archived cvars)
	arcDictionary cvarDict;
	cvarSystem->MoveCVarsToDict( CVAR_ARCHIVE, cvarDict );
	cvarDict.Serialize( ser );
	if ( ser.IsReading() ) {
		// Never sync these cvars with Steam because they require an engine or video restart
		cvarDict.Delete( "r_fullscreen" );
		cvarDict.Delete( "r_vidMode" );
		cvarDict.Delete( "r_multisamples" );
		cvarDict.Delete( "com_engineHz" );
		cvarSystem->SetCVarsFromDict( cvarDict );
		common->StartupVariable( NULL );
	}

	ser.Serialize( configSet );

	if ( ser.IsReading() ) {
		// Which binding is used on the console?
		ser.Serialize( customConfig );
		ExecConfig( false );

		if ( customConfig ) {
			for ( int i = 0; i < K_LAST_KEY; ++i ) {
				arcNetString bind;
				ser.SerializeString( bind );
				idKeyInput::SetBinding( i, bind.c_str() );
			}
		}
	} else {
		if ( !customConfig ) {
			ExecConfig( false );
		}

		customConfig = true;
		ser.Serialize( customConfig );

		for ( int i = 0; i < K_LAST_KEY; ++i ) {
			arcNetString bind = idKeyInput::GetBinding( i );
			ser.SerializeString( bind );
		}
	}

	return true;
}

/*
========================
arcNetBasePlayerProfile::StatSetInt
========================
*/
void arcNetBasePlayerProfile::StatSetInt( int s, int v ) {
	stats[s].i = v;

}

/*
========================
arcNetBasePlayerProfile::StatSetFloat
========================
*/
void arcNetBasePlayerProfile::StatSetFloat( int s, float v ) {
	stats[s].f = v;
}

/*
========================
arcNetBasePlayerProfile::StatGetInt
========================
*/
int	arcNetBasePlayerProfile::StatGetInt( int s ) const {
	return stats[s].i;
}

/*
========================
arcNetBasePlayerProfile::StatGetFloat
========================
*/
float arcNetBasePlayerProfile::StatGetFloat( int s ) const {
	return stats[s].f;
}

/*
========================
arcNetBasePlayerProfile::SaveSettings
========================
*/
void arcNetBasePlayerProfile::SaveSettings( bool forceDirty ) {
}

/*
========================
arcNetBasePlayerProfile::SaveSettings
========================
*/
void arcNetBasePlayerProfile::LoadSettings() {
}

/*
========================
arcNetBasePlayerProfile::SetAchievement
========================
*/
void arcNetBasePlayerProfile::SetAchievement( const int id ) {
}

/*
========================
arcNetBasePlayerProfile::ClearAchievement
========================
*/
void arcNetBasePlayerProfile::ClearAchievement( const int id ) {
}

/*
========================
arcNetBasePlayerProfile::GetAchievement
========================
*/
bool arcNetBasePlayerProfile::GetAchievement( const int id ) const {
}

/*
========================
arcNetBasePlayerProfile::SetConfig
========================
*/
void arcNetBasePlayerProfile::SetConfig( int config, bool save ) {
	configSet = config;
	ExecConfig( save );
}

/*
========================
arcNetBasePlayerProfile::SetConfig
========================
*/
void arcNetBasePlayerProfile::RestoreDefault() {
	ExecConfig( true, true );
}

/*
========================
arcNetBasePlayerProfile::ExecConfig
========================
*/
void arcNetBasePlayerProfile::ExecConfig( bool save, bool forceDefault ) {
}
