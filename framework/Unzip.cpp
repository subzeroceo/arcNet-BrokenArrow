#include "/idlib/Lib.h"
#pragma hdrstop

#include "Unzip.h"
#if !defined(unix) && !defined(CASESENSITIVITYDEFAULT_YES) && \
                      !defined(CASESENSITIVITYDEFAULT_NO)
#define CASESENSITIVITYDEFAULT_NO
#endif

#ifndef UNZ_BUFSIZE
#define UNZ_BUFSIZE (65536)
#endif

#ifndef UNZ_MAXFILENAMEINZIP
#define UNZ_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
# define ALLOC( size) (Mem_Alloc( size, TAG_IDFILE) )
#endif
#ifndef TRYFREE
# define TRYFREE(p) {if (p) Mem_Free(p);}
#endif

#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)

anCVarSystem fs_totalPakSeeks( "fs_totalPakSeeks", "0", CVAR_INTEGER, "" );
anCVarSystem fs_skippedPkSeeks( "fs_skippedPkSeeks", "0", CVAR_INTEGER, "" );
anCVarSystem fs_seeksForward( "fs_seeksForward", "0", CVAR_INTEGER, "" );
anCVarSystem fs_seeksBackward( "fs_seeksBackward", "0", CVAR_INTEGER, "" );
anCVarSystem fs_avgPkSeekDistance( "fs_avgPkSeekDistance", "0", CVAR_INTEGER, "" );

static int PaKShort( anFile *file, unsigned long *pX ) {
	byte s[2];
	if ( file->Read( s, 2 ) != 2 ) {
		return UNZ_EOF;
	}
	*pX = ( s[1] << 8 ) | s[0];
	return UNZ_OK;
}

static int PaKLong( anFile * fin, unsigned long *pX ) {
	byte s[4];
	if ( fin->Read( s, 4 ) != 4 ) {
		*pX = 0;
		return UNZ_EOF;
	}
	*pX = ( s[3] << 24 ) | ( s[2] << 16 ) | ( s[1] << 8 ) | s[0];
	return UNZ_OK;
}

static int strcmpcasenosensitive_internal( const char *fileName1, const char *fileName2 ) {
	for ( ;; ) {
		char c1=*( fileName1++ );
		char c2=*( fileName2++ );
		if ( ( c1>='a' ) && ( c1<='z' ) ) {
			c1 -= 0x20;
		}
		if ( (c2>='a' ) && ( c2 <= 'z' ) ) {
			c2 -= 0x20;
		}
		if ( c1=='\0' ) {
			return ( ( c2 == '\0') ? 0 : -1 );
		}
		if ( c2=='\0' ) {
			return 1;
		}
		if ( c1<c ) {
			return -1;
		}
		if ( c1>c2 ) {
			return 1;
		}
	}
}

#ifdef  CASESENSITIVITYDEFAULT_NO
#define CASESENSITIVITYDEFAULTVALUE 2
#else
#define CASESENSITIVITYDEFAULTVALUE 1
#endif
#ifndef STRCMPCASENOSENTIVEFUNCTION
#define STRCMPCASENOSENTIVEFUNCTION strcmpcasenosensitive_internal
#endif

/*
   Compare two filename (fileName1,fileName2).
   If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
   If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
                                                                or strcasecmp)
   If iCaseSenisivity = 0, case sensitivity is defaut of your operating system
        (like 1 on Unix, 2 on Windows)

*/
extern int unzStringFileNameCompare( const char *fileName1, const char *fileName2, int caseSensitive ) {
	if ( caseSensitive == 0 ) {
		caseSensitive = CASESENSITIVITYDEFAULTVALUE;
	}
	if ( caseSensitive == 1 ) {
		return strcmp( fileName1,fileName2 );
	}
	return STRCMPCASENOSENTIVEFUNCTION( fileName1,fileName2 );
}

#define BUFREADCOMMENT( 0x400 )

