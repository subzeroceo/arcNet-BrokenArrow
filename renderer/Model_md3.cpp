#include "../idlib/Lib.h"
#pragma hdrstop

#include "Model_md3.h"
#include "Model_local.h"
#include "tr_local.h"

/***********************************************************************

        idMD3Mesh

***********************************************************************/

#include "../renderer/tr_local.h"
#include "Model_local.h"

/*
========================
idRenderModelMDR_Legacy::idRenderModelMDR_Legacy
========================
*/
idRenderModelMDR_Legacy::idRenderModelMDR_Legacy() {
	dataSize = 0;
	modelData = nullptr;
	cachedModel = nullptr;
}

/*
========================
idRenderModelMDR_Legacy::~idRenderModelMDR_Legacy
========================
*/
idRenderModelMDR_Legacy::~idRenderModelMDR_Legacy() {
	if ( modelData ) {
		Mem_Free( modelData );
		modelData = nullptr;
	}

	if ( cachedModel ) {
		delete cachedModel;
		cachedModel = nullptr;
	}

	dataSize = 0;
}

/*
========================
idRenderModelMDR_Legacy::InitFromFile
========================
*/
void idRenderModelMDR_Legacy::InitFromFile(const char *fileName) {
	name = fileName;
	LoadModel();
}

/*
========================
idRenderModelMDR_Legacy::LoadModel
========================
*/
void idRenderModelMDR_Legacy::LoadModel( void ) {
	void *mdrMeshBuffer;
	int mdrMeshLen;

	if ( !purged ) {
		PurgeModel();
	}
	purged = false;

	materials.Clear();

	// Try and load the file off disk.
	mdrMeshLen = fileSystem->ReadFile( name, &mdrMeshBuffer );
	if ( mdrMeshLen <= 0 || mdrMeshBuffer == nullptr ) {
		common->Warning( "Failed to load MDR mesh %s\n", name.c_str() );
		MakeDefaultModel();
	return;
	}

	// Parse the MDR file.
	if ( !LoadMDRFile( mdrMeshBuffer, mdrMeshLen ) ) {
		MakeDefaultModel();
		fileSystem->FreeFile( mdrMeshBuffer );
		return;
	}
	// If we don't have any frames, then render as a static mesh.
	// if (modelData->numFrames == 1)
	{
	viewDef_t view;
	RenderFramesToModel( this, nullptr, &view, nullptr, 1 );
	}

	// Free the memory from reading in the ifle.
	fileSystem->FreeFile( mdrMeshBuffer );
}

