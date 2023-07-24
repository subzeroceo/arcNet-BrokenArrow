#include "/idlib/precompiled.h"
#pragma hdrstop

/*
================================================================================================

aRcResContainers

================================================================================================
*/

/*
========================
aRcResContainers::ReOpen
========================
*/
void aRcResContainers::ReOpen() {
	delete resourceFile;
	resourceFile = fileSystem->OpenFileRead( fileName );
}

/*
========================
aRcResContainers::Init
========================
*/
bool aRcResContainers::Init( const char *_fileName, uint8 containerIndex ) {
	if ( arcNetString::Icmp( _fileName, "_ordered.resources" ) == 0 ) {
		resourceFile = fileSystem->OpenFileReadMemory( _fileName );
	} else {
		resourceFile = fileSystem->OpenFileRead( _fileName );
	}

	if ( resourceFile == NULL ) {
		arcLibrary::Warning( "Unable to open resource file %s", _fileName );
		return false;
	}

	resourceFile->ReadBig( resourceMagic );
	if ( resourceMagic != RESOURCE_FILE_MAGIC ) {
		arcLibrary::FatalError( "resourceFileMagic != RESOURCE_FILE_MAGIC" );
	}

	fileName = _fileName;

	resourceFile->ReadBig( tableOffset );
	resourceFile->ReadBig( tableLength );
	// read this into a memory buffer with a single read
	char * const buf = (char *)Mem_Alloc( tableLength, TAG_RESOURCE );
	resourceFile->Seek( tableOffset, FS_SEEK_SET );
	resourceFile->Read( buf, tableLength );
	aRcFileMemory memFile( "resourceHeader", (const char *)buf, tableLength );

	// Parse the resourceFile header, which includes every resource used
	// by the game.
	memFile.ReadBig( numFileResources );

	cacheTable.SetNum( numFileResources );

	for ( int i = 0; i < numFileResources; i++ ) {
		aRcResCacheEntries &rt = cacheTable[ i ];
		rt.Read( &memFile );
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		rt.containerIndex = containerIndex;

		const int key = cacheHash.GenerateKey( rt.filename, false );
		bool found = false;
		//for ( int index = cacheHash.GetFirst( key ); index != ARCHashIndex::NULL_INDEX; index = cacheHash.GetNext( index ) ) {
		//	aRcResCacheEntries & rtc = cacheTable[index];
		//	if ( arcNetString::Icmp( rtc.filename, rt.filename ) == 0 ) {
		//		found = true;
		//		break;
		//	}
		//}
		if ( !found ) {
			//arcLibrary::Printf( "rez file name: %s\n", rt.filename.c_str() );
			cacheHash.Add( key, i );
		}
	}
	Mem_Free( buf );

	return true;
}

/*
========================
aRcResContainers::WriteManifestFile
========================
*/
void aRcResContainers::WriteManifestFile( const char *name, const arcStringList &list ) {
	arcNetString filename( name );
	filename.SetFileExtension( "manifest" );
	filename.Insert( "maps/", 0 );
	arcNetFile *outFile = fileSystem->OpenFileWrite( filename );
	if ( outFile != NULL ) {
		int num = list.Num();
		outFile->WriteBig( num );
		for ( int i = 0; i < num; i++ ) {
			outFile->WriteString( list[ i ] );
		}
		delete outFile;
	}
}

/*
========================
aRcResContainers::ReadManifestFile
========================
*/
int aRcResContainers::ReadManifestFile( const char *name, arcStringList &list ) {
	arcNetFile *inFile = fileSystem->OpenFileRead( name );
	if ( inFile != NULL ) {
		list.SetGranularity( 16384 );
		arcNetString str;
		int num;
		list.Clear();
		inFile->ReadBig( num );
		for ( int i = 0; i < num; i++ ) {
			inFile->ReadString( str );
			list.Append( str );
		}
		delete inFile;
	}
	return list.Num();
}

