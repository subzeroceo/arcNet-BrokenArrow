#include "/idlib/precompiled.h"
#pragma hdrstop

#include "Unzip.h"

/*
=================
FS_WriteFloatString
=================
*/
int FS_WriteFloatString( char *buf, const char *fmt, va_list argPtr ) {
	long i;
	unsigned long u;
	double f;
	char *str;
	int index;
	arcNetString tmp, format;

	index = 0;

	while( *fmt ) {
		switch( *fmt ) {
			case '%':
				format = "";
				format += *fmt++;
				while ( (*fmt >= '0' && *fmt <= '9') ||
						*fmt == '.' || *fmt == '-' || *fmt == '+' || *fmt == '#') {
					format += *fmt++;
				}
				format += *fmt;
				switch( *fmt ) {
					case 'f':
					case 'e':
					case 'E':
					case 'g':
					case 'G':
						f = va_arg( argPtr, double );
						if ( format.Length() <= 2 ) {
							// high precision floating point number without trailing zeros
							sprintf( tmp, "%1.10f", f );
							tmp.StripTrailing( '0' );
							tmp.StripTrailing( '.' );
							index += sprintf( buf+index, "%s", tmp.c_str() );
						}
						else {
							index += sprintf( buf+index, format.c_str(), f );
						}
						break;
					case 'd':
					case 'i':
						i = va_arg( argPtr, long );
						index += sprintf( buf+index, format.c_str(), i );
						break;
					case 'u':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'o':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'x':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'X':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'c':
						i = va_arg( argPtr, long );
						index += sprintf( buf+index, format.c_str(), (char) i );
						break;
					case 's':
						str = va_arg( argPtr, char * );
						index += sprintf( buf+index, format.c_str(), str );
						break;
					case '%':
						index += sprintf( buf+index, format.c_str() ); //-V618
						break;
					default:
						common->Error( "FS_WriteFloatString: invalid format %s", format.c_str() );
						break;
				}
				fmt++;
				break;
			case '\\':
				fmt++;
				switch( *fmt ) {
					case 't':
						index += sprintf( buf+index, "\t" );
						break;
					case 'v':
						index += sprintf( buf+index, "\v" );
						break;
					case 'n':
						index += sprintf( buf+index, "\n" );
						break;
					case '\\':
						index += sprintf( buf+index, "\\" );
						break;
					default:
						common->Error( "FS_WriteFloatString: unknown escape character \'%c\'", *fmt );
						break;
				}
				fmt++;
				break;
			default:
				index += sprintf( buf+index, "%c", *fmt );
				fmt++;
				break;
		}
	}

	return index;
}

/*
=================================================================================

arcNetFile

=================================================================================
*/

/*
=================
arcNetFile::GetName
=================
*/
const char *arcNetFile::GetName() const {
	return "";
}

/*
=================
arcNetFile::GetFullPath
=================
*/
const char *arcNetFile::GetFullPath() const {
	return "";
}

/*
=================
arcNetFile::Read
=================
*/
int arcNetFile::Read( void *buffer, int len ) {
	common->FatalError( "arcNetFile::Read: cannot read from arcNetFile" );
	return 0;
}

/*
=================
arcNetFile::Write
=================
*/
int arcNetFile::Write( const void *buffer, int len ) {
	common->FatalError( "arcNetFile::Write: cannot write to arcNetFile" );
	return 0;
}

/*
=================
arcNetFile::Length
=================
*/
int arcNetFile::Length() const {
	return 0;
}

/*
=================
arcNetFile::Timestamp
=================
*/
ARC_TIME_T arcNetFile::Timestamp() const {
	return 0;
}

/*
=================
arcNetFile::Tell
=================
*/
int arcNetFile::Tell() const {
	return 0;
}

/*
=================
arcNetFile::ForceFlush
=================
*/
void arcNetFile::ForceFlush() {
}

/*
=================
arcNetFile::Flush
=================
*/
void arcNetFile::Flush() {
}

/*
=================
arcNetFile::Seek
=================
*/
int arcNetFile::Seek( long offset, fsOrigin_t origin ) {
	return -1;
}

/*
=================
arcNetFile::Rewind
=================
*/
void arcNetFile::Rewind() {
	Seek( 0, FS_SEEK_SET );
}

/*
=================
arcNetFile::Printf
=================
*/
int arcNetFile::Printf( const char *fmt, ... ) {
	char buf[MAX_PRINT_MSG];
	int length;
	va_list argptr;

	va_start( argptr, fmt );
	length = arcNetString::vsnPrintf( buf, MAX_PRINT_MSG-1, fmt, argptr );
	va_end( argptr );

	// so notepad formats the lines correctly
  	arcNetString	work( buf );
 	work.Replace( "\n", "\r\n" );

  	return Write( work.c_str(), work.Length() );
}

