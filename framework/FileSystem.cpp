#include "/idlib/precompiled.h"
#pragma hdrstop

#include "Unzip.h"
#include "Zip.h"

#ifdef WIN32
	#include <io.h>	// for _read
#else
	#if !__MACH__ && __MWERKS__
		#include <types.h>
		#include <stat.h>
#else
		#include <sys/types.h>
		#include <sys/stat.h>
	#endif
	#include <unistd.h>
#endif


/*
=============================================================================

DOOM FILESYSTEM

All of Doom's data access is through a hierarchical file system, but the contents of
the file system can be transparently merged from several sources.

A "relativePath" is a reference to game file data, which must include a terminating zero.
"..", "\\", and ":" are explicitly illegal in qpaths to prevent any references
outside the Doom directory system.

The "base path" is the path to the directory holding all the game directories and
usually the executable. It defaults to the current directory, but can be overridden
with "+set fs_basepath c:\doom" on the command line. The base path cannot be modified
at all after startup.

The "save path" is the path to the directory where game files will be saved. It defaults
to the base path, but can be overridden with a "+set fs_savepath c:\doom" on the
command line. Any files that are created during the game (demos, screenshots, etc.) will
be created reletive to the save path.

If a user runs the game directly from a CD, the base path would be on the CD. This
should still function correctly, but all file writes will fail (harmlessly).

The "base game" is the directory under the paths where data comes from by default, and
can be either "base" or "demo".

The "current game" may be the same as the base game, or it may be the name of another
directory under the paths that should be searched for files before looking in the base
game. The game directory is set with "+set fs_game myaddon" on the command line. This is
the basis for addons.

No other directories outside of the base game and current game will ever be referenced by
filesystem functions.

Because we will have updated executables freely available online, there is no point to
trying to restrict demo / oem versions of the game with code changes. Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

If the "fs_copyfiles" cvar is set to 1, then every time a file is sourced from the base
path, it will be copied over to the save path. This is a development aid to help build
test releases and to copy working sets of files.

The relative path "sound/newstuff/test.wav" would be searched for in the following places:

for save path, base path:
	for current game, base game:
		search directory
		search zip files

downloaded files, to be written to save path + current game's directory

The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
(sound, video) must also be forced to restart.

"additional mod path search":
fs_game_base can be used to set an additional search path
in search order, fs_game, fs_game_base, BASEGAME
for instance to base a mod of D3 + D3XP assets, fs_game mymod, fs_game_base d3xp

=============================================================================
*/

#define MAX_ZIPPED_FILE_NAME	2048
#define FILE_HASH_SIZE			1024

struct searchpath_t {
	arcNetString	path;		// c:\doom
	arcNetString	gamedir;	// base
};

// search flags when opening a file
#define FSFLAG_SEARCH_DIRS		( 1 << 0 )
#define FSFLAG_RETURN_FILE_MEM	( 1 << 1 )

class aRcFileSystemLocal : public arcNetFile {
public:
							aRcFileSystemLocal();

	virtual void			Init();
	virtual void			Restart();
	virtual void			Shutdown( bool reloading );
	virtual bool			IsInitialized() const;
	virtual arcFileList *	ListFiles( const char *relativePath, const char *extension, bool sort = false, bool fullRelativePath = false, const char* gamedir = NULL );
	virtual arcFileList *	ListFilesTree( const char *relativePath, const char *extension, bool sort = false, const char* gamedir = NULL );
	virtual void			FreeFileList( arcFileList *fileList );
	virtual const char *	OSPathToRelativePath( const char *OSPath );
	virtual const char *	RelativePathToOSPath( const char *relativePath, const char *basePath );
	virtual const char *	BuildOSPath( const char *base, const char *game, const char *relativePath );
	virtual const char *	BuildOSPath( const char *base, const char *relativePath );
	virtual void			CreateOSPath( const char *OSPath );
	virtual int				ReadFile( const char *relativePath, void **buffer, ARC_TIME_T *timestamp );
	virtual void			FreeFile( void *buffer );
	virtual int				WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_savepath" );
	virtual void			RemoveFile( const char *relativePath );
	virtual	bool			RemoveDir( const char * relativePath );
	virtual bool			RenameFile( const char * relativePath, const char * newName, const char * basePath = "fs_savepath" );
	virtual arcNetFile *		OpenFileReadFlags( const char *relativePath, int searchFlags, bool allowCopyFiles = true, const char* gamedir = NULL );
	virtual arcNetFile *		OpenFileRead( const char *relativePath, bool allowCopyFiles = true, const char* gamedir = NULL );
	virtual arcNetFile *		OpenFileReadMemory( const char *relativePath, bool allowCopyFiles = true, const char* gamedir = NULL );
	virtual arcNetFile *		OpenFileWrite( const char *relativePath, const char *basePath = "fs_savepath" );
	virtual arcNetFile *		OpenFileAppend( const char *relativePath, bool sync = false, const char *basePath = "fs_basepath"   );
	virtual arcNetFile *		OpenFileByMode( const char *relativePath, fsMode_t mode );
	virtual arcNetFile *		OpenExplicitFileRead( const char *OSPath );
	virtual arcNetFile *		OpenExplicitFileWrite( const char *OSPath );
	virtual arcFile_Cached *		OpenExplicitPakFile( const char *OSPath );
	virtual void			CloseFile( arcNetFile *f );
	virtual void			FindDLL( const char *basename, char dllPath[ MAX_OSPATH ] );
	virtual void			CopyFile( const char *fromOSPath, const char *toOSPath );
	virtual findFile_t		FindFile( const char *path );
	virtual bool			FilenameCompare( const char *s1, const char *s2 ) const;
	virtual int				GetFileLength( const char * relativePath );
	virtual sysFolder_t		IsFolder( const char * relativePath, const char *basePath = "fs_basepath" );
	// resource tracking
	virtual void			EnableBackgroundCache( bool enable );
	virtual void			BeginLevelLoad( const char *name, char *_blockBuffer, int _blockBufferSize );
	virtual void			EndLevelLoad();
	virtual bool			InProductionMode() { return ( resourceFiles.Num() > 0 ) |  ( com_productionMode.GetInteger() != 0 ); }
	virtual bool			UsingResourceFiles() { return resourceFiles.Num() > 0; }
	virtual void			UnloadMapResources( const char *name );
	virtual void			UnloadResourceContainer( const char *name );
	arcNetFile *				GetResourceContainer( int idx ) {
		if ( idx >= 0 && idx < resourceFiles.Num() ) {
			return resourceFiles[ idx ]->resourceFile;
		}
		return NULL;
	}

	virtual void			StartPreload( const arcStringList &_preload );
	virtual void			StopPreload();
	arcNetFile *				GetResourceFile( const char *fileName, bool memFile );
	bool					GetResourceCacheEntry( const char *fileName, aRcResCacheEntries &rc );
	virtual int				ReadFromBGL( arcNetFile *_resourceFile, void * _buffer, int _offset, int _len );
	virtual bool			IsBinaryModel( const arcNetString & resName ) const;
	virtual bool			IsSoundSample( const arcNetString & resName ) const;
	virtual void			FreeResourceBuffer() { resourceBufferAvailable = resourceBufferSize; }
	virtual void			AddImagePreload( const char *resName, int _filter, int _repeat, int _usage, int _cube ) {
		preloadList.AddImage( resName, _filter, _repeat, _usage, _cube );
	}
	virtual void			AddSamplePreload( const char *resName ) {
		preloadList.AddSample( resName );
	}
	virtual void			AddModelPreload( const char *resName ) {
		preloadList.AddModel( resName );
	}
	virtual void			AddAnimPreload( const char *resName ) {
		preloadList.AddAnim( resName );
	}
	virtual void			AddCollisionPreload( const char *resName ) {
		preloadList.AddCollisionModel( resName );
	}
	virtual void			AddParticlePreload( const char *resName ) {
		preloadList.AddParticle( resName );
	}

	static void				Dir_f( const arcCommandArgs &args );
	static void				DirTree_f( const arcCommandArgs &args );
	static void				Path_f( const arcCommandArgs &args );
	static void				TouchFile_f( const arcCommandArgs &args );
	static void				TouchFileList_f( const arcCommandArgs &args );
	static void				BuildGame_f( const arcCommandArgs &args );
	//static void				FileStats_f( const arcCommandArgs &args );
	static void				WriteResourceFile_f ( const arcCommandArgs &args );
	static void				ExtractResourceFile_f( const arcCommandArgs &args );
	static void				UpdateResourceFile_f( const arcCommandArgs &args );
	static void				GenerateResourceCRCs_f( const arcCommandArgs &args );
	static void				CreateCRCsForResourceFileList( const arcFileList & list );

	void					BuildOrderedStartupContainer();
private:
	arcNetList<searchpath_t>	searchPaths;
	int						loadCount;			// total files read
	int						loadStack;			// total files in memory
	arcNetString					gameFolder;			// this will be a single name without separators

	static arcCVarSystem			fs_debug;
	static arcCVarSystem			fs_debugResources;
	static arcCVarSystem			fs_copyfiles;
	static arcCVarSystem			fs_buildResources;
	static arcCVarSystem			fs_game;
	static arcCVarSystem			fs_game_base;
	static arcCVarSystem			fs_enableBGL;
	static arcCVarSystem			fs_debugBGL;

	arcNetString					manifestName;
	arcStringList				fileManifest;
	aRcPreloadManifest		preloadList;

	arcNetList< aRcResContainers * > resourceFiles;
	byte *	resourceBufferPtr;
	int		resourceBufferSize;
	int		resourceBufferAvailable;
	int		numFilesOpenedAsCached;

private:

	// .resource file creation
	void					ClearResourcePacks();
	void					WriteResourcePacks();
	void					AddRenderProgs( arcStringList &files );
	void					AddFonts( arcStringList &files );

	void					ReplaceSeparators( arcNetString &path, char sep = PATHSEPARATOR_CHAR );
	int						ListOSFiles( const char *directory, const char *extension, arcStringList &list );
	arcFileHandle			OpenOSFile( const char *name, fsMode_t mode );
	void					CloseOSFile( arcFileHandle o );
	int						DirectFileLength( arcFileHandle o );
	void					CopyFile( arcNetFile *src, const char *toOSPath );
	int						AddUnique( const char *name, arcStringList &list, ARCHashIndex &hashIndex ) const;
	void					GetExtensionList( const char *extension, arcStringList &extensionList ) const;
	int						GetFileList( const char *relativePath, const arcStringList &extensions, arcStringList &list, ARCHashIndex &hashIndex, bool fullRelativePath, const char* gamedir = NULL );

	int						GetFileListTree( const char *relativePath, const arcStringList &extensions, arcStringList &list, ARCHashIndex &hashIndex, const char* gamedir = NULL );
	void					AddGameDirectory( const char *path, const char *dir );

	int						AddResourceFile( const char * resourceFileName );
	void					RemoveMapResourceFile( const char * resourceFileName );
	void					RemoveResourceFileByIndex( const int & idx );
	void					RemoveResourceFile( const char * resourceFileName );
	int						FindResourceFile( const char * resourceFileName );

	void					SetupGameDirectories( const char *gameName );
	void					Startup();
	void					InitPrecache();
	void					ReOpenCacheFiles();
};

