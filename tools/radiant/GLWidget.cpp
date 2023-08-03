#include "..//idlib/Lib.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "GLWidget.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// idGLWidget
class idMiniDrawVert {
public:
	anVec3 xyz;
	anVec2 st;
	idMiniDrawVert(float x, float y, float z, float s, float t) : xyz(x,y,z), st(s, t) {
	};
};

static idMiniDrawVert cubeData[] = {
	idMiniDrawVert(-1.0, -1.0, +1.0, 0.0, 0.0 ),
	idMiniDrawVert(+1.0, -1.0, +1.0, 1.0, 0.0 ),
	idMiniDrawVert(+1.0, +1.0, +1.0, 1.0, 1.0 ),
	idMiniDrawVert(-1.0, +1.0, +1.0, 0.0, 1.0 ),

	idMiniDrawVert(-1.0, -1.0, -1.0, 1.0, 0.0 ),
	idMiniDrawVert(-1.0, +1.0, +1.0, 1.0, 1.0 ),
	idMiniDrawVert(+1.0, +1.0, -1.0, 0.0, 1.0 ),
	idMiniDrawVert(+1.0, -1.0, -1.0, 0.0, 0.0 ),

	idMiniDrawVert(-1.0, +1.0, -1.0, 0.0, 1.0 ),
	idMiniDrawVert(-1.0, +1.0, +1.0, 0.0, 0.0 ),
	idMiniDrawVert(+1.0, +1.0, +1.0, 1.0, 0.0 ),
	idMiniDrawVert(+1.0, +1.0, -1.0, 1.0, 1.0 ),

	idMiniDrawVert(-1.0, -1.0, -1.0, 1.0, 1.0 ),
	idMiniDrawVert(+1.0, -1.0, -1.0, 0.0, 1.0 ),
	idMiniDrawVert(+1.0, -1.0, +1.0, 0.0, 0.0 ),
	idMiniDrawVert(-1.0, -1.0, +1.0, 1.0, 0.0 ),

	idMiniDrawVert(+1.0, -1.0, -1.0, 1.0, 0.0 ),
	idMiniDrawVert(+1.0, +1.0, -1.0, 1.0, 1.0 ),
	idMiniDrawVert(+1.0, +1.0, +1.0, 0.0, 1.0 ),
	idMiniDrawVert(+1.0, -1.0, +1.0, 0.0, 0.0 ),

	idMiniDrawVert(-1.0, -1.0, -1.0, 0.0, 0.0 ),
	idMiniDrawVert(-1.0, -1.0, +1.0, 1.0, 0.0 ),
	idMiniDrawVert(-1.0, +1.0, +1.0, 1.0, 1.0 ),
	idMiniDrawVert(-1.0, +1.0, -1.0, 0.0, 1.0 )
};

static int cubeSides = sizeof(cubeData) / sizeof(idMiniDrawVert);
static int numQuads = cubeSides / 4;

void glTexturedBox(anVec3 &point, float size, const anMaterial *mat) {
	qglTranslatef(point.x, point.y, point.z);
	for ( int i = 0; i < numQuads; i++ ) {
		qglBegin( GL_QUADS);
		for ( int j = 0; j < 4; j++ ) {
			anVec3 v = cubeData[i * 4 + j].xyz;
			v *= size;
			qglTexCoord2fv(cubeData[i * 4 + j].st.ToFloatPtr() );
			qglVertex3fv( v.ToFloatPtr() );
		}
		qglEnd();
	}
}

idGLWidget::idGLWidget() {
	initialized = false;
	drawable = nullptr;
}

idGLWidget::~idGLWidget() {
}

BEGIN_MESSAGE_MAP(idGLWidget, CWnd)
	//{{AFX_MSG_MAP(idGLWidget)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// idGLWidget message handlers
BOOL idGLWidget::PreCreateWindow(CREATESTRUCT& cs) {
	// TODO: Add your specialized code here and/or call the base class
	return CWnd::PreCreateWindow(cs);
}

BOOL idGLWidget::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)  {
	if (CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext) == -1 ) {
		return FALSE;
	}

	CDC *dc = GetDC();
	QEW_SetupPixelFormat(dc->m_hDC, false);
	ReleaseDC(dc);

	return TRUE;
}

