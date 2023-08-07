#ifndef __CVARSYSTEM_H__
#define __CVARSYSTEM_H__

/*
===============================================================================

	Console Variables (CVars) are used to hold scalar or string variables
	that can be changed or displayed at the console as well as accessed
	directly in code.

	CVars are mostly used to hold settings that can be changed from the
	console or saved to and loaded from configuration files. CVars are also
	occasionally used to communicate information between different modules
	of the program.

	CVars are restricted from having the same names as console commands to
	keep the console interface from being ambiguous.

	CVars can be accessed from the console in three ways:
	cvarName			prints the current value
	cvarName X			sets the value to X if the variable exists
	set cvarName X		as above, but creates the CVar if not present

	CVars may be declared in the global namespace, in classes and in functions.
	However declarations in classes and functions should always be static to
	save space and time. Making CVars static does not change their
	functionality due to their global nature.

	CVars should be contructed only through one of the constructors with name,
	value, flags and description. The name, value and description parameters
	to the constructor have to be static strings, do not use va() or the like
	functions returning a string.

	CVars may be declared multiple times using the same name string. However,
	they will all reference the same value and changing the value of one CVar
	changes the value of all CVars with the same name.

	CVars should always be declared with the correct type flag: CVAR_BOOL,
	CVAR_INTEGER or CVAR_FLOAT. If no such flag is specified the CVar
	defaults to type string. If the CVAR_BOOL flag is used there is no need
	to specify an argument auto-completion function because the CVar gets
	one assigned automatically.

	CVars are automatically range checked based on their type and any min/max
	or valid string set specified in the constructor.

	CVars are always considered cheats except when CVAR_NOCHEAT, CVAR_INIT,
	CVAR_ROM, CVAR_ARCHIVE, CVAR_SERVERINFO, CVAR_NETWORKSYNC
	is set.

===============================================================================
*/

typedef enum {
	CVAR_ALL				= -1,		// all flags
	CVAR_BOOL				= BIT(0 ),	// variable is a boolean
	CVAR_INTEGER			= BIT(1 ),	// variable is an integer
	CVAR_FLOAT				= BIT(2),	// variable is a float
	CVAR_SYSTEM				= BIT(3),	// system variable
	CVAR_RENDERER			= BIT(4),	// renderer variable
	CVAR_SOUND				= BIT(5),	// sound variable
	CVAR_GUI				= BIT(6),	// gui variable
	CVAR_GAME				= BIT(7),	// game variable
	CVAR_TOOL				= BIT(8),	// tool variable
	CVAR_STATIC				= BIT(9),	// statically declared, not user created
	CVAR_CHEAT				= BIT(10),	// variable is considered a cheat
	CVAR_NOCHEAT			= BIT(11),	// variable is not considered a cheat
	CVAR_INIT				= BIT(12),	// can only be set from the command-line
	CVAR_ROM				= BIT(13),	// display only, cannot be set by user at all
	CVAR_ARCHIVE			= BIT(14),	// set to cause it to be saved to a config file
	CVAR_MODIFIED			= BIT(15)	// set when the variable is modified
} cvarFlags_t;


/*
===============================================================================

	anCVar

===============================================================================
*/

class anCVar {
public:
							// Never use the default constructor.
							anCVar() { assert( typeid( this ) != typeid( anCVar ) ); }

							// Always use one of the following constructors.
							anCVar( const char *name, const char *value, int flags, const char *description, argCompletion_t valueCompletion = nullptr );
							anCVar( const char *name, const char *value, int flags, const char *description, float valueMin, float valueMax, argCompletion_t valueCompletion = nullptr );
							anCVar( const char *name, const char *value, int flags, const char *description, const char **valueStrings, argCompletion_t valueCompletion = nullptr );
							anCVar( const char *name, const char *value, int flags, const char *description, argCompletion_t valueCompletion = nullptr, const byte *data );

	virtual					~anCVar() {}

