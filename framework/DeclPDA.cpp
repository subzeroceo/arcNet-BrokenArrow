#include "/idlib/Lib.h"
#pragma hdrstop

anCVarSystem g_useOldPDAStrings( "g_useOldPDAStrings", "0", CVAR_BOOL, "Read strings from the .pda files rather than from the .lang file" );

/*
=================
anDeclPDA::Size
=================
*/
size_t anDeclPDA::Size() const {
	return sizeof( anDeclPDA );
}

/*
===============
anDeclPDA::Print
===============
*/
void anDeclPDA::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
anDeclPDA::List
===============
*/
void anDeclPDA::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
anDeclPDA::Parse
================
*/
bool anDeclPDA::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	anLexer src;
	anToken token;

	anString baseStrId = va( "#str_%s_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
	while( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "name" ) ) {
			src.ReadToken( &token );

			if ( g_useOldPDAStrings.GetBool() ) {
				pdaName = token;
			} else {
				pdaName = ARCLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if ( !token.Icmp( "fullname" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				fullName = token;
			} else {
				fullName = ARCLocalization::GetString( baseStrId + "fullname" );
			}
			continue;
		}

		if ( !token.Icmp( "icon" ) ) {
			src.ReadToken( &token );
			icon = token;
			continue;
		}

		if ( !token.Icmp( "_id" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				id = token;
			} else {
				id = ARCLocalization::GetString( baseStrId + "id" );
			}
			continue;
		}

		if ( !token.Icmp( "_post" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				post = token;
			} else {
				post = ARCLocalization::GetString( baseStrId + "post" );
			}
			continue;
		}

		if ( !token.Icmp( "_title" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				title = token;
			} else {
				title = ARCLocalization::GetString( baseStrId + "title" );
			}
			continue;
		}

		if ( !token.Icmp( "_security" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				security = token;
			} else {
				security = ARCLocalization::GetString( baseStrId + "security" );
			}
			continue;
		}

		if ( !token.Icmp( "email" ) ) {
			src.ReadToken( &token );
			emails.Append( static_cast<const anDeclEmail *>( declManager->FindType( DECL_EMAIL, token ) ) );
			continue;
		}

		if ( !token.Icmp( "_audio" ) ) {
			src.ReadToken( &token );
			audios.Append( static_cast<const anDeclAudio *>( declManager->FindType( DECL_AUDIO, token ) ) );
			continue;
		}

		if ( !token.Icmp( "_video" ) ) {
			src.ReadToken( &token );
			videos.Append( static_cast<const anDeclVideo *>( declManager->FindType( DECL_VIDEO, token ) ) );
			continue;
		}

	}

	if ( src.HadError() ) {
		src.Warning( " decl '%s' had a parse error", GetName() );
		return false;
	}

	originalVideos = videos.Num();
	originalEmails = emails.Num();
	return true;
}

/*
===================
anDeclPDA::DefaultDefinition
===================
*/
const char *anDeclPDA::DefaultDefinition() const {
	return
		"{\n"
		"\t"		"name  \"default\"\n"
		"}";
}

/*
===================
anDeclPDA::FreeData
===================
*/
void anDeclPDA::FreeData() {
	videos.Clear();
	audios.Clear();
	emails.Clear();
	originalEmails = 0;
	originalVideos = 0;
}

/*
=================
anDeclPDA::RemoveAddedEmailsAndVideos
=================
*/
void anDeclPDA::RemoveAddedEmailsAndVideos() const {
	int num = emails.Num();
	if ( originalEmails < num ) {
		while ( num && num > originalEmails ) {
			emails.RemoveIndex( --num );
		}
	}
	num = videos.Num();
	if ( originalVideos < num ) {
		while ( num && num > originalVideos ) {
			videos.RemoveIndex( --num );
		}
	}
}

/*
=================
anDeclPDA::SetSecurity
=================
*/
void anDeclPDA::SetSecurity( const char *sec ) const {
	security = sec;
}

/*
=================
anDeclEmail::Size
=================
*/
size_t anDeclEmail::Size() const {
	return sizeof( anDeclEmail );
}

