#ifndef __BITMSG_H__
#define __BITMSG_H__

/*
===============================================================================

  anBitMessage

  Handles byte ordering and avoids alignment errors.
  Allows concurrent writing and reading.
  The data set with Init is never freed.

===============================================================================
*/

class anBitMessage {
public:
					anBitMessage();
					~anBitMessage() {}

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
	void			WriteDir( const anVec3 &dir, int numBits );
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
	bool			WriteDeltaDict( const anDict &dict, const anDict *base );

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
	anVec3			ReadDir( int numBits ) const;
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
	bool			ReadDeltaDict( anDict &dict, const anDict *base ) const;

	static int		DirToBits( const anVec3 &dir, int numBits );
	static anVec3	BitsToDir( int bits, int numBits );

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

inline void anBitMessage::Init( byte *data, int length ) {
	writeData = data;
	readData = data;
	maxSize = length;
}

inline void anBitMessage::Init( const byte *data, int length ) {
	writeData = nullptr;
	readData = data;
	maxSize = length;
}

inline byte *anBitMessage::GetData( void ) {
	return writeData;
}

inline const byte *anBitMessage::GetData( void ) const {
	return readData;
}

inline int anBitMessage::GetMaxSize( void ) const {
	return maxSize;
}

inline void anBitMessage::SetAllowOverflow( bool set ) {
	allowOverflow = set;
}

inline bool anBitMessage::IsOverflowed( void ) const {
	return overflowed;
}

inline int anBitMessage::GetSize( void ) const {
	return curSize;
}

inline void anBitMessage::SetSize( int size ) {
	if ( size > maxSize ) {
		curSize = maxSize;
	} else {
		curSize = size;
	}
}

inline int anBitMessage::GetWriteBit( void ) const {
	return writeBit;
}

inline void anBitMessage::SetWriteBit( int bit ) {
	writeBit = bit & 7;
	if ( writeBit ) {
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

inline int anBitMessage::GetNumBitsWritten( void ) const {
	return ( ( curSize << 3 ) - ( ( 8 - writeBit ) & 7 ) );
}

inline int anBitMessage::GetRemainingWriteBits( void ) const {
	return ( maxSize << 3 ) - GetNumBitsWritten();
}

inline void anBitMessage::SaveWriteState( int &s, int &b ) const {
	s = curSize;
	b = writeBit;
}

inline void anBitMessage::RestoreWriteState( int s, int b ) {
	curSize = s;
	writeBit = b & 7;
	if ( writeBit ) {
		writeData[curSize - 1] &= ( 1 << writeBit ) - 1;
	}
}

inline int anBitMessage::GetReadCount( void ) const {
	return readCount;
}

inline void anBitMessage::SetReadCount( int bytes ) {
	readCount = bytes;
}

inline int anBitMessage::GetReadBit( void ) const {
	return readBit;
}

inline void anBitMessage::SetReadBit( int bit ) {
	readBit = bit & 7;
}

inline int anBitMessage::GetNumBitsRead( void ) const {
	return ( ( readCount << 3 ) - ( ( 8 - readBit ) & 7 ) );
}

inline int anBitMessage::GetRemainingReadBits( void ) const {
	return ( curSize << 3 ) - GetNumBitsRead();
}

inline void anBitMessage::SaveReadState( int &c, int &b ) const {
	c = readCount;
	b = readBit;
}

inline void anBitMessage::RestoreReadState( int c, int b ) {
	readCount = c;
	readBit = b & 7;
}

inline void anBitMessage::BeginWriting( void ) {
	curSize = 0;
	overflowed = false;
	writeBit = 0;
}

inline int anBitMessage::GetRemainingSpace( void ) const {
	return maxSize - curSize;
}

inline void anBitMessage::WriteByteAlign( void ) {
	writeBit = 0;
}

inline void anBitMessage::WriteChar( int c ) {
	WriteBits( c, -8 );
}

inline void anBitMessage::WriteByte( int c ) {
	WriteBits( c, 8 );
}

inline void anBitMessage::WriteShort( int c ) {
	WriteBits( c, -16 );
}

inline void anBitMessage::WriteUShort( int c ) {
	WriteBits( c, 16 );
}

inline void anBitMessage::WriteLong( int c ) {
	WriteBits( c, 32 );
}

inline void anBitMessage::WriteFloat( float f ) {
	WriteBits( *reinterpret_cast<int *>(&f), 32 );
}

inline void anBitMessage::WriteFloat( float f, int exponentBits, int mantissaBits ) {
	int bits = anMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits( bits, 1 + exponentBits + mantissaBits );
}

inline void anBitMessage::WriteAngle8( float f ) {
	WriteByte( ANGLE2BYTE( f ) );
}

inline void anBitMessage::WriteAngle16( float f ) {
	WriteShort( ANGLE2SHORT(f) );
}

inline void anBitMessage::WriteDir( const anVec3 &dir, int numBits ) {
	WriteBits( DirToBits( dir, numBits ), numBits );
}

inline void anBitMessage::WriteDeltaChar( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -8 );
}

inline void anBitMessage::WriteDeltaByte( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 8 );
}

inline void anBitMessage::WriteDeltaShort( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -16 );
}