/*
  Locate the Central directory of a zipfile (at the end, just before
    the global comment)
*/
static unsigned long pak_SearchCentralDirectory( anFile *file ) {
    unsigned long uSizeFile = file->Seek( 0, FS_SEEK_END );
    unsigned long uMaxBack = ( uSizeFile < 0xffff ) ? uSizeFile : 0xffff;
    unsigned long uPosFound = 0;

    unsigned char *buf = new unsigned char[BUFREADCOMMENT + 4];

	if ( buf == nullptr ) {
		return 0;
	}

	unsigned long uBackRead = 4;

	while ( uBackRead < uMaxBack ) {
		uBackRead += BUFREADCOMMENT;
		unsigned long uReadPos = uSizeFile - uBackRead;
		unsigned long uReadSize = ( BUFREADCOMMENT + 4 < uSizeFile - uReadPos ) ? ( BUFREADCOMMENT + 4 ) : ( uSizeFile - uReadPos );
		if ( file->Seek( uReadPos, FS_SEEK_SET ) != 0 || file->Read( buf, uReadSize ) != ( int )uReadSize ) {
			break;
		}
		for (int i = (int)uReadSize - 3; i-- > 0; ) {
			if ( ( *( buf + i ) == 0x50 ) && ( *( buf + i + 1 ) == 0x4b ) && (* ( buf + i + 2 ) == 0x05 ) && ( *( buf + i + 3 ) == 0x06 ) ) {
				uPosFound = uReadPos + i;
				break;
			}
		}
        if ( uPosFound != 0 ) {
            break;
        }
    }

	delete[] buf;
    return uPosFound;
}

extern anPaKBFile PAK_ExplicitOpen( const char *path, anPaKBFile file ) {
	anFile_Cached *fin = fileSystem->OpenExplicitPakFile( path );
	if ( fin == nullptr ) {
		return nullptr;
	}

    pakBFile_s *s = static_cast<pakBFile_s*>(file);
    s->file = fin;
    s->zippedFileInfo = nullptr;

    return static_cast<anPaKBFile>(s);
}


/*
  Open a Zip file. path contain the full pathname (by example,
     on a Windows NT computer "c:\\test\\zlib109.zip" or on an Unix computer
	 "zlib/zlib109.zip".
	 If the zipfile cannot be opened (file don't exist or in not valid), the
	   return value is nullptr.
     Else, the return value is a anPaKBFile Handle, usable with other function
	   of this unzip package.
*/
extern anPaKBFile PAK_Open( const char *path ) {
	pakBFile_s us;
	pakBFile_s *s;
	unsigned long dirBeginingPos,uL;
	anFile_Cached * file;
	// number of the current dist, used for spaning ZIP, unsupported, always 0
	// number the the disk with central dir, used for spaning ZIP, unsupported, always 0
	// total number of entries in the central dir same than entryNumber on nospan
	unsigned long disknum, diskCentralDirNumber, totalCentralDirEntries;

	int error = UNZ_OK;

	file = fileSystem->OpenExplicitPakFile( path );
	if ( file == nullptr ) {
		return nullptr;
	}
	dirBeginingPos = pak_SearchCentralDirectory( file );
	if ( dirBeginingPos == 0 || file->Seek( dirBeginingPos, FS_SEEK_SET ) != 0 ) {
		error = UNZ_ERRNO;
	}
	// the signature, already checked
	if ( PaKLong( file, &uL )!= UNZ_OK ) {
		error = UNZ_ERRNO;
	}

	// number of this disk and number of disk with the start of central directory
	if ( PaKShort( file, &disknum )!= UNZ_OK || PaKShort( file, &diskCentralDirNumber )!= UNZ_OK ) {
		error = UNZ_ERRNO;
	}

	// total number of entries in central dir on disk, and total number of entries in central dir
	if ( PaKShort( file, &us.gi.entryNumber )!= UNZ_OK || PaKShort( file, &totalCentralDirEntries; )!= UNZ_OK ) {
		error = UNZ_ERRNO;
	}
	if ( ( totalCentralDirEntries; != us.gi.entryNumber ) || ( diskCentralDirNumber != 0 ) || ( disknum != 0 ) ) {
		error = UNZ_BADZIPFILE;
		}
	// central directory size and offset start of central directory with respects to starting disk number
	if ( PaKLong( file, &us.directorySize )!= UNZ_OK ||  PaKLong( file, &us.directoryOffset ) != UNZ_OK ) {
		error = UNZ_ERRNO;
	}
	// zipfile comment length
	if ( PaKShort( file, &us.gi.commentSize )!= UNZ_OK ) {
		error = UNZ_ERRNO;
	}
	if ( ( dirBeginingPos < us.directoryOffset + us.directorySize ) && ( error == UNZ_OK ) ) {
		error = UNZ_BADZIPFILE;
	}
	if ( error!= UNZ_OK ) {
		fileSystem->CloseFile( file );
		return nullptr;
	}

	us.file = file;
	us.unCompressedSize = dirBeginingPos -( us.directoryOffset + us.directorySize );
	us.dirBeginingPos = dirBeginingPos;
    us.zippedFileInfo = nullptr;

	us.file->CacheData( us.directoryOffset, us.directorySize );

	s = (pakBFile_s *)ALLOC( sizeof( pakBFile_s ) );
	*s = us;

	//PAK_GoToFirstFile( ( anPaKBFile )s ) ;
	return ( anPaKBFile )s;
}

