#ifndef __GAME_IK_H__
#define __GAME_IK_H__

/*
===============================================================================

  IK base class with a simple fast two bone solver.

===============================================================================
*/

#define IK_ANIM				"ik_pose"

class anPhysics;
class anAnimator;

class anIK : public anClass {
public:
	CLASS_PROTOTYPE( anIK );

							anIK( void );
	virtual					~anIK( void );

	virtual bool			IsInitialized( void ) const;
	bool					IsInhibited( void ) const;

	virtual bool			Init( anEntity *self, const char *anim, const anVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

	static bool				SolveTwoBones( const anVec3 &startPos, const anVec3 &endPos, const anVec3 &dir, float len0, float len1, anVec3 &jointPos );
	static float			GetBoneAxis( const anVec3 &startPos, const anVec3 &endPos, const anVec3 &dir, anMat3 &axis );

	anPhysics*				GetPhysics();
	anAnimator*				GetAnimator();

protected:
	bool					initialized;
	bool					ik_activate;
	anEntity *				self;				// entity using the animated model
	anAnimator *			animator;			// animator on entity
	int						modifiedAnim;		// animation modified by the IK
	anVec3					modelOffset;
};

/*
===============================================================================

  IK controller for a walking character with an arbitrary number of legs.

===============================================================================
*/

class anIK_Walk : public anIK {
public:
	CLASS_PROTOTYPE( anIK_Walk );

							anIK_Walk( void );
	virtual					~anIK_Walk( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual bool			Init( anEntity *self, const char *anim, const anVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

	void					EnableAll( void );
	void					DisableAll( void );
	void					EnableLeg( int num );
	void					DisableLeg( int num );

private:
	static const int		MAX_LEGS		= 8;

	anClipModel *			footModel;

	int						numLegs;
	int						enabledLegs;
	jointHandle_t			footJoints[MAX_LEGS];
	jointHandle_t			ankleJoints[MAX_LEGS];
	jointHandle_t			kneeJoints[MAX_LEGS];
	jointHandle_t			hipJoints[MAX_LEGS];
	jointHandle_t			dirJoints[MAX_LEGS];
	jointHandle_t			waistJoint;

	anVec3					hipForward[MAX_LEGS];
	anVec3					kneeForward[MAX_LEGS];

	float					upperLegLength[MAX_LEGS];
	float					lowerLegLength[MAX_LEGS];

	anMat3					upperLegToHipJoint[MAX_LEGS];
	anMat3					lowerLegToKneeJoint[MAX_LEGS];

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
	anVec3					pivotPos;
	bool					oldHeightsValid;
	float					oldWaistHeight;
	float					oldAnkleHeights[MAX_LEGS];
	anVec3					waistOffset;
};


/*
===============================================================================

  IK controller for reaching a position with an arm or leg.

===============================================================================
*/

class anIK_Reach : public anIK {
public:
	CLASS_PROTOTYPE( anIK_Reach );

							anIK_Reach( void );
	virtual					~anIK_Reach( void );

	virtual bool			Init( anEntity *self, const char *anim, const anVec3 &modelOffset );
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

	anVec3					shoulderForward[MAX_ARMS];
	anVec3					elbowForward[MAX_ARMS];

	float					upperArmLength[MAX_ARMS];
	float					lowerArmLength[MAX_ARMS];

	anMat3					upperArmToShoulderJoint[MAX_ARMS];
	anMat3					lowerArmToElbowJoint[MAX_ARMS];
};

/*
===============================================================================

	IK controller for aiming ( sets of ) two joints at each other

===============================================================================
*/

class anIK_Aim : public anIK {
public:
	CLASS_PROTOTYPE( anIK_Aim );

							anIK_Aim( void );
	virtual					~anIK_Aim( void );

	virtual bool			Init( anEntity *self, const char *anim, const anVec3 &modelOffset );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

protected:
	typedef struct jointGroup_s {
		jointHandle_t			joint1;
		jointHandle_t			joint2;

		anVec3					lastDir;
	} jointGroup_t;

	anList< jointGroup_t >		jointGroups;
};

/*
===============================================================================

	IK controller for wheel based vehicle suspension

===============================================================================
*/

class anVehicleRigidBodyWheel;
class anTransport_RB;
//class anMotorSound;

class anIK_WheeledVehicle : public anIK {
public:
	CLASS_PROTOTYPE( anIK_WheeledVehicle );

							anIK_WheeledVehicle( void );
	virtual					~anIK_WheeledVehicle( void );

	void					AddWheel( anVehicleRigidBodyWheel &wheel );
	void					ClearWheels( void );
	virtual bool			Evaluate( void );
	virtual void			ClearJointMods( void );

	bool					Init( anTransport_RB *self, const char *anim, const anVec3 &modelOffset );

	int						GetNumWheels( void ) const { return wheels.Num(); }

protected:
	anList<anVehicleRigidBodyWheel *>	wheels;
	anTransport_RB *					rbParent;
};

class anTransport;
class snVehiclePosition;
class anBasePlayer;

class anVehicleIKSystem : public anClass {
public:
	ABSTRACT_PROTOTYPE( anVehicleIKSystem );
	//virtual					anVehicleIKSystem( void ) {}
	virtual					~anVehicleIKSystem( void ) {}

	void					InitClamp( const angleClamp_t &yaw, const angleClamp_t &pitch );
	void					SetPosition( sdVehiclePosition *_position ) { position = _position; }
	virtual anBasePlayer *	GetPlayer( void );

	virtual bool			Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms );
	virtual void			Update( void ) = 0;

protected:
	anTransport *				vehicle;
	angleClamp_t				clampYaw;
	angleClamp_t				clampPitch;
	anVehiclePosition *			position;
	anVehicleWeapon *			weapon;
};

class anVehicleSwivel : public anVehicleIKSystem {
public:
	CLASS_PROTOTYPE( anVehicleSwivel );

	virtual void			Update( void );
	virtual bool			Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms );

protected:
	anMotorSound*			yawSound;

	anAngles				angles;
	anMat3					baseAxis;
	jointHandle_t			joint;
};

class anVehicleWeaponAimer : public anVehicleIKSystem {
public:
	CLASS_PROTOTYPE( anVehicleWeaponAimer );

	virtual void			Update( void );
	virtual bool			Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms );

protected:
	idScriptedEntityHelper_Aimer aimer;
};

class anVehicleJointAimer : public anVehicleIKSystem {
public:
	CLASS_PROTOTYPE( anVehicleJointAimer );

	virtual void			Update( void );
	virtual bool			Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms );
	virtual anBasePlayer *	GetPlayer( void );

protected:
	anMotorSound *			yawSound;
	anMotorSound *			pitchSound;

	anAngles				angles;
	anMat3					baseAxis;
	jointHandle_t			joint;
	sdVehicleWeapon *		weapon2;
};

class anVehicleIK_Steering : public anVehicleIKSystem {
public:
	CLASS_PROTOTYPE( anVehicleIK_Steering );

							anVehicleIK_Steering( void );
	virtual					~anVehicleIK_Steering( void );

	virtual void			Update( void );
	virtual bool			Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms );

private:
	anPlayerArmIK			ik;
};

#endif // !__GAME_IK_H__
