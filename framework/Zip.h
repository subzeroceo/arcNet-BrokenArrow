#ifndef	__ZIP_H__
#define	__ZIP_H__

#include "zlib/zlib.h"

#if defined(STRICTUNZIP) || defined(STRICTZIPUNZIP)
// like the STRICT of WIN32, we define a pointer that cannot be converted
// from (void*) without cast
typedef struct TagunzFile__ { int unused; } unzFile__;
typedef unzFile__ *anPaKFile;
#else
typedef void *anPaKFile;
#endif

// pakTimeFmt_t contain date/time info
typedef struct tm_unz_s {
	unsigned int tm_sec;            // seconds after the minute - [0,59]
	unsigned int tm_min;            // minutes after the hour - [0,59] 
	unsigned int tm_hour;           // hours since midnight - [0,23]
	unsigned int tm_mday;           // day of the month - [1,31]
	unsigned int tm_mon;            // months since January - [0,11
	unsigned int tm_year;           // years - [1980..2044] 
} pakTimeFmt_t;

// pakGlobalInfo structure contain global data about the ZIPfile
// These data comes from the end of central dir */
typedef struct pakGlobalInfo_s {
	unsigned long						entryNumber;         // total number of entries in the central dir on this disk
	unsigned long						commentSize;         // size of the global comment of the zipfile
} pakGlobalInfo;

// pakFileInfo contain information about a file in the zipfile
typedef struct pakFileInfo_s {
    unsigned long			version;				// version made by                 2 unsigned chars
    unsigned long			versionSupport;			// version needed to extract       2 unsigned chars
    unsigned long			flag;					// general purpose bit flag        2 unsigned chars
    unsigned long			compressionType;		// compression method              2 unsigned chars
    unsigned long			dosTimeFmt;             // last mod file date in Dos fmt   4 unsigned chars
    unsigned long			crc;					// crc-32                          4 unsigned chars 
    unsigned long			compressedSize;			/// compressed size                 4 unsigned chars
    unsigned long			uncompressedSize;		//uncompressed size               4 unsigned chars
    unsigned long			fileNameSize;			// filename length                 2 unsigned chars
    unsigned long			extraFieldLen;			// extra field length              2 unsigned chars
    unsigned long			commentLength;			// file comment length             2 unsigned chars 

    unsigned long			startDiskNumber;		// disk number start               2 unsigned chars
    unsigned long			internalFileAttribs;	// internal file attributes        2 unsigned chars
    unsigned long			externalFileAttribs;	// external file attributes        4 unsigned chars

    pakTimeFmt_t					tmu_date;
} pakFileInfo;

// unz_fInfo_interntal contain internal info about a file in zipfile
typedef struct internalPakInfo_s {
	// relative offset of static header 4 unsigned chars
	unsigned long 				fileOffset;	
} internalPakInfo;

// pakReadInfo_s contain internal information about a file in zipfile,
// when reading and decompress it 
typedef struct {
	char *						readBuffer;				// internal buffer for compressed data 
	z_stream					stream;					// zLib stream structure for inflate 

	unsigned long				internalLocation;     	 // the files (internal) location within the zipfile and is used for fseek
	unsigned long 				initialised;			// flag set if stream structure is initialised

	unsigned long				extraFieldOffset;		// offset of the static extra field
	unsigned int				extraFieldSize;			// size of the static extra field 
	unsigned long				extraFieldPos;			// position in the static extra field in read

	unsigned long				crc32;					// crc32 of all data uncompressed
	unsigned long				waitCRC32;				// crc32 we must obtain after decompress all 
	unsigned long				compressedRead;			// number of unsigned char to be decompressed 
	unsigned long				unCompressedRead;		//number of unsigned char to be obtained after decomp
	anFile * 					file;					// io structore of the zipfile
	unsigned long				compressionType;		// compression method (0==store) 
	unsigned long				unCompressedSize;		// unsigned char before the zipfile, (>0 for sfx)
} pakReadInfo_s;


