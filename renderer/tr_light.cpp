#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"

static const float CHECK_BOUNDS_EPSILON = 1.0f;


/*
===========================================================================================

VERTEX CACHE GENERATORS

===========================================================================================
*/

/*
==================
R_CreateAmbientCache

Create it if needed
==================
*/
bool R_CreateAmbientCache( srfTriangles_t *tri, bool needsLighting ) {
	if ( tri->ambientCache ) {
		return true;
	}
	// we are going to use it for drawing, so make sure we have the tangents and normals
	if ( needsLighting && !tri->tangentsCalculated ) {
		R_DeriveTangents( tri );
	}

	vertexCache.Alloc( tri->verts, tri->numVerts * sizeof( tri->verts[0] ), &tri->ambientCache );
	if ( !tri->ambientCache ) {
		return false;
	}
	return true;
}

/*
==================
R_CreateLightingCache

Returns false if the cache couldn't be allocated, in which case the surface should be skipped.
==================
*/
bool R_CreateLightingCache( const anRenderEntityLocal *ent, const anRenderLightsLocal *light, srfTriangles_t *tri ) {
	anVec3		localLightOrigin;

	// fogs and blends don't need light vectors
	if ( light->lightShader->IsFogLight() || light->lightShader->IsBlendLight() ) {
		return true;
	}

	// not needed if we have vertex programs
	if ( tr.backEndRendererHasVertexPrograms ) {
		return true;
	}

	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, localLightOrigin );

	int	size = tri->ambientSurface->numVerts * sizeof( lightingCache_t );
	lightingCache_t *cache = (lightingCache_t *)_alloca16( size );

#if 1

	SIMDProcessor->CreateTextureSpaceLightVectors( &cache[0].localLightVector, localLightOrigin, tri->ambientSurface->verts, tri->ambientSurface->numVerts, tri->indexes, tri->numIndexes );

#else

	bool *used = (bool *)_alloca16( tri->ambientSurface->numVerts * sizeof( used[0] ) );
	memset( used, 0, tri->ambientSurface->numVerts * sizeof( used[0] ) );

	// because the interaction may be a very small subset of the full surface,
	// it makes sense to only deal with the verts used
	for ( int j = 0; j < tri->numIndexes; j++ ) {
		int i = tri->indexes[j];
		if ( used[i] ) {
			continue;
		}
		used[i] = true;

		anVec3 lightDir;
		const anDrawVertex *v;

		v = &tri->ambientSurface->verts[i];

		lightDir = localLightOrigin - v->xyz;

		cache[i].localLightVector[0] = lightDir * v->tangents[0];
		cache[i].localLightVector[1] = lightDir * v->tangents[1];
		cache[i].localLightVector[2] = lightDir * v->normal;
	}

#endif

	vertexCache.Alloc( cache, size, &tri->lightingCache );
	if ( !tri->lightingCache ) {
		return false;
	}
	return true;
}

/*
==================
R_CreatePrivateShadowCache

This is used only for a specific light
==================
*/
void R_CreatePrivateShadowCache( srfTriangles_t *tri ) {
	if ( !tri->shadowVertexes ) {
		return;
	}

	vertexCache.Alloc( tri->shadowVertexes, tri->numVerts * sizeof( *tri->shadowVertexes ), &tri->shadowCache );
}

/*
==================
R_CreateVertexProgramShadowCache

This is constant for any number of lights, the vertex program
takes care of projecting the verts to infinity.
==================
*/
void R_CreateVertexProgramShadowCache( srfTriangles_t *tri ) {
	if ( tri->verts == nullptr ) {
		return;
	}

	anShadowCache *temp = (anShadowCache *)_alloca16( tri->numVerts * 2 * sizeof( anShadowCache ) );

#if 1

	SIMDProcessor->CreateVertexProgramShadowCache( &temp->xyz, tri->verts, tri->numVerts );

#else

	int numVerts = tri->numVerts;
	const anDrawVertex *verts = tri->verts;
	for ( int i = 0; i < numVerts; i++ ) {
		const float *v = verts[i].xyz.ToFloatPtr();
		temp[i*2+0].xyz[0] = v[0];
		temp[i*2+1].xyz[0] = v[0];
		temp[i*2+0].xyz[1] = v[1];
		temp[i*2+1].xyz[1] = v[1];
		temp[i*2+0].xyz[2] = v[2];
		temp[i*2+1].xyz[2] = v[2];
		temp[i*2+0].xyz[3] = 1.0f;		// on the model surface
		temp[i*2+1].xyz[3] = 0.0f;		// will be projected to infinity
	}

#endif

	vertexCache.Alloc( temp, tri->numVerts * 2 * sizeof( anShadowCache ), &tri->shadowCache );
}

/*
==================
R_SkyboxTexGen
==================
*/
void R_SkyboxTexGen( drawSurf_t *surf, const anVec3 &viewOrg ) {
	anVec3	localViewOrigin;

	R_GlobalPointToLocal( surf->space->modelMatrix, viewOrg, localViewOrigin );

	int numVerts = surf->geo->numVerts;
	int size = numVerts * sizeof( anVec3 );
	anVec3 *texCoords = (anVec3 *) _alloca16( size );

	const anDrawVertex *verts = surf->geo->verts;
	for ( int i = 0; i < numVerts; i++ ) {
		texCoords[i][0] = verts[i].xyz[0] - localViewOrigin[0];
		texCoords[i][1] = verts[i].xyz[1] - localViewOrigin[1];
		texCoords[i][2] = verts[i].xyz[2] - localViewOrigin[2];
	}

	surf->dynamicTexCoords = vertexCache.AllocFrameTemp( texCoords, size );
}

/*
==================
R_WobbleskyTexGen
==================
*/
void R_WobbleskyTexGen( drawSurf_t *surf, const anVec3 &viewOrg ) {
	int		i;
	anVec3	localViewOrigin;

	const int *parms = surf->material->GetTexGenRegisters();

	float	wobbleDegrees = surf->shaderRegisters[ parms[0] ];
	float	wobbleSpeed = surf->shaderRegisters[ parms[1] ];
	float	rotateSpeed = surf->shaderRegisters[ parms[2] ];

	wobbleDegrees = wobbleDegrees * anMath::PI / 180;
	wobbleSpeed = wobbleSpeed * 2 * anMath::PI / 60;
	rotateSpeed = rotateSpeed * 2 * anMath::PI / 60;

	// very ad-hoc "wobble" transform
	float	transform[16];
	float	a = tr.viewDef->floatTime * wobbleSpeed;
	float	s = sin( a ) * sin( wobbleDegrees );
	float	c = cos( a ) * sin( wobbleDegrees );
	float	z = cos( wobbleDegrees );

	anVec3	axis[3];

	axis[2][0] = c;
	axis[2][1] = s;
	axis[2][2] = z;

	axis[1][0] = -sin( a * 2 ) * sin( wobbleDegrees );
	axis[1][2] = -s * sin( wobbleDegrees );
	axis[1][1] = sqrt( 1.0f - ( axis[1][0] * axis[1][0] + axis[1][2] * axis[1][2] ) );

	// make the second vector exactly perpendicular to the first
	axis[1] -= ( axis[2] * axis[1] ) * axis[2];
	axis[1].Normalize();

	// construct the third with a cross
	axis[0].Cross( axis[1], axis[2] );

	// add the rotate
	s = sin( rotateSpeed * tr.viewDef->floatTime );
	c = cos( rotateSpeed * tr.viewDef->floatTime );

	transform[0] = axis[0][0] * c + axis[1][0] * s;
	transform[4] = axis[0][1] * c + axis[1][1] * s;
	transform[8] = axis[0][2] * c + axis[1][2] * s;

	transform[1] = axis[1][0] * c - axis[0][0] * s;
	transform[5] = axis[1][1] * c - axis[0][1] * s;
	transform[9] = axis[1][2] * c - axis[0][2] * s;

	transform[2] = axis[2][0];
	transform[6] = axis[2][1];
	transform[10] = axis[2][2];

	transform[3] = transform[7] = transform[11] = 0.0f;
	transform[12] = transform[13] = transform[14] = 0.0f;

	R_GlobalPointToLocal( surf->space->modelMatrix, viewOrg, localViewOrigin );

	int numVerts = surf->geo->numVerts;
	int size = numVerts * sizeof( anVec3 );
	anVec3 *texCoords = (anVec3 *) _alloca16( size );

	const anDrawVertex *verts = surf->geo->verts;
	for ( i = 0; i < numVerts; i++ ) {
		anVec3 v;

		v[0] = verts[i].xyz[0] - localViewOrigin[0];
		v[1] = verts[i].xyz[1] - localViewOrigin[1];
		v[2] = verts[i].xyz[2] - localViewOrigin[2];

		R_LocalPointToGlobal( transform, v, texCoords[i] );
	}

	surf->dynamicTexCoords = vertexCache.AllocFrameTemp( texCoords, size );
}

