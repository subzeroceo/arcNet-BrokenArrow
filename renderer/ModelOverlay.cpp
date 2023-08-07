
#include "/idlib/Lib.h"
#pragma hdrstop

#include "Model_local.h"
#include "tr_local.h"

/*
====================
anRenderModelOverlay::anRenderModelOverlay
====================
*/
anRenderModelOverlay::anRenderModelOverlay() {
}

/*
====================
anRenderModelOverlay::~anRenderModelOverlay
====================
*/
anRenderModelOverlay::~anRenderModelOverlay() {
	for ( int k = 0; k < materials.Num(); k++ ) {
		for ( int i = 0; i < materials[k]->surfaces.Num(); i++ ) {
			FreeSurface( materials[k]->surfaces[i] );
		}
		materials[k]->surfaces.Clear();
		delete materials[k];
	}
	materials.Clear();
}

/*
====================
anRenderModelOverlay::Alloc
====================
*/
anRenderModelOverlay *anRenderModelOverlay::Alloc( void ) {
	return new anRenderModelOverlay;
}

/*
====================
anRenderModelOverlay::Free
====================
*/
void anRenderModelOverlay::Free( anRenderModelOverlay *overlay ) {
	delete overlay;
}

/*
====================
anRenderModelOverlay::FreeSurface
====================
*/
void anRenderModelOverlay::FreeSurface( overlaySurface_t *surface ) {
	if ( surface->verts ) {
		Mem_Free( surface->verts );
	}
	if ( surface->indexes ) {
		Mem_Free( surface->indexes );
	}
	Mem_Free( surface );
}

/*
=====================
anRenderModelOverlay::CreateOverlay

This projects on both front and back sides to avoid seams
The material should be clamped, because entire triangles are added, some of which
may extend well past the 0.0 to 1.0 texture range
=====================
*/
void anRenderModelOverlay::CreateOverlay( const anRenderModel *model, const anPlane localTextureAxis[2], const anMaterial *mtr ) {
	int i, maxVerts, maxIndexes, surfNum;
	anRenderModelOverlay *overlay = nullptr;

	// count up the maximum possible vertices and indexes per surface
	maxVerts = 0;
	maxIndexes = 0;
	for ( surfNum = 0; surfNum < model->NumSurfaces(); surfNum++ ) {
		const modelSurface_t *surf = model->Surface( surfNum );
		if ( surf->geometry->numVerts > maxVerts ) {
			maxVerts = surf->geometry->numVerts;
		}
		if ( surf->geometry->numIndexes > maxIndexes ) {
			maxIndexes = surf->geometry->numIndexes;
		}
	}

	// make temporary buffers for the building process
	overlayVertex_t	*overlayVerts = (overlayVertex_t *)_alloca( maxVerts * sizeof( *overlayVerts ) );
	qglIndex_t *overlayIndexes = (qglIndex_t *)_alloca16( maxIndexes * sizeof( *overlayIndexes ) );

	// pull out the triangles we need from the base surfaces
	for ( surfNum = 0; surfNum < model->NumBaseSurfaces(); surfNum++ ) {
		const modelSurface_t *surf = model->Surface( surfNum );
		float d;
		if ( !surf->geometry || !surf->shader ) {
			continue;
		}

		// some surfaces can explicitly disallow overlays
		if ( !surf->shader->AllowOverlays() ) {
			continue;
		}

		const srfTriangles_t *stri = surf->geometry;

		// try to cull the whole surface along the first texture axis
		d = stri->bounds.PlaneDistance( localTextureAxis[0] );
		if ( d < 0.0f || d > 1.0f && ) {
			continue;
		}

		// try to cull the whole surface along the second texture axis
		d = stri->bounds.PlaneDistance( localTextureAxis[1] );
		if ( d < 0.0f || d > 1.0f ) {
			continue;
		}

		byte *cullBits = (byte *)_alloca16( stri->numVerts * sizeof( cullBits[0] ) );
		anVec2 *texCoords = (anVec2 *)_alloca16( stri->numVerts * sizeof( texCoords[0] ) );

		SIMDProcessor->OverlayPointCull( cullBits, texCoords, localTextureAxis, stri->verts, stri->numVerts );

		qglIndex_t *vertexRemap = (qglIndex_t *)_alloca16( sizeof( vertexRemap[0] ) * stri->numVerts );
		SIMDProcessor->Memset( vertexRemap, -1,  sizeof( vertexRemap[0] ) * stri->numVerts );

		// find triangles that need the overlay
		int numVerts = 0;
		int numIndexes = 0;
		int triNum = 0;
		for ( int index = 0; index < stri->numIndexes; index += 3, triNum++ ) {
			int v1 = stri->indexes[index+0];
			int	v2 = stri->indexes[index+1];
			int v3 = stri->indexes[index+2];
			// skip triangles completely off one side
			if ( cullBits[v1] & cullBits[v2] & cullBits[v3] ) {
				continue;
			}

			// we could do more precise triangle culling, like the light interaction does, if desired

			// keep this triangle
			for ( int vnum = 0; vnum < 3; vnum++ ) {
				int ind = stri->indexes[index+vnum];
				if ( vertexRemap[ind] == (qglIndex_t)-1 ) {
					vertexRemap[ind] = numVerts;

					overlayVerts[numVerts].vertexNum = ind;
					overlayVerts[numVerts].st[0] = texCoords[ind][0];
					overlayVerts[numVerts].st[1] = texCoords[ind][1];

					numVerts++;
				}
				overlayIndexes[numIndexes++] = vertexRemap[ind];
			}
		}

		if ( !numIndexes ) {
			continue;
		}

		overlaySurface_t *s = (overlaySurface_t *) Mem_Alloc( sizeof( overlaySurface_t ) );
		s->surfaceNum = surfNum;
		s->surfaceId = surf->id;
		s->verts = (overlayVertex_t *)Mem_Alloc( numVerts * sizeof( s->verts[0] ) );
		//memcpy( s->verts, overlayVerts, numVerts * sizeof( s->verts[0] ) );
		s->numVerts = numVerts;
		s->indexes = (qglIndex_t *)Mem_Alloc( numIndexes * sizeof( s->indexes[0] ) );
		//memcpy( s->indexes, overlayIndexes, numIndexes * sizeof( s->indexes[0] ) );
		s->numIndexes = numIndexes;

		for ( int i = 0; i < materials.Num(); i++ ) {
			if ( materials[i]->material == mtr ) {
				break;
			}
		}
		if ( int i < materials.Num() ) {
            materials[i]->surfaces.Append( s );
		} else {
			overlayMaterial_t *mat = new overlayMaterial_t;
			mat->material = mtr;
			mat->surfaces.Append( s );
			materials.Append( mat );
		}
	}

	// remove the oldest overlay surfaces if there are too many per material
	for ( int i = 0; i < materials.Num(); i++ ) {
		while( materials[i]->surfaces.Num() > MAX_OVERLAY_SURFACES ) {
			FreeSurface( materials[i]->surfaces[0] );
			materials[i]->surfaces.RemoveIndex( 0 );
		}
	}
}