arcCVarSystem	aRcFileSystemLocal::fs_debug( "fs_debug", "0", CVAR_SYSTEM | CVAR_INTEGER, "", 0, 2, arcCmdSystem::ArgCompletion_Integer<0,2> );
arcCVarSystem	aRcFileSystemLocal::fs_debugResources( "fs_debugResources", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
arcCVarSystem	aRcFileSystemLocal::fs_enableBGL( "fs_enableBGL", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
arcCVarSystem	aRcFileSystemLocal::fs_debugBGL( "fs_debugBGL", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
arcCVarSystem	aRcFileSystemLocal::fs_copyfiles( "fs_copyfiles", "0", CVAR_SYSTEM | CVAR_INIT | CVAR_BOOL, "Copy every file touched to fs_savepath" );
arcCVarSystem	aRcFileSystemLocal::fs_buildResources( "fs_buildresources", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_INIT, "Copy every file touched to a resource file" );
arcCVarSystem	aRcFileSystemLocal::fs_game( "fs_game", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "mod path" );
arcCVarSystem  aRcFileSystemLocal::fs_game_base( "fs_game_base", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "alternate mod path, searched after the main fs_game path, before the basedir" );

arcCVarSystem	fs_basepath( "fs_basepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
arcCVarSystem	fs_savepath( "fs_savepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
arcCVarSystem	fs_resourceLoadPriority( "fs_resourceLoadPriority", "1", CVAR_SYSTEM , "if 1, open requests will be honored from resource files first; if 0, the resource files are checked after normal search paths" );
arcCVarSystem	fs_enableBackgroundCaching( "fs_enableBackgroundCaching", "1", CVAR_SYSTEM , "if 1 allow the 360 to precache game files in the background" );

aRcFileSystemLocal	fileSystemLocal;
arcNetFile *		fileSystem = &fileSystemLocal;

/*
================
aRcFileSystemLocal::ReadFromBGL
================
*/
int aRcFileSystemLocal::ReadFromBGL( arcNetFile *_resourceFile, void * _buffer, int _offset, int _len ) {
	if ( _resourceFile->Tell() != _offset ) {
		_resourceFile->Seek( _offset, FS_SEEK_SET );
	}
	return _resourceFile->Read( _buffer, _len );
}

/*
================
aRcFileSystemLocal::StartPreload
================
*/
void aRcFileSystemLocal::StartPreload( const arcStringList & _preload ) {
}

/*
================
aRcFileSystemLocal::StopPreload
================
*/
void aRcFileSystemLocal::StopPreload() {
}

/*
================
aRcFileSystemLocal::aRcFileSystemLocal
================
*/
aRcFileSystemLocal::aRcFileSystemLocal() {
	loadCount = 0;
	loadStack = 0;
	resourceBufferPtr = NULL;
	resourceBufferSize = 0;
	resourceBufferAvailable = 0;
	numFilesOpenedAsCached = 0;
}

/*
===========
aRcFileSystemLocal::FilenameCompare

Ignore case and separator char distinctions
===========
*/
bool aRcFileSystemLocal::FilenameCompare( const char *s1, const char *s2 ) const {
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( c1 >= 'a' && c1 <= 'z' ) {
			c1 -= ('a' - 'A');
		}
		if ( c2 >= 'a' && c2 <= 'z' ) {
			c2 -= ('a' - 'A');
		}

		if ( c1 == '\\' || c1 == ':' ) {
			c1 = '/';
		}
		if ( c2 == '\\' || c2 == ':' ) {
			c2 = '/';
		}

		if ( c1 != c2 ) {
			return true;		// strings not equal
		}
	} while( c1 );

	return false;		// strings are equal
}

/*
========================
aRcFileSystemLocal::GetFileLength
========================
*/
int aRcFileSystemLocal::GetFileLength( const char * relativePath ) {
	arcNetFile *	f;
	int			len;

	if ( !IsInitialized() ) {
		arcLibrary::FatalError( "Filesystem call made without initialization" );
	}

	if ( !relativePath || !relativePath[0] ) {
		arcLibrary::Warning( "aRcFileSystemLocal::GetFileLength with empty name" );
		return -1;
	}

	if ( resourceFiles.Num() > 0 ) {
		aRcResCacheEntries rc;
		if ( GetResourceCacheEntry( relativePath, rc ) ) {
			return rc.length;
		}
	}

	// look for it in the filesystem or pack files
	f = OpenFileRead( relativePath, false );
	if ( f == NULL ) {
		return -1;
	}

	len = ( int )f->Length();

	delete f;
	return len;
}

/*
================
aRcFileSystemLocal::OpenOSFile
================
*/
arcFileHandle aRcFileSystemLocal::OpenOSFile( const char *fileName, fsMode_t mode ) {
	arcFileHandle fp;


	DWORD dwAccess = 0;
	DWORD dwShare = 0;
	DWORD dwCreate = 0;
	DWORD dwFlags = 0;

	if ( mode == FS_WRITE ) {
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		dwShare = FILE_SHARE_READ;
		dwCreate = CREATE_ALWAYS;
		dwFlags = FILE_ATTRIBUTE_NORMAL;
	} else if ( mode == FS_READ ) {
		dwAccess = GENERIC_READ;
		dwShare = FILE_SHARE_READ;
		dwCreate = OPEN_EXISTING;
		dwFlags = FILE_ATTRIBUTE_NORMAL;
	} else if ( mode == FS_APPEND ) {
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		dwShare = FILE_SHARE_READ;
		dwCreate = OPEN_ALWAYS;
		dwFlags = FILE_ATTRIBUTE_NORMAL;
					}

	fp = CreateFile( fileName, dwAccess, dwShare, NULL, dwCreate, dwFlags, NULL );
	if ( fp == INVALID_HANDLE_VALUE ) {
		return NULL;
				}
	return fp;
}

/*
================
aRcFileSystemLocal::CloseOSFile
================
*/
void aRcFileSystemLocal::CloseOSFile( arcFileHandle o ) {
	::CloseHandle( o );
}

/*
================
aRcFileSystemLocal::DirectFileLength
================
*/
int aRcFileSystemLocal::DirectFileLength( arcFileHandle o ) {
	return GetFileSize( o, NULL );
}

/*
============
aRcFileSystemLocal::CreateOSPath

Creates any directories needed to store the given filename
============
*/
void aRcFileSystemLocal::CreateOSPath( const char *OSPath ) {
	char	*ofs;

	// make absolutely sure that it can't back up the path
	// FIXME: what about c: ?
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
#ifdef _DEBUG
		common->DPrintf( "refusing to create relative path \"%s\"\n", OSPath );
#endif
		return;
	}

	aRcStaticString< MAX_OSPATH > path( OSPath );
	path.SlashesToBackSlashes();
	for ( ofs = &path[ 1 ]; *ofs; ofs++ ) {
		if ( *ofs == PATHSEPARATOR_CHAR ) {
			// create the directory
			*ofs = 0;
			Sys_Mkdir( path );
			*ofs = PATHSEPARATOR_CHAR;
		}
	}
}

/*
=================
aRcFileSystemLocal::EnableBackgroundCache
=================
*/
void aRcFileSystemLocal::EnableBackgroundCache( bool enable ) {
	if ( !fs_enableBackgroundCaching.GetBool() ) {
		return;
	}
}

/*
=================
aRcFileSystemLocal::BeginLevelLoad
=================
*/
void aRcFileSystemLocal::BeginLevelLoad( const char *name, char *_blockBuffer, int _blockBufferSize ) {

	if ( name == NULL || *name == '\0' ) {
		return;
	}

	resourceBufferPtr = ( byte* )_blockBuffer;
	resourceBufferAvailable = _blockBufferSize;
	resourceBufferSize = _blockBufferSize;

	manifestName = name;

	fileManifest.Clear();
	preloadList.Clear();

	EnableBackgroundCache( false );

	ReOpenCacheFiles();
	manifestName.StripPath();

	if ( resourceFiles.Num() > 0 ) {
		AddResourceFile( va( "%s.resources", manifestName.c_str() ) );
	}

}

/*
=================
aRcFileSystemLocal::UnloadResourceContainer

=================
*/
void aRcFileSystemLocal::UnloadResourceContainer( const char *name ) {
	if ( name == NULL || *name == '\0' ) {
		return;
	}
	RemoveResourceFile( va( "%s.resources", name ) );
}

/*
=================
aRcFileSystemLocal::UnloadMapResources
=================
*/
void aRcFileSystemLocal::UnloadMapResources( const char *name ) {
	if ( name == NULL || *name == '\0' || arcNetString::Icmp( "_startup", name ) == 0 ) {
		return;
	}

	if ( resourceFiles.Num() > 0 ) {
		RemoveMapResourceFile( va( "%s.resources", name ) );
	}
}

/*
=================
aRcFileSystemLocal::EndLevelLoad

=================
*/
void aRcFileSystemLocal::EndLevelLoad() {
	if ( fs_buildResources.GetBool() ) {
		int saveCopyFiles = fs_copyfiles.GetInteger();
		fs_copyfiles.SetInteger( 0 );

		arcNetString manifestFileName = manifestName;
		manifestFileName.StripPath();
		manifestFileName.SetFileExtension( "manifest" );
		manifestFileName.Insert( "maps/", 0 );
		arcNetFile *outFile = fileSystem->OpenFileWrite( manifestFileName );
		if ( outFile != NULL ) {
			int num = fileManifest.Num();
			outFile->WriteBig( num );
			for ( int i = 0; i < num; i++ ) {
				outFile->WriteString( fileManifest[ i ] );
			}
			delete outFile;
		}

		aRcStaticString< MAX_OSPATH > preloadName = manifestName;
		preloadName.Insert( "maps/", 0 );
		preloadName += ".preload";
		arcNetFile *fileOut = fileSystem->OpenFileWrite( preloadName, "fs_savepath" );
		preloadList.WriteManifestToFile( fileOut );
		delete fileOut;

		fs_copyfiles.SetInteger( saveCopyFiles );
	}

	EnableBackgroundCache( true );

	resourceBufferPtr = NULL;
	resourceBufferAvailable = 0;
	resourceBufferSize = 0;

}

bool FileExistsInAllManifests( const char *filename, arcNetList< aRcManifest > &manifests ) {
	for ( int i = 0; i < manifests.Num(); i++ ) {
		if ( strstr( manifests[ i ].GetManifestName(), "_startup" ) != NULL ) {
			continue;
		}
		if ( strstr( manifests[ i ].GetManifestName(), "_pc" ) != NULL ) {
			continue;
		}
		if ( manifests[ i ].FindFile( filename ) == -1 ) {
			return false;
		}
	}
	return true;
}

bool FileExistsInAllPreloadManifests( const char *filename, arcNetList< aRcPreloadManifest > &manifests ) {
	for ( int i = 0; i < manifests.Num(); i++ ) {
		if ( strstr( manifests[ i ].GetManifestName(), "_startup" ) != NULL ) {
			continue;
		}
		if ( manifests[ i ].FindResource( filename ) == -1 ) {
			return false;
		}
	}
	return true;
}

void RemoveFileFromAllManifests( const char *filename, arcNetList< aRcManifest > &manifests ) {
	for ( int i = 0; i < manifests.Num(); i++ ) {
		if ( strstr( manifests[ i ].GetManifestName(), "_startup" ) != NULL ) {
			continue;
		}
		if ( strstr( manifests[ i ].GetManifestName(), "_pc" ) != NULL ) {
			continue;
		}
		manifests[ i ].RemoveAll( filename );
	}
}


/*
================
aRcFileSystemLocal::AddPerPlatformResources
================
*/
void aRcFileSystemLocal::AddRenderProgs( arcStringList &files ) {
	arcStringList work;

	// grab all the renderprogs
	arcNetString path = RelativePathToOSPath( "renderprogs/cgb", "fs_basepath" );
	ListOSFiles( path, "*.cgb", work );
	for ( int i = 0; i < work.Num(); i++ ) {
		files.Append( arcNetString( "renderprogs/cgb/" ) + work[i] );
	}

	path = RelativePathToOSPath( "renderprogs/hlsl", "fs_basepath" );
	ListOSFiles( path, "*.v360", work );
	for ( int i = 0; i < work.Num(); i++ ) {
		files.Append( arcNetString( "renderprogs/hlsl/" ) + work[i] );
	}
	ListOSFiles( path, "*.p360", work );
	for ( int i = 0; i < work.Num(); i++ ) {
		files.Append( arcNetString( "renderprogs/hlsl/" ) + work[i] );
	}

	path = RelativePathToOSPath( "renderprogs/gl", "fs_basepath" );
	ListOSFiles( path, "*.*", work );
	for ( int i = 0; i < work.Num(); i++ ) {
		files.Append( arcNetString( "renderprogs/gl/" ) + work[i] );
	}

}

/*
================
aRcFileSystemLocal::AddSoundResources
================
*/
void aRcFileSystemLocal::AddFonts( arcStringList &files ) {
	// temp fix for getting idaudio files in
	arcFileList *fl = ListFilesTree( "generated/images/newfonts", "*.bimage", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		files.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "newfonts", "*.dat", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		files.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
}


const char * excludeExtensions[] = {
	".idxma", ".idmsf", ".idwav", ".xma", ".msf", ".wav", ".resource"
};
const int numExcludeExtensions = sizeof( excludeExtensions ) / sizeof( excludeExtensions[ 0 ] );

bool IsExcludedFile( const arcNetString & resName ) {
	for ( int k = 0; k < numExcludeExtensions; k++ ) {
		if ( resName.Find( excludeExtensions[ k ], false ) >= 0 ) {
			return true;
		}
	}
	return false;
}

/*
================
aRcFileSystemLocal::IsBinaryModel
================
*/
bool aRcFileSystemLocal::IsBinaryModel( const arcNetString & resName ) const {
	aRcStaticString< 32 > ext;
	resName.ExtractFileExtension( ext );
	if ( ( ext.Icmp( "base" ) == 0 ) || ( ext.Icmp( "blwo" ) == 0 ) || ( ext.Icmp( "bflt" ) == 0 ) || ( ext.Icmp( "bma" ) == 0 ) ) {
		return true;
	}
	return false;
}

/*
================
aRcFileSystemLocal::IsSoundSample
================
*/
bool aRcFileSystemLocal::IsSoundSample( const arcNetString & resName ) const {
	aRcStaticString< 32 > ext;
	resName.ExtractFileExtension( ext );
	if ( ( ext.Icmp( "idxma" ) == 0 ) || ( ext.Icmp( "idwav" ) == 0 ) || ( ext.Icmp( "idmsf" ) == 0 ) || ( ext.Icmp( "xma" ) == 0 ) || ( ext.Icmp( "wav" ) == 0 ) || ( ext.Icmp( "msf" ) == 0 ) || ( ext.Icmp( "msadpcm" ) == 0 ) ) {
		return true;
	}
	return false;
}


void aRcFileSystemLocal::BuildOrderedStartupContainer() {
	arcStringList orderedFiles( 1024 );

	arcFileList * fl = ListFilesTree( "materials", "*.mtr", true );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "renderprogs", "*.v360", true );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "renderprogs", "*.p360", true );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "renderprogs", "*.cgb", true );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "renderprogs/gl", "*.*", true );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "skins", "*.skin", true );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "sound", "*.sndshd", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "def", "*.def", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "fx", "*.fx", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "particles", "*.prt", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	fl = ListFilesTree( "af", "*.af", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );
	fl = ListFilesTree( "newpdas", "*.pda", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	orderedFiles.Append( "script/doom_main.script" );
	orderedFiles.Append( "script/doom_defs.script" );
	orderedFiles.Append( "script/doom_defs.script" );
	orderedFiles.Append( "script/doom_events.script" );
	orderedFiles.Append( "script/doom_util.script" );
	orderedFiles.Append( "script/weapon_base.script" );
	orderedFiles.Append( "script/ai_base.script" );
	orderedFiles.Append( "script/weapon_fists.script" );
	orderedFiles.Append( "script/weapon_pistol.script" );
	orderedFiles.Append( "script/weapon_shotgun.script" );
	orderedFiles.Append( "script/weapon_machinegun.script" );
	orderedFiles.Append( "script/weapon_chaingun.script" );
	orderedFiles.Append( "script/weapon_handgrenade.script" );
	orderedFiles.Append( "script/weapon_plasmagun.script" );
	orderedFiles.Append( "script/weapon_rocketlauncher.script" );
	orderedFiles.Append( "script/weapon_bfg.script" );
	orderedFiles.Append( "script/weapon_soulcube.script" );
	orderedFiles.Append( "script/weapon_chainsaw.script" );
	orderedFiles.Append( "script/weapon_flashlight.script" );
	orderedFiles.Append( "script/weapon_pda.script" );
	orderedFiles.Append( "script/ai_monster_base.script" );
	orderedFiles.Append( "script/ai_monster_zombie_base.script" );
	orderedFiles.Append( "script/ai_monster_demon_archvile.script" );
	orderedFiles.Append( "script/ai_monster_demon_cherub.script" );
	orderedFiles.Append( "script/ai_monster_demon_hellknight.script" );
	orderedFiles.Append( "script/ai_monster_demon_imp.script" );
	orderedFiles.Append( "script/ai_monster_demon_maggot.script" );
	orderedFiles.Append( "script/ai_monster_demon_mancubus.script" );
	orderedFiles.Append( "script/ai_monster_demon_pinky.script" );
	orderedFiles.Append( "script/ai_monster_demon_revenant.script" );
	orderedFiles.Append( "script/ai_monster_demon_trite.script" );
	orderedFiles.Append( "script/ai_monster_demon_wraith.script" );
	orderedFiles.Append( "script/ai_monster_flying_lostsoul.script" );
	orderedFiles.Append( "script/ai_monster_flying_cacodemon.script" );
	orderedFiles.Append( "script/ai_monster_zombie.script" );
	orderedFiles.Append( "script/ai_monster_zombie_morgue.script" );
	orderedFiles.Append( "script/ai_monster_zombie_sawyer.script" );
	orderedFiles.Append( "script/ai_monster_zombie_bernie.script" );
	orderedFiles.Append( "script/ai_monster_zombie_commando_cgun.script" );
	orderedFiles.Append( "script/ai_monster_zombie_commando_tentacle.script" );
	orderedFiles.Append( "script/ai_monster_zombie_security_pistol.script" );
	orderedFiles.Append( "script/ai_monster_turret.script" );
	orderedFiles.Append( "script/ai_monster_boss_vagary.script" );
	orderedFiles.Append( "script/ai_monster_boss_cyberdemon.script" );
	orderedFiles.Append( "script/ai_monster_boss_guardian.script" );
	orderedFiles.Append( "script/ai_monster_boss_guardian_seeker.script" );
	orderedFiles.Append( "script/ai_monster_boss_sabaoth.script" );
	orderedFiles.Append( "script/ai_character.script" );
	orderedFiles.Append( "script/ai_character_prone.script" );
	orderedFiles.Append( "script/ai_character_sentry.script" );
	orderedFiles.Append( "script/ai_player.script" );
	orderedFiles.Append( "script/ai_alphalabs2_scientist1.script" );
	orderedFiles.Append( "script/ai_cinematic_le.script" );
	orderedFiles.Append( "script/map_admin1.script" );
	orderedFiles.Append( "script/map_alphalabs1.script" );
	orderedFiles.Append( "script/map_alphalabs2.script" );
	orderedFiles.Append( "script/map_alphalabs3.script" );
	orderedFiles.Append( "script/map_alphalabs3_crane.script" );
	orderedFiles.Append( "script/map_alphalabs4.script" );
	orderedFiles.Append( "script/map_caves.script" );
	orderedFiles.Append( "script/map_caves2.script" );
	orderedFiles.Append( "script/map_comm1.script" );
	orderedFiles.Append( "script/map_commoutside_lift.script" );
	orderedFiles.Append( "script/map_commoutside.script" );
	orderedFiles.Append( "script/map_cpu.script" );
	orderedFiles.Append( "script/map_cpuboss.script" );
	orderedFiles.Append( "script/map_delta1.script" );
	orderedFiles.Append( "script/map_delta2a.script" );
	orderedFiles.Append( "script/map_delta2b.script" );
	orderedFiles.Append( "script/map_delta3.script" );
	orderedFiles.Append( "script/map_delta5.script" );
	orderedFiles.Append( "script/map_enpro.script" );
	orderedFiles.Append( "script/map_hell1.script" );
	orderedFiles.Append( "script/map_hellhole.script" );
	orderedFiles.Append( "script/map_recycling1.script" );
	orderedFiles.Append( "script/map_recycling2.script" );
	orderedFiles.Append( "script/map_site3.script" );
	orderedFiles.Append( "script/map_marscity1.script" );
	orderedFiles.Append( "script/map_marscity2.script" );
	orderedFiles.Append( "script/map_mc_underground.script" );
	orderedFiles.Append( "script/map_monorail.script" );
	orderedFiles.Append( "script/d3xp_events.script" );
	orderedFiles.Append( "script/weapon_bloodstone_passive.script" );
	orderedFiles.Append( "script/weapon_bloodstone_active1.script" );
	orderedFiles.Append( "script/weapon_bloodstone_active2.script" );
	orderedFiles.Append( "script/weapon_bloodstone_active3.script" );
	orderedFiles.Append( "script/weapon_shotgun_double.script" );
	orderedFiles.Append( "script/weapon_grabber.script" );
	orderedFiles.Append( "script/ai_monster_hunter_helltime.script" );
	orderedFiles.Append( "script/ai_monster_hunter_berserk.script" );
	orderedFiles.Append( "script/ai_monster_hunter_invul.script" );
	orderedFiles.Append( "script/ai_monster_boss_maledict.script" );
	orderedFiles.Append( "script/ai_monster_demon_vulgar.script" );
	orderedFiles.Append( "script/ai_monster_demon_d3xp_bruiser.script" );
	orderedFiles.Append( "script/ai_monster_dummy_target.script" );
	orderedFiles.Append( "script/ai_monster_dummy.script" );
	orderedFiles.Append( "script/ai_monster_demon_sentry.script" );
	orderedFiles.Append( "script/ai_monster_demon_trite_jump.script" );
	orderedFiles.Append( "script/ai_monster_turret_ancient.script" );
	orderedFiles.Append( "script/ai_monster_flying_forgotten.script" );
	orderedFiles.Append( "script/ai_character_erebus3.script" );
	orderedFiles.Append( "script/d3xp_airlock.script" );
	orderedFiles.Append( "script/d3xp_bloodstone.script" );
	orderedFiles.Append( "script/map_erebus1.script" );
	orderedFiles.Append( "script/map_erebus2_helltime.script" );
	orderedFiles.Append( "script/map_erebus2.script" );
	orderedFiles.Append( "script/map_erebus3.script" );
	orderedFiles.Append( "script/map_erebus4.script" );
	orderedFiles.Append( "script/map_erebus5.script" );
	orderedFiles.Append( "script/map_erebus5_cloud.script" );
	orderedFiles.Append( "script/map_erebus6.script" );
	orderedFiles.Append( "script/map_erebus6_berzerk.script" );
	orderedFiles.Append( "script/map_phobos1.script" );
	orderedFiles.Append( "script/map_phobos2.script" );
	orderedFiles.Append( "script/map_phobos2_invul.script" );
	orderedFiles.Append( "script/map_phobos3.script" );
	orderedFiles.Append( "script/map_phobos4.script" );
	orderedFiles.Append( "script/map_deltax.script" );
	orderedFiles.Append( "script/map_hell.script" );
	orderedFiles.Append( "script/map_maledict.script" );
	orderedFiles.Append( "script/d3le-ai_monster_boss_guardian2.script" );
	orderedFiles.Append( "script/ai_follower.script" );
	orderedFiles.Append( "generated/swf/shell.bswf" );
	fl = ListFilesTree( "newfonts", "*.dat", false );
	for ( int i = 0; i < fl->GetList().Num(); i++ ) {
		orderedFiles.AddUnique( fl->GetList()[i] );
	}
	FreeFileList( fl );

	aRcResContainers::WriteResourceFile( "_ordered.resources", orderedFiles, false );
}

