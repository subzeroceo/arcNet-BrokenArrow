#pragma once

#include "MaterialView.h"
#include "../radiant/GLWidget.h"


class idGLDrawableView : public idGLDrawable {

public:
	idGLDrawableView();
	~idGLDrawableView();

	virtual void setMedia(const char *name);
	virtual void draw( int x, int y, int w, int h);
	virtual void buttonUp( int button){}
	virtual void buttonDown( int _button, float x, float y);
	virtual void mouseMove(float x, float y);
	virtual void Update() {};

	void UpdateCamera( renderView_t *refdef );
	void UpdateModel( void );
	void UpdateLights( void );

	void addLight( void );
	void deleteLight( const int lightId );
	void drawLights( renderView_t *refdef );

	void InitWorld();
	void ResetView( void );

	void setLightShader( const int lightId, const anString shaderName );
	void setLightColor( const int lightId, const anVec3 &value );
	void setLightRadius( const int lightId, const float radius );
	void setLightAllowMove( const int lightId, const bool move );
	void setObject( int Id );
	void setCustomModel( const anString modelName );
	void setShowLights( bool _showLights );
	void setLocalParm( int parmNum, float value );
	void setGlobalParm( int parmNum, float value );

protected:
	anRenderWorld			*world;
	anRenderModel			*worldModel;
	const anMaterial		*material;

	bool					showLights;

	anVec3					viewOrigin;
	anAngles				viewRotation;
	float					viewDistance;

	renderEntity_t			worldEntity;
	arcNetHandle_t			modelDefHandle;

	int						objectId;
	anString				customModelName;

	float					globalParms[MAX_TOTALSHADER_PARMS];

	typedef struct {
		renderLight_t		renderLight;
		arcNetHandle_t		lightDefHandle;
		anVec3				origin;
		const anMaterial	*shader;
		float				radius;
		anVec3				color;
		bool				allowMove;
	} lightInfo_t;

	anList<lightInfo_t>	viewLights;
};


// ==================================================================
// ==================================================================

class MaterialPreviewView : public CView, public MaterialView {
	DECLARE_DYNCREATE(MaterialPreviewView)

protected:
	MaterialPreviewView();
	virtual ~MaterialPreviewView();

public:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view

	void	MV_OnMaterialSelectionChange(MaterialDoc* pMaterial);

	void	OnModelChange( int modelId );
	void	OnCustomModelChange( anString modelName );
	void	OnShowLightsChange( bool showLights );

	void	OnLocalParmChange( int parmNum, float value );
	void	OnGlobalParmChange( int parmNum, float value );

	void	OnLightShaderChange( int lightId, anString shaderName );
	void	OnLightRadiusChange( int lightId, float radius );
	void	OnLightColorChange( int lightId, anVec3 &color );
	void	OnLightAllowMoveChange( int lightId, bool move );

	void	OnAddLight( void );
	void	OnDeleteLight( int lightId );

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	idGLWidget			renderWindow;
	idGLDrawableView	renderedView;

	anString	currentMaterial;

	DECLARE_MESSAGE_MAP()

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
