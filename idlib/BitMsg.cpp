#include "Lib.h"
#pragma hdrstop


/*
==============================================================================

  anBitMessage

==============================================================================
*/

/*
================
anBitMessage::anBitMessage
================
*/
anBitMessage::anBitMessage() {
	writeData = nullptr;
	readData = nullptr;
	maxSize = 0;
	curSize = 0;
	writeBit = 0;
	readCount = 0;
	readBit = 0;
	allowOverflow = false;
	overflowed = false;
}

/*
================
anBitMessage::CheckOverflow
================
*/
bool anBitMessage::CheckOverflow( int numBits ) {
	assert( numBits >= 0 );
	if ( numBits > GetRemainingWriteBits() ) {
		if ( !allowOverflow ) {
			anLibrary::common->FatalError( "anBitMessage: overflow without allowOverflow set" );
		}
		if ( numBits > ( maxSize << 3 ) ) {
			anLibrary::common->FatalError( "anBitMessage: %i bits is > full message size", numBits );
		}
		anLibrary::common->Printf( "anBitMessage: overflow\n" );
		BeginWriting();
		overflowed = true;
		return true;
	}
	return false;
}

/*
================
anBitMessage::GetByteSpace
================
*/
byte *anBitMessage::GetByteSpace( int length ) {
	byte *ptr;

	if ( !writeData ) {
		anLibrary::common->FatalError( "anBitMessage::GetByteSpace: cannot write to message" );
	}

	// round up to the next byte
	WriteByteAlign();

	// check for overflow
	CheckOverflow( length << 3 );

	ptr = writeData + curSize;
	curSize += length;
	return ptr;
}

/*
================
anBitMessage::WriteBits

  If the number of bits is negative a sign is included.
================
*/
void anBitMessage::WriteBits( int value, int numBits ) {
	int		put;
	int		fraction;

	if ( !writeData ) {
		anLibrary::common->Error( "anBitMessage::WriteBits: cannot write to message" );
	}

	// check if the number of bits is valid
	if ( numBits == 0 || numBits < -31 || numBits > 32 ) {
		anLibrary::common->Error( "anBitMessage::WriteBits: bad numBits %i", numBits );
	}

	// check for value overflows
	// this should be an error really, as it can go unnoticed and cause either bandwidth or corrupted data transmitted
	if ( numBits != 32 ) {
		if ( numBits > 0 ) {
			if ( value > ( 1 << numBits ) - 1 ) {
				anLibrary::common->Warning( "anBitMessage::WriteBits: value overflow %d %d", value, numBits );
			} else if ( value < 0 ) {
				anLibrary::common->Warning( "anBitMessage::WriteBits: value overflow %d %d", value, numBits );
			}
		} else {
			int r = 1 << ( - 1 - numBits );
			if ( value > r - 1 ) {
				anLibrary::common->Warning( "anBitMessage::WriteBits: value overflow %d %d", value, numBits );
			} else if ( value < -r ) {
				anLibrary::common->Warning( "anBitMessage::WriteBits: value overflow %d %d", value, numBits );
			}
		}
	}

	if ( numBits < 0 ) {
		numBits = -numBits;
	}

	// check for msg overflow
	if ( CheckOverflow( numBits ) ) {
		return;
	}

	// write the bits
	while( numBits ) {
		if ( writeBit == 0 ) {
			writeData[curSize] = 0;
			curSize++;
		}
		put = 8 - writeBit;
		if ( put > numBits ) {
			put = numBits;
		}
		fraction = value & ( ( 1 << put ) - 1 );
		writeData[curSize - 1] |= fraction << writeBit;
		numBits -= put;
		value >>= put;
		writeBit = ( writeBit + put ) & 7;
	}
}

/*
================
anBitMessage::WriteString
================
*/
void anBitMessage::WriteString( const char *s, int maxLength, bool make7Bit ) {
	if ( !s ) {
		WriteData( "", 1 );
	} else {
		int i, l;
		byte *dataPtr;
		const byte *bytePtr;

		l = anString::Length( s );
		if ( maxLength >= 0 && l >= maxLength ) {
			l = maxLength - 1;
		}
		dataPtr = GetByteSpace( l + 1 );
		bytePtr = reinterpret_cast<const byte *>( s);
		if ( make7Bit ) {
			for ( i = 0; i < l; i++ ) {
				if ( bytePtr[i] > 127 ) {
					dataPtr[i] = '.';
				} else {
					dataPtr[i] = bytePtr[i];
				}
			}
		} else {
			for ( i = 0; i < l; i++ ) {
				dataPtr[i] = bytePtr[i];
			}
		}
		dataPtr[i] = '\0';
	}
}

