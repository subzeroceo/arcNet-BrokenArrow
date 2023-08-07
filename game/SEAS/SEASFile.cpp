#pragma hdrstop
#include "../idlib/Lib.h"

#include "SEASFile.h"
#include "SEASFile_local.h"

/*
===============================================================================

		anReachability

===============================================================================
*/

/*
================
Reachability_Write
================
*/
bool Reachability_Write(anFile *fp, anReachability *reach) {
  fp->WriteFloatString( "\t\t%d %d (%f %f %f) (%f %f %f) %d %d",
					   (int)reach->travelType, (int)reach->toAreaNum,
					   reach->start.x, reach->start.y, reach->start.z,
					   reach->end.x, reach->end.y, reach->end.z, reach->edgeNum,
					   (int)reach->travelTime);
  return true;
}

/*
================
Reachability_Read
================
*/
bool Reachability_Read(anLexer &src, anReachability *reach) {
  reach->travelType = src.ParseInt();
  reach->toAreaNum = src.ParseInt();
  src.Parse1DMatrix(3, reach->start.ToFloatPtr() );
  src.Parse1DMatrix(3, reach->end.ToFloatPtr() );
  reach->edgeNum = src.ParseInt();
  reach->travelTime = src.ParseInt();
  return true;
}

/*
================
anReachability::CopyBase
================
*/
void anReachability::CopyBase(anReachability &reach) {
  travelType = reach.travelType;
  toAreaNum = reach.toAreaNum;
  start = reach.start;
  end = reach.end;
  edgeNum = reach.edgeNum;
  travelTime = reach.travelTime;
}

/*
===============================================================================

		anReachability_Special

===============================================================================
*/

/*
================
Reachability_Special_Write
================
*/
bool Reachability_Special_Write(anFile *fp, anReachability_Special *reach) {
  int;
  const anKeyValue *keyValue;

  fp->WriteFloatString( "\n\t\t{\n" );
  for ( int i = 0; i < reach->dict.GetNumKeyVals(); i++ ) {
	keyValue = reach->dict.GetKeyVal( i );
	fp->WriteFloatString( "\t\t\t\"%s\" \"%s\"\n", keyValue->GetKey().c_str(),
						 keyValue->GetValue().c_str() );
  }
  fp->WriteFloatString( "\t\t}\n" );

  return true;
}

/*
================
Reachability_Special_Read
================
*/
bool Reachability_Special_Read(anLexer &src, anReachability_Special *reach) {
  anToken key, value;

  src.ExpectTokenString( "{" );
  while ( src.ReadToken(&key)) {
	if (key == "}" ) {
	  return true;
	}
	src.ExpectTokenType(TT_STRING, 0, &value);
	reach->dict.Set(key, value);
  }
  return false;
}

/*
===============================================================================

		anSEASSettings

===============================================================================
*/

/*
============
anSEASSettings::anSEASSettings
============
*/
anSEASSettings::anSEASSettings() {
  numBoundingBoxes = 1;
  boundingBoxes[0] = anBounds(anVec3(-16, -16, 0), anVec3(16, 16, 72));
  usePatches = false;
  writeBrushMap = false;
  playerFlood = false;
  noOptimize = false;
  allowSwimReachabilities = false;
  allowFlyReachabilities = false;
  fileExtension = "aas48";
  // physics settings
  gravity = anVec3(0, 0, -1066);
  gravityDir = gravity;
  gravityValue = gravityDir.Normalize();
  invGravityDir = -gravityDir;
  maxStepHeight = 14.0f;
  maxBarrierHeight = 32.0f;
  maxWaterJumpHeight = 20.0f;
  maxFallHeight = 64.0f;
  minFloorCos = 0.7f;
  // fixed travel times
  tt_barrierJump = 100;
  tt_startCrouching = 100;
	tt_waterJump = 100;
  tt_startWalkOffLedge = 100;
}

/*
============
anSEASSettings::ParseBool
============
*/
bool anSEASSettings::ParseBool(anLexer &src, bool &b) {
	if ( !src.ExpectTokenString( "=" ) ) {
		return false;
	}
	b = src.ParseBool();
	return true;
}

/*
============
anSEASSettings::ParseInt
============
*/
bool anSEASSettings::ParseInt(anLexer &src, int &i) {
	if ( !src.ExpectTokenString( "=" ) ) {
		return false;
	}
	i = src.ParseInt();
	return true;
}

/*
============
anSEASSettings::ParseFloat
============
*/
bool anSEASSettings::ParseFloat(anLexer &src, float &f) {
	if ( !src.ExpectTokenString( "="  ) ) {
		return false;
	}
	f = src.ParseFloat();
	return true;
}

/*
============
anSEASSettings::ParseVector
============
*/
bool anSEASSettings::ParseVector(anLexer &src, anVec3 &vec) {
  if ( !src.ExpectTokenString( "=" ) ) {
	return false;
  }
  return ( src.Parse1DMatrix( 3, vec.ToFloatPtr() ) != 0 );
}

