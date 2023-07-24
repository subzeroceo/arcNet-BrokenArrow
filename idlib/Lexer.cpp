#include "precompiled.h"
#pragma hdrstop

#define PUNCTABLE

//longer punctuations first
punctuation_t default_punctuations[] = {
	//binary operators
	{">>=",P_RSHIFT_ASSIGN},
	{"<<=",P_LSHIFT_ASSIGN},
	//
	{"...",P_PARMS},
	//define merge operator
	{"##",P_PRECOMPMERGE},				// pre-compiler
	//logic operators
	{"&&",P_LOGIC_AND},					// pre-compiler
	{"||",P_LOGIC_OR},					// pre-compiler
	{">=",P_LOGIC_GEQ},					// pre-compiler
	{"<=",P_LOGIC_LEQ},					// pre-compiler
	{"==",P_LOGIC_EQ},					// pre-compiler
	{"!=",P_LOGIC_UNEQ},				// pre-compiler
	//arithmatic operators
	{"*=",P_MUL_ASSIGN},
	{"/=",P_DIV_ASSIGN},
	{"%=",P_MOD_ASSIGN},
	{"+=",P_ADD_ASSIGN},
	{"-=",P_SUB_ASSIGN},
	{"++",P_INC},
	{"--",P_DEC},
	//binary operators
	{"&=",P_BIN_AND_ASSIGN},
	{"|=",P_BIN_OR_ASSIGN},
	{"^=",P_BIN_XOR_ASSIGN},
	{">>",P_RSHIFT},					// pre-compiler
	{"<<",P_LSHIFT},					// pre-compiler
	//reference operators
	{"->",P_POINTERREF},
	//C++
	{"::",P_CPP1},
	{".*",P_CPP2},
	//arithmatic operators
	{"*",P_MUL},						// pre-compiler
	{"/",P_DIV},						// pre-compiler
	{"%",P_MOD},						// pre-compiler
	{"+",P_ADD},						// pre-compiler
	{"-",P_SUB},						// pre-compiler
	{"=",P_ASSIGN},
	//binary operators
	{"&",P_BIN_AND},					// pre-compiler
	{"|",P_BIN_OR},						// pre-compiler
	{"^",P_BIN_XOR},					// pre-compiler
	{"~",P_BIN_NOT},					// pre-compiler
	//logic operators
	{"!",P_LOGIC_NOT},					// pre-compiler
	{">",P_LOGIC_GREATER},				// pre-compiler
	{"<",P_LOGIC_LESS},					// pre-compiler
	//reference operator
	{".",P_REF},
	//seperators
	{",",P_COMMA},						// pre-compiler
	{";",P_SEMICOLON},
	//label indication
	{":",P_COLON},						// pre-compiler
	//if statement
	{"?",P_QUESTIONMARK},				// pre-compiler
	//embracements
	{"( ",P_PARENTHESESOPEN},			// pre-compiler
	{" )",P_PARENTHESESCLOSE},			// pre-compiler
	{"{",P_BRACEOPEN},					// pre-compiler
	{"}",P_BRACECLOSE},					// pre-compiler
	{"[",P_SQBRACKETOPEN},
	{"]",P_SQBRACKETCLOSE},
	//
	{"\\",P_BACKSLASH},
	//precompiler operator
	{"#",P_PRECOMP},					// pre-compiler
	{"$",P_DOLLAR},
	{NULL, 0}
};

int default_punctuationtable[256];
int default_nextpunctuation[sizeof( default_punctuations ) / sizeof(punctuation_t)];
int default_setup;

char arcLexer::baseFolder[ 256 ];

/*
================
arcLexer::CreatePunctuationTable
================
*/
void arcLexer::CreatePunctuationTable( const punctuation_t *punctuations ) {
	int i, n, lastp;
	const punctuation_t *p, *newp;

	//get memory for the table
	if ( punctuations == default_punctuations ) {
		arcLexer::punctuationtable = default_punctuationtable;
		arcLexer::nextpunctuation = default_nextpunctuation;
		if ( default_setup ) {
			return;
		}
		default_setup = true;
		i = sizeof(default_punctuations) / sizeof(punctuation_t);
	} else {
		if ( !arcLexer::punctuationtable || arcLexer::punctuationtable == default_punctuationtable ) {
			arcLexer::punctuationtable = ( int * ) Mem_Alloc(256 * sizeof( int) );
		}
		if ( arcLexer::nextpunctuation && arcLexer::nextpunctuation != default_nextpunctuation ) {
			Mem_Free( arcLexer::nextpunctuation );
		}
		for ( i = 0; punctuations[i].p; i++ ) {
		}
		arcLexer::nextpunctuation = ( int * ) Mem_Alloc( i * sizeof( int) );
	}
	memset( arcLexer::punctuationtable, 0xFF, 256 * sizeof( int) );
	memset( arcLexer::nextpunctuation, 0xFF, i * sizeof( int) );
	//add the punctuations in the list to the punctuation table
	for ( i = 0; punctuations[i].p; i++ ) {
		newp = &punctuations[i];
		lastp = -1;
		//sort the punctuations in this table entry on length (longer punctuations first)
		for ( n = arcLexer::punctuationtable[(unsigned int) newp->p[0]]; n >= 0; n = arcLexer::nextpunctuation[n] ) {
			p = &punctuations[n];
			if ( strlen( p->p ) < strlen(newp->p ) ) {
				arcLexer::nextpunctuation[i] = n;
				if ( lastp >= 0 ) {
					arcLexer::nextpunctuation[lastp] = i;
				} else {
					arcLexer::punctuationtable[( unsigned int ) newp->p[0]] = i;
				}
				break;
			}
			lastp = n;
		}
		if ( n < 0 ) {
			arcLexer::nextpunctuation[i] = -1;
			if (lastp >= 0 ) {
				arcLexer::nextpunctuation[lastp] = i;
			}
			else {
				arcLexer::punctuationtable[( unsigned int ) newp->p[0]] = i;
			}
		}
	}
}

