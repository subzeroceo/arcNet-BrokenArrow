#include "..//idlib/Lib.h"
#pragma hdrstop

#include "MEOptions.h"

/**
* Constructor for MEOptions.
*/
MEOptions::MEOptions() {
	registry.Init( "Software\\id Software\\DOOM3\\Tools\\MaterialEditor" );
	materialTreeWidth = 0;
	stageWidth = 0;
	previewPropertiesWidth = 0;
	materialEditHeight = 0;
	materialPropHeadingWidth = 0;
	previewPropHeadingWidth = 0;

}

/**
* Destructor for MEOptions.
*/
MEOptions::~MEOptions() {
}

/**
* Saves the material editor options to the registry.
*/
bool MEOptions::Save ( void ) {
	registry.SetFloat( "materialTreeWidth", materialTreeWidth);
	registry.SetFloat( "stageWidth", stageWidth);
	registry.SetFloat( "previewPropertiesWidth", previewPropertiesWidth);
	registry.SetFloat( "materialEditHeight", materialEditHeight);
	registry.SetFloat( "materialPropHeadingWidth", materialPropHeadingWidth);
	registry.SetFloat( "previewPropHeadingWidth", previewPropHeadingWidth);

	return registry.Save();
}

/**
* Loads the material editor options from the registry.
*/
bool MEOptions::Load( void ) {
	if ( !registry.Load() ) {
		return false;
	}

	materialTreeWidth = ( int )registry.GetFloat( "materialTreeWidth" );
	stageWidth = ( int )registry.GetFloat( "stageWidth" );
	previewPropertiesWidth = ( int )registry.GetFloat( "previewPropertiesWidth" );
	materialEditHeight = ( int )registry.GetFloat( "materialEditHeight" );
	materialPropHeadingWidth = ( int )registry.GetFloat( "materialPropHeadingWidth" );
	previewPropHeadingWidth = ( int )registry.GetFloat( "previewPropHeadingWidth" );

	return true;
}