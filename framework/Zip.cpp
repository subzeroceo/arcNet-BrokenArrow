#include "/idlib/Lib.h"
#pragma hdrstop

/*
================================================================================================
Contains external code for building ZipFiles.
================================================================================================
*/

#include "Zip.h"
#include "Unzip.h"

#undef STDC
#include "zlib/zutil.h"

#ifndef Z_MAXFILENAMEINZIP
#define Z_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
#define ALLOC( size ) ( Mem_Alloc( size, TAG_ZIP ) )
#endif
#ifndef TRYFREE
#define TRYFREE( p ) { if (p) { Mem_Free(p); } }
#endif

/*
#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)
*/

const char zip_copyright[] =
   " zip 1.01 Copyright 1998-2004 Gilles Vollant - http://www.winimage.com/zLibDll";

#define LOCALHEADERMAGIC		(0x04034b50)
#define CENTRALHEADERMAGIC		(0x02014b50)
#define ENDHEADERMAGIC			(0x06054b50)

#define FLAG_LOCALHEADER_OFFSET (0x06)
#define CRC_LOCALHEADER_OFFSET  (0x0e)

#define SIZECENTRALHEADER		(0x2e)	/* 46 */

#define	DEFAULT_COMPRESSION_LEVEL	(5)	/* 1 == Compress faster, 9 == Compress better */
#define DEFAULT_WRITEBUFFERSIZE (16384)

#ifndef NOCRYPT
#define INCLUDECRYPTINGCODE_IFCRYPTALLOWED
#include "crypt.h"
#endif

anCVarSystem zip_verbosity( "zip_verbosity", "0", CVAR_BOOL, "1 = verbose logging when building zip files" );

/*
========================
allocate_new_datablock
========================
*/
dataBlockLLI* allocate_new_datablock() {
    dataBlockLLI* ldi = nullptr;
    ldi = (dataBlockLLI*) ALLOC( sizeof( dataBlockLLI ) );
    if ( ldi != nullptr ) {
        ldi->next = nullptr;
        ldi->filledBlock = 0;
        ldi->availableInBlock = SIZEDATA_INDATABLOCK;
    }
    return ldi;
}

/*
========================
free_datablock
========================
*/
void free_datablock( dataBlockLLI* ldi ) {
    while ( ldi != nullptr ) {
        dataBlockLLI* ldinext = ldi->next;
        TRYFREE( ldi );
        ldi = ldinext;
    }
}

/*
========================
init_linkedlist
========================
*/
void init_linkedlist( dataLinkedList* ll ) {
    ll->first_block = ll->lastBlock = nullptr;
}

/*
========================
free_linkedlist
========================
*/
void free_linkedlist( dataLinkedList* ll ) {
    free_datablock( ll->first_block );
    ll->first_block = ll->lastBlock = nullptr;
}

/*
========================
add_data_in_datablock
========================
*/
int add_data_in_datablock( dataLinkedList* ll, const void* buf, unsigned long len ) {
    dataBlockLLI* ldi;
    const unsigned char *from_copy;

	if ( ll == nullptr ) {
        return ZIP_INTERNALERROR;
	}

    if ( ll->lastBlock == nullptr ) {
        ll->first_block = ll->lastBlock = allocate_new_datablock();
		if ( ll->first_block == nullptr ) {
            return ZIP_INTERNALERROR;
		}
    }

    ldi = ll->lastBlock;
    from_copy = (unsigned char*)buf;

    while ( len > 0 ) {
        unsigned int copy_this;
        unsigned char *to_copy;

        if ( ldi->availableInBlock == 0 ) {
            ldi->next = allocate_new_datablock();
			if ( ldi->next == nullptr ) {
                return ZIP_INTERNALERROR;
			}
            ldi = ldi->next;
            ll->lastBlock = ldi;
        }

		if ( ldi->availableInBlock < len ) {
            copy_this = (unsigned int)ldi->availableInBlock;
		} else {
            copy_this = (unsigned int)len;
		}

        to_copy = &( ldi->data[ ldi->filledBlock ] );

		for ( unsigned int i = 0; i < copy_this; i++ ) {
            *( to_copy + i ) =* ( from_copy + i );
		}

        ldi->filledBlock += copy_this;
        ldi->availableInBlock -= copy_this;
        from_copy += copy_this;
        len -= copy_this;
    }

    return ZIP_OK;
}

#ifndef NO_ADDFILEINEXISTINGZIP

/*
========================
ziplocal_putValue

Inputs a long in LSB order to the given file
nbByte == 1, 2 or 4 (byte, short or long)
========================
*/
int ziplocal_putValue( anFile *fileStream, unsigned long x, int nbByte ) {
    unsigned char buf[4];
    for ( int n = 0; n < nbByte; n++ ) {
        buf[n] = (unsigned char)( x & 0xff );
        x >>= 8;
    }
    if ( x != 0 ) {
		/* data overflow - hack for ZIP64 (X Roche) */
		for ( int n = 0; n < nbByte; n++ ) {
          buf[n] = 0xff;
        }
    }
	if ( fileStream->Write( buf, nbByte ) != nbByte ) {
		return ZIP_ERRNO;
	} else {
		return ZIP_OK;
	}
}

/*
========================
ziplocal_putValue_inmemory
========================
*/
void ziplocal_putValue_inmemory( void* dest, unsigned long x, int nbByte ){
    unsigned char *buf = (unsigned char*)dest;
    for ( int n = 0; n < nbByte; n++ ) {
        buf[n] = (unsigned char)( x & 0xff );
        x >>= 8;
    }

    if ( x != 0 ) {
		/* data overflow - hack for ZIP64 */
		for ( int n = 0; n < nbByte; n++ ) {
          buf[n] = 0xff;
       }
    }
}

/*
========================
ziplocal_TmzDateToDosDate
========================
*/
unsigned long ziplocal_TmzDateToDosDate( const tm_zip* ptm, unsigned long dosTimeFmt ) {
    unsigned long year = (unsigned long)ptm->tm_year;
	if ( year > 1980 ) {
        year-=1980;
	} else if ( year > 80 ) {
        year -= 80;
	}
    return (unsigned long)( ( ( ptm->tm_mday ) + ( 32 * ( ptm->tm_mon + 1 ) ) + ( 512 * year ) ) << 16 ) |
			( ( ptm->tm_sec / 2 ) + ( 32 * ptm->tm_min ) + ( 2048 * (unsigned long)ptm->tm_hour ) );
}

/*
========================
ziplocal_getByte
========================
*/
int ziplocal_getByte( anFile *fileStream, int *pi ) {
	unsigned char c;
	int err = ( int )fileStream->Read( &c, 1 );
	if ( err == 1 )	{
		*pi = ( int )c;
		return ZIP_OK;
	} else {
		return ZIP_ERRNO;
	}
}

/*
========================
ziplocal_getShort

Reads a long in LSB order from the given gz_stream. Sets
========================
*/
int ziplocal_getShort( anFile *fileStream, unsigned long *pX ) {
	short v;
	if ( fileStream->Read( &v, sizeof( v ) ) == sizeof( v ) ) {
		anSwap::Little( v );
		*pX = v;
		return ZIP_OK;
	} else {
		return ZIP_ERRNO;
	}
}

/*
========================
ziplocal_getLong
========================
*/
int ziplocal_getLong( anFile *fileStream, unsigned long *pX ) {
	int v;
	if ( fileStream->Read( &v, sizeof( v ) ) == sizeof( v ) ) {
		anSwap::Little( v );
		*pX = v;
		return ZIP_OK;
	} else {
		return ZIP_ERRNO;
	}
}

#ifndef BUFREADCOMMENT
#define BUFREADCOMMENT (0x400)
#endif