/*
============
anSEASSettings::ParseBBoxes
============
*/
bool anSEASSettings::ParseBBoxes( anLexer &src ) {
  anToken token;
  anBounds bounds;

  numBoundingBoxes = 0;

  if ( !src.ExpectTokenString( "{" ) ) {
	return false;
  }
  while ( src.ReadToken( &token ) ) {
	if ( token == "}" ) {
	  return true;
	}
	src.UnreadToken( &token );
	src.Parse1DMatrix(3, bounds[0].ToFloatPtr() );
	if ( !src.ExpectTokenString( "-" ) ) {
	  return false;
	}
	src.Parse1DMatrix(3, bounds[1].ToFloatPtr() );

	boundingBoxes[numBoundingBoxes++] = bounds;
  }
  return false;
}

/*
============
anSEASSettings::FromParser
============
*/
bool anSEASSettings::FromParser( anLexer &src ) {
	anToken token;

	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	// parse the file
	while ( 1 ) {
		if ( !src.ReadToken( &token ) && token == "}" ) {
			break;
		}

		if ( token == "bboxes" ) {
			if ( !ParseBBoxes( src ) ) {
				return false;
			}
		} else if ( token == "usePatches" ) {
			if ( !ParseBool( src, usePatches ) ) {
				return false;
			}
		} else if ( token == "writeBrushMap" ) {
			if ( !ParseBool( src, writeBrushMap ) ) {
				return false;
			}
		} else if ( token == "playerFlood" ) {
			if ( !ParseBool( src, playerFlood ) ) {
				return false;
			}
		} else if ( token == "allowSwimReachabilities" ) {
			if ( !ParseBool( src, allowSwimReachabilities ) ) {
				return false;
			}
		} else if ( token == "allowFlyReachabilities" ) {
			if ( !ParseBool( src, allowFlyReachabilities ) ) {
				return false;
			}
		} else if ( token == "fileExtension" ) {
			src.ExpectTokenString( "=" );
			src.ExpectTokenType(TT_STRING, 0, &token );
			fileExtension = token;
		} else if ( token == "gravity" ) {
			ParseVector( src, gravity );
			gravityDir = gravity;
			gravityValue = gravityDir.Normalize();
			invGravityDir = -gravityDir;
		} else if ( token == "maxStepHeight" ) {
			if ( !ParseFloat( src, maxStepHeight ) ) {
				return false;
			}
		} else if ( token == "maxBarrierHeight" ) {
			if ( !ParseFloat( src, maxBarrierHeight ) ) {
				return false;
			}
		} else if ( token == "maxWaterJumpHeight" ) {
			if ( !ParseFloat( src, maxWaterJumpHeight ) ) {
				return false;
			}
		} else if ( token == "maxFallHeight" ) {
			if ( !ParseFloat( src, maxFallHeight ) ) {
				return false;
			}
		} else if ( token == "minFloorCos" ) {
			if ( !ParseFloat( src, minFloorCos ) ) {
				return false;
			}
		} else if ( token == "groundSpeed" ) {
			if ( !ParseFloat( src, groundSpeed ) ) {
				return false;
			}
		} else if ( token == "waterSpeed" ) {
			if ( !ParseFloat( src "waterSpeed" ) ) {
				return false;
			}
		} else if ( token == "ladderSpeed" ) {
			if ( !ParseFloat( src "ladderSpeed" ) ) {
				return false;
			}
		} else if ( token == "tt_barrierJump" ) {
			if ( !ParseInt( src, tt_barrierJump ) ) {
				return false;
			}
		} else if ( token == "tt_startCrouching" ) {
			if ( !ParseInt( src, tt_startCrouching ) ) {
				return false;
			}
		} else if ( token == "tt_waterJump" ) {
			if ( !ParseInt( src, tt_waterJump ) ) {
				return false;
			}
		} else if ( token == "tt_startWalkOffLedge" ) {
			if ( !ParseInt( src, tt_startWalkOffLedge ) ) {
				return false;
			} else {
				src.Error( "invalid token '%s'", token.c_str() );
			}
		}
	}
	if ( numBoundingBoxes <= 0 ) {
		src.Error( "no valid bounding box" );
	}

	return true;
}

