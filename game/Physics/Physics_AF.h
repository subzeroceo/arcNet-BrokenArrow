
#ifndef __PHYSICS_AF_H__
#define __PHYSICS_AF_H__

/*
===================================================================================

	Articulated Figure physics

	Employs a constraint force based dynamic simulation using a lagrangian
	multiplier method to solve for the constraint forces.

===================================================================================
*/

class idAFConstraint;
class idAFConstraint_Fixed;
class idAFConstraint_BallAndSocketJoint;
class idAFConstraint_BallAndSocketJointFriction;
class idAFConstraint_UniversalJoint;
class idAFConstraint_UniversalJointFriction;
class idAFConstraint_CylindricalJoint;
class idAFConstraint_Hinge;
class idAFConstraint_HingeFriction;
class idAFConstraint_HingeSteering;
class idAFConstraint_Slider;
class idAFConstraint_Line;
class idAFConstraint_Plane;
class idAFConstraint_Spring;
class idAFConstraint_Contact;
class idAFConstraint_ContactFriction;
class idAFConstraint_ConeLimit;
class idAFConstraint_PyramidLimit;
class idAFBody;
class idAFTree;
class anPhysics_AF;

typedef enum {
	CONSTRAINT_INVALID,
	CONSTRAINT_FIXED,
	CONSTRAINT_BALLANDSOCKETJOINT,
	CONSTRAINT_UNIVERSALJOINT,
	CONSTRAINT_HINGE,
	CONSTRAINT_HINGESTEERING,
	CONSTRAINT_SLIDER,
	CONSTRAINT_CYLINDRICALJOINT,
	CONSTRAINT_LINE,
	CONSTRAINT_PLANE,
	CONSTRAINT_SPRING,
	CONSTRAINT_CONTACT,
	CONSTRAINT_FRICTION,
	CONSTRAINT_CONELIMIT,
	CONSTRAINT_PYRAMIDLIMIT
} constraintType_t;


//===============================================================
//
//	idAFConstraint
//
//===============================================================

// base class for all constraints
class idAFConstraint {

	friend class anPhysics_AF;
	friend class idAFTree;

public:
							idAFConstraint( void );
	virtual					~idAFConstraint( void );
	constraintType_t		GetType( void ) const { return type; }
	const anString &			GetName( void ) const { return name; }
	idAFBody *				GetBody1( void ) const { return body1; }
	idAFBody *				GetBody2( void ) const { return body2; }
	void					SetPhysics( anPhysics_AF *p ) { physics = p; }
	const anVecX &			GetMultiplier( void );
	virtual void			SetBody1( idAFBody *body );
	virtual void			SetBody2( idAFBody *body );
	virtual void			DebugDraw( void );
	virtual void			GetForce( idAFBody *body, anVec6 &force );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			GetCenter( anVec3 &center );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	constraintType_t		type;						// constraint type
	anString					name;						// name of constraint
	idAFBody *				body1;						// first constrained body
	idAFBody *				body2;						// second constrained body, nullptr for world
	anPhysics_AF *			physics;					// for adding additional constraints like limits

							// simulation variables set by Evaluate
	anMatX					J1, J2;						// matrix with left hand side of constraint equations
	anVecX					c1, c2;						// right hand side of constraint equations
	anVecX					lo, hi, e;					// low and high bounds and lcp epsilon
	idAFConstraint *		boxConstraint;				// constraint the boxIndex refers to
	int						boxIndex[6];				// indexes for special box constrained variables

							// simulation variables used during calculations
	anMatX					invI;						// transformed inertia
	anMatX					J;							// transformed constraint matrix
	anVecX					s;							// temp solution
	anVecX					lm;							// lagrange multipliers
	int						firstIndex;					// index of the first constraint row in the lcp matrix

	struct constraintFlags_s {
		bool				allowPrimary		: 1;	// true if the constraint can be used as a primary constraint
		bool				frameConstraint		: 1;	// true if this constraint is added to the frame constraints
		bool				noCollision			: 1;	// true if body1 and body2 never collide with each other
		bool				isPrimary			: 1;	// true if this is a primary constraint
		bool				isZero				: 1;	// true if 's' is zero during calculations
	} fl;

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
	void					InitSize( int size );
};

