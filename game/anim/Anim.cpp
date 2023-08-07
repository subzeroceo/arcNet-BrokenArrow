#pragma hdrstop
#include "../../idlib/Lib.h"
#include "../Game_local.h"

anCVar binaryLoadAnim( "LoadBinaryAnim", "1", 0, "enables the engine to read/load/write of binary MD6B Animation files." );

static const byte B_ANIM_MD6_VERSION = 101;
static const unsigned int B_ANIM_MD6_MAGIC = ( 'B' << 24 ) | ( 'M' << 16 ) | ( 'D' << 8 ) | B_ANIM_MD6_VERSION;

static const int JOINT_FRAME_PAD	= 1;	// one extra to be able to read one more float than is necessary

bool anAnimManager::forceExport = false;

/***********************************************************************

	anMD6Anim

***********************************************************************/

/*
====================
anMD6Anim::anMD6Anim
====================
*/
anMD6Anim::anMD6Anim() {
	refCount	= 0;
	numFrames	= 0;
	numJoints	= 0;
	frameRate	= 24;
	animLength	= 0;
	numAnimatedComponents = 0;
	totaldelta.Zero();
}

/*
====================
anMD6Anim::anMD6Anim
====================
*/
anMD6Anim::~anMD6Anim() {
	Free();
}

/*
====================
anMD6Anim::Free
====================
*/
void anMD6Anim::Free() {
	numFrames	= 0;
	numJoints	= 0;
	frameRate	= 24;
	animLength	= 0;
	numAnimatedComponents = 0;
	//name		= "";

	totaldelta.Zero();

	jointInfo.Clear();
	bounds.Clear();
	componentFrames.Clear();
}

/*
====================
anMD6Anim::NumFrames
====================
*/
int	anMD6Anim::NumFrames() const {
	return numFrames;
}

/*
====================
anMD6Anim::NumJoints
====================
*/
int	anMD6Anim::NumJoints() const {
	return numJoints;
}

/*
====================
anMD6Anim::Length
====================
*/
int anMD6Anim::Length() const {
	return animLength;
}

/*
=====================
anMD6Anim::TotalMovementDelta
=====================
*/
const anVec3 &anMD6Anim::TotalMovementDelta() const {
	return totaldelta;
}

/*
=====================
anMD6Anim::TotalMovementDelta
=====================
*/
const char *anMD6Anim::Name() const {
	return name;
}

/*
====================
anMD6Anim::Reload
====================
*/
bool anMD6Anim::Reload() {
	anStr filename;
	filename = name;
	Free();
	return LoadAnim( filename );
}

/*
====================
anMD6Anim::Allocated
====================
*/
size_t anMD6Anim::Allocated() const {
	size_t size = bounds.Allocated() + jointInfo.Allocated() + componentFrames.Allocated() + name.Allocated();
	return size;
}

