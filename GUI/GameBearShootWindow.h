#ifndef __GAME_BEARSHOOT_WINDOW_H__
#define __GAME_BEARSHOOT_WINDOW_H__

class idGameBearShootWindow;

class BSEntity {
public:
	const anMaterial *		material;
	anStr					materialName;
	float					width, height;
	bool					visible;

	anVec4					entColor;
	anVec2					position;
	float					rotation;
	float					rotationSpeed;
	anVec2					velocity;

	bool					fadeIn;
	bool					fadeOut;

	idGameBearShootWindow *	game;
	
public:
						BSEntity(idGameBearShootWindow* _game);
	virtual				~BSEntity();

	virtual void		WriteToSaveGame( anFile *savefile );
	virtual void		ReadFromSaveGame( anFile *savefile, idGameBearShootWindow* _game );

	void				SetMaterial(const char *name);
	void				SetSize( float _width, float _height );
	void				SetVisible( bool isVisible );

	virtual void		Update( float timeslice );
	virtual void		Draw(idDeviceContext *dc);

private:
};


class idGameBearShootWindow : public idWindow {
public:
	idGameBearShootWindow(anUserInterface *gui);
	idGameBearShootWindow(idDeviceContext *d, anUserInterface *gui);
	~idGameBearShootWindow();

	virtual void		WriteToSaveGame( anFile *savefile );
	virtual void		ReadFromSaveGame( anFile *savefile );

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw( inttime, float x, float y);
	virtual const char*	Activate(bool activate);
	virtual idWinVar *	GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = nullptr );

private:
	void				CommonInit();
	void				ResetGameState();

	void				UpdateBear();
	void				UpdateHelicopter();
	void				UpdateTurret();
	void				UpdateButtons();
	void				UpdateGame();
	void				UpdateScore();

	virtual bool		ParseInternalVar(const char *name, anParser *src);

private:

	idWinBool			gamerunning;
	idWinBool			onFire;
	idWinBool			onContinue;
	idWinBool			onNewGame;

	float				timeSlice;
	float				timeRemaining;
	bool				gameOver;

	int					currentLevel;
	int					goalsHit;
	bool				updateScore;
	bool				bearHitTarget;

	float				bearScale;
	bool				bearIsShrinking;
	int					bearShrinkStartTime;

	float				turretAngle;
	float				turretForce;

	float				windForce;
	int					windUpdateTime;

	anList<BSEntity*>	entities;

	BSEntity			*turret;
	BSEntity			*bear;
	BSEntity			*helicopter;
	BSEntity			*goal;
	BSEntity			*wind;
	BSEntity			*gunblast;
};

#endif //__GAME_BEARSHOOT_WINDOW_H__
