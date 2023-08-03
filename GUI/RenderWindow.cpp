#include "../idlib/Lib.h"
#pragma hdrstop

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "RenderWindow.h"

idRenderWindow::idRenderWindow(idDeviceContext *d, anUserInterface *g) : idWindow(d, g) {
	dc = d;
	gui = g;
	CommonInit();
}

idRenderWindow::idRenderWindow(anUserInterface *g) : idWindow(g) {
	gui = g;
	CommonInit();
}

idRenderWindow::~idRenderWindow() {
	renderSystem->FreeRenderWorld( world ); 
}

void idRenderWindow::CommonInit() {
	world = renderSystem->AllocRenderWorld();
	needsRender = true;
	lightOrigin = anVec4(-128.0f, 0.0f, 0.0f, 1.0f);
	lightColor = anVec4(1.0f, 1.0f, 1.0f, 1.0f);
	modelOrigin.Zero();
	viewOffset = anVec4(-128.0f, 0.0f, 0.0f, 1.0f);
	modelAnim = nullptr;
	animLength = 0;
	animEndTime = -1;
	modelDef = -1;
	updateAnimation = true;
}


void idRenderWindow::BuildAnimation( inttime) {
	if ( !updateAnimation) {
		return;
	}

	if (animName.Length() && animClass.Length()) {
		worldEntity.numJoints = worldEntity.hModel->NumJoints();
		worldEntity.joints = ( idJointMat * )Mem_Alloc16( worldEntity.numJoints * sizeof( *worldEntity.joints ) );
		modelAnim = gameEdit->ANIM_GetAnimFromEntityDef(animClass, animName);
		if (modelAnim) {
			animLength = gameEdit->ANIM_GetLength(modelAnim);
			animEndTime = time + animLength;
		}
	}
	updateAnimation = false;
}

void idRenderWindow::PreRender() {
	if (needsRender) {
		world->InitFromMap( nullptr );
		anDict spawnArgs;
		spawnArgs.Set( "classname", "light" );
		spawnArgs.Set( "name", "light_1" );
		spawnArgs.Set( "origin", lightOrigin.ToVec3().ToString());
		spawnArgs.Set( "_color", lightColor.ToVec3().ToString());
		gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &rLight );
		lightDef = world->AddLightDef( &rLight );
		if ( !modelName[0] ) {
			common->Warning( "Window '%s' in gui '%s': no model set", GetName(), GetGui()->GetSourceFile() );
		}
		memset( &worldEntity, 0, sizeof( worldEntity ) );
		spawnArgs.Clear();
		spawnArgs.Set( "classname", "func_static" );
		spawnArgs.Set( "model", modelName);
		spawnArgs.Set( "origin", modelOrigin.c_str());
		gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &worldEntity );
		if ( worldEntity.hModel ) {
			anVec3 v = modelRotate.ToVec3();
			worldEntity.axis = v.ToMat3();
			worldEntity.shaderParms[0] = 1;
			worldEntity.shaderParms[1] = 1;
			worldEntity.shaderParms[2] = 1;
			worldEntity.shaderParms[3] = 1;
			modelDef = world->AddEntityDef( &worldEntity );
		}
		needsRender = false;
	}
}

void idRenderWindow::Render( int time ) {
	rLight.origin = lightOrigin.ToVec3();
	rLight.shaderParms[SHADERPARM_RED] = lightColor.x();
	rLight.shaderParms[SHADERPARM_GREEN] = lightColor.y();
	rLight.shaderParms[SHADERPARM_BLUE] = lightColor.z();
	world->UpdateLightDef(lightDef, &rLight);
	if ( worldEntity.hModel ) {
		if (updateAnimation) {
			BuildAnimation( time );
		}
		if ( modelAnim ) {
			if ( time > animEndTime ) {
				animEndTime = time + animLength;
			}
			gameEdit->ANIM_CreateAnimFrame( worldEntity.hModel, modelAnim, worldEntity.numJoints, worldEntity.joints, animLength - (animEndTime - time), vec3_origin, false );
		}
		worldEntity.axis = anAngles( modelRotate.x(), modelRotate.y(), modelRotate.z()).ToMat3();
		world->UpdateEntityDef( modelDef, &worldEntity );
	}
}

void idRenderWindow::Draw( inttime, float x, float y) {
	PreRender();
	Render( time );

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.vieworg = viewOffset.ToVec3();;
	//refdef.vieworg.Set(-128, 0, 0);

	refdef.viewaxis.Identity();
	refdef.shaderParms[0] = 1;
	refdef.shaderParms[1] = 1;
	refdef.shaderParms[2] = 1;
	refdef.shaderParms[3] = 1;

	refdef.x = drawRect.x;
	refdef.y = drawRect.y;
	refdef.width = drawRect.w;
	refdef.height = drawRect.h;
	refdef.fov_x = 90;
	refdef.fov_y = 2 * atan(( float )drawRect.h / drawRect.w) * anMath::M_RAD2DEG;

	refdef.time = time;
	world->RenderScene(&refdef);
}

void idRenderWindow::PostParse() {
	idWindow::PostParse();
}

idWinVar *idRenderWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner ) {
	if (anString::Icmp(_name, "model" ) == 0) {
		return &modelName;
	}
	if (anString::Icmp(_name, "anim" ) == 0) {
		return &animName;
	}
	if (anString::Icmp(_name, "lightOrigin" ) == 0) {
		return &lightOrigin;
	}
	if (anString::Icmp(_name, "lightColor" ) == 0) {
		return &lightColor;
	}
	if (anString::Icmp(_name, "modelOrigin" ) == 0) {
		return &modelOrigin;
	}
	if (anString::Icmp(_name, "modelRotate" ) == 0) {
		return &modelRotate;
	}
	if (anString::Icmp(_name, "viewOffset" ) == 0) {
		return &viewOffset;
	}
	if (anString::Icmp(_name, "needsRender" ) == 0) {
		return &needsRender;
	}
	return idWindow::GetWinVarByName(_name, fixup, owner);
}

bool idRenderWindow::ParseInternalVar(const char *_name, anParser *src) {
	if (anString::Icmp(_name, "animClass" ) == 0) {
		ParseString( src, animClass);
		return true;
	}
	return idWindow::ParseInternalVar(_name, src);
}