/*
================
arcLexer::GetPunctuationFromId
================
*/
const char *arcLexer::GetPunctuationFromId( int id ) {
	int i;

	for ( i = 0; arcLexer::punctuations[i].p; i++ ) {
		if ( arcLexer::punctuations[i].n == id ) {
			return arcLexer::punctuations[i].p;
		}
	}
	return "unkown punctuation";
}

/*
================
arcLexer::GetPunctuationId
================
*/
int arcLexer::GetPunctuationId( const char *p ) {
	int i;

	for ( i = 0; arcLexer::punctuations[i].p; i++ ) {
		if ( !strcmp( arcLexer::punctuations[i].p, p ) ) {
			return arcLexer::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
arcLexer::Error
================
*/
void arcLexer::Error( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	hadError = true;

	if ( arcLexer::flags & LEXFL_NOERRORS ) {
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );

	if ( arcLexer::flags & LEXFL_NOFATALERRORS ) {
		arcLibrary::common->Warning( "file %s, line %d: %s", arcLexer::filename.c_str(), arcLexer::line, text );
	} else {
		arcLibrary::common->Error( "file %s, line %d: %s", arcLexer::filename.c_str(), arcLexer::line, text );
	}
}

/*
================
arcLexer::Warning
================
*/
void arcLexer::Warning( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	if ( arcLexer::flags & LEXFL_NOWARNINGS ) {
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	arcLibrary::common->Warning( "file %s, line %d: %s", arcLexer::filename.c_str(), arcLexer::line, text );
}

/*
================
arcLexer::SetPunctuations
================
*/
void arcLexer::SetPunctuations( const punctuation_t *p ) {
#ifdef PUNCTABLE
	if ( p ) {
		arcLexer::CreatePunctuationTable( p );
	} else {
		arcLexer::CreatePunctuationTable( default_punctuations );
	}
#endif //PUNCTABLE
	if ( p ) {
		arcLexer::punctuations = p;
	} else {
		arcLexer::punctuations = default_punctuations;
	}
}

/*
================
arcLexer::ReadWhiteSpace

Reads spaces, tabs, C-like comments etc.
When a newline character is found the scripts line counter is increased.
================
*/
int arcLexer::ReadWhiteSpace( void ) {
	while ( 1 ) {
		// skip white space
		while ( *arcLexer::script_p <= ' ' ) {
			if ( !*arcLexer::script_p ) {
				return 0;
			}
			if ( *arcLexer::script_p == '\n' ) {
				arcLexer::line++;
			}
			arcLexer::script_p++;
		}
		// skip comments
		if ( *arcLexer::script_p == '/' ) {
			// comments //
			if ( *( arcLexer::script_p+1 ) == '/' ) {
				arcLexer::script_p++;
				do {
					arcLexer::script_p++;
					if ( !*arcLexer::script_p ) {
						return 0;
					}
				}
				while ( *arcLexer::script_p != '\n' );
				arcLexer::line++;
				arcLexer::script_p++;
				if ( !*arcLexer::script_p ) {
					return 0;
				}
				continue;
			} else if ( *( arcLexer::script_p+1 ) == '*' ) {
				arcLexer::script_p++;
				while ( 1 ) {
					arcLexer::script_p++;
					if ( !*arcLexer::script_p ) {
						return 0;
					}
					if ( *arcLexer::script_p == '\n' ) {
						arcLexer::line++;
					} else if ( *arcLexer::script_p == '/' ) {
						if ( *( arcLexer::script_p-1 ) == '*' ) {
							break;
						}
						if ( *( arcLexer::script_p+1 ) == '*' ) {
							arcLexer::Warning( "nested comment" );
						}
					}
				}
				arcLexer::script_p++;
				if ( !*arcLexer::script_p ) {
					return 0;
				}
				arcLexer::script_p++;
				if ( !*arcLexer::script_p ) {
					return 0;
				}
				continue;
			}
		}
		break;
	}
	return 1;
}

/*
================
arcLexer::ReadEscapeCharacter
================
*/
int arcLexer::ReadEscapeCharacter( char *ch ) {
	int c, val, i;

	// step over the leading '\\'
	arcLexer::script_p++;
	// determine the escape character
	switch( *arcLexer::script_p ) {
		case '\\': c = '\\'; break;
		case 'n': c = '\n'; break;
		case 'r': c = '\r'; break;
		case 't': c = '\t'; break;
		case 'v': c = '\v'; break;
		case 'b': c = '\b'; break;
		case 'f': c = '\f'; break;
		case 'a': c = '\a'; break;
		case '\'': c = '\''; break;
		case '\"': c = '\"'; break;
		case '\?': c = '\?'; break;
		case 'x': {
			arcLexer::script_p++;
			for ( i = 0, val = 0;; i++, arcLexer::script_p++ ) {
				c = *arcLexer::script_p;
				if (c >= '0' && c <= '9' ) {
					c = c - '0';
				} else if (c >= 'A' && c <= 'Z' ) {
					c = c - 'A' + 10;
				} else if (c >= 'a' && c <= 'z' ) {
					c = c - 'a' + 10;
				} else {
					break;
			}
				val = (val << 4) + c;
			}
			arcLexer::script_p--;
			if (val > 0xFF) {
				arcLexer::Warning( "too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
		default: { //NOTE: decimal ASCII code, NOT octal
			if ( *arcLexer::script_p < '0' || *arcLexer::script_p > '9' ) {
				arcLexer::Error( "unknown escape char" );
			}
			for ( i = 0, val = 0;; i++, arcLexer::script_p++ ) {
				c = *arcLexer::script_p;
				if (c >= '0' && c <= '9' )
					c = c - '0';
				else
					break;
				val = val * 10 + c;
			}
			arcLexer::script_p--;
			if (val > 0xFF) {
				arcLexer::Warning( "[WARNING]too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
	}
	// step over the escape character or the last digit of the number
	arcLexer::script_p++;
	// store the escape character
	*ch = c;
	// succesfully read escape character
	return 1;
}

/*
================
arcLexer::ReadString

Escape characters are interpretted.
Reads two strings with only a white space between them as one string.
================
*/
int arcLexer::ReadString( arcNetToken *token, int quote ) {
	int tmpline;
	const char *tmpscript_p;
	char ch;

	if ( quote == '\"' ) {
		token->type = TT_STRING;
	} else {
		token->type = TT_LITERAL;
	}

	// leading quote
	arcLexer::script_p++;

	while ( 1 ) {
		// if there is an escape character and escape characters are allowed
		if ( *arcLexer::script_p == '\\' && !( arcLexer::flags & LEXFL_NOSTRINGESCAPECHARS) ) {
			if ( !arcLexer::ReadEscapeCharacter( &ch ) ) {
				return 0;
			}
			token->AppendDirty( ch );
		// if a trailing quote
		} else if ( *arcLexer::script_p == quote) {
			// step over the quote
			arcLexer::script_p++;
			// if consecutive strings should not be concatenated
			if ( ( arcLexer::flags & LEXFL_NOSTRINGCONCAT) &&
					( !( arcLexer::flags & LEXFL_ALLOWBACKSLASHSTRINGCONCAT) || (quote != '\"' ) ) ) {
				break;
			}

			tmpscript_p = arcLexer::script_p;
			tmpline = arcLexer::line;
			// read white space between possible two consecutive strings
			if ( !arcLexer::ReadWhiteSpace() ) {
				arcLexer::script_p = tmpscript_p;
				arcLexer::line = tmpline;
				break;
			}

			if ( arcLexer::flags & LEXFL_NOSTRINGCONCAT ) {
				if ( *arcLexer::script_p != '\\' ) {
					arcLexer::script_p = tmpscript_p;
					arcLexer::line = tmpline;
					break;
				}
				// step over the '\\'
				arcLexer::script_p++;
				if ( !arcLexer::ReadWhiteSpace() || ( *arcLexer::script_p != quote ) ) {
					arcLexer::Error( "expecting string after '\' terminated line" );
					return 0;
				}
			}

			// if there's no leading qoute
			if ( *arcLexer::script_p != quote ) {
				arcLexer::script_p = tmpscript_p;
				arcLexer::line = tmpline;
				break;
			}
			// step over the new leading quote
			arcLexer::script_p++;
		} else {
			if ( *arcLexer::script_p == '\0' ) {
				arcLexer::Error( "missing trailing quote" );
				return 0;
			}
			if ( *arcLexer::script_p == '\n' ) {
				arcLexer::Error( "newline inside string" );
				return 0;
			}
			token->AppendDirty( *arcLexer::script_p++ );
		}
	}
	token->data[token->len] = '\0';

	if ( token->type == TT_LITERAL ) {
		if ( !( arcLexer::flags & LEXFL_ALLOWMULTICHARLITERALS) ) {
			if ( token->Length() != 1 ) {
				arcLexer::Warning( "[WARNING ]literal is not one character long" );
			}
		}
		token->subtype = ( *token)[0];
	} else {
		// the sub type is the length of the string
		token->subtype = token->Length();
	}
	return 1;
}

/*
================
arcLexer::ReadName
================
*/
int arcLexer::ReadName( arcNetToken *token ) {
	char c;

	token->type = TT_NAME;
	do {
		token->AppendDirty( *arcLexer::script_p++ );
		c = *arcLexer::script_p;
	} while ( ( c >= 'a' && c <= 'z' ) ||
				( c >= 'A' && c <= 'Z' ) ||
				( c >= '0' && c <= '9' ) ||
				c == '_' ||
				// if treating all tokens as strings, don't parse '-' as a seperate token
				( ( arcLexer::flags & LEXFL_ONLYSTRINGS) && (c == '-' ) ) ||
				// if special path name characters are allowed
				( ( arcLexer::flags & LEXFL_ALLOWPATHNAMES) && (c == '/' || c == '\\' || c == ':' || c == '.' ) ) );
	token->data[token->len] = '\0';
	//the sub type is the length of the name
	token->subtype = token->Length();
	return 1;
}

/*
================
arcLexer::CheckString
================
*/
ARC_INLINE int arcLexer::CheckString( const char *str ) const {
	for ( int i = 0; str[i]; i++ ) {
		if ( arcLexer::script_p[i] != str[i] ) {
			return false;
		}
	}
	return true;
}

/*
================
arcLexer::ReadNumber
================
*/
int arcLexer::ReadNumber( arcNetToken *token ) {
	int i;
	int dot;
	char c, c2;

	token->type = TT_NUMBER;
	token->subtype = 0;
	token->intvalue = 0;
	token->floatvalue = 0;

	c = *arcLexer::script_p;
	c2 = *( arcLexer::script_p + 1 );

	if ( c == '0' && c2 != '.' ) {
		// check for a hexadecimal number
		if ( c2 == 'x' || c2 == 'X' ) {
			token->AppendDirty( *arcLexer::script_p++ );
			token->AppendDirty( *arcLexer::script_p++ );
			c = *arcLexer::script_p;
			while ((c >= '0' && c <= '9' ) || (c >= 'a' && c <= 'f' ) || (c >= 'A' && c <= 'F' ) ) {
				token->AppendDirty( c );
				c = *( ++arcLexer::script_p );
			}
			token->subtype = TT_HEX | TT_INTEGER;
		// check for a binary number
		} else if ( c2 == 'b' || c2 == 'B' ) {
			token->AppendDirty( *arcLexer::script_p++ );
			token->AppendDirty( *arcLexer::script_p++ );
			c = *arcLexer::script_p;
			while ( c == '0' || c == '1' ) {
				token->AppendDirty( c );
				c = *( ++arcLexer::script_p );
			}
			token->subtype = TT_BINARY | TT_INTEGER;
		// its an octal number
		} else {
			token->AppendDirty( *arcLexer::script_p++ );
			c = *arcLexer::script_p;
			while ( c >= '0' && c <= '7' ) {
				token->AppendDirty( c );
				c = *( ++arcLexer::script_p );
			}
			token->subtype = TT_OCTAL | TT_INTEGER;
		}
	} else {
		// decimal integer or floating point number or ip address
		dot = 0;
		while ( 1 ) {
			if ( c >= '0' && c <= '9' ) {
			} else if ( c == '.' ) {
				dot++;
			} else {
				break;
			}
			token->AppendDirty( c );
			c = *( ++arcLexer::script_p );
		}
		if ( c == 'e' && dot == 0 ) {
			//We have scientific notation without a decimal point
			dot++;
		}
		// if a floating point number
		if ( dot == 1 ) {
			token->subtype = TT_DECIMAL | TT_FLOAT;
			// check for floating point exponent
			if ( c == 'e' ) {
				//Append the e so that GetFloatValue code works
				token->AppendDirty( c );
				c = *( ++arcLexer::script_p );
				if ( c == '-' ) {
					token->AppendDirty( c );
					c = *( ++arcLexer::script_p );
				} else if ( c == '+' ) {
					token->AppendDirty( c );
					c = *( ++arcLexer::script_p );
				}
				while ( c >= '0' && c <= '9' ) {
					token->AppendDirty( c );
					c = *( ++arcLexer::script_p );
				}
			// chck for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
			} else if ( c == '#' ) {
				c2 = 4;
				if ( CheckString( "INF" ) ) {
					token->subtype |= TT_INFINITE;
				} else if ( CheckString( "IND" ) ) {
					token->subtype |= TT_INDEFINITE;
				} else if ( CheckString( "NAN" ) ) {
					token->subtype |= TT_NAN;
				} else if ( CheckString( "QNAN" ) ) {
					token->subtype |= TT_NAN;
					c2++;
				} else if ( CheckString( "SNAN" ) ) {
					token->subtype |= TT_NAN;
					c2++;
				}
				for ( i = 0; i < c2; i++ ) {
					token->AppendDirty( c );
					c = *( ++arcLexer::script_p );
				}
				while ( c >= '0' && c <= '9' ) {
					token->AppendDirty( c );
					c = *( ++arcLexer::script_p );
				}
				if ( !( arcLexer::flags & LEXFL_ALLOWFLOATEXCEPTIONS) ) {
					token->AppendDirty( 0 );	// zero terminate for c_str
					arcLexer::Error( "parsed %s", token->c_str() );
				}
			}
		} else if ( dot > 1 ) {
			if ( !( arcLexer::flags & LEXFL_ALLOWIPADDRESSES ) ) {
				arcLexer::Error( "[ERROR] more than one period (dot) in number" );
				return 0;
			}
			if ( dot != 3 ) {
				arcLexer::Error( "[ERROR] ip address should have three periods (dots)" );
				return 0;
			}
			token->subtype = TT_IPADDRESS;
		} else {
			token->subtype = TT_DECIMAL | TT_INTEGER;
		}
	}

	if ( token->subtype & TT_FLOAT ) {
		if ( c > ' ' ) {
			// single-precision: float
			if ( c == 'f' || c == 'F' ) {
				token->subtype |= TT_SINGLE_PRECISION;
				arcLexer::script_p++;
			// extended-precision: long double
			} else if ( c == 'l' || c == 'L' ) {
				token->subtype |= TT_EXTENDED_PRECISION;
				arcLexer::script_p++;
			// default is double-precision: double
			} else {
				token->subtype |= TT_DOUBLE_PRECISION;
			}
		} else {
			token->subtype |= TT_DOUBLE_PRECISION;
		}
	} else if ( token->subtype & TT_INTEGER ) {
		if ( c > ' ' ) {
			// default: signed long
			for ( i = 0; i < 2; i++ ) {
				// long integer
				if ( c == 'l' || c == 'L' ) {
					token->subtype |= TT_LONG;
				} else if ( c == 'u' || c == 'U' ) {
					token->subtype |= TT_UNSIGNED;
				} else {
					break;
				}
				c = *( ++arcLexer::script_p );
			}
		}
	} else if ( token->subtype & TT_IPADDRESS ) {
		if ( c == ':' ) {
			token->AppendDirty( c );
			c = *( ++arcLexer::script_p );
			while ( c >= '0' && c <= '9' ) {
				token->AppendDirty( c );
				c = *( ++arcLexer::script_p );
			}
			token->subtype |= TT_IPPORT;
		}
	}
	token->data[token->len] = '\0';
	return 1;
}

/*
================
arcLexer::ReadPunctuation
================
*/
int arcLexer::ReadPunctuation( arcNetToken *token ) {
	int l, n, i;
	char *p;
	const punctuation_t *punc;

#ifdef PUNCTABLE
	for (n = arcLexer::punctuationtable[(unsigned int)*( arcLexer::script_p )]; n >= 0; n = arcLexer::nextpunctuation[n] )
	{
		punc = &( arcLexer::punctuations[n] );
#else
	int i;

	for ( i = 0; arcLexer::punctuations[i].p; i++ ) {
		punc = &arcLexer::punctuations[i];
#endif
		p = punc->p;
		// check for this punctuation in the script
		for ( l = 0; p[l] && arcLexer::script_p[l]; l++ ) {
			if ( arcLexer::script_p[l] != p[l] ) {
				break;
			}
		}
		if ( !p[l] ) {
			//
			token->EnsureAlloced( l+1, false );
			for ( i = 0; i <= l; i++ ) {
				token->data[i] = p[i];
			}
			token->len = l;
			//
			arcLexer::script_p += l;
			token->type = TT_PUNCTUATION;
			// sub type is the punctuation id
			token->subtype = punc->n;
			return 1;
		}
	}
	return 0;
}

/*
================
arcLexer::ReadToken
================
*/
int arcLexer::ReadToken( arcNetToken *token ) {
	int c;

	if ( !loaded ) {
		arcLibrary::common->Error( "Lexer::ReadToken: no file loaded" );
		return 0;
	}

	// if there is a token available (from unreadToken)
	if ( tokenavailable ) {
		tokenavailable = 0;
		*token = arcLexer::token;
		return 1;
	}
	// save script pointer
	lastScript_p = script_p;
	// save line counter
	lastline = line;
	// clear the token stuff
	token->data[0] = '\0';
	token->len = 0;
	// start of the white space
	whiteSpaceStart_p = script_p;
	token->whiteSpaceStart_p = script_p;
	// read white space before token
	if ( !ReadWhiteSpace() ) {
		return 0;
	}
	// end of the white space
	arcLexer::whiteSpaceEnd_p = script_p;
	token->whiteSpaceEnd_p = script_p;
	// line the token is on
	token->line = line;
	// number of lines crossed before token
	token->linesCrossed = line - lastline;
	// clear token flags
	token->flags = 0;

	c = *arcLexer::script_p;

	// if we're keeping everything as whitespace deliminated strings
	if ( arcLexer::flags & LEXFL_ONLYSTRINGS ) {
		// if there is a leading quote
		if ( c == '\"' || c == '\'' ) {
			if ( !arcLexer::ReadString( token, c ) ) {
				return 0;
			}
		} else if ( !arcLexer::ReadName( token ) ) {
			return 0;
		}
	}
	// if there is a number
	else if ( (c >= '0' && c <= '9' ) ||
			(c == '.' && ( *( arcLexer::script_p + 1 ) >= '0' && *( arcLexer::script_p + 1 ) <= '9' ) ) ) {
		if ( !arcLexer::ReadNumber( token ) ) {
			return 0;
		}
		// if names are allowed to start with a number
		if ( arcLexer::flags & LEXFL_ALLOWNUMBERNAMES ) {
			c = *arcLexer::script_p;
			if ( (c >= 'a' && c <= 'z' ) ||	(c >= 'A' && c <= 'Z' ) || c == '_' ) {
				if ( !arcLexer::ReadName( token ) ) {
					return 0;
				}
			}
		}
	// if there is a leading quote
	} else if ( c == '\"' || c == '\'' ) {
		if ( !arcLexer::ReadString( token, c ) ) {
			return 0;
		}
		// if there is a name
	} else if ( (c >= 'a' && c <= 'z' ) ||	(c >= 'A' && c <= 'Z' ) || c == '_' ) {
		if ( !arcLexer::ReadName( token ) ) {
			return 0;
		}
	// names may also start with a slash when pathnames are allowed
	} else if ( ( arcLexer::flags & LEXFL_ALLOWPATHNAMES ) && ( (c == '/' || c == '\\' ) || c == '.' ) ) {
		if ( !arcLexer::ReadName( token ) ) {
			return 0;
		}
	// check for punctuations
	} else if ( !arcLexer::ReadPunctuation( token ) ) {
		arcLexer::Error( "[WARNING] unknown punctuation %c", c );
		return 0;
	}
	// succesfully read a token
	return 1;
}

/*
================
arcLexer::ExpectTokenString
================
*/
int arcLexer::ExpectTokenString( const char *string ) {
	arcNetToken token;

	if ( !arcLexer::ReadToken( &token ) ) {
		arcLexer::Error( "[ERROR] couldn't find expected '%s'", string );
		return 0;
	}
	if ( token != string ) {
		arcLexer::Error( "[WARNING] expected '%s' but found '%s'", string, token.c_str() );
		return 0;
	}
	return 1;
}

/*
================
arcLexer::ExpectTokenType
================
*/
int arcLexer::ExpectTokenType( int type, int subtype, arcNetToken *token ) {
	arcNetString str;

	if ( !arcLexer::ReadToken( token ) ) {
		arcLexer::Error( "[ERROR] couldn't read expected token" );
		return 0;
	}

	if ( token->type != type ) {
		switch( type ) {
			case TT_STRING: str = "string"; break;
			case TT_LITERAL: str = "literal"; break;
			case TT_NUMBER: str = "number"; break;
			case TT_NAME: str = "name"; break;
			case TT_PUNCTUATION: str = "punctuation"; break;
			default: str = "unknown type"; break;
		}
		arcLexer::Error( "[WARNING] expected a %s but found '%s'", str.c_str(), token->c_str() );
		return 0;
	}
	if ( token->type == TT_NUMBER ) {
		if ( (token->subtype & subtype) != subtype ) {
			str.Clear();
			if ( subtype & TT_DECIMAL ) str = "decimal ";
			if ( subtype & TT_HEX ) str = "hex ";
			if ( subtype & TT_OCTAL ) str = "octal ";
			if ( subtype & TT_BINARY ) str = "binary ";
			if ( subtype & TT_UNSIGNED ) str += "unsigned ";
			if ( subtype & TT_LONG ) str += "long ";
			if ( subtype & TT_FLOAT ) str += "float ";
			if ( subtype & TT_INTEGER ) str += "integer ";
			str.StripTrailing( ' ' );
			arcLexer::Error( "[WARNING] expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if ( token->type == TT_PUNCTUATION ) {
		if ( subtype < 0 ) {
			arcLexer::Error( "[ERROR] wrong punctuation subtype" );
			return 0;
		}
		if ( token->subtype != subtype ) {
			arcLexer::Error( "[WARNING] expected '%s' but found '%s'", GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
arcLexer::ExpectAnyToken
================
*/
int arcLexer::ExpectAnyToken( arcNetToken *token ) {
	if ( !arcLexer::ReadToken( token ) ) {
		arcLexer::Error( "[ERROR] couldn't read expected token" );
		return 0;
	} else {
		return 1;
	}
}

/*
================
arcLexer::CheckTokenString
================
*/
int arcLexer::CheckTokenString( const char *string ) {
	arcNetToken tok;

	if ( !ReadToken( &tok ) ) {
		return 0;
	}
	// if the given string is available
	if ( tok == string ) {
		return 1;
	}
	// unread token
	script_p = lastScript_p;
	line = lastline;
	return 0;
}

/*
================
arcLexer::CheckTokenType
================
*/
int arcLexer::CheckTokenType( int type, int subtype, arcNetToken *token ) {
	arcNetToken tok;

	if ( !ReadToken( &tok ) ) {
		return 0;
	}
	// if the type matches
	if (tok.type == type && (tok.subtype & subtype) == subtype) {
		*token = tok;
		return 1;
	}
	// unread token
	script_p = lastScript_p;
	line = lastline;
	return 0;
}

/*
================
arcLexer::PeekTokenString
================
*/
int arcLexer::PeekTokenString( const char *string ) {
	arcNetToken tok;

	if ( !ReadToken( &tok ) ) {
		return 0;
	}

	// unread token
	script_p = lastScript_p;
	line = lastline;

	// if the given string is available
	if ( tok == string ) {
		return 1;
	}
	return 0;
}

/*
================
arcLexer::PeekTokenType
================
*/
int arcLexer::PeekTokenType( int type, int subtype, arcNetToken *token ) {
	arcNetToken tok;

	if ( !ReadToken( &tok ) ) {
		return 0;
	}

	// unread token
	script_p = lastScript_p;
	line = lastline;

	// if the type matches
	if ( tok.type == type && ( tok.subtype & subtype ) == subtype ) {
		*token = tok;
		return 1;
	}
	return 0;
}

/*
================
arcLexer::SkipUntilString
================
*/
int arcLexer::SkipUntilString( const char *string ) {
	arcNetToken token;

	while ( arcLexer::ReadToken( &token ) ) {
		if ( token == string ) {
			return 1;
		}
	}
	return 0;
}

/*
================
arcLexer::SkipRestOfLine
================
*/
int arcLexer::SkipRestOfLine( void ) {
	arcNetToken token;

	while ( arcLexer::ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			arcLexer::script_p = lastScript_p;
			arcLexer::line = lastline;
			return 1;
		}
	}
	return 0;
}

/*
=================
arcLexer::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int arcLexer::SkipBracedSection( bool parseFirstBrace ) {
	arcNetToken token;
	int depth;

	depth = parseFirstBrace ? 0 : 1;
	do {
		if ( !ReadToken( &token ) ) {
			return false;
		}
		if ( token.type == TT_PUNCTUATION ) {
			if ( token == "{" ) {
				depth++;
			} else if ( token == "}" ) {
				depth--;
			}
		}
	} while ( depth );
	return true;
}

/*
================
arcLexer::UnreadToken
================
*/
void arcLexer::UnreadToken( const arcNetToken *token ) {
	if ( arcLexer::tokenavailable ) {
		arcLibrary::common->FatalError( "Lexer::UnreadToken, unread token twice\n" );
	}
	arcLexer::token = *token;
	arcLexer::tokenavailable = 1;
}

/*
================
arcLexer::ReadTokenOnLine
================
*/
int arcLexer::ReadTokenOnLine( arcNetToken *token ) {
	arcNetToken tok;

	if ( !arcLexer::ReadToken( &tok ) ) {
		arcLexer::script_p = lastScript_p;
		arcLexer::line = lastline;
		return false;
	}
	// if no lines were crossed before this token
	if ( !tok.linesCrossed ) {
		*token = tok;
		return true;
	}
	// restore our position
	arcLexer::script_p = lastScript_p;
	arcLexer::line = lastline;
	token->Clear();
	return false;
}

/*
================
arcLexer::ReadRestOfLine
================
*/
const char*	arcLexer::ReadRestOfLine( arcNetString& out) {
	while ( 1 ) {
		if ( *arcLexer::script_p == '\n' ) {
			arcLexer::line++;
			break;
		}

		if ( !*arcLexer::script_p ) {
			break;
		}

		if ( *arcLexer::script_p <= ' ' ) {
			out += " ";
		} else {
			out += *arcLexer::script_p;
		}
		arcLexer::script_p++;

	}

	out.Strip( ' ' );
	return out.c_str();
}

/*
================
arcLexer::ParseInt
================
*/
int arcLexer::ParseInt( void ) {
	arcNetToken token;

	if ( !arcLexer::ReadToken( &token ) ) {
		arcLexer::Error( "[ERROR] Failed to read expected integer" );
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		arcLexer::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -( (signed int) token.GetIntValue() );
	} else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		arcLexer::Error( "[WARNING] expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
arcLexer::ParseBool
================
*/
bool arcLexer::ParseBool( void ) {
	arcNetToken token;
	if ( !arcLexer::ExpectTokenType( TT_NUMBER, 0, &token ) ) {
		arcLexer::Error( "[ERROR] Failed to read expected boolean value." );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
arcLexer::ParseFloat
================
*/
float arcLexer::ParseFloat( bool *errorFlag ) {
	arcNetToken token;

	if ( errorFlag ) {
		*errorFlag = false;
	}

	if ( !arcLexer::ReadToken( &token ) ) {
		if ( errorFlag ) {
			arcLexer::Warning( "[ERROR] Failed to read expected floating point number" );
			*errorFlag = true;
		} else {
			arcLexer::Error( "[ERROR] Failed to read expected floating point number" );
		}
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		arcLexer::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	} else if ( token.type != TT_NUMBER ) {
		if ( errorFlag ) {
			arcLexer::Warning( "[WARNING] expected float value, found '%s'", token.c_str() );
			*errorFlag = true;
		} else {
			arcLexer::Error( "[WARNING] expected float value, found '%s'", token.c_str() );
		}
	}
	return token.GetFloatValue();
}

/*
================
arcLexer::Parse1DMatrix
================
*/
int arcLexer::Parse1DMatrix( int x, float *m ) {
	if ( !arcLexer::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( int i = 0; i < x; i++ ) {
		m[i] = arcLexer::ParseFloat();
	}

	if ( !arcLexer::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
arcLexer::Parse2DMatrix
================
*/
int arcLexer::Parse2DMatrix( int y, int x, float *m ) {
	if ( !arcLexer::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( int i = 0; i < y; i++ ) {
		if ( !arcLexer::Parse1DMatrix( x, m + i * x ) ) {
			return false;
		}
	}

	if ( !arcLexer::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
arcLexer::Parse3DMatrix
================
*/
int arcLexer::Parse3DMatrix( int z, int y, int x, float *m ) {
	if ( !arcLexer::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( int i = 0; i < z; i++ ) {
		if ( !arcLexer::Parse2DMatrix( y, x, m + i * x*y ) ) {
			return false;
		}
	}

	if ( !arcLexer::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
=================
ARCParser::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains exact characters between braces.

  FIXME: this should use ReadToken and replace the token white space with correct indents and newlines
=================
*/
const char *arcLexer::ParseBracedSectionExact( arcNetString &out, int tabs ) {
	int		depth;
	bool	doTabs;
	bool	skipWhite;

	out.Empty();

	if ( !arcLexer::ExpectTokenString( "{" ) ) {
		return out.c_str( );
	}

	out = "{";
	depth = 1;
	skipWhite = false;
	doTabs = tabs >= 0;

	while ( depth && *arcLexer::script_p ) {
		char c = *( arcLexer::script_p++ );
		switch ( c ) {
			case '\t':
			case ' ': {
				if ( skipWhite ) {
					continue;
				}
				break;
			} case '\n': {
				if ( doTabs ) {
					skipWhite = true;
					out += c;
					continue;
				}
				break;
			} case '{': {
				depth++;
				tabs++;
				break;
			} case '}': {
				depth--;
				tabs--;
				break;
			}
		}

		if ( skipWhite ) {
			int i = tabs;
			if ( c == '{' ) {
				i--;
			}
			skipWhite = false;
			for (; i > 0; i-- ) {
				out += '\t';
			}
		}
		out += c;
	}
	return out.c_str();
}

/*
=================
arcLexer::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
const char *arcLexer::ParseBracedSection( arcNetString &out ) {
	arcNetToken token;
	int depth;

	out.Empty();
	if ( !arcLexer::ExpectTokenString( "{" ) ) {
		return out.c_str();
	}
	out = "{";
	depth = 1;
	do {
		if ( !arcLexer::ReadToken( &token ) ) {
			Error( "[WARNING] missing closing brace" );
			return out.c_str();
		}

		// if the token is on a new line
		for ( int i = 0; i < token.linesCrossed; i++ ) {
			out += "\r\n";
		}

		if ( token.type == TT_PUNCTUATION ) {
			if ( token[0] == '{' ) {
				depth++;
			} else if ( token[0] == '}' ) {
				depth--;
			}
		}

		if ( token.type == TT_STRING ) {
			out += "\"" + token + "\"";
		} else {
			out += token;
		}
		out += " ";
	} while ( depth );

	return out.c_str();
}

/*
=================
arcLexer::ParseRestOfLine

  parse the rest of the line
=================
*/
const char *arcLexer::ParseRestOfLine( arcNetString &out ) {
	arcNetToken token;

	out.Empty();
	while ( arcLexer::ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			arcLexer::script_p = lastScript_p;
			arcLexer::line = lastline;
			break;
		}
		if ( out.Length() ) {
			out += " ";
		}
		out += token;
	}
	return out.c_str();
}

/*
================
arcLexer::GetLastWhiteSpace
================
*/
int arcLexer::GetLastWhiteSpace( arcNetString &whiteSpace ) const {
	whiteSpace.Clear();
	for ( const char *p = whiteSpaceStart_p; p < whiteSpaceEnd_p; p++ ) {
		whiteSpace.Append( *p );
	}
	return whiteSpace.Length();
}

/*
================
arcLexer::GetLastWhiteSpaceStart
================
*/
int arcLexer::GetLastWhiteSpaceStart( void ) const {
	return whiteSpaceStart_p - buffer;
}

/*
================
arcLexer::GetLastWhiteSpaceEnd
================
*/
int arcLexer::GetLastWhiteSpaceEnd( void ) const {
	return whiteSpaceEnd_p - buffer;
}

/*
================
arcLexer::Reset
================
*/
void arcLexer::Reset( void ) {
	// pointer in script buffer
	arcLexer::script_p = arcLexer::buffer;
	// pointer in script buffer before reading token
	arcLexer::lastScript_p = arcLexer::buffer;
	// begin of white space
	arcLexer::whiteSpaceStart_p = NULL;
	// end of white space
	arcLexer::whiteSpaceEnd_p = NULL;
	// set if there's a token available in arcLexer::token
	arcLexer::tokenavailable = 0;

	arcLexer::line = 1;
	arcLexer::lastline = 1;
	// clear the saved token
	arcLexer::token = "";
}

/*
================
arcLexer::EndOfFile
================
*/
int arcLexer::EndOfFile( void ) {
	return arcLexer::script_p >= arcLexer::end_p;
}

/*
================
arcLexer::NumLinesCrossed
================
*/
int arcLexer::NumLinesCrossed( void ) {
	return arcLexer::line - arcLexer::lastline;
}

/*
================
arcLexer::LoadFile
================
*/
int arcLexer::LoadFile( const char *filename, bool OSPath ) {
	arcNetFile *fp;
	arcNetString pathname;
	int length;
	char *buf;

	if ( arcLexer::loaded ) {
		arcLibrary::common->Error( "[WARNING] Lexer::LoadFile: another script already loaded" );
		return false;
	}

	if ( !OSPath && ( baseFolder[0] != '\0' ) ) {
		pathname = va( "%s/%s", baseFolder, filename );
	} else {
		pathname = filename;
	}
	if ( OSPath ) {
		fp = arcLibrary::fileSystem->OpenExplicitFileRead( pathname );
	} else {
		fp = arcLibrary::fileSystem->OpenFileRead( pathname );
	}
	if ( !fp ) {
		return false;
	}
	length = fp->Length();
	buf = (char *) Mem_Alloc( length + 1 );
	buf[length] = '\0';
	fp->Read( buf, length );
	arcLexer::fileTime = fp->Timestamp();
	arcLexer::filename = fp->GetFullPath();
	arcLibrary::fileSystem->CloseFile( fp );

	arcLexer::buffer = buf;
	arcLexer::length = length;
	// pointer in script buffer
	arcLexer::script_p = arcLexer::buffer;
	// pointer in script buffer before reading token
	arcLexer::lastScript_p = arcLexer::buffer;
	// pointer to end of script buffer
	arcLexer::end_p = &( arcLexer::buffer[length] );

	arcLexer::tokenavailable = 0;
	arcLexer::line = 1;
	arcLexer::lastline = 1;
	arcLexer::allocated = true;
	arcLexer::loaded = true;

	return true;
}

/*
================
arcLexer::LoadMemory
================
*/
int arcLexer::LoadMemory( const char *ptr, int length, const char *name, int startLine ) {
	if ( arcLexer::loaded ) {
		arcLibrary::common->Error( "[WARNING] LoadMemory another script already loaded" );
		return false;
	}
	arcLexer::filename = name;
	arcLexer::buffer = ptr;
	arcLexer::fileTime = 0;
	arcLexer::length = length;
	// pointer in script buffer
	arcLexer::script_p = arcLexer::buffer;
	// pointer in script buffer before reading token
	arcLexer::lastScript_p = arcLexer::buffer;
	// pointer to end of script buffer
	arcLexer::end_p = &( arcLexer::buffer[length] );

	arcLexer::tokenavailable = 0;
	arcLexer::line = startLine;
	arcLexer::lastline = startLine;
	arcLexer::allocated = false;
	arcLexer::loaded = true;

	return true;
}

/*
================
arcLexer::FreeSource
================
*/
void arcLexer::FreeSource( void ) {
#ifdef PUNCTABLE
	if ( arcLexer::punctuationtable && arcLexer::punctuationtable != default_punctuationtable ) {
		Mem_Free( (void *) arcLexer::punctuationtable );
		arcLexer::punctuationtable = NULL;
	}
	if ( arcLexer::nextpunctuation && arcLexer::nextpunctuation != default_nextpunctuation ) {
		Mem_Free( (void *) arcLexer::nextpunctuation );
		arcLexer::nextpunctuation = NULL;
	}
#endif //PUNCTABLE
	if ( arcLexer::allocated ) {
		Mem_Free( (void *) arcLexer::buffer );
		arcLexer::buffer = NULL;
		arcLexer::allocated = false;
	}
	arcLexer::tokenavailable = 0;
	arcLexer::token = "";
	arcLexer::loaded = false;
}

/*
================
arcLexer::arcLexer
================
*/
arcLexer::arcLexer( void ) {
	arcLexer::loaded = false;
	arcLexer::filename = "";
	arcLexer::flags = 0;
	arcLexer::SetPunctuations( NULL );
	arcLexer::allocated = false;
	arcLexer::fileTime = 0;
	arcLexer::length = 0;
	arcLexer::line = 0;
	arcLexer::lastline = 0;
	arcLexer::tokenavailable = 0;
	arcLexer::token = "";
	arcLexer::next = NULL;
	arcLexer::hadError = false;
}

/*
================
arcLexer::arcLexer
================
*/
arcLexer::arcLexer( int flags ) {
	arcLexer::loaded = false;
	arcLexer::filename = "";
	arcLexer::flags = flags;
	arcLexer::SetPunctuations( NULL );
	arcLexer::allocated = false;
	arcLexer::fileTime = 0;
	arcLexer::length = 0;
	arcLexer::line = 0;
	arcLexer::lastline = 0;
	arcLexer::tokenavailable = 0;
	arcLexer::token = "";
	arcLexer::next = NULL;
	arcLexer::hadError = false;
}

/*
================
arcLexer::arcLexer
================
*/
arcLexer::arcLexer( const char *filename, int flags, bool OSPath ) {
	arcLexer::loaded = false;
	arcLexer::flags = flags;
	arcLexer::SetPunctuations( NULL );
	arcLexer::allocated = false;
	arcLexer::token = "";
	arcLexer::next = NULL;
	arcLexer::hadError = false;
	arcLexer::LoadFile( filename, OSPath );
}

/*
================
arcLexer::arcLexer
================
*/
arcLexer::arcLexer( const char *ptr, int length, const char *name, int flags ) {
	arcLexer::loaded = false;
	arcLexer::flags = flags;
	arcLexer::SetPunctuations( NULL );
	arcLexer::allocated = false;
	arcLexer::token = "";
	arcLexer::next = NULL;
	arcLexer::hadError = false;
	arcLexer::LoadMemory( ptr, length, name );
}

/*
================
arcLexer::~arcLexer
================
*/
arcLexer::~arcLexer( void ) {
	arcLexer::FreeSource();
}

/*
================
arcLexer::SetBaseFolder
================
*/
void arcLexer::SetBaseFolder( const char *path ) {
	arcNetString::Copynz( baseFolder, path, sizeof( baseFolder ) );
}

/*
================
arcLexer::HadError
================
*/
bool arcLexer::HadError( void ) const {
	return hadError;
}