/*
=================
arcNetFile::VPrintf
=================
*/
int arcNetFile::VPrintf( const char *fmt, va_list args ) {
	char buf[MAX_PRINT_MSG];
	int length;

	length = arcNetString::vsnPrintf( buf, MAX_PRINT_MSG-1, fmt, args );
	return Write( buf, length );
}

/*
=================
arcNetFile::WriteFloatString
=================
*/
int arcNetFile::WriteFloatString( const char *fmt, ... ) {
	char buf[MAX_PRINT_MSG];
	int len;
	va_list argPtr;

	va_start( argPtr, fmt );
	len = FS_WriteFloatString( buf, fmt, argPtr );
	va_end( argPtr );

	return Write( buf, len );
}

/*
 =================
 arcNetFile::ReadInt
 =================
 */
int arcNetFile::ReadInt( int &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleLong(value);
	return result;
}

/*
 =================
 arcNetFile::ReadUnsignedInt
 =================
 */
int arcNetFile::ReadUnsignedInt( unsigned int &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleLong(value);
	return result;
}

/*
 =================
 arcNetFile::ReadShort
 =================
 */
int arcNetFile::ReadShort( short &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleShort(value);
	return result;
}

/*
 =================
 arcNetFile::ReadUnsignedShort
 =================
 */
int arcNetFile::ReadUnsignedShort( unsigned short &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleShort(value);
	return result;
}

/*
 =================
 arcNetFile::ReadChar
 =================
 */
int arcNetFile::ReadChar( char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 arcNetFile::ReadUnsignedChar
 =================
 */
int arcNetFile::ReadUnsignedChar( unsigned char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 arcNetFile::ReadFloat
 =================
 */
int arcNetFile::ReadFloat( float &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleFloat(value);
	return result;
}

/*
 =================
 arcNetFile::ReadBool
 =================
 */
int arcNetFile::ReadBool( bool &value ) {
	unsigned char c;
	int result = ReadUnsignedChar( c );
	value = c ? true : false;
	return result;
}

/*
 =================
 arcNetFile::ReadString
 =================
 */
int arcNetFile::ReadString( arcNetString &string ) {
	int len;
	int result = 0;

	ReadInt( len );
	if ( len >= 0 ) {
		string.Fill( ' ', len );
		result = Read( &string[ 0 ], len );
	}
	return result;
}

/*
 =================
 arcNetFile::ReadVec2
 =================
 */
int arcNetFile::ReadVec2( arcVec2 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof( float ), sizeof(vec)/sizeof( float ) );
	return result;
}

/*
 =================
 arcNetFile::ReadVec3
 =================
 */
int arcNetFile::ReadVec3( arcVec3 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof( float ), sizeof(vec)/sizeof( float ) );
	return result;
}

/*
 =================
 arcNetFile::ReadVec4
 =================
 */
int arcNetFile::ReadVec4( arcVec4 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof( float ), sizeof(vec)/sizeof( float ) );
	return result;
}

/*
 =================
 arcNetFile::ReadVec6
 =================
 */
int arcNetFile::ReadVec6( arcVec6 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof( float ), sizeof(vec)/sizeof( float ) );
	return result;
}

/*
 =================
 arcNetFile::ReadMat3
 =================
 */
int arcNetFile::ReadMat3( arcMat3 &mat ) {
	int result = Read( &mat, sizeof( mat ) );
	LittleRevBytes( &mat, sizeof( float ), sizeof(mat)/sizeof( float ) );
	return result;
}

/*
 =================
 arcNetFile::WriteInt
 =================
 */
