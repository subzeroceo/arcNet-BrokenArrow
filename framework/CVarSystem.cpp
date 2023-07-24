#include "idlib/precompiled.h"
#include "idlib/Lib.h"
#pragma hdrstop

arcCVarSystem * arcCVarSystem::staticVars = NULL;

/*
===============================================================================

	ARCInternalCVarSys

===============================================================================
*/

class ARCInternalCVarSys : public arcCVarSystem {
	friend class arcCVarSysLocal;
public:
							ARCInternalCVarSys();
							ARCInternalCVarSys( const char *newName, const char *newValue, int newFlags );
							ARCInternalCVarSys( const arcCVarSystem *cvar );
	virtual					~ARCInternalCVarSys();

	const char **			CopyValueStrings( const char **strings );
	void					Update( const arcCVarSystem *cvar );
	void					UpdateValue();
	void					UpdateCheat();
	void					Set( const char *newValue, bool force, bool fromServer );
	void					Reset();

private:
	arcNetString				nameString;				// name
	arcNetString				resetString;			// resetting will change to this value
	arcNetString				valueString;			// value
	arcNetString				descriptionString;		// description

	virtual const char *	InternalGetResetString() const;

	virtual void			InternalSetString( const char *newValue );
	virtual void			InternalServerSetString( const char *newValue );
	virtual void			InternalSetBool( const bool newValue );
	virtual void			InternalSetInteger( const int newValue );
	virtual void			InternalSetFloat( const float newValue );
	virtual void			InternalSetByte( const byte *newValue );
};

/*
============
ARCInternalCVarSys::ARCInternalCVarSys
============
*/
ARCInternalCVarSys::ARCInternalCVarSys() {
}

/*
============
ARCInternalCVarSys::ARCInternalCVarSys
============
*/
ARCInternalCVarSys::ARCInternalCVarSys( const char *newName, const char *newValue, int newFlags ) {
	nameString = newName;
	name = nameString.c_str();
	valueString = newValue;
	value = valueString.c_str();
	resetString = newValue;
	descriptionString = "";
	description = descriptionString.c_str();
	flags = ( newFlags & ~CVAR_STATIC ) | CVAR_MODIFIED;
	valueMin = 1;
	valueMax = -1;
	valueStrings = NULL;
	valueCompletion = 0;
	UpdateValue();
	UpdateCheat();
	internalVar = this;
}

/*
============
ARCInternalCVarSys::ARCInternalCVarSys
============
*/
ARCInternalCVarSys::ARCInternalCVarSys( const arcCVarSystem *cvar ) {
	nameString = cvar->GetName();
	name = nameString.c_str();
	valueString = cvar->GetString();
	value = valueString.c_str();
	resetString = cvar->GetString();
	descriptionString = cvar->GetDescription();
	description = descriptionString.c_str();
	flags = cvar->GetFlags() | CVAR_MODIFIED;
	valueMin = cvar->GetMinValue();
	valueMax = cvar->GetMaxValue();
	//byteValue = cvar->GetByteValue();
	valueStrings = CopyValueStrings( cvar->GetValueStrings() );
	valueCompletion = cvar->GetValueCompletion();
	UpdateValue();
	UpdateCheat();
	internalVar = this;
}

/*
============
ARCInternalCVarSys::~ARCInternalCVarSys
============
*/
ARCInternalCVarSys::~ARCInternalCVarSys() {
	Mem_Free( valueStrings );
	valueStrings = NULL;
}

/*
============
ARCInternalCVarSys::CopyValueStrings
============
*/
const char **ARCInternalCVarSys::CopyValueStrings( const char **strings ) {
	int i, totalLength;
	const char **ptr;
	char *str;

	if ( !strings ) {
		return NULL;
	}

	totalLength = 0;
	for ( i = 0; strings[i] != NULL; i++ ) {
		totalLength += arcNetString::Length( strings[i] ) + 1;
	}

	ptr = (const char **) Mem_Alloc( ( i + 1 ) * sizeof( char * ) + totalLength, TAG_CVAR );
	str = (char *)( ( ( byte * ) ptr ) + ( i + 1 ) * sizeof( char * ) );

	for ( i = 0; strings[i] != NULL; i++ ) {
		ptr[i] = str;
		strcpy( str, strings[i] );
		str += arcNetString::Length( strings[i] ) + 1;
	}
	ptr[i] = NULL;

	return ptr;
}