/*
=================
R_SpecularTexGen

Calculates the specular coordinates for cards without vertex programs.
=================
*/
static void R_SpecularTexGen( drawSurf_t *surf, const anVec3 &globalLightOrigin, const anVec3 &viewOrg ) {
	const srfTriangles_t *tri;
	anVec3	localLightOrigin;
	anVec3	localViewOrigin;

	R_GlobalPointToLocal( surf->space->modelMatrix, globalLightOrigin, localLightOrigin );
	R_GlobalPointToLocal( surf->space->modelMatrix, viewOrg, localViewOrigin );

	tri = surf->geo;

	// FIXME: change to 3 component?
	int	size = tri->numVerts * sizeof( anVec4 );
	anVec4 *texCoords = (anVec4 *) _alloca16( size );

#if 1
	SIMDProcessor->CreateSpecularTextureCoords( texCoords, localLightOrigin, localViewOrigin, tri->verts, tri->numVerts, tri->indexes, tri->numIndexes );
#else
	bool *used = (bool *)_alloca16( tri->numVerts * sizeof( used[0] ) );
	memset( used, 0, tri->numVerts * sizeof( used[0] ) );

	// because the interaction may be a very small subset of the full surface,
	// it makes sense to only deal with the verts used
	for ( int j = 0; j < tri->numIndexes; j++ ) {
		int i = tri->indexes[j];
		if ( used[i] ) {
			continue;
		}
		used[i] = true;

		float ilength;

		const anDrawVertex *v = &tri->verts[i];

		anVec3 lightDir = localLightOrigin - v->xyz;
		anVec3 viewDir = localViewOrigin - v->xyz;

		ilength = anMath::RSqrt( lightDir * lightDir );
		lightDir[0] *= ilength;
		lightDir[1] *= ilength;
		lightDir[2] *= ilength;

		ilength = anMath::RSqrt( viewDir * viewDir );
		viewDir[0] *= ilength;
		viewDir[1] *= ilength;
		viewDir[2] *= ilength;

		lightDir += viewDir;

		texCoords[i][0] = lightDir * v->tangents[0];
		texCoords[i][1] = lightDir * v->tangents[1];
		texCoords[i][2] = lightDir * v->normal;
		texCoords[i][3] = 1;
	}

#endif
	surf->dynamicTexCoords = vertexCache.AllocFrameTemp( texCoords, size );
}


static void R_RotateLight( anVec3 &target, anVec3 &up, anVec3 &right, const anVec3 &delta ) {
	anVec3 dst, newtarget = target + delta;

	up += target;
	right += target;

	double len = target.Length() * newtarget.Length();

	if ( len > 0.1f ) {
		// calculate the rotation angle between the vectors
		double dp = target * newtarget;
		double dv = dp / len;
		double angle = anMath::RAD2DEG( anMath::ACos( dv ) );
		// get a vector orthogonal to the rotation plane
		anVec3 cross = target.Cross( newtarget );
		cross.Normalize();
		if ( cross[0] || cross[1] || cross[2] ) {
			// build the rotation matrix
			anMat3 rot = anRotation( vec3_origin, cross, angle ).ToMat3();

			rot.ProjectVector( target, dst );
			target = dst;
			rot.ProjectVector( up, dst );
			up = dst;
			rot.ProjectVector( right, dst);
			right = dst;
		}
	}

	// project the up and right vectors onto a plane that goes through the target and
	// has normal vector target.Normalize()
	anVec3 normal = target;
	normal.Normalize();
	double dist = normal * target;

	double d = ( normal * up ) - dist;
	up -= d * normal;

	double d = ( normal * right ) - dist;
	right -= d * normal;

	// FIXME: maybe calculate the right vector with a cross product between the target
	// and up vector, just to make sure the up and right vectors are perpendicular
	// get the up and right vectors relative to the target
	up -= target;
	right -= target;
	target = newtarget;
}
//=======================================================================================================

/*
=============
R_SetEntityDefViewEntity

If the entityDef isn't already on the viewEntity list, create
a viewEntity and add it to the list with an empty scissor rect.

This does not instantiate dynamic models for the entity yet.
=============
*/
viewEntity_t *R_SetEntityDefViewEntity( anRenderEntityLocal *def ) {
	viewEntity_t		*vModel;

	if ( def->viewCount == tr.viewCount ) {
		return def->viewEntity;
	}
	def->viewCount = tr.viewCount;

	// set the model and modelview matricies
	vModel = (viewEntity_t *)R_ClearedFrameAlloc( sizeof( *vModel ) );
	vModel->entityDef = def;

	// the scissorRect will be expanded as the model bounds is accepted into visible portal chains
	vModel->scissorRect.Clear();

	// copy the model and weapon depth hack for back-end use
	vModel->modelDepthHack = def->parms.modelDepthHack;
	vModel->ViewDepthHack = def->parms.ViewDepthHack;

	R_AxisToModelMatrix( def->parms.axis, def->parms.origin, vModel->modelMatrix );

	// we may not have a viewDef if we are just creating shadows at entity creation time
	if ( tr.viewDef ) {
		GL_MultMatrixAligned( vModel->modelMatrix, tr.viewDef->worldSpace.modelViewMatrix, vModel->modelViewMatrix );

		vModel->next = tr.viewDef->viewEntitys;
		tr.viewDef->viewEntitys = vModel;
	}

	def->viewEntity = vModel;

	return vModel;
}

/*
====================
R_TestPointInViewLight
====================
*/
static const float INSIDE_LIGHT_FRUSTUM_SLOP = 32;
// this needs to be greater than the dist from origin to corner of near clip plane
static bool R_TestPointInViewLight( const anVec3 &org, const anRenderLightsLocal *light ) {
	int		i;
	anVec3	local;

	for ( i = 0; i < 6; i++ ) {
		float d = light->frustum[i].Distance( org );
		if ( d > INSIDE_LIGHT_FRUSTUM_SLOP ) {
			return false;
		}
	}

	return true;
}

