#ifndef __GUISCRIPT_H
#define __GUISCRIPT_H

#include "Window.h"
#include "Winvar.h"

struct idGSWinVar {
	idGSWinVar() {
		var = nullptr;
		own = false;
	}
	idWinVar* var;
	bool own;
};

class idGuiScriptList;

class idGuiScript {
	friend class idGuiScriptList;
	friend class idWindow;

public:
	idGuiScript();
	~idGuiScript();

	bool Parse(anParser *src);
	void Execute(idWindow *win) {
		if (handler) {
			handler(win, &parms);
		}
	}
	void FixupParms(idWindow *win);
	size_t Size() {
		int sz = sizeof(*this);
		for ( int i = 0; i < parms.Num(); i++ ) {
			sz += parms[i].var->Size();
		}
		return sz;
	}

	void WriteToSaveGame( anFile *savefile );
	void ReadFromSaveGame( anFile *savefile );

protected:
	int conditionReg;
	idGuiScriptList *ifList;
	idGuiScriptList *elseList;
	anList<idGSWinVar> parms;
	void (*handler) (idWindow *window, anList<idGSWinVar> *src);

};


class idGuiScriptList {
	anList<idGuiScript*> list;
public:
	idGuiScriptList() { list.SetGranularity( 4 ); };
	~idGuiScriptList() { list.DeleteContents( true ); };
	void Execute(idWindow *win);
	void Append(idGuiScript* gs) {
		list.Append(gs);
	}
	size_t Size() {
		int sz = sizeof(*this);
		for ( int i = 0; i < list.Num(); i++ ) {
			sz += list[i]->Size();
		}
		return sz;
	}
	void FixupParms(idWindow *win);
	void ReadFromDemoFile( class anSavedGamesFile *f ) {};
	void WriteToDemoFile( class anSavedGamesFile *f ) {};

	void WriteToSaveGame( anFile *savefile );
	void ReadFromSaveGame( anFile *savefile );
};

#endif // __GUISCRIPT_H
