
#ifndef __SEASFILEMANAGER_H__
#define __SEASFILEMANAGER_H__

/*
===============================================================================

	SEAS File Manager

===============================================================================
*/

class anSEASFileManager {
public:
	virtual						~anSEASFileManager( void ) {}

	virtual anSEASFile *			LoadSEAS( const char *fileName, unsigned int mapFileCRC ) = 0;
	virtual void				FreeSEAS( anSEASFile *file ) = 0;
};

extern anSEASFileManager *		SEASFileManager;

#endif