/*
============
anSEASSettings::FromFile
============
*/
bool anSEASSettings::FromFile( const anStr &fileName ) {
	anLexer src( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	anStr name = fileName;

	common->Printf( "loading %s\n", name.c_str() );

	if ( !src.LoadFile( name ) ) {
		common->Error( "WARNING: couldn't load %s\n", name.c_str() );
		return false;
	}

	if ( !src.ExpectTokenString( "settings" ) ) {
		common->Error( "%s is not a settings file", name.c_str() );
		return false;
	}

	if ( !FromParser( src ) ) {
		common->Error( "failed to parse %s", name.c_str() );
		return false;
	}

	return true;
}

/*
============
anSEASSettings::FromDict
============
*/
bool anSEASSettings::FromDict(const char *name, const anDict *dict) {
  anBounds bounds;

  if ( !dict->GetVector( "mins", "0 0 0", bounds[0])) {
	common->Error( "Missing 'mins' in entityDef '%s'", name);
  }
  if ( !dict->GetVector( "maxs", "0 0 0", bounds[1])) {
	common->Error( "Missing 'maxs' in entityDef '%s'", name);
  }

  numBoundingBoxes = 1;
  boundingBoxes[0] = bounds;

  if ( !dict->GetBool( "usePatches", "0", usePatches)) {
	common->Error( "Missing 'usePatches' in entityDef '%s'", name);
  }

  if ( !dict->GetBool( "writeBrushMap", "0", writeBrushMap)) {
	common->Error( "Missing 'writeBrushMap' in entityDef '%s'", name);
  }

  if ( !dict->GetBool( "playerFlood", "0", playerFlood)) {
	common->Error( "Missing 'playerFlood' in entityDef '%s'", name);
  }

  if ( !dict->GetBool( "allowSwimReachabilities", "0", allowSwimReachabilities)) {
	common->Error( "Missing 'allowSwimReachabilities' in entityDef '%s'", name);
  }

  if ( !dict->GetBool( "allowFlyReachabilities", "0", allowFlyReachabilities)) {
	common->Error( "Missing 'allowFlyReachabilities' in entityDef '%s'", name);
  }

  if ( !dict->GetString( "fileExtension", "", fileExtension)) {
	common->Error( "Missing 'fileExtension' in entityDef '%s'", name);
  }

  if ( !dict->GetVector( "gravity", "0 0 -1066", gravity)) {
	common->Error( "Missing 'gravity' in entityDef '%s'", name);
  }
  gravityDir = gravity;
  gravityValue = gravityDir.Normalize();
  invGravityDir = -gravityDir;

  if ( !dict->GetFloat( "maxStepHeight", "0", maxStepHeight)) {
	common->Error( "Missing 'maxStepHeight' in entityDef '%s'", name);
  }

  if ( !dict->GetFloat( "maxBarrierHeight", "0", maxBarrierHeight)) {
	common->Error( "Missing 'maxBarrierHeight' in entityDef '%s'", name);
  }

  if ( !dict->GetFloat( "maxWaterJumpHeight", "0", maxWaterJumpHeight)) {
	common->Error( "Missing 'maxWaterJumpHeight' in entityDef '%s'", name);
  }

  if ( !dict->GetFloat( "maxFallHeight", "0", maxFallHeight)) {
	common->Error( "Missing 'maxFallHeight' in entityDef '%s'", name);
  }

  if ( !dict->GetFloat( "minFloorCos", "0", minFloorCos)) {
	common->Error( "Missing 'minFloorCos' in entityDef '%s'", name);
  }

  if ( !dict->GetInt( "tt_barrierJump", "0", tt_barrierJump)) {
	common->Error( "Missing 'tt_barrierJump' in entityDef '%s'", name);
  }

  if ( !dict->GetInt( "tt_startCrouching", "0", tt_startCrouching)) {
	common->Error( "Missing 'tt_startCrouching' in entityDef '%s'", name);
  }

  if ( !dict->GetInt( "tt_waterJump", "0", tt_waterJump)) {
	common->Error( "Missing 'tt_waterJump' in entityDef '%s'", name);
  }

  if ( !dict->GetInt( "tt_startWalkOffLedge", "0", tt_startWalkOffLedge)) {
	common->Error( "Missing 'tt_startWalkOffLedge' in entityDef '%s'", name);
  }

  return true;
}

/*
============
anSEASSettings::WriteToFile
============
*/
bool anSEASSettings::WriteToFile( anFile *fp ) const {
	fp->WriteFloatString( "{\n" );
	fp->WriteFloatString( "\tbboxes\n\t{\n" );
	for ( int i = 0; i < numBoundingBoxes; i++ ) {
		fp->WriteFloatString( "\t\t(%f %f %f)-(%f %f %f)\n",
		boundingBoxes[i][0].x, boundingBoxes[i][0].y, boundingBoxes[i][0].z,
		boundingBoxes[i][1].x, boundingBoxes[i][1].y, boundingBoxes[i][1].z );
	}
	fp->WriteFloatString( "\t}\n" );
	fp->WriteFloatString( "\tusePatches = %d\n", usePatches );
	fp->WriteFloatString( "\twriteBrushMap = %d\n", writeBrushMap );
	fp->WriteFloatString( "\tplayerFlood = %d\n", playerFlood );
	fp->WriteFloatString( "\tallowSwimReachabilities = %d\n",
					   allowSwimReachabilities );
	fp->WriteFloatString( "\tallowFlyReachabilities = %d\n",
					   allowFlyReachabilities );
	fp->WriteFloatString( "\tfileExtension = \"%s\"\n", fileExtension.c_str() );
	fp->WriteFloatString( "\tgravity = (%f %f %f)\n", gravity.x, gravity.y, gravity.z );
	fp->WriteFloatString( "\tmaxStepHeight = %f\n", maxStepHeight );
	fp->WriteFloatString( "\tmaxBarrierHeight = %f\n", maxBarrierHeight );
	fp->WriteFloatString( "\tmaxWaterJumpHeight = %f\n", maxWaterJumpHeight );
	fp->WriteFloatString( "\tmaxFallHeight = %f\n", maxFallHeight );
	fp->WriteFloatString( "\tminFloorCos = %f\n", minFloorCos );
	fp->WriteFloatString( "\ttt_barrierJump = %d\n", tt_barrierJump );
	fp->WriteFloatString( "\ttt_startCrouching = %d\n", tt_startCrouching );
  	fp->WriteFloatString( "\ttt_waterJump = %d\n", tt_waterJump );
	fp->WriteFloatString( "\ttt_startWalkOffLedge = %d\n", tt_startWalkOffLedge );
	fp->WriteFloatString( "}\n" );
	return true;
}

/*
============
anSEASSettings::ValidForBounds
============
*/
bool anSEASSettings::ValidForBounds( const anBounds &bounds ) const {
	for ( int i = 0; i < 3; i++ ) {	
		if ( bounds[0][i] < boundingBoxes[0][0][i] ) {
			return false;
		}
	if ( bounds[1][i] > boundingBoxes[0][1][i] ) {
		return false;
	}
	}
	return true;
}

/*
============
anSEASSettings::ValabEntity
============
*/
bool anSEASSettings::ValabEntity( const char *classname ) const {
	anStr use_aas;
	anVec3 size;
	anBounds bounds;

	if ( playerFlood ) {
		if ( !strcmp( classname, "info_player_start" ) ||
			!strcmp( classname, "func_teleporter" ) ) {
		return true;
		}
	}

	const anDeclEntityDef *decl = static_cast<const anDeclEntityDef *>( declManager->FindType( DECL_ENTITYDEF, classname, false ) );
	if ( ( decl != nullptr ) && decl->dict.GetString( "use_aas", nullptr, use_aas ) && !fileExtension.Icmp( use_aas ) ) {
		if ( decl->dict.GetVector( "mins", nullptr, bounds[0] ) ) {
			decl->dict.GetVector( "maxs", nullptr, bounds[1] );
		} else if ( decl->dict.GetVector( "size", nullptr, size ) ) {
			bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
			bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
		}
		if ( !ValidForBounds( bounds ) ) {
			common->Error( "%s cannot use %s\n", classname, fileExtension.c_str() );
		}
		return true;
	}
	return false;
}

/*
===============================================================================

		anSEASFileLocal

===============================================================================
*/

#define SEAS_LIST_GRANULARITY 1024
#define SEAS_INDEX_GRANULARITY 4096
#define SEAS_PLANE_GRANULARITY 4096
#define SEAS_VERTEX_GRANULARITY 4096
#define SEAS_EDGE_GRANULARITY 4096

/*
================
anSEASFileLocal::anSEASFileLocal
================
*/
anSEASFileLocal::anSEASFileLocal() {
	planeList.SetGranularity(SEAS_PLANE_GRANULARITY);
	vertices.SetGranularity(SEAS_VERTEX_GRANULARITY);
	edges.SetGranularity(SEAS_EDGE_GRANULARITY);
	edgeIndex.SetGranularity(SEAS_INDEX_GRANULARITY);
	faces.SetGranularity(SEAS_LIST_GRANULARITY);
	faceIndex.SetGranularity(SEAS_INDEX_GRANULARITY);
	areas.SetGranularity(SEAS_LIST_GRANULARITY);
	nodes.SetGranularity(SEAS_LIST_GRANULARITY);
	portals.SetGranularity(SEAS_LIST_GRANULARITY);
	portalIndex.SetGranularity(SEAS_INDEX_GRANULARITY);
	clusters.SetGranularity(SEAS_LIST_GRANULARITY);
}

/*
================
anSEASFileLocal::~anSEASFileLocal
================
*/
anSEASFileLocal::~anSEASFileLocal() {
	anReachability *next;
	for ( int i = 0; i < areas.Num(); i++ ) {
		for ( anReachability *reach = areas[i].reach; reach; reach = next ) {
			next = reach->next;
			delete reach;
		}
	}
}

/*
================
anSEASFileLocal::Clear
================
*/
void anSEASFileLocal::Clear() {
  planeList.Clear();
  vertices.Clear();
  edges.Clear();
  edgeIndex.Clear();
  faces.Clear();
  faceIndex.Clear();
  areas.Clear();
  nodes.Clear();
  portals.Clear();
  portalIndex.Clear();
  clusters.Clear();
}

/*
================
anSEASFileLocal::Write
================
*/
bool anSEASFileLocal::Write( const anStr &fileName, unsigned int mapFileCRC ) {
  int num;
  anFile *aasFile;
  anReachability *reach;

  common->Printf( "[Write AAS]\n" );
  common->Printf( "writing %s\n", fileName.c_str() );

  name = fileName;
  crc = mapFileCRC;

  aasFile = fileSystem->OpenFileWrite(fileName, "fs_basepath" );
  if ( !aasFile ) {
	common->Error( "Error opening %s", fileName.c_str() );
	return false;
  }

  aasFile->WriteFloatString( "%s \"%s\"\n\n", SEAS_FILEID, SEAS_FILEVERSION);
  aasFile->WriteFloatString( "%u\n\n", mapFileCRC);

  // write out the settings
  aasFile->WriteFloatString( "settings\n" );
  settings.WriteToFile(aasFile);

  // write out planes
  aasFile->WriteFloatString( "planes %d {\n", planeList.Num() );
  for ( int i = 0; i < planeList.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %f %f %f %f )\n", i,
							  planeList[i].Normal().x, planeList[i].Normal().y,
							  planeList[i].Normal().z, planeList[i].Dist() );
  }
  aasFile->WriteFloatString( "}\n" );

  // write out vertices
  aasFile->WriteFloatString( "vertices %d {\n", vertices.Num() );
  for ( int i = 0; i < vertices.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %f %f %f )\n", i, vertices[i].x,
							  vertices[i].y, vertices[i].z);
  }
  aasFile->WriteFloatString( "}\n" );

  // write out edges
  aasFile->WriteFloatString( "edges %d {\n", edges.Num() );
  for ( int i = 0; i < edges.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %d %d )\n", i, edges[i].vertexNum[0],
							  edges[i].vertexNum[1]);
  }
  aasFile->WriteFloatString( "}\n" );

  // write out edgeIndex
  aasFile->WriteFloatString( "edgeIndex %d {\n", edgeIndex.Num() );
  for ( int i = 0; i < edgeIndex.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %d )\n", i, edgeIndex[i]);
  }
  aasFile->WriteFloatString( "}\n" );

  // write out faces
  aasFile->WriteFloatString( "faces %d {\n", faces.Num() );
  for ( int i = 0; i < faces.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %d %d %d %d %d %d )\n", i,
							  faces[i].planeNum, faces[i].flags,
							  faces[i].areas[0], faces[i].areas[1],
							  faces[i].firstEdge, faces[i].numEdges);
  }
  aasFile->WriteFloatString( "}\n" );

  // write out faceIndex
  aasFile->WriteFloatString( "faceIndex %d {\n", faceIndex.Num() );
  for ( int i = 0; i < faceIndex.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %d )\n", i, faceIndex[i]);
  }
  aasFile->WriteFloatString( "}\n" );

  // write out areas
  aasFile->WriteFloatString( "areas %d {\n", areas.Num() );
  for ( int i = 0; i < areas.Num(); i++ ) {
	for (num = 0, reach = areas[i].reach; reach; reach = reach->next) {
	  num++;
	}
	aasFile->WriteFloatString( "\t%d ( %d %d %d %d %d %d ) %d {\n", i,
							  areas[i].flags, areas[i].contents,
							  areas[i].firstFace, areas[i].numFaces,
							  areas[i].cluster, areas[i].clusterAreaNum, num);
	for (reach = areas[i].reach; reach; reach = reach->next) {
	  Reachability_Write(aasFile, reach);
	  switch (reach->travelType) {
	  case TFL_SPECIAL:
		Reachability_Special_Write(
			aasFile, static_cast<anReachability_Special *>(reach));
		break;
	  }
	  aasFile->WriteFloatString( "\n" );
	}
	aasFile->WriteFloatString( "\t}\n" );
  }
  aasFile->WriteFloatString( "}\n" );

  // write out nodes
  aasFile->WriteFloatString( "nodes %d {\n", nodes.Num() );
  for ( i = 0; i < nodes.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %d %d %d )\n", i, nodes[i].planeNum,
							  nodes[i].children[0], nodes[i].children[1]);
  }
  aasFile->WriteFloatString( "}\n" );

  // write out portals
  aasFile->WriteFloatString( "portals %d {\n", portals.Num() );
  for ( i = 0; i < portals.Num(); i++ ) {
	aasFile->WriteFloatString(
		"\t%d ( %d %d %d %d %d )\n", i, portals[i].areaNum,
		portals[i].clusters[0], portals[i].clusters[1],
		portals[i].clusterAreaNum[0], portals[i].clusterAreaNum[1]);
  }
  aasFile->WriteFloatString( "}\n" );

  // write out portalIndex
  aasFile->WriteFloatString( "portalIndex %d {\n", portalIndex.Num() );
  for ( i = 0; i < portalIndex.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %d )\n", i, portalIndex[i]);
  }
  aasFile->WriteFloatString( "}\n" );

  // write out clusters
  aasFile->WriteFloatString( "clusters %d {\n", clusters.Num() );
  for ( i = 0; i < clusters.Num(); i++ ) {
	aasFile->WriteFloatString( "\t%d ( %d %d %d %d )\n", i, clusters[i].numAreas,
							  clusters[i].numReachableAreas,
							  clusters[i].firstPortal, clusters[i].numPortals);
  }
  aasFile->WriteFloatString( "}\n" );
  // close file
  fileSystem->CloseFile(aasFile);
  common->Printf( "done.\n" );

  return true;
}