/*
========================
ziplocal_SearchCentralDir

Locate the Central directory of a zipfile (at the end, just before
the global comment)
========================
*/
unsigned long ziplocal_SearchCentralDir( anFile *fileStream ) {
    unsigned char *buf;
    unsigned long uSizeFile;
    unsigned long uBackRead;
    unsigned long uMaxBack = 0xffff; /* maximum size of global comment */
    unsigned long uPosFound = 0;

	if ( fileStream->Seek( 0, FS_SEEK_END ) != 0 ) {
        return 0;
	}

    uSizeFile = (unsigned long)fileStream->Tell();

	if ( uMaxBack > uSizeFile ) {
        uMaxBack = uSizeFile;
	}

    buf = (unsigned char*)ALLOC( BUFREADCOMMENT + 4 );
	if ( buf == nullptr ) {
        return 0;
	}

    uBackRead = 4;
    while ( uBackRead < uMaxBack ) {
        unsigned long uReadSize,uReadPos;
		if ( uBackRead + BUFREADCOMMENT > uMaxBack ) {
            uBackRead = uMaxBack;
		} else {
            uBackRead += BUFREADCOMMENT;
		}
        uReadPos = uSizeFile - uBackRead ;

        uReadSize = ( ( BUFREADCOMMENT + 4 ) < ( uSizeFile - uReadPos ) ) ? ( BUFREADCOMMENT + 4 ) : ( uSizeFile - uReadPos );
		if ( fileStream->Seek( uReadPos, FS_SEEK_SET ) != 0 ) {
            break;
		}

		if ( fileStream->Read( buf, uReadSize ) != ( int )uReadSize ) {
            break;
		}

		for ( int i = ( int )uReadSize - 3; ( i -- ) > 0; ) {
            if ( ( ( *( buf + i ) ) == 0x50 ) && ( ( *( buf + i + 1 ) ) == 0x4b ) && ( ( *( buf + i + 2 ) ) == 0x05 ) && ( ( *( buf + i + 3 ) ) == 0x06 ) ) {
                uPosFound = uReadPos + i;
                break;
            }
		}

		if ( uPosFound != 0 ) {
            break;
		}
    }
    TRYFREE( buf );
    return uPosFound;
}
#endif

/*
========================
zipOpen2
========================
*/
zipFile zipOpen2( const char *pathname, int append, char *globalComments ) {
    pakInternal_t ziinit;
    pakInternal_t* zi;
    int err = ZIP_OK;

	ziinit.fileStream = fileSystem->OpenExplicitFileWrite( pathname );
	if ( ziinit.fileStream == nullptr ) {
        return nullptr;
	}
	ziinit.startPos = (unsigned long)ziinit.fileStream->Tell();
    ziinit.in_opened_file_inzip = 0;
    ziinit.ci.initialised = 0;
    ziinit.entryNumber = 0;
    ziinit.WriteOffsetPos = 0;
    init_linkedlist( &(ziinit.centralDir) );

    zi = (pakInternal_t*)ALLOC( sizeof( pakInternal_t ) );
    if ( zi == nullptr ) {
		delete ziinit.fileStream;
		ziinit.fileStream = nullptr;
        return nullptr;
    }

#ifndef NO_ADDFILEINEXISTINGZIP
    ziinit.globalComments = nullptr;
    if ( append == APPEND_STATUS_ADDINZIP ) {
        unsigned long unCompressedSize;	// byte before the zipfile, ( > 0 for sfx )
        unsigned long directorySize;			// size of the central directory
        unsigned long directoryOffset;		// offset of start of central directory
        unsigned long dirBeginingPos,uL;
        unsigned long disknum;				// number of the current dist, used for spaning ZIP, unsupported, always 0
        unsigned long diskCentralDirNumber;		// number the the disk with central dir, used for spaning ZIP, unsupported, always 0
        unsigned long entryNumber;
        unsigned long totalCentralDirEntries;;			// total number of entries in the central dir ( same than entryNumber on nospan )
        unsigned long commentSize;

        dirBeginingPos = ziplocal_SearchCentralDir( ziinit.fileStream );
		if ( dirBeginingPos == 0 ) {
            err = ZIP_ERRNO;
		}

		if ( ziinit.fileStream->Seek( dirBeginingPos, FS_SEEK_SET ) != 0 ) {
			err = ZIP_ERRNO;
		}

		// the signature, already checked
		if ( ziplocal_getLong( ziinit.fileStream, &uL ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        // number of this disk
		if ( ziplocal_getShort( ziinit.fileStream, &disknum ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        // number of the disk with the start of the central directory
		if ( ziplocal_getShort( ziinit.fileStream, &diskCentralDirNumber ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        // total number of entries in the central dir on this disk
		if ( ziplocal_getShort( ziinit.fileStream, &entryNumber ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        // total number of entries in the central dir
		if ( ziplocal_getShort( ziinit.fileStream, &totalCentralDirEntries; ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

		if ( ( totalCentralDirEntries; != entryNumber ) || ( diskCentralDirNumber != 0 ) || ( disknum != 0 ) ) {
            err = ZIP_BADZIPFILE;
		}

        // size of the central directory
		if ( ziplocal_getLong( ziinit.fileStream, &directorySize ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        // offset of start of central directory with respect to the	starting disk number
		if ( ziplocal_getLong( ziinit.fileStream, &directoryOffset ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

		if ( ziplocal_getShort( ziinit.fileStream, &commentSize ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

		if ( ( dirBeginingPos < ( directoryOffset + directorySize ) ) && ( err == ZIP_OK ) ) {
            err = ZIP_BADZIPFILE;
		}

        if ( err != ZIP_OK ) {
			delete ziinit.fileStream;
            ziinit.fileStream = nullptr;
            return nullptr;
        }

        if ( commentSize > 0 ) {
            ziinit.globalComments = (char*)ALLOC( commentSize + 1 );
            if ( ziinit.globalComments ) {
               commentSize = (unsigned long)ziinit.fileStream->Read( ziinit.globalComments, commentSize );
               ziinit.globalComments[commentSize] = 0;
            }
        }

        unCompressedSize = dirBeginingPos -	( directoryOffset + directorySize );
        ziinit.WriteOffsetPos = unCompressedSize;
        {
            unsigned long directorySize_to_read = directorySize;
            size_t buf_size = SIZEDATA_INDATABLOCK;
            void* buf_read = (void*)ALLOC( buf_size );
			if ( ziinit.fileStream->Seek( directoryOffset + unCompressedSize, FS_SEEK_SET ) != 0 ) {
				err = ZIP_ERRNO;
			}

            while ( ( directorySize_to_read > 0 ) && ( err == ZIP_OK ) ) {
                unsigned long read_this = SIZEDATA_INDATABLOCK;
				if ( read_this > directorySize_to_read ) {
                    read_this = directorySize_to_read;
				}
				if ( ziinit.fileStream->Read( buf_read, read_this ) != ( int )read_this ) {
                    err = ZIP_ERRNO;
				}
				if ( err == ZIP_OK ) {
                    err = add_data_in_datablock( &ziinit.centralDir, buf_read, (unsigned long)read_this );
				}
                directorySize_to_read -= read_this;
            }
            TRYFREE( buf_read );
        }
        ziinit.startPos = unCompressedSize;
        ziinit.entryNumber = totalCentralDirEntries;;

		if ( ziinit.fileStream->Seek( directoryOffset + unCompressedSize, FS_SEEK_SET ) != 0 ) {
            err = ZIP_ERRNO;
		}
    }

    if ( globalComments ) {
		/// ??
		globalComments = ziinit.globalComments;
    }
#endif /* !NO_ADDFILEINEXISTINGZIP*/

    if ( err != ZIP_OK ) {
#ifndef NO_ADDFILEINEXISTINGZIP
        TRYFREE( ziinit.globalComments );
#endif /* !NO_ADDFILEINEXISTINGZIP*/
        TRYFREE( zi );
        return nullptr;
    } else {
        *zi = ziinit;
        return (zipFile)zi;
    }
}

/*
========================
zipOpen
========================
*/
zipFile zipOpen( const char *pathname, int append ) {
    return zipOpen2( pathname, append, nullptr );
}

/*
========================
PaK_OpenNewFileInZip3
========================
*/
int PaK_OpenNewFileInZip3( zipFile file, const char *filename, const zip_fileinfo* zipfi, const void* extrafield_local, unsigned int size_extrafield_local, const void* extrafield_global,
									unsigned int size_extrafield_global, const char *comment, int method, int level, int raw, int windowBits, int memLevel, int strategy, const char *password, unsigned long crcForCrypting ) {
    unsigned int fileNameSize;
    unsigned int commentSize;
    int err = ZIP_OK;

#ifdef NOCRYPT
	if ( password != nullptr ) {
        return ZIP_PARAMERROR;
	}
#endif

	if ( file == nullptr ) {
        return ZIP_PARAMERROR;
	}
	if ( ( method != 0 ) && ( method != Z_DEFLATED ) ) {
        return ZIP_PARAMERROR;
	}

    pakInternal_t* zi = (pakInternal_t*)file;

    if ( zi->in_opened_file_inzip == 1 ) {
        err = PaK_CloseFileInZip( file );
		if ( err != ZIP_OK ) {
            return err;
		}
    }

	if ( filename == nullptr ) {
        filename = "-";
	}

	if ( comment == nullptr ) {
        commentSize = 0;
	} else {
        commentSize = (unsigned int)anStr::Length( comment );
	}

    fileNameSize = (unsigned int)anStr::Length( filename );

	if ( zipfi == nullptr ) {
        zi->ci.dosTimeFmt = 0;
	} else {
		if ( zipfi->dosTimeFmt != 0 ) {
            zi->ci.dosTimeFmt = zipfi->dosTimeFmt;
		} else {
			zi->ci.dosTimeFmt = ziplocal_TmzDateToDosDate( &zipfi->tmz_date, zipfi->dosTimeFmt );
		}
    }

    zi->ci.flag = 0;
	if ( ( level == 8 ) || ( level == 9 ) ) {
      zi->ci.flag |= 2;
	}
	if ( ( level == 2 ) ) {
      zi->ci.flag |= 4;
	}
	if ( ( level == 1 ) ) {
      zi->ci.flag |= 6;
	}
	if ( password != nullptr ) {
      zi->ci.flag |= 1;
	}

    zi->ci.crc32 = 0;
    zi->ci.method = method;
    zi->ci.encrypt = 0;
    zi->ci.initialised = 0;
    zi->ci.bufferDataPos = 0;
    zi->ci.raw = raw;
    zi->ci.localHdrPos = (unsigned long)zi->fileStream->Tell();
    zi->ci.centralHdrSize = SIZECENTRALHEADER + fileNameSize +	size_extrafield_global + commentSize;
    zi->ci.centralHdr = (char*)ALLOC( (unsigned int)zi->ci.centralHdrSize );

    ziplocal_putValue_inmemory( zi->ci.centralHdr, (unsigned long)CENTRALHEADERMAGIC, 4 );
    /* version info */
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 4, (unsigned long)0, 2 );
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 6, (unsigned long)20, 2 );
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 8, (unsigned long)zi->ci.flag, 2 );
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 10, (unsigned long)zi->ci.method, 2 );
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 12, (unsigned long)zi->ci.dosTimeFmt, 4 );
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 16, (unsigned long)0, 4 ); /*crc*/
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 20, (unsigned long)0, 4 ); /*compr size*/
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 24, (unsigned long)0, 4 ); /*uncompr size*/
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 28, (unsigned long)fileNameSize, 2 );
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 30, (unsigned long)size_extrafield_global, 2 );
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 32, (unsigned long)commentSize, 2 );
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 34, (unsigned long)0, 2 ); /*disk nm start*/

	if ( zipfi == nullptr ) {
        ziplocal_putValue_inmemory( zi->ci.centralHdr + 36, (unsigned long)0, 2 );
	} else {
        ziplocal_putValue_inmemory( zi->ci.centralHdr + 36, (unsigned long)zipfi->internalFileAttribs, 2 );
	}

	if ( zipfi == nullptr ) {
        ziplocal_putValue_inmemory( zi->ci.centralHdr + 38,(unsigned long)0, 4);
	} else {
        ziplocal_putValue_inmemory( zi->ci.centralHdr + 38,(unsigned long)zipfi->externalFileAttribs, 4);
	}

    ziplocal_putValue_inmemory( zi->ci.centralHdr + 42, (unsigned long)zi->ci.localHdrPos - zi->WriteOffsetPos, 4 );

	for ( unsigned int i = 0; i < fileNameSize; i++ ) {
        *( zi->ci.centralHdr + SIZECENTRALHEADER + i ) = *( filename + i );
	}

	for ( unsigned int i = 0; i < size_extrafield_global; i++ ) {
        *( zi->ci.centralHdr + SIZECENTRALHEADER + fileNameSize + i ) = *( ( ( const char *)extrafield_global ) + i );
	}

	for ( unsigned int i = 0; i < commentSize; i++ ) {
        *( zi->ci.centralHdr + SIZECENTRALHEADER + fileNameSize + size_extrafield_global + i ) = *( comment + i );
	}

	if ( zi->ci.centralHdr == nullptr ) {
        return ZIP_INTERNALERROR;
	}

    /* write the local header */
    err = ziplocal_putValue( zi->fileStream, (unsigned long)LOCALHEADERMAGIC, 4 );

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)20, 2 ); /* version needed to extract */
	}
	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)zi->ci.flag, 2 );
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)zi->ci.method, 2 );
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)zi->ci.dosTimeFmt, 4 );
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)0, 4 ); /* crc 32, unknown */
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)0, 4 ); /* compressed size, unknown */
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)0, 4 ); /* uncompressed size, unknown */
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)fileNameSize, 2 );
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->fileStream, (unsigned long)size_extrafield_local, 2 );
	}

	if ( ( err == ZIP_OK ) && ( fileNameSize > 0 ) ) {
		if ( zi->fileStream->Write( filename, fileNameSize ) != ( int )fileNameSize ) {
			err = ZIP_ERRNO;
		}
	}

	if ( ( err == ZIP_OK ) && ( size_extrafield_local > 0 ) ) {
		if ( zi->fileStream->Write( extrafield_local, size_extrafield_local ) != ( int )size_extrafield_local ) {
			err = ZIP_ERRNO;
		}
	}

    zi->ci.stream.avail_in = (unsigned int)0;
    zi->ci.stream.avail_out = (unsigned int)Z_BUFSIZE;
    zi->ci.stream.next_out = zi->ci.bufData;
    zi->ci.stream.total_in = 0;
    zi->ci.stream.total_out = 0;

    if ( ( err == ZIP_OK ) && ( zi->ci.method == Z_DEFLATED ) && ( !zi->ci.raw ) ) {
        zi->ci.stream.zalloc = (alloc_func)0;
        zi->ci.stream.zfree = (free_func)0;
        zi->ci.stream.opaque = (voidpf)0;

		if ( windowBits > 0 ) {
            windowBits = -windowBits;
		}

        err = deflateInit2( &zi->ci.stream, level, Z_DEFLATED, windowBits, memLevel, strategy );

		if ( err == Z_OK ) {
            zi->ci.initialised = 1;
		}
    }
