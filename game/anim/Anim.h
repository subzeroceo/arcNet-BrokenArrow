#ifndef __ANIM_H__
#define __ANIM_H__

//
// animation channels
// these can be changed by modmakers and licensees to be whatever they need.
const int ANIM_NumAnimChannels		= 5;
const int ANIM_MaxAnimsPerChannel	= 3;
const int ANIM_MaxSyncedAnims		= 3;

//
// animation channels.  make sure to change script/doom_defs.script if you add any channels, or change their order
//
const int ANIMCHANNEL_ALL			= 0;
const int ANIMCHANNEL_TORSO			= 1;
const int ANIMCHANNEL_LEGS			= 2;
const int ANIMCHANNEL_HEAD			= 3;
const int ANIMCHANNEL_EYELIDS		= 4;

// for converting from 24 frames per second to milliseconds
inline int FRAME2MS( int frameNum ) {
	return ( frameNum * 1000 ) / 24;
}

class anRenderModel;
class anAnimator;
class anAnimBlend;
class function_t;
class anEntity;
class anSaveGame;
class anRestoreGame;

typedef struct {
	int		cycleCount;	// how many times the anim has wrapped to the begining (0 for clamped anims)
	int		frame1;
	int		frame2;
	float	frontlerp;
	float	backlerp;
} frameBlend_t;

typedef struct {
	int						nameIndex;
	int						parentNum;
	int						animBits;
	int						firstComponent;
} jointAnimInfo_t;

typedef struct {
	jointHandle_t			num;
	jointHandle_t			parentNum;
	int						channel;
} jointInfo_t;

//
// joint modifier modes.  make sure to change script/doom_defs.script if you add any, or change their order.
//
typedef enum {
	JOINTMOD_NONE,				// no modification
	JOINTMOD_LOCAL,				// modifies the joint's position or orientation in joint local space
	JOINTMOD_LOCAL_OVERRIDE,	// sets the joint's position or orientation in joint local space
	JOINTMOD_WORLD,				// modifies joint's position or orientation in model space
	JOINTMOD_WORLD_OVERRIDE		// sets the joint's position or orientation in model space
} jointModTransform_t;

typedef struct {
	jointHandle_t			jointnum;
	anMat3					mat;
	anVec3					pos;
	jointModTransform_t		transform_pos;
	jointModTransform_t		transform_axis;
} jointMod_t;

#define	ANIM_BIT_TX			0
#define	ANIM_BIT_TY			1
#define	ANIM_BIT_TZ			2
#define	ANIM_BIT_QX			3
#define	ANIM_BIT_QY			4
#define	ANIM_BIT_QZ			5

#define	ANIM_TX				BIT( ANIM_BIT_TX )
#define	ANIM_TY				BIT( ANIM_BIT_TY )
#define	ANIM_TZ				BIT( ANIM_BIT_TZ )
#define	ANIM_QX				BIT( ANIM_BIT_QX )
#define	ANIM_QY				BIT( ANIM_BIT_QY )
#define	ANIM_QZ				BIT( ANIM_BIT_QZ )

typedef enum {
	FC_SCRIPTFUNCTION,
	FC_SCRIPTFUNCTIONOBJECT,
	FC_EVENTFUNCTION,
	FC_SOUND,
	FC_SOUND_VOICE,
	FC_SOUND_VOICE2,
	FC_SOUND_BODY,
	FC_SOUND_BODY2,
	FC_SOUND_BODY3,
	FC_SOUND_WEAPON,
	FC_SOUND_ITEM,
	FC_SOUND_GLOBAL,
	FC_SOUND_CHATTER,
	FC_SKIN,
	FC_TRIGGER,
	FC_TRIGGER_SMOKE_PARTICLE,
	FC_MELEE,
	FC_DIRECTDAMAGE,
	FC_BEGINATTACK,
	FC_ENDATTACK,
	FC_MUZZLEFLASH,
	FC_CREATEMISSILE,
	FC_LAUNCHMISSILE,
	FC_FIREMISSILEATTARGET,
	FC_FOOTSTEP,
	FC_LEFTFOOT,
	FC_RIGHTFOOT,
	FC_ENABLE_EYE_FOCUS,
	FC_DISABLE_EYE_FOCUS,
	FC_FX,
	FC_DISABLE_GRAVITY,
	FC_ENABLE_GRAVITY,
	FC_JUMP,
	FC_ENABLE_CLIP,
	FC_DISABLE_CLIP,
	FC_ENABLE_WALK_IK,
	FC_DISABLE_WALK_IK,
	FC_ENABLE_LEG_IK,
	FC_DISABLE_LEG_IK,
	FC_RECORDDEMO,
	FC_AVIGAME,
	FC_LAUNCH_PROJECTILE,
	FC_TRIGGER_FX,
	FC_START_EMITTER,
	FC_STOP_EMITTER,
} frameCommandType_t;

