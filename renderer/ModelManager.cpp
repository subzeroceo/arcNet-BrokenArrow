#include "/idlib/precompiled.h"
#pragma hdrstop

#include "Model_local.h"
#include "tr_local.h"	// just for R_FreeWorldInteractions and R_CreateWorldInteractions


//arcCVarSystem binaryLoadRenderModels( "binaryLoadRenderModels", "1", 0, "enable binary load/write of render models" );
arcCVarSystem preload_MapModels( "preload_MapModels", "1", CVAR_SYSTEM | CVAR_BOOL, "preload models during begin or end levelload" );
arcCVarSystem postLoadExportModels( "postLoadExportModels", "0", CVAR_BOOL | CVAR_RENDERER, "export models after loading to OBJ model format" );

class ARCModelManagerLocal : public ARCModelManager {
public:
							ARCModelManagerLocal();
	virtual					~ARCModelManagerLocal() {}

	virtual void			Init();
	virtual void			Shutdown();
	virtual ARCRenderModel	*AllocModel();
	virtual void			FreeModel( ARCRenderModel *model );
	virtual ARCRenderModel	*FindModel( const char *modelName );
	virtual ARCRenderModel	*CheckModel( const char *modelName );
	virtual ARCRenderModel	*DefaultModel();
	virtual void			AddModel( ARCRenderModel *model );
	virtual void			RemoveModel( ARCRenderModel *model );
	virtual void			ReloadModels( bool forceAll = false );
	virtual void			FreeModelVertexCaches();
	virtual void			WritePrecacheCommands( arcNetFile *file );
	virtual void			BeginLevelLoad();
	virtual void			Preload( const idPreloadManifest &manifest );
	virtual void			EndLevelLoad();

	virtual	void			PrintMemInfo( MemInfo_t *mi );

private:
	arcNetList<ARCRenderModel*>models;
	ARCHashIndex			hash;
	ARCRenderModel			*defaultModel;
	ARCRenderModel			*beamModel;
	ARCRenderModel			*spriteModel;
	ARCRenderModel			*trailModel;
	bool					insideLevelLoad;		// don't actually load now

	ARCRenderModel			*GetModel( const char *modelName, bool createIfNotFound );

	static void				PrintModel_f( const arcCommandArgs &args );
	static void				ListModels_f( const arcCommandArgs &args );
	static void				ReloadModels_f( const arcCommandArgs &args );
	static void				TouchModel_f( const arcCommandArgs &args );
};


ARCModelManagerLocal		localModelManager;
ARCModelManagerLocal		*renderModelManager = &localModelManager;

/*
==============
ARCModelManagerLocal::ARCModelManagerLocal
==============
*/
ARCModelManagerLocal::ARCModelManagerLocal() {
	defaultModel = NULL;
	beamModel = NULL;
	spriteModel = NULL;
	insideLevelLoad = false;
	trailModel = NULL;
}

/*
==============
ARCModelManagerLocal::PrintModel_f
==============
*/
void ARCModelManagerLocal::PrintModel_f( const arcCommandArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "usage: printModel <modelName>\n" );
		return;
	}

	ARCRenderModel *model = renderModelManager->CheckModel( args.Argv( 1 ) );
	if ( !model ) {
		common->Printf( "model \"%s\" not found\n", args.Argv( 1 ) );
		return;
	}

	model->Print();
}

/*
==============
ARCModelManagerLocal::ListModels_f
==============
*/
void ARCModelManagerLocal::ListModels_f( const arcCommandArgs &args ) {
	int		totalMem = 0;
	int		inUse = 0;

	common->Printf( " mem   srf verts tris\n" );
	common->Printf( " ---   --- ----- ----\n" );

	for ( int i = 0; i < localModelManager.models.Num(); i++ ) {
		ARCRenderModel	*model = localModelManager.models[i];
		if ( !model->IsLoaded() ) {
			continue;
		}
		model->List();
		totalMem += model->Memory();
		inUse++;
	}

	common->Printf( " ---   --- ----- ----\n" );
	common->Printf( " mem   srf verts tris\n" );

	common->Printf( "%i loaded models\n", inUse );
	common->Printf( "total memory: %4.1fM\n", ( float )totalMem / (1024*1024) );
}