/*
====================
anMD6Anim::LoadAnim
====================
*/
bool anMD6Anim::LoadAnim( const char *filename ) {
	anLexer	parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	anToken	token;

	anStr generatedFileName = "generated/anim/";
	generatedFileName.AppendPath( filename );
	generatedFileName.SetFileExtension( ".bMD5anim" );

	// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it
	ARC_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( filename );

	anFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
	if ( binaryLoadAnim.GetBool() && LoadBinary( file, sourceTimeStamp ) ) {
		name = filename;
		if ( cvarSystem->GetCVarBool( "fs_buildresources" ) ) {
			// for resource gathering write this anim to the preload file for this map
			fileSystem->AddAnimPreload( name );
		}
		return true;
	}

	if ( !parser.LoadFile( filename ) ) {
		return false;
	}

	name = filename;

	Free();

	parser.ExpectTokenString( MD5_VERSION_STRING );
	int version = parser.ParseInt();
	if ( version != MD5_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}

	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if ( numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", numFrames );
	}

	// parse num joints
	parser.ExpectTokenString( "numJoints" );
	numJoints = parser.ParseInt();
	if ( numJoints <= 0 ) {
		parser.Error( "Invalid number of joints: %d", numJoints );
	}

	// parse frame rate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if ( frameRate < 0 ) {
		parser.Error( "Invalid frame rate: %d", frameRate );
	}

	// parse number of animated components
	parser.ExpectTokenString( "numAnimatedComponents" );
	numAnimatedComponents = parser.ParseInt();
	if ( ( numAnimatedComponents < 0 ) || ( numAnimatedComponents > numJoints * 6 ) ) {
		parser.Error( "Invalid number of animated components: %d", numAnimatedComponents );
	}

	// parse the hierarchy
	jointInfo.SetGranularity( 1 );
	jointInfo.SetNum( numJoints );
	parser.ExpectTokenString( "hierarchy" );
	parser.ExpectTokenString( "{" );
	for ( int i = 0; i < numJoints; i++ ) {
		parser.ReadToken( &token );
		jointInfo[i].nameIndex = animationLib.JointIndex( token );

		// parse parent num
		jointInfo[i].parentNum = parser.ParseInt();
		if ( jointInfo[i].parentNum >= i ) {
			parser.Error( "Invalid parent num: %d", jointInfo[i].parentNum );
		}

		if ( ( i != 0 ) && ( jointInfo[i].parentNum < 0 ) ) {
			parser.Error( "Animations may have only one root joint" );
		}

		// parse anim bits
		jointInfo[i].animBits = parser.ParseInt();
		if ( jointInfo[i].animBits & ~63 ) {
			parser.Error( "Invalid anim bits: %d", jointInfo[i].animBits );
		}

		// parse first component
		jointInfo[i].firstComponent = parser.ParseInt();
		if ( ( numAnimatedComponents > 0 ) && ( ( jointInfo[i].firstComponent < 0 ) || ( jointInfo[i].firstComponent >= numAnimatedComponents ) ) ) {
			parser.Error( "Invalid first component: %d", jointInfo[i].firstComponent );
		}
	}

	parser.ExpectTokenString( "}" );

	// parse bounds
	parser.ExpectTokenString( "bounds" );
	parser.ExpectTokenString( "{" );
	bounds.SetGranularity( 1 );
	bounds.SetNum( numFrames );
	for ( int i = 0; i < numFrames; i++ ) {
		parser.Parse1DMatrix( 3, bounds[i][0].ToFloatPtr() );
		parser.Parse1DMatrix( 3, bounds[i][1].ToFloatPtr() );
	}
	parser.ExpectTokenString( "}" );

	// parse base frame
	baseFrame.SetGranularity( 1 );
	baseFrame.SetNum( numJoints );
	parser.ExpectTokenString( "baseframe" );
	parser.ExpectTokenString( "{" );
	for ( int i = 0; i < numJoints; i++ ) {
		anCQuat q;
		parser.Parse1DMatrix( 3, baseFrame[i].t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, q.ToFloatPtr() );//baseFrame[i].q.ToFloatPtr() );
		baseFrame[i].q = q.ToQuat();//.w = baseFrame[i].q.CalcW();
		baseFrame[i].w = 0.0f;
	}
	parser.ExpectTokenString( "}" );

	// parse frames
	componentFrames.SetGranularity( 1 );
	componentFrames.SetNum( numAnimatedComponents * numFrames + JOINT_FRAME_PAD );
	componentFrames[numAnimatedComponents * numFrames + JOINT_FRAME_PAD - 1] = 0.0f;

	float *componentPtr = componentFrames.Ptr();
	for ( int i = 0; i < numFrames; i++ ) {
		parser.ExpectTokenString( "frame" );
		int num = parser.ParseInt();
		if ( num != i ) {
			parser.Error( "Expected frame number %d", i );
		}
		parser.ExpectTokenString( "{" );

		for ( int j = 0; j < numAnimatedComponents; j++, componentPtr++ ) {
			*componentPtr = parser.ParseFloat();
		}

		parser.ExpectTokenString( "}" );
	}

	// get total move delta
	if ( !numAnimatedComponents ) {
		totaldelta.Zero();
	} else {
		componentPtr = &componentFrames[ jointInfo[0].firstComponent ];
		if ( jointInfo[0].animBits & ANIM_TX ) {
			for ( int i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[0].t.x;
			}
			totaldelta.x = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		} else {
			totaldelta.x = 0.0f;
		}
		if ( jointInfo[0].animBits & ANIM_TY ) {
			for ( int i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[0].t.y;
			}
			totaldelta.y = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
			componentPtr++;
		} else {
			totaldelta.y = 0.0f;
		}
		if ( jointInfo[0].animBits & ANIM_TZ ) {
			for ( int i = 0; i < numFrames; i++ ) {
				componentPtr[ numAnimatedComponents * i ] -= baseFrame[0].t.z;
			}
			totaldelta.z = componentPtr[ numAnimatedComponents * ( numFrames - 1 ) ];
		} else {
			totaldelta.z = 0.0f;
		}
	}
	baseFrame[0].t.Zero();

	// we don't count last frame because it would cause a 1 frame pause at the end
	animLength = ( ( numFrames - 1 ) * 1000 + frameRate - 1 ) / frameRate;

	if ( binaryLoadAnim.GetBool() ) {
		anLib::Printf( "Writing %s\n", generatedFileName.c_str() );
		anFileLocal outputFile( fileSystem->OpenFileWrite( generatedFileName, "fs_basepath" ) );
		WriteBinary( outputFile, sourceTimeStamp );
	}

	// done
	return true;
}

