

#ifndef __REGEXP_H__
#define __REGEXP_H__

class idWindow;
class idWinVar;

class anRegister {
public:
						anRegister();
						anRegister( const char *p, int t );

	enum REGTYPE { VEC4 = 0, FLOAT, BOOL, INT, STRING, VEC2, VEC3, RECTANGLE, NUMTYPES } ;
	static int REGCOUNT[NUMTYPES];

	bool				enabled;
	short				type;
	anStr				name;
	int					regCount;
	unsigned short		regs[4];
	idWinVar *			var;

	void				SetToRegs( float *registers );
	void				GetFromRegs( float *registers );
	void				CopyRegs( anRegister *src );
	void				Enable( bool b ) { enabled = b; }
	void				ReadFromDemoFile( anSavedGamesFile *f );
	void				WriteToDemoFile( anSavedGamesFile *f );
	void				WriteToSaveGame( anFile *savefile );
	void				ReadFromSaveGame( anFile *savefile );
};

inline anRegister::anRegister( void ) {
}

inline anRegister::anRegister( const char *p, int t ) {
	name = p;
	type = t;
	assert( t >= 0 && t < NUMTYPES );
	regCount = REGCOUNT[t];
	enabled = ( type == STRING ) ? false : true;
	var = nullptr;
};

inline void anRegister::CopyRegs( anRegister *src ) {
	regs[0] = src->regs[0];
	regs[1] = src->regs[1];
	regs[2] = src->regs[2];
	regs[3] = src->regs[3];
}

class anRegisterList {
public:

						anRegisterList();
						~anRegisterList();

	void				AddReg( const char *name, int type, anParser *src, idWindow *win, idWinVar *var );
	void				AddReg( const char *name, int type, anVec4 data, idWindow *win, idWinVar *var );

	anRegister *		FindReg( const char *name );
	void				SetToRegs( float *registers );
	void				GetFromRegs( float *registers );
	void				Reset();
	void				ReadFromDemoFile( anSavedGamesFile *f );
	void				WriteToDemoFile( anSavedGamesFile *f );
	void				WriteToSaveGame( anFile *savefile );
	void				ReadFromSaveGame( anFile *savefile );

private:
	anList<anRegister*>	regs;
	anHashIndex			regHash;
};

inline anRegisterList::anRegisterList() {
	regs.SetGranularity( 4 );
	regHash.SetGranularity( 4 );
	regHash.Clear( 32, 4 );
}

inline anRegisterList::~anRegisterList() {
}

#endif /* !__REGEXP_H__ */
