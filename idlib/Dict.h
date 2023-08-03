#ifndef __DICT_H__
#define __DICT_H__

/*
===============================================================================

Key/value dictionary

This is a dictionary class that tracks an arbitrary number of key / value
pair combinations. It is used for map entity spawning, GUI state management,
and other things.

Keys are compared case-insensitive.

Does not allocate memory until the first key/value pair is added.

===============================================================================
*/

class anKeyValue {
	friend class anDict;

public:
	const anString &	GetKey( void ) const { return *key; }
	const anString &	GetValue( void ) const { return *value; }

	size_t				Allocated( void ) const { return key->Allocated() + value->Allocated(); }
	size_t				Size( void ) const { return sizeof( *this ) + key->Size() + value->Size(); }

	bool				operator==( const anKeyValue &kv ) const { return ( key == kv.key && value == kv.value ); }

private:
	const ARCPoolString *	key;
	const ARCPoolString *	value;
};

class anDict {
public:
						anDict( void );
						anDict( const anDict &other );	// allow declaration with assignment
						~anDict( void );

						// set the granularity for the index
	void				SetGranularity( int granularity );
						// set hash size
	void				SetHashSize( int hashSize );
						// clear existing key/value pairs and copy all key/value pairs from other
	anDict &		operator=( const anDict &other );
						// copy from other while leaving existing key/value pairs in place
	void				Copy( const anDict &other );
						// clear existing key/value pairs and transfer key/value pairs from other
	void				TransferKeyValues( anDict &other );
						// parse dict from parser
	bool				Parse( anParser &parser );
						// copy key/value pairs from other dict not present in this dict
	void				SetDefaults( const anDict *dict );
						// clear dict freeing up memory
	void				Clear( void );
						// print the dict
	void				Print() const;

	size_t				Allocated( void ) const;
	size_t				Size( void ) const { return sizeof( *this ) + Allocated(); }

	void				Set( const char *key, const char *value );
	void				SetFloat( const char *key, float val );
	void				SetInt( const char *key, int val );
	void				SetBool( const char *key, bool val );
	void				SetVector( const char *key, const anVec3 &val );
	void				SetVec2( const char *key, const anVec2 &val );
	void				SetVec4( const char *key, const anVec4 &val );
	void				SetAngles( const char *key, const anAngles &val );
	void				SetMatrix( const char *key, const anMat3 &val );

						// these return default values of 0.0, 0 and false
	const char *		GetString( const char *key, const char *defaultString = "" ) const;
	float				GetFloat( const char *key, const char *defaultString = "0" ) const;
	int					GetInt( const char *key, const char *defaultString = "0" ) const;
	bool				GetBool( const char *key, const char *defaultString = "0" ) const;
	anVec3				GetVector( const char *key, const char *defaultString = nullptr ) const;
	anVec2				GetVec2( const char *key, const char *defaultString = nullptr ) const;
	anVec4				GetVec4( const char *key, const char *defaultString = nullptr ) const;
	anAngles			GetAngles( const char *key, const char *defaultString = nullptr ) const;
	anMat3				GetMatrix( const char *key, const char *defaultString = nullptr ) const;

	bool				GetString( const char *key, const char *defaultString, const char **out ) const;
	bool				GetString( const char *key, const char *defaultString, anString &out ) const;
	bool				GetFloat( const char *key, const char *defaultString, float &out ) const;
	bool				GetInt( const char *key, const char *defaultString, int &out ) const;
	bool				GetBool( const char *key, const char *defaultString, bool &out ) const;
	bool				GetVector( const char *key, const char *defaultString, anVec3 &out ) const;
	bool				GetVec2( const char *key, const char *defaultString, anVec2 &out ) const;
	bool				GetVec4( const char *key, const char *defaultString, anVec4 &out ) const;
	bool				GetAngles( const char *key, const char *defaultString, anAngles &out ) const;
	bool				GetMatrix( const char *key, const char *defaultString, anMat3 &out ) const;

	int					GetNumKeyVals( void ) const;
	const anKeyValue *	GetKeyVal( int index ) const;
						// returns the key/value pair with the given key
						// returns nullptr if the key/value pair does not exist
	const anKeyValue *	FindKey( const char *key ) const;
						// returns the index to the key/value pair with the given key
						// returns -1 if the key/value pair does not exist
	int					FindKeyIndex( const char *key ) const;
						// delete the key/value pair with the given key
	void				Delete( const char *key );
						// finds the next key/value pair with the given key prefix.
						// lastMatch can be used to do additional searches past the first match.
	const anKeyValue *	MatchPrefix( const char *prefix, const anKeyValue *lastMatch = nullptr ) const;
						// randomly chooses one of the key/value pairs with the given key prefix and returns it's value
	const char *		RandomPrefix( const char *prefix, arcRandom &random ) const;