// fixed or rigid joint which allows zero degrees of freedom
// constrains body1 to have a fixed position and orientation relative to body2
class idAFConstraint_Fixed : public idAFConstraint {

public:
							idAFConstraint_Fixed( const anString &name, idAFBody *body1, idAFBody *body2 );
	void					SetRelativeOrigin( const anVec3 &origin ) { this->offset = origin; }
	void					SetRelativeAxis( const anMat3 &axis ) { this->relAxis = axis; }
	virtual void			SetBody1( idAFBody *body );
	virtual void			SetBody2( idAFBody *body );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			GetCenter( anVec3 &center );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					offset;						// offset of body1 relative to body2 in body2 space
	anMat3					relAxis;					// rotation of body1 relative to body2

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
	void					InitOffset( void );
};

// ball and socket or spherical joint which allows 3 degrees of freedom
// constrains body1 relative to body2 with a ball and socket joint
class idAFConstraint_BallAndSocketJoint : public idAFConstraint {

public:
							idAFConstraint_BallAndSocketJoint( const anString &name, idAFBody *body1, idAFBody *body2 );
							~idAFConstraint_BallAndSocketJoint( void );
	void					SetAnchor( const anVec3 &worldPosition );
	anVec3					GetAnchor( void ) const;
	void					SetNoLimit( void );
	void					SetConeLimit( const anVec3 &coneAxis, const float coneAngle, const anVec3 &body1Axis );
	void					SetPyramidLimit( const anVec3 &pyramidAxis, const anVec3 &baseAxis,
											const float angle1, const float angle2, const anVec3 &body1Axis );
	void					SetLimitEpsilon( const float e );
	void					SetFriction( const float f ) { friction = f; }
	float					GetFriction( void ) const;
	virtual void			DebugDraw( void );
	virtual void			GetForce( idAFBody *body, anVec6 &force );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			GetCenter( anVec3 &center );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					anchor1;					// anchor in body1 space
	anVec3					anchor2;					// anchor in body2 space
	float					friction;					// joint friction
	idAFConstraint_ConeLimit *coneLimit;				// cone shaped limit
	idAFConstraint_PyramidLimit *pyramidLimit;			// pyramid shaped limit
	idAFConstraint_BallAndSocketJointFriction *fc;		// friction constraint

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// ball and socket joint friction
class idAFConstraint_BallAndSocketJointFriction : public idAFConstraint {

public:
							idAFConstraint_BallAndSocketJointFriction( void );
	void					Setup( idAFConstraint_BallAndSocketJoint *cc );
	bool					Add( anPhysics_AF *phys, float invTimeStep );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );

protected:
	idAFConstraint_BallAndSocketJoint *joint;

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// universal, Cardan or Hooke joint which allows 2 degrees of freedom
// like a ball and socket joint but also constrains the rotation about the cardan shafts
class idAFConstraint_UniversalJoint : public idAFConstraint {

public:
							idAFConstraint_UniversalJoint( const anString &name, idAFBody *body1, idAFBody *body2 );
							~idAFConstraint_UniversalJoint( void );
	void					SetAnchor( const anVec3 &worldPosition );
	anVec3					GetAnchor( void ) const;
	void					SetShafts( const anVec3 &cardanShaft1, const anVec3 &cardanShaft2 );
	void					GetShafts( anVec3 &cardanShaft1, anVec3 &cardanShaft2 ) { cardanShaft1 = shaft1; cardanShaft2 = shaft2; }
	void					SetNoLimit( void );
	void					SetConeLimit( const anVec3 &coneAxis, const float coneAngle );
	void					SetPyramidLimit( const anVec3 &pyramidAxis, const anVec3 &baseAxis,
											const float angle1, const float angle2 );
	void					SetLimitEpsilon( const float e );
	void					SetFriction( const float f ) { friction = f; }
	float					GetFriction( void ) const;
	virtual void			DebugDraw( void );
	virtual void			GetForce( idAFBody *body, anVec6 &force );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			GetCenter( anVec3 &center );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					anchor1;					// anchor in body1 space
	anVec3					anchor2;					// anchor in body2 space
	anVec3					shaft1;						// body1 cardan shaft in body1 space
	anVec3					shaft2;						// body2 cardan shaft in body2 space
	anVec3					axis1;						// cardan axis in body1 space
	anVec3					axis2;						// cardan axis in body2 space
	float					friction;					// joint friction
	idAFConstraint_ConeLimit *coneLimit;				// cone shaped limit
	idAFConstraint_PyramidLimit *pyramidLimit;			// pyramid shaped limit
	idAFConstraint_UniversalJointFriction *fc;			// friction constraint

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// universal joint friction
class idAFConstraint_UniversalJointFriction : public idAFConstraint {

public:
							idAFConstraint_UniversalJointFriction( void );
	void					Setup( idAFConstraint_UniversalJoint *cc );
	bool					Add( anPhysics_AF *phys, float invTimeStep );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );

protected:
	idAFConstraint_UniversalJoint *joint;			// universal joint

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// cylindrical joint which allows 2 degrees of freedom
// constrains body1 to lie on a line relative to body2 and allows only translation along and rotation about the line
class idAFConstraint_CylindricalJoint : public idAFConstraint {

public:
							idAFConstraint_CylindricalJoint( const anString &name, idAFBody *body1, idAFBody *body2 );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );

protected:

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// hinge, revolute or pin joint which allows 1 degree of freedom
// constrains all motion of body1 relative to body2 except the rotation about the hinge axis
class idAFConstraint_Hinge : public idAFConstraint {

public:
							idAFConstraint_Hinge( const anString &name, idAFBody *body1, idAFBody *body2 );
							~idAFConstraint_Hinge( void );
	void					SetAnchor( const anVec3 &worldPosition );
	anVec3					GetAnchor( void ) const;
	void					SetAxis( const anVec3 &axis );
	void					GetAxis( anVec3 &a1, anVec3 &a2 ) const { a1 = axis1; a2 = axis2; }
	anVec3					GetAxis( void ) const;
	void					SetNoLimit( void );
	void					SetLimit( const anVec3 &axis, const float angle, const anVec3 &body1Axis );
	void					SetLimitEpsilon( const float e );
	float					GetAngle( void ) const;
	void					SetSteerAngle( const float degrees );
	void					SetSteerSpeed( const float speed );
	void					SetFriction( const float f ) { friction = f; }
	float					GetFriction( void ) const;
	virtual void			DebugDraw( void );
	virtual void			GetForce( idAFBody *body, anVec6 &force );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			GetCenter( anVec3 &center );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					anchor1;					// anchor in body1 space
	anVec3					anchor2;					// anchor in body2 space
	anVec3					axis1;						// axis in body1 space
	anVec3					axis2;						// axis in body2 space
	anMat3					initialAxis;				// initial axis of body1 relative to body2
	float					friction;					// hinge friction
	idAFConstraint_ConeLimit *coneLimit;				// cone limit
	idAFConstraint_HingeSteering *steering;				// steering
	idAFConstraint_HingeFriction *fc;					// friction constraint

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// hinge joint friction
class idAFConstraint_HingeFriction : public idAFConstraint {

public:
							idAFConstraint_HingeFriction( void );
	void					Setup( idAFConstraint_Hinge *cc );
	bool					Add( anPhysics_AF *phys, float invTimeStep );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );

protected:
	idAFConstraint_Hinge *	hinge;						// hinge

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// constrains two bodies attached to each other with a hinge to get a specified relative orientation
class idAFConstraint_HingeSteering : public idAFConstraint {

public:
							idAFConstraint_HingeSteering( void );
	void					Setup( idAFConstraint_Hinge *cc );
	void					SetSteerAngle( const float degrees ) { steerAngle = degrees; }
	void					SetSteerSpeed( const float speed ) { steerSpeed = speed; }
	void					SetEpsilon( const float e ) { epsilon = e; }
	bool					Add( anPhysics_AF *phys, float invTimeStep );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );

	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	idAFConstraint_Hinge *	hinge;						// hinge
	float					steerAngle;					// desired steer angle in degrees
	float					steerSpeed;					// steer speed
	float					epsilon;					// lcp epsilon

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// slider, prismatic or translational constraint which allows 1 degree of freedom
// constrains body1 to lie on a line relative to body2, the orientation is also fixed relative to body2
class idAFConstraint_Slider : public idAFConstraint {

public:
							idAFConstraint_Slider( const anString &name, idAFBody *body1, idAFBody *body2 );
	void					SetAxis( const anVec3 &ax );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			GetCenter( anVec3 &center );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					axis;						// axis along which body1 slides in body2 space
	anVec3					offset;						// offset of body1 relative to body2
	anMat3					relAxis;					// rotation of body1 relative to body2

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// line constraint which allows 4 degrees of freedom
// constrains body1 to lie on a line relative to body2, does not constrain the orientation.
class idAFConstraint_Line : public idAFConstraint {

public:
							idAFConstraint_Line( const anString &name, idAFBody *body1, idAFBody *body2 );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );

protected:

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// plane constraint which allows 5 degrees of freedom
// constrains body1 to lie in a plane relative to body2, does not constrain the orientation.
class idAFConstraint_Plane : public idAFConstraint {

public:
							idAFConstraint_Plane( const anString &name, idAFBody *body1, idAFBody *body2 );
	void					SetPlane( const anVec3 &normal, const anVec3 &anchor );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					anchor1;					// anchor in body1 space
	anVec3					anchor2;					// anchor in body2 space
	anVec3					planeNormal;				// plane normal in body2 space

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// spring constraint which allows 6 or 5 degrees of freedom based on the spring limits
// constrains body1 relative to body2 with a spring
class idAFConstraint_Spring : public idAFConstraint {

public:
							idAFConstraint_Spring( const anString &name, idAFBody *body1, idAFBody *body2 );
	void					SetAnchor( const anVec3 &worldAnchor1, const anVec3 &worldAnchor2 );
	void					SetSpring( const float stretch, const float compress, const float damping, const float restLength );
	void					SetLimit( const float minLength, const float maxLength );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			GetCenter( anVec3 &center );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					anchor1;					// anchor in body1 space
	anVec3					anchor2;					// anchor in body2 space
	float					kstretch;					// spring constant when stretched
	float					kcompress;					// spring constant when compressed
	float					damping;					// spring damping
	float					restLength;					// rest length of spring
	float					minLength;					// minimum spring length
	float					maxLength;					// maximum spring length

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// constrains body1 to either be in contact with or move away from body2
class idAFConstraint_Contact : public idAFConstraint {

public:
							idAFConstraint_Contact( void );
							~idAFConstraint_Contact( void );
	void					Setup( idAFBody *b1, idAFBody *b2, contactInfo_t &c );
	const contactInfo_t &	GetContact( void ) const { return contact; }
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			GetCenter( anVec3 &center );

protected:
	contactInfo_t			contact;					// contact information
	idAFConstraint_ContactFriction *fc;					// contact friction

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// contact friction
class idAFConstraint_ContactFriction : public idAFConstraint {

public:
							idAFConstraint_ContactFriction( void );
	void					Setup( idAFConstraint_Contact *cc );
	bool					Add( anPhysics_AF *phys, float invTimeStep );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );

protected:
	idAFConstraint_Contact *cc;							// contact constraint

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// constrains an axis attached to body1 to be inside a cone relative to body2
class idAFConstraint_ConeLimit : public idAFConstraint {

public:
							idAFConstraint_ConeLimit( void );
	void					Setup( idAFBody *b1, idAFBody *b2, const anVec3 &coneAnchor, const anVec3 &coneAxis,
									const float coneAngle, const anVec3 &body1Axis );
	void					SetAnchor( const anVec3 &coneAnchor );
	void					SetBody1Axis( const anVec3 &body1Axis );
	void					SetEpsilon( const float e ) { epsilon = e; }
	bool					Add( anPhysics_AF *phys, float invTimeStep );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					coneAnchor;					// top of the cone in body2 space
	anVec3					coneAxis;					// cone axis in body2 space
	anVec3					body1Axis;					// axis in body1 space that should stay within the cone
	float					cosAngle;					// cos( coneAngle / 2 )
	float					sinHalfAngle;				// sin( coneAngle / 4 )
	float					cosHalfAngle;				// cos( coneAngle / 4 )
	float					epsilon;					// lcp epsilon

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};

// constrains an axis attached to body1 to be inside a pyramid relative to body2
class idAFConstraint_PyramidLimit : public idAFConstraint {

public:
							idAFConstraint_PyramidLimit( void );
	void					Setup( idAFBody *b1, idAFBody *b2, const anVec3 &pyramidAnchor,
									const anVec3 &pyramidAxis, const anVec3 &baseAxis,
									const float pyramidAngle1, const float pyramidAngle2, const anVec3 &body1Axis );
	void					SetAnchor( const anVec3 &pyramidAxis );
	void					SetBody1Axis( const anVec3 &body1Axis );
	void					SetEpsilon( const float e ) { epsilon = e; }
	bool					Add( anPhysics_AF *phys, float invTimeStep );
	virtual void			DebugDraw( void );
	virtual void			Translate( const anVec3 &translation );
	virtual void			Rotate( const anRotation &rotation );
	virtual void			Save( anSaveGame *saveFile ) const;
	virtual void			Restore( anRestoreGame *saveFile );

protected:
	anVec3					pyramidAnchor;				// top of the pyramid in body2 space
	anMat3					pyramidBasis;				// pyramid basis in body2 space with base[2] being the pyramid axis
	anVec3					body1Axis;					// axis in body1 space that should stay within the cone
	float					cosAngle[2];				// cos( pyramidAngle / 2 )
	float					sinHalfAngle[2];			// sin( pyramidAngle / 4 )
	float					cosHalfAngle[2];			// cos( pyramidAngle / 4 )
	float					epsilon;					// lcp epsilon

protected:
	virtual void			Evaluate( float invTimeStep );
	virtual void			ApplyFriction( float invTimeStep );
};


//===============================================================
//
//	idAFBody
//
//===============================================================

typedef struct AFBodyPState_s {
	anVec3					worldOrigin;				// position in world space
	anMat3					worldAxis;					// axis at worldOrigin
	anVec6					spatialVelocity;			// linear and rotational velocity of body
	anVec6					externalForce;				// external force and torque applied to body
} AFBodyPState_t;


class idAFBody {

