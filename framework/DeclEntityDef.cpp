#include "/idlib/Lib.h"
#pragma hdrstop


/*
=================
anDeclEntityDef::Size
=================
*/
size_t anDeclEntityDef::Size() const {
	return sizeof( anDeclEntityDef ) + dict.Allocated();
}

/*
================
anDeclEntityDef::FreeData
================
*/
void anDeclEntityDef::FreeData() {
	dict.Clear();
}

/*
================
anDeclEntityDef::Parse
================
*/
bool anDeclEntityDef::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	anLexer src;
	anToken	token, token2;

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
	anList<const anDeclEntityDef *> defList;

	while ( 1 ) {
		const anKeyValue *kv;
		kv = dict.MatchPrefix( "inherit", nullptr );
		if ( !kv ) {
			break;
		}

		const anDeclEntityDef *copy = static_cast<const anDeclEntityDef *>( declManager->FindType( DECL_ENTITYDEF, kv->GetValue(), false ) );
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
		dict.SetDefaults( &defList[i]->dict );
	}

	game->CacheDictionaryMedia( &dict );

	return true;
}

/*
================
anDeclEntityDef::DefaultDefinition
================
*/
const char *anDeclEntityDef::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"\"DEFAULTED\"\t\"1\"\n"
		"}";
}

/*
================
anDeclEntityDef::Print

Dumps all key/value pairs, including inherited ones
================
*/
void anDeclEntityDef::Print() {
	dict.Print();
}