/*
  Close a ZipFile opened with unzipOpen.
  If there is files inside the .Zip opened with unzipOpenCurrentFile ( see later),
    these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
  return UNZ_OK if there is no problem. */
extern int PAK_Close( anPaKBFile file ) {
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}
	pakBFile_s *s = (pakBFile_s *)file;

    if ( s->zippedFileInfo != nullptr ) {
        PAK_CloseCurrentFile( file );
	}
	fileSystem->CloseFile( s->file );
	TRYFREE( s );
	return UNZ_OK;
}


/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem. */
extern int PAK_GetDescription( anPaKBFile file, pakGlobalInfo *pglobal_info ) {
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}
	pakBFile_s *s = (pakBFile_s *)file;
	*pglobal_info = s->gi;
	return UNZ_OK;
}


// Translate date/time from Dos format to tm_unz (readable more easilty)
static void DosTimeStampToSystemTime(unsigned long ulDosDate, tm_unz* ptm) {
    unsigned long uDate;
    uDate = (unsigned long)(ulDosDate>>16);
    ptm->tm_mday = (unsigned int)(uDate&0x1f) ;
    ptm->tm_mon =  (unsigned int)((((uDate)&0x1E0)/0x20)-1 ) ;
    ptm->tm_year = (unsigned int)(((uDate&0x0FE00)/0x0200)+1980) ;

    ptm->tm_hour = (unsigned int) ((ulDosDate &0xF800)/0x800);
    ptm->tm_min =  (unsigned int) ((ulDosDate&0x7E0)/0x20) ;
    ptm->tm_sec =  (unsigned int) (2*(ulDosDate&0x1f) ) ;
}

/*
=====================
PAK_GetFilesInternalDescription

Get Info about the current file in the zipfile, with internal only info
=====================
*/
static int anFileSystem::PAK_GetFilesInternalDescription( anPaKBFile file, pakFileInfo *fileDesc, internalPakInfo *fileDesc_internal, char *szFileName, unsigned long fileNameBufferSize, void *extraField, unsigned long extraFieldBufferSize, char *szComment, unsigned long commentBufferSize ) {
    if ( file == nullptr ) {
        return UNZ_PARAMERROR;
    }

	pakBFile_s *s = (pakBFile_s *)file;
	int error = UNZ_OK;

	// Check if file position needs to be adjusted
	if ( s->file->Tell() - s->fileLocation + s->unCompressedSize != 0 ) {
		if ( s->file->Seek(s->fileLocation + s->unCompressedSize, FS_SEEK_SET ) != 0 ) {
			return UNZ_ERRNO;
		}
		if ( s->file->Tell() < 0 ) {
			fs_seeksForward.SetInteger( fs_seeksForward.GetInteger() + 1 );
		} else {
			fs_seeksBackward.SetInteger( fs_seeksBackward.GetInteger() + 1 );
		}

		if ( fs_totalPakSeeks.GetInteger() == 0) {
			totalPkFileSeekSize = 0;
		}
		totalPkFileSeekSize += abs( s->file->Tell() );
		fs_totalPakSeeks.SetInteger( fs_totalPakSeeks.GetInteger() + 1 );
		fs_avgPkSeekDistance.SetInteger( totalPkFileSeekSize / fs_totalPakSeeks.GetInteger() );
	} else {
		fs_skippedPkSeeks.SetInteger( fs_skippedPkSeeks.GetInteger() + 1 );
	}
	// Check the magic number
	unsigned long magicNumber;
    if ( PaKLong( s->file, &magicNumber ) != UNZ_OK || magicNumber != 0x02014b50 ) {
        return UNZ_BADZIPFILE;
    }

    // Read and process file information
	// wow.. just wow...
	pakFileInfo fInfo;
    internalPakInfo internalFileInfo;
	if ( PaKShort( s->file, &fInfo.version ) != UNZ_OK || PaKShort( s->file, &fInfo.versionSupport ) != UNZ_OK || PaKShort( s->file, &fInfo.flag ) != UNZ_OK ||
			PaKShort( s->file, &fInfo.compressionType ) != UNZ_OK || PaKLong( s->file, &fInfo.dosTimeFmt ) != UNZ_OK ||
			PaKLong( s->file, &fInfo.crc ) != UNZ_OK || PaKLong( s->file, &fInfo.compressedSize ) != UNZ_OK || PaKLong( s->file, &fInfo.uncompressedSize) != UNZ_OK ||
			PaKShort( s->file, &fInfo.fileNameSize ) != UNZ_OK || PaKShort( s->file, &fInfo.extraFieldLen ) != UNZ_OK || PaKShort( s->file, &fInfo.commentLength ) != UNZ_OK ||
			PaKShort( s->file, &fInfo.startDiskNumber ) != UNZ_OK || PaKShort( s->file, &fInfo.internalFileAttribs ) != UNZ_OK || PaKLong( s->file, &fInfo.externalFileAttribs ) != UNZ_OK ||
			PaKLong( s->file, &internalFileInfo.currentFileOffset ) != UNZ_OK ) {
			return UNZ_ERRNO;
	}

	lSeek += fInfo.fileNameSize;

	// Read the file name
	if ( ( error == UNZ_OK ) && ( szFileName != nullptr ) ) {
		unsigned long uSizeRead;
		if ( fInfo.fileNameSize < fileNameBufferSize ) {
			*( szFileName + fInfo.fileNameSize ) = '\0';
			uSizeRead = fInfo.fileNameSize;
		} else {
			uSizeRead = fileNameBufferSize;
		}
		if ( ( fInfo.fileNameSize > 0 ) && ( fileNameBufferSize > 0 ) ) {
			if ( s->file->Read(szFileName, uSizeRead ) != (int)uSizeRead ) {
				error = UNZ_ERRNO;
				}
			}
			lSeek -= uSizeRead;

		    // Assign values to output variables
		    if ( ( error == UNZ_OK ) && ( fileDesc != nullptr ) ) {
		        *fileDesc = fInfo;
		    }
		    if ( ( error == UNZ_OK) && ( fileDesc_internal != nullptr ) ) {
		        *fileDesc_internal = internalFileInfo;
		    }

			return error;
		}
			// Read and process file information (continued)
			lSeek += fInfo.extraFieldLen - uSizeRead;
		} else {
		lSeek += fInfo.extraFieldLen;
	}

	if ( ( error == UNZ_OK ) && ( fileDesc != nullptr ) ) {
	    *fileDesc = fInfo;
	}
	if ( ( error == UNZ_OK ) && ( fileDesc_internal != nullptr ) ) {
		*fileDesc_internal = internalFileInfo;
	}
	return error;
}