/*
============
ARCInternalCVarSys::Update
============
*/
void ARCInternalCVarSys::Update( const arcCVarSystem *cvar ) {
	// if this is a statically declared variable
	if ( cvar->GetFlags() & CVAR_STATIC ) {
		if ( flags & CVAR_STATIC ) {
			// the code has more than one static declaration of the same variable, make sure they have the same properties
			if ( resetString.Icmp( cvar->GetString() ) != 0 ) {
				common->Warning( "CVar '%s' declared multiple times with different initial value", nameString.c_str() );
			}
			if ( ( flags & ( CVAR_BOOL | CVAR_INTEGER | CVAR_FLOAT | CVAR_BYTE ) ) != ( cvar->GetFlags() & ( CVAR_BOOL | CVAR_INTEGER | CVAR_FLOAT | CVAR_BYTE ) ) ) {
				common->Warning( "CVar '%s' declared multiple times with different type", nameString.c_str() );
			}
			if ( valueMin != cvar->GetMinValue() || valueMax != cvar->GetMaxValue() ) {
				common->Warning( "CVar '%s' declared multiple times with different minimum/maximum", nameString.c_str() );
			}
		}

		// the code is now specifying a variable that the user already set a value for, take the new value as the reset value
		resetString = cvar->GetString();
		descriptionString = cvar->GetDescription();
		description = descriptionString.c_str();
		valueMin = cvar->GetMinValue();
		valueMax = cvar->GetMaxValue();
		Mem_Free( valueStrings );
		valueStrings = CopyValueStrings( cvar->GetValueStrings() );
		valueCompletion = cvar->GetValueCompletion();
		UpdateValue();
		cvarSystem->SetModifiedFlags( cvar->GetFlags() );
	}

	flags |= cvar->GetFlags();

	UpdateCheat();

	// only allow one non-empty reset string without a warning
	if ( resetString.Length() == 0 ) {
		resetString = cvar->GetString();
	} else if ( cvar->GetString()[0] && resetString.Cmp( cvar->GetString() ) != 0 ) {
		common->Warning( "cvar \"%s\" given initial values: \"%s\" and \"%s\"\n", nameString.c_str(), resetString.c_str(), cvar->GetString() );
	}
}

/*
============
ARCInternalCVarSys::UpdateValue
============
*/
void ARCInternalCVarSys::UpdateValue() {
	bool clamped = false;

	if ( flags & CVAR_BOOL ) {
		integerValue = ( atoi( value ) != 0 );
		floatValue = integerValue;
		if ( arcNetString::Icmp( value, "0" ) != 0 && arcNetString::Icmp( value, "1" ) != 0 ) {
			valueString = arcNetString( (bool)( integerValue != 0 ) );
			value = valueString.c_str();
		}
	} else if ( flags & CVAR_INTEGER ) {
		integerValue = ( int )atoi( value );
		if ( valueMin < valueMax ) {
			if ( integerValue < valueMin ) {
				integerValue = ( int )valueMin;
				clamped = true;
			} else if ( integerValue > valueMax ) {
				integerValue = ( int )valueMax;
				clamped = true;
			}
		}
		if ( clamped || !arcNetString::IsNumeric( value ) || arcNetString::FindChar( value, '.' ) ) {
			valueString = arcNetString( integerValue );
			value = valueString.c_str();
		}
		floatValue = ( float )integerValue;
	} else if ( flags & CVAR_FLOAT ) {
		floatValue = ( float )atof( value );
		if ( valueMin < valueMax ) {
			if ( floatValue < valueMin ) {
				floatValue = valueMin;
				clamped = true;
			} else if ( floatValue > valueMax ) {
				floatValue = valueMax;
				clamped = true;
			}
		}
		if ( clamped || !arcNetString::IsNumeric( value ) ) {
			valueString = arcNetString( floatValue );
			value = valueString.c_str();
		}
		integerValue = ( int )floatValue;
    } else if ( flags & CVAR_BYTE ) {
        byteValue = ( byte )byteValue;
        valueString = arcNetString( byteValue );
        value = valueString.c_str();
        integerValue = ( int )byteValue;
        floatValue = ( float )byteValue;
	} else {
		if ( valueStrings && valueStrings[0] ) {
			integerValue = 0;
			for ( int i = 0; valueStrings[i]; i++ ) {
				if ( valueString.Icmp( valueStrings[i] ) == 0 ) {
					integerValue = i;
					break;
				}
			}
			valueString = valueStrings[integerValue];
			value = valueString.c_str();
			floatValue = ( float )integerValue;
		} else if ( valueString.Length() < 32 ) {
			floatValue = ( float )atof( value );
			integerValue = ( int )floatValue;
		} else {
			floatValue = 0.0f;
			integerValue = 0;
		}
	}
}

/*
============
ARCInternalCVarSys::UpdateCheat
============
*/
void ARCInternalCVarSys::UpdateCheat() {
	// all variables are considered cheats except for a few types
	if ( flags & ( CVAR_NOCHEAT | CVAR_INIT | CVAR_ROM | CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NETWORKSYNC ) ) {
		flags &= ~CVAR_CHEAT;
	} else {
		flags |= CVAR_CHEAT;
	}
}