/*
================
aRcFileSystemLocal::WriteResourcePacks
================
*/
void aRcFileSystemLocal::WriteResourcePacks() {

	arcStringList filesNotCommonToAllMaps( 16384 );		// files that are not shared by all maps, used to trim the common list
	arcStringList filesCommonToAllMaps( 16384 );		// files that are shared by all maps, will include startup files, renderprogs etc..
	aRcPreloadManifest commonPreloads;				// preload entries that exist in all map preload files

	arcNetString path = RelativePathToOSPath( "maps/", "fs_savepath" );

	arcStringList manifestFiles;
	ListOSFiles( path, ".manifest", manifestFiles );
	arcStringList preloadFiles;
	ListOSFiles( path, ".preload", preloadFiles );

	arcNetList< aRcManifest > manifests;				// list of all manifest files
	// load all file manifests
	for ( int i = 0; i < manifestFiles.Num(); i++ ) {
		arcNetString path = "maps/";
		path += manifestFiles[ i ];
		aRcManifest manifest;
		if ( manifest.LoadManifest( path ) ) {
			//manifest.Print();
			manifest.RemoveAll( va( "strings/%s", ID_LANG_ENGLISH ) );	// remove all .lang files
			manifest.RemoveAll( va( "strings/%s", ID_LANG_FRENCH ) );
			manifest.RemoveAll( va( "strings/%s", ID_LANG_ITALIAN ) );
			manifest.RemoveAll( va( "strings/%s", ID_LANG_GERMAN ) );
			manifest.RemoveAll( va( "strings/%s", ID_LANG_SPANISH ) );
			manifest.RemoveAll( va( "strings/%s", ID_LANG_JAPANESE ) );
			manifests.Append( manifest );
		}
	}

	arcNetList< aRcPreloadManifest > preloadManifests;	// list of all preload manifest files
	// load all preload manifests
	for ( int i = 0; i < preloadFiles.Num(); i++ ) {
		arcNetString path = "maps/";
		path += preloadFiles[ i ];
		if ( path.Find( "_startup", false ) >= 0 ) {
			continue;
		}
		aRcPreloadManifest preload;
		if ( preload.LoadManifest( path ) ) {
			preloadManifests.Append( preload );
			//preload.Print();
		}
	}

	// build common list of files
	for ( int i = 0; i < manifests.Num(); i++ ) {
		aRcManifest &manifest = manifests[ i ];
		for ( int j = 0; j < manifest.NumFiles(); j++ ) {
			arcNetString name = manifest.GetFileNameByIndex( j );
			if ( name.CheckExtension( ".cfg" ) || (name.Find( ".lang", false ) >= 0 ) ) {
				continue;
			}
			if ( FileExistsInAllManifests( name, manifests ) ) {
				filesCommonToAllMaps.AddUnique( name );
			} else {
				filesNotCommonToAllMaps.AddUnique( name );
			}
		}
	}
	// common list of preload reosurces, image, sample or models
	for ( int i = 0; i < preloadManifests.Num(); i++ ) {
		aRcPreloadManifest &preload = preloadManifests[ i ];
		for ( int j = 0; j < preload.NumResources(); j++ ) {
			arcNetString name = preload.GetResourceNameByIndex( j );
			if ( FileExistsInAllPreloadManifests( name, preloadManifests ) ) {
				commonPreloads.Add( preload.GetPreloadByIndex( j ) );
				arcLibrary::Printf( "Common preload added %s\n", name.c_str() );
			} else {
				arcLibrary::Printf( "preload missed %s\n", name.c_str() );
			}
		}
	}

	AddRenderProgs( filesCommonToAllMaps );
	AddFonts( filesCommonToAllMaps );

	arcStringList work;

	// remove all common files from each map manifest
	for ( int i = 0; i < manifests.Num(); i++ ) {
		if ( ( strstr( manifests[ i ].GetManifestName(), "_startup" ) != NULL ) || ( strstr( manifests[ i ].GetManifestName(), "_pc" ) != NULL ) ) {
			continue;
		}
		//arcLibrary::Printf( "%04d referenced files for %s\n", manifests[ i ].GetReferencedFileCount(), manifests[ i ].GetManifestName() );

		for ( int j = 0; j < filesCommonToAllMaps.Num(); j++ ) {
			manifests[ i ].RemoveAll( filesCommonToAllMaps[ j ] );
		}
		//arcLibrary::Printf( "%04d referenced files for %s\n", manifests[ i ].GetReferencedFileCount(), manifests[ i ].GetManifestName() );
	}

	arcStringList commonImages( 2048 );
	arcStringList commonModels( 2048 );
	arcStringList commonAnims( 2048 );
	arcStringList commonCollision( 2048 );
	arcStringList soundFiles( 2048 );		// don't write these per map so we fit on disc

	for ( int i = 0; i < manifests.Num(); i++ ) {
		arcNetString resourceFileName = manifests[ i ].GetManifestName();
		if ( resourceFileName.Find( "_startup.manifest", false ) >= 0 ) {
			// add all the startup manifest files to the common list
			for ( int j = 0; j < manifests[ i ].NumFiles(); j++ ) {
				arcNetString check = manifests[i].GetFileNameByIndex( j );
				if ( check.CheckExtension( ".cfg" ) == false ) {
					filesCommonToAllMaps.AddUnique( check.c_str() );
				}
			}
			continue;
		}

		arcStaticList< arcNetString, 16384 > mapFiles;		// map files from the manifest, these are static for easy debugging
		arcStaticList< arcNetString, 16384 > mapFilesTwo;	// accumulates non bimage, bmodel and sample files
		commonImages.Clear();	// collect images and models separately so they can be added in linear preload order
		commonModels.Clear();
		commonAnims.Clear();
		commonCollision.Clear();

		manifests[ i ].PopulateList( mapFiles );

		for ( int j = 0; j < mapFiles.Num(); j++ ) {
			arcNetString & resName = mapFiles[ j ];
			if ( resName.Find( ".bimage", false ) >= 0 ) {
				commonImages.AddUnique( resName );
				continue;
			}
			if ( IsBinaryModel( resName ) ) {
				commonModels.AddUnique( resName );
				continue;
			}
			if ( IsSoundSample( resName ) ) {
				soundFiles.AddUnique( resName );
				continue;
			}
			if ( resName.Find( ".bik", false ) >= 0 ) {
				// don't add bik files
				continue;
			}
			if ( resName.Find ( ".bmd5anim", false ) >= 0 ) {
				commonAnims.AddUnique( resName );
				continue;
			}
			if ( resName.Find ( ".bcmodel", false ) >= 0 ) {
				commonCollision.AddUnique( resName );
				continue;
			}
			if ( resName.Find( ".lang", false ) >= 0 ) {
				continue;
			}
			mapFilesTwo.AddUnique( resName );
		}

		for ( int j = 0; j < commonImages.Num(); j++ ) {
			mapFilesTwo.AddUnique( commonImages[ j ] );
		}
		for ( int j = 0; j < commonModels.Num(); j++ ) {
			mapFilesTwo.AddUnique( commonModels[ j ] );
		}
		for ( int j = 0; j < commonAnims.Num(); j++ ) {
			mapFilesTwo.AddUnique( commonAnims[ j ] );
		}
		for ( int j = 0; j < commonCollision.Num(); j++ ) {
			mapFilesTwo.AddUnique( commonCollision[ j ] );
		}
		// write map resources
		arcStringList mapFilesToWrite;
		for ( int j = 0; j < mapFilesTwo.Num(); j++ ) {
			mapFilesToWrite.Append( mapFilesTwo[ j ] );
		}
		aRcResContainers::WriteResourceFile( resourceFileName, mapFilesToWrite, false );
	}

	// add  the new manifests just written
	path = RelativePathToOSPath( "maps", "fs_savepath" );
	ListOSFiles( path, "*.preload", work );
	for ( int i = 0; i < work.Num(); i++ ) {
		filesCommonToAllMaps.Append( arcNetString( "maps/" ) + work[ i ] );
	}

	filesCommonToAllMaps.Append( "_common.preload" );

	// write out common models, images and sounds to separate containers
	//arcStringList commonSounds( 2048 );
	commonImages.Clear();
	commonModels.Clear();

	arcStringList commonFiles;
	for ( int i = 0; i < filesCommonToAllMaps.Num(); i++ ) {
		arcNetString & resName = filesCommonToAllMaps[ i ];
		if ( resName.Find( ".bimage", false ) >= 0 ) {
			commonImages.AddUnique( resName );
			continue;
		}
		if ( IsBinaryModel( resName ) ) {
			commonModels.AddUnique( resName );
			continue;
		}
		if ( IsSoundSample( resName ) ) {
			soundFiles.AddUnique( resName );
			continue;
		}
		if ( resName.Find( ".bik", false ) >= 0 ) {
			// no bik files in the .resource
			continue;
		}
		if ( resName.Find( ".lang", false ) >= 0 ) {
			// no bik files in the .resource
			continue;
		}
		commonFiles.AddUnique( resName );
	}

	for ( int j = 0; j < commonImages.Num(); j++ ) {
		commonFiles.AddUnique( commonImages[ j ] );
	}
	for ( int j = 0; j < commonModels.Num(); j++ ) {
		commonFiles.AddUnique( commonModels[ j ] );
	}

	//aRcResContainers::WriteResourceFile( "_common_images", commonImages );
	//aRcResContainers::WriteResourceFile( "_common_models", commonModels );

	commonPreloads.WriteManifest( "_common.preload" );
	aRcResContainers::WriteResourceFile( "_common", commonFiles, false );


	arcNetList< arcStringList > soundOutputFiles;
	soundOutputFiles.SetNum( 16 );

	struct soundVOInfo_t {
		const char *filename;
		const char *voqualifier;
		arcStringList * samples;
	};
	const soundVOInfo_t soundFileInfo[] = {
		{ "fr", "sound/vo/french/", &soundOutputFiles[ 0 ] },
		{ "it", "sound/vo/italian/", &soundOutputFiles[ 1 ] },
		{ "gr", "sound/vo/german/", &soundOutputFiles[ 2 ] },
		{ "sp", "sound/vo/spanish/", &soundOutputFiles[ 3 ] },
		{ "jp", "sound/vo/japanese/", &soundOutputFiles[ 4 ] },
		{ "en", "sound/vo/", &soundOutputFiles[ 5 ] }	// english last so the other langs are culled first
	};
	const int numSoundFiles = sizeof( soundFileInfo ) / sizeof ( soundVOInfo_t );

	for ( int k = soundFiles.Num() - 1; k > 0; k-- ) {
		for ( int l = 0; l < numSoundFiles; l++ ) {
			if ( soundFiles[ k ].Find( soundFileInfo[ l ].voqualifier, false ) >= 0 ) {
				soundFileInfo[ l ].samples->AddUnique( soundFiles[ k ] );
				soundFiles.RemoveIndex( k );
			}
		}
	}

	for ( int k = 0; k < numSoundFiles; k++ ) {
		arcStringList & sampleList = *soundFileInfo[ k ].samples;

		// write pc
		aRcResContainers::WriteResourceFile( va( "_sound_pc_%s", soundFileInfo[ k ].filename ), sampleList, false );
		for ( int l = 0; l < sampleList.Num(); l++ ) {
			sampleList[ l ].Replace( ".idwav", ".idxma" );
		}
	}

	aRcResContainers::WriteResourceFile( "_sound_pc", soundFiles, false );
	for ( int k = 0; k < soundFiles.Num(); k++ ) {
		soundFiles[ k ].Replace( ".idwav", ".idxma" );
	}

	for ( int k = 0; k < soundFiles.Num(); k++ ) {
		soundFiles[ k ].Replace( ".idxma", ".idmsf" );
	}

	BuildOrderedStartupContainer();

	ClearResourcePacks();
}