/*
================
anBitMessage::WriteData
================
*/
void anBitMessage::WriteData( const void *data, int length ) {
	memcpy( GetByteSpace( length ), data, length );
}

/*
================
anBitMessage::WriteNetadr
================
*/
void anBitMessage::WriteNetadr( const netadr_t adr ) {
	byte *dataPtr;
	dataPtr = GetByteSpace( 4 );
	memcpy( dataPtr, adr.ip, 4 );
	WriteUShort( adr.port );
}

/*
================
anBitMessage::WriteDelta
================
*/
void anBitMessage::WriteDelta( int oldValue, int newValue, int numBits ) {
	if ( oldValue == newValue ) {
		WriteBits( 0, 1 );
		return;
	}
	WriteBits( 1, 1 );
	WriteBits( newValue, numBits );
}

/*
================
anBitMessage::WriteDeltaByteCounter
================
*/
void anBitMessage::WriteDeltaByteCounter( int oldValue, int newValue ) {
	int i, x;

	x = oldValue ^ newValue;
	for ( i = 7; i > 0; i-- ) {
		if ( x & ( 1 << i ) ) {
			i++;
			break;
		}
	}
	WriteBits( i, 3 );
	if ( i ) {
		WriteBits( ( ( 1 << i ) - 1 ) & newValue, i );
	}
}

/*
================
anBitMessage::WriteDeltaShortCounter
================
*/
void anBitMessage::WriteDeltaShortCounter( int oldValue, int newValue ) {
	int i, x;

	x = oldValue ^ newValue;
	for ( i = 15; i > 0; i-- ) {
		if ( x & ( 1 << i ) ) {
			i++;
			break;
		}
	}
	WriteBits( i, 4 );
	if ( i ) {
		WriteBits( ( ( 1 << i ) - 1 ) & newValue, i );
	}
}

/*
================
anBitMessage::WriteDeltaLongCounter
================
*/
void anBitMessage::WriteDeltaLongCounter( int oldValue, int newValue ) {
	int i, x;

	x = oldValue ^ newValue;
	for ( i = 31; i > 0; i-- ) {
		if ( x & ( 1 << i ) ) {
			i++;
			break;
		}
	}
	WriteBits( i, 5 );
	if ( i ) {
		WriteBits( ( ( 1 << i ) - 1 ) & newValue, i );
	}
}

/*
==================
anBitMessage::WriteDeltaDict
==================
*/
bool anBitMessage::WriteDeltaDict( const anDict &dict, const anDict *base ) {
	int i;
	const anKeyValue *kv, *basekv;
	bool changed = false;

	if ( base != nullptr ) {

		for ( i = 0; i < dict.GetNumKeyVals(); i++ ) {
			kv = dict.GetKeyVal( i );
			basekv = base->FindKey( kv->GetKey() );
			if ( basekv == nullptr || basekv->GetValue().Icmp( kv->GetValue() ) != 0 ) {
				WriteString( kv->GetKey() );
				WriteString( kv->GetValue() );
				changed = true;
			}
		}

		WriteString( "" );

		for ( i = 0; i < base->GetNumKeyVals(); i++ ) {
			basekv = base->GetKeyVal( i );
			kv = dict.FindKey( basekv->GetKey() );
			if ( kv == nullptr ) {
				WriteString( basekv->GetKey() );
				changed = true;
			}
		}

		WriteString( "" );

	} else {

		for ( i = 0; i < dict.GetNumKeyVals(); i++ ) {
			kv = dict.GetKeyVal( i );
			WriteString( kv->GetKey() );
			WriteString( kv->GetValue() );
			changed = true;
		}
		WriteString( "" );

		WriteString( "" );

	}

	return changed;
}

