#pragma hdrstop
#include "/idlib/Lib.h"

#include "File_SaveGame.h"

// TODO: CRC on each block

/*
========================
ZlibAlloc
========================
*/
void * ZlibAlloc( void *opaque, uInt items, uInt size ) {
	return Mem_Alloc( items * size, TAG_SAVEGAMES );
}

/*
========================
ZlibFree
========================
*/
void ZlibFree( void *opaque, void * address ) {
	Mem_Free( address );
}

anCVarSystem sgf_threads( "sgf_threads", "2", CVAR_INTEGER, "0 = all foreground, 1 = background write, 2 = background write + compress" );
anCVarSystem sgf_checksums( "sgf_checksums", "1", CVAR_BOOL, "enable save game file checksums" );
anCVarSystem sgf_testCorruption( "sgf_testCorruption", "-1", CVAR_INTEGER, "test corruption at the 128 kB compressed block" );

// this is supposed to get faster going from -15 to -9, but it gets slower as well as worse compression
anCVarSystem sgf_windowBits( "sgf_windowBits", "-15", CVAR_INTEGER, "zlib window bits" );

bool anFile_SGPipelined::cancelToTerminate = false;

class idSGFcompressThread : public anSysThread {
public:
	virtual int			Run() { sgf->CompressBlock(); return 0; }
	anFile_SGPipelined * sgf;
};
class idSGFdecompressThread : public anSysThread {
public:
	virtual int			Run() { sgf->DecompressBlock(); return 0; }
	anFile_SGPipelined * sgf;
};
class anSGFwriteThread : public anSysThread {
public:
	virtual int			Run() { sgf->WriteBlock(); return 0; }
	anFile_SGPipelined * sgf;
};
class anSGFreedThread : public anSysThread {
public:
	virtual int			Run() { sgf->ReadBlock(); return 0; }
	anFile_SGPipelined * sgf;
};

/*
============================
anFile_SGPipelined::anFile_SGPipelined
============================
*/
anFile_SGPipelined::anFile_SGPipelined() :
		mode( CLOSED ),
		compressedLength( 0 ),
		uncompressedProducedBytes( 0 ),
		uncompressedConsumedBytes( 0 ),
		compressedProducedBytes( 0 ),
		compressedConsumedBytes( 0 ),
		dataZlib( nullptr ),
		bytesZlib( 0 ),
		dataIO( nullptr ),
		bytesIO( 0 ),
		zLibFlushType( Z_NO_FLUSH ),
		zStreamEndHit( false ),
		numChecksums( 0 ),
		nativeFile( nullptr ),
		nativeFileEndHit( false ),
		finished( false ),
		readThread( nullptr ),
		writeThread( nullptr ),
		decompressThread( nullptr ),
		compressThread( nullptr ),
		blockFinished( true ),
		buildVersion( "" ),
		saveFormatVersion( 0 ) {

	memset( &zStream, 0, sizeof( zStream ) );
	memset( compressed, 0, sizeof( compressed ) );
	memset( uncompressed, 0, sizeof( uncompressed ) );
	zStream.zalloc = ZlibAlloc;
	zStream.zfree = ZlibFree;
}

/*
============================
anFile_SGPipelined::~anFile_SGPipelined
============================
*/
anFile_SGPipelined::~anFile_SGPipelined() {
	Finish();

	// free the threads
	if ( compressThread != nullptr ) {
		delete compressThread;
		compressThread = nullptr;
	}
	if ( decompressThread != nullptr ) {
		delete decompressThread;
		decompressThread = nullptr;
	}
	if ( readThread != nullptr ) {
		delete readThread;
		readThread = nullptr;
	}
	if ( writeThread != nullptr ) {
		delete writeThread;
		writeThread = nullptr;
	}

	// close the native file
/*	if ( nativeFile != nullptr ) {
		delete nativeFile;
		nativeFile = nullptr;
	} */

	dataZlib = nullptr;
	dataIO = nullptr;
}

/*
========================
anFile_SGPipelined::ReadBuildVersion
========================
*/
bool anFile_SGPipelined::ReadBuildVersion() {
	return ReadString( buildVersion ) != 0;
}

/*
========================
anFile_SGPipelined::ReadSaveFormatVersion
========================
*/
bool anFile_SGPipelined::ReadSaveFormatVersion() {
	if ( ReadBig( pointerSize ) <= 0 ) {
		return false;
	}
	return ReadBig( saveFormatVersion ) != 0;
}

