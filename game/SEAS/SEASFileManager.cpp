#pragma hdrstop
#include "../idlib/Lib.h"


#include "SEASFile.h"
#include "SEASFile_local.h"

/*
===============================================================================

	AAS File Manager

===============================================================================
*/

class anSEASFileManagerLocal : public anSEASFileManager {
public:
	virtual						~anSEASFileManagerLocal() {}

	virtual anSEASFile *		LoadSEAS( const char *fileName, unsigned int mapFileCRC );
	virtual void				FreeSEAS( anSEASFile *file );
};

anSEASFileManagerLocal			SEASFileManagerLocal;
anSEASFileManager *				SEASFileManager = &SEASFileManagerLocal;


/*
================
anSEASFileManagerLocal::LoadSEAS
================
*/
anSEASFile *anSEASFileManagerLocal::LoadSEAS( const char *fileName, unsigned int mapFileCRC ) {
	anSEASFileLocal *file = new (TAG_AAS) anSEASFileLocal();
	if ( !file->Load( fileName, mapFileCRC ) ) {
		delete file;
		return nullptr;
	}
	return file;
}

/*
================
anSEASFileManagerLocal::FreeSEAS
================
*/
void anSEASFileManagerLocal::FreeSEAS( anSEASFile *file ) {
	delete file;
}
