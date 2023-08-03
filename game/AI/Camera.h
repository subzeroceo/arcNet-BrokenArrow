
#ifndef __GAME_CAMERA_H__
#define __GAME_CAMERA_H__

/*
===============================================================================

Camera providing an alternative view of the level.

===============================================================================
*/

class idCamera : public anEntity {
public:
	ABSTRACT_PROTOTYPE( idCamera );

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view ) = 0;
	virtual renderView_t *	GetRenderView();
	virtual void			Stop( void ){} ;
};

/*
===============================================================================

idCameraView

===============================================================================
*/

extern const anEventDef EV_SetFOV;
extern const anEventDef EV_Camera_Start;
extern const anEventDef EV_Camera_Stop;

class idCameraView : public idCamera {
public:
	CLASS_PROTOTYPE( idCameraView );
 							idCameraView();

	// save games
	void					Save( anSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( anRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( );
	virtual void			GetViewParms( renderView_t *view );
	virtual void			Stop( void );

protected:
	void					Event_Activate( anEntity *activator );
	void					Event_SetAttachments();
	void					SetAttachment( anEntity **e, const char *p );

// bdube: changed fov to interpolated value
	idInterpolate<float>	fov;

	anEntity				*attachedTo;
	anEntity				*attachedView;


// bdube: added setfov event
	void					Event_SetFOV		( float fov );
	void					Event_BlendFOV		( float beginFOV, float endFOV, float blendTime );
	void					Event_GetFOV		( void );

};



/*
===============================================================================

A camera which follows a path defined by an animation.

===============================================================================
*/


// rjohnson: camera is now contained in a def for frame commands

/*
==============================================================================================

	rvCameraAnimation

==============================================================================================
*/
class idDeclCameraDef;

typedef struct {
	anCQuat				q;
	anVec3				t;
	float				fov;
} cameraFrame_t;

class rvCameraAnimation {
private:
	anList<int>					cameraCuts;
	anList<cameraFrame_t>		camera;
	anList<frameLookup_t>		frameLookup;
	anList<frameCommand_t>		frameCommands;
	int							frameRate;
	anString						name;
	anString						realname;

public:
								rvCameraAnimation();
								rvCameraAnimation( const idDeclCameraDef *cameraDef, const rvCameraAnimation *anim );
								~rvCameraAnimation();

	void						SetAnim( const idDeclCameraDef *cameraDef, const char *sourcename, const char *animname, anString filename );
	const char					*Name( void ) const;
	const char					*FullName( void ) const;
	int							NumFrames( void ) const;
	const cameraFrame_t *		GetAnim( int index ) const;
	int							NumCuts( void ) const;
	const int					GetCut( int index ) const;
	const int					GetFrameRate( void ) const;

	const char					*AddFrameCommand( const class idDeclCameraDef *cameraDef, const anList<int>& frames, anLexer &src, const anDict *def );
	void						CallFrameCommands( anEntity *ent, int from, int to ) const;
	void						CallFrameCommandSound ( const frameCommand_t& command, anEntity* ent, const s_channelType channel ) const;
};

ARC_INLINE const cameraFrame_t *rvCameraAnimation::GetAnim( int index ) const {
	return &camera[ index ];
}

ARC_INLINE const int rvCameraAnimation::GetCut( int index ) const {
	return cameraCuts[ index ];
}

ARC_INLINE const int rvCameraAnimation::GetFrameRate( void ) const {
	return frameRate;
}

/*
==============================================================================================

	idDeclCameraDef

==============================================================================================
*/

// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclCameraDef : public idDecl, public Serializable<'IDCD'> {
public:
								idDeclCameraDef( SerialInputStream &stream );

	virtual void				Write( SerialOutputStream &stream ) const;
	virtual void				AddReferences() const;
#else
class idDeclCameraDef : public idDecl {
#endif

public:
								idDeclCameraDef();
								~idDeclCameraDef();

	virtual size_t				Size( void ) const;
	virtual const char *		DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength, bool noCaching );
	virtual void				FreeData( void );


// jscott: to prevent a recursive crash
	virtual	bool				RebuildTextSource( void ) { return( false ); }
// scork: for detailed error-reporting
	virtual bool				Validate( const char *psText, int iTextLength, anString &strReportTo ) const;


	void						Touch( void ) const;

	int							NumAnims( void ) const;
	const rvCameraAnimation *	GetAnim( int index ) const;
	int							GetSpecificAnim( const char *name ) const;
	int							GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;

private:
	void						CopyDecl( const idDeclCameraDef *decl );
	bool						ParseAnim( anLexer &src, int numDefaultAnims );

private:
	anList<rvCameraAnimation *>	anims;
};

ARC_INLINE const rvCameraAnimation *idDeclCameraDef::GetAnim( int index ) const {
	if ( ( index < 1 ) || ( index > anims.Num() ) ) {
		return nullptr;
	}
	return anims[ index - 1 ];
}

/*
==============================================================================================

	idCameraAnim

==============================================================================================
*/
class idCameraAnim : public idCamera {
public:
	CLASS_PROTOTYPE( idCameraAnim );

							idCameraAnim();
							~idCameraAnim();

	// save games
	void					Save( anSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( anRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view );

private:
	int						threadNum;
	anVec3					offset;
	int						starttime;
	int						cycle;
	const idDeclCameraDef	*cameraDef;
	int						lastFrame;
	anEntityPtr<anEntity>	activator;

	void					Start( void );
	void					Stop( void );
	void					Think( void );

	void					LoadAnim( void );
	void					Event_Start( void );
	void					Event_Stop( void );
	void					Event_SetCallback( void );
	void					Event_Activate( anEntity *activator );


// mekberg: wait support
	void					Event_IsActive( );

	anList<dword>			imageTable;
	anList<int>				imageCmds;

};



/*
===============================================================================

rvCameraPortalSky

===============================================================================
*/
// jscott: for portal skies
class rvCameraPortalSky : public idCamera {
public:
	CLASS_PROTOTYPE( rvCameraPortalSky );

							rvCameraPortalSky( void ) {}
							~rvCameraPortalSky( void ) {}

	// save games
	void					Save( anSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( anRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view );
};

/*
===============================================================================

rvCameraPlayback

===============================================================================
*/
class rvCameraPlayback : public idCamera {
public:
	CLASS_PROTOTYPE( rvCameraPlayback );

							rvCameraPlayback( void ) {}
							~rvCameraPlayback( void ) {}

	// save games
	void					Save( anSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( anRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view );

private:
	int						startTime;
	const rvDeclPlayback	*playback;
};


#endif /* !__GAME_CAMERA_H__ */
