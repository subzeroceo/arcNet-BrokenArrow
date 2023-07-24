#include "/idlib/precompiled.h"
#pragma hdrstop

arcCVarSystem g_useOldPDAStrings( "g_useOldPDAStrings", "0", CVAR_BOOL, "Read strings from the .pda files rather than from the .lang file" );

/*
=================
arcDeclPDA::Size
=================
*/
size_t arcDeclPDA::Size() const {
	return sizeof( arcDeclPDA );
}

/*
===============
arcDeclPDA::Print
===============
*/
void arcDeclPDA::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
arcDeclPDA::List
===============
*/
void arcDeclPDA::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
arcDeclPDA::Parse
================
*/
bool arcDeclPDA::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	arcLexer src;
	arcNetToken token;

	arcNetString baseStrId = va( "#str_%s_", GetName() );

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
			emails.Append( static_cast<const arcDeclEmail *>( declManager->FindType( DECL_EMAIL, token ) ) );
			continue;
		}

		if ( !token.Icmp( "_audio" ) ) {
			src.ReadToken( &token );
			audios.Append( static_cast<const arcDeclAudio *>( declManager->FindType( DECL_AUDIO, token ) ) );
			continue;
		}

		if ( !token.Icmp( "_video" ) ) {
			src.ReadToken( &token );
			videos.Append( static_cast<const arcDeclVideo *>( declManager->FindType( DECL_VIDEO, token ) ) );
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
arcDeclPDA::DefaultDefinition
===================
*/
const char *arcDeclPDA::DefaultDefinition() const {
	return
		"{\n"
		"\t"		"name  \"default\"\n"
		"}";
}

/*
===================
arcDeclPDA::FreeData
===================
*/
void arcDeclPDA::FreeData() {
	videos.Clear();
	audios.Clear();
	emails.Clear();
	originalEmails = 0;
	originalVideos = 0;
}

/*
=================
arcDeclPDA::RemoveAddedEmailsAndVideos
=================
*/
void arcDeclPDA::RemoveAddedEmailsAndVideos() const {
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
arcDeclPDA::SetSecurity
=================
*/
void arcDeclPDA::SetSecurity( const char *sec ) const {
	security = sec;
}

/*
=================
arcDeclEmail::Size
=================
*/
size_t arcDeclEmail::Size() const {
	return sizeof( arcDeclEmail );
}

/*
===============
arcDeclEmail::Print
===============
*/
void arcDeclEmail::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
arcDeclEmail::List
===============
*/
void arcDeclEmail::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
arcDeclEmail::Parse
================
*/
bool arcDeclEmail::Parse( const char *_text, const int textLength, bool allowBinaryVersion ) {
	arcLexer src;
	arcNetToken token;

	arcNetString baseStrId = va( "#str_%s_email_", GetName() );

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
arcDeclEmail::DefaultDefinition
===================
*/
const char *arcDeclEmail::DefaultDefinition() const {
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
arcDeclEmail::FreeData
===================
*/
void arcDeclEmail::FreeData() {
}

/*
=================
arcDeclVideo::Size
=================
*/
size_t arcDeclVideo::Size() const {
	return sizeof( arcDeclVideo );
}

/*
===============
arcDeclVideo::Print
===============
*/
void arcDeclVideo::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
arcDeclVideo::List
===============
*/
void arcDeclVideo::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
arcDeclVideo::Parse
================
*/
bool arcDeclVideo::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	arcLexer src;
	arcNetToken token;

	arcNetString baseStrId = va( "#str_%s_video_", GetName() );

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
arcDeclVideo::DefaultDefinition
===================
*/
const char *arcDeclVideo::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"name\t5Default Video\n"
		"\t"	"}\n"
		"}";
}

/*
===================
arcDeclVideo::FreeData
===================
*/
void arcDeclVideo::FreeData() {
}

/*
=================
arcDeclAudio::Size
=================
*/
size_t arcDeclAudio::Size() const {
	return sizeof( arcDeclAudio );
}

/*
===============
arcDeclAudio::Print
===============
*/
void arcDeclAudio::Print() const {
	common->Printf( "Implement me\n" );
}

/*
===============
arcDeclAudio::List
===============
*/
void arcDeclAudio::List() const {
	common->Printf( "Implement me\n" );
}

/*
================
arcDeclAudio::Parse
================
*/
bool arcDeclAudio::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	arcLexer src;
	arcNetToken token;

	arcNetString baseStrId = va( "#str_%s_audio_", GetName() );

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
arcDeclAudio::DefaultDefinition
===================
*/
const char *arcDeclAudio::DefaultDefinition() const {
	return
		"{\n"
		"\t"	"{\n"
		"\t\t"		"name\t5Default Audio\n"
		"\t"	"}\n"
		"}";
}

/*
===================
arcDeclAudio::FreeData
===================
*/
void arcDeclAudio::FreeData() {
}
