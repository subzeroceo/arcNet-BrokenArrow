
#ifndef GESTATEMODIFIER_H_
#define GESTATEMODIFIER_H_

#ifndef GEMODIFIER_H_
#include "GEModifier.h"
#endif

class rvGEStateModifier : public rvGEModifier
{
public:

	rvGEStateModifier ( const char* name, idWindow* window, arcDictionary& dict );

	virtual bool		Apply	( void );
	virtual bool		Undo	( void );

protected:

	bool	SetState	( arcDictionary& dict );

	rvGEWindowWrapper::EWindowType	mWindowType;
	rvGEWindowWrapper::EWindowType	mUndoWindowType;
	arcDictionary							mDict;
	arcDictionary							mUndoDict;
};

#endif // GESTATEMODIFIER_H_