void idGLWidget::OnPaint() {
	if ( !initialized) {
		CDC *dc = GetDC();
		QEW_SetupPixelFormat(dc->m_hDC, false);
		ReleaseDC(dc);
		initialized = true;
	}
	CPaintDC dc( this ); // device context for painting

	CRect rect;
	GetClientRect(rect);

	if ( !qwglMakeCurrent(dc.m_hDC, win32.hGLRC) ) {
	}

	qglViewport(0, 0, rect.Width(), rect.Height() );
	qglScissor(0, 0, rect.Width(), rect.Height() );
	qglMatrixMode( GL_PROJECTION);
	qglLoadIdentity();
	qglClearColor (0.4f, 0.4f, 0.4f, 0.7f);
	qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	qglDisable( GL_DEPTH_TEST);
	qglDisable( GL_BLEND);
	qglOrtho(0, rect.Width(), 0, rect.Height(), -256, 256);

	if (drawable) {
		drawable->draw( 1, 1, rect.Width()-1, rect.Height()-1 );
	} else {
		qglViewport(0, 0, rect.Width(), rect.Height() );
		qglScissor(0, 0, rect.Width(), rect.Height() );
		qglMatrixMode( GL_PROJECTION);
		qglLoadIdentity();
		qglClearColor (0.4f, 0.4f, 0.4f, 0.7f);
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	qwglSwapBuffers(dc);
	qglFlush();
	qwglMakeCurrent(win32.hDC, win32.hGLRC);
}

extern bool Sys_KeyDown( int key);

void idGLDrawable::buttonDown( int _button, float x, float y) {
	pressX = x;
	pressY = y;
	button = _button;
	if (button == MK_RBUTTON) {
		handleMove = true;
	}
}

void idGLDrawable::buttonUp( int button, float x, float y) {
	handleMove = false;
}

extern float fDiff(float f1, float f2);
void idGLDrawable::mouseMove(float x, float y) {
	if (handleMove) {
		Update();
		if (Sys_KeyDown(VK_MENU) ) {
			// scale
			float *px = &x;
			float *px2 = &pressX;

			if (fDiff(y, pressY) > fDiff(x, pressX) ) {
				px = &y;
				px2 = &pressY;
			}

			if (*px > *px2) {
				// zoom in
				scale += 0.1f;
				if ( scale > 10.0f ) {
					scale = 10.0f;
				}
			} else if (*px < *px2) {
				// zoom out
				scale -= 0.1f;
				if ( scale <= 0.001f ) {
					scale = 0.001f;
				}
			}

			*px2 = *px;
			::SetCursorPos(pressX, pressY);

		} else if (Sys_KeyDown(VK_SHIFT) ) {
			// rotate
		} else {
			// origin
			if (x != pressX) {
				xOffset += (x - pressX);
				pressX = x;
			}
			if (y != pressY) {
				yOffset -= (y - pressY);
				pressY = y;
			}
			//::SetCursorPos(pressX, pressY);
		}
	}
}

void idGLDrawable::draw( int x, int y, int w, int h) {
	GL_State( GLS_DEFAULT );
	qglViewport(x, y, w, h);
	qglScissor(x, y, w, h);

	qglMatrixMode( GL_PROJECTION);

	qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
	qglClear( GL_COLOR_BUFFER_BIT);

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
	qglLineWidth(0.5);
	qglColor3f( 1, 1, 1 );

	globalImages->BindNull();
	qglBegin( GL_LINE_LOOP );
	qglColor3f( 1, 0, 0 );
	qglVertex2f(x + 3, y + 3);
	qglColor3f(0, 1, 0 );
	qglVertex2f(x + 3, h - 3);
	qglColor3f(0, 0, 1 );
	qglVertex2f(w - 3, h - 3);
	qglColor3f( 1, 1, 1 );
	qglVertex2f(w - 3, y + 3);
	qglEnd();
}

static int viewAngle = -98;
void idGLDrawableMaterial::buttonDown( int button, float x, float y) {
	idGLDrawable::buttonDown(button, x, y);
	//viewAngle += (button == MK_LBUTTON) ? 15 : -15;
}

void idGLDrawableMaterial::mouseMove(float x, float y) {
	if (handleMove) {
		Update();
		bool doScale = Sys_KeyDown(VK_MENU);
		bool doLight = Sys_KeyDown(VK_SHIFT);
		if (doScale || doLight) {
			// scale
			float *px = &x;
			float *px2 = &pressX;

			if (fDiff(y, pressY) > fDiff(x, pressX) ) {
				px = &y;
				px2 = &pressY;
			}

			if (*px > *px2) {
				// zoom in
				if (doScale) {
					scale += 0.1f;
					if ( scale > 10.0f ) {
						scale = 10.0f;
					}
				} else {
					light += 0.05f;
					if ( light > 2.0f ) {
						light = 2.0f;
					}
				}
			} else if (*px < *px2) {
				// zoom out
				if (doScale) {
					scale -= 0.1f;
					if ( scale <= 0.001f ) {
						scale = 0.001f;
					}
				} else {
					light -= 0.05f;
					if ( light < 0.0f ) {
						light = 0.0f;
					}
				}
			}
			*px2 = *px;
			::SetCursorPos(pressX, pressY);
		} else {
			// origin
			if (x != pressX) {
				xOffset += (x - pressX);
				pressX = x;
			}
			if (y != pressY) {
				yOffset -= (y - pressY);
				pressY = y;
			}
			//::SetCursorPos(pressX, pressY);
		}
	}
}

void idGLDrawableMaterial::draw( int x, int y, int w, int h) {
	const anMaterial *mat = material;
	if (mat) {
		qglViewport(x, y, w, h);
		qglScissor(x, y, w, h);
		qglMatrixMode( GL_PROJECTION);
		qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
		qglClear( GL_COLOR_BUFFER_BIT);

		if (worldDirty) {
			InitWorld();
			renderLight_t	parms;
			anDict spawnArgs;
			spawnArgs.Set( "classname", "light" );
			spawnArgs.Set( "name", "light_1" );
			spawnArgs.Set( "origin", "0 0 0" );
			anString str;
			sprintf(str, "%f %f %f", light, light, light);
			spawnArgs.Set( "_color", str);
			engineEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &parms );
			lightDef = world->AddLightDef( &parms );

			anImage *img = (mat->GetNumStages() > 0 ) ? mat->GetStage(0 )->texture.image : mat->GetEditorImage();

			if (img == nullptr ) {
				common->Warning( "Unable to load image for preview for %s", mat->GetName() );
				return;
			}

			int width = img->uploadWidth;
			int height = img->uploadHeight;

			width *= scale;
			height *= scale;

			srfTriangles_t *tris = worldModel->AllocSurfaceTriangles( 4, 6 );
			tris->numVerts = 4;
			tris->numIndexes = 6;

			tris->indexes[0] = 0;
			tris->indexes[1] = 1;
			tris->indexes[2] = 2;
			tris->indexes[3] = 3;
			tris->indexes[4] = 1;
			tris->indexes[5] = 0;

			tris->verts[0].xyz.x = 64;
			tris->verts[0].xyz.y = -xOffset + 0 - width / 2;
			tris->verts[0].xyz.z = yOffset + 0 - height / 2;
			tris->verts[0].st.x = 1;
			tris->verts[0].st.y = 1;

			tris->verts[1].xyz.x = 64;
			tris->verts[1].xyz.y = -xOffset + width / 2;
			tris->verts[1].xyz.z = yOffset + height / 2;
			tris->verts[1].st.x = 0;
			tris->verts[1].st.y = 0;

			tris->verts[2].xyz.x = 64;
			tris->verts[2].xyz.y = -xOffset + 0 - width / 2;
			tris->verts[2].xyz.z = yOffset + height / 2;
			tris->verts[2].st.x = 1;
			tris->verts[2].st.y = 0;

			tris->verts[3].xyz.x = 64;
			tris->verts[3].xyz.y = -xOffset + width / 2;
			tris->verts[3].xyz.z = yOffset + 0 - height / 2;
			tris->verts[3].st.x = 0;
			tris->verts[3].st.y = 1;

			tris->verts[0].normal = tris->verts[1].xyz.Cross(tris->verts[3].xyz);
			tris->verts[1].normal = tris->verts[2].normal = tris->verts[3].normal = tris->verts[0].normal;
			AddTris(tris, mat);

			worldModel->FinishSurfaces();

			renderEntity_t worldEntity;

			memset( &worldEntity, 0, sizeof( worldEntity ) );
			if ( mat->HasGui() ) {
				worldEntity.gui[ 0 ] = mat->GlobalGui();
			}
			worldEntity.hModel = worldModel;
			worldEntity.axis = mat3_default;
			worldEntity.shaderParms[0] = 1;
			worldEntity.shaderParms[1] = 1;
			worldEntity.shaderParms[2] = 1;
			worldEntity.shaderParms[3] = 1;
			modelDef = world->AddEntityDef( &worldEntity );

			worldDirty = false;
		}

		renderView_t	refdef;
		// render it
		renderSystem->BeginFrame(w, h);
		memset( &refdef, 0, sizeof( refdef ) );
		refdef.vieworg.Set(viewAngle, 0, 0 );

		refdef.viewAxis = anAngles(0,0,0 ).ToMat3();
		refdef.shaderParms[0] = 1;
		refdef.shaderParms[1] = 1;
		refdef.shaderParms[2] = 1;
		refdef.shaderParms[3] = 1;

		refdef.width = SCREEN_WIDTH;
		refdef.height = SCREEN_HEIGHT;
		refdef.fov_x = 90;
		refdef.fov_y = 2 * atan(( float )h / w) * anMath::M_RAD2DEG;

		refdef.time = eventLoop->Milliseconds();

		world->RenderScene( &refdef );
		int frontEnd, backEnd;
		renderSystem->EndFrame( &frontEnd, &backEnd );

		qglMatrixMode( GL_MODELVIEW );
		qglLoadIdentity();
	}
}

