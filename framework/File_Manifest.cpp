#include "/idlib/Lib.h"
#pragma hdrstop


/*
================================================================================================

anPreloadManifest

================================================================================================
*/

/*
========================
anPreloadManifest::LoadManifest
========================
*/
bool anPreloadManifest::LoadManifest( const char *fileName ) {
	anFile * inFile = fileSystem->OpenFileReadMemory( fileName );
	if ( inFile != nullptr ) {
		int numEntries;
		inFile->ReadBig( numEntries );
		inFile->ReadString( filename );
		entries.SetNum( numEntries );
		for ( int i = 0; i < numEntries; i++ ) {
			entries[i].Read( inFile );
		}
delete inFile;
		return true;
	}
	return false;
}

/*
================================================================================================

anManifest

================================================================================================
*/
/*
========================
anManifest::LoadManifest
========================
*/
bool anManifest::LoadManifest( const char *_fileName ) {
	anFile *file = fileSystem->OpenFileRead( _fileName , false );
	if ( file != nullptr ) {
		return LoadManifestFromFile( file );
	}
	return false;
}

/*
========================
anManifest::LoadManifestFromFile

// this will delete the file when finished
========================
*/
bool anManifest::LoadManifestFromFile( anFile *file ) {
	if ( file == nullptr ) {
		return false;
	}
	filename = file->GetName();
	anString str;
	int num;
	file->ReadBig( num );
	cacheTable.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		file->ReadString( cacheTable[i] );
		//if ( FindFile( cacheTable[i].filename ) == nullptr ) {
			// we only care about the first usage
			const int key = cacheHash.GenerateKey( cacheTable[i], false );
			cacheHash.Add( key, i );
		//}
	}
	delete file;
	return true;
}

/*
========================
anManifest::WriteManifestFile
========================
*/
void anManifest::WriteManifestFile( const char *fileName ) {
	anFile *file = fileSystem->OpenFileWrite( fileName );
	if ( file == nullptr ) {
		return;
	}
	anString str;
	int num = cacheTable.Num();
	file->WriteBig( num );
	for ( int i = 0; i < num; i++ ) {
		file->WriteString( cacheTable[i] );
	}
	delete file;
}

/*
========================
anPreloadManifest::WriteManifestFile
========================
*/
void anPreloadManifest::WriteManifest( const char *fileName ) {
	anFile *file = fileSystem->OpenFileWrite( fileName, "fs_savepath" );
	if ( file != nullptr ) {
		WriteManifestToFile( file );
		delete file;
	}
}

/*
========================
anManifest::FindFile
========================
*/
int anManifest::FindFile( const char *fileName ) {
	const int key = cacheHash.GenerateKey( fileName, false );
	for ( int index = cacheHash.GetFirst( key ); index != anHashIndex::NULL_INDEX; index = cacheHash.GetNext( index ) ) {
		if ( anString::Icmp( cacheTable[index], fileName ) == 0 ) {
			return index;
		}
	}
	return -1;
}

/*
========================
anManifest::RemoveAll
========================
*/
void anManifest::RemoveAll( const char *_fileName ) {
	for ( int i = 0; i < cacheTable.Num(); i++ ) {
		if ( cacheTable[i].Icmp( _fileName ) == 0 ) {
			const int key =cacheHash.GenerateKey( cacheTable[i], false );
			cacheTable.RemoveIndex( i );
			cacheHash.RemoveIndex( key, i );
			i--;
		}
	}
}

/*
========================
anManifest::GetFileNameByIndex
========================
*/
const anString &anManifest::GetFileNameByIndex( int idx ) const {
	return cacheTable[ idx ];
}

/*
=========================
anManifest::AddFile
=========================
*/
void anManifest::AddFile( const char *fileName ) {
	//if ( FindFile( fileName ) == nullptr ) {
		// we only care about the first usage
		const int key = cacheHash.GenerateKey( fileName, false );
		int idx = cacheTable.Append( fileName );
		cacheHash.Add( key, idx );
	//}
}
