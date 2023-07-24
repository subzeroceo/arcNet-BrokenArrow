#ifndef __GAME_FOOTPRINTS_H__
#define __GAME_FOOTPRINTS_H__

class arcFootPrintManager {
public:
	virtual void Init( void ) = 0;
	virtual void Shutdown( void ) = 0;

	virtual bool AddFootPrint( const arcVec3 &point, const arcVec3 &forward, const arcVec3 &up, bool right ) = 0;

	virtual void Think( void ) = 0;

	virtual renderEntity_t* GetRenderEntity( void ) = 0;
	virtual int				GetModelHandle( void ) = 0;
};

extern arcFootPrintManager *footPrintManager;

#endif //__GAME_FOOTPRINTS_H__
