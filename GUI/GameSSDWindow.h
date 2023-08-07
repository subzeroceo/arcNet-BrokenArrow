#ifndef __GAME_SSD_WINDOW_H__
#define __GAME_SSD_WINDOW_H__

class idGameSSDWindow;

class SSDCrossHair {

public:
	enum {
		CROSSHAIR_STANDARD = 0,
		CROSSHAIR_SUPER,
		CROSSHAIR_COUNT
	};
	const anMaterial*	crosshairMaterial[CROSSHAIR_COUNT];
	int					currentCrosshair;
	float				crosshairWidth, crosshairHeight;

public:
				SSDCrossHair();
				~SSDCrossHair();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile );

	void		InitCrosshairs();
	void		Draw(idDeviceContext *dc, const anVec2& cursor);
};

enum {
	SSD_ENTITY_BASE = 0,
	SSD_ENTITY_ASTEROID,
	SSD_ENTITY_ASTRONAUT,
	SSD_ENTITY_EXPLOSION,
	SSD_ENTITY_POINTS,
	SSD_ENTITY_PROJECTILE,
	SSD_ENTITY_POWERUP
};

class SSDEntity  {

public:
	//SSDEntity Information
	int					type;
	int					id;
	anStr				materialName;
	const anMaterial*	material;
	anVec3				position;
	anVec2				size;
	float				radius;
	float				hitRadius;
	float				rotation;

	anVec4				matColor;

	anStr				text;
	float				textScale;
	anVec4				foreColor;

	idGameSSDWindow*	game;
	int					currentTime;
	int					lastUpdate;
	int					elapsed;

	bool				destroyed;
	bool				noHit;
	bool				noPlayerDamage;

	bool				inUse;

	
public:
						SSDEntity();
	virtual				~SSDEntity();

	virtual void		WriteToSaveGame( anFile *savefile );
	virtual void		ReadFromSaveGame( anFile *savefile,  idGameSSDWindow* _game );

	void				EntityInit();

	void				SetGame(idGameSSDWindow* _game);
	void				SetMaterial(const char *_name);
	void				SetPosition(const anVec3 &_position);
	void				SetSize(const anVec2& _size);
	void				SetRadius(float _radius, float _hitFactor = 1.0f);
	void				SetRotation(float _rotation);

	void				Update();
	bool				HitTest(const anVec2& pt);
	

	virtual void		EntityUpdate() {};
	virtual void		Draw(idDeviceContext *dc);
	virtual void		DestroyEntity();

	virtual void		OnHit( intkey) {};
	virtual void		OnStrikePlayer() {};

	anBounds			WorldToScreen(const anBounds worldBounds);
	anVec3				WorldToScreen(const anVec3 &worldPos);

	anVec3				ScreenToWorld(const anVec3 &screenPos);

};

/*
*****************************************************************************
* SSDMover	
****************************************************************************
*/

class SSDMover : public SSDEntity {

public:
	anVec3				speed;
	float				rotationSpeed;

public:
						SSDMover();
	virtual				~SSDMover();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile,  idGameSSDWindow* _game  );

	void				MoverInit(const anVec3 &_speed, float _rotationSpeed);

	virtual void		EntityUpdate();


};

/*
*****************************************************************************
* SSDAsteroid	
****************************************************************************
*/

#define MAX_ASTEROIDS 64

class SSDAsteroid : public SSDMover {

public:

	int					health;

public:
						SSDAsteroid();
						~SSDAsteroid();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile,  idGameSSDWindow* _game  );

	void				Init(idGameSSDWindow* _game, const anVec3 &startPosition, const anVec2& _size, float _speed, float rotate, int _health);

	virtual void		EntityUpdate();
	static SSDAsteroid*	GetNewAsteroid(idGameSSDWindow* _game, const anVec3 &startPosition, const anVec2& _size, float _speed, float rotate, int _health);
	static SSDAsteroid*	GetSpecificAsteroid( int id);
	static void			WriteAsteroids(anFile* savefile);
	static void			ReadAsteroids(anFile* savefile, idGameSSDWindow* _game);


	
