#include "../idlib/Lib.h"
#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

/*
===============================================================================

	File System

	No stdio calls should be used by any part of the game, because of all sorts
	of directory and separator char issues. Throughout the game a forward slash
	should be used as a separator. The file system takes care of the conversion
	to an OS specific separator. The file system treats all file and directory
	names as case insensitive.

	The following cvars store paths used by the file system:

	"fs_basepath"		path to local install, read-only
	"fs_savepath"		path to config, save game, etc. files, read & write
	"fs_cdpath"			path to cd, read-only
	"fs_devpath"		path to files created during development, read & write

	The base path for file saving can be set to "fs_savepath" or "fs_devpath".

===============================================================================
*/

#include "File.h"
static const ARC_TIME_T	FILE_NOT_FOUND_TIMESTAMP	= 0xFFFFFFFF;
static const int		MAX_PURE_PAKS				= 128;
static const int		MAX_OSPATH					= 256;

// modes for OpenFileByMode. used as bit mask internally
typedef enum {
	FS_READ		= 0,
	FS_WRITE	= 1,
	FS_APPEND	= 2
} fsMode_t;

typedef enum {
	PURE_OK,		// we are good to connect as-is
	PURE_RESTART,	// restart required
	PURE_MISSING,	// pak files missing on the client
	PURE_NODLL		// no DLL could be extracted
} fsPureReply_t;

typedef enum {
	DLTYPE_URL,
	DLTYPE_FILE
} dlType_t;

typedef enum {
	DL_WAIT,		// waiting in the list for beginning of the download
	DL_INPROGRESS,	// in progress
	DL_DONE,		// download completed, success
	DL_ABORTING,	// this one can be set during a download, it will force the next progress callback to abort - then will go to DL_FAILED
	DL_FAILED
} dlStatus_t;

typedef enum {
	FILE_EXEC,
	FILE_OPEN
} dlMime_t;

typedef enum {
	FIND_NO,
	FIND_YES,
	FIND_ADDON
} findFile_t;

typedef struct urlDownload_s {
	anString				url;
	anString				urlAuth;
	anString				referer;
	char					dlerror[ MAX_STRING_CHARS ];
	int						dltotal;
	int						dlnow;
	int						dlstatus;
	//int						curlstatus;
	dlStatus_t				status;
} urlDownload_t;

typedef struct fileDownload_s {
	//int						offset;
	int						position;
	int						length;
	void *					buffer;
} fileDownload_t;

typedef struct backgroundDownload_s {
	struct backgroundDownload_s	*next;	// set by the fileSystem
	dlType_t				opcode;
	anFile *				f;
	fileDownload_t			file;
	urlDownload_t			url;
	volatile bool			completed;
} backgroundDownload_t;

typedef struct proxyDownload_s {
    anString               proxyUrl;
    anString               proxyAuth;
} proxyDownload_t;

// file list for directory listings
class anFileList {
	friend class anFileSystem;
public:
	const char *			GetBasePath( void ) const { return basePath; }
	int						GetNumFiles( void ) const { return list.Num(); }
	const char *			GetFile( int index ) const { return list[index]; }
	const anStringList &	GetList( void ) const { return list; }

private:
	anString				basePath;
	anStringList			list;
};

// mod list
class anModList {
	friend class anFileSystem;
public:
	int						GetNumMods( void ) const { return mods.Num(); }
	const char *			GetMod( int index ) const { return mods[index]; }
	const char *			GetDescription( int index ) const { return descriptions[index]; }

private:
	anStringList			mods;
	anStringList			descriptions;
};

struct metaDataContext_t {
	const anDict *	meta;
	bool			addon;
	anString		pak;
};

class anMetaDataList {
	friend class anFileSystem;
public:

							~anMetaDataList( void ) { 
								for ( int i = 0; i < metaData.Num(); i++ ) {
									delete metaData[i].meta;
								}
							}