void idGLDrawableMaterial::setMedia(const char *name) {
	anImage *img = nullptr;
	if (name && *name) {
		material = declManager->FindMaterial( name );
		if (material) {
			const materialStage_t *stage = (material->GetNumStages() > 0 ) ? material->GetStage(0 ) : nullptr;
			if (stage) {
				img = stage->texture.image;
			} else {
				img = material->GetEditorImage();
			}
		}
	} else {
		material = nullptr;
	}
	// set scale to get a good fit
	if (material && img) {
		float size = (img->uploadWidth > img->uploadHeight) ? img->uploadWidth : img->uploadHeight;
		// use 128 as base scale of 1.0
		scale = 128.0 / size;
	} else {
		scale = 1.0;
	}
	xOffset = 0.0;
	yOffset = 0.0;
	worldDirty = true;
}

idGLDrawableModel::idGLDrawableModel(const char *name) {
	worldModel = renderModelManager->FindModel( name );
	light = 1.0;
	worldDirty = true;
}

idGLDrawableModel::idGLDrawableModel() {
	worldModel = renderModelManager->DefaultModel();
	light = 1.0;
}

void idGLDrawableModel::setMedia(const char *name) {
	worldModel = renderModelManager->FindModel( name );
	worldDirty = true;
	xOffset = 0.0;
	yOffset = 0.0;
	zOffset = -128;
	rotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	radius = 2.6f;
	lastPress.Zero();
}