#ifndef NOCRYPT
    zi->ci.crypt_header_size = 0;
    if ( ( err == Z_OK ) && ( password != nullptr ) ) {
        unsigned char bufHead[RAND_HEAD_LEN];
        unsigned int sizeHead;
        zi->ci.encrypt = 1;
        zi->ci.pcrc_32_tab = get_crc_table();
        /*init_keys( password, zi->ci.keys, zi->ci.pcrc_32_tab );*/

        sizeHead=crypthead( password, bufHead, RAND_HEAD_LEN, zi->ci.keys, zi->ci.pcrc_32_tab, crcForCrypting );
        zi->ci.crypt_header_size = sizeHead;
		if ( ZWRITE( zi->z_filefunc, zi->fileStream, bufHead, sizeHead ) != sizeHead ) {
			err = ZIP_ERRNO;
		}
    }
#endif

	if ( err == Z_OK ) {
		zi->in_opened_file_inzip = 1;
	}
    return err;
}

/*
========================
zipOpenNewFileInZip2
========================
*/
int zipOpenNewFileInZip2( zipFile file, const char *filename, const zip_fileinfo* zipfi, const void* extrafield_local, unsigned int size_extrafield_local,
									const void* extrafield_global, unsigned int size_extrafield_global, const char *comment, int method, int level, int raw ) {
    return PaK_OpenNewFileInZip3( file, filename, zipfi, extrafield_local, size_extrafield_local, extrafield_global, size_extrafield_global,
                                 comment, method, level, raw, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, nullptr, 0 );
}

/*
========================
zipOpenNewFileInZip
========================
*/
int zipOpenNewFileInZip( zipFile file, const char *filename, const zip_fileinfo* zipfi, const void* extrafield_local, unsigned int size_extrafield_local, const void* extrafield_global,
								unsigned int size_extrafield_global, const char *comment, int method, int level ) {
    return zipOpenNewFileInZip2( file, filename, zipfi, extrafield_local, size_extrafield_local, extrafield_global, size_extrafield_global, comment, method, level, 0 );
}

