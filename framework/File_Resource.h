#ifndef __FILE_RESOURCE_H__
#define __FILE_RESOURCE_H__

/*
==============================================================

  Resource containers

==============================================================
*/

class aRcResCacheEntries {
public:
	aRcResCacheEntries() {
		Clear();
	}
	void Clear() {
		filename.Empty();
		//filename = NULL;
		offset = 0;
		length = 0;
		containerIndex = 0;
	}
	size_t Read( arcNetFile *f ) {
		size_t sz = f->ReadString( filename );
		sz += f->ReadBig( offset );
		sz += f->ReadBig( length );
		return sz;
	}
	size_t Write( arcNetFile *f ) {
		size_t sz = f->WriteString( filename );
		sz += f->WriteBig( offset );
		sz += f->WriteBig( length );
		return sz;
	}
	aRcStaticString< 256 >	filename;
	int					offset;							// into the resource file
	int 				length;
	uint8				containerIndex;
};

static const uint32 RESOURCE_FILE_MAGIC = 0xD000000D;
class aRcResContainers {
	friend class	aRcFileSystemLocal;
	//friend class	idReadSpawnThread;
public:
	aRcResContainers() {
		resourceFile = NULL;
		tableOffset = 0;
		tableLength = 0;
		resourceMagic = 0;
		numFileResources = 0;
	}
	~aRcResContainers() {
		delete resourceFile;
		cacheTable.Clear();
	}
	bool Init( const char * fileName, uint8 containerIndex );
	static void WriteResourceFile( const char *fileName, const arcStringList &manifest, const bool &_writeManifest );
	static void WriteManifestFile( const char *name, const arcStringList &list );
	static int ReadManifestFile( const char *filename, arcStringList &list );
	static void ExtractResourceFile ( const char * fileName, const char * outPath, bool copyWavs );
	static void UpdateResourceFile( const char *filename, const arcStringList &filesToAdd );
	arcNetFile *OpenFile( const char *fileName );
	const char * GetFileName() const { return fileName.c_str(); }
	void SetContainerIndex( const int & _idx );
	void ReOpen();
private:
	aRcStaticString< 256 > fileName;
	arcNetFile *	resourceFile;			// open file handle
	// offset should probably be a 64 bit value for development, but 4 gigs won't fit on
	// a DVD layer, so it isn't a retail limitation.
	int		tableOffset;			// table offset
	int		tableLength;			// table length
	int		resourceMagic;			// magic
	int		numFileResources;		// number of file resources in this container
	arcNetList< aRcResCacheEntries, TAG_RESOURCE>	cacheTable;
	ARCHashIndex	cacheHash;
};


#endif /* !__FILE_RESOURCE_H__ */