/*
========================
anMD6Anim::LoadBinary
========================
*/
bool anMD6Anim::LoadBinary( anFile *file, ARC_TIME_T sourceTimeStamp ) {
	if ( file == nullptr ) {
		return false;
	}

	unsigned int magic = 0;
	file->ReadBig( magic );
	if ( magic != B_ANIM_MD6_MAGIC ) {
		return false;
	}

	ARC_TIME_T loadedTimeStamp;
	file->ReadBig( loadedTimeStamp );
	if ( !fileSystem->InProductionMode() && sourceTimeStamp != loadedTimeStamp ) {
		return false;
	}

	file->ReadBig( numFrames );
	file->ReadBig( frameRate );
	file->ReadBig( animLength );
	file->ReadBig( numJoints );
	file->ReadBig( numAnimatedComponents );

	int num;
	file->ReadBig( num );
	bounds.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		anBounds & b = bounds[i];
		file->ReadBig( b[0] );
		file->ReadBig( b[1] );
	}

	file->ReadBig( num );
	jointInfo.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		jointAnimInfo_t & j = jointInfo[i];

		anStr jointName;
		file->ReadString( jointName );
		if ( jointName.IsEmpty() ) {
			j.nameIndex = -1;
		} else {
			j.nameIndex = animationLib.JointIndex( jointName.c_str() );
		}

		file->ReadBig( j.parentNum );
		file->ReadBig( j.animBits );
		file->ReadBig( j.firstComponent );
	}

	file->ReadBig( num );
	baseFrame.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		anJointQuat & j = baseFrame[i];
		file->ReadBig( j.q.x );
		file->ReadBig( j.q.y );
		file->ReadBig( j.q.z );
		file->ReadBig( j.q.w );
		file->ReadVec3( j.t );
		j.w = 0.0f;
	}

	file->ReadBig( num );
	componentFrames.SetNum( num + JOINT_FRAME_PAD );
	for ( int i = 0; i < componentFrames.Num(); i++ ) {
		file->ReadFloat( componentFrames[i] );
	}

	//file->ReadString( name );
	file->ReadVec3( totaldelta );
	//file->ReadBig( refCount );

	return true;
}

