#include "/idlib/Lib.h"
#pragma hdrstop

anCVarSystem anDemoFile::com_logDemos( "com_logDemos", "0", CVAR_SYSTEM | CVAR_BOOL, "Write demo.log with debug information in it" );
anCVarSystem anDemoFile::com_compressDemos( "com_compressDemos", "1", CVAR_SYSTEM | CVAR_INTEGER | CVAR_ARCHIVE, "Compression scheme for demo files\n0: None    (Fast, large files)\n1: LZW     (Fast to compress, Fast to decompress, medium/small files)\n2: LZSS    (Slow to compress, Fast to decompress, small files)\n3: Huffman (Fast to compress, Slow to decompress, medium files)\nSee also: The 'CompressDemo' command" );
anCVarSystem anDemoFile::com_preloadDemos( "com_preloadDemos", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE, "Load the whole demo in to RAM before running it" );

#define DEMO_MAGIC GAME_NAME " RDEMO"

/*
================
anDemoFile::anDemoFile
================
*/
anDemoFile::anDemoFile() {
	f = nullptr;
	fLog = nullptr;
	log = false;
	fileImage = nullptr;
	compressor = nullptr;
	writing = false;
}

/*
================
anDemoFile::~anDemoFile
================
*/
anDemoFile::~anDemoFile() {
	Close();
}

/*
================
anDemoFile::AllocCompressor
================
*/
anCompressor *anDemoFile::AllocCompressor( int type ) {
	switch ( type ) {
	case 0: return anCompressor::AllocNoCompression();
	default:
	case 1: return anCompressor::AllocLZW();
	case 2: return anCompressor::AllocLZSS();
	case 3: return anCompressor::AllocHuffman();
	}
}

/*
================
anDemoFile::OpenForReading
================
*/
bool anDemoFile::OpenForReading( const char *fileName ) {
	static const int magicLen = sizeof(DEMO_Magic ) / sizeof(DEMO_MAGIC[0] );
	char magicBuffer[magicLen];
	int compression;
	int fileLength;

	Close();

	f = fileSystem->OpenFileRead( fileName );
	if ( !f ) {
		return false;
	}

	fileLength = f->Length();

	if ( com_preloadDemos.GetBool() ) {
		fileImage = (byte *)Mem_Alloc( fileLength, TAG_CRAP );
		f->Read( fileImage, fileLength );
		fileSystem->CloseFile( f );
		f = new (TAG_SYSTEM) anFileMemory( va( "preloaded(%s)", fileName ), (const char *)fileImage, fileLength );
	}

	if ( com_logDemos.GetBool() ) {
		fLog = fileSystem->OpenFileWrite( "demoread.log" );
	}

	writing = false;

	f->Read(magicBuffer, magicLen);
	if ( memcmp(magicBuffer, DEMO_MAGIC, magicLen) == 0 ) {
		f->ReadInt( compression );
	} else {
		// Ideally we would error out if the magic string isn't there,
		// but for backwards compatibility we are going to assume it's just an uncompressed demo file
		compression = 0;
		f->Rewind();
	}

	compressor = AllocCompressor( compression );
	compressor->Init( f, false, 8 );

	return true;
}

/*
================
anDemoFile::SetLog
================
*/
void anDemoFile::SetLog(bool b, const char *p) {
	log = b;
	if (p) {
		logStr = p;
	}
}

/*
================
anDemoFile::Log
================
*/
void anDemoFile::Log(const char *p) {
	if ( fLog && p && *p ) {
		fLog->Write( p, strlen(p) );
	}
}

/*
================
anDemoFile::OpenForWriting
================
*/
bool anDemoFile::OpenForWriting( const char *fileName ) {
	Close();

	f = fileSystem->OpenFileWrite( fileName );
	if ( f == nullptr ) {
		return false;
	}

	if ( com_logDemos.GetBool() ) {
		fLog = fileSystem->OpenFileWrite( "demowrite.log" );
	}

	writing = true;

	f->Write(DEMO_MAGIC, sizeof(DEMO_Magic ) );
	f->WriteInt( com_compressDemos.GetInteger() );
	f->Flush();

	compressor = AllocCompressor( com_compressDemos.GetInteger() );
	compressor->Init( f, true, 8 );

	return true;
}

/*
================
anDemoFile::Close
================
*/
void anDemoFile::Close() {
	if ( writing && compressor ) {
		compressor->FinishCompress();
	}

	if ( f ) {
		fileSystem->CloseFile( f );
		f = nullptr;
	}
	if ( fLog ) {
		fileSystem->CloseFile( fLog );
		fLog = nullptr;
	}
	if ( fileImage ) {
		Mem_Free( fileImage );
		fileImage = nullptr;
	}
	if ( compressor ) {
		delete compressor;
		compressor = nullptr;
	}

	demoStrings.DeleteContents( true );
}

/*
================
anDemoFile::ReadHashString
================
*/
const char *anDemoFile::ReadHashString() {
	int		index;

	if ( log && fLog ) {
		const char *text = va( "%s > Reading hash string\n", logStr.c_str() );
		fLog->Write( text, strlen( text ) );
	}

	ReadInt( index );

	if ( index == -1 ) {
		// read a new string for the table
		anStr	*str = new (TAG_SYSTEM) anStr;

		anStr data;
		ReadString( data );
		*str = data;

		demoStrings.Append( str );

		return *str;
	}

	if ( index < -1 || index >= demoStrings.Num() ) {
		Close();
		common->Error( "demo hash index out of range" );
	}

	return demoStrings[index]->c_str();
}

/*
================
anDemoFile::WriteHashString
================
*/
void anDemoFile::WriteHashString( const char *str ) {
	if ( log && fLog ) {
		const char *text = va( "%s > Writing hash string\n", logStr.c_str() );
		fLog->Write( text, strlen( text ) );
	}
	// see if it is already in the has table
	for ( int i = 0; i < demoStrings.Num(); i++ ) {
		if ( !strcmp( demoStrings[i]->c_str(), str ) ) {
			WriteInt( i );
			return;
		}
	}

	// add it to our table and the demo table
	anStr	*copy = new (TAG_SYSTEM) anStr( str );
//common->Printf( "hash:%i = %s\n", demoStrings.Num(), str );
	demoStrings.Append( copy );
	int cmd = -1;
	WriteInt( cmd );
	WriteString( str );
}

/*
================
anDemoFile::ReadDict
================
*/
void anDemoFile::ReadDict( anDict &dict ) {
	int i, c;
	anStr key, val;

	dict.Clear();
	ReadInt( c );
	for ( i = 0; i < c; i++ ) {
		key = ReadHashString();
		val = ReadHashString();
		dict.Set( key, val );
	}
}

/*
================
anDemoFile::WriteDict
================
*/
void anDemoFile::WriteDict( const anDict &dict ) {
	int i, c;

	c = dict.GetNumKeyVals();
	WriteInt( c );
	for ( i = 0; i < c; i++ ) {
		WriteHashString( dict.GetKeyVal( i )->GetKey() );
		WriteHashString( dict.GetKeyVal( i )->GetValue() );
	}
}

/*
 ================
 anDemoFile::Read
 ================
 */
int anDemoFile::Read( void *buffer, int len ) {
	int read = compressor->Read( buffer, len );
	if ( read == 0 && len >= 4 ) {
		*(demoSystem_t *)buffer = DS_FINISHED;
	}
	return read;
}

/*
 ================
 anDemoFile::Write
 ================
 */
int anDemoFile::Write( const void *buffer, int len ) {
	return compressor->Write( buffer, len );
}




