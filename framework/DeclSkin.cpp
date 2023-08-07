#include "/idlib/Lib.h"
#pragma hdrstop


/*
=================
anDeclSkin::Size
=================
*/
size_t anDeclSkin::Size() const {
	return sizeof( anDeclSkin );
}

/*
================
anDeclSkin::FreeData
================
*/
void anDeclSkin::FreeData() {
	mappings.Clear();
}

/*
================
anDeclSkin::Parse
================
*/
bool anDeclSkin::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	anLexer src;
	anToken	token, token2;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	associatedModels.Clear();

	while (1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}
		if ( !src.ReadToken( &token2 ) ) {
			src.Warning( "Unexpected end of file" );
			MakeDefault();
			return false;
		}

		if ( !token.Icmp( "model" ) ) {
			associatedModels.Append( token2 );
			continue;
		}

		skinMapping_t	map;

		if ( !token.Icmp( "*" ) ) {
			// wildcard
			map.from = nullptr;
		} else {
			map.from = declManager->FindMaterial( token );
		}

		map.to = declManager->FindMaterial( token2 );

		mappings.Append( map );
	}

	return false;
}

/*
================
anDeclSkin::SetDefaultText
================
*/
bool anDeclSkin::SetDefaultText() {
	// if there exists a material with the same name
	if ( declManager->FindType( DECL_MATERIAL, GetName(), false ) ) {
		char generated[2048];

		anStr::snPrintf( generated, sizeof( generated ),
						"skin %s // IMPLICITLY GENERATED\n"
						"{\n"
						"_default %s\n"
						"}\n", GetName(), GetName() );
		SetText( generated );
		return true;
	} else {
		return false;
	}
}

/*
================
anDeclSkin::DefaultDefinition
================
*/
const char *anDeclSkin::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"\"*\"\t\"_default\"\n"
		"}";
}

/*
================
anDeclSkin::GetNumModelAssociations
================
*/
const int anDeclSkin::GetNumModelAssociations(void ) const {
	return associatedModels.Num();
}

/*
================
anDeclSkin::GetAssociatedModel
================
*/
const char *anDeclSkin::GetAssociatedModel( int index ) const {
	if ( index >= 0 && index < associatedModels.Num() ) {
		return associatedModels[index];
	}
	return "";
}

/*
===============
RemapShaderBySkin
===============
*/
const anMaterial *anDeclSkin::RemapShaderBySkin( const anMaterial *shader ) const {
	int		i;

	if ( !shader ) {
		return nullptr;
	}

	// never remap surfaces that were originally nodraw, like collision hulls
	if ( !shader->IsDrawn() ) {
		return shader;
	}

	for ( i = 0; i < mappings.Num(); i++ ) {
		const skinMapping_t	*map = &mappings[i];

		// nullptr = wildcard match
		if ( !map->from || map->from == shader ) {
			return map->to;
		}
	}

	// didn't find a match or wildcard, so stay the same
	return shader;
}