/*
========================
anFile_SGPipelined::GetPointerSize
========================
*/
int anFile_SGPipelined::GetPointerSize() const {
	if ( pointerSize == 0 ) {
		// in original savegames we weren't saving the pointer size, so the 2 high bytes of the save version will be 0
		return 4;
	}  else {
		return pointerSize;
	}
}

/*
============================
anFile_SGPipelined::Finish
============================
*/
void anFile_SGPipelined::Finish() {
	if ( mode == WRITE ) {
		// wait for the compression thread to complete, which may kick off a write
		if ( compressThread != nullptr ) {
			compressThread->WaitForThread();
		}

		// force the next compression to emit everything
		zLibFlushType = Z_FINISH;
		FlushUncompressedBlock();

		if ( compressThread != nullptr ) {
			compressThread->WaitForThread();
		}

		if ( writeThread != nullptr ) {
			// wait for the IO thread to exit
			writeThread->WaitForThread();
		} else if ( nativeFile == nullptr && !nativeFileEndHit ) {
			// wait for the last block to be consumed
			blockRequested.Wait();
			finished = true;
			blockAvailable.Raise();
			blockFinished.Wait();
		}

		// free zlib tables
		deflateEnd( &zStream );
	} else if ( mode == READ ) {
		// wait for the decompression thread to complete, which may kick off a read
		if ( decompressThread != nullptr ) {
			decompressThread->WaitForThread();
		}

		if ( readThread != nullptr ) {
			// wait for the IO thread to exit
			readThread->WaitForThread();
		} else if ( nativeFile == nullptr && !nativeFileEndHit ) {
			// wait for the last block to be consumed
			blockAvailable.Wait();
			finished = true;
			blockRequested.Raise();
			blockFinished.Wait();
		}

		// free zlib tables
		inflateEnd( &zStream );
	}

	mode = CLOSED;
}

/*
============================
anFile_SGPipelined::Abort
============================
*/
void anFile_SGPipelined::Abort() {
	if ( mode == WRITE ) {

		if ( compressThread != nullptr ) {
			compressThread->WaitForThread();
		}
		if ( writeThread != nullptr ) {
			writeThread->WaitForThread();
		} else if ( nativeFile == nullptr && !nativeFileEndHit ) {
			blockRequested.Wait();
			finished = true;
			dataIO = nullptr;
			bytesIO = 0;
			blockAvailable.Raise();
			blockFinished.Wait();
		}
	} else if ( mode == READ ) {
		if ( decompressThread != nullptr ) {
			decompressThread->WaitForThread();
		}
		if ( readThread != nullptr ) {
			readThread->WaitForThread();
		} else if ( nativeFile == nullptr && !nativeFileEndHit ) {
			blockAvailable.Wait();
			finished = true;
			dataIO = nullptr;
			bytesIO = 0;
			blockRequested.Raise();
			blockFinished.Wait();
		}
	}

	mode = CLOSED;
}

/*
===================================================================================

WRITE PATH

===================================================================================
*/

/*
============================
anFile_SGPipelined::OpenForWriting
============================
*/
bool anFile_SGPipelined::OpenForWriting( const char *const filename, bool useNativeFile ) {
	assert( mode == CLOSED );

	name = filename;
	osPath = filename;
	mode = WRITE;
	nativeFile = nullptr;
	numChecksums = 0;

	if ( useNativeFile ) {
		nativeFile = fileSystem->OpenFileWrite( filename );
		if ( nativeFile == nullptr ) {
			return false;
		}
	}

	// raw deflate with no header / checksum
	// use max memory for fastest compression
	// optimize for higher speed
	//mem.PushHeap();
	int status = deflateInit2( &zStream, Z_BEST_SPEED, Z_DEFLATED, sgf_windowBits.GetInteger(), 9, Z_DEFAULT_STRATEGY );
	//mem.PopHeap();
	if ( status != Z_OK ) {
		anLibrary::FatalError( "anFile_SGPipelined::OpenForWriting: deflateInit2() error %i", status );
	}

	// initial buffer setup
	zStream.avail_out = COMPRESSED_BLOCK_SIZE;
	zStream.next_out = (Bytef * )compressed;

	if ( sgf_checksums.GetBool() ) {
		zStream.avail_out -= sizeof( uint32 );
	}

	if ( sgf_threads.GetInteger() >= 1 ) {
		compressThread = new (TAG_IDFILE) idSGFcompressThread();
		compressThread->sgf = this;
		compressThread->StartWorkerThread( "SGF_CompressThread", CORE_2B, THREAD_NORMAL );
	}
	if ( nativeFile != nullptr && sgf_threads.GetInteger() >= 2 ) {
		writeThread = new (TAG_IDFILE) anSGFwriteThread();
		writeThread->sgf = this;
		writeThread->StartWorkerThread( "SGF_WriteThread", CORE_2A, THREAD_NORMAL );
	}

	return true;
}

