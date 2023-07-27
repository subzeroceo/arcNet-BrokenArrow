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

char anLexer::baseFolder[ 256 ];

/*
================
anLexer::CreatePunctuationTable
================
*/
void anLexer::CreatePunctuationTable( const punctuation_t *punctuations ) {
	int i, n, lastp;
	const punctuation_t *p, *newp;

	//get memory for the table
	if ( punctuations == default_punctuations ) {
		anLexer::punctuationtable = default_punctuationtable;
		anLexer::nextpunctuation = default_nextpunctuation;
		if ( default_setup ) {
			return;
		}
		default_setup = true;
		i = sizeof(default_punctuations) / sizeof(punctuation_t);
	} else {
		if ( !anLexer::punctuationtable || anLexer::punctuationtable == default_punctuationtable ) {
			anLexer::punctuationtable = ( int * ) Mem_Alloc(256 * sizeof( int) );
		}
		if ( anLexer::nextpunctuation && anLexer::nextpunctuation != default_nextpunctuation ) {
			Mem_Free( anLexer::nextpunctuation );
		}
		for ( i = 0; punctuations[i].p; i++ ) {
		}
		anLexer::nextpunctuation = ( int * ) Mem_Alloc( i * sizeof( int) );
	}
	memset( anLexer::punctuationtable, 0xFF, 256 * sizeof( int) );
	memset( anLexer::nextpunctuation, 0xFF, i * sizeof( int) );
	//add the punctuations in the list to the punctuation table
	for ( i = 0; punctuations[i].p; i++ ) {
		newp = &punctuations[i];
		lastp = -1;
		//sort the punctuations in this table entry on length (longer punctuations first)
		for ( n = anLexer::punctuationtable[(unsigned int) newp->p[0]]; n >= 0; n = anLexer::nextpunctuation[n] ) {
			p = &punctuations[n];
			if ( strlen( p->p ) < strlen(newp->p ) ) {
				anLexer::nextpunctuation[i] = n;
				if ( lastp >= 0 ) {
					anLexer::nextpunctuation[lastp] = i;
				} else {
					anLexer::punctuationtable[( unsigned int ) newp->p[0]] = i;
				}
				break;
			}
			lastp = n;
		}
		if ( n < 0 ) {
			anLexer::nextpunctuation[i] = -1;
			if (lastp >= 0 ) {
				anLexer::nextpunctuation[lastp] = i;
			}
			else {
				anLexer::punctuationtable[( unsigned int ) newp->p[0]] = i;
			}
		}
	}
}

/*
================
anLexer::GetPunctuationFromId
================
*/
const char *anLexer::GetPunctuationFromId( int id ) {
	int i;

	for ( i = 0; anLexer::punctuations[i].p; i++ ) {
		if ( anLexer::punctuations[i].n == id ) {
			return anLexer::punctuations[i].p;
		}
	}
	return "unkown punctuation";
}