	friend class anPhysics_AF;
	friend class idAFTree;

public:
							idAFBody( void );
							idAFBody( const anString &name, anClipModel *clipModel, float density );
							~idAFBody( void );

	void					Init( void );
	const anString &			GetName( void ) const { return name; }
	const anVec3 &			GetWorldOrigin( void ) const { return current->worldOrigin; }
	const anMat3 &			GetWorldAxis( void ) const { return current->worldAxis; }
	const anVec3 &			GetLinearVelocity( void ) const { return current->spatialVelocity.SubVec3(0); }
	const anVec3 &			GetAngularVelocity( void ) const { return current->spatialVelocity.SubVec3( 1 ); }
	anVec3					GetPointVelocity( const anVec3 &point ) const;
	const anVec3 &			GetCenterOfMass( void ) const { return centerOfMass; }
	void					SetClipModel( anClipModel *clipModel );
	anClipModel *			GetClipModel( void ) const { return clipModel; }
	void					SetClipMask( const int mask ) { clipMask = mask; fl.clipMaskSet = true; }
	int						GetClipMask( void ) const { return clipMask; }
	void					SetSelfCollision( const bool enable ) { fl.selfCollision = enable; }
	void					SetWorldOrigin( const anVec3 &origin ) { current->worldOrigin = origin; }
	void					SetWorldAxis( const anMat3 &axis ) { current->worldAxis = axis; }
	void					SetLinearVelocity( const anVec3 &linear ) const { current->spatialVelocity.SubVec3(0) = linear; }
	void					SetAngularVelocity( const anVec3 &angular ) const { current->spatialVelocity.SubVec3( 1 ) = angular; }
	void					SetFriction( float linear, float angular, float contact );
	float					GetContactFriction( void ) const { return contactFriction; }
	void					SetBouncyness( float bounce );
	float					GetBouncyness( void ) const { return bouncyness; }
	void					SetDensity( float density, const anMat3 &inertiaScale = mat3_identity );
	float					GetInverseMass( void ) const { return invMass; }
	anMat3					GetInverseWorldInertia( void ) const { return current->worldAxis.Transpose() * inverseInertiaTensor * current->worldAxis; }

