#include "/idlib/Lib.h"
#pragma hdrstop

/*
================================================================================================

anResourceContainers

================================================================================================
*/

/*
========================
anResourceContainers::ReOpen
========================
*/
void anResourceContainers::ReOpen() {
	delete resFile;
	resFile = fileSystem->OpenFileRead( fileName );
}

/*
========================
anResourceContainers::Init
========================
*/
bool anResourceContainers::Init( const char *fName, uint8 containerIndex ) {
	if ( anStr::Icmp( fName, "_ordered.resources" ) == 0 ) {
		resFile = fileSystem->OpenFileReadMemory( fName );
	} else {
		resFile = fileSystem->OpenFileRead( fName );
	}

	if ( resFile == nullptr ) {
		anLibrary::Warning( "Unable to open resource file %s", fName );
		return false;
	}

	resFile->ReadBig( resourceMagic );
	if ( resourceMagic != RESOURCE_FILE_MAGIC ) {
		anLibrary::FatalError( "resourceFileMagic != RESOURCE_FILE_MAGIC" );
	}

	fileName = fName;

	resFile->ReadBig( tableOffset );
	resFile->ReadBig( tableLength );
	// read this into a memory buffer with a single read
	char * const buf = (char *)Mem_Alloc( tableLength, TAG_RESOURCE );
	resFile->Seek( tableOffset, FS_SEEK_SET );
	resFile->Read( buf, tableLength );
	anFileMemory memFile( "resourceHeader", (const char *)buf, tableLength );

	// Parse the resFile header, which includes every resource used
	// by the game.
	memFile.ReadBig( numFileResources );

	cacheTable.SetNum( numFileResources );

	for ( int i = 0; i < numFileResources; i++ ) {
		anResourceCacheEntries &rt = cacheTable[i];
		rt.Read( &memFile );
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		rt.containerIndex = containerIndex;

		const int key = cacheHash.GenerateKey( rt.filename, false );
		bool found = false;
		//for ( int index = cacheHash.GetFirst( key ); index != anHashIndex::nullptr_INDEX; index = cacheHash.GetNext( index ) ) {
		//	anResourceCacheEntries & rtc = cacheTable[index];
		//	if ( anStr::Icmp( rtc.filename, rt.filename ) == 0 ) {
		//		found = true;
		//		break;
		//	}
		//}
		if ( !found ) {
			//anLibrary::Printf( "rez file name: %s\n", rt.filename.c_str() );
			cacheHash.Add( key, i );
		}
	}
	Mem_Free( buf );

	return true;
}

/*
========================
anResourceContainers::WriteManifestFile
========================
*/
void anResourceContainers::WriteManifestFile( const char *name, const anStringList &list ) {
	anStr filename( name );
	filename.SetFileExtension( "manifest" );
	filename.Insert( "maps/", 0 );
	anFile *outFile = fileSystem->OpenFileWrite( filename );
	if ( outFile != nullptr ) {
		int num = list.Num();
		outFile->WriteBig( num );
		for ( int i = 0; i < num; i++ ) {
			outFile->WriteString( list[i] );
		}
		delete outFile;
	}
}

