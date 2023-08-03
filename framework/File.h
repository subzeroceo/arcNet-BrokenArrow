#ifndef __FILE_H__
#define __FILE_H__
#include <cstdio>
/*
==============================================================

  File Stream

==============================================================
*/
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
// mode parm for Seek
typedef enum {
	FS_SEEK_SET = SEEK_SET,
	FS_SEEK_CUR	= SEEK_CUR,
	FS_SEEK_END	= SEEK_END,
} fsOrigin_t;

class anFileSystem;
class anFile {
public:
	virtual					~anFile( void ) {};
							// Get the name of the file.
	virtual const char *	GetName( void );
							// Get the full file path.
	virtual const char *	GetFullPath( void );
							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len );
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len );
							// Returns the length of the file.
	virtual int				Length( void );
							// Return a time value for reload operations.
	virtual ARC_TIME_T		Timestamp( void );
							// Returns offset in file.
	virtual int				Tell( void );
							// Forces flush on files being writting to.
	virtual void			ForceFlush( void );
							// Causes any buffered data to be written to the file.
	virtual void			Flush( void );
							// Seek on a file.
	virtual int				Seek( long offset, fsOrigin_t origin );
							// Go back to the beginning of the file.
	virtual void			Rewind( void );
							// Like fprintf.
	virtual int				Printf( const char *fmt, ... ) an_attribute( ( format( printf, 2, 3 )  ) );
							// Like fprintf but with argument pointer
	virtual int				VPrintf( const char *fmt, va_list arg );
							// Write a string with high precision floating point numbers to the file.
	virtual int				WriteFloatString( const char *fmt, ... ) an_attribute( ( format( printf, 2, 3 ) ) );
	
	// Endian portable alternatives to Read(...)
	virtual int				ReadInt( int &value );
	virtual int				ReadUnsignedInt( unsigned int &value );
	virtual int				ReadShort( short &value );
	virtual int				ReadUnsignedShort( unsigned short &value );
	virtual int				ReadChar( char &value );
	virtual int				ReadUnsignedChar( unsigned char &value );
	virtual int				ReadFloat( float &value );
	virtual int				ReadDouble( double &value );
	virtual int				ReadBool( bool &value );
	virtual int				ReadString( anString &string );
	virtual int				ReadVec2( anVec2 &vec );
	virtual int				ReadVec3( anVec3 &vec );
	virtual int				ReadVec4( anVec4 &vec );
	virtual int				ReadVec6( anVec6 &vec );
	virtual int				ReadMat3( anMat3 &mat );
	virtual int 			ReadCQuat( anCQuat &quat );
	virtual int 			ReadAngles( anAngles &angles );
	virtual int				ReadRadians( anRadians &rads );
	virtual int				Read1DFloatArray( float *dst );
	virtual int				ReadFloatArray( float *src, const int num );

	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value );
	virtual int				WriteUnsignedInt( const unsigned int value );
	virtual int				WriteShort( const short value );
	virtual int				WriteUnsignedShort( unsigned short value );
	virtual int				WriteChar( const char value );
	virtual int				WriteUnsignedChar( const unsigned char value );
	virtual int				WriteFloat( const float value );
	virtual int				WriteDouble( const double value );
	virtual int				WriteBool( const bool value );
	virtual int				WriteString( const char *string );
	virtual int				WriteVec2( const anVec2 &vec );
	virtual int				WriteVec3( const anVec3 &vec );
	virtual int				WriteVec4( const anVec4 &vec );
	virtual int				WriteVec6( const anVec6 &vec );
	virtual int				WriteMat3( const anMat3 &mat );
	virtual int				WriteCQuat( const anCQuat &quat );
	virtual int				WriteAngles( const anAngles &angles );
	virtual int				WriteRadians( const anRadians &rads );
	virtual int				Write1DFloatArray( const int num, const float *src );
	virtual int				WriteFloatArray( const float *src, const int num );
};

