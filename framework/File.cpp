#include "../idlib/Lib.h"
#pragma hdrstop

#include "Unzip.h"

#define	MAX_PRINT_MSG		4096

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
	anString tmp, format;

	index = 0;

	while ( *fmt ) {
		switch ( *fmt ) {
			case '%':
				format = "";
				format += *fmt++;
				while ( (*fmt >= '0' && *fmt <= '9') ||
						*fmt == '.' || *fmt == '-' || *fmt == '+' || *fmt == '#') {
					format += *fmt++;
				}
				format += *fmt;
				switch ( *fmt ) {
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
						} else {
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
						index += sprintf( buf+index, format.c_str() );
						break;
					default:
						common->Error( "FS_WriteFloatString: invalid format %s", format.c_str() );
						break;
				}
				fmt++;
				break;
			case '\\':
				fmt++;
				switch ( *fmt ) {
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

anFile

=================================================================================
*/

/*
=================
anFile::GetName
=================
*/
const char *anFile::GetName( void ) {
	return "";
}

/*
=================
anFile::GetFullPath
=================
*/
const char *anFile::GetFullPath( void ) {
	return "";
}

/*
=================
anFile::Read
=================
*/
int anFile::Read( void *buffer, int len ) {
	common->FatalError( "anFile::Read: cannot read from anFile" );
	return 0;
}

/*
=================
anFile::Write
=================
*/
int anFile::Write( const void *buffer, int len ) {
	common->FatalError( "anFile::Write: cannot write to anFile" );
	return 0;
}

/*
=================
anFile::Length
=================
*/
int anFile::Length( void ) {
	anFile *h;
	long pos = Tell( h ) ;
	Seek( h, 0, SEEK_END );
	long end = Tell( h );
	Seek( h, pos, SEEK_SET );
	return end;//0;
}

/*
=================
anFile::Timestamp
=================
*/
ARC_TIME_T anFile::Timestamp( void ) {
	return 0;
}

/*
=================
anFile::Tell
=================
*/
int anFile::Tell( void ) {
	return 0;
}

/*
=================
anFile::ForceFlush
=================
*/
void anFile::ForceFlush( void ) {
}

/*
=================
anFile::Flush
=================
*/
void anFile::Flush( void ) {
}

/*
=================
anFile::Seek
=================
*/
int anFile::Seek( long offset, fsOrigin_t origin ) {
	return -1;
}

/*
=================
anFile::Rewind
=================
*/
void anFile::Rewind( void ) {
	Seek( 0, FS_SEEK_SET );
}

/*
=================
anFile::Printf
=================
*/
int anFile::Printf( const char *fmt, ... ) {
	char buf[MAX_PRINT_MSG];
	int length;
	va_list argptr;

	va_start( argptr, fmt );
	length = anString::vsnPrintf( buf, MAX_PRINT_MSG-1, fmt, argptr );
	va_end( argptr );

	// so notepad formats the lines correctly
  	anString	work( buf );
 	work.Replace( "\n", "\r\n" );
  
  	return Write( work.c_str(), work.Length() );
}

/*
=================
anFile::VPrintf
=================
*/
int anFile::VPrintf( const char *fmt, va_list args ) {
	char buf[MAX_PRINT_MSG];
	int length;

	length = anString::vsnPrintf( buf, MAX_PRINT_MSG-1, fmt, args );
	return Write( buf, length );
}

/*
=================
anFile::WriteFloatString
=================
*/
int anFile::WriteFloatString( const char *fmt, ... ) {
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
 anFile::ReadInt
 =================
 */
int anFile::ReadInt( int &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleLong( value );
	return result;
}

/*
 =================
 anFile::ReadUnsignedInt
 =================
 */
int anFile::ReadUnsignedInt( unsigned int &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleLong( value );
	return result;
}

/*
 =================
 anFile::ReadShort
 =================
 */
int anFile::ReadShort( short &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleShort( value );
	return result;
}

/*
 =================
 anFile::ReadUnsignedShort
 =================
 */
int anFile::ReadUnsignedShort( unsigned short &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleShort( value );
	return result;
}

/*
 =================
 anFile::ReadChar
 =================
 */
int anFile::ReadChar( char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 anFile::ReadUnsignedChar
 =================
 */
int anFile::ReadUnsignedChar( unsigned char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 anFile::ReadFloat
 =================
 */
int anFile::ReadFloat( float &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleFloat( value );
	return result;
}

/*
 =================
 anFile::ReadBool
 =================
 */
int anFile::ReadBool( bool &value ) {
	unsigned char c;
	int result = ReadUnsignedChar( c );
	value = c ? true : false;
	return result;
}

/*
 =================
 anFile::ReadString
 =================
 */
int anFile::ReadString( anString &string ) {
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
 anFile::ReadVec2
 =================
 */
int anFile::ReadVec2( anVec2 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof( float ), sizeof( vec )/sizeof( float ) );
	return result;
}

/*
 =================
 anFile::ReadVec3
 =================
 */
int anFile::ReadVec3( anVec3 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof( float ), sizeof( vec )/sizeof( float ) );
	return result;
}

/*
 =================
 anFile::ReadVec4
 =================
 */
int anFile::ReadVec4( anVec4 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof( float ), sizeof( vec )/sizeof( float ) );
	return result;
}

/*
 =================
 anFile::ReadVec6
 =================
 */
int anFile::ReadVec6( anVec6 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof( float ), sizeof( vec )/sizeof( float ) );
	return result;
}

/*
 =================
 anFile::ReadMat3
 =================
 */
int anFile::ReadMat3( anMat3 &mat ) {
	int result = Read( &mat, sizeof( mat ) );
	LittleRevBytes( &mat, sizeof( float ), sizeof( mat )/sizeof( float ) );
	return result;
}

/*
 =================
 anFile::WriteInt
 =================
 */
int anFile::WriteInt( const int value ) {
	int v = LittleLong( value );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteUnsignedInt
 =================
 */
int anFile::WriteUnsignedInt( const unsigned int value ) {
	unsigned int v = LittleLong( value );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteShort
 =================
 */
int anFile::WriteShort( const short value ) {
	short v = LittleShort( value );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteUnsignedShort
 =================
 */
int anFile::WriteUnsignedShort( const unsigned short value ) {
	unsigned short v = LittleShort( value );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteChar
 =================
 */
int anFile::WriteChar( const char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 anFile::WriteUnsignedChar
 =================
 */
int anFile::WriteUnsignedChar( const unsigned char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 anFile::WriteFloat
 =================
 */
int anFile::WriteFloat( const float value ) {
	float v = LittleFloat( value );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteBool
 =================
 */
int anFile::WriteBool( const bool value ) {
	unsigned char c = value;
	return WriteUnsignedChar( c );
}

/*
 =================
 anFile::WriteString
 =================
 */
int anFile::WriteString( const char *value ) {
	int len;
	
	len = strlen( value );
	WriteInt( len );
    return Write( value, len );
}

/*
 =================
 anFile::WriteVec2
 =================
 */
int anFile::WriteVec2( const anVec2 &vec ) {
	anVec2 v = vec;
	LittleRevBytes( &v, sizeof( float ), sizeof(v)/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteVec3
 =================
 */
int anFile::WriteVec3( const anVec3 &vec ) {
	anVec3 v = vec;
	LittleRevBytes( &v, sizeof( float ), sizeof(v)/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteVec4
 =================
 */
int anFile::WriteVec4( const anVec4 &vec ) {
	anVec4 v = vec;
	LittleRevBytes( &v, sizeof( float ), sizeof(v)/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteVec6
 =================
 */
int anFile::WriteVec6( const anVec6 &vec ) {
	anVec6 v = vec;
	LittleRevBytes( &v, sizeof( float ), sizeof( v )/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 anFile::WriteMat3
 =================
 */
int anFile::WriteMat3( const anMat3 &mat ) {
	anMat3 v = mat;
	LittleRevBytes( &v, sizeof( float ), sizeof( v )/sizeof( float ) );
	return Write( &v, sizeof( v ) );
}

/*
=================================================================================

anFileMemory

=================================================================================
*/


/*
=================
anFileMemory::anFileMemory
=================
*/
anFileMemory::anFileMemory( void ) {
	name = "*unknown*";
	maxSize = 0;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = nullptrptr;
	curPtr = nullptrptr;
}

/*
=================
anFileMemory::anFileMemory
=================
*/
anFileMemory::anFileMemory( const char *name ) {
	this->name = name;
	maxSize = 0;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = nullptrptr;
	curPtr = nullptrptr;
}

/*
=================
anFileMemory::anFileMemory
=================
*/
anFileMemory::anFileMemory( const char *name, char *data, int length ) {
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
anFileMemory::anFileMemory
=================
*/
anFileMemory::anFileMemory( const char *name, const char *data, int length ) {
	this->name = name;
	maxSize = 0;
	fileSize = length;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_READ );
	filePtr = const_cast<char *>( data );
	curPtr = data;
}

/*
=================
anFileMemory::~anFileMemory
=================
*/
anFileMemory::~anFileMemory( void ) {
	if ( filePtr && allocated > 0 && maxSize == 0 ) {
		Mem_Free( filePtr );
	}
}

/*
=================
anFileMemory::Read
=================
*/
int anFileMemory::Read( void *buffer, int len ) {
	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "anFileMemory::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( curPtr + len > filePtr + fileSize ) {
		len = filePtr + fileSize - curPtr;
	}
	memcpy( buffer, curPtr, len );
	curPtr += len;
	return len;
}

/*
=================
anFileMemory::Write
=================
*/
int anFileMemory::Write( const void *buffer, int len ) {
	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "anFileMemory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	int alloc = curPtr + len + 1 - filePtr - allocated; // need room for len+1
	if ( alloc > 0 ) {
		if ( maxSize != 0 ) {
			common->Error( "anFileMemory::Write: exceeded maximum size %d", maxSize );
			return 0;
		}
		int extra = granularity * ( 1 + alloc / granularity );
		char *newPtr = (char *) Mem_Alloc( allocated + extra );
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
	memcpy( curPtr, buffer, len );
	curPtr += len;
	fileSize += len;
	filePtr[ fileSize ] = 0; // len + 1
	return len;
}

/*
=================
anFileMemory::Length
=================
*/
int anFileMemory::Length( void ) {
	return fileSize;
}

/*
=================
anFileMemory::Timestamp
=================
*/
ARC_TIME_T anFileMemory::Timestamp( void ) {
	return 0;
}

/*
=================
anFileMemory::Tell
=================
*/
int anFileMemory::Tell( void ) {
	return ( curPtr - filePtr );
}

/*
=================
anFileMemory::ForceFlush
=================
*/
void anFileMemory::ForceFlush( void ) {
}

/*
=================
anFileMemory::Flush
=================
*/
void anFileMemory::Flush( void ) {
	anFile *file = FS_FileForHandle( f );
	setvbuf( file, nullptrptr, _IONBF, 0 );
}

/*
=================
anFileMemory::Seek

returns zero on success and -1 on failure
=================
*/
int anFileMemory::Seek( long offset, fsOrigin_t origin ) {
	switch ( origin ) {
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
			common->FatalError( "anFileMemory::Seek: bad origin for %s\n", name.c_str() );
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
=================
anFileMemory::MakeReadOnly
=================
*/
void anFileMemory::MakeReadOnly( void ) {
	mode = ( 1 << FS_READ );
	Rewind();
}

/*
=================
anFileMemory::Clear
=================
*/
void anFileMemory::Clear( bool freeMemory ) {
	fileSize = 0;
	granularity = 16384;
	if ( freeMemory ) {
		allocated = 0;
		Mem_Free( filePtr );
		filePtr = nullptrptr;
		curPtr = nullptrptr;
	} else {
		curPtr = filePtr;
	}
}

/*
=================
anFileMemory::SetData
=================
*/
void anFileMemory::SetData( const char *data, int length ) {
	fileSize = length;
	maxSize = 0;
	allocated = 0;
	granularity = 16384;
	mode = 1 << FS_READ;
	filePtr = const_cast<char *>( data );
	curPtr = filePtr;
}

/*
=================================================================================

anFileBitMsg

=================================================================================
*/

/*
=================
anFileBitMsg::anFileBitMsg
=================
*/
anFileBitMsg::anFileBitMsg( anBitMsg &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_WRITE );
	this->msg = &msg;
}

/*
=================
anFileBitMsg::anFileBitMsg
=================
*/
anFileBitMsg::anFileBitMsg( const anBitMsg &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_READ );
	this->msg = const_cast<anBitMsg *>( &msg );
}

/*
=================
anFileBitMsg::~anFileBitMsg
=================
*/
anFileBitMsg::~anFileBitMsg( void ) {
}

/*
=================
anFileBitMsg::Read
=================
*/
int anFileBitMsg::Read( void *buffer, int len ) {
	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "anFileBitMsg::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	return msg->ReadData( buffer, len );
}

/*
=================
anFileBitMsg::Write
=================
*/
int anFileBitMsg::Write( const void *buffer, int len ) {
	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "anFileMemory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	msg->WriteData( buffer, len );
	return len;
}

/*
=================
anFileBitMsg::Length
=================
*/
int anFileBitMsg::Length( void ) {
	return msg->GetSize();
}

/*
=================
anFileBitMsg::Timestamp
=================
*/
ARC_TIME_T anFileBitMsg::Timestamp( void ) {
	return 0;
}

/*
=================
anFileBitMsg::Tell
=================
*/
int anFileBitMsg::Tell( void ) {
	if ( mode & FS_READ ) {
		return msg->GetReadCount();
	} else {
		return msg->GetSize();
	}
}

/*
=================
anFileBitMsg::ForceFlush
=================
*/
void anFileBitMsg::ForceFlush( void ) {
}

/*
=================
anFileBitMsg::Flush
=================
*/
void anFileBitMsg::Flush( void ) {
}

/*
=================
anFileBitMsg::Seek

  returns zero on success and -1 on failure
=================
*/
int anFileBitMsg::Seek( long offset, fsOrigin_t origin ) {
	return -1;
}


/*
=================================================================================

anFilePermanent

=================================================================================
*/

/*
=================
anFilePermanent::anFilePermanent
=================
*/
anFilePermanent::anFilePermanent( void ) {
	name = "invalid";
	o = nullptr;
	mode = 0;
	fileSize = 0;
	handleSync = false;
}

/*
=================
anFilePermanent::~anFilePermanent
=================
*/
anFilePermanent::~anFilePermanent( void ) {
	if ( o ) {
		fclose( o );
	}
}

/*
=================
anFilePermanent::Read

Properly handles partial reads
=================
*/
int anFilePermanent::Read( void *buffer, int len ) {
	int		block, remaining;
	int		read;
	byte *	buf;
	int		tries;

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "anFilePermanent::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	buf = (byte *)buffer;

	remaining = len;
	tries = 0;
	while ( remaining ) {
		block = remaining;
		read = fread( buf, 1, block, o );
		if ( read == 0 ) {
			// we might have been trying to read from a CD, which
			// sometimes returns a 0 read on windows
			if ( !tries ) {
				tries = 1;
			} else {
				fileSystem->AddToReadCount( len - remaining );
				return len-remaining;
			}
		}

		if ( read == -1 ) {
			common->FatalError( "anFilePermanent::Read: -1 bytes read from %s", name.c_str() );
		}

		remaining -= read;
		buf += read;
	}
	fileSystem->AddToReadCount( len );
	return len;
}

/*
=================
anFilePermanent::Write

Properly handles partial writes
=================
*/
int anFilePermanent::Write( const void *buffer, int len ) {
	int		block, remaining;
	int		written;
	byte *	buf;
	int		tries;

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "anFilePermanent::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	buf = (byte *)buffer;

	remaining = len;
	tries = 0;
	while( remaining ) {
		block = remaining;
		written = fwrite( buf, 1, block, o );
		if ( written == 0 ) {
			if ( !tries ) {
				tries = 1;
			}
			else {
				common->Printf( "anFilePermanent::Write: 0 bytes written to %s\n", name.c_str() );
				return 0;
			}
		}

		if ( written == -1 ) {
			common->Printf( "anFilePermanent::Write: -1 bytes written to %s\n", name.c_str() );
			return 0;
		}

		remaining -= written;
		buf += written;
		fileSize += written;
	}
	if ( handleSync ) {
		fflush( o );
	}
	return len;
}

/*
=================
anFilePermanent::ForceFlush
=================
*/
void anFilePermanent::ForceFlush( void ) {
	setvbuf( o, nullptr, _IONBF, 0 );
}

/*
=================
anFilePermanent::Flush
=================
*/
void anFilePermanent::Flush( void ) {
	fflush( o );
}

/*
=================
anFilePermanent::Tell
=================
*/
int anFilePermanent::Tell( void ) {
	return ftell( o );
}

/*
================
anFilePermanent::Length
================
*/
int anFilePermanent::Length( void ) {
	return fileSize;
}

/*
================
anFilePermanent::Timestamp
================
*/
ARC_TIME_T anFilePermanent::Timestamp( void ) {
	return Sys_FileTimeStamp( o );
}

/*
=================
anFilePermanent::Seek

  returns zero on success and -1 on failure
=================
*/
int anFilePermanent::Seek( long offset, fsOrigin_t origin ) {
	int _origin;

	switch ( origin ) {
		case FS_SEEK_CUR: {
			_origin = SEEK_CUR;
			break;
		}
		case FS_SEEK_END: {
			_origin = SEEK_END;
			break;
		}
		case FS_SEEK_SET: {
			_origin = SEEK_SET;
			break;
		}
		default: {
			_origin = SEEK_CUR;
			common->FatalError( "anFilePermanent::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}

	return fseek( o, offset, _origin );
}


/*
=================================================================================

anCompressedArchive

=================================================================================
*/

/*
=================
anCompressedArchive::anCompressedArchive
=================
*/
anCompressedArchive::anCompressedArchive( void ) {
	name = "invalid";
	zipFilePos = 0;
	fileSize = 0;
	memset( &z, 0, sizeof( z ) );
}

/*
=================
anCompressedArchive::~anCompressedArchive
=================
*/
anCompressedArchive::~anCompressedArchive( void ) {
	PAK_CloseCurrentFile( z );
	PAK_Close( z );
}

/*
=================
anCompressedArchive::Read

Properly handles partial reads
=================
*/
int anCompressedArchive::Read( void *buffer, int len ) {
	int l = unzReadCurrentFile( z, buffer, len );
	fileSystem->AddToReadCount( l );
	return l;
}

/*
=================
anCompressedArchive::Write
=================
*/
int anCompressedArchive::Write( const void *buffer, int len ) {
	common->FatalError( "anCompressedArchive::Write: cannot write to the zipped file %s", name.c_str() );
	return 0;
}

/*
=================
anCompressedArchive::ForceFlush
=================
*/
void anCompressedArchive::ForceFlush( void ) {
	common->FatalError( "anCompressedArchive::ForceFlush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
anCompressedArchive::Flush
=================
*/
void anCompressedArchive::Flush( void ) {
	common->FatalError( "anCompressedArchive::Flush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
anCompressedArchive::Tell
=================
*/
int anCompressedArchive::Tell( void ) {
	return PAK_Tell( z );
}

/*
================
anCompressedArchive::Length
================
*/
int anCompressedArchive::Length( void ) {
	return fileSize;
}

/*
================
anCompressedArchive::Timestamp
================
*/
ARC_TIME_T anCompressedArchive::Timestamp( void ) {
	return 0;
}

/*
=================
anCompressedArchive::Seek

  returns zero on success and -1 on failure
=================
*/
#define ZIP_SEEK_BUF_SIZE	(1<<15)

int anCompressedArchive::Seek( long offset, fsOrigin_t origin ) {
	int res, i;
	char *buf;

	switch ( origin ) {
		case FS_SEEK_END: {
			offset = fileSize - offset;
		}
		case FS_SEEK_SET: {
			// set the file position in the zip file (also sets the current file info)
			PAK_SetFileDataLocation( z, zipFilePos );
			PAK_OpenCurrentFile( z );
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
			common->FatalError( "anCompressedArchive::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}
	return -1;
}
