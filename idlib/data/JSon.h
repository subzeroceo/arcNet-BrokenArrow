#ifndef JSON_H
#define JSON_H

#ifdef JSON_IMPLEMENTATION
#include <stdio.h>
#endif

#define IS_SEPARATOR(x)    ((x) == ' ' || (x) == '\t' || (x) == '\n' || (x) == '\r' || (x) == ',' || (x) == ':')
#define IS_STRUCT_OPEN(x)  ((x) == '{' || (x) == '[')
#define IS_STRUCT_CLOSE(x) ((x) == '}' || (x) == ']')


typedef class anJSonFormat {
public:
/// --------------------------------------------------------------------------
//  						 Array Functions
// --------------------------------------------------------------------------


	// Get pointer to first value in array
	// When given pointer to an array, returns pointer to the first
	// returns nullptr if array is empty or not an array.
	const char *							ArrayGetFirstValue( const char *json, const char *jsonEnd );

	// Get pointer to next value in array
	// When given pointer to a value, returns pointer to the next value
	// returns nullptr when no next value.
	const char *							ArrayGetNextValue( const char *json, const char *jsonEnd );

	// Get pointers to values in an array
	// returns 0 if not an array, array is empty, or out of data
	// returns number of values in the array and copies into index if successful
	unsigned int 							ArrayGetIndex( const char *json, const char *jsonEnd, const char **indexes, unsigned int numIndexes );

	// Get pointer to indexed value from array
	// returns nullptr if not an array, no index, or out of data
	const char *							ArrayGetValue( const char *json, const char *jsonEnd, unsigned int index );

// --------------------------------------------------------------------------
//   Object Functions
// --------------------------------------------------------------------------

// Get pointer to named value from object
// returns nullptr if not an object, name not found, or out of data
	const char *							ObjectGetNamedValue( const char *json, const char *jsonEnd, const char *name);

// --------------------------------------------------------------------------
//   Value Functions
// --------------------------------------------------------------------------

	// Get type of value
	// returns JSONTYPE_ERROR if out of data
	unsigned int 							ValueGetType( const char *json, const char *jsonEnd );

	// Get value as string
	// returns 0 if out of data
	// returns length and copies into string if successful, including terminating nul.
	// string values are stripped of enclosing quotes but not escaped
	unsigned int 							ValueGetString( const char *json, const char *jsonEnd, char *outString, unsigned int stringLen);

	// Get value as appropriate type
	// returns 0 if value is false, value is null, or out of data
	// returns 1 if value is true
	// returns value otherwise
	double 									ValueGetDouble( const char *json, const char *jsonEnd );
	float 									ValueGetFloat( const char *json, const char *jsonEnd );
	int 									ValueGetInt( const char *json, const char *jsonEnd );

private:

	static const char *SkipSeparators( const char *json, const char *jsonEnd );
	static const char *SkipString( const char *json, const char *jsonEnd );
	static const char *SkipStruct( const char *json, const char *jsonEnd );
	static const char *SkipValue( const char *json, const char *jsonEnd );
	static const char *SkipValueAndSeparators( const char *json, const char *jsonEnd );

enum {
	JSONTYPE_STRING, // string
	JSONTYPE_OBJECT, // object
	JSONTYPE_ARRAY,  // array
	JSONTYPE_VALUE,  // number, true, false, or null
	JSONTYPE_ERROR   // out of data
};

};
#endif

// --------------------------------------------------------------------------
//   Internal Functions
// --------------------------------------------------------------------------

static const char *anJSonFormat::SkipSeparators( const char *json, const char *jsonEnd ) {
	while ( json < jsonEnd && IS_SEPARATOR(* json) ) {
		json++;
	}
	return json;
}

static const char *anJSonFormat::SkipString( const char *json, const char *jsonEnd ) {
	for ( json++; json < jsonEnd && *json != '"'; json++ ) {
		if ( *json == '\\' ) {
			json++;
		}
	}
	return (json + 1 > jsonEnd) ? jsonEnd : json + 1;
}

inline static const char *anJSonFormat::SkipStruct( const char *json, const char *jsonEnd ) {
	json = SkipSeparators( json + 1, jsonEnd );
	while ( json < jsonEnd && !IS_STRUCT_CLOSE(* json) )
		json = SkipValueAndSeparators( json, jsonEnd );

	return ( json + 1 > jsonEnd ) ? jsonEnd : json + 1;
}

inline static const char *anJSonFormat::SkipValue( const char *json, const char *jsonEnd ) {
	if ( json >= jsonEnd ) {
		return jsonEnd;
	} else if ( *json == '"' ) {
		json = SkipString( json, jsonEnd );
 	} else if (IS_STRUCT_OPEN(*json ) ) {
		json = SkipStruct( json, jsonEnd );
	} else {
		while ( json < jsonEnd && !IS_SEPARATOR(* json) && !IS_STRUCT_CLOSE(* json) )
			json++;
	}

	return json;
}

inline static const char *anJSonFormat::SkipValueAndSeparators( const char *json, const char *jsonEnd ) {
	json = SkipValue( json, jsonEnd );
	return SkipSeparators( json, jsonEnd );
}

// returns 0 if value requires more parsing, 1 if no more data/false/null, 2 if true
inline static unsigned int anJSonFormat::NoParse( const char *json, const char *jsonEnd ) {
	if ( !json || json >= jsonEnd || *json == 'f' || *json == 'n') {
		return 1;
	}
	if ( *json == 't' ) {
		return 2;
	}

	return 0;
}

