#ifndef __BITMSG_H__
#define __BITMSG_H__

/*
===============================================================================

  ARCBitMessage

  Handles byte ordering and avoids alignment errors.
  Allows concurrent writing and reading.
  The data set with Init is never freed.

===============================================================================
*/

class ARCBitMessage {
public:
					ARCBitMessage();
					~ARCBitMessage() {}

	void			Init( byte *data, int length );
	void			Init( const byte *data, int length );
	byte *			GetData( void );						// get data for writing
	const byte *	GetData( void ) const;					// get data for reading
	int				GetMaxSize( void ) const;				// get the maximum message size
	void			SetAllowOverflow( bool set );			// generate error if not set and message is overflowed
	bool			IsOverflowed( void ) const;				// returns true if the message was overflowed

	int				GetSize( void ) const;					// size of the message in bytes
	void			SetSize( int size );					// set the message size
	int				GetWriteBit( void ) const;				// get current write bit
	void			SetWriteBit( int bit );					// set current write bit
	int				GetNumBitsWritten( void ) const;		// returns number of bits written
	int				GetRemainingWriteBits( void ) const;	// space left in bits for writing
	void			SaveWriteState( int &s, int &b ) const;	// save the write state
	void			RestoreWriteState( int s, int b );		// restore the write state

	int				GetReadCount( void ) const;				// bytes read so far
	void			SetReadCount( int bytes );				// set the number of bytes and bits read
	int				GetReadBit( void ) const;				// get current read bit
	void			SetReadBit( int bit );					// set current read bit
	int				GetNumBitsRead( void ) const;			// returns number of bits read
	int				GetRemainingReadBits( void ) const;		// number of bits left to read
	void			SaveReadState( int &c, int &b ) const;	// save the read state
	void			RestoreReadState( int c, int b );		// restore the read state

	void			BeginWriting( void );					// begin writing
	int				GetRemainingSpace( void ) const;		// space left in bytes
	void			WriteByteAlign( void );					// write up to the next byte boundary
	void			WriteBits( int value, int numBits );	// write the specified number of bits
	void			WriteChar( int c );
	void			WriteByte( int c );
	void			WriteShort( int c );
	void			WriteUShort( int c );
	void			WriteLong( int c );
	void			WriteFloat( float f );
	void			WriteFloat( float f, int exponentBits, int mantissaBits );
	void			WriteAngle8( float f );
	void			WriteAngle16( float f );
	void			WriteDir( const arcVec3 &dir, int numBits );
	void			WriteString( const char *s, int maxLength = -1, bool make7Bit = true );
	void			WriteData( const void *data, int length );
	void			WriteNetadr( const netadr_t adr );

	void			WriteDeltaChar( int oldValue, int newValue );
	void			WriteDeltaByte( int oldValue, int newValue );
	void			WriteDeltaShort( int oldValue, int newValue );
	void			WriteDeltaLong( int oldValue, int newValue );
	void			WriteDeltaFloat( float oldValue, float newValue );
	void			WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits );
	void			WriteDeltaByteCounter( int oldValue, int newValue );
	void			WriteDeltaShortCounter( int oldValue, int newValue );
	void			WriteDeltaLongCounter( int oldValue, int newValue );
	bool			WriteDeltaDict( const arcDictionary &dict, const arcDictionary *base );

	void			BeginReading( void ) const;				// begin reading.
	int				GetRemaingData( void ) const;			// number of bytes left to read
	void			ReadByteAlign( void ) const;			// read up to the next byte boundary
	int				ReadBits( int numBits ) const;			// read the specified number of bits
	int				ReadChar( void ) const;
	int				ReadByte( void ) const;
	int				ReadShort( void ) const;
	int				ReadUShort( void ) const;
	int				ReadLong( void ) const;
	float			ReadFloat( void ) const;
	float			ReadFloat( int exponentBits, int mantissaBits ) const;
	float			ReadAngle8( void ) const;
	float			ReadAngle16( void ) const;
	arcVec3			ReadDir( int numBits ) const;
	int				ReadString( char *buffer, int bufferSize ) const;
	int				ReadData( void *data, int length ) const;
	void			ReadNetadr( netadr_t *adr ) const;

	int				ReadDeltaChar( int oldValue ) const;
	int				ReadDeltaByte( int oldValue ) const;
	int				ReadDeltaShort( int oldValue ) const;
	int				ReadDeltaLong( int oldValue ) const;
	float			ReadDeltaFloat( float oldValue ) const;
	float			ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const;
	int				ReadDeltaByteCounter( int oldValue ) const;
	int				ReadDeltaShortCounter( int oldValue ) const;
	int				ReadDeltaLongCounter( int oldValue ) const;
	bool			ReadDeltaDict( arcDictionary &dict, const arcDictionary *base ) const;

	static int		DirToBits( const arcVec3 &dir, int numBits );
	static arcVec3	BitsToDir( int bits, int numBits );

