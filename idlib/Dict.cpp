#include "Lib.h"
#pragma hdrstop

anStringPool		anDict::globalKeys;
anStringPool		anDict::globalValues;

/*
================
anDict::operator=

  clear existing key/value pairs and copy all key/value pairs from other
================
*/
anDict &anDict::operator=( const anDict &other ) {
	int i;

	// check for assignment to self
	if ( this == &other ) {
		return *this;
	}

	Clear();

	args = other.args;
	argHash = other.argHash;

	for ( i = 0; i < args.Num(); i++ ) {
		args[i].key = globalKeys.CopyString( args[i].key );
		args[i].value = globalValues.CopyString( args[i].value );
	}

	return *this;
}

/*
================
anDict::Copy

  copy all key value pairs without removing existing key/value pairs not present in the other dict
================
*/
void anDict::Copy( const anDict &other ) {
	int i, n, *found;
	anKeyValue kv;

	// check for assignment to self
	if ( this == &other ) {
		return;
	}

	n = other.args.Num();

	if ( args.Num() ) {
		found = ( int*) _alloca16( other.args.Num() * sizeof( int ) );
        for ( i = 0; i < n; i++ ) {
			found[i] = FindKeyIndex( other.args[i].GetKey() );
		}
	} else {
		found = nullptr;
	}

	for ( i = 0; i < n; i++ ) {
		if ( found && found[i] != -1 ) {
			// first set the new value and then free the old value to allow proper self copying
			const ARCPoolString *oldValue = args[found[i]].value;
			args[found[i]].value = globalValues.CopyString( other.args[i].value );
			globalValues.FreeString( oldValue );
		} else {
			kv.key = globalKeys.CopyString( other.args[i].key );
			kv.value = globalValues.CopyString( other.args[i].value );
			argHash.Add( argHash.GenerateKey( kv.GetKey(), false ), args.Append( kv ) );
		}
	}
}

/*
================
anDict::TransferKeyValues

  clear existing key/value pairs and transfer key/value pairs from other
================
*/
void anDict::TransferKeyValues( anDict &other ) {
	int i, n;

	if ( this == &other ) {
		return;
	}

	if ( other.args.Num() && other.args[0].key->GetPool() != &globalKeys ) {
		common->FatalError( "anDict::TransferKeyValues: can't transfer values across a DLL boundary" );
		return;
	}

	Clear();

	n = other.args.Num();
	args.SetNum( n );
	for ( i = 0; i < n; i++ ) {
		args[i].key = other.args[i].key;
		args[i].value = other.args[i].value;
	}
	argHash = other.argHash;

	other.args.Clear();
	other.argHash.Free();
}

/*
================
anDict::Parse
================
*/
bool anDict::Parse( anParser &parser ) {
	anToken	token;
	anToken	token2;

	bool errors = false;

	parser.ExpectTokenString( "{" );
	parser.ReadToken( &token );
	while ( ( token.type != TT_PUNCTUATION ) || ( token != "}" ) ) {
		if ( token.type != TT_STRING ) {
			parser.Error( "Expected quoted string, but found '%s'", token.c_str() );
		}

		if ( !parser.ReadToken( &token2 ) ) {
			parser.Error( "Unexpected end of file" );
		}

		if ( FindKey( token ) ) {
			parser.Warning( "'%s' already defined", token.c_str() );
			errors = true;
		}
		Set( token, token2 );

		if ( !parser.ReadToken( &token ) ) {
			parser.Error( "Unexpected end of file" );
		}
	}

	return !errors;
}

/*
================
anDict::SetDefaults
================
*/
void anDict::SetDefaults( const anDict *dict ) {
	int i, n;
	const anKeyValue *kv, *def;
	anKeyValue newkv;

	n = dict->args.Num();
	for ( i = 0; i < n; i++ ) {
		def = &dict->args[i];
		kv = FindKey( def->GetKey() );
		if ( !kv ) {
			newkv.key = globalKeys.CopyString( def->key );
			newkv.value = globalValues.CopyString( def->value );
			argHash.Add( argHash.GenerateKey( newkv.GetKey(), false ), args.Append( newkv ) );
		}
	}
}

