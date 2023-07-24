#include "/idlib/precompiled.h"
#pragma hdrstop


/*
================================================================================================

aRcPreloadManifest

================================================================================================
*/

/*
========================
aRcPreloadManifest::LoadManifest
========================
*/
bool aRcPreloadManifest::LoadManifest( const char *fileName ) {
	arcNetFile * inFile = fileSystem->OpenFileReadMemory( fileName );
	if ( inFile != NULL ) {
		int numEntries;
		inFile->ReadBig( numEntries );
		inFile->ReadString( filename );
		entries.SetNum( numEntries );
		for ( int i = 0; i < numEntries; i++ ) {
			entries[ i ].Read( inFile );
		}
delete inFile;
		return true;
	}
	return false;
}

/*
================================================================================================

aRcManifest

================================================================================================
*/
/*
========================
aRcManifest::LoadManifest
========================
*/
bool aRcManifest::LoadManifest( const char *_fileName ) {
	arcNetFile *file = fileSystem->OpenFileRead( _fileName , false );
	if ( file != NULL ) {
		return LoadManifestFromFile( file );
	}
	return false;
}

/*
========================
aRcManifest::LoadManifestFromFile

// this will delete the file when finished
========================
*/
bool aRcManifest::LoadManifestFromFile( arcNetFile *file ) {
	if ( file == NULL ) {
		return false;
	}
	filename = file->GetName();
	arcNetString str;
	int num;
	file->ReadBig( num );
	cacheTable.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		file->ReadString( cacheTable[ i ] );
		//if ( FindFile( cacheTable[ i ].filename ) == NULL ) {
			// we only care about the first usage
			const int key = cacheHash.GenerateKey( cacheTable[ i ], false );
			cacheHash.Add( key, i );
		//}
	}
	delete file;
	return true;
}

/*
========================
aRcManifest::WriteManifestFile
========================
*/
void aRcManifest::WriteManifestFile( const char *fileName ) {
	arcNetFile *file = fileSystem->OpenFileWrite( fileName );
	if ( file == NULL ) {
		return;
	}
	arcNetString str;
	int num = cacheTable.Num();
	file->WriteBig( num );
	for ( int i = 0; i < num; i++ ) {
		file->WriteString( cacheTable[ i ] );
	}
	delete file;
}

/*
========================
aRcPreloadManifest::WriteManifestFile
========================
*/
void aRcPreloadManifest::WriteManifest( const char *fileName ) {
	arcNetFile *file = fileSystem->OpenFileWrite( fileName, "fs_savepath" );
	if ( file != NULL ) {
		WriteManifestToFile( file );
		delete file;
	}
}

/*
========================
aRcManifest::FindFile
========================
*/
int aRcManifest::FindFile( const char *fileName ) {
	const int key =cacheHash.GenerateKey( fileName, false );
	for ( int index = cacheHash.GetFirst( key ); index != ARCHashIndex::NULL_INDEX; index = cacheHash.GetNext( index ) ) {
		if ( arcNetString::Icmp( cacheTable[index], fileName ) == 0 ) {
			return index;
		}
	}
	return -1;
}

/*
========================
aRcManifest::RemoveAll
========================
*/
void aRcManifest::RemoveAll( const char * _fileName ) {
	for ( int i = 0; i < cacheTable.Num(); i++ ) {
		if ( cacheTable[ i ].Icmp( _fileName ) == 0 ) {
			const int key =cacheHash.GenerateKey( cacheTable[ i ], false );
			cacheTable.RemoveIndex( i );
			cacheHash.RemoveIndex( key, i );
			i--;
		}
	}
}

/*
========================
aRcManifest::GetFileNameByIndex
========================
*/
const arcNetString & aRcManifest::GetFileNameByIndex( int idx ) const {
	return cacheTable[ idx ];
}

/*
=========================
aRcManifest::AddFile
=========================
*/
void aRcManifest::AddFile( const char *fileName ) {
	//if ( FindFile( fileName ) == NULL ) {
		// we only care about the first usage
		const int key = cacheHash.GenerateKey( fileName, false );
		int idx = cacheTable.Append( fileName );
		cacheHash.Add( key, idx );
	//}
}