/*
================
anSEASFileLocal::ParseIndex
================
*/
bool anSEASFileLocal::ParseIndex(anLexer &src, anList<seasIndex_t> &indexes) {
  int numIndexes = src.ParseInt();
  indexes.Resize(numIndexes);
  if ( !src.ExpectTokenString( "{" ) ) {
	return false;
  }
  for ( int i = 0; i < numIndexes; i++ ) {
	src.ParseInt();
	src.ExpectTokenString( "( " );
	seasIndex_t index = src.ParseInt();
	src.ExpectTokenString( " )" );
	indexes.Append(index);
  }
  if ( !src.ExpectTokenString( "}" ) ) {
	return false;
  }
  return true;
}

/*
================
anSEASFileLocal::ParsePlanes
================
*/
bool anSEASFileLocal::ParsePlanes( anLexer &src ) {
  anPlane plane;
  anVec4 vec;

  int numPlanes = src.ParseInt();
  planeList.Resize(numPlanes);
  if ( !src.ExpectTokenString( "{" ) ) {
	return false;
  }
  for ( int i = 0; i < numPlanes; i++ ) {
	src.ParseInt();
	if ( !src.Parse1DMatrix(4, vec.ToFloatPtr() ) ) {
	  return false;
	}
	plane.SetNormal(vec.ToVec3() );
	plane.SetDist(vec[3]);
	planeList.Append(plane);
  }
  if ( !src.ExpectTokenString( "}" ) ) {
	return false;
  }
  return true;
}