/*
============================
anFile_SGPipelined::OpenForWriting
============================
*/
bool anFile_SGPipelined::OpenForWriting( anFile * file )  {
	assert( mode == CLOSED );

	if ( file == nullptr ) {
		return false;
	}

	name = file->GetName();
	osPath = file->GetFullPath();
	mode = WRITE;
	nativeFile = file;
	numChecksums = 0;


	// raw deflate with no header / checksum
	// use max memory for fastest compression
	// optimize for higher speed
	//mem.PushHeap();
	int status = deflateInit2( &zStream, Z_BEST_SPEED, Z_DEFLATED, sgf_windowBits.GetInteger(), 9, Z_DEFAULT_STRATEGY );
	//mem.PopHeap();
	if ( status != Z_OK ) {
		anLibrary::FatalError( "anFile_SGPipelined::OpenForWriting: deflateInit2() error %i", status );
	}

	// initial buffer setup
	zStream.avail_out = COMPRESSED_BLOCK_SIZE;
	zStream.next_out = (Bytef * )compressed;

	if ( sgf_checksums.GetBool() ) {
		zStream.avail_out -= sizeof( uint32 );
	}

	if ( sgf_threads.GetInteger() >= 1 ) {
		compressThread = new (TAG_IDFILE) idSGFcompressThread();
		compressThread->sgf = this;
		compressThread->StartWorkerThread( "SGF_CompressThread", CORE_2B, THREAD_NORMAL );
	}
	if ( nativeFile != nullptr && sgf_threads.GetInteger() >= 2 ) {
		writeThread = new (TAG_IDFILE) anSGFwriteThread();
		writeThread->sgf = this;
		writeThread->StartWorkerThread( "SGF_WriteThread", CORE_2A, THREAD_NORMAL );
	}

	return true;
}

/*
============================
anFile_SGPipelined::NextWriteBlock

Modifies:
	dataIO
	bytesIO
============================
*/
bool anFile_SGPipelined::NextWriteBlock( blockForIO_t * block ) {
	assert( mode == WRITE );

	blockRequested.Raise();		// the background thread is done with the last block

	if ( nativeFileEndHit ) {
		return false;
	}

	blockAvailable.Wait();	// wait for a new block to come through the pipeline

	if ( finished || block == nullptr ) {
		nativeFileEndHit = true;
		blockRequested.Raise();
		blockFinished.Raise();
		return false;
	}

	compressedLength += bytesIO;

	block->data = dataIO;
	block->bytes = bytesIO;

	dataIO = nullptr;
	bytesIO = 0;

	return true;
}

/*
============================
anFile_SGPipelined::WriteBlock

Modifies:
	dataIO
	bytesIO
	nativeFile
============================
*/
void anFile_SGPipelined::WriteBlock() {
	assert( nativeFile != nullptr );

	compressedLength += bytesIO;

	nativeFile->Write( dataIO, bytesIO );

	dataIO = nullptr;
	bytesIO = 0;
}

/*
============================
anFile_SGPipelined::FlushCompressedBlock

Called when a compressed block fills up, and also to flush the final partial block.
Flushes everything from [compressedConsumedBytes -> compressedProducedBytes)

Reads:
	compressed
	compressedProducedBytes

Modifies:
	dataZlib
	bytesZlib
	compressedConsumedBytes
============================
*/
void anFile_SGPipelined::FlushCompressedBlock() {
	// block until the background thread is done with the last block
	if ( writeThread != nullptr ) {
		writeThread->WaitForThread();
	} if ( nativeFile == nullptr ) {
		if ( !nativeFileEndHit ) {
			blockRequested.Wait();
		}
	}

	// prepare the next block to be written out
	dataIO = &compressed[ compressedConsumedBytes & ( COMPRESSED_BUFFER_SIZE - 1 ) ];
	bytesIO = compressedProducedBytes - compressedConsumedBytes;
	compressedConsumedBytes = compressedProducedBytes;

	if ( writeThread != nullptr ) {
		// signal a new block is available to be written out
		writeThread->SignalWork();
	} else if ( nativeFile != nullptr ) {
		// write syncronously
		WriteBlock();
	} else {
		// signal a new block is available to be written out
		blockAvailable.Raise();
	}
}

