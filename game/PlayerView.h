#ifndef __GAME_PLAYERVIEW_H__
#define __GAME_PLAYERVIEW_H__

/*
===============================================================================

  Player view.

===============================================================================
*/

// screenBlob_t are for the on-screen damage claw marks, etc
typedef struct {
	const anMaterial *	material;
	float				x, y, w, h;
	float				s1, t1, s2, t2;
	int					finishTime;
	int					startFadeTime;
	float				driftAmount;
} screenBlob_t;

#define	MAX_SCREEN_BLOBS	8

class arcNetBasePlayerView {
public:
						arcNetBasePlayerView();

	void				Save( arcSaveGame *savefile ) const;
	void				Restore( arcRestoreGame *savefile );

	void				SetPlayerEntity( class arcNetBasePlayer *playerEnt );

	void				ClearEffects( void );

	void				DamageImpulse( anVec3 localKickDir, const anDict *damageDef );

	void				WeaponFireFeedback( const anDict *weaponDef );

	anAngles			AngleOffset( void ) const;			// returns the current kick angle

	anMat3				ShakeAxis( void ) const;			// returns the current shake angle

	void				CalculateShake( void );

	// this may involve rendering to a texture and displaying
	// that with a warp model or in double vision mode
	void				RenderPlayerView( anUserInterface *hud );

	void				Fade( anVec4 color, int time );

	void				Flash( anVec4 color, int time );

	void				AddBloodSpray( float duration );

	// temp for view testing
	void				EnableBFGVision( bool b ) { bfgVision = b; };

private:
	void				SingleView( anUserInterface *hud, const renderView_t *view );
	void				DoubleVision( anUserInterface *hud, const renderView_t *view, int offset );
	void				ScreenFade();

	screenBlob_t *		GetScreenBlob();

	screenBlob_t		screenBlobs[MAX_SCREEN_BLOBS];

	int					dvFinishTime;		// double vision will be stopped at this time
	const anMaterial *	dvMaterial;			// material to take the double vision screen shot

	int					kickFinishTime;		// view kick will be stopped at this time
	anAngles			kickAngles;

	const anMaterial *	armorMaterial;		// armor damage view effect
	const anMaterial *	irGogglesMaterial;	// ir effect
	const anMaterial *	bloodSprayMaterial; // blood spray
	float				lastDamageTime;		// accentuate the tunnel effect for a while

	anVec4				fadeColor;			// fade color
	anVec4				fadeToColor;		// color to fade to
	anVec4				fadeFromColor;		// color to fade from
	float				fadeRate;			// fade rate
	int					fadeTime;			// fade time

	anAngles			shakeAng;			// from the sound sources

	arcNetBasePlayer *			player;
	renderView_t		view;
};

#endif // !__GAME_PLAYERVIEW_H__
