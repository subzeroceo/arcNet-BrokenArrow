#ifndef __DEMOFILE_H__
#define __DEMOFILE_H__

/*
===============================================================================

	Demo file

===============================================================================
*/
class anFile;
class anFileSystem;
class anFileSystem;

typedef enum {
	DS_FINISHED,
	DS_RENDER,
	DS_SOUND,
	DS_VERSION
} demoSystem_t;

class anDemoFile : public anFile {
public:
					anDemoFile();
					~anDemoFile();

	const char *	GetName() { return (f?f->GetName():"" ); }
	const char *	GetFullPath() { return (f?f->GetFullPath():"" ); }

	void			SetLog( bool b, const char *p );
	void			Log( const char *p );
	bool			OpenForReading( const char *fileName );
	bool			OpenForWriting( const char *fileName );
	void			Close();

	const char *	ReadHashString();
	void			WriteHashString( const char *str );

	void			ReadDict( anDict &dict );
	void			WriteDict( const anDict &dict );

	int				Read( void *buffer, int len );
	int				Write( const void *buffer, int len );

private:
	static anCompressor *AllocCompressor( int type );

	bool			writing;
	byte *			fileImage;
	anFile *		f;
	anCompressor *	compressor;

	anList<anString*>	demoStrings;
	anFile *		fLog;
	bool			log;
	anString			logStr;

	static anCVarSystem	com_logDemos;
	static anCVarSystem	com_compressDemos;
	static anCVarSystem	com_preloadDemos;
};

#endif // !__DEMOFILE_H__