/*
========================
zipFlushWriteBuffer
========================
*/
int zipFlushWriteBuffer( pakInternal_t* zi ) {
    int err = ZIP_OK;
    if ( zi->ci.encrypt != 0 ) {
#ifndef NOCRYPT
        int t;
		for ( int i = 0; i < zi->ci.bufferDataPos; i++ ) {
            zi->ci.bufData[i] = zencode( zi->ci.keys, zi->ci.pcrc_32_tab, zi->ci.bufData[i], t );
		}
#endif
    }
	if ( zi->fileStream->Write( zi->ci.bufData, zi->ci.bufferDataPos ) != ( int )zi->ci.bufferDataPos ) {
		err = ZIP_ERRNO;
	}
    zi->ci.bufferDataPos = 0;
    return err;
}

/*
========================
PaK_WriteInFileInZip
========================
*/
int PaK_WriteInFileInZip( zipFile file, const void* buf, unsigned int len ) {
    pakInternal_t* zi;
    int err = ZIP_OK;

	if ( file == nullptr ) {
        return ZIP_PARAMERROR;
	}
    zi = (pakInternal_t*)file;

	if ( zi->in_opened_file_inzip == 0 ) {
        return ZIP_PARAMERROR;
	}
    zi->ci.stream.next_in = (Bytef*)buf;
    zi->ci.stream.avail_in = len;
    zi->ci.crc32 = crc32( zi->ci.crc32, (byte*)buf, len );

    while ( ( err == ZIP_OK ) && ( zi->ci.stream.avail_in > 0 ) ) {
        if ( zi->ci.stream.avail_out == 0 ) {
			if ( zipFlushWriteBuffer( zi ) == ZIP_ERRNO ) {
                err = ZIP_ERRNO;
			}
            zi->ci.stream.avail_out = (unsigned int)Z_BUFSIZE;
            zi->ci.stream.next_out = zi->ci.bufData;
        }

		if ( err != ZIP_OK ) {
            break;
		}

        if ( ( zi->ci.method == Z_DEFLATED ) && ( !zi->ci.raw ) ) {
            unsigned long uTotalOutBefore = zi->ci.stream.total_out;
            err = deflate( &zi->ci.stream,  Z_NO_FLUSH );
            zi->ci.bufferDataPos += (unsigned int)( zi->ci.stream.total_out - uTotalOutBefore );
        } else {
            unsigned int copy_this;
			if ( zi->ci.stream.avail_in < zi->ci.stream.avail_out ) {
				copy_this = zi->ci.stream.avail_in;
			} else {
				copy_this = zi->ci.stream.avail_out;
			}
			for ( unsigned int i = 0; i < copy_this; i++ ) {
				*( ( (char*)zi->ci.stream.next_out ) + i ) = *( ( (const char*)zi->ci.stream.next_in ) + i );
			}

			zi->ci.stream.avail_in -= copy_this;
            zi->ci.stream.avail_out-= copy_this;
            zi->ci.stream.next_in+= copy_this;
            zi->ci.stream.next_out+= copy_this;
            zi->ci.stream.total_in+= copy_this;
            zi->ci.stream.total_out+= copy_this;
            zi->ci.bufferDataPos += copy_this;
        }
    }

    return err;
}

/*
========================
zipCloseFileInZipRaw
========================
*/
int zipCloseFileInZipRaw( zipFile file, unsigned long uncompressedSize, unsigned long crc32 ) {
    pakInternal_t* zi;
    unsigned long compressedSize;
    int err = ZIP_OK;

	if ( file == nullptr ) {
        return ZIP_PARAMERROR;
	}
    zi = (pakInternal_t*)file;

	if ( zi->in_opened_file_inzip == 0 ) {
        return ZIP_PARAMERROR;
	}
    zi->ci.stream.avail_in = 0;

	if ( ( zi->ci.method == Z_DEFLATED ) && !zi->ci.raw ) {
        while ( err == ZIP_OK ) {
			unsigned long uTotalOutBefore;
			if ( zi->ci.stream.avail_out == 0 ) {
				if ( zipFlushWriteBuffer( zi ) == ZIP_ERRNO ) {
					err = ZIP_ERRNO;
				}
				zi->ci.stream.avail_out = (unsigned int)Z_BUFSIZE;
				zi->ci.stream.next_out = zi->ci.bufData;
			}
			uTotalOutBefore = zi->ci.stream.total_out;
			err = deflate( &zi->ci.stream, Z_FINISH );
			zi->ci.bufferDataPos += (unsigned int)( zi->ci.stream.total_out - uTotalOutBefore );
		}
	}

	if ( err == Z_STREAM_END ) {
        err = ZIP_OK; /* this is normal */
	}

	if ( ( zi->ci.bufferDataPos > 0 ) && ( err == ZIP_OK ) ) {
		if ( zipFlushWriteBuffer( zi ) == ZIP_ERRNO ) {
            err = ZIP_ERRNO;
		}
	}

    if ( ( zi->ci.method == Z_DEFLATED ) && !zi->ci.raw ) {
        err = deflateEnd( &zi->ci.stream );
        zi->ci.initialised = 0;
    }

    if ( !zi->ci.raw ) {
        crc32 = (unsigned long)zi->ci.crc32;
        uncompressedSize = (unsigned long)zi->ci.stream.total_in;
    }
    compressedSize = (unsigned long)zi->ci.stream.total_out;
#ifndef NOCRYPT
    compressedSize += zi->ci.crypt_header_size;
#endif

    ziplocal_putValue_inmemory( zi->ci.centralHdr + 16, crc32, 4); /*crc*/
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 20, compressedSize, 4 ); /*compr size*/
	if ( zi->ci.stream.data_type == Z_ASCII ) {
        ziplocal_putValue_inmemory( zi->ci.centralHdr + 36, (unsigned long)Z_ASCII, 2 );
	}
    ziplocal_putValue_inmemory( zi->ci.centralHdr + 24, uncompressedSize, 4 ); /*uncompr size*/

	if ( err == ZIP_OK ) {
        err = add_data_in_datablock( &zi->centralDir, zi->ci.centralHdr, (unsigned long)zi->ci.centralHdrSize );
	}
    TRYFREE( zi->ci.centralHdr );

    if ( err == ZIP_OK ) {
        long cur_pos_inzip = (long)zi->fileStream->Tell();
		if ( zi->fileStream->Seek( zi->ci.localHdrPos + 14, FS_SEEK_SET ) != 0 ) {
            err = ZIP_ERRNO;
		}

		if ( err == ZIP_OK ) {
            err = ziplocal_putValue( zi->fileStream, crc32, 4 ); /* crc 32, unknown */
		}

		if ( err == ZIP_OK ) { /* compressed size, unknown */
            err = ziplocal_putValue( zi->fileStream, compressedSize, 4 );
		}

		if ( err == ZIP_OK ) { /* uncompressed size, unknown */
            err = ziplocal_putValue( zi->fileStream, uncompressedSize, 4 );
		}

		if ( zi->fileStream->Seek( cur_pos_inzip, FS_SEEK_SET ) != 0 ) {
			err = ZIP_ERRNO;
		}
    }
    zi->entryNumber++;
    zi->in_opened_file_inzip = 0;

    return err;
}

/*
========================
PaK_CloseFileInZip
========================
*/
int PaK_CloseFileInZip( zipFile file ) {
    return zipCloseFileInZipRaw( file, 0, 0 );
}