	void					SetFrictionDirection( const anVec3 &dir );
	bool					GetFrictionDirection( anVec3 &dir ) const;

	void					SetContactMotorDirection( const anVec3 &dir );
	bool					GetContactMotorDirection( anVec3 &dir ) const;
	void					SetContactMotorVelocity( float vel ) { contactMotorVelocity = vel; }
	float					GetContactMotorVelocity( void ) const { return contactMotorVelocity; }
	void					SetContactMotorForce( float force ) { contactMotorForce = force; }
	float					GetContactMotorForce( void ) const { return contactMotorForce; }

	void					AddForce( const anVec3 &point, const anVec3 &force );
	void					InverseWorldSpatialInertiaMultiply( anVecX &dst, const float *v ) const;
	anVec6 &				GetResponseForce( int index ) { return reinterpret_cast<anVec6 &>(response[ index * 8 ]); }

	void					Save( anSaveGame *saveFile );
	void					Restore( anRestoreGame *saveFile );

private:
							// properties
	anString				name;						// name of body
	idAFBody *				parent;						// parent of this body
	anList<idAFBody *>		children;					// children of this body
	anClipModel *			clipModel;					// model used for collision detection
	idAFConstraint *		primaryConstraint;			// primary constraint (this->constraint->body1 = this)
	anList<idAFConstraint *>constraints;				// all constraints attached to this body
	idAFTree *				tree;						// tree structure this body is part of
	float					linearFriction;				// translational friction
	float					angularFriction;			// rotational friction
	float					contactFriction;			// friction with contact surfaces
	float					bouncyness;					// bounce
	int						clipMask;					// contents this body collides with
	anVec3					frictionDir;				// specifies a single direction of friction in body space
	anVec3					contactMotorDir;			// contact motor direction
	float					contactMotorVelocity;		// contact motor velocity
	float					contactMotorForce;			// maximum force applied to reach the motor velocity

