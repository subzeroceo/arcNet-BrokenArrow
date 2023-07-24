#include "/idlib/precompiled.h"
#pragma hdrstop


/*
=================
arcDeclEntityDef::Size
=================
*/
size_t arcDeclEntityDef::Size() const {
	return sizeof( arcDeclEntityDef ) + dict.Allocated();
}

/*
================
arcDeclEntityDef::FreeData
================
*/
void arcDeclEntityDef::FreeData() {
	dict.Clear();
}

/*
================
arcDeclEntityDef::Parse
================
*/
bool arcDeclEntityDef::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	arcLexer src;
	arcNetToken	token, token2;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	while (1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}
		if ( token.type != TT_STRING ) {
			src.Warning( "Expected quoted string, but found '%s'", token.c_str() );
			MakeDefault();
			return false;
		}

		if ( !src.ReadToken( &token2 ) ) {
			src.Warning( "Unexpected end of file" );
			MakeDefault();
			return false;
		}

		if ( dict.FindKey( token ) ) {
			src.Warning( "'%s' already defined", token.c_str() );
		}
		dict.Set( token, token2 );
	}

	// we always automatically set a "classname" key to our name
	dict.Set( "classname", GetName() );

	// "inherit" keys will cause all values from another entityDef to be copied into this one
	// if they don't conflict.  We can't have circular recursions, because each entityDef will
	// never be parsed mroe than once

	// find all of the dicts first, because copying inherited values will modify the dict
	arcNetList<const arcDeclEntityDef *> defList;

	while ( 1 ) {
		const idKeyValue *kv;
		kv = dict.MatchPrefix( "inherit", NULL );
		if ( !kv ) {
			break;
		}

		const arcDeclEntityDef *copy = static_cast<const arcDeclEntityDef *>( declManager->FindType( DECL_ENTITYDEF, kv->GetValue(), false ) );
		if ( !copy ) {
			src.Warning( "Unknown entityDef '%s' inherited by '%s'", kv->GetValue().c_str(), GetName() );
		} else {
			defList.Append( copy );
		}

		// delete this key/value pair
		dict.Delete( kv->GetKey() );
	}

	// now copy over the inherited key / value pairs
	for ( int i = 0; i < defList.Num(); i++ ) {
		dict.SetDefaults( &defList[ i ]->dict );
	}

	game->CacheDictionaryMedia( &dict );

	return true;
}

/*
================
arcDeclEntityDef::DefaultDefinition
================
*/
const char *arcDeclEntityDef::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"\"DEFAULTED\"\t\"1\"\n"
		"}";
}

/*
================
arcDeclEntityDef::Print

Dumps all key/value pairs, including inherited ones
================
*/
void arcDeclEntityDef::Print() {
	dict.Print();
}