/*
============
ARCInternalCVarSys::Set
============
*/
void ARCInternalCVarSys::Set( const char *newValue, bool force, bool fromServer ) {
	if ( !newValue ) {
		newValue = resetString.c_str();
	}

	if ( !force ) {
		if ( flags & CVAR_ROM ) {
			common->Printf( "%s is read only.\n", nameString.c_str() );
			return;
		}

		if ( flags & CVAR_INIT ) {
			common->Printf( "%s is write protected.\n", nameString.c_str() );
			return;
		}
	}

	if ( valueString.Icmp( newValue ) == 0 ) {
		return;
	}

	valueString = newValue;
	value = valueString.c_str();
	UpdateValue();

	SetModified();
	cvarSystem->SetModifiedFlags( flags );
}

/*
============
ARCInternalCVarSys::Reset
============
*/
void ARCInternalCVarSys::Reset() {
	valueString = resetString;
	value = valueString.c_str();
	UpdateValue();
}

/*
============
ARCInternalCVarSys::InternalGetResetString
============
*/
const char * ARCInternalCVarSys::InternalGetResetString() const {
	return resetString;
}

/*
============
ARCInternalCVarSys::InternalSetString
============
*/
void ARCInternalCVarSys::InternalSetString( const char *newValue ) {
	Set( newValue, true, false );
}

/*
===============
ARCInternalCVarSys::InternalServerSetString
===============
*/
void ARCInternalCVarSys::InternalServerSetString( const char *newValue ) {
	Set( newValue, true, true );
}

/*
============
ARCInternalCVarSys::InternalSetBool
============
*/
void ARCInternalCVarSys::InternalSetBool( const bool newValue ) {
	Set( arcNetString( newValue ), true, false );
}

/*
============
ARCInternalCVarSys::InternalSetInteger
============
*/
void ARCInternalCVarSys::InternalSetInteger( const int newValue ) {
	Set( arcNetString( newValue ), true, false );
}

/*
============
ARCInternalCVarSys::InternalSetFloat
============
*/
void ARCInternalCVarSys::InternalSetFloat( const float newValue ) {
	Set( arcNetString( newValue ), true, false );
}

/*
============
ARCInternalCVarSys::InternalSetFloat
============
*/
void ARCInternalCVarSys::InternalSetByte( const byte *newValue ) {
	Set( arcNetString( newValue ), true, false );
}

/*
===============================================================================

	arcCVarSysLocal

===============================================================================
*/

class arcCVarSysLocal : public arcCVarSystem {
public:
							arcCVarSysLocal();

	virtual					~arcCVarSysLocal() {}

	virtual void			Init();
	virtual void			Shutdown();
	virtual bool			IsInitialized() const;

	virtual void			Register( arcCVarSystem *cvar );

	virtual arcCVarSystem			*Find( const char *name );

	virtual void			SetCVarString( const char *name, const char *value, int flags = 0 );
	virtual void			SetCVarBool( const char *name, const bool value, int flags = 0 );
	virtual void			SetCVarInteger( const char *name, const int value, int flags = 0 );
	virtual void			SetCVarFloat( const char *name, const float value, int flags = 0 );

	virtual const char		*GetCVarString( const char *name ) const;
	virtual bool			GetCVarBool( const char *name ) const;
	virtual int				GetCVarInteger( const char *name ) const;
	virtual float			GetCVarFloat( const char *name ) const;

	virtual bool			Command( const arcCommandArgs &args );

	virtual void			CommandCompletion( void(*callback)( const char *s ) );
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) );

	virtual void			SetModifiedFlags( int flags );
	virtual int				GetModifiedFlags() const;
	virtual void			ClearModifiedFlags( int flags );

	virtual void			ResetFlaggedVariables( int flags );
	virtual void			RemoveFlaggedAutoCompletion( int flags );
	virtual void			WriteFlaggedVariables( int flags, const char *setCmd, arcNetFile *f ) const;

	virtual void			MoveCVarsToDict( int flags, arcDictionary & dict, bool onlyModified ) const;
	virtual void			SetCVarsFromDict( const arcDictionary &dict );

	void					RegisterInternal( arcCVarSystem *cvar );
	ARCInternalCVarSys		*FindInternal( const char *name ) const;
	void					SetInternal( const char *name, const char *value, int flags );

private:
	bool					initialized;
	arcNetList<ARCInternalCVarSys*, TAG_CVAR>	cvars;
	ARCHashIndex				cvarHash;
	int						modifiedFlags;