// pKFile_s contain internal information about the zipfile
typedef struct {
	anFile_Cached * 			file;                 // io structore of the zipfile 
	pakGlobalInfo				globalInfo;     		  // public global information 
	unsigned long				unCompressedSize;// unsigned char before the zipfile, (>0 for sfx)
	unsigned long 				fileNumber;             // number of the current file in the zipfile
	unsigned long 				fileLocation;   // pos of the current file in the central dir
	unsigned long 				fileStatusCode;      // flag about the usability of the current file
	unsigned long 				dirBeginingPos;			// position of the beginning of the central dir

	unsigned long 				directorySize;		// size of the central directory
	unsigned long 				directoryOffset;	// offset of start of central directory with
										// respect to the starting disk number

	pakFileInfo 				currentFileInfo; // public info about the current file in zip
	internalPakInfo 			internalFileInfo; // private info about it
    pakReadInfo_s* 				zippedFileInfo; // structure about the current
	                                   // file if we are decompressing it
} pKFile_s;

#define UNZ_OK 							( 0 )
#define UNZ_END_OF_LIST_OF_FILE 		( -100 )
#define UNZ_ERRNO 						( Z_ERRN O)
#define UNZ_EOF 						( 0 )
#define UNZ_PARAMERROR 					( -102 )
#define UNZ_BADZIPFILE 					( -103 )
#define UNZ_INTERNALERROR 				( -104 )
#define UNZ_CRCERROR 					( -105 )

#define UNZ_CASESENSITIVE 				1
#define UNZ_NOTCASESENSITIVE			2
#define UNZ_OSDEFAULTCASE 				0

extern int PaK_CompareFilename( const char *fileName1, const char *fileName2, int caseSensitive );

/*
   Compare two filename (fileName1,fileName2).
   If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
   If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
								or strcasecmp)
   If iCaseSenisivity = 0, case sensitivity is defaut of your operating system
	(like 1 on Unix, 2 on Windows)
*/

extern anPaKFile PAK_Open( const char *path );
extern anPaKFile PAK_ExplicitOpen( const char *path, anPaKFile file );

/*
  Open a Zip file. path contain the full pathname (by example,
     on a Windows NT computer "c:\\zlib\\zlib111.zip" or on an Unix computer
	 "zlib/zlib111.zip".
	 If the zipfile cannot be opened (file don't exist or in not valid), the
	   return value is nullptr.
     Else, the return value is a anPaKFile Handle, usable with other function
	   of this unzip package.
*/

extern int PAK_Close( anPaKFile file );

/*
  Close a ZipFile opened with unzipOpen.
  If there is files inside the .Zip opened with PAK_OpenCurrentFile ( see later),
    these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
  return UNZ_OK if there is no problem. */

extern int PAK_GetDescription( anPaKFile file, pakGlobalInfo *pglobal_info );

/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem. */


extern int PAK_GetComments( anPaKFile file, char *szComment, unsigned long uSizeBuf );

/*
  Get the global comment string of the ZipFile, in the szComment buffer.
  uSizeBuf is the size of the szComment buffer.
  return the number of unsigned char copied or an error code <0
*/


/***************************************************************************/
/* Unzip package allow you browse the directory of the zipfile */

extern int PAK_GoToFirstFile( anPaKFile file );

/*
  Set the current file of the zipfile to the first file.
  return UNZ_OK if there is no problem
*/

extern int PAK_NextFile( anPaKFile file );

/*
  Set the current file of the zipfile to the next file.
  return UNZ_OK if there is no problem
  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
*/

extern int PAK_GetFileStatus( anPaKFile file, unsigned long *pos );

/*
  Get the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/

extern int PAK_SetFileDataLocation( anPaKFile file, unsigned long pos );

/*
  Set the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/

extern int PAK_LocateFile( anPaKFile file, const char *szFileName, int caseSensitive );

/*
  Try locate the file szFileName in the zipfile.
  For the caseSensitive signification, see PaK_CompareFilename

  return value :
  UNZ_OK if the file is found. It becomes the current file.
  UNZ_END_OF_LIST_OF_FILE if the file is not found
*/


extern int PAK_GetFileDescription( anPaKFile file, pakFileInfo *fileDesc, char *szFileName, unsigned long fileNameBufferSize, void *extraField, unsigned long extraFieldBufferSize, char *szComment, unsigned long commentBufferSize );