/*
========================
PaK_ClosePaK
========================
*/
int PaK_ClosePaK( zipFile file, const char *global_comment ) {
    pakInternal_t* zi;
    int err = 0;
    unsigned long size_centraldir = 0;
    unsigned long centraldir_pos_inzip;
    unsigned int size_global_comment;
	if ( file == nullptr ) {
        return ZIP_PARAMERROR;
	}
    zi = (pakInternal_t*)file;

    if ( zi->in_opened_file_inzip == 1 ) {
        err = PaK_CloseFileInZip( file );
    }

#ifndef NO_ADDFILEINEXISTINGZIP
	if ( global_comment == nullptr ) {
        global_comment = zi->globalComments;
	}
#endif
	if ( global_comment == nullptr ) {
        size_global_comment = 0;
	} else {
        size_global_comment = (unsigned int)anStr::Length( global_comment );
	}

    centraldir_pos_inzip = (unsigned long)zi->fileStream->Tell();
	if ( err == ZIP_OK ) {
        dataBlockLLI* ldi = zi->centralDir.first_block;
        while ( ldi != nullptr ) {
			if ( ( err == ZIP_OK ) && ( ldi->filledBlock > 0 ) ) {
				if ( zi->fileStream->Write( ldi->data, ldi->filledBlock ) != ( int )ldi->filledBlock ) {
					err = ZIP_ERRNO;
				}
			}
            size_centraldir += ldi->filledBlock;
            ldi = ldi->next;
        }
    }
    free_datablock( zi->centralDir.first_block );

	if ( err == ZIP_OK ) { /* Magic End */
        err = ziplocal_putValue( zi->fileStream, (unsigned long)ENDHEADERMAGIC, 4 );
	}

	if ( err == ZIP_OK ) { /* number of this disk */
		err = ziplocal_putValue( zi->fileStream, (unsigned long)0, 2 );
	}

	if ( err == ZIP_OK ) { /* number of the disk with the start of the central directory */
        err = ziplocal_putValue( zi->fileStream, (unsigned long)0, 2 );
	}

	if ( err == ZIP_OK ) { /* total number of entries in the central dir on this disk */
        err = ziplocal_putValue( zi->fileStream, (unsigned long)zi->entryNumber, 2 );
	}

	if ( err == ZIP_OK ) { /* total number of entries in the central dir */
        err = ziplocal_putValue( zi->fileStream, (unsigned long)zi->entryNumber, 2 );
	}

	if ( err == ZIP_OK ) { /* size of the central directory */
        err = ziplocal_putValue( zi->fileStream, (unsigned long)size_centraldir, 4 );
	}

	if ( err == ZIP_OK ) { /* offset of start of central directory with respect to the starting disk number */
        err = ziplocal_putValue( zi->fileStream, (unsigned long)( centraldir_pos_inzip - zi->WriteOffsetPos ), 4 );
	}

	if ( err == ZIP_OK ) { /* zipfile comment length */
        err = ziplocal_putValue( zi->fileStream, (unsigned long)size_global_comment, 2 );
	}

	if ( ( err == ZIP_OK ) && ( size_global_comment > 0 ) ) {
		if ( zi->fileStream->Write( global_comment, size_global_comment ) != ( int )size_global_comment ) {
			err = ZIP_ERRNO;
		}
	}

	delete zi->fileStream;
	zi->fileStream = nullptr;

#ifndef NO_ADDFILEINEXISTINGZIP
    TRYFREE( zi->globalComments );
#endif
    TRYFREE( zi );

    return err;
}

/*
========================
anPaKAssembly::AddFileFilters
========================
*/
void anPaKAssembly::AddFileFilters( const char *filters ) {
#if 0
	anStringList exts;
	anStringListBreakupString( exts, filters, "|" );
	if ( ( exts.Num() > 0 ) && ( exts[ exts.Num() - 1 ] == "" ) ) {
		exts.RemoveIndex( exts.Num() - 1 );
	}
	filterExts.Append( exts );
#endif
}

/*
========================
anPaKAssembly::AddUncompressedFileFilters
========================
*/
void anPaKAssembly::AddUncompressedFileFilters( const char *filters ) {
#if 0
	anStringList exts;
	anStringListBreakupString( exts, filters, "|" );
	if ( ( exts.Num() > 0 ) && ( exts[ exts.Num() - 1 ] == "" ) ) {
		exts.RemoveIndex( exts.Num() - 1 );
	}
	uncompressedFilterExts.Append( exts );
#endif
}

/*
========================
anPaKAssembly::Build

builds a zip file of all the files in the specified folder, overwriting if necessary
========================
*/
bool anPaKAssembly::Build( const char *zipPath, const char *folder, bool cleanFolder ) {
	zipFileName = zipPath;
	sourceFolderName = folder;

	if ( !CreateZipFile( false ) ) {
		// don't clean the folder if the zip fails
		return false;
	}

	if ( cleanFolder ) {
		CleanSourceFolder();
	}
	return true;
}

/*
========================
anPaKAssembly::Update

updates a zip file with the files in the specified folder
========================
*/
bool anPaKAssembly::Update( const char *zipPath, const char *folder, bool cleanFolder ) {
	// if this file doesn't exist, just build it
	if ( fileSystem->GetTimestamp( zipPath ) == FILE_NOT_FOUND_TIMESTAMP ) {
		return Build( zipPath, folder, cleanFolder );
	}
	zipFileName = zipPath;
	sourceFolderName = folder;

	if ( !CreateZipFile( true ) ) {
		// don't clean the folder if the zip fails
		return false;
	}

	if ( cleanFolder ) {
		CleanSourceFolder();
	}
	return true;
}

/*
========================
anPaKAssembly::GetFileTime
========================
*/
bool anPaKAssembly::GetFileTime( const anStr &filename, unsigned long *dostime ) const {
	{
		FILETIME filetime;
		WIN32_FIND_DATA fileData;
		HANDLE			findHandle = FindFirstFile( filename.c_str(), &fileData );
		if ( findHandle != INVALID_HANDLE_VALUE ) {
			FileTimeToLocalFileTime( &(fileData.ftLastWriteTime), &filetime );
			FileTimeToDosDateTime( &filetime, ((LPWORD)dostime) + 1, ((LPWORD)dostime) + 0 );
			FindClose( findHandle );
			return true;
		}
		FindClose( findHandle );
	}
	return false;
}

/*
========================
anPaKAssembly::IsFiltered
========================
*/
bool anPaKAssembly::IsFiltered( const anStr &filename ) const {
	if ( filterExts.Num() == 0 && uncompressedFilterExts.Num() == 0 ) {
		return false;
	}
	for ( int j = 0; j < filterExts.Num(); j++ ) {
		anStr fileExt = anStr( "." + filterExts[j] );
		if ( filename.Right( fileExt.Length() ).Icmp( fileExt ) == 0 ) {
			return false;
		}
	}
	for ( int j = 0; j < uncompressedFilterExts.Num(); j++ ) {
		anStr fileExt = anStr( "." + uncompressedFilterExts[j] );
		if ( filename.Right( fileExt.Length() ).Icmp( fileExt ) == 0 ) {
			return false;
		}
	}
	return true;
}

/*
========================
anPaKAssembly::IsUncompressed
========================
*/
bool anPaKAssembly::IsUncompressed( const anStr &filename ) const {
	if ( uncompressedFilterExts.Num() == 0 ) {
		return false;
	}
	for ( int j = 0; j < uncompressedFilterExts.Num(); j++ ) {
		anStr fileExt = anStr( "." + uncompressedFilterExts[j] );
		if ( filename.Right( fileExt.Length() ).Icmp( fileExt ) == 0 ) {
			return true;
		}
	}
	return false;
}