/*
================
anBitMessage::ReadBits

  If the number of bits is negative a sign is included.
================
*/
int anBitMessage::ReadBits( int numBits ) const {
	int		value;
	int		valueBits;
	int		get;
	int		fraction;
	bool	sgn;

	if ( !readData ) {
		anLibrary::common->FatalError( "anBitMessage::ReadBits: cannot read from message" );
	}

	// check if the number of bits is valid
	if ( numBits == 0 || numBits < -31 || numBits > 32 ) {
		anLibrary::common->FatalError( "anBitMessage::ReadBits: bad numBits %i", numBits );
	}

	value = 0;
	valueBits = 0;

	if ( numBits < 0 ) {
		numBits = -numBits;
		sgn = true;
	} else {
		sgn = false;
	}

	// check for overflow
	if ( numBits > GetRemainingReadBits() ) {
		return -1;
	}

	while ( valueBits < numBits ) {
		if ( readBit == 0 ) {
			readCount++;
		}
		get = 8 - readBit;
		if ( get > (numBits - valueBits) ) {
			get = numBits - valueBits;
		}
		fraction = readData[readCount - 1];
		fraction >>= readBit;
		fraction &= ( 1 << get ) - 1;
		value |= fraction << valueBits;

		valueBits += get;
		readBit = ( readBit + get ) & 7;
	}

	if ( sgn ) {
		if ( value & ( 1 << ( numBits - 1 ) ) ) {
			value |= -1 ^ ( ( 1 << numBits ) - 1 );
		}
	}

	return value;
}

/*
================
anBitMessage::ReadString
================
*/
int anBitMessage::ReadString( char *buffer, int bufferSize ) const {
	int	l, c;

	ReadByteAlign();
	l = 0;
	while( 1 ) {
		c = ReadByte();
		if ( c <= 0 || c >= 255 ) {
			break;
		}
		// translate all fmt spec to avoid crash bugs in string routines
		if ( c == '%' ) {
			c = '.';
		}

		// we will read past any excessively long string, so
		// the following data can be read, but the string will
		// be truncated
		if ( l < bufferSize - 1 ) {
			buffer[l] = c;
			l++;
		}
	}

	buffer[l] = 0;
	return l;
}

/*
================
anBitMessage::ReadData
================
*/
int anBitMessage::ReadData( void *data, int length ) const {
	int cnt;

	ReadByteAlign();
	cnt = readCount;

	if ( readCount + length > curSize ) {
		if ( data ) {
			memcpy( data, readData + readCount, GetRemaingData() );
		}
		readCount = curSize;
	} else {
		if ( data ) {
			memcpy( data, readData + readCount, length );
		}
		readCount += length;
	}

	return ( readCount - cnt );
}

/*
================
anBitMessage::ReadNetadr
================
*/
void anBitMessage::ReadNetadr( netadr_t *adr ) const {
	int i;

	adr->type = NA_IP;
	for ( i = 0; i < 4; i++ ) {
		adr->ip[i] = ReadByte();
	}
	adr->port = ReadUShort();
}

/*
================
anBitMessage::ReadDelta
================
*/
int anBitMessage::ReadDelta( int oldValue, int numBits ) const {
	if ( ReadBits( 1 ) ) {
		return ReadBits( numBits );
	}
	return oldValue;
}

/*
================
anBitMessage::ReadDeltaByteCounter
================
*/
int anBitMessage::ReadDeltaByteCounter( int oldValue ) const {
	int i, newValue;

	i = ReadBits( 3 );
	if ( !i ) {
		return oldValue;
	}
	newValue = ReadBits( i );
	return ( oldValue & ~( ( 1 << i ) - 1 ) | newValue );
}

/*
================
anBitMessage::ReadDeltaShortCounter
================
*/
int anBitMessage::ReadDeltaShortCounter( int oldValue ) const {
	int i, newValue;

	i = ReadBits( 4 );
	if ( !i ) {
		return oldValue;
	}
	newValue = ReadBits( i );
	return ( oldValue & ~( ( 1 << i ) - 1 ) | newValue );
}

/*
================
anBitMessage::ReadDeltaLongCounter
================
*/
int anBitMessage::ReadDeltaLongCounter( int oldValue ) const {
	int i, newValue;

	i = ReadBits( 5 );
	if ( !i ) {
		return oldValue;
	}
	newValue = ReadBits( i );
	return ( oldValue & ~( ( 1 << i ) - 1 ) | newValue );
}

/*
==================
anBitMessage::ReadDeltaDict
==================
*/
bool anBitMessage::ReadDeltaDict( anDict &dict, const anDict *base ) const {
	char		key[MAX_STRING_CHARS];
	char		value[MAX_STRING_CHARS];
	bool		changed = false;

	if ( base != nullptr ) {
		dict = *base;
	} else {
		dict.Clear();
	}

	while( ReadString( key, sizeof( key ) ) != 0 ) {
		ReadString( value, sizeof( value ) );
		dict.Set( key, value );
		changed = true;
	}

	while( ReadString( key, sizeof( key ) ) != 0 ) {
		dict.Delete( key );
		changed = true;
	}

	return changed;
}