/*
=================
aRcFileSystemLocal::CopyFile

Copy a fully specified file from one place to another`
=================
*/
void aRcFileSystemLocal::CopyFile( const char *fromOSPath, const char *toOSPath ) {

	arcNetFile * src = OpenExplicitFileRead( fromOSPath );
	if ( src == NULL ) {
		arcLibrary::Warning( "Could not open %s for read", fromOSPath );
		return;
	}

	if ( arcNetString::Icmp( fromOSPath, toOSPath ) == 0 ) {
		// same file can happen during build games
		return;
	}

	CopyFile( src, toOSPath );
	delete src;

	if ( strstr( fromOSPath, ".wav" ) != NULL ) {
		aRcStaticString< MAX_OSPATH > newFromPath = fromOSPath;
		aRcStaticString< MAX_OSPATH > newToPath = toOSPath;

		arcLibrary::Printf( "Copying console samples for %s\n", newFromPath.c_str() );
		newFromPath.SetFileExtension( "xma" );
		newToPath.SetFileExtension( "xma" );
		src = OpenExplicitFileRead( newFromPath );
		if ( src == NULL ) {
			arcLibrary::Warning( "Could not open %s for read", newFromPath.c_str() );
		} else {
			CopyFile( src, newToPath );
			delete src;
			src = NULL;
		}

		newFromPath.SetFileExtension( "msf" );
		newToPath.SetFileExtension( "msf" );
		src = OpenExplicitFileRead( newFromPath );
		if ( src == NULL ) {
			arcLibrary::Warning( "Could not open %s for read", newFromPath.c_str() );
		} else {
			CopyFile( src, newToPath );
			delete src;
		}

		newFromPath.BackSlashesToSlashes();
		newFromPath.ToLower();
		if ( newFromPath.Find( "/vo/", false ) >= 0 ) {
			for ( int i = 0; i < Sys_NumLangs(); i++ ) {
				const char *lang = Sys_Lang( i );
				if ( arcNetString::Icmp( lang, ID_LANG_ENGLISH ) == 0 ) {
					continue;
				}
				newFromPath = fromOSPath;
				newToPath = toOSPath;
				newFromPath.BackSlashesToSlashes();
				newFromPath.ToLower();
				newToPath.BackSlashesToSlashes();
				newToPath.ToLower();
				newFromPath.Replace( "/vo/", va( "/vo/%s/", lang ) );
				newToPath.Replace( "/vo/", va( "/vo/%s/", lang ) );

				src = OpenExplicitFileRead( newFromPath );
				if ( src == NULL ) {
					arcLibrary::Warning( "LOCALIZATION PROBLEM: Could not open %s for read", newFromPath.c_str() );
				} else {
					CopyFile( src, newToPath );
					delete src;
					src = NULL;
				}

				newFromPath.SetFileExtension( "xma" );
				newToPath.SetFileExtension( "xma" );
				src = OpenExplicitFileRead( newFromPath );
				if ( src == NULL ) {
					arcLibrary::Warning( "LOCALIZATION PROBLEM: Could not open %s for read", newFromPath.c_str() );
				} else {
					CopyFile( src, newToPath );
					delete src;
					src = NULL;
				}

				newFromPath.SetFileExtension( "msf" );
				newToPath.SetFileExtension( "msf" );
				src = OpenExplicitFileRead( newFromPath );
				if ( src == NULL ) {
					arcLibrary::Warning( "LOCALIZATION PROBLEM: Could not open %s for read", newFromPath.c_str() );
				} else {
					CopyFile( src, newToPath );
					delete src;
				}

			}
		}
	}
}

/*
=================
aRcFileSystemLocal::CopyFile
=================
*/
void aRcFileSystemLocal::CopyFile( arcNetFile *src, const char *toOSPath ) {
	arcNetFile * dst = OpenExplicitFileWrite( toOSPath );
	if ( dst == NULL ) {
		arcLibrary::Warning( "Could not open %s for write", toOSPath );
		return;
	}

	common->Printf( "copy %s to %s\n", src->GetName(), toOSPath );

	int len = src->Length();
	int copied = 0;
	while ( copied < len ) {
		byte buffer[4096];
		int read = src->Read( buffer, Min( 4096, len - copied ) );
		if ( read <= 0 ) {
			arcLibrary::Warning( "Copy failed during read" );
			break;
	}
		int written = dst->Write( buffer, read );
		if ( written < read ) {
			arcLibrary::Warning( "Copy failed during write" );
			break;
	}
		copied += written;
	}

	delete dst;
}

/*
====================
aRcFileSystemLocal::ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
void aRcFileSystemLocal::ReplaceSeparators( arcNetString &path, char sep ) {
	char *s;

	for ( s = &path[ 0 ]; *s; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = sep;
		}
	}
}

/*
========================
IsOSPath
========================
*/
static bool IsOSPath( const char * path ) {
	assert( path );

	if ( arcNetString::Icmpn( path, "mtp:", 4 ) == 0 ) {
		return true;
	}


	if ( arcNetString::Length( path ) >= 2 ) {
		if ( path[ 1 ] == ':' ) {
			if ( ( path[ 0 ] > 64 && path[ 0 ] < 91 ) || ( path[ 0 ] > 96 && path[ 0 ] < 123 ) ) {
				// already an OS path starting with a drive.
				return true;
			}
		}
		if ( path[ 0 ] == '\\' || path[ 0 ] == '/' ) {
			// a root path
			return true;
		}
	}
	return false;
}

/*
========================
aRcFileSystemLocal::BuildOSPath
========================
*/
const char * aRcFileSystemLocal::BuildOSPath( const char * base, const char * relativePath ) {
	// handle case of this already being an OS path
	if ( IsOSPath( relativePath ) ) {
		return relativePath;
	}

	return BuildOSPath( base, gameFolder, relativePath );
}

/*
===================
aRcFileSystemLocal::BuildOSPath
===================
*/
const char *aRcFileSystemLocal::BuildOSPath( const char *base, const char *game, const char *relativePath ) {
	static char OSPath[MAX_STRING_CHARS];
	arcNetString newPath;

	// handle case of this already being an OS path
	if ( IsOSPath( relativePath ) ) {
		return relativePath;
	}

	arcNetString strBase = base;
	strBase.StripTrailing( '/' );
	strBase.StripTrailing( '\\' );
	sprintf( newPath, "%s/%s/%s", strBase.c_str(), game, relativePath );
	ReplaceSeparators( newPath );
	arcNetString::Copynz( OSPath, newPath, sizeof( OSPath ) );
	return OSPath;
}

