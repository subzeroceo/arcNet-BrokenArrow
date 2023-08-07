#include "../idlib/Lib.h"
#pragma hdrstop
#include "tr_local.h"

/*
Remember C++ operators
Understanding the difference between checking if all conditions are true (&&) versus checking
if at least one condition is true (||) is crucial in programming.

Using && (logical AND) ensures that all conditions must be true for the overall expression to be true.
If any of the conditions are false, the whole expression evaluates to false.

On the other hand, using || (logical OR) checks if at least one of the conditions is true.
If any of the conditions are true, the overall expression evaluates to true.


Any errors during parsing just set MF_DEFAULTED and return, rather than throwing
a hard error. This will cause the material to fall back to default material,
but otherwise let things continue.

Each material may have a set of calculations that must be evaluated before
drawing with it.

Every expression that a material uses can be evaluated at one time, which
will allow for perfect common subexpression removal when I get around to
writing it.

Without this, scrolling an entire surface could result in evaluating the
same texture matrix calculations a half dozen times.

  Open question: should I allow arbitrary per-vertex color, texCoord, and vertex
  calculations to be specified in the material code?

  Every stage will definately have a valid image pointer.

  We might want the ability to change the sort value based on conditionals,
  but it could be a hassle to implement,

*/

// keep all of these on the stack, when they are static it makes material parsing non-reentrant
typedef struct mtrParsingData_s {
	bool			registerIsTemporary[MAX_EXP_REGS];
	float			shaderRegisters[MAX_EXP_REGS];
	expOp_t			shaderOps[MAX_EXP_OPS];
	materialStage_t	parseStages[MAX_SHADER_STAGES];

	bool			registersAreConstant;
	bool			forceOverlays;
} mtrParsingData_t;

/*
=============
anMaterial::CommonInit
=============
*/
void anMaterial::CommonInit() {
	desc = "<none>";
	renderBump = "";
	contentFlags = CONTENTS_SOLID;
	surfaceFlags = SURFTYPE_NONE;
	materialFlags = 0;
	sort = SS_BAD;
	coverage = MC_BAD;
	cullType = CT_FRONT_SIDED;
	deform = DFRM_NONE;
	numOps = 0;
	ops = nullptr;
	numRegisters = 0;
	expressionRegisters = nullptr;
	constantRegisters = nullptr;
	numStages = 0;
	numAmbientStages = 0;
	stages = nullptr;
	editorImage = nullptr;
	lightFalloffImage = nullptr;
	shouldCreateBackSides = false;
	entityGui = 0;
	fogLight = false;
	blendLight = false;
	ambientLight = false;
	noFog = false;
	hasSubview = false;
	allowOverlays = true;
	unsmoothedTangents = false;
	gui = nullptr;
	memset( deformRegisters, 0, sizeof( deformRegisters ) );
	editorAlpha = 1.0;
	spectrum = 0;
	polygonOffset = 0;
	suppressInSubview = false;
	refCount = 0;
	portalSky = false;

	decalInfo.stayTime = 10000;
	decalInfo.fadeTime = 4000;
	decalInfo.start[0] = 1;
	decalInfo.start[1] = 1;
	decalInfo.start[2] = 1;
	decalInfo.start[3] = 1;
	decalInfo.end[0] = 0;
	decalInfo.end[1] = 0;
	decalInfo.end[2] = 0;
	decalInfo.end[3] = 0;
}

/*
=============
anMaterial::anMaterial
=============
*/
anMaterial::anMaterial() {
	CommonInit();

	// we put this here instead of in CommonInit, because
	// we don't want it cleared when a material is purged
	surfaceArea = 0;
}

/*
=============
anMaterial::~anMaterial
=============
*/
anMaterial::~anMaterial() {
}

/*
===============
anMaterial::FreeData
===============
*/
void anMaterial::FreeData() {
	int i;

	if ( stages ) {
		// delete any idCinematic textures
		for ( i = 0; i < numStages; i++ ) {
			if ( stages[i].texture.cinematic != nullptr ) {
				delete stages[i].texture.cinematic;
				stages[i].texture.cinematic = nullptr;
			}
			if ( stages[i].newStage != nullptr ) {
				Mem_Free( stages[i].newStage );
				stages[i].newStage = nullptr;
			}
		}
		R_StaticFree( stages );
		stages = nullptr;
	}
	if ( expressionRegisters != nullptr ) {
		R_StaticFree( expressionRegisters );
		expressionRegisters = nullptr;
	}
	if ( constantRegisters != nullptr ) {
		R_StaticFree( constantRegisters );
		constantRegisters = nullptr;
	}
	if ( ops != nullptr ) {
		R_StaticFree( ops );
		ops = nullptr;
	}
}

/*
==============
anMaterial::GetEditorImage
==============
*/
anImage *anMaterial::GetEditorImage( void ) const {
	if ( editorImage ) {
		return editorImage;
	}

	// if we don't have an editorImageName, use the first stage image
	if ( !editorImageName.Length() ) {
		// _D3XP :: First check for a diffuse image, then use the first
		if ( numStages && stages ) {
			int i;
			for ( i = 0; i < numStages; i++ ) {
				if ( stages[i].lighting == SL_DIFFUSE ) {
					editorImage = stages[i].texture.image;
					break;
				}
			}
			if ( !editorImage ) {
				editorImage = stages[0].texture.image;
			}
		} else {
			editorImage = globalImages->defaultImage;
		}
	} else {
		// look for an explicit one
		editorImage = globalImages->ImageFromFile( editorImageName, TF_DEFAULT, true, TR_REPEAT, TD_DEFAULT );
	}

	if ( !editorImage ) {
		editorImage = globalImages->defaultImage;
	}

	return editorImage;
}


// info parms
typedef struct {
	char	*name;
	int		clearSolid, surfaceFlags, contents;
} infoParm_t;

static infoParm_t	infoParms[] = {
	// game relevant attributes
	{"solid",		0,	0,	CONTENTS_SOLID },		// may need to override a clearSolid
	{"water",		1,	0,	CONTENTS_WATER },		// used for water
	{"playerclip",	0,	0,	CONTENTS_PLAYERCLIP },	// solid to players
	{"monsterclip",	0,	0,	CONTENTS_MONSTERCLIP },	// solid to monsters
	{"moveableclip",0,	0,	CONTENTS_MOVEABLECLIP },// solid to moveable entities
	{"ikclip",		0,	0,	CONTENTS_IKCLIP },		// solid to IK
	{"blood",		0,	0,	CONTENTS_BLOOD },		// used to detect blood decals
	{"trigger",		0,	0,	CONTENTS_TRIGGER_SEAS },		// used for triggers
	{"aassolid",	0,	0,	CONTENTS_SOLID_SEAS },	// solid for AAS
	{"aasobstacle",	0,	0,	CONTENTS_OBSTACLE_SEAS },// used to compile an obstacle into AAS that can be enabled/disabled
	//{"flashlight_trigger", 0, 0, CONTENTS_LIGHT_TRIGGER }, // used for triggers that are activated by the flashlight
	{"nonsolid",	1,	0,	0 			},			// clears the solid flag
	{"nullNormal",	0,	SURF_nullptrNORMAL,0 },		// renderbump will draw as 0x80 0x80 0x80

	// utility relevant attributes
	{"areaportal",	1,	0,	CONTENTS_AREAPORTAL },	// divides areas
	{"qer_nocarve",	1,	0,	CONTENTS_NOCSG},		// don't cut brushes in editor

	{"discrete",	1,	SURF_DISCRETE,	0 },		// surfaces should not be automatically merged together or
													// clipped to the world,
													// because they represent discrete objects like gui shaders
													// mirrors, or autosprites
	{"noFragment",	0,	SURF_NOFRAGMENT, 0 },

	{"slick",		0,	SURF_SLICK,		0 },
	{"collision",	0,	SURF_COLLISION,	0 },
	{"noimpact",	0,	SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
	{"nodamage",	0,	SURF_NODAMAGE,	0 },		// no falling damage when hitting
	{"ladder",		0,	SURF_LADDER,	0 },		// climbable
	{"nosteps",		0,	SURF_NOSTEPS,	0 },		// no footsteps

	// material types for particle, sound, footstep feedback
	{"metal",		0,  SURFTYPE_METAL,		0 },	// metal
	{"stone",		0,  SURFTYPE_STONE,		0 },	// stone
	{"flesh",		0,  SURFTYPE_FLESH,		0 },	// flesh
	{"wood",		0,  SURFTYPE_WOOD,		0 },	// wood
	{"cardboard",	0,	SURFTYPE_CARDBOARD,	0 },	// cardboard
	{"liquid",		0,	SURFTYPE_LIQUID,	0 },	// liquid
	{"glass",		0,	SURFTYPE_GLASS,		0 },	// glass
	{"plastic",		0,	SURFTYPE_PLASTIC,	0 },	// plastic
	{"ricochet",	0,	SURFTYPE_RICOCHET,	0 },	// behaves like metal but causes a ricochet sound

	// unassigned surface types
	{"surftype10",	0,	SURFTYPE_10,	0 },
	{"surftype11",	0,	SURFTYPE_11,	0 },
	{"surftype12",	0,	SURFTYPE_12,	0 },
	{"surftype13",	0,	SURFTYPE_13,	0 },
	{"surftype14",	0,	SURFTYPE_14,	0 },
	{"surftype15",	0,	SURFTYPE_15,	0 },
};

static const int numInfoParms = sizeof( infoParms ) / sizeof ( infoParms[0] );

/*
===============
anMaterial::CheckSurfaceParm

See if the current token matches one of the surface parm bit flags
===============
*/
bool anMaterial::CheckSurfaceParm( anToken *token ) {
	for ( int i = 0; i < numInfoParms; i++ ) {
		if ( !token->Icmp( infoParms[i].name ) ) {
			if ( infoParms[i].surfaceFlags & SURF_TYPE_MASK ) {
				// ensure we only have one surface type set
				surfaceFlags &= ~SURF_TYPE_MASK;
			}
			surfaceFlags |= infoParms[i].surfaceFlags;
			contentFlags |= infoParms[i].contents;
			if ( infoParms[i].clearSolid ) {
				contentFlags &= ~CONTENTS_SOLID;
			}
			return true;
		}
	}
	return false;
}

/*
===============
anMaterial::MatchToken

Sets defaultShader and returns false if the next token doesn't match
===============
*/
bool anMaterial::MatchToken( anLexer &src, const char *match ) {
	if ( !src.ExpectTokenString( match ) ) {
		SetMaterialFlag( MF_DEFAULTED );
		return false;
	}
	return true;
}

/*
=================
anMaterial::ParseSort
=================
*/
void anMaterial::ParseSort( anLexer &src ) {
	anToken token;

	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Warning( "missing sort parameter" );
		SetMaterialFlag( MF_DEFAULTED );
		return;
	}

	if ( !token.Icmp( "subView" ) ) {
		sort = SS_SUBVIEW;
	} else if ( !token.Icmp( "opaque" ) ) {
		sort = SS_OPAQUE;
	} else if ( !token.Icmp( "decal" ) ) {
		sort = SS_DECAL;
	} else if ( !token.Icmp( "far" ) ) {
		sort = SS_FAR;
	} else if ( !token.Icmp( "medium" ) ) {
		sort = SS_MEDIUM;
	} else if ( !token.Icmp( "close" ) ) {
		sort = SS_CLOSE;
	} else if ( !token.Icmp( "almostNearest" ) ) {
		sort = SS_ALMOST_NEAREST;
	} else if ( !token.Icmp( "nearest" ) ) {
		sort = SS_NEAREST;
	} else if ( !token.Icmp( "postProcess" ) ) {
		sort = SS_POST_PROCESS;
	} else if ( !token.Icmp( "portalSky" ) ) {
		sort = SS_PORTAL_SKY;
	} else {
		sort = atof( token );
	}
}