/*
========================
anMD6Anim::WriteBinary
========================
*/
void anMD6Anim::WriteBinary( anFile *file, ARC_TIME_T sourceTimeStamp ) {
	if ( file == nullptr ) {
		return;
	}

	file->WriteBig( B_ANIM_MD6_MAGIC );
	file->WriteBig( sourceTimeStamp );

	file->WriteBig( numFrames );
	file->WriteBig( frameRate );
	file->WriteBig( animLength );
	file->WriteBig( numJoints );
	file->WriteBig( numAnimatedComponents );

	file->WriteBig( bounds.Num() );
	for ( int i = 0; i < bounds.Num(); i++ ) {
		anBounds & b = bounds[i];
		file->WriteBig( b[0] );
		file->WriteBig( b[1] );
	}

	file->WriteBig( jointInfo.Num() );
	for ( int i = 0; i < jointInfo.Num(); i++ ) {
		jointAnimInfo_t & j = jointInfo[i];
		anStr jointName = animationLib.JointName( j.nameIndex );
		file->WriteString( jointName );
		file->WriteBig( j.parentNum );
		file->WriteBig( j.animBits );
		file->WriteBig( j.firstComponent );
	}

	file->WriteBig( baseFrame.Num() );
	for ( int i = 0; i < baseFrame.Num(); i++ ) {
		anJointQuat & j = baseFrame[i];
		file->WriteBig( j.q.x );
		file->WriteBig( j.q.y );
		file->WriteBig( j.q.z );
		file->WriteBig( j.q.w );
		file->WriteVec3( j.t );
	}

	file->WriteBig( componentFrames.Num() - JOINT_FRAME_PAD );
	for ( int i = 0; i < componentFrames.Num(); i++ ) {
		file->WriteFloat( componentFrames[i] );
	}

	//file->WriteString( name );
	file->WriteVec3( totaldelta );
	//file->WriteBig( refCount );
}

/*
====================
anMD6Anim::IncreaseRefs
====================
*/
void anMD6Anim::IncreaseRefs() const {
	refCount++;
}

/*
====================
anMD6Anim::DecreaseRefs
====================
*/
void anMD6Anim::DecreaseRefs() const {
	refCount--;
}

/*
====================
anMD6Anim::NumRefs
====================
*/
int anMD6Anim::NumRefs() const {
	return refCount;
}

/*
====================
anMD6Anim::GetFrameBlend
====================
*/
void anMD6Anim::GetFrameBlend( int frameNum, frameBlend_t &frame ) const {
	frame.cycleCount	= 0;
	frame.backlerp		= 0.0f;
	frame.frontlerp		= 1.0f;

	// frame 1 is first frame
	frameNum--;
	if ( frameNum < 0 ) {
		frameNum = 0;
	} else if ( frameNum >= numFrames ) {
		frameNum = numFrames - 1;
	}

	frame.frame1 = frameNum;
	frame.frame2 = frameNum;
}

/*
====================
anMD6Anim::ConvertTimeToFrame
====================
*/
void anMD6Anim::ConvertTimeToFrame( int time, int cyclecount, frameBlend_t &frame ) const {
	int frameTime, frameNum;

	if ( numFrames <= 1 ) {
		frame.frame1		= 0;
		frame.frame2		= 0;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		frame.cycleCount	= 0;
		return;
	}

	if ( time <= 0 ) {
		frame.frame1		= 0;
		frame.frame2		= 1;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		frame.cycleCount	= 0;
		return;
	}

	frameTime			= time * frameRate;
	frameNum			= frameTime / 1000;
	frame.cycleCount	= frameNum / ( numFrames - 1 );

	if ( ( cyclecount > 0 ) && ( frame.cycleCount >= cyclecount ) ) {
		frame.cycleCount	= cyclecount - 1;
		frame.frame1		= numFrames - 1;
		frame.frame2		= frame.frame1;
		frame.backlerp		= 0.0f;
		frame.frontlerp		= 1.0f;
		return;
	}

	frame.frame1 = frameNum % ( numFrames - 1 );
	frame.frame2 = frame.frame1 + 1;
	if ( frame.frame2 >= numFrames ) {
		frame.frame2 = 0;
	}

	frame.backlerp	= ( frameTime % 1000 ) * 0.001f;
	frame.frontlerp	= 1.0f - frame.backlerp;
}