class anFileMemory : public anFile {
	friend class			anFileSystem;
public:
							anFileMemory( void );	// file for writing without name
							anFileMemory( const char *name );	// file for writing
							anFileMemory( const char *name, char *data, int length );	// file for writing
							anFileMemory( const char *name, const char *data, int length );	// file for reading
	virtual					~anFileMemory( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void );
	virtual ARC_TIME_T		Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

							// changes memory file to read only
	virtual void			MakeReadOnly( void );
							// clear the file
	virtual void			Clear( bool freeMemory = true );
							// set data for reading
	void					SetData( const char *data, int length );
							// returns const pointer to the memory buffer
	const char *			GetDataPtr( void ) const { return filePtr; }
							// set the file granularity
	void					SetGranularity( int g ) { assert( g > 0 ); granularity = g; }

private:
	anString				name;			// name of the file
	int						mode;			// open mode
	int						maxSize;		// maximum size of file
	int						fileSize;		// size of the file
	int						allocated;		// allocated size
	int						granularity;	// file granularity
	char *					filePtr;		// buffer holding the file data
	char *					curPtr;			// current read/write pointer
};

class anFileBitMsg : public anFile {
	friend class			anFileSystem;
public:
							anFileBitMsg( anBitMsg &msg );
							anFileBitMsg( const anBitMsg &msg );
	virtual					~anFileBitMsg( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void );
	virtual ARC_TIME_T		Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	anString				name;			// name of the file
	int						mode;			// open mode
	anBitMsg *				msg;
};

class anFilePermanent : public anFile {
	friend class			anFileSystem;
public:
							anFilePermanent( void );
	virtual					~anFilePermanent( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void );
	virtual ARC_TIME_T		Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

	// returns file pointer
	FILE *					GetFilePtr( void ) { return o; }

private:
	anString				name;			// relative path of the file - relative path
	anString				fullPath;		// full file path - OS path
	int						mode;			// open mode
	int						fileSize;		// size of the file
	FILE *					o;				// file handle
	bool					handleSync;		// true if written data is immediately flushed
};

class anFileBuffered : public anFile {
	friend class			anFileSystem;

public:
							anFileBuffered( const int granularity = 2048 * 1024 );	// file for reading
							anFileBuffered( anFile *source, const int granularity = 2048 * 1024 );	// file for reading
	 							~anFileBuffered( void );

	const char *			GetName( void ) { return source->GetName(); }
	const char *			GetFullPath( void ) { return source->GetFullPath(); }
	int						Read( void *buffer, int len );
	int						Length( void ) const { return source->Length(); }
	unsigned int			Timestamp( void ) { return source->Timestamp(); }
	int						Tell( void );
	int						Seek( long offset, fsOrigin_t origin );

	void					SetSource( anFile *source );
	void					ReleaseSource();

private:
	int						ReadInternal( void *buffer, int len );
	void					SeekInternal( long offset );

private:
	anFile *				source;			// source file pointer
	int						granularity;	// file granularity
	int						available;		// current amount of data left in buffer
	long					sourceOffset;	// offset in source file of file data
	const byte *			filePtr;		// buffer holding the file data
	const byte *			curPtr;			// current read pointer
};

class anCompressedArchive : public anFile {
	friend class			anFileSystem;
public:
							anCompressedArchive( void );
	virtual					~anCompressedArchive( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void );
	virtual ARC_TIME_T		Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	anString				name;			// name of the file in the pak
	anString				fullPath;		// full file path including pak file name
	int						zipFilePos;		// zip file info position in pak
	int						fileSize;		// size of the file
	void *					z;				// unzip info
};

class anFileCached : public idFile_Permanent {
	friend class			idFileSystemLocal;
public:
	anFileCached();
	virtual					~anFileCached();

	void					CacheData( uint64 offset, uint64 length );

	int						Read( void *buffer, int len );

	int						Tell() const;
	int						Seek( long offset, fsOrigin_t origin );

private:
	uint				internalFilePos;
	uint				bufferedStartOffset;
	uint				bufferedEndOffset;
	byte *				buffered;
};

#endif // !__FILE_H__