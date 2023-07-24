#ifndef __DEMOFILE_H__
#define __DEMOFILE_H__

/*
===============================================================================

	Demo file

===============================================================================
*/

typedef enum {
	DS_FINISHED,
	DS_RENDER,
	DS_SOUND,
	DS_VERSION
} demoSystem_t;

class ARCDemoFile : public arcNetFile {
public:
					ARCDemoFile();
					~ARCDemoFile();

	const char *	GetName() { return (f?f->GetName():"" ); }
	const char *	GetFullPath() { return (f?f->GetFullPath():"" ); }

	void			SetLog( bool b, const char *p );
	void			Log( const char *p );
	bool			OpenForReading( const char *fileName );
	bool			OpenForWriting( const char *fileName );
	void			Close();

	const char *	ReadHashString();
	void			WriteHashString( const char *str );

	void			ReadDict( arcDictionary &dict );
	void			WriteDict( const arcDictionary &dict );

	int				Read( void *buffer, int len );
	int				Write( const void *buffer, int len );

private:
	static idCompressor *AllocCompressor( int type );

	bool			writing;
	byte *			fileImage;
	arcNetFile *		f;
	idCompressor *	compressor;

	arcNetList<arcNetString*>	demoStrings;
	arcNetFile *		fLog;
	bool			log;
	arcNetString			logStr;

	static arcCVarSystem	com_logDemos;
	static arcCVarSystem	com_compressDemos;
	static arcCVarSystem	com_preloadDemos;
};

#endif /* !__DEMOFILE_H__ */
