#ifndef __PARSER_H__
#define __PARSER_H__

/*
===============================================================================

	C/C++ compatible pre-compiler

===============================================================================
*/

#define DEFINE_FIXED			0x0001

#define BUILTIN_LINE			1
#define BUILTIN_FILE			2
#define BUILTIN_DATE			3
#define BUILTIN_TIME			4
#define BUILTIN_STDC			5

#define INDENT_IF				0x0001
#define INDENT_ELSE				0x0002
#define INDENT_ELIF				0x0004
#define INDENT_IFDEF			0x0008
#define INDENT_IFNDEF			0x0010

// macro definitions
typedef struct define_s {
	char *			name;						// define name
	int				flags;						// define flags
	int				builtin;					// > 0 if builtin define
	int				numParms;					// number of define parameters
	anToken *		parms;						// define parameters
	anToken *		tokens;						// macro tokens (possibly containing parm tokens)
	struct define_s	*next;						// next defined macro in a list
	struct define_s	*hashNext;					// next define in the hash chain
} define_t;

// indents used for conditional compilation directives:
// #if, #else, #elif, #ifdef, #ifndef
typedef struct indent_s {
	int				type;						// indent type
	int				skip;						// true if skipping current indent
	anLexer *		script;						// script the indent was in
	struct indent_s	*next;						// next indent on the indent stack
} indent_t;


class anParser {

public:
					// constructor
					anParser();
					anParser( int flags );
					anParser( const char *filename, int flags = 0, bool OSPath = false );
					anParser( const char *ptr, int length, const char *name, int flags = 0 );
					// destructor
					~anParser();
					// load a source file
	int				LoadFile( const char *filename, bool OSPath = false );
					// load a source from the given memory with the given length
					// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	int				LoadMemory( const char *ptr, int length, const char *name );
					// free the current source
	void			FreeSource( bool keepDefines = false );
					// returns true if a source is loaded
	int				IsLoaded( void ) const { return anParser::loaded; }
					// read a token from the source
	int				ReadToken( anToken *token );
					// expect a certain token, reads the token when available
	int				ExpectTokenString( const char *string );
					// expect a certain token type
	int				ExpectTokenType( int type, int subtype, anToken *token );
					// expect a token
	int				ExpectAnyToken( anToken *token );
					// returns true if the next token equals the given string and removes the token from the source
	int				CheckTokenString( const char *string );
					// returns true if the next token equals the given type and removes the token from the source
	int				CheckTokenType( int type, int subtype, anToken *token );
					// returns true if the next token equals the given string but does not remove the token from the source
	int				PeekTokenString( const char *string );
					// returns true if the next token equals the given type but does not remove the token from the source
	int				PeekTokenType( int type, int subtype, anToken *token );
					// skip tokens until the given token string is read
	int				SkipUntilString( const char *string );
					// skip the rest of the current line
	int				SkipRestOfLine( void );
					// skip the braced section
	int				SkipBracedSection( bool parseFirstBrace = true );
					// parse a braced section into a string
	const char *	ParseBracedSection( anString &out, int tabs = -1 );
					// parse a braced section into a string, maintaining indents and newlines
	const char *	ParseBracedSectionExact( anString &out, int tabs = -1 );
					// parse the rest of the line
	const char *	ParseRestOfLine( anString &out );
					// unread the given token
	void			UnreadToken( anToken *token );
					// read a token only if on the current line
	int				ReadTokenOnLine( anToken *token );
					// read a signed integer
	int				ParseInt( void );
					// read a boolean
	bool			ParseBool( void );
					// read a floating point number
	float			ParseFloat( void );
					// parse matrices with floats
	int				Parse1DMatrix( int x, float *m );
	int				Parse2DMatrix( int y, int x, float *m );
	int				Parse3DMatrix( int z, int y, int x, float *m );
					// get the white space before the last read token
	int				GetLastWhiteSpace( anString &whiteSpace ) const;
					// Set a marker in the source file (there is only one marker)
	void			SetMarker( void );
					// Get the string from the marker to the current position
	void			GetStringFromMarker( anString& out, bool clean = false );
					// add a define to the source
	int				AddDefine( const char *string );
					// add builtin defines
	void			AddBuiltinDefines( void );
					// set the source include path
	void			SetIncludePath( const char *path );
					// set the punctuation set
	void			SetPunctuations( const punctuation_t *p );
					// returns a pointer to the punctuation with the given id
	const char *	GetPunctuationFromId( int id );
					// get the id for the given punctuation
	int				GetPunctuationId( const char *p );
					// set lexer flags
	void			SetFlags( int flags );
					// get lexer flags
	int				GetFlags( void ) const;
					// returns the current filename
	const char *	GetFileName( void ) const;
					// get current offset in current script
	const int		GetFileOffset( void ) const;
					// get file time for current script
	const ARC_TIME_T	GetFileTime( void ) const;
					// returns the current line number
	const int		GetLineNum( void ) const;
					// print an error message
	void			Error( const char *str, ... ) const an_attribute((format(printf,2,3) ) );
					// print a warning message
	void			Warning( const char *str, ... ) const an_attribute((format(printf,2,3) ) );