	int						GetNumMetaData() const { return metaData.Num(); }
	const anDict &			GetMetaData( int index ) const { return *metaData[index].meta; }
	const metaDataContext_t&GetMetaDataContext( int index ) const { return metaData[index]; }
	const anDict *			FindMetaData( const char *name, const anDict *defaultDict = nullptr ) {
								for ( int i = 0; i < metaData.Num(); i++ ) {
									const char *value = metaData[i].meta->GetString( "metadata_name" );
									if ( !anString::Icmp( name, value ) ) {
										return metaData[i].meta;
									}
								}
								return defaultDict;
							}
	int						FindMetaDataIndex( const char *name ) {
								for ( int i = 0; i < metaData.Num(); i++ ) {
									const char *value = metaData[i].meta->GetString( "metadata_name" );
									if ( !anString::Icmp( name, value ) ) {
										return i;
									}
								}
								return -1;
							}

	const metaDataContext_t *	FindMetaDataContext( const char *name ) const {
								for ( int i = 0; i < metaData.Num(); i++ ) {
									const char *value = metaData[i].meta->GetString( "metadata_name" );
									if ( !anString::Icmp( name, value ) ) {
										return &metaData[i];
									}
								}
								return nullptr;
							}

private:
	anList<metaDataContext_t>	metaData;
};

class anFileSystem {
public:
							~anFileSystem() {}

							// Initializes the file system.
	void					Init( void );

							// Restarts the file system.
	void					Restart( void );

							// Shutdown the file system.
	void					Shutdown( bool reloading );

							// Returns true if the file system is initialized.
	bool					IsInitialized( void ) const;
// added
							// Enables/Disables quiet mode.
	void					SetQuietMode( bool enable );
							// Returns quiet mode status.
	bool					GetQuietMode( void ) const;
// end added
							// Returns true if we are doing an fs_copyfiles.
	bool					PerformingCopyFiles( void ) const;

							// Returns a list of mods found along with descriptions
							// 'mods' contains the directory names to be passed to fs_game
							// 'descriptions' contains a free form string to be used in the UI
	anModList *				ListMods( void );

							// Frees the given mod list
	void					FreeModList( anModList *modList );
// added
							// Returns a list of metadata specified in addon.conf (and pakmeta.conf)
	anMetaDataList *		ListMetaData( const char *metaDataTag );
							// Frees the given metadata list
	void					FreeMetaDataList( anMetaDataList *metaDataList );

							// Lists dll files with the directories specified by fs_toolsPath
							// Directory should not have either a leading or trailing '/'														
	anFileList *			ListTools();
// end added
							// Lists files with the given extension in the given directory.
							// Directory should not have either a leading or trailing '/'
							// The returned files will not include any directories or '/' unless fullRelativePath is set.
							// The extension must include a leading dot and may not contain wildcards.
							// If extension is "/", only subdirectories will be returned.
	anFileList *			ListFiles( const char *relativePath, const char *extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = nullptr );

							// Lists files in the given directory and all subdirectories with the given extension.
							// Directory should not have either a leading or trailing '/'
							// The returned files include a full relative path.
							// The extension must include a leading dot and may not contain wildcards.
	anFileList *			ListFilesTree( const char *relativePath, const char *extension, bool sort = false, const char* gamedir = nullptr );

							// Frees the given file list.
	void					FreeFileList( anFileList *fileList );

	int						Delete( char *filename ) const;

							// Converts a relative path to a full OS path.
	const char *			OSPathToRelativePath( const char *OSPath );

							// Converts a full OS path to a relative path.
	const char *			RelativePathToOSPath( const char *relativePath, const char *basePath = "fs_devpath" );

							// Builds a full OS path from the given components.
	const char *			BuildOSPath( const char *base, const char *game, const char *relativePath );

							// Creates the given OS path for as far as it doesn't exist already.
	void					CreateOSPath( const char *OSPath );
// added
							// Returns a full OS path where the initial base of the file system exists
	const char *			GetBasePath() const;
							// Returns a full OS path where files can be written to
	const char *			GetSavePath() const;
							// Returns a full OS path where user visible files can be written to
	const char *			GetUserPath() const;
							// Returns the current game path
	const char *			GetGamePath() const;
// end added
							// Returns true if a file is in a pak file.
	bool					FileIsInPAK( const char *relativePath );