typedef struct {
	int						num;
	int						firstCommand;
} frameLookup_t;

typedef struct {
	frameCommandType_t		type;
	anStr					*string;

	union {
		const idSoundShader	*soundShader;
		const function_t	*function;
		const anDeclSkin	*skin;
		int					index;
	};
} frameCommand_t;

typedef struct {
	bool					prevent_idle_override		: 1;
	bool					random_cycle_start			: 1;
	bool					ai_no_turn					: 1;
	bool					anim_turn					: 1;
} animFlags_t;

/*
==============================================================================================

	anMD6Anim

==============================================================================================
*/

class anMD6Anim {
private:
	int						numFrames;
	int						frameRate;
	int						animLength;
	int						numJoints;
	int						numAnimatedComponents;
	anList<anBounds, TAG_MD5_ANIM>		bounds;
	anList<jointAnimInfo_t, TAG_MD5_ANIM>	jointInfo;
	anList<anJointQuat, TAG_MD5_ANIM>		baseFrame;
	anList<float, TAG_MD5_ANIM>			componentFrames;
	anStr					name;
	anVec3					totaldelta;
	mutable int				refCount;

public:
							anMD6Anim();
							~anMD6Anim();

	void					Free();
	bool					Reload();
	size_t					Allocated() const;
	size_t					Size() const { return sizeof( *this ) + Allocated(); };
	bool					LoadAnim( const char *filename );
	bool					LoadBinary( anFile * file, ARC_TIME_T sourceTimeStamp );
	void					WriteBinary( anFile * file, ARC_TIME_T sourceTimeStamp );

	void					IncreaseRefs() const;
	void					DecreaseRefs() const;
	int						NumRefs() const;

	void					CheckModelHierarchy( const anRenderModel *model ) const;
	void					GetInterpolatedFrame( frameBlend_t &frame, anJointQuat *joints, const int *index, int numIndexes ) const;
	void					GetSingleFrame( int frameNum, anJointQuat *joints, const int *index, int numIndexes ) const;
	int						Length() const;
	int						NumFrames() const;
	int						NumJoints() const;
	const anVec3			&TotalMovementDelta() const;
	const char				*Name() const;

	void					GetFrameBlend( int frameNum, frameBlend_t &frame ) const;	// frame 1 is first frame
	void					ConvertTimeToFrame( int time, int cyclecount, frameBlend_t &frame ) const;

	void					GetOrigin( anVec3 &offset, int currentTime, int cyclecount ) const;
	void					GetOriginRotation( anQuat &rotation, int time, int cyclecount ) const;
	void					GetBounds( anBounds &bounds, int currentTime, int cyclecount ) const;
};

/*
==============================================================================================

	anAnim

==============================================================================================
*/

class anAnim {
private:
	const class anDeclModelDef	*modelDef;
	const anMD6Anim				*anims[ ANIM_MaxSyncedAnims ];
	int							numAnims;
	anStr						name;
	anStr						realname;
	anList<frameLookup_t, TAG_ANIM>		frameLookup;
	anList<frameCommand_t, TAG_ANIM>		frameCommands;
	animFlags_t					flags;

public:
								anAnim();
								anAnim( const anDeclModelDef *modelDef, const anAnim *anim );
								~anAnim();