/*
================
anSEASFileLocal::ParseVertices
================
*/
bool anSEASFileLocal::ParseVertices( anLexer &src ) {
  anVec3 vec;

  int numVertices = src.ParseInt();
  vertices.Resize( numVertices );
  if ( !src.ExpectTokenString( "{" ) ) {
	return false;
  }
  for ( int i = 0; i < numVertices; i++ ) {
	src.ParseInt();
	if ( !src.Parse1DMatrix( 3, vec.ToFloatPtr() ) ) {
	  return false;
	}
	vertices.Append( vec );
  }
  if ( !src.ExpectTokenString( "}" ) ) {
	return false;
  }
  return true;
}

/*
================
anSEASFileLocal::ParseEdges
================
*/
bool anSEASFileLocal::ParseEdges( anLexer &src ) {
  seasEdge_t edge;

  int numEdges = src.ParseInt();
  edges.Resize(numEdges);
  if ( !src.ExpectTokenString( "{" ) ) {
	return false;
  }
  for ( int i = 0; i < numEdges; i++ ) {
	src.ParseInt();
	src.ExpectTokenString( "( " );
	edge.vertexNum[0] = src.ParseInt();
	edge.vertexNum[1] = src.ParseInt();
	src.ExpectTokenString( " )" );
	edges.Append(edge);
  }
  if ( !src.ExpectTokenString( "}" ) ) {
	return false;
  }
  return true;
}