/*
====================
anMD6Anim::GetOrigin
====================
*/
void anMD6Anim::GetOrigin( anVec3 &offset, int time, int cyclecount ) const {
	offset = baseFrame[0].t;
	if ( !( jointInfo[0].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) ) {
		// just use the baseframe
		return;
	}

	frameBlend_t frame;
	ConvertTimeToFrame( time, cyclecount, frame );

	const float *componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[0].firstComponent ];
	const float *componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[0].firstComponent ];

	if ( jointInfo[0].animBits & ANIM_TX ) {
		offset.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if ( jointInfo[0].animBits & ANIM_TY ) {
		offset.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		componentPtr1++;
		componentPtr2++;
	}

	if ( jointInfo[0].animBits & ANIM_TZ ) {
		offset.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
	}

	if ( frame.cycleCount ) {
		offset += totaldelta * ( float )frame.cycleCount;
	}
}

/*
====================
anMD6Anim::GetOriginRotation
====================
*/
void anMD6Anim::GetOriginRotation( anQuat &rotation, int time, int cyclecount ) const {
	int animBits = jointInfo[0].animBits;
	if ( !( animBits & ( ANIM_QX | ANIM_QY | ANIM_QZ ) ) ) {
		// just use the baseframe
		rotation = baseFrame[0].q;
		return;
	}

	frameBlend_t frame;
	ConvertTimeToFrame( time, cyclecount, frame );

	const float	*jointframe1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[0].firstComponent ];
	const float	*jointframe2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[0].firstComponent ];

	if ( animBits & ANIM_TX ) {
		jointframe1++;
		jointframe2++;
	}

	if ( animBits & ANIM_TY ) {
		jointframe1++;
		jointframe2++;
	}

	if ( animBits & ANIM_TZ ) {
		jointframe1++;
		jointframe2++;
	}

	anQuat q1;
	anQuat q2;

	switch ( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
		case ANIM_QX:
			q1.x = jointframe1[0];
			q2.x = jointframe2[0];
			q1.y = baseFrame[0].q.y;
			q2.y = q1.y;
			q1.z = baseFrame[0].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY:
			q1.y = jointframe1[0];
			q2.y = jointframe2[0];
			q1.x = baseFrame[0].q.x;
			q2.x = q1.x;
			q1.z = baseFrame[0].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QZ:
			q1.z = jointframe1[0];
			q2.z = jointframe2[0];
			q1.x = baseFrame[0].q.x;
			q2.x = q1.x;
			q1.y = baseFrame[0].q.y;
			q2.y = q1.y;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QY:
			q1.x = jointframe1[0];
			q1.y = jointframe1[1];
			q2.x = jointframe2[0];
			q2.y = jointframe2[1];
			q1.z = baseFrame[0].q.z;
			q2.z = q1.z;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QZ:
			q1.x = jointframe1[0];
			q1.z = jointframe1[1];
			q2.x = jointframe2[0];
			q2.z = jointframe2[1];
			q1.y = baseFrame[0].q.y;
			q2.y = q1.y;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QY|ANIM_QZ:
			q1.y = jointframe1[0];
			q1.z = jointframe1[1];
			q2.y = jointframe2[0];
			q2.z = jointframe2[1];
			q1.x = baseFrame[0].q.x;
			q2.x = q1.x;
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
		case ANIM_QX|ANIM_QY|ANIM_QZ:
			q1.x = jointframe1[0];
			q1.y = jointframe1[1];
			q1.z = jointframe1[2];
			q2.x = jointframe2[0];
			q2.y = jointframe2[1];
			q2.z = jointframe2[2];
			q1.w = q1.CalcW();
			q2.w = q2.CalcW();
			break;
	}

	rotation.Slerp( q1, q2, frame.backlerp );
}

