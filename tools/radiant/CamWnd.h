#if !defined(AFX_CAMWND_H__44B4BA03_781B_11D1_B53C_00AA00A410FC__INCLUDED_)
#define AFX_CAMWND_H__44B4BA03_781B_11D1_B53C_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef enum {
	cd_wire,
	cd_solid,
	cd_texture,
	cd_light,
	cd_blend
} camera_draw_mode;

typedef struct {
	int			width, height;

	arcVec3		origin;
	arcAngles	angles;

	camera_draw_mode	draw_mode;

	arcVec3		color;			// background

	arcVec3		forward, right, up;	// move matrix
	arcVec3		vup, vpn, vright;	// view matrix
} camera_t;

// CCamWnd window
class CXYWnd;

class CCamWnd : public CWnd {
  DECLARE_DYNCREATE(CCamWnd);
// Construction
public:
	CCamWnd();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCamWnd)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ShiftTexture_BrushPrimit(face_t *f, int x, int y);
	CXYWnd* m_pXYFriend;
	void SetXYFriend(CXYWnd* pWnd);
	virtual ~CCamWnd();
	camera_t& Camera(){return m_Camera;};
	void Cam_MouseControl(float dtime);
	void Cam_ChangeFloor(bool up);
	void BuildRendererState();
	void ToggleRenderMode();
	void ToggleRebuildMode();
	void ToggleEntityMode();
	void ToggleSelectMode();
	void ToggleAnimationMode();
	void ToggleSoundMode();
	void SetProjectionMatrix();
	void UpdateCameraView();

	void BuildEntityRenderState( entity_t *ent, bool update );
	bool GetRenderMode() {
		return renderMode;
	}
	bool GetRebuildMode() {
		return rebuildMode;
	}
	bool GetEntityMode() {
		return entityMode;
	}
	bool GetAnimationMode() {
		return animationMode;
	}
	bool GetSelectMode() {
		return selectMode;
	}
	bool GetSoundMode() {
		return soundMode;
	}


	bool UpdateRenderEntities();
	void MarkWorldDirty();

	void SetView( const arcVec3 &origin, const arcAngles &angles ) {
		m_Camera.origin = origin;
		m_Camera.angles = angles;
	}

protected:
	void Cam_Init();
	void Cam_BuildMatrix();
	void Cam_PositionDrag();
	void Cam_MouseLook();
	void Cam_MouseDown( int x, int y, int buttons);
	void Cam_MouseUp ( int x, int y, int buttons);
	void Cam_MouseMoved ( int x, int y, int buttons);
	void InitCull();
	bool CullBrush (brush_t *b, bool cubicOnly);
	void Cam_Draw();
	void Cam_Render();

	// game renderer interaction
	arcNetHandle_t	worldModelDef;
	ARCRenderModel	*worldModel;		// createRawModel of the brush and patch geometry
	bool	worldDirty;
	bool	renderMode;
	bool	rebuildMode;
	bool	entityMode;
	bool	selectMode;
	bool	animationMode;
	bool	soundMode;
	void	FreeRendererState();
	void	UpdateCaption();
	bool	BuildBrushRenderData(brush_t *brush);
	void	DrawEntityData();

	camera_t m_Camera;
	int	m_nCambuttonstate;
	CPoint m_ptButton;
	CPoint m_ptCursor;
	CPoint m_ptLastCursor;
	face_t* m_pSide_select;
	arcVec3 m_vCull1;
	arcVec3 m_vCull2;
	int m_nCullv1[3];
	int m_nCullv2[3];
	bool m_bClipMode;
	arcVec3 saveOrg;
	arcAngles saveAng;
	bool saveValid;

	// Generated message map functions
protected:
	void OriginalMouseDown(UINT nFlags, CPoint point);
	void OriginalMouseUp(UINT nFlags, CPoint point);
	//{{AFX_MSG(CCamWnd)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAMWND_H__44B4BA03_781B_11D1_B53C_00AA00A410FC__INCLUDED_)