#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../Atmosphere.h"
#include "RainEffect.h"

CLASS_DECLARATION( arcEntity, RainFX )
END_CLASS

/*
==============
RainFX::Spawn
==============
*/
void RainFX::Spawn( void ) {
	renderEntity_t *re = GetRenderEntity();

	re->numInsts = 9;
	re->insts = new sdInstInfo[ re->numInsts ];

	anVec3 zero;
	anMat3 I;
	zero.Zero();
	I.Identity();
	this->SetPosition( zero, I );

	BecomeActive( TH_THINK );
	UpdateVisuals();
}

/*
==============
RainFX::Think
==============
*/
void RainFX::Think( void ) {
	renderEntity_t *re = GetRenderEntity();
	if ( re->hModel == nullptr ) {
		return;
	}

	anBounds modelbb = re->hModel->Bounds();
	anVec3 extents = ( modelbb.GetMaxs() - modelbb.GetMins() ) * 0.5f;

	arcPlayer *p = gameLocal.GetLocalViewPlayer();
	anVec3 const &v = p->GetViewPos();
	int gridx = anMath::Ftoi( anMath::Floor(v.x / extents.x) );
	int gridy = anMath::Ftoi( anMath::Floor(v.y / extents.y) );

	anBounds bounds;
	bounds.Clear();
	sdInstInfo *inst = re->insts;
	for ( int y=- 1; y<= 1; y++ ) {
		for ( int x=- 1; x<= 1; x++ ) {
			anBounds bb2;
			inst->fadeOrigin = inst->inst.origin = anVec3( (x + gridx) * extents.x, (y + gridy) * extents.y, v.z );
			inst->inst.axis.Identity();
			inst->maxVisDist = 0;
			inst->minVisDist = 0.f;
			bb2 = modelbb.Translate( inst->inst.origin );
			bounds.AddBounds( bb2 );
			inst++;
		}
	}
	re->flags.overridenBounds = true;
	re->bounds = bounds;

	UpdateVisuals();
	Present();
}

/*
==============
RainPrecipitation::RainPrecipitation
==============
*/
RainPrecipitation::RainPrecipitation( PrecipitationParameters const &_parms ) : parms( _parms ) {
	renderEntityHandle = -1;
	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.hModel = parms.model;

	renderEntity.numInsts = 9;
	renderEntity.insts = new sdInstInfo[ renderEntity.numInsts ];

	renderEntity.axis.Identity();
	renderEntity.origin.Zero();

	SetupEffect();
}

/*
==============
RainPrecipitation::~RainPrecipitation
==============
*/
RainPrecipitation::~RainPrecipitation() {
	FreeRenderEntity();
	delete []renderEntity.insts;
	//renderModelManager->FreeModel( renderEntity.hModel );
}

/*
==============
RainPrecipitation::SetupEffect
==============
*/
void RainPrecipitation::SetupEffect( void ) {
	renderEffect_t &renderEffect = effect.GetRenderEffect();
	renderEffect.declEffect = parms.effect;
	renderEffect.axis.Identity();
	renderEffect.loop = true;
	renderEffect.shaderParms[SHADERPARM_RED]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_BLUE]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	renderEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;

	effectRunning = false;
}

/*
==============
RainPrecipitation::SetMaxActiveParticles
==============
*/
void RainPrecipitation::SetMaxActiveParticles( int num ) {
}

/*
==============
RainPrecipitation::Update
==============
*/
void RainPrecipitation::Update( void ) {
	renderEntity_t *re = GetRenderEntity();
	if ( re->hModel == nullptr ) {
		return;
	}

	anBounds modelbb = re->hModel->Bounds();
	anVec3 extents = ( modelbb.GetMaxs() - modelbb.GetMins() ) * 0.5f;

	arcPlayer *p = gameLocal.GetLocalViewPlayer();
	anVec3 const &v = p->GetViewPos();
	int gridx = anMath::Ftoi( anMath::Floor(v.x / extents.x) );
	int gridy = anMath::Ftoi( anMath::Floor(v.y / extents.y) );

	anBounds bounds;
	bounds.Clear();
	sdInstInfo *inst = re->insts;
	for ( int y=- 1; y<= 1; y++ ) {
		for ( int x=- 1; x<= 1; x++ ) {
			anBounds bb2;
			inst->fadeOrigin = inst->inst.origin = anVec3( ( x + gridx ) * extents.x, ( y + gridy ) * extents.y, v.z );
			inst->inst.axis.Identity();
			inst->maxVisDist = 0;
			inst->minVisDist = 0.f;
			bb2 = modelbb.Translate( inst->inst.origin );
			bounds.AddBounds( bb2 );
			inst++;
		}
	}
	re->flags.overridenBounds = true;
	re->bounds = bounds;

	if ( renderEntityHandle == -1 ) {
		renderEntityHandle = gameRenderWorld->AddEntityDef( re );
	} else {
		gameRenderWorld->UpdateEntityDef( renderEntityHandle, re );
	}

	if ( !effect.GetRenderEffect().declEffect ) {
		return;
	}

	anVec3 viewOrg;
	renderView_t view;
	if ( DemoMngr::GetInstance().CalculateRenderView( &view ) ) {
		viewOrg = view.vieworg;
	} else {
		// If we are inside don't run the bacground effect
		arcPlayer *player = gameLocal.GetLocalViewPlayer();
		if ( player == nullptr ) {
			return;
		}
		viewOrg = player->GetRenderView()->vieworg;
	}

	int area = gameRenderWorld->PointInArea( viewOrg );
	bool runEffect = false;
	if ( area >= 0 ) {
		if ( gameRenderWorld->GetAreaPortalFlags( area ) & ( 1 << PORTAL_OUTSIDE ) ) {
			runEffect = true && !g_skipLocalizedPrecipitation.GetBool();
		}
	}

	// Update the background effect
	if ( runEffect ) {
		effect.GetRenderEffect().origin = viewOrg;
		if ( !effectRunning ) {
			effect.Start( gameLocal.time );
			effectRunning = true;
		} else {
			effect.Update();
		}
	} else {
		effect.StopDetach();
		effectRunning = false;
	}
}

/*
==============
RainPrecipitation::Init
==============
*/
void RainPrecipitation::Init( void ) {
}

/*
==============
RainPrecipitation::FreeRenderEntity
==============
*/
void RainPrecipitation::FreeRenderEntity( void ) {
	if ( renderEntityHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( renderEntityHandle );
		renderEntityHandle = -1;
	}
	if ( !effect.GetRenderEffect().declEffect ) return;

	effect.FreeRenderEffect();
}