/*
===================
R_PointInFrustum

Assumes positive sides face outward
===================
*/
static bool R_PointInFrustum( anVec3 &p, anPlane *planes, int numPlanes ) {
	for ( int i = 0; i < numPlanes; i++ ) {
		float d = planes[i].Distance( p );
		if ( d > 0 ) {
			return false;
		}
	}
	return true;
}

/*
=============
R_SetLightDefViewLight

If the lightDef isn't already on the viewLight list, create
a viewLight and add it to the list with an empty scissor rect.
=============
*/
viewLight_t *R_SetLightDefViewLight( anRenderLightsLocal *light ) {
	viewLight_t *vLight;

	if ( light->viewCount == tr.viewCount ) {
		return light->viewLight;
	}
	light->viewCount = tr.viewCount;

	// add to the view light chain
	vLight = (viewLight_t *)R_ClearedFrameAlloc( sizeof( *vLight ) );
	vLight->lightDef = light;

	// the scissorRect will be expanded as the light bounds is accepted into visible portal chains
	vLight->scissorRect.Clear();

	// calculate the shadow cap optimization states
	vLight->viewInsideLight = R_TestPointInViewLight( tr.viewDef->renderView.vieworg, light );
	if ( !vLight->viewInsideLight ) {
		vLight->viewSeesShadowPlaneBits = 0;
		for ( int i = 0; i < light->numShadowFrustums; i++ ) {
			float d = light->shadowFrustums[i].planes[5].Distance( tr.viewDef->renderView.vieworg );
			if ( d < INSIDE_LIGHT_FRUSTUM_SLOP ) {
				vLight->viewSeesShadowPlaneBits|= 1 << i;
			}
		}
	} else {
		// this should not be referenced in this case
		vLight->viewSeesShadowPlaneBits = 63;
	}

	// see if the light center is in view, which will allow us to cull invisible shadows
	vLight->viewSeesGlobalLightOrigin = R_PointInFrustum( light->globalLightOrigin, tr.viewDef->frustum, 4 );

	// copy data used by backend
	vLight->globalLightOrigin = light->globalLightOrigin;
	vLight->lightProject[0] = light->lightProject[0];
	vLight->lightProject[1] = light->lightProject[1];
	vLight->lightProject[2] = light->lightProject[2];
	vLight->lightProject[3] = light->lightProject[3];
	vLight->fogPlane = light->frustum[5];
	vLight->frustumTris = light->frustumTris;
	vLight->falloffImage = light->falloffImage;
	vLight->lightShader = light->lightShader;
	vLight->shaderRegisters = nullptr;		// allocated and evaluated in R_AddLightSurfaces

	// link the view light
	vLight->next = tr.viewDef->viewLights;
	tr.viewDef->viewLights = vLight;

	light->viewLight = vLight;

	return vLight;
}

/*
=================
anRenderWorldLocal::CreateLightDefInteractions

When a lightDef is determined to effect the view (contact the frustum and non-0 light), it will check to
make sure that it has interactions for all the entityDefs that it might possibly contact.

This does not guarantee that all possible interactions for this light are generated, only that
the ones that may effect the current view are generated. so it does need to be called every view.

This does not cause entityDefs to create dynamic models, all work is done on the referenceBounds.

All entities that have non-empty interactions with viewLights will
have viewEntities made for them and be put on the viewEntity list,
even if their surfaces aren't visible, because they may need to cast shadows.

Interactions are usually removed when a entityDef or lightDef is modified, unless the change
is known to not effect them, so there is no danger of getting a stale interaction, we just need to
check that needed ones are created.

An interaction can be at several levels:

Don't interact (but share an area) (numSurfaces = 0 )
Entity reference bounds touches light frustum, but surfaces haven't been generated (numSurfaces = -1 )
Shadow surfaces have been generated, but light surfaces have not.  The shadow surface may still be empty due to bounds being conservative.
Both shadow and light surfaces have been generated.  Either or both surfaces may still be empty due to conservative bounds.

=================
*/
void anRenderWorldLocal::CreateLightDefInteractions( anRenderLightsLocal *ldef ) {
	areaReference_t		*eref;
	areaReference_t		*lref;
	anRenderEntityLocal		*edef;
	portalArea_t	*area;
	anInteraction	*inter;

	for ( lref = ldef->references; lref; lref = lref->ownerNext ) {
		area = lref->area;
		// check all the models in this area
		for ( eref = area->entityRefs.areaNext; eref != &area->entityRefs; eref = eref->areaNext ) {
			edef = eref->entity;

			// if the entity doesn't have any light-interacting surfaces, we could skip this,
			// but we don't want to instantiate dynamic models yet, so we can't check that on
			// most things

			// if the entity isn't viewed
			if ( tr.viewDef && edef->viewCount != tr.viewCount ) {
				// if the light doesn't cast shadows, skip
				if ( !ldef->lightShader->LightCastsShadows() ) {
					continue;
				}
				// if we are suppressing its shadow in this view, skip
				if ( !r_skipSuppress.GetBool() ) {
					if ( edef->parms.suppressShadowInViewID && edef->parms.suppressShadowInViewID == tr.viewDef->renderView.viewID ) {
						continue;
					}
					if ( edef->parms.suppressShadowInLightID && edef->parms.suppressShadowInLightID == ldef->parms.lightId ) {
						continue;
					}
				}
			}

			// some big outdoor meshes are flagged to not create any dynamic interactions
			// when the level designer knows that nearby moving lights shouldn't actually hit them
			if ( edef->parms.noDynamicInteractions && edef->world->generateAllInteractionsCalled ) {
				continue;
			}

			// if any of the edef's interaction match this light, we don't
			// need to consider it.
			if ( r_useInteractionTable.GetBool() && this->interactionTable ) {
				// allocating these tables may take several megs on big maps, but it saves 3% to 5% of
				// the CPU time.  The table is updated at interaction::AllocAndLink() and interaction::UnlinkAndFree()
				int index = ldef->index * this->interactionTableWidth + edef->index;
				inter = this->interactionTable[index];
				if ( inter ) {
					// if this entity wasn't in view already, the scissor rect will be empty,
					// so it will only be used for shadow casting
					if ( !inter->IsEmpty() ) {
						R_SetEntityDefViewEntity( edef );
					}
					continue;
				}
			} else {
				// scan the doubly linked lists, which may have several dozen entries
				// we could check either model refs or light refs for matches, but it is
				// assumed that there will be less lights in an area than models
				// so the entity chains should be somewhat shorter (they tend to be fairly close).
				for ( inter = edef->firstInteraction; inter != nullptr; inter = inter->entityNext ) {
					if ( inter->lightDef == ldef ) {
						break;
					}
				}

				// if we already have an interaction, we don't need to do anything
				if ( inter != nullptr ) {
					// if this entity wasn't in view already, the scissor rect will be empty,
					// so it will only be used for shadow casting
					if ( !inter->IsEmpty() ) {
						R_SetEntityDefViewEntity( edef );
					}
					continue;
				}
			}

			//
			// create a new interaction, but don't do any work other than bbox to frustum culling
			//
			anInteraction *inter = anInteraction::AllocAndLink( edef, ldef );

			// do a check of the entity reference bounds against the light frustum,
			// trying to avoid creating a viewEntity if it hasn't been already
			float	modelMatrix[16];
			float	*m;

			if ( edef->viewCount == tr.viewCount ) {
				m = edef->viewEntity->modelMatrix;
			} else {
				R_AxisToModelMatrix( edef->parms.axis, edef->parms.origin, modelMatrix );
				m = modelMatrix;
			}

			if ( R_CullLocalBox( edef->referenceBounds, m, 6, ldef->frustum ) ) {
				inter->MakeEmpty();
				continue;
			}

			// we will do a more precise per-surface check when we are checking the entity

			// if this entity wasn't in view already, the scissor rect will be empty,
			// so it will only be used for shadow casting
			R_SetEntityDefViewEntity( edef );
		}
	}
}