	void						SetAnim( const anDeclModelDef *modelDef, const char *sourcename, const char *animname, int num, const anMD6Anim *md5anims[ ANIM_MaxSyncedAnims ] );
	const char					*Name() const;
	const char					*FullName() const;
	const anMD6Anim				*MD5Anim( int num ) const;
	const anDeclModelDef		*ModelDef() const;
	int							Length() const;
	int							NumFrames() const;
	int							NumAnims() const;
	const anVec3				&TotalMovementDelta() const;
	bool						GetOrigin( anVec3 &offset, int animNum, int time, int cyclecount ) const;
	bool						GetOriginRotation( anQuat &rotation, int animNum, int currentTime, int cyclecount ) const;
	bool						GetBounds( anBounds &bounds, int animNum, int time, int cyclecount ) const;
	const char					*AddFrameCommand( const class anDeclModelDef *modelDef, int frameNum, anLexer &src, const anDict *def );
	void						CallFrameCommands( anEntity *ent, int from, int to ) const;
	bool						HasFrameCommands() const;

								// returns first frame (zero based) that command occurs.  returns -1 if not found.
	int							FindFrameForFrameCommand( frameCommandType_t framecommand, const frameCommand_t **command ) const;
	void						SetAnimFlags( const animFlags_t &animflags );
	const animFlags_t			&GetAnimFlags() const;
};

/*
==============================================================================================

	anDeclModelDef

==============================================================================================
*/

class anDeclModelDef : public anDecl {
public:
								anDeclModelDef();
								~anDeclModelDef();

	virtual size_t				Size() const;
	virtual const char *		DefaultDefinition() const;
	virtual bool				Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void				FreeData();

	void						Touch() const;

	const anDeclSkin *			GetDefaultSkin() const;
	const anJointQuat *			GetDefaultPose() const;
	void						SetupJoints( int *numJoints, anJointMat **jointList, anBounds &frameBounds, bool removeOriginOffset ) const;
	anRenderModel *				ModelHandle() const;
	void						GetJointList( const char *jointnames, anList<jointHandle_t> &jointList ) const;
	const jointInfo_t *			FindJoint( const char *name ) const;

	int							NumAnims() const;
	const anAnim *				GetAnim( int index ) const;
	int							GetSpecificAnim( const char *name ) const;
	int							GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;
	const anDeclSkin *			GetSkin() const;
	const char *				GetModelName() const;
	const anList<jointInfo_t> &	Joints() const;
	const int *					JointParents() const;
	int							NumJoints() const;
	const jointInfo_t *			GetJoint( int jointHandle ) const;
	const char *				GetJointName( int jointHandle ) const;
	int							NumJointsOnChannel( int channel ) const;
	const int *					GetChannelJoints( int channel ) const;

	const anVec3 &				GetVisualOffset() const;

private:
	void						CopyDecl( const anDeclModelDef *decl );
	bool						ParseAnim( anLexer &src, int numDefaultAnims );

private:
	anVec3						offset;
	anList<jointInfo_t, TAG_ANIM>			joints;
	anList<int, TAG_ANIM>					jointParents;
	anList<int, TAG_ANIM>					channelJoints[ ANIM_NumAnimChannels ];
	anRenderModel *				modelHandle;
	anList<anAnim *, TAG_ANIM>			anims;
	const anDeclSkin *			skin;
};

/*
==============================================================================================

	anAnimBlend

==============================================================================================
*/

class anAnimBlend {
private:
	const class anDeclModelDef	*modelDef;
	int							starttime;
	int							endtime;
	int							timeOffset;
	float						rate;

	int							blendStartTime;
	int							blendDuration;
	float						blendStartValue;
	float						blendEndValue;

	float						animWeights[ ANIM_MaxSyncedAnims ];
	short						cycle;
	short						frame;
	short						animNum;
	bool						allowMove;
	bool						allowFrameCommands;

	friend class				anAnimator;