/*
============================
anFile_SGPipelined::CompressBlock

Called when an uncompressed block fills up, and also to flush the final partial block.
Flushes everything from [uncompressedConsumedBytes -> uncompressedProducedBytes)

Modifies:
	dataZlib
	bytesZlib
	compressed
	compressedProducedBytes
	zStream
	zStreamEndHit
============================
*/
void anFile_SGPipelined::CompressBlock() {
	zStream.next_in = (Bytef * )dataZlib;
	zStream.avail_in = (uInt) bytesZlib;

	dataZlib = nullptr;
	bytesZlib = 0;

	// if this is the finish block, we may need to write
	// multiple buffers even after all input has been consumed
	while( zStream.avail_in > 0 || zLibFlushType == Z_FINISH ) {

		const int zstat = deflate( &zStream, zLibFlushType );

		if ( zstat != Z_OK && zstat != Z_STREAM_END ) {
			anLibrary::FatalError( "anFile_SGPipelined::CompressBlock: deflate() returned %i", zstat );
		}

		if ( zStream.avail_out == 0 || zLibFlushType == Z_FINISH ) {

			if ( sgf_checksums.GetBool() ) {
				size_t blockSize = zStream.total_out + numChecksums * sizeof( uint32 ) - compressedProducedBytes;
				uint32 checksum = MD5_BlockChecksum( zStream.next_out - blockSize, blockSize );
				zStream.next_out[0] = ( ( checksum >>  0 ) & 0xFF );
				zStream.next_out[1] = ( ( checksum >>  8 ) & 0xFF );
				zStream.next_out[2] = ( ( checksum >> 16 ) & 0xFF );
				zStream.next_out[3] = ( ( checksum >> 24 ) & 0xFF );
				numChecksums++;
			}

			// flush the output buffer IO
			compressedProducedBytes = zStream.total_out + numChecksums * sizeof( uint32 );
			FlushCompressedBlock();
			if ( zstat == Z_STREAM_END ) {
				assert( zLibFlushType == Z_FINISH );
				zStreamEndHit = true;
				return;
			}

			assert( 0 == ( compressedProducedBytes & ( COMPRESSED_BLOCK_SIZE - 1 ) ) );

			zStream.avail_out = COMPRESSED_BLOCK_SIZE;
			zStream.next_out = (Bytef * )&compressed[ compressedProducedBytes & ( COMPRESSED_BUFFER_SIZE - 1 ) ];

			if ( sgf_checksums.GetBool() ) {
				zStream.avail_out -= sizeof( uint32 );
			}
		}
	}
}

/*
============================
anFile_SGPipelined::FlushUncompressedBlock

Called when an uncompressed block fills up, and also to flush the final partial block.
Flushes everything from [uncompressedConsumedBytes -> uncompressedProducedBytes)

Reads:
	uncompressed
	uncompressedProducedBytes

Modifies:
	dataZlib
	bytesZlib
	uncompressedConsumedBytes
============================
*/
void anFile_SGPipelined::FlushUncompressedBlock() {
	// block until the background thread has completed
	if ( compressThread != nullptr ) {
		// make sure thread has completed the last work
		compressThread->WaitForThread();
	}

	// prepare the next block to be consumed by Zlib
	dataZlib = &uncompressed[ uncompressedConsumedBytes & ( UNCOMPRESSED_BUFFER_SIZE - 1 ) ];
	bytesZlib = uncompressedProducedBytes - uncompressedConsumedBytes;
	uncompressedConsumedBytes = uncompressedProducedBytes;

	if ( compressThread != nullptr ) {
		// signal thread for more work
		compressThread->SignalWork();
	} else {
		// run syncronously
		CompressBlock();
	}
}

