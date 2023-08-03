#ifndef __FILE_RESOURCE_H__
#define __FILE_RESOURCE_H__

/*
==============================================================

  Resource containers

==============================================================
*/

class anResourceCacheEntries {
public:
	anResourceCacheEntries() {Clear();}
	void Clear() {
		filename.Empty();
		//filename = nullptr;
		offset = 0;
		length = 0;
		containerIndex = 0;
	}
	size_t Read( anFile *f ) {
		size_t sz = f->ReadString( filename );
		sz += f->ReadBig( offset );
		sz += f->ReadBig( length );
		return sz;
	}
	size_t Write( anFile *f ) {
		size_t sz = f->WriteString( filename );
		sz += f->WriteBig( offset );
		sz += f->WriteBig( length );
		return sz;
	}
	anStaticString< 256 >	filename;
	int					offset;							// into the resource file
	int 				length;
	uint8				containerIndex;
};

static const uint32 RESOURCE_FILE_MAGIC = 0xD000000D;
class anResourceContainers {
	friend class	anFileSystem;
	//friend class	anReadSpawnThread;
public:
	anResourceContainers() {
		resFile = nullptr;
		tableOffset = 0;
		tableLength = 0;
		resourceMagic = 0;
		numFileResources = 0;
	}
	~anResourceContainers() {
		delete resFile;
		cacheTable.Clear();
	}
	bool			Init( const char *fileName, uint8 containerIndex );
	static void		WriteResourceFile( const char *fileName, const anStringList &manifest, const bool &_writeManifest );
	static void		WriteManifestFile( const char *name, const anStringList &list );
	static int		ReadManifestFile( const char *filename, anStringList &list );
	static void		ExtractResourceFile ( const char *fileName, const char *outPath, bool copyWavs );
	static void		UpdateResourceFile( const char *filename, const anStringList &filesToAdd );
	anFile *		OpenFile( const char *fileName );
	const char *	GetFileName() const { return fileName.c_str(); }
	void			SetContainerIndex( const int & _idx );
	void			ReOpen();
private:
	anStaticString<256> fileName;
	anFile *	resFile;			// open file handle
	// offset should probably be a 64 bit value for development, but 4 gigs won't fit on
	// a DVD layer, so it isn't a retail limitation.
	int			tableOffset;			// table offset
	int			tableLength;			// table length
	int			resourceMagic;			// magic
	int			numFileResources;		// number of file resources in this container
	anList<anResourceCacheEntries, TAG_RESOURCE>	cacheTable;
	anHashIndex	cacheHash;
};


#endif // !__FILE_RESOURCE_H__