/*
========================
idRenderModelMDR_Legacy::LoadMDRFile
========================
*/
bool idRenderModelMDR_Legacy::LoadMDRFile( void *buffer, int filesize) {
	int i, j, k, l;
	mdrHeader_t *pinmodel, *mdr;
	mdrFrame_t *frame;
	mdrLOD_t *lod, *curlod;
	mdrSurface_t *surf, *cursurf;
	mdrTriangle_t *tri, *curtri;
	mdrVertex_t *v, *curv;
	mdrWeight_t *weight, *curweight;
	mdrTag_t *tag, *curtag;
	int size;
	const anMaterial *sh;

	const char *mod_name = name.c_str();

	anStr shaderPrefix = name;
	shaderPrefix.StripFileExtension();

	bounds.Clear();

	pinmodel = ( mdrHeader_t *)buffer;

	pinmodel->version = LittleLong( pinmodel->version );
	if ( pinmodel->version != MDR_VERSION ) {
	common->Warning( "R_LoadMDR: %s has wrong version (%i should be %i)\n", mod_name, pinmodel->version, MDR_VERSION);
	return false;
	}

	size = LittleLong( pinmodel->ofsEnd );

	if ( size > filesize ) {
		common->Warning( "R_LoadMDR: Header of %s is broken. Wrong filesize declared!\n", mod_name );
		return false;
	}

	LL( pinmodel->numFrames );
	LL( pinmodel->numBones -);
	LL( pinmodel->ofsFrames );

	// This is a model that uses some type of compressed Bones. We don't want to
	// uncompress every bone for each rendered frame over and over again, we'll
	// uncompress it in this function already, so we must adjust the size of the
	// target mdr.
	if ( pinmodel->ofsFrames < 0 ) {
		// mdrFrame_t is larger than mdrCompFrame_t:
		size += pinmodel->numFrames * sizeof( frame->name );
		// now add enough space for the uncompressed bones.
		size += pinmodel->numFrames * pinmodel->numBones * ( ( sizeof( mdrBone_t ) - sizeof( mdrCompBone_t ) ) );
	}

	// simple bounds check
	if ( pinmodel->numBones < 0 ||
		sizeof(* mdr) + pinmodel->numFrames * ( sizeof(* frame) + ( pinmodel->numBones - 1 ) * sizeof(* frame->bones) ) > size ) {
		common->Warning( "R_LoadMDR: %s has broken structure.\n", mod_name );
		return false;
	}

	dataSize += size;
	modelData = mdr = ( mdrHeader_t *)Mem_Alloc( size );

	// Copy all the values over from the file and fix endian issues in the
	// process, if necessary.

	mdr->ident = LittleLong( pinmodel->ident );
	mdr->version = pinmodel->version; // Don't need to swap byte order on this
	                    // one, we already did above.
	strcpy( mdr->name, pinmodel->name );
	mdr->numFrames = pinmodel->numFrames;
	mdr->numBones = pinmodel->numBones;
	mdr->numLODs = LittleLong( pinmodel->numLODs );
	mdr->numTags = LittleLong( pinmodel->numTags );
	// We don't care about the other offset values, we'll generate them ourselves
	// while loading.

	numLods = mdr->numLODs;

	if ( mdr->numFrames < 1 ) {
		common->Warning( "R_LoadMDR: %s has no frames\n", mod_name );
		return false;
	}

	/* The first frame will be put into the first free space after the header */
	frame = ( mdrFrame_t *)( mdr + 1 );
	mdr->ofsFrames = (int)( (byte *)frame - (byte *)mdr );

	if ( pinmodel->ofsFrames < 0 ) {
	mdrCompFrame_t *cframe;

	// compressed model...
	cframe = (mdrCompFrame_t *)( (byte *)pinmodel - pinmodel->ofsFrames );

    for ( i = 0; i < mdr->numFrames; i++ ) {
      for ( j = 0; j < 3; j++ ) {
        frame->bounds[0][j] = LittleFloat(cframe->bounds[0][j] );
        frame->bounds[1][j] = LittleFloat(cframe->bounds[1][j] );
        frame->localOrigin[j] = LittleFloat(cframe->localOrigin[j] );
      }

      if ( i == 0 ) {
        bounds[0] = frame->bounds[0];
        bounds[1] = frame->bounds[1];
      }

		frame->radius = LittleFloat( cframe->radius );
		frame->name[0] = '\0'; // No name supplied in the compressed version.

		for ( j = 0; j < mdr->numBones; j++ ) {
        for ( k = 0; k < ( sizeof( cframe->bones[j].Comp ) / 2 ); k++ ) {
          // Do swapping for the uncompressing functions. They seem to use
          // shorts values only, so I assume this will work. Never tested it on
          // other platforms, though.

          ( (unsigned short *)( cframe->bones[j].Comp ) )[k] =
              LittleShort( ( (unsigned short *)( cframe->bones[j].Comp ) )[k] );
        }

        /* Now do the actual uncompressing */
        MC_UnCompress( frame->bones[j].matrix, cframe->bones[j].Comp );
      }

      // Next Frame...
      cframe = (mdrCompFrame_t *)&cframe->bones[j];
      frame = (mdrFrame_t *)&frame->bones[j];
	}
	} else {
		mdrFrame_t *curframe;

    // uncompressed model...
    //

    curframe = ( mdrFrame_t *)( (byte *)pinmodel + pinmodel->ofsFrames );

    // swap all the frames
    for (i = 0; i < mdr->numFrames; i++ ) {
      for (j = 0; j < 3; j++ ) {
        frame->bounds[0][j] = LittleFloat(curframe->bounds[0][j] );
        frame->bounds[1][j] = LittleFloat(curframe->bounds[1][j] );
        frame->localOrigin[j] = LittleFloat(curframe->localOrigin[j] );
      }

      if ( i == 0 ) {
        bounds[0] = frame->bounds[0];
        bounds[1] = frame->bounds[1];
      }

      frame->radius = LittleFloat(curframe->radius);
      strcpy( frame->name, curframe->name );

      for ( int j = 0; j < ( int)( mdr->numBones * sizeof( mdrBone_t) / 4); j++ ) {
        ( (float *)frame->bones )[j] = LittleFloat( ( (float *)curframe->bones)[j] );
      }

      curframe = (mdrFrame_t *)&curframe->bones[mdr->numBones];
      frame = (mdrFrame_t *)&frame->bones[mdr->numBones];
    }
  }

  // frame should now point to the first free address after all frames.
  lod = (mdrLOD_t *)frame;
  mdr->ofsLODs = ( int)( (byte *)lod - (byte *)mdr );

  curlod = (mdrLOD_t *)( (byte *)pinmodel + LittleLong( pinmodel->ofsLODs ) );

  // swap all the LOD's
  for (l = 0; l < mdr->numLODs; l++ ) {
    // simple bounds check
    if ((byte *)(lod + 1) > (byte *)mdr + size) {
      common->Warning( "R_LoadMDR: %s has broken structure.\n", mod_name );
      return false;
    }

    lod->numSurfaces = LittleLong( curlod->numSurfaces );

    // swap all the surfaces
    surf = ( mdrSurface_t *)(lod + 1);
    lod->ofsSurfaces = ( int)( (byte *)surf - (byte *)lod );
    cursurf = ( mdrSurface_t *)( (byte *)curlod + LittleLong( curlod->ofsSurfaces ) );

    for ( int i = 0; i < lod->numSurfaces; i++ ) {
      // simple bounds check
      if ( (byte *)( surf + 1) > (byte *)mdr + size) {
        common->Warning( "R_LoadMDR: %s has broken structure.\n", mod_name );
        return false;
      }

      // first do some copying stuff
      strcpy( surf->name, cursurf->name);
      strcpy( surf->shader, cursurf->shader);

      surf->ofsHeader = (byte *)mdr - (byte *)surf;

      surf->numVerts = LittleLong(cursurf->numVerts);
      surf->numTriangles = LittleLong(cursurf->numTriangles);
      // numBoneReferences and BoneReferences generally seem to be unused

      // lowercase the surface name so skin compares are faster
      strlwr( surf->name);

      // register the shaders
      anStr materialName = va( "%s/%s", shaderPrefix.c_str(), surf->shader);
      materialName.Replace( " ", "_");
      materialName.Replace( "-", "_");
      sh = declManager->FindMaterial(
          materialName,
          true); // R_FindShader( surf->shader, LIGHTMAP_NONE, true);
      if (sh != nullptr) {
        surf->shaderIndex = sh->Index();
      } else {
        surf->shaderIndex = 0;
        common->Warning( "Failed to find material %s\n", materialName);
      }

      materials.Append(sh);

      // now copy the vertexes.
      v = ( mdrVertex_t *)( surf + 1);
      surf->ofsVerts = ( int)( (byte *)v - (byte *)surf);
      curv = ( mdrVertex_t *)( (byte *)cursurf + LittleLong(cursurf->ofsVerts));

      for (j = 0; j < surf->numVerts; j++ ) {
        LL(curv->numWeights);

        // simple bounds check
        if (curv->numWeights < 0 ||
            (byte *)(v + 1) + (curv->numWeights - 1) * sizeof(*weight) >
                (byte *)mdr + size) {
          common->Warning( "R_LoadMDR: %s has broken structure.\n", mod_name );
          return false;
        }

        v->normal[0] = LittleFloat(curv->normal[0] );
        v->normal[1] = LittleFloat(curv->normal[1] );
        v->normal[2] = LittleFloat(curv->normal[2] );

        v->texCoords[0] = LittleFloat(curv->texCoords[0] );
        v->texCoords[1] = LittleFloat(curv->texCoords[1] );

        v->numWeights = curv->numWeights;
        weight = &v->weights[0];
        curweight = &curv->weights[0];

        // Now copy all the weights
        for (k = 0; k < v->numWeights; k++ ) {
          weight->boneIndex = LittleLong(curweight->boneIndex);
          weight->boneWeight = LittleFloat(curweight->boneWeight);

          weight->offset[0] = LittleFloat(curweight->offset[0] );
          weight->offset[1] = LittleFloat(curweight->offset[1] );
          weight->offset[2] = LittleFloat(curweight->offset[2] );

          weight++;
          curweight++;
        }
        v = ( mdrVertex_t *)weight;
        curv = ( mdrVertex_t *)curweight;
      }

      // we know the offset to the triangles now:
      tri = ( mdrTriangle_t *)v;
      surf->ofsTriangles = ( int)( (byte *)tri - (byte *)surf);
      curtri = ( mdrTriangle_t *)( (byte *)cursurf +
                                 LittleLong(cursurf->ofsTriangles));

      // simple bounds check
      if ( surf->numTriangles < 0 ||
          (byte *)(tri + surf->numTriangles) > (byte *)mdr + size) {
        common->Warning( "R_LoadMDR: %s has broken structure.\n", mod_name );
        return false;
      }

      for ( int j = 0; j < surf->numTriangles; j++ ) {
        tri->indexes[0] = LittleLong(curtri->indexes[0] );
        tri->indexes[1] = LittleLong(curtri->indexes[1] );
        tri->indexes[2] = LittleLong(curtri->indexes[2] );

        tri++;
        curtri++;
      }

      // tri now points to the end of the surface.
      surf->ofsEnd = (byte *)tri - (byte *)surf;
      surf = ( mdrSurface_t *)tri;

      // find the next surface.
      cursurf = ( mdrSurface_t *)( (byte *)cursurf + LittleLong(cursurf->ofsEnd));
    }

    // surf points to the next lod now.
    lod->ofsEnd = ( int)( (byte *)surf - (byte *)lod);
    lod = ( mdrLOD_t *)surf;

    // find the next LOD.
    curlod = ( mdrLOD_t *)( (byte *)curlod + LittleLong( curlod->ofsEnd ) );
  }

  // lod points to the first tag now, so update the offset too.
  tag = ( mdrTag_t *)lod;
  mdr->ofsTags = ( int )( (byte *)tag - (byte *)mdr);
  curtag = ( mdrTag_t *)( (byte *)pinmodel + LittleLong( pinmodel->ofsTags ) );

  // simple bounds check
  if ( mdr->numTags < 0 || (byte *)( tag + mdr->numTags) > (byte *)mdr + size ) {
    common->Warning( "R_LoadMDR: %s has broken structure.\n", mod_name );
    return false;
  }

  for ( int i = 0; i < mdr->numTags; i++ ) {
    tag->boneIndex = LittleLong(curtag->boneIndex);
    strcpy( tag->name, curtag->name);
    tag++;
    curtag++;
  }

  // And finally we know the real offset to the end.
  mdr->ofsEnd = ( int)( (byte *)tag - (byte *)mdr);

  // Store the pointers for each frame, so we have it for faster lookup.
  for ( int frameId = 0; frameId < mdr->numFrames; frameId++ ) {
    mdrHeader_t *header;
    mdrFrame_t *frame;

    header = ( mdrHeader_t *)modelData;
    int frameSize = (size_t)( &( ( mdrFrame_t *)0)->bones[header->numBones] );

    frame = ( mdrFrame_t *)( (byte *)header + header->ofsFrames +
                           frameId * frameSize );
    frames.Append(frame);
  }

  return true;
}