private:
	byte *			writeData;			// pointer to data for writing
	const byte *	readData;			// pointer to data for reading
	int				maxSize;			// maximum size of message in bytes
	int				curSize;			// current size of message in bytes
	int				writeBit;			// number of bits written to the last written byte
	mutable int		readCount;			// number of bytes read so far
	mutable int		readBit;			// number of bits read from the last read byte
	bool			allowOverflow;		// if false, generate an error when the message is overflowed
	bool			overflowed;			// set to true if the buffer size failed (with allowOverflow set)

private:
	bool			CheckOverflow( int numBits );
	byte *			GetByteSpace( int length );
	void			WriteDelta( int oldValue, int newValue, int numBits );
	int				ReadDelta( int oldValue, int numBits ) const;
};


ARC_INLINE void ARCBitMessage::Init( byte *data, int length ) {
	writeData = data;
	readData = data;
	maxSize = length;
}

ARC_INLINE void ARCBitMessage::Init( const byte *data, int length ) {
	writeData = NULL;
	readData = data;
	maxSize = length;
}

ARC_INLINE byte *ARCBitMessage::GetData( void ) {
	return writeData;
}

ARC_INLINE const byte *ARCBitMessage::GetData( void ) const {
	return readData;
}

ARC_INLINE int ARCBitMessage::GetMaxSize( void ) const {
	return maxSize;
}

ARC_INLINE void ARCBitMessage::SetAllowOverflow( bool set ) {
	allowOverflow = set;
}

ARC_INLINE bool ARCBitMessage::IsOverflowed( void ) const {
	return overflowed;
}

ARC_INLINE int ARCBitMessage::GetSize( void ) const {
	return curSize;
}

ARC_INLINE void ARCBitMessage::SetSize( int size ) {
	if ( size > maxSize ) {
		curSize = maxSize;
	} else {
		curSize = size;
	}
}

ARC_INLINE int ARCBitMessage::GetWriteBit( void ) const {
	return writeBit;
}

ARC_INLINE void ARCBitMessage::SetWriteBit( int bit ) {
	writeBit = bit & 7;
	if ( writeBit ) {
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

ARC_INLINE int ARCBitMessage::GetNumBitsWritten( void ) const {
	return ( ( curSize << 3 ) - ( ( 8 - writeBit ) & 7 ) );
}

ARC_INLINE int ARCBitMessage::GetRemainingWriteBits( void ) const {
	return ( maxSize << 3 ) - GetNumBitsWritten();
}

ARC_INLINE void ARCBitMessage::SaveWriteState( int &s, int &b ) const {
	s = curSize;
	b = writeBit;
}

ARC_INLINE void ARCBitMessage::RestoreWriteState( int s, int b ) {
	curSize = s;
	writeBit = b & 7;
	if ( writeBit ) {
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

ARC_INLINE int ARCBitMessage::GetReadCount( void ) const {
	return readCount;
}

ARC_INLINE void ARCBitMessage::SetReadCount( int bytes ) {
	readCount = bytes;
}

ARC_INLINE int ARCBitMessage::GetReadBit( void ) const {
	return readBit;
}

ARC_INLINE void ARCBitMessage::SetReadBit( int bit ) {
	readBit = bit & 7;
}

ARC_INLINE int ARCBitMessage::GetNumBitsRead( void ) const {
	return ( ( readCount << 3 ) - ( ( 8 - readBit ) & 7 ) );
}

ARC_INLINE int ARCBitMessage::GetRemainingReadBits( void ) const {
	return ( curSize << 3 ) - GetNumBitsRead();
}

ARC_INLINE void ARCBitMessage::SaveReadState( int &c, int &b ) const {
	c = readCount;
	b = readBit;
}

ARC_INLINE void ARCBitMessage::RestoreReadState( int c, int b ) {
	readCount = c;
	readBit = b & 7;
}

ARC_INLINE void ARCBitMessage::BeginWriting( void ) {
	curSize = 0;
	overflowed = false;
	writeBit = 0;
}

ARC_INLINE int ARCBitMessage::GetRemainingSpace( void ) const {
	return maxSize - curSize;
}

ARC_INLINE void ARCBitMessage::WriteByteAlign( void ) {
	writeBit = 0;
}

ARC_INLINE void ARCBitMessage::WriteChar( int c ) {
	WriteBits( c, -8 );
}

ARC_INLINE void ARCBitMessage::WriteByte( int c ) {
	WriteBits( c, 8 );
}

ARC_INLINE void ARCBitMessage::WriteShort( int c ) {
	WriteBits( c, -16 );
}

ARC_INLINE void ARCBitMessage::WriteUShort( int c ) {
	WriteBits( c, 16 );
}

ARC_INLINE void ARCBitMessage::WriteLong( int c ) {
	WriteBits( c, 32 );
}

ARC_INLINE void ARCBitMessage::WriteFloat( float f ) {
	WriteBits( *reinterpret_cast<int *>(&f), 32 );
}

ARC_INLINE void ARCBitMessage::WriteFloat( float f, int exponentBits, int mantissaBits ) {
	int bits = arcMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits( bits, 1 + exponentBits + mantissaBits );
}

ARC_INLINE void ARCBitMessage::WriteAngle8( float f ) {
	WriteByte( ANGLE2BYTE( f ) );
}

ARC_INLINE void ARCBitMessage::WriteAngle16( float f ) {
	WriteShort( ANGLE2SHORT(f) );
}

ARC_INLINE void ARCBitMessage::WriteDir( const arcVec3 &dir, int numBits ) {
	WriteBits( DirToBits( dir, numBits ), numBits );
}

ARC_INLINE void ARCBitMessage::WriteDeltaChar( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -8 );
}

ARC_INLINE void ARCBitMessage::WriteDeltaByte( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 8 );
}

ARC_INLINE void ARCBitMessage::WriteDeltaShort( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -16 );
}

ARC_INLINE void ARCBitMessage::WriteDeltaLong( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 32 );
}