/*
================
aRcFileSystemLocal::OSPathToRelativePath

takes a full OS path, as might be found in data from a media creation
program, and converts it to a relativePath by stripping off directories

Returns false if the osPath tree doesn't match any of the existing
search paths.

================
*/
const char *aRcFileSystemLocal::OSPathToRelativePath( const char *OSPath ) {
	if ( ( OSPath[0] != '/' ) && ( OSPath[0] != '\\' ) && ( arcNetString::FindChar( OSPath, ':' ) < 0 ) ) {
		// No colon and it doesn't start with a slash... it must already be a relative path
		return OSPath;
	}
	arcStaticList< aRcStaticString< 32 >, 5 > basePaths;
	basePaths.Append( "base" );
	basePaths.Append( "d3xp" );
	basePaths.Append( "d3le" );
	if ( fs_game.GetString()[0] != 0 ) {
		basePaths.Append( fs_game.GetString() );
	}
	if ( fs_game_base.GetString()[0] != 0 ) {
		basePaths.Append( fs_game_base.GetString() );
	}
	arcStaticList<int, MAX_OSPATH> slashes;
	for ( const char * s = OSPath; *s != 0; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			slashes.Append( s - OSPath );
		}
	}
	for ( int n = 0; n < slashes.Num() - 1; n++ ) {
		const char * start = OSPath + slashes[n] + 1;
		const char * end = OSPath + slashes[n+1];
		int componentLength = end - start;
		if ( componentLength == 0 ) {
			continue;
		}
		for ( int i = 0; i < basePaths.Num(); i++ ) {
			if ( componentLength != basePaths[i].Length() ) {
				continue;
			}
			if ( basePaths[i].Icmpn( start, componentLength ) == 0 ) {
				// There are some files like:
				// W:\d3xp\base\...
				// But we can't search backwards because there are others like:
				// W:\doom3\base\models\mapobjects\base\...
				// So instead we check for 2 base paths next to each other and take the 2nd in that case
				if ( n < slashes.Num() - 2 ) {
					const char * start2 = OSPath + slashes[n+1] + 1;
					const char * end2 = OSPath + slashes[n+2];
					int componentLength2 = end2 - start2;
					if ( componentLength2 > 0 ) {
						for ( int j = 0; j < basePaths.Num(); j++ ) {
							if ( componentLength2 != basePaths[j].Length() ) {
								continue;
							}
							if ( basePaths[j].Icmpn( start2, basePaths[j].Length() ) == 0 ) {
								return end2 + 1;
							}
						}
					}
				}
				return end + 1;
			}
		}
	}
	arcLibrary::Warning( "OSPathToRelativePath failed on %s", OSPath );
	return OSPath;
}

/*
=====================
aRcFileSystemLocal::RelativePathToOSPath

Returns a fully qualified path that can be used with stdio libraries
=====================
*/
const char *aRcFileSystemLocal::RelativePathToOSPath( const char *relativePath, const char *basePath ) {
	const char *path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}
	return BuildOSPath( path, gameFolder, relativePath );
}

/*
=================
aRcFileSystemLocal::RemoveFile
=================
*/
void aRcFileSystemLocal::RemoveFile( const char *relativePath ) {
	arcNetString OSPath;

	if ( fs_basepath.GetString()[0] ) {
		OSPath = BuildOSPath( fs_basepath.GetString(), gameFolder, relativePath );
		::DeleteFile( OSPath );
	}

	OSPath = BuildOSPath( fs_savepath.GetString(), gameFolder, relativePath );
	::DeleteFile( OSPath );
}

/*
========================
aRcFileSystemLocal::RemoveDir
========================
*/
bool aRcFileSystemLocal::RemoveDir( const char * relativePath ) {
	bool success = true;
	if ( fs_savepath.GetString()[0] ) {
		success &= Sys_Rmdir( BuildOSPath( fs_savepath.GetString(), relativePath ) );
	}
	success &= Sys_Rmdir( BuildOSPath( fs_basepath.GetString(), relativePath ) );
	return success;
}

/*
============
aRcFileSystemLocal::ReadFile

Filename are relative to the search path
a null buffer will just return the file length and time without loading
timestamp can be NULL if not required
============
*/
int aRcFileSystemLocal::ReadFile( const char *relativePath, void **buffer, ARC_TIME_T *timestamp ) {

	arcNetFile *	f;
	byte *		buf;
	int			len;
	bool		isConfig;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
		return 0;
	}

	if ( relativePath == NULL || !relativePath[0] ) {
		common->FatalError( "aRcFileSystemLocal::ReadFile with empty name\n" );
		return 0;
	}

	if ( timestamp ) {
		*timestamp = FILE_NOT_FOUND_TIMESTAMP;
	}

	if ( buffer ) {
		*buffer = NULL;
	}

	if ( buffer == NULL && timestamp != NULL && resourceFiles.Num() > 0 ) {
		static aRcResCacheEntries rc;
		int size = 0;
		if ( GetResourceCacheEntry( relativePath, rc ) ) {
			*timestamp = 0;
			size = rc.length;
		}
		return size;
	}

	buf = NULL;	// quiet compiler warning

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( relativePath, ".cfg" ) == relativePath + strlen( relativePath ) - 4 ) {
		isConfig = true;
		if ( eventLoop && eventLoop->JournalLevel() == 2 ) {
			int		r;

			loadCount++;
			loadStack++;

			common->DPrintf( "Loading %s from journal file.\n", relativePath );
			len = 0;
			r = eventLoop->com_journalDataFile->Read( &len, sizeof( len ) );
			if ( r != sizeof( len ) ) {
				*buffer = NULL;
				return -1;
			}
			buf = ( byte * )Mem_ClearedAlloc(len+1, TAG_IDFILE);
			*buffer = buf;
			r = eventLoop->com_journalDataFile->Read( buf, len );
			if ( r != len ) {
				common->FatalError( "Read from journalDataFile failed" );
			}

			// guarantee that it will have a trailing 0 for string operations
			buf[len] = 0;

			return len;
		}
	} else {
		isConfig = false;
	}

	// look for it in the filesystem or pack files
	f = OpenFileRead( relativePath, ( buffer != NULL ) );
	if ( f == NULL ) {
		if ( buffer ) {
			*buffer = NULL;
		}
		return -1;
	}
	len = f->Length();

	if ( timestamp ) {
		*timestamp = f->Timestamp();
	}

	if ( !buffer ) {
		CloseFile( f );
		return len;
	}

	loadCount++;
	loadStack++;

	buf = ( byte * )Mem_ClearedAlloc(len+1, TAG_IDFILE);
	*buffer = buf;

	f->Read( buf, len );

	// guarantee that it will have a trailing 0 for string operations
	buf[len] = 0;
	CloseFile( f );

	// if we are journalling and it is a config file, write it to the journal file
	if ( isConfig && eventLoop && eventLoop->JournalLevel() == 1 ) {
		common->DPrintf( "Writing %s to journal file.\n", relativePath );
		eventLoop->com_journalDataFile->Write( &len, sizeof( len ) );
		eventLoop->com_journalDataFile->Write( buf, len );
		eventLoop->com_journalDataFile->Flush();
	}

	return len;
}

/*
=============
aRcFileSystemLocal::FreeFile
=============
*/
void aRcFileSystemLocal::FreeFile( void *buffer ) {
	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	if ( !buffer ) {
		common->FatalError( "aRcFileSystemLocal::FreeFile( NULL )" );
	}
	loadStack--;

	Mem_Free( buffer );
}

/*
============
aRcFileSystemLocal::WriteFile

Filenames are relative to the search path
============
*/
int aRcFileSystemLocal::WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath ) {
	arcNetFile *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	if ( !relativePath || !buffer ) {
		common->FatalError( "aRcFileSystemLocal::WriteFile: NULL parameter" );
	}

	f = aRcFileSystemLocal::OpenFileWrite( relativePath, basePath );
	if ( !f ) {
		common->Printf( "Failed to open %s\n", relativePath );
		return -1;
	}

	size = f->Write( buffer, size );

	CloseFile( f );

	return size;
}

/*
========================
aRcFileSystemLocal::RenameFile
========================
*/
bool aRcFileSystemLocal::RenameFile( const char * relativePath, const char * newName, const char * basePath ) {
	const char * path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}

	arcNetString oldOSPath = BuildOSPath( path, gameFolder, relativePath );
	arcNetString newOSPath = BuildOSPath( path, gameFolder, newName );

	// this gives atomic-delete-on-rename, like POSIX rename()
	// There is a MoveFileTransacted() on vista and above, not sure if that means there
	// is a race condition inside MoveFileEx...
	const bool success = ( MoveFileEx( oldOSPath.c_str(), newOSPath.c_str(), MOVEFILE_REPLACE_EXISTING ) != 0 );

	if ( !success ) {
		const int err = GetLastError();
		arcLibrary::Warning( "RenameFile( %s, %s ) error %i", newOSPath.c_str(), oldOSPath.c_str(), err );
	}
	return success;
}

/*
===============
aRcFileSystemLocal::AddUnique
===============
*/
int aRcFileSystemLocal::AddUnique( const char *name, arcStringList &list, ARCHashIndex &hashIndex ) const {
	int i, hashKey;

	hashKey = hashIndex.GenerateKey( name );
	for ( i = hashIndex.First( hashKey ); i >= 0; i = hashIndex.Next( i ) ) {
		if ( list[i].Icmp( name ) == 0 ) {
			return i;
		}
	}
	i = list.Append( name );
	hashIndex.Add( hashKey, i );
	return i;
}

/*
===============
aRcFileSystemLocal::GetExtensionList
===============
*/
void aRcFileSystemLocal::GetExtensionList( const char *extension, arcStringList &extensionList ) const {
	int s, e, l;

	l = arcNetString::Length( extension );
	s = 0;
	while( 1 ) {
		e = arcNetString::FindChar( extension, '|', s, l );
		if ( e != -1 ) {
			extensionList.Append( arcNetString( extension, s, e ) );
			s = e + 1;
		} else {
			extensionList.Append( arcNetString( extension, s, l ) );
			break;
		}
	}
}

/*
===============
aRcFileSystemLocal::GetFileList

Does not clear the list first so this can be used to progressively build a file list.
When 'sort' is true only the new files added to the list are sorted.
===============
*/
int aRcFileSystemLocal::GetFileList( const char *relativePath, const arcStringList &extensions, arcStringList &list, ARCHashIndex &hashIndex, bool fullRelativePath, const char * gamedir ) {
	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	if ( !extensions.Num() ) {
		return 0;
	}

	if ( !relativePath ) {
		return 0;
	}

	int pathLength = strlen( relativePath );
	if ( pathLength ) {
		pathLength++;	// for the trailing '/'
	}

	aRcStaticString< MAX_OSPATH > strippedName;
	if ( resourceFiles.Num() > 0 ) {
		int idx = resourceFiles.Num() - 1;
		while ( idx >= 0 ) {
			for ( int i = 0; i < resourceFiles[ idx ]->cacheTable.Num(); i++ ) {
				aRcResCacheEntries & rt = resourceFiles[ idx ]->cacheTable[ i ];
				// if the name is not long anough to at least contain the path

				if ( rt.filename.Length() <= pathLength ) {
					continue;
				}

				// check for a path match without the trailing '/'
				if ( pathLength && arcNetString::Icmpn( rt.filename, relativePath, pathLength - 1 ) != 0 ) {
					continue;
				}

				// ensure we have a path, and not just a filename containing the path
				if ( rt.filename[ pathLength ] == '\0' || rt.filename[pathLength - 1] != '/' ) {
					continue;
				}

				// make sure the file is not in a subdirectory
				int j = pathLength;
				for (; rt.filename[j+1] != '\0'; j++ ) {
					if ( rt.filename[ j ] == '/' ) {
						break;
					}
				}
				if ( rt.filename[ j + 1 ] ) {
					continue;
				}

				// check for extension match
				for ( j = 0; j < extensions.Num(); j++ ) {
					if ( rt.filename.Length() >= extensions[j].Length() && extensions[j].Icmp( rt.filename.c_str() +   rt.filename.Length() - extensions[j].Length() ) == 0 ) {
						break;
					}
				}
				if ( j >= extensions.Num() ) {
					continue;
				}

				// unique the match
				if ( fullRelativePath ) {
					arcNetString work = relativePath;
					work += "/";
					work += rt.filename.c_str() + pathLength;
					work.StripTrailing( '/' );
					AddUnique( work, list, hashIndex );
				} else {
					arcNetString work = rt.filename.c_str() + pathLength;
					work.StripTrailing( '/' );
					AddUnique( work, list, hashIndex );
				}
			}
			idx--;
		}
	}

	// search through the path, one element at a time, adding to list
	for ( int sp = searchPaths.Num() - 1; sp >= 0; sp-- ) {
		if ( gamedir != NULL && gamedir[0] != 0 ) {
			if ( searchPaths[sp].gamedir != gamedir) {
				continue;
			}
		}

		arcNetString netpath = BuildOSPath( searchPaths[sp].path, searchPaths[sp].gamedir, relativePath );

		for ( int i = 0; i < extensions.Num(); i++ ) {

			// scan for files in the filesystem
			arcStringList sysFiles;
			ListOSFiles( netpath, extensions[i], sysFiles );

			// if we are searching for directories, remove . and ..
			if ( extensions[i][0] == '/' && extensions[i][1] == 0 ) {
				sysFiles.Remove( "." );
				sysFiles.Remove( ".." );
			}

			for ( int j = 0; j < sysFiles.Num(); j++ ) {
				// unique the match
				if ( fullRelativePath ) {
					arcNetString work = relativePath;
					work += "/";
					work += sysFiles[j];
					AddUnique( work, list, hashIndex );
				} else {
					AddUnique( sysFiles[j], list, hashIndex );
				}
			}
		}
	}

	return list.Num();
}