	void						Reset( const anDeclModelDef *_modelDef );
	void						CallFrameCommands( anEntity *ent, int fromtime, int totime ) const;
	void						SetFrame( const anDeclModelDef *modelDef, int animnum, int frame, int currenttime, int blendtime );
	void						CycleAnim( const anDeclModelDef *modelDef, int animnum, int currenttime, int blendtime );
	void						PlayAnim( const anDeclModelDef *modelDef, int animnum, int currenttime, int blendtime );
	bool						BlendAnim( int currentTime, int channel, int numJoints, anJointQuat *blendFrame, float &blendWeight, bool removeOrigin, bool overrideBlend, bool printInfo ) const;
	void						BlendOrigin( int currentTime, anVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const;
	void						BlendDelta( int fromtime, int totime, anVec3 &blendDelta, float &blendWeight ) const;
	void						BlendDeltaRotation( int fromtime, int totime, anQuat &blendDelta, float &blendWeight ) const;
	bool						AddBounds( int currentTime, anBounds &bounds, bool removeOriginOffset ) const;

public:
								anAnimBlend();
	void						Save( anSaveGame *savefile ) const;
	void						Restore( anRestoreGame *savefile, const anDeclModelDef *modelDef );
	const char					*AnimName() const;
	const char					*AnimFullName() const;
	float						GetWeight( int currenttime ) const;
	float						GetFinalWeight() const;
	void						SetWeight( float newweight, int currenttime, int blendtime );
	int							NumSyncedAnims() const;
	bool						SetSyncedAnimWeight( int num, float weight );
	void						Clear( int currentTime, int clearTime );
	bool						IsDone( int currentTime ) const;
	bool						FrameHasChanged( int currentTime ) const;
	int							GetCycleCount() const;
	void						SetCycleCount( int count );
	void						SetPlaybackRate( int currentTime, float newRate );
	float						GetPlaybackRate() const;
	void						SetStartTime( int startTime );
	int							GetStartTime() const;
	int							GetEndTime() const;
	int							GetFrameNumber( int currenttime ) const;
	int							AnimTime( int currenttime ) const;
	int							NumFrames() const;
	int							Length() const;
	int							PlayLength() const;
	void						AllowMovement( bool allow );
	void						AllowFrameCommands( bool allow );
	const anAnim				*Anim() const;
	int							AnimNum() const;
};

/*
==============================================================================================

	anAFPoseJointMod

==============================================================================================
*/

typedef enum {
	AF_JOINTMOD_AXIS,
	AF_JOINTMOD_ORIGIN,
	AF_JOINTMOD_BOTH
} AFJointModType_t;

class anAFPoseJointMod {
public:
								anAFPoseJointMod();

	AFJointModType_t			mod;
	anMat3						axis;
	anVec3						origin;
};

inline anAFPoseJointMod::anAFPoseJointMod() {
	mod = AF_JOINTMOD_AXIS;
	axis.Identity();
	origin.Zero();
}

/*
==============================================================================================

	anAnimator

==============================================================================================
*/

class anAnimator {
public:
								anAnimator();
								~anAnimator();

	size_t						Allocated() const;
	size_t						Size() const;

	void						Save( anSaveGame *savefile ) const;					// archives object for save game file
	void						Restore( anRestoreGame *savefile );					// unarchives object from save game file

	void						SetEntity( anEntity *ent );
	anEntity					*GetEntity() const ;
	void						RemoveOriginOffset( bool remove );
	bool						RemoveOrigin() const;

	void						GetJointList( const char *jointnames, anList<jointHandle_t> &jointList ) const;

	int							NumAnims() const;
	const anAnim				*GetAnim( int index ) const;
	int							GetAnim( const char *name ) const;
	bool						HasAnim( const char *name ) const;

	void						ServiceAnims( int fromtime, int totime );
	bool						IsAnimating( int currentTime ) const;

	void						GetJoints( int *numJoints, anJointMat **jointsPtr );
	int							NumJoints() const;
	jointHandle_t				GetFirstChild( jointHandle_t jointnum ) const;
	jointHandle_t				GetFirstChild( const char *name ) const;

	anRenderModel				*SetModel( const char *modelname );
	anRenderModel				*ModelHandle() const;
	const anDeclModelDef		*ModelDef() const;