							// Returns a space separated string containing the checksums of all referenced pak files.
							// will call SetPureServerChecksums internally to restrict itself
	void					UpdatePureServerChecksums( void );

							// setup the mapping of OS -> game pak checksum
	bool					UpdateGamePakChecksums( void );

							// 0-terminated list of pak checksums
							// if pureChecksums[ 0 ] == 0, all data sources will be allowed
							// otherwise, only pak files that match one of the checksums will be checked for files
							// with the sole exception of .cfg files.
							// the function tries to configure pure mode from the paks already referenced and this new list
							// it returns wether the switch was successfull, and sets the missing checksums
							// the process is verbosive when fs_debug 1
	fsPureReply_t			SetPureServerChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int enginePakChecksum, int missingChecksums[ MAX_PURE_PAKS ], int *missingGamePakChecksum );

							// fills a 0-terminated list of pak checksums for a client
							// if OS is -1, give the current game pak checksum. if >= 0, lookup the game pak table ( server only)
	void					GetPureServerChecksums( int checksums[ MAX_PURE_PAKS ], int OS, int *enginePakChecksum );

							// before doing a restart, force the pure list and the search order
							// if the given checksum list can't be completely processed and set, will error out
	void					SetRestartChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int enginePakChecksum );

							// equivalent to calling SetPureServerChecksums with an empty list
	void					ClearPureChecksums( void );

							// get a mask of supported OSes. if not pure, returns -1
	int						GetOSMask( void );
// added
							// returns true if the file exists
	bool					FileExists( const char *relativePath );

							// returns true if the file exists
	bool					FileExistsExplicit( const char *OSPath );

							// the timestamp or FILE_NOT_FOUND_TIMESTAMP
	unsigned int			GetTimestamp( const char *relativePath );
// end added
							// Reads a complete file.
							// Returns the length of the file, or -1 on failure.
							// A null buffer will just return the file length without loading.
							// A null timestamp will be ignored.
							// As a quick check for existance. -1 length == not present.
							// A 0 byte will always be appended at the end, so string ops are safe.
							// The buffer should be considered read-only, because it may be cached for other uses.
	int						ReadFile( const char *relativePath, void **buffer, ARC_TIME_T *timestamp = nullptr );

							// Frees the memory allocated by ReadFile.
	void					FreeFile( void *buffer );

							// Writes a complete file, will create any needed subdirectories.
							// Returns the length of the file, or -1 on failure.
	int						WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_savepath" );

							// Removes the given file.
	void					RemoveFile( const char *relativePath );
// added
							// Removes the specified directory.
	bool					RemoveDir( const char * relativePath ) = 0;
							// Removes the given directory from a full OS path
	bool					RemoveExplicitDir( const char *OSPath, bool nonEmpty, bool recursive );
							// Renames a file, taken from idTech5 (minus the fsPath_t)
	bool					RenameFile( const char *relativePath, const char *newName, const char *basePath = "fs_savepath" ) = 0;