/*
  Get Info about the current file
  if fileDesc!=nullptr, the *fileDesc structure will contain somes info about
	    the current file
  if szFileName!=nullptr, the filemane string will be copied in szFileName
			(fileNameBufferSize is the size of the buffer)
  if extraField!=nullptr, the extra field information will be copied in extraField
			(extraFieldBufferSize is the size of the buffer).
			This is the Central-header version of the extra field
  if szComment! = nullptr, the comment string of the file will be copied in szComment
			(commentBufferSize is the size of the buffer)
*/

/***************************************************************************/
/* for reading the content of the current zipfile, you can open it, read data
   from it, and close it (you can close it before reading all the file)
   */

// Open for reading data the current file in the zipfile.
// If there is no error, the return value is UNZ_OK.
extern int PAK_OpenCurrentFile( anPaKFile file );


// Close the file in zip opened with PAK_OpenCurrentFile
// Return UNZ_CRCERROR if all the file was read but the CRC is not good
extern int PAK_CloseCurrentFile( anPaKFile file );

extern int PAK_ReadCurrentFile( anPaKFile file, void *buf, unsigned len );

/*
  Read unsigned chars from the current file (opened by PAK_OpenCurrentFile)
  buf contain buffer where data must be copied
  len the size of buf.

  return the number of unsigned char copied if somes unsigned chars are copied
  return 0 if the end of file was reached
  return <0 with error code if there is an error
    (UNZ_ERRNO for IO error, or zLib error for uncompress error)
*/

extern long PAK_Tell( anPaKFile file );

extern int PaK_IsEoF( anPaKFile file );


extern int PaK_GetExtraField( anPaKFile file, void* buf, unsigned len );


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
linkedlist datablock internal
========================
*/
typedef struct dataBlockLLI_s {
	struct dataBlockLLI_s *			next; // next data block
	unsigned long					availableInBlock;
	unsigned long					filledBlock;
	unsigned long					unused; /* for future use and alignement */
	unsigned char					data[SIZEDATA_INDATABLOCK];
} dataBlockLLI;

/*
========================
dataLinkedList
========================
*/
typedef struct dataLinkedList_s {
	dataBlockLLI *	firstBlock;
	dataBlockLLI *	lastBlock;
} dataLinkedList;

/*
========================
curfInfo
========================
*/
typedef struct {
	z_stream						stream;					// zLib stream structure for inflate
	int								initialised;		// 1 is stream is initialised
	unsigned int					bufferDataPos;	// last written byte in bufData

	unsigned long					localHdrPos;		// offset of the local header of the file currenty writing
	char*							centralHdr;			// central header data for the current file 
	unsigned long					centralHdrSize;		// size of the central header for cur file 
	unsigned long					flag;					// flag of the file currently writing 

	int								method;					// compression method of file currenty wr
	int								raw;					// 1 for directly writing raw data
	byte							bufData[Z_BUFSIZE];	// buffer contain compressed data to be writ
	unsigned long					dosTimeFmt;
	unsigned long					crc32;
	int								encrypt;
#ifndef NOCRYPT
	unsigned long					keys[3];				// keys defining the pseudo-random sequence
	const unsigned long*			pcrc_32_tab;
	int								crypt_header_size;
#endif
} curfInfo;

//#define NO_ADDFILEINEXISTINGZIP
/*
========================
pakInternal_t
========================
*/
typedef struct {
	anFile *						fileStream;				// io structore of the zipfile
	dataLinkedList					centralDir;			// datablock with central dir in construction
	int								in_opened_file_inzip;	// 1 if a file in the zip is currently writ.
	curfInfo						ci;						// info on the file curretly writing

	unsigned long					startPos;				// position of the beginning of the zipfile
	unsigned long					WriteOffsetPos;
	unsigned long					entryNumber;
#ifndef NO_ADDFILEINEXISTINGZIP
	char*							globalComments;
#endif
} pakInternal_t;

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
extern zipFile zipOpen2( const char *pathname, int append, char *globalComments );