/*
==============
ARCModelManagerLocal::ReloadModels_f
==============
*/
void ARCModelManagerLocal::ReloadModels_f( const arcCommandArgs &args ) {
	if ( arcNetString::Icmp( args.Argv(1 ), "all" ) == 0 ) {
		localModelManager.ReloadModels( true );
	} else {
		localModelManager.ReloadModels( false );
	}
}

/*
==============
ARCModelManagerLocal::TouchModel_f

Precache a specific model
==============
*/
void ARCModelManagerLocal::TouchModel_f( const arcCommandArgs &args ) {
	const char	*model = args.Argv( 1 );

	if ( !model[0] ) {
		common->Printf( "usage: touchModel <modelName>\n" );
		return;
	}

	common->Printf( "touchModel %s\n", model );
	session->UpdateScreen();
	ARCRenderModel *m = renderModelManager->CheckModel( model );
	if ( !m ) {
		common->Printf( "...not found\n" );
	}
}

/*
=================
ARCModelManagerLocal::WritePrecacheCommands
=================
*/
void ARCModelManagerLocal::WritePrecacheCommands( arcNetFile *f ) {
	for ( int i = 0; i < models.Num(); i++ ) {
		ARCRenderModel	*model = models[i];
		if ( !model ) {
			continue;
		}
		if ( !model->IsReloadable() ) {
			continue;
		}

		char	str[1024];
		sprintf( str, "touchModel %s\n", model->Name() );
		common->Printf( "%s", str );
		f->Printf( "%s", str );
	}
}

/*
=================
ARCModelManagerLocal::Init
=================
*/
void ARCModelManagerLocal::Init() {
	cmdSystem->AddCommand( "listModels", ListModels_f, CMD_FL_RENDERER, "lists all models" );
	cmdSystem->AddCommand( "printModel", PrintModel_f, CMD_FL_RENDERER, "prints model info", arcCmdSystem::ArgCompletion_ModelName );
	cmdSystem->AddCommand( "reloadModels", ReloadModels_f, CMD_FL_RENDERER|CMD_FL_CHEAT, "reloads models" );
	cmdSystem->AddCommand( "touchModel", TouchModel_f, CMD_FL_RENDERER, "touches a model", arcCmdSystem::ArgCompletion_ModelName );

	insideLevelLoad = false;

	// create a default model
	aRcModelStatic *model = new aRcModelStatic;
	model->InitEmpty( "_DEFAULT" );
	model->MakeDefaultModel();
	model->SetLevelLoadReferenced( true );
	defaultModel = model;
	AddModel( model );

	// create the beam model
	aRcModelStatic *beam = new idRenderModelBeam;
	beam->InitEmpty( "_BEAM" );
	beam->SetLevelLoadReferenced( true );
	beamModel = beam;
	AddModel( beam );

	aRcModelStatic *sprite = new idRenderModelSprite;
	sprite->InitEmpty( "_SPRITE" );
	sprite->SetLevelLoadReferenced( true );
	spriteModel = sprite;
	AddModel( sprite );
}

/*
=================
ARCModelManagerLocal::Shutdown
=================
*/
void ARCModelManagerLocal::Shutdown() {
	models.DeleteContents( true );
	hash.Free();
}