//===============================================================================================================

/*
=================
R_LinkLightSurf
=================
*/
void R_LinkLightSurf( const drawSurf_t **link, const srfTriangles_t *tri, const viewEntity_t *space, const anRenderLightsLocal *light, const anMaterial *shader, const anScreenRect &scissor, bool viewInsideShadow ) {
	if ( !space ) {
		space = &tr.viewDef->worldSpace;
	}

	drawSurf_t *drawSurf = (drawSurf_t *)R_FrameAlloc( sizeof( *drawSurf ) );

	drawSurf->geo = tri;
	drawSurf->space = space;
	drawSurf->material = shader;
	drawSurf->scissorRect = scissor;
	drawSurf->dsFlags = 0;
	if ( viewInsideShadow ) {
		drawSurf->dsFlags |= DSF_VIEW_INSIDE_SHADOW;
	}

	if ( !shader ) {
		// shadows won't have a shader
		drawSurf->shaderRegisters = nullptr;
	} else {
		// process the shader expressions for conditionals / color / texcoords
		const float *constRegs = shader->ConstantRegisters();
		if ( constRegs ) {
			// this shader has only constants for parameters
			drawSurf->shaderRegisters = constRegs;
		} else {
			// FIXME: share with the ambient surface?
			float *regs = (float *)R_FrameAlloc( shader->GetNumRegisters() * sizeof( float ) );
			drawSurf->shaderRegisters = regs;
			shader->EvaluateRegisters( regs, space->entityDef->parms.shaderParms, tr.viewDef, space->entityDef->parms.sndRef );
		}

		// calculate the specular coordinates if we aren't using vertex programs
		if ( !tr.backEndRendererHasVertexPrograms && !r_skipSpecular.GetBool() && tr.backEndRenderer != BE_ARB ) {
			R_SpecularTexGen( drawSurf, light->globalLightOrigin, tr.viewDef->renderView.vieworg );
			// if we failed to allocate space for the specular calculations, drop the surface
			if ( !drawSurf->dynamicTexCoords ) {
				return;
			}
		}
	}

	// actually link it in
	drawSurf->nextOnLight = *link;
	*link = drawSurf;
}

/*
======================
R_ClippedLightScissorRectangle
======================
*/
anScreenRect R_ClippedLightScissorRectangle( viewLight_t *vLight ) {
	int i, j;
	const anRenderLightsLocal *light = vLight->lightDef;
	anScreenRect r;

	r.Clear();
	for ( i = 0; i < 6; i++ ) {
		const anWinding *ow = light->frustumWindings[i];
		// projected lights may have one of the frustums degenerated
		if ( !ow ) {
			continue;
		}

		// the light frustum planes face out from the light,
		// so the planes that have the view origin on the negative
		// side will be the "back" faces of the light, which must have
		// some fragment inside the portalStack to be visible
		if ( light->frustum[i].Distance( tr.viewDef->renderView.vieworg ) >= 0 ) {
			continue;
		}

		anFixedWinding w = *ow;

		// now check the winding against each of the frustum planes
		for ( j = 0; j < 5; j++ ) {
			if ( !w.ClipInPlace( -tr.viewDef->frustum[j] ) ) {
				break;
			}
		}

		// project these points to the screen and add to bounds
		for ( j = 0; j < w.GetNumPoints(); j++ ) {
			anPlane		eye, clip;
			anVec3		ndc;

			R_TransformModelToClip( w[j].ToVec3(), tr.viewDef->worldSpace.modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip );

			if ( clip[3] <= 0.01f ) {
				clip[3] = 0.01f;
			}

			R_TransformClipToDevice( clip, tr.viewDef, ndc );

			float windowX = 0.5f * ( 1.0f + ndc[0] ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 );
			float windowY = 0.5f * ( 1.0f + ndc[1] ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 );

			if ( windowX > tr.viewDef->scissor.x2 ) {
				windowX = tr.viewDef->scissor.x2;
			} else if ( windowX < tr.viewDef->scissor.x1 ) {
				windowX = tr.viewDef->scissor.x1;
			}
			if ( windowY > tr.viewDef->scissor.y2 ) {
				windowY = tr.viewDef->scissor.y2;
			} else if ( windowY < tr.viewDef->scissor.y1 ) {
				windowY = tr.viewDef->scissor.y1;
			}

			r.AddPoint( windowX, windowY );
		}
	}

	// add the fudge boundary
	r.Expand();

	return r;
}

/*
==================
R_CalcLightScissorRectangle

The light screen bounds will be used to crop the scissor rect during
stencil clears and interaction drawing
==================
*/
int	c_clippedLight, c_unclippedLight;
anScreenRect	R_CalcLightScissorRectangle( viewLight_t *vLight ) {
	anScreenRect r;
	srfTriangles_t *tri;
	anPlane eye, clip;
	anVec3 ndc;

	if ( vLight->lightDef->parms.pointLight ) {
		anBounds bounds;
		anRenderLightsLocal *lightDef = vLight->lightDef;
		tr.viewDef->viewFrustum.ProjectionBounds( anBox( lightDef->parms.origin, lightDef->parms.lightRadius, lightDef->parms.axis ), bounds );
		return R_ScreenRectFromViewFrustumBounds( bounds );
	}

	if ( r_useClippedLightScissors.GetInteger() == 2 ) {
		return R_ClippedLightScissorRectangle( vLight );
	}

	r.Clear();

	tri = vLight->lightDef->frustumTris;
	for ( int i = 0; i < tri->numVerts; i++ ) {
		R_TransformModelToClip( tri->verts[i].xyz, tr.viewDef->worldSpace.modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip );
		// if it is near clipped, clip the winding polygons to the view frustum
		if ( clip[3] <= 1 ) {
			c_clippedLight++;
			if ( r_useClippedLightScissors.GetInteger() ) {
				return R_ClippedLightScissorRectangle( vLight );
			} else {
				r.x1 = r.y1 = 0;
				r.x2 = ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 ) - 1;
				r.y2 = ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 ) - 1;
				return r;
			}
		}

		R_TransformClipToDevice( clip, tr.viewDef, ndc );

		float windowX = 0.5f * ( 1.0f + ndc[0] ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 );
		float windowY = 0.5f * ( 1.0f + ndc[1] ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 );

		if ( windowX > tr.viewDef->scissor.x2 ) {
			windowX = tr.viewDef->scissor.x2;
		} else if ( windowX < tr.viewDef->scissor.x1 ) {
			windowX = tr.viewDef->scissor.x1;
		}
		if ( windowY > tr.viewDef->scissor.y2 ) {
			windowY = tr.viewDef->scissor.y2;
		} else if ( windowY < tr.viewDef->scissor.y1 ) {
			windowY = tr.viewDef->scissor.y1;
		}

		r.AddPoint( windowX, windowY );
	}

	// add the fudge boundary
	r.Expand();

	c_unclippedLight++;

	return r;
}