/*
===============
aRcFileSystemLocal::ListFiles
===============
*/
arcFileList *aRcFileSystemLocal::ListFiles( const char *relativePath, const char *extension, bool sort, bool fullRelativePath, const char* gamedir ) {
	ARCHashIndex hashIndex( 4096, 4096 );
	arcStringList extensionList;

	arcFileList *fileList = new (TAG_IDFILE) arcFileList;
	fileList->basePath = relativePath;

	GetExtensionList( extension, extensionList );

	GetFileList( relativePath, extensionList, fileList->list, hashIndex, fullRelativePath, gamedir );

	if ( sort ) {
		fileList->list.SortWithTemplate( idSort_PathStr() );
	}

	return fileList;
}

/*
===============
aRcFileSystemLocal::GetFileListTree
===============
*/
int aRcFileSystemLocal::GetFileListTree( const char *relativePath, const arcStringList &extensions, arcStringList &list, ARCHashIndex &hashIndex, const char* gamedir ) {
	int i;
	arcStringList slash, folders( 128 );
	ARCHashIndex folderHashIndex( 1024, 128 );

	// recurse through the subdirectories
	slash.Append( "/" );
	GetFileList( relativePath, slash, folders, folderHashIndex, true, gamedir );
	for ( i = 0; i < folders.Num(); i++ ) {
		if ( folders[i][0] == '.' ) {
			continue;
		}
		if ( folders[i].Icmp( relativePath ) == 0 ){
			continue;
		}
		GetFileListTree( folders[i], extensions, list, hashIndex, gamedir );
	}

	// list files in the current directory
	GetFileList( relativePath, extensions, list, hashIndex, true, gamedir );

	return list.Num();
}

/*
===============
aRcFileSystemLocal::ListFilesTree
===============
*/
arcFileList *aRcFileSystemLocal::ListFilesTree( const char *relativePath, const char *extension, bool sort, const char* gamedir ) {
	ARCHashIndex hashIndex( 4096, 4096 );
	arcStringList extensionList;

	arcFileList *fileList = new (TAG_IDFILE) arcFileList();
	fileList->basePath = relativePath;
	fileList->list.SetGranularity( 4096 );

	GetExtensionList( extension, extensionList );

	GetFileListTree( relativePath, extensionList, fileList->list, hashIndex, gamedir );

	if ( sort ) {
		fileList->list.SortWithTemplate( idSort_PathStr() );
	}

	return fileList;
}

/*
===============
aRcFileSystemLocal::FreeFileList
===============
*/
void aRcFileSystemLocal::FreeFileList( arcFileList *fileList ) {
	delete fileList;
}

/*
===============
aRcFileSystemLocal::ListOSFiles

 call to the OS for a listing of files in an OS directory
===============
*/
int	aRcFileSystemLocal::ListOSFiles( const char *directory, const char *extension, arcStringList &list ) {
	if ( !extension ) {
		extension = "";
	}

	return Sys_ListFiles( directory, extension, list );
}

/*
================
aRcFileSystemLocal::Dir_f
================
*/
void aRcFileSystemLocal::Dir_f( const arcCommandArgs &args ) {
	arcNetString		relativePath;
	arcNetString		extension;
	arcFileList *fileList;
	int			i;

	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		common->Printf( "usage: dir <directory> [extension]\n" );
		return;
	}

	if ( args.Argc() == 2 ) {
		relativePath = args.Argv( 1 );
		extension = "";
	}
	else {
		relativePath = args.Argv( 1 );
		extension = args.Argv( 2 );
		if ( extension[0] != '.' ) {
			common->Warning( "extension should have a leading dot" );
		}
	}
	relativePath.BackSlashesToSlashes();
	relativePath.StripTrailing( '/' );

	common->Printf( "Listing of %s/*%s\n", relativePath.c_str(), extension.c_str() );
	common->Printf( "---------------\n" );

	fileList = fileSystemLocal.ListFiles( relativePath, extension );

	for ( i = 0; i < fileList->GetNumFiles(); i++ ) {
		common->Printf( "%s\n", fileList->GetFile( i ) );
	}
	common->Printf( "%d files\n", fileList->list.Num() );

	fileSystemLocal.FreeFileList( fileList );
}

/*
================
aRcFileSystemLocal::DirTree_f
================
*/
void aRcFileSystemLocal::DirTree_f( const arcCommandArgs &args ) {
	arcNetString		relativePath;
	arcNetString		extension;
	arcFileList *fileList;
	int			i;

	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		common->Printf( "usage: dirtree <directory> [extension]\n" );
		return;
	}

	if ( args.Argc() == 2 ) {
		relativePath = args.Argv( 1 );
		extension = "";
	}
	else {
		relativePath = args.Argv( 1 );
		extension = args.Argv( 2 );
		if ( extension[0] != '.' ) {
			common->Warning( "extension should have a leading dot" );
		}
	}
	relativePath.BackSlashesToSlashes();
	relativePath.StripTrailing( '/' );

	common->Printf( "Listing of %s/*%s /s\n", relativePath.c_str(), extension.c_str() );
	common->Printf( "---------------\n" );

	fileList = fileSystemLocal.ListFilesTree( relativePath, extension );

	for ( i = 0; i < fileList->GetNumFiles(); i++ ) {
		common->Printf( "%s\n", fileList->GetFile( i ) );
	}
	common->Printf( "%d files\n", fileList->list.Num() );

	fileSystemLocal.FreeFileList( fileList );
}

/*
================
aRcFileSystemLocal::ClearResourcePacks
================
*/
void aRcFileSystemLocal::ClearResourcePacks() {
}

/*
================
aRcFileSystemLocal::BuildGame_f
================
*/
void aRcFileSystemLocal::BuildGame_f( const arcCommandArgs &args ) {
	fileSystemLocal.WriteResourcePacks();
}

/*
================
aRcFileSystemLocal::WriteResourceFile_f
================
*/
void aRcFileSystemLocal::WriteResourceFile_f( const arcCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: writeResourceFile <manifest file>\n" );
		return;
	}

	arcStringList manifest;
	aRcResContainers::ReadManifestFile( args.Argv( 1 ), manifest );
	aRcResContainers::WriteResourceFile( args.Argv( 1 ), manifest, false );
}


/*
================
aRcFileSystemLocal::UpdateResourceFile_f
================
*/
void aRcFileSystemLocal::UpdateResourceFile_f( const arcCommandArgs &args ) {
	if ( args.Argc() < 3  ) {
		common->Printf( "Usage: updateResourceFile <resource file> <files>\n" );
		return;
	}

	arcNetString filename =  args.Argv( 1 );
	arcStringList filesToAdd;
	for ( int i = 2; i < args.Argc(); i++ ) {
		filesToAdd.Append( args.Argv( i ) );
	}
	aRcResContainers::UpdateResourceFile( filename, filesToAdd );
}

/*
================
aRcFileSystemLocal::ExtractResourceFile_f
================
*/
void aRcFileSystemLocal::ExtractResourceFile_f( const arcCommandArgs &args ) {
	if ( args.Argc() < 3  ) {
		common->Printf( "Usage: extractResourceFile <resource file> <outpath> <copysound>\n" );
		return;
	}

	arcNetString filename =  args.Argv( 1 );
	arcNetString outPath = args.Argv( 2 );
	bool copyWaves = ( args.Argc() > 3 );
	aRcResContainers::ExtractResourceFile( filename, outPath, copyWaves );
}

/*
============
aRcFileSystemLocal::Path_f
============
*/
void aRcFileSystemLocal::Path_f( const arcCommandArgs &args ) {
	common->Printf( "Current search path:\n" );
	for ( int i = 0; i < fileSystemLocal.searchPaths.Num(); i++ ) {
		common->Printf( "%s/%s\n", fileSystemLocal.searchPaths[i].path.c_str(), fileSystemLocal.searchPaths[i].gamedir.c_str() );
	}
}

/*
============
aRcFileSystemLocal::TouchFile_f

The only purpose of this function is to allow game script files to copy
arbitrary files furing an "fs_copyfiles 1" run.
============
*/
void aRcFileSystemLocal::TouchFile_f( const arcCommandArgs &args ) {
	arcNetFile *f;

	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: touchFile <file>\n" );
		return;
	}

	f = fileSystemLocal.OpenFileRead( args.Argv( 1 ) );
	if ( f ) {
		fileSystemLocal.CloseFile( f );
	}
}

/*
============
aRcFileSystemLocal::TouchFileList_f

Takes a text file and touches every file in it, use one file per line.
============
*/
void aRcFileSystemLocal::TouchFileList_f( const arcCommandArgs &args ) {

	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: touchFileList <filename>\n" );
		return;
	}

	const char *buffer = NULL;
	ARCParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	if ( fileSystem->ReadFile( args.Argv( 1 ), ( void** )&buffer, NULL ) && buffer ) {
		src.LoadMemory( buffer, strlen( buffer ), args.Argv( 1 ) );
		if ( src.IsLoaded() ) {
			arcNetToken token;
			while( src.ReadToken( &token ) ) {
				common->Printf( "%s\n", token.c_str() );
				const bool captureToImage = false;
				common->UpdateScreen( captureToImage );
				arcNetFile *f = fileSystemLocal.OpenFileRead( token );
				if ( f ) {
					fileSystemLocal.CloseFile( f );
				}
			}
		}
	}

}

/*
============
aRcFileSystemLocal::GenerateResourceCRCs_f

Generates a CRC checksum file for each .resources file.
============
*/
void aRcFileSystemLocal::GenerateResourceCRCs_f( const arcCommandArgs &args ) {
	arcLibrary::Printf( "Generating CRCs for resource files...\n" );

	std::auto_ptr<arcFileList> baseResourceFileList( fileSystem->ListFiles( ".", ".resources" ) );
	if ( baseResourceFileList.get() != NULL ) {
		CreateCRCsForResourceFileList ( *baseResourceFileList );
	}

	std::auto_ptr<arcFileList> mapResourceFileList( fileSystem->ListFilesTree( "maps", ".resources" ) );
	if ( mapResourceFileList.get() != NULL ) {
		CreateCRCsForResourceFileList ( *mapResourceFileList );
	}

	arcLibrary::Printf( "Done generating CRCs for resource files.\n" );
}

/*
================
aRcFileSystemLocal::CreateCRCsForResourceFileList
================
*/
void aRcFileSystemLocal::CreateCRCsForResourceFileList( const arcFileList & list ) {
	for ( int fileIndex = 0; fileIndex < list.GetNumFiles(); ++fileIndex ) {
		arcLibrary::Printf( " Processing %s.\n", list.GetFile( fileIndex ) );

		std::auto_ptr<aRcFileMemory> currentFile( static_cast<aRcFileMemory *>( fileSystem->OpenFileReadMemory( list.GetFile( fileIndex ) ) ) );

		if ( currentFile.get() == NULL ) {
			arcLibrary::Printf( " Error reading %s.\n", list.GetFile( fileIndex ) );
			continue;
		}

		uint32 resourceMagic;
		currentFile->ReadBig( resourceMagic );

		if ( resourceMagic != RESOURCE_FILE_MAGIC ) {
			arcLibrary::Printf( "Resource file magic number doesn't match, skipping %s.\n", list.GetFile( fileIndex ) );
			continue;
		}

		int tableOffset;
		currentFile->ReadBig( tableOffset );

		int tableLength;
		currentFile->ReadBig( tableLength );

		// Read in the table
		currentFile->Seek( tableOffset, FS_SEEK_SET );

		int numFileResources;
		currentFile->ReadBig( numFileResources );

		arcNetList< aRcResCacheEntries > cacheEntries;
		cacheEntries.SetNum( numFileResources );

		for ( int innerFileIndex = 0; innerFileIndex < numFileResources; ++innerFileIndex ) {
			cacheEntries[innerFileIndex].Read( currentFile.get() );
		}

		// All tables read, now seek to each one and calculate the CRC.
		arcTempArray< unsigned long > innerFileCRCs( numFileResources );
		for ( int innerFileIndex = 0; innerFileIndex < numFileResources; ++innerFileIndex ) {
			const char * innerFileDataBegin = currentFile->GetDataPtr() + cacheEntries[innerFileIndex].offset;

			innerFileCRCs[innerFileIndex] = CRC32_BlockChecksum( innerFileDataBegin, cacheEntries[innerFileIndex].length );
		}

		// Get the CRC for all the CRCs.
		const unsigned long totalCRC = CRC32_BlockChecksum( innerFileCRCs.Ptr(), innerFileCRCs.Size() );

		// Write the .crc file corresponding to the .resources file.
		arcNetString crcFilename = list.GetFile( fileIndex );
		crcFilename.SetFileExtension( ".crc" );
		std::auto_ptr<arcNetFile> crcOutputFile( fileSystem->OpenFileWrite( crcFilename, "fs_basepath" ) );
		if ( crcOutputFile.get() == NULL ) {
			arcLibrary::Printf( "Error writing CRC file %s.\n", crcFilename );
			continue;
		}

		const uint32 CRC_FILE_MAGIC = 0xCC00CC00; // I just made this up, it has no meaning.
		const uint32 CRC_FILE_VERSION = 1;
		crcOutputFile->WriteBig( CRC_FILE_MAGIC );
		crcOutputFile->WriteBig( CRC_FILE_VERSION );
		crcOutputFile->WriteBig( totalCRC );
		crcOutputFile->WriteBig( numFileResources );
		crcOutputFile->WriteBigArray( innerFileCRCs.Ptr(), numFileResources );
	}
}

