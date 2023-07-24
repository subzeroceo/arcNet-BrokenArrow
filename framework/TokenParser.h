#ifndef __TOKENPARSER_H__
#define __TOKENPARSER_H__
class idBinaryToken {
public:
	idBinaryToken() {
		tokenType = 0;
		tokenSubType = 0;
	}
	idBinaryToken( const arcNetToken &tok ) {
		token = tok.c_str();
		tokenType = tok.type;
		tokenSubType = tok.subtype;
	}
	bool operator==( const idBinaryToken &b ) const {
		return ( tokenType == b.tokenType && tokenSubType == b.tokenSubType && token.Cmp( b.token ) == 0 );
	}
	void Read( arcNetFile *inFile ) {
		inFile->ReadString( token );
		inFile->ReadBig( tokenType );
		inFile->ReadBig( tokenSubType );
	}
	void Write( arcNetFile *inFile ) {
		inFile->WriteString( token );
		inFile->WriteBig( tokenType );
		inFile->WriteBig( tokenSubType );
	}
	arcNetString token;
	int8  tokenType;
	short tokenSubType;
};

class idTokenIndexes {
public:
	idTokenIndexes() {}
	void Clear() {
		tokenIndexes.Clear();
	}
	int Append( short sdx ) {
		return tokenIndexes.Append( sdx );
	}
	int Num() {
		return tokenIndexes.Num();
	}
	void SetNum( int num ) {
		tokenIndexes.SetNum( num );
	}
	short &	operator[]( const int index ) {
		return tokenIndexes[index];
	}
	void SetName( const char *name ) {
		fileName = name;
	}
	const char *GetName() {
		return fileName.c_str();
	}
	void Write( arcNetFile *outFile ) {
		outFile->WriteString( fileName );
		outFile->WriteBig( ( int )tokenIndexes.Num() );
		outFile->WriteBigArray( tokenIndexes.Ptr(), tokenIndexes.Num() );
	}
	void Read( arcNetFile *inFile ) {
		inFile->ReadString( fileName );
		int num;
		inFile->ReadBig( num );
		tokenIndexes.SetNum( num );
		inFile->ReadBigArray( tokenIndexes.Ptr(), num );
	}
private:
	arcNetList< short > tokenIndexes;
	arcNetString fileName;
};

class idTokenParser {
public:
	idTokenParser() {
		timeStamp = FILE_NOT_FOUND_TIMESTAMP;
		preloaded = false;
		currentToken = 0;
		currentTokenList = 0;
	}
	~idTokenParser() {
		Clear();
	}
	void Clear() {
		tokens.Clear();
		guiTokenIndexes.Clear();
		currentToken = 0;
		currentTokenList = -1;
		preloaded = false;
	}
	void LoadFromFile( const char *filename );
	void WriteToFile (const char *filename );
	void LoadFromParser( ARCParser &parser, const char *guiName );

	bool StartParsing( const char *fileName );
	void DoneParsing() { currentTokenList = -1; }

	bool IsLoaded() { return tokens.Num() > 0; }
	bool ReadToken( arcNetToken * tok );
	int	ExpectTokenString( const char *string );
	int	ExpectTokenType( int type, int subtype, arcNetToken *token );
	int ExpectAnyToken( arcNetToken *token );
	void SetMarker() {}
	void UnreadToken( const arcNetToken *token );
	void Error( VERIFY_FORMAT_STRING const char *str, ... );
	void Warning( VERIFY_FORMAT_STRING const char *str, ... );
	int ParseInt();
	bool ParseBool();
	float ParseFloat( bool *errorFlag = NULL );
	void UpdateTimeStamp( ARC_TIME_T &t ) {
		if ( t > timeStamp ) {
			timeStamp = t;
		}
	}
private:
	arcNetList< idBinaryToken > tokens;
	arcNetList< idTokenIndexes > guiTokenIndexes;
	int currentToken;
	int currentTokenList;
	ARC_TIME_T timeStamp;
	bool preloaded;
};

#endif /* !__TOKENPARSER_H__ */