	const char *			GetName() const { return internalVar->name; }
	int						GetFlags() const { return internalVar->flags; }
	const char *			GetDescription() const { return internalVar->description; }
	float					GetMinValue() const { return internalVar->valueMin; }
	float					GetMaxValue() const { return internalVar->valueMax; }
	const char **			GetValueStrings() const { return valueStrings; }
	argCompletion_t			GetValueCompletion() const { return valueCompletion; }

	bool					IsModified() const { return ( internalVar->flags & CVAR_MODIFIED ) != 0; }
	void					SetModified() { internalVar->flags |= CVAR_MODIFIED; }
	void					ClearModified() { internalVar->flags &= ~CVAR_MODIFIED; }

	const char *			GetDefaultString() const { return internalVar->InternalGetResetString(); }
	const char *			GetString() const { return internalVar->value; }
	bool					GetBool() const { return ( internalVar->integerValue != 0 ); }
	int						GetInteger() const { return internalVar->integerValue; }
	float					GetFloat() const { return internalVar->floatValue; }
	byte *					GetByte() const { return internalVar->byteValue; }

	void					SetString( const char *value ) { internalVar->InternalSetString( value ); }
	void					SetBool( const bool value ) { internalVar->InternalSetBool( value ); }
	void					SetInteger( const int value ) { internalVar->InternalSetInteger( value ); }
	void					SetFloat( const float value ) { internalVar->InternalSetFloat( value ); }
	void					SetByte() { internalVar->InternalSetByte( value ); }

	void					SetInternalVar( anCVar *cvar ) { internalVar = cvar; }

	static void				RegisterStaticVars();

protected:
	const char *			name;					// name
	const char *			value;					// value
	const char *			description;			// description
	int						flags;					// CVAR_? flags
	float					valueMin;				// minimum value
	float					valueMax;				// maximum value
	const char **			valueStrings;			// valid value strings
	argCompletion_t			valueCompletion;		// value auto-completion function
	int						integerValue;			// atoi( string )
	float					floatValue;				// atof( value )
	byte *					byteValue; 				// New member variable for storing a byte value

	anCVar *	internalVar;			// internal cvar
	anCVar *	next;					// next statically declared cvar

private:
	void					Init( const char *name, const char *value, int flags, const char *description, float valueMin, float valueMax, const char **valueStrings, argCompletion_t valueCompletion );

	virtual void			InternalSetString( const char *newValue ) {}
	virtual void			InternalSetBool( const bool newValue ) {}
	virtual void			InternalSetInteger( const int newValue ) {}
	virtual void			InternalSetFloat( const float newValue ) {}
	virtual void			InternalSetByte( const *byte newValue ) {}

	virtual const char *	InternalGetResetString() const { return value; }

	static anCVar *staticVars;
};

typedef anCVar cvar;
inline anCVar::anCVar( const char *name, const char *value, int flags, const char *description, argCompletion_t valueCompletion ) {
	if ( !valueCompletion && ( flags & CVAR_BOOL ) ) {
		valueCompletion = arcCmdSystem::ArgCompletion_Boolean;
	}
	Init( name, value, flags, description, 1, -1, nullptr, valueCompletion );
}

inline anCVar::anCVar( const char *name, const char *value, int flags, const char *description, float valueMin, float valueMax, argCompletion_t valueCompletion ) {
	Init( name, value, flags, description, valueMin, valueMax, nullptr, valueCompletion );
}

inline anCVar::anCVar( const char *name, const char *value, int flags, const char *description, const char **valueStrings, argCompletion_t valueCompletion ) {
	Init( name, value, flags, description, 1, -1, valueStrings, valueCompletion );
}

inline anCVar::anCVar( const char *name, const char *value, int flags, const char *description, argCompletion_t valueCompletion, const byte *data ) {
    if ( !valueCompletion && (flags & CVAR_BYTE)) {
        valueCompletion = arcCmdSystem::ArgCompletion_byte;
    }
    Init( name, value, flags, description, 1, -1, data, valueCompletion );
}

/*
===============================================================================

	anCVarSystem

===============================================================================
*/

class anCVarSystem {
public:
	virtual					~anCVarSystem() {}