/*
=================
R_AddLightSurfaces

Calc the light shader values, removing any light from the viewLight list
if it is determined to not have any visible effect due to being flashed off or turned off.

Adds entities to the viewEntity list if they are needed for shadow casting.

Add any precomputed shadow volumes.

Removes lights from the viewLights list if they are completely
turned off, or completely off screen.

Create any new interactions needed between the viewLights
and the viewEntitys due to game movement
=================
*/
void R_AddLightSurfaces( void ) {
	viewLight_t		*vLight;
	anRenderLightsLocal *light;
	viewLight_t		**ptr;

	// go through each visible light, possibly removing some from the list
	ptr = &tr.viewDef->viewLights;
	while ( *ptr ) {
		vLight = *ptr;
		light = vLight->lightDef;

		const anMaterial *lightShader = light->lightShader;
		if ( !lightShader ) {
			common->Error( "R_AddLightSurfaces: nullptr lightShader" );
		}

		// see if we are suppressing the light in this view
		if ( !r_skipSuppress.GetBool() ) {
			if ( light->parms.suppressLightInViewID
			&& light->parms.suppressLightInViewID == tr.viewDef->renderView.viewID ) {
				*ptr = vLight->next;
				light->viewCount = -1;
				continue;
			}
			if ( light->parms.allowLightInViewID
			&& light->parms.allowLightInViewID != tr.viewDef->renderView.viewID ) {
				*ptr = vLight->next;
				light->viewCount = -1;
				continue;
			}
		}

		// evaluate the light shader registers
		float *lightRegs =(float *)R_FrameAlloc( lightShader->GetNumRegisters() * sizeof( float ) );
		vLight->shaderRegisters = lightRegs;
		lightShader->EvaluateRegisters( lightRegs, light->parms.shaderParms, tr.viewDef, light->parms.sndRef );

		// if this is a purely additive light and no stage in the light shader evaluates
		// to a positive light value, we can completely skip the light
		if ( !lightShader->IsFogLight() && !lightShader->IsBlendLight() ) {
			int lightStageNum;
			for ( lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ ) {
				const materialStage_t	*lightStage = lightShader->GetStage( lightStageNum );
				// ignore stages that fail the condition
				if ( !lightRegs[ lightStage->conditionRegister ] ) {
					continue;
				}

				const int *registers = lightStage->color.registers;

				// snap tiny values to zero to avoid lights showing up with the wrong color
				if ( lightRegs[ registers[0] ] < 0.001f ) {
					lightRegs[ registers[0] ] = 0.0f;
				}
				if ( lightRegs[ registers[1] ] < 0.001f ) {
					lightRegs[ registers[1] ] = 0.0f;
				}
				if ( lightRegs[ registers[2] ] < 0.001f ) {
					lightRegs[ registers[2] ] = 0.0f;
				}

				// FIXME:	when using the following values the light shows up bright red when using nvidia drivers/hardware
				//			this seems to have been fixed ?
				//lightRegs[ registers[0] ] = 1.5143074e-005f;
				//lightRegs[ registers[1] ] = 1.5483369e-005f;
				//lightRegs[ registers[2] ] = 1.7014690e-005f;

				if ( lightRegs[ registers[0] ] > 0.0f ||
					lightRegs[ registers[1] ] > 0.0f ||
					lightRegs[ registers[2] ] > 0.0f ) {
					break;
				}
			}
			if ( lightStageNum == lightShader->GetNumStages() ) {
				// we went through all the stages and didn't find one that adds anything
				// remove the light from the viewLights list, and change its frame marker
				// so interaction generation doesn't think the light is visible and
				// create a shadow for it
				*ptr = vLight->next;
				light->viewCount = -1;
				continue;
			}
		}

		if ( r_useLightScissors.GetBool() ) {
			// calculate the screen area covered by the light frustum
			// which will be used to crop the stencil cull
			anScreenRect scissorRect = R_CalcLightScissorRectangle( vLight );
			// intersect with the portal crossing scissor rectangle
			vLight->scissorRect.Intersect( scissorRect );

			if ( r_showLightScissors.GetBool() ) {
				R_ShowColoredScreenRect( vLight->scissorRect, light->index );
			}
		}

#if 0
		// this never happens, because CullLightByPortals() does a more precise job
		if ( vLight->scissorRect.IsEmpty() ) {
			// this light doesn't touch anything on screen, so remove it from the list
			*ptr = vLight->next;
			continue;
		}
#endif

		// this one stays on the list
		ptr = &vLight->next;

		// if we are doing a soft-shadow novelty test, regenerate the light with
		// a random offset every time
		if ( r_lightSourceRadius.GetFloat() != 0.0f ) {
			for ( int i = 0; i < 3; i++ ) {
				light->globalLightOrigin[i] += r_lightSourceRadius.GetFloat() * ( -1 + 2 * (rand()&0xfff)/( float )0xfff );
			}
		}

		// create interactions with all entities the light may touch, and add viewEntities
		// that may cast shadows, even if they aren't directly visible.  Any real work
		// will be deferred until we walk through the viewEntities
		tr.viewDef->renderWorld->CreateLightDefInteractions( light );
		tr.pc.c_viewLights++;

		// fog lights will need to draw the light frustum triangles, so make sure they
		// are in the vertex cache
		if ( lightShader->IsFogLight() ) {
			if ( !light->frustumTris->ambientCache ) {
				if ( !R_CreateAmbientCache( light->frustumTris, false ) ) {
					// skip if we are out of vertex memory
					continue;
				}
			}
			// touch the surface so it won't get purged
			vertexCache.Touch( light->frustumTris->ambientCache );
		}

		// add the prelight shadows for the static world geometry
		if ( light->parms.prelightModel && r_useOptimizedShadows.GetBool() ) {
			if ( !light->parms.prelightModel->NumSurfaces() ) {
				common->Error( "no surfs in prelight model '%s'", light->parms.prelightModel->Name() );
			}

			srfTriangles_t	*tri = light->parms.prelightModel->Surface( 0 )->geometry;
			if ( !tri->shadowVertexes ) {
				common->Error( "R_AddLightSurfaces: prelight model '%s' without shadowVertexes", light->parms.prelightModel->Name() );
			}

			// these shadows will all have valid bounds, and can be culled normally
			if ( r_useShadowCulling.GetBool() ) {
				if ( R_CullLocalBox( tri->bounds, tr.viewDef->worldSpace.modelMatrix, 5, tr.viewDef->frustum ) ) {
					continue;
				}
			}

			// if we have been purged, re-upload the shadowVertexes
			if ( !tri->shadowCache ) {
				R_CreatePrivateShadowCache( tri );
				if ( !tri->shadowCache ) {
					continue;
				}
			}

			// touch the shadow surface so it won't get purged
			vertexCache.Touch( tri->shadowCache );

			if ( !tri->indexCache && r_useIndexBuffers.GetBool() ) {
				vertexCache.Alloc( tri->indexes, tri->numIndexes * sizeof( tri->indexes[0] ), &tri->indexCache, true );
			}
			if ( tri->indexCache ) {
				vertexCache.Touch( tri->indexCache );
			}

			R_LinkLightSurf( &vLight->globalShadows, tri, nullptr, light, nullptr, vLight->scissorRect, true /* FIXME? */ );
		}
	}
}

//===============================================================================================================

