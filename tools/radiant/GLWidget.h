
#if !defined(AFX_IDGLWIDGET_H__6399A341_2976_4A6E_87DD_9AF4DBD4C5DB__INCLUDED_)
#define AFX_IDGLWIDGET_H__6399A341_2976_4A6E_87DD_9AF4DBD4C5DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// idGLWidget window

class idGLDrawable {
public:
	idGLDrawable();
	~idGLDrawable() {};
	virtual void draw( int x, int y, int w, int h);
	virtual void setMedia(const char *name){}
	virtual void buttonDown( int button, float x, float y);
	virtual void buttonUp( int button, float x, float y);
	virtual void mouseMove(float x, float y);
	virtual int getRealTime() {return realTime;};
	virtual bool ScreenCoords() {
		return true;
	}
	void SetRealTime( int i) {
		realTime = i;
	}
	virtual void Update() {};
	float getScale() {
		return scale;
	}
	void setScale(float f) {
		scale = f;
	}
protected:
	float scale;
	float xOffset;
	float yOffset;
	float zOffset;
	float pressX;
	float pressY;
	bool  handleMove;
	int button;
	int realTime;
};

class idGLDrawableWorld : public idGLDrawable {
public:
	idGLDrawableWorld();
	~idGLDrawableWorld();
	void AddTris(surfTriangles_t *tris, const arcMaterial *mat);
	virtual void draw( int x, int y, int w, int h);
	void InitWorld();
protected:
	ARCRenderWorld *world;
	ARCRenderModel *worldModel;
	arcNetHandle_t	worldModelDef;
	arcNetHandle_t	lightDef;
	arcNetHandle_t   modelDef;
};

class idGLDrawableMaterial : public idGLDrawableWorld {
public:

	idGLDrawableMaterial(const arcMaterial *mat) {
		material = mat;
		scale = 1.0;
		light = 1.0;
		worldDirty = true;
	}

	idGLDrawableMaterial() {
		material = NULL;
		light = 1.0;
		worldDirty = true;
		realTime = 50;
	}

	~idGLDrawableMaterial() {
	}

	virtual void setMedia(const char *name);
	virtual void draw( int x, int y, int w, int h);
	virtual void buttonUp( int button){}
	virtual void buttonDown( int button, float x, float y);
	virtual void mouseMove(float x, float y);
	virtual void Update() { worldDirty = true ;};

protected:
	const arcMaterial *material;
	bool worldDirty;
	float light;
};

class idGLDrawableModel : public idGLDrawableWorld {
public:

	idGLDrawableModel(const char *name);

	idGLDrawableModel();

	~idGLDrawableModel() {}

	virtual void setMedia(const char *name);

	virtual void buttonDown( int button, float x, float y);
	virtual void mouseMove(float x, float y);
	virtual void draw( int x, int y, int w, int h);
	virtual void Update() { worldDirty = true ;};
	virtual bool ScreenCoords() {
		return false;
	}
	void SetSkin( const char *skin );

protected:
	bool worldDirty;
	float light;
	arcNetString skinStr;
	arcQuats rotation;
	arcVec3 lastPress;
	float radius;
	arcVec4 rect;

};

class idGLDrawableConsole : public idGLDrawable {
public:

	idGLDrawableConsole () {
	}

	~idGLDrawableConsole() {
	}

	virtual void setMedia(const char *name) {
	}


	virtual void draw( int x, int y, int w, int h);

	virtual int getRealTime() {return 0;};

protected:

};

class idGLWidget : public CWnd {
// Construction
public:
	idGLWidget();
	void setDrawable(idGLDrawable *d);
// Attributes
public:
// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(idGLWidget)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~idGLWidget();

	// Generated message map functions
protected:
	idGLDrawable *drawable;
	bool initialized;
	//{{AFX_MSG(idGLWidget)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

class idGLConsoleWidget : public idGLWidget {
	idGLDrawableConsole console;
public:
	idGLConsoleWidget() {
	};
	~idGLConsoleWidget() {
	}
	void init();
protected:
	//{{AFX_MSG(idGLConsoleWidget)
	afx_msg void OnPaint();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IDGLWIDGET_H__6399A341_2976_4A6E_87DD_9AF4DBD4C5DB__INCLUDED_)