private:
	static void				Toggle_f( const arcCommandArgs &args );
	static void				Set_f( const arcCommandArgs &args );
	static void				Reset_f( const arcCommandArgs &args );
	static void				ListByFlags( const arcCommandArgs &args, cvarFlags_t flags );
	static void				List_f( const arcCommandArgs &args );
	static void				Restart_f( const arcCommandArgs &args );
	static void				CvarAdd_f( const arcCommandArgs &args );
};

arcCVarSysLocal			localCVarSystem;
arcCVarSystem *				cvarSystem = &localCVarSystem;

#define NUM_COLUMNS				77		// 78 - 1
#define NUM_NAME_CHARS			33
#define NUM_DESCRIPTION_CHARS	( NUM_COLUMNS - NUM_NAME_CHARS )
#define FORMAT_STRING			"%-32s "

const char *CreateColumn( const char *text, int columnWidth, const char *indent, arcNetString &string ) {
	int i, lastLine;

	string.Clear();
	for ( lastLine = i = 0; text[i] != '\0'; i++ ) {
		if ( i - lastLine >= columnWidth || text[i] == '\n' ) {
			while ( i > 0 && text[i] > ' ' && text[i] != '/' && text[i] != ',' && text[i] != '\\' ) {
				i--;
			}
			while ( lastLine < i ) {
				string.Append( text[lastLine++] );
			}
			string.Append( indent );
			lastLine++;
		}
	}
	while( lastLine < i ) {
		string.Append( text[lastLine++] );
	}
	return string.c_str();
}

/*
============
arcCVarSysLocal::FindInternal
============
*/
ARCInternalCVarSys *arcCVarSysLocal::FindInternal( const char *name ) const {
	int hash = cvarHash.GenerateKey( name, false );
	for ( int i = cvarHash.First( hash ); i != -1; i = cvarHash.Next( i ) ) {
		if ( cvars[i]->nameString.Icmp( name ) == 0 ) {
			return cvars[i];
		}
	}
	return NULL;
}

/*
============
arcCVarSysLocal::SetInternal
============
*/
void arcCVarSysLocal::SetInternal( const char *name, const char *value, int flags ) {
	ARCInternalCVarSys *internal = FindInternal( name );

	if ( internal ) {
		internal->InternalSetString( value );
		internal->flags |= flags & ~CVAR_STATIC;
		internal->UpdateCheat();
	} else {
		internal = new (TAG_SYSTEM) ARCInternalCVarSys( name, value, flags );
		int hash = cvarHash.GenerateKey( internal->nameString.c_str(), false );
		cvarHash.Add( hash, cvars.Append( internal ) );
	}
}

/*
============
arcCVarSysLocal::arcCVarSysLocal
============
*/
arcCVarSysLocal::arcCVarSysLocal() {
	initialized = false;
	modifiedFlags = 0;
}

/*
============
arcCVarSysLocal::Init
============
*/
void arcCVarSysLocal::Init() {
	modifiedFlags = 0;

	cmdSystem->AddCommand( "toggle", Toggle_f, CMD_FL_SYSTEM, "toggles a cvar" );
	cmdSystem->AddCommand( "set", Set_f, CMD_FL_SYSTEM, "sets a cvar" );
	cmdSystem->AddCommand( "seta", Set_f, CMD_FL_SYSTEM, "sets a cvar" );
	cmdSystem->AddCommand( "sets", Set_f, CMD_FL_SYSTEM, "sets a cvar" );
	cmdSystem->AddCommand( "sett", Set_f, CMD_FL_SYSTEM, "sets a cvar" );
	cmdSystem->AddCommand( "setu", Set_f, CMD_FL_SYSTEM, "sets a cvar" );
	cmdSystem->AddCommand( "reset", Reset_f, CMD_FL_SYSTEM, "resets a cvar" );
	cmdSystem->AddCommand( "listvars", List_f, CMD_FL_SYSTEM, "lists cvars" );
	cmdSystem->AddCommand( "vars_restart", Restart_f, CMD_FL_SYSTEM, "restart the cvar system" );
	cmdSystem->AddCommand( "cvarAdd", CvarAdd_f, CMD_FL_SYSTEM, "adds a value to a numeric cvar" );

	initialized = true;
}

/*
============
arcCVarSysLocal::Shutdown
============
*/
void arcCVarSysLocal::Shutdown() {
	cvars.DeleteContents( true );
	cvarHash.Free();
	initialized = false;
}

/*
============
arcCVarSysLocal::IsInitialized
============
*/
bool arcCVarSysLocal::IsInitialized() const {
	return initialized;
}

