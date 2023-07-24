#ifndef __DECL_BINARY__
#define __DECL_BINARY__

/*
==============
ARCBinaryDecl

A helper class to ensure that binary data generation is handled uniformly across decl types
Should be added to the Parse() member of an arcDecl-derived class

To ensure that tokens are generated consistently it should be created after the LEXFL_  parsing flags are set on the input ARCParser

The destructor handles setting dependencies and storing any binary generated data
==============
*/


/*
============
ARCBinaryDecl
============
*/
class ARCBinaryDecl {
public:
							ARCBinaryDecl( arcDecl *decl_, const char *text, int textLength, ARCParser &src_ );
							~ARCBinaryDecl();

	void					GetBinaryBuffer( const byte *&buffer, int& length ) const;
	const arcFile_Memory*	GetOutputFile() const { return binaryOutput; }
	void					Finish( void );

private:
	ARCParser				&src;				// parser that we're reading from/settings up
	arcFile_Memory			*binaryOutput;		// tokenized parser output to be stored on the decl

	arcDeclAudio			*decl;				// source/target decl
	arcDeclMaterialCLass	*type;				// type of the decl, used for decl type-specific behavior

	byte*					declBuffer;			// binary buffer retrieved from the decl
	int						declBufferLength;	// binary buffer length retrieved from the decl
};


/*
============
ARCBinaryDecl::ARCBinaryDecl
============
*/
ARC_INLINE ARCBinaryDecl::ARCBinaryDecl( arcDecl *decl_, const char *text, int textLength, ARCParser &src_ ) :
	binaryOutput( NULL ),
	decl( decl_ ),
	src( src_ ),
	declBuffer( NULL ),
	declBufferLength( 0 ) {

	type = declManager->GetDeclType( decl->GetType() );

	arcNetTokenCache* cache = NULL;
	if ( !type->UsePrivateTokens() ) {
		cache = &declManager->GetGlobalTokenCache();
	}

	if ( decl->HasBinaryBuffer() && decl->GetState() != DS_DEFAULTED ) {
		decl->GetBinarySource( declBuffer, declBufferLength );
		src.LoadMemoryBinary( declBuffer, declBufferLength, va( "%s: %s", decl->GetFileName(), decl->GetName() ), cache );
	} else {
		// store expanded text (except for templates) so that it's written in its entirety to the binary decl file
		if ( cvarSystem->GetCVarBool( "com_writeBinaryDecls" ) && !type->WriteBinary() && !type->AlwaysGenerateBinary() && decl->GetState() != DS_DEFAULTED ) {
			src.LoadMemory( text, textLength, va( "%s: %s", decl->GetFileName(), decl->GetName() ), decl->GetLineNum() );
			if ( decl->GetFileLevelIncludeDependencies() != NULL ) {
				src.AddIncludes( *( decl->GetFileLevelIncludeDependencies() ) );
			}

			arcStringBuilder_Heap builder;

			arcNetToken token;
			while ( src.ReadToken( &token )) {
				if ( token.type == TT_STRING ) {
					builder += "\"";
				}
				builder += token;
				if ( token.type == TT_STRING ) {
					builder += "\"";
				}
				builder += " ";
			}
			decl->SetText( builder.c_str() );
			src.FreeSource( true );
		}
		src.LoadMemory( text, textLength, va( "%s: %s", decl->GetFileName(), decl->GetName() ), decl->GetLineNum() );
	}

	// no need to setup file dependencies in binary mode
	if ( decl->GetFileLevelIncludeDependencies() != NULL && !decl->HasBinaryBuffer() ) {
		src.AddIncludes( *( decl->GetFileLevelIncludeDependencies() ) );
	}

	if ( type->WriteBinary() ) {
		if ( ( cvarSystem->GetCVarBool( "com_writeBinaryDecls" ) || type->AlwaysGenerateBinary() ) &&
			( !decl->HasBinaryBuffer() || ( type->AlwaysGenerateBinary() && decl->GetState() == DS_DEFAULTED ) ) ) {
			binaryOutput = fileSystem->OpenMemoryFile( decl->GetName() );
			binaryOutput->SetGranularity( 256 );

			src.WriteBinary( binaryOutput, cache );
			src.ResetBinaryParsing();
		}
	}
}

/*
============
ARCBinaryDecl::~ARCBinaryDecl
============
*/
ARC_INLINE ARCBinaryDecl::~ARCBinaryDecl() {
	Finish();
}

/*
============
ARCBinaryDecl::GetBinaryBuffer
============
*/
ARC_INLINE void ARCBinaryDecl::GetBinaryBuffer( const byte*& buffer, int& length ) const {
	if ( declBuffer == NULL || declBufferLength == 0 ) {
		if ( binaryOutput != NULL ) {
			buffer = reinterpret_cast< const byte* >( binaryOutput->GetDataPtr() );
			length = binaryOutput->Length();
			return;
		}
	}

	buffer = declBuffer;
	length = declBufferLength;
}

/*
============
ARCBinaryDecl::Finish
============
*/
ARC_INLINE void ARCBinaryDecl::Finish( void ) {
	// no dependencies in binary mode, except for types that always generate
	if ( decl != NULL && ( binaryOutput == NULL || type->AlwaysGenerateBinary() ) ) {
		declManager->AddDependencies( decl, src );
	}

	if ( decl != NULL && binaryOutput != NULL && decl->GetState() != DS_DEFAULTED ) {
		decl->SetBinarySource( reinterpret_cast< const byte* >( binaryOutput->GetDataPtr() ), binaryOutput->Length() );
	}

	if ( decl != NULL && declBuffer != NULL ) {
		decl->FreeSourceBuffer( declBuffer );
		declBuffer = NULL;
	}

	if ( binaryOutput != NULL ) {
		fileSystem->CloseFile( binaryOutput );
		binaryOutput = NULL;
	}

	decl = NULL;
}


#endif // !__DECL_PARSE_HELPER__