							// derived properties
	float					mass;						// mass of body
	float					invMass;					// inverse mass
	anVec3					centerOfMass;				// center of mass of body
	anMat3					inertiaTensor;				// inertia tensor
	anMat3					inverseInertiaTensor;		// inverse inertia tensor

							// physics state
	AFBodyPState_t			state[2];
	AFBodyPState_t *		current;					// current physics state
	AFBodyPState_t *		next;						// next physics state
	AFBodyPState_t			saved;						// saved physics state
	anVec3					atRestOrigin;				// origin at rest
	anMat3					atRestAxis;					// axis at rest

							// simulation variables used during calculations
	anMatX					inverseWorldSpatialInertia;	// inverse spatial inertia in world space
	anMatX					I, invI;					// transformed inertia
	anMatX					J;							// transformed constraint matrix
	anVecX					s;							// temp solution
	anVecX					totalForce;					// total force acting on body
	anVecX					auxForce;					// force from auxiliary constraints
	anVecX					acceleration;				// acceleration
	float *					response;					// forces on body in response to auxiliary constraint forces
	int *					responseIndex;				// index to response forces
	int						numResponses;				// number of response forces
	int						maxAuxiliaryIndex;			// largest index of an auxiliary constraint constraining this body
	int						maxSubTreeAuxiliaryIndex;	// largest index of an auxiliary constraint constraining this body or one of it's children