void idGLDrawableModel::SetSkin( const char *skin ) {
	skinStr = skin;
}

void idGLDrawableModel::buttonDown( int _button, float x, float y) {
	pressX = x;
	pressY = y;

	lastPress.y = -( float )( 2 * x - rect.z ) / rect.z;
	lastPress.x = -( float )( 2 * y - rect.w ) / rect.w;
	lastPress.z = 0.0f;
	button = _button;
	if (button == MK_RBUTTON || button == MK_LBUTTON) {
		handleMove = true;
	}
}

void idGLDrawableModel::mouseMove(float x, float y) {
	if (handleMove) {
		Update();
		if (button == MK_LBUTTON) {
			float cury = ( float )( 2 * x - rect.z ) / rect.z;
			float curx = ( float )( 2 * y - rect.w ) / rect.w;
			anVec3 to( -curx, -cury, 0.0f );
			to.ProjectSelfOntoSphere( radius );
			lastPress.ProjectSelfOntoSphere( radius );
			anVec3 axis;
			axis.Cross( to, lastPress );
			float len = ( lastPress - to ).Length() / ( 2.0f * radius );
			len = anMath::ClampFloat( -1.0f, 1.0f, len );
			float phi = 2.0f * asin ( len ) ;

			axis.Normalize();
			axis *= sin( phi / 2.0f );
			anQuats rot( axis.z, axis.y, axis.x, cos( phi / 2.0f ) );
			rot.Normalize();

			rotation *= rot;
			rotation.Normalize();

			lastPress = to;
			lastPress.z = 0.0f;
		} else {
			bool doScale = Sys_KeyDown(VK_MENU);
			bool doLight = Sys_KeyDown(VK_SHIFT);
			if (doLight) {
				// scale
				float *px = &x;
				float *px2 = &pressX;

				if (fDiff(y, pressY) > fDiff(x, pressX) ) {
					px = &y;
					px2 = &pressY;
				}

				if (*px > *px2) {
					light += 0.05f;
					if ( light > 2.0f ) {
						light = 2.0f;
					}
				} else if (*px < *px2) {
					light -= 0.05f;
					if ( light < 0.0f ) {
						light = 0.0f;
					}
				}
				*px2 = *px;
				::SetCursorPos(pressX, pressY);
			} else {
				// origin
				if (x != pressX) {
					if (doScale) {
						zOffset += (x - pressX);
					} else {
						xOffset += (x - pressX);
					}
					pressX = x;
				}
				if (y != pressY) {
					if (doScale) {
						zOffset -= (y - pressY);
					} else {
						yOffset -= (y - pressY);
					}
					pressY = y;
				}
				//::SetCursorPos(pressX, pressY);
			}
		}
	}
}

