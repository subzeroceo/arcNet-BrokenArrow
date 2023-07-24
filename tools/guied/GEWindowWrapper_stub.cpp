#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/EditWindow.h"
#include "../../ui/ListWindow.h"
#include "../../ui/BindWindow.h"
#include "../../ui/RenderWindow.h"
#include "../../ui/ChoiceWindow.h"

#include "GEWindowWrapper.h"

static rvGEWindowWrapper stub_wrap( NULL, rvGEWindowWrapper::WT_UNKNOWN );

rvGEWindowWrapper::rvGEWindowWrapper( idWindow* window, EWindowType type ) { }

rvGEWindowWrapper* rvGEWindowWrapper::GetWrapper( idWindow* window ) { return &stub_wrap; }

void rvGEWindowWrapper::SetStateKey( const char*, const char*, bool ) { }

void rvGEWindowWrapper::Finish() { }
