
#ifndef __GAME_IK_H__
#define __GAME_IK_H__

/*
===============================================================================

  IK base class with a simple fast two bone solver.

===============================================================================
*/

#define IK_ANIM				"ik_pose"

class idIK {
public:
							idIK( void );
	virtual					~idIK( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	bool					IsInitialized( void ) const;

	virtual bool			Init( idEntity *self, const char *anim, const arcVec3 &modelOffset );
	virtual void			Evaluate( void );
	virtual void			ClearJointMods( void );

	bool					SolveTwoBones( const arcVec3 &startPos, const arcVec3 &endPos, const arcVec3 &dir, float len0, float len1, arcVec3 &jointPos );
	float					GetBoneAxis( const arcVec3 &startPos, const arcVec3 &endPos, const arcVec3 &dir, arcMat3 &axis );

protected:
	bool					initialized;
	bool					ik_activate;
	idEntity *				self;				// entity using the animated model
	idAnimator *			animator;			// animator on entity
	int						modifiedAnim;		// animation modified by the IK
	arcVec3					modelOffset;
};


/*
===============================================================================

  IK controller for a walking character with an arbitrary number of legs.

===============================================================================
*/

class idIK_Walk : public idIK {
public:

							idIK_Walk( void );
	virtual					~idIK_Walk( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual bool			Init( idEntity *self, const char *anim, const arcVec3 &modelOffset );
	virtual void			Evaluate( void );
	virtual void			ClearJointMods( void );

	void					EnableAll( void );
	void					DisableAll( void );
	void					EnableLeg( int num );
	void					DisableLeg( int num );

private:
	static const int		MAX_LEGS		= 8;

	idClipModel *			footModel;

	int						numLegs;
	int						enabledLegs;
	jointHandle_t			footJoints[MAX_LEGS];
	jointHandle_t			ankleJoints[MAX_LEGS];
	jointHandle_t			kneeJoints[MAX_LEGS];
	jointHandle_t			hipJoints[MAX_LEGS];
	jointHandle_t			dirJoints[MAX_LEGS];
	jointHandle_t			waistJoint;

	arcVec3					hipForward[MAX_LEGS];
	arcVec3					kneeForward[MAX_LEGS];

	float					upperLegLength[MAX_LEGS];
	float					lowerLegLength[MAX_LEGS];

	arcMat3					upperLegToHipJoint[MAX_LEGS];
	arcMat3					lowerLegToKneeJoint[MAX_LEGS];

	float					smoothing;
	float					waistSmoothing;
	float					footShift;
	float					waistShift;
	float					minWaistFloorDist;
	float					minWaistAnkleDist;
	float					footUpTrace;
	float					footDownTrace;
	bool					tiltWaist;
	bool					usePivot;

	// state
	int						pivotFoot;
	float					pivotYaw;
	arcVec3					pivotPos;
	bool					oldHeightsValid;
	float					oldWaistHeight;
	float					oldAnkleHeights[MAX_LEGS];
	arcVec3					waistOffset;
};


/*
===============================================================================

  IK controller for reaching a position with an arm or leg.

===============================================================================
*/

class idIK_Reach : public idIK {
public:

							idIK_Reach( void );
	virtual					~idIK_Reach( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual bool			Init( idEntity *self, const char *anim, const arcVec3 &modelOffset );
	virtual void			Evaluate( void );
	virtual void			ClearJointMods( void );

private:

	static const int		MAX_ARMS	= 2;

	int						numArms;
	int						enabledArms;
	jointHandle_t			handJoints[MAX_ARMS];
	jointHandle_t			elbowJoints[MAX_ARMS];
	jointHandle_t			shoulderJoints[MAX_ARMS];
	jointHandle_t			dirJoints[MAX_ARMS];

	arcVec3					shoulderForward[MAX_ARMS];
	arcVec3					elbowForward[MAX_ARMS];

	float					upperArmLength[MAX_ARMS];
	float					lowerArmLength[MAX_ARMS];

	arcMat3					upperArmToShoulderJoint[MAX_ARMS];
	arcMat3					lowerArmToElbowJoint[MAX_ARMS];
};

#endif /* !__GAME_IK_H__ */