/*
========================
anResourceContainers::ReadManifestFile
========================
*/
int anResourceContainers::ReadManifestFile( const char *name, anStringList &list ) {
	anFile *inFile = fileSystem->OpenFileRead( name );
	if ( inFile != nullptr ) {
		list.SetGranularity( 16384 );
		anStr str;
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
anResourceContainers::UpdateResourceFile
========================
*/
void anResourceContainers::UpdateResourceFile( const char *fName, const anStringList &fUpdate ) {
	anFile *outFile = fileSystem->OpenFileWrite( va( "%s.new", fName ) );
	if ( outFile == nullptr ) {
		anLibrary::Warning( "Unable to open resource file %s or new output file", fName );
		return;
	}

	uint32 magic = 0;
	int tableOffset = 0;
	int tableLength = 0;
	anList< anResourceCacheEntries > entries;
	anStringList filesToUpdate =fUpdate;

	anFile *inFile = fileSystem->OpenFileRead( fName );
	if ( inFile == nullptr ) {
		magic = RESOURCE_FILE_MAGIC;

		outFile->WriteBig( magic );
		outFile->WriteBig( tableOffset );
		outFile->WriteBig( tableLength );
	} else {
		inFile->ReadBig( magic );
		if ( magic != RESOURCE_FILE_MAGIC ) {
			delete inFile;
			return;
		}

		inFile->ReadBig( tableOffset );
		inFile->ReadBig( tableLength );
		// read this into a memory buffer with a single read
		char * const buf = (char *)Mem_Alloc( tableLength, TAG_RESOURCE );
		inFile->Seek( tableOffset, FS_SEEK_SET );
		inFile->Read( buf, tableLength );
		anFileMemory memFile( "resourceHeader", (const char *)buf, tableLength );

		int numFileResources = 0;
		memFile.ReadBig( numFileResources );

		outFile->WriteBig( magic );
		outFile->WriteBig( tableOffset );
		outFile->WriteBig( tableLength );

		entries.SetNum( numFileResources );

		for ( int i = 0; i < numFileResources; i++ ) {
			entries[i].Read( &memFile );
			anLibrary::Printf( "examining %s\n", entries[i].filename.c_str() );
			byte *fileData = nullptr;

			for ( int j = filesToUpdate.Num() - 1; j >= 0; j-- ) {
				if ( filesToUpdate[ j ].Icmp( entries[i].filename ) == 0 ) {
					anFile *newFile = fileSystem->OpenFileReadMemory( filesToUpdate[ j ] );
					if ( newFile != nullptr ) {
						anLibrary::Printf( "Updating %s\n", filesToUpdate[ j ].c_str() );
						entries[i].length = newFile->Length();
						fileData = (byte *)Mem_Alloc( entries[i].length, TAG_TEMP );
						newFile->Read( fileData, newFile->Length() );
						delete newFile;
					}
					filesToUpdate.RemoveIndex( j );
				}
			}

			if ( fileData == nullptr ) {
				inFile->Seek( entries[i].offset, FS_SEEK_SET );
				fileData = (byte *)Mem_Alloc( entries[i].length, TAG_TEMP );
				inFile->Read( fileData, entries[i].length );
			}

			entries[i].offset = outFile->Tell();
			outFile->Write( ( void* )fileData, entries[i].length );

			Mem_Free( fileData );
		}
		Mem_Free( buf );
	}

	while ( filesToUpdate.Num() > 0 ) {
		anFile *newFile = fileSystem->OpenFileReadMemory( filesToUpdate[0] );
		if ( newFile != nullptr ) {
			anLibrary::Printf( "Appending %s\n", filesToUpdate[0].c_str() );
			anResourceCacheEntries rt;
			rt.filename = filesToUpdate[0];
			rt.length = newFile->Length();
			byte *fileData = (byte * )Mem_Alloc( rt.length, TAG_TEMP );
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

	tableOffset = outFile->Tell();
	outFile->WriteBig( entries.Num() );

	// write the individual resource entries
	for ( int i = 0; i < entries.Num(); i++ ) {
		entries[i].Write( outFile );
	}

	// go back and write the header offsets again, now that we have file offsets and lengths
	tableLength = outFile->Tell() - tableOffset;
	outFile->Seek( 0, FS_SEEK_SET );
	outFile->WriteBig( magic );
	outFile->WriteBig( tableOffset );
	outFile->WriteBig( tableLength );

	delete outFile;
	delete inFile;
}

/*
========================
anResourceContainers::ExtractResourceFile
========================
*/
void anResourceContainers::SetContainerIndex( const int & _idx ) {
	for ( int i = 0; i < cacheTable.Num(); i++ ) {
		cacheTable[i].containerIndex = _idx;
	}
}

/*
========================
anResourceContainers::ExtractResourceFile
========================
*/
void anResourceContainers::ExtractResourceFile( const char *fName, const char *_outPath, bool _copyWavs ) {
	anFile *inFile = fileSystem->OpenFileRead( fName );

	if ( inFile == nullptr ) {
		anLibrary::Warning( "Unable to open resource file %s", fName );
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
	anFileMemory memFile( "resourceHeader", (const char *)buf, _tableLength );

	int _numFileResources;
	memFile.ReadBig( _numFileResources );

	for ( int i = 0; i < _numFileResources; i++ ) {
		anResourceCacheEntries rt;
		rt.Read( &memFile );
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		byte *fbuf = nullptr;
		if ( _copyWavs && ( rt.filename.Find( ".idwav" ) >= 0 ||  rt.filename.Find( ".idxma" ) >= 0 ||  rt.filename.Find( ".idmsf" ) >= 0 ) ) {
			rt.filename.SetFileExtension( "wav" );
			rt.filename.Replace( "generated/", "" );
			int len = fileSystem->GetFileLength( rt.filename );
			fbuf =  (byte *)Mem_Alloc( len, TAG_RESOURCE );
			fileSystem->ReadFile( rt.filename, (void**)&fbuf, nullptr );
		} else {
			inFile->Seek( rt.offset, FS_SEEK_SET );
			fbuf =  (byte *)Mem_Alloc( rt.length, TAG_RESOURCE );
			inFile->Read( fbuf, rt.length );
		}
		anStr outName = _outPath;
		outName.AppendPath( rt.filename );
		anFile *outFile = fileSystem->OpenExplicitFileWrite( outName );
		if ( outFile != nullptr ) {
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
anResourceContainers::Open
========================
*/
void anResourceContainers::WriteResourceFile( const char *manifestName, const anStringList &manifest, const bool &_writeManifest ) {
	if ( manifest.Num() == 0 ) {
		return;
	}

	anLibrary::Printf( "Writing resource file %s\n", manifestName );

	// build multiple output files at 1GB each
	anList < anStringList > outPutFiles;

	anManifest outManifest;
	int64 size = 0;
	anStringList flist;
	flist.SetGranularity( 16384 );
	for ( int i = 0; i < manifest.Num(); i++ ) {
		flist.Append( manifest[i] );
		size += fileSystem->GetFileLength( manifest[i] );
		if ( size > 1024 * 1024 * 1024 ) {
			outPutFiles.Append( flist );
			size = 0;
			flist.Clear();
		}
		outManifest.AddFile( manifest[i] );
	}

	outPutFiles.Append( flist );

	if ( _writeManifest ) {
		anStr temp = manifestName;
		temp.Replace( "maps/", "rcData/" );
		temp.StripFileExtension();
		temp.SetFileExtension( "rcData" );
		outManifest.WriteManifestFile( temp );
	}

	for ( int idx = 0; idx < outPutFiles.Num(); idx++ ) {
		anStringList &fileList = outPutFiles[ idx ];
		if ( fileList.Num() == 0 ) {
			continue;
		}

		anStr fileName = manifestName;
		if ( idx > 0 ) {
			fileName = va( "%s_%02d", manifestName, idx );
		}
		fileName.SetFileExtension( "resources" );

		anFile *resFile = fileSystem->OpenFileWrite( fileName );

		if ( resFile == nullptr ) {
			anLibrary::Warning( "Cannot open %s for writing.\n", fileName.c_str() );
			return;
		}

		anLibrary::Printf( "Writing resource file %s\n", fileName.c_str() );

		int	tableOffset = 0;
		int	tableLength = 0;
		int	tableNewLength = 0;
		uint32	resourceFileMagic = RESOURCE_FILE_MAGIC;

		resFile->WriteBig( resourceFileMagic );
		resFile->WriteBig( tableOffset );
		resFile->WriteBig( tableLength );

		anList< anResourceCacheEntries > entries;

		entries.Resize( fileList.Num() );

		for ( int i = 0; i < fileList.Num(); i++ ) {
			anResourceCacheEntries ent;

			ent.filename = fileList[i];
			ent.length = 0;
			ent.offset = 0;

			anFile *file = fileSystem->OpenFileReadMemory( ent.filename, false );
			anFileMemory *fm = dynamic_cast<anFileMemory*>( file );
			if ( fm == nullptr ) {
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
				anLibrary::Printf( "." );
			}
		}

		anLibrary::Printf( "\n" );

		// write the table out now that we have all the files
		tableOffset = resFile->Tell();

		// count how many we are going to write for this platform
		int	numFileResources = entries.Num();

		resFile->WriteBig( numFileResources );

		// write the individual resource entries
		for ( int i = 0; i < entries.Num(); i++ ) {
			entries[i].Write( resFile );
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