// end added
							// Opens a file for reading.
	anFile *				OpenFileRead( const char *relativePath, bool allowCopyFiles = true, const char* gamedir = nullptr );

							// Opens a file for writing, will create any needed subdirectories.
	anFile *				OpenFileWrite( const char *relativePath, const char *basePath = "fs_savepath" );

							// Opens a file for writing at the end.
	anFile *				OpenFileAppend( const char *filename, bool sync = false, const char *basePath = "fs_basepath" );

							// Opens a file for reading, writing, or appending depending on the value of mode.
	anFile *				OpenFileByMode( const char *relativePath, fsMode_t mode );

							// Opens a file for reading from a full OS path.
	anFile *				OpenExplicitFileRead( const char *OSPath );

							// Opens a file for writing to a full OS path.
	anFile *				OpenExplicitFileWrite( const char *OSPath );

							// opens an archive file container
	anFileCached *			OpenExplicitPakFile( const char *OSPath );

							// Closes a file.
	void					CloseFile( anFile *f );

							// Returns immediately, performing the read from a background thread.
	void					BackgroundDownload( backgroundDownload_t *bgl );

							// resets the bytes read counter
	void					ResetReadCount( void );

							// retrieves the current read count
	int						GetReadCount( void );

							// adds to the read count
	void					AddToReadCount( int c );

							// look for a dynamic module
	void					FindDLL( const char *basename, char dllPath[ MAX_OSPATH ], bool updateChecksum );

							// case sensitive filesystems use an internal directory cache
							// the cache is cleared when calling OpenFileWrite and RemoveFile
							// in some cases you may need to use this directly
	void					ClearDirCache( void );

							// don't use for large copies - allocates a single memory block for the copy
	void					CopyFile( const char *fromOSPath, const char *toOSPath );

							// lookup a relative path, return the size or 0 if not found
	int						ValidateDownloadPakForChecksum( int checksum, char path[ MAX_STRING_CHARS ], bool isGamePak );

	anFile *				MakeTemporaryFile( void );
	bool					IsExtension( const char *fileName, const char *ext, int nLength ) const;
	sysFolder_t 			IsFolder( const char * relativePath, const char *basePath = "fs_basepath" );

	void					EnableBackgroundCache( bool enable );
	void					BeginLevelLoad( const char *name, char *_blockBuffer, int _blockBufferSize  );
	void					EndLevelLoad();
	bool 					IsBinaryModel( const idStr & resName ) const;

							// make downloaded pak files known so pure negotiation works next time
	int						AddPakBFile( const char *path );
	int						AddZipFile( const char *path );

							// look for a file in the loaded paks or the addon paks
							// if the file is found in addons, FS's internal structures are ready for a reloadEngine
	findFile_t				FindFile( const char *path, bool scheduleAddons = false );

							// get map/addon decls and take into account addon paks that are not on the search list
							// the decl 'name' is in the "path" entry of the dict
	int						GetNumMaps();
	const anDict *			GetMapDecl( int i );
	void					FindMapScreenshot( const char *path, char *buf, int len );

							// ignore case and seperator char distinctions
	bool					CompareFileName( const char *s1, const char *s2 ) const;
	anFile *				GetResourceContainer( int idx ) {
		if ( idx >= 0 && idx < resourceFiles.Num() ) {
			return resourceFiles[ idx ]->resourceFile;
		}
		return NULL;
	}

	// textures
	void					ReadTGA( const char *name, byte **pic, int *width, int *height, unsigned *timestamp = 0, bool markPaksReferenced = true );
	void					WriteTGA( const char *name, const byte *pic, int width, int height, bool flipVertical );
	void					FreeTGA( byte *pic );

	static void				Dir_f( const anCommandArgs &args );
	static void				DirTree_f( const anCommandArgs &args );
	static void				Path_f( const anCommandArgs &args );
	static void				TouchFile_f( const anCommandArgs &args );
	static void				TouchFileList_f( const anCommandArgs &args );
	bool					IsEoF( const anFile *f ) const;
	bool					ExplicitIsEOF( const anFile *file ) const;

