
#ifndef __UNZIP_H__
#define __UNZIP_H__

#include "zlib/zlib.h"

#if defined(STRICTUNZIP) || defined(STRICTZIPUNZIP)
/* like the STRICT of WIN32, we define a pointer that cannot be converted
    from (void*) without cast */
typedef struct TagunzFile__ { int unused; } unzFile__;
typedef unzFile__ *anPaKBFile;
#else
typedef void *anPaKBFile;
#endif

// tm_unz contain date/time info
typedef struct tm_unz_s {
	unsigned int tm_sec;            // seconds after the minute - [0,59]
	unsigned int tm_min;            // minutes after the hour - [0,59] 
	unsigned int tm_hour;           // hours since midnight - [0,23]
	unsigned int tm_mday;           // day of the month - [1,31]
	unsigned int tm_mon;            // months since January - [0,11
	unsigned int tm_year;           // years - [1980..2044] 
} tm_unz;

/* pakGlobalInfo structure contain global data about the ZIPfile
   These data comes from the end of central dir */
typedef struct pakGlobalInfo_s {
	unsigned long						entryNumber;         // total number of entries in the central dir on this disk
	unsigned long						commentSize;         // size of the global comment of the zipfile
} pakGlobalInfo;


/* pakFileInfo contain information about a file in the zipfile */
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

    tm_unz					tmu_date;
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


// pakBFile_s contain internal information about the zipfile
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
} pakBFile_s;

#define UNZ_OK                                  (0 )
#define UNZ_END_OF_LIST_OF_FILE (-100)
#define UNZ_ERRNO               (Z_ERRNO)
#define UNZ_EOF                 (0 )
#define UNZ_PARAMERROR                  (-102)
#define UNZ_BADZIPFILE                  (-103)
#define UNZ_INTERNALERROR               (-104)
#define UNZ_CRCERROR                    (-105)

#define UNZ_CASESENSITIVE		1
#define UNZ_NOTCASESENSITIVE	2
#define UNZ_OSDEFAULTCASE		0

extern int unzStringFileNameCompare( const char *fileName1, const char *fileName2, int caseSensitive );

/*
   Compare two filename (fileName1,fileName2).
   If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
   If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
								or strcasecmp)
   If iCaseSenisivity = 0, case sensitivity is defaut of your operating system
	(like 1 on Unix, 2 on Windows)
*/

extern anPaKBFile PAK_Open( const char *path );
extern anPaKBFile PAK_ExplicitOpen( const char* path, anPaKBFile file );

/*
  Open a Zip file. path contain the full pathname (by example,
     on a Windows NT computer "c:\\zlib\\zlib111.zip" or on an Unix computer
	 "zlib/zlib111.zip".
	 If the zipfile cannot be opened (file don't exist or in not valid), the
	   return value is nullptr.
     Else, the return value is a anPaKBFile Handle, usable with other function
	   of this unzip package.
*/

extern int PAK_Close( anPaKBFile file );

/*
  Close a ZipFile opened with unzipOpen.
  If there is files inside the .Zip opened with PAK_OpenCurrentFile ( see later),
    these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
  return UNZ_OK if there is no problem. */

extern int PAK_GetDescription( anPaKBFile file, pakGlobalInfo *pglobal_info );

/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem. */


extern int PAK_GetComments( anPaKBFile file, char *szComment, unsigned long uSizeBuf );

/*
  Get the global comment string of the ZipFile, in the szComment buffer.
  uSizeBuf is the size of the szComment buffer.
  return the number of unsigned char copied or an error code <0
*/


/***************************************************************************/
/* Unzip package allow you browse the directory of the zipfile */

extern int PAK_GoToFirstFile( anPaKBFile file );

/*
  Set the current file of the zipfile to the first file.
  return UNZ_OK if there is no problem
*/

extern int PAK_NextFile( anPaKBFile file );

/*
  Set the current file of the zipfile to the next file.
  return UNZ_OK if there is no problem
  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
*/

extern int PAK_GetFileStatus( anPaKBFile file, unsigned long *pos );

/*
  Get the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/

extern int PAK_SetFileDataLocation( anPaKBFile file, unsigned long pos );

/*
  Set the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/

extern int PAK_LocateFile( anPaKBFile file, const char *szFileName, int caseSensitive );

/*
  Try locate the file szFileName in the zipfile.
  For the caseSensitive signification, see unzStringFileNameCompare

  return value :
  UNZ_OK if the file is found. It becomes the current file.
  UNZ_END_OF_LIST_OF_FILE if the file is not found
*/


extern int PAK_GetFileDescription( anPaKBFile file, pakFileInfo *fileDesc, char *szFileName, unsigned long fileNameBufferSize, void *extraField, unsigned long extraFieldBufferSize, char *szComment, unsigned long commentBufferSize );

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
extern int PAK_OpenCurrentFile( anPaKBFile file );


// Close the file in zip opened with PAK_OpenCurrentFile
// Return UNZ_CRCERROR if all the file was read but the CRC is not good
extern int PAK_CloseCurrentFile( anPaKBFile file );

extern int unzReadCurrentFile( anPaKBFile file, void *buf, unsigned len );

/*
  Read unsigned chars from the current file (opened by PAK_OpenCurrentFile)
  buf contain buffer where data must be copied
  len the size of buf.

  return the number of unsigned char copied if somes unsigned chars are copied
  return 0 if the end of file was reached
  return <0 with error code if there is an error
    (UNZ_ERRNO for IO error, or zLib error for uncompress error)
*/

extern long PAK_Tell( anPaKBFile file );

extern int unzeof( anPaKBFile file );


extern int unzGetLocalExtrafield( anPaKBFile file, void* buf, unsigned len );


#endif /* __UNZIP_H__ */