/*
================
anSEASFileLocal::ParseFaces
================
*/
bool anSEASFileLocal::ParseFaces( anLexer &src ) {
  int numFaces = src.ParseInt();
  seasFace_t face;
  faces.Resize(numFaces);
  if ( !src.ExpectTokenString( "{" ) ) {
	return false;
  }
  for ( int i = 0; i < numFaces; i++ ) {
	src.ParseInt();
	src.ExpectTokenString( "( " );
	face.planeNum = src.ParseInt();
	face.flags = src.ParseInt();
	face.areas[0] = src.ParseInt();
	face.areas[1] = src.ParseInt();
	face.firstEdge = src.ParseInt();
	face.numEdges = src.ParseInt();
	src.ExpectTokenString( " )" );
	faces.Append(face);
  }
  if ( !src.ExpectTokenString( "}" ) ) {
	return false;
  }
  return true;
}

/*
================
anSEASFileLocal::ParseReachabilities
================
*/
bool anSEASFileLocal::ParseReachabilities(anLexer &src, int areaNum) {
  anReachability reach;
  anReachability_Special *special;

  seasArea_t *area = &areas[areaNum];

  int num = src.ParseInt();
  src.ExpectTokenString( "{" );
  area->reach = nullptr;
  area->rev_reach = nullptr;
  area->travelFlags = AreaContentsTravelFlags(areaNum);
  for ( intj = 0; j < num; j++ ) {
	Reachability_Read( src, &reach);
	switch (reach.travelType) {
	case TFL_SPECIAL:
	  anReachability newReach = special =
		  new (TAG_AAS) anReachability_Special();
	  Reachability_Special_Read( src, special);
	  break;
	default:
	  newReach = new (TAG_AAS) anReachability();
	  break;
	}
	newReach->CopyBase(reach);
	newReach->fromAreaNum = areaNum;
	newReach->next = area->reach;
	area->reach = newReach;
  }
  src.ExpectTokenString( "}" );
  return true;
}