/*
================
anBitMessage::DirToBits
================
*/
int anBitMessage::DirToBits( const anVec3 &dir, int numBits ) {
	int max, bits;
	float bias;

	assert( numBits >= 6 && numBits <= 32 );
	assert( dir.LengthSqr() - 1.0f < 0.01f );

	numBits /= 3;
	max = ( 1 << ( numBits - 1 ) ) - 1;
	bias = 0.5f / max;

	bits = FLOATSIGNBITSET( dir.x ) << ( numBits * 3 - 1 );
	bits |= ( anMath::Ftoi( ( anMath::Fabs( dir.x ) + bias ) * max ) ) << ( numBits * 2 );
	bits |= FLOATSIGNBITSET( dir.y ) << ( numBits * 2 - 1 );
	bits |= ( anMath::Ftoi( ( anMath::Fabs( dir.y ) + bias ) * max ) ) << ( numBits * 1 );
	bits |= FLOATSIGNBITSET( dir.z ) << ( numBits * 1 - 1 );
	bits |= ( anMath::Ftoi( ( anMath::Fabs( dir.z ) + bias ) * max ) ) << ( numBits * 0 );
	return bits;
}

/*
================
anBitMessage::BitsToDir
================
*/
anVec3 anBitMessage::BitsToDir( int bits, int numBits ) {
	static float sign[2] = { 1.0f, -1.0f };
	int max;
	float invMax;
	anVec3 dir;

	assert( numBits >= 6 && numBits <= 32 );

	numBits /= 3;
	max = ( 1 << ( numBits - 1 ) ) - 1;
	invMax = 1.0f / max;

	dir.x = sign[( bits >> ( numBits * 3 - 1 ) ) & 1] * ( ( bits >> ( numBits * 2 ) ) & max ) * invMax;
	dir.y = sign[( bits >> ( numBits * 2 - 1 ) ) & 1] * ( ( bits >> ( numBits * 1 ) ) & max ) * invMax;
	dir.z = sign[( bits >> ( numBits * 1 - 1 ) ) & 1] * ( ( bits >> ( numBits * 0 ) ) & max ) * invMax;
	dir.NormalizeFast();
	return dir;
}


/*
==============================================================================

  anBitMsgDelta

==============================================================================
*/

const int MAX_DATA_BUFFER		= 1024;

/*
================
anBitMsgDelta::WriteBits
================
*/
void anBitMsgDelta::WriteBits( int value, int numBits ) {
	if ( newBase ) {
		newBase->WriteBits( value, numBits );
	}

	if ( !base ) {
		writeDelta->WriteBits( value, numBits );
		changed = true;
	} else {
		int baseValue = base->ReadBits( numBits );
		if ( baseValue == value ) {
			writeDelta->WriteBits( 0, 1 );
		} else {
			writeDelta->WriteBits( 1, 1 );
			writeDelta->WriteBits( value, numBits );
			changed = true;
		}
	}
}

/*
================
anBitMsgDelta::WriteDelta
================
*/
void anBitMsgDelta::WriteDelta( int oldValue, int newValue, int numBits ) {
	if ( newBase ) {
		newBase->WriteBits( newValue, numBits );
	}

	if ( !base ) {
		if ( oldValue == newValue ) {
			writeDelta->WriteBits( 0, 1 );
		} else {
			writeDelta->WriteBits( 1, 1 );
			writeDelta->WriteBits( newValue, numBits );
		}
		changed = true;
	} else {
		int baseValue = base->ReadBits( numBits );
		if ( baseValue == newValue ) {
			writeDelta->WriteBits( 0, 1 );
		} else {
			writeDelta->WriteBits( 1, 1 );
			if ( oldValue == newValue ) {
				writeDelta->WriteBits( 0, 1 );
				changed = true;
			} else {
				writeDelta->WriteBits( 1, 1 );
				writeDelta->WriteBits( newValue, numBits );
				changed = true;
			}
		}
	}
}

/*
================
anBitMsgDelta::ReadBits
================
*/
int anBitMsgDelta::ReadBits( int numBits ) const {
	int value;

	if ( !base ) {
		value = readDelta->ReadBits( numBits );
		changed = true;
	} else {
		int baseValue = base->ReadBits( numBits );
		if ( !readDelta || readDelta->ReadBits( 1 ) == 0 ) {
			value = baseValue;
		} else {
			value = readDelta->ReadBits( numBits );
			changed = true;
		}
	}

	if ( newBase ) {
		newBase->WriteBits( value, numBits );
	}
	return value;
}