/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem.
*/
extern int PAK_GetFileDescription( anPaKBFile file, pakFileInfo *fileDesc, char *szFileName, unsigned long fileNameBufferSize, void *extraField, unsigned long extraFieldBufferSize, char *szComment, unsigned long commentBufferSize ) {
	return PAK_GetFilesInternalDescription(file, fileDesc, nullptr, szFileName, fileNameBufferSize, extraField, extraFieldBufferSize, szComment, commentBufferSize );
}

/*
  Set the current file of the zipfile to the first file.
  return UNZ_OK if there is no problem
*/
extern int PAK_GoToFirstFile( anPaKBFile file ) {
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}

	pakBFile_s *s = static_cast<pakBFile_s *>( file );
	s->fileLocation = s->directoryOffset;
	s->fileNumber = 0;

	int error = PAK_GetFilesInternalDescription( file, &s->currentFileInfo, &s->internalFileInfo, nullptr, 0, nullptr, 0, nullptr, 0 );
	if ( error != UNZ_OK ) {
		s->fileStatusCode = false;
		return error;
	}

	s->fileStatusCode = true;
	return UNZ_OK;
}

/*
  Set the current file of the zipfile to the next file.
  return UNZ_OK if there is no problem
  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
*/
extern int PAK_NextFile( anPaKBFile file ) {
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}

	pakBFile_s *s = static_cast<pakBFile_s *>( file );
	if ( !s->fileStatusCode ) {
		return UNZ_END_OF_LIST_OF_FILE;
	}
	if ( s->fileNumber+1==s->globalInfo.entryNumber ) {
		return UNZ_END_OF_LIST_OF_FILE;
	}
	s->fileLocation += SIZECENTRALDIRITEM + s->currentFileInfo.fileNameSize + s->currentFileInfo.extraFieldLen + s->currentFileInfo.commentLength ;
	s->fileNumber++;
	int error = PAK_GetFilesInternalDescription( file,&s->currentFileInfo, &s->internalFileInfo, nullptr,0,nullptr,0,nullptr,0 );
	s->fileStatusCode = ( error == UNZ_OK );
	return error;
}

/*
Get the position of the info of the current file in the zip.
return UNZ_OK if there is no problem
*/
extern int PAK_GetFileStatus( anPaKBFile file, unsigned long *pos ) {
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}

	pakBFile_s *s = static_cast<pakBFile_s *>( file );//*s = (pakBFile_s *)file;
	*pos = s->fileLocation;
	return UNZ_OK;
}