/*
=================
ARCModelManagerLocal::GetModel
=================
*/
ARCRenderModel *ARCModelManagerLocal::GetModel( const char *modelName, bool createIfNotFound ) {
	//arcNetString::StripExtension( modelName )
	if ( !modelName || !modelName[0] ) {
		return NULL;
	}
	//int key = HashKey_String( modelName );

	aRcStaticString< MAX_OSPATH > canonical = modelName;
	canonical.ToLower();

	aRcStaticString< MAX_OSPATH > extension;

	// see if it is already present
	int key = hash.GenerateKey( modelName, false );
	for ( int i = hash.First( key ); i != -1; i = hash.Next( i ) ) {
		ARCRenderModel *model = models[i];
		if ( canonical.Icmp( model->Name() ) == 0 ) {
			if ( !model->IsLoaded() ) {
				arcNetString generatedFileName = "generated/rendermodels/";
				generatedFileName.AppendPath( canonical );
				generatedFileName.SetFileExtension( va( "b%s", extension.c_str() ) );

				// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it
				ARC_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( canonical );
				ARC_TIME_T generatedTimeStamp = fileSystem->GetTimestamp( generatedFileName );
				if ( generatedTimeStamp > sourceTimeStamp ) {
					if ( model->SupportsBinaryModel() && binaryLoadRenderModels.GetBool() ) {
					arcFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
					model->PurgeModel();
					if ( !model->LoadBinaryModel( file, sourceTimeStamp ) ) {
						model->LoadModel();
					}
				} else {
					// reload it if it was purged
					model->LoadModel();
			} else if ( insideLevelLoad && !model->IsLevelLoadReferenced() ) {
				// we are reusing a model already in memory, but
				// touch all the materials to make sure they stay
				// in memory as well
				model->TouchData();
			}
			model->SetLevelLoadReferenced( true );
			return model;
		}
	}

	// see if we can load it

	// determine which subclass of ARCRenderModel to initialize

	ARCRenderModel	*model = NULL;
	arcNetString extension;
	canonical.ExtractFileExtension( extension );
	//if ( ( extension.Icmp( "md8" ) == 0 ) || (extension.Icmp( "obj" ) == 0) || ( extension.Icmp( "ase" ) == 0 ) || ( extension.Icmp( "lwo" ) == 0 ) || ( extension.Icmp( "flt" ) == 0 ) || ( extension.Icmp( "ma" ) == 0 ) ) {
		//model = new( TAG_MODEL ) aRcModelStatic;
		//model->InitFromFile( canonical );
	if ( ( extension.Icmp( "ase" ) == 0 ) || ( extension.Icmp( "lwo" ) == 0 ) || ( extension.Icmp( "flt" ) == 0 ) ) {
		model = new aRcModelStatic;
		model->InitFromFile( modelName );
	} else if ( extension.Icmp( "ma" ) == 0 ) {
		model = new aRcModelStatic;
		model->InitFromFile( modelName );
	} else if ( extension.Icmp( MD5_MESH_EXT ) == 0 ) {
		model = new idRenderModelMD5;
		model->InitFromFile( modelName );
	} else if ( extension.Icmp( "md3" ) == 0 ) {
		model = new idRenderModelMD3;
		model->InitFromFile( modelName );
	} else if ( extension.Icmp( "prt" ) == 0  ) {
		model = new idRenderModelPrt;
		model->InitFromFile( modelName );
	} else if ( extension.Icmp( "liquid" ) == 0  ) {
		model = new ARCLiquidModel;
		model->InitFromFile( modelName );
	}

	aRcStaticString< MAX_OSPATH > generatedFileName;

	if ( model != NULL ) {
		generatedFileName = "generated/rendermodels/";
		generatedFileName.AppendPath( canonical );
		generatedFileName.SetFileExtension( va( "b%s", extension.c_str() ) );

		// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it
		ARC_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( canonical );
		ARC_TIME_T generatedTimeStamp = fileSystem->GetTimestamp( generatedFileName );
		if ( generatedTimeStamp > sourceTimeStamp ) {
			arcFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
			if ( !model->SupportsBinaryModel() || !binaryLoadRenderModels.GetBool() ) {
				model->InitFromFile( canonical );
			}
		} else {
			if ( !model->LoadBinaryModel( file, sourceTimeStamp ) ) {
				model->InitFromFile( canonical );
				// default models shouldn't be cached as binary models
				if ( !model->IsDefaultModel() ) {
					arcFileLocal outputFile( fileSystem->OpenFileWrite( generatedFileName, "fs_basepath" ) );
					arcLibrary::Printf( "Writing %s\n", generatedFileName.c_str() );
					model->WriteBinaryModel( outputFile );
				}
			} else {
				arcLibrary::Printf( "loaded binary model %s from file %s\n", model->Name(), generatedFileName.c_str() );
			}
		}
	} else if ( model == NULL ) {
		if ( extension.Length() ) {
			common->Warning( "unknown model type '%s'", canonical.c_str() );
		}

		if ( !createIfNotFound ) {
			return NULL;
		}

		aRcModelStatic	*smodel = new aRcModelStatic;
		smodel->InitEmpty( modelName );
		smodel->MakeDefaultModel();

		model = smodel;
	}
	if ( cvarSystem->GetCVarBool( "fs_buildresources" ) ) {
		fileSystem->AddModelPreload( canonical );
	}
	model->SetLevelLoadReferenced( true );

	if ( !createIfNotFound && model->IsDefaultModel() ) {
		delete model;
		model = NULL;
		return NULL;
	}

	if ( cvarSystem->GetCVarBool( "fs_buildEngine" ) ) {
		fileSystem->AddModelPreload( model->Name() );
	}

	if ( postLoadExportModels.GetBool() && ( model != defaultModel && model != beamModel && model != spriteModel ) ) {
		aRcStaticString< MAX_OSPATH > exportedFileName;

		exportedFileName = "exported/rendermodels/";
		exportedFileName.AppendPath( canonical );
		exportedFileName.SetFileExtension( ".obj" );

		ARC_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( canonical );
		ARC_TIME_T timeStamp = fileSystem->GetTimestamp( exportedFileName );

		// TODO: only update if generated has changed

		//if ( timeStamp == FILE_NOT_FOUND_TIMESTAMP ) {
			arcFileLocal objFile( fileSystem->OpenFileWrite( exportedFileName, "fs_basepath" ) );
			arcLibrary::Printf( "Writing %s\n", exportedFileName.c_str() );

			exportedFileName.SetFileExtension( ".mtl" );
			arcFileLocal mtlFile( fileSystem->OpenFileWrite( exportedFileName, "fs_basepath" ) );

			model->ExportOBJ( objFile, mtlFile );
		}
	}
	AddModel( model );

	return model;
}

/*
=================
ARCModelManagerLocal::AllocModel
=================
*/
ARCRenderModel *ARCModelManagerLocal::AllocModel() {
	return new( TAG_MODEL ) aRcModelStatic();
}

/*
=================
ARCModelManagerLocal::AllocBSEModel
=================
*/
aRcModelBSE* ARCModelManagerLocal::AllocBSEModel() {
	return new( TAG_MODEL ) aRcModelBSE;
}

/*
=================
ARCModelManagerLocal::FreeModel
=================
*/
void ARCModelManagerLocal::FreeModel( ARCRenderModel *model ) {
	if ( !model ) {
		return;
	}
	if ( !dynamic_cast<aRcModelStatic *>( model ) ) {
		common->Error( "ARCModelManager::FreeModel: model '%s' is not a static model", model->Name() );
		return;
	}
	if ( model == defaultModel ) {
		common->Error( "ARCModelManager::FreeModel: can't free the default model" );
		return;
	}
	if ( model == beamModel ) {
		common->Error( "ARCModelManager::FreeModel: can't free the beam model" );
		return;
	}
	if ( model == spriteModel ) {
		common->Error( "ARCModelManager::FreeModel: can't free the sprite model" );
		return;
	}

	R_CheckForEntityDefsUsingModel( model );

	delete model;
}

/*
=================
ARCModelManagerLocal::FindModel
=================
*/
ARCRenderModel *ARCModelManagerLocal::FindModel( const char *modelName ) {
	return GetModel( modelName, true );
}

/*
=================
ARCModelManagerLocal::CheckModel
=================
*/
ARCRenderModel *ARCModelManagerLocal::CheckModel( const char *modelName ) {
	return GetModel( modelName, false );
}

/*
=================
ARCModelManagerLocal::DefaultModel
=================
*/
ARCRenderModel *ARCModelManagerLocal::DefaultModel() {
	return defaultModel;
}

/*
=================
ARCModelManagerLocal::AddModel
=================
*/
void ARCModelManagerLocal::AddModel( ARCRenderModel *model ) {
	hash.Add( hash.GenerateKey( model->Name(), false ), models.Append( model ) );
}

/*
=================
ARCModelManagerLocal::RemoveModel
=================
*/
void ARCModelManagerLocal::RemoveModel( ARCRenderModel *model ) {
	int index = models.FindIndex( model );
	if ( index != -1 ) {
		hash.RemoveIndex( hash.GenerateKey( model->Name(), false ), index );
		models.RemoveIndex( index );
	}
}

/*
=================
ARCModelManagerLocal::ReloadModels
=================
*/
void ARCModelManagerLocal::ReloadModels( bool forceAll ) {
	if ( forceAll ) {
		common->Printf( "Reloading all model files...\n" );
	} else {
		common->Printf( "Checking for changed model files...\n" );
	}

	R_FreeDerivedData();

	// skip the default model at index 0
	for ( int i = 1; i < models.Num(); i++ ) {
		ARCRenderModel *model = models[i];
		// we may want to allow world model reloading in the future, but we don't now
		if ( !model->IsReloadable() ) {
			continue;
		}

		if ( !forceAll ) {
			// check timestamp
			ARC_TIME_T current;
			fileSystem->ReadFile( model->Name(), NULL, &current );
			if ( current <= model->Timestamp() ) {
				continue;
			}
		}

		common->DPrintf( "reloading %s.\n", model->Name() );
		model->LoadModel();
	}

	// we must force the world to regenerate, because models may
	// have changed size, making their references invalid
	R_ReCreateWorldReferences();
}

/*
=================
ARCModelManagerLocal::FreeModelVertexCaches
=================
*/
void ARCModelManagerLocal::FreeModelVertexCaches() {
	for ( int i = 0; i < models.Num(); i++ ) {
		ARCRenderModel *model = models[i];
		model->FreeVertexCache();
	}
}

/*
=================
ARCModelManagerLocal::BeginLevelLoad
=================
*/
void ARCModelManagerLocal::BeginLevelLoad() {
	insideLevelLoad = true;

	for ( int i = 0; i < models.Num(); i++ ) {
		ARCRenderModel *model = models[i];
		// Reloads all of the models
		if ( com_purgeAll.GetBool() && model->IsReloadable() ) {
			R_CheckForEntityDefsUsingModel( model );
			model->PurgeModel();
		}

		model->SetLevelLoadReferenced( false );
	}

	// purge unused triangle surface memory
	vertexCache.FreeStaticData();
	R_PurgeTriSurfData( frameData );
}

/*
=================
ARCModelManagerLocal::Preload
=================
*/
void ARCModelManagerLocal::Preload( const aRcPreloadManifest& manifest ) {
	if ( preload_MapModels.GetBool() ) {
		// preload this levels images
		int	start = Sys_Milliseconds();
		int numLoaded = 0;
		arcNetList< preloadSort_t > preloadSort;
		preloadSort.Resize( manifest.NumResources() );
		for ( int i = 0; i < manifest.NumResources(); i++ ) {
			const preloadEntry_s& p = manifest.GetPreloadByIndex( i );
			aRcResCacheEntries rc;
			aRcStaticString< MAX_OSPATH > filename;
			if ( p.resType == PRELOAD_MODEL ) {
				filename = "generated/rendermodels/";
				filename += p.resourceName;
				aRcStaticString< 16 > ext;
				filename.ExtractFileExtension( ext );
				filename.SetFileExtension( va( "b%s", ext.c_str() ) );
			}

			if ( p.resType == PRELOAD_PARTICLE ) {
				filename = "generated/particles/";
				filename += p.resourceName;
				filename += ".bprt";
			}

			if ( !filename.IsEmpty() ) {
				if ( fileSystem->GetResourceCacheEntry( filename, rc ) ) {
					preloadSort_t ps = {};
					ps.idx = i;
					ps.ofs = rc.offset;
					preloadSort.Append( ps );
				}
			}
		}

		preloadSort.SortWithTemplate( aRcSortPreload() );

		for ( int i = 0; i < preloadSort.Num(); i++ ) {
			const preloadSort_t& ps = preloadSort[ i ];
			const preloadEntry_s& p = manifest.GetPreloadByIndex( ps.idx );
			if ( p.resType == PRELOAD_MODEL ) {
				idRenderModel* model = FindModel( p.resourceName );
				if ( model != NULL ) {
					model->SetLevelLoadReferenced( true );
				}
			} else if ( p.resType == PRELOAD_PARTICLE ) {
				declManager->FindType( DECL_PARTICLE, p.resourceName );
			}
			numLoaded++;
		}

		int	end = Sys_Milliseconds();
		common->Printf( "%05d models preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001f );
		common->Printf( "----------------------------------------\n" );
	}
}

/*
=================
ARCModelManagerLocal::EndLevelLoad
=================
*/
void ARCModelManagerLocal::EndLevelLoad() {
	common->Printf( "----- ARCModelManagerLocal::EndLevelLoad -----\n" );

	int start = Sys_Milliseconds();

	insideLevelLoad = false;
	int	purgeCount = 0;
	int	keepCount = 0;
	int	loadCount = 0;

	// purge any models not touched
	for ( int i = 0; i < models.Num(); i++ ) {
		ARCRenderModel *model = models[i];
		if ( !model->IsLevelLoadReferenced() && model->IsLoaded() && model->IsReloadable() ) {
			common->Printf( "Begin Level Load - Purging: %s\n", model->Name() );
			purgeCount++;
			R_CheckForEntityDefsUsingModel( model );
			model->PurgeModel();
		} else {
			common->Printf( "Level Load - Not Purged: %s\n", model->Name() );
			keepCount++;
		}
	}

	// purge unused triangle surface memory
	R_PurgeTriSurfData( frameData );

	// load any new ones
	for ( int i = 0; i < models.Num(); i++ ) {
		ARCRenderModel *model = models[i];
		if ( model->IsLevelLoadReferenced() && !model->IsLoaded() && model->IsReloadable() ) {
			loadCount++;
			model->LoadModel();
			if ( ( loadCount & 15 ) == 0 ) {
				session->PacifierUpdate();
			}
		}
	}
	// create static vertex/index buffers for all models
	for ( int i = 0; i < models.Num(); i++ ) {
		common->UpdateLevelLoadPacifier();
		idRenderModel *model = models[i];
		if ( model->IsLoaded() ) {
			for ( int j = 0; j < model->NumSurfaces(); j++ ) {
				R_CreateStaticBuffersForTri( *( model->Surface( j )->geometry ) );
			}
		}
	}

	int	end = Sys_Milliseconds();
	common->Printf( "%5i models purged from previous level, ", purgeCount );
	common->Printf( "%5i models kept.\n", keepCount );
	if ( loadCount ) {
		common->Printf( "%5i new models loaded in %5.1f seconds\n", loadCount, (end-start) * 0.001 );
	}
	common->Printf( "---------------------------------------------------\n" );
}

/*
=================
ARCModelManagerLocal::PrintMemInfo
=================
*/
void ARCModelManagerLocal::PrintMemInfo( MemInfo_t *mi ) {
	arcNetFile *f = fileSystem->OpenFileWrite( mi->filebase + "_models.txt" );
	if ( !f ) {
		return;
	}

	// sort first
	int *sortIndex = new int[ localModelManager.models.Num()];

	for ( int i = 0; i <  localModelManager.models.Num(); i++ ) {
		sortIndex[i] = i;
	}

	for ( int i = 0; i <  localModelManager.models.Num() - 1; i++ ) {
		for ( int j = i + 1; j <  localModelManager.models.Num(); j++ ) {
			if (  localModelManager.models[sortIndex[i]]->Memory() <  localModelManager.models[sortIndex[j]]->Memory() ) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for ( int i = 0; i < localModelManager.models.Num(); i++ ) {
		ARCRenderModel *model = localModelManager.models[sortIndex[i]];
		if ( !model->IsLoaded() ) {
			continue;
		}

		int mem = model->Memory();
		int totalMem += mem;
		f->Printf( "%s %s\n", arcNetString::FormatNumber( mem ).c_str(), model->Name() );
	}

	delete sortIndex;
	mi->modelAssetsTotal = totalMem;

	f->Printf( "\nTotal model bytes allocated: %s\n", arcNetString::FormatNumber( totalMem ).c_str() );
	fileSystem->CloseFile( f );
}