/*
=================
anMaterial::ParseStereoEye
=================
*/
void anMaterial::ParseStereoEye( anLexer &src ) {
	anToken token;

	if ( !src.ReadTokenOnLine( &token ) ) {
		src.Warning( "missing eye parameter" );
		SetMaterialFlag( MF_DEFAULTED );
		return;
	}

	if ( !token.Icmp( "left" ) ) {
		stereoEye = -1;
	} else if ( !token.Icmp( "right" ) ) {
		stereoEye = 1;
	} else {
		stereoEye = 0;
	}
}

/*
=================
anMaterial::ParseDecalInfo
=================
*/
void anMaterial::ParseDecalInfo( anLexer &src ) {
	anToken token;

	decalInfo.stayTime = src.ParseFloat() * 1000;
	decalInfo.fadeTime = src.ParseFloat() * 1000;
	float start[4], end[4];
	src.Parse1DMatrix( 4, start );
	src.Parse1DMatrix( 4, end );
	for ( int i = 0; i < 4; i++ ) {
		decalInfo.start[i] = start[i];
		decalInfo.end[i] = end[i];
	}
}

/*
=============
anMaterial::GetExpressionConstant
=============
*/
int anMaterial::GetExpressionConstant( float f ) {
	for ( int i = EXP_REG_NUM_PREDEFINED; i < numRegisters; i++ ) {
		if ( !pd->registerIsTemporary[i] && pd->shaderRegisters[i] == f ) {
			return i;
		}
	}
	if ( numRegisters == MAX_EXP_REGS ) {
		common->Warning( "GetExpressionConstant: material '%s' hit MAX_EXP_REGS", GetName() );
		SetMaterialFlag( MF_DEFAULTED );
		return 0;
	}
	pd->registerIsTemporary[i] = false;
	pd->shaderRegisters[i] = f;
	numRegisters++;

	return i;
}

/*
=============
anMaterial::GetExpressionTemporary
=============
*/
int anMaterial::GetExpressionTemporary( void ) {
	if ( numRegisters == MAX_EXP_REGS ) {
		common->Warning( "GetExpressionTemporary: material '%s' hit MAX_EXP_REGS", GetName() );
		SetMaterialFlag( MF_DEFAULTED );
		return 0;
	}
	pd->registerIsTemporary[numRegisters] = true;
	numRegisters++;
	return numRegisters - 1;
}

/*
=============
anMaterial::GetExpressionOp
=============
*/
expOp_t	*anMaterial::GetExpressionOp( void ) {
	if ( numOps == MAX_EXP_OPS ) {
		common->Warning( "GetExpressionOp: material '%s' hit MAX_EXP_OPS", GetName() );
		SetMaterialFlag( MF_DEFAULTED );
		return &pd->shaderOps[0];
	}

	return &pd->shaderOps[numOps++];
}

/*
=================
anMaterial::EmitOp
=================
*/
int anMaterial::EmitOp( int a, int b, expOpType_t opType ) {
	// optimize away identity operations
	if ( opType == OP_TYPE_ADD ) {
		if ( !pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 0 ) {
			return b;
		}
		if ( !pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 0 ) {
			return a;
		}
		if ( !pd->registerIsTemporary[a] && !pd->registerIsTemporary[b] ) {
			return GetExpressionConstant( pd->shaderRegisters[a] + pd->shaderRegisters[b] );
		}
	}
	if ( opType == OP_TYPE_MULTIPLY ) {
		if ( !pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 1 ) {
			return b;
		}
		if ( !pd->registerIsTemporary[a] && pd->shaderRegisters[a] == 0 ) {
			return a;
		}
		if ( !pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 1 ) {
			return a;
		}
		if ( !pd->registerIsTemporary[b] && pd->shaderRegisters[b] == 0 ) {
			return b;
		}
		if ( !pd->registerIsTemporary[a] && !pd->registerIsTemporary[b] ) {
			return GetExpressionConstant( pd->shaderRegisters[a] * pd->shaderRegisters[b] );
		}
	}

	expOp_t	*op = GetExpressionOp();
	op->opType = opType;
	op->a = a;
	op->b = b;
	op->c = GetExpressionTemporary();

	return op->c;
}

/*
=================
anMaterial::ParseEmitOp
=================
*/
int anMaterial::ParseEmitOp( anLexer &src, int a, expOpType_t opType, int priority ) {
	int b = ParseExpressionPriority( src, priority );
	return EmitOp( a, b, opType );
}