void idGLDrawableModel::draw( int x, int y, int w, int h) {
	if ( !worldModel ) {
		return;
	}
	if ( worldModel->IsDynamicModel() != DM_STATIC ) {
		//return;
	}

	rect.Set( x, y, w, h );

	qglViewport(x, y, w, h);
	qglScissor(x, y, w, h);
	qglMatrixMode( GL_PROJECTION);
	qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
	qglClear( GL_COLOR_BUFFER_BIT);

	if (worldDirty) {
		//InitWorld();
		world->InitFromMap( nullptr );
		renderLight_t	parms;
		anDict spawnArgs;
		spawnArgs.Set( "classname", "light" );
		spawnArgs.Set( "name", "light_1" );
		spawnArgs.Set( "origin", "-128 0 0" );
		anString str;
		sprintf(str, "%f %f %f", light, light, light);
		spawnArgs.Set( "_color", str);
		engineEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &parms );
		lightDef = world->AddLightDef( &parms );

		renderEntity_t worldEntity;
		memset( &worldEntity, 0, sizeof( worldEntity ) );
		spawnArgs.Clear();
		spawnArgs.Set( "classname", "func_static" );
		spawnArgs.Set( "name", spawnArgs.GetString( "model" ) );
		spawnArgs.Set( "origin", "0 0 0" );
		if ( skinStr.Length() ) {
			spawnArgs.Set( "skin", skinStr );
		}
		engineEdit->ParseSpawnArgsToRenderEntity(&spawnArgs, &worldEntity);
		worldEntity.hModel = worldModel;

		worldEntity.axis = rotation.ToMat3();

		worldEntity.shaderParms[0] = 1;
		worldEntity.shaderParms[1] = 1;
		worldEntity.shaderParms[2] = 1;
		worldEntity.shaderParms[3] = 1;
		modelDef = world->AddEntityDef( &worldEntity );

		worldDirty = false;
	}

	renderView_t	refdef;
	// render it
	renderSystem->BeginFrame(w, h);
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.vieworg.Set(zOffset, xOffset, -yOffset);

	refdef.viewAxis = anAngles(0,0,0 ).ToMat3();
	refdef.shaderParms[0] = 1;
	refdef.shaderParms[1] = 1;
	refdef.shaderParms[2] = 1;
	refdef.shaderParms[3] = 1;

	refdef.width = SCREEN_WIDTH;
	refdef.height = SCREEN_HEIGHT;
	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan(( float )h / w) * anMath::M_RAD2DEG;

	refdef.time = eventLoop->Milliseconds();

	world->RenderScene( &refdef );
	int frontEnd, backEnd;
	renderSystem->EndFrame( &frontEnd, &backEnd );

	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();
}

void idGLWidget::OnLButtonDown( UINT nFlags, CPoint point ) {
	SetCapture();
	if (drawable) {
		if ( drawable->ScreenCoords() ) {
			ClientToScreen( &point );
		}
		drawable->buttonDown(MK_LBUTTON, point.x, point.y);
	}
}

void idGLWidget::OnLButtonUp( UINT nFlags, CPoint point ) {
	if (drawable) {
		if ( drawable->ScreenCoords() ) {
			ClientToScreen( &point );
		}
		drawable->buttonUp(MK_LBUTTON, point.x, point.y);
	}
	ReleaseCapture();
}

void idGLWidget::OnMButtonDown( UINT nFlags, CPoint point ) {
	SetCapture();
	if (drawable) {
		if ( drawable->ScreenCoords() ) {
			ClientToScreen( &point );
		}
		drawable->buttonDown( MK_MBUTTON, point.x, point.y );
	}
}

void idGLWidget::OnMButtonUp( UINT nFlags, CPoint point ) {
	if (drawable) {
		if ( drawable->ScreenCoords() ) {
			ClientToScreen( &point );
		}
		drawable->buttonUp( MK_MBUTTON, point.x, point.y );
	}
	ReleaseCapture();
}

