#include "/idlib/precompiled.h"
#pragma hdrstop


/*
=================
arcDeclSkin::Size
=================
*/
size_t arcDeclSkin::Size() const {
	return sizeof( arcDeclSkin );
}

/*
================
arcDeclSkin::FreeData
================
*/
void arcDeclSkin::FreeData() {
	mappings.Clear();
}

/*
================
arcDeclSkin::Parse
================
*/
bool arcDeclSkin::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	arcLexer src;
	arcNetToken	token, token2;

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
			map.from = NULL;
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
arcDeclSkin::SetDefaultText
================
*/
bool arcDeclSkin::SetDefaultText() {
	// if there exists a material with the same name
	if ( declManager->FindType( DECL_MATERIAL, GetName(), false ) ) {
		char generated[2048];

		arcNetString::snPrintf( generated, sizeof( generated ),
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
arcDeclSkin::DefaultDefinition
================
*/
const char *arcDeclSkin::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"\"*\"\t\"_default\"\n"
		"}";
}

/*
================
arcDeclSkin::GetNumModelAssociations
================
*/
const int arcDeclSkin::GetNumModelAssociations(void ) const {
	return associatedModels.Num();
}

/*
================
arcDeclSkin::GetAssociatedModel
================
*/
const char *arcDeclSkin::GetAssociatedModel( int index ) const {
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
const arcMaterial *arcDeclSkin::RemapShaderBySkin( const arcMaterial *shader ) const {
	int		i;

	if ( !shader ) {
		return NULL;
	}

	// never remap surfaces that were originally nodraw, like collision hulls
	if ( !shader->IsDrawn() ) {
		return shader;
	}

	for ( i = 0; i < mappings.Num(); i++ ) {
		const skinMapping_t	*map = &mappings[i];

		// NULL = wildcard match
		if ( !map->from || map->from == shader ) {
			return map->to;
		}
	}

	// didn't find a match or wildcard, so stay the same
	return shader;
}