/*
========================
anPaKAssembly::CreateZipFile
========================
*/
bool anPaKAssembly::CreateZipFile( bool appendFiles ) {
#if 0
//#ifdef ID_PC
	if ( zipFileName.IsEmpty() || sourceFolderName.IsEmpty() ) {
		anLibrary::Warning( "[%s] - invalid parameters!", __FUNCTION__ );
		return false;
	}

	// need to clear the filesystem's zip cache before we can open and write
	//fileSystem->ClearZipCache();

	anLibrary::Printf( "Building zip file: '%s'\n", zipFileName.c_str() );

	sourceFolderName.StripTrailing( "\\" );
	sourceFolderName.StripTrailing( "/" );

#if 0
	// attempt to check the file out
	if ( !Sys_IsFileWritable( zipFileName ) ) {
		if ( ( anLibrary::sourceControl == nullptr ) || !anLibrary::sourceControl->CheckOut( zipFileName ) ) {
			anLibrary::Warning( "READONLY zip file couldn't be checked out: %s", zipFileName.c_str() );
		} else {
			anLibrary::Printf( "Checked out: %s\n", zipFileName.c_str() );
		}
	}
#endif

	// if not appending, set the file size to zero to "create it from scratch"
	if ( !appendFiles ) {
		anLibrary::PrintfIf( zip_verbosity.GetBool(), "Overwriting zip file: '%s'\n", zipFileName.c_str() );
		anFile *zipFile = fileSystem->OpenExplicitFileWrite( zipFileName );
		if ( zipFile != nullptr ) {
			delete zipFile;
			zipFile = nullptr;
		}
	} else {
		anLibrary::PrintfIf( zip_verbosity.GetBool(), "Appending to zip file: '%s'\n", zipFileName.c_str() );
	}

	// enumerate the files to zip up in the source folder
	anStaticString< MAX_OSPATH > relPath;
	relPath =
	fileSystem->OSPathToRelativePath( sourceFolderName );
	anFileList *files = fileSystem->ListFilesTree( relPath, "*.*" );

	// check to make sure that at least one file will be added to the package
	int atLeastOneFilteredFile = false;
	for ( int i = 0; i < files->GetNumFiles(); i++ ) {
		anStr filename = files->GetFile( i );

		if ( !IsFiltered( filename ) ) {
			atLeastOneFilteredFile = true;
			break;
		}
	}
	if ( !atLeastOneFilteredFile ) {
		// although we didn't actually update/create a zip file, it's because no files would be added anyway, which would result in a corrupted zip
		anLibrary::Printf( "Skipping zip creation/modification, no additional changes need to be made...\n" );
		return true;
	}

	// open the zip file
	zipFile zf = zipOpen( zipFileName, appendFiles ? APPEND_STATUS_ADDINZIP : 0 );
	if ( zf == nullptr ) {
		anLibrary::Warning( "[%s] - error opening file '%s'!", __FUNCTION__, zipFileName.c_str() );
		return false;
	}

	// add the files to the zip file
	for ( int i = 0; i < files->GetNumFiles(); i++ ) {

		// add each file to the zip file
		zip_fileinfo zi;
		memset( &zi, 0, sizeof( zip_fileinfo ) );

		anStr filename = files->GetFile( i );

		if ( IsFiltered( filename ) ) {
			anLibrary::PrintfIf( zip_verbosity.GetBool(), "...Skipping: '%s'\n", filename.c_str() );
			continue;
		}

		anStr filenameInZip = filename;
		filenameInZip.Strip( relPath );
		filenameInZip.StripLeading( "/" );

		anStaticString< MAX_OSPATH > ospath;
		ospath = fileSystem->RelativePathToOSPath( filename );
		GetFileTime( ospath, &zi.dosTimeFmt );

		anLibrary::PrintfIf( zip_verbosity.GetBool(), "...Adding: '%s' ", filenameInZip.c_str() );

		int compressionMethod = Z_DEFLATED;
		if ( IsUncompressed( filenameInZip ) ) {
			compressionMethod = 0;
		}

		int errcode = PaK_OpenNewFileInZip3( zf, filenameInZip, &zi, nullptr, 0, nullptr, 0, nullptr /* comment*/,
											compressionMethod,	DEFAULT_COMPRESSION_LEVEL, 0, -MAX_WBITS, DEF_MEM_LEVEL,
											Z_DEFAULT_STRATEGY, nullptr /*password*/, 0 /*fileCRC*/ );

		if ( errcode != ZIP_OK ) {
			anLibrary::Warning( "Error opening file in zipfile!" );
			continue;
		} else {
			// open the source file
			anFilePermanent src( filename, ospath, FS_READ );
			if ( !src.IsOpen() ) {
				anLibrary::Warning( "Error opening source file!" );
				continue;
			}

			// copy the file data into the zip file
			arcTempArray<byte> buffer( DEFAULT_WRITEBUFFERSIZE );
			size_t total = 0;
			while ( size_t bytesRead = src.Read( buffer.Ptr(), buffer.Size() ) ) {
				if ( bytesRead > 0 ) {
					errcode = PaK_WriteInFileInZip( zf, buffer.Ptr(), (unsigned int)bytesRead );
					if ( errcode != ZIP_OK ) {
						anLibrary::Warning( "Error writing to zipfile (%i bytes)!", bytesRead );
						continue;
					}
				}
				total += bytesRead;
			}
			assert( total == ( size_t)src.Length() );
		}

		errcode = PaK_CloseFileInZip( zf );
		if ( errcode != ZIP_OK ) {
			anLibrary::Warning( "Error zipping source file!" );
			continue;
		}
		anLibrary::PrintfIf( zip_verbosity.GetBool(), "\n" );
	}

	// free the file list
	if ( files != nullptr ) {
		fileSystem->FreeFileList( files );
	}

	// close the zip file
	int closeError = PaK_ClosePaK( zf, nullptr );
	if ( closeError != ZIP_OK ) {
		anLibrary::Warning( "[%s] - error closing file '%s'!", __FUNCTION__, zipFileName.c_str() );
		return false;
	}

	anLibrary::Printf( "Done.\n" );

	return true;
#else

	return false;
#endif

}

/*
========================
anPaKAssembly::CreateZipFileFromFileList
========================
*/
bool anPaKAssembly::CreateZipFileFromFileList( const char *name, const anList<anFileMemory *> & srcFiles ) {
	zipFileName = name;
	return CreateZipFileFromFiles( srcFiles );
}
/*
========================
anPaKAssembly::CreateZipFileFromFiles
========================
*/
bool anPaKAssembly::CreateZipFileFromFiles( const anList<anFileMemory *> & srcFiles ) {
	if ( zipFileName.IsEmpty() ) {
		anLibrary::Warning( "[%s] - invalid parameters!", __FUNCTION__ );
		return false;
	}

	// need to clear the filesystem's zip cache before we can open and write
	//fileSystem->ClearZipCache();

	anLibrary::Printf( "Building zip file: '%s'\n", zipFileName.c_str() );

	// do not allow overwrite as this should be a tempfile attempt to check the file out
	if ( !Sys_IsFileWritable( zipFileName ) ) {
		anLibrary::PrintfIf( zip_verbosity.GetBool(), "File %s not writeable, cannot proceed.\n", zipFileName.c_str() );
		return false;
	}

	// open the zip file
	zipFile zf = zipOpen( zipFileName, 0 );
	if ( zf == nullptr ) {
		anLibrary::Warning( "[%s] - error opening file '%s'!", __FUNCTION__, zipFileName.c_str() );
		return false;
	}

	// add the files to the zip file
	for ( int i = 0; i < srcFiles.Num(); i++ ) {

		// add each file to the zip file
		zip_fileinfo zi;
		memset( &zi, 0, sizeof( zip_fileinfo ) );

		anFileMemory * src = srcFiles[i];
		src->MakeReadOnly();

		anLibrary::PrintfIf( zip_verbosity.GetBool(), "...Adding: '%s' ", src->GetName() );

		int compressionMethod = Z_DEFLATED;
		if ( IsUncompressed( src->GetName() ) ) {
			compressionMethod = 0;
		}

		int errcode = PaK_OpenNewFileInZip3( zf, src->GetName(), &zi, nullptr, 0, nullptr, 0, nullptr /* comment*/,
			compressionMethod,	DEFAULT_COMPRESSION_LEVEL, 0, -MAX_WBITS, DEF_MEM_LEVEL,
			Z_DEFAULT_STRATEGY, nullptr /*password*/, 0 /*fileCRC*/ );

		if ( errcode != ZIP_OK ) {
			anLibrary::Warning( "Error opening file in zipfile!" );
			continue;
		} else {
			// copy the file data into the zip file
			arcTempArray<byte> buffer( DEFAULT_WRITEBUFFERSIZE );
			size_t total = 0;
			while ( size_t bytesRead = src->Read( buffer.Ptr(), buffer.Size() ) ) {
				if ( bytesRead > 0 ) {
					errcode = PaK_WriteInFileInZip( zf, buffer.Ptr(), (unsigned int)bytesRead );
					if ( errcode != ZIP_OK ) {
						anLibrary::Warning( "Error writing to zipfile (%i bytes)!", bytesRead );
						continue;
					}
				}
				total += bytesRead;
			}
			assert( total == ( size_t)src->Length() );
		}

		errcode = PaK_CloseFileInZip( zf );
		if ( errcode != ZIP_OK ) {
			anLibrary::Warning( "Error zipping source file!" );
			continue;
		}
		anLibrary::PrintfIf( zip_verbosity.GetBool(), "\n" );
	}

	// close the zip file
	int closeError = PaK_ClosePaK( zf, zipFileName );
	if ( closeError != ZIP_OK ) {
		anLibrary::Warning( "[%s] - error closing file '%s'!", __FUNCTION__, zipFileName.c_str() );
		return false;
	}

	anLibrary::PrintfIf( zip_verbosity.GetBool(), "Done.\n" );

	return true;
}