/*
================
anDict::Clear
================
*/
void anDict::Clear( void ) {
	int i;

	for ( i = 0; i < args.Num(); i++ ) {
		globalKeys.FreeString( args[i].key );
		globalValues.FreeString( args[i].value );
	}

	args.Clear();
	argHash.Free();
}

/*
================
anDict::Print
================
*/
void anDict::Print() const {
	int i;
	int n;

	n = args.Num();
	for ( i = 0; i < n; i++ ) {
		anLibrary::common->Printf( "%s = %s\n", args[i].GetKey().c_str(), args[i].GetValue().c_str() );
	}
}

int KeyCompare( const anKeyValue *a, const anKeyValue *b ) {
	return anString::Cmp( a->GetKey(), b->GetKey() );
}

/*
================
anDict::Checksum
================
*/
int	anDict::Checksum( void ) const {
	unsigned long ret;
	int i, n;

	anList<anKeyValue> sorted = args;
	sorted.Sort( KeyCompare );
	n = sorted.Num();
	CRC32_InitChecksum( ret );
	for ( i = 0; i < n; i++ ) {
		CRC32_UpdateChecksum( ret, sorted[i].GetKey().c_str(), sorted[i].GetKey().Length() );
		CRC32_UpdateChecksum( ret, sorted[i].GetValue().c_str(), sorted[i].GetValue().Length() );
	}
	CRC32_FinishChecksum( ret );
	return ret;
}

/*
================
anDict::Allocated
================
*/
size_t anDict::Allocated( void ) const {
	int		i;
	size_t	size;

	size = args.Allocated() + argHash.Allocated();
	for ( i = 0; i < args.Num(); i++ ) {
		size += args[i].Size();
	}

	return size;
}

/*
================
anDict::Set
================
*/
void anDict::Set( const char *key, const char *value ) {
	int i;
	anKeyValue kv;

	if ( key == nullptr || key[0] == '\0' ) {
		return;
	}

	i = FindKeyIndex( key );
	if ( i != -1 ) {
		// first set the new value and then free the old value to allow proper self copying
		const ARCPoolString *oldValue = args[i].value;
		args[i].value = globalValues.AllocString( value );
		globalValues.FreeString( oldValue );
	} else {
		kv.key = globalKeys.AllocString( key );
		kv.value = globalValues.AllocString( value );
		argHash.Add( argHash.GenerateKey( kv.GetKey(), false ), args.Append( kv ) );
	}
}

/*
================
anDict::GetFloat
================
*/
bool anDict::GetFloat( const char *key, const char *defaultString, float &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = atof( s );
	return found;
}

/*
================
anDict::GetInt
================
*/
bool anDict::GetInt( const char *key, const char *defaultString, int &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = atoi( s );
	return found;
}

/*
================
anDict::GetBool
================
*/
bool anDict::GetBool( const char *key, const char *defaultString, bool &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = ( atoi( s ) != 0 );
	return found;
}

/*
================
anDict::GetAngles
================
*/
bool anDict::GetAngles( const char *key, const char *defaultString, anAngles &out ) const {
	bool		found;
	const char	*s;

	if ( !defaultString ) {
		defaultString = "0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f %f", &out.pitch, &out.yaw, &out.roll );
	return found;
}

/*
================
anDict::GetVector
================
*/
bool anDict::GetVector( const char *key, const char *defaultString, anVec3 &out ) const {
	bool		found;
	const char	*s;

	if ( !defaultString ) {
		defaultString = "0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f %f", &out.x, &out.y, &out.z );
	return found;
}

/*
================
anDict::GetVec2
================
*/
bool anDict::GetVec2( const char *key, const char *defaultString, anVec2 &out ) const {
	bool		found;
	const char	*s;

	if ( !defaultString ) {
		defaultString = "0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f", &out.x, &out.y );
	return found;
}

