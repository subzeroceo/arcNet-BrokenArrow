#ifndef __DECLPDA_H__
#define __DECLPDA_H__

/*
===============================================================================

	arcDeclPDA

===============================================================================
*/


class arcDeclEmail : public arcDecleration {
public:
							arcDeclEmail() {}

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
	arcNetString					text;
	arcNetString					subject;
	arcNetString					date;
	arcNetString					to;
	arcNetString					from;
};


class arcDeclVideo : public arcDecleration {
public:
							arcDeclVideo() : preview( NULL ), video( NULL ), audio( NULL ) {};

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const arcMaterial *		GetRoq() const { return video; }
	const arcSoundShader *	GetWave() const { return audio; }
	const char *			GetVideoName() const { return videoName; }
	const char *			GetInfo() const { return info; }
	const arcMaterial *		GetPreview() const { return preview; }

private:
	const arcMaterial *		preview;
	const arcMaterial *		video;
	arcNetString					videoName;
	arcNetString					info;
	const arcSoundShader *	audio;
};


class arcDeclAudio : public arcDecleration {
public:
							arcDeclAudio() : audio( NULL ) {};

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const char *			GetAudioName() const { return audioName; }
	const arcSoundShader *	GetWave() const { return audio; }
	const char *			GetInfo() const { return info; }

private:
	const arcSoundShader *	audio;
	arcNetString					audioName;
	arcNetString					info;
};

class arcDeclPDA : public arcDecleration {
public:
							arcDeclPDA() { originalEmails = originalVideos = 0; };

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	virtual void			AddVideo( const arcDeclVideo * video, bool unique = true ) const { if ( unique ) { videos.AddUnique( video ); } else { videos.Append( video ); } }
	virtual void			AddAudio( const arcDeclAudio * audio, bool unique = true ) const { if ( unique ) { audios.AddUnique( audio ); } else { audios.Append( audio ); } }
	virtual void			AddEmail( const arcDeclEmail * email, bool unique = true ) const { if ( unique ) { emails.AddUnique( email ); } else { emails.Append( email ); } }
	virtual void			RemoveAddedEmailsAndVideos() const;

	virtual const int		GetNumVideos() const { return videos.Num(); }
	virtual const int		GetNumAudios() const { return audios.Num(); }
	virtual const int		GetNumEmails() const { return emails.Num(); }
	virtual const arcDeclVideo *GetVideoByIndex( int index ) const { return ( index < 0 || index > videos.Num() ? NULL : videos[index] ); }
	virtual const arcDeclAudio *GetAudioByIndex( int index ) const { return ( index < 0 || index > audios.Num() ? NULL : audios[index] ); }
	virtual const arcDeclEmail *GetEmailByIndex( int index ) const { return ( index < 0 || index > emails.Num() ? NULL : emails[index] ); }

	virtual void			SetSecurity( const char *sec ) const;

	const char *			GetPdaName() const { return pdaName; }
	const char *			GetSecurity() const {return security; }
	const char *			GetFullName() const { return fullName; }
	const char *			GetIcon() const { return icon; }
	const char *			GetPost() const { return post; }
	const char *			GetID() const { return id; }
	const char *			GetTitle() const { return title; }

private:
	mutable arcNetList<const arcDeclVideo *>	videos;
	mutable arcNetList<const arcDeclAudio *>	audios;
	mutable arcNetList<const arcDeclEmail *>	emails;
	arcNetString					pdaName;
	arcNetString					fullName;
	arcNetString					icon;
	arcNetString					id;
	arcNetString					post;
	arcNetString					title;
	mutable arcNetString			security;
	mutable	int				originalEmails;
	mutable int				originalVideos;
};

#endif /* !__DECLPDA_H__ */
