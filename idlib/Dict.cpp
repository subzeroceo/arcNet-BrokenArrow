#include "precompiled.h"
#pragma hdrstop

anStringPool		arcDictionary::globalKeys;
anStringPool		arcDictionary::globalValues;

/*
================
arcDictionary::operator=

  clear existing key/value pairs and copy all key/value pairs from other
================
*/
arcDictionary &arcDictionary::operator=( const arcDictionary &other ) {
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
arcDictionary::Copy

  copy all key value pairs without removing existing key/value pairs not present in the other dict
================
*/
void arcDictionary::Copy( const arcDictionary &other ) {
	int i, n, *found;
	idKeyValue kv;

	// check for assignment to self
	if ( this == &other ) {
		return;
	}

	n = other.args.Num();

	if ( args.Num() ) {
		found = ( int * ) _alloca16( other.args.Num() * sizeof( int ) );
        for ( i = 0; i < n; i++ ) {
			found[i] = FindKeyIndex( other.args[i].GetKey() );
		}
	} else {
		found = NULL;
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
arcDictionary::TransferKeyValues

  clear existing key/value pairs and transfer key/value pairs from other
================
*/
void arcDictionary::TransferKeyValues( arcDictionary &other ) {
	int i, n;

	if ( this == &other ) {
		return;
	}

	if ( other.args.Num() && other.args[0].key->GetPool() != &globalKeys ) {
		common->FatalError( "arcDictionary::TransferKeyValues: can't transfer values across a DLL boundary" );
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
arcDictionary::Parse
================
*/
bool arcDictionary::Parse( ARCParser &parser ) {
	arcNetToken	token;
	arcNetToken	token2;

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
arcDictionary::SetDefaults
================
*/
void arcDictionary::SetDefaults( const arcDictionary *dict ) {
	int i, n;
	const idKeyValue *kv, *def;
	idKeyValue newkv;

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
arcDictionary::Clear
================
*/
void arcDictionary::Clear( void ) {
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
arcDictionary::Print
================
*/
void arcDictionary::Print() const {
	int i;
	int n;

	n = args.Num();
	for ( i = 0; i < n; i++ ) {
		anLibrary::common->Printf( "%s = %s\n", args[i].GetKey().c_str(), args[i].GetValue().c_str() );
	}
}

int KeyCompare( const idKeyValue *a, const idKeyValue *b ) {
	return anString::Cmp( a->GetKey(), b->GetKey() );
}

/*
================
arcDictionary::Checksum
================
*/
int	arcDictionary::Checksum( void ) const {
	unsigned long ret;
	int i, n;

	anList<idKeyValue> sorted = args;
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
arcDictionary::Allocated
================
*/
size_t arcDictionary::Allocated( void ) const {
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
arcDictionary::Set
================
*/
void arcDictionary::Set( const char *key, const char *value ) {
	int i;
	idKeyValue kv;

	if ( key == NULL || key[0] == '\0' ) {
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
arcDictionary::GetFloat
================
*/
bool arcDictionary::GetFloat( const char *key, const char *defaultString, float &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = atof( s );
	return found;
}

/*
================
arcDictionary::GetInt
================
*/
bool arcDictionary::GetInt( const char *key, const char *defaultString, int &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = atoi( s );
	return found;
}

/*
================
arcDictionary::GetBool
================
*/
bool arcDictionary::GetBool( const char *key, const char *defaultString, bool &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = ( atoi( s ) != 0 );
	return found;
}

/*
================
arcDictionary::GetAngles
================
*/
bool arcDictionary::GetAngles( const char *key, const char *defaultString, arcAngles &out ) const {
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
arcDictionary::GetVector
================
*/
bool arcDictionary::GetVector( const char *key, const char *defaultString, anVec3 &out ) const {
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
arcDictionary::GetVec2
================
*/
bool arcDictionary::GetVec2( const char *key, const char *defaultString, anVec2 &out ) const {
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
arcDictionary::GetVec4
================
*/
bool arcDictionary::GetVec4( const char *key, const char *defaultString, anVec4 &out ) const {
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
arcDictionary::GetMatrix
================
*/
bool arcDictionary::GetMatrix( const char *key, const char *defaultString, anMat3 &out ) const {
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
		anLibrary::common->Error( "arcDictionary::WriteToFileHandle: bad string" );
	}
	f->Write( s, strlen(s) + 1 );
}

/*
================
arcDictionary::FindKey
================
*/
const idKeyValue *arcDictionary::FindKey( const char *key ) const {
	int i, hash;

	if ( key == NULL || key[0] == '\0' ) {
		anLibrary::common->DWarning( "arcDictionary::FindKey: empty key" );
		return NULL;
	}

	hash = argHash.GenerateKey( key, false );
	for ( i = argHash.First( hash ); i != -1; i = argHash.Next( i ) ) {
		if ( args[i].GetKey().Icmp( key ) == 0 ) {
			return &args[i];
		}
	}

	return NULL;
}

/*
================
arcDictionary::FindKeyIndex
================
*/
int arcDictionary::FindKeyIndex( const char *key ) const {
	if ( key == NULL || key[0] == '\0' ) {
		anLibrary::common->DWarning( "arcDictionary::FindKeyIndex: empty key" );
		return NULL;
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
arcDictionary::Delete
================
*/
void arcDictionary::Delete( const char *key ) {
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
		assert( FindKey( args[i].GetKey() ) != NULL );
	}
#endif
}

/*
================
arcDictionary::MatchPrefix
================
*/
const idKeyValue *arcDictionary::MatchPrefix( const char *prefix, const idKeyValue *lastMatch ) const {
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
	return NULL;
}

/*
================
arcDictionary::RandomPrefix
================
*/
const char *arcDictionary::RandomPrefix( const char *prefix, arcRandom &random ) const {
	int count;
	const int MAX_RANDOM_KEYS = 2048;
	const char *list[MAX_RANDOM_KEYS];
	const idKeyValue *kv;

	list[0] = "";
	for ( count = 0, kv = MatchPrefix( prefix ); kv && count < MAX_RANDOM_KEYS; kv = MatchPrefix( prefix, kv ) ) {
		list[count++] = kv->GetValue().c_str();
	}
	return list[random.RandomInt( count )];
}

/*
================
arcDictionary::WriteToFileHandle
================
*/
void arcDictionary::WriteToFileHandle( anFile *f ) const {
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
		anLibrary::common->Error( "arcDictionary::ReadFromFileHandle: bad string" );
	}

	return anString( str );
}

/*
================
arcDictionary::ReadFromFileHandle
================
*/
void arcDictionary::ReadFromFileHandle( anFile *f ) {
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
arcDictionary::Init
================
*/
void arcDictionary::Init( void ) {
	globalKeys.SetCaseSensitive( false );
	globalValues.SetCaseSensitive( true );
}

/*
================
arcDictionary::Shutdown
================
*/
void arcDictionary::Shutdown( void ) {
	globalKeys.Clear();
	globalValues.Clear();
}

/*
================
arcDictionary::ShowMemoryUsage_f
================
*/
void arcDictionary::ShowMemoryUsage_f( const anCommandArgs &args ) {
	anLibrary::common->Printf( "%5d KB in %d keys\n", globalKeys.Size() >> 10, globalKeys.Num() );
	anLibrary::common->Printf( "%5d KB in %d values\n", globalValues.Size() >> 10, globalValues.Num() );
}

/*
================
arcDictStringSortCmp
================
*/
// NOTE: the const wonkyness is required to make msvc happy
template<>
ARC_INLINE int arcListSortCompare( const ARCPoolString * const *a, const ARCPoolString * const *b ) {
	return (*a)->Icmp( **b );
}

/*
================
arcDictionary::ListKeys_f
================
*/
void arcDictionary::ListKeys_f( const anCommandArgs &args ) {
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
arcDictionary::ListValues_f
================
*/
void arcDictionary::ListValues_f( const anCommandArgs &args ) {
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