	struct bodyFlags_s {
		bool				clipMaskSet			: 1;	// true if this body has a clip mask set
		bool				selfCollision		: 1;	// true if this body can collide with other bodies of this AF
		bool				spatialInertiaSparse: 1;	// true if the spatial inertia matrix is sparse
		bool				useFrictionDir		: 1;	// true if a single friction direction should be used
		bool				useContactMotorDir	: 1;	// true if a contact motor should be used
		bool				isZero				: 1;	// true if 's' is zero during calculations
	} fl;
};


//===============================================================
//
//	idAFTree
//
//===============================================================

class idAFTree {
	friend class anPhysics_AF;

public:
	void					Factor( void ) const;
	void					Solve( int auxiliaryIndex = 0 ) const;
	void					Response( const idAFConstraint *constraint, int row, int auxiliaryIndex ) const;
	void					CalculateForces( float timeStep ) const;
	void					SetMaxSubTreeAuxiliaryIndex( void );
	void					SortBodies( void );
	void					SortBodies_r( anList<idAFBody*>&sortedList, idAFBody *body );
	void					DebugDraw( const anVec4 &color ) const;

private:
	anList<idAFBody *>		sortedBodies;
};


//===============================================================
//
//	anPhysics_AF
//
//===============================================================

typedef struct AFPState_s {
	int						atRest;						// >= 0 if articulated figure is at rest
	float					noMoveTime;					// time the articulated figure is hardly moving
	float					activateTime;				// time since last activation
	float					lastTimeStep;				// last time step
	anVec6					pushVelocity;				// velocity with which the af is pushed
} AFPState_t;

typedef struct AFCollision_s {
	trace_t					trace;
	idAFBody *				body;
} AFCollision_t;

class anPhysics_AF : public anPhysics_Base {

public:
	CLASS_PROTOTYPE( anPhysics_AF );

