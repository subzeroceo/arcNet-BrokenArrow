#ifndef __FILE_H__
#define __FILE_H__

/*
==============================================================

  File Streams.

==============================================================
*/

// mode parm for Seek
typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

class aRcFileSystemLocal;


class arcNetFile {
public:
	virtual					~arcNetFile() {};
							// Get the name of the file.
	virtual const char *	GetName() const;
							// Get the full file path.
	virtual const char *	GetFullPath() const;
							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len );
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len );
							// Returns the length of the file.
	virtual int				Length() const;
							// Return a time value for reload operations.
	virtual ARC_TIME_T		Timestamp() const;
							// Returns offset in file.
	virtual int				Tell() const;
							// Forces flush on files being writting to.
	virtual void			ForceFlush();
							// Causes any buffered data to be written to the file.
	virtual void			Flush();
							// Seek on a file.
	virtual int				Seek( long offset, fsOrigin_t origin );
							// Go back to the beginning of the file.
	virtual void			Rewind();
							// Like fprintf.
	virtual int				Printf( VERIFY_FORMAT_STRING const char *fmt, ... );
							// Like fprintf but with argument pointer
	virtual int				VPrintf( const char *fmt, va_list arg );
							// Write a string with high precision floating point numbers to the file.
	virtual int				WriteFloatString( VERIFY_FORMAT_STRING const char *fmt, ... );

	// Endian portable alternatives to Read(...)
	virtual int				ReadInt( int &value );
	virtual int				ReadUnsignedInt( unsigned int &value );
	virtual int				ReadShort( short &value );
	virtual int				ReadUnsignedShort( unsigned short &value );
	virtual int				ReadChar( char &value );
	virtual int				ReadUnsignedChar( unsigned char &value );
	virtual int				ReadFloat( float &value );
	virtual int				ReadBool( bool &value );
	virtual int				ReadString( arcNetString &string );
	virtual int				ReadVec2( arcVec2 &vec );
	virtual int				ReadVec3( arcVec3 &vec );
	virtual int				ReadVec4( arcVec4 &vec );
	virtual int				ReadVec6( arcVec6 &vec );
	virtual int				ReadMat3( arcMat3 &mat );

	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value );
	virtual int				WriteUnsignedInt( const unsigned int value );
	virtual int				WriteShort( const short value );
	virtual int				WriteUnsignedShort( unsigned short value );
	virtual int				WriteChar( const char value );
	virtual int				WriteUnsignedChar( const unsigned char value );
	virtual int				WriteFloat( const float value );
	virtual int				WriteBool( const bool value );
	virtual int				WriteString( const char *string );
	virtual int				WriteVec2( const arcVec2 &vec );
	virtual int				WriteVec3( const arcVec3 &vec );
	virtual int				WriteVec4( const arcVec4 &vec );
	virtual int				WriteVec6( const arcVec6 &vec );
	virtual int				WriteMat3( const arcMat3 &mat );

	template<class type> ARC_INLINE size_t ReadBig( type &c ) {
		size_t r = Read( &c, sizeof( c ) );
		idSwap::Big( c );
		return r;
	}

	template<class type> ARC_INLINE size_t ReadBigArray( type *c, int count ) {
		size_t r = Read( c, sizeof( c[0] ) * count );
		idSwap::BigArray( c, count );
		return r;
	}

	template<class type> ARC_INLINE size_t WriteBig( const type &c ) {
		type b = c;
		idSwap::Big( b );
		return Write( &b, sizeof( b ) );
	}

	template<class type> ARC_INLINE size_t WriteBigArray( const type *c, int count ) {
		size_t r = 0;
		for ( int i = 0; i < count; i++ ) {
			r += WriteBig( c[i] );
		}
		return r;
	}
};

/*
================================================
aRcFileMemory
================================================
*/
class aRcFileMemory : public arcNetFile {
	friend class			aRcFileSystemLocal;

public:
							aRcFileMemory();	// file for writing without name
							aRcFileMemory( const char *name );	// file for writing
							aRcFileMemory( const char *name, char *data, int length );	// file for writing
							aRcFileMemory( const char *name, const char *data, int length );	// file for reading
	virtual					~aRcFileMemory();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length() const;
	virtual void			SetLength( size_t len );
	virtual ARC_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

	// Set the given length and don't allow the file to grow.
	void					SetMaxLength( size_t len );
							// changes memory file to read only
	void					MakeReadOnly();
	// Change the file to be writable
	void					MakeWritable();
							// clear the file
	virtual void			Clear( bool freeMemory = true );
							// set data for reading
	void					SetData( const char *data, int length );
							// returns const pointer to the memory buffer
	const char *			GetDataPtr() const { return filePtr; }
							// returns pointer to the memory buffer
	char *					GetDataPtr() { return filePtr; }
							// set the file granularity
	void					SetGranularity( int g ) { assert( g > 0 ); granularity = g; }
	void					PreAllocate( size_t len );

