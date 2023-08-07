#include "../idlib/Lib.h"
#include "FileSystem.h"
#pragma hdrstop

#include "Unzip.h"
/*
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

#if ARC_ENABLE_CURL
	#include "../curl/include/curl/curl.h"
#endif
*/
/*
=============================================================================

FILESYSTEM

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

The "cd path" is the path to an alternate hierarchy that will be searched if a file
is not located in the base path. A user can do a partial install that copies some
data to a base path created on their hard drive and leave the rest on the cd. It defaults
to the current directory, but it can be overridden with "+set fs_cdpath g:\doom" on the
command line.

The "dev path" is the path to an alternate hierarchy where the editors and tools used
during development (Radiant, AF editor, dmap, runAAS) will write files to. It defaults to
the cd path, but can be overridden with a "+set fs_devpath c:\doom" on the command line.

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

To save disk space and speed up file loading, directory trees can be collapsed into zip
files. The files use a ".pk5" extension to prevent users from unzipping them accidentally,
but otherwise they are simply normal zip files. A game directory can have multiple zip
files of the form "pak0.pk5", "pak1.pk5", etc. Zip files are searched in decending order
from the highest number to the lowest, and will always take precedence over the filesystem.
This allows a pk5 distributed as a patch to override all existing data.

Because we will have updated executables freely available online, there is no point to
trying to restrict demo / oem versions of the game with code changes. Demo / oem versions
should be exactly the same executables as release versions, but with different data that
automatically restricts where game media can come from to prevent add-ons from working.

After the paths are initialized, Doom will look for the product.txt file. If not found
and verified, the game will run in restricted mode. In restricted mode, only files
contained in demo/pak0.pk5 will be available for loading, and only if the zip header is
verified to not have been modified. A single exception is made for DoomConfig.cfg. Files
can still be written out in restricted mode, so screenshots and demos are allowed.
Restricted mode can be tested by setting "+set fs_restrict 1" on the command line, even
if there is a valid product.txt under the basepath or cdpath.

If the "fs_copyfiles" cvar is set to 1, then every time a file is sourced from the cd
path, it will be copied over to the save path. This is a development aid to help build
test releases and to copy working sets of files.

If the "fs_copyfiles" cvar is set to 2, any file found in fs_cdpath that is newer than
it's fs_savepath version will be copied to fs_savepath (in addition to the fs_copyfiles 1
behaviour).

If the "fs_copyfiles" cvar is set to 3, files from both basepath and cdpath will be copied
over to the save path. This is useful when copying working sets of files mainly from base
path with an additional cd path (which can be a slower network drive for instance).

If the "fs_copyfiles" cvar is set to 4, files that exist in the cd path but NOT the base path
will be copied to the save path

NOTE: fs_copyfiles and case sensitivity. On fs_caseSensitiveOS 0 filesystems ( win32 ), the
copied files may change casing when copied over.

The relative path "sound/newstuff/test.wav" would be searched for in the following places:

for save path, dev path, base path, cd path:
	for current game, base game:
		search directory
		search zip files

downloaded files, to be written to save path + current game's directory

The filesystem can be safely shutdown and reinitialized with different
basedir / cddir / game combinations, but all other subsystems that rely on it
( sound, video) must also be forced to restart.


"fs_caseSensitiveOS":
This cvar is set on operating systems that use case sensitive filesystems (Linux and OSX)
It is a common situation to have the media reference filenames, whereas the file on disc 
only matches in a case-insensitive way. When "fs_caseSensitiveOS" is set, the filesystem
will always do a case insensitive search.
IMPORTANT: This only applies to files, and not to directories. There is no case-insensitive
matching of directories. All directory names should be lowercase, when "com_developer" is 1,
the filesystem will warn when it catches bad directory situations (regardless of the
"fs_caseSensitiveOS" setting)
When bad casing in directories happen and "fs_caseSensitiveOS" is set, BuildOSPath will
attempt to correct the situation by forcing the path to lowercase. This assumes the media
is stored all lowercase.

"additional mod path search":
fs_game_base can be used to set an additional search path
in search order, fs_game, fs_game_base, BASEGAME
for instance to base a mod of D3 + D3XP assets, fs_game mymod, fs_game_base d3xp

=============================================================================
*/

// define to fix special-cases for GetPackStatus so that files that shipped in 
// the wrong place for Doom 3 don't break pure servers.
#define _PURE_SPECIAL_CASES	

typedef bool (*pureExclusionFunc_t)( const struct pureExclusion_s &excl, int l, const anStr &name );

typedef struct pureExclusion_s {
	int					nameLen;
	int					extLen;
	const char *		name;
	const char *		ext;
	pureExclusionFunc_t	func;
} pureExclusion_t;

bool excludeExtension( const pureExclusion_t &excl, int l, const anStr &name ) {
	if ( l > excl.extLen && !anStr::Icmp( name.c_str() + l - excl.extLen, excl.ext ) ) {
		return true;
	}
	return false;
}

bool excludePathPrefixAndExtension( const pureExclusion_t &excl, int l, const anStr &name ) {
	if ( l > excl.nameLen && !anStr::Icmp( name.c_str() + l - excl.extLen, excl.ext ) && !name.IcmpPrefixPath( excl.name ) ) {
		return true;
	}
	return false;
}

bool excludeFullName( const pureExclusion_t &excl, int l, const anStr &name ) {
	if ( l == excl.nameLen && !name.Icmp( excl.name ) ) {
		return true;
	}
	return false;
}

static pureExclusion_t pureExclusions[] = {
	{ 0,	0,	nullptr,											"/",		excludeExtension },
	{ 0,	0,	nullptr,											"\\",		excludeExtension },
	{ 0,	0,	nullptr,											".gui",		excludeExtension },
	{ 0,	0,	nullptr,											".pd",		excludeExtension },
	{ 0,	0,	nullptr,											".lang",	excludeExtension },
	{ 0,	0,	"sound/VO",										".ogg",		excludePathPrefixAndExtension },
	{ 0,	0,	"sound/VO",										".wav",		excludePathPrefixAndExtension },
#if	defined _PURE_SPECIAL_CASES	
	// add any special-case files or paths for pure servers here
	{ 0,	0,	"sound/intro/intro_unsa_cutscene.ogg",			nullptr,		excludeFullName },
	{ 0,	0,	"sound/feedback",								".ogg",		excludePathPrefixAndExtension },
	{ 0,	0,	"sound/feedback",								".wav",		excludePathPrefixAndExtension },
	{ 0,	0,	"guis/assets/mainmenu/chnote.tga",				nullptr,		excludeFullName },
	{ 0,	0,	"sound/levels/unsa/better_place.ogg",			nullptr,		excludeFullName },
	{ 0,	0,	"fonts/bigchars.tga",							nullptr,		excludeFullName },
	{ 0,	0,	"dds/bigchars.dds",								nullptr,		excludeFullName },
	{ 0,	0,	"fonts",										".tga",		excludePathPrefixAndExtension },
	{ 0,	0,	"dds/fonts",									".dds",		excludePathPrefixAndExtension },
	{ 0,	0,	"default.cfg",									nullptr,		excludeFullName },
	// russian zpak001.pk5
	{ 0,	0,  "fonts",										".dat",		excludePathPrefixAndExtension },
	{ 0,	0,	"guis/temp.guied",								nullptr,		excludeFullName },
#endif
	{ 0,	0,	nullptr,											nullptr,		nullptr }
};

// ensures that lengths for pure exclusions are correct
class anInitExclusions {
public:
	anInitExclusions() {
		for ( int i = 0; pureExclusions[i].func != nullptr; i++ ) {
			if ( pureExclusions[i].name ) {
				pureExclusions[i].nameLen = anStr::Length( pureExclusions[i].name );
			}
			if ( pureExclusions[i].ext ) {
				pureExclusions[i].extLen = anStr::Length( pureExclusions[i].ext );
			}
		}
	}
};

static anInitExclusions	initExclusions;

#define MAX_ZIPPED_FILE_NAME	2048
#define FILE_HASH_SIZE			1024

typedef struct fileInPack_s {
	anStr			name;						// name of the file
	unsigned long		pos;						// file info position in zip
	struct fileInPack_s * next;						// next file in the hash
} fileInPack_t;

typedef enum {
	BINARY_UNKNOWN = 0,
	BINARY_YES,
	BINARY_NO
} binaryStatus_t;

typedef enum {
	PURE_UNKNOWN = 0,	// need to run the pak through GetPackStatus
	PURE_NEUTRAL,	// neutral regarding pureness. gets in the pure list if referenced
	PURE_ALWAYS,	// always referenced - for pak* named files, unless NEVER
	PURE_NEVER		// VO paks. may be referenced, won't be in the pure lists
} pureStatus_t;

typedef struct {
	anList<int>			depends;
	anList<anDict *>	mapDecls;
} addonInfo_t;

typedef struct {
	anStr			pakFileName;				// c:\doom\base\pak0.pk5
	anPaKFile			handle;
	int					checksum;
	int					numFiles;
	int					length;
	bool				referenced;
	binaryStatus_t		pakFileStatus;
	bool				addon;						// this is an addon pack - pakAddonSearch tells if it's 'active'
	bool				pakAddonSearch;				// is in the search list
	addonInfo_t *		pakInfo;
	pureStatus_t		pureStatus;
	bool				isNew;						// for downloaded paks
	fileInPack_t *		hashPaKTable[FILE_HASH_SIZE];
	fileInPack_t *		buildBuffer;
} pK5Nb_t;

typedef struct {
	anStr			path;						// c:\doom
	anStr			gamedir;					// base
} directory_t;

typedef struct searchpath_s {
	pK5Nb_t *			pack;						// only one of pack / dir will be non nullptr
	directory_t *		dir;
	struct searchpath_s *next;
} searchPath_t;

typedef struct {
	anStringList		path;
	anStr			OSPath;
} caseMatch_t;

// search flags when opening a file
#define FSFLAG_SEARCH_DIRS		( 1 << 0 )
#define FSFLAG_SEARCH_PAKS		( 1 << 1 )
#define FSFLAG_PURE_NOREF		( 1 << 2 )
#define FSFLAG_BINARY_ONLY		( 1 << 3 )
#define FSFLAG_SEARCH_ADDONS	( 1 << 4 )

// 3 search path (fs_savepath fs_basepath fs_cdpath)
// + .jpg and .tga
#define MAX_CACHED_DIRS			6

// how many OSes to handle game paks for ( we don't have to know them precisely )
#define MAX_GAME_OS				6
#define BINARY_CONFIG			"pakFileStatus.conf"
#define ADDON_CONFIG			"addon.conf"

class anDEntry : public anStringList {
public:
						anDEntry() {}
	virtual				~anDEntry() {}

	bool				Matches( const char *directory, const char *extension ) const;
	void				Init( const char *directory, const char *extension, const anStringList &list );
	void				Clear( void );

private:
	anStr				directory;
	anStr				extension;
};

