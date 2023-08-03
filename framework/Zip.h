#ifndef	__ZIP_H__
#define	__ZIP_H__

#include "zlib/zlib.h"

/*
================================================================================================

Contains external code for building ZipFiles.

The Unzip Package allows extraction of a file from .ZIP file, compatible with
PKZip 2.04g, !WinZip, !InfoZip tools and compatibles. Encryption and multi-volume ZipFiles
( span) are not supported. Old compressions used by old PKZip 1.x are not supported.

================================================================================================
*/
#if defined(STRICTZIP) || defined(STRICTZIPUNZIP)
/* like the STRICT of WIN32, we define a pointer that cannot be converted
    from (void*) without cast */
	typedef struct TagzipFile__ { int unused; } zipFile__;
	typedef zipFile__ *zipFile;
#else
	typedef void* zipFile;
#endif

#define ZIP_OK                      (0 )
#define ZIP_EOF                     (0 )
#define ZIP_ERRNO                   (Z_ERRNO)
#define ZIP_PARAMERROR              (-102)
#define ZIP_BADZIPFILE              (-103)
#define ZIP_INTERNALERROR           (-104)

#ifndef Z_BUFSIZE
#define Z_BUFSIZE (16384)
#endif

/*
========================
tm_zip
contains date/time info
========================
*/
typedef struct tm_zip_s	{
    unsigned int					tm_sec;            		/* seconds after the minute - [0,59] */
    unsigned int					tm_min;            		/* minutes after the hour - [0,59] */
    unsigned int					tm_hour;           		/* hours since midnight - [0,23] */
    unsigned int					tm_mday;           		/* day of the month - [1,31] */
    unsigned int					tm_mon;            		/* months since January - [0,11] */
    unsigned int					tm_year;           		/* years - [1980..2044] */
} tm_zip;

/*
========================
zip_fileinfo
========================
*/
typedef struct {
    tm_zip							tmz_date;				/* date in understandable format           */
    unsigned long					dosTimeFmt;				/* if dos_date == 0, tmu_date is used      */
//    unsigned long					  flag;					/* general purpose bit flag        2 bytes */

    unsigned long					internalFileAttribs;			/* internal file attributes        2 bytes */
    unsigned long					externalFileAttribs;			/* external file attributes        4 bytes */
} zip_fileinfo;

#define NOCRYPT						// ignore passwords
#define SIZEDATA_INDATABLOCK		(4096-(4*4) )

/*
========================
linkedlist_datablock_internal
========================
*/
typedef struct linkedlist_datablock_internal_s {
	struct linkedlist_datablock_internal_s*	next_datablock;
	unsigned long					avail_in_this_block;
	unsigned long					filled_in_this_block;
	unsigned long					unused; /* for future use and alignement */
	unsigned char					data[SIZEDATA_INDATABLOCK];
} linkedlist_datablock_internal;

/*
========================
linkedlist_data
========================
*/
typedef struct linkedlist_data_s {
	linkedlist_datablock_internal*	first_block;
	linkedlist_datablock_internal*	last_block;
} linkedlist_data;

/*
========================
curfInfo
========================
*/
typedef struct {
	z_stream						stream;					/* zLib stream structure for inflate */
	int								initialised;		/* 1 is stream is initialised */
	unsigned int					pos_in_buffered_data;	/* last written byte in buffered_data */

	unsigned long					pos_local_header;		/* offset of the local header of the file currenty writing */
	char*							central_header;			/* central header data for the current file */
	unsigned long					size_centralheader;		/* size of the central header for cur file */
	unsigned long					flag;					/* flag of the file currently writing */

	int								method;					/* compression method of file currenty wr.*/
	int								raw;					/* 1 for directly writing raw data */
	byte							buffered_data[Z_BUFSIZE];/* buffer contain compressed data to be writ*/
	unsigned long					dosTimeFmt;
	unsigned long					crc32;
	int								encrypt;
#ifndef NOCRYPT
	unsigned long					keys[3];				/* keys defining the pseudo-random sequence */
	const unsigned long*			pcrc_32_tab;
	int								crypt_header_size;
#endif
} curfInfo;

//#define NO_ADDFILEINEXISTINGZIP
/*
========================
zip_internal
========================
*/
typedef struct {
	anFile*							filestream;				/* io structore of the zipfile */
	linkedlist_data					central_dir;			/* datablock with central dir in construction*/
	int								in_opened_file_inzip;	/* 1 if a file in the zip is currently writ.*/
	curfInfo					ci;						/* info on the file curretly writing */

	unsigned long					begin_pos;				/* position of the beginning of the zipfile */
	unsigned long					add_position_when_writting_offset;
	unsigned long					entryNumber;
#ifndef NO_ADDFILEINEXISTINGZIP
	char*							globalcomment;
#endif
} zip_internal;

#define APPEND_STATUS_CREATE        (0 )
#define APPEND_STATUS_CREATEAFTER   (1 )
#define APPEND_STATUS_ADDINZIP      (2)

/*
  Create a zipfile.
     pathname contain on Windows XP a filename like "c:\\zlib\\zlib113.zip" or on
       an Unix computer "zlib/zlib113.zip".
     if the file pathname exist and append==APPEND_STATUS_CREATEAFTER, the zip
       will be created at the end of the file.
         (useful if the file contain a self extractor code)
     if the file pathname exist and append==APPEND_STATUS_ADDINZIP, we will
       add files in existing zip (be sure you don't add file that doesn't exist)
     If the zipfile cannot be opened, the return value is nullptr.
     Else, the return value is a zipFile Handle, usable with other function
       of this zip package.
*/