	// Doesn't change how much is allocated, but allows you to set the size of the file to smaller than it should be.
	// Useful for stripping off a checksum at the end of the file
	void					TruncateData( size_t len );

	void					TakeDataOwnership();

	size_t					GetMaxLength() { return maxSize; }
	size_t					GetAllocated() { return allocated; }

protected:
	arcNetString					name;			// name of the file
private:
	int						mode;			// open mode
	size_t					maxSize;		// maximum size of file
	size_t					fileSize;		// size of the file
	size_t					allocated;		// allocated size
	int						granularity;	// file granularity
	char *					filePtr;		// buffer holding the file data
	char *					curPtr;			// current read/write pointer
};


class arcFile_BitMsg : public arcNetFile {
	friend class			aRcFileSystemLocal;

public:
							arcFile_BitMsg( ARCBitMessage &msg );
							arcFile_BitMsg( const ARCBitMessage &msg );
	virtual					~arcFile_BitMsg();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length() const;
	virtual ARC_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	arcNetString					name;			// name of the file
	int						mode;			// open mode
	ARCBitMessage *				msg;
};


class arcFile_Permanent : public arcNetFile {
	friend class			aRcFileSystemLocal;

public:
							arcFile_Permanent();
	virtual					~arcFile_Permanent();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length() const;
	virtual ARC_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

	// returns file pointer
	arcFileHandle			GetFilePtr() { return o; }

private:
	arcNetString					name;			// relative path of the file - relative path
	arcNetString					fullPath;		// full file path - OS path
	int						mode;			// open mode
	int						fileSize;		// size of the file
	arcFileHandle			o;				// file handle
	bool					handleSync;		// true if written data is immediately flushed
};

class arcFile_Cached : public arcFile_Permanent {
	friend class			aRcFileSystemLocal;
public:
	arcFile_Cached();
	virtual					~arcFile_Cached();

	void					CacheData( uint64 offset, uint64 length );

	virtual int				Read( void *buffer, int len );

	virtual int				Tell() const;
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	uint64				internalFilePos;
	uint64				bufferedStartOffset;
	uint64				bufferedEndOffset;
	byte *				buffered;
};


class arcFile_InZip : public arcNetFile {
	friend class			aRcFileSystemLocal;

public:
							arcFile_InZip();
	virtual					~arcFile_InZip();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length() const;
	virtual ARC_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	arcNetString					name;			// name of the file in the pak
	arcNetString					fullPath;		// full file path including pak file name
	int						zipFilePos;		// zip file info position in pak
	int						fileSize;		// size of the file
	void *					z;				// unzip info
};

#if 1
class arcFile_InnerResource : public arcNetFile {
	friend class			aRcFileSystemLocal;

public:
							arcFile_InnerResource( const char *_name, arcNetFile *rezFile, int _offset, int _len );
	virtual					~arcFile_InnerResource();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len ) { assert( false ); return 0; }
	virtual int				Length() const { return length; }
	virtual ARC_TIME_T		Timestamp() const { return 0; }
	virtual int				Tell() const;
	virtual int				Seek( long offset, fsOrigin_t origin );
	void					SetResourceBuffer( byte * buf ) {
		resourceBuffer = buf;
		internalFilePos = 0;
	}

private:
	arcNetString				name;				// name of the file in the pak
	int					offset;				// offset in the resource file
	int					length;				// size
	arcNetFile *			resourceFile;		// actual file
	int					internalFilePos;	// seek offset
	byte *				resourceBuffer;		// if using the temp save memory
};
#endif
/*
================================================
arcFileLocal is a FileStream wrapper that automatically closes a file when the
class variable goes out of scope. Note that the pointer passed in to the constructor can be for
any type of File Stream that ultimately inherits from arcNetFile, and that this is not actually a
SmartPointer, as it does not keep a reference count.
================================================
*/
class arcFileLocal {
public:
	// Constructor that accepts and stores the file pointer.
	arcFileLocal( arcNetFile *_file )	: file( _file ) {
	}

	// Destructor that will destroy (close) the file when this wrapper class goes out of scope.
	~arcFileLocal();

	// Cast to a file pointer.
	operator arcNetFile * () const {
		return file;
	}

	// Member access operator for treating the wrapper as if it were the file, itself.
	arcNetFile * operator -> () const {
		return file;
	}

protected:
	arcNetFile *file;	// The managed file pointer.
};



#endif /* !__FILE_H__ */