							anPhysics_AF( void );
							~anPhysics_AF( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

							// initialisation
	int						AddBody( idAFBody *body );	// returns body id
	void					AddConstraint( idAFConstraint *constraint );
	void					AddFrameConstraint( idAFConstraint *constraint );
							// force a body to have a certain id
	void					ForceBodyId( idAFBody *body, int newId );
							// get body or constraint id
	int						GetBodyId( idAFBody *body ) const;
	int						GetBodyId( const char *bodyName ) const;
	int						GetConstraintId( idAFConstraint *constraint ) const;
	int						GetConstraintId( const char *constraintName ) const;
							// number of bodies and constraints
	int						GetNumBodies( void ) const;
	int						GetNumConstraints( void ) const;
							// retrieve body or constraint
	idAFBody *				GetBody( const char *bodyName ) const;
	idAFBody *				GetBody( const int id ) const;
	idAFBody *				GetMasterBody( void ) const { return masterBody; }
	idAFConstraint *		GetConstraint( const char *constraintName ) const;
	idAFConstraint *		GetConstraint( const int id ) const;
							// delete body or constraint
	void					DeleteBody( const char *bodyName );
	void					DeleteBody( const int id );
	void					DeleteConstraint( const char *constraintName );
	void					DeleteConstraint( const int id );

							// get all the contact constraints acting on the body
	int						GetBodyContactConstraints( const int id, idAFConstraint_Contact *contacts[], int maxContacts ) const;
							// set the default friction for bodies
	void					SetDefaultFriction( float linear, float angular, float contact );
							// suspend settings
	void					SetSuspendSpeed( const anVec2 &velocity, const anVec2 &acceleration );
							// set the time and tolerances used to determine if the simulation can be suspended when the figure hardly moves for a while
	void					SetSuspendTolerance( const float noMoveTime, const float translationTolerance, const float rotationTolerance );
							// set minimum and maximum simulation time in seconds
	void					SetSuspendTime( const float minTime, const float maxTime );
							// set the time scale value
	void					SetTimeScale( const float ts ) { timeScale = ts; }
							// set time scale ramp
	void					SetTimeScaleRamp( const float start, const float end );
							// set the joint friction scale
	void					SetJointFrictionScale( const float scale ) { jointFrictionScale = scale; }
							// set joint friction dent
	void					SetJointFrictionDent( const float dent, const float start, const float end );
							// get the current joint friction scale
	float					GetJointFrictionScale( void ) const;
							// set the contact friction scale
	void					SetContactFrictionScale( const float scale ) { contactFrictionScale = scale; }
							// set contact friction dent
	void					SetContactFrictionDent( const float dent, const float start, const float end );
							// get the current contact friction scale
	float					GetContactFrictionScale( void ) const;
							// enable or disable collision detection
	void					SetCollision( const bool enable ) { enableCollision = enable; }
							// enable or disable self collision
	void					SetSelfCollision( const bool enable ) { selfCollision = enable; }
							// enable or disable coming to a dead stop
	void					SetComeToRest( bool enable ) { comeToRest = enable; }
							// call when structure of articulated figure changes
	void					SetChanged( void ) { changedAF = true; }

							// enable or disable fast evaluation
	void					SetFastEval( const bool enable ) { fastEval = enable; }

							// enable/disable activation by impact
	void					EnableImpact( void );
	void					DisableImpact( void );
							// lock of unlock the world constraints
	void					LockWorldConstraints( const bool lock ) { worldConstraintsLocked = lock; }
							// set force pushable
	void					SetForcePushable( const bool enable ) { forcePushable = enable; }
							// update the clip model positions
	void					UpdateClipModels( void );

public:	// common physics interface
	void					SetClipModel( anClipModel *model, float density, int id = 0, bool freeOld = true );
	anClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	//MCG: added SetImpulseThreshold
	void					SetImpulseThreshold( float newIT ) { impulseThreshold = newIT; };

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const anBounds &		GetBounds( int id = -1 ) const;
	const anBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse );
	void					AddForce( const int id, const anVec3 &point, const anVec3 &force );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	void					Activate( void );
	void					PutToRest( void );
	bool					IsPushable( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const anVec3 &newOrigin, int id = -1 );
	void					SetAxis( const anMat3 &newAxis, int id = -1 );

	void					Translate( const anVec3 &translation, int id = -1 );
	void					Rotate( const anRotation &rotation, int id = -1 );

	const anVec3 &			GetOrigin( int id = 0 ) const;
	const anMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const anVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const anVec3 &newAngularVelocity, int id = 0 );