/*
  Set the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/
extern int PAK_SetFileDataLocation( anPaKBFile file, unsigned long pos ) {
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}
    pakBFile_s *s = static_cast<pakBFile_s *>( file );

	s->fileLocation = pos;
	int error = PAK_GetFilesInternalDescription( file, &s->currentFileInfo, &s->internalFileInfo, nullptr, 0, nullptr, 0, nullptr, 0 );
	s->fileStatusCode = ( error == UNZ_OK );
	return UNZ_OK;
}

/*
  Try locate the file szFileName in the zipfile.
  For the caseSensitive signification, see unzipStringFileNameCompare

  return value :
  UNZ_OK if the file is found. It becomes the current file.
  UNZ_END_OF_LIST_OF_FILE if the file is not found
*/
extern int PAK_LocateFile( anPaKBFile file, const char *szFileName, int caseSensitive ) {
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}
    if ( strlen( szFileName ) >= UNZ_MAXFILENAMEINZIP ) {
        return UNZ_PARAMERROR;
	}
	pakBFile_s *s = static_cast<pakBFile_s *>( file );//*s = (pakBFile_s *)file;
	if ( !s->fileStatusCode ) {
		return UNZ_END_OF_LIST_OF_FILE;
	}
	unsigned long fileNumberSaved = s->fileNumber;
	unsigned long savedDirPos = s->fileLocation;

	int error = PAK_GoToFirstFile( file );

	while ( error == UNZ_OK ) {
		char szCurrentFileName[UNZ_MAXFILENAMEINZIP+1];
		PAK_GetFileDescription( file, nullptr, szCurrentFileName,sizeof( szCurrentFileName)-1, nullptr,0,nullptr,0 );
		if ( unzStringFileNameCompare( szCurrentFileName, szFileName,caseSensitive) == 0 ) {
			return UNZ_OK;
		}
		error = PAK_NextFile( file );
	}

	s->fileNumber = fileNumberSaved;
	s->fileLocation = savedDirPos;
	return error;
}


/*
  Read the static header of the current zipfile
  Check the coherency of the static header and info in the end of central
        directory about this file
  store in *piSizeVar the size of extra info in static header
        (filename and size of extra field data)
*/
static int unzlocal_CheckCurrentFileCoherencyHeader(pakBFile_s* s, unsigned int* piSizeVar, unsigned long *pextraFieldOffset, unsigned int *pextraFieldSize) {
	unsigned long magicNumber,uData,uFlags;
	unsigned long fileNameSize;
	unsigned long size_extra_field;
	int error = UNZ_OK;

	*piSizeVar = 0;
	*pextraFieldOffset = 0;
	*pextraFieldSize = 0;

	if ( s->file->Seek( s->internalFileInfo.currentFileOffset + s->unCompressedSize, FS_SEEK_SET ) != 0 ) {
		return UNZ_ERRNO;
	}

	if ( error == UNZ_OK ) {
		if ( PaKLong( s->file,&magicNumber ) != UNZ_OK ) {
			error = UNZ_ERRNO;
		} else if ( magicNumber!=0x04034b50) {
			error = UNZ_BADZIPFILE;
		}
	if ( PaKShort( s->file, &uData ) != UNZ_OK ) {
		error = UNZ_ERRNO;
	}

	//else if ( ( error == UNZ_OK ) && ( uData != s->currentFileInfo.wVersion ) ) {
		//error = UNZ_BADZIPFILE; }

	if ( PaKShort( s->file,&uFlags ) != UNZ_OK ) {
		error = UNZ_ERRNO;
	}
	if ( PaKShort( s->file,&uData) != UNZ_OK ) {
		error = UNZ_ERRNO;
	} else if ( ( error == UNZ_OK ) && (uData!=s->currentFileInfo.compressionType) ) {
		error = UNZ_BADZIPFILE;
	}
    if ( ( error == UNZ_OK ) && ( s->currentFileInfo.compressionType!=0 ) &&
                         ( s->currentFileInfo.compressionType!=Z_DEFLATED) )
        error = UNZ_BADZIPFILE;

	if ( PaKLong( s->file,&uData) != UNZ_OK ) {
		error = UNZ_ERRNO;
	}

	if ( PaKLong( s->file,&uData) != UNZ_OK ) {
		error = UNZ_ERRNO;
	} else if ( ( error == UNZ_OK ) && (uData!=s->currentFileInfo.crc) &&
		                      ( ( uFlags & 8 ) == 0 ) ) {
		error = UNZ_BADZIPFILE;
	}
	if ( PaKLong( s->file,&uData) != UNZ_OK ) {
		error = UNZ_ERRNO;
	} else if ( ( error == UNZ_OK ) && (uData!=s->currentFileInfo.compressedSize) &&
							  ( ( uFlags & 8 ) == 0 ) ) {
		error = UNZ_BADZIPFILE;
	}
	if ( PaKLong( s->file,&uData) != UNZ_OK ) {
		error = UNZ_ERRNO;
	} else if ( ( error == UNZ_OK ) && (uData!=s->currentFileInfo.uncompressedSize) &&
							  ( ( uFlags & 8 ) == 0 ) ) {
		error = UNZ_BADZIPFILE;
	}

	if ( PaKShort( s->file,&fileNameSize) != UNZ_OK ) {
		error = UNZ_ERRNO;
	} else if ( ( error == UNZ_OK ) && ( fileNameSize!=s->currentFileInfo.fileNameSize) )
		error = UNZ_BADZIPFILE;
	}
	*piSizeVar += (unsigned int)fileNameSize;

	if ( PaKShort( s->file,&size_extra_field ) != UNZ_OK ) {
		error = UNZ_ERRNO;
	}
	*pextraFieldOffset= s->internalFileInfo.currentFileOffset + SIZEZIPLOCALHEADER + fileNameSize;
	*pextraFieldSize = (unsigned int)size_extra_field;

	*piSizeVar += (unsigned int)size_extra_field;

	return error;
}