extern int zipOpenNewFileInZip( zipFile file, const char *filename, const zip_fileinfo* zipfi, const void* extrafield_local, uInt size_extrafield_local, const void* extrafield_global, uInt size_extrafield_global, const char *comment, int method, int level );

/*
  Same than zipOpenNewFileInZip, except if raw=1, we write raw file
*/
extern int zipOpenNewFileInZip2( zipFile file, const char *filename, const zip_fileinfo* zipfi, const void* extrafield_local, uInt size_extrafield_local,
								 const void *extrafield_global, uInt size_extrafield_global, const char *comment, int method, int level, int raw );

/*
  Same than zipOpenNewFileInZip2, except
    windowBits,memLevel,,strategy : see parameter strategy in deflateInit2
    password : crypting password (nullptr for no crypting)
    crcForCtypting : crc of file to compress (needed for crypting)
*/
extern int PaK_OpenNewFileInZip3( zipFile file, const char *filename, const zip_fileinfo* zipfi, const void* extrafield_local, uInt size_extrafield_local, const void* extrafield_global, uInt size_extrafield_global, const char *comment, int method, int level, int raw, int windowBits, int memLevel, int strategy, const char *password, uLong crcForCtypting );

/*
  Write data in the zipfile
*/
extern int PaK_WriteInFileInZip( zipFile file, const void* buf, unsigned int len );


/*
  Close the current file in the zipfile
*/
extern int PaK_CloseFileInZip( zipFile file );


/*
  Close the current file in the zipfile, for fiel opened with
    parameter raw=1 in zipOpenNewFileInZip2
  uncompressedSize and crc32 are value for the uncompressed size
*/
extern int zipCloseFileInZipRaw( zipFile file, uLong uncompressedSize, uLong crc32 );


/*
  Close the zipfile
*/
extern int PaK_ClosePaK( zipFile file, const char *global_comment );


/*
================================================
anPaKAssembly

simple interface for zipping up a folder of files
by default, the source folder files are added recursively
================================================
*/
class anPaKAssembly {
public:
						anPaKAssembly() {}
						~anPaKAssembly() {}

public:
						// adds a list of file extensions ( e.g. "bcm|bmodel|" ) to be compressed in the zip file
	void				AddFileFilters( const char *filters );
						// adds a list of file extensions ( e.g. "genmodel|" ) to be added to the zip file, in an uncompressed form
	void				AddUncompressedFileFilters( const char *filters );
						// builds a zip file of all the files in the specified folder, overwriting if necessary
	bool				Build( const char *zipPath, const char *folder, bool cleanFolder );
						// updates a zip file with the files in the specified folder
	bool				Update( const char *zipPath, const char *folder, bool cleanFolder );

						// helper function to zip up all the files and put in a new zip file
	static bool			BuildMapFolderZip( const char *mapFileName );
						// helper function to update a map folder zip for newer files
	static bool			UpdateMapFolderZip( const char *mapFileName );

						// combines multiple in-memory files into a single memory file
	static anFileMemory *CombineFiles( const anList<anFileMemory *> &srcFiles );
						// extracts multiple in-memory files from a single memory file
	static bool			ExtractFiles( anFileMemory *&srcFile, anList<anFileMemory *> &destFiles );

	void				CleanSourceFolder();

	bool				CreateZipFileFromFileList( const char *name, const anList<anFileMemory *> &srcFiles );

	zipFile				CreateZipFile( const char *name );
	bool				AddFile( zipFile zf, anFileMemory *fm, bool deleteFile );
	void				CloseZipFile( zipFile zf );
private:
	bool				CreateZipFile( bool appendFiles );
	bool				CreateZipFileFromFiles( const anList<anFileMemory *> &srcFiles );
	bool				GetFileTime( const anStr &filename, unsigned long *dostime ) const;
	bool				IsFiltered( const anStr &filename ) const;
	bool				IsUncompressed( const anStr &filename ) const;

private:
	anStr				zipFileName;				// os path to the zip file
	anStr				sourceFolderName;			// source folder of files to zip or add
	anStringList		filterExts;					// file extensions we want to compressed
	anStringList		uncompressedFilterExts;		// file extensions we don't want to compress
};

#endif	// __ZIP_H__