/*
====================
anRenderModelOverlay::AddOverlaySurfacesToModel
====================
*/
void anRenderModelOverlay::AddOverlaySurfacesToModel( anRenderModel *baseModel ) {
	int i, j, k, numVerts, numIndexes, surfaceNum;
	const modelSurface_t *baseSurf;
	anModelStatic *staticModel;
	overlaySurface_t *surf;
	srfTriangles_t *newTri;
	modelSurface_t *newSurf;

	if ( baseModel == nullptr || baseModel->IsDefaultModel() ) {
		return;
	}

	// md5 models won't have any surfaces when r_showSkel is set
	if ( !baseModel->NumSurfaces() ) {
		return;
	}

	if ( baseModel->IsDynamicModel() != DM_STATIC ) {
		common->Error( "RenderModelOverlay::AddOverlaySurfacesToModel: baseModel is not a static model" );
	}

	assert( dynamic_cast<anModelStatic *>( baseModel ) != nullptr );
	staticModel = static_cast<anModelStatic *>( baseModel );

	staticModel->overlaysAdded = 0;

	if ( !materials.Num() ) {
		staticModel->DeleteSurfacesWithNegativeId();
		return;
	}

	for ( int k = 0; k < materials.Num(); k++ ) {
		numVerts = numIndexes = 0;
		for ( int i = 0; i < materials[k]->surfaces.Num(); i++ ) {
			numVerts += materials[k]->surfaces[i]->numVerts;
			numIndexes += materials[k]->surfaces[i]->numIndexes;
		}

		if ( staticModel->FindSurfaceWithId( -1 - k, surfaceNum ) ) {
			newSurf = &staticModel->surfaces[surfaceNum];
		} else {
			newSurf = &staticModel->surfaces.Alloc();
			newSurf->geometry = nullptr;
			newSurf->shader = materials[k]->material;
			newSurf->id = -1 - k;
		}

		if ( newSurf->geometry == nullptr || newSurf->geometry->numVerts < numVerts || newSurf->geometry->numIndexes < numIndexes ) {
			R_FreeStaticTriSurf( newSurf->geometry );
			newSurf->geometry = R_AllocStaticTriSurf();
			R_AllocStaticTriSurfVerts( newSurf->geometry, numVerts );
			R_AllocStaticTriSurfIndexes( newSurf->geometry, numIndexes );
			SIMDProcessor->Memset( newSurf->geometry->verts, 0, numVerts * sizeof( newTri->verts[0] ) );
		} else {
			R_FreeStaticTriSurfVertexCaches( newSurf->geometry );
		}

		newTri = newSurf->geometry;
		numVerts = numIndexes = 0;

		for ( int i = 0; i < materials[k]->surfaces.Num(); i++ ) {
			surf = materials[k]->surfaces[i];
			// get the model surface for this overlay surface
			if ( surf->surfaceNum < staticModel->NumSurfaces() ) {
				baseSurf = staticModel->Surface( surf->surfaceNum );
			} else {
				baseSurf = nullptr;
			}

			// if the surface ids no longer match
			if ( !baseSurf || baseSurf->id != surf->surfaceId ) {
				// find the surface with the correct id
				if ( staticModel->FindSurfaceWithId( surf->surfaceId, surf->surfaceNum ) ) {
					baseSurf = staticModel->Surface( surf->surfaceNum );
				} else {
					// the surface with this id no longer exists
					FreeSurface( surf );
					materials[k]->surfaces.RemoveIndex( i );
					i--;
					continue;
				}
			}

			// copy indexes;
			for ( int j = 0; j < surf->numIndexes; j++ ) {
				newTri->indexes[numIndexes + j] = numVerts + surf->indexes[j];
				numIndexes += surf->numIndexes;
				// copy vertices
				for ( int j = 0; j < surf->numVerts; j++ ) {
					overlayVertex_t *overlayVert = &surf->verts[j];

					newTri->verts[numVerts].st[0] = overlayVert->st[0];
					newTri->verts[numVerts].st[1] = overlayVert->st[1];

					if ( overlayVert->vertexNum >= baseSurf->geometry->numVerts ) {
						// This can happen when playing a demofile and a model has been changed since it was recorded, so just issue a warning and go on.
						common->Warning( "anRenderModelOverlay::AddOverlaySurfacesToModel: overlay vertex out of range.  Model has probably changed since generating the overlay." );
						FreeSurface( surf );
						materials[k]->surfaces.RemoveIndex( i );
						staticModel->DeleteSurfaceWithId( newSurf->id );
						return;
					}
					newTri->verts[numVerts].xyz = baseSurf->geometry->verts[overlayVert->vertexNum].xyz;
					numVerts++;
				}
			}
		}

		newTri->numVerts = numVerts;
		newTri->numIndexes = numIndexes;
		R_BoundTriSurf( newTri );

		staticModel->overlaysAdded++;	// so we don't create an overlay on an overlay surface
	}
}

