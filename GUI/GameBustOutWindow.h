#ifndef __GAME_BUSTOUT_WINDOW_H__
#define __GAME_BUSTOUT_WINDOW_H__

class idGameBustOutWindow;

typedef enum {
	POWERUP_NONE = 0,
	POWERUP_BIGPADDLE,
	POWERUP_MULTIBALL
} powerupType_t;

class BOEntity {
public:
	bool					visible;

	anString					materialName;
	const anMaterial *		material;
	float					width, height;
	anVec4					color;
	anVec2					position;
	anVec2					velocity;

	powerupType_t			powerup;

	bool					removed;
	bool					fadeOut;

	idGameBustOutWindow *	game;
	
public:
							BOEntity(idGameBustOutWindow* _game);
	virtual					~BOEntity();

	virtual void			WriteToSaveGame( anFile *savefile );
	virtual void			ReadFromSaveGame( anFile *savefile, idGameBustOutWindow* _game );

	void					SetMaterial(const char* name);
	void					SetSize( float _width, float _height );
	void					SetColor( float r, float g, float b, float a );
	void					SetVisible( bool isVisible );

	virtual void			Update( float timeslice, int guiTime );
	virtual void			Draw(idDeviceContext *dc);

private:
};

typedef enum {
	COLLIDE_NONE = 0,
	COLLIDE_DOWN,
	COLLIDE_UP,
	COLLIDE_LEFT,
	COLLIDE_RIGHT
} collideDir_t;

class BOBrick {
public:
	float			x;
	float			y;
	float			width;
	float			height;
	powerupType_t	powerup;

	bool			isBroken;

	BOEntity		*ent;

public:
					BOBrick();
					BOBrick( BOEntity *_ent, float _x, float _y, float _width, float _height );
					~BOBrick();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile, idGameBustOutWindow *game );

	void			SetColor( anVec4 bcolor );
	collideDir_t	checkCollision( anVec2 pos, anVec2 vel );

private:
};

#define BOARD_ROWS 12

class idGameBustOutWindow : public idWindow {
public:
	idGameBustOutWindow(anUserInterface *gui);
	idGameBustOutWindow(idDeviceContext *d, anUserInterface *gui);
	~idGameBustOutWindow();

	virtual void		WriteToSaveGame( anFile *savefile );
	virtual void		ReadFromSaveGame( anFile *savefile );

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw( inttime, float x, float y);
	virtual const char*	Activate(bool activate);
	virtual idWinVar *	GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = nullptr );

	anList<BOEntity*>	entities;

private:
	void				CommonInit();
	void				ResetGameState();

	void				ClearBoard();
	void				ClearPowerups();
	void				ClearBalls();

	void				LoadBoardFiles();
	void				SetCurrentBoard();
	void				UpdateGame();
	void				UpdatePowerups();
	void				UpdatePaddle();
	void				UpdateBall();
	void				UpdateScore();

	BOEntity *			CreateNewBall();
	BOEntity *			CreatePowerup( BOBrick *brick );

	virtual bool		ParseInternalVar(const char *name, anParser *src);

private:

	idWinBool			gamerunning;
	idWinBool			onFire;
	idWinBool			onContinue;
	idWinBool			onNewGame;
	idWinBool			onNewLevel;

	float				timeSlice;
	bool				gameOver;

	int					numLevels;
	byte *				levelBoardData;
	bool				boardDataLoaded;

	int					numBricks;
	int					currentLevel;

	bool				updateScore;
	int					gameScore;
	int					nextBallScore;

	int					bigPaddleTime;
	float				paddleVelocity;

	float				ballSpeed;
	int					ballsRemaining;
	int					ballsInPlay;
	bool				ballHitCeiling;

	anList<BOEntity*>	balls;
	anList<BOEntity*>	powerUps;

	BOBrick				*paddle;
	anList<BOBrick*>	board[BOARD_ROWS];
};

#endif //__GAME_BUSTOUT_WINDOW_H__
