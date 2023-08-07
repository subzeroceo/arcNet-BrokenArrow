#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ARC_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "FootPrints.h"

#include "HardcodedParticleSystem.h"

/*********************************************************************************************************
arcFootPrintManagerLocal
*********************************************************************************************************/

class arcFootPrintManagerLocal : public arcFootPrintManager {
public:

	arcFootPrintManagerLocal() : system( nullptr ) {}
	virtual void					Init( void );
	virtual void					Deinit( void );

	virtual bool					AddFootPrint( const anVec3 & point, const anVec3 &forward, const anVec3 &up, bool right );

	virtual void					Think( void );

	virtual renderEntity_t *		GetRenderEntity( void );
	virtual qhandle_t				GetModelHandle( void );

private:
	struct footprint {
		anVec3 p;
		anVec3 f;
		anVec3 l;
		int age;
	};

	static const int MAX_FOOTSTEPS = 128;

	footprint footprints[ MAX_FOOTSTEPS ];

	HardcodedParticleSystem *system;
	unsigned short index;

	unsigned int start;
	unsigned int end;
};

arcFootPrintManagerLocal footPrintManagerLocal;
arcFootPrintManager *footPrintManager = &footPrintManagerLocal;

/*
==============
arcFootPrintManagerLocal::Init
==============
*/
void arcFootPrintManagerLocal::Init( void ) {
	const anDeclStringMap* map = gameLocal.declStringMapType[ "footPrintDef" ];
	if ( !map ) {
		gameLocal.Error( "FootPrints stringMap 'footPrintDef' not found" );
		return;
	}
	int maxnumpnts = MAX_FOOTSTEPS * 4;
	int maxnumtris = MAX_FOOTSTEPS * 2;
	system = new HardcodedParticleSystem;
	system->AddSurfaceDB( declHolder.declMaterialType.LocalFind( map->GetDict().GetString( "material", "_black" ) ), maxnumpnts, maxnumtris * 3 );
	system->GetRenderEntity().axis.Identity();
	system->GetRenderEntity().origin.Zero();

	start = 0;
	end = 0;
}

/*
==============
arcFootPrintManagerLocal::Deinit
==============
*/
void arcFootPrintManagerLocal::Shutdown( void ) {
	delete system;
	system = nullptr;
}

/*
==============
arcFootPrintManagerLocal::AddFootPrint
==============
*/
bool arcFootPrintManagerLocal::AddFootPrint( const anVec3 &point, const anVec3 &forward, const anVec3 &up, bool right ) {
	int num = end - start;
	int index = end % MAX_FOOTSTEPS;
	end++;
	if ( num == MAX_FOOTSTEPS ) {
		start++;
	}
	footprint *f = &footprints[index];
	f->f = forward;
	f->f.Normalize();
	f->l = up.Cross( f->f );
	f->f = f->l.Cross( up );
	f->p = point;
	f->age = gameLocal.time;
	if ( !right ) {
		f->l = -f->l;
	}
	return false;
}

/*
==============
arcFootPrintManagerLocal::Think
==============
*/
void arcFootPrintManagerLocal::Think( void ) {
	system->SetDoubleBufferedModel();

	while ( start < end ) {
		int index = start % MAX_FOOTSTEPS;
		footprint *f = &footprints[index];
		if ( ( gameLocal.time - f->age ) > SEC2MS( 60.f ) ) {
			start++;
		} else {
			break;
		}
	}

	srfTriangles_t *surf = system->GetTriSurf( 0 );
	surf->numIndexes = 0;
	surf->numVerts = 0;
	surf->bounds.Clear();

	for (unsigned int i=start; i<end; i++ ) {
		int index = i % MAX_FOOTSTEPS;
		footprint *f = &footprints[index];

		anDrawVertex *v = &surf->verts[ surf->numVerts ];
		vertIndex_t  *idx = &surf->indexes[ surf->numIndexes ];

		byte alpha = 255;
		int a = ( gameLocal.time - f->age );
		if ( a > SEC2MS( 50 ) ) {
			alpha = ( byte )( anMath::ClampFloat( 0.f, 1.f, 1.f - ( ( a - SEC2MS( 50 ) ) / ( float )( SEC2MS( 10 ) ) ) ) * 255 );
		}

		v->Clear();
		v->xyz = f->p + f->f * 8 + f->l * 3;
		v->SetST( 0.f, 0.f );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		v++;
		v->Clear();
		v->xyz = f->p + f->f * 8 - f->l * 3;
		v->SetST( 1.f, 0.f );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		v++;
		v->Clear();
		v->xyz = f->p - f->f * 8 + f->l * 3;
		v->SetST( 0.f, 1.f );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		v++;
		v->Clear();
		v->xyz = f->p - f->f * 8 - f->l * 3;
		v->SetST( 1.f, 1.f );
		v->color[0] = alpha;
		v->color[1] = alpha;
		v->color[2] = alpha;
		v->color[3] = alpha;
		v++;

		idx[0] = surf->numVerts + 0;
		idx[1] = surf->numVerts + 1;
		idx[2] = surf->numVerts + 2;
		idx[3] = surf->numVerts + 1;
		idx[4] = surf->numVerts + 3;
		idx[5] = surf->numVerts + 2;

		surf->numIndexes += 6;
		surf->numVerts += 4;

		surf->bounds.AddPoint( f->p );
	}

	if ( surf->numVerts ) {
		surf->bounds.ExpandSelf( 10.f );
		system->GetRenderEntity().hModel->FreeVertexCache();
		system->GetRenderEntity().hModel->SetBounds( surf->bounds);
		system->GetRenderEntity().bounds = surf->bounds;

		system->PresentRenderEntity();
	}
}

/*
==============
arcFootPrintManagerLocal::GetRenderEntity
==============
*/
renderEntity_t* arcFootPrintManagerLocal::GetRenderEntity( void ) {
	return system == nullptr ? nullptr : &system->GetRenderEntity();
}

/*
==============
arcFootPrintManagerLocal::GetModelHandle
==============
*/
int arcFootPrintManagerLocal::GetModelHandle( void ) {
	return system == nullptr ? -1 : system->GetModelHandle();
}