// --------------------------------------------------------------------------
//   Array Functions
// --------------------------------------------------------------------------

inline const char *anJSonFormat::ArrayGetFirstValue( const char *json, const char *jsonEnd ) {
	if ( !json || json >= jsonEnd || !IS_STRUCT_OPEN(* json) )
		return nullptr;

	json = SkipSeparators( json + 1, jsonEnd );

	return (json >= jsonEnd || IS_STRUCT_CLOSE(*json) ) ? nullptr : json;
}

inline const char *anJSonFormat::ArrayGetNextValue( const char *json, const char *jsonEnd ) {
	if ( !json || json >= jsonEnd || IS_STRUCT_CLOSE(* json) )
		return nullptr;

	json = SkipValueAndSeparators( json, jsonEnd );

	return ( json >= jsonEnd || IS_STRUCT_CLOSE(* json) ) ? nullptr : json;
}

inline unsigned int anJSonFormat::ArrayGetIndex( const char *json, const char *jsonEnd, const char **indexes, unsigned int numIndexes ) {
	unsigned int length = 0;

	for ( json = ArrayGetFirstValue( json, jsonEnd ); json; json = ArrayGetNextValue( json, jsonEnd ) ) {
		if ( indexes && numIndexes ) {
			*indexes++ = json;
			numIndexes--;
		}
		length++;
	}

	return length;
}

inline const char *anJSonFormat::ArrayGetValue( const char *json, const char *jsonEnd, unsigned int index ) {
	for ( json = ArrayGetFirstValue( json, jsonEnd ); json && index; json = ArrayGetNextValue( json, jsonEnd )) {
		index--;
	}

	return json;
}

// --------------------------------------------------------------------------
//   Object Functions
// --------------------------------------------------------------------------

inline const char *anJSonFormat::ObjectGetNamedValue( const char *json, const char *jsonEnd, const char *name ) {
	unsigned int nameLen = strlen( name );

	for ( json = ArrayGetFirstValue( json, jsonEnd );
	json; json = ArrayGetNextValue( json, jsonEnd ) ) {
		if ( *json == '"' ) {
			const char *thisNameStart, *thisNameEnd;

			thisNameStart = json + 1;
			json = SkipString( json, jsonEnd );
			thisNameEnd = json - 1;
			json = SkipSeparators( json, jsonEnd );

			if ( (unsigned int)( thisNameEnd - thisNameStart ) == nameLen ) {
				if ( strncmp( thisNameStart, name, nameLen ) == 0 ) {
					return json;
				}
			}
		}
	}

	return nullptr;
}

// --------------------------------------------------------------------------
//   Value Functions
// --------------------------------------------------------------------------

inline unsigned int anJSonFormat::ValueGetType( const char *json, const char *jsonEnd ) {
	if ( !json || json >= jsonEnd) {
		return JSONTYPE_ERROR;
	} else if ( *json == '"' ) {
		return JSONTYPE_STRING;
	} else if (*json == '{') {
		return JSONTYPE_OBJECT;
	} else if (*json == '[') {
		return JSONTYPE_ARRAY;
	}
	return JSONTYPE_VALUE;
}

unsigned int anJSonFormat::ValueGetString( const char *json, const char *jsonEnd, char *outString, unsigned int stringLen ) {
	const char *stringEnd, *stringStart;

	if ( !json ) {
		*outString = '\0';
		return 0;
	}

	stringStart = json;
	stringEnd = SkipValue( stringStart, jsonEnd );
	if ( stringEnd >= jsonEnd ) {
		*outString = '\0';
		return 0;
	}

	// skip enclosing quotes if they exist
	if ( *stringStart == '"' ) {
		stringStart++;
	}
	if ( *( stringEnd - 1 ) == '"' ) {
		stringEnd--;
	}
	stringLen--;
	if ( stringLen > stringEnd - stringStart ) {
		stringLen = stringEnd - stringStart;
	}
	json = stringStart;
	while ( stringLen-- ) {
		*outString++ = *json++;
	}
	*outString = '\0';

	return stringEnd - stringStart;
}

inline double anJSonFormat::ValueGetDouble( const char *json, const char *jsonEnd ) {
	char cValue[256];
	double dValue = 0.0;
	unsigned int np = NoParse( json, jsonEnd );

	if ( np ) {
		return (double)( np - 1 );
	}
	if ( !JSON_ValueGetString(json, jsonEnd, cValue, 256 ) ) {
		return 0.0;
	}
	sscanf( cValue, "%lf", &dValue );

	return dValue;
}

inline float anJSonFormat::ValueGetFloat( const char *json, const char *jsonEnd ) {
	char cValue[256];
	float fValue = 0.0f;
	unsigned int np = NoParse( json, jsonEnd );

	if ( np ) {
		return (float)( np - 1 );
	}
	if ( !ValueGetString( json, jsonEnd, cValue, 256 ) ) {
		return 0.0f;
	}
	sscanf( cValue, "%f", &fValue );

	return fValue;
}

inline int anJSonFormat::ValueGetInt( const char *json, const char *jsonEnd ) {
	char cValue[256];
	int iValue = 0;
	unsigned int np = NoParse( json, jsonEnd );

	if ( np ) {
		return np - 1;
	}
	if ( !ValueGetString( json, jsonEnd, cValue, 256 ) ) {
		return 0;
	}
	sscanf( cValue, "%d", &iValue );

	return iValue;
}

#undef IS_SEPARATOR
#undef IS_STRUCT_OPEN
#undef IS_STRUCT_CLOSE

#endif