/*
============
arcCVarSysLocal::Register
============
*/
void arcCVarSysLocal::Register( arcCVarSystem *cvar ) {
	cvar->SetInternalVar( cvar );

	ARCInternalCVarSys *internal = FindInternal( cvar->GetName() );

	if ( internal ) {
		internal->Update( cvar );
	} else {
		internal = new (TAG_SYSTEM) ARCInternalCVarSys( cvar );
		int hash = cvarHash.GenerateKey( internal->nameString.c_str(), false );
		cvarHash.Add( hash, cvars.Append( internal ) );
	}

	cvar->SetInternalVar( internal );
}

/*
============
arcCVarSysLocal::Find
============
*/
arcCVarSystem *arcCVarSysLocal::Find( const char *name ) {
	return FindInternal( name );
}

/*
============
arcCVarSysLocal::SetCVarString
============
*/
void arcCVarSysLocal::SetCVarString( const char *name, const char *value, int flags ) {
	SetInternal( name, value, flags );
}

/*
============
arcCVarSysLocal::SetCVarBool
============
*/
void arcCVarSysLocal::SetCVarBool( const char *name, const bool value, int flags ) {
	SetInternal( name, arcNetString( value ), flags );
}

/*
============
arcCVarSysLocal::SetCVarInteger
============
*/
void arcCVarSysLocal::SetCVarInteger( const char *name, const int value, int flags ) {
	SetInternal( name, arcNetString( value ), flags );
}

/*
============
arcCVarSysLocal::SetCVarFloat
============
*/
void arcCVarSysLocal::SetCVarFloat( const char *name, const float value, int flags ) {
	SetInternal( name, arcNetString( value ), flags );
}

/*
============
arcCVarSysLocal::GetCVarString
============
*/
const char *arcCVarSysLocal::GetCVarString( const char *name ) const {
	ARCInternalCVarSys *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetString();
	}
	return "";
}

/*
============
arcCVarSysLocal::GetCVarBool
============
*/
bool arcCVarSysLocal::GetCVarBool( const char *name ) const {
	ARCInternalCVarSys *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetBool();
	}
	return false;
}

/*
============
arcCVarSysLocal::GetCVarInteger
============
*/
int arcCVarSysLocal::GetCVarInteger( const char *name ) const {
	ARCInternalCVarSys *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetInteger();
	}
	return 0;
}

/*
============
arcCVarSysLocal::GetCVarFloat
============
*/
float arcCVarSysLocal::GetCVarFloat( const char *name ) const {
	ARCInternalCVarSys *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetFloat();
	}
	return 0.0f;
}

/*
============
arcCVarSysLocal::Command
============
*/
bool arcCVarSysLocal::Command( const arcCommandArgs &args ) {
	ARCInternalCVarSys *internal = FindInternal( args.Argv( 0 ) );

	if ( internal == NULL ) {
		return false;
	}

	if ( args.Argc() == 1 ) {
		// print the variable
		common->Printf( "\"%s\" is:\"%s\"" S_COLOR_WHITE " default:\"%s\"\n", internal->nameString.c_str(), internal->valueString.c_str(), internal->resetString.c_str() );
		if ( arcNetString::Length( internal->GetDescription() ) > 0 ) {
			common->Printf( S_COLOR_WHITE "%s\n", internal->GetDescription() );
		}
	} else {
		// set the value
		internal->Set( args.Args(), false, false );
	}
	return true;
}

/*
============
arcCVarSysLocal::CommandCompletion
============
*/
void arcCVarSysLocal::CommandCompletion( void(*callback)( const char *s ) ) {
	for ( int i = 0; i < cvars.Num(); i++ ) {
		callback( cvars[i]->GetName() );
	}
}

/*
============
arcCVarSysLocal::ArgCompletion
============
*/
void arcCVarSysLocal::ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) {
	arcCommandArgs args.TokenizeString( cmdString, false );

	for ( int i = 0; i < cvars.Num(); i++ ) {
		if ( !cvars[i]->valueCompletion ) {
			continue;
		}
		if ( arcNetString::Icmp( args.Argv( 0 ), cvars[i]->nameString.c_str() ) == 0 ) {
			cvars[i]->valueCompletion( args, callback );
			break;
		}
	}
}

/*
============
arcCVarSysLocal::SetModifiedFlags
============
*/
void arcCVarSysLocal::SetModifiedFlags( int flags ) {
	modifiedFlags |= flags;
}

/*
============
arcCVarSysLocal::GetModifiedFlags
============
*/
int arcCVarSysLocal::GetModifiedFlags() const {
	return modifiedFlags;
}

/*
============
arcCVarSysLocal::ClearModifiedFlags
============
*/
void arcCVarSysLocal::ClearModifiedFlags( int flags ) {
	modifiedFlags &= ~flags;
}

/*
============
arcCVarSysLocal::ResetFlaggedVariables
============
*/
void arcCVarSysLocal::ResetFlaggedVariables( int flags ) {
	for ( int i = 0; i < cvars.Num(); i++ ) {
		ARCInternalCVarSys *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			cvar->Set( NULL, true, true );
		}
	}
}