private:
	friend dword 			BackgroundDownloadThread( void *parms );

	const char *			CaseSearch( const char *inDir );

	void					ReplaceSeparators( anString &path, char sep = PATHSEPERATOR_CHAR );
	long					HashFileName( const char *fname ) const;
	int						ListOSFiles( const char *directory, const char *extension, anStringList &list );
	FILE *					OpenOSFile( const char *name, const char *mode, anString *caseSensitiveName = nullptr );
	FILE *					OpenOSFileCorrectName( anString &path, const char *mode );
	int						DirectFileLength( FILE *o );
	void					CopyFile( anFile *src, const char *toOSPath );

	static void				CheckFilenameIsMutable( const char *filename, const char *function );

	int						AddUnique( const char *name, anStringList &list, anHashIndex &hashIndex ) const;
	void					GetExtensionList( const char *extension, anStringList &extensionList ) const;
	int						GetFileList( const char *relativePath, const anStringList &extensions, anStringList &list, anHashIndex &hashIndex, bool fullRelativePath, const char* gamedir = nullptr );

	void					SortFileList( const char **filelist, int numfiles ) const;

	int						GetFileListTree( const char *relativePath, const anStringList &extensions, anStringList &list, anHashIndex &hashIndex, const char* gamedir = nullptr );
	pack_t *				LoadZipFile( const char *zipfile );
	void					AddGameDirectory( const char *path, const char *dir );
	void					SetupGameDirectories( const char *gameName );
	void					Startup( void );
	void					SetRestrictions( void );
							// some files can be obtained from directories without compromising si_pure
	bool					FileAllowedFromDir( const char *path );
							// searches all the paks, no pure check
	pack_t *				GetPackForChecksum( int checksum, bool searchAddons = false );
							// searches all the paks, no pure check
	pack_t *				FindPakForFileChecksum( const char *relativePath, int fileChecksum, bool bReference );
	anCompressedArchive *			ReadFileFromZip( pack_t *pak, fileInPack_t *pakFile, const char *relativePath );
	int						GetFileChecksum( anFile *file );
	pureStatus_t			GetPackStatus( pack_t *pak );
	addonInfo_t *			ParseAddonDef( const char *buf, const int len );
	void					FollowAddonDependencies( pack_t *pak );

	static size_t			CurlWriteFunction( void *ptr, size_t size, size_t nmemb, void *stream );
							// curl_progress_callback in curl.h
	static int				CurlProgressFunction( void *clientp, double dltotal, double dlnow, double ultotal, double ulnow );
private:

	searchpath_t *			searchPaths;
	int						readCount;			// total bytes read
	int						loadCount;			// total files read
	int						loadStack;			// total files in memory
	anString				engineFolder;		// this will be a single name without separators

	searchpath_t			*addonPaks;			// not loaded up, but we saw them

	anDict					mapDict;			// for GetMapDecl

	static anCVar			fs_debug;
	static anCVar			fs_restrict;
	static anCVar			fs_copyfiles;
	static anCVar			fs_basepath;
	static anCVar			fs_savepath;
	static anCVar			fs_cdpath;
	static anCVar			fs_devpath;
	static anCVar			fs_game;
	static anCVar			fs_game_base;
	static anCVar			fs_caseSensitiveOS;
	static anCVar			fs_searchAddons;
	static anCVar 			fs_usePkB;

	backgroundDownload_t *	backgroundDownloads;
	backgroundDownload_t	defaultBackgroundDownload;
	xthreadInfo				backgroundThread;

	bool 					usePkB;		// for binary archive .pk files ".pkB"
	anList<pack_t *>		pak4;
	anList<pack_t *>		pak5;
	bool					loadedFileFromDir;		// set to true once a file was loaded from a directory - can't switch to pure anymore
	anList<int>				restartChecksums;		// used during a restart to set things in right order
	anList<int>				addonChecksums;			// list of checksums that should go to the search list directly ( for restarts )
	int						restartGamePakChecksum;
	int						engineDLLChecksum;		// the checksum of the last loaded game DLL
	int						enginePakChecksum;		// the checksum of the pak holding the loaded game DLL

	int						enginePakForOS[ MAX_GAME_OS ];

	anDEntry				dir_cache[ MAX_CACHED_DIRS ]; // fifo
	int						dir_cache_index;
	int						dir_cache_count;
	anList<casematch_t>		dir_case;
}; extern anFileSystem *	fileSystem;


/*
================================================
FileScoped: A File stream wrapper that automatically closes file( s) when the
class variable goes out of scope.
NOTE: The pointer passed in to the constructor can be for
any type of File Stream that it ultimately inherits from the primary file class,
inwhich is not actually a "SmartPointer", as it does not retain reference counts.
================================================
*/
class anFileScoped {
public:
	// Constructor that accepts and stores the file pointer.
	anFileScoped( anFile *_file ) : file( _file ) {
	}

	// Destructor that will destroy (close) the file when this wrapper class goes out of scope.
	~anFileScoped() {
		fileSystem->CloseFile( file );
	}

	// Cast to a file pointer.
	operator anFile*() const {
		return file;
	}

	// Member access operator for treating the wrapper as if it were the file, itself.
	anFile *operator ->() const {
		return file;
	}

protected:
	anFile *file;	// The managed file pointer.
};

#endif // !__FILESYSTEM_H__