/*
=================
idRenderModelMD3::IsDynamicModel
=================
*/
dynamicModel_t idRenderModelMDR_Legacy::IsDynamicModel() const {
  // if ( modelData->numFrames == 1 ) {
  //	return DM_STATIC;
  // } else {
  // return DM_CACHED;
  return DM_STATIC;
}

/*
=================
idRenderModelMD3::TouchData
=================
*/
void idRenderModelMDR_Legacy::TouchData() {}

/*
=================
idRenderModelMD3::TouchData
=================
*/
int idRenderModelMDR_Legacy::NumJoints( void ) const {
  return modelData->numBones;
}

/*
=================
idRenderModelMD3::GetJoints
=================
*/
const idMD5Joint *idRenderModelMDR_Legacy::GetJoints( void ) const {
  return nullptr;
}

/*
=================
idRenderModelMD3::GetJointHandle
=================
*/
jointHandle_t idRenderModelMDR_Legacy::GetJointHandle( const char *name ) const {
  return (jointHandle_t)-1;
}

/*
=================
idRenderModelMD3::Bounds
=================
*/
anBounds
idRenderModelMDR_Legacy::Bounds( const class anRenderEntity *ent ) const {
  return bounds;
}

/*
=================
idRenderModelMD3::Memory
=================
*/
int idRenderModelMDR_Legacy::Memory() const {
return dataSize;
}