int arcNetFile::WriteInt( const int value ) {
	int v = LittleLong(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteUnsignedInt
 =================
 */
int arcNetFile::WriteUnsignedInt( const unsigned int value ) {
	unsigned int v = LittleLong(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteShort
 =================
 */
int arcNetFile::WriteShort( const short value ) {
	short v = LittleShort(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteUnsignedShort
 =================
 */
int arcNetFile::WriteUnsignedShort( const unsigned short value ) {
	unsigned short v = LittleShort(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteChar
 =================
 */
int arcNetFile::WriteChar( const char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 arcNetFile::WriteUnsignedChar
 =================
 */
int arcNetFile::WriteUnsignedChar( const unsigned char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 arcNetFile::WriteFloat
 =================
 */
int arcNetFile::WriteFloat( const float value ) {
	float v = LittleFloat(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteBool
 =================
 */
int arcNetFile::WriteBool( const bool value ) {
	unsigned char c = value;
	return WriteUnsignedChar( c );
}

/*
 =================
 arcNetFile::WriteString
 =================
 */
int arcNetFile::WriteString( const char *value ) {
	int len = strlen( value );
	WriteInt( len );
    return Write( value, len );
}

/*
 =================
 arcNetFile::WriteVec2
 =================
 */
int arcNetFile::WriteVec2( const arcVec2 &vec ) {
	arcVec2 v = vec;
	LittleRevBytes( &v, sizeof( float ), sizeof( v)/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteVec3
 =================
 */
int arcNetFile::WriteVec3( const arcVec3 &vec ) {
	arcVec3 v = vec;
	LittleRevBytes( &v, sizeof( float ), sizeof( v)/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteVec4
 =================
 */
int arcNetFile::WriteVec4( const arcVec4 &vec ) {
	arcVec4 v = vec;
	LittleRevBytes( &v, sizeof( float ), sizeof( v)/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteVec6
 =================
 */
int arcNetFile::WriteVec6( const arcVec6 &vec ) {
	arcVec6 v = vec;
	LittleRevBytes( &v, sizeof( float ), sizeof( v)/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 arcNetFile::WriteMat3
 =================
 */
int arcNetFile::WriteMat3( const arcMat3 &mat ) {
	arcMat3 v = mat;
	LittleRevBytes(&v, sizeof( float ), sizeof( v)/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
=================================================================================

aRcFileMemory

=================================================================================
*/


/*
=================
aRcFileMemory::aRcFileMemory
=================
*/
aRcFileMemory::aRcFileMemory() {
	name = "*unknown*";
	maxSize = 0;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = NULL;
	curPtr = NULL;
}

/*
=================
aRcFileMemory::aRcFileMemory
=================
*/
aRcFileMemory::aRcFileMemory( const char *name ) {
	this->name = name;
	maxSize = 0;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = NULL;
	curPtr = NULL;
}

/*
=================
aRcFileMemory::aRcFileMemory
=================
*/
aRcFileMemory::aRcFileMemory( const char *name, char *data, int length ) {
	this->name = name;
	maxSize = length;
	fileSize = 0;
	allocated = length;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = data;
	curPtr = data;
}

/*
=================
aRcFileMemory::aRcFileMemory
=================
*/
aRcFileMemory::aRcFileMemory( const char *name, const char *data, int length ) {
	this->name = name;
	maxSize = 0;
	fileSize = length;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_READ );
	filePtr = const_cast<char *>( data);
	curPtr = const_cast<char *>( data);
}

/*
=================
aRcFileMemory::TakeDataOwnership

this also makes the file read only
=================
*/
void aRcFileMemory::TakeDataOwnership() {
	if ( filePtr != NULL && fileSize > 0 ) {
		maxSize = 0;
		mode = ( 1 << FS_READ );
		allocated = fileSize;
	}
}

/*
=================
aRcFileMemory::~aRcFileMemory
=================
*/
aRcFileMemory::~aRcFileMemory() {
	if ( filePtr && allocated > 0 && maxSize == 0 ) {
		Mem_Free( filePtr );
	}
}

/*
=================
aRcFileMemory::Read
=================
*/
int aRcFileMemory::Read( void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "aRcFileMemory::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( curPtr + len > filePtr + fileSize ) {
		len = filePtr + fileSize - curPtr;
	}
	memcpy( buffer, curPtr, len );
	curPtr += len;
	return len;
}

arcCVarSystem memcpyImpl( "memcpyImpl", "0", 0, "Which implementation of memcpy to use for aRcFileMemory::Write() [0/1 - standard (1 eliminates branch misprediction), 2 - auto-vectorized]" );
void * memcpy2( void * __restrict b, const void * __restrict a, size_t n ) {
	char * s1 = (char *)b;
	const char * s2 = (const char *)a;
	for (; 0 < n; --n ) {
		*s1++ = *s2++;
	}
	return b;
}

/*
=================
aRcFileMemory::Write
=================
*/
idHashTableT< int, int > histogram;
CONSOLE_COMMAND( outputHistogram, "", 0 ) {
	for ( int i = 0; i < histogram.Num(); i++ ) {
		int key;
		histogram.GetIndexKey( i, key );
		int * value = histogram.GetIndex( i );

		arcLibrary::Printf( "%d\t%d\n", key, *value );
	}
}

CONSOLE_COMMAND( clearHistogram, "", 0 ) {
	histogram.Clear();
}

int aRcFileMemory::Write( const void *buffer, int len ) {
	if ( len == 0 ) {
		// ~4% falls into this case for some reason...
		return 0;
	}

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "aRcFileMemory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	int alloc = curPtr + len + 1 - filePtr - allocated; // need room for len+1
	if ( alloc > 0 ) {
		if ( maxSize != 0 ) {
			common->Error( "aRcFileMemory::Write: exceeded maximum size %d", maxSize );
			return 0;
		}
		int extra = granularity * ( 1 + alloc / granularity );
		char *newPtr = (char *) Mem_Alloc( allocated + extra, TAG_IDFILE );
		if ( allocated ) {
			memcpy( newPtr, filePtr, allocated );
		}
		allocated += extra;
		curPtr = newPtr + ( curPtr - filePtr );
		if ( filePtr ) {
			Mem_Free( filePtr );
		}
		filePtr = newPtr;
	}

	//memcpy( curPtr, buffer, len );
	memcpy2( curPtr, buffer, len );

#if 0
	if ( memcpyImpl.GetInteger() == 0 ) {
		memcpy( curPtr, buffer, len );
	} else if ( memcpyImpl.GetInteger() == 1 ) {
		memcpy( curPtr, buffer, len );
	} else if ( memcpyImpl.GetInteger() == 2 ) {
		memcpy2( curPtr, buffer, len );
	}
#endif

#if 0
	int * value;
	if ( histogram.Get( len, &value ) && value != NULL ) {
		(*value)++;
	} else {
		histogram.Set( len, 1 );
	}
#endif

	curPtr += len;
	fileSize += len;
	filePtr[ fileSize ] = 0; // len + 1
	return len;
}

/*
=================
aRcFileMemory::Length
=================
*/
int aRcFileMemory::Length() const {
	return fileSize;
}

/*
========================
aRcFileMemory::SetLength
========================
*/
void aRcFileMemory::SetLength( size_t len ) {
	PreAllocate( len );
	fileSize = len;
}

/*
========================
aRcFileMemory::PreAllocate
========================
*/
void aRcFileMemory::PreAllocate( size_t len ) {
	if ( len > allocated ) {
		if ( maxSize != 0 ) {
			arcLibrary::Error( "aRcFileMemory::SetLength: exceeded maximum size %d", maxSize );
		}
		char * newPtr = (char *)Mem_Alloc( len, TAG_IDFILE );
		if ( allocated > 0 ) {
			memcpy( newPtr, filePtr, allocated );
		}
		allocated = len;
		curPtr = newPtr + ( curPtr - filePtr );
		if ( filePtr != NULL ) {
			Mem_Free( filePtr );
		}
		filePtr = newPtr;
	}
}

/*
=================
aRcFileMemory::Timestamp
=================
*/
ARC_TIME_T aRcFileMemory::Timestamp() const {
	return 0;
}

/*
=================
aRcFileMemory::Tell
=================
*/
int aRcFileMemory::Tell() const {
	return ( curPtr - filePtr );
}

/*
=================
aRcFileMemory::ForceFlush
=================
*/
void aRcFileMemory::ForceFlush() {
}

/*
=================
aRcFileMemory::Flush
=================
*/
void aRcFileMemory::Flush() {
}

/*
=================
aRcFileMemory::Seek

  returns zero on success and -1 on failure
=================
*/
int aRcFileMemory::Seek( long offset, fsOrigin_t origin ) {

	switch( origin ) {
		case FS_SEEK_CUR: {
			curPtr += offset;
			break;
		}
		case FS_SEEK_END: {
			curPtr = filePtr + fileSize - offset;
			break;
		}
		case FS_SEEK_SET: {
			curPtr = filePtr + offset;
			break;
		}
		default: {
			common->FatalError( "aRcFileMemory::Seek: bad origin for %s\n", name.c_str() );
			return -1;
		}
	}
	if ( curPtr < filePtr ) {
		curPtr = filePtr;
		return -1;
	}
	if ( curPtr > filePtr + fileSize ) {
		curPtr = filePtr + fileSize;
		return -1;
	}
	return 0;
}

/*
========================
aRcFileMemory::SetMaxLength
========================
*/
void aRcFileMemory::SetMaxLength( size_t len ) {
	size_t oldLength = fileSize;

	SetLength( len );

	maxSize = len;
	fileSize = oldLength;
}

/*
=================
aRcFileMemory::MakeReadOnly
=================
*/
void aRcFileMemory::MakeReadOnly() {
	mode = ( 1 << FS_READ );
	Rewind();
}

/*
========================
aRcFileMemory::MakeWritable
========================
*/
void aRcFileMemory::MakeWritable() {
	mode = ( 1 << FS_WRITE );
	Rewind();
}

/*
=================
aRcFileMemory::Clear
=================
*/
void aRcFileMemory::Clear( bool freeMemory ) {
	fileSize = 0;
	granularity = 16384;
	if ( freeMemory ) {
		allocated = 0;
		Mem_Free( filePtr );
		filePtr = NULL;
		curPtr = NULL;
	} else {
		curPtr = filePtr;
	}
}

/*
=================
aRcFileMemory::SetData
=================
*/
void aRcFileMemory::SetData( const char *data, int length ) {
	maxSize = 0;
	fileSize = length;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_READ );
	filePtr = const_cast<char *>( data);
	curPtr = const_cast<char *>( data);
}

/*
========================
aRcFileMemory::TruncateData
========================
*/
void aRcFileMemory::TruncateData( size_t len ) {
	if ( len > allocated ) {
		arcLibrary::Error( "aRcFileMemory::TruncateData: len (%d) exceeded allocated size (%d)", len, allocated );
	} else {
		fileSize = len;
	}
}

/*
=================================================================================

arcFile_BitMsg

=================================================================================
*/

/*
=================
arcFile_BitMsg::arcFile_BitMsg
=================
*/
arcFile_BitMsg::arcFile_BitMsg( ARCBitMessage &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_WRITE );
	this->msg = &msg;
}

/*
=================
arcFile_BitMsg::arcFile_BitMsg
=================
*/
arcFile_BitMsg::arcFile_BitMsg( const ARCBitMessage &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_READ );
	this->msg = const_cast<ARCBitMessage *>(&msg);
}

/*
=================
arcFile_BitMsg::~arcFile_BitMsg
=================
*/
arcFile_BitMsg::~arcFile_BitMsg() {
}

/*
=================
arcFile_BitMsg::Read
=================
*/
int arcFile_BitMsg::Read( void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "arcFile_BitMsg::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	return msg->ReadData( buffer, len );
}

/*
=================
arcFile_BitMsg::Write
=================
*/
int arcFile_BitMsg::Write( const void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "aRcFileMemory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	msg->WriteData( buffer, len );
	return len;
}

/*
=================
arcFile_BitMsg::Length
=================
*/
int arcFile_BitMsg::Length() const {
	return msg->GetSize();
}

/*
=================
arcFile_BitMsg::Timestamp
=================
*/
ARC_TIME_T arcFile_BitMsg::Timestamp() const {
	return 0;
}

/*
=================
arcFile_BitMsg::Tell
=================
*/
int arcFile_BitMsg::Tell() const {
	if ( mode == FS_READ ) {
		return msg->GetReadCount();
	} else {
		return msg->GetSize();
	}
}

/*
=================
arcFile_BitMsg::ForceFlush
=================
*/
void arcFile_BitMsg::ForceFlush() {
}

/*
=================
arcFile_BitMsg::Flush
=================
*/
void arcFile_BitMsg::Flush() {
}

/*
=================
arcFile_BitMsg::Seek

  returns zero on success and -1 on failure
=================
*/
int arcFile_BitMsg::Seek( long offset, fsOrigin_t origin ) {
	return -1;
}


/*
=================================================================================

arcFile_Permanent

=================================================================================
*/

/*
=================
arcFile_Permanent::arcFile_Permanent
=================
*/
arcFile_Permanent::arcFile_Permanent() {
	name = "invalid";
	o = NULL;
	mode = 0;
	fileSize = 0;
	handleSync = false;
}

/*
=================
arcFile_Permanent::~arcFile_Permanent
=================
*/
arcFile_Permanent::~arcFile_Permanent() {
	if ( o ) {
		CloseHandle( o );
	}
}

/*
=================
arcFile_Permanent::Read

Properly handles partial reads
=================
*/
int arcFile_Permanent::Read( void *buffer, int len ) {
	int		block, remaining;
	int		read;
	byte *	buf;
	int		tries;

	if ( !(mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "arcFile_Permanent::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	buf = ( byte * )buffer;

	remaining = len;
	tries = 0;
	while( remaining ) {
		block = remaining;
		DWORD bytesRead;
		if ( !ReadFile( o, buf, block, &bytesRead, NULL ) ) {
			arcLibrary::Warning( "arcFile_Permanent::Read failed with %d from %s", GetLastError(), name.c_str() );
		}
		read = bytesRead;
		if ( read == 0 ) {
			// we might have been trying to read from a CD, which
			// sometimes returns a 0 read on windows
			if ( !tries ) {
				tries = 1;
			}
			else {
				return len-remaining;
			}
		}

		if ( read == -1 ) {
			common->FatalError( "arcFile_Permanent::Read: -1 bytes read from %s", name.c_str() );
		}

		remaining -= read;
		buf += read;
	}
	return len;
}

/*
=================
arcFile_Permanent::Write

Properly handles partial writes
=================
*/
int arcFile_Permanent::Write( const void *buffer, int len ) {
	int		block, remaining;
	int		written;
	byte *	buf;
	int		tries;

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "arcFile_Permanent::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	buf = ( byte * )buffer;

	remaining = len;
	tries = 0;
	while( remaining ) {
		block = remaining;
		DWORD bytesWritten;
		WriteFile( o, buf, block, &bytesWritten, NULL );
		written = bytesWritten;
		if ( written == 0 ) {
			if ( !tries ) {
				tries = 1;
			}
			else {
				common->Printf( "arcFile_Permanent::Write: 0 bytes written to %s\n", name.c_str() );
				return 0;
			}
		}

		if ( written == -1 ) {
			common->Printf( "arcFile_Permanent::Write: -1 bytes written to %s\n", name.c_str() );
			return 0;
		}

		remaining -= written;
		buf += written;
		fileSize += written;
	}
	if ( handleSync ) {
		Flush();
	}
	return len;
}

/*
=================
arcFile_Permanent::ForceFlush
=================
*/
void arcFile_Permanent::ForceFlush() {
	FlushFileBuffers( o );
}

/*
=================
arcFile_Permanent::Flush
=================
*/
void arcFile_Permanent::Flush() {
	FlushFileBuffers( o );
}

/*
=================
arcFile_Permanent::Tell
=================
*/
int arcFile_Permanent::Tell() const {
	return SetFilePointer( o, 0, NULL, FILE_CURRENT );
}

/*
================
arcFile_Permanent::Length
================
*/
int arcFile_Permanent::Length() const {
	return fileSize;
}

/*
================
arcFile_Permanent::Timestamp
================
*/
ARC_TIME_T arcFile_Permanent::Timestamp() const {
	ARC_TIME_T ts = Sys_FileTimeStamp( o );
	return ts;
}

/*
=================
arcFile_Permanent::Seek

  returns zero on success and -1 on failure
=================
*/
int arcFile_Permanent::Seek( long offset, fsOrigin_t origin ) {
	int retVal = INVALID_SET_FILE_POINTER;
	switch( origin ) {
		case FS_SEEK_CUR: retVal = SetFilePointer( o, offset, NULL, FILE_CURRENT ); break;
		case FS_SEEK_END: retVal = SetFilePointer( o, offset, NULL, FILE_END ); break;
		case FS_SEEK_SET: retVal = SetFilePointer( o, offset, NULL, FILE_BEGIN ); break;
	}
	return ( retVal == INVALID_SET_FILE_POINTER ) ? -1 : 0;
}

#if 1
/*
=================================================================================

arcFile_Cached

=================================================================================
*/

/*
=================
arcFile_Cached::arcFile_Cached
=================
*/
arcFile_Cached::arcFile_Cached() : arcFile_Permanent() {
	internalFilePos = 0;
	bufferedStartOffset = 0;
	bufferedEndOffset = 0;
	buffered = NULL;
}

/*
=================
arcFile_Cached::~arcFile_Cached
=================
*/
arcFile_Cached::~arcFile_Cached() {
	Mem_Free( buffered );
}

/*
=================
arcFile_ReadBuffered::BufferData

Buffer a section of the file
=================
*/
void arcFile_Cached::CacheData( uint64 offset, uint64 length ) {
	Mem_Free( buffered );
	bufferedStartOffset = offset;
	bufferedEndOffset = offset + length;
	buffered = ( byte* )Mem_Alloc( length, TAG_RESOURCE );
	if ( buffered == NULL ) {
		return;
	}
	int internalFilePos = arcFile_Permanent::Tell();
	arcFile_Permanent::Seek( offset, FS_SEEK_SET );
	arcFile_Permanent::Read( buffered, length );
	arcFile_Permanent::Seek( internalFilePos, FS_SEEK_SET );
}

/*
=================
arcFile_ReadBuffered::Read

=================
*/
int arcFile_Cached::Read( void *buffer, int len ) {
	if ( internalFilePos >= bufferedStartOffset && internalFilePos + len < bufferedEndOffset ) {
		// this is in the buffer
		memcpy( buffer, (void*)&buffered[ internalFilePos - bufferedStartOffset ], len );
		internalFilePos += len;
		return len;
	}
	int read = arcFile_Permanent::Read( buffer, len );
	if ( read != -1 ) {
		internalFilePos += ( int64 )read;
	}
	return read;
}



/*
=================
arcFile_Cached::Tell
=================
*/
int arcFile_Cached::Tell() const {
	return internalFilePos;
}

/*
=================
arcFile_Cached::Seek

  returns zero on success and -1 on failure
=================
*/
int arcFile_Cached::Seek( long offset, fsOrigin_t origin ) {
	if ( origin == FS_SEEK_SET && offset >= bufferedStartOffset && offset < bufferedEndOffset ) {
		// don't do anything to the actual file ptr, just update or internal position
		internalFilePos = offset;
		return 0;
	}

	int retVal = arcFile_Permanent::Seek( offset, origin );
	internalFilePos = arcFile_Permanent::Tell();
	return retVal;
}
#endif

/*
=================================================================================

arcFile_InZip

=================================================================================
*/

/*
=================
arcFile_InZip::arcFile_InZip
=================
*/
arcFile_InZip::arcFile_InZip() {
	name = "invalid";
	zipFilePos = 0;
	fileSize = 0;
	memset( &z, 0, sizeof( z ) );
}

/*
=================
arcFile_InZip::~arcFile_InZip
=================
*/
arcFile_InZip::~arcFile_InZip() {
	unzCloseCurrentFile( z );
	unzClose( z );
}

/*
=================
arcFile_InZip::Read

Properly handles partial reads
=================
*/
int arcFile_InZip::Read( void *buffer, int len ) {
	int l = unzReadCurrentFile( z, buffer, len );
	return l;
}

/*
=================
arcFile_InZip::Write
=================
*/
int arcFile_InZip::Write( const void *buffer, int len ) {
	common->FatalError( "arcFile_InZip::Write: cannot write to the zipped file %s", name.c_str() );
	return 0;
}

/*
=================
arcFile_InZip::ForceFlush
=================
*/
void arcFile_InZip::ForceFlush() {
	common->FatalError( "arcFile_InZip::ForceFlush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
arcFile_InZip::Flush
=================
*/
void arcFile_InZip::Flush() {
	common->FatalError( "arcFile_InZip::Flush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
arcFile_InZip::Tell
=================
*/
int arcFile_InZip::Tell() const {
	return unztell( z );
}

/*
================
arcFile_InZip::Length
================
*/
int arcFile_InZip::Length() const {
	return fileSize;
}

/*
================
arcFile_InZip::Timestamp
================
*/
ARC_TIME_T arcFile_InZip::Timestamp() const {
	return 0;
}

/*
=================
arcFile_InZip::Seek

  returns zero on success and -1 on failure
=================
*/
#define ZIP_SEEK_BUF_SIZE	(1<<15)

int arcFile_InZip::Seek( long offset, fsOrigin_t origin ) {
	int res, i;
	char *buf;

	switch( origin ) {
		case FS_SEEK_END: {
			offset = fileSize - offset;
		}
		case FS_SEEK_SET: {
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition( z, zipFilePos );
			unzOpenCurrentFile( z );
			if ( offset <= 0 ) {
				return 0;
			}
		}
		case FS_SEEK_CUR: {
			buf = (char *) _alloca16( ZIP_SEEK_BUF_SIZE );
			for ( i = 0; i < ( offset - ZIP_SEEK_BUF_SIZE ); i += ZIP_SEEK_BUF_SIZE ) {
				res = unzReadCurrentFile( z, buf, ZIP_SEEK_BUF_SIZE );
				if ( res < ZIP_SEEK_BUF_SIZE ) {
					return -1;
				}
			}
			res = i + unzReadCurrentFile( z, buf, offset - i );
			return ( res == offset ) ? 0 : -1;
		}
		default: {
			common->FatalError( "arcFile_InZip::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}
	return -1;
}

#if 1

/*
=================================================================================

arcFile_InnerResource

=================================================================================
*/

/*
=================
arcFile_InnerResource::arcFile_InnerResource
=================
*/
arcFile_InnerResource::arcFile_InnerResource( const char *_name, arcNetFile *rezFile, int _offset, int _len ) {
	name = _name;
	offset = _offset;
	length = _len;
	resourceFile = rezFile;
	internalFilePos = 0;
	resourceBuffer = NULL;
}

/*
=================
arcFile_InnerResource::~arcFile_InnerResource
=================
*/
arcFile_InnerResource::~arcFile_InnerResource() {
	if ( resourceBuffer != NULL ) {
		fileSystem->FreeResourceBuffer();
	}
}

/*
=================
arcFile_InnerResource::Read

Properly handles partial reads
=================
*/
int arcFile_InnerResource::Read( void *buffer, int len ) {
	if ( resourceFile == NULL ) {
		return 0;
	}

	if ( internalFilePos + len > length ) {
		len = length - internalFilePos;
	}

	int read = 0; //fileSystem->ReadFromBGL( resourceFile, (byte*)buffer, offset + internalFilePos, len );

	if ( read != len ) {
		if ( resourceBuffer != NULL ) {
			memcpy( buffer, &resourceBuffer[ internalFilePos ], len );
			read = len;
		} else {
			read = fileSystem->ReadFromBGL( resourceFile, buffer, offset + internalFilePos, len );
		}
	}

	internalFilePos += read;

	return read;
}

/*
=================
arcFile_InnerResource::Tell
=================
*/
int arcFile_InnerResource::Tell() const {
	return internalFilePos;
}


/*
=================
arcFile_InnerResource::Seek

  returns zero on success and -1 on failure
=================
*/

int arcFile_InnerResource::Seek( long offset, fsOrigin_t origin ) {
	switch( origin ) {
		case FS_SEEK_END: {
			internalFilePos = length - offset - 1;
			return 0;
		}
		case FS_SEEK_SET: {
			internalFilePos = offset;
			if ( internalFilePos >= 0 && internalFilePos < length ) {
				return 0;
			}
			return -1;
		}
		case FS_SEEK_CUR: {
			internalFilePos += offset;
			if ( internalFilePos >= 0 && internalFilePos < length ) {
				return 0;
			}
			return -1;
		}
		default: {
			common->FatalError( "arcFile_InnerResource::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}
	return -1;
}
#endif

/*
================================================================================================

arcFileLocal

================================================================================================
*/

/*
========================
arcFileLocal::~arcFileLocal

Destructor that will destroy (close) the managed file when this wrapper class goes out of scope.
========================
*/
arcFileLocal::~arcFileLocal() {
	if ( file != NULL ) {
		delete file;
		file = NULL;
	}
}

static const char * testEndianNessFilename = "temp.bin";
struct testEndianNess_t {
	testEndianNess_t() {
		a = 0x12345678;
		b = 0x12345678;
		c = 3.0f;
		d = -4.0f;
		e = "test";
		f = arcVec3( 1.0f, 2.0f, -3.0f );
		g = false;
		h = true;
		for ( int index = 0; index < sizeof( i ); index++ ) {
			i[index] = 0x37;
		}
	}
	bool operator==( testEndianNess_t & test ) const {
		return a == test.a &&
			b == test.b &&
			c == test.c &&
			d == test.d &&
			e == test.e &&
			f == test.f &&
			g == test.g &&
			h == test.h &&
			( memcmp( i, test.i, sizeof( i ) ) == 0 );
	}
	int				a;
	unsigned int	b;
	float			c;
	float			d;
	arcNetString			e;
	arcVec3			f;
	bool			g;
	bool			h;
	byte			i[10];
};
CONSOLE_COMMAND( testEndianNessWrite, "Tests the read/write compatibility between platforms", 0 ) {
	arcFileLocal file( fileSystem->OpenFileWrite( testEndianNessFilename ) );
	if ( file == NULL ) {
		arcLibrary::Printf( "Couldn't open the %s testfile.\n", testEndianNessFilename );
		return;
	}

	testEndianNess_t testData;

	file->WriteBig( testData.a );
	file->WriteBig( testData.b );
	file->WriteFloat( testData.c );
	file->WriteFloat( testData.d );
	file->WriteString( testData.e );
	file->WriteVec3( testData.f );
	file->WriteBig( testData.g );
	file->WriteBig( testData.h );
	file->Write( testData.i, sizeof( testData.i )/ sizeof( testData.i[0] ) );
}

CONSOLE_COMMAND( testEndianNessRead, "Tests the read/write compatibility between platforms", 0 ) {
	arcFileLocal file( fileSystem->OpenFileRead( testEndianNessFilename ) );
	if ( file == NULL ) {
		arcLibrary::Printf( "Couldn't find the %s testfile.\n", testEndianNessFilename );
		return;
	}

	testEndianNess_t srcData;
	testEndianNess_t testData;

	memset( &testData, 0, sizeof( testData ) );

	file->ReadBig( testData.a );
	file->ReadBig( testData.b );
	file->ReadFloat( testData.c );
	file->ReadFloat( testData.d );
	file->ReadString( testData.e );
	file->ReadVec3( testData.f );
	file->ReadBig( testData.g );
	file->ReadBig( testData.h );
	file->Read( testData.i, sizeof( testData.i )/ sizeof( testData.i[0] ) );

	assert( srcData == testData );
}

CONSOLE_COMMAND( testEndianNessReset, "Tests the read/write compatibility between platforms", 0 ) {
	fileSystem->RemoveFile( testEndianNessFilename );
}