/*
================
anBitMsgDelta::ReadDelta
================
*/
int anBitMsgDelta::ReadDelta( int oldValue, int numBits ) const {
	int value;

	if ( !base ) {
		if ( readDelta->ReadBits( 1 ) == 0 ) {
			value = oldValue;
		} else {
			value = readDelta->ReadBits( numBits );
		}
		changed = true;
	} else {
		int baseValue = base->ReadBits( numBits );
		if ( !readDelta || readDelta->ReadBits( 1 ) == 0 ) {
			value = baseValue;
		} else if ( readDelta->ReadBits( 1 ) == 0 ) {
			value = oldValue;
			changed = true;
		} else {
			value = readDelta->ReadBits( numBits );
			changed = true;
		}
	}

	if ( newBase ) {
		newBase->WriteBits( value, numBits );
	}
	return value;
}

/*
================
anBitMsgDelta::WriteString
================
*/
void anBitMsgDelta::WriteString( const char *s, int maxLength ) {
	if ( newBase ) {
		newBase->WriteString( s, maxLength );
	}

	if ( !base ) {
		writeDelta->WriteString( s, maxLength );
		changed = true;
	} else {
		char baseString[MAX_DATA_BUFFER];
		base->ReadString( baseString, sizeof( baseString ) );
		if ( anString::Cmp( s, baseString ) == 0 ) {
			writeDelta->WriteBits( 0, 1 );
		} else {
			writeDelta->WriteBits( 1, 1 );
			writeDelta->WriteString( s, maxLength );
			changed = true;
		}
	}
}

/*
================
anBitMsgDelta::WriteData
================
*/
void anBitMsgDelta::WriteData( const void *data, int length ) {
	if ( newBase ) {
		newBase->WriteData( data, length );
	}

	if ( !base ) {
		writeDelta->WriteData( data, length );
		changed = true;
	} else {
		byte baseData[MAX_DATA_BUFFER];
		assert( length < sizeof( baseData ) );
		base->ReadData( baseData, length );
		if ( memcmp( data, baseData, length ) == 0 ) {
			writeDelta->WriteBits( 0, 1 );
		} else {
			writeDelta->WriteBits( 1, 1 );
			writeDelta->WriteData( data, length );
			changed = true;
		}
	}
}

/*
================
anBitMsgDelta::WriteDict
================
*/
void anBitMsgDelta::WriteDict( const anDict &dict ) {
	if ( newBase ) {
		newBase->WriteDeltaDict( dict, nullptr );
	}

	if ( !base ) {
		writeDelta->WriteDeltaDict( dict, nullptr );
		changed = true;
	} else {
		anDict baseDict;
		base->ReadDeltaDict( baseDict, nullptr );
		changed = writeDelta->WriteDeltaDict( dict, &baseDict );
	}
}

/*
================
anBitMsgDelta::WriteDeltaByteCounter
================
*/
void anBitMsgDelta::WriteDeltaByteCounter( int oldValue, int newValue ) {
	if ( newBase ) {
		newBase->WriteBits( newValue, 8 );
	}

	if ( !base ) {
		writeDelta->WriteDeltaByteCounter( oldValue, newValue );
		changed = true;
	} else {
		int baseValue = base->ReadBits( 8 );
		if ( baseValue == newValue ) {
			writeDelta->WriteBits( 0, 1 );
		} else {
			writeDelta->WriteBits( 1, 1 );
			writeDelta->WriteDeltaByteCounter( oldValue, newValue );
			changed = true;
		}
	}
}

/*
================
anBitMsgDelta::WriteDeltaShortCounter
================
*/
void anBitMsgDelta::WriteDeltaShortCounter( int oldValue, int newValue ) {
	if ( newBase ) {
		newBase->WriteBits( newValue, 16 );
	}

	if ( !base ) {
		writeDelta->WriteDeltaShortCounter( oldValue, newValue );
		changed = true;
	} else {
		int baseValue = base->ReadBits( 16 );
		if ( baseValue == newValue ) {
			writeDelta->WriteBits( 0, 1 );
		} else {
			writeDelta->WriteBits( 1, 1 );
			writeDelta->WriteDeltaShortCounter( oldValue, newValue );
			changed = true;
		}
	}
}