/*
=================
idRenderModelMDR_Legacy::RenderFramesToModel
=================
*/
void idRenderModelMDR_Legacy::RenderFramesToModel( anRenderModelStatic *staticModel, const struct anRenderEntityParms *ent, const struct viewDef_s *view, const anMaterial *overrideMaterial, int scale ) {
  mdrBone_t bones[MDR_MAX_BONES], *bonePtr, *bone;

  int currentFrame = 0;
  int oldFrameNum = 0;

  // don't lerp if lerping off, or this is the only frame, or the last frame...
  float backlerp =
      0; // if backlerp is 0, lerping is off and frontlerp is never used
  float frontlerp = 1;

  mdrHeader_t *header = modelData;

  int frameSize = (size_t)( &( ( mdrFrame_t *)0)->bones[header->numBones] );

  mdrFrame_t *frame = ( mdrFrame_t *)( (byte *)header + header->ofsFrames +
                                     currentFrame * frameSize );
  mdrFrame_t *oldFrame = ( mdrFrame_t *)( (byte *)header + header->ofsFrames +
                                        oldFrameNum * frameSize );

  mdrLOD_t *lod = ( mdrLOD_t *)( (byte *)header + header->ofsLODs );
  mdrSurface_t *surface = ( mdrSurface_t *)( (byte *)lod + lod->ofsSurfaces );

  int baseIndex = 0;
  int baseVertex = 0;
  for ( int i = 0; i < lod->numSurfaces; i++ ) {
    srfTriangles_t *tri = R_AllocStaticTriSurf();
    R_AllocStaticTriSurfVerts(tri, surface->numVerts);
    R_AllocStaticTriSurfIndexes(tri, surface->numTriangles * 3);
    tri->bounds.Clear();

    modelSurface_t surf;

    surf.id = i;
    surf.geometry = tri;

    surf.shader = materials[i];

    // Set up all triangles.
    int *triangles = ( int *)( (byte *)surface + surface->ofsTriangles);
    int indexes = surface->numTriangles * 3;
    for ( int j = 0; j < indexes; j++ ) {
      tri->indexes[baseIndex + j] = baseVertex + triangles[j];
    }
    tri->numIndexes += indexes;

    //
    // lerp all the needed bones
    //
    if ( !backlerp) {
      // no lerping needed
      bonePtr = frame->bones;
    } else {
      bonePtr = bones;
      for ( int i = 0; i < header->numBones * 12; i++ ) {
        ( (float *)bonePtr )[i] = frontlerp * ( (float *)frame->bones )[i] +
                                backlerp * ( (float *)oldFrame->bones )[i];
      }
    }

    //
    // deform the vertexes by the lerped bones
    //
    int numVerts = surface->numVerts;
    mdrVertex_t *v = ( mdrVertex_t *)( (byte *)surface + surface->ofsVerts );
    for ( int j = 0; j < numVerts; j++ ) {
      anVec3 tempVert, tempNormal;
      mdrWeight_t *w;

      tempVert.Zero();
      tempNormal.Zero();
      w = v->weights;
      for ( int k = 0; k < v->numWeights; k++, w++ ) {
        bone = bonePtr + w->boneIndex;

        tempVert[0] += w->boneWeight *
                       ( Dot( bone->matrix[0], w->offset) + bone->matrix[0][3] );
        tempVert[1] += w->boneWeight *
                       ( Dot( bone->matrix[1], w->offset) + bone->matrix[1][3] );
        tempVert[2] += w->boneWeight *
                       ( Dot( bone->matrix[2], w->offset) + bone->matrix[2][3] );

        tempNormal[0] += w->boneWeight * Dot( bone->matrix[0], v->normal );
        tempNormal[1] += w->boneWeight * Dot( bone->matrix[1], v->normal );
        tempNormal[2] += w->boneWeight * Dot( bone->matrix[2], v->normal );
      }

      tri->verts[baseVertex + j].xyz[0] = tempVert[0];
      tri->verts[baseVertex + j].xyz[1] = tempVert[1];
      tri->verts[baseVertex + j].xyz[2] = tempVert[2];

      tri->verts[baseVertex + j].st[0] = v->texCoords[0];
      tri->verts[baseVertex + j].st[1] = v->texCoords[1];

      v = ( mdrVertex_t *)&v->weights[v->numWeights];
    }

    // tess.numVertexes += surface->numVerts;
    tri->numVerts += surface->numVerts;

    // baseVertex += surface->numVerts;
    // baseIndex += surface->numTriangles * 3;

    R_BoundTriSurf( tri );

    staticModel->AddSurface( surf );
    staticModel->bounds.AddPoint( surf.geometry->bounds[0] );
    staticModel->bounds.AddPoint( surf.geometry->bounds[1] );

    surface = ( mdrSurface_t *)( (byte *)surface + surface->ofsEnd);
  }
}