protected:
	static SSDAsteroid	asteroidPool[MAX_ASTEROIDS];
	
};

/*
*****************************************************************************
* SSDAstronaut	
****************************************************************************
*/
#define MAX_ASTRONAUT 8

class SSDAstronaut : public SSDMover {

public:

	int					health;

public:
							SSDAstronaut();
							~SSDAstronaut();
	
	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile,  idGameSSDWindow* _game  );

	void					Init(idGameSSDWindow* _game, const anVec3 &startPosition, float _speed, float rotate, int _health);

	static SSDAstronaut*	GetNewAstronaut(idGameSSDWindow* _game, const anVec3 &startPosition, float _speed, float rotate, int _health);
	static SSDAstronaut*	GetSpecificAstronaut( int id);
	static void				WriteAstronauts(anFile* savefile);
	static void				ReadAstronauts(anFile* savefile, idGameSSDWindow* _game);

protected:
	static SSDAstronaut	astronautPool[MAX_ASTRONAUT];

};

/*
*****************************************************************************
* SSDExplosion	
****************************************************************************
*/
#define MAX_EXPLOSIONS 64

class SSDExplosion : public SSDEntity {

public:
	anVec2	finalSize;
	int		length;
	int		beginTime;
	int		endTime;
	int		explosionType;

	//The entity that is exploding
	SSDEntity*			buddy;
	bool				killBuddy;
	bool				followBuddy;

	enum {
		EXPLOSION_NORMAL = 0,
		EXPLOSION_TELEPORT = 1
	};

public:
	SSDExplosion();
	~SSDExplosion();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile,  idGameSSDWindow* _game  );

	void				Init(idGameSSDWindow* _game, const anVec3 &_position, const anVec2& _size, int _length, int _type, SSDEntity* _buddy, bool _killBuddy = true, bool _followBuddy = true);

	virtual void		EntityUpdate();
	static SSDExplosion*	GetNewExplosion(idGameSSDWindow* _game, const anVec3 &_position, const anVec2& _size, int _length, int _type, SSDEntity* _buddy, bool _killBuddy = true, bool _followBuddy = true);
	static SSDExplosion*	GetSpecificExplosion( int id);
	static void				WriteExplosions(anFile* savefile);
	static void				ReadExplosions(anFile* savefile, idGameSSDWindow* _game);

protected:
	static SSDExplosion	explosionPool[MAX_EXPLOSIONS];
};

#define MAX_POINTS 16

class SSDPoints : public SSDEntity {

	int		length;
	int		distance;
	int		beginTime;
	int		endTime;

	anVec3	beginPosition;
	anVec3	endPosition;

	anVec4	beginColor;
	anVec4	endColor;

	
public:
	SSDPoints();
	~SSDPoints();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile,  idGameSSDWindow* _game  );

	void				Init(idGameSSDWindow* _game, SSDEntity* _ent, int _points, int _length, int _distance, const anVec4& color);
	virtual void		EntityUpdate();

	static SSDPoints*	GetNewPoints(idGameSSDWindow* _game, SSDEntity* _ent, int _points, int _length, int _distance, const anVec4& color);
	static SSDPoints*	GetSpecificPoints( int id);
	static void			WritePoints(anFile* savefile);
	static void			ReadPoints(anFile* savefile, idGameSSDWindow* _game);

protected:
	static SSDPoints	pointsPool[MAX_POINTS];
};

#define MAX_PROJECTILES 64

class SSDProjectile : public SSDEntity {

	anVec3	dir;
	anVec3	speed;
	int		beginTime;
	int		endTime;