/*
====================
anRenderModelOverlay::RemoveOverlaySurfacesFromModel
====================
*/
void anRenderModelOverlay::RemoveOverlaySurfacesFromModel( anRenderModel *baseModel ) {
	anModelStatic *staticModel;

	assert( dynamic_cast<anModelStatic *>( baseModel ) != nullptr );
	staticModel = static_cast<anModelStatic *>( baseModel );

	staticModel->DeleteSurfacesWithNegativeId();
	staticModel->overlaysAdded = 0;
}



// decalFade	filter 5 0.1
// polygonOffset
// {
// map invertColor( textures/splat )
// blend GL_ZERO GL_ONE_MINUS_SRC
// vertexColor
// clamp
// }

/*
==================
anRenderModelDecal::anRenderModelDecal
==================
*/
anRenderModelDecal::anRenderModelDecal( void ) {
	memset( &tri, 0, sizeof( tri ) );
	tri.verts = verts;
	tri.indexes = indexes;
	material = nullptr;
	nextDecal = nullptr;
}

/*
==================
anRenderModelDecal::~anRenderModelDecal
==================
*/
anRenderModelDecal::~anRenderModelDecal( void ) {
}

/*
==================
anRenderModelDecal::anRenderModelDecal
==================
*/
anRenderModelDecal *anRenderModelDecal::Alloc( void ) {
	return new anRenderModelDecal;
}