/*
=================
idRenderModelMDR_Legacy::InstantiateDynamicModel
=================
*/
anRenderModel *idRenderModelMDR_Legacy::InstantiateDynamicModel(
    const class anRenderEntity *ent, const struct viewDef_s *view,
    anRenderModel *cachedModel) {
  anRenderModelStatic *staticModel;
  class anRenderEntityLocal *renderEntity = (class anRenderEntityLocal *)ent;

  if (cachedModel) {
    delete cachedModel;
    cachedModel = nullptr;
  }

  staticModel = new anRenderModelStatic;
  staticModel->bounds.Clear();

  int scale = 1; // ent->scale;
  if (scale == 0)
    scale = 1;

  if (ent != nullptr) {
    const anMaterial *material = renderEntity->GetCustomShader();
    RenderFramesToModel(staticModel, &renderEntity->parms, view, material,
                        scale);
  } else {
    RenderFramesToModel(staticModel, nullptr, view, nullptr, scale);
  }

  return staticModel;
}

/*
=================
idRenderModelMD3::GetJointHandle
=================
*/
const char *idRenderModelMDR_Legacy::GetJointName(jointHandle_t handle) const {
  return "not_implemented";
}

/*
=================
idRenderModelMD3::GetDefaultPose
=================
*/
const idJointQuat *idRenderModelMDR_Legacy::GetDefaultPose( void ) const {
  return nullptr;
}
/*
=================
idRenderModelMD3::NearestJoint
=================
*/
int idRenderModelMDR_Legacy::NearestJoint( int surfaceNum, int a, int b,
                                          int c) const {
  return 0;
}