anCVar	anFileSystem::fs_restrict( "fs_restrict", "", CVAR_SYSTEM | CVAR_INIT | CVAR_BOOL, "" );
anCVar	anFileSystem::fs_debug( "fs_debug", "0", CVAR_SYSTEM | CVAR_INTEGER, "", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
anCVar	anFileSystem::fs_copyfiles( "fs_copyfiles", "0", CVAR_SYSTEM | CVAR_INIT | CVAR_INTEGER, "", 0, 4, idCmdSystem::ArgCompletion_Integer<0,3> );
anCVar	anFileSystem::fs_basepath( "fs_basepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
anCVar	anFileSystem::fs_savepath( "fs_savepath", "", CVAR_SYSTEM | CVAR_INIT, "" );
anCVar	anFileSystem::fs_cdpath( "fs_cdpath", "", CVAR_SYSTEM | CVAR_INIT, "" );
anCVar	anFileSystem::fs_devpath( "fs_devpath", "", CVAR_SYSTEM | CVAR_INIT, "" );
anCVar	anFileSystem::fs_game( "fs_game", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "mod path" );
anCVar  anFileSystem::fs_game_base( "fs_game_base", "", CVAR_SYSTEM | CVAR_INIT | CVAR_SERVERINFO, "alternate mod path, searched after the main fs_game path, before the basedir" );
#ifdef WIN32
anCVar	anFileSystem::fs_caseSensitiveOS( "fs_caseSensitiveOS", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
#else
anCVar	anFileSystem::fs_caseSensitiveOS( "fs_caseSensitiveOS", "1", CVAR_SYSTEM | CVAR_BOOL, "" );
#endif
anCVar	anFileSystem::fs_searchAddons( "fs_searchAddons", "0", CVAR_SYSTEM | CVAR_BOOL, "search all addon pk5s ( disables addon functionality )" );

anFileSystem	fileSystem;
anFileSystem *		fileSystem = &fileSystem;

/*
================
anFileSystem::anFileSystem
================
*/
anFileSystem::anFileSystem( void ) {
	searchPaths = nullptr;
	readCount = 0;
	loadCount = 0;
	loadStack = 0;
	dir_cache_index = 0;
	dir_cache_count = 0;
	d3xp = 0;
	loadedFileFromDir = false;
	restartGamePakChecksum = 0;
	memset( &backgroundThread, 0, sizeof( backgroundThread ) );
	addonPaks = nullptr;
}

/*
================
anFileSystem::HashFileName

return a hash value for the filename
================
*/
long anFileSystem::HashFileName( const char *fName ) const {
	char letter;
	int hash = 0, i = 0;
	while ( fName[i] != '\0' ) {
		char letter = anStr::ToLower( fName[i] );
		if ( letter == '.' ) {
			break;				// don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';		// damn path names
		}
		hash += (long)( letter ) * ( i+119 );
		i++;
	}
	hash &= ( FILE_HASH_SIZE-1 );
	return hash;
}

/*
===========
anFileSystem::CompareFileName

Ignore case and separator char distinctions
===========
*/
bool anFileSystem::CompareFileName( const char *s1, const char *s2 ) const {
	do {
		int c1 = *s1++;
		int c2 = *s2++;
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
=================
FS_GetZipChecksum

Compares whether the given pak file matches a referenced checksum
=================
*/
bool PaK_CompareFileChecksum( const char *file ) {
	int checksum;

	pk4 = FS_LoadZipFile( file, "" );

	if ( !pk4 ) {
		return false;
	}

	checksum = pk4->enginePakChecksum;
	FS_FreePak( pk4 );

	for ( int index = 0; index < enginePakChecksum; index++ ) {
		if ( fs_usePkB == fs_serverReferencedPaks[index] ) {
			return true;
		}
	}

	return false;
}

/*
================
anFileSystemLocal::IsBinaryModel
================
*/
bool anFileSystem::IsBinaryModel( const anStr &resName ) const {
	anStrStatic< 32 > ext;
	resName.ExtractFileExtension( ext );
	if ( ( ext.Icmp( "base" ) == 0 ) || ( ext.Icmp( "blwo" ) == 0 ) || ( ext.Icmp( "bflt" ) == 0 ) || ( ext.Icmp( "bma" ) == 0 ) ) {
		return true;
	}
	return false;
}

/*
========================
anFileSystemLocal::GetFileLength
========================
*/
int anFileSystem::GetFileLength( const char *relativePath ) {
	anFile *	f;
	int			len;

	if ( !IsInitialized() ) {
		idLib::FatalError( "Filesystem call made without initialization" );
	}

	if ( !relativePath || !relativePath[0] ) {
		idLib::Warning( "anFileSystemLocal::GetFileLength with empty name" );
		return -1;
	}

	if ( resourceFiles.Num() > 0 ) {
		idResourceCacheEntry rc;
		if ( GetResourceCacheEntry( relativePath, rc ) ) {
			return rc.length;
		}
	}

	// look for it in the filesystem or pack files
	f = OpenFileRead( relativePath, false );
	if ( f == nullptr ) {
		return -1;
	}

	len = (int)f->Length();

	delete f;
	return len;
}

/*
================
anFileSystem::OpenOSFile

optional caseSensitiveName is set to case sensitive file name as found on disc (fs_caseSensitiveOS only)
================
*/
FILE *anFileSystem::OpenOSFile( const char *fileName, const char *mode, anStr *caseSensitiveName ) {
	FILE* fp = nullptr;
	anStr fpath, entry;// Declare anStr variables fpath and entry
	anStringList list;

	if ( !fs_caseSensitiveOS.GetBool() ) {// Check if fs_caseSensitiveOS is false
		fp = fopen( fileName, mode );
		if ( fp == nullptr ) { // Check if the file failed to open
			fpath = fileName;  // Set fpath to the file name
			// Strip the file name from fpath
			fpath.StripFilename();
			// Strip the trailing path separator from fpath
			fpath.StripTrailing( PATHSEPERATOR_CHAR );
			// Call ListOSFiles to get a list of files in the directory
			if ( ListOSFiles( fpath, nullptr, list ) == -1 ) {
				// if there is an error, return an error
				common->Warning( "[FileSystem] OpenOSFile: ListOSFiles could not open %s", fileName );
				// Return nullptr if ListOSFiles fails
				return nullptr;
			}
			// Iterate over the files in the list
			for ( int i = 0; i < list.Num(); i++ ) {
				// Create the full file path by concatenating fpath, PATHSEPERATOR_CHAR, and the current file name
				entry = fpath + PATHSEPERATOR_CHAR + list[i];
				// Compare the entry with the file name
				if ( !entry.Icmp( fileName ) ) {
					// Open the file with the entry as the file name
					fp = fopen( entry, mode );
					// if we are sucsessful opening the file, then we check if our case sensitive name it is not nullptr
					if ( fp ) {
						if ( caseSensitiveName ) {
							*caseSensitiveName = entry;
							caseSensitiveName->StripPath();
						}
						// Check if fs_debug is non-zero
						if ( fs_debug.GetInteger() ) {
								common->Printf( "anFileSystem::OpenFileRead: changed %s to %s\n", fileName, entry.c_str() );
						}
						break;
					} else {
						// not supposed to happen if ListOSFiles is doing it's job correctly
						common->Warning( "anFileSystem::OpenFileRead: fs_caseSensitiveOS 1 could not open %s", entry.c_str() );
					}
				}
			}
		}
	} else {
		// Declare a struct stat variable buf
		struct stat buf;
		// Check if the file is not a regular file
		if ( stat( fileName, &buf ) != -1 && !S_ISREG( buf.st_mode ) ) {
			return nullptr;
		}
		// Open the file with the given file name and mode
		fp = fopen( fileName, mode );
	}
	// repeat/reread the above casesense if statment stage description above ^^^^
	// i know i could copyed and pasted it down here too. =)
	if ( caseSensitiveName ) {
		*caseSensitiveName = fileName;
		caseSensitiveName->StripPath();
	}
	return fp;
}

/*
================
anFileSystem::OpenOSFileCorrectName
================
*/
FILE *anFileSystem::OpenOSFileCorrectName( anStr &path, const char *mode ) {
	anStr caseName;
	FILE *f = OpenOSFile( path.c_str(), mode, &caseName );
	if ( f ) {
		path.StripFilename();
		path += PATHSEPERATOR_STR;
		path += caseName;
	}
	return f;
}

/*
================
anFileSystem::DirectFileLength
================
*/
int anFileSystem::DirectFileLength( FILE *o ) {
	int		pos;
	int		end;

	pos = ftell( o );
	fseek( o, 0, SEEK_END );
	end = ftell( o );
	fseek( o, pos, SEEK_SET );
	return end;
}

/*
============
anFileSystem::GetBasePath

Returns a full OS path where the initial base of the file system exists
============
*/
const char *anFileSystem::GetBasePath() const {
	return fs_game_base.GetString();
}

/*
============
anFileSystem::GetSavePath

Returns a full OS path where files can be written to
============
*/
const char *anFileSystem::GetSavePath() const {
	return fs_savepath.GetString();
}

/*
============
anFileSystem::GetUserPath

Returns a full OS path where user visible files can be written to
============
*/
const char *anFileSystem::GetUserPath() const {
	return fs_basepath.GetString();
}

/*
============
anFileSystem::GetGamePath

Returns the current game path
============
*/
const char *anFileSystem::GetGamePath() const {
	return fs_game.GetString();
}

/*static anFile *Create( FILE* fp, bool readable, bool writeable, const char *fpath, const char *name ) {
	idfile_override_t* overr = (cs_idfile_override_t *)idMemorySystem_malloc( sizeof( cs_idfile_override_t ), 52, 0 );
	overr->vftbl = &g_override_file_vftbl;
	overr->m_cfile = fp;
	overr->m_readable=readable;
	overr->m_writable = writeable;
	overr->m_fullpath = fpath;
	overr->m_fname = name;
	return overr;
}*/
/*
============
anFileSystem::CreateOSPath

Creates any directories needed to store the given filename
============
*/
void anFileSystem::CreateOSPath( const char *OSPath ) {
	char	*ofs;

	// make absolutely sure that it can't back up the path
	// FIXME: what about c: ?
	if ( strstr( OSPath, ".." ) || strstr( OSPath, "::" ) ) {
#ifdef _DEBUG		
		common->DPrintf( "refusing to create relative path \"%s\"\n", OSPath );
#endif
		return;
	}
	anStrStatic<MAX_OSPATH> path( OSPath );
	path.SlashesToBackSlashes();
	//anStr path( OSPath );
	for ( ofs = &path[1]; *ofs ; ofs++ ) {
		if ( *ofs == PATHSEPERATOR_CHAR ) {	
			// create the directory
			*ofs = 0;
			Sys_Mkdir( path );
			*ofs = PATHSEPERATOR_CHAR;
		}
	}
}

/*
=================
anFileSystem::CopyFile

Copy a fully specified file from one place to another
=================
*/
void anFileSystem::CopyFile( const char *fromOSPath, const char *toOSPath ) {
	FILE	*f;
	int		len;
	byte	*buf;

	common->Printf( "copy %s to %s\n", fromOSPath, toOSPath );
	f = OpenOSFile( fromOSPath, "rb" );
	if ( !f ) {
		return;
	}
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fseek( f, 0, SEEK_SET );

	buf = (byte *)Mem_Alloc( len );
	if ( fread( buf, 1, len, f ) != (unsigned int)len ) {
		common->FatalError( "short read in anFileSystem::CopyFile()\n" );
	}
	fclose( f );

	CreateOSPath( toOSPath );
	f = OpenOSFile( toOSPath, "wb" );
	if ( !f ) {
		common->Printf( "could not create destination file\n" );
		Mem_Free( buf );
		return;
	}
	if ( fwrite( buf, 1, len, f ) != (unsigned int)len ) {
		common->FatalError( "short write in anFileSystem::CopyFile()\n" );
	}
	fclose( f );
	Mem_Free( buf );
}

/*
=================
anFileSystem::CopyFile
=================
*/
void anFileSystem::CopyFile( const anFile *src, const char *toOSPath ) {
	FILE	*f;
	int		len;
	byte	*buf;

	common->Printf( "copy %s to %s\n", src->GetName(), toOSPath );
	src->Seek( 0, FS_SEEK_END );
	len = src->Tell();
	src->Seek( 0, FS_SEEK_SET );

	buf = (byte *)Mem_Alloc( len );
	if ( src->Read( buf, len ) != len ) {
		common->FatalError( "Short read in anFileSystem::CopyFile()\n" );
	}

	CreateOSPath( toOSPath );
	f = OpenOSFile( toOSPath, "wb" );
	if ( !f ) {
		common->Printf( "could not create destination file\n" );
		Mem_Free( buf );
		return;
	}
	if ( fwrite( buf, 1, len, f ) != (unsigned int)len ) {
		common->FatalError( "Short write in anFileSystem::CopyFile()\n" );
	}
	fclose( f );
	Mem_Free( buf );
}
/*
=================
FS_CheckFilenameIsMutable

if trying to maniuplate a file with the platform library, QVM, or pk3, pk4, or pk5 extension as well as zip extensions
=================
 */
static void anFileSystem::CheckFilenameIsMutable( const char *filename, const char *function ) {
	// Check if the filename ends with the library extension, or pk3/pk4 extension
	if ( Sys_DllExtension( filename ) || common->CompareExtension( filename, ".pk4" )) || common->CompareExtension( filename, ".pk3" || || common->CompareExtension( filename, ".PaKb" ) ) {
		common->Error( "%s: Not allowed to manipulate '%s' due "
			"to %s extension", function, filename, common->GetExtension( filename ) );
	}
}

/*
====================
anFileSystem::ReplaceSeparators

Fix things up differently for win/unix/mac
====================
*/
void anFileSystem::ReplaceSeparators( anStr &path, char sep ) {
	for ( char *s = &path[0]; *s ; s++ ) {
		if ( *s == '/' || *s == '\\' ) {
			*s = sep;
		}
	}
}

/*
===================
anFileSystem::BuildOSPath
===================
*/
const char *anFileSystem::BuildOSPath( const char *base, const char *game, const char *relativePath ) {
	static char OSPath[MAX_STRING_CHARS];
	anStr newPath;

	if ( fs_caseSensitiveOS.GetBool() || com_developer.GetBool() ) {
		// extract the path, make sure it's all lowercase
		anStr testPath, fileName;

		sprintf( testPath, "%s/%s", game , relativePath );
		testPath.StripFilename();
		if ( testPath.HasUpper() ) {
			common->Warning( "Non-portable: path contains uppercase characters: %s", testPath.c_str() );
			// attempt a fixup on the fly
			if ( fs_caseSensitiveOS.GetBool() ) {
				testPath.ToLower();
				fileName = relativePath;
				fileName.StripPath();
				sprintf( newPath, "%s/%s/%s", base, testPath.c_str(), fileName.c_str() );
				ReplaceSeparators( newPath );
				common->DPrintf( "Fixed up to %s\n", newPath.c_str() );
				anStr::Copynz( OSPath, newPath, sizeof( OSPath ) );
				return OSPath;
			}
		}
	}

	anStr strBase = base;
	strBase.StripTrailing( '/' );
	strBase.StripTrailing( '\\' );
	sprintf( newPath, "%s/%s/%s", strBase.c_str(), game, relativePath );
	ReplaceSeparators( newPath );
	anStr::Copynz( OSPath, newPath, sizeof( OSPath ) );
	return OSPath;
}

/*
================
anFileSystem::OSPathToRelativePath

takes a full OS path, as might be found in data from a media creation
program, and converts it to a relativePath by stripping off directories

Returns false if the osPath tree doesn't match any of the existing
search paths.

================
*/
const char *anFileSystem::OSPathToRelativePath( const char *OSPath ) {
	static char relativePath[MAX_STRING_CHARS];
	char *s, *base;

	// skip a drive letter?

	// search for anything with "base" in it
	// Ase files from max may have the form of:
	// "//Purgatory/purgatory/doom/base/models/mapobjects/bitch/hologirl.tga"
	// which won't match any of our drive letter based search paths
	bool ignoreWarning = false;
#ifdef ARCNET_DEMO_BUILD
	base = strstr( OSPath, BASE_GAMEDIR );	
	anStr tempStr = OSPath;
	tempStr.ToLower();
	if ( ( strstr( tempStr, "//" ) || strstr( tempStr, "w:" ) ) && strstr( tempStr, "/doom/base/" ) ) {
		// will cause a warning but will load the file. ase models have
		// hard coded doom/base/ in the material names
		base = strstr( OSPath, "base" );
		ignoreWarning = true;
	}
#else
	// look for the first complete directory name
	base = (char *)strstr( OSPath, BASE_GAMEDIR );
	while ( base ) {
		char c1 = '\0', c2;
		if ( base > OSPath ) {
			c1 = *(base - 1);
		}
		c2 = *( base + strlen( BASE_GAMEDIR ) );
		if ( ( c1 == '/' || c1 == '\\' ) && ( c2 == '/' || c2 == '\\' ) ) {
			break;
		}
		base = strstr( base + 1, BASE_GAMEDIR );
	}
#endif
	// fs_game and fs_game_base support - look for first complete name with a mod path
	// ( fs_game searched before fs_game_base )
	const char *fsgame = nullptr;
	int igame = 0;
	for ( igame = 0; igame < 2; igame++ ) {
		if ( igame == 0 ) {
			fsgame = fs_game.GetString();
		} else if ( igame == 1 ) {
			fsgame = fs_game_base.GetString();
		}
		if ( base == nullptr && fsgame && strlen( fsgame ) ) {
			base = (char *)strstr( OSPath, fsgame );
			while ( base ) {
				char c1 = '\0', c2;
				if ( base > OSPath ) {
					c1 = *(base - 1);
				}
				c2 = *( base + strlen( fsgame ) );
				if ( ( c1 == '/' || c1 == '\\' ) && ( c2 == '/' || c2 == '\\' ) ) {
					break;
				}
				base = strstr( base + 1, fsgame );
			}
		}
	}

	if ( base ) {
		s = strstr( base, "/" );
		if ( !s ) {
			s = strstr( base, "\\" );
		}
		if ( s ) {
			strcpy( relativePath, s + 1 );
			if ( fs_debug.GetInteger() > 1 ) {
				common->Printf( "anFileSystem::OSPathToRelativePath: %s becomes %s\n", OSPath, relativePath );
			}
			return relativePath;
		}
	}

	if ( !ignoreWarning ) {
		common->Warning( "anFileSystem::OSPathToRelativePath failed on %s", OSPath );
	}
	strcpy( relativePath, "" );
	return relativePath;
}

/*
=====================
anFileSystem::RelativePathToOSPath

Returns a fully qualified path that can be used with stdio libraries
=====================
*/
const char *anFileSystem::RelativePathToOSPath( const char *relativePath, const char *basePath ) {
	const char *path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}
	return BuildOSPath( path, engineFolder, relativePath );
}

/*
=================
anFileSystem::RemoveFile
=================
*/
void anFileSystem::RemoveFile( const char *relativePath ) {
	anStr OSPath;

	if ( fs_devpath.GetString()[0] ) {
		OSPath = BuildOSPath( fs_devpath.GetString(), engineFolder, relativePath );
		remove( OSPath );
	}

	OSPath = BuildOSPath( fs_savepath.GetString(), engineFolder, relativePath );
	remove( OSPath );
	ClearDirCache();
}

/*
========================
anFileSystemLocal::RemoveDir
========================
*/
bool anFileSystemLocal::RemoveDir( const char * relativePath ) {
	bool success = true;
	if ( fs_savepath.GetString()[0] ) {
		success &= Sys_Rmdir( BuildOSPath( fs_savepath.GetString(), relativePath ) );
	}
	success &= Sys_Rmdir( BuildOSPath( fs_basepath.GetString(), relativePath ) );
	return success;
}
/*
=================
anFileSystem::RemoveExplicitDir

Removes the given directory from a full OS path
FIXME:
=================
*/
bool anFileSystem::RemoveExplicitDir( const char *OSPath, bool nonEmpty, bool recursive ) {
	// Check if OSPath is empty
	if ( !OSPath[0] ) {
		return false; 
	}

	// Build full path 
	anStr fullPath = BuildOSPath( fs_savepath.GetString(), engineFolder, OSPath );

	// Check if recursive delete
	if ( !recursive ) {
		// Try to remove directory (will fail if non-empty)
		if ( remove( fullPath.c_str()) == 0 ) {
			return true;
		} else {
			return false;
		}
	} else {
	// Recursively delete directory
	return RemoveDir( fullPath.c_str() );
}

/*
========================
anFileSystemLocal::RenameFile
========================
*/
bool anFileSystemLocal::RenameFile( const char *relativePath, const char *newName, const char *basePath ) {
	const char *path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}

	anStr oldOSPath = BuildOSPath( path, gameFolder, relativePath );
	anStr newOSPath = BuildOSPath( path, gameFolder, newName );

	// this gives atomic-delete-on-rename, like POSIX rename()
	// There is a MoveFileTransacted() on vista and above, not sure if that means there
	// is a race condition inside MoveFileEx...
	const bool success = ( MoveFileEx( oldOSPath.c_str(), newOSPath.c_str(), MOVEFILE_REPLACE_EXISTING ) != 0 );

	if ( !success ) {
		const int err = GetLastError();
		idLib::Warning( "RenameFile( %s, %s ) error %i", newOSPath.c_str(), oldOSPath.c_str(), err );
	}
	return success;
}

/*
================
anFileSystem::FileIsInPAK
================
*/
bool anFileSystem::FileIsInPAK( const char *relativePath ) {
	searchPath_t	*search;
	pK5Nb_t			*pak;
	fileInPack_t	*pakFile;
	long			hash;

	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	if ( !relativePath ) {
		common->FatalError( "anFileSystem::FileIsInPAK: nullptr 'relativePath' parameter passed\n" );
	}

	// qpaths are not supposed to have a leading slash
	if ( relativePath[0] == '/' || relativePath[0] == '\\' ) {
		relativePath++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo" 
	if ( strstr( relativePath, ".." ) || strstr( relativePath, "::" ) ) {
		return false;
	}

	//
	// search through the path, one element at a time
	//

	hash = HashFileName( relativePath );

	for ( search = searchPaths; search; search = search->next ) {
		// is the element a pak file?
		if ( search->pack && search->pack->hashPaKTable[hash] ) {
			// disregard if it doesn't match one of the allowed pure pak files - or is a localization file
			if ( serverPaks.Num() ) {
				GetPackStatus( search->pack );
				if ( search->pack->pureStatus != PURE_NEVER && !serverPaks.Find( search->pack ) ) {
					continue; // not on the pure server pak list
				}
			}

			// look through all the pak file elements
			pak = search->pack;
			pakFile = pak->hashPaKTable[hash];
			do {
				// case and separator insensitive comparisons
				if ( !CompareFileName( pakFile->name, relativePath ) ) {
					return true;
				}
				pakFile = pakFile->next;
			} while( pakFile != nullptr );
		}
	}
	return false;
}

/*
===============
anFileSystemLocal::AddUnique
===============
*/
int anFileSystemLocal::AddUnique( const char *name, anStrList &list, idHashIndex &hashIndex ) const {
	int hashKey = hashIndex.GenerateKey( name );
	for ( int i = hashIndex.First( hashKey ); i >= 0; i = hashIndex.Next( i ) ) {
		if ( list[i].Icmp( name ) == 0 ) {
			return i;
		}
	}
	int i = list.Append( name );
	hashIndex.Add( hashKey, i );
	return i;
}

/*
============
anFileSystem::ReadFile

Filename are relative to the search path
a nullptr buffer will just return the file length and time without loading
timestamp can be nullptr if not required
============
*/
int anFileSystem::ReadFile( const char *relativePath, void **buffer, ARC_TIME_T *timestamp ) {
	anFile *	f;
	byte *		buf;
	int			len;
	bool		isConfig;

	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	if ( !relativePath || !relativePath[0] ) {
		common->FatalError( "[FileSystem] ReadFile: with empty name\n" );
	}

	if ( timestamp ) {
		*timestamp = FILE_NOT_FOUND_TIMESTAMP;
	}

	if ( buffer ) {
		*buffer = nullptr;
	}

	buf = nullptr;	// quiet compiler warning

	// if this is a .cfg file and we are playing back a journal, read
	// it from the journal file
	if ( strstr( relativePath, ".cfg" ) == relativePath + strlen( relativePath ) - 4 ) {
		isConfig = true;
		if ( eventLoop && eventLoop->JournalLevel() == 2 ) {
			loadCount++;
			loadStack++;

			common->DPrintf( "Loading %s from journal file.\n", relativePath );
			len = 0;
			int r = eventLoop->com_journalDataFile->Read( &len, sizeof( len ) );
			if ( r != sizeof( len ) ) {
				*buffer = nullptr;
				return -1;
			}
			buf = (byte *)Mem_ClearedAlloc(len+1);
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
	f = OpenFileRead( relativePath, ( buffer != nullptr ) );
	if ( f == nullptr ) {
		if ( buffer ) {
			*buffer = nullptr;
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

	buf = (byte *)Mem_ClearedAlloc(len+1);
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
anFileSystem::FreeFile
=============
*/
void anFileSystem::FreeFile( void *buffer ) {
	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}
	if ( !buffer ) {
		common->FatalError( "anFileSystem::FreeFile( nullptr )" );
	}
	loadStack--;

	Mem_Free( buffer );
}

/*
==============
FS_Delete

using fs_homepath for the file to remove
==============
*/
int anFileSystem::DeleteFile( const char *name ) const {
	if ( !fs_searchpaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	if ( !name || name[0] == 0 ) {
		return 0;
	}

	// for safety, only allow deletion from the save directory
	if ( anStr::Ncmp( name, "save/", 5 ) != 0 ) {
		return 0;
	}

	const char *osPath = BuildOSPath( fs_homepath->string, fs_gamedir, name );

	if ( Remove( osPath ) != -1 ) { 
		//filename = delete[] filename;
		return 1;
	}

	return 0;
}

/*
============
anFileSystem::WriteFile

Filenames are relative to the search path
============
*/
int anFileSystem::WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath ) {
	anFile *f;

	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	if ( !relativePath || !buffer ) {
		common->FatalError( "anFileSystem::WriteFile: nullptr parameter" );
	}

	f = anFileSystem::OpenFileWrite( relativePath, basePath );
	if ( !f ) {
		common->Printf( "Failed to open %s\n", relativePath );
		return -1;
	}

	size = f->Write( buffer, size );

	CloseFile( f );

	return size;
}

/*
=================
anFileSystem::ReadTGA
=================
*/
void anFileSystem::ReadTGA( const char *name, byte **pic, int *width, int *height, unsigned *timestamp, bool markPaksReferenced ) {
}

/*
=================
anFileSystem::WriteTGA
=================
*/
void anFileSystem::WriteTGA( const char *name, const byte *pic, int width, int height, bool flipVertical ) {
	int bufferSize = width*height*4 + 18;
	int imgStart = 18;

	byte *buffer = (byte *)Mem_Alloc( bufferSize );
	memset( buffer, 0, 18 );
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width&255;
	buffer[13] = width>>8;
	buffer[14] = height&255;
	buffer[15] = height>>8;
	buffer[16] = 32;	// pixel size
	if ( !flipVertical ) {
		buffer[17] = ( 1<<5 );	// flip bit, for normal top to bottom raster order
	}

	// swap rgb to bgr
	for ( int i = imgStart; i < bufferSize; i += 4 ) {
		buffer[i] = pic[i-imgStart+2];		// blue
		buffer[i+1] = pic[i-imgStart+1];		// green
		buffer[i+2] = pic[i-imgStart+0];		// red
		buffer[i+3] = pic[i-imgStart+3];		// alpha
	}

	WriteFile( filename, buffer, bufferSize );
	Mem_Free( buffer );
}

/*
=================
anFileSystem::FreeTGA
=================
*/
void anFileSystem::FreeTGA( byte *pic ) {
}

/*
=================
anFileSystem::ParseAddonDef
=================
*/
addonInfo_t *anFileSystem::ParseAddonDef( const char *buf, const int len ) {
	anLexer		src;
	anToken		token, token2;
	addonInfo_t	*info;

	src.LoadMemory( buf, len, "<addon.conf>" );
	src.SetFlags( DECL_LEXER_FLAGS );
	if ( !src.SkipUntilString( "addonDef" ) ) {
		src.Warning( "ParseAddonDef: no addonDef" );
		return nullptr;
	}
	if ( !src.ReadToken( &token ) ) {
		src.Warning( "Expected {" );
		return nullptr;
	}
	info = new addonInfo_t;
	// read addonDef
	while ( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			delete info;
			return nullptr;
		}
		if ( !token.Icmp( "}" ) ) {
			break;
		}
		if ( token.type != TT_STRING ) {
			src.Warning( "Expected quoted string, but found '%s'", token.c_str() );
			delete info;
			return nullptr;
		}
		int checksum;
		if ( sscanf( token.c_str(), "0x%x", &checksum ) != 1 && sscanf( token.c_str(), "%x", &checksum ) != 1 ) {
			src.Warning( "Could not parse checksum '%s'", token.c_str() );
			delete info;
			return nullptr;
		}
		info->depends.Append( checksum );
	}
	// read any number of mapDef entries
	while ( 1 ) {
		if ( !src.SkipUntilString( "mapDef" ) ) {
			return info;
		}
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Expected map path" );
			info->mapDecls.DeleteContents( true );
			delete info;
			return nullptr;
		}
		anDict *dict = new anDict;
		dict->Set( "path", token.c_str() );
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Expected {" );
			info->mapDecls.DeleteContents( true );
			delete dict;
			delete info;
			return nullptr;
		}
		while ( 1 ) {
			if ( !src.ReadToken( &token ) ) {
				break;
			}
			if ( !token.Icmp( "}" ) ) {
				break;
			}
			if ( token.type != TT_STRING ) {
				src.Warning( "Expected quoted string, but found '%s'", token.c_str() );
				info->mapDecls.DeleteContents( true );
				delete dict;
				delete info;
				return nullptr;
			}

			if ( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				info->mapDecls.DeleteContents( true );
				delete dict;
				delete info;
				return nullptr;
			}

			if ( dict->FindKey( token ) ) {
				src.Warning( "'%s' already defined", token.c_str() );
			}
			dict->Set( token, token2 );
		}
		info->mapDecls.Append( dict );
	}
	assert( false );
	return nullptr;
}

/*
=================
anFileSystem::LoadZipFile
=================
*/
pK5Nb_t *anFileSystem::LoadPaKFile( const char *pakFile ) {
	fileInPack_t *	buildBuffer;
	pK5Nb_t *		pack;
	anPaKFile			uf;
	int				err;
	pakGlobalInfo gi;
	char			filename_inzip[MAX_ZIPPED_FILE_NAME];
	pakFileInfo	fInfo;
	int				i;
	long			hash;
	int				fs_numHeaderLongs;
	int *			fs_headerLongs;
	FILE			*f;
	int				len;
	int				confHash;
	fileInPack_t	*pakFile;

	f = OpenOSFile( zipfile, "rb" );
	if ( !f ) {
		return nullptr;
	}
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fclose( f );

	fs_numHeaderLongs = 0;

	uf = PAK_Open( zipfile );
	err = PAK_GetDescription( uf, &gi );

	if ( err != UNZ_OK ) {
		return nullptr;
	}

	buildBuffer = new fileInPack_t[gi.entryNumber];
	pack = new pK5Nb_t;
	for ( i = 0; i < FILE_HASH_SIZE; i++ ) {
		pack->hashPaKTable[i] = nullptr;
	}

	pack->pakFileName = zipfile;
	pack->handle = uf;
	pack->numFiles = gi.entryNumber;
	pack->buildBuffer = buildBuffer;
	pack->referenced = false;
	pack->pakFileStatus = BINARY_UNKNOWN;
	pack->addon = false;
	pack->pakAddonSearch = false;
	pack->pakInfo = nullptr;
	pack->pureStatus = PURE_UNKNOWN;
	pack->isNew = false;

	pack->length = len;

	PAK_GoToFirstFile(uf);
	fs_headerLongs = (int *)Mem_ClearedAlloc( gi.entryNumber * sizeof(int) );
	for ( i = 0; i < (int)gi.entryNumber; i++ ) {
		err = PAK_GetFileDescription( uf, &fInfo, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0 );
		if ( err != UNZ_OK ) {
			break;
		}
		if ( fInfo.uncompressedSize > 0 ) {
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong( fInfo.crc );
		}
		hash = HashFileName( filename_inzip );
		buildBuffer[i].name = filename_inzip;
		buildBuffer[i].name.ToLower();
		buildBuffer[i].name.BackSlashesToSlashes();
		// store the file position in the zip
		PAK_GetFileStatus( uf, &buildBuffer[i].pos );
		// add the file to the hash
		buildBuffer[i].next = pack->hashPaKTable[hash];
		pack->hashPaKTable[hash] = &buildBuffer[i];
		// go to the next file in the zip
		PAK_NextFile(uf);
	}

	// check if this is an addon pak
	pack->addon = false;
	confHash = HashFileName( ADDON_CONFIG );
	for ( pakFile = pack->hashPaKTable[confHash]; pakFile; pakFile = pakFile->next ) {
		if ( !CompareFileName( pakFile->name, ADDON_CONFIG ) ) {			
			pack->addon = true;			
			anCompressedArchive *file = ReadFileFromZip( pack, pakFile, ADDON_CONFIG );
			// may be just an empty file if you don't bother about the mapDef
			if ( file && file->Length() ) {
				char *buf;
				buf = new char[ file->Length() + 1 ];
				file->Read( (void *)buf, file->Length() );
				buf[ file->Length() ] = '\0';
				pack->pakInfo = ParseAddonDef( buf, file->Length() );
				delete[] buf;
			}
			if ( file ) {
				CloseFile( file );
			}
			break;
		}
	}

	pack->checksum = MD4_BlockChecksum( fs_headerLongs, 4 * fs_numHeaderLongs );
	pack->checksum = LittleLong( pack->checksum );

	Mem_Free( fs_headerLongs );

	return pack;
}

/*
===============
anFileSystem::AddZipFile

adds a downloaded pak file to the list so we can work out what we have and what we still need
the isNew flag is set to true, indicating that we cannot add this pak to the search lists without a restart
===============
*/
int anFileSystem::AddZipFile( const char *path ) {
	anStr fullpath = fs_savepath.GetString();

	fullpath.AppendPath( path );
	pK5Nb_t *pak = LoadZipFile( fullpath );
	if ( !pak ) {
		common->Warning( "Add .pak file %s failed\n", path );
		return 0;
	}
	// insert the pak at the end of the search list - temporary until we restart
	pak->isNew = true;
	searchPath_t *search = new searchPath_t;
	search->dir = nullptr;
	search->pack = pak;
	search->next = nullptr;
	searchPath_t *last = searchPaths;
	while ( last->next ) {
		last = last->next;
	}
	last->next = search;
	common->Printf( "Appended pk5 %s with checksum 0x%x\n", pak->pakFileName.c_str(), pak->checksum );
	return pak->checksum;
}

/*
===============
anFileSystem::AddUnique
===============
*/
int anFileSystem::AddUnique( const char *name, anStringList &list, anHashIndex &hashIndex ) const {
	int hashKey = hashIndex.GenerateKey( name );
	for ( int i = hashIndex.First( hashKey ); i >= 0; i = hashIndex.Next( i ) ) {
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
anFileSystem::GetExtensionList
===============
*/
void anFileSystem::GetExtensionList( const char *extension, anStringList &extensionList ) const {
	int l = anStr::Length( extension );
	int s = 0;
	while ( 1 ) {
		int e = anStr::FindChar( extension, '|', s, l );
		if ( e != -1 ) {
			extensionList.Append( anStr( extension, s, e ) );
			s = e + 1;
		} else {
			extensionList.Append( anStr( extension, s, l ) );
			break;
		}
	}
}

/*
===============
anFileSystem::GetFileList

Does not clear the list first so this can be used to progressively build a file list.
When 'sort' is true only the new files added to the list are sorted.
===============
*/
int anFileSystem::GetFileList( const char *relativePath, const anStringList &extensions, anStringList &list, anHashIndex &hashIndex, bool fullRelativePath, const char *gamedir ) {
	searchPath_t *	search;
	fileInPack_t *	buildBuffer;
	int				pathLength;
	int				length;
	const char *	name;
	pK5Nb_t *		pak;
	anStr			work;

	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	if ( !extensions.Num() || ! relativePath ) {
		return 0;
	}
	pathLength = strlen( relativePath );
	if ( pathLength ) {
		pathLength++;	// for the trailing '/'
	}

	// search through the path, one element at a time, adding to list
	for ( search = searchPaths; search != nullptr; search = search->next ) {
		if ( search->dir ) {
			if ( gamedir && strlen( gamedir ) ) {
				if ( search->dir->gamedir != gamedir ) {
					continue;
				}
			}

			anStringList	sysFiles;
			anStr		netpath;

			netpath = BuildOSPath( search->dir->path, search->dir->gamedir, relativePath );

			for ( int i = 0; i < extensions.Num(); i++ ) {
				// scan for files in the filesystem
				ListOSFiles( netpath, extensions[i], sysFiles );
				// if we are searching for directories, remove . and ..
				if ( extensions[i][0] == '/' && extensions[i][1] == 0 ) {
					sysFiles.Remove( "." );
					sysFiles.Remove( ".." );
				}

				for ( int j = 0; j < sysFiles.Num(); j++ ) {
					// unique the match
					if ( fullRelativePath ) {
						work = relativePath;
						work += "/";
						work += sysFiles[j];
						AddUnique( work, list, hashIndex );
					} else {
						AddUnique( sysFiles[j], list, hashIndex );
					}
				}
			}
		} else if ( search->pack ) {
			// look through all the pak file elements
			// exclude any extra packs if we have server paks to search
			if ( serverPaks.Num() ) {
				GetPackStatus( search->pack );
				if ( search->pack->pureStatus != PURE_NEVER && !serverPaks.Find( search->pack ) ) {
					continue; // not on the pure server pak list
				}
			}

			pak = search->pack;
			buildBuffer = pak->buildBuffer;
			for ( int i = 0; i < pak->numFiles; i++ ) {
				length = buildBuffer[i].name.Length();
				// if the name is not long anough to at least contain the path
				if ( length <= pathLength ) {
					continue;
				}
				name = buildBuffer[i].name;

				// check for a path match without the trailing '/'
				if ( pathLength && anStr::Icmpn( name, relativePath, pathLength - 1 ) != 0 ) {
					continue;
				}
 
				// ensure we have a path, and not just a filename containing the path
				if ( name[ pathLength ] == '\0' || name[pathLength - 1] != '/' ) {
					continue;
				}
 
				// make sure the file is not in a subdirectory
				for ( int j = pathLength; name[j+1] != '\0'; j++ ) {
					if ( name[j] == '/' ) {
						break;
					}
				}
				if ( name[j+1] ) {
					continue;
				}

				// check for extension match
				for ( int j = 0; j < extensions.Num(); j++ ) {
					if ( length >= extensions[j].Length() && extensions[j].Icmp( name + length - extensions[j].Length() ) == 0 ) {
						break;
					}
				}
				if ( j >= extensions.Num() ) {
					continue;
				}

				// unique the match
				if ( fullRelativePath ) {
					work = relativePath;
					work += "/";
					work += name + pathLength;
					work.StripTrailing( '/' );
					AddUnique( work, list, hashIndex );
				} else {
					work = name + pathLength;
					work.StripTrailing( '/' );
					AddUnique( work, list, hashIndex );
				}
			}
		}
	}

	return list.Num();
}
/*
================
SortFileList


anFileSystem::ListSortedFiles( "/path/to/directory", fileList)
or 
fileSystem->ListSortedFiles( "/path/to/directory", fileList)
================
*/
void anFileSystem::SortFileList( anFileList fileList ) const {
    anStringList sortedList;
    int numSortedFiles = 0;

    for (int i = 0; i < fileList.Num(); i++) {
        for ( int j = 0; j < numSortedFiles; j++ ) {
            if ( fileList[i].Cmp( sortedList[j] ) < 0 ) {
                break;
            }
        }
        sortedList.Insert( fileList[i], j);
        numSortedFiles++;
    }
}

}
/*
================
SortFileList
================
*/
void anFileSystem::ListSortedFiles( const char *directory, anFileList &fileList ) const {
	ListFiles( directory, fileList );
	SortFileList( fileList );
}

/*
===============
anFileSystem::ListFiles
===============
*/
anFileList *anFileSystem::ListFiles( const char *relativePath, const char *extension, bool sort, bool fullRelativePath, const char *gamedir ) {
	anHashIndex hashIndex( 4096, 4096 );
	anStringList extensionList;

	anFileList *fileList = new anFileList;
	fileList->basePath = relativePath;

	GetExtensionList( extension, extensionList );
	GetFileList( relativePath, extensionList, fileList->list, hashIndex, fullRelativePath, gamedir );

	if ( sort ) {
		anStringListSortPaths( fileList->list );
	}

	return fileList;
}

/*
===============
anFileSystem::GetFileListTree
===============
*/
int anFileSystem::GetFileListTree( const char *relativePath, const anStringList &extensions, anStringList &list, anHashIndex &hashIndex, const char *gamedir ) {
	anStringList slash, folders( 128 );
	anHashIndex folderHashIndex( 1024, 128 );

	// recurse through the subdirectories
	slash.Append( "/" );
	GetFileList( relativePath, slash, folders, folderHashIndex, true, gamedir );
	for ( int i = 0; i < folders.Num(); i++ ) {
		if ( folders[i][0] == '.' ) {
			continue;
		}
		if ( folders[i].Icmp( relativePath ) == 0 ) {
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
anFileSystem::ListFilesTree
===============
*/
anFileList *anFileSystem::ListFilesTree( const char *relativePath, const char *extension, bool sort, const char *gamedir ) {
	anHashIndex hashIndex( 4096, 4096 );
	anStringList extensionList;

	anFileList *fileList = new anFileList();
	fileList->basePath = relativePath;
	fileList->list.SetGranularity( 4096 );

	GetExtensionList( extension, extensionList );
	GetFileListTree( relativePath, extensionList, fileList->list, hashIndex, gamedir );

	if ( sort ) {
		anStringListSortPaths( fileList->list );
	}

	return fileList;
}

/*
===============
anFileSystem::FreeFileList
===============
*/
void anFileSystem::FreeFileList( anFileList *fileList ) {
	delete[] fileList;
}

/*
===============
anFileSystem::ListMetaData
===============
*/
anMetaDataList *anFileSystem::ListMetaData( const char *metaDataTag ) {
	// You can use the provided metaDataTag parameter to filter
	//the metadata based on a specific tag.
	// Create an instance of anMetaDataLisa to hold the metadata
	//entries.
	anMetaDataList *metaDataList = new anMetaDataList;
	for ( int i = 0; i < meta.Num(); i++ ) {
		//metaDataList->meta.Add( metaDataTag, meta[i] );
		metaDataList->meta.Add( "metaData", meta[i].name );
		return metaDataList;
	}
	// Add the retrieved metadata entries to the `metaDataList`.
	// You can use the `metaDataList->meta.Add()` function to add each metadata entry.

	// Return the populated metaDataList.
	return metaDataTag;
}
anMetaDataList *anFileSystem::ListMetaData( const char *metaDataTag ) {
	anMetaDataList *metaDataList = new anMetaDataList;
	metaDataList = metaDataTag[i].meta->GetString( "metadata_addon" );

	FindMetaDataContext( metaDataTag, metaDataList );
	for ( int i = 0; i < meta.GetNumMetaData(); i++ ) {
		metaDataList = meta[i].meta->FindMetaData( metaDataTag, metaDataList );
	}
	return metaDataTag;
}

/*
===============
anFileSystem::FreeMetaDataList
===============
*/
void anFileSystem::FreeMetaDataList( anMetaDataList *metaDataList ) {
	// Iterate over each metadata entry in the `metaDataList`.
    for ( int i = 0; i < metaDataList->GetNumMetaData(); i++ ) {
		// Delete the associated `Dictionary in each metadata entry.
        delete metaDataList->GetMetaData( i );
	}
	// Clear the `metaDataList` itself.
    metaDataList->meta.Clear();
    // Finally, delete the meta data to free the memory.
	delete[] metaDataList;
}

/*
===============
anFileSystem::FreeMetaDataList

FIXME: implement me!
===============
*/
anFileList *anFileSystem::ListTools() {
	assert( 0 );
}

/*
===============
anFileSystem::ListMods
===============
*/
anModList *anFileSystem::ListMods( void ) {
	const int 	MAX_DESCRIPTION = 256;
	char 		desc[ MAX_DESCRIPTION ];

	anStringList	dirs;
	anStringList	pk5s;

	anModList	*list = new anModList;

	const char	*search[ 4 ];
	int			isearch;

	search[0] = fs_savepath.GetString();
	search[1] = fs_devpath.GetString();
	search[2] = fs_basepath.GetString();
	search[3] = fs_cdpath.GetString();

	for ( isearch = 0; isearch < 4; isearch++ ) {
		dirs.Clear();
		pk5s.Clear();

		// scan for directories
		ListOSFiles( search[isearch], "/", dirs );

		dirs.Remove( "." );
		dirs.Remove( ".." );
		dirs.Remove( "base" );
		dirs.Remove( "pb" );

		// see if there are any pk5 files in each directory
		for ( int i = 0; i < dirs.Num(); i++ ) {
			anStr gamepath = BuildOSPath( search[isearch], dirs[i], "" );
			ListOSFiles( gamepath, ".pk5", pk5s );
			if ( pk5s.Num() ) {
				if ( !list->mods.Find( dirs[i] ) ) {
					list->mods.Append( dirs[i] );
				}
			}
		}
	}

	list->mods.Sort();

	// read the descriptions for each mod - search all paths
	for ( i = 0; i < list->mods.Num(); i++ ) {
		for ( isearch = 0; isearch < 4; isearch++ ) {
			anStr descfile = BuildOSPath( search[isearch], list->mods[i], "description.txt" );
			FILE *f = OpenOSFile( descfile, "r" );
			if ( f ) {
				if ( fgets( desc, MAX_DESCRIPTION, f ) ) {
					list->descriptions.Append( desc );
					fclose( f );
					break;
				} else {
					common->DWarning( "Error reading %s", descfile.c_str() );
					fclose( f );
					continue;
				}
			}
		}

		if ( isearch == 4 ) {
			list->descriptions.Append( list->mods[i] );
		}
	}

	list->mods.Insert( "" );
	list->descriptions.Insert( "ArC-NeT" );

	assert( list->mods.Num() == list->descriptions.Num() );
	return list;
}

/*
===============
anFileSystem::FreeModList
===============
*/
void anFileSystem::FreeModList( anModList *modList ) {
	delete modList;
}

/*
===============
anDEntry::Matches
===============
*/
bool anDEntry::Matches(const char *directory, const char *extension) const {
	if ( !anDEntry::directory.Icmp( directory ) && !anDEntry::extension.Icmp( extension ) ) {
		return true;
	}
	return false;
}

/*
===============
anDEntry::Init
===============
*/
void anDEntry::Init( const char *directory, const char *extension, const anStringList &list ) {
	anDEntry::directory = directory;
	anDEntry::extension = extension;
	anStringList::operator=(list);
}

/*
===============
anDEntry::Clear
===============
*/
void anDEntry::Clear( void ) {
	directory.Clear();
	extension.Clear();
	anStringList::Clear();
}

/*
===============
anFileSystem::ListOSFiles

call to the OS for a listing of files in an OS directory
optionally, perform some caching of the entries
===============
*/
int	anFileSystem::ListOSFiles( const char *directory, const char *extension, anStringList &list ) {
	if ( !extension ) {
		extension = "";
	}

	if ( !fs_caseSensitiveOS.GetBool() ) {
		return Sys_ListFiles( directory, extension, list );
	}

	// try in cache
	int i = dir_cache_index - 1;

	while ( i >= dir_cache_index - dir_cache_count ) {
		int j = ( i + MAX_CACHED_DIRS ) % MAX_CACHED_DIRS;
		if ( dir_cache[j].Matches( directory, extension ) ) {
			if ( fs_debug.GetInteger() ) {
				common->Printf( "FileSystem::ListOSFiles: cache hit: %s\n", directory );
			}
			list = dir_cache[j];
			return list.Num();
		}
		i--;
	}

	if ( fs_debug.GetInteger() ) {
		common->Printf( "FileSystem::ListOSFiles: cache miss: %s\n", directory );
	}

	if ( ret == -1 ) {
		return -1;
		// try a case insensitive directory walk
		const char *cased_dir = CaseSearch( directory );
		if ( !cased_dir ) {
			return -1;
		}
		ret = Sys_ListFiles( cased_dir, extension, list );
		if ( ret == -1 ) {
			common->DPrintf( "FileSystem::ListOSFiles: unexpected, Sys_ListFiles failed on case matched directory\n" );
			return -1;
		}
 	}

	// push a new entry if case matched we are caching with the requested directory name
	dir_cache[dir_cache_index].Init( directory, extension, list );
	dir_cache_index = ( ++dir_cache_index ) % MAX_CACHED_DIRS;
/*	if ( dir_cache_count < MAX_CACHED_DIRS ) {
		dir_cache_count++;
	}
 	dir_cache_index = 0;
 	dir_cache_count = 0;
+	for ( int i = 0; i < MAX_CACHED_DIRS; i++ ) {
+		dir_cache[i].Clear();
+	}
+
+	dir_case.Clear();*/

	return ret;
}

/*
================
anFileSystem::Dir_f
================
*/
void anFileSystem::Dir_f( const anCommandArgs &args ) {
	anStr	relativePath;
	anStr	extension;

	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		common->Printf( "usage: dir <directory> [extension]\n" );
		return;
	}

	if ( args.Argc() == 2 ) {
		relativePath = args.Argv( 1 );
		extension = "";
	} else {
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

	anFileList *fileList = fileSystem.ListFiles( relativePath, extension );

	for ( int i = 0; i < fileList->GetNumFiles(); i++ ) {
		common->Printf( "%s\n", fileList->GetFile( i ) );
	}
	common->Printf( "%d files\n", fileList->list.Num() );
	fileSystem.FreeFileList( fileList );
}

/*
================
anFileSystem::DirTree_f
================
*/
void anFileSystem::DirTree_f( const anCommandArgs &args ) {
	anStr	relativePath;
	anStr	extension;

	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		common->Printf( "usage: dirtree <directory> [extension]\n" );
		return;
	}

	if ( args.Argc() == 2 ) {
		relativePath = args.Argv( 1 );
		extension = "";
	} else {
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

	anFileList *fileList = fileSystem.ListFilesTree( relativePath, extension );

	for ( int i = 0; i < fileList->GetNumFiles(); i++ ) {
		common->Printf( "%s\n", fileList->GetFile( i ) );
	}
	common->Printf( "%d files\n", fileList->list.Num() );
	fileSystem.FreeFileList( fileList );
}

/*
============
anFileSystem::Path_f
============
*/
void anFileSystem::Path_f( const anCommandArgs &args ) {
	anStr status;

	common->Printf( "Current search path:\n" );
	for ( searchPath_t *sp = fileSystem.searchPaths; sp; sp = sp->next ) {
		if ( sp->pack ) {
			if ( com_developer.GetBool() ) {
				sprintf( status, "%s (%i files - 0x%x %s", sp->pack->pakFileName.c_str(), sp->pack->numFiles, sp->pack->checksum, sp->pack->referenced ? "referenced" : "not referenced" );
				if ( sp->pack->addon ) {
					status += " - addon)\n";
				} else {
					status += " )\n";
				}
				common->Printf( status.c_str() );
			} else {
				common->Printf( "%s (%i files)\n", sp->pack->pakFileName.c_str(), sp->pack->numFiles );
			}
			if ( fileSystem.serverPaks.Num() ) {
				if ( fileSystem.serverPaks.Find( sp->pack ) ) {
					common->Printf( "    on the pure list\n" );
				} else {
					common->Printf( "    not on the pure list\n" );
				}
			}
		} else {
			common->Printf( "%s/%s\n", sp->dir->path.c_str(), sp->dir->gamedir.c_str() );
		}
	}
	common->Printf( "game DLL: 0x%x in pak: 0x%x\n", fileSystem.engineDLLChecksum, fileSystem.enginePakChecksum );
#if ARC_FAKE_PURE
	common->Printf( "NOTE: ARC_FAKE_PURE is enabled\n" );
#endif
	for ( int i = 0; i < MAX_GAME_OS; i++ ) {
		if ( fileSystem.enginePakForOS[i] ) {
			common->Printf( "OS %d - pak 0x%x\n", i, fileSystem.enginePakForOS[i] );
		}
	}
	// show addon packs that are *not* in the search lists
	common->Printf( "Addon pk5s:\n" );
	for ( sp = fileSystem.addonPaks; sp; sp = sp->next ) {
		if ( com_developer.GetBool() ) {
			common->Printf( "%s (%i files - 0x%x)\n", sp->pack->pakFileName.c_str(), sp->pack->numFiles, sp->pack->checksum );
		} else {
			common->Printf( "%s (%i files)\n", sp->pack->pakFileName.c_str(), sp->pack->numFiles );
		}		
	}
}

/*
============
anFileSystem::GetOSMask
============
*/
int anFileSystem::GetOSMask( void ) {
	for ( int i = 0; i < MAX_GAME_OS; i++ ) {
		if ( fileSystem.enginePakForOS[i] ) {
			int ret |= ( 1 << i );
		}
	}
	if ( !ret ) {
		return -1;
	}
	return ret;
}

/*
============
anFileSystem::TouchFile_f

The only purpose of this function is to allow game script files to copy
arbitrary files furing an "fs_copyfiles 1" run.
============
*/
void anFileSystem::TouchFile_f( const anCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: touchFile <file>\n" );
		return;
	}

	anFile *f = fileSystem.OpenFileRead( args.Argv( 1 ) );
	if ( f ) {
		fileSystem.CloseFile( f );
	}
}

/*
============
anFileSystem::TouchFileList_f

Takes a text file and touches every file in it, use one file per line.
============
*/
void anFileSystem::TouchFileList_f( const anCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: touchFileList <filename>\n" );
		return;
	}

	const char *buffer = nullptr;
	anParser src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	if ( fileSystem->ReadFile( args.Argv( 1 ), (void **)&buffer, nullptr ) && buffer ) {
		src.LoadMemory( buffer, strlen( buffer ), args.Argv( 1 ) );
		if ( src.IsLoaded() ) {
			anToken token;
			while( src.ReadToken( &token ) ) {
				common->Printf( "%s\n", token.c_str() );
				session->UpdateScreen();
				anFile *f = fileSystem.OpenFileRead( token );
				if ( f ) {
					fileSystem.CloseFile( f );
				}
			}
		}
	}
}

/*
================
anFileSystem::IsEoF

checks if the file has ended, or still being read.
================
*/
bool anFileSystem::IsEoF( const anFile *f ) const {
	// Check if current position is past end of file
	return ( f->Tell() >= f->Length() );
}

/*
================
anFileSystem::ExplicitIsEOF

for debugging and additional file checks/information 
================
*/
bool ExplicitIsEOF( anFile *file ) const {
	int pos = file->Tell();
	if ( pos < 0 ) {
		// Handle error
		common->DPrintf( "[FileSystem - WARNING] EoF file position Error" );
		return false; 
	}

	int len = file->Length();
	if ( len < 0 ) {
		// Handle error
		common->DPrintf( "[FileSystem - WARNING] EoF file length Error" );
		return false;
	}

	return pos >= len;
}

/*
================
anFileSystem::AddGameDirectory

Sets engineFolder, adds the directory to the head of the search paths, then loads any pk5 files.
================
*/
void anFileSystem::AddGameDirectory( const char *path, const char *dir ) {
	int				i;
	searchPath_t *	search;
	pK5Nb_t *		pak;
	anStr		pakfile;
	anStringList	pakfiles;

	// check if the search path already exists
	for ( search = searchPaths; search; search = search->next ) {
		// if this element is a pak file
		if ( !search->dir ) {
			continue;
		}
		if ( search->dir->path.Cmp( path ) == 0 && search->dir->gamedir.Cmp( dir ) == 0 ) {
			return;
		}
	}

	engineFolder = dir;

	//
	// add the directory to the search path
	//
	search = new searchPath_t;
	search->dir = new directory_t;
	search->pack = nullptr;

	search->dir->path = path;
	search->dir->gamedir = dir;
	search->next = searchPaths;
	searchPaths = search;

	// find all pak files in this directory
	pakfile = BuildOSPath( path, dir, "" );
	pakfile[ pakfile.Length() - 1 ] = 0;	// strip the trailing slash

	ListOSFiles( pakfile, ".pk5", pakfiles );

	// sort them so that later alphabetic matches override
	// earlier ones. This makes pak1.pk5 override pak0.pk5
	pakfiles.Sort();

	for ( i = 0; i < pakfiles.Num(); i++ ) {
		pakfile = BuildOSPath( path, dir, pakfiles[i] );
		pak = LoadZipFile( pakfile );
		if ( !pak ) {
			continue;
		}
		// insert the pak after the directory it comes from
		search = new searchPath_t;
		search->dir = nullptr;
		search->pack = pak;
		search->next = searchPaths->next;
		searchPaths->next = search;
		common->Printf( "Loaded pk5 %s with checksum 0x%x\n", pakfile.c_str(), pak->checksum );
	}
}

/*
================
anFileSystem::SetupGameDirectories

  Takes care of the correct search order.
================
*/
void anFileSystem::SetupGameDirectories( const char *gameName ) {
	// setup cdpath
	if ( fs_cdpath.GetString()[0] ) {
		AddGameDirectory( fs_cdpath.GetString(), gameName );
	}

	// setup basepath
	if ( fs_basepath.GetString()[0] ) {
		AddGameDirectory( fs_basepath.GetString(), gameName );
	}

	// setup devpath
	if ( fs_devpath.GetString()[0] ) {
		AddGameDirectory( fs_devpath.GetString(), gameName );
	}

	// setup savepath
	if ( fs_savepath.GetString()[0] ) {
		AddGameDirectory( fs_savepath.GetString(), gameName );
	}
}

/*
===============
anFileSystem::FollowDependencies
===============
*/
void anFileSystem::FollowAddonDependencies( pK5Nb_t *pak ) {
	assert( pak );
	if ( !pak->pakInfo || !pak->pakInfo->depends.Num() ) {
		return;
	}
	int i, num = pak->pakInfo->depends.Num();
	for ( i = 0; i < num; i++ ) {
		pK5Nb_t *deppak = GetPackForChecksum( pak->pakInfo->depends[i], true );
		if ( deppak ) {
			// make sure it hasn't been marked for search already
			if ( !deppak->pakAddonSearch ) {
				// must clean addonChecksums as we go
				int addon_index = addonChecksums.FindIndex( deppak->checksum );
				if ( addon_index >= 0 ) {
					addonChecksums.RemoveIndex( addon_index );
				}
				deppak->pakAddonSearch = true;
				common->Printf( "Addon pk5 %s 0x%x depends on pak %s 0x%x, will be searched\n",
								pak->pakFileName.c_str(), pak->checksum,
								deppak->pakFileName.c_str(), deppak->checksum );
				FollowAddonDependencies( deppak );
			}
		} else {
			common->Printf( "Addon pk5 %s 0x%x depends on unknown pak 0x%x\n",
							pak->pakFileName.c_str(), pak->checksum, pak->pakInfo->depends[i] );
		}
	}
}

/*
================
anFileSystem::Startup
================
*/
void anFileSystem::Startup( void ) {
	searchPath_t	**search;
	int				i;
	pK5Nb_t			*pak;
	int				addon_index;

	common->Printf( "------ Initializing File System ------\n" );

	if ( restartChecksums.Num() ) {
		common->Printf( "restarting in pure mode with %d pak files\n", restartChecksums.Num() );
	}
	if ( addonChecksums.Num() ) {
		common->Printf( "restarting filesystem with %d addon pak file( s) to include\n", addonChecksums.Num() );
	}

	SetupGameDirectories( BASE_GAMEDIR );

	// fs_game_base override
	if ( fs_game_base.GetString()[0] &&
		 anStr::Icmp( fs_game_base.GetString(), BASE_GAMEDIR ) ) {
		SetupGameDirectories( fs_game_base.GetString() );
	}

	// fs_game override
	if ( fs_game.GetString()[0] &&
		 anStr::Icmp( fs_game.GetString(), BASE_GAMEDIR ) &&
		 anStr::Icmp( fs_game.GetString(), fs_game_base.GetString() ) ) {
		SetupGameDirectories( fs_game.GetString() );
	}

	// currently all addons are in the search list - deal with filtering out and dependencies now
	// scan through and deal with dependencies
	search = &searchPaths;
	while ( *search ) {
		if ( !( *search )->pack || !( *search )->pack->addon ) {
			search = &( ( *search )->next );
			continue;
		}
		pak = ( *search )->pack;
		if ( fs_searchAddons.GetBool() ) {
			// when we have fs_searchAddons on we should never have addonChecksums
			assert( !addonChecksums.Num() );
			pak->pakAddonSearch = true;
			search = &( ( *search )->next );
			continue;
		}
		addon_index = addonChecksums.FindIndex( pak->checksum );
		if ( addon_index >= 0 ) {
			assert( !pak->pakAddonSearch );	// any pak getting flagged as pakAddonSearch should also have been removed from addonChecksums already
			pak->pakAddonSearch = true;
			addonChecksums.RemoveIndex( addon_index );
			FollowAddonDependencies( pak );
		}
		search = &( ( *search )->next );
	}

	// now scan to filter out addons not marked pakAddonSearch
	search = &searchPaths;
	while ( *search ) {
		if ( !( *search )->pack || !( *search )->pack->addon ) {
			search = &( ( *search )->next );
			continue;
		}
		assert( !( *search )->dir );
		pak = ( *search )->pack;
		if ( pak->pakAddonSearch ) {
			common->Printf( "Addon pk5 %s with checksum 0x%x is on the search list\n",
							pak->pakFileName.c_str(), pak->checksum );
			search = &( ( *search )->next );
		} else {
			// remove from search list, put in addons list
			searchPath_t *paksearch = *search;
			*search = ( *search )->next;
			paksearch->next = addonPaks;
			addonPaks = paksearch;
			common->Printf( "Addon pk5 %s with checksum 0x%x is on addon list\n",
							pak->pakFileName.c_str(), pak->checksum );				
		}
	}

	// all addon paks found and accounted for
	assert( !addonChecksums.Num() );
	addonChecksums.Clear();	// just in case

	if ( restartChecksums.Num() ) {
		search = &searchPaths;
		while ( *search ) {
			if ( !( *search )->pack ) {
				search = &( ( *search )->next );
				continue;
			}
			if ( ( i = restartChecksums.FindIndex( ( *search )->pack->checksum ) ) != -1 ) {
				if ( i == 0 ) {
					// this pak is the next one in the pure search order
					serverPaks.Append( ( *search )->pack );
					restartChecksums.RemoveIndex( 0 );
					if ( !restartChecksums.Num() ) {
						break; // early out, we're done
					}
					search = &( ( *search )->next );
					continue;
				} else {
					// this pak will be on the pure list, but order is not right yet
					searchPath_t	*aux;
					aux = ( *search )->next;
					if ( !aux ) {
						// last of the list can't be swapped back
						if ( fs_debug.GetBool() ) {
							common->Printf( "found pure checksum %x at index %d, but the end of search path is reached\n", ( *search )->pack->checksum, i );
							anStr checks;
							checks.Clear();
							for ( i = 0; i < serverPaks.Num(); i++ ) {
								checks += va( "%p ", serverPaks[i] );
							}
							common->Printf( "%d pure paks - %s \n", serverPaks.Num(), checks.c_str() );
							checks.Clear();
							for ( i = 0; i < restartChecksums.Num(); i++ ) {
								checks += va( "%x ", restartChecksums[i] );
							}
							common->Printf( "%d paks left - %s\n", restartChecksums.Num(), checks.c_str() );
						}
						common->FatalError( "Failed to restart with pure mode restrictions for server connect" );
					}
					// put this search path at the end of the list
					searchPath_t *search_end;
					search_end = ( *search )->next;
					while ( search_end->next ) {
						search_end = search_end->next;
					}
					search_end->next = *search;
					*search = ( *search )->next;
					search_end->next->next = nullptr;
					continue;
				}
			}
			// this pak is not on the pure list
			search = &( ( *search )->next );
		}
		// the list must be empty
		if ( restartChecksums.Num() ) {
			if ( fs_debug.GetBool() ) {
				anStr checks;
				checks.Clear();
				for ( i = 0; i < serverPaks.Num(); i++ ) {
					checks += va( "%p ", serverPaks[i] );
				}
				common->Printf( "%d pure paks - %s \n", serverPaks.Num(), checks.c_str() );
				checks.Clear();
				for ( i = 0; i < restartChecksums.Num(); i++ ) {
					checks += va( "%x ", restartChecksums[i] );
				}
				common->Printf( "%d paks left - %s\n", restartChecksums.Num(), checks.c_str() );
			}
			common->FatalError( "Failed to restart with pure mode restrictions for server connect" );
		}
		// also the game pak checksum
		// we could check if the game pak is actually present, but we would not be restarting if there wasn't one @ first pure check
		enginePakChecksum = restartGamePakChecksum;
	}

	// add our commands
	cmdSystem->AddCommand( "dir", Dir_f, CMD_FL_SYSTEM, "lists a folder", idCmdSystem::ArgCompletion_FileName );
	cmdSystem->AddCommand( "dirtree", DirTree_f, CMD_FL_SYSTEM, "lists a folder with subfolders" );
	cmdSystem->AddCommand( "path", Path_f, CMD_FL_SYSTEM, "lists search paths" );
	cmdSystem->AddCommand( "touchFile", TouchFile_f, CMD_FL_SYSTEM, "touches a file" );
	cmdSystem->AddCommand( "touchFileList", TouchFileList_f, CMD_FL_SYSTEM, "touches a list of files" );

	// print the current search paths
	Path_f( anCommandArgs() );

	common->Printf( "file system initialized.\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
===================
anFileSystem::SetRestrictions

Looks for product keys and restricts media add on ability
if the full version is not found
===================
*/
void anFileSystem::SetRestrictions( void ) {
#ifdef ARCNET_DEMO_BUILD
	common->Printf( "\nRunning in restricted demo mode.\n\n" );
	// make sure that the pak file has the header checksum we expect
	searchPath_t	*search;
	for ( search = searchPaths; search; search = search->next ) {
		if ( search->pack ) {
			// a tiny attempt to keep the checksum from being scannable from the exe
			if ( ( search->pack->checksum ^ 0x84268436u ) != ( DEMO_PAK_CHECKSUM ^ 0x84268436u ) ) {
				common->FatalError( "Corrupted %s: 0x%x", search->pack->pakFileName.c_str(), search->pack->checksum );
			}
		}
	}
	cvarSystem->SetCVarBool( "fs_restrict", true );
#endif
}

/*
=====================
anFileSystem::UpdatePureServerChecksums
=====================
*/
void anFileSystem::UpdatePureServerChecksums( void ) {
	searchPath_t	*search;
	int				i;
	pureStatus_t	status;

	serverPaks.Clear();
	for ( search = searchPaths; search; search = search->next ) {
		// is the element a referenced pak file?
		if ( !search->pack ) {
			continue;
		}
		status = GetPackStatus( search->pack );
		if ( status == PURE_NEVER ) {
			continue;
		}
		if ( status == PURE_NEUTRAL && !search->pack->referenced ) {
			continue;
		}
		serverPaks.Append( search->pack );
		if ( serverPaks.Num() >= MAX_PURE_PAKS ) {
			common->FatalError( "MAX_PURE_PAKS ( %d ) exceeded\n", MAX_PURE_PAKS );
		}
	}
	if ( fs_debug.GetBool() ) {
		anStr checks;
		for ( i = 0; i < serverPaks.Num(); i++ ) {
			checks += va( "%x ", serverPaks[i]->checksum );
		}
		common->Printf( "set pure list - %d paks ( %s)\n", serverPaks.Num(), checks.c_str() );
	}
}

/*
=====================
anFileSystem::UpdateGamePakChecksums
=====================
*/
bool anFileSystem::UpdateGamePakChecksums( void ) {
	searchPath_t	*search;
	fileInPack_t	*pakFile;
	int				confHash;
	anFile			*confFile;
	char			*buf;
	anLexer			*lexConf;
	anToken			token;
	int				id;

	confHash = HashFileName( BINARY_CONFIG );

	memset( enginePakForOS, 0, sizeof( enginePakForOS ) );
	for ( search = searchPaths; search; search = search->next ) {
		if ( !search->pack ) {
			continue;
		}
		search->pack->pakFileStatus = BINARY_NO;
		for ( pakFile = search->pack->hashPaKTable[confHash]; pakFile; pakFile = pakFile->next ) {
			if ( !CompareFileName( pakFile->name, BINARY_CONFIG ) ) {
				search->pack->pakFileStatus = BINARY_YES;
				confFile = ReadFileFromZip( search->pack, pakFile, BINARY_CONFIG );
				buf = new char[ confFile->Length() + 1 ];
				confFile->Read( (void *)buf, confFile->Length() );
				buf[ confFile->Length() ] = '\0';
				lexConf = new anLexer( buf, confFile->Length(), confFile->GetFullPath() );
				while ( lexConf->ReadToken( &token ) ) {
					if ( token.IsNumeric() ) {
						id = atoi( token );
						if ( id < MAX_GAME_OS && !enginePakForOS[ id ] ) {
							if ( fs_debug.GetBool() ) {
								common->Printf( "Adding game pak checksum for OS %d: %s 0x%x\n", id, confFile->GetFullPath(), search->pack->checksum );
							}
 							enginePakForOS[ id ] = search->pack->checksum;
						}
					}
				}
				CloseFile( confFile );
				delete lexConf;
				delete[] buf;
			}
		}
	}

	// some sanity checks on the game code references
	// make sure that at least the local OS got a pure reference
	if ( !enginePakForOS[ BUILD_OS_ID ] ) {
		common->Warning( "No game code pak reference found for the local OS" );
		return false;
	}

	if ( !cvarSystem->GetCVarBool( "net_serverAllowServerMod" ) &&
		enginePakChecksum != enginePakForOS[ BUILD_OS_ID ] ) {
		common->Warning( "The current game code doesn't match pak files (net_serverAllowServerMod is off)" );
		return false;
	}

	return true;
}

/*
=====================
anFileSystem::GetPackForChecksum
=====================
*/
pK5Nb_t* anFileSystem::GetPackForChecksum( int checksum, bool searchAddons ) {
	searchPath_t	*search;
	for ( search = searchPaths; search; search = search->next ) {
		if ( !search->pack ) {
			continue;
		}
		if ( search->pack->checksum == checksum ) {
			return search->pack;
		}
	}
	if ( searchAddons ) {
		for ( search = addonPaks; search; search = search->next ) {
			assert( search->pack && search->pack->addon );
			if ( search->pack->checksum == checksum ) {
				return search->pack;
			}
		}
	}
	return nullptr;
}

/*
===============
anFileSystem::ValidateDownloadPakForChecksum
===============
*/
int anFileSystem::ValidateDownloadPakForChecksum( int checksum, char path[ MAX_STRING_CHARS ], bool isBinary ) {
	int			i;
	anStringList	testList;
	anStr		name;
	anStr		relativePath;
	bool		pakBinary;
	pK5Nb_t		*pak = GetPackForChecksum( checksum );

	if ( !pak ) {
		return 0;
	}

	// validate this pak for a potential download
	// ignore pak*.pk5 for download. those are reserved to distribution and cannot be downloaded
	name = pak->pakFileName;
	name.StripPath();
	if ( strstr( name.c_str(), "pak" ) == name.c_str() ) {
		common->DPrintf( "%s is not a donwloadable pak\n", pak->pakFileName.c_str() );
		return 0;
	}
	// check the pakFileStatus
	// a pure server sets the pakFileStatus flag when starting the game
	assert( pak->pakFileStatus != BINARY_UNKNOWN );
	pakBinary = ( pak->pakFileStatus == BINARY_YES ) ? true : false;
	if ( isBinary != pakBinary ) {
		common->DPrintf( "%s pakFileStatus flag mismatch\n", pak->pakFileName.c_str() );
		return 0;
	}

	// extract a path that includes the fs_game: != OSPathToRelativePath
	testList.Append( fs_savepath.GetString() );
	testList.Append( fs_devpath.GetString() );
	testList.Append( fs_basepath.GetString() );
	testList.Append( fs_cdpath.GetString() );
	for ( i = 0; i < testList.Num(); i ++ ) {
		if ( testList[i].Length() && !testList[i].Icmpn( pak->pakFileName, testList[i].Length() ) ) {
			relativePath = pak->pakFileName.c_str() + testList[i].Length() + 1;
			break;
		}
	}
	if ( i == testList.Num() ) {
		common->Warning( "anFileSystem::ValidateDownloadPak: failed to extract relative path for %s", pak->pakFileName.c_str() );
		return 0;
	}
	anStr::Copynz( path, relativePath, MAX_STRING_CHARS );
	return pak->length;
}

/*
=====================
anFileSystem::ClearPureChecksums
=====================
*/
void anFileSystem::ClearPureChecksums( void ) {
	common->DPrintf( "Cleared pure server lock\n" );
	serverPaks.Clear();
}

/*
=====================
anFileSystem::SetPureServerChecksums
set the pure paks according to what the server asks
if that's not possible, identify why and build an answer
can be:
  loadedFileFromDir - some files were loaded from directories instead of paks (a restart in pure pak-only is required)
  missing/wrong checksums - some pak files would need to be installed/updated (downloaded for instance)
  some pak files currently referenced are not referenced by the server
  wrong order - if the pak order doesn't match, means some stuff could have been loaded from somewhere else
server referenced files are prepended to the list if possible ( that doesn't break pureness )
DLL:
  the checksum of the pak containing the DLL is maintained seperately, the server can send different replies by OS
=====================
*/
fsPureReply_t anFileSystem::SetPureServerChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int _gamePakChecksum, int missingChecksums[ MAX_PURE_PAKS ], int *missingGamePakChecksum ) {
	pK5Nb_t			*pack;
	int				i, j, imissing;
	bool			success = true;
	bool			canPrepend = true;
	char			dllName[MAX_OSPATH];
	int				dllHash;
	fileInPack_t *	pakFile;

	sys->DLL_GetFileName( "game", dllName, MAX_OSPATH );
	dllHash = HashFileName( dllName );

	imissing = 0;
	missingChecksums[0] = 0;
	assert( missingGamePakChecksum );
	*missingGamePakChecksum = 0;

	if ( pureChecksums[0] == 0 ) {
		ClearPureChecksums();
		return PURE_OK;
	}

	if ( !serverPaks.Num() ) {
		// there was no pure lockdown yet - lock to what we already have
		UpdatePureServerChecksums();
	}
	i = 0; j = 0;
	while ( pureChecksums[i] ) {
		if ( j < serverPaks.Num() && serverPaks[ j ]->checksum == pureChecksums[i] ) {
			canPrepend = false; // once you start matching into the list there is no prepending anymore
			i++; j++; // the pak is matched, is in the right order, continue..
		} else {
			pack = GetPackForChecksum( pureChecksums[i], true );
			if ( pack && pack->addon && !pack->pakAddonSearch ) {
				// this is an addon pack, and it's not on our current search list
				// setting success to false meaning that a restart including this addon is required
				if ( fs_debug.GetBool() ) {
					common->Printf( "pak %s checksumed 0x%x is on addon list. Restart required.\n", pack->pakFileName.c_str(), pack->checksum );
				}
				success = false;
			}
			if ( pack && pack->isNew ) {
				// that's a downloaded pack, we will need to restart
				if ( fs_debug.GetBool() ) {
					common->Printf( "pak %s checksumed 0x%x is a newly downloaded file. Restart required.\n", pack->pakFileName.c_str(), pack->checksum );
				}
				success = false;
			}
			if ( pack ) {
				if ( canPrepend ) {
					// we still have a chance
					if ( fs_debug.GetBool() ) {
						common->Printf( "prepend pak %s checksumed 0x%x at index %d\n", pack->pakFileName.c_str(), pack->checksum, j );
					}
					// NOTE: there is a light possibility this adds at the end of the list if UpdatePureServerChecksums didn't set anything
					serverPaks.Insert( pack, j );
					i++; j++; // continue..
				} else {
					success = false;
					if ( fs_debug.GetBool() ) {
						// verbose the situation
						if ( serverPaks.Find( pack ) ) {
							common->Printf( "pak %s checksumed 0x%x is in the pure list at wrong index. Current index is %d, found at %d\n", pack->pakFileName.c_str(), pack->checksum, j, serverPaks.FindIndex( pack ) );
						} else {
							common->Printf( "pak %s checksumed 0x%x can't be added to pure list because of search order\n", pack->pakFileName.c_str(), pack->checksum );
						}
					}
					i++; // advance server checksums only
				}
			} else {
				// didn't find a matching checksum
				success = false;
				missingChecksums[ imissing++ ] = pureChecksums[i];
				missingChecksums[ imissing ] = 0;
				if ( fs_debug.GetBool() ) {
					common->Printf( "checksum not found - 0x%x\n", pureChecksums[i] );
				}
				i++; // advance the server checksums only
			}
		}
	}
	while ( j < serverPaks.Num() ) {
		success = false; // just in case some extra pak files are referenced at the end of our local list
		if ( fs_debug.GetBool() ) {
			common->Printf( "pak %s checksumed 0x%x is an extra reference at the end of local pure list\n", serverPaks[ j ]->pakFileName.c_str(), serverPaks[ j ]->checksum );
		}
		j++;
	}

	// DLL checksuming
	if ( !_gamePakChecksum ) {
		// server doesn't have knowledge of code we can use ( OS issue )
		return PURE_NODLL;
	}
	assert( engineDLLChecksum );
#if ARC_FAKE_PURE
	enginePakChecksum = _gamePakChecksum;
#endif
	if ( _gamePakChecksum != enginePakChecksum ) {
		// current DLL is wrong, search for a pak with the approriate checksum
		// ( search all paks, the pure list is not relevant here )
		pack = GetPackForChecksum( _gamePakChecksum );
		if ( !pack ) {
			if ( fs_debug.GetBool() ) {
				common->Printf( "missing the game code pak ( 0x%x )\n", _gamePakChecksum );
			}
			// if there are other paks missing they have also been marked above
			*missingGamePakChecksum = _gamePakChecksum;
			return PURE_MISSING;
		}
		// if assets paks are missing, don't try any of the DLL restart / NODLL
		if ( imissing ) {
			return PURE_MISSING;
		}
		// we have a matching pak
		if ( fs_debug.GetBool() ) {
			common->Printf( "server's game code pak candidate is '%s' ( 0x%x )\n", pack->pakFileName.c_str(), pack->checksum );
		}
		// make sure there is a valid DLL for us
		if ( pack->hashPaKTable[ dllHash ] ) {
			for ( pakFile = pack->hashPaKTable[ dllHash ]; pakFile; pakFile = pakFile->next ) {
				if ( !CompareFileName( pakFile->name, dllName ) ) {
					enginePakChecksum = _gamePakChecksum;		// this will be used to extract the DLL in pure mode FindDLL
					return PURE_RESTART;
				}
			}
		}
		common->Warning( "media is misconfigured. server claims pak '%s' ( 0x%x ) has media for us, but '%s' is not found\n", pack->pakFileName.c_str(), pack->checksum, dllName );
		return PURE_NODLL;
	}

	// we reply to missing after DLL check so it can be part of the list
	if ( imissing ) {
		return PURE_MISSING;
	}

	// one last check
	if ( loadedFileFromDir ) {
		success = false;
		if ( fs_debug.GetBool() ) {
			common->Printf( "SetPureServerChecksums: there are files loaded from dir\n" );
		}
	}
	return ( success ? PURE_OK : PURE_RESTART );
}

/*
=====================
anFileSystem::GetPureServerChecksums
=====================
*/
void anFileSystem::GetPureServerChecksums( int checksums[ MAX_PURE_PAKS ], int OS, int *_gamePakChecksum ) {
	for ( int i = 0; i < serverPaks.Num(); i++ ) {
		checksums[i] = serverPaks[i]->checksum;
	}
	checksums[i] = 0;
	if ( _gamePakChecksum ) {
		if ( OS >= 0 ) {
			*_gamePakChecksum = enginePakForOS[ OS ];
		} else {
			*_gamePakChecksum = enginePakChecksum;
		}
	}
}

/*
=====================
anFileSystem::SetRestartChecksums
=====================
*/
void anFileSystem::SetRestartChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int enginePakChecksum ) {
	restartChecksums.Clear();
	int i = 0;
	while ( pureChecksums[i] ) {
		pK5Nb_t *pack = GetPackForChecksum( pureChecksums[i], true );
		if ( !pack ) {
			common->FatalError( "SetRestartChecksums failed: no pak for checksum 0x%x\n", pureChecksums[i] );
		}
		if ( pack->addon && addonChecksums.FindIndex( pack->checksum ) < 0 ) {
			// can't mark it pure if we're not even gonna search it :-)
			addonChecksums.Append( pack->checksum );
		}
		restartChecksums.Append( pureChecksums[i] );
		i++;
	}
	restartGamePakChecksum = enginePakChecksum;
}

/*
================
anFileSystem::Init

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void anFileSystem::Init( void ) {
	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	common->StartupVariable( "fs_basepath", false );
	common->StartupVariable( "fs_savepath", false );
	common->StartupVariable( "fs_cdpath", false );
	common->StartupVariable( "fs_devpath", false );
	common->StartupVariable( "fs_game", false );
	common->StartupVariable( "fs_game_base", false );
	common->StartupVariable( "fs_copyfiles", false );
	common->StartupVariable( "fs_restrict", false );
	common->StartupVariable( "fs_searchAddons", false );

	if ( fs_basepath.GetString()[0] == '\0' ) {
		fs_basepath.SetString( Sys_DefaultBasePath() );
	}
	if ( fs_savepath.GetString()[0] == '\0' ) {
		fs_savepath.SetString( Sys_DefaultSavePath() );
	}
	if ( fs_cdpath.GetString()[0] == '\0' ) {
		fs_cdpath.SetString( Sys_DefaultCDPath() );
	}

	if ( fs_devpath.GetString()[0] == '\0' ) {
#ifdef WIN32
		fs_devpath.SetString( fs_cdpath.GetString()[0] ? fs_cdpath.GetString() : fs_basepath.GetString() );
#else
		fs_devpath.SetString( fs_savepath.GetString() );
#endif
	}

	// try to start up normally
	Startup();

	// see if we are going to allow add-ons
	SetRestrictions();

	// spawn a thread to handle background file reads
	StartBackgroundDownloadThread();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	// Dedicated servers can run with no outside files at all
	if ( ReadFile( "default.cfg", nullptr, nullptr ) <= 0 ) {
		common->FatalError( "Couldn't load default.cfg" );
	}
}

/*
================
anFileSystem::Restart
================
*/
void anFileSystem::Restart( void ) {
	// free anything we currently have loaded
	Shutdown( true );
	// clear pak references
	//FS_ClearPakReferences( 0 );
	Startup();
	// see if we are going to allow add-ons
	SetRestrictions();
	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( ReadFile( "default.cfg", nullptr, nullptr ) <= 0 ) {
		common->FatalError( "Couldn't load default.cfg" );
	}
}

/*
================
anFileSystem::Shutdown

Frees all resources and closes all files
================
*/
void anFileSystem::Shutdown( bool reloading ) {
	searchPath_t *sp, *next, *loop;

	engineFolder.Clear();

	serverPaks.Clear();
	if ( !reloading ) {
		restartChecksums.Clear();
		addonChecksums.Clear();
	}
	loadedFileFromDir = false;
	engineDLLChecksum = 0;
	enginePakChecksum = 0;

	ClearDirCache();

	// free everything - loop through searchPaths and addonPaks
	for ( loop = searchPaths; loop; loop == searchPaths ? loop = addonPaks : loop = nullptr ) {
		for ( sp = loop; sp; sp = next ) {
			next = sp->next;
			if ( sp->pack ) {
				PAK_Close( sp->pack->handle );
				delete [] sp->pack->buildBuffer;
				if ( sp->pack->pakInfo ) {
					sp->pack->pakInfo->mapDecls.DeleteContents( true );
					delete sp->pack->pakInfo;
				}
				delete sp->pack;
			}
			if ( sp->dir ) {
				delete sp->dir;
			}
			delete sp;
		}
	}

	// any FS_ calls will now be an error until reinitialized
	searchPaths = nullptr;
	addonPaks = nullptr;

	cmdSystem->RemoveCommand( "path" );
	cmdSystem->RemoveCommand( "dir" );
	cmdSystem->RemoveCommand( "dirtree" );
	cmdSystem->RemoveCommand( "touchFile" );

	mapDict.Clear();
}

/*
================
anFileSystem::IsInitialized
================
*/
bool anFileSystem::IsInitialized( void ) const {
	return ( searchPaths != nullptr );
}

/*
================
anFileSystem::IsInitialized

Enables/Disables quiet mode.
================
*/
void anFileSystem::SetQuietMode( bool enable ) {
	assert( 0 );
}

/*
================
anFileSystem::IsInitialized

Returns quiet mode status.
================
*/
bool anFileSystem::GetQuietMode( void ) const {
	assert( 0 );
}

/*
=================================================================================

Opening files

=================================================================================
*/

/*
===========
anFileSystem::FileAllowedFromDir
===========
*/
bool anFileSystem::FileAllowedFromDir( const char *path ) {
	unsigned int l = strlen( path );

	if ( !strcmp( path + l - 4, ".cfg" )		// for config files
		|| !strcmp( path + l - 4, ".dat" )		// for journal files
		|| !strcmp( path + l - 4, ".dll" )		// dynamic modules are handled a different way for pure
		|| !strcmp( path + l - 3, ".so" )
		|| ( l > 6 && !strcmp( path + l - 6, ".dylib" ) )
		|| ( l > 10 && !strcmp( path + l - 10, ".scriptcfg" ) )	// configuration script, such as map cycle
#if ARC_PURE_ALLOWDDS
		 || !strcmp( path + l - 4, ".dds" )
#endif
		 ) {
		// note: cd and xp keys, as well as config.spec are opened through an explicit OS path and don't hit this
		return true;
	}
	// saved games
	if ( strstr( path, "saved" ) == path &&
		( !strcmp( path + l - 4, ".tga" ) || !strcmp( path + l -4, ".txt" ) || !strcmp( path + l - 5, ".save" ) ) ) {
		return true;
	}
	// snapshots/screen shots
	if ( strstr( path, "snapshots" ) == path && !strcmp( path + l - 4, ".tga" ) ) {
		return true;
	}
	// objective tgas
	if ( strstr( path, "maps/game" ) == path && 
		!strcmp( path + l - 4, ".tga" ) ) {
		return true;
	}
	// splash screens extracted from addons
	if ( strstr( path, "guis/splash/addon" ) == path &&
		 !strcmp( path + l -4, ".tga" ) ) {
		return true;
	}

	return false;
}

/*
===========
anFileSystem::GetPackStatus
===========
*/
pureStatus_t anFileSystem::GetPackStatus( pK5Nb_t *pak ) {
	int				l, hashindex;
	bool			abrt;
	anStr		name;

	if ( pak->pureStatus != PURE_UNKNOWN ) {
		return pak->pureStatus;
	}

	// check content for PURE_NEVER
	int i = 0;
	fileInPack_t *file = pak->buildBuffer;
	for ( hashindex = 0; hashindex < FILE_HASH_SIZE; hashindex++ ) {
		abrt = false;
		file = pak->hashPaKTable[ hashindex ];
		while ( file ) {
			abrt = true;
			l = file->name.Length();
			for ( int j = 0; pureExclusions[j].func != nullptr; j++ ) {
				if ( pureExclusions[j].func( pureExclusions[j], l, file->name ) ) {
					abrt = false;
					break;
				}
			}
			if ( abrt ) {
				common->DPrintf( "pak '%s' candidate for pure: '%s'\n", pak->pakFileName.c_str(), file->name.c_str() );
				break;
			}
			file = file->next;
			i++;
		}
		if ( abrt ) {
			break;
		}
	}
	if ( i == pak->numFiles ) {
		pak->pureStatus = PURE_NEVER;
		return PURE_NEVER;
	}

	// check pak name for PURE_ALWAYS
	pak->pakFileName.ExtractFileName( name );
	if ( !name.IcmpPrefixPath( "pak" ) ) {
		pak->pureStatus = PURE_ALWAYS;
		return PURE_ALWAYS;
	}

	pak->pureStatus = PURE_NEUTRAL;
	return PURE_NEUTRAL;
}

//static constexpr unsigned INDEX_OF_READFILE = 29;
//static constexpr unsigned INDEX_OF_OPENFILEREAD = 37;

/*
===========
anFileSystem::ReadFileFromZip
===========
*/
anCompressedArchive * anFileSystem::ReadFileFromZip( pK5Nb_t *pak, fileInPack_t *pakFile, const char *relativePath ) {
	pKFile_s *			zfi;
	FILE *			fp;
	anCompressedArchive *file = new anCompressedArchive();

	// open a new file on the pakfile
	file->z = PAK_ExplicitOpen( pak->pakFileName, pak->handle );
	if ( file->z == nullptr ) {
		common->FatalError( "Couldn't reopen %s", pak->pakFileName.c_str() );
	}
	file->name = relativePath;
	file->fullPath = pak->pakFileName + "/" + relativePath;
	zfi = (pKFile_s *)file->z;
	// in case the file was new
	fp = zfi->file;
	// set the file position in the zip file (also sets the current file info)
	PAK_SetFileDataLocation( pak->handle, pakFile->pos );
	// copy the file info into the unzip structure
	memcpy( zfi, pak->handle, sizeof(pKFile_s) );
	// we copy this back into the structure
	zfi->file = fp;
	// open the file in the zip
	PAK_OpenCurrentFile( file->z );
	file->zipFilePos = pakFile->pos;
	file->fileSize = zfi->currentFileInfo.uncompressedSize;
	return file;
}

/*
===========
anFileSystem::OpenFileReadFlags

Finds the file in the search path, following search flag recommendations
Returns filesize and an open FILE pointer.
Used for streaming data out of either a
separate file or a ZIP file.
===========
*/
anFile *anFileSystem::OpenFileReadFlags( const char *relativePath, int searchFlags, pK5Nb_t **foundInPak, bool allowCopyFiles, const char *gamedir ) {
	searchPath_t *	search;
	anStr			netpath;
	pK5Nb_t *		pak;
	fileInPack_t *	pakFile;
	directory_t *	dir;
	long			hash;
	FILE *			fp;
	
	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	if ( !relativePath ) {
		common->FatalError( "anFileSystem::OpenFileRead: nullptr 'relativePath' parameter passed\n" );
	}

	if ( foundInPak ) {
		*foundInPak = nullptr;
	}

	// qpaths are not supposed to have a leading slash
	if ( relativePath[0] == '/' || relativePath[0] == '\\' ) {
		relativePath++;
	}

	// make absolutely sure that it can't back up the path.
	// The searchpaths do guarantee that something will always
	// be prepended, so we don't need to worry about "c:" or "//limbo" 
	if ( strstr( relativePath, ".." ) || strstr( relativePath, "::" ) ) {
		return nullptr;
	}

	// make sure the doomkey file is only readable by game at initialization
	// any other time the key should only be accessed in memory using the provided functions
	if (  || relativePath[0] == '\0' && common->IsInitialized() && ( anStr::Icmp( relativePath, CDKEY_FILE ) == 0 || anStr::Icmp( relativePath, XPKEY_FILE ) == 0 ) ) {
		return nullptr;
	}

	// search through the path, one element at a time
	hash = HashFileName( relativePath );

	for ( search = searchPaths; search; search = search->next ) {
		if ( search->dir && ( searchFlags & FSFLAG_SEARCH_DIRS ) ) {
			// check a file in the directory tree
			// if we are running restricted, the only files we
			// will allow to come from the directory are .cfg files
			if ( fs_restrict.GetBool() || serverPaks.Num() ) {
				if ( !FileAllowedFromDir( relativePath ) ) {
					continue;
				}
			}

			dir = search->dir;

			if ( gamedir && strlen( gamedir ) ) {
				if ( dir->gamedir != gamedir ) {
					continue;
				}
			}

			netpath = BuildOSPath( dir->path, dir->gamedir, relativePath );
			fp = OpenOSFileCorrectName( netpath, "rb" );
			if ( !fp ) {
				continue;
			}

			anFilePermanent *file = new anFilePermanent();
			file->o = fp;
			file->name = relativePath;
			file->fullPath = netpath;
			file->mode = ( 1 << FS_READ );
			file->fileSize = DirectFileLength( file->o );
			if ( fs_debug.GetInteger() ) {
				common->Printf( "anFileSystem::OpenFileRead: %s (found in '%s/%s')\n", relativePath, dir->path.c_str(), dir->gamedir.c_str() );
			}

			if ( !loadedFileFromDir && !FileAllowedFromDir( relativePath ) ) {
				if ( restartChecksums.Num() ) {
					common->FatalError( "'%s' loaded from directory: Failed to restart with pure mode restrictions for server connect", relativePath );
				}
				common->DPrintf( "filesystem: switching to pure mode will require a restart. '%s' loaded from directory.\n", relativePath );
				loadedFileFromDir = true;
			}

			// if fs_copyfiles is set
			if ( allowCopyFiles && fs_copyfiles.GetInteger() ) {
				anStr copypath;
				anStr name;
				copypath = BuildOSPath( fs_savepath.GetString(), dir->gamedir, relativePath );
				netpath.ExtractFileName( name );
				copypath.StripFilename();
				copypath += PATHSEPERATOR_STR;
				copypath += name;

				bool isFromCDPath = !dir->path.Cmp( fs_cdpath.GetString() );
				bool isFromSavePath = !dir->path.Cmp( fs_savepath.GetString() );
				bool isFromBasePath = !dir->path.Cmp( fs_basepath.GetString() );

				switch ( fs_copyfiles.GetInteger() ) {
					case 1:
						// copy from cd path only
						if ( isFromCDPath ) {
							CopyFile( netpath, copypath );
						}
						break;
					case 2:
						// from cd path + timestamps
						if ( isFromCDPath ) {
							CopyFile( netpath, copypath );
						} else if ( isFromSavePath || isFromBasePath ) {
							anStr sourcepath;
							sourcepath = BuildOSPath( fs_cdpath.GetString(), dir->gamedir, relativePath );
							FILE *f1 = OpenOSFile( sourcepath, "r" );
							if ( f1 ) {
								ARC_TIME_T t1 = Sys_FileTimeStamp( f1 );
								fclose( f1 );
								FILE *f2 = OpenOSFile( copypath, "r" );
								if ( f2 ) {
									ARC_TIME_T t2 = Sys_FileTimeStamp( f2 );
									fclose( f2 );
									if ( t1 > t2 ) {
										CopyFile( sourcepath, copypath );
									}
								}
							}
						}
						break;
					case 3:
						if ( isFromCDPath || isFromBasePath ) {
							CopyFile( netpath, copypath );
						}
						break;
					case 4:
						if ( isFromCDPath && !isFromBasePath ) {
							CopyFile( netpath, copypath );
						}
						break;
				}
			}

			return file;
		} else if ( search->pack && ( searchFlags & FSFLAG_SEARCH_PAKS ) ) {
			if ( !search->pack->hashPaKTable[hash] ) {
				continue;
			}

			// disregard if it doesn't match one of the allowed pure pak files
			if ( serverPaks.Num() ) {
				GetPackStatus( search->pack );
				if ( search->pack->pureStatus != PURE_NEVER && !serverPaks.Find( search->pack ) ) {
					continue; // not on the pure server pak list
				}
			}

			// look through all the pak file elements
			pak = search->pack;

			if ( searchFlags & FSFLAG_BINARY_ONLY && fs_usebinarypak.GetBool() ) {
				// check to see if this pak is binary
				if ( pak->pakFileStatus == BINARY_UNKNOWN ) {
					fileInPack_t *pakFile;				
					int confHash = HashFileName( BINARY_CONFIG );
					pak->pakFileStatus = BINARY_NO;
					for ( pakFile = search->pack->hashPaKTable[confHash]; pakFile; pakFile = pakFile->next ) {
						if ( !CompareFileName( pakFile->name, BINARY_CONFIG ) ) {
							pak->pakFileStatus = BINARY_YES;
							break;
						}
					}
				}
				if ( pak->pakFileStatus == BINARY_NO && fs_usebinarypak.GetBool() ) {
					common->FatalError( "Binary pak '%s' not found in '%s'!\n", BINARY_CONFIG, pak->pakFileName.c_str() );
					continue; // not a pakFileStatus pak, skip
				}
			}

			for ( pakFile = pak->hashPaKTable[hash]; pakFile; pakFile = pakFile->next ) {
				// case and separator insensitive comparisons
				if ( !CompareFileName( pakFile->name, relativePath ) ) {
					anCompressedArchive *file = ReadFileFromZip( pak, pakFile, relativePath );
					if ( foundInPak ) {
						*foundInPak = pak;
					}

					if ( !pak->referenced && !( searchFlags & FSFLAG_PURE_NOREF ) ) {
						// mark this pak referenced
						if ( fs_debug.GetInteger() ) {
							common->Printf( "anFileSystem::OpenFileRead: %s -> adding %s to referenced paks\n", relativePath, pak->pakFileName.c_str() );
						}
						pak->referenced = true;
					}

					if ( fs_debug.GetInteger() ) {
						common->Printf( "anFileSystem::OpenFileRead: %s (found in '%s')\n", relativePath, pak->pakFileName.c_str() );
					}
					return file;
				}
			}
		}
	}

	if ( searchFlags & FSFLAG_SEARCH_ADDONS ) {
		for ( search = addonPaks; search; search = search->next ) {
			assert( search->pack );
			fileInPack_t	*pakFile;
			pak = search->pack;
			for ( pakFile = pak->hashPaKTable[hash]; pakFile; pakFile = pakFile->next ) {
				if ( !CompareFileName( pakFile->name, relativePath ) ) {
					anCompressedArchive *file = ReadFileFromZip( pak, pakFile, relativePath );
					if ( foundInPak ) {
						*foundInPak = pak;
					}
					// we don't toggle pure on paks found in addons - they can't be used without a reloadEngine anyway
					if ( fs_debug.GetInteger() ) {
						common->Printf( "anFileSystem::OpenFileRead: %s (found in addon pk5 '%s')\n", relativePath, search->pack->pakFileName.c_str() );
					}
					return file;
				}
			}
		}
	}
	
	if ( fs_debug.GetInteger() ) {
		common->Printf( "Can't find %s\n", relativePath );
	}
	
	return nullptr;
}

/*
===========
anFileSystem::OpenFileRead
===========
*/
anFile *anFileSystem::OpenFileRead( const char *relativePath, bool allowCopyFiles, const char *gamedir ) {
	return OpenFileReadFlags( relativePath, FSFLAG_SEARCH_DIRS | FSFLAG_SEARCH_PAKS, nullptr, allowCopyFiles, gamedir );
}

/*
===========
anFileSystem::OpenFileWrite
===========
*/
anFile *anFileSystem::OpenFileWrite( const char *relativePath, const char *basePath ) {
	const char *path;
	anStr OSpath;
	anFilePermanent *f;

	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}

	OSpath = BuildOSPath( path, engineFolder, relativePath );

	if ( fs_debug.GetInteger() ) {
		common->Printf( "anFileSystem::OpenFileWrite: %s\n", OSpath.c_str() );
	}

	// if the dir we are writing to is in our current list, it will be outdated
	// so just flush everything
	ClearDirCache();

	common->DPrintf( "writing to: %s\n", OSpath.c_str() );
	CreateOSPath( OSpath );

	f = new anFilePermanent();
	f->o = OpenOSFile( OSpath, "wb" );
	if ( !f->o ) {
		delete f;
		return nullptr;
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
anFileSystem::OpenExplicitFileRead
===========
*/
anFile *anFileSystem::OpenExplicitFileRead( const char *OSPath ) {
	anFilePermanent *f;

	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	if ( fs_debug.GetInteger() ) {
		common->Printf( "anFileSystem::OpenExplicitFileRead: %s\n", OSPath );
	}

	common->DPrintf( "anFileSystem::OpenExplicitFileRead - reading from: %s\n", OSPath );

	f = new anFilePermanent();
	f->o = OpenOSFile( OSPath, "rb" );
	if ( !f->o ) {
		delete f;
		return nullptr;
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
anFileSystem::OpenExplicitFileWrite
===========
*/
anFile *anFileSystem::OpenExplicitFileWrite( const char *OSPath ) {
	anFilePermanent *f;

	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	if ( fs_debug.GetInteger() ) {
		common->Printf( "anFileSystem::OpenExplicitFileWrite: %s\n", OSPath );
	}

	common->DPrintf( "writing to: %s\n", OSPath );
	CreateOSPath( OSPath );

    //uniquePtr<anFilePermanent> f = uniquePtr<anFilePermanent>();
	f = new anFilePermanent();
	f->o = OpenOSFile( OSPath, "wb" );
	if ( !f->o ) {
		delete f;
		return nullptr;
	}
	f->name = OSPath;
	f->fullPath = OSPath;
	f->mode = ( 1 << FS_WRITE );
	f->handleSync = false;
	f->fileSize = 0;

	return f;//f.release();
}

/*
===========
anFileSystem::OpenFileAppend
===========
*/
anFile *anFileSystem::OpenFileAppend( const char *relativePath, bool sync, const char *basePath ) {
	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}

	const char *path = cvarSystem->GetCVarString( basePath );
	if ( !path[0] ) {
		path = fs_savepath.GetString();
	}

	anStr OSpath = BuildOSPath( path, engineFolder, relativePath );
	CreateOSPath(OSpath);

	if ( fs_debug.GetInteger() ) {
		common->Printf( "anFileSystem::OpenFileAppend: %s\n", OSpath.c_str() );
	}

	anFilePermanent *f = new anFilePermanent();
	f->o = OpenOSFile( OSpath, "ab" );
	if ( !f->o ) {
		delete f;
		return nullptr;
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
anFileSystem::OpenFileByMode
================
*/
anFile *anFileSystem::OpenFileByMode( const char *relativePath, fsMode_t mode ) {
    switch ( mode ) {
        case FS_READ:
            return OpenFileRead( relativePath );
        case FS_WRITE:
            return OpenFileWrite( relativePath );
        case FS_APPEND:
            return OpenFileAppend( relativePath, true );
        default:
			//CloseFile();
			common->FatalError( "anFileSystem::OpenFileByMode: bad mode" );
			return nullptr;
    }
}

/*
==============
anFileSystem::CloseFile
==============
*/
void anFileSystem::CloseFile( anFile *f ) {
	if ( !searchPaths ) {
		common->FatalError( "[FileSystem] called without initialization\n" );
	}
	delete f;
}

int anFileSystem::FTell( fileHandle_t f ) {
	int pos;
	if ( fsh[f].zipFile == true ) {
		pos = unztell( fsh[f].handleFiles.file.z );
	} else {
		pos = ftell( fsh[f].handleFiles.file.o );
	}
	return pos;
}
/*
=================================================================================

back ground loading

=================================================================================
*/

/*
=================
anFileSystem::CurlWriteFunction
=================
*/
size_t anFileSystem::CurlWriteFunction( void *ptr, size_t size, size_t nmemb, void *stream ) {
	backgroundDownload_t *bgl = (backgroundDownload_t *)stream;
	if ( !bgl->f ) {
		return size * nmemb;
	}
	#ifdef _WIN32
		return _write( static_cast<anFilePermanent*>(bgl->f)->GetFilePtr()->_file, ptr, size * nmemb );
	#else
		return fwrite( ptr, size, nmemb, static_cast<anFilePermanent*>(bgl->f)->GetFilePtr() );
	#endif
}

/*
=================
anFileSystem::CurlProgressFunction
=================
*/
int anFileSystem::CurlProgressFunction( void *clientp, double dltotal, double dlnow, double ultotal, double ulnow ) {
	backgroundDownload_t *bgl = (backgroundDownload_t *)clientp;
	if ( bgl->url.status == DL_ABORTING ) {
		return 1;
	}
	bgl->url.dltotal = dltotal;
	bgl->url.dlnow = dlnow;
	return 0;
}

/*
===================
BackgroundDownload

Reads part of a file from a background thread.
===================
*/
dword BackgroundDownloadThread( void *parms ) {
	while( 1 ) {
		Sys_EnterCriticalSection();
		backgroundDownload_t	*bgl = fileSystem.backgroundDownloads;
		if ( !bgl ) {
			Sys_LeaveCriticalSection();
			Sys_WaitForEvent();
			continue;
		}
		// remove this from the list
		fileSystem.backgroundDownloads = bgl->next;
		Sys_LeaveCriticalSection();

		bgl->next = nullptr;

		if ( bgl->opcode == DLTYPE_FILE ) {
			// use the low level read function, because fread may allocate memory
			#if defined(WIN32)
				_read( static_cast<anFilePermanent*>(bgl->f)->GetFilePtr()->_file, bgl->file.buffer, bgl->file.length );
			#else
				fread(  bgl->file.buffer, bgl->file.length, 1, static_cast<anFilePermanent*>(bgl->f)->GetFilePtr() );
			#endif
			bgl->completed = true;
		} else {
#if ARC_ENABLE_CURL
			// DLTYPE_URL
			// use a local buffer for curl error since the size define is local
			char error_buf[ CURL_ERROR_SIZE ];
			bgl->url.dlerror[0] = '\0';
			CURL *session = curl_easy_init();
			CURLcode ret;
			if ( !session ) {
				bgl->url.dlstatus = CURLE_FAILED_INIT;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = curl_easy_setopt( session, CURLOPT_ERRORBUFFER, error_buf );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = curl_easy_setopt( session, CURLOPT_URL, bgl->url.url.c_str() );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = curl_easy_setopt( session, CURLOPT_FAILONERROR, 1 );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = curl_easy_setopt( session, CURLOPT_WRITEFUNCTION, anFileSystem::CurlWriteFunction );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = curl_easy_setopt( session, CURLOPT_WRITEDATA, bgl );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = curl_easy_setopt( session, CURLOPT_NOPROGRESS, 0 );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = curl_easy_setopt( session, CURLOPT_PROGRESSFUNCTION, anFileSystem::CurlProgressFunction );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			ret = curl_easy_setopt( session, CURLOPT_PROGRESSDATA, bgl );
			if ( ret ) {
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			bgl->url.dlnow = 0;
			bgl->url.dltotal = 0;
			bgl->url.status = DL_INPROGRESS;
			ret = curl_easy_perform( session );
			if ( ret ) {
				Sys_Printf( "curl_easy_perform failed: %s\n", error_buf );
				anStr::Copynz( bgl->url.dlerror, error_buf, MAX_STRING_CHARS );
				bgl->url.dlstatus = ret;
				bgl->url.status = DL_FAILED;
				bgl->completed = true;
				continue;
			}
			bgl->url.status = DL_DONE;
			bgl->completed = true;
#else
			bgl->url.status = DL_FAILED;
			bgl->completed = true;
#endif
		}
	}
	return 0;
}

/*
=================
anFileSystem::StartBackgroundReadThread
=================
*/
void anFileSystem::StartBackgroundDownloadThread() {
	if ( !backgroundThread.threadHandle ) {
		Sys_CreateThread( (xthread_t)BackgroundDownloadThread, nullptr, THREAD_NORMAL, backgroundThread, "backgroundDownload", g_threads, &g_thread_count );
		if ( !backgroundThread.threadHandle ) {
			common->Warning( "anFileSystem::StartBackgroundDownloadThread: failed" );
		}
	} else {
		common->Printf( "background thread already running\n" );
	}
}

/*
=================
anFileSystem::BackgroundDownload
=================
*/
void anFileSystem::BackgroundDownload( backgroundDownload_t *bgl ) {
	if ( bgl->opcode == DLTYPE_FILE ) {
		if ( dynamic_cast<anFilePermanent *>(bgl->f) ) {
			// add the bgl to the background download list
			Sys_EnterCriticalSection();
			bgl->next = backgroundDownloads;
			backgroundDownloads = bgl;
			Sys_TriggerEvent();
			Sys_LeaveCriticalSection();
		} else {
			// read zipped file directly
			bgl->f->Seek( bgl->file.position, FS_SEEK_SET );
			bgl->f->Read( bgl->file.buffer, bgl->file.length );
			bgl->completed = true;
		}
	} else {
		Sys_EnterCriticalSection();
		bgl->next = backgroundDownloads;
		backgroundDownloads = bgl;
		Sys_TriggerEvent();
		Sys_LeaveCriticalSection();
	}
}

const char *anFileSystem_local::CaseSearch( const char *inDir ) {
	const char *ret;

	// FIXME: go faster with a hash?
	for ( int i = 0; i < dir_case.Num(); i++ ) {
		if ( !dir_case[i].path.Cmp( inDir ) ) {
			ret = dir_case[i].OSpath.c_str();
			common->Printf( "index %d: '%s' matched as '%s'\n", i, inDir, ret );
			if ( ret[0] == '\0' ) {
				return nullptr;
			}
			return ret;
		}
	}
	casematch_t entry;
	entry.path = inDir;
	common->Printf( "CaseSearch not found: '%s'\n", inDir );
	// walk down the directory tree searching for a case insensitive match
	// use StripFilename to bust out the chunks
	anStringList dirs;
	anStringList entries;
	anStr walk_path = inDir;
	anStr current_dir;
	int list_ret;
	do {
		walk_path.ExtractFileName( current_dir );
		dirs.Append( current_dir );
		walk_path.StripFilename(); // this is double work
		common->Printf( "have walk_path: %s, current_dir: %s\n", walk_path.c_str(), current_dir.c_str() );
	} while ( walk_path.Length() && ( list_ret = Sys_ListFiles( walk_path.c_str(), "/", entries ) == -1 ) );
	// we have walked up to the first directory
	if ( list_ret == -1 ) {
		common->DPrintf( "WARNING: Failed to locate matching root directory for '%s'\n", inDir );
		dir_case.Append( entry );
		return nullptr;
	}
	// start walking down and doing matches
	bool bMatched;
	entry.OSpath = walk_path;
	for ( int i = dirs.Num()-1; i >= 0; i-- ) {
		common->Printf( "chunk: %s\n", dirs[i].c_str() );
		bMatched = false;
		for ( int j = 0; j < entries.Num(); j++ ) {
			common->Printf( "match %s and %s\n", dirs[i].c_str(), entries[j].c_str() );
			if ( !dirs[i].Icmp(entries[j] ) ) {
				common->Printf( "we have a match, add this to the path and go down\n" );
				bMatched = true;
				break; // NOTE: we could keep scanning and detect conflicts?
			}
		}
		// if we didn't match, abort
		if ( !bMatched ) {
			common->Printf( "no match\n" );
			entry.OSpath = "";
			dir_case.Append( entry );
			return nullptr;
		}
		entry.OSpath += PATHSEPERATOR_STR;
		entry.OSpath += entries[j];
		// get the directory list
		if ( Sys_ListFiles( entry.OSpath.c_str(), "/", entries ) == -1 ) {
			common->DPrintf( "WARNING: didn't find entries in '%s' after successful icase match\n", entry.OSpath.c_str() );
			entry.OSpath = "";
			dir_case.Append( entry );
			return nullptr;
		}
	}

	dir_case.Append( entry );
	ret = dir_case[ dir_case.Num() - 1 ].OSpath.c_str();
	common->Printf( "case matched '%s' as '%s'\n", inDir, ret );
	if ( ret[0] == '\0' ) {
		common->DPrintf( "WARNING: unexpected empty entry after successful case walk for '%s'\n", inDir );
		return nullptr;
	}
	return ret;
}

/*
=================
anFileSystem::PerformingCopyFiles
=================
*/
bool anFileSystem::PerformingCopyFiles( void ) const {
	return fs_copyfiles.GetInteger() > 0;
}

/*
=================
anFileSystem::FileExists
=================
*/
bool anFileSystem::FileExists( const char *relativePath ) {
	anFileScoped file( OpenFileRead( relativePath ) );
	//anFile *file = OpenFileRead( testpath, "" );

	if ( !file ) {
		//FClose( file );
		return false;
	}

	return true;
}

/*
=================
anFileSystem::FileExists

show and tell! if path and file exists
=================
*/
bool anFileSystem::FileExistsExplicit( const char *relativePath ) {
	anFileScoped file( OpenExplicitFileRead( relativePath ) );
	//return OpenExplicitFileRead(relativePath) != nullptr;
	if ( !file ) {
		return false;
	}

	return true;
}

/*
================
anFileSystem::GetTimestamp

The timestamp function takes thhe relativePath as input and returns
an unsigned integer representing the 'timestamp' of the file at the given path.
If the relativePath is nullptr/empty, it returns a predefined constant
value. Otherwise, it calls the ReadFile function passing the relativePath
and a pointer to the timestamp variable, and then returns the timestamp
value.
================
*/
unsigned int anFileSystem::GetTimestamp( const char *relativePath ) {
    if ( relativePath == nullptr || relativePath[0] == '\0' ) {
        return FILE_NOT_FOUND_TIMESTAMP;
    }

    unsigned int timestamp = 0;
    ReadFile( relativePath, nullptr, &timestamp );

    return timestamp;
}

/*
=================
anFileSystem::FindPakForFileChecksum
=================
*/
pK5Nb_t *anFileSystem::FindPakForFileChecksum( const char *relativePath, int findChecksum, bool bReference ) {
	searchPath_t	*search;
	pK5Nb_t			*pak;
	fileInPack_t	*pakFile;
	int				hash;
	assert( !serverPaks.Num() );
	hash = HashFileName( relativePath );
	for ( search = searchPaths; search; search = search->next ) {
		if ( search->pack && search->pack->hashPaKTable[ hash ] ) {
			pak = search->pack;
			for ( pakFile = pak->hashPaKTable[ hash ]; pakFile; pakFile = pakFile->next ) {
				if ( !CompareFileName( pakFile->name, relativePath ) ) {
					anCompressedArchive *file = ReadFileFromZip( pak, pakFile, relativePath );
					if ( findChecksum == GetFileChecksum( file ) ) {
						if ( fs_debug.GetBool() ) {
							common->Printf( "found '%s' with checksum 0x%x in pak '%s'\n", relativePath, findChecksum, pak->pakFileName.c_str() );
						}
						if ( bReference ) {
							pak->referenced = true;
							// FIXME: use dependencies for pak references
						}
						CloseFile( file );
						return pak;
					} else if ( fs_debug.GetBool() ) {
						common->Printf( "'%s' in .pK5nb '%s' has != checksum %x\n", relativePath, pak->pakFileName.c_str(), GetFileChecksum( file ) );
					}
					CloseFile( file );
				}
			}
		}
	}
	if ( fs_debug.GetBool() ) {
		common->Printf( "no pak file found for '%s' checksumed %x\n", relativePath, findChecksum );
	}
	return nullptr;
}

/*
=================
anFileSystem::GetFileChecksum
=================
*/
int anFileSystem::GetFileChecksum( anFile *file ) {
	int ret;
	file->Seek( 0, FS_SEEK_END );
	int len = file->Tell();
	file->Seek( 0, FS_SEEK_SET );
	byte *buf = (byte *)Mem_Alloc( len );
	if ( file->Read( buf, len ) != len ) {
		common->FatalError( "Short read in anFileSystem::GetFileChecksum()\n" );
	}
	ret = MD4_BlockChecksum( buf, len );
	Mem_Free( buf );
	return ret;
}

/*
=================
anFileSystem::FindDLL
=================
*/
void anFileSystem::FindDLL( const char *name, char _dllPath[ MAX_OSPATH ], bool updateChecksum ) {
	anFile			*dllFile = nullptr;
	char			dllName[MAX_OSPATH];
	anStr			dllPath;
	int				dllHash;
	pK5Nb_t *inPak;
	pK5Nb_t			*pak;
	fileInPack_t	*pakFile;	

	sys->DLL_GetFileName( name, dllName, MAX_OSPATH );
	dllHash = HashFileName( dllName );

#if ARC_FAKE_PURE
	if ( 1 ) {
#else
	if ( !serverPaks.Num() ) {
#endif
		// from executable directory first - this is handy for developement
		dllPath = Sys_EXEPath();
		dllPath.StripFilename();
		dllPath.AppendPath( dllName );
		dllFile = OpenExplicitFileRead( dllPath );
	}
	if ( !dllFile ) {
		if ( !serverPaks.Num() ) {
			// not running in pure mode, try to extract from a pak file first
			dllFile = OpenFileReadFlags( dllName, FSFLAG_SEARCH_PAKS | FSFLAG_PURE_NOREF | FSFLAG_BINARY_ONLY, &inPak );
			if ( dllFile ) {
				common->Printf( "found DLL in .pK5nb file: %s\n", dllFile->GetFullPath() );
				dllPath = RelativePathToOSPath( dllName, "fs_savepath" );
				CopyFile( dllFile, dllPath );
				CloseFile( dllFile );
				dllFile = OpenFileReadFlags( dllName, FSFLAG_SEARCH_DIRS );
				if ( !dllFile ) {
					common->Error( "DLL extraction to fs_savepath failed\n" );
				} else if ( updateChecksum ) {
					engineDLLChecksum = GetFileChecksum( dllFile );
					enginePakChecksum = inPak->checksum;
					updateChecksum = false;	// don't try again below
				}
			} else {
				// didn't find a source in a pak file, try in the directory
				dllFile = OpenFileReadFlags( dllName, FSFLAG_SEARCH_DIRS );
				if ( dllFile ) {
					if ( updateChecksum ) {
						engineDLLChecksum = GetFileChecksum( dllFile );
						// see if we can mark a pak file
						pak = FindPakForFileChecksum( dllName, engineDLLChecksum, false );
						pak ? enginePakChecksum = pak->checksum : enginePakChecksum = 0;
						updateChecksum = false;
					}
				}
			}
		} else {
			// we are in pure mode. this path to be reached only for game DLL situations
			// with a code pak checksum given by server
			assert( enginePakChecksum );
			assert( updateChecksum );
			pak = GetPackForChecksum( enginePakChecksum );
			if ( !pak ) {
				// not supposed to happen, bug in pure code?
				common->Warning( "FindDLL pure mode: Locating Failed .pK5nb not found ( 0x%x )\n", enginePakChecksum );
			} else {
				// extract and copy
				for ( pakFile = pak->hashPaKTable[dllHash]; pakFile; pakFile = pakFile->next ) {
					if ( !CompareFileName( pakFile->name, dllName ) ) {
						dllFile = ReadFileFromZip( pak, pakFile, dllName );
						common->Printf( "found DLL the Primary Engine .pK5nb file: %s\n", pak->pakFileName.c_str() );
						dllPath = RelativePathToOSPath( dllName, "fs_savepath" );
						CopyFile( dllFile, dllPath );
						CloseFile( dllFile );
						dllFile = OpenFileReadFlags( dllName, FSFLAG_SEARCH_DIRS );
						if ( !dllFile ) {
							common->Error( "DLL extraction to fs_savepath failed\n" );
						} else {
							engineDLLChecksum = GetFileChecksum( dllFile );
							updateChecksum = false;	// don't try again below
						}						
					}
				}
			}
		}
	}
	if ( updateChecksum ) {
		if ( dllFile ) {
			engineDLLChecksum = GetFileChecksum( dllFile );
		} else {
			engineDLLChecksum = 0;
		}
		enginePakChecksum = 0;
	}
	if ( dllFile ) {
		dllPath = dllFile->GetFullPath();
		CloseFile( dllFile );
		dllFile = nullptr;
	} else {
		dllPath = "";
	}
	anStr::snPrintf( _dllPath, MAX_OSPATH, dllPath.c_str() );
}

/*
================
anFileSystem::ClearDirCache
================
*/
void anFileSystem::ClearDirCache( void ) {
	dir_cache_index = 0;
	dir_cache_count = 0;
	for ( int i = 0; i < MAX_CACHED_DIRS; i++ ) {
		dir_cache[i].Clear();
	}
}

/*
===============
anFileSystem::MakeTemporaryFile
===============
*/
anFile * anFileSystem::MakeTemporaryFile( void ) {
	FILE *f = tmpfile();
	if ( !f ) {
		common->Warning( "[File-System]: System Failed to create temporary file %s", strerror( errno ) );
		return nullptr;
	}
	anFilePermanent *file = new anFilePermanent();
	file->o = f;
	file->name = "<tmpfile>";
	file->fullPath = "<tmpfile>";
	file->mode = ( 1 << FS_READ ) + ( 1 << FS_WRITE );
	file->fileSize = 0;
	return file;
}

/*
===============
anFileSystem::FindFile
===============
*/
 findFile_t anFileSystem::FindFile( const char *path, bool scheduleAddons ) {
	pK5Nb_t *pak;
	anFile *f = OpenFileReadFlags( path, FSFLAG_SEARCH_DIRS | FSFLAG_SEARCH_PAKS | FSFLAG_SEARCH_ADDONS, &pak );
	if ( !f ) {
		return FIND_NO;
	}
	if ( !pak ) {
		// found in FS, not even in paks
		return FIND_YES;
	}
	// marking addons for inclusion on reload - may need to do that even when already in the search path
	if ( scheduleAddons && pak->addon && addonChecksums.FindIndex( pak->checksum ) < 0 ) {
		addonChecksums.Append( pak->checksum );			
	}
	// an addon that's not on search list yet? that will require a restart
	if ( pak->addon && !pak->pakAddonSearch ) {
		delete f;
		return FIND_ADDON;
	}
	delete f;
	return FIND_YES;
}

/*
===============
anFileSystem::IsExtension
===============
*/
bool anFileSystem::IsExtension( const char *fileName, const char *ext, int nLength ) const {
	int extLength = anStr::Length( ext );

	// check the name and ext length
	if ( extLength > nLength ) {
		return false;
	}

	fileName += nLength - extLength;

	return !anStr::Icmp( fileName, ext );
}

/*
===============
anFileSystem::GetNumMaps

account for actual decls and for addon maps.

The Second loop adds additional information to the count of maps.
The first loop iterates over fileHandles and counts the number of
maps with the ".map" extension(perhaps decls and maps?).
 The second loop iterates over paks and checks if each pak file
 is an addon pak (addon = true). If so, it then iterates over
 hashPaKTable and counts the number of maps with the ".map" extension
 within that pak file.
===============
*/
int anFileSystem::GetNumMaps() {
	int numMaps = 0;
	for ( anFile *f : fileHandles ) {
		if ( IsExtension( f->GetName(), ".arcMap", 4 ) ) {
			numMaps++;
			for ( pK5Nb_t *pak : paks ) {
				if ( pak->addon ) {
					for ( pakFile_t *pakFile : pak->hashPaKTable ) {
						if ( IsExtension( pakFile->name, ".arcMap", 4 ) ) {
							numMaps++;
						}
					}
				}
				return numMaps;
			}

			searchPath_t *search = nullptr;
			int ret = declManager->GetNumDecls( DECL_MAPDEF );
		}
	}
	// add to this all addon decls - coming from all addon packs ( searched or not )
	for ( int i = 0; i < 2; i++ ) {
		if ( i == 0 ) {
			search = searchPaths;
		} else if ( i == 1 ) {
			search = addonPaks;
		}
		for ( ; search ; search = search->next ) {
			if ( !search->pack || !search->pack->addon || !search->pack->pakInfo ) {
				continue;
			}
			ret += search->pack->pakInfo->mapDecls.Num();
		}
	}
	return ret;
}

/*
===============
anFileSystem::GetMapDecl

retrieve the decl dictionary, add a 'path' value
===============
*/
const anDict *anFileSystem::GetMapDecl( int iDecl ) {
	int numDecls = declManager->GetNumDecls( DECL_MAPDEF );
	searchPath_t * search = nullptr;

	if ( iDecl < numDecls ) {
		const idDecl *mapDecl = declManager->DeclByIndex( DECL_MAPDEF, iDecl );
		const anDeclEntityDef *mapDef = static_cast<const anDeclEntityDef *>( mapDecl );
		if ( !mapDef ) {
			common->Error( "anFileSystem::GetMapDecl %d: not found\n", iDecl );
		}
		mapDict = mapDef->dict;
		mapDict.Set( "path", mapDef->GetName() );
		return &mapDict;
	}
	iDecl -= numDecls;
	for ( int i = 0; i < 2; i++ ) {
		if ( i == 0 ) {
			search = searchPaths;
		} else if ( i == 1 ) {
			search = addonPaks;
		}
		for ( ; search ; search = search->next ) {
			if ( !search->pack || !search->pack->addon || !search->pack->pakInfo ) {
				continue;
			}
			// each addon may have a bunch of map decls
			if ( iDecl <search->pack->pakInfo->mapDecls.Num() ) {
				mapDict = *search->pack->pakInfo->mapDecls[ iDecl ];
				return &mapDict;
			}
			iDecl -= search->pack->pakInfo->mapDecls.Num();
			assert( iDecl >= 0 );
		}
	}
	return nullptr;
}

/*
===============
anFileSystem::FindMapScreenshot
===============
*/
void anFileSystem::FindMapScreenshot( const char *path, char *buf, int len ) {
	anFile	*file;
	anStr	mapname = path;

	mapname.StripPath();
	mapname.StripFileExtension();
	
	anStr::snPrintf( buf, len, "guis/assets/splash/%s.tga", mapname.c_str() );
	if ( ReadFile( buf, nullptr, nullptr ) == -1 ) {
		// try to extract from an addon
		file = OpenFileReadFlags( buf, FSFLAG_SEARCH_ADDONS );
		if ( file ) {
			// save it out to an addon splash directory
			int dlen = file->Length();
			char *data = new char[ dlen ];
			file->Read( data, dlen );
			CloseFile( file );
			anStr::snPrintf( buf, len, "guis/assets/splash/addon/%s.tga", mapname.c_str() );
			WriteFile( buf, data, dlen );
			delete[] data;
		} else {
			anStr::Copynz( buf, "guis/assets/splash/pdtempa", len );
		}
	}
}