/*
========================
aRcResContainers::UpdateResourceFile
========================
*/
void aRcResContainers::UpdateResourceFile( const char *_filename, const arcStringList &_filesToUpdate ) {
	arcNetFile *outFile = fileSystem->OpenFileWrite( va( "%s.new", _filename ) );
	if ( outFile == NULL ) {
		arcLibrary::Warning( "Unable to open resource file %s or new output file", _filename );
		return;
	}

	uint32 magic = 0;
	int _tableOffset = 0;
	int _tableLength = 0;
	arcNetList< aRcResCacheEntries > entries;
	arcStringList filesToUpdate = _filesToUpdate;

	arcNetFile *inFile = fileSystem->OpenFileRead( _filename );
	if ( inFile == NULL ) {
		magic = RESOURCE_FILE_MAGIC;

		outFile->WriteBig( magic );
		outFile->WriteBig( _tableOffset );
		outFile->WriteBig( _tableLength );
	} else {
		inFile->ReadBig( magic );
		if ( magic != RESOURCE_FILE_MAGIC ) {
			delete inFile;
			return;
		}

		inFile->ReadBig( _tableOffset );
		inFile->ReadBig( _tableLength );
		// read this into a memory buffer with a single read
		char * const buf = (char *)Mem_Alloc( _tableLength, TAG_RESOURCE );
		inFile->Seek( _tableOffset, FS_SEEK_SET );
		inFile->Read( buf, _tableLength );
		aRcFileMemory memFile( "resourceHeader", (const char *)buf, _tableLength );

		int _numFileResources = 0;
		memFile.ReadBig( _numFileResources );

		outFile->WriteBig( magic );
		outFile->WriteBig( _tableOffset );
		outFile->WriteBig( _tableLength );

		entries.SetNum( _numFileResources );

		for ( int i = 0; i < _numFileResources; i++ ) {
			entries[ i ].Read( &memFile );
			arcLibrary::Printf( "examining %s\n", entries[ i ].filename.c_str() );
			byte * fileData = NULL;

			for ( int j = filesToUpdate.Num() - 1; j >= 0; j-- ) {
				if ( filesToUpdate[ j ].Icmp( entries[ i ].filename ) == 0 ) {
					arcNetFile *newFile = fileSystem->OpenFileReadMemory( filesToUpdate[ j ] );
					if ( newFile != NULL ) {
						arcLibrary::Printf( "Updating %s\n", filesToUpdate[ j ].c_str() );
						entries[ i ].length = newFile->Length();
						fileData = ( byte * )Mem_Alloc( entries[ i ].length, TAG_TEMP );
						newFile->Read( fileData, newFile->Length() );
						delete newFile;
					}
					filesToUpdate.RemoveIndex( j );
				}
			}

			if ( fileData == NULL ) {
				inFile->Seek( entries[ i ].offset, FS_SEEK_SET );
				fileData = ( byte * )Mem_Alloc( entries[ i ].length, TAG_TEMP );
				inFile->Read( fileData, entries[ i ].length );
			}

			entries[ i ].offset = outFile->Tell();
			outFile->Write( ( void* )fileData, entries[ i ].length );

			Mem_Free( fileData );
		}
		Mem_Free( buf );
	}

	while ( filesToUpdate.Num() > 0 ) {
		arcNetFile *newFile = fileSystem->OpenFileReadMemory( filesToUpdate[ 0 ] );
		if ( newFile != NULL ) {
			arcLibrary::Printf( "Appending %s\n", filesToUpdate[ 0 ].c_str() );
			aRcResCacheEntries rt;
			rt.filename = filesToUpdate[ 0 ];
			rt.length = newFile->Length();
			byte * fileData = ( byte * )Mem_Alloc( rt.length, TAG_TEMP );
			newFile->Read( fileData, rt.length );
			int idx = entries.Append( rt );
			if ( idx >= 0 ) {
				entries[ idx ].offset = outFile->Tell();
				outFile->Write( ( void* )fileData, entries[ idx ].length );
			}
			delete newFile;
			Mem_Free( fileData );
		}
		filesToUpdate.RemoveIndex( 0 );
	}

	_tableOffset = outFile->Tell();
	outFile->WriteBig( entries.Num() );

	// write the individual resource entries
	for ( int i = 0; i < entries.Num(); i++ ) {
		entries[ i ].Write( outFile );
	}

	// go back and write the header offsets again, now that we have file offsets and lengths
	_tableLength = outFile->Tell() - _tableOffset;
	outFile->Seek( 0, FS_SEEK_SET );
	outFile->WriteBig( magic );
	outFile->WriteBig( _tableOffset );
	outFile->WriteBig( _tableLength );

	delete outFile;
	delete inFile;
}

