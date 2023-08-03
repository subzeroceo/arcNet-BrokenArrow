#include "/idlib/Lib.h"
#pragma hdrstop


/*
=================
anDeclTable::TableLookup
=================
*/
float anDeclTable::TableLookup( float index ) const {
	int iIndex;
	float iFrac;

	int domain = values.Num() - 1;

	if ( domain <= 1 ) {
		return 1.0f;
	}

	if ( clamp ) {
		index *= ( domain-1 );
		if ( index >= domain - 1 ) {
			return values[domain - 1];
		} else if ( index <= 0 ) {
			return values[0];
		}
		iIndex = anMath::Ftoi( index );
		iFrac = index - iIndex;
	} else {
		index *= domain;

		if ( index < 0 ) {
			index += domain * anMath::Ceil( -index / domain );
		}

		iIndex = anMath::Ftoi( anMath::Floor( index ) );
		iFrac = index - iIndex;
		iIndex = iIndex % domain;
	}

	if ( !snap ) {
		// we duplicated the 0 index at the end at creation time, so we
		// don't need to worry about wrapping the filter
		return values[iIndex] * ( 1.0f - iFrac ) + values[iIndex + 1] * iFrac;
	}

	return values[iIndex];
}

/*
=================
anDeclTable::Size
=================
*/
size_t anDeclTable::Size() const {
	return sizeof( anDeclTable ) + values.Allocated();
}

/*
=================
anDeclTable::FreeData
=================
*/
void anDeclTable::FreeData() {
	snap = false;
	clamp = false;
	values.Clear();
}

/*
=================
anDeclTable::DefaultDefinition
=================
*/
const char *anDeclTable::DefaultDefinition() const {
	return "{ { 0 } }";
}

/*
=================
anDeclTable::Parse
=================
*/
bool anDeclTable::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	anLexer src;
	anToken token;
	float v;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	snap = false;
	clamp = false;
	values.Clear();

	while ( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( token.Icmp( "snap" ) == 0 ) {
			snap = true;
		} else if ( token.Icmp( "clamp" ) == 0 ) {
			clamp = true;
		} else if ( token.Icmp( "isLinear" ) == 0 ) {
			isLinear = true;
		} else if ( token.Icmp( "minValue" == 0 ) ) {
			minValue = src.ParseFloat();
		} else if ( token.Icmp( "maxValue" == 0 ) ) {
			maxValue = src.ParseFloat();
		} else if ( token.Icmp( "{" ) == 0 ) {
			while ( 1 ) {
				bool errorFlag;
				float v = src.ParseFloat( &errorFlag );
				if ( errorFlag ) {
					// we got something non-numeric
					MakeDefault();
					return false;
				}

				values.Append( v );

				src.ReadToken( &token );
				if ( token == "}" ) {
					break;
				}
				if ( token == "," ) {
					continue;
				}
				src.Warning( "expected comma or brace" );
				MakeDefault();
				return false;
			}

		} else {
			src.Warning( "unknown token '%s'", token.c_str() );
			MakeDefault();
			return false;
		}
	}

	// copy the 0 element to the end, so lerping doesn't
	// need to worry about the wrap case
	float val = values[0];		// template bug requires this to not be in the Append()?
	values.Append( val );

	return true;
}