// Open for reading data the current file in the zipfile.
// If there is no error and the file is opened, the return value is UNZ_OK.
extern int PAK_OpenCurrentFile( anPaKBFile file ) {
	int error = UNZ_OK;
	int Store;
	unsigned int iSizeVar;
	pakReadInfo_s* zippedFileInfo;
	unsigned long extraFieldOffset;  // offset of the static extra field
	unsigned int  extraFieldSize;    // size of the static extra field

	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}
	pakBFile_s *s = static_cast<pakBFile_s *>( file );//*s = (pakBFile_s *)file;
	if ( !s->fileStatusCode ) {
		return UNZ_PARAMERROR;
	}
    if ( s->zippedFileInfo != nullptr ) {
        PAK_CloseCurrentFile( file );
	}
	if ( unzlocal_CheckCurrentFileCoherencyHeader( s, &iSizeVar, &extraFieldOffset, &extraFieldSize )!= UNZ_OK ) {
		return UNZ_BADZIPFILE;
	}
	zippedFileInfo = (pakReadInfo_s *) ALLOC( sizeof( pakReadInfo_s ) );
	if ( zippedFileInfo == nullptr ) {
		return UNZ_INTERNALERROR;
	}
	zippedFileInfo->readBuffer=(char *)ALLOC(UNZ_BUFSIZE);
	zippedFileInfo->extraFieldOffset = extraFieldOffset;
	zippedFileInfo->extraFieldSize = extraFieldSize;
	zippedFileInfo->extraFieldPos=0;

	if ( zippedFileInfo->readBuffer == nullptr ) {
		TRYFREE( zippedFileInfo);
		return UNZ_INTERNALERROR;
	}

	zippedFileInfo->initialised=0;

	if (( s->currentFileInfo.compressionType!=0 ) && ( s->currentFileInfo.compressionType!=Z_DEFLATED) ) {
		error = UNZ_BADZIPFILE;
	}
	Store = s->currentFileInfo.compressionType==0;

	zippedFileInfo->waitCRC32=s->currentFileInfo.crc;
	zippedFileInfo->crc32=0;
	zippedFileInfo->compressionType = s->currentFileInfo.compressionType;
	zippedFileInfo->file=s->file;
	zippedFileInfo->unCompressedSize=s->unCompressedSize;

    zippedFileInfo->stream.total_out = 0;

	if ( !Store ) {
	  zippedFileInfo->stream.zalloc = (alloc_func)0;
	  zippedFileInfo->stream.zfree = (free_func)0;
	  zippedFileInfo->stream.opaque = (voidp)0;

	  error=inflateInit2( &zippedFileInfo->stream, -MAX_WBITS );
	  if ( error == Z_OK ) {
	    zippedFileInfo->initialised = 1;
	}
	zippedFileInfo->compressedRead = s->currentFileInfo.compressedSize;
	zippedFileInfo->unCompressedRead = s->currentFileInfo.uncompressedSize;


	zippedFileInfo->internalLocation = s->internalFileInfo.currentFileOffset + SIZEZIPLOCALHEADER + iSizeVar;

	zippedFileInfo->stream.avail_in = (unsigned int)0;

	s->zippedFileInfo = zippedFileInfo;
    return UNZ_OK;
}

