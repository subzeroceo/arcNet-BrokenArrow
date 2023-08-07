#include "/idlib/Lib.h"
#include "idlib/Lib.h"
#pragma hdrstop

anCVarSystem * anCVarSystem::staticVars = nullptr;

/*
===============================================================================

	anInternalCVar

===============================================================================
*/

class anInternalCVar : public anCVarSystem {
	friend class anCVarSysLocal;
public:
							anInternalCVar();
							anInternalCVar( const char *newName, const char *newValue, int newFlags );
							anInternalCVar( const anCVarSystem *cvar );
	virtual					~anInternalCVar();

	const char **			CopyValueStrings( const char **strings );
	void					Update( const anCVarSystem *cvar );
	void					UpdateValue();
	void					UpdateCheat();
	void					Set( const char *newValue, bool force, bool fromServer );
	void					Reset();

private:
	anStr				nameString;				// name
	anStr				resetString;			// resetting will change to this value
	anStr				valueString;			// value
	anStr				descriptionString;		// description

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
anInternalCVar::anInternalCVar
============
*/
anInternalCVar::anInternalCVar() {
}

/*
============
anInternalCVar::anInternalCVar
============
*/
anInternalCVar::anInternalCVar( const char *newName, const char *newValue, int newFlags ) {
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
	valueStrings = nullptr;
	valueCompletion = 0;
	UpdateValue();
	UpdateCheat();
	internalVar = this;
}

/*
============
anInternalCVar::anInternalCVar
============
*/
anInternalCVar::anInternalCVar( const anCVarSystem *cvar ) {
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
anInternalCVar::~anInternalCVar
============
*/
anInternalCVar::~anInternalCVar() {
	Mem_Free( valueStrings );
	valueStrings = nullptr;
}

/*
============
anInternalCVar::CopyValueStrings
============
*/
const char **anInternalCVar::CopyValueStrings( const char **strings ) {
	int i, totalLength;
	const char **ptr;
	char *str;

	if ( !strings ) {
		return nullptr;
	}

	totalLength = 0;
	for ( i = 0; strings[i] != nullptr; i++ ) {
		totalLength += anStr::Length( strings[i] ) + 1;
	}

	ptr = (const char **) Mem_Alloc( ( i + 1 ) * sizeof(char *) + totalLength, TAG_CVAR );
	str = (char *)( ( (byte *) ptr ) + ( i + 1 ) * sizeof(char *) );

	for ( i = 0; strings[i] != nullptr; i++ ) {
		ptr[i] = str;
		strcpy( str, strings[i] );
		str += anStr::Length( strings[i] ) + 1;
	}
	ptr[i] = nullptr;

	return ptr;
}

/*
============
anInternalCVar::Update
============
*/
void anInternalCVar::Update( const anCVarSystem *cvar ) {
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
anInternalCVar::UpdateValue
============
*/
void anInternalCVar::UpdateValue() {
	bool clamped = false;

	if ( flags & CVAR_BOOL ) {
		integerValue = ( atoi( value ) != 0 );
		floatValue = integerValue;
		if ( anStr::Icmp( value, "0" ) != 0 && anStr::Icmp( value, "1" ) != 0 ) {
			valueString = anStr( (bool)( integerValue != 0 ) );
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
		if ( clamped || !anStr::IsNumeric( value ) || anStr::FindChar( value, '.' ) ) {
			valueString = anStr( integerValue );
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
		if ( clamped || !anStr::IsNumeric( value ) ) {
			valueString = anStr( floatValue );
			value = valueString.c_str();
		}
		integerValue = ( int )floatValue;
    } else if ( flags & CVAR_BYTE ) {
        byteValue = ( byte )byteValue;
        valueString = anStr( byteValue );
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
anInternalCVar::UpdateCheat
============
*/
void anInternalCVar::UpdateCheat() {
	// all variables are considered cheats except for a few types
	if ( flags & ( CVAR_NOCHEAT | CVAR_INIT | CVAR_ROM | CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NETWORKSYNC ) ) {
		flags &= ~CVAR_CHEAT;
	} else {
		flags |= CVAR_CHEAT;
	}
}

/*
============
anInternalCVar::Set
============
*/
void anInternalCVar::Set( const char *newValue, bool force, bool fromServer ) {
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
anInternalCVar::Reset
============
*/
void anInternalCVar::Reset() {
	valueString = resetString;
	value = valueString.c_str();
	UpdateValue();
}

/*
============
anInternalCVar::InternalGetResetString
============
*/
const char *anInternalCVar::InternalGetResetString() const {
	return resetString;
}

/*
============
anInternalCVar::InternalSetString
============
*/
void anInternalCVar::InternalSetString( const char *newValue ) {
	Set( newValue, true, false );
}

/*
===============
anInternalCVar::InternalServerSetString
===============
*/
void anInternalCVar::InternalServerSetString( const char *newValue ) {
	Set( newValue, true, true );
}

/*
============
anInternalCVar::InternalSetBool
============
*/
void anInternalCVar::InternalSetBool( const bool newValue ) {
	Set( anStr( newValue ), true, false );
}

/*
============
anInternalCVar::InternalSetInteger
============
*/
void anInternalCVar::InternalSetInteger( const int newValue ) {
	Set( anStr( newValue ), true, false );
}

/*
============
anInternalCVar::InternalSetFloat
============
*/
void anInternalCVar::InternalSetFloat( const float newValue ) {
	Set( anStr( newValue ), true, false );
}

/*
============
anInternalCVar::InternalSetFloat
============
*/
void anInternalCVar::InternalSetByte( const byte *newValue ) {
	Set( anStr( newValue ), true, false );
}

/*
===============================================================================

	anCVarSysLocal

===============================================================================
*/

class anCVarSysLocal : public anCVarSystem {
public:
							anCVarSysLocal();

	virtual					~anCVarSysLocal() {}

	virtual void			Init();
	virtual void			Shutdown();
	virtual bool			IsInitialized() const;

	virtual void			Register( anCVarSystem *cvar );

	virtual anCVarSystem			*Find( const char *name );

	virtual void			SetCVarString( const char *name, const char *value, int flags = 0 );
	virtual void			SetCVarBool( const char *name, const bool value, int flags = 0 );
	virtual void			SetCVarInteger( const char *name, const int value, int flags = 0 );
	virtual void			SetCVarFloat( const char *name, const float value, int flags = 0 );

	virtual const char		*GetCVarString( const char *name ) const;
	virtual bool			GetCVarBool( const char *name ) const;
	virtual int				GetCVarInteger( const char *name ) const;
	virtual float			GetCVarFloat( const char *name ) const;

	virtual bool			Command( const anCommandArgs &args );

	virtual void			CommandCompletion( void(*callback)( const char *s ) );
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) );

	virtual void			SetModifiedFlags( int flags );
	virtual int				GetModifiedFlags() const;
	virtual void			ClearModifiedFlags( int flags );

	virtual void			ResetFlaggedVariables( int flags );
	virtual void			RemoveFlaggedAutoCompletion( int flags );
	virtual void			WriteFlaggedVariables( int flags, const char *setCmd, anFile *f ) const;

	virtual void			MoveCVarsToDict( int flags, anDict & dict, bool onlyModified ) const;
	virtual void			SetCVarsFromDict( const anDict &dict );

	void					RegisterInternal( anCVarSystem *cvar );
	anInternalCVar		*FindInternal( const char *name ) const;
	void					SetInternal( const char *name, const char *value, int flags );

private:
	bool					initialized;
	anList<anInternalCVar*, TAG_CVAR>	cvars;
	anHashIndex				cvarHash;
	int						modifiedFlags;

private:
	static void				Toggle_f( const anCommandArgs &args );
	static void				Set_f( const anCommandArgs &args );
	static void				Reset_f( const anCommandArgs &args );
	static void				ListByFlags( const anCommandArgs &args, cvarFlags_t flags );
	static void				List_f( const anCommandArgs &args );
	static void				Restart_f( const anCommandArgs &args );
	static void				CvarAdd_f( const anCommandArgs &args );
};

anCVarSysLocal			localCVarSystem;
anCVarSystem *				cvarSystem = &localCVarSystem;

#define NUM_COLUMNS				77		// 78 - 1
#define NUM_NAME_CHARS			33
#define NUM_DESCRIPTION_CHARS	( NUM_COLUMNS - NUM_NAME_CHARS )
#define FORMAT_STRING			"%-32s "

const char *CreateColumn( const char *text, int columnWidth, const char *indent, anStr &string ) {
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
anCVarSysLocal::FindInternal
============
*/
anInternalCVar *anCVarSysLocal::FindInternal( const char *name ) const {
	int hash = cvarHash.GenerateKey( name, false );
	for ( int i = cvarHash.First( hash ); i != -1; i = cvarHash.Next( i ) ) {
		if ( cvars[i]->nameString.Icmp( name ) == 0 ) {
			return cvars[i];
		}
	}
	return nullptr;
}

/*
============
anCVarSysLocal::SetInternal
============
*/
void anCVarSysLocal::SetInternal( const char *name, const char *value, int flags ) {
	anInternalCVar *internal = FindInternal( name );

	if ( internal ) {
		internal->InternalSetString( value );
		internal->flags |= flags & ~CVAR_STATIC;
		internal->UpdateCheat();
	} else {
		internal = new (TAG_SYSTEM) anInternalCVar( name, value, flags );
		int hash = cvarHash.GenerateKey( internal->nameString.c_str(), false );
		cvarHash.Add( hash, cvars.Append( internal ) );
	}
}

/*
============
anCVarSysLocal::anCVarSysLocal
============
*/
anCVarSysLocal::anCVarSysLocal() {
	initialized = false;
	modifiedFlags = 0;
}

/*
============
anCVarSysLocal::Init
============
*/
void anCVarSysLocal::Init() {
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
anCVarSysLocal::Shutdown
============
*/
void anCVarSysLocal::Shutdown() {
	cvars.DeleteContents( true );
	cvarHash.Free();
	initialized = false;
}

/*
============
anCVarSysLocal::IsInitialized
============
*/
bool anCVarSysLocal::IsInitialized() const {
	return initialized;
}

/*
============
anCVarSysLocal::Register
============
*/
void anCVarSysLocal::Register( anCVarSystem *cvar ) {
	cvar->SetInternalVar( cvar );

	anInternalCVar *internal = FindInternal( cvar->GetName() );

	if ( internal ) {
		internal->Update( cvar );
	} else {
		internal = new (TAG_SYSTEM) anInternalCVar( cvar );
		int hash = cvarHash.GenerateKey( internal->nameString.c_str(), false );
		cvarHash.Add( hash, cvars.Append( internal ) );
	}

	cvar->SetInternalVar( internal );
}

/*
============
anCVarSysLocal::Find
============
*/
anCVarSystem *anCVarSysLocal::Find( const char *name ) {
	return FindInternal( name );
}

/*
============
anCVarSysLocal::SetCVarString
============
*/
void anCVarSysLocal::SetCVarString( const char *name, const char *value, int flags ) {
	SetInternal( name, value, flags );
}

/*
============
anCVarSysLocal::SetCVarBool
============
*/
void anCVarSysLocal::SetCVarBool( const char *name, const bool value, int flags ) {
	SetInternal( name, anStr( value ), flags );
}

/*
============
anCVarSysLocal::SetCVarInteger
============
*/
void anCVarSysLocal::SetCVarInteger( const char *name, const int value, int flags ) {
	SetInternal( name, anStr( value ), flags );
}

/*
============
anCVarSysLocal::SetCVarFloat
============
*/
void anCVarSysLocal::SetCVarFloat( const char *name, const float value, int flags ) {
	SetInternal( name, anStr( value ), flags );
}

/*
============
anCVarSysLocal::GetCVarString
============
*/
const char *anCVarSysLocal::GetCVarString( const char *name ) const {
	anInternalCVar *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetString();
	}
	return "";
}

/*
============
anCVarSysLocal::GetCVarBool
============
*/
bool anCVarSysLocal::GetCVarBool( const char *name ) const {
	anInternalCVar *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetBool();
	}
	return false;
}

/*
============
anCVarSysLocal::GetCVarInteger
============
*/
int anCVarSysLocal::GetCVarInteger( const char *name ) const {
	anInternalCVar *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetInteger();
	}
	return 0;
}

/*
============
anCVarSysLocal::GetCVarFloat
============
*/
float anCVarSysLocal::GetCVarFloat( const char *name ) const {
	anInternalCVar *internal = FindInternal( name );
	if ( internal ) {
		return internal->GetFloat();
	}
	return 0.0f;
}

/*
============
anCVarSysLocal::Command
============
*/
bool anCVarSysLocal::Command( const anCommandArgs &args ) {
	anInternalCVar *internal = FindInternal( args.Argv( 0 ) );

	if ( internal == nullptr ) {
		return false;
	}

	if ( args.Argc() == 1 ) {
		// print the variable
		common->Printf( "\"%s\" is:\"%s\"" S_COLOR_WHITE " default:\"%s\"\n", internal->nameString.c_str(), internal->valueString.c_str(), internal->resetString.c_str() );
		if ( anStr::Length( internal->GetDescription() ) > 0 ) {
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
anCVarSysLocal::CommandCompletion
============
*/
void anCVarSysLocal::CommandCompletion( void(*callback)( const char *s ) ) {
	for ( int i = 0; i < cvars.Num(); i++ ) {
		callback( cvars[i]->GetName() );
	}
}

/*
============
anCVarSysLocal::ArgCompletion
============
*/
void anCVarSysLocal::ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) {
	anCommandArgs args.TokenizeString( cmdString, false );

	for ( int i = 0; i < cvars.Num(); i++ ) {
		if ( !cvars[i]->valueCompletion ) {
			continue;
		}
		if ( anStr::Icmp( args.Argv( 0 ), cvars[i]->nameString.c_str() ) == 0 ) {
			cvars[i]->valueCompletion( args, callback );
			break;
		}
	}
}

/*
============
anCVarSysLocal::SetModifiedFlags
============
*/
void anCVarSysLocal::SetModifiedFlags( int flags ) {
	modifiedFlags |= flags;
}

/*
============
anCVarSysLocal::GetModifiedFlags
============
*/
int anCVarSysLocal::GetModifiedFlags() const {
	return modifiedFlags;
}

/*
============
anCVarSysLocal::ClearModifiedFlags
============
*/
void anCVarSysLocal::ClearModifiedFlags( int flags ) {
	modifiedFlags &= ~flags;
}

/*
============
anCVarSysLocal::ResetFlaggedVariables
============
*/
void anCVarSysLocal::ResetFlaggedVariables( int flags ) {
	for ( int i = 0; i < cvars.Num(); i++ ) {
		anInternalCVar *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			cvar->Set( nullptr, true, true );
		}
	}
}

/*
============
anCVarSysLocal::RemoveFlaggedAutoCompletion
============
*/
void anCVarSysLocal::RemoveFlaggedAutoCompletion( int flags ) {
	for ( int i = 0; i < cvars.Num(); i++ ) {
		anInternalCVar *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			cvar->valueCompletion = nullptr;
		}
	}
}

/*
============
anCVarSysLocal::WriteFlaggedVariables

Appends lines containing "set variable value" for all variables
with the "flags" flag set to true.
============
*/
void anCVarSysLocal::WriteFlaggedVariables( int flags, const char *setCmd, anFile *f ) const {
	for ( int i = 0; i < cvars.Num(); i++ ) {
		anInternalCVar *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			f->Printf( "%s %s \"%s\"\n", setCmd, cvar->GetName(), cvar->GetString() );
		}
	}
}

/*
============
anCVarSysLocal::MoveCVarsToDict
============
*/
void anCVarSysLocal::MoveCVarsToDict( int flags, anDict & dict, bool onlyModified ) const {
	dict.Clear();
	for ( int i = 0; i < cvars.Num(); i++ ) {
		anCVarSystem *cvar = cvars[i];
		if ( cvar->GetFlags() & flags ) {
			if ( onlyModified && anStr::Icmp( cvar->GetString(), cvar->GetDefaultString() ) == 0 ) {
				continue;
			}
			dict.Set( cvar->GetName(), cvar->GetString() );
		}
	}
}

/*
============
anCVarSysLocal::SetCVarsFromDict
============
*/
void anCVarSysLocal::SetCVarsFromDict( const anDict &dict ) {
	for ( int i = 0; i < dict.GetNumKeyVals(); i++ ) {
		const anKeyValue *kv = dict.GetKeyVal( i );
		anInternalCVar *internal = FindInternal( kv->GetKey() );
		if ( internal ) {
			internal->InternalServerSetString( kv->GetValue() );
		}
	}
}

/*
============
anCVarSysLocal::Toggle_f
============
*/
void anCVarSysLocal::Toggle_f( const anCommandArgs &args ) {
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

	anInternalCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );

	if ( cvar == nullptr ) {
		common->Warning( "Toggle_f: cvar \"%s\" not found", args.Argv( 1 ) );
		return;
	}

	if ( argc > 3 ) {
		// cycle through multiple values
		text = cvar->GetString();
		for ( int i = 2; i < argc; i++ ) {
			if ( !anStr::Icmp( text, args.Argv( i ) ) ) {
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
		cvar->Set( anStr( current ), false, false );
	}
}

/*
============
anCVarSysLocal::Set_f
============
*/
void anCVarSysLocal::Set_f( const anCommandArgs &args ) {
	const char *str = args.Args( 2, args.Argc() - 1 );
	localCVarSystem.SetCVarString( args.Argv(1 ), str );
}

/*
============
anCVarSysLocal::Reset_f
============
*/
void anCVarSysLocal::Reset_f( const anCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf ( "usage: reset <variable>\n" );
		return;
	}
	anInternalCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
	if ( !cvar ) {
		return;
	}

	cvar->Reset();
}

/*
============
anCVarSysLocal::CvarAdd_f
============
*/
void anCVarSysLocal::CvarAdd_f( const anCommandArgs &args ) {
	if ( args.Argc() != 3 ) {
		common->Printf ( "usage: cvarAdd <variable> <value>\n" );
	}

	anInternalCVar *cvar = localCVarSystem.FindInternal( args.Argv( 1 ) );
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
//anSortCommandDef
//================================================
//*/
//class anSortInternalCVar : public anSortQuick< const anInternalCVar *, anSortInternalCVar > {
//public:
//	int Compare( const anInternalCVar * & a, const anInternalCVar * & b ) const { return anStr::Icmp( a.GetName(), b.GetName() ); }
//};

void anCVarSysLocal::ListByFlags( const anCommandArgs &args, cvarFlags_t flags ) {
	anStr indent;
	anList<const anInternalCVar *, TAG_CVAR>cvarList;

	enum {
		SHOW_VALUE,
		SHOW_DESCRIPTION,
		SHOW_TYPE,
		SHOW_FLAGS
	} show;

	int argNum = 1;
	show = SHOW_VALUE;

	if ( anStr::Icmp( args.Argv( argNum ), "-" ) == 0 || anStr::Icmp( args.Argv( argNum ), "/" ) == 0 ) {
		if ( anStr::Icmp( args.Argv( argNum + 1 ), "help" ) == 0 || anStr::Icmp( args.Argv( argNum + 1 ), "?" ) == 0 ) {
			argNum = 3;
			show = SHOW_DESCRIPTION;
		} else if ( anStr::Icmp( args.Argv( argNum + 1 ), "type" ) == 0 || anStr::Icmp( args.Argv( argNum + 1 ), "range" ) == 0 ) {
			argNum = 3;
			show = SHOW_TYPE;
		} else if ( anStr::Icmp( args.Argv( argNum + 1 ), "flags" ) == 0 ) {
			argNum = 3;
			show = SHOW_FLAGS;
		}
	}

	if ( args.Argc() > argNum ) {
		anStr match = args.Args( argNum, -1 );
		match.Replace( " ", "" );
	} else {
		anStr match = "";
	}

	for ( int i = 0; i < localCVarSystem.cvars.Num(); i++ ) {
		const anInternalCVar *cvar = localCVarSystem.cvars[i];
		if ( !( cvar->GetFlags() & flags ) ) {
			continue;
		}

		if ( match.Length() && !cvar->nameString.Filter( match, false ) ) {
			continue;
		}

		cvarList.Append( cvar );
	}

	//cvarList.SortWithTemplate( anSortInternalCVar() );

	switch ( show ) {
		case SHOW_VALUE: {
			for ( i = 0; i < cvarList.Num(); i++ ) {
				const anInternalCVar *cvar = cvarList[i];
				common->Printf( FORMAT_STRING S_COLOR_WHITE "\"%s\"\n", cvar->nameString.c_str(), cvar->valueString.c_str() );
			}
			break;
		}
		case SHOW_DESCRIPTION: {
			anStr indent.Fill( ' ', NUM_NAME_CHARS );
			anStr indent.Insert( "\n", 0 );
			for ( i = 0; i < cvarList.Num(); i++ ) {
				const anInternalCVar *cvar = cvarList[i];
				common->Printf( FORMAT_STRING S_COLOR_WHITE "%s\n", cvar->nameString.c_str(), CreateColumn( cvar->GetDescription(), NUM_DESCRIPTION_CHARS, indent, string ) );
			}
			break;
		}
		case SHOW_TYPE: {
			for ( i = 0; i < cvarList.Num(); i++ ) {
			const anInternalCVar *cvar = cvarList[i];
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
						common->Printf( FORMAT_STRING S_COLOR_RED "float " S_COLOR_WHITE "[%s, %s]\n", cvar->GetName(), anStr( cvar->GetMinValue() ).c_str(), anStr( cvar->GetMaxValue() ).c_str() );
					} else {
						common->Printf( FORMAT_STRING S_COLOR_RED "float\n", cvar->GetName() );
					}
				} else if ( cvar->GetValueStrings() ) {
					common->Printf( FORMAT_STRING S_COLOR_WHITE "string " S_COLOR_WHITE "[", cvar->GetName() );
					for ( int j = 0; cvar->GetValueStrings()[j] != nullptr; j++ ) {
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
				const anInternalCVar *cvar = cvarList[i];
				common->Printf( FORMAT_STRING, cvar->GetName() );
				anStr string = "";
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
anCVarSysLocal::List_f
============
*/
void anCVarSysLocal::List_f( const anCommandArgs &args ) {
	ListByFlags( args, CVAR_ALL );
}

/*
============
anCVarSysLocal::Restart_f
============
*/
void anCVarSysLocal::Restart_f( const anCommandArgs &args ) {
	for ( int i = 0; i < localCVarSystem.cvars.Num(); i++ ) {
	anInternalCVar *cvar = localCVarSystem.cvars[i];
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