/* Note : there is no delete function into a zipfile.
   If you want delete file into a zipfile, you must open a zipfile, and create another
   Of couse, you can use RAW reading and writing to copy the file you did not want delte
*/
extern zipFile zipOpen( const char *pathname, int append );

/*
  Open a file in the ZIP for writing.
  filename : the filename in zip (if nullptr, '-' without quote will be used
  *zipfi contain supplemental information
  if extrafield_local!=nullptr and size_extrafield_local>0, extrafield_local
    contains the extrafield data the the local header
  if extrafield_global!=nullptr and size_extrafield_global>0, extrafield_global
    contains the extrafield data the the local header
  if comment != nullptr, comment contain the comment string
  method contain the compression method (0 for store, Z_DEFLATED for deflate)
  level contain the level of compression (can be Z_DEFAULT_COMPRESSION)
*/
extern zipFile zipOpen2( const char* pathname, int append, char *globalcomment );

extern int zipOpenNewFileInZip( zipFile file, const char* filename, const zip_fileinfo* zipfi, const void* extrafield_local, uInt size_extrafield_local, const void* extrafield_global, uInt size_extrafield_global, const char* comment, int method, int level );

/*
  Same than zipOpenNewFileInZip, except if raw=1, we write raw file
*/
extern int zipOpenNewFileInZip2( zipFile file, const char* filename, const zip_fileinfo* zipfi, const void* extrafield_local, uInt size_extrafield_local,
								 const void* extrafield_global, uInt size_extrafield_global, const char* comment, int method, int level, int raw );

/*
  Same than zipOpenNewFileInZip2, except
    windowBits,memLevel,,strategy : see parameter strategy in deflateInit2
    password : crypting password (nullptr for no crypting)
    crcForCtypting : crc of file to compress (needed for crypting)
*/
extern int zipOpenNewFileInZip3( zipFile file, const char* filename, const zip_fileinfo* zipfi, const void* extrafield_local, uInt size_extrafield_local,
								 const void* extrafield_global, uInt size_extrafield_global, const char* comment, int method, int level, int raw, int windowBits,
								 int memLevel, int strategy, const char* password, uLong crcForCtypting );

/*
  Write data in the zipfile
*/
extern int zipWriteInFileInZip( zipFile file, const void* buf, unsigned int len );


/*
  Close the current file in the zipfile
*/
extern int zipCloseFileInZip( zipFile file );


/*
  Close the current file in the zipfile, for fiel opened with
    parameter raw=1 in zipOpenNewFileInZip2
  uncompressedSize and crc32 are value for the uncompressed size
*/
extern int zipCloseFileInZipRaw( zipFile file, uLong uncompressedSize, uLong crc32 );


/*
  Close the zipfile
*/
extern int zipClose( zipFile file, const char* global_comment );


/*
================================================
idZipBuilder

simple interface for zipping up a folder of files
by default, the source folder files are added recursively
================================================
*/
class idZipBuilder {
public:
						idZipBuilder() {}
						~idZipBuilder() {}

public:
						// adds a list of file extensions ( e.g. "bcm|bmodel|" ) to be compressed in the zip file
	void				AddFileFilters( const char *filters );
						// adds a list of file extensions ( e.g. "genmodel|" ) to be added to the zip file, in an uncompressed form
	void				AddUncompressedFileFilters( const char *filters );
						// builds a zip file of all the files in the specified folder, overwriting if necessary
	bool				Build( const char* zipPath, const char *folder, bool cleanFolder );
						// updates a zip file with the files in the specified folder
	bool				Update( const char* zipPath, const char *folder, bool cleanFolder );

						// helper function to zip up all the files and put in a new zip file
	static bool			BuildMapFolderZip( const char *mapFileName );
						// helper function to update a map folder zip for newer files
	static bool			UpdateMapFolderZip( const char *mapFileName );

						// combines multiple in-memory files into a single memory file
	static anFileMemory * CombineFiles( const anList<anFileMemory *> &srcFiles );
						// extracts multiple in-memory files from a single memory file
	static bool			ExtractFiles( anFileMemory * & srcFile, anList<anFileMemory *> &destFiles );

	void				CleanSourceFolder();

	bool				CreateZipFileFromFileList( const char *name, const anList<anFileMemory *> &srcFiles );

	zipFile				CreateZipFile( const char *name );
	bool				AddFile( zipFile zf, anFileMemory *fm, bool deleteFile );
	void				CloseZipFile( zipFile zf );
private:
	bool				CreateZipFile( bool appendFiles );
	bool				CreateZipFileFromFiles( const anList<anFileMemory *> &srcFiles );
	bool				GetFileTime( const anString &filename, unsigned long *dostime ) const;
	bool				IsFiltered( const anString &filename ) const;
	bool				IsUncompressed( const anString &filename ) const;

private:
	anString				zipFileName;				// os path to the zip file
	anString				sourceFolderName;			// source folder of files to zip or add
	anStringList			filterExts;					// file extensions we want to compressed
	anStringList			uncompressedFilterExts;		// file extensions we don't want to compress
};

#endif	/* __ZIP_H__ */