/*
========================
anPaKAssembly::CleanSourceFolder

this folder is assumed to be a path under FSPATH_BASE
========================
*/
zipFile anPaKAssembly::CreateZipFile( const char *name ) {
	anLibrary::Printf( "Creating zip file: '%s'\n", name );

	// do not allow overwrite as this should be a tempfile attempt to check the file out
	if ( !Sys_IsFileWritable( name ) ) {
		anLibrary::PrintfIf( zip_verbosity.GetBool(), "File %s not writeable, cannot proceed.\n", name );
		return nullptr;
	}

	// open the zip file
	zipFile zf = zipOpen( name, 0 );
	if ( zf == nullptr ) {
		anLibrary::Warning( "[%s] - error opening file '%s'!", __FUNCTION__, name );
	}
	return zf;
}

/*
========================
anPaKAssembly::CleanSourceFolder

this folder is assumed to be a path under FSPATH_BASE
========================
*/
bool anPaKAssembly::AddFile( zipFile zf, anFileMemory *src, bool deleteFile ) {
	// add each file to the zip file
	zip_fileinfo zi;
	memset( &zi, 0, sizeof( zip_fileinfo ) );


	src->MakeReadOnly();

	anLibrary::PrintfIf( zip_verbosity.GetBool(), "...Adding: '%s' ", src->GetName() );

	int compressionMethod = Z_DEFLATED;
	if ( IsUncompressed( src->GetName() ) ) {
		compressionMethod = Z_NO_COMPRESSION;
	}

	int errcode = PaK_OpenNewFileInZip3( zf, src->GetName(), &zi, nullptr, 0, nullptr, 0, nullptr /* comment*/,
		compressionMethod,	DEFAULT_COMPRESSION_LEVEL, 0, -MAX_WBITS, DEF_MEM_LEVEL,
		Z_DEFAULT_STRATEGY, nullptr /*password*/, 0 /*fileCRC*/ );

	if ( errcode != ZIP_OK ) {
		anLibrary::Warning( "Error opening file in zipfile!" );
		if ( deleteFile ) {
			src->Clear( true );
			delete src;
		}
		return false;
	} else {
		// copy the file data into the zip file
		arcTempArray<byte> buffer( DEFAULT_WRITEBUFFERSIZE );
		size_t total = 0;
		while ( size_t bytesRead = src->Read( buffer.Ptr(), buffer.Size() ) ) {
			if ( bytesRead > 0 ) {
				errcode = PaK_WriteInFileInZip( zf, buffer.Ptr(), (unsigned int)bytesRead );
				if ( errcode != ZIP_OK ) {
					anLibrary::Warning( "Error writing to zipfile (%i bytes)!", bytesRead );
					continue;
				}
			}
			total += bytesRead;
		}
		assert( total == ( size_t)src->Length() );
	}

	errcode = PaK_CloseFileInZip( zf );
	if ( errcode != ZIP_OK ) {
		anLibrary::Warning( "Error zipping source file!" );
		if ( deleteFile ) {
			src->Clear( true );
			delete src;
		}
		return false;
	}
	anLibrary::PrintfIf( zip_verbosity.GetBool(), "\n" );
	if ( deleteFile ) {
		src->Clear( true );
		delete src;
	}
	return true;
}

/*
========================
anPaKAssembly::CleanSourceFolder

this folder is assumed to be a path under FSPATH_BASE
========================
*/
void anPaKAssembly::CloseZipFile( zipFile zf ) {
	// close the zip file
	int closeError = PaK_ClosePaK( zf, zipFileName );
	if ( closeError != ZIP_OK ) {
		anLibrary::Warning( "[%s] - error closing file '%s'!", __FUNCTION__, zipFileName.c_str() );
	}
	anLibrary::PrintfIf( zip_verbosity.GetBool(), "Done.\n" );
}
/*
========================
anPaKAssembly::CleanSourceFolder

this folder is assumed to be a path under FSPATH_BASE
========================
*/
void anPaKAssembly::CleanSourceFolder() {
#if 0
//#ifdef ID_PC_WIN
	anStringList deletedFiles;

	// make sure this is a valid path, we don't want to go nuking
	// some user path  or something else unintentionally
	anStr ospath = sourceFolderName;
	ospath.SlashesToBackSlashes();
	ospath.ToLower();

	char relPath[MAX_OSPATH];
	fileSystem->OSPathToRelativePath( ospath, relPath, MAX_OSPATH );

	// get the game's base path
	anStr basePath = fileSystem->GetBasePathStr( FSPATH_BASE );
	basePath.AppendPath( BASE_GAMEDIR );
	basePath.AppendPath( "maps" );
	basePath.SlashesToBackSlashes();
	basePath.ToLower();
	// path must be off of our base path, ospath can't have .map on the end, and
	// do some additional sanity checks
	if ( ( ospath.Find( basePath ) == 0 ) && ( ospath.Right( 4 ) != ".map" ) &&
		( ospath != "c:\\" ) && ( ospath.Length() > basePath.Length() ) ) {
			// get the files in the current directory
			anFileList *files = fileSystem->ListFilesTree( relPath, "*.*" );
			if ( files->GetNumFiles() && zip_verbosity.GetBool() ) {
				anLibrary::Printf( "Deleting files in '%s'...\n", relPath );
			}
			for ( int i = 0; i < files->GetNumFiles(); i++ ) {
				if ( IsFiltered( files->GetFile( i ) ) ) {
					continue;
				}
				// nuke 'em
				if ( zip_verbosity.GetBool() ) {
					anLibrary::Printf( "\t...%s\n", files->GetFile( i ) );
				}
				fileSystem->RemoveFile( files->GetFile( i ) );

				char ospath2[MAX_OSPATH];
				fileSystem->RelativePathToOSPath( files->GetFile( i ), ospath2, MAX_OSPATH );
				deletedFiles.Append( ospath2 );
			}
			fileSystem->FreeFileList( files );
			fileSystem->RemoveDir( relPath );
	} else {
		anLibrary::Printf( "Warning: anPaKAssembly::CleanSourceFolder - Non-standard path: '%s'!\n", ospath.c_str() );
		return;
	}

	// figure out which deleted files need to be removed from source control, and then remove those files
	anStringList filesToRemoveFromSourceControl;
	for ( int i = 0; i < deletedFiles.Num(); i++ ) {
		scFileStatus_t fileStatus = anLibrary::sourceControl->GetFileStatus( deletedFiles[i] );
		if ( SCF_IS_IN_SOURCE_CONTROL( fileStatus ) ) {
			filesToRemoveFromSourceControl.Append( deletedFiles[i] );
		}
	}
	if ( filesToRemoveFromSourceControl.Num() > 0 ) {
		anLibrary::sourceControl->Delete( filesToRemoveFromSourceControl );
	}

#endif
}

/*
========================
anPaKAssembly::BuildMapFolderZip
========================
*/
const char *ZIP_FILE_EXTENSION = "pk4";
bool anPaKAssembly::BuildMapFolderZip( const char *mapFileName ) {
	anStr zipFileName = mapFileName;
	zipFileName.SetFileExtension( ZIP_FILE_EXTENSION );
	anStr pathToZip = mapFileName;
	pathToZip.StripFileExtension();
	anPaKAssembly zip;
	zip.AddFileFilters( "bcm|bmodel|proc|" );
	zip.AddUncompressedFileFilters( "genmodel|sbcm|tbcm|" );
	bool success = zip.Build( zipFileName, pathToZip, true );
	// even if the zip build failed we want to clear the source folder so no contributing files are left around
	if ( !success ) {
		zip.CleanSourceFolder();
	}
	return success;
}

/*
========================
anPaKAssembly::UpdateMapFolderZip
========================
*/
bool anPaKAssembly::UpdateMapFolderZip( const char *mapFileName ) {
	anStr zipFileName = mapFileName;
	zipFileName.SetFileExtension( ZIP_FILE_EXTENSION );
	anStr pathToZip = mapFileName;
	pathToZip.StripFileExtension();
	anPaKAssembly zip;
	zip.AddFileFilters( "bcm|bmodel|proc|" );
	zip.AddUncompressedFileFilters( "genmodel|sbcm|tbcm|" );
	bool success = zip.Update( zipFileName, pathToZip, true );
	// even if the zip build failed we want to clear the source folder so no contributing files are left around
	if ( !success ) {
		zip.CleanSourceFolder();
	}
	return success;
}