/*
================
anLexer::GetPunctuationId
================
*/
int anLexer::GetPunctuationId( const char *p ) {
	int i;

	for ( i = 0; anLexer::punctuations[i].p; i++ ) {
		if ( !strcmp( anLexer::punctuations[i].p, p ) ) {
			return anLexer::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
anLexer::Error
================
*/
void anLexer::Error( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	hadError = true;

	if ( anLexer::flags & LEXFL_NOERRORS ) {
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );

	if ( anLexer::flags & LEXFL_NOFATALERRORS ) {
		anLibrary::common->Warning( "file %s, line %d: %s", anLexer::filename.c_str(), anLexer::line, text );
	} else {
		anLibrary::common->Error( "file %s, line %d: %s", anLexer::filename.c_str(), anLexer::line, text );
	}
}

/*
================
anLexer::Warning
================
*/
void anLexer::Warning( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	if ( anLexer::flags & LEXFL_NOWARNINGS ) {
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	anLibrary::common->Warning( "file %s, line %d: %s", anLexer::filename.c_str(), anLexer::line, text );
}

/*
================
anLexer::SetPunctuations
================
*/
void anLexer::SetPunctuations( const punctuation_t *p ) {
#ifdef PUNCTABLE
	if ( p ) {
		anLexer::CreatePunctuationTable( p );
	} else {
		anLexer::CreatePunctuationTable( default_punctuations );
	}
#endif //PUNCTABLE
	if ( p ) {
		anLexer::punctuations = p;
	} else {
		anLexer::punctuations = default_punctuations;
	}
}

/*
================
anLexer::ReadWhiteSpace

Reads spaces, tabs, C-like comments etc.
When a newline character is found the scripts line counter is increased.
================
*/
int anLexer::ReadWhiteSpace( void ) {
	while ( 1 ) {
		// skip white space
		while ( *anLexer::script_p <= ' ' ) {
			if ( !*anLexer::script_p ) {
				return 0;
			}
			if ( *anLexer::script_p == '\n' ) {
				anLexer::line++;
			}
			anLexer::script_p++;
		}
		// skip comments
		if ( *anLexer::script_p == '/' ) {
			// comments //
			if ( *( anLexer::script_p+1 ) == '/' ) {
				anLexer::script_p++;
				do {
					anLexer::script_p++;
					if ( !*anLexer::script_p ) {
						return 0;
					}
				}
				while ( *anLexer::script_p != '\n' );
				anLexer::line++;
				anLexer::script_p++;
				if ( !*anLexer::script_p ) {
					return 0;
				}
				continue;
			} else if ( *( anLexer::script_p+1 ) == '*' ) {
				anLexer::script_p++;
				while ( 1 ) {
					anLexer::script_p++;
					if ( !*anLexer::script_p ) {
						return 0;
					}
					if ( *anLexer::script_p == '\n' ) {
						anLexer::line++;
					} else if ( *anLexer::script_p == '/' ) {
						if ( *( anLexer::script_p-1 ) == '*' ) {
							break;
						}
						if ( *( anLexer::script_p+1 ) == '*' ) {
							anLexer::Warning( "nested comment" );
						}
					}
				}
				anLexer::script_p++;
				if ( !*anLexer::script_p ) {
					return 0;
				}
				anLexer::script_p++;
				if ( !*anLexer::script_p ) {
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
anLexer::ReadEscapeCharacter
================
*/
int anLexer::ReadEscapeCharacter( char *ch ) {
	int c, val, i;

	// step over the leading '\\'
	anLexer::script_p++;
	// determine the escape character
	switch( *anLexer::script_p ) {
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
			anLexer::script_p++;
			for ( i = 0, val = 0;; i++, anLexer::script_p++ ) {
				c = *anLexer::script_p;
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
			anLexer::script_p--;
			if (val > 0xFF) {
				anLexer::Warning( "too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
		default: { //NOTE: decimal ASCII code, NOT octal
			if ( *anLexer::script_p < '0' || *anLexer::script_p > '9' ) {
				anLexer::Error( "unknown escape char" );
			}
			for ( i = 0, val = 0;; i++, anLexer::script_p++ ) {
				c = *anLexer::script_p;
				if (c >= '0' && c <= '9' )
					c = c - '0';
				else
					break;
				val = val * 10 + c;
			}
			anLexer::script_p--;
			if (val > 0xFF) {
				anLexer::Warning( "[WARNING]too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
	}
	// step over the escape character or the last digit of the number
	anLexer::script_p++;
	// store the escape character
	*ch = c;
	// succesfully read escape character
	return 1;
}

/*
================
anLexer::ReadString

Escape characters are interpretted.
Reads two strings with only a white space between them as one string.
================
*/
int anLexer::ReadString( arcNetToken *token, int quote ) {
	int tmpline;
	const char *tmpscript_p;
	char ch;

	if ( quote == '\"' ) {
		token->type = TT_STRING;
	} else {
		token->type = TT_LITERAL;
	}

	// leading quote
	anLexer::script_p++;

	while ( 1 ) {
		// if there is an escape character and escape characters are allowed
		if ( *anLexer::script_p == '\\' && !( anLexer::flags & LEXFL_NOSTRINGESCAPECHARS) ) {
			if ( !anLexer::ReadEscapeCharacter( &ch ) ) {
				return 0;
			}
			token->AppendDirty( ch );
		// if a trailing quote
		} else if ( *anLexer::script_p == quote) {
			// step over the quote
			anLexer::script_p++;
			// if consecutive strings should not be concatenated
			if ( ( anLexer::flags & LEXFL_NOSTRINGCONCAT) &&
					( !( anLexer::flags & LEXFL_ALLOWBACKSLASHSTRINGCONCAT) || (quote != '\"' ) ) ) {
				break;
			}

			tmpscript_p = anLexer::script_p;
			tmpline = anLexer::line;
			// read white space between possible two consecutive strings
			if ( !anLexer::ReadWhiteSpace() ) {
				anLexer::script_p = tmpscript_p;
				anLexer::line = tmpline;
				break;
			}

			if ( anLexer::flags & LEXFL_NOSTRINGCONCAT ) {
				if ( *anLexer::script_p != '\\' ) {
					anLexer::script_p = tmpscript_p;
					anLexer::line = tmpline;
					break;
				}
				// step over the '\\'
				anLexer::script_p++;
				if ( !anLexer::ReadWhiteSpace() || ( *anLexer::script_p != quote ) ) {
					anLexer::Error( "expecting string after '\' terminated line" );
					return 0;
				}
			}

			// if there's no leading qoute
			if ( *anLexer::script_p != quote ) {
				anLexer::script_p = tmpscript_p;
				anLexer::line = tmpline;
				break;
			}
			// step over the new leading quote
			anLexer::script_p++;
		} else {
			if ( *anLexer::script_p == '\0' ) {
				anLexer::Error( "missing trailing quote" );
				return 0;
			}
			if ( *anLexer::script_p == '\n' ) {
				anLexer::Error( "newline inside string" );
				return 0;
			}
			token->AppendDirty( *anLexer::script_p++ );
		}
	}
	token->data[token->len] = '\0';

	if ( token->type == TT_LITERAL ) {
		if ( !( anLexer::flags & LEXFL_ALLOWMULTICHARLITERALS) ) {
			if ( token->Length() != 1 ) {
				anLexer::Warning( "[WARNING ]literal is not one character long" );
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
anLexer::ReadName
================
*/
int anLexer::ReadName( arcNetToken *token ) {
	char c;

	token->type = TT_NAME;
	do {
		token->AppendDirty( *anLexer::script_p++ );
		c = *anLexer::script_p;
	} while ( ( c >= 'a' && c <= 'z' ) ||
				( c >= 'A' && c <= 'Z' ) ||
				( c >= '0' && c <= '9' ) ||
				c == '_' ||
				// if treating all tokens as strings, don't parse '-' as a seperate token
				( ( anLexer::flags & LEXFL_ONLYSTRINGS) && (c == '-' ) ) ||
				// if special path name characters are allowed
				( ( anLexer::flags & LEXFL_ALLOWPATHNAMES) && (c == '/' || c == '\\' || c == ':' || c == '.' ) ) );
	token->data[token->len] = '\0';
	//the sub type is the length of the name
	token->subtype = token->Length();
	return 1;
}

/*
================
anLexer::CheckString
================
*/
ARC_INLINE int anLexer::CheckString( const char *str ) const {
	for ( int i = 0; str[i]; i++ ) {
		if ( anLexer::script_p[i] != str[i] ) {
			return false;
		}
	}
	return true;
}

/*
================
anLexer::ReadNumber
================
*/
int anLexer::ReadNumber( arcNetToken *token ) {
	int i;
	int dot;
	char c, c2;

	token->type = TT_NUMBER;
	token->subtype = 0;
	token->intvalue = 0;
	token->floatvalue = 0;

	c = *anLexer::script_p;
	c2 = *( anLexer::script_p + 1 );

	if ( c == '0' && c2 != '.' ) {
		// check for a hexadecimal number
		if ( c2 == 'x' || c2 == 'X' ) {
			token->AppendDirty( *anLexer::script_p++ );
			token->AppendDirty( *anLexer::script_p++ );
			c = *anLexer::script_p;
			while ((c >= '0' && c <= '9' ) || (c >= 'a' && c <= 'f' ) || (c >= 'A' && c <= 'F' ) ) {
				token->AppendDirty( c );
				c = *( ++anLexer::script_p );
			}
			token->subtype = TT_HEX | TT_INTEGER;
		// check for a binary number
		} else if ( c2 == 'b' || c2 == 'B' ) {
			token->AppendDirty( *anLexer::script_p++ );
			token->AppendDirty( *anLexer::script_p++ );
			c = *anLexer::script_p;
			while ( c == '0' || c == '1' ) {
				token->AppendDirty( c );
				c = *( ++anLexer::script_p );
			}
			token->subtype = TT_BINARY | TT_INTEGER;
		// its an octal number
		} else {
			token->AppendDirty( *anLexer::script_p++ );
			c = *anLexer::script_p;
			while ( c >= '0' && c <= '7' ) {
				token->AppendDirty( c );
				c = *( ++anLexer::script_p );
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
			c = *( ++anLexer::script_p );
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
				c = *( ++anLexer::script_p );
				if ( c == '-' ) {
					token->AppendDirty( c );
					c = *( ++anLexer::script_p );
				} else if ( c == '+' ) {
					token->AppendDirty( c );
					c = *( ++anLexer::script_p );
				}
				while ( c >= '0' && c <= '9' ) {
					token->AppendDirty( c );
					c = *( ++anLexer::script_p );
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
					c = *( ++anLexer::script_p );
				}
				while ( c >= '0' && c <= '9' ) {
					token->AppendDirty( c );
					c = *( ++anLexer::script_p );
				}
				if ( !( anLexer::flags & LEXFL_ALLOWFLOATEXCEPTIONS) ) {
					token->AppendDirty( 0 );	// zero terminate for c_str
					anLexer::Error( "parsed %s", token->c_str() );
				}
			}
		} else if ( dot > 1 ) {
			if ( !( anLexer::flags & LEXFL_ALLOWIPADDRESSES ) ) {
				anLexer::Error( "[ERROR] more than one period (dot) in number" );
				return 0;
			}
			if ( dot != 3 ) {
				anLexer::Error( "[ERROR] ip address should have three periods (dots)" );
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
				anLexer::script_p++;
			// extended-precision: long double
			} else if ( c == 'l' || c == 'L' ) {
				token->subtype |= TT_EXTENDED_PRECISION;
				anLexer::script_p++;
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
				c = *( ++anLexer::script_p );
			}
		}
	} else if ( token->subtype & TT_IPADDRESS ) {
		if ( c == ':' ) {
			token->AppendDirty( c );
			c = *( ++anLexer::script_p );
			while ( c >= '0' && c <= '9' ) {
				token->AppendDirty( c );
				c = *( ++anLexer::script_p );
			}
			token->subtype |= TT_IPPORT;
		}
	}
	token->data[token->len] = '\0';
	return 1;
}

/*
================
anLexer::ReadPunctuation
================
*/
int anLexer::ReadPunctuation( arcNetToken *token ) {
	int l, n, i;
	char *p;
	const punctuation_t *punc;

#ifdef PUNCTABLE
	for (n = anLexer::punctuationtable[(unsigned int)*( anLexer::script_p )]; n >= 0; n = anLexer::nextpunctuation[n] )
	{
		punc = &( anLexer::punctuations[n] );
#else
	int i;

	for ( i = 0; anLexer::punctuations[i].p; i++ ) {
		punc = &anLexer::punctuations[i];
#endif
		p = punc->p;
		// check for this punctuation in the script
		for ( l = 0; p[l] && anLexer::script_p[l]; l++ ) {
			if ( anLexer::script_p[l] != p[l] ) {
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
			anLexer::script_p += l;
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
anLexer::ReadToken
================
*/
int anLexer::ReadToken( arcNetToken *token ) {
	int c;

	if ( !loaded ) {
		anLibrary::common->Error( "Lexer::ReadToken: no file loaded" );
		return 0;
	}

	// if there is a token available (from unreadToken)
	if ( tokenavailable ) {
		tokenavailable = 0;
		*token = anLexer::token;
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
	anLexer::whiteSpaceEnd_p = script_p;
	token->whiteSpaceEnd_p = script_p;
	// line the token is on
	token->line = line;
	// number of lines crossed before token
	token->linesCrossed = line - lastline;
	// clear token flags
	token->flags = 0;

	c = *anLexer::script_p;

	// if we're keeping everything as whitespace deliminated strings
	if ( anLexer::flags & LEXFL_ONLYSTRINGS ) {
		// if there is a leading quote
		if ( c == '\"' || c == '\'' ) {
			if ( !anLexer::ReadString( token, c ) ) {
				return 0;
			}
		} else if ( !anLexer::ReadName( token ) ) {
			return 0;
		}
	}
	// if there is a number
	else if ( (c >= '0' && c <= '9' ) ||
			(c == '.' && ( *( anLexer::script_p + 1 ) >= '0' && *( anLexer::script_p + 1 ) <= '9' ) ) ) {
		if ( !anLexer::ReadNumber( token ) ) {
			return 0;
		}
		// if names are allowed to start with a number
		if ( anLexer::flags & LEXFL_ALLOWNUMBERNAMES ) {
			c = *anLexer::script_p;
			if ( (c >= 'a' && c <= 'z' ) ||	(c >= 'A' && c <= 'Z' ) || c == '_' ) {
				if ( !anLexer::ReadName( token ) ) {
					return 0;
				}
			}
		}
	// if there is a leading quote
	} else if ( c == '\"' || c == '\'' ) {
		if ( !anLexer::ReadString( token, c ) ) {
			return 0;
		}
		// if there is a name
	} else if ( (c >= 'a' && c <= 'z' ) ||	(c >= 'A' && c <= 'Z' ) || c == '_' ) {
		if ( !anLexer::ReadName( token ) ) {
			return 0;
		}
	// names may also start with a slash when pathnames are allowed
	} else if ( ( anLexer::flags & LEXFL_ALLOWPATHNAMES ) && ( (c == '/' || c == '\\' ) || c == '.' ) ) {
		if ( !anLexer::ReadName( token ) ) {
			return 0;
		}
	// check for punctuations
	} else if ( !anLexer::ReadPunctuation( token ) ) {
		anLexer::Error( "[WARNING] unknown punctuation %c", c );
		return 0;
	}
	// succesfully read a token
	return 1;
}

/*
================
anLexer::ExpectTokenString
================
*/
int anLexer::ExpectTokenString( const char *string ) {
	arcNetToken token;

	if ( !anLexer::ReadToken( &token ) ) {
		anLexer::Error( "[ERROR] couldn't find expected '%s'", string );
		return 0;
	}
	if ( token != string ) {
		anLexer::Error( "[WARNING] expected '%s' but found '%s'", string, token.c_str() );
		return 0;
	}
	return 1;
}

/*
================
anLexer::ExpectTokenType
================
*/
int anLexer::ExpectTokenType( int type, int subtype, arcNetToken *token ) {
	anString str;

	if ( !anLexer::ReadToken( token ) ) {
		anLexer::Error( "[ERROR] couldn't read expected token" );
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
		anLexer::Error( "[WARNING] expected a %s but found '%s'", str.c_str(), token->c_str() );
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
			anLexer::Error( "[WARNING] expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if ( token->type == TT_PUNCTUATION ) {
		if ( subtype < 0 ) {
			anLexer::Error( "[ERROR] wrong punctuation subtype" );
			return 0;
		}
		if ( token->subtype != subtype ) {
			anLexer::Error( "[WARNING] expected '%s' but found '%s'", GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
anLexer::ExpectAnyToken
================
*/
int anLexer::ExpectAnyToken( arcNetToken *token ) {
	if ( !anLexer::ReadToken( token ) ) {
		anLexer::Error( "[ERROR] couldn't read expected token" );
		return 0;
	} else {
		return 1;
	}
}

/*
================
anLexer::CheckTokenString
================
*/
int anLexer::CheckTokenString( const char *string ) {
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
anLexer::CheckTokenType
================
*/
int anLexer::CheckTokenType( int type, int subtype, arcNetToken *token ) {
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
anLexer::PeekTokenString
================
*/
int anLexer::PeekTokenString( const char *string ) {
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
anLexer::PeekTokenType
================
*/
int anLexer::PeekTokenType( int type, int subtype, arcNetToken *token ) {
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
anLexer::SkipUntilString
================
*/
int anLexer::SkipUntilString( const char *string ) {
	arcNetToken token;

	while ( anLexer::ReadToken( &token ) ) {
		if ( token == string ) {
			return 1;
		}
	}
	return 0;
}

/*
================
anLexer::SkipRestOfLine
================
*/
int anLexer::SkipRestOfLine( void ) {
	arcNetToken token;

	while ( anLexer::ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			anLexer::script_p = lastScript_p;
			anLexer::line = lastline;
			return 1;
		}
	}
	return 0;
}

/*
=================
anLexer::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int anLexer::SkipBracedSection( bool parseFirstBrace ) {
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
anLexer::UnreadToken
================
*/
void anLexer::UnreadToken( const arcNetToken *token ) {
	if ( anLexer::tokenavailable ) {
		anLibrary::common->FatalError( "Lexer::UnreadToken, unread token twice\n" );
	}
	anLexer::token = *token;
	anLexer::tokenavailable = 1;
}

/*
================
anLexer::ReadTokenOnLine
================
*/
int anLexer::ReadTokenOnLine( arcNetToken *token ) {
	arcNetToken tok;

	if ( !anLexer::ReadToken( &tok ) ) {
		anLexer::script_p = lastScript_p;
		anLexer::line = lastline;
		return false;
	}
	// if no lines were crossed before this token
	if ( !tok.linesCrossed ) {
		*token = tok;
		return true;
	}
	// restore our position
	anLexer::script_p = lastScript_p;
	anLexer::line = lastline;
	token->Clear();
	return false;
}

/*
================
anLexer::ReadRestOfLine
================
*/
const char*	anLexer::ReadRestOfLine( anString& out) {
	while ( 1 ) {
		if ( *anLexer::script_p == '\n' ) {
			anLexer::line++;
			break;
		}

		if ( !*anLexer::script_p ) {
			break;
		}

		if ( *anLexer::script_p <= ' ' ) {
			out += " ";
		} else {
			out += *anLexer::script_p;
		}
		anLexer::script_p++;

	}

	out.Strip( ' ' );
	return out.c_str();
}

/*
================
anLexer::ParseInt
================
*/
int anLexer::ParseInt( void ) {
	arcNetToken token;

	if ( !anLexer::ReadToken( &token ) ) {
		anLexer::Error( "[ERROR] Failed to read expected integer" );
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		anLexer::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -( (signed int) token.GetIntValue() );
	} else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		anLexer::Error( "[WARNING] expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
anLexer::ParseBool
================
*/
bool anLexer::ParseBool( void ) {
	arcNetToken token;
	if ( !anLexer::ExpectTokenType( TT_NUMBER, 0, &token ) ) {
		anLexer::Error( "[ERROR] Failed to read expected boolean value." );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
anLexer::ParseFloat
================
*/
float anLexer::ParseFloat( bool *errorFlag ) {
	arcNetToken token;

	if ( errorFlag ) {
		*errorFlag = false;
	}

	if ( !anLexer::ReadToken( &token ) ) {
		if ( errorFlag ) {
			anLexer::Warning( "[ERROR] Failed to read expected floating point number" );
			*errorFlag = true;
		} else {
			anLexer::Error( "[ERROR] Failed to read expected floating point number" );
		}
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		anLexer::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	} else if ( token.type != TT_NUMBER ) {
		if ( errorFlag ) {
			anLexer::Warning( "[WARNING] expected float value, found '%s'", token.c_str() );
			*errorFlag = true;
		} else {
			anLexer::Error( "[WARNING] expected float value, found '%s'", token.c_str() );
		}
	}
	return token.GetFloatValue();
}

/*
================
anLexer::Parse1DMatrix
================
*/
int anLexer::Parse1DMatrix( int x, float *m ) {
	if ( !anLexer::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( int i = 0; i < x; i++ ) {
		m[i] = anLexer::ParseFloat();
	}

	if ( !anLexer::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
anLexer::Parse2DMatrix
================
*/
int anLexer::Parse2DMatrix( int y, int x, float *m ) {
	if ( !anLexer::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( int i = 0; i < y; i++ ) {
		if ( !anLexer::Parse1DMatrix( x, m + i * x ) ) {
			return false;
		}
	}

	if ( !anLexer::ExpectTokenString( " )" ) ) {
		return false;
	}
	return true;
}

/*
================
anLexer::Parse3DMatrix
================
*/
int anLexer::Parse3DMatrix( int z, int y, int x, float *m ) {
	if ( !anLexer::ExpectTokenString( "( " ) ) {
		return false;
	}

	for ( int i = 0; i < z; i++ ) {
		if ( !anLexer::Parse2DMatrix( y, x, m + i * x*y ) ) {
			return false;
		}
	}

	if ( !anLexer::ExpectTokenString( " )" ) ) {
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
const char *anLexer::ParseBracedSectionExact( anString &out, int tabs ) {
	int		depth;
	bool	doTabs;
	bool	skipWhite;

	out.Empty();

	if ( !anLexer::ExpectTokenString( "{" ) ) {
		return out.c_str( );
	}

	out = "{";
	depth = 1;
	skipWhite = false;
	doTabs = tabs >= 0;

	while ( depth && *anLexer::script_p ) {
		char c = *( anLexer::script_p++ );
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
anLexer::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
const char *anLexer::ParseBracedSection( anString &out ) {
	arcNetToken token;
	int depth;

	out.Empty();
	if ( !anLexer::ExpectTokenString( "{" ) ) {
		return out.c_str();
	}
	out = "{";
	depth = 1;
	do {
		if ( !anLexer::ReadToken( &token ) ) {
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
anLexer::ParseRestOfLine

  parse the rest of the line
=================
*/
const char *anLexer::ParseRestOfLine( anString &out ) {
	arcNetToken token;

	out.Empty();
	while ( anLexer::ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			anLexer::script_p = lastScript_p;
			anLexer::line = lastline;
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
anLexer::GetLastWhiteSpace
================
*/
int anLexer::GetLastWhiteSpace( anString &whiteSpace ) const {
	whiteSpace.Clear();
	for ( const char *p = whiteSpaceStart_p; p < whiteSpaceEnd_p; p++ ) {
		whiteSpace.Append( *p );
	}
	return whiteSpace.Length();
}

/*
================
anLexer::GetLastWhiteSpaceStart
================
*/
int anLexer::GetLastWhiteSpaceStart( void ) const {
	return whiteSpaceStart_p - buffer;
}

/*
================
anLexer::GetLastWhiteSpaceEnd
================
*/
int anLexer::GetLastWhiteSpaceEnd( void ) const {
	return whiteSpaceEnd_p - buffer;
}

/*
================
anLexer::Reset
================
*/
void anLexer::Reset( void ) {
	// pointer in script buffer
	anLexer::script_p = anLexer::buffer;
	// pointer in script buffer before reading token
	anLexer::lastScript_p = anLexer::buffer;
	// begin of white space
	anLexer::whiteSpaceStart_p = NULL;
	// end of white space
	anLexer::whiteSpaceEnd_p = NULL;
	// set if there's a token available in anLexer::token
	anLexer::tokenavailable = 0;

	anLexer::line = 1;
	anLexer::lastline = 1;
	// clear the saved token
	anLexer::token = "";
}

/*
================
anLexer::EndOfFile
================
*/
int anLexer::EndOfFile( void ) {
	return anLexer::script_p >= anLexer::end_p;
}

/*
================
anLexer::NumLinesCrossed
================
*/
int anLexer::NumLinesCrossed( void ) {
	return anLexer::line - anLexer::lastline;
}

/*
================
anLexer::LoadFile
================
*/
int anLexer::LoadFile( const char *filename, bool OSPath ) {
	anFile *fp;
	anString pathname;
	int length;
	char *buf;

	if ( anLexer::loaded ) {
		anLibrary::common->Error( "[WARNING] Lexer::LoadFile: another script already loaded" );
		return false;
	}

	if ( !OSPath && ( baseFolder[0] != '\0' ) ) {
		pathname = va( "%s/%s", baseFolder, filename );
	} else {
		pathname = filename;
	}
	if ( OSPath ) {
		fp = anLibrary::fileSystem->OpenExplicitFileRead( pathname );
	} else {
		fp = anLibrary::fileSystem->OpenFileRead( pathname );
	}
	if ( !fp ) {
		return false;
	}
	length = fp->Length();
	buf = (char *) Mem_Alloc( length + 1 );
	buf[length] = '\0';
	fp->Read( buf, length );
	anLexer::fileTime = fp->Timestamp();
	anLexer::filename = fp->GetFullPath();
	anLibrary::fileSystem->CloseFile( fp );

	anLexer::buffer = buf;
	anLexer::length = length;
	// pointer in script buffer
	anLexer::script_p = anLexer::buffer;
	// pointer in script buffer before reading token
	anLexer::lastScript_p = anLexer::buffer;
	// pointer to end of script buffer
	anLexer::end_p = &( anLexer::buffer[length] );

	anLexer::tokenavailable = 0;
	anLexer::line = 1;
	anLexer::lastline = 1;
	anLexer::allocated = true;
	anLexer::loaded = true;

	return true;
}

/*
================
anLexer::LoadMemory
================
*/
int anLexer::LoadMemory( const char *ptr, int length, const char *name, int startLine ) {
	if ( anLexer::loaded ) {
		anLibrary::common->Error( "[WARNING] LoadMemory another script already loaded" );
		return false;
	}
	anLexer::filename = name;
	anLexer::buffer = ptr;
	anLexer::fileTime = 0;
	anLexer::length = length;
	// pointer in script buffer
	anLexer::script_p = anLexer::buffer;
	// pointer in script buffer before reading token
	anLexer::lastScript_p = anLexer::buffer;
	// pointer to end of script buffer
	anLexer::end_p = &( anLexer::buffer[length] );

	anLexer::tokenavailable = 0;
	anLexer::line = startLine;
	anLexer::lastline = startLine;
	anLexer::allocated = false;
	anLexer::loaded = true;

	return true;
}

/*
================
anLexer::FreeSource
================
*/
void anLexer::FreeSource( void ) {
#ifdef PUNCTABLE
	if ( anLexer::punctuationtable && anLexer::punctuationtable != default_punctuationtable ) {
		Mem_Free( (void *) anLexer::punctuationtable );
		anLexer::punctuationtable = NULL;
	}
	if ( anLexer::nextpunctuation && anLexer::nextpunctuation != default_nextpunctuation ) {
		Mem_Free( (void *) anLexer::nextpunctuation );
		anLexer::nextpunctuation = NULL;
	}
#endif //PUNCTABLE
	if ( anLexer::allocated ) {
		Mem_Free( (void *) anLexer::buffer );
		anLexer::buffer = NULL;
		anLexer::allocated = false;
	}
	anLexer::tokenavailable = 0;
	anLexer::token = "";
	anLexer::loaded = false;
}

/*
================
anLexer::anLexer
================
*/
anLexer::anLexer( void ) {
	anLexer::loaded = false;
	anLexer::filename = "";
	anLexer::flags = 0;
	anLexer::SetPunctuations( NULL );
	anLexer::allocated = false;
	anLexer::fileTime = 0;
	anLexer::length = 0;
	anLexer::line = 0;
	anLexer::lastline = 0;
	anLexer::tokenavailable = 0;
	anLexer::token = "";
	anLexer::next = NULL;
	anLexer::hadError = false;
}

/*
================
anLexer::anLexer
================
*/
anLexer::anLexer( int flags ) {
	anLexer::loaded = false;
	anLexer::filename = "";
	anLexer::flags = flags;
	anLexer::SetPunctuations( NULL );
	anLexer::allocated = false;
	anLexer::fileTime = 0;
	anLexer::length = 0;
	anLexer::line = 0;
	anLexer::lastline = 0;
	anLexer::tokenavailable = 0;
	anLexer::token = "";
	anLexer::next = NULL;
	anLexer::hadError = false;
}

/*
================
anLexer::anLexer
================
*/
anLexer::anLexer( const char *filename, int flags, bool OSPath ) {
	anLexer::loaded = false;
	anLexer::flags = flags;
	anLexer::SetPunctuations( NULL );
	anLexer::allocated = false;
	anLexer::token = "";
	anLexer::next = NULL;
	anLexer::hadError = false;
	anLexer::LoadFile( filename, OSPath );
}

/*
================
anLexer::anLexer
================
*/
anLexer::anLexer( const char *ptr, int length, const char *name, int flags ) {
	anLexer::loaded = false;
	anLexer::flags = flags;
	anLexer::SetPunctuations( NULL );
	anLexer::allocated = false;
	anLexer::token = "";
	anLexer::next = NULL;
	anLexer::hadError = false;
	anLexer::LoadMemory( ptr, length, name );
}

/*
================
anLexer::~anLexer
================
*/
anLexer::~anLexer( void ) {
	anLexer::FreeSource();
}

/*
================
anLexer::SetBaseFolder
================
*/
void anLexer::SetBaseFolder( const char *path ) {
	anString::Copynz( baseFolder, path, sizeof( baseFolder ) );
}

/*
================
anLexer::HadError
================
*/
bool anLexer::HadError( void ) const {
	return hadError;
}