inline void anBitMessage::WriteDeltaLong( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 32 );
}

inline void anBitMessage::WriteDeltaFloat( float oldValue, float newValue ) {
	WriteDelta( *reinterpret_cast<int *>(&oldValue), *reinterpret_cast<int *>(&newValue), 32 );
}

inline void anBitMessage::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits ) {
	int oldBits = anMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = anMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

inline void anBitMessage::BeginReading( void ) const {
	readCount = 0;
	readBit = 0;
}

inline int anBitMessage::GetRemaingData( void ) const {
	return curSize - readCount;
}

inline void anBitMessage::ReadByteAlign( void ) const {
	readBit = 0;
}

inline int anBitMessage::ReadChar( void ) const {
	return ( signed char)ReadBits( -8 );
}

inline int anBitMessage::ReadByte( void ) const {
	return (unsigned char)ReadBits( 8 );
}

inline int anBitMessage::ReadShort( void ) const {
	return ( short)ReadBits( -16 );
}

inline int anBitMessage::ReadUShort( void ) const {
	return (unsigned short)ReadBits( 16 );
}

inline int anBitMessage::ReadLong( void ) const {
	return ReadBits( 32 );
}

inline float anBitMessage::ReadFloat( void ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadBits( 32 );
	return value;
}

inline float anBitMessage::ReadFloat( int exponentBits, int mantissaBits ) const {
	int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return anMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

inline float anBitMessage::ReadAngle8( void ) const {
	return BYTE2ANGLE( ReadByte() );
}

inline float anBitMessage::ReadAngle16( void ) const {
	return SHORT2ANGLE( ReadShort() );
}

inline anVec3 anBitMessage::ReadDir( int numBits ) const {
	return BitsToDir( ReadBits( numBits ), numBits );
}

inline int anBitMessage::ReadDeltaChar( int oldValue ) const {
	return ( signed char)ReadDelta( oldValue, -8 );
}

inline int anBitMessage::ReadDeltaByte( int oldValue ) const {
	return (unsigned char)ReadDelta( oldValue, 8 );
}

inline int anBitMessage::ReadDeltaShort( int oldValue ) const {
	return ( short)ReadDelta( oldValue, -16 );
}

inline int anBitMessage::ReadDeltaLong( int oldValue ) const {
	return ReadDelta( oldValue, 32 );
}

inline float anBitMessage::ReadDeltaFloat( float oldValue ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadDelta( *reinterpret_cast<int *>(&oldValue), 32 );
	return value;
}

inline float anBitMessage::ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const {
	int oldBits = anMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = ReadDelta( oldBits, 1 + exponentBits + mantissaBits );
	return anMath::BitsToFloat( newBits, exponentBits, mantissaBits );
}


/*
===============================================================================

  anBitMsgDelta

===============================================================================
*/

class anBitMsgDelta {
public:
					anBitMsgDelta();
					~anBitMsgDelta() {}

	void			Init( const anBitMessage *base, anBitMessage *newBase, anBitMessage *delta );
	void			Init( const anBitMessage *base, anBitMessage *newBase, const anBitMessage *delta );
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
	void			WriteDir( const anVec3 &dir, int numBits );
	void			WriteString( const char *s, int maxLength = -1 );
	void			WriteData( const void *data, int length );
	void			WriteDict( const anDict &dict );

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
	anVec3			ReadDir( int numBits ) const;
	void			ReadString( char *buffer, int bufferSize ) const;
	void			ReadData( void *data, int length ) const;
	void			ReadDict( anDict &dict );

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
	const anBitMessage *base;			// base
	anBitMessage *		newBase;		// new base
	anBitMessage *		writeDelta;		// delta from base to new base for writing
	const anBitMessage *readDelta;		// delta from base to new base for reading
	mutable bool	changed;		// true if the new base is different from the base

private:
	void			WriteDelta( int oldValue, int newValue, int numBits );
	int				ReadDelta( int oldValue, int numBits ) const;
};

inline anBitMsgDelta::anBitMsgDelta() {
	base = nullptr;
	newBase = nullptr;
	writeDelta = nullptr;
	readDelta = nullptr;
	changed = false;
}

inline void anBitMsgDelta::Init( const anBitMessage *base, anBitMessage *newBase, anBitMessage *delta ) {
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = delta;
	this->readDelta = delta;
	this->changed = false;
}

inline void anBitMsgDelta::Init( const anBitMessage *base, anBitMessage *newBase, const anBitMessage *delta ) {
	this->base = base;
	this->newBase = newBase;
	this->writeDelta = nullptr;
	this->readDelta = delta;
	this->changed = false;
}

inline bool anBitMsgDelta::HasChanged( void ) const {
	return changed;
}

inline void anBitMsgDelta::WriteChar( int c ) {
	WriteBits( c, -8 );
}

inline void anBitMsgDelta::WriteByte( int c ) {
	WriteBits( c, 8 );
}

inline void anBitMsgDelta::WriteShort( int c ) {
	WriteBits( c, -16 );
}

inline void anBitMsgDelta::WriteUShort( int c ) {
	WriteBits( c, 16 );
}

inline void anBitMsgDelta::WriteLong( int c ) {
	WriteBits( c, 32 );
}

inline void anBitMsgDelta::WriteFloat( float f ) {
	WriteBits( *reinterpret_cast<int *>(&f), 32 );
}

inline void anBitMsgDelta::WriteFloat( float f, int exponentBits, int mantissaBits ) {
	int bits = anMath::FloatToBits( f, exponentBits, mantissaBits );
	WriteBits( bits, 1 + exponentBits + mantissaBits );
}

inline void anBitMsgDelta::WriteAngle8( float f ) {
	WriteBits( ANGLE2BYTE( f ), 8 );
}

inline void anBitMsgDelta::WriteAngle16( float f ) {
	WriteBits( ANGLE2SHORT(f), 16 );
}

inline void anBitMsgDelta::WriteDir( const anVec3 &dir, int numBits ) {
	WriteBits( anBitMessage::DirToBits( dir, numBits ), numBits );
}

inline void anBitMsgDelta::WriteDeltaChar( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -8 );
}

inline void anBitMsgDelta::WriteDeltaByte( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 8 );
}

