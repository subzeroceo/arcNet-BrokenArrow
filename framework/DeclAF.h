#ifndef __DECLAF_H__
#define __DECLAF_H__

/*
===============================================================================

	Articulated Figure

===============================================================================
*/

class arcDeclAF;

typedef enum {
	DECLAF_CONSTRAINT_INVALID,
	DECLAF_CONSTRAINT_FIXED,
	DECLAF_CONSTRAINT_BALLANDSOCKETJOINT,
	DECLAF_CONSTRAINT_UNIVERSALJOINT,
	DECLAF_CONSTRAINT_HINGE,
	DECLAF_CONSTRAINT_SLIDER,
	DECLAF_CONSTRAINT_SPRING
} declAFConstraintType_t;

typedef enum {
	DECLAF_JOINTMOD_AXIS,
	DECLAF_JOINTMOD_ORIGIN,
	DECLAF_JOINTMOD_BOTH
} declAFJointMod_t;

typedef bool (*getJointTransform_t)( void *model, const arcJointMat *frame, const char *jointName, arcVec3 &origin, arcMat3 &axis );

class arcAFVector {
public:
	enum {
		VEC_COORDS = 0,
		VEC_JOINT,
		VEC_BONECENTER,
		VEC_BONEDIR
	}						type;
	arcNetString					joint1;
	arcNetString					joint2;

public:
							arcAFVector();

	bool					Parse( arcLexer &src );
	bool					Finish( const char *fileName, const getJointTransform_t GetJointTransform, const arcJointMat *frame, void *model ) const;
	bool					Write( arcNetFile *f ) const;
	const char *			ToString( arcNetString &str, const int precision = 8 );
	const arcVec3 &			ToVec3() const { return vec; }
	arcVec3 &				ToVec3() { return vec; }

private:
	mutable arcVec3			vec;
	bool					negate;
};

class arcDeclAF_Body {
public:
	arcNetString					name;
	arcNetString					jointName;
	declAFJointMod_t		jointMod;
	int						modelType;
	arcAFVector				v1, v2;
	int						numSides;
	float					width;
	float					density;
	arcAFVector				origin;
	arcAngles				angles;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	arcMat3					inertiaScale;
	float					linearFriction;
	float					angularFriction;
	float					contactFriction;
	arcNetString					containedJoints;
	arcAFVector				frictionDirection;
	arcAFVector				contactMotorDirection;
public:
	void					SetDefault( const arcDeclAF *file );
};

class arcDeclAF_Constraint {
public:
	arcNetString					name;
	arcNetString					body1;
	arcNetString					body2;
	declAFConstraintType_t	type;
	float					friction;
	float					stretch;
	float					compress;
	float					damping;
	float					restLength;
	float					minLength;
	float					maxLength;
	arcAFVector				anchor;
	arcAFVector				anchor2;
	arcAFVector				shaft[2];
	arcAFVector				axis;
	enum {
		LIMIT_NONE = -1,
		LIMIT_CONE,
		LIMIT_PYRAMID
	}						limit;
	arcAFVector				limitAxis;
	float					limitAngles[3];

public:
	void					SetDefault( const arcDeclAF *file );
};

class arcDeclAF : public arcDecleration {
	friend class arcAFFileManager;
public:
							arcDeclAF();
	virtual					~arcDeclAF();

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	virtual void			Finish( const getJointTransform_t GetJointTransform, const arcJointMat *frame, void *model ) const;

	bool					Save();

	void					NewBody( const char *name );
	void					RenameBody( const char *oldName, const char *newName );
	void					DeleteBody( const char *name );

	void					NewConstraint( const char *name );
	void					RenameConstraint( const char *oldName, const char *newName );
	void					DeleteConstraint( const char *name );

	static int				ContentsFromString( const char *str );
	static const char *		ContentsToString( const int contents, arcNetString &str );

	static declAFJointMod_t	JointModFromString( const char *str );
	static const char *		JointModToString( declAFJointMod_t jointMod );

public:
	bool					modified;
	arcNetString					model;
	arcNetString					skin;
	float					defaultLinearFriction;
	float					defaultAngularFriction;
	float					defaultContactFriction;
	float					defaultConstraintFriction;
	float					totalMass;
	arcVec2					suspendVelocity;
	arcVec2					suspendAcceleration;
	float					noMoveTime;
	float					noMoveTranslation;
	float					noMoveRotation;
	float					minMoveTime;
	float					maxMoveTime;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	arcNetList<arcDeclAF_Body *, TAG_IDLIB_LIST_PHYSICS>			bodies;
	arcNetList<arcDeclAF_Constraint *, TAG_IDLIB_LIST_PHYSICS>	constraints;

private:
	bool					ParseContents( arcLexer &src, int &c ) const;
	bool					ParseBody( arcLexer &src );
	bool					ParseFixed( arcLexer &src );
	bool					ParseBallAndSocketJoint( arcLexer &src );
	bool					ParseUniversalJoint( arcLexer &src );
	bool					ParseHinge( arcLexer &src );
	bool					ParseSlider( arcLexer &src );
	bool					ParseSpring( arcLexer &src );
	bool					ParseSettings( arcLexer &src );

	bool					WriteBody( arcNetFile *f, const arcDeclAF_Body &body ) const;
	bool					WriteFixed( arcNetFile *f, const arcDeclAF_Constraint &c ) const;
	bool					WriteBallAndSocketJoint( arcNetFile *f, const arcDeclAF_Constraint &c ) const;
	bool					WriteUniversalJoint( arcNetFile *f, const arcDeclAF_Constraint &c ) const;
	bool					WriteHinge( arcNetFile *f, const arcDeclAF_Constraint &c ) const;
	bool					WriteSlider( arcNetFile *f, const arcDeclAF_Constraint &c ) const;
	bool					WriteSpring( arcNetFile *f, const arcDeclAF_Constraint &c ) const;
	bool					WriteConstraint( arcNetFile *f, const arcDeclAF_Constraint &c ) const;
	bool					WriteSettings( arcNetFile *f ) const;

	bool					RebuildTextSource();
};

#endif /* !__DECLAF_H__ */