#define LL(x) x = LittleLong(x)

/*
=================
idRenderModelMD3_Legacy::InitFromFile
=================
*/
void idRenderModelMD3_Legacy::InitFromFile(const char *fileName) {
  int i, j;
  md3Header_t *pinmodel;
  md3Frame_t *frame;
  md3Surface_t *surf;
  md3Shader_t *shader;
  md3Triangle_t *tri;
  md3St_t *st;
  md3XyzNormal_t *xyz;
  md3Tag_t *tag;
  void *buffer;
  int version;
  int size;

  name = fileName;

  size = fileSystem->ReadFile(fileName, &buffer, nullptr);
  if ( !size || size < 0) {
    return;
  }

  pinmodel = (md3Header_t *)buffer;

  version = LittleLong( pinmodel->version);
  if (version != MD3_VERSION) {
    fileSystem->FreeFile(buffer);
    common->Warning( "InitFromFile: %s has wrong version (%i should be %i)",
                    fileName, version, MD3_VERSION);
    return;
  }

  size = LittleLong( pinmodel->ofsEnd);
  dataSize += size;
  md3 = (md3Header_t *)Mem_Alloc(size);

  memcpy(md3, buffer, LittleLong( pinmodel->ofsEnd));

  LL(md3->ident);
  LL(md3->version);
  LL(md3->numFrames );
  LL(md3->numTags);
  LL(md3->numSurfaces );
  LL(md3->ofsFrames );
  LL(md3->ofsTags);
  LL(md3->ofsSurfaces);
  LL(md3->ofsEnd);

  if (md3->numFrames < 1) {
    common->Warning( "InitFromFile: %s has no frames", fileName);
    fileSystem->FreeFile(buffer);
    return;
  }

  // swap all the frames
  frame = (md3Frame_t *)( (byte *)md3 + md3->ofsFrames );
  for (i = 0; i < md3->numFrames; i++, frame++ ) {
    frame->radius = LittleFloat(frame->radius);
    for (j = 0; j < 3; j++ ) {
      frame->bounds[0][j] = LittleFloat(frame->bounds[0][j] );
      frame->bounds[1][j] = LittleFloat(frame->bounds[1][j] );
      frame->localOrigin[j] = LittleFloat(frame->localOrigin[j] );
    }
  }

  // swap all the tags
  tag = (md3Tag_t *)( (byte *)md3 + md3->ofsTags);
  for (i = 0; i < md3->numTags * md3->numFrames; i++, tag++ ) {
    for (j = 0; j < 3; j++ ) {
      tag->origin[j] = LittleFloat( tag->origin[j] );
      tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
      tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
      tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
    }
  }

  // swap all the surfaces
  surf = (md3Surface_t *)( (byte *)md3 + md3->ofsSurfaces);
  for (i = 0; i < md3->numSurfaces; i++ ) {

    LL( surf->ident);
    LL( surf->flags);
    LL( surf->numFrames );
    LL( surf->numShaders);
    LL( surf->numTriangles);
    LL( surf->ofsTriangles);
    LL( surf->numVerts);
    LL( surf->ofsShaders);
    LL( surf->ofsSt);
    LL( surf->ofsXyzNormals);
    LL( surf->ofsEnd);

    if ( surf->numVerts > SHADER_MAX_VERTEXES) {
      common->Error( "InitFromFile: %s has more than %i verts on a surface (%i)",
                    fileName, SHADER_MAX_VERTEXES, surf->numVerts);
    }
    if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES) {
      common->Error(
          "InitFromFile: %s has more than %i triangles on a surface (%i)",
          fileName, SHADER_MAX_INDEXES / 3, surf->numTriangles);
    }

    // change to surface identifier
    surf->ident = 0; // SF_MD3;

    // lowercase the surface name so skin compares are faster
    int slen = ( int)strlen( surf->name);
    for (j = 0; j < slen; j++ ) {
      surf->name[j] = tolower( surf->name[j] );
    }

    // strip off a trailing _1 or _2
    // this is a crutch for q3data being a mess
    j = strlen( surf->name);
    if (j > 2 && surf->name[j - 2] == '_') {
      surf->name[j - 2] = 0;
    }

    // register the shaders
    shader = (md3Shader_t *)( (byte *)surf + surf->ofsShaders);
    for (j = 0; j < surf->numShaders; j++, shader++ ) {
      const anMaterial *sh;

      sh = declManager->FindMaterial(shader->name);
      shader->shader = sh;
    }

    // swap all the triangles
    tri = (md3Triangle_t *)( (byte *)surf + surf->ofsTriangles);
    for (j = 0; j < surf->numTriangles; j++, tri++ ) {
      LL(tri->indexes[0] );
      LL(tri->indexes[1] );
      LL(tri->indexes[2] );
    }

    // swap all the ST
    st = (md3St_t *)( (byte *)surf + surf->ofsSt);
    for (j = 0; j < surf->numVerts; j++, st++ ) {
      st->st[0] = LittleFloat(st->st[0] );
      st->st[1] = LittleFloat(st->st[1] );
    }

    // swap all the XyzNormals
    xyz = (md3XyzNormal_t *)( (byte *)surf + surf->ofsXyzNormals);
    for (j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++ ) {
      xyz->xyz[0] = LittleShort(xyz->xyz[0] );
      xyz->xyz[1] = LittleShort(xyz->xyz[1] );
      xyz->xyz[2] = LittleShort(xyz->xyz[2] );

      xyz->normal = LittleShort(xyz->normal);
    }

    // find the next surface
    surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd);
  }

  fileSystem->FreeFile(buffer);
}