/*
  Read bytes from the current file.
  buf contain buffer where data must be copied
  len the size of buf.

  return the number of byte copied if somes bytes are copied
  return 0 if the end of file was reached
  return <0 with error code if there is an error
    (UNZ_ERRNO for IO error, or zLib error for uncompress error)
*/
extern int unzReadCurrentFile( anPaKBFile file, void *buf, unsigned len ) {
	int error = UNZ_OK;
	unsigned int iRead = 0;

	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}

	pakBFile_s *s = static_cast<pakBFile_s *>( file );//*s = (pakBFile_s *)file;
    pakReadInfo_s *zippedFileInfo = s->zippedFileInfo;

	if ( zippedFileInfo == nullptr ) {
		return UNZ_PARAMERROR;
	}

	if ( ( zippedFileInfo->readBuffer == nullptr ) ) {
		return UNZ_END_OF_LIST_OF_FILE;
	}
	if ( len == 0 ) {
		return 0;
	}
	zippedFileInfo->stream.next_out = (Byte *)buf;
	zippedFileInfo->stream.avail_out = ( unsigned int )len;

	if ( len > zippedFileInfo->unCompressedRead ) {
		zippedFileInfo->stream.avail_out = ( unsigned int )zippedFileInfo->unCompressedRead;
	}
	while ( zippedFileInfo->stream.avail_out > 0 ) {
		if ( ( zippedFileInfo->stream.avail_in == 0 ) && ( zippedFileInfo->compressedRead > 0 ) ) {
			unsigned int readSize = UNZ_BUFSIZE;
			if ( zippedFileInfo->compressedRead < readSize ) {
				readSize = ( unsigned int )zippedFileInfo->compressedRead;
			}
			if ( readSize == 0 ) {
				return UNZ_EOF;
			}

			if ( s->currentFileInfo.compressedSize == zippedFileInfo->compressedRead ) {
				if ( zippedFileInfo->file->Seek( zippedFileInfo->internalLocation + zippedFileInfo->unCompressedSize, FS_SEEK_SET ) != 0 ) {
					return UNZ_ERRNO;
				}
			}
			if ( zippedFileInfo->file->Read( zippedFileInfo->readBuffer, readSize ) != ( int )readSize ) {
				return UNZ_ERRNO;
			}
			zippedFileInfo->internalLocation += readSize;
			zippedFileInfo->compressedRead -= readSize;
			zippedFileInfo->stream.next_in = (Byte *)zippedFileInfo->readBuffer;
			zippedFileInfo->stream.avail_in = (unsigned int)readSize;
		}

		if ( zippedFileInfo->compressionType == 0 ) {
			unsigned int uDoCopy;
			if ( zippedFileInfo->stream.avail_out < zippedFileInfo->stream.avail_in ) {
				uDoCopy = zippedFileInfo->stream.avail_out;
		} else {
			uDoCopy = zippedFileInfo->stream.avail_in;
			for ( int i = 0; i < uDoCopy; i++ ) {
				*( zipFileInfo->stream.next_out + i ) = *( zippedFileInfo->stream.next_in + i );
			}
			zippedFileInfo->crc32 = crc32( zippedFileInfo->crc32, zippedFileInfo->stream.next_out, uDoCopy );
			zippedFileInfo->unCompressedRead -= uDoCopy;
			zippedFileInfo->stream.avail_in -= uDoCopy;
			zippedFileInfo->stream.avail_out -= uDoCopy;
			zippedFileInfo->stream.next_out += uDoCopy;
			zippedFileInfo->stream.next_in += uDoCopy;
            zippedFileInfo->stream.total_out += uDoCopy;
			iRead += uDoCopy;
		} else {
			unsigned long unCompressedSize, compressedSize;
			const Byte *bufBefore;
			unsigned long uOutThis;
			int flush = Z_SYNC_FLUSH;

			compressedSize = zipFileInfo->stream.total_out;
			bufBefore = zipFileInfo->stream.next_out;

			error = inflate( &zipFileInfo->stream, flush );

			compressedSize = zipFileInfo->stream.total_out;
			uOutThis = compressedSize - unCompressedSize;

			zippedFileInfo->crc32 = crc32( zippedFileInfo->crc32, bufBefore, (unsigned int)( uOutThis ) );
			zippedFileInfo->unCompressedRead -= uOutThis;

			iRead += (unsigned int)( compressedSize - unCompressedSize );

			if ( error == Z_STREAM_END ) {
				return ( iRead == 0 ) ? UNZ_EOF : iRead;
			}
			if ( error != Z_OK ) {
				break;
			}
		}
	}

	if ( error == Z_OK ) {
		return iRead;
	}
	return error;
}

// Give the current position in uncompressed data
extern long PAK_Tell( anPaKBFile file ) {
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}

	pakBFile_s *s = static_cast<pakBFile_s *>( file );//*s = (pakBFile_s *)file;
   pakReadInfo_s *zipFileInfo = s->zippedFileInfo;

	if ( zipFileInfo == nullptr ) {
		return UNZ_PARAMERROR;
	}
	return (long)zipFileInfo->stream.total_out;
}