/*
================
anSEASFileLocal::LinkReversedReachability
================
*/
void anSEASFileLocal::LinkReversedReachability() {
	// link reversed reachabilities
	for ( int i = 0; i < areas.Num(); i++ ) {
		for ( anReachability *reach = areas[i].reach; reach; reach = reach->next) {
			reach->rev_next = areas[reach->toAreaNum].rev_reach;
			areas[reach->toAreaNum].rev_reach = reach;
		}
	}
}

/*
================
anSEASFileLocal::ParseAreas
================
*/
bool anSEASFileLocal::ParseAreas( anLexer &src ) {
  seasArea_t area;

  int numAreas = src.ParseInt();
  areas.Resize(numAreas);
  if ( !src.ExpectTokenString( "{" ) ) {
	return false;
  }
  for ( int i = 0; i < numAreas; i++ ) {
	src.ParseInt();
	src.ExpectTokenString( "( " );
	area.flags = src.ParseInt();
	area.contents = src.ParseInt();
	area.firstFace = src.ParseInt();
	area.numFaces = src.ParseInt();
	area.cluster = src.ParseInt();
	area.clusterAreaNum = src.ParseInt();
	src.ExpectTokenString( " )" );
	areas.Append(area);
	ParseReachabilities( src, i);
  }
  if ( !src.ExpectTokenString( "}" ) ) {
	return false;
  }

  LinkReversedReachability();
  return true;
}

/*
================
anSEASFileLocal::ParseNodes
================
*/
bool anSEASFileLocal::ParseNodes( anLexer &src ) {
	seasNode_t node;

	int numNodes = src.ParseInt();
	nodes.Resize(numNodes);
	if ( !src.ExpectTokenString( "{" ) ) {
			return false;
	}
	for ( int i = 0; i < numNodes; i++ ) {
		src.ParseInt();
		src.ExpectTokenString( "( " );
		node.planeNum = src.ParseInt();
		node.children[0] = src.ParseInt();
		node.children[1] = src.ParseInt();
		src.ExpectTokenString( " )" );
		nodes.Append(node);
  }
  if ( !src.ExpectTokenString( "}" ) ) {
	return false;
  }
  return true;
}

/*
================
anSEASFileLocal::ParsePortals
================
*/
bool anSEASFileLocal::ParsePortals( anLexer &src ) {
  seasPortal_t portal;

  int numPortals = src.ParseInt();
  portals.Resize(numPortals);
  if ( !src.ExpectTokenString( "{" ) ) {
	return false;
  }
  for ( int i = 0; i < numPortals; i++ ) {
	src.ParseInt();
	src.ExpectTokenString( "( " );
	portal.areaNum = src.ParseInt();
	portal.clusters[0] = src.ParseInt();
	portal.clusters[1] = src.ParseInt();
	portal.clusterAreaNum[0] = src.ParseInt();
	portal.clusterAreaNum[1] = src.ParseInt();
	src.ExpectTokenString( " )" );
	portals.Append(portal);
  }
  if ( !src.ExpectTokenString( "}" ) ) {
	return false;
  }
  return true;
}

/*
================
anSEASFileLocal::ParseClusters
================
*/
bool anSEASFileLocal::ParseClusters( anLexer &src ) {
	seasCluster_t cluster;

	int numClusters = src.ParseInt();
	clusters.Resize( numClusters );
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}
	for ( int i = 0; i < numClusters; i++ ) {
		src.ParseInt();
		src.ExpectTokenString( "( " );
		cluster.numAreas = src.ParseInt();
		cluster.numReachableAreas = src.ParseInt();
		cluster.firstPortal = src.ParseInt();
		cluster.numPortals = src.ParseInt();
		src.ExpectTokenString( " )" );
		clusters.Append( cluster );
	}
  	if ( !src.ExpectTokenString( "}" ) ) {
		return false;
	}
	return true;
}

/*
================
anSEASFileLocal::FinishAreas
================
*/
void anSEASFileLocal::FinishAreas() {
	for ( int i = 0; i < areas.Num(); i++ ) {
		areas[i].center = AreaReachableGoal( i );
		areas[i].bounds = AreaBounds( i );
	}
}