/*
========================
aRcResContainers::ExtractResourceFile
========================
*/
void aRcResContainers::SetContainerIndex( const int & _idx ) {
	for ( int i = 0; i < cacheTable.Num(); i++ ) {
		cacheTable[ i ].containerIndex = _idx;
	}
}

/*
========================
aRcResContainers::ExtractResourceFile
========================
*/
void aRcResContainers::ExtractResourceFile ( const char * _fileName, const char * _outPath, bool _copyWavs ) {
	arcNetFile *inFile = fileSystem->OpenFileRead( _fileName );

	if ( inFile == NULL ) {
		arcLibrary::Warning( "Unable to open resource file %s", _fileName );
		return;
	}

	uint32 magic;
	inFile->ReadBig( magic );
	if ( magic != RESOURCE_FILE_MAGIC ) {
		delete inFile;
		return;
	}

	int _tableOffset;
	int _tableLength;
	inFile->ReadBig( _tableOffset );
	inFile->ReadBig( _tableLength );
	// read this into a memory buffer with a single read
	char * const buf = (char *)Mem_Alloc( _tableLength, TAG_RESOURCE );
	inFile->Seek( _tableOffset, FS_SEEK_SET );
	inFile->Read( buf, _tableLength );
	aRcFileMemory memFile( "resourceHeader", (const char *)buf, _tableLength );

	int _numFileResources;
	memFile.ReadBig( _numFileResources );

	for ( int i = 0; i < _numFileResources; i++ ) {
		aRcResCacheEntries rt;
		rt.Read( &memFile );
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		byte *fbuf = NULL;
		if ( _copyWavs && ( rt.filename.Find( ".idwav" ) >= 0 ||  rt.filename.Find( ".idxma" ) >= 0 ||  rt.filename.Find( ".idmsf" ) >= 0 ) ) {
			rt.filename.SetFileExtension( "wav" );
			rt.filename.Replace( "generated/", "" );
			int len = fileSystem->GetFileLength( rt.filename );
			fbuf =  ( byte * )Mem_Alloc( len, TAG_RESOURCE );
			fileSystem->ReadFile( rt.filename, (void**)&fbuf, NULL );
		} else {
			inFile->Seek( rt.offset, FS_SEEK_SET );
			fbuf =  ( byte * )Mem_Alloc( rt.length, TAG_RESOURCE );
			inFile->Read( fbuf, rt.length );
		}
		arcNetString outName = _outPath;
		outName.AppendPath( rt.filename );
		arcNetFile *outFile = fileSystem->OpenExplicitFileWrite( outName );
		if ( outFile != NULL ) {
			outFile->Write( ( byte* )fbuf, rt.length );
			delete outFile;
		}
		Mem_Free( fbuf );
	}
	delete inFile;
	Mem_Free( buf );
}