	anVec3	endPosition;

public:
	SSDProjectile();
	~SSDProjectile();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile,  idGameSSDWindow* _game  );

	void				Init(idGameSSDWindow* _game, const anVec3 &_beginPosition, const anVec3 &_endPosition, float _speed, float _size);
	virtual void		EntityUpdate();

	static SSDProjectile* GetNewProjectile(idGameSSDWindow* _game, const anVec3 &_beginPosition, const anVec3 &_endPosition, float _speed, float _size);
	static SSDProjectile* GetSpecificProjectile( int id);
	static void				WriteProjectiles(anFile* savefile);
	static void				ReadProjectiles(anFile* savefile, idGameSSDWindow* _game);

protected:
	static SSDProjectile	projectilePool[MAX_PROJECTILES];
};


#define MAX_POWERUPS 64

/** 
* Powerups work in two phases:
*	1.) Closed container hurls at you
*		If you shoot the container it open
*	3.) If an opened powerup hits the player he aquires the powerup
* Powerup Types:
*	Health - Give a specific amount of health
*	Super Blaster - Increases the power of the blaster (lasts a specific amount of time)
*	Asteroid Nuke - Destroys all asteroids on screen as soon as it is aquired
*	Rescue Powerup - Rescues all astronauts as soon as it is acquited
*	Bonus Points - Gives some bonus points when acquired
*/
class SSDPowerup : public SSDMover {

	enum {
		POWERUP_STATE_CLOSED = 0,
		POWERUP_STATE_OPEN
	};

	enum {
		POWERUP_TYPE_HEALTH = 0,
		POWERUP_TYPE_SUPER_BLASTER,
		POWERUP_TYPE_ASTEROID_NUKE,
		POWERUP_TYPE_RESCUE_ALL,
		POWERUP_TYPE_BONUS_POINTS,
		POWERUP_TYPE_DAMAGE,
		POWERUP_TYPE_MAX
	};

	int powerupState;
	int powerupType;


public:


public:
	SSDPowerup();
	virtual ~SSDPowerup();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile,  idGameSSDWindow* _game  );

	virtual void		OnHit( intkey);
	virtual void		OnStrikePlayer();

	void	OnOpenPowerup();
	void	OnActivatePowerup();

	

	void	Init(idGameSSDWindow* _game, float _speed, float _rotation);

	static SSDPowerup* GetNewPowerup(idGameSSDWindow* _game, float _speed, float _rotation);
	static SSDPowerup* GetSpecificPowerup( int id);
	static void			WritePowerups(anFile* savefile);
	static void			ReadPowerups(anFile* savefile, idGameSSDWindow* _game);

protected:
	static SSDPowerup	powerupPool[MAX_POWERUPS];

};


typedef struct {
	float	spawnBuffer;
	int		needToWin;
} SSDLevelData_t;

typedef struct {
	float	speedMin, speedMax;
	float	sizeMin, sizeMax;
	float	rotateMin, rotateMax;
	int		spawnMin, spawnMax;
	int		asteroidHealth;
	int		asteroidPoints;
	int		asteroidDamage;
} SSDAsteroidData_t;

typedef struct {
	float	speedMin, speedMax;
	float	rotateMin, rotateMax;
	int		spawnMin, spawnMax;
	int		health;
	int		points;
	int		penalty;
} SSDAstronautData_t;

typedef struct {
	float	speedMin, speedMax;
	float	rotateMin, rotateMax;
	int		spawnMin, spawnMax;
} SSDPowerupData_t;

typedef struct {
	float	speed;
	int		damage;
	int		size;
} SSDWeaponData_t;

/** 
* SSDLevelStats_t
*	Data that is used for each level. This data is reset
*	each new level.
*/
typedef struct {
	int					shotCount;
	int					hitCount;
	int					destroyedAsteroids;
	int					nextAsteroidSpawnTime;

	int					killedAstronauts;
	int					savedAstronauts;

	//Astronaut Level Data
	int					nextAstronautSpawnTime;

	//Powerup Level Data
	int					nextPowerupSpawnTime;

	SSDEntity*			targetEnt;
} SSDLevelStats_t;

