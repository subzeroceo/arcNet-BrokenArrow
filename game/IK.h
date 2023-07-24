// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_IK_H__
#define __GAME_IK_H__

/*
===============================================================================

  IK base class with a simple fast two bone solver.

===============================================================================
*/

#define IK_ANIM				"ik_pose"

class arcPhysics;
class arcAnimator;

class idIK : public idClass {
public:
	CLASS_PROTOTYPE( idIK );

							idIK( void );
	virtual					~idIK( void );

	virtual bool			IsInitialized( void ) const;
	bool					IsInhibited( void ) const;

	virtual bool			Init( arcEntity *self, const char *anim, const arcVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

	static bool				SolveTwoBones( const arcVec3 &startPos, const arcVec3 &endPos, const arcVec3 &dir, float len0, float len1, arcVec3 &jointPos );
	static float			GetBoneAxis( const arcVec3 &startPos, const arcVec3 &endPos, const arcVec3 &dir, arcMat3 &axis );

	arcPhysics*				GetPhysics();
	arcAnimator*				GetAnimator();

protected:
	bool					initialized;
	bool					ik_activate;
	arcEntity *				self;				// entity using the animated model
	arcAnimator *			animator;			// animator on entity
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
	CLASS_PROTOTYPE( idIK_Walk );

							idIK_Walk( void );
	virtual					~idIK_Walk( void );

	virtual bool			Init( arcEntity *self, const char *anim, const arcVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

	void					EnableAll( void );
	void					DisableAll( void );
	void					EnableLeg( int num );
	void					DisableLeg( int num );

private:
	static const int		MAX_LEGS		= 8;

	arcClipModel *			footModel;

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
	CLASS_PROTOTYPE( idIK_Reach );

							idIK_Reach( void );
	virtual					~idIK_Reach( void );

	virtual bool			Init( arcEntity *self, const char *anim, const arcVec3 &modelOffset );
	virtual bool			Evaluate( void );
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

/*
===============================================================================

	IK controller for aiming ( sets of ) two joints at each other

===============================================================================
*/

class arcIK_Aim : public idIK {
public:
	CLASS_PROTOTYPE( arcIK_Aim );

							arcIK_Aim( void );
	virtual					~arcIK_Aim( void );

	virtual bool			Init( arcEntity *self, const char *anim, const arcVec3& modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

protected:
	typedef struct jointGroup_s {
		jointHandle_t			joint1;
		jointHandle_t			joint2;

		arcVec3					lastDir;
	} jointGroup_t;

	arcNetList< jointGroup_t >		jointGroups;
};

#endif /* !__GAME_IK_H__ */
