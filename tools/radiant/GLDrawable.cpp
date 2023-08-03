#include "GLDrawable.h"
#include "../../renderer/tr_local.h"
#include "../../renderer/ImmediateMode.h"

extern bool Sys_KeyDown( int key );

static int viewAngle = -98;

#ifdef __linux__
//FIXME(johl): currently only implemented on Windows,, change interface of GLDrawable to get rid of these?
#define VK_MENU 0
#define VK_SHIFT 0

bool Sys_KeyDown( int ) {
	return false;
}
#endif

idGLDrawable::idGLDrawable() {
	scale = 1.0;
	xOffset = 0.0;
	yOffset = 0.0;
	handleMove = false;
	realTime = 0;
}

void idGLDrawable::buttonDown( MouseButton _button, float x, float y ) {
	pressX = x;
	pressY = y;
	button = _button;
	if (button == MouseButton::Right) {
		handleMove = true;
	}
}

void idGLDrawable::buttonUp( MouseButton button, float x, float y ) {
	handleMove = false;
}

void idGLDrawable::mouseMove( float x, float y ) {
	if (handleMove) {
		Update();
		if (Sys_KeyDown( VK_MENU ) ) {
			// scale
			float *px = &x;
			float *px2 = &pressX;

			if (anMath::Diff(y, pressY) > anMath::Diff(x, pressX) ) {
				px = &y;
				px2 = &pressY;
			}

			if (*px > *px2) {
				// zoom in
				scale += 0.1f;
				if (scale > 10.0f) {
					scale = 10.0f;
				}
			} else if (*px < *px2) {
				// zoom out
				scale -= 0.1f;
				if (scale <= 0.001f) {
					scale = 0.001f;
				}
			}

			*px2 = *px;
			//::SetCursorPos(pressX, pressY);

		} else if (Sys_KeyDown( VK_SHIFT ) ) {
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

void idGLDrawable::draw( int x, int y, int w, int h ) {
	GL_State( GLS_DEFAULT );
	glViewport( x, y, w, h );
	glScissor( x, y, w, h );

	float clearColor[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	glClearBufferfv( GL_COLOR, 0, clearColor );

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	globalImages->BindNull();
	fhImmediateMode im;
	im.Color3f( 1, 1, 1 );

	im.Begin( GL_LINE_LOOP );
	im.Color3f( 1, 0, 0 );
	im.Vertex2f( x + 3, y + 3 );
	im.Color3f( 0, 1, 0 );
	im.Vertex2f( x + 3, h - 3 );
	im.Color3f( 0, 0, 1 );
	im.Vertex2f( w - 3, h - 3 );
	im.Color3f( 1, 1, 1 );
	im.Vertex2f( w - 3, y + 3 );
	im.End();
}

idGLDrawableWorld::idGLDrawableWorld() {
	world = nullptr;
	worldModel = nullptr;
	InitWorld();
}

idGLDrawableWorld::~idGLDrawableWorld() {
	delete world;
}

void idGLDrawableWorld::AddTris( srfTriangles_t *tris, const anMaterial *mat ) {
	modelSurface_t	surf;
	surf.geometry = tris;
	surf.shader = mat;
	worldModel->AddSurface( surf );
}

void idGLDrawableWorld::draw( int x, int y, int w, int h ) {
}

void idGLDrawableWorld::InitWorld() {
	if (world == nullptr ) {
		world = renderSystem->AllocRenderWorld();
	}
	if (worldModel == nullptr ) {
		worldModel = renderModelManager->AllocModel();
	}
	world->InitFromMap( nullptr );
	worldModel->InitEmpty( va( "GLWorldModel_%i", Sys_Milliseconds() ) );
}

void idGLDrawableWorld::InitLight( const anVec3& position ) {
	renderLight_t	parms;
	anDict spawnArgs;
	spawnArgs.Set( "classname", "light" );
	spawnArgs.Set( "name", "light_1" );
	spawnArgs.SetVector( "origin", position );
	anString str;
	sprintf( str, "%f %f %f", light, light, light );
	spawnArgs.Set( "_color", str );
	engineEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &parms );
	lightDef = world->AddLightDef( &parms );
}

void idGLDrawableWorld::mouseMove( float x, float y ) {
	if (handleMove) {
		Update();
		if (button == MouseButton::Left) {
			float cury = ( float )(2 * x - rect.z) / rect.z;
			float curx = ( float )(2 * y - rect.w) / rect.w;
			anVec3 to( -curx, -cury, 0.0f );
			to.ProjectSelfOntoSphere( radius );
			lastPress.ProjectSelfOntoSphere( radius );
			anVec3 axis;
			axis.Cross( to, lastPress );
			float len = (lastPress - to).Length() / (2.0f * radius);
			len = anMath::ClampFloat( -1.0f, 1.0f, len );
			float phi = 2.0f * asin( len );

			axis.Normalize();
			axis *= sin( phi / 2.0f );
			anQuats rot( axis.z, axis.y, axis.x, cos( phi / 2.0f ) );
			rot.Normalize();

			rotation *= rot;
			rotation.Normalize();

			lastPress = to;
			lastPress.z = 0.0f;
		} else {
			bool doScale = Sys_KeyDown( VK_MENU );
			bool doLight = Sys_KeyDown( VK_SHIFT );
			if (doLight) {
				// scale
				float *px = &x;
				float *px2 = &pressX;

				if (anMath::Diff(y, pressY) > anMath::Diff(x, pressX) ) {
					px = &y;
					px2 = &pressY;
				}

				if (*px > *px2) {
					light += 0.05f;
					if (light > 2.0f) {
						light = 2.0f;
					}
				} else if (*px < *px2) {
					light -= 0.05f;
					if (light < 0.0f) {
						light = 0.0f;
					}
				}
				*px2 = *px;
				//::SetCursorPos( pressX, pressY );
			} else {
				// origin
				if (x != pressX) {
					if (doScale) {
						zOffset += (x - pressX);
						scale = Min( 10.0f, scale + (x - pressX)*0.01f );
					}
					else {
						xOffset += (x - pressX);
					}
					pressX = x;
				}
				if (y != pressY) {
					if (doScale) {
						zOffset -= (y - pressY);
						scale = Max( 0.001f, scale - (y - pressY)*0.01f );
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
void idGLDrawableWorld::buttonDown( MouseButton button, float x, float y ) {
	pressX = x;
	pressY = y;

	lastPress.y = -( float )(2 * x - rect.z) / rect.z;
	lastPress.x = -( float )(2 * y - rect.w) / rect.w;
	lastPress.z = 0.0f;
	this->button = button;
	if (button == MouseButton::Right || button == MouseButton::Left) {
		handleMove = true;
	}
}

void idGLDrawableMaterial::draw( int x, int y, int w, int h ) {
	const anMaterial *mat = material;
	if (mat) {
		glViewport( x, y, w, h );
		glScissor( x, y, w, h );

		float clearColor[] = { 0.1f, 0.1f, 0.1f, 0.0f };
		glClearBufferfv( GL_COLOR, 0, clearColor );

		if (worldDirty) {
			InitWorld();
			InitLight( anVec3( 0, 0, 0 ) );

			anImage *img = (mat->GetNumStages() > 0 ) ? mat->GetStage( 0 )->texture.image : mat->GetEditorImage();

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

			tris->verts[0].normal = tris->verts[1].xyz.Cross( tris->verts[3].xyz );
			tris->verts[1].normal = tris->verts[2].normal = tris->verts[3].normal = tris->verts[0].normal;
			AddTris( tris, mat );

			worldModel->FinishSurfaces();

			renderEntity_t worldEntity;

			memset( &worldEntity, 0, sizeof( worldEntity ) );
			if (mat->HasGui() ) {
				worldEntity.gui[0] = mat->GlobalGui();
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
		renderSystem->BeginFrame( w, h );
		memset( &refdef, 0, sizeof( refdef ) );
		refdef.vieworg.Set( viewAngle, 0, 0 );

		refdef.viewAxis = anAngles( 0, 0, 0 ).ToMat3();
		refdef.shaderParms[0] = 1;
		refdef.shaderParms[1] = 1;
		refdef.shaderParms[2] = 1;
		refdef.shaderParms[3] = 1;

		refdef.width = SCREEN_WIDTH;
		refdef.height = SCREEN_HEIGHT;
		refdef.fov_x = 90;
		refdef.fov_y = 2 * atan( ( float )h / w ) * anMath::M_RAD2DEG;

		refdef.time = eventLoop->Milliseconds();

		world->RenderScene( &refdef );

		renderSystem->EndFrame();

		GL_ModelViewMatrix.LoadIdentity();
	}

}

void idGLDrawableMaterial::setMedia( const char *name ) {
	anImage *img = nullptr;
	if (name && *name) {
		material = declManager->FindMaterial( name );
		if (material) {
			const materialStage_t *stage = ( material->GetNumStages() > 0 ) ? material->GetStage( 0 ) : nullptr;
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

idGLDrawableModel::idGLDrawableModel( const char *name ) {
	worldModel = renderModelManager->FindModel( name );
	light = 1.0;
	worldDirty = true;
}

idGLDrawableModel::idGLDrawableModel() {
	worldModel = renderModelManager->DefaultModel();
	light = 1.0;
}

void idGLDrawableModel::setMedia( const char *name ) {
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

void idGLDrawableModel::draw( int x, int y, int w, int h ) {
	if ( !worldModel) {
		return;
	}
	if (worldModel->IsDynamicModel() != DM_STATIC) {
		//return;
	}

	rect.Set( x, y, w, h );

	glViewport( x, y, w, h );
	glScissor( x, y, w, h );

	float clearColor[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	glClearBufferfv( GL_COLOR, 0, clearColor );

	if (worldDirty) {
		//InitWorld();
		world->InitFromMap( nullptr );
		InitLight( anVec3( -128, 0, 0 ) );

		anDict spawnArgs;
		renderEntity_t worldEntity;
		memset( &worldEntity, 0, sizeof( worldEntity ) );
		spawnArgs.Clear();
		spawnArgs.Set( "classname", "func_static" );
		spawnArgs.Set( "name", spawnArgs.GetString( "model" ) );
		spawnArgs.Set( "origin", "0 0 0" );
		if (skinStr.Length() ) {
			spawnArgs.Set( "skin", skinStr );
		}
		engineEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &worldEntity );
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
	renderSystem->BeginFrame( w, h );
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.vieworg.Set( zOffset, xOffset, -yOffset );

	refdef.viewAxis = anAngles( 0, 0, 0 ).ToMat3();
	refdef.shaderParms[0] = 1;
	refdef.shaderParms[1] = 1;
	refdef.shaderParms[2] = 1;
	refdef.shaderParms[3] = 1;

	refdef.width = SCREEN_WIDTH;
	refdef.height = SCREEN_HEIGHT;
	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan( ( float )h / w ) * anMath::M_RAD2DEG;

	refdef.time = eventLoop->Milliseconds();

	world->RenderScene( &refdef );

	renderSystem->EndFrame();

	GL_ModelViewMatrix.LoadIdentity();
}