inline void anBitMsgDelta::WriteDeltaShort( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, -16 );
}

inline void anBitMsgDelta::WriteDeltaLong( int oldValue, int newValue ) {
	WriteDelta( oldValue, newValue, 32 );
}

inline void anBitMsgDelta::WriteDeltaFloat( float oldValue, float newValue ) {
	WriteDelta( *reinterpret_cast<int *>(&oldValue), *reinterpret_cast<int *>(&newValue), 32 );
}

inline void anBitMsgDelta::WriteDeltaFloat( float oldValue, float newValue, int exponentBits, int mantissaBits ) {
	int oldBits = anMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = anMath::FloatToBits( newValue, exponentBits, mantissaBits );
	WriteDelta( oldBits, newBits, 1 + exponentBits + mantissaBits );
}

inline int anBitMsgDelta::ReadChar( void ) const {
	return ( signed char)ReadBits( -8 );
}

inline int anBitMsgDelta::ReadByte( void ) const {
	return (unsigned char)ReadBits( 8 );
}

inline int anBitMsgDelta::ReadShort( void ) const {
	return ( short)ReadBits( -16 );
}

inline int anBitMsgDelta::ReadUShort( void ) const {
	return (unsigned short)ReadBits( 16 );
}

inline int anBitMsgDelta::ReadLong( void ) const {
	return ReadBits( 32 );
}

inline float anBitMsgDelta::ReadFloat( void ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadBits( 32 );
	return value;
}

inline float anBitMsgDelta::ReadFloat( int exponentBits, int mantissaBits ) const {
	int bits = ReadBits( 1 + exponentBits + mantissaBits );
	return anMath::BitsToFloat( bits, exponentBits, mantissaBits );
}

inline float anBitMsgDelta::ReadAngle8( void ) const {
	return BYTE2ANGLE( ReadByte() );
}

inline float anBitMsgDelta::ReadAngle16( void ) const {
	return SHORT2ANGLE( ReadShort() );
}

inline anVec3 anBitMsgDelta::ReadDir( int numBits ) const {
	return anBitMessage::BitsToDir( ReadBits( numBits ), numBits );
}

inline int anBitMsgDelta::ReadDeltaChar( int oldValue ) const {
	return ( signed char)ReadDelta( oldValue, -8 );
}

inline int anBitMsgDelta::ReadDeltaByte( int oldValue ) const {
	return (unsigned char)ReadDelta( oldValue, 8 );
}

inline int anBitMsgDelta::ReadDeltaShort( int oldValue ) const {
	return ( short)ReadDelta( oldValue, -16 );
}

inline int anBitMsgDelta::ReadDeltaLong( int oldValue ) const {
	return ReadDelta( oldValue, 32 );
}

inline float anBitMsgDelta::ReadDeltaFloat( float oldValue ) const {
	float value;
	*reinterpret_cast<int *>(&value) = ReadDelta( *reinterpret_cast<int *>(&oldValue), 32 );
	return value;
}

inline float anBitMsgDelta::ReadDeltaFloat( float oldValue, int exponentBits, int mantissaBits ) const {
	int oldBits = anMath::FloatToBits( oldValue, exponentBits, mantissaBits );
	int newBits = ReadDelta( oldBits, 1 + exponentBits + mantissaBits );
	return anMath::BitsToFloat( newBits, exponentBits, mantissaBits );
}

#endif /* !__BITMSG_H__ */