/*
============================
anFile_SGPipelined::Write

Modifies:
	uncompressed
	uncompressedProducedBytes
============================
*/
int anFile_SGPipelined::Write( const void * buffer, int length ) {
	if ( buffer == nullptr || length <= 0 ) {
		return 0;
	}

#if 1	// quick and dirty fix for user-initiated forced shutdown during a savegame
	if ( cancelToTerminate ) {
		if ( mode != CLOSED ) {
			Abort();
		}
		return 0;
	}
#endif

	assert( mode == WRITE );
	size_t lengthRemaining = length;
	const byte * buffer_p = (const byte *)buffer;
	while ( lengthRemaining > 0 ) {
		const size_t ofsInBuffer = uncompressedProducedBytes & ( UNCOMPRESSED_BUFFER_SIZE - 1 );
		const size_t ofsInBlock = uncompressedProducedBytes & ( UNCOMPRESSED_BLOCK_SIZE - 1 );
		const size_t remainingInBlock = UNCOMPRESSED_BLOCK_SIZE - ofsInBlock;
		const size_t copyToBlock = ( lengthRemaining < remainingInBlock ) ? lengthRemaining : remainingInBlock;

		memcpy( uncompressed + ofsInBuffer, buffer_p, copyToBlock );
		uncompressedProducedBytes += copyToBlock;

		buffer_p += copyToBlock;
		lengthRemaining -= copyToBlock;

		if ( copyToBlock == remainingInBlock ) {
			FlushUncompressedBlock();
		}
	}
	return length;
}

/*
===================================================================================

READ PATH

===================================================================================
*/

/*
============================
anFile_SGPipelined::OpenForReading
============================
*/
bool anFile_SGPipelined::OpenForReading( const char *const filename, bool useNativeFile ) {
	assert( mode == CLOSED );

	name = filename;
	osPath = filename;
	mode = READ;
	nativeFile = nullptr;
	numChecksums = 0;

	if ( useNativeFile ) {
		nativeFile = fileSystem->OpenFileRead( filename );
		if ( nativeFile == nullptr ) {
			return false;
		}
	}

	// init zlib for raw inflate with a 32k dictionary
	//mem.PushHeap();
	int status = inflateInit2( &zStream, sgf_windowBits.GetInteger() );
	//mem.PopHeap();
	if ( status != Z_OK ) {
		anLibrary::FatalError( "anFile_SGPipelined::OpenForReading: inflateInit2() error %i", status );
	}

	// spawn threads
	if ( sgf_threads.GetInteger() >= 1 ) {
		decompressThread = new (TAG_IDFILE) idSGFdecompressThread();
		decompressThread->sgf = this;
		decompressThread->StartWorkerThread( "SGF_DecompressThread", CORE_2B, THREAD_NORMAL );
	}
	if ( nativeFile != nullptr && sgf_threads.GetInteger() >= 2 ) {
		readThread = new (TAG_IDFILE) anSGFreedThread();
		readThread->sgf = this;
		readThread->StartWorkerThread( "SGF_ReadThread", CORE_2A, THREAD_NORMAL );
	}

	return true;
}


/*
============================
anFile_SGPipelined::OpenForReading
============================
*/
bool anFile_SGPipelined::OpenForReading( anFile * file ) {
	assert( mode == CLOSED );

	if ( file == nullptr ) {
		return false;
	}

	name = file->GetName();
	osPath = file->GetFullPath();
	mode = READ;
	nativeFile = file;
	numChecksums = 0;

	// init zlib for raw inflate with a 32k dictionary
	//mem.PushHeap();
	int status = inflateInit2( &zStream, sgf_windowBits.GetInteger() );
	//mem.PopHeap();
	if ( status != Z_OK ) {
		anLibrary::FatalError( "anFile_SGPipelined::OpenForReading: inflateInit2() error %i", status );
	}

	// spawn threads
	if ( sgf_threads.GetInteger() >= 1 ) {
		decompressThread = new (TAG_IDFILE) idSGFdecompressThread();
		decompressThread->sgf = this;
		decompressThread->StartWorkerThread( "SGF_DecompressThread", CORE_1B, THREAD_NORMAL );
	}
	if ( nativeFile != nullptr && sgf_threads.GetInteger() >= 2 ) {
		readThread = new (TAG_IDFILE) anSGFreedThread();
		readThread->sgf = this;
		readThread->StartWorkerThread( "SGF_ReadThread", CORE_1A, THREAD_NORMAL );
	}

	return true;
}