/*
========================
aRcResContainers::Open
========================
*/
void aRcResContainers::WriteResourceFile( const char *manifestName, const arcStringList &manifest, const bool &_writeManifest ) {
	if ( manifest.Num() == 0 ) {
		return;
	}

	arcLibrary::Printf( "Writing resource file %s\n", manifestName );

	// build multiple output files at 1GB each
	arcNetList < arcStringList > outPutFiles;

	aRcManifest outManifest;
	int64 size = 0;
	arcStringList flist;
	flist.SetGranularity( 16384 );
	for ( int i = 0; i < manifest.Num(); i++ ) {
		flist.Append( manifest[ i ] );
		size += fileSystem->GetFileLength( manifest[ i ] );
		if ( size > 1024 * 1024 * 1024 ) {
			outPutFiles.Append( flist );
			size = 0;
			flist.Clear();
		}
		outManifest.AddFile( manifest[ i ] );
	}

	outPutFiles.Append( flist );

	if ( _writeManifest ) {
		arcNetString temp = manifestName;
		temp.Replace( "maps/", "manifests/" );
		temp.StripFileExtension();
		temp.SetFileExtension( "manifest" );
		outManifest.WriteManifestFile( temp );
	}

	for ( int idx = 0; idx < outPutFiles.Num(); idx++ ) {
		arcStringList &fileList = outPutFiles[ idx ];
		if ( fileList.Num() == 0 ) {
			continue;
		}

		arcNetString fileName = manifestName;
		if ( idx > 0 ) {
			fileName = va( "%s_%02d", manifestName, idx );
		}
		fileName.SetFileExtension( "resources" );

		arcNetFile *resFile = fileSystem->OpenFileWrite( fileName );

		if ( resFile == NULL ) {
			arcLibrary::Warning( "Cannot open %s for writing.\n", fileName.c_str() );
			return;
		}

		arcLibrary::Printf( "Writing resource file %s\n", fileName.c_str() );

		int	tableOffset = 0;
		int	tableLength = 0;
		int	tableNewLength = 0;
		uint32	resourceFileMagic = RESOURCE_FILE_MAGIC;

		resFile->WriteBig( resourceFileMagic );
		resFile->WriteBig( tableOffset );
		resFile->WriteBig( tableLength );

		arcNetList< aRcResCacheEntries > entries;

		entries.Resize( fileList.Num() );

		for ( int i = 0; i < fileList.Num(); i++ ) {
			aRcResCacheEntries ent;

			ent.filename = fileList[ i ];
			ent.length = 0;
			ent.offset = 0;

			arcNetFile *file = fileSystem->OpenFileReadMemory( ent.filename, false );
			aRcFileMemory *fm = dynamic_cast< aRcFileMemory* >( file );
			if ( fm == NULL ) {
				continue;
			}
			// if the entry is uncompressed, align the file pointer to a 16 byte boundary
			// so it will be usable if memory mapped
			ent.length = fm->Length();

			// always get the offset, even if the file will have zero length
			ent.offset = resFile->Tell();

			entries.Append( ent );

			if ( ent.length == 0 ) {
				ent.filename = "";
				delete fm;
				continue;
			}

			resFile->Write( fm->GetDataPtr(), ent.length );

			delete fm;

			// pacifier every ten megs
			if ( ( ent.offset + ent.length ) / 10000000 != ent.offset / 10000000 ) {
				arcLibrary::Printf( "." );
			}
		}

		arcLibrary::Printf( "\n" );

		// write the table out now that we have all the files
		tableOffset = resFile->Tell();

		// count how many we are going to write for this platform
		int	numFileResources = entries.Num();

		resFile->WriteBig( numFileResources );

		// write the individual resource entries
		for ( int i = 0; i < entries.Num(); i++ ) {
			entries[ i ].Write( resFile );
			if ( i + 1 == numFileResources ) {
				// we just wrote out the last new entry
				tableNewLength = resFile->Tell() - tableOffset;
			}
		}

		// go back and write the header offsets again, now that we have file offsets and lengths
		tableLength = resFile->Tell() - tableOffset;
		resFile->Seek( 0, FS_SEEK_SET );
		resFile->WriteBig( resourceFileMagic );
		resFile->WriteBig( tableOffset );
		resFile->WriteBig( tableLength );
		delete resFile;
	}
}