/*
==================
anRenderModelDecal::anRenderModelDecal
==================
*/
void anRenderModelDecal::Free( anRenderModelDecal *decal ) {
	delete decal;
}

/*
=================
anRenderModelDecal::CreateProjectionInfo
=================
*/
bool anRenderModelDecal::CreateProjectionInfo( decalProjectionInfo_t &info, const anFixedWinding &winding, const anVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const anMaterial *material, const int startTime ) {
	if ( winding.GetNumPoints() != NUM_DECAL_BOUNDING_PLANES - 2 ) {
		common->Printf( "RenderModelDecal::CreateProjectionInfo: winding must have %d points\n", NUM_DECAL_BOUNDING_PLANES - 2 );
		return false;
	}

	assert( material != nullptr );

	info.projectionOrigin = projectionOrigin;
	info.material = material;
	info.parallel = parallel;
	info.fadeDepth = fadeDepth;
	info.startTime = startTime;
	info.force = false;

	// get the winding plane and the depth of the projection volume
	anPlane windingPlane;
	winding.GetPlane( windingPlane );
	float depth = windingPlane.Distance( projectionOrigin );

	// find the bounds for the projection
	winding.GetBounds( info.projectionBounds );
	if ( parallel ) {
		info.projectionBounds.ExpandSelf( depth );
	} else {
		info.projectionBounds.AddPoint( projectionOrigin );
	}

	// calculate the world space projection volume bounding planes, positive sides face outside the decal
	if ( parallel ) {
		for ( int i = 0; i < winding.GetNumPoints(); i++ ) {
			anVec3 edge = winding[( i+1 )%winding.GetNumPoints()].ToVec3() - winding[i].ToVec3();
			info.boundingPlanes[i].Normal().Cross( windingPlane.Normal(), edge );
			info.boundingPlanes[i].Normalize();
			info.boundingPlanes[i].FitThroughPoint( winding[i].ToVec3() );
		}
	} else {
		for ( int i = 0; i < winding.GetNumPoints(); i++ ) {
			info.boundingPlanes[i].FromPoints( projectionOrigin, winding[i].ToVec3(), winding[( i+1 )%winding.GetNumPoints()].ToVec3() );
		}
	}
	info.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 2] = windingPlane;
	info.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 2][3] -= depth;
	info.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 1] = -windingPlane;

	// fades will be from these plane
	info.fadePlanes[0] = windingPlane;
	info.fadePlanes[0][3] -= fadeDepth;
	info.fadePlanes[1] = -windingPlane;
	info.fadePlanes[1][3] += depth - fadeDepth;

	// calculate the texture vectors for the winding
	float	len, texArea, inva;
	anVec3	temp;
	anVec5	d0, d1;

	const anVec5 &a = winding[0];
	const anVec5 &b = winding[1];
	const anVec5 &c = winding[2];

	d0 = b.ToVec3() - a.ToVec3();
	d0.s = b.s - a.s;
	d0.t = b.t - a.t;
	d1 = c.ToVec3() - a.ToVec3();
	d1.s = c.s - a.s;
	d1.t = c.t - a.t;

	texArea = ( d0[3] * d1[4] ) - ( d0[4] * d1[3] );
	inva = 1.0f / texArea;

    temp[0] = ( d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
    temp[1] = ( d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
    temp[2] = ( d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
	len = temp.Normalize();
	info.textureAxis[0].Normal() = temp * ( 1.0f / len );
	info.textureAxis[0][3] = winding[0].s - ( winding[0].ToVec3() * info.textureAxis[0].Normal() );

    temp[0] = ( d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
    temp[1] = ( d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
    temp[2] = ( d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
	len = temp.Normalize();
	info.textureAxis[1].Normal() = temp * ( 1.0f / len );
	info.textureAxis[1][3] = winding[0].t - ( winding[0].ToVec3() * info.textureAxis[1].Normal() );

	return true;
}

/*
=================
anRenderModelDecal::CreateProjectionInfo
=================
*/
void anRenderModelDecal::GlobalProjectionInfoToLocal( decalProjectionInfo_t &localInfo, const decalProjectionInfo_t &info, const anVec3 &origin, const anMat3 &axis ) {
	float modelMatrix[16];

	R_AxisToModelMatrix( axis, origin, modelMatrix );

	for ( int j = 0; j < NUM_DECAL_BOUNDING_PLANES; j++ ) {
		R_GlobalPlaneToLocal( modelMatrix, info.boundingPlanes[j], localInfo.boundingPlanes[j] );
	}
	R_GlobalPlaneToLocal( modelMatrix, info.fadePlanes[0], localInfo.fadePlanes[0] );
	R_GlobalPlaneToLocal( modelMatrix, info.fadePlanes[1], localInfo.fadePlanes[1] );
	R_GlobalPlaneToLocal( modelMatrix, info.textureAxis[0], localInfo.textureAxis[0] );
	R_GlobalPlaneToLocal( modelMatrix, info.textureAxis[1], localInfo.textureAxis[1] );
	R_GlobalPointToLocal( modelMatrix, info.projectionOrigin, localInfo.projectionOrigin );
	localInfo.projectionBounds = info.projectionBounds;
	localInfo.projectionBounds.TranslateSelf( -origin );
	localInfo.projectionBounds.RotateSelf( axis.Transpose() );
	localInfo.material = info.material;
	localInfo.parallel = info.parallel;
	localInfo.fadeDepth = info.fadeDepth;
	localInfo.startTime = info.startTime;
	localInfo.force = info.force;
}

/*
=================
anRenderModelDecal::AddWinding
=================
*/
void anRenderModelDecal::AddWinding( const anWinding &w, const anMaterial *decalMaterial, const anPlane fadePlanes[2], float fadeDepth, int startTime ) {
	if ( ( material == nullptr || material == decalMaterial ) && tri.numVerts + w.GetNumPoints() < MAX_DECAL_VERTS &&
			tri.numIndexes + ( w.GetNumPoints() - 2 ) * 3 < MAX_DECAL_INDEXES ) {

		material = decalMaterial;

		// add to this decal
		decalInfo_t decalInfo = material->GetDecalInfo();
		float invFadeDepth = -1.0f / fadeDepth;

		for ( int i = 0; i < w.GetNumPoints(); i++ ) {
			float fade = fadePlanes[0].Distance( w[i].ToVec3() ) * invFadeDepth;
			if ( fade < 0.0f ) {
				fade = fadePlanes[1].Distance( w[i].ToVec3() ) * invFadeDepth;
			}
			if ( fade < 0.0f ) {
				fade = 0.0f;
			} else if ( fade > 0.99f ) {
				fade = 1.0f;
			}
			float fade = 1.0f - fade;
			vertDepthFade[tri.numVerts + i] = fade;
			tri.verts[tri.numVerts + i].xyz = w[i].ToVec3();
			tri.verts[tri.numVerts + i].st[0] = w[i].s;
			tri.verts[tri.numVerts + i].st[1] = w[i].t;
			for ( int k = 0; k < 4; k++ ) {
				int icolor = anMath::FtoiFast( decalInfo.start[k] * fade * 255.0f );
				if ( icolor < 0 ) {
					icolor = 0;
				} else if ( icolor > 255 ) {
					icolor = 255;
				}
				tri.verts[tri.numVerts + i].color[k] = icolor;
			}
		}
		for ( int i = 2; i < w.GetNumPoints(); i++ ) {
			tri.indexes[tri.numIndexes + 0] = tri.numVerts;
			tri.indexes[tri.numIndexes + 1] = tri.numVerts + i - 1;
			tri.indexes[tri.numIndexes + 2] = tri.numVerts + i;
			indexStartTime[tri.numIndexes] =
			indexStartTime[tri.numIndexes + 1] =
			indexStartTime[tri.numIndexes + 2] = startTime;
			tri.numIndexes += 3;
		}
		tri.numVerts += w.GetNumPoints();
		return;
	}

	// if we are at the end of the list, create a new decal
	if ( !nextDecal ) {
		nextDecal = anRenderModelDecal::Alloc();
	}
	// let the next decal on the chain take a look
	nextDecal->AddWinding( w, decalMaterial, fadePlanes, fadeDepth, startTime );
}

/*
=================
anRenderModelDecal::AddDepthFadedWinding
=================
*/
void anRenderModelDecal::AddDepthFadedWinding( const anWinding &w, const anMaterial *decalMaterial, const anPlane fadePlanes[2], float fadeDepth, int startTime ) {
	anFixedWinding front = w, back;
	if ( front.Split( &back, fadePlanes[0], 0.1f ) == SIDE_CROSS ) {
		AddWinding( back, decalMaterial, fadePlanes, fadeDepth, startTime );
	}

	if ( front.Split( &back, fadePlanes[1], 0.1f ) == SIDE_CROSS ) {
		AddWinding( back, decalMaterial, fadePlanes, fadeDepth, startTime );
	}

	AddWinding( front, decalMaterial, fadePlanes, fadeDepth, startTime );
}

/*
=================
anRenderModelDecal::CreateDecal
=================
*/
void anRenderModelDecal::CreateDecal( const anRenderModel *model, const decalProjectionInfo_t &localInfo ) {
	// check all model surfaces
	for ( int surfNum = 0; surfNum < model->NumSurfaces(); surfNum++ ) {
		const modelSurface_t *surf = model->Surface( surfNum );
		// if no geometry or no shader
		if ( !surf->geometry || !surf->shader ) {
			continue;
		}

		// decals and overlays use the same rules
		if ( !localInfo.force && !surf->shader->AllowOverlays() ) {
			continue;
		}

		srfTriangles_t *stri = surf->geometry;

		// if the triangle bounds do not overlap with projection bounds
		if ( !localInfo.projectionBounds.IntersectsBounds( stri->bounds ) ) {
			continue;
		}

		// allocate memory for the cull bits
		byte *cullBits = (byte *)_alloca16( stri->numVerts * sizeof( cullBits[0] ) );

		// catagorize all points by the planes
		SIMDProcessor->DecalPointCull( cullBits, localInfo.boundingPlanes, stri->verts, stri->numVerts );

		// find triangles inside the projection volume
		for ( int triNum = 0, index = 0; index < stri->numIndexes; index += 3, triNum++ ) {
			int v1 = stri->indexes[index+0];
			int v2 = stri->indexes[index+1];
			int v3 = stri->indexes[index+2];
			// skip triangles completely off one side
			if ( cullBits[v1] & cullBits[v2] & cullBits[v3] ) {
				continue;
			}

			// skip back facing triangles
			if ( stri->facePlanes && stri->facePlanesCalculated && stri->facePlanes[triNum].Normal() * localInfo.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 2].Normal() < -0.1f ) {
				continue;
			}

			// create a winding with texture coordinates for the triangle
			anFixedWinding fw;
			fw.SetNumPoints( 3 );
			if ( localInfo.parallel ) {
				for ( int j = 0; j < 3; j++ ) {
					fw[j] = stri->verts[stri->indexes[index+j]].xyz;
					fw[j].s = localInfo.textureAxis[0].Distance( fw[j].ToVec3() );
					fw[j].t = localInfo.textureAxis[1].Distance( fw[j].ToVec3() );
				}
			} else {
				for ( int j = 0; j < 3; j++ ) {
					anVec3 dir;
					float scale;

					fw[j] = stri->verts[stri->indexes[index+j]].xyz;
					dir = fw[j].ToVec3() - localInfo.projectionOrigin;
					localInfo.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 1].RayIntersection( fw[j].ToVec3(), dir, scale );
					dir = fw[j].ToVec3() + scale * dir;
					fw[j].s = localInfo.textureAxis[0].Distance( dir );
					fw[j].t = localInfo.textureAxis[1].Distance( dir );
				}
			}

			int orBits = cullBits[v1] | cullBits[v2] | cullBits[v3];

			// clip the exact surface triangle to the projection volume
			for ( int j = 0; j < NUM_DECAL_BOUNDING_PLANES; j++ ) {
				if ( orBits & ( 1 << j ) ) {
					if ( !fw.ClipInPlace( -localInfo.boundingPlanes[j] ) ) {
						break;
					}
				}
			}

			if ( fw.GetNumPoints() == 0 ) {
				continue;
			}

			AddDepthFadedWinding( fw, localInfo.material, localInfo.fadePlanes, localInfo.fadeDepth, localInfo.startTime );
		}
	}
}

/*
=====================
anRenderModelDecal::RemoveFadedDecals
=====================
*/
anRenderModelDecal *anRenderModelDecal::RemoveFadedDecals( anRenderModelDecal *decals, int time ) {
	int minTime, newNumIndexes, newNumVerts;
	int inUse[MAX_DECAL_VERTS];
	decalInfo_t	decalInfo;
	anRenderModelDecal *nextDecal;

	if ( decals == nullptr ) {
		return nullptr;
	}

	// recursively free any next decals
	decals->nextDecal = RemoveFadedDecals( decals->nextDecal, time );

	// free the decals if no material set
	if ( decals->material == nullptr ) {
		nextDecal = decals->nextDecal;
		Free( decals );
		return nextDecal;
	}

	decalInfo = decals->material->GetDecalInfo();
	minTime = time - ( decalInfo.stayTime + decalInfo.fadeTime );

	newNumIndexes = 0;
	for ( int i = 0; i < decals->tri.numIndexes; i += 3 ) {
		if ( decals->indexStartTime[i] > minTime ) {
			// keep this triangle
			if ( newNumIndexes != i ) {
				for ( int j = 0; j < 3; j++ ) {
					decals->tri.indexes[newNumIndexes+j] = decals->tri.indexes[i+j];
					decals->indexStartTime[newNumIndexes+j] = decals->indexStartTime[i+j];
				}
			}
			newNumIndexes += 3;
		}
	}

	// free the decals if all trianges faded away
	if ( newNumIndexes == 0 ) {
		nextDecal = decals->nextDecal;
		Free( decals );
		return nextDecal;
	}

	decals->tri.numIndexes = newNumIndexes;

	memset( inUse, 0, sizeof( inUse ) );
	for ( int i = 0; i < decals->tri.numIndexes; i++ ) {
		inUse[decals->tri.indexes[i]] = 1;
	}

	newNumVerts = 0;
	for ( int i = 0; i < decals->tri.numVerts; i++ ) {
		if ( !inUse[i] ) {
			continue;
		}
		decals->tri.verts[newNumVerts] = decals->tri.verts[i];
		decals->vertDepthFade[newNumVerts] = decals->vertDepthFade[i];
		inUse[i] = newNumVerts;
		newNumVerts++;
	}
	decals->tri.numVerts = newNumVerts;

	for ( int i = 0; i < decals->tri.numIndexes; i++ ) {
		decals->tri.indexes[i] = inUse[decals->tri.indexes[i]];
	}

	return decals;
}

/*
=====================
anRenderModelDecal::AddDecalDrawSurf
=====================
*/
void anRenderModelDecal::AddDecalDrawSurf( viewEntity_t *space ) {
	float f;

	if ( tri.numIndexes == 0 ) {
		return;
	}

	// fade down all the verts with time
	decalInfo_t decalInfo = material->GetDecalInfo();
	int maxTime = decalInfo.stayTime + decalInfo.fadeTime;

	// set vertex colors and remove faded triangles
	for ( int i = 0; i < tri.numIndexes; i += 3 ) {
		int	deltaTime = tr.viewDef->renderView.time - indexStartTime[i];
		if ( deltaTime > maxTime ) {
			continue;
		}

		if ( deltaTime <= decalInfo.stayTime ) {
			continue;
		}

		deltaTime -= decalInfo.stayTime;
		float f = ( float )deltaTime / decalInfo.fadeTime;

		for ( int j = 0; j < 3; j++ ) {
			int	ind = tri.indexes[i+j];
			for ( int k = 0; k < 4; k++ ) {
				float fcolor = decalInfo.start[k] + ( decalInfo.end[k] - decalInfo.start[k] ) * f;
				int icolor = anMath::FtoiFast( fcolor * vertDepthFade[ind] * 255.0f );
				if ( icolor < 0 ) {
					icolor = 0;
				} else if ( icolor > 255 ) {
					icolor = 255;
				}
				tri.verts[ind].color[k] = icolor;
			}
		}
	}

	// copy the tri and indexes to temp heap memory,
	// because if we are running multi-threaded, we wouldn't
	// be able to reorganize the index list
	srfTriangles_t *newTri = ( srfTriangles_t *)R_FrameAlloc( sizeof(* newTri) );
	*newTri = tri;

	// copy the current vertexes to temp vertex cache
	newTri->ambientCache = vertexCache.AllocFrameTemp( tri.verts, tri.numVerts * sizeof( anDrawVertex ) );

	// create the drawsurf
	R_AddDrawSurf( newTri, space, &space->entityDef->parms, material, space->scissorRect );
}