/*
============================
anFile_SGPipelined::NextReadBlock

Reads the next data block from the filesystem into the memory buffer.

Modifies:
	compressed
	compressedProducedBytes
	nativeFileEndHit
============================
*/
bool anFile_SGPipelined::NextReadBlock( blockForIO_t * block, size_t lastReadBytes ) {
	assert( mode == READ );

	assert( ( lastReadBytes & ( COMPRESSED_BLOCK_SIZE - 1 ) ) == 0 || block == nullptr );
	compressedProducedBytes += lastReadBytes;

	blockAvailable.Raise();		// a new block is available for the pipeline to consume

	if ( nativeFileEndHit ) {
		return false;
	}

	blockRequested.Wait();		// wait for the last block to be consumed by the pipeline

	if ( finished || block == nullptr ) {
		nativeFileEndHit = true;
		blockAvailable.Raise();
		blockFinished.Raise();
		return false;
	}

	assert( 0 == ( compressedProducedBytes & ( COMPRESSED_BLOCK_SIZE - 1 ) ) );
	block->data = & compressed[compressedProducedBytes & ( COMPRESSED_BUFFER_SIZE - 1 )];
	block->bytes = COMPRESSED_BLOCK_SIZE;

	return true;
}

/*
============================
anFile_SGPipelined::ReadBlock

Reads the next data block from the filesystem into the memory buffer.

Modifies:
	compressed
	compressedProducedBytes
	nativeFile
	nativeFileEndHit
============================
*/
void anFile_SGPipelined::ReadBlock() {
	assert( nativeFile != nullptr );
	// normally run in a separate thread
	if ( nativeFileEndHit ) {
		return;
	}
	// when we are reading the last block of the file, we may not fill the entire block
	assert( 0 == ( compressedProducedBytes & ( COMPRESSED_BLOCK_SIZE - 1 ) ) );
	byte * dest = &compressed[ compressedProducedBytes & ( COMPRESSED_BUFFER_SIZE - 1 ) ];
	size_t ioBytes = nativeFile->Read( dest, COMPRESSED_BLOCK_SIZE );
	compressedProducedBytes += ioBytes;
	if ( ioBytes != COMPRESSED_BLOCK_SIZE ) {
		nativeFileEndHit = true;
	}
}

/*
============================
anFile_SGPipelined::PumpCompressedBlock

Reads:
	compressed
	compressedProducedBytes

Modifies:
	dataIO
	byteIO
	compressedConsumedBytes
============================
*/
void anFile_SGPipelined::PumpCompressedBlock() {
	// block until the background thread is done with the last block
	if ( readThread != nullptr ) {
		readThread->WaitForThread();
	} else if ( nativeFile == nullptr ) {
		if ( !nativeFileEndHit ) {
			blockAvailable.Wait();
		}
	}

	// fetch the next block read in
	dataIO = &compressed[ compressedConsumedBytes & ( COMPRESSED_BUFFER_SIZE - 1 ) ];
	bytesIO = compressedProducedBytes - compressedConsumedBytes;
	compressedConsumedBytes = compressedProducedBytes;

	if ( readThread != nullptr ) {
		// signal read thread to read another block
		readThread->SignalWork();
	} else if ( nativeFile != nullptr ) {
		// run syncronously
		ReadBlock();
	} else {
		// request a new block
		blockRequested.Raise();
	}
}