/*
==================
R_IssueEntityDefCallback
==================
*/
bool R_IssueEntityDefCallback( anRenderEntityLocal *def ) {
	if ( r_checkBounds.GetBool() ) {
		anBounds oldBounds = def->referenceBounds;
	}

	def->archived = false;		// will need to be written to the demo file
	tr.pc.entityDefCallbacks++;
	if ( tr.viewDef ) {
		bool update = def->parms.callback( &def->parms, &tr.viewDef->renderView );
	} else {
		bool update = def->parms.callback( &def->parms, nullptr );
	}

	if ( !def->parms.hModel ) {
		common->Error( "R_IssueEntityDefCallback: dynamic entity callback didn't set model" );
	}

	if ( r_checkBounds.GetBool() ) {
		if (	oldBounds[0][0] > def->referenceBounds[0][0] + CHECK_BOUNDS_EPSILON ||
				oldBounds[0][1] > def->referenceBounds[0][1] + CHECK_BOUNDS_EPSILON ||
				oldBounds[0][2] > def->referenceBounds[0][2] + CHECK_BOUNDS_EPSILON ||
				oldBounds[1][0] < def->referenceBounds[1][0] - CHECK_BOUNDS_EPSILON ||
				oldBounds[1][1] < def->referenceBounds[1][1] - CHECK_BOUNDS_EPSILON ||
				oldBounds[1][2] < def->referenceBounds[1][2] - CHECK_BOUNDS_EPSILON ) {
			common->Printf( "entity %i callback extended reference bounds\n", def->index );
		}
	}

	return update;
}
/*
void R_FreeEntityDefDerivedData( struct idRendererSystem *renderSys, struct viewEntity_t *ent ) {
    if ( ent->mesh ) {
        FreeDefs( ent->mesh );
    }
    for ( int i = 0; i < ent->numDrawSurfs; i++ ) {
        if ( ent->localModels[i] ) {
            R_FreeMesh( ent->submesh[i] );
        }
    }
    Mem_Free( ent->submesh );
    Mem_Free( ent );
}

void R_ViewEntityDefDynamicModel( } renderEntity_t *def, const renderView_t *renderView ) {
	if ( def == tr.viewDef->viewEntities ) {
		return;
	}

	//if ( tr.viewDef->isXraySubview && def->parms.xrayIndex == 2 ) {
		//return;
	//}
	//if ( !tr.viewDef->isXraySubview && def->parms.xrayIndex == 1 ) {
		//return;
	//}

	//R_FreeEntityDefDerivedData( def, false, false );
	if ( def->parms.hModel == nullptr ) {
		return;
	}

	anRenderModel *model = R_EntityDefModelHandle( def->entityDef );
	if ( model == nullptr || model->IsDynamicModel() == DM_STATIC ) {
		return;
	}

	// set the local origin and axis
	anVec3 localViewOrigin, localViewAxis[3];
	anAngles localViewAngles;
	for ( int i = 0; i < 3; i++ ) {
		localViewAxis[i] = renderView->viewAxis[i] * def->entityDef->parms.axis[i];
		localViewOrigin[i] = renderView->vieworg[i] - def->entityDef->parms.origin[i];
	}
	localViewAngles = renderView->viewangles;

	anBounds localBounds
}
*/
/*
===================
R_EntityDefDynamicModel

Issues a deferred entity callback if necessary.
If the model isn't dynamic, it returns the original.
Returns the cached dynamic model if present, otherwise creates
it and any necessary overlays
===================
*/
anRenderModel *R_EntityDefDynamicModel( anRenderEntityLocal *def ) {
	// allow deferred entities to construct themselves
	if ( def->parms.callback ) {
		bool callbackUpdate = R_IssueEntityDefCallback( def );
	} else {
		bool callbackUpdate = false;
	}

	anRenderModel *model = def->parms.hModel;

	if ( !model ) {
		common->Error( "R_EntityDefDynamicModel: nullptr model" );
	}

	if ( model->IsDynamicModel() == DM_STATIC ) {
		def->dynamicModel = nullptr;
		def->dynamicModelFrameCount = 0;
		return model;
	}

	// continously animating models (particle systems, etc) will have their snapshot updated every single view
	if ( callbackUpdate || ( model->IsDynamicModel() == DM_CONTINUOUS && def->dynamicModelFrameCount != tr.frameCount ) ) {
		R_ClearEntityDefDynamicModel( def );
	}

	// if we don't have a snapshot of the dynamic model, generate it now
	if ( !def->dynamicModel ) {
		// instantiate the snapshot of the dynamic model, possibly reusing memory from the cached snapshot
		def->cachedDynamicModel = model->InstantiateDynamicModel( &def->parms, tr.viewDef, def->cachedDynamicModel );
		if ( def->cachedDynamicModel ) {
			// add any overlays to the snapshot of the dynamic model
			if ( def->overlay && !r_skipOverlays.GetBool() ) {
				def->overlay->AddOverlaySurfacesToModel( def->cachedDynamicModel );
			} else {
				anRenderModelOverlay::RemoveOverlaySurfacesFromModel( def->cachedDynamicModel );
			}

			if ( r_checkBounds.GetBool() ) {
				anBounds b = def->cachedDynamicModel->Bounds();
				if (	b[0][0] < def->referenceBounds[0][0] - CHECK_BOUNDS_EPSILON ||
						b[0][1] < def->referenceBounds[0][1] - CHECK_BOUNDS_EPSILON ||
						b[0][2] < def->referenceBounds[0][2] - CHECK_BOUNDS_EPSILON ||
						b[1][0] > def->referenceBounds[1][0] + CHECK_BOUNDS_EPSILON ||
						b[1][1] > def->referenceBounds[1][1] + CHECK_BOUNDS_EPSILON ||
						b[1][2] > def->referenceBounds[1][2] + CHECK_BOUNDS_EPSILON ) {
					common->Printf( "entity %i dynamic model exceeded reference bounds\n", def->index );
				}
			}
		}

		def->dynamicModel = def->cachedDynamicModel;
		def->dynamicModelFrameCount = tr.frameCount;
	}

	// set model depth hack value
	if ( def->dynamicModel && model->DepthHack() != 0.0f && tr.viewDef ) {
		anPlane eye, clip;
		anVec3 ndc;
		R_TransformModelToClip( def->parms.origin, tr.viewDef->worldSpace.modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip );
		R_TransformClipToDevice( clip, tr.viewDef, ndc );
		def->parms.modelDepthHack = model->DepthHack() * ( 1.0f - ndc.z );
	}

	// FIXME: if any of the surfaces have deforms, create a frame-temporary model with references to the
	// undeformed surfaces.  This would allow deforms to be light interacting.

	return def->dynamicModel;
}

