// Defines a material type - such as concrete, metal, glass etc
#ifdef BINARYDECLS

/*
Define the allowed and designated colors: Determine the set of allowed colors that users can pick from when
editing the texture file. These colors will correspond to specific surface types. You can define an enum or
a mapping between colors and surface types.
Defines a material type - such as concrete, metal, glass etc with the appropiate interaction(s).
Implement color scanning and mapping: When scanning the texture map, compare each pixel's color to the allowed
and designated colors. If a match is found, associate it with the corresponding surface type. You can store
this information in a data structure or use it directly for further processing, such as sound generation or
interaction behavior.

Utilize the surface type information: With the surface type information available for each pixel in the
texture map, you can use it to determine how objects or entities should behave when interacting with different
surface types. This can include sound effects, decal projection, physics interactions, or any other desired behavior.

Read the def file: Parse the def file to determine the assigned surface type for each object. This information will
define how objects behave and interact with the environment based on the assigned surface type.

By following these steps, this  will incorporate the color scanning and mapping functionality,
enabling objects to respond based on the designated surface types.

TODO: allow exporting of this decl type in a preparsed form as well as binary
*/
void Write( SerialOutputStream &stream ) const {}
arcDeclSurfType( SerialInputStream &stream ) {}
void		AddReferences() const {}

/*
=======================
arcDeclSurfType::Parse
=======================
*/
bool arcDeclSurfType::Parse( const char *text, const int textLength ) {
	arcLexer src;
	arcNetToken token, token2;

	//type = typeName.GetBinaryIndex();

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	while ( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" )) {
			break;
		} else if (token == "rgb" ) {
			mTint[0] = src.ParseInt();
			src.ExpectTokenString( "," );
			mTint[1] = src.ParseInt();
			src.ExpectTokenString( "," );
			mTint[2] = src.ParseInt();
		} else if ( token == "properties" ) {
			src.ReadToken( &token );
			properties = token;
		} else if ( token == "type" ) {
			src.ReadToken( &token );
			type = token;
		} else if ( token == "description" ) {
			src.ReadToken( &token );
			mDescription = token;
			continue;
		} else {
			src.Error( "[MST Parse] Invalid or unexpected token %s\n", token.c_str() );
			return false;
		}
	}
	return true;
}

/*
=======================
arcDeclSurfType::DefaultDefinition
=======================
*/
const char* arcDeclSurfType::DefaultDefinition( void ) const {
	return "{ namedescription n\"<DEFAULTED>\" rgb 0,0,0 }";
}

size_t arcDeclSurfType::Size( void ) const {

}

/*
=======================
arcDeclSurfType::FreeData
=======================
*/
void arcDeclSurfType::FreeData( void ) {
	if ( m_ImageData ) {
		delete [] m_ImageData;
		m_ImageData = nullptr;
	}
}

/*
=======================
arcDeclSurfType::FreeData
=======================
*/
bool arcDeclSurfType::Validate( const char *psText, int iTextLength, arcNetString &strReportTo ) const {
	// Validate text according to business logic
	bool valid = true;
	if ( iTextLength < 10 ) {
		valid = false;
		strReportTo = "Text too short";
	}

}

/*
=======================
arcDeclSurfType::FreeDMT_GetMaterialTypeArrayata
=======================
*/
byte *arcDeclSurfType::MT_GetMaterialTypeArray( arcNetString image, int &width, int &height ) {
	// Load image at path
	MyImage image( imagePath );

	// Get width and height
	width = image.GetWidth();
	height = image.GetHeight();

	// Extract material type data into byte array
	byte* types = new byte[width * height];
	 Populate types array...

	return types;
}

// Parses a hexadecimal color code and returns the corresponding RGBA values.
// return The RGBA values corresponding to the hexadecimal color code.
// example of how to use this parsing code for the rest of the codebase if needed:
// const char *hex_code = "#FFA500"
// int rgba = R_ParseHexDecimals(hex_code)
// print message log here:("(%d, %d, %d, %d)\n", rgba >> 24 & 0xFF, rgba >> 16 & 0xFF, rgba >> 8 & 0xFF, rgba & 0xFF)
//
void R_ParseHexDecimals( const char *hDecimal ) {
    const char *rgba = hDecimal;
    int length = strlen( hDecimal );

    if ( hDecimal[0] == '#' )
        rgba++;

    if ( length == 3 ) { // RGB format without alpha
        int r = R_HexDecimals( rgba[0] ) * 16 + R_HexDecimals( rgba[0] );
        int g = R_HexDecimals( rgba[1] ) * 16 + R_HexDecimals( rgba[1] );
        int b = R_HexDecimals( rgba[2] ) * 16 + R_HexDecimals( rgba[2] );
        return ( r, g, b );
    } else if ( length == 4) { // RGBA format with alpha
        int r = R_HexDecimals( rgba[0] ) * 16 + R_HexDecimals( rgba[0] );
        int g = R_HexDecimals( rgba[1] ) * 16 + R_HexDecimals( rgba[1] );
        int b = R_HexDecimals( rgba[2] ) * 16 + R_HexDecimals( rgba[2] );
        int a = R_HexDecimals( rgba[3] ) * 16 + R_HexDecimals( rgba[3] );
        return ( r, g, b, a );
    } else if ( length == 6 ) { // RGB format without alpha
        int r = ( R_HexDecimals( rgba[0] ) * 16 + R_HexDecimals( rgba[1] ) );
        int g = ( R_HexDecimals( rgba[2] ) * 16 + R_HexDecimals( rgba[3] ) );
        int b = ( R_HexDecimals( rgba[4] ) * 16 + R_HexDecimals( rgba[5] ) );
        return ( r, g, b );
    } else if ( length == 8 ) { // RGBA format with alpha
        int r = ( R_HexDecimals( rgba[0] ) * 16 + R_HexDecimals( rgba[1] ) );
        int g = ( R_HexDecimals( rgba[2] ) * 16 + R_HexDecimals( rgba[3] ) );
        int b = ( R_HexDecimals( rgba[4] ) * 16 + R_HexDecimals( rgba[5] ) );
        int a = ( R_HexDecimals( rgba[6] ) * 16 + R_HexDecimals( rgba[7] ) );
        return ( r, g, b, a );
    } else {
        throw RB_LogComment( "Invalid hex code format." );
    }
	common->Printf( "(%d, %d, %d, %d)\n", rgba >> 24 & 0xFF, rgba >> 16 & 0xFF, rgba >> 8 & 0xFF, rgba & 0xFF );
}

/*
===============
R_HexDecimals
===============
*/
static int R_HexDecimals( char c ) {
	if ( c >= '0' && c <= '9' ) {
		return c - '0';
	}

	if ( c >= 'A' && c <= 'F' ) {
		return 10 + c - 'A';
	}

	if ( c >= 'a' && c <= 'f' ) {
		return 10 + c - 'a';
	}

	return -1;
}