/*
============================
anFile_SGPipelined::DecompressBlock

Decompresses the next data block from the memory buffer

Normally this runs in a separate thread when signalled, but
can be called in the main thread for debugging.

This will not exit until a complete block has been decompressed,
unless end-of-file is reached.

This may require additional compressed blocks to be read.

Reads:
	nativeFileEndHit

Modifies:
	dataIO
	bytesIO
	uncompressed
	uncompressedProducedBytes
	zStreamEndHit
	zStream
============================
*/
void anFile_SGPipelined::DecompressBlock() {
	if ( zStreamEndHit ) {
		return;
	}

	assert( ( uncompressedProducedBytes & ( UNCOMPRESSED_BLOCK_SIZE - 1 ) ) == 0 );
	zStream.next_out = (Bytef * )&uncompressed[ uncompressedProducedBytes & ( UNCOMPRESSED_BUFFER_SIZE - 1 ) ];
	zStream.avail_out = UNCOMPRESSED_BLOCK_SIZE;

	while( zStream.avail_out > 0 ) {
		if ( zStream.avail_in == 0 ) {
			do {
				PumpCompressedBlock();
				if ( bytesIO == 0 && nativeFileEndHit ) {
					// don't try to decompress any more if there is no more data
					zStreamEndHit = true;
					return;
				}
			} while ( bytesIO == 0 );

			zStream.next_in = (Bytef *) dataIO;
			zStream.avail_in = (uInt) bytesIO;

			dataIO = nullptr;
			bytesIO = 0;

			if ( sgf_checksums.GetBool() ) {
				if ( sgf_testCorruption.GetInteger() == numChecksums ) {
					zStream.next_in[0] ^= 0xFF;
				}
				zStream.avail_in -= sizeof( uint32 );
				uint32 checksum = MD5_BlockChecksum( zStream.next_in, zStream.avail_in );
				if (	!verify( zStream.next_in[zStream.avail_in + 0] == ( ( checksum >>  0 ) & 0xFF ) ) ||
						!verify( zStream.next_in[zStream.avail_in + 1] == ( ( checksum >>  8 ) & 0xFF ) ) ||
						!verify( zStream.next_in[zStream.avail_in + 2] == ( ( checksum >> 16 ) & 0xFF ) ) ||
						!verify( zStream.next_in[zStream.avail_in + 3] == ( ( checksum >> 24 ) & 0xFF ) ) ) {
					// don't try to decompress any more if the checksum is wrong
					zStreamEndHit = true;
					return;
				}
				numChecksums++;
			}
		}

		const int zstat = inflate( &zStream, Z_SYNC_FLUSH );

		uncompressedProducedBytes = zStream.total_out;

		if ( zstat == Z_STREAM_END ) {
			// don't try to decompress any more
			zStreamEndHit = true;
			return;
		}
		if ( zstat != Z_OK ) {
			anLibrary::Warning( "anFile_SGPipelined::DecompressBlock: inflate() returned %i", zstat );
			zStreamEndHit = true;
			return;
		}
	}

	assert( ( uncompressedProducedBytes & ( UNCOMPRESSED_BLOCK_SIZE - 1 ) ) == 0 );
}

/*
============================
anFile_SGPipelined::PumpUncompressedBlock

Called when an uncompressed block is drained.

Reads:
	uncompressed
	uncompressedProducedBytes

Modifies:
	dataZlib
	bytesZlib
	uncompressedConsumedBytes
============================
*/
void anFile_SGPipelined::PumpUncompressedBlock() {
	if ( decompressThread != nullptr ) {
		// make sure thread has completed the last work
		decompressThread->WaitForThread();
	}

	// fetch the next block produced by Zlib
	dataZlib = &uncompressed[ uncompressedConsumedBytes & ( UNCOMPRESSED_BUFFER_SIZE - 1 ) ];
	bytesZlib = uncompressedProducedBytes - uncompressedConsumedBytes;
	uncompressedConsumedBytes = uncompressedProducedBytes;

	if ( decompressThread != nullptr ) {
		// signal thread for more work
		decompressThread->SignalWork();
	} else {
		// run syncronously
		DecompressBlock();
	}
}

/*
============================
anFile_SGPipelined::Read

Modifies:
	dataZlib
	bytesZlib
============================
*/
int anFile_SGPipelined::Read( void * buffer, int length ) {
	if ( buffer == nullptr || length <= 0 ) {
		return 0;
	}

	assert( mode == READ );

	size_t ioCount = 0;
	size_t lengthRemaining = length;
	byte * buffer_p = (byte *)buffer;
	while ( lengthRemaining > 0 ) {
		while ( bytesZlib == 0 ) {
			PumpUncompressedBlock();
			if ( bytesZlib == 0 && zStreamEndHit ) {
				return ioCount;
			}
		}

		const size_t copyFromBlock = ( lengthRemaining < bytesZlib ) ? lengthRemaining : bytesZlib;

		memcpy( buffer_p, dataZlib, copyFromBlock );
		dataZlib += copyFromBlock;
		bytesZlib -= copyFromBlock;

		buffer_p += copyFromBlock;
		ioCount += copyFromBlock;
		lengthRemaining -= copyFromBlock;
	}
	return ioCount;
}

