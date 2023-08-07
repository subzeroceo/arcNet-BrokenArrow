
#ifndef REGEXP_H_
#define REGEXP_H_

class idWindow;

class anRegister {
public:
	anRegister() {};
	anRegister(const char *p, int t) {
		name = p;
		type = t;
		assert(t >= 0 && t < NUMTYPES);
		regCount = REGCOUNT[t];
		enabled = (type == STRING) ? false : true;
	};
	bool enabled;
	int type;
	int regCount;
	enum REGTYPE { VEC4 = 0, FLOAT, BOOL, INT, STRING, VEC2, VEC3, NUMTYPES } ;
	static int REGCOUNT[NUMTYPES];
	anStr name;
	int regs[4];
	void SetToRegs(float *registers, idTypedDict *state);
	void SetToRegList(anList<float> *registers, idTypedDict *state);
	void GetFromRegs(float *registers, idTypedDict *state);
	void CopyRegs(anRegister *src) {
		regs[0] = src->regs[0];
		regs[1] = src->regs[1];
		regs[2] = src->regs[2];
		regs[3] = src->regs[3];
	}
	void Enable(bool b) {
		enabled = b;
	}
	void ReadFromDemoFile(anSavedGamesFile *f);
	void WriteToDemoFile(anSavedGamesFile *f);

};

class anRegisterList {
	anList<anRegister> regs;
public:
	
	// 
	void RemoveReg ( const char *name );
	// 

	void AddReg(const char *name, int type, anParser *src, idWindow *win);
	void AddReg(const char *name, int type, anVec4 data, idWindow *win);
	anRegister *FindReg(const char *name);
	int			FindRegIndex ( const char *name );
	void SetToRegs(float *registers, idTypedDict *state);
	void GetFromRegs(float *registers, idTypedDict *state);
	void Reset();
	void ReadFromDemoFile(anSavedGamesFile *f);
	void WriteToDemoFile(anSavedGamesFile *f);

};

#endif