	virtual void			Init() = 0;
	virtual void			Shutdown() = 0;
	virtual bool			IsInitialized() const = 0;

							// Registers a CVar.
	virtual void			Register( anCVar *cvar ) = 0;

							// Finds the CVar with the given name.
							// Returns nullptr if there is no CVar with the given name.
	virtual anCVar *Find( const char *name ) = 0;

							// Sets the value of a CVar by name.
	virtual void			SetCVarString( const char *name, const char *value, int flags = 0 ) = 0;
	virtual void			SetCVarBool( const char *name, const bool value, int flags = 0 ) = 0;
	virtual void			SetCVarInteger( const char *name, const int value, int flags = 0 ) = 0;
	virtual void			SetCVarFloat( const char *name, const float value, int flags = 0 ) = 0;
	virtual void			SetCVarByte( const char *name, const *byte value, int flags = 0 ) = 0;

							// Gets the value of a CVar by name.
	virtual const char *	GetCVarString( const char *name ) const = 0;
	virtual bool			GetCVarBool( const char *name ) const = 0;
	virtual int				GetCVarInteger( const char *name ) const = 0;
	virtual float			GetCVarFloat( const char *name ) const = 0;
	virtual byte			GetCVarByte( const char *name ) const = 0;

							// Called by the command system when argv(0 ) doesn't match a known command.
							// Returns true if argv(0 ) is a variable reference and prints or changes the CVar.
	virtual bool			Command( const anCommandArgs &args ) = 0;

							// Command and argument completion using callback for each valid string.
	virtual void			CommandCompletion( void(*callback)( const char *s ) ) = 0;
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) = 0;

							// Sets/gets/clears modified flags that tell what kind of CVars have changed.
	virtual void			SetModifiedFlags( int flags ) = 0;
	virtual int				GetModifiedFlags() const = 0;
	virtual void			ClearModifiedFlags( int flags ) = 0;

							// Resets variables with one of the given flags set.
	virtual void			ResetFlaggedVariables( int flags ) = 0;

							// Removes auto-completion from the flagged variables.
	virtual void			RemoveFlaggedAutoCompletion( int flags ) = 0;

							// Writes variables with one of the given flags set to the given file.
	virtual void			WriteFlaggedVariables( int flags, const char *setCmd, anFile *f ) const = 0;

							// Moves CVars to and from dictionaries.
	virtual void			MoveCVarsToDict( int flags, anDict & dict, bool onlyModified = false ) const = 0;
	virtual void			SetCVarsFromDict( const anDict &dict ) = 0;
};

extern anCVarSystem *		cvarSystem;

/*
===============================================================================

	CVar Registration

	Each DLL using CVars has to declare a private copy of the static variable
	anCVar::staticVars like this: anCVar * anCVar::staticVars = nullptr;
	Furthermore anCVar::RegisterStaticVars() has to be called after the
	cvarSystem pointer is set when the DLL is first initialized.

===============================================================================
*/

inline void anCVar::Init( const char *name, const char *value, int flags, const char *description, float valueMin, float valueMax, const char **valueStrings, argCompletion_t valueCompletion ) {
	this->name = name;
	this->value = value;
	this->flags = flags;
	this->description = description;
	this->flags = flags | CVAR_STATIC;
	this->valueMin = valueMin;
	this->valueMax = valueMax;
	this->valueStrings = valueStrings;
	this->valueCompletion = valueCompletion;
	this->integerValue = 0;
	this->floatValue = 0.0f;
	this->byteValue = 0;
	this->internalVar = this;
	if ( staticVars != (anCVar *)0xFFFFFFFF ) {
		this->next = staticVars;
		staticVars = this;
	} else {
		cvarSystem->Register( this );
	}
}

inline void anCVar::RegisterStaticVars() {
	if ( staticVars != (anCVar *)0xFFFFFFFF ) {
		for ( anCVar *cvar = staticVars; cvar; cvar = cvar->next ) {
			cvarSystem->Register( cvar );
		}
		staticVars = (anCVar *)0xFFFFFFFF;
	}
}

#endif // !__CVARSYSTEM_H__