/*
===================================================================================

TEST CODE

===================================================================================
*/

/*
============================
TestProcessFile
============================
*/
static void TestProcessFile( const char *const filename ) {
	anLibrary::Printf( "Processing %s:\n", filename );
	// load some test data
	void *testData;
	const int testDataLength = fileSystem->ReadFile( filename, &testData, nullptr );

	const char *const outFileName = "junk/savegameTest.bin";
	anFile_SGPipelined *saveFile = new (TAG_IDFILE) anFile_SGPipelined;
	saveFile->OpenForWriting( outFileName, true );

	const uint64 startWriteMicroseconds = Sys_Microseconds();

	saveFile->Write( testData, testDataLength );
	delete saveFile;		// final flush
	const int readDataLength = fileSystem->GetFileLength( outFileName );

	const uint64 endWriteMicroseconds = Sys_Microseconds();
	const uint64 writeMicroseconds = endWriteMicroseconds - startWriteMicroseconds;

	anLibrary::Printf( "%lld microseconds to compress %i bytes to %i written bytes = %4.1f MB/s\n",
		writeMicroseconds, testDataLength, readDataLength, ( float )readDataLength / writeMicroseconds );

	void * readData = (void *)Mem_Alloc( testDataLength, TAG_SAVEGAMES );

	const uint64 startReadMicroseconds = Sys_Microseconds();

	anFile_SGPipelined *loadFile = new (TAG_IDFILE) anFile_SGPipelined;
	loadFile->OpenForReading( outFileName, true );
	loadFile->Read( readData, testDataLength );
	delete loadFile;

	const uint64 endReadMicroseconds = Sys_Microseconds();
	const uint64 readMicroseconds = endReadMicroseconds - startReadMicroseconds;

	anLibrary::Printf( "%lld microseconds to decompress = %4.1f MB/s\n", readMicroseconds, ( float )testDataLength / readMicroseconds );

	int comparePoint;
	for ( comparePoint = 0; comparePoint < testDataLength; comparePoint++ ) {
		if ( ( (byte *)readData)[comparePoint] != ( (byte *)testData)[comparePoint] ) {
			break;
		}
	}
	if ( comparePoint != testDataLength ) {
		anLibrary::Printf( "Compare failed at %i.\n", comparePoint );
		assert( 0 );
	} else {
		anLibrary::Printf( "Compare succeeded.\n" );
	}
	Mem_Free( readData );
	Mem_Free( testData );
}

/*
============================
TestSaveGameFile
============================
*/
CONSOLE_COMMAND( TestSaveGameFile, "Exercises the pipelined savegame code", 0 ) {
#if 1
	TestProcessFile( "maps/game/wasteland1/wasteland1.map" );
#else
	// test every file in base (found a fencepost error >100 files in originally!)
	anFileList * fileList = fileSystem->ListFiles( "", "" );
	for ( int i = 0; i < fileList->GetNumFiles(); i++ ) {
		TestProcessFile( fileList->GetFile( i ) );
		common->UpdateConsoleDisplay();
	}
	delete fileList;
#endif
}

/*
============================
TestCompressionSpeeds
============================
*/
CONSOLE_COMMAND( TestCompressionSpeeds, "Compares zlib and our code", 0 ) {
	const char *const filename = "-colorMap.tga";

	anLibrary::Printf( "Processing %s:\n", filename );
	// load some test data
	void *testData;
	const int testDataLength = fileSystem->ReadFile( filename, &testData, nullptr );

	const int startWriteMicroseconds = Sys_Microseconds();

	anCompressor *compressor = anCompressor::AllocLZW();
//	anFile *f = fileSystem->OpenFileWrite( "junk/lzwTest.bin" );
	anFileMemory *f = new (TAG_IDFILE) anFileMemory( "junk/lzwTest.bin" );
	compressor->Init( f, true, 8 );

	compressor->Write( testData, testDataLength );

	const int readDataLength = f->Tell();

	delete compressor;
	delete f;

	const int endWriteMicroseconds = Sys_Microseconds();
	const int writeMicroseconds = endWriteMicroseconds - startWriteMicroseconds;

	anLibrary::Printf( "%i microseconds to compress %i bytes to %i written bytes = %4.1f MB/s\n",
		writeMicroseconds, testDataLength, readDataLength, ( float )readDataLength / writeMicroseconds );

}