/*
================
aRcFileSystemLocal::AddResourceFile
================
*/
int aRcFileSystemLocal::AddResourceFile( const char * resourceFileName ) {
	aRcStaticString< MAX_OSPATH > resourceFile = va( "maps/%s", resourceFileName );
	aRcResContainers *rc = new aRcResContainers();
	if ( rc->Init( resourceFile, resourceFiles.Num() ) ) {
		resourceFiles.Append( rc );
		common->Printf( "Loaded resource file %s\n", resourceFile.c_str() );
		return resourceFiles.Num() - 1;
	}
	return -1;
}

/*
================
aRcFileSystemLocal::FindResourceFile
================
*/
int aRcFileSystemLocal::FindResourceFile( const char * resourceFileName ) {
	for ( int i = 0; i < resourceFiles.Num(); i++ ) {
		if ( arcNetString::Icmp( resourceFileName, resourceFiles[ i ]->GetFileName() ) == 0 ) {
			return i;
		}
	}
	return -1;
}
/*
================
aRcFileSystemLocal::RemoveResourceFileByIndex
================
*/
void aRcFileSystemLocal::RemoveResourceFileByIndex( const int &idx ) {
	if ( idx >= 0 && idx < resourceFiles.Num() ) {
		if ( idx >= 0 && idx < resourceFiles.Num() ) {
			delete resourceFiles[ idx ];
			resourceFiles.RemoveIndex( idx );
			for ( int i = 0; i < resourceFiles.Num(); i++ ) {
				// fixup any container indexes
				resourceFiles[ i ]->SetContainerIndex( i );
			}
		}
	}
}

/*
================
aRcFileSystemLocal::RemoveMapResourceFile
================
*/
void aRcFileSystemLocal::RemoveMapResourceFile( const char * resourceFileName ) {
	int idx = FindResourceFile( va( "maps/%s", resourceFileName ) );
	if ( idx >= 0 ) {
		RemoveResourceFileByIndex( idx );
	}
}

/*
================
aRcFileSystemLocal::RemoveResourceFile
================
*/
void aRcFileSystemLocal::RemoveResourceFile( const char * resourceFileName ) {
	int idx = FindResourceFile( resourceFileName );
	if ( idx >= 0 ) {
		RemoveResourceFileByIndex( idx );
	}
}

/*
================
aRcFileSystemLocal::AddGameDirectory

Sets gameFolder, adds the directory to the head of the search paths
================
*/
void aRcFileSystemLocal::AddGameDirectory( const char *path, const char *dir ) {
	// check if the search path already exists
	for ( int i = 0; i < searchPaths.Num(); i++ ) {
		if ( searchPaths[i].path.Cmp( path ) == 0 && searchPaths[i].gamedir.Cmp( dir ) == 0 ) {
			return;
		}
	}

	gameFolder = dir;

	//
	// add the directory to the search path
	//
	searchpath_t & search = searchPaths.Alloc();
	search.path = path;
	search.gamedir = dir;

	arcNetString pakfile = BuildOSPath( path, dir, "" );
	pakfile[ pakfile.Length() - 1 ] = 0;	// strip the trailing slash

	arcStringList pakfiles;
	ListOSFiles( pakfile, ".resources", pakfiles );
	pakfiles.SortWithTemplate( idSort_PathStr() );
	if ( pakfiles.Num() > 0 ) {
		// resource files present, ignore pak files
		for ( int i = 0; i < pakfiles.Num(); i++ ) {
			pakfile = pakfiles[i]; //BuildOSPath( path, dir, pakfiles[i] );
			aRcResContainers *rc = new aRcResContainers();
			if ( rc->Init( pakfile, resourceFiles.Num() ) ) {
				resourceFiles.Append( rc );
				common->Printf( "Loaded resource file %s\n", pakfile.c_str() );
				//com_productionMode.SetInteger( 2 );
			}
		}
	}
}

/*
================
aRcFileSystemLocal::SetupGameDirectories

  Takes care of the correct search order.
================
*/
void aRcFileSystemLocal::SetupGameDirectories( const char *gameName ) {
	// setup basepath
	if ( fs_basepath.GetString()[0] ) {
		AddGameDirectory( fs_basepath.GetString(), gameName );
	}
	// setup savepath
	if ( fs_savepath.GetString()[0] ) {
		AddGameDirectory( fs_savepath.GetString(), gameName );
	}
}


const char *cachedStartupFiles[] = {
	"game:\\base\\video\\loadvideo.bik"
};
const int numStartupFiles = sizeof( cachedStartupFiles ) / sizeof ( cachedStartupFiles[ 0 ] );

const char *cachedNormalFiles[] = {
	"game:\\base\\_sound_xenon_en.resources",	// these will fail silently on the files that are not on disc
	"game:\\base\\_sound_xenon_fr.resources",
	"game:\\base\\_sound_xenon_jp.resources",
	"game:\\base\\_sound_xenon_sp.resources",
	"game:\\base\\_sound_xenon_it.resources",
	"game:\\base\\_sound_xenon_gr.resources",
	"game:\\base\\_sound_xenon.resources",
	"game:\\base\\_common.resources",
	"game:\\base\\_ordered.resources",
	"game:\\base\\video\\mars_rotation.bik"		// cache this to save the consumer from hearing SEEK.. SEEK... SEEK.. SEEK  SEEEK while at the main menu
};
const int numNormalFiles = sizeof( cachedNormalFiles ) / sizeof ( cachedNormalFiles[ 0 ] );

const char *dontCacheFiles[] = {
	"game:\\base\\maps\\*.*",	// these will fail silently on the files that are not on disc
	"game:\\base\\video\\*.*",
	"game:\\base\\sound\\*.*",
};
const int numDontCacheFiles = sizeof( dontCacheFiles ) / sizeof ( dontCacheFiles[ 0 ] );

/*
================
aRcFileSystemLocal::InitPrecache
================
*/
void aRcFileSystemLocal::InitPrecache() {
	if ( !fs_enableBackgroundCaching.GetBool() ) {
		return;
	}
	numFilesOpenedAsCached = 0;
}

/*
================
aRcFileSystemLocal::ReOpenCacheFiles
================
*/
void aRcFileSystemLocal::ReOpenCacheFiles() {

	if ( !fs_enableBackgroundCaching.GetBool() ) {
		return;
	}
}