/*
=================
idRenderModelMD3_Legacy::IsDynamicModel
=================
*/
dynamicModel_t idRenderModelMD3_Legacy::IsDynamicModel() const {
  return DM_CACHED;
}

/*
=================
idRenderModelMD3_Legacy::LerpMeshVertexes
=================
*/
void idRenderModelMD3_Legacy::LerpMeshVertexes(srfTriangles_t *tri,
                                               const struct md3Surface_s *surf,
                                               const float backlerp,
                                               const int frame,
                                               const int oldframe) const {
  short *oldXyz, *newXyz;
  float oldXyzScale, newXyzScale;
  int vertNum;
  int numVerts;

  newXyz = (short *)( (byte *)surf + surf->ofsXyzNormals) +
           (frame * surf->numVerts * 4);

  newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);

  numVerts = surf->numVerts;

  if (backlerp == 0) {
    // just copy the vertexes
    for (vertNum = 0; vertNum < numVerts; vertNum++, newXyz += 4) {
      anDrawVertex *outvert = &tri->verts[tri->numVerts];

      outvert->xyz.x = newXyz[0] * newXyzScale;
      outvert->xyz.y = newXyz[1] * newXyzScale;
      outvert->xyz.z = newXyz[2] * newXyzScale;

      tri->numVerts++;
    }
  } else {
    //
    // interpolate and copy the vertexes
    //
    oldXyz = (short *)( (byte *)surf + surf->ofsXyzNormals) +
             (oldframe * surf->numVerts * 4);

    oldXyzScale = MD3_XYZ_SCALE * backlerp;

    for (vertNum = 0; vertNum < numVerts; vertNum++, oldXyz += 4, newXyz += 4) {
      anDrawVertex *outvert = &tri->verts[tri->numVerts];

      // interpolate the xyz
      outvert->xyz.x = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
      outvert->xyz.y = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
      outvert->xyz.z = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

      tri->numVerts++;
    }
  }
}