/*
================
anDict::GetVec4
================
*/
bool anDict::GetVec4( const char *key, const char *defaultString, anVec4 &out ) const {
	bool		found;
	const char	*s;

	if ( !defaultString ) {
		defaultString = "0 0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f %f %f", &out.x, &out.y, &out.z, &out.w );
	return found;
}

/*
================
anDict::GetMatrix
================
*/
bool anDict::GetMatrix( const char *key, const char *defaultString, anMat3 &out ) const {
	const char	*s;
	bool		found;

	if ( !defaultString ) {
		defaultString = "1 0 0 0 1 0 0 0 1";
	}

	found = GetString( key, defaultString, &s );
	out.Identity();		// sccanf has a bug in it on Mac OS 9.  Sigh.
	sscanf( s, "%f %f %f %f %f %f %f %f %f", &out[0].x, &out[0].y, &out[0].z, &out[1].x, &out[1].y, &out[1].z, &out[2].x, &out[2].y, &out[2].z );
	return found;
}

/*
================
WriteString
================
*/
static void WriteString( const char *s, anFile *f ) {
	int	len = strlen( s );
	if ( len >= MAX_STRING_CHARS-1 ) {
		anLibrary::common->Error( "anDict::WriteToFileHandle: bad string" );
	}
	f->Write( s, strlen( s) + 1 );
}

/*
================
anDict::FindKey
================
*/
const anKeyValue *anDict::FindKey( const char *key ) const {
	int i, hash;

	if ( key == nullptr || key[0] == '\0' ) {
		anLibrary::common->DWarning( "anDict::FindKey: empty key" );
		return nullptr;
	}

	hash = argHash.GenerateKey( key, false );
	for ( i = argHash.First( hash ); i != -1; i = argHash.Next( i ) ) {
		if ( args[i].GetKey().Icmp( key ) == 0 ) {
			return &args[i];
		}
	}

	return nullptr;
}

/*
================
anDict::FindKeyIndex
================
*/
int anDict::FindKeyIndex( const char *key ) const {
	if ( key == nullptr || key[0] == '\0' ) {
		anLibrary::common->DWarning( "anDict::FindKeyIndex: empty key" );
		return nullptr;
	}

	int hash = argHash.GenerateKey( key, false );
	for ( int i = argHash.First( hash ); i != -1; i = argHash.Next( i ) ) {
		if ( args[i].GetKey().Icmp( key ) == 0 ) {
			return i;
		}
	}

	return -1;
}

/*
================
anDict::Delete
================
*/
void anDict::Delete( const char *key ) {
	int hash, i;

	hash = argHash.GenerateKey( key, false );
	for ( i = argHash.First( hash ); i != -1; i = argHash.Next( i ) ) {
		if ( args[i].GetKey().Icmp( key ) == 0 ) {
			globalKeys.FreeString( args[i].key );
			globalValues.FreeString( args[i].value );
			args.RemoveIndex( i );
			argHash.RemoveIndex( hash, i );
			break;
		}
	}

#if 0
	// make sure all keys can still be found in the hash index
	for ( i = 0; i < args.Num(); i++ ) {
		assert( FindKey( args[i].GetKey() ) != nullptr );
	}
#endif
}

/*
================
anDict::MatchPrefix
================
*/
const anKeyValue *anDict::MatchPrefix( const char *prefix, const anKeyValue *lastMatch ) const {
	int	i;
	int len;
	int start;

	assert( prefix );
	len = strlen( prefix );

	start = -1;
	if ( lastMatch ) {
		start = args.FindIndex( *lastMatch );
		assert( start >= 0 );
		if ( start < 1 ) {
			start = 0;
		}
	}

	for ( i = start + 1; i < args.Num(); i++ ) {
		if ( !args[i].GetKey().Icmpn( prefix, len ) ) {
			return &args[i];
		}
	}
	return nullptr;
}

