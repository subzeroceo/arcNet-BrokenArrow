#ifndef __TOKEN_H__
#define __TOKEN_H__

/*
===============================================================================

	arcNetToken is a token read from a file or memory with anLexer or ARCParser

===============================================================================
*/

// token types
#define TT_STRING					1		// string
#define TT_LITERAL					2		// literal
#define TT_NUMBER					3		// number
#define TT_NAME						4		// name
#define TT_PUNCTUATION				5		// punctuation

// number sub types
#define TT_INTEGER					0x00001		// integer
#define TT_DECIMAL					0x00002		// decimal number
#define TT_HEX						0x00004		// hexadecimal number
#define TT_OCTAL					0x00008		// octal number
#define TT_BINARY					0x00010		// binary number
#define TT_LONG						0x00020		// long int
#define TT_UNSIGNED					0x00040		// unsigned int
#define TT_FLOAT					0x00080		// floating point number
#define TT_SINGLE_PRECISION			0x00100		// float
#define TT_DOUBLE_PRECISION			0x00200		// double
#define TT_EXTENDED_PRECISION		0x00400		// long double
#define TT_INFINITE					0x00800		// infinite 1.#INF
#define TT_INDEFINITE				0x01000		// indefinite 1.#IND
#define TT_NAN						0x02000		// NaN
#define TT_IPADDRESS				0x04000		// ip address
#define TT_IPPORT					0x08000		// ip port
#define TT_VALUESVALID				0x10000		// set if intvalue and floatvalue are valid

// string sub type is the length of the string
// literal sub type is the ASCII code
// punctuation sub type is the punctuation id
// name sub type is the length of the name

class arcNetToken : public anString {

	friend class ARCParser;
	friend class anLexer;

public:
	int				type;								// token type
	int				subtype;							// token sub type
	int				line;								// line in script the token was on
	int				linesCrossed;						// number of lines crossed in white space before token
	int				flags;								// token flags, used for recursive defines

public:
					arcNetToken( void );
					arcNetToken( const arcNetToken *token );
					~arcNetToken( void );

	void			operator=( const anString& text );
	void			operator=( const char *text );

	double			GetDoubleValue( void );				// double value of TT_NUMBER
	float			GetFloatValue( void );				// float value of TT_NUMBER
	unsigned long	GetUnsignedLongValue( void );		// unsigned long value of TT_NUMBER
	int				GetIntValue( void );				// int value of TT_NUMBER
	int				WhiteSpaceBeforeToken( void ) const;// returns length of whitespace before token
	void			ClearTokenWhiteSpace( void );		// forget whitespace before token

	void			NumberValue( void );				// calculate values for a TT_NUMBER

private:
	unsigned long	intvalue;							// integer value
	double			floatvalue;							// floating point value
	const char *	whiteSpaceStart_p;					// start of white space before token, only used by anLexer
	const char *	whiteSpaceEnd_p;					// end of white space before token, only used by anLexer
	arcNetToken *		next;								// next token in chain, only used by ARCParser

	void			AppendDirty( const char a );		// append character without adding trailing zero
};

ARC_INLINE arcNetToken::arcNetToken( void ) {
}

ARC_INLINE arcNetToken::arcNetToken( const arcNetToken *token ) {
	*this = *token;
}

ARC_INLINE arcNetToken::~arcNetToken( void ) {
}

ARC_INLINE void arcNetToken::operator=( const char *text) {
	*static_cast<anString *>( this ) = text;
}

ARC_INLINE void arcNetToken::operator=( const anString& text ) {
	*static_cast<anString *>( this ) = text;
}

ARC_INLINE double arcNetToken::GetDoubleValue( void ) {
	if ( type != TT_NUMBER ) {
		return 0.0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return floatvalue;
}

ARC_INLINE float arcNetToken::GetFloatValue( void ) {
	return ( float ) GetDoubleValue();
}

ARC_INLINE unsigned long	arcNetToken::GetUnsignedLongValue( void ) {
	if ( type != TT_NUMBER ) {
		return 0;
	}
	if ( !( subtype & TT_VALUESVALID ) ) {
		NumberValue();
	}
	return intvalue;
}

ARC_INLINE int arcNetToken::GetIntValue( void ) {
	return ( int ) GetUnsignedLongValue();
}

ARC_INLINE int arcNetToken::WhiteSpaceBeforeToken( void ) const {
	return ( whiteSpaceEnd_p > whiteSpaceStart_p );
}

ARC_INLINE void arcNetToken::AppendDirty( const char a ) {
	EnsureAlloced( len + 2, true );
	data[len++] = a;
}

#endif