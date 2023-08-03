
#ifndef __CHOICEWINDOW_H
#define __CHOICEWINDOW_H

#include "Window.h"

class anUserInterface;
class idChoiceWindow : public idWindow {
public:
						idChoiceWindow(anUserInterface *gui);
						idChoiceWindow(idDeviceContext *d, anUserInterface *gui);
	virtual				~idChoiceWindow();

	virtual const char	*HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void 		PostParse();
	virtual void 		Draw( inttime, float x, float y);
	virtual void		Activate( bool activate, anString &act );
	virtual size_t		Allocated(){return idWindow::Allocated();}; 
  
	virtual idWinVar	*GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = nullptr );

	void				RunNamedEvent( const char* eventName );
	
private:
	virtual bool		ParseInternalVar(const char *name, anParser *src);
	void				CommonInit();
	void				UpdateChoice();
	void				ValidateChoice();
	
	void				InitVars();
						// true: read the updated cvar from cvar system, gui from dict
						// false: write to the cvar system, to the gui dict
						// force == true overrides liveUpdate 0
	void				UpdateVars( bool read, bool force = false );

	void				UpdateChoicesAndVals( void );
	
	int					currentChoice;
	int					choiceType;
	anString				latchedChoices;
	idWinStr			choicesStr;
	anString				latchedVals;
	idWinStr			choiceVals;
	anStringList			choices;
	anStringList			values;

	idWinStr			guiStr;
	idWinStr			cvarStr;
	anCVar *			cvar;
	idMultiWinVar		updateStr;

	idWinBool			liveUpdate;
	idWinStr			updateGroup;
};

#endif // __CHOICEWINDOW_H