void idGLWidget::OnMouseMove( UINT nFlags, CPoint point ) {
	if (drawable) {
		if ( drawable->ScreenCoords() ) {
			ClientToScreen( &point );
		}
		drawable->mouseMove( point.x, point.y );
		RedrawWindow();
	}
}

BOOL idGLWidget::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
	if (drawable) {
		float f = drawable->getScale();
		if ( zDelta > 0.0f ) {
			f += 0.1f;
		} else {
			f -= 0.1f;
		}
		if ( f <= 0.0f ) {
			f = 0.1f;
		}
		if ( f > 5.0f ) {
			f = 5.0f;
		}
		drawable->setScale(f);
	}
	return TRUE;
}

void idGLWidget::OnRButtonDown( UINT nFlags, CPoint point ) {
	SetCapture();
	if (drawable) {
		if ( drawable->ScreenCoords() ) {
			ClientToScreen( &point );
		}
		drawable->buttonDown( MK_RBUTTON, point.x, point.y );
	}
}

void idGLWidget::OnRButtonUp( UINT nFlags, CPoint point ) {
	if (drawable) {
		if ( drawable->ScreenCoords() ) {
			ClientToScreen( &point );
		}
		drawable->buttonUp( MK_RBUTTON, point.x, point.y );
	}
	ReleaseCapture();
}

void idGLWidget::setDrawable(idGLDrawable *d) {
	drawable = d;
	if (d->getRealTime() ) {
		SetTimer( 1, d->getRealTime(), nullptr );
	}
}

void idGLWidget::OnTimer(UINT nIDEvent) {
	if (drawable && drawable->getRealTime() ) {
		Invalidate(FALSE);
	} else {
		KillTimer(1 );
	}
}

idGLDrawable::idGLDrawable() {
	scale = 1.0;
	xOffset = 0.0;
	yOffset = 0.0;
	handleMove = false;
	realTime = 0;
}

void idGLDrawableConsole::draw( int x, int y, int w, int h) {
	qglPushAttrib( GL_ALL_ATTRIB_BITS );
	qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
	qglScissor( 0, 0, w, h );
	qglClear( GL_COLOR_BUFFER_BIT );
	renderSystem->BeginFrame( w, h );

	console->Draw( true );

	renderSystem->EndFrame( nullptr, nullptr );
	qglPopAttrib();
}

void idGLConsoleWidget::init() {
	setDrawable(&console);
}

void idGLConsoleWidget::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
	sysEvent_t	ev;

	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_KEY;
	ev.evValue2 = 1;
	ev.evValue = nChar;

	::console->ProcessEvent( &ev, true );
}

BEGIN_MESSAGE_MAP(idGLConsoleWidget, idGLWidget)
	//{{AFX_MSG_MAP(idGLConsoleWidget)
	ON_WM_PAINT()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void idGLConsoleWidget::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) {
	sysEvent_t	ev;

	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_KEY;
	ev.evValue2 = 0;
	ev.evValue = nChar;

	::console->ProcessEvent( &ev, true );
}

void idGLConsoleWidget::OnPaint() {
	idGLWidget::OnPaint();
}

void idGLConsoleWidget::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
	sysEvent_t	ev;

	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_CHAR;
	ev.evValue = nChar;

	::console->ProcessEvent( &ev, true );
}

void idGLConsoleWidget::OnLButtonDown( UINT nFlags, CPoint point ) {
	SetFocus();
}

BOOL idGLWidget::OnEraseBkgnd(CDC* pDC) {
	return FALSE;
	//return CWnd::OnEraseBkgnd(pDC);
}

idGLDrawableWorld::idGLDrawableWorld() {
	world = nullptr;
	worldModel = nullptr;
	InitWorld();
}

idGLDrawableWorld::~idGLDrawableWorld() {
	delete world;
}

void idGLDrawableWorld::AddTris(srfTriangles_t *tris, const anMaterial *mat) {
	modelSurface_t	surf;
	surf.geometry = tris;
	surf.shader = mat;
	worldModel->AddSurface( surf );
}

void idGLDrawableWorld::draw( int x, int y, int w, int h) {
}

void idGLDrawableWorld::InitWorld() {
	if ( world == nullptr ) {
		world = renderSystem->AllocRenderWorld();
	}
	if ( worldModel == nullptr ) {
		worldModel = renderModelManager->AllocModel();
	}
	world->InitFromMap( nullptr );
	worldModel->InitEmpty( va( "GLWorldModel_%i", Sys_Milliseconds() ) );
}