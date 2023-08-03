#ifndef __DECLPDA_H__
#define __DECLPDA_H__

/*
===============================================================================

	anDeclPDA

===============================================================================
*/


class anDeclEmail : public anDecl {
public:
							anDeclEmail() {}

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const char *			GetFrom() const { return from; }
	const char *			GetBody() const { return text; }
	const char *			GetSubject() const { return subject; }
	const char *			GetDate() const { return date; }
	const char *			GetTo() const { return to; }

private:
	anString					text;
	anString					subject;
	anString					date;
	anString					to;
	anString					from;
};


class anDeclVideo : public anDecl {
public:
							anDeclVideo() : preview( nullptr ), video( nullptr ), audio( nullptr ) {};

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const anMaterial *		GetRoq() const { return video; }
	const anSoundShader *	GetWave() const { return audio; }
	const char *			GetVideoName() const { return videoName; }
	const char *			GetInfo() const { return info; }
	const anMaterial *		GetPreview() const { return preview; }

private:
	const anMaterial *		preview;
	const anMaterial *		video;
	anString					videoName;
	anString					info;
	const anSoundShader *	audio;
};


class anDeclAudio : public anDecl {
public:
							anDeclAudio() : audio( nullptr ) {};

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const char *			GetAudioName() const { return audioName; }
	const anSoundShader *	GetWave() const { return audio; }
	const char *			GetInfo() const { return info; }

private:
	const anSoundShader *	audio;
	anString					audioName;
	anString					info;
};

class anDeclPDA : public anDecl {
public:
							anDeclPDA() { originalEmails = originalVideos = 0; };

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	virtual void			AddVideo( const anDeclVideo * video, bool unique = true ) const { if ( unique ) { videos.AddUnique( video ); } else { videos.Append( video ); } }
	virtual void			AddAudio( const anDeclAudio * audio, bool unique = true ) const { if ( unique ) { audios.AddUnique( audio ); } else { audios.Append( audio ); } }
	virtual void			AddEmail( const anDeclEmail * email, bool unique = true ) const { if ( unique ) { emails.AddUnique( email ); } else { emails.Append( email ); } }
	virtual void			RemoveAddedEmailsAndVideos() const;

	virtual const int		GetNumVideos() const { return videos.Num(); }
	virtual const int		GetNumAudios() const { return audios.Num(); }
	virtual const int		GetNumEmails() const { return emails.Num(); }
	virtual const anDeclVideo *GetVideoByIndex( int index ) const { return ( index < 0 || index > videos.Num() ? nullptr : videos[index] ); }
	virtual const anDeclAudio *GetAudioByIndex( int index ) const { return ( index < 0 || index > audios.Num() ? nullptr : audios[index] ); }
	virtual const anDeclEmail *GetEmailByIndex( int index ) const { return ( index < 0 || index > emails.Num() ? nullptr : emails[index] ); }

	virtual void			SetSecurity( const char *sec ) const;

	const char *			GetPdaName() const { return pdaName; }
	const char *			GetSecurity() const {return security; }
	const char *			GetFullName() const { return fullName; }
	const char *			GetIcon() const { return icon; }
	const char *			GetPost() const { return post; }
	const char *			GetID() const { return id; }
	const char *			GetTitle() const { return title; }

private:
	mutable anList<const anDeclVideo *>	videos;
	mutable anList<const anDeclAudio *>	audios;
	mutable anList<const anDeclEmail *>	emails;
	anString					pdaName;
	anString					fullName;
	anString					icon;
	anString					id;
	anString					post;
	anString					title;
	mutable anString			security;
	mutable	int				originalEmails;
	mutable int				originalVideos;
};

#endif /* !__DECLPDA_H__ */