/*
===============
anDeclEmail::Print
===============
*/
void anDeclEmail::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
anDeclEmail::List
===============
*/
void anDeclEmail::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
anDeclEmail::Parse
================
*/
bool anDeclEmail::Parse( const char *_text, const int textLength, bool allowBinaryVersion ) {
	anLexer src;
	anToken token;

	anString baseStrId = va( "#str_%s_email_", GetName() );

	src.LoadMemory( _text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	text = "";
	// scan through, identifying each individual parameter
	while( 1 ) {

		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "subject" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				subject = token;
			} else {
				subject = ARCLocalization::GetString( baseStrId + "subject" );
			}
			continue;
		}

		if ( !token.Icmp( "to" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				to = token;
			} else {
				to = ARCLocalization::GetString( baseStrId + "to" );
			}
			continue;
		}

		if ( !token.Icmp( "from" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				from = token;
			} else {
				from = ARCLocalization::GetString( baseStrId + "from" );
			}
			continue;
		}

		if ( !token.Icmp( "date" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				date = token;
			} else {
				date = ARCLocalization::GetString( baseStrId + "date" );
			}
			continue;
		}

		if ( !token.Icmp( "text" ) ) {
			src.ReadToken( &token );
			if ( token != "{" ) {
				src.Warning( "Email decl '%s' had a parse error", GetName() );
				return false;
			}
			while ( src.ReadToken( &token ) && token != "}" ) {
				text += token;
			}
			if ( !g_useOldPDAStrings.GetBool() ) {
				text = ARCLocalization::GetString( baseStrId + "text" );
			}
			continue;
		}
	}

	if ( src.HadError() ) {
		src.Warning( "Email decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
anDeclEmail::DefaultDefinition
===================
*/
const char *anDeclEmail::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"to\t5Mail recipient\n"
		"\t\t"		"subject\t5Nothing\n"
		"\t\t"		"from\t5No one\n"
		"\t"	"}\n"
		"}";
}

/*
===================
anDeclEmail::FreeData
===================
*/
void anDeclEmail::FreeData() {
}

/*
=================
anDeclVideo::Size
=================
*/
size_t anDeclVideo::Size() const {
	return sizeof( anDeclVideo );
}

/*
===============
anDeclVideo::Print
===============
*/
void anDeclVideo::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
anDeclVideo::List
===============
*/
void anDeclVideo::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
anDeclVideo::Parse
================
*/
bool anDeclVideo::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	anLexer src;
	anToken token;

	anString baseStrId = va( "#str_%s_video_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
	while( 1 ) {

		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "name" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				videoName = token;
			} else {
				videoName = ARCLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if ( !token.Icmp( "preview" ) ) {
			src.ReadToken( &token );
			preview = declManager->FindMaterial( token );
			continue;
		}

		if ( !token.Icmp( "video" ) ) {
			src.ReadToken( &token );
			video = declManager->FindMaterial( token );
			continue;
		}

		if ( !token.Icmp( "info" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				info = token;
			} else {
				info = ARCLocalization::GetString( baseStrId + "info" );
			}
			continue;
		}

		if ( !token.Icmp( "audio" ) ) {
			src.ReadToken( &token );
			audio = declManager->FindSound( token );
			continue;
		}

	}

	if ( src.HadError() ) {
		src.Warning( "Video decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
anDeclVideo::DefaultDefinition
===================
*/
const char *anDeclVideo::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"name\t5Default Video\n"
		"\t"	"}\n"
		"}";
}

/*
===================
anDeclVideo::FreeData
===================
*/
void anDeclVideo::FreeData() {
}

/*
=================
anDeclAudio::Size
=================
*/
size_t anDeclAudio::Size() const {
	return sizeof( anDeclAudio );
}

/*
===============
anDeclAudio::Print
===============
*/
void anDeclAudio::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
anDeclAudio::List
===============
*/
void anDeclAudio::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
anDeclAudio::Parse
================
*/
bool anDeclAudio::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	anLexer src;
	anToken token;

	anString baseStrId = va( "#str_%s_audio_", GetName() );

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES |	LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_NOFATALERRORS );
	src.SkipUntilString( "{" );

	// scan through, identifying each individual parameter
	while( 1 ) {

		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( token == "}" ) {
			break;
		}

		if ( !token.Icmp( "name" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				audioName = token;
			} else {
				audioName = ARCLocalization::GetString( baseStrId + "name" );
			}
			continue;
		}

		if ( !token.Icmp( "audio" ) ) {
			src.ReadToken( &token );
			audio = declManager->FindSound( token );
			continue;
		}

		if ( !token.Icmp( "info" ) ) {
			src.ReadToken( &token );
			if ( g_useOldPDAStrings.GetBool() ) {
				info = token;
			} else {
				info = ARCLocalization::GetString( baseStrId + "info" );
			}
			continue;
		}
	}

	if ( src.HadError() ) {
		src.Warning( "Audio decl '%s' had a parse error", GetName() );
		return false;
	}
	return true;
}

/*
===================
anDeclAudio::DefaultDefinition
===================
*/
const char *anDeclAudio::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"name\t5Default Audio\n"
		"\t"	"}\n"
		"}";
}

/*
===================
anDeclAudio::FreeData
===================
*/
void anDeclAudio::FreeData() {
}