/*
=================
anMaterial::ParseTerm

Returns a register index
=================
*/
int anMaterial::ParseTerm( anLexer &src ) {
	src.ReadToken( &token );

	if ( anToken token == "( " ) {
		int a = ParseExpression( src );
		MatchToken( src, " )" );
		return a;
	}

	if ( !token.Icmp( "time" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_TIME;
	}
	if ( !token.Icmp( "parm0" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM0;
	}
	if ( !token.Icmp( "parm1" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM1;
	}
	if ( !token.Icmp( "parm2" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM2;
	}
	if ( !token.Icmp( "parm3" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM3;
	}
	if ( !token.Icmp( "parm4" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM4;
	}
	if ( !token.Icmp( "parm5" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM5;
	}
	if ( !token.Icmp( "parm6" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM6;
	}
	if ( !token.Icmp( "parm7" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM7;
	}
	if ( !token.Icmp( "parm8" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM8;
	}
	if ( !token.Icmp( "parm9" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM9;
	}
	if ( !token.Icmp( "parm10" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM10;
	}
	if ( !token.Icmp( "parm11" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_PARM11;
	}
	if ( !token.Icmp( "global0" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL0;
	}
	if ( !token.Icmp( "global1" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL1;
	}
	if ( !token.Icmp( "global2" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL2;
	}
	if ( !token.Icmp( "global3" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL3;
	}
	if ( !token.Icmp( "global4" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL4;
	}
	if ( !token.Icmp( "global5" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL5;
	}
	if ( !token.Icmp( "global6" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL6;
	}
	if ( !token.Icmp( "global7" ) ) {
		pd->registersAreConstant = false;
		return EXP_REG_GLOBAL7;
	}
	if ( !token.Icmp( "fragmentPrograms" ) ) {
		return GetExpressionConstant( ( float ) qglConfig.ARBFragmentProgramAvailable );
	}

	if ( !token.Icmp( "sound" ) ) {
		pd->registersAreConstant = false;
		return EmitOp( 0, 0, OP_TYPE_SOUND );
	}

	// parse negative numbers
	if ( token == "-" ) {
		src.ReadToken( &token );
		if ( token.type == TT_NUMBER || token == "." ) {
			return GetExpressionConstant( -( float ) token.GetFloatValue() );
		}
		src.Warning( "Bad negative number '%s'", token.c_str() );
		SetMaterialFlag( MF_DEFAULTED );
		return 0;
	}

	if ( token.type == TT_NUMBER || token == "." || token == "-" ) {
		return GetExpressionConstant( ( float ) token.GetFloatValue() );
	}

	// see if it is a table name
	const anDeclTable *table = static_cast<const anDeclTable *>( declManager->FindType( DECL_TABLE, token.c_str(), false ) );
	if ( !table ) {
		src.Warning( "Bad term '%s'", token.c_str() );
		SetMaterialFlag( MF_DEFAULTED );
		return 0;
	}

	// parse a table expression
	MatchToken( src, "[" );

	int b = ParseExpression( src );

	MatchToken( src, "]" );

	return EmitOp( table->Index(), b, OP_TYPE_TABLE );
}

/*
=================
anMaterial::ParseExpressionPriority

Returns a register index
=================
*/
#define	TOP_PRIORITY 4
int anMaterial::ParseExpressionPriority( anLexer &src, int priority ) {
	anToken token;

	if ( priority == 0 ) {
		return ParseTerm( src );
	}

	int a = ParseExpressionPriority( src, priority - 1 );

	if ( TestMaterialFlag( MF_DEFAULTED ) ) {	// we have a parse error
		return 0;
	}

	if ( !src.ReadToken( &token ) ) {
		// we won't get EOF in a real file, but we can
		// when parsing from generated strings
		return a;
	}

	if ( priority == 1 && token == "*" ) {
		return ParseEmitOp( src, a, OP_TYPE_MULTIPLY, priority );
	}
	if ( priority == 1 && token == "/" ) {
		return ParseEmitOp( src, a, OP_TYPE_DIVIDE, priority );
	}
	if ( priority == 1 && token == "%" ) {	// implied truncate both to integer
		return ParseEmitOp( src, a, OP_TYPE_MOD, priority );
	}
	if ( priority == 2 && token == "+" ) {
		return ParseEmitOp( src, a, OP_TYPE_ADD, priority );
	}
	if ( priority == 2 && token == "-" ) {
		return ParseEmitOp( src, a, OP_TYPE_SUBTRACT, priority );
	}
	if ( priority == 3 && token == ">" ) {
		return ParseEmitOp( src, a, OP_TYPE_GT, priority );
	}
	if ( priority == 3 && token == ">=" ) {
		return ParseEmitOp( src, a, OP_TYPE_GE, priority );
	}
	if ( priority == 3 && token == "<" ) {
		return ParseEmitOp( src, a, OP_TYPE_LT, priority );
	}
	if ( priority == 3 && token == "<=" ) {
		return ParseEmitOp( src, a, OP_TYPE_LE, priority );
	}
	if ( priority == 3 && token == "==" ) {
		return ParseEmitOp( src, a, OP_TYPE_EQ, priority );
	}
	if ( priority == 3 && token == "!=" ) {
		return ParseEmitOp( src, a, OP_TYPE_NE, priority );
	}
	if ( priority == 4 && token == "&&" ) {
		return ParseEmitOp( src, a, OP_TYPE_AND, priority );
	}
	if ( priority == 4 && token == "||" ) {
		return ParseEmitOp( src, a, OP_TYPE_OR, priority );
	}

	// assume that anything else terminates the expression
	// not too robust error checking...
	if ( priority == 4 && token == "!" ) {   // implied truncate both to integer
		return ParseEmitOp( src, a, OP_TYPE_NOT, priority );
	}
	src.UnreadToken( &token );

	return a;
}

/*
=================
anMaterial::ParseExpression

Returns a register index
=================
*/
int anMaterial::ParseExpression( anLexer &src ) {
	return ParseExpressionPriority( src, TOP_PRIORITY );
}


/*
===============
anMaterial::ClearStage
===============
*/
void anMaterial::ClearStage( materialStage_t *ss ) {
	ss->drawStateBits = 0;
	ss->conditionRegister = GetExpressionConstant( 1 );
	ss->color.registers[0] =
	ss->color.registers[1] =
	ss->color.registers[2] =
	ss->color.registers[3] = GetExpressionConstant( 1 );
}

/*
===============
anMaterial::NameToSrcBlendMode
===============
*/
int anMaterial::NameToSrcBlendMode( const anStr &name ) {
	if ( !name.Icmp( "GL_ONE" ) ) {
		return GLS_SRCBLEND_ONE;
	} else if ( !name.Icmp( "GL_ZERO" ) ) {
		return GLS_SRCBLEND_ZERO;
	} else if ( !name.Icmp( "GL_DST_COLOR" ) ) {
		return GLS_SRCBLEND_DST_COLOR;
	} else if ( !name.Icmp( "GL_ONE_MINUS_DST_COLOR" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	} else if ( !name.Icmp( "GL_SRC_ALPHA" ) ) {
		return GLS_SRCBLEND_SRC_ALPHA;
	} else if ( !name.Icmp( "GL_ONE_MINUS_SRC_ALPHA" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !name.Icmp( "GL_DST_ALPHA" ) ) {
		return GLS_SRCBLEND_DST_ALPHA;
	} else if ( !name.Icmp( "GL_ONE_MINUS_DST_ALPHA" ) ) {
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !name.Icmp( "GL_SRC_ALPHA_SATURATE" ) ) {
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	common->Warning( "unknown blend mode '%s' in material '%s'", name.c_str(), GetName() );
	SetMaterialFlag( MF_DEFAULTED );

	return GLS_SRCBLEND_ONE;
}

/*
===============
anMaterial::NameToDstBlendMode
===============
*/
int anMaterial::NameToDstBlendMode( const anStr &name ) {
	if ( !name.Icmp( "GL_ONE" ) ) {
		return GLS_DSTBLEND_ONE;
	} else if ( !name.Icmp( "GL_ZERO" ) ) {
		return GLS_DSTBLEND_ZERO;
	} else if ( !name.Icmp( "GL_SRC_ALPHA" ) ) {
		return GLS_DSTBLEND_SRC_ALPHA;
	} else if ( !name.Icmp( "GL_ONE_MINUS_SRC_ALPHA" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !name.Icmp( "GL_DST_ALPHA" ) ) {
		return GLS_DSTBLEND_DST_ALPHA;
	} else if ( !name.Icmp( "GL_ONE_MINUS_DST_ALPHA" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !name.Icmp( "GL_SRC_COLOR" ) ) {
		return GLS_DSTBLEND_SRC_COLOR;
	} else if ( !name.Icmp( "GL_ONE_MINUS_SRC_COLOR" ) ) {
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	common->Warning( "unknown blend mode '%s' in material '%s'", name.c_str(), GetName() );
	SetMaterialFlag( MF_DEFAULTED );

	return GLS_DSTBLEND_ONE;
}

/*
================
anMaterial::ParseBlend
================
*/
void anMaterial::ParseBlend( anLexer &src, materialStage_t *stage ) {
	anToken token;
	int		srcBlend, dstBlend;

	if ( !src.ReadToken( &token ) ) {
		return;
	}

	// blending combinations
	if ( !token.Icmp( "blend" ) ) {
		stage->drawStateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		return;
	}
	if ( !token.Icmp( "add" ) ) {
		stage->drawStateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
		return;
	}
	if ( !token.Icmp( "filter" ) || !token.Icmp( "modulate" ) ) {
		stage->drawStateBits = GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		return;
	}
	if ( !token.Icmp( "none" ) ) {
		// none is used when defining an alpha mask that doesn't draw
		stage->drawStateBits = GLS_SRCBLEND_ZERO | GLS_DSTBLEND_ONE;
		return;
	}
	if ( !token.Icmp( "localmap" ) ) {
		stage->lighting = SL_BUMP;
		return;
	}
	if ( !token.Icmp( "diffusemap" ) ) {
		stage->lighting = SL_DIFFUSE;
		return;
	}
	if ( !token.Icmp( "specularmap" ) ) {
		stage->lighting = SL_SPECULAR;
		return;
	}

	srcBlend = NameToSrcBlendMode( token );

	MatchToken( src, "," );
	if ( !src.ReadToken( &token ) ) {
		return;
	}
	dstBlend = NameToDstBlendMode( token );

	stage->drawStateBits = srcBlend | dstBlend;
}

/*
================
anMaterial::ParseVertexParm

If there is a single value, it will be repeated across all elements
If there are two values, 3 = 0.0, 4 = 1.0
if there are three values, 4 = 1.0
================
*/
void anMaterial::ParseVertexParm( anLexer &src, materialStage_t *newStage ) {
	anToken				token;

	src.ReadTokenOnLine( &token );
	int	parm = token.GetIntValue();
	if ( !token.IsNumeric() || parm < 0 || parm >= MAX_VERTEX_PARMS ) {
		common->Warning( "bad vertexParm number\n" );
		SetMaterialFlag( MF_DEFAULTED );
		return;
	}
	if ( parm >= newStage->numVertexParms ) {
		newStage->numVertexParms = parm+1;
	}

	newStage->vertexParms[parm][0] = ParseExpression( src );

	src.ReadTokenOnLine( &token );
	if ( !token[0] || token.Icmp( "," ) ) {
		newStage->vertexParms[parm][1] =
		newStage->vertexParms[parm][2] =
		newStage->vertexParms[parm][3] = newStage->vertexParms[parm][0];
		return;
	}

	newStage->vertexParms[parm][1] = ParseExpression( src );

	src.ReadTokenOnLine( &token );
	if ( !token[0] || token.Icmp( "," ) ) {
		newStage->vertexParms[parm][2] = GetExpressionConstant( 0 );
		newStage->vertexParms[parm][3] = GetExpressionConstant( 1 );
		return;
	}

	newStage->vertexParms[parm][2] = ParseExpression( src );

	src.ReadTokenOnLine( &token );
	if ( !token[0] || token.Icmp( "," ) ) {
		newStage->vertexParms[parm][3] = GetExpressionConstant( 1 );
		return;
	}

	newStage->vertexParms[parm][3] = ParseExpression( src );
}


/*
================
anMaterial::ParseFragmentMap
================
*/
void anMaterial::ParseFragmentMap( anLexer &src, materialStage_t *newStage ) {
	str;
	anToken token;

	textureFilter_t tf = TF_DEFAULT;
	textureRepeat_t trp = TR_REPEAT;
	textureDepth_t td = TD_DEFAULT;
	allowPicmip = true;
	cubeFiles_t cubeMap = CF_2D;
	bool				allowPicmip;

	src.ReadTokenOnLine( &token );
	int	unit = token.GetIntValue();
	if ( !token.IsNumeric() || unit < 0 || unit >= MAX_FRAGMENT_IMAGES ) {
		common->Warning( "bad fragmentMap number\n" );
		SetMaterialFlag( MF_DEFAULTED );
		return;
	}

	// unit 1 is the normal map.. make sure it gets flagged as the proper depth
	if ( unit == 1 ) {
		td = TD_BUMP;
	}

	if ( unit >= newStage->numFragmentProgramImages ) {
		newStage->numFragmentProgramImages = unit+1;
	}

	while( 1 ) {
		src.ReadTokenOnLine( &token );
		if ( !token.Icmp( "cubeMap" ) ) {
			cubeMap = CF_NATIVE;
			continue;
		}
		if ( !token.Icmp( "cameraCubeMap" ) ) {
			cubeMap = CF_CAMERA;
			continue;
		}
		if ( !token.Icmp( "nearest" ) ) {
			tf = TF_NEAREST;
			continue;
		}
		if ( !token.Icmp( "linear" ) ) {
			tf = TF_LINEAR;
			continue;
		}
		if ( !token.Icmp( "clamp" ) ) {
			trp = TR_CLAMP;
			continue;
		}
		if ( !token.Icmp( "repeat" ) ) {
			trp = TR_REPEAT;
			continue;
		}
		if ( !token.Icmp( "zeroclamp" ) ) {
			trp = TR_CLAMP_TO_ZERO;
			continue;
		}
		if ( !token.Icmp( "alphazeroclamp" ) ) {
			trp = TR_CLAMP_TO_ZERO_ALPHA;
			continue;
		}
		if ( !token.Icmp( "forceHighQuality" ) ) {
			td = TD_HIGH_QUALITY;
			continue;
		}

		if ( !token.Icmp( "uncompressed" ) || !token.Icmp( "highquality" ) ) {
			if ( !globalImages->image_ignoreHighQuality.GetInteger() ) {
				td = TD_HIGH_QUALITY;
			}
			continue;
		}
		if ( !token.Icmp( "nopicmip" ) ) {
			allowPicmip = false;
			continue;
		}
		// assume anything else is the image name
		src.UnreadToken( &token );
		break;
	}
	const char *str = R_ParsePastImageProgram( src );

	newStage->fragmentProgramImages[unit] =globalImages->ImageFromFile( str, tf, allowPicmip, trp, td, cubeMap );
	if ( !newStage->fragmentProgramImages[unit] ) {
		newStage->fragmentProgramImages[unit] = globalImages->defaultImage;
	}
}

/*
===============
anMaterial::MultiplyTextureMatrix
===============
*/
void anMaterial::MultiplyTextureMatrix( textureStage_t *ts, int registers[2][3] ) {
	int old[2][3];

	if ( !ts->hasMatrix ) {
		ts->hasMatrix = true;
		memcpy( ts->matrix, registers, sizeof( ts->matrix ) );
		return;
	}

	memcpy( old, ts->matrix, sizeof( old ) );

	// multiply the two maticies
	ts->matrix[0][0] = EmitOp(
							EmitOp( old[0][0], registers[0][0], OP_TYPE_MULTIPLY ),
							EmitOp( old[0][1], registers[1][0], OP_TYPE_MULTIPLY ), OP_TYPE_ADD );
	ts->matrix[0][1] = EmitOp(
							EmitOp( old[0][0], registers[0][1], OP_TYPE_MULTIPLY ),
							EmitOp( old[0][1], registers[1][1], OP_TYPE_MULTIPLY ), OP_TYPE_ADD );
	ts->matrix[0][2] = EmitOp(
							EmitOp(
								EmitOp( old[0][0], registers[0][2], OP_TYPE_MULTIPLY ),
								EmitOp( old[0][1], registers[1][2], OP_TYPE_MULTIPLY ), OP_TYPE_ADD ),
							old[0][2], OP_TYPE_ADD );

	ts->matrix[1][0] = EmitOp(
							EmitOp( old[1][0], registers[0][0], OP_TYPE_MULTIPLY ),
							EmitOp( old[1][1], registers[1][0], OP_TYPE_MULTIPLY ), OP_TYPE_ADD );
	ts->matrix[1][1] = EmitOp(
							EmitOp( old[1][0], registers[0][1], OP_TYPE_MULTIPLY ),
							EmitOp( old[1][1], registers[1][1], OP_TYPE_MULTIPLY ), OP_TYPE_ADD );
	ts->matrix[1][2] = EmitOp(
							EmitOp(
								EmitOp( old[1][0], registers[0][2], OP_TYPE_MULTIPLY ),
								EmitOp( old[1][1], registers[1][2], OP_TYPE_MULTIPLY ), OP_TYPE_ADD ),
							old[1][2], OP_TYPE_ADD );

}

/*
=================
anMaterial::ParseStage
An open brace has been parsed
{
	if <expression>
	map <imageprogram>
	"nearest" "linear" "clamp" "zeroclamp" "uncompressed" "highquality" "nopicmip"
	scroll, scale, rotate
}
=================
*/
void anMaterial::ParseStage( anLexer &src, const textureRepeat_t trpDefault ) {
	anToken				token;
	const char			*str;
	char				imageName[MAX_IMAGE_NAME];
	int					a, b;
	int					matrix[2][3];
	materialStage_t	newStage;

	if ( numStages >= MAX_SHADER_STAGES ) {
		SetMaterialFlag( MF_DEFAULTED );
		common->Warning( "material '%s' exceeded %i stages", GetName(), MAX_SHADER_STAGES );
	}

	textureFilter_t tf = TF_DEFAULT;
	textureRepeat_t trp = trpDefault;
	textureDepth_t td = TD_DEFAULT;
	bool allowPicmip = true;
	cubeFiles_t cubeMap = CF_2D;

	imageName[0] = 0;

	memset( &newStage, 0, sizeof( newStage ) );

	materialStage_t *ss = &pd->parseStages[numStages];
	textureStage_t ts = &ss->texture;

	ClearStage( ss );

	while ( 1 ) {
		if ( TestMaterialFlag( MF_DEFAULTED ) ) {	// we have a parse error
			return;
		}
		if ( !src.ExpectAnyToken( &token ) ) {
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}

		// the close brace for the entire material ends the draw block
		if ( token == "}" ) {
			break;
		}

		//BSM Nerve: Added for stage naming in the material editor
		if ( !token.Icmp( "name" ) ) {
			src.SkipRestOfLine();
			continue;
		}

		// image options
		if ( !token.Icmp( "blend" ) ) {
			ParseBlend( src, ss );
			continue;
		}

		if ( !token.Icmp( "map" ) ) {
			const char *str = R_ParsePastImageProgram( src );
			anStr::Copynz( imageName, str, sizeof( imageName ) );
			continue;
		}

		if ( !token.Icmp( "remoteRenderMap" ) ) {
			ts->dynamic = DI_REMOTE_RENDER;
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			continue;
		}

		if ( !token.Icmp( "mirrorRenderMap" ) ) {
			ts->dynamic = DI_MIRROR_RENDER;
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			ts->texgen = TG_SCREEN;
			continue;
		}

		if ( !token.Icmp( "xrayRenderMap" ) ) {
			ts->dynamic = DI_XRAY_RENDER;
			ts->width = src.ParseInt();
			ts->height = src.ParseInt();
			ts->texgen = TG_SCREEN;
			continue;
		}
		if ( !token.Icmp( "screen" ) ) {
			ts->texgen = TG_SCREEN;
			continue;
		}
		if ( !token.Icmp( "screen2" ) ) {
			ts->texgen = TG_SCREEN2;
			continue;
		}
		if ( !token.Icmp( "glassWarp" ) ) {
			ts->texgen = TG_GLASSWARP;
			continue;
		}

		if ( !token.Icmp( "videomap" ) ) {
			// note that videomaps will always be in clamp mode, so texture
			// coordinates had better be in the 0 to 1 range
			if ( !src.ReadToken( &token ) ) {
				common->Warning( "missing parameter for 'videoMap' keyword in material '%s'", GetName() );
				continue;
			}
			bool loop = false;
			if ( !token.Icmp( "loop" ) ) {
				loop = true;
				if ( !src.ReadToken( &token ) ) {
					common->Warning( "missing parameter for 'videoMap' keyword in material '%s'", GetName() );
					continue;
				}
			}
			ts->cinematic = idCinematic::Alloc();
			ts->cinematic->InitFromFile( token.c_str(), loop );
			continue;
		}

		if ( !token.Icmp( "sndmap" ) ) {
			if ( !src.ReadToken( &token ) ) {
				common->Warning( "missing parameter for 'sndmap' keyword in material '%s'", GetName() );
				continue;
			}
			ts->cinematic = new idSndWindow();
			ts->cinematic->InitFromFile( token.c_str(), true );
			continue;
		}

		if ( !token.Icmp( "cubeMap" ) ) {
			str = R_ParsePastImageProgram( src );
			anStr::Copynz( imageName, str, sizeof( imageName ) );
			cubeMap = CF_NATIVE;
			continue;
		}

		if ( !token.Icmp( "cameraCubeMap" ) ) {
			str = R_ParsePastImageProgram( src );
			anStr::Copynz( imageName, str, sizeof( imageName ) );
			cubeMap = CF_CAMERA;
			continue;
		}
			blockSize = blockSize_;en.Icmp( "ignoreAlphaTest" ) ) {
			ss->ignoreAlphaTest = true;
			continue;
		}
		if ( !token.Icmp( "default" ) ) {
			tf = TF_DEFAULT;
			continue;
		}
		if ( !token.Icmp( "nearest" ) ) {
			tf = TF_NEAREST;
			continue;
		}
		if ( !token.Icmp( "linearnearest" ) ) {
			tf = TF_LINEARNEAREST;
			continue;
		}
		if ( !token.Icmp( "linear" ) ) {
			tf = TF_LINEAR;
			continue;
		}
		if ( !token.Icmp( "clamp" ) ) {
			trp = TR_CLAMP;
			continue;
		}
		if ( !token.Icmp( "repeat" ) ) { // changed noclamp to repeat
			trp = TR_REPEAT;
			continue;
		}
		if ( !token.Icmp( "zeroclamp" ) ) {
			trp = TR_CLAMP_TO_ZERO;
			continue;
		}
		if ( !token.Icmp( "alphazeroclamp" ) ) {
			trp = TR_CLAMP_TO_ZERO_ALPHA;
			continue;
		}
		if ( !token.Icmp( "clamptox" ) ) {
			trp = TR_CLAMP_X;
			continue;
		}
		if ( !token.Icmp( "clamptoy" ) ) {
			trp = TR_CLAMP_Y;
			continue;
		}
		if ( !token.Icmp( "mirror" ) ) {
			trp = TR_MIRROR;
			continue;
		}
		if ( !token.Icmp( "mirrorx" ) ) {
			trp = TR_MIRROR_X;
			continue;
		}
		if ( !token.Icmp( "mirrory" ) ) {
			trp = TR_MIRROR_Y;
			continue;
		}

		if ( !token.Icmp( "uncompressed" ) || !token.Icmp( "highquality" ) ) {
			if ( !globalImages->image_ignoreHighQuality.GetInteger() ) {
				td = TD_HIGH_QUALITY;
			}
			continue;
		}
		if ( !token.Icmp( "forceHighQuality" ) ) {
			td = TD_HIGH_QUALITY;
			continue;
		}
		if ( !token.Icmp( "nopicmip" ) ) {
			allowPicmip = false;
			continue;
		}
		if ( !token.Icmp( "vertexColorIgnore" ) ) {
			ss->vertexColor = SVC_IGNORE;
			continue;
		}
		if ( !token.Icmp( "vertexColor" ) ) {
			ss->vertexColor = SVC_MODULATE;
			continue;
		}
		if ( !token.Icmp( "vertexColorAlpha" ) ) {
			ss->vertexColor = SVC_MODULATE_ALPHA;
			continue;
		}
		if ( !token.Icmp( "inverseVertexColorAlpha" ) ) {
			ss->vertexColor = SVC_INVERSE_MODULATE_ALPHA; // FIXME: this should be SVC_INVERSE_MODULATE_ALPHA not implemented yet
			continue;
		if ( !token.Icmp( "inverseVertexColor" ) ) {
			ss->vertexColor = SVC_INVERSE_MODULATE;
			continue;
		} else if ( !token.Icmp( "privatePolygonOffset" ) ) {
		// privatePolygonOffset
			if ( !src.ReadTokenOnLine( &token ) ) {
				ss->privatePolygonOffset = 1;
				continue;
			}
			// explict larger (or negative) offset
			src.UnreadToken( &token );
			ss->privatePolygonOffset = src.ParseFloat();
			continue;
		}

		// texture coordinate generation
		if ( !token.Icmp( "texGen" ) ) {
			src.ExpectAnyToken( &token );
			if ( !token.Icmp( "normal" ) ) {
				ts->texgen = TG_DIFFUSE_CUBE;
			} else if ( !token.Icmp( "reflect" ) ) {
				ts->texgen = TG_REFLECT_CUBE;
			} else if ( !token.Icmp( "skybox" ) ) {
				ts->texgen = TG_SKYBOX_CUBE;
			} else if ( !token.Icmp( "wobbleSky" ) ) {
				ts->texgen = TG_WOBBLESKY_CUBE;
				texGenRegisters[0] = ParseExpression( src );
				texGenRegisters[1] = ParseExpression( src );
				texGenRegisters[2] = ParseExpression( src );
			} else {
				common->Warning( "bad texGen '%s' in material %s", token.c_str(), GetName() );
				SetMaterialFlag( MF_DEFAULTED );
			}
			continue;
		}
		if ( !token.Icmp( "scroll" ) || !token.Icmp( "translate" ) ) {
			a = ParseExpression( src );
			MatchToken( src, "," );
			b = ParseExpression( src );
			matrix[0][0] = GetExpressionConstant( 1 );
			matrix[0][1] = GetExpressionConstant( 0 );
			matrix[0][2] = a;
			matrix[1][0] = GetExpressionConstant( 0 );
			matrix[1][1] = GetExpressionConstant( 1 );
			matrix[1][2] = b;

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}
		if ( !token.Icmp( "scale" ) ) {
			a = ParseExpression( src );
			MatchToken( src, "," );
			b = ParseExpression( src );
			// this just scales without a centering
			matrix[0][0] = a;
			matrix[0][1] = GetExpressionConstant( 0 );
			matrix[0][2] = GetExpressionConstant( 0 );
			matrix[1][0] = GetExpressionConstant( 0 );
			matrix[1][1] = b;
			matrix[1][2] = GetExpressionConstant( 0 );

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}
		if ( !token.Icmp( "centerScale" ) ) {
			a = ParseExpression( src );
			MatchToken( src, "," );
			b = ParseExpression( src );
			// this subtracts 0.5, then scales, then adds 0.5
			matrix[0][0] = a;
			matrix[0][1] = GetExpressionConstant( 0 );
			matrix[0][2] = EmitOp( GetExpressionConstant( 0.5f ), EmitOp( GetExpressionConstant( 0.5f ), a, OP_TYPE_MULTIPLY ), OP_TYPE_SUBTRACT );
			matrix[1][0] = GetExpressionConstant( 0 );
			matrix[1][1] = b;
			matrix[1][2] = EmitOp( GetExpressionConstant( 0.5f ), EmitOp( GetExpressionConstant( 0.5f ), b, OP_TYPE_MULTIPLY ), OP_TYPE_SUBTRACT );

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}
		if ( !token.Icmp( "shear" ) ) {
			a = ParseExpression( src );
			MatchToken( src, "," );
			b = ParseExpression( src );
			// this subtracts 0.5, then shears, then adds 0.5
			matrix[0][0] = GetExpressionConstant( 1 );
			matrix[0][1] = a;
			matrix[0][2] = EmitOp( GetExpressionConstant( -0.5f ), a, OP_TYPE_MULTIPLY );
			matrix[1][0] = b;
			matrix[1][1] = GetExpressionConstant( 1 );
			matrix[1][2] = EmitOp( GetExpressionConstant( -0.5f ), b, OP_TYPE_MULTIPLY );

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}
		if ( !token.Icmp( "rotate" ) ) {
			const anDeclTable *table;
			int		sinReg, cosReg;

			// in cycles
			a = ParseExpression( src );

			table = static_cast<const anDeclTable *>( declManager->FindType( DECL_TABLE, "sinTable", false ) );
			if ( !table ) {
				common->Warning( "no sinTable for rotate defined" );
				SetMaterialFlag( MF_DEFAULTED );
				return;
			}
			sinReg = EmitOp( table->Index(), a, OP_TYPE_TABLE );

			table = static_cast<const anDeclTable *>( declManager->FindType( DECL_TABLE, "cosTable", false ) );
			if ( !table ) {
				common->Warning( "no cosTable for rotate defined" );
				SetMaterialFlag( MF_DEFAULTED );
				return;
			}
			cosReg = EmitOp( table->Index(), a, OP_TYPE_TABLE );

			// this subtracts 0.5, then rotates, then adds 0.5
			matrix[0][0] = cosReg;
			matrix[0][1] = EmitOp( GetExpressionConstant( 0 ), sinReg, OP_TYPE_SUBTRACT );
			matrix[0][2] = EmitOp( EmitOp( EmitOp( GetExpressionConstant( -0.5f ), cosReg, OP_TYPE_MULTIPLY ),
										EmitOp( GetExpressionConstant( 0.5f ), sinReg, OP_TYPE_MULTIPLY ), OP_TYPE_ADD ),
										GetExpressionConstant( -0.5f ), OP_TYPE_ADD );

			matrix[1][0] = sinReg;
			matrix[1][1] = cosReg;
			matrix[1][2] = EmitOp( EmitOp( EmitOp( GetExpressionConstant( -0.5 ), sinReg, OP_TYPE_MULTIPLY ),
										EmitOp( GetExpressionConstant( -0.5 ), cosReg, OP_TYPE_MULTIPLY ), OP_TYPE_ADD ),
										GetExpressionConstant( 0.5 ), OP_TYPE_ADD );

			MultiplyTextureMatrix( ts, matrix );
			continue;
		}

		// color mask options
		if ( !token.Icmp( "maskRed" ) ) {
			ss->drawStateBits |= GLS_REDMASK;
			continue;
		}
		if ( !token.Icmp( "maskGreen" ) ) {
			ss->drawStateBits |= GLS_GREENMASK;
			continue;
		}
		if ( !token.Icmp( "maskBlue" ) ) {
			ss->drawStateBits |= GLS_BLUEMASK;
			continue;
		}
		if ( !token.Icmp( "maskAlpha" ) ) {
			ss->drawStateBits |= GLS_ALPHAMASK;
			continue;
		}
		if ( !token.Icmp( "maskColor" ) ) {
			ss->drawStateBits |= GLS_COLORMASK;
			continue;
		}
		if ( !token.Icmp( "maskDepth" ) ) {
			ss->drawStateBits |= GLS_DEPTHMASK;
			continue;
		}
		if ( !token.Icmp( "alphaTest" ) ) {
			ss->hasAlphaTest = true;
			ss->alphaTestRegister = ParseExpression( src );
			coverage = MC_PERFORATED;
			continue;
		}

		// shorthand for 2D modulated
		if ( !token.Icmp( "colored" ) ) {
			ss->color.registers[0] = EXP_REG_PARM0;
			ss->color.registers[1] = EXP_REG_PARM1;
			ss->color.registers[2] = EXP_REG_PARM2;
			ss->color.registers[3] = EXP_REG_PARM3;
			pd->registersAreConstant = false;
			continue;
		}

		if ( !token.Icmp( "color" ) ) {
			ss->color.registers[0] = ParseExpression( src );
			MatchToken( src, "," );
			ss->color.registers[1] = ParseExpression( src );
			MatchToken( src, "," );
			ss->color.registers[2] = ParseExpression( src );
			MatchToken( src, "," );
			ss->color.registers[3] = ParseExpression( src );
			continue;
		}
		if ( !token.Icmp( "red" ) ) {
			ss->color.registers[0] = ParseExpression( src );
			continue;
		}
		if ( !token.Icmp( "green" ) ) {
			ss->color.registers[1] = ParseExpression( src );
			continue;
		}
		if ( !token.Icmp( "blue" ) ) {
			ss->color.registers[2] = ParseExpression( src );
			continue;
		}
		if ( !token.Icmp( "alpha" ) ) {
			ss->color.registers[3] = ParseExpression( src );
			continue;
		}
		if ( !token.Icmp( "rgb" ) ) {
			ss->color.registers[0] = ss->color.registers[1] =
				ss->color.registers[2] = ParseExpression( src );
			continue;
		}
		if ( !token.Icmp( "rgba" ) ) {
			ss->color.registers[0] = ss->color.registers[1] =
				ss->color.registers[2] = ss->color.registers[3] = ParseExpression( src );
			continue;
		}

		if ( !token.Icmp( "if" ) ) {
			ss->conditionRegister = ParseExpression( src );
			continue;
		}
		if ( !token.Icmp( "program" ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				newStage.vertexProgram = R_FindARBProgram( GL_VERTEX_PROGRAM_ARB, token.c_str() );
				newStage.fragmentProgram = R_FindARBProgram( GL_FRAGMENT_PROGRAM_ARB, token.c_str() );
			}
			continue;
		}
		if ( !token.Icmp( "fragmentProgram" ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				newStage.fragmentProgram = R_FindARBProgram( GL_FRAGMENT_PROGRAM_ARB, token.c_str() );
			}
			continue;
		}
		if ( !token.Icmp( "vertexProgram" ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				newStage.vertexProgram = R_FindARBProgram( GL_VERTEX_PROGRAM_ARB, token.c_str() );
			}
			continue;
		}
		if ( !token.Icmp( "megaTexture" ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				newStage.megaTexture = new idMegaTexture;
				if ( !newStage.megaTexture->InitFromMegaFile( token.c_str() ) ) {
					delete newStage.megaTexture;
					SetMaterialFlag( MF_DEFAULTED );
					continue;
				}
				newStage.vertexProgram = R_FindARBProgram( GL_VERTEX_PROGRAM_ARB, "megaTexture.vfp" );
				newStage.fragmentProgram = R_FindARBProgram( GL_FRAGMENT_PROGRAM_ARB, "megaTexture.vfp" );
				continue;
			}
		}

		if ( !token.Icmp( "vertexParm2" ) ) {
			ParseVertexParm2( src, &newStage );
			continue;
		}

		if ( !token.Icmp( "vertexParm" ) ) {
			ParseVertexParm( src, &newStage );
			continue;
		}

		if ( !token.Icmp( "fragmentMap" ) ) {
			ParseFragmentMap( src, &newStage );
			continue;
		}
		common->Warning( "unknown token '%s' in material '%s'", token.c_str(), GetName() );
		SetMaterialFlag( MF_DEFAULTED );
		return;
	}

	// if we are using newStage, allocate a copy of it
	if ( newStage.fragmentProgram || newStage.vertexProgram ) {
		//newStage.glslProgram = renderProgManager.FindGLSLProgram( GetName(), newStage.vertexProgram, newStage.fragmentProgram );
		ss->newStage = (materialStage_t *)Mem_Alloc( sizeof( newStage ) );//, TAG_MATERIAL );
		*( ss->newStage) = newStage;
	}

	// successfully parsed a stage
	numStages++;

	// select a compressed depth based on what the stage is
	if ( td == TD_DEFAULT ) {
		switch ( ss->lighting ) {
		case SL_AMBIENT:
			td = SL_AMBIENT;
			break;
		case SL_BUMP:
			td = TD_BUMP;
			break;
		case SL_DIFFUSE:
			td = TD_DIFFUSE;
			break;
		case SL_SPECULAR:
			td = TD_SPECULAR;
			break;
		case SL_COVERAGE:
			td = SL_COVERAGE;
			break;
		default:
			break;
		}
	}

	// now load the image with all the parms we parsed
	if ( imageName[0] ) {
		ts->image = globalImages->ImageFromFile( imageName, tf, allowPicmip, trp, td, cubeMap );
		if ( !ts->image ) {
			ts->image = globalImages->defaultImage;
		}
	} else if ( !ts->cinematic && !ts->dynamic && !ss->newStage ) {
		common->Warning( "material '%s' had stage with no image", GetName() );
		ts->image = globalImages->defaultImage;
	}
}

/*
===============
anMaterial::ParseDeform
===============
*/
void anMaterial::ParseDeform( anLexer &src ) {
    anToken token;

    // Expect a token from the source lexer
    if ( !src.ExpectAnyToken( &token ) ) {
        return;
    }

    // Check if the token is "sprite" or "tube"
    if ( !token.Icmp( "sprite" ) || !token.Icmp( "tube" ) ) {
        // Set the deform type accordingly
        deform = !token.Icmp( "sprite" ) ? DFRM_SPRITE : DFRM_TUBE;
        // Set the cullType to CT_TWO_SIDED
        cullType = CT_TWO_SIDED;
        // Set the material flag MF_NOSHADOWS
        SetMaterialFlag( MF_NOSHADOWS );
        return;
    }

    // Check if the token is "flare"
    if ( !token.Icmp( "flare" ) ) {
        // Set the deform type to DFRM_FLARE
        deform = DFRM_FLARE;
        // Parse the expression from the source lexer and store it in deformRegisters[0]
        deformRegisters[0] = ParseExpression( src );
        // Set the material flag MF_NOSHADOWS
        SetMaterialFlag( MF_NOSHADOWS );
        return;
    }

    // Check if the token is "expand", "move", "turbulent", "eyeBall", "particle", or "particle2"
    if ( !token.Icmp( "expand" ) || !token.Icmp( "move" ) || !token.Icmp( "turbulent" ) || !token.Icmp( "eyeBall" ) || !token.Icmp( "particle" ) || !token.Icmp( "particle2" ) !token.Icmp( "glow" ) ) {
        // Expect another token from the source lexer
        if ( !src.ExpectAnyToken( &token ) ) {
            // Print a warning message if the token is missing
            src.Warning( "deform particle missing particle name" );
            // Set the material flag MF_DEFAULTED
            SetMaterialFlag( MF_DEFAULTED );
            return;
        }

        // Check the value of the token and set the deform type and deformDecl accordingly
        if ( !token.Icmp( "expand" ) ) {
            deform = DFRM_EXPAND;
        } else if ( !token.Icmp( "move" ) ) {
            deform = DFRM_MOVE;
        } else if ( !token.Icmp( "turbulent" ) ) {
            deform = DFRM_TURB;
            deformDecl = declManager->FindType( DECL_TABLE, token.c_str(), true );
            deformRegisters[0] = ParseExpression( src );
            deformRegisters[1] = ParseExpression( src );
            deformRegisters[2] = ParseExpression( src );
        } else if ( !token.Icmp( "eyeBall" ) ) {
            deform = DFRM_EYEBALL;
        } else if ( !token.Icmp( "particle" ) || !token.Icmp( "particle2" ) ) {
            deform = DFRM_PARTICLE;
        } else if ( !token.Icmp( "glow" ) ) {
            deform = DFRM_GLOW;
        }
        // Find the deform declaration of type DECL_PARTICLE with the given token name
        deformDecl = declManager->FindType( DECL_PARTICLE, token.c_str(), true );
        return;
    }

    // Print a warning message if the token is not recognized
    src.Warning( "Bad deform type '%s'", token.c_str() );
    // Set the material flag MF_DEFAULTED
    SetMaterialFlag( MF_DEFAULTED );
}
/*
==============
anMaterial::AddImplicitStages

If a material has diffuse or specular stages without any
bump stage, add an implicit _flat bumpmap stage.

If a material has a bump stage but no diffuse or specular
stage, add a _white diffuse stage.

It is valid to have either a diffuse or specular without the other.

It is valid to have a reflection map and a bump map for bumpy reflection
==============
*/
void anMaterial::AddImplicitStages( const textureRepeat_t trpDefault /* = TR_REPEAT  */ ) {
	char buffer[1024];
	anLexer newSrc;
	bool hasDiffuse = false, hasSpecular = false, hasBump = false, hasReflection = false;

	for ( int i = 0; i < numStages; i++ ) {
		if ( pd->parseStages[i].lighting == SL_BUMP ) {
			hasBump = true;
		}
		if ( pd->parseStages[i].lighting == SL_DIFFUSE ) {
			hasDiffuse = true;
		}
		if ( pd->parseStages[i].lighting == SL_SPECULAR ) {
			hasSpecular = true;
		}
		if ( pd->parseStages[i].texture.texgen == TG_REFLECT_CUBE ) {
			hasReflection = true;
		}
	}

	// if it doesn't have an interaction at all, don't add anything
	if ( !hasBump && !hasDiffuse && !hasSpecular ) {
		return;
	}

	if ( numStages == MAX_SHADER_STAGES ) {
		return;
	}

	if ( !hasBump ) {
		anStr::snPrintf( buffer, sizeof( buffer ), "blend bumpmap\nmap _flat\n}\n" );
		newSrc.LoadMemory( buffer, strlen(buffer), "bumpmap" );
		newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
		ParseStage( newSrc, trpDefault );
		newSrc.FreeSource();
	}

	if ( !hasDiffuse && !hasSpecular && !hasReflection ) {
		anStr::snPrintf( buffer, sizeof( buffer ), "blend diffusemap\nmap _white\n}\n" );
		newSrc.LoadMemory( buffer, strlen(buffer), "diffusemap" );
		newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
		ParseStage( newSrc, trpDefault );
		newSrc.FreeSource();
	}
}

/*
===============
anMaterial::SortInteractionStages

The renderer expects bump, then diffuse, then specular
There can be multiple bump maps, followed by additional
diffuse and specular stages, which allows cross-faded bump mapping.

Ambient stages can be interspersed anywhere, but they are
ignored during interactions, and all the interaction
stages are ignored during ambient drawing.
===============
*/
void anMaterial::SortInteractionStages() {
	for ( int i = 0; i < numStages; i = j ) {
		// find the next bump map
		for ( int j = i + 1; j < numStages; j++ ) {
			if ( pd->parseStages[j].lighting == SL_BUMP ) {
				// if the very first stage wasn't a bumpmap,
				// this bumpmap is part of the first group
				if ( pd->parseStages[i].lighting != SL_BUMP ) {
					continue;
				}
				break;
			}
		}

		// bubble sort everything bump / diffuse / specular
		for ( int l = 1; l < j-i; l++ ) {
			for ( int k = i; k < j-l; k++ ) {
				if ( pd->parseStages[k].lighting > pd->parseStages[k+1].lighting ) {
					materialStage_t	temp;

					temp = pd->parseStages[k];
					pd->parseStages[k] = pd->parseStages[k+1];
					pd->parseStages[k+1] = temp;
				}
			}
		}
	}
}

/*
=================
anMaterial::ParseMaterial

The current text pointer is at the explicit text definition of the
Parse it into the global material variable. Later functions will optimize it.

If there is any error during parsing, defaultShader will be set.
=================
*/
void anMaterial::ParseMaterial( anLexer &src ) {
	anToken		token;
	char		buffer[1024];
	const char	*str;
	anLexer		newSrc;

	int s = 0;

	numOps = 0;
	numRegisters = EXP_REG_NUM_PREDEFINED;	// leave space for the parms to be copied in
	for ( int i = 0; i < numRegisters; i++ ) {
		pd->registerIsTemporary[i] = true;		// they aren't constants that can be folded
	}

	numStages = 0;

	textureRepeat_t	trpDefault = TR_REPEAT;		// allow a global setting for repeat

	while ( 1 ) {
		if ( TestMaterialFlag( MF_DEFAULTED ) ) {	// we have a parse error
			return;
		}
		if ( !src.ExpectAnyToken( &token ) ) {
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}

		// end of material definition
		if ( token == "}" ) {
			break;
		} else if ( !token.Icmp( "qer_editorimage" ) ) {
			src.ReadTokenOnLine( &token );
			editorImageName = token.c_str();
			src.SkipRestOfLine();
			continue;
		// description
		} else if ( !token.Icmp( "description" ) ) {
			src.ReadTokenOnLine( &token );
			desc = token.c_str();
			continue;
		// check for the surface / content bit flags
		} else if ( CheckSurfaceParm( &token ) ) {
			continue;
		// polygonOffset
		} else if ( !token.Icmp( "polygonOffset" ) ) {
			SetMaterialFlag( MF_POLYGONOFFSET );
			if ( !src.ReadTokenOnLine( &token ) ) {
				polygonOffset = 1;
				continue;
			}
			// explict larger (or negative) offset
			polygonOffset = token.GetFloatValue();
			continue;
		// noshadow
		} else if ( !token.Icmp( "noShadows" ) ) {
			SetMaterialFlag( MF_NOSHADOWS );
			continue;
		} else if ( !token.Icmp( "suppressInSubview" ) ) {
			suppressInSubview = true;
			continue;
		} else if ( !token.Icmp( "portalSky" ) ) {
			portalSky = true;
			continue;
		// noSelfShadow
		} else if ( !token.Icmp( "noSelfShadow" ) ) {
			SetMaterialFlag( MF_NOSELFSHADOW );
			continue;
		// noPortalFog
		} else if ( !token.Icmp( "noPortalFog" ) ) {
			SetMaterialFlag( MF_NOPORTALFOG );
			continue;
		// forceShadows allows nodraw surfaces to cast shadows
		} else if ( !token.Icmp( "forceShadows" ) ) {
			SetMaterialFlag( MF_FORCESHADOWS );
			continue;
		// overlay / decal suppression
		} else if ( !token.Icmp( "noOverlays" ) ) {
			allowOverlays = false;
			continue;
		// moster blood overlay forcing for alpha tested or translucent surfaces
		} else if ( !token.Icmp( "forceOverlays" ) ) {
			pd->forceOverlays = true;
			continue;
		// translucent
		} else if ( !token.Icmp( "translucent" ) ) {
			coverage = MC_TRANSLUCENT;
			continue;
		// global zero clamp
		} else if ( !token.Icmp( "zeroclamp" ) ) {
			trpDefault = TR_CLAMP_TO_ZERO;
			continue;
		// global clamp
		} else if ( !token.Icmp( "clamp" ) ) {
			trpDefault = TR_CLAMP;
			continue;
		// global clamp
		} else if ( !token.Icmp( "alphazeroclamp" ) ) {
			trpDefault = TR_CLAMP_TO_ZERO;
			continue;
		// forceOpaque is used for skies-behind-windows
		} else if ( !token.Icmp( "forceOpaque" ) ) {
			coverage = MC_OPAQUE;
			continue;
		// twoSided
		} else if ( !token.Icmp( "twoSided" ) ) {
			cullType = CT_TWO_SIDED;
			// twoSided implies no-shadows, because the shadow
			// volume would be coplanar with the surface, giving depth fighting
			// we could make this no-self-shadows, but it may be more important
			// to receive shadows from no-self-shadow monsters
			SetMaterialFlag( MF_NOSHADOWS );
		// backSided
		} else if ( !token.Icmp( "backSided" ) ) {
			cullType = CT_BACK_SIDED;
			// the shadow code doesn't handle this, so just disable shadows.
			// We could fix this in the future if there was a need.
			SetMaterialFlag( MF_NOSHADOWS );
		// foglight
		} else if ( !token.Icmp( "fogLight" ) ) {
			fogLight = true;
			continue;
		// blendlight
		} else if ( !token.Icmp( "blendLight" ) ) {
			blendLight = true;
			continue;
		// ambientLight
		} else if ( !token.Icmp( "ambientLight" ) ) {
			ambientLight = true;
			continue;
		// mirror
		} else if ( !token.Icmp( "mirror" ) ) {
			sort = SS_SUBVIEW;
			coverage = MC_OPAQUE;
			continue;
		// noFog
		} else if ( !token.Icmp( "noFog" ) ) {
			noFog = true;
			continue;
		// unsmoothedTangents
		} else if ( !token.Icmp( "unsmoothedTangents" ) ) {
			unsmoothedTangents = true;
			continue;
		// lightFallofImage <imageprogram>
		// specifies the image to use for the third axis of projected
		// light volumes
		} else if ( !token.Icmp( "lightFalloffImage" ) ) {
			str = R_ParsePastImageProgram( src );
			anStr	copy;

			copy = str;	// so other things don't step on it
			lightFalloffImage = globalImages->ImageFromFile( copy, TF_DEFAULT, false, TR_CLAMP /* TR_CLAMP_TO_ZERO */, TD_DEFAULT );
			continue;
		// guisurf <guifile> | guisurf entity
		// an entity guisurf must have an anUserInterfaces
		// specified in the renderEntity
		} else if ( !token.Icmp( "guisurf" ) ) {
			src.ReadTokenOnLine( &token );
			if ( !token.Icmp( "entity" ) ) {
				entityGui = 1;
			} else if ( !token.Icmp( "entity2" ) ) {
				entityGui = 2;
			} else if ( !token.Icmp( "entity3" ) ) {
				entityGui = 3;
			} else {
				gui = uiManager->FindGui( token.c_str(), true );
			}
			continue;
		// sort
		} else if ( !token.Icmp( "sort" ) ) {
			ParseSort( src );
			continue;
		// spectrum <integer>
		} else if ( !token.Icmp( "spectrum" ) ) {
			src.ReadTokenOnLine( &token );
			spectrum = atoi( token.c_str() );
			continue;
		// deform < sprite | tube | flare >
		} else if ( !token.Icmp( "deform" ) ) {
			ParseDeform( src );
			continue;
		// decalInfo <staySeconds> <fadeSeconds> ( <start rgb> ) ( <end rgb> )
		} else if ( !token.Icmp( "decalInfo" ) ) {
			ParseDecalInfo( src );
			continue;
		// renderbump <args...>
		} else if ( !token.Icmp( "renderbump" ) ) {
			src.ParseRestOfLine( renderBump );
			continue;
		// diffusemap for stage shortcut
		} else if ( !token.Icmp( "diffusemap" ) ) {
			str = R_ParsePastImageProgram( src );
			anStr::snPrintf( buffer, sizeof( buffer ), "blend diffusemap\nmap %s\n}\n", str );
			newSrc.LoadMemory( buffer, strlen(buffer), "diffusemap" );
			newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
			ParseStage( newSrc, trpDefault );
			newSrc.FreeSource();
			continue;
		// specularmap for stage shortcut
		} else if ( !token.Icmp( "specularmap" ) ) {
			str = R_ParsePastImageProgram( src );
			anStr::snPrintf( buffer, sizeof( buffer ), "blend specularmap\nmap %s\n}\n", str );
			newSrc.LoadMemory( buffer, strlen(buffer), "specularmap" );
			newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
			ParseStage( newSrc, trpDefault );
			newSrc.FreeSource();
			continue;
		// normalmap for stage shortcut
		} else if ( !token.Icmp( "bumpmap" ) ) {
			str = R_ParsePastImageProgram( src );
			anStr::snPrintf( buffer, sizeof( buffer ), "blend bumpmap\nmap %s\n}\n", str );
			newSrc.LoadMemory( buffer, strlen(buffer), "bumpmap" );
			newSrc.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
			ParseStage( newSrc, trpDefault );
			newSrc.FreeSource();
			continue;
		// DECAL_MACRO for backwards compatibility with the preprocessor macros
		} else if ( !token.Icmp( "DECAL_MACRO" ) ) {
			// polygonOffset
			SetMaterialFlag( MF_POLYGONOFFSET );
			polygonOffset = 1;

			// discrete
			surfaceFlags |= SURF_DISCRETE;
			contentFlags &= ~CONTENTS_SOLID;

			// sort decal
			sort = SS_DECAL;

			// noShadows
			SetMaterialFlag( MF_NOSHADOWS );
			continue;
		} else if ( token == "{" ) {
			// create the new stage
			ParseStage( src, trpDefault );
			continue;
		} else {
			common->Warning( "unknown general material parameter '%s' in '%s'", token.c_str(), GetName() );
			SetMaterialFlag( MF_DEFAULTED );
			return;
		}
	}

	// add _flat or _white stages if needed
	AddImplicitStages();

	// order the diffuse / bump / specular stages properly
	SortInteractionStages();

	// if we need to do anything with normals (lighting or environment mapping)
	// and two sided lighting was asked for, flag
	// shouldCreateBackSides() and change culling back to single sided,
	// so we get proper tangent vectors on both sides

	// we can't just call ReceivesLighting(), because the stages are still
	// in temporary form
	if ( cullType == CT_TWO_SIDED ) {
		for ( int i = 0; i < numStages; i++ ) {
			if ( pd->parseStages[i].lighting != SL_AMBIENT || pd->parseStages[i].texture.texgen != TG_EXPLICIT ) {
				if ( cullType == CT_TWO_SIDED ) {
					cullType = CT_FRONT_SIDED;
					shouldCreateBackSides = true;
				}
				break;
			}
		}
	}

	// currently a surface can only have one unique texgen for all the stages on old hardware
	texGen_t firstGen = TG_EXPLICIT;
	for ( int i = 0; i < numStages; i++ ) {
		if ( pd->parseStages[i].texture.texgen != TG_EXPLICIT ) {
			if ( firstGen == TG_EXPLICIT ) {
				firstGen = pd->parseStages[i].texture.texgen;
			} else if ( firstGen != pd->parseStages[i].texture.texgen ) {
				common->Warning( "material '%s' has multiple stages with a texgen", GetName() );
				break;
			}
		}
	}
}

/*
=========================
anMaterial::SetGui
=========================
*/
void anMaterial::SetGui( const char *_gui ) const {
	gui = uiManager->FindGui( _gui, true, false, true );
}

/*
=========================
anMaterial::Parse

Parses the current material definition and finds all necessary images.
=========================
*/
bool anMaterial::Parse( const char *text, const int textLength ) {
	anLexer	src;
	anToken	token;
	mtrParsingData_t parsingData;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	// reset to the unparsed state
	CommonInit();

	memset( &parsingData, 0, sizeof( parsingData ) );

	pd = &parsingData;	// this is only valid during parse

	// parse it
	ParseMaterial( src );

	// if we are doing an fs_copyfiles, also reference the editorImage
	if ( cvarSystem->GetCVarInteger( "fs_copyFiles" ) ) {
		GetEditorImage();
	}

	//
	// count non-lit stages
	numAmbientStages = 0;

	for ( int i = 0; i < numStages; i++ ) {
		if ( pd->parseStages[i].lighting == SL_AMBIENT ) {
			numAmbientStages++;
		}
	}

	// see if there is a subview stage
	if ( sort == SS_SUBVIEW ) {
		hasSubview = true;
	} else {
		hasSubview = false;
		for ( int i = 0; i < numStages; i++ ) {
			if ( pd->parseStages[i].texture.dynamic ) {
				hasSubview = true;
			}
		}
	}

	// automatically determine coverage if not explicitly set
	if ( coverage == MC_BAD ) {
		// automatically set MC_TRANSLUCENT if we don't have any interaction stages and
		// the first stage is blended and not an alpha test mask or a subview
		if ( !numStages ) {
			// non-visible
			coverage = MC_TRANSLUCENT;
		} else if ( numStages != numAmbientStages ) {
			// we have an interaction draw
			coverage = MC_OPAQUE;
		} else if (
			( pd->parseStages[0].drawStateBits & GLS_DSTBLEND_BITS ) != GLS_DSTBLEND_ZERO ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_COLOR ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_ONE_MINUS_DST_COLOR ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_DST_ALPHA ||
			( pd->parseStages[0].drawStateBits & GLS_SRCBLEND_BITS ) == GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
			) {
			// blended with the destination
				coverage = MC_TRANSLUCENT;
		} else {
			coverage = MC_OPAQUE;
		}
	}

	// translucent automatically implies noshadows
	if ( coverage == MC_TRANSLUCENT ) {
		SetMaterialFlag( MF_NOSHADOWS );
	} else {
		// mark the contents as opaque
		contentFlags |= CONTENTS_OPAQUE;
	}

	// if we are translucent, draw with an alpha in the editor
	if ( coverage == MC_TRANSLUCENT ) {
		editorAlpha = 0.5;
	} else {
		editorAlpha = 1.0;
	}

	// the sorts can make reasonable defaults
	if ( sort == SS_BAD ) {
		if ( TestMaterialFlag(MF_POLYGONOFFSET) ) {
			sort = SS_DECAL;
		} else if ( coverage == MC_TRANSLUCENT ) {
			sort = SS_MEDIUM;
		} else {
			sort = SS_OPAQUE;
		}
	}

	// anything that references _currentRender will automatically get sort = SS_POST_PROCESS
	// and coverage = MC_TRANSLUCENT
	for ( int i = 0; i < numStages; i++ ) {
		materialStage_t	*pStage = &pd->parseStages[i];
		if ( pStage->texture.image == globalImages->currentRenderImage ) {
			if ( sort != SS_PORTAL_SKY ) {
				sort = SS_POST_PROCESS;
				coverage = MC_TRANSLUCENT;
			}
			break;
		}
		if ( pStage->newStage ) {
			for ( int j = 0; j < pStage->newStage->numFragmentProgramImages; j++ ) {
				if ( pStage->newStage->fragmentProgramImages[j] == globalImages->currentRenderImage ) {
					if ( sort != SS_PORTAL_SKY ) {
						sort = SS_POST_PROCESS;
						coverage = MC_TRANSLUCENT;
					}
					i = numStages;
					break;
				}
			}
		}
	}

	// set the drawStateBits depth flags
	for ( i = 0; i < numStages; i++ ) {
		materialStage_t	*pStage = &pd->parseStages[i];
		if ( sort == SS_POST_PROCESS ) {
			// post-process effects fill the depth buffer as they draw, so only the
			// topmost post-process effect is rendered
			pStage->drawStateBits |= GLS_DEPTHFUNC_LESS;
		} else if ( coverage == MC_TRANSLUCENT || pStage->ignoreAlphaTest ) {
			// translucent surfaces can extend past the exactly marked depth buffer
			pStage->drawStateBits |= GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK;
		} else {
			// opaque and perforated surfaces must exactly match the depth buffer,
			// which gets alpha test correct
			pStage->drawStateBits |= GLS_DEPTHFUNC_EQUAL | GLS_DEPTHMASK;
		}
	}

	// determine if this surface will accept overlays / decals

	if ( pd->forceOverlays ) {
		// explicitly flaged in material definition
		allowOverlays = true;
	} else {
		if ( !IsDrawn() ) {
			allowOverlays = false;
		}
		if ( Coverage() != MC_OPAQUE ) {
			allowOverlays = false;
		}
		if ( GetSurfaceFlags() & SURF_NOIMPACT ) {
			allowOverlays = false;
		}
	}

	// add a tiny offset to the sort orders, so that different materials
	// that have the same sort value will at least sort consistantly, instead
	// of flickering back and forth
/* this messed up in-game guis
	if ( sort != SS_SUBVIEW ) {
		int	hash, l;

		l = name.Length();
		hash = 0;
		for ( int i = 0; i < l; i++ ) {
			hash ^= name[i];
		}
		sort += hash * 0.01;
	}
*/

	if ( numStages ) {
		stages = (materialStage_t *)R_StaticAlloc( numStages * sizeof( stages[0] ) );
		memcpy( stages, pd->parseStages, numStages * sizeof( stages[0] ) );
	}

	if ( numOps ) {
		ops = (expOp_t *)R_StaticAlloc( numOps * sizeof( ops[0] ) );
		memcpy( ops, pd->shaderOps, numOps * sizeof( ops[0] ) );
	}

	if ( numRegisters ) {
		expressionRegisters = (float *)R_StaticAlloc( numRegisters * sizeof( expressionRegisters[0] ) );
		memcpy( expressionRegisters, pd->shaderRegisters, numRegisters * sizeof( expressionRegisters[0] ) );
	}

	// see if the registers are completely constant, and don't need to be evaluated
	// per-surface
	CheckForConstantRegisters();

	pd = nullptr;	// the pointer will be invalid after exiting this function

	// finish things up
	if ( TestMaterialFlag( MF_DEFAULTED ) ) {
		MakeDefault();
		return false;
	}
	return true;
}

/*
===================
anMaterial::Print
===================
*/
char *opNames[] = {
	"OP_TYPE_ADD",
	"OP_TYPE_SUBTRACT",
	"OP_TYPE_MULTIPLY",
	"OP_TYPE_DIVIDE",
	"OP_TYPE_MOD",
	"OP_TYPE_TABLE",
	"OP_TYPE_GT",
	"OP_TYPE_GE",
	"OP_TYPE_LT",
	"OP_TYPE_LE",
	"OP_TYPE_EQ",
	"OP_TYPE_NE",
	"OP_TYPE_AND",
	"OP_TYPE_OR"
};

void anMaterial::Print() const {
	for ( int i = EXP_REG_NUM_PREDEFINED; i < GetNumRegisters(); i++ ) {
		common->Printf( "register %i: %f\n", i, expressionRegisters[i] );
	}
	common->Printf( "\n" );
	for ( int i = 0; i < numOps; i++ ) {
		const expOp_t *op = &ops[i];
		if ( op->opType == OP_TYPE_TABLE ) {
			common->Printf( "%i = %s[ %i ]\n", op->c, declManager->DeclByIndex( DECL_TABLE, op->a )->GetName(), op->b );
		} else {
			common->Printf( "%i = %i %s %i\n", op->c, op->a, opNames[ op->opType ], op->b );
		}
	}
}

/*
===============
anMaterial::Save
===============
*/
bool anMaterial::Save( const char *fileName ) {
	return ReplaceSourceFileText();
}

/*
===============
anMaterial::AddReference
===============
*/
void anMaterial::AddReference() {
	refCount++;

	for ( int i = 0; i < numStages; i++ ) {
		materialStage_t *s = &stages[i];
		if ( s->texture.image ) {
			s->texture.image->AddReference();
		}
	}
}

/*
===============
anMaterial::EvaluateRegisters

Parameters are taken from the localSpace and the renderView,
then all expressions are evaluated, leaving the material registers
set to their apropriate values.
===============
*/
void anMaterial::EvaluateRegisters( float *registers, const float shaderParms[MAX_ENTITY_SHADER_PARMS], const viewDef_t *view, anSoundEmitter *soundEmitter ) const {
	// copy the material constants
	for ( int i = EXP_REG_NUM_PREDEFINED; i < numRegisters; i++ ) {
		registers[i] = expressionRegisters[i];
	}

	// copy the local and global parameters
	registers[EXP_REG_TIME] = view->floatTime;
	registers[EXP_REG_PARM0] = shaderParms[0];
	registers[EXP_REG_PARM1] = shaderParms[1];
	registers[EXP_REG_PARM2] = shaderParms[2];
	registers[EXP_REG_PARM3] = shaderParms[3];
	registers[EXP_REG_PARM4] = shaderParms[4];
	registers[EXP_REG_PARM5] = shaderParms[5];
	registers[EXP_REG_PARM6] = shaderParms[6];
	registers[EXP_REG_PARM7] = shaderParms[7];
	registers[EXP_REG_PARM8] = shaderParms[8];
	registers[EXP_REG_PARM9] = shaderParms[9];
	registers[EXP_REG_PARM10] = shaderParms[10];
	registers[EXP_REG_PARM11] = shaderParms[11];
	registers[EXP_REG_GLOBAL0] = view->renderView.shaderParms[0];
	registers[EXP_REG_GLOBAL1] = view->renderView.shaderParms[1];
	registers[EXP_REG_GLOBAL2] = view->renderView.shaderParms[2];
	registers[EXP_REG_GLOBAL3] = view->renderView.shaderParms[3];
	registers[EXP_REG_GLOBAL4] = view->renderView.shaderParms[4];
	registers[EXP_REG_GLOBAL5] = view->renderView.shaderParms[5];
	registers[EXP_REG_GLOBAL6] = view->renderView.shaderParms[6];
	registers[EXP_REG_GLOBAL7] = view->renderView.shaderParms[7];

	expOp_t	*op = ops;
	for ( int i = 0; i < numOps; i++, op++ ) {
		switch ( op->opType ) {
		case OP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case OP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case OP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case OP_TYPE_DIVIDE:
			registers[op->c] = registers[op->a] / registers[op->b];
			break;
		case OP_TYPE_MOD:
			int b = ( int )registers[op->b];
			b = b != 0 ? b : 1;
			registers[op->c] = ( int )registers[op->a] % b;
			break;
		case OP_TYPE_TABLE: {
				const anDeclTable *table = static_cast<const anDeclTable *>( declManager->DeclByIndex( DECL_TABLE, op->a ) );
				registers[op->c] = table->TableLookup( registers[op->b] );
			}
			break;
		case OP_TYPE_SOUND:
			if ( soundEmitter ) {
				registers[op->c] = soundEmitter->CurrentAmplitude();
			} else {
				registers[op->c] = 0;
			}
			break;
		case OP_TYPE_GT:
			registers[op->c] = registers[ op->a ] > registers[op->b];
			break;
		case OP_TYPE_GE:
			registers[op->c] = registers[ op->a ] >= registers[op->b];
			break;
		case OP_TYPE_LT:
			registers[op->c] = registers[ op->a ] < registers[op->b];
			break;
		case OP_TYPE_LE:
			registers[op->c] = registers[ op->a ] <= registers[op->b];
			break;
		case OP_TYPE_EQ:
			registers[op->c] = registers[ op->a ] == registers[op->b];
			break;
		case OP_TYPE_NE:
			registers[op->c] = registers[ op->a ] != registers[op->b];
			break;
		case OP_TYPE_AND:
			registers[op->c] = registers[ op->a ] && registers[op->b];
			break;
		case OP_TYPE_OR:
			registers[op->c] = registers[ op->a ] || registers[op->b];
			break;
		default:
			common->FatalError( "R_EvaluateExpression: bad opcode" );
		}
	}

}

/*
=============
anMaterial::Texgen
=============
*/
texGen_t anMaterial::Texgen() const {
	if ( stages ) {
		for ( int i = 0; i < numStages; i++ ) {
			if ( stages[i].texture.texgen != TG_EXPLICIT ) {
				return stages[i].texture.texgen;
			}
		}
	}

	return TG_EXPLICIT;
}

/*
=============
anMaterial::GetImageWidth
=============
*/
int anMaterial::GetImageWidth( void ) const {
	assert( GetStage( 0 ) && GetStage( 0 )->texture.image );
	return GetStage( 0 )->texture.image->uploadWidth;
}

/*
=============
anMaterial::GetImageHeight
=============
*/
int anMaterial::GetImageHeight( void ) const {
	assert( GetStage( 0 ) && GetStage( 0 )->texture.image );
	return GetStage( 0 )->texture.image->uploadHeight;
}

/*
=============
anMaterial::CinematicLength
=============
*/
int	anMaterial::CinematicLength() const {
	if ( !stages || !stages[0].texture.cinematic ) {
		return 0;
	}
	return stages[0].texture.cinematic->AnimationLength();
}

/*
=============
anMaterial::UpdateCinematic
=============
*/
void anMaterial::UpdateCinematic( int time ) const {
	if ( !stages || !stages[0].texture.cinematic || !backEnd.viewDef ) {
		return;
	}
	stages[0].texture.cinematic->ImageForTime( tr.primaryRenderView.time );
}

/*
=============
anMaterial::CloseCinematic
=============
*/
void anMaterial::CloseCinematic( void ) const {
	for ( int i = 0; i < numStages; i++ ) {
		if ( stages[i].texture.cinematic ) {
			stages[i].texture.cinematic->Close();
			delete stages[i].texture.cinematic;
			stages[i].texture.cinematic = nullptr;
		}
	}
}

/*
=============
anMaterial::ResetCinematicTime
=============
*/
void anMaterial::ResetCinematicTime( int time ) const {
	for ( int i = 0; i < numStages; i++ ) {
		if ( stages[i].texture.cinematic ) {
			stages[i].texture.cinematic->ResetTime( time );
		}
	}
}

/*
=============
anMaterial::ConstantRegisters
=============
*/
const float *anMaterial::ConstantRegisters() const {
	if ( !r_useConstantMaterials.GetBool() ) {
		return nullptr;
	}
	return constantRegisters;
}

/*
==================
anMaterial::CheckForConstantRegisters

As of 5/2/03, about half of the unique materials loaded on typical
maps are constant, but 2/3 of the surface references are.
This is probably an optimization of dubious value.
==================
*/
static int	c_constant, c_variable;
void anMaterial::CheckForConstantRegisters() {
	if ( !pd->registersAreConstant ) {
		return;
	}

	// evaluate the registers once, and save them
	constantRegisters = (float *)R_ClearedStaticAlloc( GetNumRegisters() * sizeof( float ) );

	float shaderParms[MAX_ENTITY_SHADER_PARMS];
	memset( shaderParms, 0, sizeof( shaderParms ) );
	viewDef_t	viewDef;
	memset( &viewDef, 0, sizeof( viewDef ) );

	EvaluateRegisters( constantRegisters, shaderParms, &viewDef, 0 );
}

/*
===================
anMaterial::ImageName
===================
*/
const char *anMaterial::ImageName( void ) const {
	if ( numStages == 0 ) {
		return "_scratch";
	}
	anImage *image = stages[0].texture.image;
	if ( image ) {
		return image->imgName;
	}
	return "_scratch";
}

/*
===================
anMaterial::SetImageClassifications

Just for image resource tracking.
===================
*/
void anMaterial::SetImageClassifications( int tag ) const {
	for ( int i = 0; i < numStages; i++ ) {
		anImage	*image = stages[i].texture.image;
		if ( image ) {
			image->SetClassification( tag );
		}
	}
}

/*
=================
anMaterial::Size
=================
*/
size_t anMaterial::Size( void ) const {
	return sizeof( anMaterial );
}

/*
===================
anMaterial::SetDefaultText
===================
*/
bool anMaterial::SetDefaultText( void ) {
	// if there exists an image with the same name
	if ( 1 ) { //fileSystem->ReadFile( GetName(), nullptr ) != -1 ) {
		char generated[2048];
		anStr::snPrintf( generated, sizeof( generated ),
		"// IMPLICITLY GENERATED\n"
		"material %s // add your .mtl to this obj's .def y\n"
		"{\n"
		"{\n"
		"blend blend // texture blend modes, defaulted as is\n"
		"colored  // vertex coloring\n"
		"map \"%s// texture via relative path\\n"
		"clampto // desired clamp/tiling modes\n"
		"}\n"
		"} // EXPLICITLY ENDED\n", GetName(), GetName() );
		SetText( generated );
		return true;
	} else {
		return false;
	}
}

/*
===================
anMaterial::DefaultDefinition
===================
*/
const char *anMaterial::DefaultDefinition() const {
	return
		"{\n"
	"\t"	"{\n"
	"\t\t"		"blend\tblend\n"
	"\t\t"		"map\t\t_default\n"
	"\t"	"}\n"
		"}";
}

/*
===================
anMaterial::GetBumpStage
===================
*/
const materialStage_t *anMaterial::GetBumpStage( void ) const {
	for ( int i = 0; i < numStages; i++ ) {
		if ( stages[i].lighting == SL_BUMP ) {
			return &stages[i];
		}
	}
	return nullptr;
}

/*
===================
anMaterial::ReloadImages
===================
*/
void anMaterial::ReloadImages( bool force ) const {
	for ( int i = 0; i < numStages; i++ ) {
		if ( stages[i].newStage ) {
			for ( int j = 0; j < stages[i].newStage->numFragmentProgramImages; j++ ) {
				if ( stages[i].newStage->fragmentProgramImages[j] ) {
					stages[i].newStage->fragmentProgramImages[j]->Reload( false, force );
				}
			}
		} else if ( stages[i].texture.image ) {
			stages[i].texture.image->Reload( false, force );
		}
	}
}