/*
================
anBitMsgDelta::WriteDeltaLongCounter
================
*/
void anBitMsgDelta::WriteDeltaLongCounter( int oldValue, int newValue ) {
	if ( newBase ) {
		newBase->WriteBits( newValue, 32 );
	}

	if ( !base ) {
		writeDelta->WriteDeltaLongCounter( oldValue, newValue );
		changed = true;
	} else {
		int baseValue = base->ReadBits( 32 );
		if ( baseValue == newValue ) {
			writeDelta->WriteBits( 0, 1 );
		} else {
			writeDelta->WriteBits( 1, 1 );
			writeDelta->WriteDeltaLongCounter( oldValue, newValue );
			changed = true;
		}
	}
}

/*
================
anBitMsgDelta::ReadString
================
*/
void anBitMsgDelta::ReadString( char *buffer, int bufferSize ) const {
	if ( !base ) {
		readDelta->ReadString( buffer, bufferSize );
		changed = true;
	} else {
		char baseString[MAX_DATA_BUFFER];
		base->ReadString( baseString, sizeof( baseString ) );
		if ( !readDelta || readDelta->ReadBits( 1 ) == 0 ) {
			anString::Copynz( buffer, baseString, bufferSize );
		} else {
			readDelta->ReadString( buffer, bufferSize );
			changed = true;
		}
	}

	if ( newBase ) {
		newBase->WriteString( buffer );
	}
}

/*
================
anBitMsgDelta::ReadData
================
*/
void anBitMsgDelta::ReadData( void *data, int length ) const {
	if ( !base ) {
		readDelta->ReadData( data, length );
		changed = true;
	} else {
		char baseData[MAX_DATA_BUFFER];
		assert( length < sizeof( baseData ) );
		base->ReadData( baseData, length );
		if ( !readDelta || readDelta->ReadBits( 1 ) == 0 ) {
			memcpy( data, baseData, length );
		} else {
			readDelta->ReadData( data, length );
			changed = true;
		}
	}

	if ( newBase ) {
		newBase->WriteData( data, length );
	}
}

/*
================
anBitMsgDelta::ReadDict
================
*/
void anBitMsgDelta::ReadDict( anDict &dict ) {
	if ( !base ) {
		readDelta->ReadDeltaDict( dict, nullptr );
		changed = true;
	} else {
		anDict baseDict;
		base->ReadDeltaDict( baseDict, nullptr );
		if ( !readDelta ) {
			dict = baseDict;
		} else {
			changed = readDelta->ReadDeltaDict( dict, &baseDict );
		}
	}

	if ( newBase ) {
		newBase->WriteDeltaDict( dict, nullptr );
	}
}

/*
================
anBitMsgDelta::ReadDeltaByteCounter
================
*/
int anBitMsgDelta::ReadDeltaByteCounter( int oldValue ) const {
	int value;

	if ( !base ) {
		value = readDelta->ReadDeltaByteCounter( oldValue );
		changed = true;
	} else {
		int baseValue = base->ReadBits( 8 );
		if ( !readDelta || readDelta->ReadBits( 1 ) == 0 ) {
			value = baseValue;
		} else {
			value = readDelta->ReadDeltaByteCounter( oldValue );
			changed = true;
		}
	}

	if ( newBase ) {
		newBase->WriteBits( value, 8 );
	}
	return value;
}

/*
================
anBitMsgDelta::ReadDeltaShortCounter
================
*/
int anBitMsgDelta::ReadDeltaShortCounter( int oldValue ) const {
	int value;

	if ( !base ) {
		value = readDelta->ReadDeltaShortCounter( oldValue );
		changed = true;
	} else {
		int baseValue = base->ReadBits( 16 );
		if ( !readDelta || readDelta->ReadBits( 1 ) == 0 ) {
			value = baseValue;
		} else {
			value = readDelta->ReadDeltaShortCounter( oldValue );
			changed = true;
		}
	}

	if ( newBase ) {
		newBase->WriteBits( value, 16 );
	}
	return value;
}

/*
================
anBitMsgDelta::ReadDeltaLongCounter
================
*/
int anBitMsgDelta::ReadDeltaLongCounter( int oldValue ) const {
	int value;

	if ( !base ) {
		value = readDelta->ReadDeltaLongCounter( oldValue );
		changed = true;
	} else {
		int baseValue = base->ReadBits( 32 );
		if ( !readDelta || readDelta->ReadBits( 1 ) == 0 ) {
			value = baseValue;
		} else {
			value = readDelta->ReadDeltaLongCounter( oldValue );
			changed = true;
		}
	}

	if ( newBase ) {
		newBase->WriteBits( value, 32 );
	}
	return value;
}