	void				WriteToFileHandle( anFile *f ) const;
	void				ReadFromFileHandle( anFile *f );

						// returns a unique checksum for this dictionary's content
	int					Checksum( void ) const;

	static void			Init( void );
	static void			Shutdown( void );

	static void			ShowMemoryUsage_f( const anCommandArgs &args );
	static void			ListKeys_f( const anCommandArgs &args );
	static void			ListValues_f( const anCommandArgs &args );

private:
	anList<anKeyValue>	args;
	anHashIndex			argHash;

	static anStringPool	globalKeys;
	static anStringPool	globalValues;
};


ARC_INLINE anDict::anDict( void ) {
	args.SetGranularity( 16 );
	argHash.SetGranularity( 16 );
	argHash.Clear( 128, 16 );
}

ARC_INLINE anDict::anDict( const anDict &other ) {
	*this = other;
}

ARC_INLINE anDict::~anDict( void ) {
	Clear();
}

ARC_INLINE void anDict::SetGranularity( int granularity ) {
	args.SetGranularity( granularity );
	argHash.SetGranularity( granularity );
}

ARC_INLINE void anDict::SetHashSize( int hashSize ) {
	if ( args.Num() == 0 ) {
		argHash.Clear( hashSize, 16 );
	}
}

ARC_INLINE void anDict::SetFloat( const char *key, float val ) {
	Set( key, va( "%f", val ) );
}

ARC_INLINE void anDict::SetInt( const char *key, int val ) {
	Set( key, va( "%i", val ) );
}

ARC_INLINE void anDict::SetBool( const char *key, bool val ) {
	Set( key, va( "%i", val ) );
}

ARC_INLINE void anDict::SetVector( const char *key, const anVec3 &val ) {
	Set( key, val.ToString() );
}

ARC_INLINE void anDict::SetVec4( const char *key, const anVec4 &val ) {
	Set( key, val.ToString() );
}

ARC_INLINE void anDict::SetVec2( const char *key, const anVec2 &val ) {
	Set( key, val.ToString() );
}

ARC_INLINE void anDict::SetAngles( const char *key, const anAngles &val ) {
	Set( key, val.ToString() );
}

ARC_INLINE void anDict::SetMatrix( const char *key, const anMat3 &val ) {
	Set( key, val.ToString() );
}

ARC_INLINE bool anDict::GetString( const char *key, const char *defaultString, const char **out ) const {
	const anKeyValue *kv = FindKey( key );
	if ( kv ) {
		*out = kv->GetValue();
		return true;
	}
	*out = defaultString;
	return false;
}

ARC_INLINE bool anDict::GetString( const char *key, const char *defaultString, anString &out ) const {
	const anKeyValue *kv = FindKey( key );
	if ( kv ) {
		out = kv->GetValue();
		return true;
	}
	out = defaultString;
	return false;
}

ARC_INLINE const char *anDict::GetString( const char *key, const char *defaultString ) const {
	const anKeyValue *kv = FindKey( key );
	if ( kv ) {
		return kv->GetValue();
	}
	return defaultString;
}

ARC_INLINE float anDict::GetFloat( const char *key, const char *defaultString ) const {
	return atof( GetString( key, defaultString ) );
}

ARC_INLINE int anDict::GetInt( const char *key, const char *defaultString ) const {
	return atoi( GetString( key, defaultString ) );
}

ARC_INLINE bool anDict::GetBool( const char *key, const char *defaultString ) const {
	return ( atoi( GetString( key, defaultString ) ) != 0 );
}

ARC_INLINE anVec3 anDict::GetVector( const char *key, const char *defaultString ) const {
	anVec3 out;
	GetVector( key, defaultString, out );
	return out;
}

ARC_INLINE anVec2 anDict::GetVec2( const char *key, const char *defaultString ) const {
	anVec2 out;
	GetVec2( key, defaultString, out );
	return out;
}

ARC_INLINE anVec4 anDict::GetVec4( const char *key, const char *defaultString ) const {
	anVec4 out;
	GetVec4( key, defaultString, out );
	return out;
}

ARC_INLINE anAngles anDict::GetAngles( const char *key, const char *defaultString ) const {
	anAngles out;
	GetAngles( key, defaultString, out );
	return out;
}

ARC_INLINE anMat3 anDict::GetMatrix( const char *key, const char *defaultString ) const {
	anMat3 out;
	GetMatrix( key, defaultString, out );
	return out;
}

ARC_INLINE int anDict::GetNumKeyVals( void ) const {
	return args.Num();
}

ARC_INLINE const anKeyValue *anDict::GetKeyVal( int index ) const {
	if ( index >= 0 && index < args.Num() ) {
		return &args[index];
	}
	return nullptr;
}

#endif /* !__DICT_H__ */