/*
================
anDict::RandomPrefix
================
*/
const char *anDict::RandomPrefix( const char *prefix, arcRandom &random ) const {
	int count;
	const int MAX_RANDOM_KEYS = 2048;
	const char *list[MAX_RANDOM_KEYS];
	const anKeyValue *kv;

	list[0] = "";
	for ( count = 0, kv = MatchPrefix( prefix ); kv && count < MAX_RANDOM_KEYS; kv = MatchPrefix( prefix, kv ) ) {
		list[count++] = kv->GetValue().c_str();
	}
	return list[random.RandomInt( count )];
}

/*
================
anDict::WriteToFileHandle
================
*/
void anDict::WriteToFileHandle( anFile *f ) const {
	int c = LittleLong( args.Num() );
	f->Write( &c, sizeof( c ) );
	for ( int i = 0; i < args.Num(); i++ ) {	// don't loop on the swapped count use the original
		WriteString( args[i].GetKey().c_str(), f );
		WriteString( args[i].GetValue().c_str(), f );
	}
}

/*
================
ReadString
================
*/
static anString ReadString( anFile *f ) {
	char	str[MAX_STRING_CHARS];
	int		len;

	for ( len = 0; len < MAX_STRING_CHARS; len++ ) {
		f->Read( (void *)&str[len], 1 );
		if ( str[len] == 0 ) {
			break;
		}
	}
	if ( len == MAX_STRING_CHARS ) {
		anLibrary::common->Error( "anDict::ReadFromFileHandle: bad string" );
	}

	return anString( str );
}

/*
================
anDict::ReadFromFileHandle
================
*/
void anDict::ReadFromFileHandle( anFile *f ) {
	int c;
	anString key, val;

	Clear();

	f->Read( &c, sizeof( c ) );
	c = LittleLong( c );
	for ( int i = 0; i < c; i++ ) {
		key = ReadString( f );
		val = ReadString( f );
		Set( key, val );
	}
}

/*
================
anDict::Init
================
*/
void anDict::Init( void ) {
	globalKeys.SetCaseSensitive( false );
	globalValues.SetCaseSensitive( true );
}

/*
================
anDict::Shutdown
================
*/
void anDict::Shutdown( void ) {
	globalKeys.Clear();
	globalValues.Clear();
}

/*
================
anDict::ShowMemoryUsage_f
================
*/
void anDict::ShowMemoryUsage_f( const anCommandArgs &args ) {
	anLibrary::common->Printf( "%5d KB in %d keys\n", globalKeys.Size() >> 10, globalKeys.Num() );
	anLibrary::common->Printf( "%5d KB in %d values\n", globalValues.Size() >> 10, globalValues.Num() );
}

/*
================
anDictStringSortCmp
================
*/
// NOTE: the const wonkyness is required to make msvc happy
template<>
ARC_INLINE int arcListSortCompare( const ARCPoolString * const *a, const ARCPoolString * const *b ) {
	return (*a)->Icmp( **b );
}

/*
================
anDict::ListKeys_f
================
*/
void anDict::ListKeys_f( const anCommandArgs &args ) {
	int i;
	anList<const ARCPoolString *> keyStrings;

	for ( i = 0; i < globalKeys.Num(); i++ ) {
		keyStrings.Append( globalKeys[i] );
	}
	keyStrings.Sort();
	for ( i = 0; i < keyStrings.Num(); i++ ) {
		anLibrary::common->Printf( "%s\n", keyStrings[i]->c_str() );
	}
	anLibrary::common->Printf( "%5d keys\n", keyStrings.Num() );
}

/*
================
anDict::ListValues_f
================
*/
void anDict::ListValues_f( const anCommandArgs &args ) {
	int i;
	anList<const ARCPoolString *> valueStrings;

	for ( i = 0; i < globalValues.Num(); i++ ) {
		valueStrings.Append( globalValues[i] );
	}
	valueStrings.Sort();
	for ( i = 0; i < valueStrings.Num(); i++ ) {
		anLibrary::common->Printf( "%s\n", valueStrings[i]->c_str() );
	}
	anLibrary::common->Printf( "%5d values\n", valueStrings.Num() );
}