/*
============
arcCVarSysLocal::RemoveFlaggedAutoCompletion
============
*/
void arcCVarSysLocal::RemoveFlaggedAutoCompletion( int flags ) {
	for ( int i = 0; i < cvars.Num(); i++ ) {
		ARCInternalCVarSys *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			cvar->valueCompletion = NULL;
		}
	}
}

/*
============
arcCVarSysLocal::WriteFlaggedVariables

Appends lines containing "set variable value" for all variables
with the "flags" flag set to true.
============
*/
void arcCVarSysLocal::WriteFlaggedVariables( int flags, const char *setCmd, arcNetFile *f ) const {
	for ( int i = 0; i < cvars.Num(); i++ ) {
		ARCInternalCVarSys *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			f->Printf( "%s %s \"%s\"\n", setCmd, cvar->GetName(), cvar->GetString() );
		}
	}
}

/*
============
arcCVarSysLocal::MoveCVarsToDict
============
*/
void arcCVarSysLocal::MoveCVarsToDict( int flags, arcDictionary & dict, bool onlyModified ) const {
	dict.Clear();
	for ( int i = 0; i < cvars.Num(); i++ ) {
		arcCVarSystem *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			if ( onlyModified && arcNetString::Icmp( cvar->GetString(), cvar->GetDefaultString() ) == 0 ) {
				continue;
			}
			dict.Set( cvar->GetName(), cvar->GetString() );
		}
	}
}

/*
============
arcCVarSysLocal::SetCVarsFromDict
============
*/
void arcCVarSysLocal::SetCVarsFromDict( const arcDictionary &dict ) {
	for ( int i = 0; i < dict.GetNumKeyVals(); i++ ) {
		const idKeyValue *kv = dict.GetKeyVal( i );
		ARCInternalCVarSys *internal = FindInternal( kv->GetKey() );
		if ( internal ) {
			internal->InternalServerSetString( kv->GetValue() );
		}
	}
}

/*
============
arcCVarSysLocal::Toggle_f
============
*/
void arcCVarSysLocal::Toggle_f( const arcCommandArgs &args ) {
	float current, set;
	const char *text;

	int argc = args.Argc();
	if ( argc < 2 ) {
		common->Printf ( "usage:\n"
			"   toggle <variable>  - toggles between 0 and 1\n"
			"   toggle <variable> <value> - toggles between 0 and <value>\n"
			"   toggle <variable> [string 1] [string 2]...[string n] - cycles through all strings\n" );
		return;
	}

	ARCInternalCVarSys *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );

	if ( cvar == NULL ) {
		common->Warning( "Toggle_f: cvar \"%s\" not found", args.Argv( 1 ) );
		return;
	}

	if ( argc > 3 ) {
		// cycle through multiple values
		text = cvar->GetString();
		for ( int i = 2; i < argc; i++ ) {
			if ( !arcNetString::Icmp( text, args.Argv( i ) ) ) {
				// point to next value
				i++;
				break;
			}
		}
		if ( i >= argc ) {
			i = 2;
		}

		common->Printf( "set %s = %s\n", args.Argv(1 ), args.Argv( i ) );
		cvar->Set( va( "%s", args.Argv( i ) ), false, false );
	} else {
		// toggle between 0 and 1
		current = cvar->GetFloat();
		if ( argc == 3 ) {
			set = atof( args.Argv( 2 ) );
		} else {
			set = 1.0f;
		}
		if ( current == 0.0f ) {
			current = set;
		} else {
			current = 0.0f;
		}
		common->Printf( "set %s = %f\n", args.Argv(1 ), current );
		cvar->Set( arcNetString( current ), false, false );
	}
}

/*
============
arcCVarSysLocal::Set_f
============
*/
void arcCVarSysLocal::Set_f( const arcCommandArgs &args ) {
	const char *str = args.Args( 2, args.Argc() - 1 );
	localCVarSystem.SetCVarString( args.Argv(1 ), str );
}

/*
============
arcCVarSysLocal::Reset_f
============
*/
void arcCVarSysLocal::Reset_f( const arcCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf ( "usage: reset <variable>\n" );
		return;
	}
	ARCInternalCVarSys *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
	if ( !cvar ) {
		return;
	}

	cvar->Reset();
}