/*
=============
idRenderModelMD3_Legacy::InstantiateDynamicModel
=============
*/
anRenderModel *idRenderModelMD3_Legacy::InstantiateDynamicModel(
    const struct renderEntity_s *ent, const struct viewDef_s *view,
    anRenderModel *cachedModel) {
  int i, j;
  float backlerp;
  int *triangles;
  float *texCoords;
  int indexes;
  int numVerts;
  md3Surface_t *surface;
  int frame, oldframe;
  anModelStatic *staticModel;

  if (cachedModel) {
    delete cachedModel;
    cachedModel = nullptr;
  }

  staticModel = new anModelStatic;
  staticModel->bounds.Clear();

  surface = (md3Surface_t *)( (byte *)md3 + md3->ofsSurfaces);

  // TODO: these need set by an entity
  frame = ent->shaderParms[SHADERPARM_MD3_FRAME]; // probably want to keep
                                                  // frames < 1000 or so
  oldframe = ent->shaderParms[SHADERPARM_MD3_LASTFRAME];
  backlerp = ent->shaderParms[SHADERPARM_MD3_BACKLERP];

  for (i = 0; i < md3->numSurfaces; i++ ) {
    srfTriangles_t *tri = R_AllocStaticTriSurf();
    R_AllocStaticTriSurfVerts(tri, surface->numVerts);
    R_AllocStaticTriSurfIndexes(tri, surface->numTriangles * 3);
    tri->bounds.Clear();

    modelSurface_t surf;

    surf.geometry = tri;

    md3Shader_t *shaders =
        (md3Shader_t *)( (byte *)surface + surface->ofsShaders);
    surf.shader = shaders->shader;

    LerpMeshVertexes(tri, surface, backlerp, frame, oldframe);

    triangles = ( int *)( (byte *)surface + surface->ofsTriangles);
    indexes = surface->numTriangles * 3;
    for (j = 0; j < indexes; j++ ) {
      tri->indexes[j] = triangles[j];
    }
    tri->numIndexes += indexes;

    texCoords = (float *)( (byte *)surface + surface->ofsSt);

    numVerts = surface->numVerts;
    for (j = 0; j < numVerts; j++ ) {
      anDrawVertex *stri = &tri->verts[j];
      stri->st[0] = texCoords[j * 2 + 0];
      stri->st[1] = texCoords[j * 2 + 1];
    }

    R_BoundTriSurf( tri );

    staticModel->AddSurface( surf );
    staticModel->bounds.AddPoint( surf.geometry->bounds[0] );
    staticModel->bounds.AddPoint( surf.geometry->bounds[1] );

    // find the next surface
    surface = (md3Surface_t *)( (byte *)surface + surface->ofsEnd);
  }

  return staticModel;
}

/*
=====================
idRenderModelMD3_Legacy::Bounds
=====================
*/
anBounds idRenderModelMD3_Legacy::Bounds( const struct renderEntity_s *ent ) const {
  anBounds ret.Clear();

  if ( !ent || !md3 ) {
    // just give it the editor bounds
    ret.AddPoint( anVec3( -10, -10, -10 ) );
    ret.AddPoint( anVec3( 10, 10, 10) );
    return ret;
  }

  md3Frame_t *frame = (md3Frame_t *)( (byte *)md3 + md3->ofsFrames );

  ret.AddPoint( frame->bounds[0] );
  ret.AddPoint( frame->bounds[1] );

  return ret;
}
