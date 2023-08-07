#ifndef __DECLAF_H__
#define __DECLAF_H__

/*
===============================================================================

	Articulated Figure

===============================================================================
*/

class anDeclAF;

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

typedef bool (*getJointTransform_t)( void *model, const anJointMat *frame, const char *jointName, anVec3 &origin, anMat3 &axis );

class arcAFVector {
public:
	enum {
		VEC_COORDS = 0,
		VEC_JOINT,
		VEC_BONECENTER,
		VEC_BONEDIR
	}						type;
	anStr					joint1;
	anStr					joint2;

public:
							arcAFVector();

	bool					Parse( anLexer &src );
	bool					Finish( const char *fileName, const getJointTransform_t GetJointTransform, const anJointMat *frame, void *model ) const;
	bool					Write( anFile *f ) const;
	const char *			ToString( anStr &str, const int precision = 8 );
	const anVec3 &			ToVec3() const { return vec; }
	anVec3 &				ToVec3() { return vec; }

private:
	mutable anVec3			vec;
	bool					negate;
};

class anDeclAF_Body {
public:
	anStr					name;
	anStr					jointName;
	declAFJointMod_t		jointMod;
	int						modelType;
	arcAFVector				v1, v2;
	int						numSides;
	float					width;
	float					density;
	arcAFVector				origin;
	anAngles				angles;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	anMat3					inertiaScale;
	float					linearFriction;
	float					angularFriction;
	float					contactFriction;
	anStr					containedJoints;
	arcAFVector				frictionDirection;
	arcAFVector				contactMotorDirection;
public:
	void					SetDefault( const anDeclAF *file );
};

class anDeclAF_Constraint {
public:
	anStr					name;
	anStr					body1;
	anStr					body2;
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
	void					SetDefault( const anDeclAF *file );
};

class anDeclAF : public anDecl {
	friend class arcAFFileManager;
public:
							anDeclAF();
	virtual					~anDeclAF();

	virtual size_t			Size() const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	virtual void			Finish( const getJointTransform_t GetJointTransform, const anJointMat *frame, void *model ) const;

	bool					Save();

	void					NewBody( const char *name );
	void					RenameBody( const char *oldName, const char *newName );
	void					DeleteBody( const char *name );

	void					NewConstraint( const char *name );
	void					RenameConstraint( const char *oldName, const char *newName );
	void					DeleteConstraint( const char *name );

	static int				ContentsFromString( const char *str );
	static const char *		ContentsToString( const int contents, anStr &str );

	static declAFJointMod_t	JointModFromString( const char *str );
	static const char *		JointModToString( declAFJointMod_t jointMod );

public:
	bool					modified;
	anStr					model;
	anStr					skin;
	float					defaultLinearFriction;
	float					defaultAngularFriction;
	float					defaultContactFriction;
	float					defaultConstraintFriction;
	float					totalMass;
	anVec2					suspendVelocity;
	anVec2					suspendAcceleration;
	float					noMoveTime;
	float					noMoveTranslation;
	float					noMoveRotation;
	float					minMoveTime;
	float					maxMoveTime;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	anList<anDeclAF_Body *, TAG_IDLIB_LIST_PHYSICS>			bodies;
	anList<anDeclAF_Constraint *, TAG_IDLIB_LIST_PHYSICS>	constraints;

private:
	bool					ParseContents( anLexer &src, int &c ) const;
	bool					ParseBody( anLexer &src );
	bool					ParseFixed( anLexer &src );
	bool					ParseBallAndSocketJoint( anLexer &src );
	bool					ParseUniversalJoint( anLexer &src );
	bool					ParseHinge( anLexer &src );
	bool					ParseSlider( anLexer &src );
	bool					ParseSpring( anLexer &src );
	bool					ParseSettings( anLexer &src );

	bool					WriteBody( anFile *f, const anDeclAF_Body &body ) const;
	bool					WriteFixed( anFile *f, const anDeclAF_Constraint &c ) const;
	bool					WriteBallAndSocketJoint( anFile *f, const anDeclAF_Constraint &c ) const;
	bool					WriteUniversalJoint( anFile *f, const anDeclAF_Constraint &c ) const;
	bool					WriteHinge( anFile *f, const anDeclAF_Constraint &c ) const;
	bool					WriteSlider( anFile *f, const anDeclAF_Constraint &c ) const;
	bool					WriteSpring( anFile *f, const anDeclAF_Constraint &c ) const;
	bool					WriteConstraint( anFile *f, const anDeclAF_Constraint &c ) const;
	bool					WriteSettings( anFile *f ) const;

	bool					RebuildTextSource();
};

#endif /* !__DECLAF_H__ */