/*
========================
anPaKAssembly::CombineFiles
========================
*/
anFileMemory * anPaKAssembly::CombineFiles( const anList<anFileMemory *> & srcFiles ) {
	anFileMemory * destFile = nullptr;

#if 0
//#ifdef ID_PC

	// create a new temp file so we can zip into it without refactoring the zip routines
	char ospath[MAX_OSPATH];
	const char *tempName = "temp.tmp";
	fileSystem->RelativePathToOSPath( tempName, ospath, MAX_OSPATH, FSPATH_SAVE );
	fileSystem->RemoveFile( ospath );

	// combine src files into dest filename just specified
	anPaKAssembly zip;
	zip.zipFileName = ospath;
	bool ret = zip.CreateZipFileFromFiles( srcFiles );

	// read the temp file created into a memory file to return
	if ( ret ) {
		destFile = new anFileMemory();

		if ( !destFile->Load( tempName, ospath ) ) {
			assert( false && "couldn't read the combined file" );
			delete destFile;
			destFile = nullptr;
		}

		// delete the temp file
		fileSystem->RemoveFile( ospath );

		// make the new file readable
		destFile->MakeReadOnly();
	}

#endif

	return destFile;
}

CONSOLE_COMMAND( testZipBuilderCombineFiles, "test routine for memory zip file building", 0 ) {
#if 0
	anList<anFileMemory *> list;
	const char *	testString = "test";
	int				numFiles = 2;

	if ( args.Argc() > 2 ) {
		anLibrary::Printf( "usage: testZipBuilderExtractFiles [numFiles]\n" );
		return;
	}

	for ( int arg = 1; arg < args.Argc(); arg++ ) {
		numFiles = atoi( args.Argv( arg ) );
	}

	// allocate all the test files
	for ( int i = 0; i < numFiles; i++ ) {
		anFileMemory * file = new anFileMemory( va( "%s%d.txt", testString, i + 1 ) );
		file->MakeWritable();
		anStr str = va( "%s%d", testString, i + 1 );
		file->WriteString( str );
		list.Append( file );
	}

	// combine the files into a single memory file
	anPaKAssembly zip;
	anFileMemory * file = zip.CombineFiles( list );
	if ( file != nullptr ) {
		file->MakeReadOnly();

		char ospath[MAX_OSPATH];
		const char *tempName = "temp.zip";
		fileSystem->RelativePathToOSPath( tempName, ospath, MAX_OSPATH, FSPATH_SAVE );

		// remove previous file if it exists
		fileSystem->RemoveFile( ospath );

		if ( file->Save( tempName, ospath ) ) {
			anLibrary::PrintfIf( zip_verbosity.GetBool(), va( "File written: %s.\n", ospath ) );
		} else {
			anLibrary::Error( "Could not save the file." );
		}

		delete file;
	}

	list.DeleteContents();
#endif
	// Now look at the temp.zip, unzip it to see if it works
}

/*
========================
anPaKAssembly::ExtractFiles
========================
*/
bool anPaKAssembly::ExtractFiles( anFileMemory * & srcFile, anList<anFileMemory *> & destFiles ) {
	bool ret = false;

#if 0
//#ifdef ID_PC

	destFiles.Clear();

	// write the memory file to temp storage so we can unzip it without refactoring the unzip routines
	char ospath[MAX_OSPATH];
	const char *tempName = "temp.tmp";
	fileSystem->RelativePathToOSPath( tempName, ospath, MAX_OSPATH, FSPATH_SAVE );
	ret = srcFile->Save( tempName, ospath );
	assert( ret && "couldn't create temp file" );

	if ( ret ) {

		anLibrary::PrintfIf( zip_verbosity.GetBool(), "Opening archive %s:\n", ospath );
		anPaKFile zip = PAK_Open( ospath );

		int numFiles = 0;
		int result = PAK_GoToFirstFile( zip );
		while( result == UNZ_OK ) {
			numFiles++;
			pakFileInfo curFileInfo;
			char fileName[MAX_OSPATH];
			PAK_GetFileDescription( zip, &curFileInfo, fileName, MAX_OSPATH, nullptr, 0, nullptr, 0 );

			anLibrary::PrintfIf( zip_verbosity.GetBool(), "%d: %s, size: %d \\ %d\n", numFiles, fileName, curFileInfo.compressedSize, curFileInfo.uncompressedSize );

			// create a buffer big enough to hold the entire uncompressed file
			void * buff = Mem_Alloc( curFileInfo.uncompressedSize, TAG_TEMP );
			result = PAK_OpenCurrentFile( zip );
			if ( result == UNZ_OK ) {
				result = PAK_ReadCurrentFile( zip, buff, curFileInfo.uncompressedSize );
				PAK_CloseCurrentFile( zip );
			}

			// create the new memory file
			anFileMemory * outFile = new anFileMemory( fileName );
			outFile->SetReadOnlyData( (const char *)buff, curFileInfo.uncompressedSize );

			destFiles.Append( outFile );

			result = PAK_NextFile( zip );
		}

		// close it so we can delete the zip file and create a new one
		PAK_Close( zip );

		// delete the temp zipfile
		fileSystem->RemoveFile( ospath );
	}

#endif

	return ret;
}

CONSOLE_COMMAND( testZipBuilderExtractFiles, "test routine for memory zip file extraction", 0 ) {
#if 0
	anList<anFileMemory *> list;
	anFileMemory * zipfile;
	const char *	testString = "test";
	int				numFiles = 2;
	bool			overallSuccess = true;
	bool			success = true;

	if ( args.Argc() > 2 ) {
		anLibrary::Printf( "usage: testZipBuilderExtractFiles [numFiles]\n" );
		return;
	}

	for ( int arg = 1; arg < args.Argc(); arg++ ) {
		numFiles = atoi( args.Argv( arg ) );
	}

	// create a temp.zip file with string files
	{
		// allocate all the test files
		for ( int i = 0; i < numFiles; i++ ) {
			anFileMemory * file = new anFileMemory( va( "%s%d.txt", testString, i + 1 ) );
			file->MakeWritable();
			anStr str = va( "%s%d", testString, i + 1 );
			file->WriteString( str );
			list.Append( file );
		}

		// combine the files into a single memory file
		anPaKAssembly zip;
		zipfile = zip.CombineFiles( list );

		success = ( zipfile != nullptr );
		overallSuccess &= success;
		anLibrary::Printf( "Zip file created: %s\n", success ? "^2PASS" : "^1FAIL" );

		// destroy all the test files
		list.DeleteContents();
	}

	// unzip the file into separate memory files
	if ( overallSuccess ) {

		// extract all the test files using the single zip file from above
		anPaKAssembly zip;
		if ( !zip.ExtractFiles( zipfile, list ) ) {
			anLibrary::Error( "Could not extract files." );
		}

		success = ( list.Num() == numFiles );
		overallSuccess &= success;
		anLibrary::Printf( "Number of files: %s\n", success ? "^2PASS" : "^1FAIL" );

		for ( int i = 0; i < list.Num(); i++ ) {
			anStr str;
			anFileMemory * file = list[i];
			file->MakeReadOnly();
			file->ReadString( str );

			anStr filename = va( "%s%d.txt", testString, i + 1 );
			anStr contents = va( "%s%d", testString, i + 1 );

			// test the filename
			bool nameSuccess = ( file->GetName() == filename );
			overallSuccess &= nameSuccess;

			// test the string
			bool contentSuccess = ( str == contents );
			overallSuccess &= contentSuccess;

			anLibrary::Printf( "Extraction of file, %s: %s^0, contents check: %s\n", filename.c_str(), nameSuccess ? "^2PASS" : "^1FAIL", contentSuccess ? "^2PASS" : "^1FAIL" );
		}

		list.DeleteContents();
	}

	if ( zipfile != nullptr ) {
		delete zipfile;
	}

	anLibrary::Printf( "[%s] overall tests: %s\n", __FUNCTION__, overallSuccess ? "^2PASS" : "^1FAIL" );
#endif
}