	void						ForceUpdate();
	void						ClearForceUpdate();
	bool						CreateFrame( int animtime, bool force );
	bool						FrameHasChanged( int animtime ) const;
	void						GetDelta( int fromtime, int totime, anVec3 &delta ) const;
	bool						GetDeltaRotation( int fromtime, int totime, anMat3 &delta ) const;
	void						GetOrigin( int currentTime, anVec3 &pos ) const;
	bool						GetBounds( int currentTime, anBounds &bounds );

	anAnimBlend					*CurrentAnim( int channelNum );
	void						Clear( int channelNum, int currentTime, int cleartime );
	void						SetFrame( int channelNum, int animnum, int frame, int currenttime, int blendtime );
	void						CycleAnim( int channelNum, int animnum, int currenttime, int blendtime );
	void						PlayAnim( int channelNum, int animnum, int currenttime, int blendTime );

								// copies the current anim from fromChannelNum to channelNum.
								// the copied anim will have frame commands disabled to avoid executing them twice.
	void						SyncAnimChannels( int channelNum, int fromChannelNum, int currenttime, int blendTime );

	void						SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const anVec3 &pos );
	void						SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const anMat3 &mat );
	void						ClearJoint( jointHandle_t jointnum );
	void						ClearAllJoints();

	void						InitAFPose();
	void						SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const anMat3 &axis, const anVec3 &origin );
	void						FinishAFPose( int animnum, const anBounds &bounds, const int time );
	void						SetAFPoseBlendWeight( float blendWeight );
	bool						BlendAFPose( anJointQuat *blendFrame ) const;
	void						ClearAFPose();

	void						ClearAllAnims( int currentTime, int cleartime );

	jointHandle_t				GetJointHandle( const char *name ) const;
	const char *				GetJointName( jointHandle_t handle ) const;
	int							GetChannelForJoint( jointHandle_t joint ) const;
	bool						GetJointTransform( jointHandle_t jointHandle, int currenttime, anVec3 &offset, anMat3 &axis );
	bool						GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, anVec3 &offset, anMat3 &axis );

	const animFlags_t			GetAnimFlags( int animnum ) const;
	int							NumFrames( int animnum ) const;
	int							NumSyncedAnims( int animnum ) const;
	const char					*AnimName( int animnum ) const;
	const char					*AnimFullName( int animnum ) const;
	int							AnimLength( int animnum ) const;
	const anVec3				&TotalMovementDelta( int animnum ) const;

private:
	void						FreeData();
	void						PushAnims( int channel, int currentTime, int blendTime );

private:
	const anDeclModelDef *		modelDef;
	anEntity *					entity;

	anAnimBlend					channels[ ANIM_NumAnimChannels ][ ANIM_MaxAnimsPerChannel ];
	anList<jointMod_t *, TAG_ANIM>		jointMods;
	int							numJoints;
	anJointMat *				joints;

	mutable int					lastTransformTime;		// mutable because the value is updated in CreateFrame
	mutable bool				stoppedAnimatingUpdate;
	bool						removeOriginOffset;
	bool						forceUpdate;

	anBounds					frameBounds;

	float						AFPoseBlendWeight;
	anList<int, TAG_ANIM>		AFPoseJoints;
	anList<anAFPoseJointMod, TAG_ANIM> AFPoseJointMods;
	anList<anJointQuat, TAG_ANIM>AFPoseJointFrame;
	anBounds					AFPoseBounds;
	int							AFPoseTime;
};

/*
==============================================================================================

	anAnimManager

==============================================================================================
*/

class anAnimManager {
public:
								anAnimManager();
								~anAnimManager();

	static bool					forceExport;

	void						Shutdown();
	anMD6Anim *					GetAnim( const char *name );
	void						Preload( const idPreloadManifest &manifest );
	void						ReloadAnims();
	void						ListAnims() const;
	int							JointIndex( const char *name );
	const char *				JointName( int index ) const;

	void						ClearAnimsInUse();
	void						FlushUnusedAnims();

private:
	idHashTable<anMD6Anim *>	animations;
	anStringList				jointnames;
	anHashIndex					jointnamesHash;
};

#endif // !__ANIM_H__