#ifndef __SELECT_H_
#define __SELECT_H_

typedef enum {
	sel_brush,
	// sel_sticky_brush,
	// sel_face,
	sel_vertex,
	sel_edge,
	sel_singlevertex,
	sel_curvepoint,
	sel_area,
	sel_addpoint,			// for dropping points
	sel_editpoint			// for editing points
} select_t;

class CDragPoint {
public:
	arcVec3 vec;
	brush_t *pBrush;
	int nType;
	bool priority;
	CDragPoint() {};
	CDragPoint(brush_t *b, arcVec3 v, int type, bool p) {
		pBrush = b;
		VectorCopy( v, ve );
		nType = type;
		priority = p;
	}

	void Set(brush_t *b, arcVec3 v, int type) {
		pBrush = b;
		VectorCopy( v, vec );
		nType = type;
	}

	bool PointWithin( arcVec3 p, int nView = -1 );
};


typedef struct {
	brush_t		*brush;
	face_t		*face;
	CDragPoint *point;
	float		dist;
	bool	selected;
} qertrace_t;


#define	SF_SELECTED_ONLY	 0x01
#define	SF_ENTITIES_FIRST	 0x02
#define	SF_SINGLEFACE		   0x04
#define SF_IGNORECURVES    0x08
#define SF_IGNOREGROUPS    0x10
#define SF_CYCLE           0x20


qertrace_t Test_Ray( const arcVec3 &origin, const arcVec3 &dir, int flags );
CDragPoint *PointRay( const arcVec3 &org, const arcVec3 &dir, float *dist);
void SelectCurvePointByRay( const arcVec3 &org, const arcVec3 &dir, int buttons);
void SelectSplinePointByRay( const arcVec3 &org, const arcVec3 &dir, int buttons);

void Select_GetBounds(arcVec3 &mins, arcVec3 &maxs);
void Select_Brush(brush_t *b, bool bComplete = true, bool bStatus = true);
void Select_Ray(arcVec3 origin, arcVec3 dir, int flags);
void Select_Delete( void );
void Select_Deselect(bool bDeselectFaces = true);
void Select_Invert( void );
void Select_Clone( void );
void Select_Move(arcVec3 delta, bool bSnap = true);
void WINAPI Select_SetTexture (texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale = false, void* pPlugTexdef = NULL, bool update = true);
void Select_FlipAxis( int axis);
void Select_RotateAxis( int axis, float deg, bool bPaint = true, bool bMouse = false);
void Select_CompleteTall( void );
void Select_PartialTall( void );
void Select_Touching( void );
void Select_Inside( void );
void Select_CenterOrigin();
void Select_AllOfType();
void Select_Reselect();
void Select_FitTexture(float height = 1.0, float width = 1.0 );
void Select_InitializeRotation();
void Select_FinalizeRotation();

// absolute texture coordinates
// TTimo NOTE: this is stuff for old brushes format and rotation texture lock .. sort of in-between with bush primitives
void ComputeAbsolute(face_t* f, arcVec3& p1, arcVec3& p2, arcVec3& p3);
void AbsoluteToLocal( const arcPlane &normal2, face_t* f, arcVec3& p1, arcVec3& p2, arcVec3& p3);
void Select_Hide(bool invert = false);
void Select_ShowAllHidden();
void Select_WireFrame( bool wireFrame );
void Select_ForceVisible( bool visible );
void Select_Name(const char *pName);
void Select_AddProjectedLight();
void Select_GetMid (arcVec3 &mid);
void Select_SetDefaultTexture(const arcMaterial *mat, bool fitScale, bool setTexture);
void Select_UpdateTextureName(const char *name);

void Select_FlipTexture(bool y);
void Select_SetPatchFit(float dim1, float dim2, float srcWidth, float srcHeight, float rot);
void Select_SetPatchST(float s1, float t1, float s2, float t2);
void Select_ProjectFaceOntoPatch( face_t *face );
void Select_CopyPatchTextureCoords( patchMesh_t *p );
void Select_AllTargets();
void Select_Scale(float x, float y, float z);
void Select_RotateTexture(float amt, bool absolute = false);
void Select_ScaleTexture(float x, float y, bool update = true, bool absolute = true);
void Select_DefaultTextureScale(bool horz, bool vert, bool update = true);
void Select_ShiftTexture(float x, float y, bool autoAdjust = false);
void Select_GetTrueMid (arcVec3 &mid);
void Select_Scale(float x, float y, float z);


#endif