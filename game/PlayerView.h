#ifndef __GAME_PLAYERVIEW_H__
#define __GAME_PLAYERVIEW_H__

/*
===============================================================================

  Player view.

===============================================================================
*/

// screenBlob_t are for the on-screen damage claw marks, etc
typedef struct {
	const arcMaterial *	material;
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

	void				DamageImpulse( arcVec3 localKickDir, const arcDict *damageDef );

	void				WeaponFireFeedback( const arcDict *weaponDef );

	arcAngles			AngleOffset( void ) const;			// returns the current kick angle

	arcMat3				ShakeAxis( void ) const;			// returns the current shake angle

	void				CalculateShake( void );

	// this may involve rendering to a texture and displaying
	// that with a warp model or in double vision mode
	void				RenderPlayerView( idUserInterface *hud );

	void				Fade( arcVec4 color, int time );

	void				Flash( arcVec4 color, int time );

	void				AddBloodSpray( float duration );

	// temp for view testing
	void				EnableBFGVision( bool b ) { bfgVision = b; };

private:
	void				SingleView( idUserInterface *hud, const renderView_t *view );
	void				DoubleVision( idUserInterface *hud, const renderView_t *view, int offset );
	void				ScreenFade();

	screenBlob_t *		GetScreenBlob();

	screenBlob_t		screenBlobs[MAX_SCREEN_BLOBS];

	int					dvFinishTime;		// double vision will be stopped at this time
	const arcMaterial *	dvMaterial;			// material to take the double vision screen shot

	int					kickFinishTime;		// view kick will be stopped at this time
	arcAngles			kickAngles;

	const arcMaterial *	armorMaterial;		// armor damage view effect
	const arcMaterial *	irGogglesMaterial;	// ir effect
	const arcMaterial *	bloodSprayMaterial; // blood spray
	float				lastDamageTime;		// accentuate the tunnel effect for a while

	arcVec4				fadeColor;			// fade color
	arcVec4				fadeToColor;		// color to fade to
	arcVec4				fadeFromColor;		// color to fade from
	float				fadeRate;			// fade rate
	int					fadeTime;			// fade time

	arcAngles			shakeAng;			// from the sound sources

	arcNetBasePlayer *			player;
	renderView_t		view;
};

#endif // !__GAME_PLAYERVIEW_H__