					// add a global define that will be added to all opened sources
	static int		AddGlobalDefine( const char *string );
					// remove the given global define
	static int		RemoveGlobalDefine( const char *name );
					// remove all global defines
	static void		RemoveAllGlobalDefines( void );
					// set the base folder to load files from
	static void		SetBaseFolder( const char *path );

private:
	int				loaded;						// set when a source file is loaded from file or memory
	anString			filename;					// file name of the script
	anString			includepath;				// path to include files
	bool			OSPath;						// true if the file was loaded from an OS path
	const punctuation_t *punctuations;			// punctuations to use
	int				flags;						// flags used for script parsing
	anLexer *		scriptStack;				// stack with scripts of the source
	anToken *		tokens;						// tokens to read first
	define_t *		defines;					// list with macro definitions
	define_t **		defineHash;					// hash chain with defines
	indent_t *		indentStack;				// stack with indents
	int				skip;						// > 0 if skipping conditional code
	const char*		marker_p;

	static define_t *globalDefines;				// list with global defines added to every source loaded

private:
	void			PushIndent( int type, int skip );
	void			PopIndent( int *type, int *skip );
	void			PushScript( anLexer *script );
	int				ReadSourceToken( anToken *token );
	int				ReadLine( anToken *token );
	int				UnreadSourceToken( anToken *token );
	int				ReadDefineParms( define_t *define, anToken **parms, int maxParms );
	int				StringizeTokens( anToken *tokens, anToken *token );
	int				MergeTokens( anToken *t1, anToken *t2 );
	int				ExpandBuiltinDefine( anToken *defToken, define_t *define, anToken **firstToken, anToken **lastToken );
	int				ExpandDefine( anToken *defToken, define_t *define, anToken **firstToken, anToken **lastToken );
	int				ExpandDefineIntoSource( anToken *defToken, define_t *define );
	void			AddGlobalDefinesToSource( void );
	define_t *		CopyDefine( define_t *define );
	define_t *		FindHashedDefine(define_t **defineHash, const char *name);
	int				FindDefineParm( define_t *define, const char *name );
	void			AddDefineToHash(define_t *define, define_t **defineHash );
	static void		PrintDefine( define_t *define );
	static void		FreeDefine( define_t *define );
	static define_t *FindDefine( define_t *defines, const char *name );
	static define_t *DefineFromString( const char *string);
	define_t *		CopyFirstDefine( void );
	int				DirectiveInclude( void );
	int				DirectiveUNdef( void );
	int				DirectiveIFdef( int type );
	int				DirectiveIFDEF( void );
	int				DirectiveIFndef( void );
	int				DirectiveElse( void );
	int				DirectiveEndif ( void );
	int				EvaluateTokens( anToken *tokens, signed long int *intvalue, double *floatvalue, int integer );
	int				Evaluate( signed long int *intvalue, double *floatvalue, int integer );
	int				DollarEvaluate( signed long int *intvalue, double *floatvalue, int integer);
	int				DirectiveDefine( void );
	int				DirectiveElif ( void );
	int				DirectiveIF( void );
	int				DirectiveLine( void );
	int				DirectiveError( void );
	int				DirectiveWarning( void );
	int				DirectivePragma( void );
	void			UnreadSignToken( void );
	int				DirectiveEval( void );
	int				DirectiveEvalfloat( void );
	int				ReadDirective( void );
	int				DollarDirectiveEvalInt( void );
	int				DollarDirectiveEvalfloat( void );
	int				ReadDollarDirective( void );
};

ARC_INLINE const char *anParser::GetFileName( void ) const {
	if ( anParser::scriptStack ) {
		return anParser::scriptStack->GetFileName();
	} else {
		return "";
	}
}

ARC_INLINE const int anParser::GetFileOffset( void ) const {
	if ( anParser::scriptStack ) {
		return anParser::scriptStack->GetFileOffset();
	} else {
		return 0;
	}
}

ARC_INLINE const ARC_TIME_T anParser::GetFileTime( void ) const {
	if ( anParser::scriptStack ) {
		return anParser::scriptStack->GetFileTime();
	} else {
		return 0;
	}
}

ARC_INLINE const int anParser::GetLineNum( void ) const {
	if ( anParser::scriptStack ) {
		return anParser::scriptStack->GetLineNum();
	} else {
		return 0;
	}
}

#endif