/** 
* SSDGameStats_t
*	Data that is used for the game that is currently running. Memset this
*	to completely reset the game
*/
typedef struct {
	bool				gameRunning;

	int					score;
	int					prebonusscore;

	int					health;

	int					currentWeapon;
	int					currentLevel;
	int					nextLevel;

	SSDLevelStats_t		levelStats;
} SSDGameStats_t;


class idGameSSDWindow : public idWindow {
public:
	idGameSSDWindow(anUserInterface *gui);
	idGameSSDWindow(idDeviceContext *d, anUserInterface *gui);
	~idGameSSDWindow();

	virtual void	WriteToSaveGame( anFile *savefile );
	virtual void	ReadFromSaveGame( anFile *savefile );

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual idWinVar*	GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = nullptr );
	
	
	virtual void		Draw( inttime, float x, float y);

	void				AddHealth( inthealth);
	void				AddScore(SSDEntity* ent, int points);
	void				AddDamage( intdamage);

	void				OnNuke();
	void				OnRescueAll();
	void				OnSuperBlaster();

	SSDEntity*			GetSpecificEntity( inttype, int id);

	void				PlaySound(const char *sound);




	static idRandom		random;	
	int					ssdTime;
	
private:
	
	//Initialization
	virtual bool		ParseInternalVar(const char *name, anParser *src);
	void				ParseLevelData( intlevel, const anStr& levelDataString);
	void				ParseAsteroidData( intlevel, const anStr& asteroidDataString);
	void				ParseWeaponData( intweapon, const anStr& weaponDataString);
	void				ParseAstronautData( intlevel, const anStr& astronautDataString);
	void				ParsePowerupData( intlevel, const anStr& powerupDataString);

	void				CommonInit();
	void				ResetGameStats();
	void				ResetLevelStats();
	void				ResetEntities();

	//Game Running Methods
	void				StartGame();
	void				StopGame();
	void				GameOver();

	//Starting the Game
	void				BeginLevel( intlevel);
	void				ContinueGame();

	//Stopping the Game
	void				LevelComplete();
	void				GameComplete();

	

	void				UpdateGame();
	void				CheckForHits();
	void				ZOrderEntities();

	void				SpawnAsteroid();

	void				FireWeapon( intkey);
	SSDEntity*			EntityHitTest(const anVec2& pt);

	void				HitAsteroid(SSDAsteroid* asteroid, int key);
	void				AsteroanStringuckPlayer(SSDAsteroid* asteroid);

	
	

	
	void				RefreshGuiData();

	anVec2				GetCursorWorld();

	//Astronaut Methods
	void				SpawnAstronaut();
	void				HitAstronaut(SSDAstronaut* astronaut, int key);
	void				AstronautStruckPlayer(SSDAstronaut* astronaut);

	//Powerup Methods
	void				SpawnPowerup();


	void				StartSuperBlaster();
	void				StopSuperBlaster();

	//void				FreeSoundEmitter( bool immediate );

	


public:

	//WinVars used to call functions from the guis
	idWinBool					beginLevel;
	idWinBool					resetGame;
	idWinBool					continueGame;
	idWinBool					refreshGuiData;

	SSDCrossHair				crosshair;
	anBounds					screenBounds;

	//Level Data
	int							levelCount;
	anList<SSDLevelData_t>		levelData;
	anList<SSDAsteroidData_t>	asteroidData;
	anList<SSDAstronautData_t>	astronautData;
	anList<SSDPowerupData_t>	powerupData;

	
	//Weapon Data
	int							weaponCount;
	anList<SSDWeaponData_t>		weaponData;

	int							superBlasterTimeout;

	//All current game data is stored in this structure (except the entity list)
	SSDGameStats_t				gameStats;
	anList<SSDEntity*>			entities;

	int							currentSound;
	
};

#endif //__GAME_SSD_WINDOW_H__