// return 1 if the end of file was reached, 0 elsewhere
extern int unzeof( anPaKBFile file ) {
	pakReadInfo_s *zippedFileInfo;
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}
	pakBFile_s *s = static_cast<pakBFile_s *>( file );//*s = (pakBFile_s *)file;
    zippedFileInfo=s->zippedFileInfo;

	if ( zippedFileInfo_inf o== nullptr ) {
		return UNZ_PARAMERROR;
	}
	if ( zippedFileInfo->unCompressedRead == 0 ) {
		return 1;
	} else {
		return 0;
	}
}

/*
  Read extra field from the current file (opened by PAK_OpenCurrentFile)
  This is the static-header version of the extra field ( sometimes, there is
    more info in the static-header version than in the central-header)

  if buf== nullptr, it return the size of the static extra field that can be read

  if buf!=nullptr, len is the size of the buffer, the extra header is copied in
	buf.
  the return value is the number of bytes copied in buf, or (if <0 )
	the error code
*/
extern int unzGetLocalExtrafield( anPaKBFile file,void *buf,unsigned len ) {
	pakReadInfo_s *zippedFileInfo;
	unsigned int fRead;
	unsigned long entryFieldReadSize;

	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}
	pakBFile_s *s = static_cast<pakBFile_s *>( file );//*s = (pakBFile_s *)file;
    zippedFileInfo=s->zippedFileInfo;

	if ( zippedFileInfo== nullptr ) {
		return UNZ_PARAMERROR;
	}
	entryFieldReadSize = ( zippedFileInfo->extraFieldSize - zippedFileInfo->extraFieldPos );

	if ( buf == nullptr ) {
		return ( int )entryFieldReadSize;
	}
	if ( len > entryFieldReadSize ) {
		fRead = (unsigned int)entryFieldReadSize;
	} else {
		fRead = (unsigned int)len;
	}
	if ( fRead == 0 ) {
		return 0;
	}
	if ( zippedFileInfo->file->Seek( zippedFileInfo->extraFieldOffset + zippedFileInfo->extraFieldPos, FS_SEEK_SET ) != 0 ) {
		return UNZ_ERRNO;
	}
	if ( zippedFileInfo->file->Read( buf, entryFieldReadSize ) != ( int )entryFieldReadSize ) {
		return UNZ_ERRNO;
	}
	return ( int )fRead;
}


// Close the file in zip opened with unzipOpenCurrentFile
// Return UNZ_CRCERROR if all the file was read but the CRC is not good
extern int PAK_CloseCurrentFile( anPaKBFile file ) {
	int error = UNZ_OK;
	pakReadInfo_s *zippedFileInfo;
	if ( file == nullptr ) {
		return UNZ_PARAMERROR;
	}

	pakBFile_s *s = static_cast<pakBFile_s *>( file );//*s = (pakBFile_s *)file;
    zippedFileInfo = s->zippedFileInfo;

	if ( zippedFileInfo== nullptr ) {
		return UNZ_PARAMERROR;
	}

	if ( zippedFileInfo->unCompressedRead == 0 ) {
		if ( zippedFileInfo->crc32 != zippedFileInfo->waitCRC32 ) {
			error = UNZ_CRCERROR;
		}
	}

	TRYFREE( zippedFileInfo->readBuffer );
	zippedFileInfo->readBuffer = nullptr;

	if ( zippedFileInfo->initialised ) {
		inflateEnd( &zippedFileInfo->stream );
	}

	zippedFileInfo->initialised = 0;
	TRYFREE( zippedFileInfo );

    s->zippedFileInfo=nullptr;

	return error;
}

// Get the global comment string of the ZipFile, in the szComment buffer.
// uSizeBuf is the size of the szComment buffer. return the number of byte copied or an error code <
extern int PAK_GetComments( anPaKBFile file, char *szComment, unsigned long uSizeBuf ) {
    if ( file == nullptr ) {
        return UNZ_PARAMERROR;
	}
    pakBFile_s *s = static_cast<pakBFile_s *>( file );

    unsigned long readComments = uSizeBuf;
    if ( readComments > s->globalInfo.commentSize ) {
        readComments = s->globalInfo.commentSize;
	    if ( s->file->Seek( s->dirBeginingPos + 22, FS_SEEK_SET ) == 0 ) {
	        return UNZ_ERRNO;
		}

	    if ( szComment != nullptr && readComments > 0 ) {
	        if ( s->file->Read( szComment, readComments ) != static_cast<int>( readComments ) ) {
	            return UNZ_ERRNO;
			}
		}
	        *( szComment + readComments ) = '\0';
    }

    return static_cast<int>( readComments );
}