/*
============
arcCVarSysLocal::CvarAdd_f
============
*/
void arcCVarSysLocal::CvarAdd_f( const arcCommandArgs &args ) {
	if ( args.Argc() != 3 ) {
		common->Printf ( "usage: cvarAdd <variable> <value>\n" );
	}

	ARCInternalCVarSys *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
	if ( !cvar ) {
		return;
	}

	const float newValue = cvar->GetFloat() + atof( args.Argv( 2 ) );
	cvar->SetFloat( newValue );
	common->Printf( "%s = %f\n", cvar->GetName(), newValue );
}
//
///*
//================================================
//ARCSortCommandDef
//================================================
//*/
//class ARCSortInternalCVar : public ARCSortQuick< const ARCInternalCVarSys *, ARCSortInternalCVar > {
//public:
//	int Compare( const ARCInternalCVarSys * & a, const ARCInternalCVarSys * & b ) const { return arcNetString::Icmp( a.GetName(), b.GetName() ); }
//};

void arcCVarSysLocal::ListByFlags( const arcCommandArgs &args, cvarFlags_t flags ) {
	arcNetString indent;
	arcNetList<const ARCInternalCVarSys *, TAG_CVAR>cvarList;

	enum {
		SHOW_VALUE,
		SHOW_DESCRIPTION,
		SHOW_TYPE,
		SHOW_FLAGS
	} show;

	int argNum = 1;
	show = SHOW_VALUE;

	if ( arcNetString::Icmp( args.Argv( argNum ), "-" ) == 0 || arcNetString::Icmp( args.Argv( argNum ), "/" ) == 0 ) {
		if ( arcNetString::Icmp( args.Argv( argNum + 1 ), "help" ) == 0 || arcNetString::Icmp( args.Argv( argNum + 1 ), "?" ) == 0 ) {
			argNum = 3;
			show = SHOW_DESCRIPTION;
		} else if ( arcNetString::Icmp( args.Argv( argNum + 1 ), "type" ) == 0 || arcNetString::Icmp( args.Argv( argNum + 1 ), "range" ) == 0 ) {
			argNum = 3;
			show = SHOW_TYPE;
		} else if ( arcNetString::Icmp( args.Argv( argNum + 1 ), "flags" ) == 0 ) {
			argNum = 3;
			show = SHOW_FLAGS;
		}
	}

	if ( args.Argc() > argNum ) {
		arcNetString match = args.Args( argNum, -1 );
		match.Replace( " ", "" );
	} else {
		arcNetString match = "";
	}

	for ( int i = 0; i < localCVarSystem.cvars.Num(); i++ ) {
		const ARCInternalCVarSys *cvar = localCVarSystem.cvars[i];
		if ( !( cvar->GetFlags() & flags ) ) {
			continue;
		}

		if ( match.Length() && !cvar->nameString.Filter( match, false ) ) {
			continue;
		}

		cvarList.Append( cvar );
	}

	//cvarList.SortWithTemplate( ARCSortInternalCVar() );

	switch( show ) {
		case SHOW_VALUE: {
			for ( i = 0; i < cvarList.Num(); i++ ) {
				const ARCInternalCVarSys *cvar = cvarList[i];
				common->Printf( FORMAT_STRING S_COLOR_WHITE "\"%s\"\n", cvar->nameString.c_str(), cvar->valueString.c_str() );
			}
			break;
		}
		case SHOW_DESCRIPTION: {
			arcNetString indent.Fill( ' ', NUM_NAME_CHARS );
			arcNetString indent.Insert( "\n", 0 );
			for ( i = 0; i < cvarList.Num(); i++ ) {
				const ARCInternalCVarSys *cvar = cvarList[i];
				common->Printf( FORMAT_STRING S_COLOR_WHITE "%s\n", cvar->nameString.c_str(), CreateColumn( cvar->GetDescription(), NUM_DESCRIPTION_CHARS, indent, string ) );
			}
			break;
		}
		case SHOW_TYPE: {
			for ( i = 0; i < cvarList.Num(); i++ ) {
			const ARCInternalCVarSys *cvar = cvarList[i];
				if ( cvar->GetFlags() & CVAR_BOOL ) {
					common->Printf( FORMAT_STRING S_COLOR_CYAN "bool\n", cvar->GetName() );
				} else if ( cvar->GetFlags() & CVAR_INTEGER ) {
					if ( cvar->GetMinValue() < cvar->GetMaxValue() ) {
						common->Printf( FORMAT_STRING S_COLOR_GREEN "int " S_COLOR_WHITE "[%d, %d]\n", cvar->GetName(), ( int ) cvar->GetMinValue(), ( int ) cvar->GetMaxValue() );
					} else {
						common->Printf( FORMAT_STRING S_COLOR_GREEN "int\n", cvar->GetName() );
					}
				} else if ( cvar->GetFlags() & CVAR_FLOAT ) {
					if ( cvar->GetMinValue() < cvar->GetMaxValue() ) {
						common->Printf( FORMAT_STRING S_COLOR_RED "float " S_COLOR_WHITE "[%s, %s]\n", cvar->GetName(), arcNetString( cvar->GetMinValue() ).c_str(), arcNetString( cvar->GetMaxValue() ).c_str() );
					} else {
						common->Printf( FORMAT_STRING S_COLOR_RED "float\n", cvar->GetName() );
					}
				} else if ( cvar->GetValueStrings() ) {
					common->Printf( FORMAT_STRING S_COLOR_WHITE "string " S_COLOR_WHITE "[", cvar->GetName() );
					for ( int j = 0; cvar->GetValueStrings()[j] != NULL; j++ ) {
						if ( j ) {
							common->Printf( S_COLOR_WHITE ", %s", cvar->GetValueStrings()[j] );
						} else {
							common->Printf( S_COLOR_WHITE "%s", cvar->GetValueStrings()[j] );
						}
					}
					common->Printf( S_COLOR_WHITE "]\n" );
				} else {
					common->Printf( FORMAT_STRING S_COLOR_WHITE "string\n", cvar->GetName() );
				}
			}
			break;
		}
		case SHOW_FLAGS: {
			for ( int i = 0; i < cvarList.Num(); i++ ) {
				const ARCInternalCVarSys *cvar = cvarList[i];
				common->Printf( FORMAT_STRING, cvar->GetName() );
				arcNetString string = "";
				if ( cvar->GetFlags() & CVAR_BOOL ) {
					string += S_COLOR_CYAN "B ";
				} else if ( cvar->GetFlags() & CVAR_INTEGER ) {
					string += S_COLOR_GREEN "I ";
				} else if ( cvar->GetFlags() & CVAR_FLOAT ) {
					string += S_COLOR_RED "F ";
				} else {
					string += S_COLOR_WHITE "S ";
				}
				if ( cvar->GetFlags() & CVAR_SYSTEM ) {
					string += S_COLOR_WHITE "SYS  ";
				} else if ( cvar->GetFlags() & CVAR_RENDERER ) {
					string += S_COLOR_WHITE "RNDR ";
				} else if ( cvar->GetFlags() & CVAR_SOUND ) {
					string += S_COLOR_WHITE "SND  ";
				} else if ( cvar->GetFlags() & CVAR_GUI ) {
					string += S_COLOR_WHITE "GUI  ";
				} else if ( cvar->GetFlags() & CVAR_GAME ) {
					string += S_COLOR_WHITE "GAME ";
				} else if ( cvar->GetFlags() & CVAR_TOOL ) {
					string += S_COLOR_WHITE "TOOL ";
				} else {
					string += S_COLOR_WHITE "     ";
				}
				string += ( cvar->GetFlags() & CVAR_SERVERINFO ) ?	"SI "	: "   ";
				string += ( cvar->GetFlags() & CVAR_STATIC ) ?		"ST "	: "   ";
				string += ( cvar->GetFlags() & CVAR_CHEAT ) ?		"CH "	: "   ";
				string += ( cvar->GetFlags() & CVAR_INIT ) ?		"IN "	: "   ";
				string += ( cvar->GetFlags() & CVAR_ROM ) ?			"RO "	: "   ";
				string += ( cvar->GetFlags() & CVAR_ARCHIVE ) ?		"AR "	: "   ";
				string += ( cvar->GetFlags() & CVAR_MODIFIED ) ?	"MO "	: "   ";
				string += "\n";
				common->Printf( string );
			}
			break;
		}
	}

	common->Printf( "\n%i cvars listed\n\n", cvarList.Num() );
	common->Printf(	"listCvar [search string]          = list cvar values\n"
				"listCvar -help [search string]    = list cvar descriptions\n"
				"listCvar -type [search string]    = list cvar types\n"
				"listCvar -flags [search string]   = list cvar flags\n" );
}

/*
============
arcCVarSysLocal::List_f
============
*/
void arcCVarSysLocal::List_f( const arcCommandArgs &args ) {
	ListByFlags( args, CVAR_ALL );
}

/*
============
arcCVarSysLocal::Restart_f
============
*/
void arcCVarSysLocal::Restart_f( const arcCommandArgs &args ) {
	for ( int i = 0; i < localCVarSystem.cvars.Num(); i++ ) {
	ARCInternalCVarSys *cvar = localCVarSystem.cvars[i];
		// don't mess with rom values
		if ( cvar->flags & ( CVAR_ROM | CVAR_INIT ) ) {
			continue;
		}

		// throw out any variables the user created
		if ( !( cvar->flags & CVAR_STATIC ) ) {
			int hash = localCVarSystem.cvarHash.GenerateKey( cvar->nameString, false );
			delete cvar;
			localCVarSystem.cvars.RemoveIndex( i );
			localCVarSystem.cvarHash.RemoveIndex( hash, i );
			i--;
			continue;
		}

		cvar->Reset();
	}
}