	const anVec3 &			GetLinearVelocity( int id = 0 ) const;
	const anVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const;
	void					ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const;
	int						ClipContents( const anClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	bool					EvaluateContacts( void );

	void					SetPushed( int deltaTime );
	const anVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const anVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( anEntity *master, const bool orientated = true );

	void					WriteToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const anBitMsgDelta &msg );

private:
							// articulated figure
	anList<idAFTree *>		trees;							// tree structures
	anList<idAFBody *>		bodies;							// all bodies
	anList<idAFConstraint *>constraints;					// all frame independent constraints
	anList<idAFConstraint *>primaryConstraints;				// list with primary constraints
	anList<idAFConstraint *>auxiliaryConstraints;			// list with auxiliary constraints
	anList<idAFConstraint *>frameConstraints;				// constraints that only live one frame
	anList<idAFConstraint_Contact *>contactConstraints;		// contact constraints
	anList<int>				contactBodies;					// body id for each contact
	anList<AFCollision_t>	collisions;						// collisions
	bool					changedAF;						// true when the articulated figure just changed

							// properties
	float					linearFriction;					// default translational friction
	float					angularFriction;				// default rotational friction
	float					contactFriction;				// default friction with contact surfaces
	float					bouncyness;						// default bouncyness
	float					totalMass;						// total mass of articulated figure
	float					forceTotalMass;					// force this total mass

	anVec2					suspendVelocity;				// simulation may not be suspended if a body has more velocity
	anVec2					suspendAcceleration;			// simulation may not be suspended if a body has more acceleration
	float					noMoveTime;						// suspend simulation if hardly any movement for this many seconds
	float					noMoveTranslation;				// maximum translation considered no movement
	float					noMoveRotation;					// maximum rotation considered no movement
	float					minMoveTime;					// if > 0 the simulation is never suspended before running this many seconds
	float					maxMoveTime;					// if > 0 the simulation is always suspeded after running this many seconds
	float					impulseThreshold;				// threshold below which impulses are ignored to avoid continuous activation

	float					timeScale;						// the time is scaled with this value for slow motion effects
	float					timeScaleRampStart;				// start of time scale change
	float					timeScaleRampEnd;				// end of time scale change

	float					jointFrictionScale;				// joint friction scale
	float					jointFrictionDent;				// joint friction dives from 1 to this value and goes up again
	float					jointFrictionDentStart;			// start time of joint friction dent
	float					jointFrictionDentEnd;			// end time of joint friction dent
	float					jointFrictionDentScale;			// dent scale

	float					contactFrictionScale;			// contact friction scale
	float					contactFrictionDent;			// contact friction dives from 1 to this value and goes up again
	float					contactFrictionDentStart;		// start time of contact friction dent
	float					contactFrictionDentEnd;			// end time of contact friction dent
	float					contactFrictionDentScale;		// dent scale

	bool					enableCollision;				// if true collision detection is enabled
	bool					selfCollision;					// if true the self collision is allowed
	bool					comeToRest;						// if true the figure can come to rest
	bool					linearTime;						// if true use the linear time algorithm
	bool					noImpact;						// if true do not activate when another object collides
	bool					worldConstraintsLocked;			// if true world constraints cannot be moved
	bool					forcePushable;					// if true can be pushed even when bound to a master

	bool					fastEval;						// if true the fast eval is on

							// physics state
	AFPState_t				current;
	AFPState_t				saved;

	idAFBody *				masterBody;						// master body
	idLCP *					lcp;							// linear complementarity problem solver

private:
	void					BuildTrees( void );
	bool					IsClosedLoop( const idAFBody *body1, const idAFBody *body2 ) const;
	void					PrimaryFactor( void );
	void					EvaluateBodies( float timeStep );
	void					EvaluateConstraints( float timeStep );
	void					AddFrameConstraints( void );
	void					RemoveFrameConstraints( void );
	void					ApplyFriction( float timeStep, float endTimeMSec );
	void					PrimaryForces( float timeStep  );
	void					AuxiliaryForces( float timeStep );
	void					VerifyContactConstraints( void );
	void					SetupContactConstraints( void );
	void					ApplyContactForces( void );
	void					Evolve( float timeStep );
	anEntity *				SetupCollisionForBody( idAFBody *body ) const;
	bool					CollisionImpulse( float timeStep, idAFBody *body, trace_t &collision );
	bool					ApplyCollisions( float timeStep );
	void					CheckForCollisions( float timeStep );
	void					ClearExternalForce( void );
	void					AddGravity( void );
	void					SwapStates( void );
	bool					TestIfAtRest( float timeStep );
	void					Rest( void );
	void					AddPushVelocity( const anVec6 &pushVelocity );
	void					DebugDraw( void );
};

#endif // !__PHYSICS_AF_H__