/*
=================
R_AddDrawSurf
=================
*/
void R_AddDrawSurf( const srfTriangles_t *tri, const viewEntity_t *space, const renderEntity_t *renderEntity, const anMaterial *shader, const anScreenRect &scissor ) {
	drawSurf_t		*drawSurf;
	const float		*shaderParms;
	static float	refRegs[MAX_EXP_REGS];	// don't put on stack, or VC++ will do a page touch
	float			generatedShaderParms[MAX_ENTITY_SHADER_PARMS];

	drawSurf = (drawSurf_t *)R_FrameAlloc( sizeof( *drawSurf ) );
	drawSurf->geo = tri;
	drawSurf->space = space;
	drawSurf->material = shader;
	drawSurf->scissorRect = scissor;
	drawSurf->sort = shader->GetSort() + tr.sortOffset;
	drawSurf->dsFlags = 0;

	// bumping this offset each time causes surfaces with equal sort orders to still
	// deterministically draw in the order they are added
	tr.sortOffset += 0.000001f;

	// if it doesn't fit, resize the list
	if ( tr.viewDef->numDrawSurfs == tr.viewDef->maxDrawSurfs ) {
		drawSurf_t **old = tr.viewDef->drawSurfs;
		if ( tr.viewDef->maxDrawSurfs == 0 ) {
			tr.viewDef->maxDrawSurfs = INITIAL_DRAWSURFS;
			int count = 0;
		} else {
			count = tr.viewDef->maxDrawSurfs * sizeof( tr.viewDef->drawSurfs[0] );
			tr.viewDef->maxDrawSurfs *= 2;
		}
		tr.viewDef->drawSurfs = (drawSurf_t **)R_FrameAlloc( tr.viewDef->maxDrawSurfs * sizeof( tr.viewDef->drawSurfs[0] ) );
		memcpy( tr.viewDef->drawSurfs, old, count );
	}
	tr.viewDef->drawSurfs[tr.viewDef->numDrawSurfs] = drawSurf;
	tr.viewDef->numDrawSurfs++;

	// process the shader expressions for conditionals / color / texcoords
	const float	*constRegs = shader->ConstantRegisters();
	if ( constRegs ) {
		// shader only uses constant values
		drawSurf->shaderRegisters = constRegs;
	} else {
		float *regs = (float *)R_FrameAlloc( shader->GetNumRegisters() * sizeof( float ) );
		drawSurf->shaderRegisters = regs;

		// a reference shader will take the calculated stage color value from another shader
		// and use that for the parm0-parm3 of the current shader, which allows a stage of
		// a light model and light flares to pick up different flashing tables from
		// different light shaders
		if ( renderEntity->shaderRef ) {
			// evaluate the reference shader to find our shader parms
			const materialStage_t *pStage;

			renderEntity->shaderRef->EvaluateRegisters( refRegs, renderEntity->shaderParms, tr.viewDef, renderEntity->sndRef );
			pStage = renderEntity->shaderRef->GetStage(0 );

			memcpy( generatedShaderParms, renderEntity->shaderParms, sizeof( generatedShaderParms ) );
			generatedShaderParms[0] = refRegs[ pStage->color.registers[0] ];
			generatedShaderParms[1] = refRegs[ pStage->color.registers[1] ];
			generatedShaderParms[2] = refRegs[ pStage->color.registers[2] ];

			shaderParms = generatedShaderParms;
		} else {
			// evaluate with the entityDef's shader parms
			shaderParms = renderEntity->shaderParms;
		}

		if ( space->entityDef && space->entityDef->parms.timeGroup ) {
			float oldFloatTime = tr.viewDef->floatTime;
			int oldTime = tr.viewDef->renderView.time;

			tr.viewDef->floatTime = game->GetTimeGroupTime( space->entityDef->parms.timeGroup ) * 0.001;
			tr.viewDef->renderView.time = game->GetTimeGroupTime( space->entityDef->parms.timeGroup );
		}

		shader->EvaluateRegisters( regs, shaderParms, tr.viewDef, renderEntity->sndRef );

		if ( space->entityDef && space->entityDef->parms.timeGroup ) {
			tr.viewDef->floatTime = oldFloatTime;
			tr.viewDef->renderView.time = oldTime;
		}
	}

	// check for deformations
	R_DeformDrawSurf( drawSurf );

	// skybox surfaces need a dynamic texgen
	switch ( shader->Texgen() ) {
		case TG_SKYBOX_CUBE:
			R_SkyboxTexGen( drawSurf, tr.viewDef->renderView.vieworg );
			break;
		case TG_WOBBLESKY_CUBE:
			R_WobbleskyTexGen( drawSurf, tr.viewDef->renderView.vieworg );
			break;
	}

	// check for gui surfaces
	anUserInterfaces *gui = nullptr;

	if ( !space->entityDef ) {
		gui = shader->GlobalGui();
	} else {
		int guiNum = shader->GetEntityGui() - 1;
		if ( guiNum >= 0 && guiNum < MAX_RENDERENTITY_GUI ) {
			gui = renderEntity->gui[ guiNum ];
		}
		if ( gui == nullptr ) {
			gui = shader->GlobalGui();
		}
	}

	if ( gui ) {
		// force guis on the fast time
		float oldFloatTime = tr.viewDef->floatTime;
		int oldTime = tr.viewDef->renderView.time;

		tr.viewDef->floatTime = game->GetTimeGroupTime( 1 ) * 0.001;
		tr.viewDef->renderView.time = game->GetTimeGroupTime( 1 );

		anBounds ndcBounds;

		if ( !R_PreciseCullSurface( drawSurf, ndcBounds ) ) {
			// did we ever use this to forward an entity color to a gui that didn't set color?
//			memcpy( tr.guiShaderParms, shaderParms, sizeof( tr.guiShaderParms ) );
			R_RenderGuiSurf( gui, drawSurf );
		}

		tr.viewDef->floatTime = oldFloatTime;
		tr.viewDef->renderView.time = oldTime;
	}
	// we can't add subviews at this point, because that would
	// increment tr.viewCount, messing up the rest of the surface
	// adds for this view
}

/*
===============
R_AddAmbientDrawsurfs

Adds surfaces for the given viewEntity
Walks through the viewEntitys list and creates drawSurf_t for each surface of
each viewEntity that has a non-empty scissorRect
===============
*/
static void R_AddAmbientDrawsurfs( viewEntity_t *vEntity ) {
	anRenderEntityLocal *def = vEntity->entityDef;

	if ( def->dynamicModel ) {
		anRenderModel *model = def->dynamicModel;
	} else {
		anRenderModel *model = def->parms.hModel;
	}

	// add all the surfaces
	int total = model->NumSurfaces();
	for ( int i = 0; i < total; i++ ) {
		const modelSurface_t *surf = model->Surface( i );
		// for debugging, only show a single surface at a time
		if ( r_singleSurface.GetInteger() >= 0 && i != r_singleSurface.GetInteger() ) {
			continue;
		}

		srfTriangles_t *tri = surf->geometry;
		if ( !tri ) {
			continue;
		}
		if ( !tri->numIndexes ) {
			continue;
		}
		const anMaterial *shader = surf->shader;
		shader = R_RemapShaderBySkin( shader, def->parms.customSkin, def->parms.customShader );

		R_GlobalShaderOverride( &shader );

		if ( !shader ) {
			continue;
		}
		if ( !shader->IsDrawn() ) {
			continue;
		}

		// debugging tool to make sure we are have the correct pre-calculated bounds
		if ( r_checkBounds.GetBool() ) {
			for ( int j = 0; j < tri->numVerts; j++ ) {
				for ( int k = 0; k < 3; k++ ) {
					if ( tri->verts[j].xyz[k] > tri->bounds[1][k] + CHECK_BOUNDS_EPSILON
						|| tri->verts[j].xyz[k] < tri->bounds[0][k] - CHECK_BOUNDS_EPSILON ) {
						common->Printf( "bad tri->bounds on %s:%s\n", def->parms.hModel->Name(), shader->GetName() );
						break;
					}
					if ( tri->verts[j].xyz[k] > def->referenceBounds[1][k] + CHECK_BOUNDS_EPSILON
						|| tri->verts[j].xyz[k] < def->referenceBounds[0][k] - CHECK_BOUNDS_EPSILON ) {
						common->Printf( "bad referenceBounds on %s:%s\n", def->parms.hModel->Name(), shader->GetName() );
						break;
					}
				}
				if ( k != 3 ) {
					break;
				}
			}
		}

		if ( !R_CullLocalBox( tri->bounds, vEntity->modelMatrix, 5, tr.viewDef->frustum ) ) {
			def->visibleCount = tr.viewCount;
			// make sure we have an ambient cache
			if ( !R_CreateAmbientCache( tri, shader->ReceivesLighting() ) ) {
				// don't add anything if the vertex cache was too full to give us an ambient cache
				return;
			}
			// touch it so it won't get purged
			vertexCache.Touch( tri->ambientCache );

			if ( r_useIndexBuffers.GetBool() && !tri->indexCache ) {
				vertexCache.Alloc( tri->indexes, tri->numIndexes * sizeof( tri->indexes[0] ), &tri->indexCache, true );
			}
			if ( tri->indexCache ) {
				vertexCache.Touch( tri->indexCache );
			}

			// add the surface for drawing
			R_AddDrawSurf( tri, vEntity, &vEntity->entityDef->parms, shader, vEntity->scissorRect );

			// ambientViewCount is used to allow light interactions to be rejected
			// if the ambient surface isn't visible at all
			tri->ambientViewCount = tr.viewCount;
		}
	}

	// add the lightweight decal surfaces
	for ( anRenderModelDecal *decal = def->decals; decal; decal = decal->Next() ) {
		decal->AddDecalDrawSurf( vEntity );
	}
}

