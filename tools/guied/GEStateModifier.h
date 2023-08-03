
#ifndef GESTATEMODIFIER_H_
#define GESTATEMODIFIER_H_

#ifndef GEMODIFIER_H_
#include "GEModifier.h"
#endif

class rvGEStateModifier : public rvGEModifier
{
public:

	rvGEStateModifier ( const char* name, idWindow* window, anDict& dict );

	virtual bool		Apply	( void );
	virtual bool		Undo	( void );

protected:

	bool	SetState	( anDict& dict );

	rvGEWindowWrapper::EWindowType	mWindowType;
	rvGEWindowWrapper::EWindowType	mUndoWindowType;
	anDict							mDict;
	anDict							mUndoDict;
};

#endif // GESTATEMODIFIER_H_