ARC_INLINE void ARCBitMessage::WriteDeltaFloat( float oldValue, float newValue ) {
	WriteDelta( *reinterpret_cast<int *>(&oldValue), *reinterpret_cast<int *>(&newValue), 32 );
}

ARC_INLINE void ARCBitMessage::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits ) {
	int oldBits = arcMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = arcMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

ARC_INLINE void ARCBitMessage::BeginReading( void ) const {
	readCount = 0;
	readBit = 0;
}

ARC_INLINE int ARCBitMessage::GetRemaingData( void ) const {
	return curSize - readCount;
}

ARC_INLINE void ARCBitMessage::ReadByteAlign( void ) const {
	readBit = 0;
}

ARC_INLINE int ARCBitMessage::ReadChar( void ) const {
	return (signed char)ReadBits( -8 );
}

ARC_INLINE int ARCBitMessage::ReadByte( void ) const {
	return (unsigned char)ReadBits( 8 );
}

ARC_INLINE int ARCBitMessage::ReadShort( void ) const {
	return (short)ReadBits( -16 );
}

ARC_INLINE int ARCBitMessage::ReadUShort( void ) const {
	return (unsigned short)ReadBits( 16 );
}

ARC_INLINE int ARCBitMessage::ReadLong( void ) const {
	return ReadBits( 32 );
}

ARC_INLINE float ARCBitMessage::ReadFloat( void ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadBits( 32 );
	return value;
}

ARC_INLINE float ARCBitMessage::ReadFloat( int exponentBits, int mantissaBits ) const {
	int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return arcMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

ARC_INLINE float ARCBitMessage::ReadAngle8( void ) const {
	return BYTE2ANGLE( ReadByte() );
}

ARC_INLINE float ARCBitMessage::ReadAngle16( void ) const {
	return SHORT2ANGLE( ReadShort() );
}

ARC_INLINE arcVec3 ARCBitMessage::ReadDir( int numBits ) const {
	return BitsToDir( ReadBits( numBits ), numBits );
}

ARC_INLINE int ARCBitMessage::ReadDeltaChar( int oldValue ) const {
	return (signed char)ReadDelta( oldValue, -8 );
}

ARC_INLINE int ARCBitMessage::ReadDeltaByte( int oldValue ) const {
	return (unsigned char)ReadDelta( oldValue, 8 );
}

ARC_INLINE int ARCBitMessage::ReadDeltaShort( int oldValue ) const {
	return (short)ReadDelta( oldValue, -16 );
}

ARC_INLINE int ARCBitMessage::ReadDeltaLong( int oldValue ) const {
	return ReadDelta( oldValue, 32 );
}

ARC_INLINE float ARCBitMessage::ReadDeltaFloat( float oldValue ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadDelta( *reinterpret_cast<int *>(&oldValue), 32 );
	return value;
}

ARC_INLINE float ARCBitMessage::ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const {
	int oldBits = arcMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = ReadDelta( oldBits, 1 + exponentBits + mantissaBits );
	return arcMath::BitsToFloat( newBits, exponentBits, mantissaBits );
}


/*
===============================================================================

  idBitMsgDelta

===============================================================================
*/

class idBitMsgDelta {
public:
					idBitMsgDelta();
					~idBitMsgDelta() {}

	void			Init( const ARCBitMessage *base, ARCBitMessage *newBase, ARCBitMessage *delta );
	void			Init( const ARCBitMessage *base, ARCBitMessage *newBase, const ARCBitMessage *delta );
	bool			HasChanged( void ) const;

	void			WriteBits( int value, int numBits );
	void			WriteChar( int c );
	void			WriteByte( int c );
	void			WriteShort( int c );
	void			WriteUShort( int c );
	void			WriteLong( int c );
	void			WriteFloat( float f );
	void			WriteFloat( float f, int exponentBits, int mantissaBits );
	void			WriteAngle8( float f );
	void			WriteAngle16( float f );
	void			WriteDir( const arcVec3 &dir, int numBits );
	void			WriteString( const char *s, int maxLength = -1 );
	void			WriteData( const void *data, int length );
	void			WriteDict( const arcDictionary &dict );

	void			WriteDeltaChar( int oldValue, int newValue );
	void			WriteDeltaByte( int oldValue, int newValue );
	void			WriteDeltaShort( int oldValue, int newValue );
	void			WriteDeltaLong( int oldValue, int newValue );
	void			WriteDeltaFloat( float oldValue, float newValue );
	void			WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits );
	void			WriteDeltaByteCounter( int oldValue, int newValue );
	void			WriteDeltaShortCounter( int oldValue, int newValue );
	void			WriteDeltaLongCounter( int oldValue, int newValue );

	int				ReadBits( int numBits ) const;
	int				ReadChar( void ) const;
	int				ReadByte( void ) const;
	int				ReadShort( void ) const;
	int				ReadUShort( void ) const;
	int				ReadLong( void ) const;
	float			ReadFloat( void ) const;
	float			ReadFloat( int exponentBits, int mantissaBits ) const;
	float			ReadAngle8( void ) const;
	float			ReadAngle16( void ) const;
	arcVec3			ReadDir( int numBits ) const;
	void			ReadString( char *buffer, int bufferSize ) const;
	void			ReadData( void *data, int length ) const;
	void			ReadDict( arcDictionary &dict );

	int				ReadDeltaChar( int oldValue ) const;
	int				ReadDeltaByte( int oldValue ) const;
	int				ReadDeltaShort( int oldValue ) const;
	int				ReadDeltaLong( int oldValue ) const;
	float			ReadDeltaFloat( float oldValue ) const;
	float			ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const;
	int				ReadDeltaByteCounter( int oldValue ) const;
	int				ReadDeltaShortCounter( int oldValue ) const;
	int				ReadDeltaLongCounter( int oldValue ) const;

private:
	const ARCBitMessage *base;			// base
	ARCBitMessage *		newBase;		// new base
	ARCBitMessage *		writeDelta;		// delta from base to new base for writing
	const ARCBitMessage *readDelta;		// delta from base to new base for reading
	mutable bool	changed;		// true if the new base is different from the base

private:
	void			WriteDelta( int oldValue, int newValue, int numBits );
	int				ReadDelta( int oldValue, int numBits ) const;
};

ARC_INLINE idBitMsgDelta::idBitMsgDelta() {
	base = NULL;
	newBase = NULL;
	writeDelta = NULL;
	readDelta = NULL;
	changed = false;
}

ARC_INLINE void idBitMsgDelta::Init( const ARCBitMessage *base, ARCBitMessage *newBase, ARCBitMessage *delta ) {
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = delta;
	this->readDelta = delta;
	this->changed = false;
}

ARC_INLINE void idBitMsgDelta::Init( const ARCBitMessage *base, ARCBitMessage *newBase, const ARCBitMessage *delta ) {
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = NULL;
	this->readDelta = delta;
	this->changed = false;
}

ARC_INLINE bool idBitMsgDelta::HasChanged( void ) const {
	return changed;
}

ARC_INLINE void idBitMsgDelta::WriteChar( int c ) {
	WriteBits( c, -8 );
}

ARC_INLINE void idBitMsgDelta::WriteByte( int c ) {
	WriteBits( c, 8 );
}

ARC_INLINE void idBitMsgDelta::WriteShort( int c ) {
	WriteBits( c, -16 );
}

ARC_INLINE void idBitMsgDelta::WriteUShort( int c ) {
	WriteBits( c, 16 );
}

ARC_INLINE void idBitMsgDelta::WriteLong( int c ) {
	WriteBits( c, 32 );
}

ARC_INLINE void idBitMsgDelta::WriteFloat( float f ) {
	WriteBits( *reinterpret_cast<int *>(&f), 32 );
}

ARC_INLINE void idBitMsgDelta::WriteFloat( float f, int exponentBits, int mantissaBits ) {
	int bits = arcMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits( bits, 1 + exponentBits + mantissaBits );
}

ARC_INLINE void idBitMsgDelta::WriteAngle8( float f ) {
	WriteBits( ANGLE2BYTE( f ), 8 );
}

ARC_INLINE void idBitMsgDelta::WriteAngle16( float f ) {
	WriteBits( ANGLE2SHORT(f), 16 );
}

ARC_INLINE void idBitMsgDelta::WriteDir( const arcVec3 &dir, int numBits ) {
	WriteBits( ARCBitMessage::DirToBits( dir, numBits ), numBits );
}

ARC_INLINE void idBitMsgDelta::WriteDeltaChar( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -8 );
}