/*
==================
R_CalcEntityScissorRectangle
==================
*/
anScreenRect R_CalcEntityScissorRectangle( viewEntity_t *vEntity ) {
	anBounds bounds;
	anRenderEntityLocal *def = vEntity->entityDef;
	tr.viewDef->viewFrustum.ProjectionBounds( anBox( def->referenceBounds, def->parms.origin, def->parms.axis ), bounds );
	return R_ScreenRectFromViewFrustumBounds( bounds );
}

/*
===================
R_AddModelSurfaces

Here is where dynamic models actually get instantiated, and necessary
interactions get created.  This is all done on a sort-by-model basis
to keep source data in cache (most likely L2) as any interactions and
shadows are generated, since dynamic models will typically be lit by
two or more lights.
===================
*/
void R_AddModelSurfaces( void ) {
	// clear the ambient surface list
	tr.viewDef->numDrawSurfs = 0;
	tr.viewDef->maxDrawSurfs = 0;	// will be set to INITIAL_DRAWSURFS on R_AddDrawSurf

	// go through each entity that is either visible to the view, or to
	// any light that intersects the view (for shadows)
	for ( viewEntity_t *vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next ) {
		if ( r_useEntityScissors.GetBool() ) {
			// calculate the screen area covered by the entity
			anScreenRect scissorRect = R_CalcEntityScissorRectangle( vEntity );
			// intersect with the portal crossing scissor rectangle
			vEntity->scissorRect.Intersect( scissorRect );
			if ( r_showEntityScissors.GetBool() ) {
				R_ShowColoredScreenRect( vEntity->scissorRect, vEntity->entityDef->index );
			}
		}

		float oldFloatTime;
		int oldTime;

		game->SelectTimeGroup( vEntity->entityDef->parms.timeGroup );

		if ( vEntity->entityDef->parms.timeGroup ) {
			oldFloatTime = tr.viewDef->floatTime;
			oldTime = tr.viewDef->renderView.time;

			tr.viewDef->floatTime = game->GetTimeGroupTime( vEntity->entityDef->parms.timeGroup ) * 0.001;
			tr.viewDef->renderView.time = game->GetTimeGroupTime( vEntity->entityDef->parms.timeGroup );
		}

		if ( tr.viewDef->isXraySubview && vEntity->entityDef->parms.xrayIndex == 1 ) {
			if ( vEntity->entityDef->parms.timeGroup ) {
				tr.viewDef->floatTime = oldFloatTime;
				tr.viewDef->renderView.time = oldTime;
			}
			continue;
		} else if ( !tr.viewDef->isXraySubview && vEntity->entityDef->parms.xrayIndex == 2 ) {
			if ( vEntity->entityDef->parms.timeGroup ) {
				tr.viewDef->floatTime = oldFloatTime;
				tr.viewDef->renderView.time = oldTime;
			}
			continue;
		}

		// add the ambient surface if it has a visible rectangle
		if ( !vEntity->scissorRect.IsEmpty() ) {
			anRenderModel *model = R_EntityDefDynamicModel( vEntity->entityDef );
			if ( model == nullptr || model->NumSurfaces() <= 0 ) {
				if ( vEntity->entityDef->parms.timeGroup ) {
					tr.viewDef->floatTime = oldFloatTime;
					tr.viewDef->renderView.time = oldTime;
				}
				continue;
			}

			R_AddAmbientDrawsurfs( vEntity );
			tr.pc.visibleViewEntities++;
		} else {
			tr.pc.c_shadowViewEntities++;
		}

		//
		// for all the entity / light interactions on this entity, add them to the view
		//
		if ( tr.viewDef->isXraySubview ) {
			if ( vEntity->entityDef->parms.xrayIndex == 2 ) {
				for ( anInteraction *inter = vEntity->entityDef->firstInteraction; inter != nullptr && !inter->IsEmpty(); inter = next ) {
						anInteraction *next = inter->entityNext;
					if ( inter->lightDef->viewCount != tr.viewCount ) {
						continue;
					}
					inter->AddActiveInteraction();
				}
			}
		} else {
			// all empty interactions are at the end of the list so once the
			// first is encountered all the remaining interactions are empty
			for ( anInteraction *inter = vEntity->entityDef->firstInteraction; inter != nullptr && !inter->IsEmpty(); inter = next ) {
				anInteraction *next = inter->entityNext;

				// skip any lights that aren't currently visible
				// this is run after any lights that are turned off have already
				// been removed from the viewLights list, and had their viewCount cleared
				if ( inter->lightDef->viewCount != tr.viewCount ) {
					continue;
				}
				inter->AddActiveInteraction();
			}
		}

		if ( vEntity->entityDef->parms.timeGroup ) {
			tr.viewDef->floatTime = oldFloatTime;
			tr.viewDef->renderView.time = oldTime;
		}

	}
}

/*
=====================
R_RemoveUnecessaryViewLights
=====================
*/
void R_RemoveUnecessaryViewLights( void ) {
	// go through each visible light
	for ( viewLight_t *vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next ) {
		// if the light didn't have any lit surfaces visible, there is no need to
		// draw any of the shadows.  We still keep the vLight for debugging
		// draws
		if ( !vLight->localInteractions && !vLight->globalInteractions && !vLight->translucentInteractions ) {
			vLight->localShadows = nullptr;
			vLight->globalShadows = nullptr;
		}
	}

	if ( r_useShadowSurfaceScissor.GetBool() ) {
		// shrink the light scissor rect to only intersect the surfaces that will actually be drawn.
		// This doesn't seem to actually help, perhaps because the surface scissor
		// rects aren't actually the surface, but only the portal clippings.
		for ( vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next ) {
			if ( !vLight->lightShader->LightCastsShadows() ) {
				continue;
			}

			anScreenRect surfRect.Clear();

			for ( const drawSurf_t *surf = vLight->globalInteractions; surf; surf = surf->nextOnLight ) {
				surfRect.Union( surf->scissorRect );
			}
			for ( surf = vLight->localShadows; surf; surf = surf->nextOnLight ) {
				const_cast<drawSurf_t *>( surf)->scissorRect.Intersect( surfRect );
			}

			for ( surf = vLight->localInteractions; surf; surf = surf->nextOnLight ) {
				surfRect.Union( surf->scissorRect );
			}
			for ( surf = vLight->globalShadows; surf; surf = surf->nextOnLight ) {
				const_cast<drawSurf_t *>( surf)->scissorRect.Intersect( surfRect );
			}

			for ( surf = vLight->translucentInteractions; surf; surf = surf->nextOnLight ) {
				surfRect.Union( surf->scissorRect );
			}

			vLight->scissorRect.Intersect( surfRect );
		}
	}
}
