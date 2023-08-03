// Defines a material type - such as concrete, metal, glass etc
#ifdef BINARYDECLS

/*
Define the allowed and designated colors: Determine the set of allowed colors that users can pick from when
editing the texture file. These colors will correspond to specific surface types. You can define an enum or
a mapping between colors and surface types.
Defines a material type - such as concrete, metal, glass etc with the appropiate interaction( s).
Implement color scanning and mapping: When scanning the texture map, compare each pixel's color to the allowed
and designated colors. If a match is found, associate it with the corresponding surface type. You can store
this information in a data structure or use it directly for further processing, such as sound generation or
interaction behavior.

Utilize the surface type information: With the surface type information available for each pixel in the
texture map, you can use it to determine how objects or entities should behave when interacting with different
surface types. This can include sound effects, decal projection, physics interactions, or any other desired behavior.

Define Allowed Colors: Determine a set of allowed colors that users can choose
from when editing the texture file. Each color will correspond to a specific surface type.
This can be done using an enum or a mapping between colors and surface types.

Implement Color Scanning and Mapping: When scanning the texture map,
compare each pixel's color to the allowed colors defined in the previous step.
If a match is found, associate it with the corresponding surface type.
This mapping information can be stored in a data structure or used directly for further processing,
such as generating sound effects or defining interaction behavior.

Utilize Surface Type Information: With the surface type information available for each pixel
in the texture map, you can determine how objects or entities should behave when interacting
with different surface types. This can include applying specific sound effects,
projecting decals, defining physics interactions, or any other desired behavior
based on the assigned surface type.

Read the Definition File: Parse a definition file (likely in a specific format) to determine
the assigned surface type for each object. This information will define how objects behave
and interact with the environment based on their assigned surface type.

By following these steps, the anDeclSurfaceType class can incorporate color scanning and mapping functionality,
allowing objects to respond based on the designated surface types.

TODO: allow exporting of this decl type in a preparsed form as well as binary
*/
void Write( SerialOutputStream &stream ) const {}
anDeclSurfaceType( SerialInputStream &stream ) {}
void		AddReferences() const {}

/*
=======================
anDeclSurfaceType::Parse
=======================
*/
bool anDeclSurfaceType::Parse( const char *text, const int textLength ) {
	anLexer src;
	anToken token, token2;

	//type = typeName.GetBinaryIndex();

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	while ( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		} else if (token == "rgb" ) {
			mTint[0] = src.ParseInt();
			src.ExpectTokenString( "," );
			mTint[1] = src.ParseInt();
			src.ExpectTokenString( "," );
			mTint[2] = src.ParseInt();
			//stage->color[0] = src.ParseFloat();
			//stage->color[1] = src.ParseFloat();
			//stage->color[2] = src.ParseFloat();
			//stage->color[3] = src.ParseFloat();
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
anDeclSurfaceType::DefaultDefinition
=======================
*/
const char *anDeclSurfaceType::DefaultDefinition( void ) const {
	return "{ name n\"<DEFAULTED>\" description n\"<DEFAULTED>\" rgb 0,0,0 }";
}

size_t anDeclSurfaceType::Size( void ) const {

}

/*
=======================
anDeclSurfaceType::FreeData
=======================
*/
void anDeclSurfaceType::FreeData( void ) {
	if ( images ) {
		images [] images;
		images = nullptr;
	}
}

/*
=======================
anDeclSurfaceType::FreeData
=======================
*/
bool anDeclSurfaceType::Validate( const char *psText, int iTextLength, anString &strReportTo ) const {
	// Validate text according to business logic
	bool valid = true;
	if ( iTextLength < 10 ) {
		valid = false;
		strReportTo = "Text too short";
	}
}

/*
=======================
anDeclSurfaceType::GetMaterialTypeArrayata
=======================
*/
byte *anDeclSurfaceType::ST_GetSurfaceTypeArray( anString image, int &width, int &height ) {
	// Load image at path
	MyImage image( imagePath );

	// Get width and height
	width = image.GetWidth();
	height = image.GetHeight();

	// Extract material type data into byte array
	byte *types = new byte[width * height];
	 Populate types array...

	return types;
}

std::vector<anDeclSurfaceType> ST_GetSurfaceTypeArray( const anString& image, int& width, int& height, const std::vector<Color>& colors ) {
    // Load the image and retrieve the width and height
    // ... (implementation specific to your codebase)

    // Get the pixel colors from the image
    mTint = GetPixelColors( image, width, height );

    // Create an array of anDeclSurfaceType objects
    std::vector<anDeclSurfaceType> surfaceTypeArray;

    // Map each pixel color to the corresponding surface type object
    for ( int i = 0; i < pixelColors.size(); ++i) {
        const Color& pixelColor = pixelColors[i];

        // Find the corresponding surface type based on the pixel color
        anDeclSurfaceType surfaceType;

        // Set the color tint
        surfaceType.mTint[0] = pixelColor.GetRed();
        surfaceType.mTint[1] = pixelColor.GetGreen();
        surfaceType.mTint[2] = pixelColor.GetBlue();

        // Find the corresponding surface type based on the pixel color
        for ( intj = 0; j < colors.size(); ++j) {
            if (pixelColor == colors[j]) {
                surfaceType.type = anString::Format( "SurfaceType%d", j + 1);
                break;
            }
        }

        // Add the surface type object to the array
        surfaceTypeArray.push_back( surfaceType);
    }

    return surfaceTypeArray;
}

byte* ST_GetSurfaceTypeArray(const anString& image, int& width, int& height, const std::vector<Color>& colors) {
    // Load the image and retrieve the width and height
    // ... (implementation specific to your codebase)

    // Create the surface type array
    byte* surfaceTypeArray = new byte[width * height];

    // Get the pixel colors from the image
    std::vector<Color> pixelColors = GetPixelColors(image, width, height);

    // Map each pixel color to the corresponding surface type byte
    for ( int i = 0; i < pixelColors.size(); ++i) {
        const Color& pixelColor = pixelColors[i];

        // Find the corresponding surface type based on the pixel color
        byte surfaceType = 0; // Default surface type if no match is found
        for ( intj = 0; j < colors.size(); ++j) {
            if (pixelColor == colors[j]) {
                surfaceType = j + 1; // Assign surface type as index + 1
                break;
            }
        }

        // Assign the surface type to the corresponding pixel in the array
        surfaceTypeArray[i] = surfaceType;
    }

    return surfaceTypeArray;
}

// Parses a hexadecimal color code and returns the corresponding RGBA values.
// return The RGBA values corresponding to the hexadecimal color code.
// example of how to use this parsing code for the rest of the codebase if needed:
// const char *hex_code = "#FFA500"
// int rgba = R_ParseHexDecimals(hex_code)
// print message log here:( "(%d, %d, %d, %d)\n", rgba >> 24 & 0xFF, rgba >> 16 & 0xFF, rgba >> 8 & 0xFF, rgba & 0xFF)
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