ARC_INLINE void idBitMsgDelta::WriteDeltaByte( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 8 );
}

ARC_INLINE void idBitMsgDelta::WriteDeltaShort( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -16 );
}

ARC_INLINE void idBitMsgDelta::WriteDeltaLong( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 32 );
}

ARC_INLINE void idBitMsgDelta::WriteDeltaFloat( float oldValue, float newValue ) {
	WriteDelta( *reinterpret_cast<int *>(&oldValue), *reinterpret_cast<int *>(&newValue), 32 );
}

ARC_INLINE void idBitMsgDelta::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits ) {
	int oldBits = arcMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = arcMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

ARC_INLINE int idBitMsgDelta::ReadChar( void ) const {
	return (signed char)ReadBits( -8 );
}

ARC_INLINE int idBitMsgDelta::ReadByte( void ) const {
	return (unsigned char)ReadBits( 8 );
}

ARC_INLINE int idBitMsgDelta::ReadShort( void ) const {
	return (short)ReadBits( -16 );
}

ARC_INLINE int idBitMsgDelta::ReadUShort( void ) const {
	return (unsigned short)ReadBits( 16 );
}

ARC_INLINE int idBitMsgDelta::ReadLong( void ) const {
	return ReadBits( 32 );
}

ARC_INLINE float idBitMsgDelta::ReadFloat( void ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadBits( 32 );
	return value;
}

ARC_INLINE float idBitMsgDelta::ReadFloat( int exponentBits, int mantissaBits ) const {
	int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return arcMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

ARC_INLINE float idBitMsgDelta::ReadAngle8( void ) const {
	return BYTE2ANGLE( ReadByte() );
}

ARC_INLINE float idBitMsgDelta::ReadAngle16( void ) const {
	return SHORT2ANGLE( ReadShort() );
}

ARC_INLINE arcVec3 idBitMsgDelta::ReadDir( int numBits ) const {
	return ARCBitMessage::BitsToDir( ReadBits( numBits ), numBits );
}

ARC_INLINE int idBitMsgDelta::ReadDeltaChar( int oldValue ) const {
	return (signed char)ReadDelta( oldValue, -8 );
}

ARC_INLINE int idBitMsgDelta::ReadDeltaByte( int oldValue ) const {
	return (unsigned char)ReadDelta( oldValue, 8 );
}

ARC_INLINE int idBitMsgDelta::ReadDeltaShort( int oldValue ) const {
	return (short)ReadDelta( oldValue, -16 );
}

ARC_INLINE int idBitMsgDelta::ReadDeltaLong( int oldValue ) const {
	return ReadDelta( oldValue, 32 );
}

ARC_INLINE float idBitMsgDelta::ReadDeltaFloat( float oldValue ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadDelta( *reinterpret_cast<int *>(&oldValue), 32 );
	return value;
}

ARC_INLINE float idBitMsgDelta::ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const {
	int oldBits = arcMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = ReadDelta( oldBits, 1 + exponentBits + mantissaBits );
	return arcMath::BitsToFloat( newBits, exponentBits, mantissaBits );
}

#endif /* !__BITMSG_H__ */
