
#ifndef __DECLMATCLASS_H__
#define __DECLMATCLASS_H__

// Defines a material type - such as concrete, metal, glass etc
class arcDeclSurfType : public arcDecl {
public:
						arcDeclSurfType();
						arcDeclSurfType( void ) { *( ulong *)mTint = 0; }
						~arcDeclSurfType( void ) {}
// allow exporting of this decl type in a preparsed form
	virtual void		Write( SerialOutputStream &stream ) const;
	virtual void		AddReferences() const;

	void				SetDescription( arcNetString &desc ) { mDescription = desc; }
	const arcNetString &GetDescription( void ) const { return( mDescription ); }

	void				SetTint( byte tint[4] ) { *( ulong *)mTint = *( ulong *)tint; }
	int					GetTint( void ) const { return ( *( int *)mTint ); }

	float				GetRed( void ) const { return ( mTint[0] / 255.0f ); }
	float				GetGreen( void ) const { return ( mTint[1] / 255.0f ); }
	float				GetBlue( void ) const { return ( mTint[2] / 255.0f ); }

	virtual const char	*DefaultDefinition( void ) const;
	virtual bool		Parse( const char *text, const int textLength );
	virtual void		FreeData( void );
	virtual size_t		Size( void ) const;
	virtual size_t		Size( void ) const { return sizeof( arcDeclSurfType ); }
	const arcDict		&GetProperties( void ) const { return properties; }
	virtual	bool		RebuildTextSource( void ) { return false; }
	virtual bool		Validate( const char *psText, int iTextLength, arcNetString &strReportTo ) const;

private:
	arcNetString				mDescription;
	byte						mTint[4];
	arcNetString				type;
	arcDict						properties;
};

byte *MT_GetMaterialTypeArray( arcNetString image, int &width, int &height );


/*
=======================
arcDeclSurfType::DefaultDefinition
=======================
*/
const char* arcDeclSurfType::DefaultDefinition( void ) const {
	return "{ description \"<DEFAULTED>\" rgb 0,0,0 }";
}

/*
=======================
arcDeclSurfType::Parse
=======================
*/
bool arcDeclSurfType::Parse( const char* text, const int textLength ) {
	arcLexer src;
	arcNetToken token, token2;

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
		} else if (token == "properties" ) {
			src.ReadToken( &token );
			properties = token;
		} else if (token == "type" ) {
			src.ReadToken( &token );
			type = token;
		} else if (token == "description" ) {
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
arcDeclSurfType::FreeData
=======================
*/
void arcDeclSurfType::FreeData( void ) {
}

#endif