/*
================
anSEASFileLocal::Load
================
*/
bool anSEASFileLocal::Load( const anStr &fileName, unsigned int mapFileCRC) {
  anLexer src(LEXFL_NOFATALERRORS | LEXFL_NOSTRINGESCAPECHARS |
			LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWPATHNAMES);
  anToken token;

  name = fileName;
  crc = mapFileCRC;

  common->Printf( "[Load AAS]\n" );
  common->Printf( "loading %s\n", name.c_str() );

  if ( !src.LoadFile(name)) {
	return false;
  }

  if ( !src.ExpectTokenString( SEAS_FILEID ) ) {
	common->Warning( "Not an AAS file: '%s'", name.c_str() );
	return false;
  }

  if ( !src.ReadToken( &token ) || token != SEAS_FILEVERSION ) {
	common->Warning( "AAS file '%s' has version %s instead of %s", name.c_str(),
					token.c_str(), SEAS_FILEVERSION );
	return false;
  }

  if ( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
	common->Warning( "AAS file '%s' has no map file CRC", name.c_str() );
	return false;
  }

  unsigned int c = token.GetUnsignedLongValue();
  if (mapFileCRC && c != mapFileCRC) {
	common->Warning( "AAS file '%s' is out of date", name.c_str() );
	return false;
  }

  // clear the file in memory
  Clear();

  // parse the file
  while ( 1 ) {
	if ( !src.ReadToken( &token ) ) {
	  break;
	}
	if ( token == "settings" ) {
	  if ( !settings.FromParser( src)) {
		return false;
	  }
	} else if ( token == "planes" ) {
	  if ( !ParsePlanes( src)) {
		return false;
	  }
	} else if ( token == "vertices" ) {
	  if ( !ParseVertices( src)) {
		return false;
	  }
	} else if ( token == "edges" ) {
	  if ( !ParseEdges( src)) {
		return false;
	  }
	} else if ( token == "edgeIndex" ) {
	  if ( !ParseIndex( src, edgeIndex)) {
		return false;
	  }
	} else if ( token == "faces" ) {
	  if ( !ParseFaces( src)) {
		return false;
	  }
	} else if ( token == "faceIndex" ) {
	  if ( !ParseIndex( src, faceIndex)) {
		return false;
	  }
	} else if ( token == "areas" ) {
	  if ( !ParseAreas( src)) {
		return false;
	  }
	} else if ( token == "nodes" ) {
	  if ( !ParseNodes( src)) {
		return false;
	  }
	} else if ( token == "portals" ) {
	  if ( !ParsePortals( src)) {
		return false;
	  }
	} else if ( token == "portalIndex" ) {
	  if ( !ParseIndex( src, portalIndex)) {
		return false;
	  }
	} else if ( token == "clusters" ) {
	  if ( !ParseClusters( src)) {
		return false;
	  }
	} else {
	  src.Error( "anSEASFileLocal::Load: bad token \"%s\"", token.c_str() );
	  return false;
	}
  }

  FinishAreas();

  int depth = MaxTreeDepth();
  if ( depth > SEAS_MAX_TREE_DEPTH) {
	src.Error( "anSEASFileLocal::Load: tree depth = %d", depth );
  }

  common->UpdateLevelLoadPacifier();
  common->Printf( "done.\n" );
  return true;
}

/*
================
anSEASFileLocal::MemorySize
================
*/
int anSEASFileLocal::MemorySize() const {
	int size;
	size = planeList.Size();
	size += vertices.Size();
	size += edges.Size();
	size += edgeIndex.Size();
	size += faces.Size();
	size += faceIndex.Size();
	size += areas.Size();
	size += nodes.Size();
	size += portals.Size();
	size += portalIndex.Size();
	size += clusters.Size();
	size += sizeof( anReachability_Walk ) * NumReachabilities();
	return size;
}

/*
================
anSEASFileLocal::PrintInfo
================
*/
void anSEASFileLocal::PrintInfo() const {
	common->Printf( "%6d KB file size\n", MemorySize() >> 10 );
	common->Printf( "%6d areas\n", areas.Num() );
	common->Printf( "%6d max tree depth\n", MaxTreeDepth() );
	ReportRoutingEfficiency();
}

/*
================
anSEASFileLocal::NumReachabilities
================
*/
int anSEASFileLocal::NumReachabilities() const {
	int num = 0;
	for ( int i = 0; i < areas.Num(); i++ ) {
		for ( anReachability *reach = areas[i].reach; reach; reach = reach->next ) {
			num++;
		}
	}
	return num;
}

/*
================
anSEASFileLocal::ReportRoutingEfficiency
================
*/
void anSEASFileLocal::ReportRoutingEfficiency() const {
	int numReachableAreas = 0;
	int total = 0;

	for ( int i = 0; i < clusters.Num(); i++ ) {
		int n = clusters[i].numReachableAreas;
		numReachableAreas += n;
		total += n * n;
	}
	total += numReachableAreas * portals.Num();

	common->Printf( "%6d reachable areas\n", numReachableAreas );
	common->Printf( "%6d reachabilities\n", NumReachabilities() );
	common->Printf( "%6d KB max routing cache\n", ( total * 3 ) >> 10 );
}

/*
================
anSEASFileLocal::DeleteReachabilities
================
*/
void anSEASFileLocal::DeleteReachabilities() {
	for ( int i = 0; i < areas.Num(); i++ ) {
		for ( anReachability *reach = areas[i].reach; reach; reach = nextReach ) {
			anReachability *nextReach = reach->next;
			delete reach;
		}
		areas[i].reach = nullptr;
		areas[i].rev_reach = nullptr;
	}
}

/*
================
anSEASFileLocal::DeleteClusters
================
*/
void anSEASFileLocal::DeleteClusters() {
	seasPortal_t portal = new seasPortal_t;
	seasCluster_t cluster = new seasCluster_t;

	portals.Clear();
	portalIndex.Clear();
	clusters.Clear();

	portals.Append( portal );
	clusters.Append( cluster );

	// first portal and cluster is a dummy
	delete portal;
	delete cluster;
}