/*
====================
anMD6Anim::GetBounds
====================
*/
void anMD6Anim::GetBounds( anBounds &bnds, int time, int cyclecount ) const {
	frameBlend_t frame;
	ConvertTimeToFrame( time, cyclecount, frame );

	bnds = bounds[ frame.frame1 ];
	bnds.AddBounds( bounds[ frame.frame2 ] );

	// origin position
	anVec3 offset = baseFrame[0].t;
	if ( jointInfo[0].animBits & ( ANIM_TX | ANIM_TY | ANIM_TZ ) ) {
		const float *componentPtr1 = &componentFrames[ numAnimatedComponents * frame.frame1 + jointInfo[0].firstComponent ];
		const float *componentPtr2 = &componentFrames[ numAnimatedComponents * frame.frame2 + jointInfo[0].firstComponent ];
		if ( jointInfo[0].animBits & ANIM_TX ) {
			offset.x = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if ( jointInfo[0].animBits & ANIM_TY ) {
			offset.y = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
			componentPtr1++;
			componentPtr2++;
		}

		if ( jointInfo[0].animBits & ANIM_TZ ) {
			offset.z = *componentPtr1 * frame.frontlerp + *componentPtr2 * frame.backlerp;
		}
	}

	bnds[0] -= offset;
	bnds[1] -= offset;
}

/*
====================
DecodeInterpolatedFrames

====================
*/
int DecodeInterpolatedFrames( anJointQuat *joints, anJointQuat *blendJoints, int *lerpIndex, const float *frame1, const float *frame2, const jointAnimInfo_t *jointInfo, const int *index, const int numIndexes ) {
	int numLerpJoints = 0;
	for ( int i = 0; i < numIndexes; i++ ) {
		const int j = index[i];
		const jointAnimInfo_t *infoPtr = &jointInfo[j];
		const int animBits = infoPtr->animBits;
		if ( animBits != 0 ) {
			lerpIndex[numLerpJoints++] = j;

			anJointQuat *jointPtr = &joints[j];
			anJointQuat *blendPtr = &blendJoints[j];
			*blendPtr = *jointPtr;

			const float *jointframe1 = frame1 + infoPtr->firstComponent;
			const float *jointframe2 = frame2 + infoPtr->firstComponent;

			if ( animBits & (ANIM_TX|ANIM_TY|ANIM_TZ) ) {
				if ( animBits & ANIM_TX ) {
					jointPtr->t.x = *jointframe1++;
					blendPtr->t.x = *jointframe2++;
				}
				if ( animBits & ANIM_TY ) {
					jointPtr->t.y = *jointframe1++;
					blendPtr->t.y = *jointframe2++;
				}
				if ( animBits & ANIM_TZ ) {
					jointPtr->t.z = *jointframe1++;
					blendPtr->t.z = *jointframe2++;
				}
			}

			if ( animBits & (ANIM_QX|ANIM_QY|ANIM_QZ) ) {
				if ( animBits & ANIM_QX ) {
					jointPtr->q.x = *jointframe1++;
					blendPtr->q.x = *jointframe2++;
				}
				if ( animBits & ANIM_QY ) {
					jointPtr->q.y = *jointframe1++;
					blendPtr->q.y = *jointframe2++;
				}
				if ( animBits & ANIM_QZ ) {
					jointPtr->q.z = *jointframe1++;
					blendPtr->q.z = *jointframe2++;
				}
				jointPtr->q.w = jointPtr->q.CalcW();
				blendPtr->q.w = blendPtr->q.CalcW();
			}
		}
	}
	return numLerpJoints;
}

/*
====================
anMD6Anim::GetInterpolatedFrame
====================
*/
void anMD6Anim::GetInterpolatedFrame( frameBlend_t &frame, anJointQuat *joints, const int *index, int numIndexes ) const {
	// copy the baseframe
	SIMDProcessor->Memcpy( joints, baseFrame.Ptr(), baseFrame.Num() * sizeof( baseFrame[0] ) );

	if ( numAnimatedComponents == 0 ) {
		// just use the base frame
		return;
	}

	anJointQuat * blendJoints = (anJointQuat *)_alloca16( baseFrame.Num() * sizeof( blendJoints[0] ) );
	int *lerpIndex = (int *)_alloca16( baseFrame.Num() * sizeof( lerpIndex[0] ) );

	const float *frame1 = &componentFrames[frame.frame1 * numAnimatedComponents];
	const float *rame2 = &componentFrames[frame.frame2 * numAnimatedComponents];

	int numLerpJoints = DecodeInterpolatedFrames( joints, blendJoints, lerpIndex, frame1, frame2, jointInfo.Ptr(), index, numIndexes );

	SIMDProcessor->BlendJoints( joints, blendJoints, frame.backlerp, lerpIndex, numLerpJoints );

	if ( frame.cycleCount ) {
		joints[0].t += totaldelta * ( float )frame.cycleCount;
	}
}

/*
====================
DecodeSingleFrame
====================
*/
void DecodeSingleFrame( anJointQuat *joints, const float *frame, const jointAnimInfo_t *jointInfo, const int *index, const int numIndexes ) {
	for ( int i = 0; i < numIndexes; i++ ) {
		const int j = index[i];
		const jointAnimInfo_t * infoPtr = &jointInfo[j];

		const int animBits = infoPtr->animBits;
		if ( animBits != 0 ) {
			anJointQuat *jointPtr = &joints[j];
			const float *jointframe = frame + infoPtr->firstComponent;
			if ( animBits & ( ANIM_TX|ANIM_TY|ANIM_TZ ) ) {
				if ( animBits & ANIM_TX ) {
					jointPtr->t.x = *jointframe++;
				}
				if ( animBits & ANIM_TY ) {
					jointPtr->t.y = *jointframe++;
				}
				if ( animBits & ANIM_TZ ) {
					jointPtr->t.z = *jointframe++;
				}
			}

			if ( animBits & ( ANIM_QX|ANIM_QY|ANIM_QZ ) ) {
				if ( animBits & ANIM_QX ) {
					jointPtr->q.x = *jointframe++;
				}
				if ( animBits & ANIM_QY ) {
					jointPtr->q.y = *jointframe++;
				}
				if ( animBits & ANIM_QZ ) {
					jointPtr->q.z = *jointframe++;
				}
				jointPtr->q.w = jointPtr->q.CalcW();
			}
		}
	}
}

/*
====================
anMD6Anim::GetSingleFrame
====================
*/
void anMD6Anim::GetSingleFrame( int frameNum, anJointQuat *joints, const int *index, int numIndexes ) const {
	// copy the baseframe
	SIMDProcessor->Memcpy( joints, baseFrame.Ptr(), baseFrame.Num() * sizeof( baseFrame[0] ) );

	if ( frameNum == 0 || numAnimatedComponents == 0 ) {
		// just use the base frame
		return;
	}

	const float * frame = &componentFrames[frameNum * numAnimatedComponents];

	DecodeSingleFrame( joints, frame, jointInfo.Ptr(), index, numIndexes );
}

/*
====================
anMD6Anim::CheckModelHierarchy
====================
*/
void anMD6Anim::CheckModelHierarchy( const anRenderModel *model ) const {
	if ( jointInfo.Num() != model->NumJoints() ) {
		if ( !fileSystem->InProductionMode() ) {
			gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", model->Name(), name.c_str() );
		} else {
			//gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", model->Name(), name.c_str() );
			return;
		}
	}

	const anMD5Joint *modelJoints = model->GetJoints();
	for ( int i = 0; i < jointInfo.Num(); i++ ) {
		int jointNum = jointInfo[i].nameIndex;
		if ( modelJoints[i].name != animationLib.JointName( jointNum ) ) {
			gameLocal.Error( "Model '%s''s joint names don't match anim '%s''s", model->Name(), name.c_str() );
		}
		int parent;
		if ( modelJoints[i].parent ) {
			parent = modelJoints[i].parent - modelJoints;
		} else {
			parent = -1;
		}
		if ( parent != jointInfo[i].parentNum ) {
			gameLocal.Error( "Model '%s' has different joint hierarchy than anim '%s'", model->Name(), name.c_str() );
		}
	}
}

/***********************************************************************

	anAnimManager

***********************************************************************/

/*
====================
anAnimManager::anAnimManager
====================
*/
anAnimManager::anAnimManager() {
}

/*
====================
anAnimManager::~anAnimManager
====================
*/
anAnimManager::~anAnimManager() {
	Shutdown();
}

/*
====================
anAnimManager::Shutdown
====================
*/
void anAnimManager::Shutdown() {
	animations.DeleteContents();
	jointnames.Clear();
	jointnamesHash.Free();
}

/*
====================
anAnimManager::GetAnim
====================
*/
anMD6Anim *anAnimManager::GetAnim( const char *name ) {
	anMD6Anim **animptrptr;
	anMD6Anim *anim;

	// see if it has been asked for before
	animptrptr = nullptr;
	if ( animations.Get( name, &animptrptr ) ) {
		anim = *animptrptr;
	} else {
		anStr extension;
		anStr filename = name;

		filename.ExtractFileExtension( extension );
		if ( extension != MD5_ANIM_EXT ) {
			return nullptr;
		}

		anim = new (TAG_ANIM) anMD6Anim();
		if ( !anim->LoadAnim( filename ) ) {
			gameLocal.Warning( "Couldn't load anim: '%s'", filename.c_str() );
			delete anim;
			anim = nullptr;
		}
		animations.Set( filename, anim );
	}

	return anim;
}

/*
================
anAnimManager::Preload
================
*/
void anAnimManager::Preload( const idPreloadManifest &manifest ) {
	if ( manifest.NumResources() >= 0 ) {
		common->Printf( "Preloading anims...\n" );
		int	start = Sys_Milliseconds();
		int numLoaded = 0;
		for ( int i = 0; i < manifest.NumResources(); i++ ) {
			const preloadEntry_s & p = manifest.GetPreloadByIndex( i );
			if ( p.resType == PRELOAD_ANIM ) {
				GetAnim( p.resourceName );
				numLoaded++;
			}
		}
		int	end = Sys_Milliseconds();
		common->Printf( "%05d anims preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
		common->Printf( "----------------------------------------\n" );
	}
}

/*
================
anAnimManager::ReloadAnims
================
*/
void anAnimManager::ReloadAnims() {
	anMD6Anim **animptr;

	for ( int i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr != nullptr && *animptr != nullptr ) {
			( *animptr )->Reload();
		}
	}
}

/*
================
anAnimManager::JointIndex
================
*/
int	anAnimManager::JointIndex( const char *name ) {
	int hash = jointnamesHash.GenerateKey( name );
	for ( int i = jointnamesHash.First( hash ); i != -1; i = jointnamesHash.Next( i ) ) {
		if ( jointnames[i].Cmp( name ) == 0 ) {
			return i;
		}
	}

	i = jointnames.Append( name );
	jointnamesHash.Add( hash, i );
	return i;
}

/*
================
anAnimManager::JointName
================
*/
const char *anAnimManager::JointName( int index ) const {
	return jointnames[index];
}

/*
================
anAnimManager::ListAnims
================
*/
void anAnimManager::ListAnims() const {
	anMD6Anim	**animptr;
	anMD6Anim	*anim;
	size_t		size;
	size_t		s;
	size_t		namesize;
	int			num;

	num = 0;
	size = 0;
	for ( int i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr != nullptr && *animptr != nullptr ) {
			anim = *animptr;
			s = anim->Size();
			gameLocal.Printf( "%8d bytes : %2d refs : %s\n", s, anim->NumRefs(), anim->Name() );
			size += s;
			num++;
		}
	}

	namesize = jointnames.Size() + jointnamesHash.Size();
	for ( int i = 0; i < jointnames.Num(); i++ ) {
		namesize += jointnames[i].Size();
	}

	gameLocal.Printf( "\n%d memory used in %d anims\n", size, num );
	gameLocal.Printf( "%d memory used in %d joint names\n", namesize, jointnames.Num() );
}

/*
================
anAnimManager::FlushUnusedAnims
================
*/
void anAnimManager::FlushUnusedAnims() {
	anMD6Anim **animptr;
	anList<anMD6Anim *> removeAnims;

	for ( int i = 0; i < animations.Num(); i++ ) {
		animptr = animations.GetIndex( i );
		if ( animptr != nullptr && *animptr != nullptr ) {
			if ( ( *animptr )->NumRefs() <= 0 ) {
				removeAnims.Append( *animptr );
			}
		}
	}

	for ( int i = 0; i < removeAnims.Num(); i++ ) {
		animations.Remove( removeAnims[i]->Name() );
		delete removeAnims[i];
	}
}
