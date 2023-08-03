
#ifndef __DECLMATCLASS_H__
#define __DECLMATCLASS_H__

// Defines a material type - such as concrete, metal, glass etc
class anDeclSurfaceType : public arcDecl {
public:
						anDeclSurfaceType();
						anDeclSurfaceType( void ) { *(unsigned long *)mTint = 0; }
						~anDeclSurfaceType( void ) {}
// allow exporting of this decl type in a preparsed form
	virtual void		Write( SerialOutputStream &stream ) const;
	virtual void		AddReferences() const;

	void				SetDescription( anString &desc ) { mDescription = desc; }
	const anString &	GetDescription( void ) const { return( mDescription ); }

	void				SetTint( byte tint[4] ) { *(unsigned long *)mTint = *(unsigned long *)tint; }
	int					GetTint( void ) const { return ( *( int*)mTint ); }

	float				GetRed( void ) const { return ( mTint[0] / 255.0f ); }
	float				GetGreen( void ) const { return ( mTint[1] / 255.0f ); }
	float				GetBlue( void ) const { return ( mTint[2] / 255.0f ); }

	virtual const char	*DefaultDefinition( void ) const;
	virtual bool		Parse( const char *text, const int textLength );
	virtual void		FreeData( void );
	virtual size_t		Size( void ) const;
	virtual size_t		Size( void ) const { return sizeof( anDeclSurfaceType ); }

	int					GetNumImages( void ) const { return images.Num(); }
	const an2DBounds &	GetImage( int index ) const { return images[index]; }

	const anDict &		GetProperties( void ) const { return properties; }
	virtual	bool		RebuildTextSource( void ) { return false; }
	virtual bool		Validate( const char *psText, int iTextLength, anString &strReportTo ) const;

private:
	anString				mDescription;
	GLbyte					mTint[4];
	anString				type;
	anDict					properties;
	const anMaterial *		material;
	anList<an2DBounds>		images;
};

byte *ST_GetSurfaceTypeArray( anString image, int &width, int &height );


/*
=======================
anDeclSurfaceType::DefaultDefinition
=======================
*/
const char *anDeclSurfaceType::DefaultDefinition( void ) const {
	return "{ description \"<DEFAULTED>\" rgb 0,0,0 }";
}

/*
=======================
anDeclSurfaceType::Parse
=======================
*/
bool anDeclSurfaceType::Parse( const char *text, const int textLength ) {
	anLexer src;
	anToken token, token2;

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

#endif