/*
================
aRcFileSystemLocal::Startup
================
*/
void aRcFileSystemLocal::Startup() {
	common->Printf( "------ Initializing File System ------\n" );

	InitPrecache();

	SetupGameDirectories( BASE_GAMEDIR );

	// fs_game_base override
	if ( fs_game_base.GetString()[0] &&
		 arcNetString::Icmp( fs_game_base.GetString(), BASE_GAMEDIR ) ) {
		SetupGameDirectories( fs_game_base.GetString() );
	}

	// fs_game override
	if ( fs_game.GetString()[0] &&
		 arcNetString::Icmp( fs_game.GetString(), BASE_GAMEDIR ) &&
		 arcNetString::Icmp( fs_game.GetString(), fs_game_base.GetString() ) ) {
		SetupGameDirectories( fs_game.GetString() );
	}

	// add our commands
	cmdSystem->AddCommand( "dir", Dir_f, CMD_FL_SYSTEM, "lists a folder", arcCmdSystem::ArgCompletion_FileName );
	cmdSystem->AddCommand( "dirtree", DirTree_f, CMD_FL_SYSTEM, "lists a folder with subfolders" );
	cmdSystem->AddCommand( "path", Path_f, CMD_FL_SYSTEM, "lists search paths" );
	cmdSystem->AddCommand( "touchFile", TouchFile_f, CMD_FL_SYSTEM, "touches a file" );
	cmdSystem->AddCommand( "touchFileList", TouchFileList_f, CMD_FL_SYSTEM, "touches a list of files" );

	cmdSystem->AddCommand( "buildGame", BuildGame_f, CMD_FL_SYSTEM, "builds game pak files" );
	cmdSystem->AddCommand( "writeResourceFile", WriteResourceFile_f, CMD_FL_SYSTEM, "writes a .resources file from a supplied manifest" );
	cmdSystem->AddCommand( "extractResourceFile", ExtractResourceFile_f, CMD_FL_SYSTEM, "extracts to the supplied resource file to the supplied path" );
	cmdSystem->AddCommand( "updateResourceFile", UpdateResourceFile_f, CMD_FL_SYSTEM, "updates or appends the supplied files in the supplied resource file" );

	cmdSystem->AddCommand( "generateResourceCRCs", GenerateResourceCRCs_f, CMD_FL_SYSTEM, "Generates CRC checksums for all the resource files." );

	// print the current search paths
	Path_f( arcCommandArgs() );

	common->Printf( "file system initialized.\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
================
aRcFileSystemLocal::Init

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void aRcFileSystemLocal::Init() {
	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	common->StartupVariable( "fs_basepath" );
	common->StartupVariable( "fs_savepath" );
	common->StartupVariable( "fs_game" );
	common->StartupVariable( "fs_game_base" );
	common->StartupVariable( "fs_copyfiles" );

	if ( fs_basepath.GetString()[0] == '\0' ) {
		fs_basepath.SetString( Sys_DefaultBasePath() );
	}
	if ( fs_savepath.GetString()[0] == '\0' ) {
		fs_savepath.SetString( Sys_DefaultSavePath() );
	}

	// try to start up normally
	Startup();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	// Dedicated servers can run with no outside files at all
	if ( ReadFile( "default.cfg", NULL, NULL ) <= 0 ) {
		common->FatalError( "Couldn't load default.cfg" );
	}
}

/*
================
aRcFileSystemLocal::Restart
================
*/
void aRcFileSystemLocal::Restart() {
	// free anything we currently have loaded
	Shutdown( true );

	Startup();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( ReadFile( "default.cfg", NULL, NULL ) <= 0 ) {
		common->FatalError( "Couldn't load default.cfg" );
	}
}

/*
================
aRcFileSystemLocal::Shutdown

Frees all resources and closes all files
================
*/
void aRcFileSystemLocal::Shutdown( bool reloading ) {
	gameFolder.Clear();
	searchPaths.Clear();

	resourceFiles.DeleteContents();


	cmdSystem->RemoveCommand( "path" );
	cmdSystem->RemoveCommand( "dir" );
	cmdSystem->RemoveCommand( "dirtree" );
	cmdSystem->RemoveCommand( "touchFile" );
}

/*
================
aRcFileSystemLocal::IsInitialized
================
*/
bool aRcFileSystemLocal::IsInitialized() const {
	return ( searchPaths.Num() != 0 );
}


/*
=================================================================================

Opening files

=================================================================================
*/

/*
========================
aRcFileSystemLocal::GetResourceCacheEntry

Returns false if the entry isn't found
========================
*/
bool aRcFileSystemLocal::GetResourceCacheEntry( const char *fileName, aRcResCacheEntries &rc ) {
	aRcStaticString< MAX_OSPATH > canonical;
	if ( strstr( fileName, ":" ) != NULL ) {
		// os path, convert to relative? scripts can pass in an OS path
		//arcLibrary::Printf( "RESOURCE: os path passed %s\n", fileName );
		return NULL;
	} else {
		canonical = fileName;
	}

	canonical.BackSlashesToSlashes();
	canonical.ToLower();
	int idx = resourceFiles.Num() - 1;
	while ( idx >= 0 ) {
		const int key = resourceFiles[ idx ]->cacheHash.GenerateKey( canonical, false );
		for ( int index = resourceFiles[ idx ]->cacheHash.GetFirst( key ); index != ARCHashIndex::NULL_INDEX; index = resourceFiles[ idx ]->cacheHash.GetNext( index ) ) {
			aRcResCacheEntries & rt = resourceFiles[ idx ]->cacheTable[index];
			if ( arcNetString::Icmp( rt.filename, canonical ) == 0 ) {
				rc.filename = rt.filename;
				rc.length = rt.length;
				rc.containerIndex = idx;
				rc.offset = rt.offset;
				return true;
			}
		}
		idx--;
	}
	return false;
}

/*
========================
aRcFileSystemLocal::GetResourceFile

Returns NULL
========================
*/

arcNetFile * aRcFileSystemLocal::GetResourceFile( const char *fileName, bool memFile ) {

	if ( resourceFiles.Num() == 0 ) {
		return NULL;
	}

	static aRcResCacheEntries rc;
	if ( GetResourceCacheEntry( fileName, rc ) ) {
		if ( fs_debugResources.GetBool() ) {
			arcLibrary::Printf( "RES: loading file %s\n", rc.filename.c_str() );
		}
		arcFile_InnerResource *file = new arcFile_InnerResource( rc.filename, resourceFiles[ rc.containerIndex ]->resourceFile, rc.offset, rc.length );
		if ( file != NULL && ( memFile || rc.length <= resourceBufferAvailable ) || rc.length < 8 * 1024 * 1024 ) {
			byte *buf = NULL;
			if ( rc.length < resourceBufferAvailable ) {
				buf = resourceBufferPtr;
				resourceBufferAvailable = 0;
			} else {
		if ( fs_debugResources.GetBool() ) {
				arcLibrary::Printf( "MEM: Allocating %05d bytes for a resource load\n", rc.length );
}
				buf = ( byte * )Mem_Alloc( rc.length, TAG_TEMP );
			}
			file->Read( (void*)buf, rc.length );

			if ( buf == resourceBufferPtr ) {
				file->SetResourceBuffer( buf );
				return file;
			} else {
				aRcFileMemory *mfile = new aRcFileMemory( rc.filename, ( const char * )buf, rc.length );
				if ( mfile != NULL ) {
					mfile->TakeDataOwnership();
					delete file;
					return mfile;
				}
			}
		}
		return file;
	}

	return NULL;
}


/*
===========
aRcFileSystemLocal::OpenFileReadFlags

Finds the file in the search path, following search flag recommendations
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
arcNetFile *aRcFileSystemLocal::OpenFileReadFlags( const char *relativePath, int searchFlags, bool allowCopyFiles, const char* gamedir ) {

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
		return NULL;
	}

	if ( relativePath == NULL ) {
		common->FatalError( "aRcFileSystemLocal::OpenFileRead: NULL 'relativePath' parameter passed\n" );
		return NULL;
	}

	// qpaths are not supposed to have a leading slash
	if ( relativePath[0] == '/' || relativePath[0] == '\\' ) {
		relativePath++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo"
	if ( strstr( relativePath, ".." ) || strstr( relativePath, "::" ) ) {
		return NULL;
	}

	// edge case
	if ( relativePath[0] == '\0' ) {
		return NULL;
	}

	if ( fs_debug.GetBool() ) {
		arcLibrary::Printf( "FILE DEBUG: opening %s\n", relativePath );
	}

	if ( resourceFiles.Num() > 0 && fs_resourceLoadPriority.GetInteger() ==  1 ) {
		arcNetFile * rf = GetResourceFile( relativePath, ( searchFlags & FSFLAG_RETURN_FILE_MEM ) != 0 );
		if ( rf != NULL ) {
			return rf;
		}
 	}

	//
	// search through the path, one element at a time
	//
	if ( searchFlags & FSFLAG_SEARCH_DIRS ) {
		for ( int sp = searchPaths.Num() - 1; sp >= 0; sp-- ) {
			if ( gamedir != NULL && gamedir[0] != 0 ) {
				if ( searchPaths[sp].gamedir != gamedir ) {
					continue;
				}
			}

			arcNetString netpath = BuildOSPath( searchPaths[sp].path, searchPaths[sp].gamedir, relativePath );
			arcFileHandle fp = OpenOSFile( netpath, FS_READ );
			if ( !fp ) {
				continue;
			}

			arcFile_Permanent * file = new (TAG_IDFILE) arcFile_Permanent();
			file->o = fp;
			file->name = relativePath;
			file->fullPath = netpath;
			file->mode = ( 1 << FS_READ );
			file->fileSize = DirectFileLength( file->o );
			if ( fs_debug.GetInteger() ) {
				common->Printf( "arcNetFile::OpenFileRead: %s (found in '%s/%s')\n", relativePath, searchPaths[sp].path.c_str(), searchPaths[sp].gamedir.c_str() );
			}

			// if fs_copyfiles is set
			if ( allowCopyFiles ) {

				arcNetString copypath;
				arcNetString name;
				copypath = BuildOSPath( fs_savepath.GetString(), searchPaths[sp].gamedir, relativePath );
				netpath.ExtractFileName( name );
				copypath.StripFilename();
				copypath += PATHSEPARATOR_STR;
				copypath += name;

				if ( fs_buildResources.GetBool() ) {
					aRcStaticString< MAX_OSPATH > relativePath = OSPathToRelativePath( copypath );
					relativePath.BackSlashesToSlashes();
					relativePath.ToLower();

					if ( IsSoundSample( relativePath ) ) {
						aRcStaticString< MAX_OSPATH > samplePath = relativePath;
						samplePath.SetFileExtension( "idwav" );
						if ( samplePath.Find( "generated/" ) == -1 ) {
							samplePath.Insert( "generated/", 0 );
						}
						fileManifest.AddUnique( samplePath );
						if ( relativePath.Find( "/vo/", false ) >= 0 ) {
							// this is vo so add the language variants
							for ( int i = 0; i < Sys_NumLangs(); i++ ) {
								const char *lang = Sys_Lang( i );
								if ( arcNetString::Icmp( lang, ID_LANG_ENGLISH ) == 0 ) {
									continue;
								}
								samplePath = relativePath;
								samplePath.Replace( "/vo/", va( "/vo/%s/", lang ) );
								samplePath.SetFileExtension( "idwav" );
								if ( samplePath.Find( "generated/" ) == -1 ) {
									samplePath.Insert( "generated/", 0 );
								}
								fileManifest.AddUnique( samplePath );

							}
						}
					} else if ( relativePath.Icmpn( "guis/", 5 ) == 0 ) {
						// this is a gui so add the language variants
						for ( int i = 0; i < Sys_NumLangs(); i++ ) {
							const char *lang = Sys_Lang( i );
							if ( arcNetString::Icmp( lang, ID_LANG_ENGLISH ) == 0 ) {
								fileManifest.Append( relativePath );
								continue;
							}
							aRcStaticString< MAX_OSPATH > guiPath = relativePath;
							guiPath.Replace( "guis/", va( "guis/%s/", lang ) );
							fileManifest.Append( guiPath );
						}
					} else {
						// never add .amp files
						if ( strstr( relativePath, ".amp" ) == NULL ) {
							fileManifest.Append( relativePath );
						}
					}

				}

				if ( fs_copyfiles.GetBool() ) {
					CopyFile( netpath, copypath );
				}
			}

			if ( searchFlags & FSFLAG_RETURN_FILE_MEM ) {
				aRcFileMemory * memFile = new (TAG_IDFILE) aRcFileMemory( file->name );
				memFile->SetLength( file->fileSize );
				file->Read( (void *)memFile->GetDataPtr(), file->fileSize );
				delete file;
				memFile->TakeDataOwnership();
				return memFile;
			}

			return file;
		}
	}

	if ( resourceFiles.Num() > 0 && fs_resourceLoadPriority.GetInteger() ==  0 ) {
		arcNetFile * rf = GetResourceFile( relativePath, ( searchFlags & FSFLAG_RETURN_FILE_MEM ) != 0 );
		if ( rf != NULL ) {
			return rf;
		}
	}

	if ( fs_debug.GetInteger( ) ) {
		common->Printf( "Can't find %s\n", relativePath );
	}

	return NULL;
}

/*
===========
aRcFileSystemLocal::OpenFileRead
===========
*/
arcNetFile *aRcFileSystemLocal::OpenFileRead( const char *relativePath, bool allowCopyFiles, const char* gamedir ) {
	return OpenFileReadFlags( relativePath, FSFLAG_SEARCH_DIRS, allowCopyFiles, gamedir );
}

/*
===========
aRcFileSystemLocal::OpenFileReadMemory
===========
*/
arcNetFile *aRcFileSystemLocal::OpenFileReadMemory( const char *relativePath, bool allowCopyFiles, const char* gamedir ) {
	return OpenFileReadFlags( relativePath, FSFLAG_SEARCH_DIRS | FSFLAG_RETURN_FILE_MEM, allowCopyFiles, gamedir );
}

/*
===========
aRcFileSystemLocal::OpenFileWrite
===========
*/
arcNetFile *aRcFileSystemLocal::OpenFileWrite( const char *relativePath, const char *basePath ) {

	const char *path;
	arcNetString OSpath;
	arcFile_Permanent *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}

	OSpath = BuildOSPath( path, gameFolder, relativePath );

	if ( fs_debug.GetInteger() ) {
		common->Printf( "arcNetFile::OpenFileWrite: %s\n", OSpath.c_str() );
	}

	common->DPrintf( "writing to: %s\n", OSpath.c_str() );
	CreateOSPath( OSpath );

	f = new (TAG_IDFILE) arcFile_Permanent();
	f->o = OpenOSFile( OSpath, FS_WRITE );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = relativePath;
	f->fullPath = OSpath;
	f->mode = ( 1 << FS_WRITE );
	f->handleSync = false;
	f->fileSize = 0;

	return f;
}

/*
===========
aRcFileSystemLocal::OpenExplicitFileRead
===========
*/
arcNetFile *aRcFileSystemLocal::OpenExplicitFileRead( const char *OSPath ) {
	arcFile_Permanent *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	if ( fs_debug.GetInteger() ) {
		common->Printf( "arcNetFile::OpenExplicitFileRead: %s\n", OSPath );
	}

	//common->DPrintf( "arcNetFile::OpenExplicitFileRead - reading from: %s\n", OSPath );

	f = new (TAG_IDFILE) arcFile_Permanent();
	f->o = OpenOSFile( OSPath, FS_READ );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_READ );
	f->handleSync = false;
	f->fileSize = DirectFileLength( f->o );

	return f;
}

/*
===========
aRcFileSystemLocal::OpenExplicitPakFile
===========
*/
arcFile_Cached *aRcFileSystemLocal::OpenExplicitPakFile( const char *OSPath ) {
	arcFile_Cached *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	//if ( fs_debug.GetInteger() ) {
	//	common->Printf( "arcNetFile::OpenExplicitFileRead: %s\n", OSPath );
	//}

	//common->DPrintf( "arcNetFile::OpenExplicitFileRead - reading from: %s\n", OSPath );

	f = new (TAG_IDFILE) arcFile_Cached();
	f->o = OpenOSFile( OSPath, FS_READ );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_READ );
	f->handleSync = false;
	f->fileSize = DirectFileLength( f->o );

	return f;
}

/*
===========
aRcFileSystemLocal::OpenExplicitFileWrite
===========
*/
arcNetFile *aRcFileSystemLocal::OpenExplicitFileWrite( const char *OSPath ) {
	arcFile_Permanent *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	if ( fs_debug.GetInteger() ) {
		common->Printf( "arcNetFile::OpenExplicitFileWrite: %s\n", OSPath );
	}

	common->DPrintf( "writing to: %s\n", OSPath );
	CreateOSPath( OSPath );

	f = new (TAG_IDFILE) arcFile_Permanent();
	f->o = OpenOSFile( OSPath, FS_WRITE );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_WRITE );
	f->handleSync = false;
	f->fileSize = 0;

	return f;
}

/*
===========
aRcFileSystemLocal::OpenFileAppend
===========
*/
arcNetFile *aRcFileSystemLocal::OpenFileAppend( const char *relativePath, bool sync, const char *basePath ) {

	const char *path;
	arcNetString OSpath;
	arcFile_Permanent *f;

	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}

	path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}

	OSpath = BuildOSPath( path, gameFolder, relativePath );
	CreateOSPath( OSpath );

	if ( fs_debug.GetInteger() ) {
		common->Printf( "arcNetFile::OpenFileAppend: %s\n", OSpath.c_str() );
	}

	f = new (TAG_IDFILE) arcFile_Permanent();
	f->o = OpenOSFile( OSpath, FS_APPEND );
	if ( !f->o ) {
		delete f;
		return NULL;
	}
	f->name = relativePath;
	f->fullPath = OSpath;
	f->mode = ( 1 << FS_WRITE ) + ( 1 << FS_APPEND );
	f->handleSync = sync;
	f->fileSize = DirectFileLength( f->o );

	return f;
}

/*
================
aRcFileSystemLocal::OpenFileByMode
================
*/
arcNetFile *aRcFileSystemLocal::OpenFileByMode( const char *relativePath, fsMode_t mode ) {
	if ( mode == FS_READ ) {
		return OpenFileRead( relativePath );
	}
	if ( mode == FS_WRITE ) {
		return OpenFileWrite( relativePath );
	}
	if ( mode == FS_APPEND ) {
		return OpenFileAppend( relativePath, true );
	}
	common->FatalError( "aRcFileSystemLocal::OpenFileByMode: bad mode" );
	return NULL;
}

/*
==============
aRcFileSystemLocal::CloseFile
==============
*/
void aRcFileSystemLocal::CloseFile( arcNetFile *f ) {
	if ( !IsInitialized() ) {
		common->FatalError( "Filesystem call made without initialization\n" );
	}
	delete f;
}

/*
=================
aRcFileSystemLocal::FindDLL
=================
*/
void aRcFileSystemLocal::FindDLL( const char *name, char _dllPath[ MAX_OSPATH ] ) {
	char dllName[MAX_OSPATH];
	sys->DLL_GetFileName( name, dllName, MAX_OSPATH );

	// from executable directory first - this is handy for developement
	arcNetString dllPath = Sys_EXEPath( );
	dllPath.StripFilename( );
	dllPath.AppendPath( dllName );
	arcNetFile * dllFile = OpenExplicitFileRead( dllPath );

	if ( dllFile ) {
		dllPath = dllFile->GetFullPath();
		CloseFile( dllFile );
		dllFile = NULL;
	} else {
		dllPath = "";
	}
	arcNetString::snPrintf( _dllPath, MAX_OSPATH, dllPath.c_str() );
}

/*
===============
aRcFileSystemLocal::FindFile
===============
*/
 findFile_t aRcFileSystemLocal::FindFile( const char *path ) {
	arcNetFile *f = OpenFileReadFlags( path, FSFLAG_SEARCH_DIRS );
	if ( f == NULL ) {
		return FIND_NO;
	}
	delete f;
	return FIND_YES;
}

/*
===============
aRcFileSystemLocal::IsFolder
===============
*/
sysFolder_t aRcFileSystemLocal::IsFolder( const char * relativePath, const char *basePath ) {
	return Sys_IsFolder( RelativePathToOSPath( relativePath, basePath ) );
}
