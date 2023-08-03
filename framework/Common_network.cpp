#include "/idlib/Lib.h"
#pragma hdrstop

#include "Common_local.h"

anCVarSystem net_clientMaxPrediction( "net_clientMaxPrediction", "5000", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "maximum number of milliseconds a client can predict ahead of server." );
anCVarSystem net_snapRate( "net_snapRate", "100", CVAR_SYSTEM | CVAR_INTEGER, "How many milliseconds between sending snapshots" );
anCVarSystem net_ucmdRate( "net_ucmdRate", "40", CVAR_SYSTEM | CVAR_INTEGER, "How many milliseconds between sending usercmds" );

anCVarSystem net_debug_snapShotTime( "net_debug_snapShotTime", "0", CVAR_BOOL | CVAR_ARCHIVE, "" );
anCVarSystem com_forceLatestSnap( "com_forceLatestSnap", "0", CVAR_BOOL, "" );

// Enables effective snap rate: dynamically adjust the client snap rate based on:
//	-client FPS
//	-server FPS (interpolated game time received / interval it was received over)
//  -local buffered time (leave a cushion to absorb spikes, slow down when infront of it, speed up when behind it) ie: net_minBufferedSnapPCT_Static
anCVarSystem net_effectiveSnapRateEnable( "net_effectiveSnapRateEnable", "1", CVAR_BOOL, "Dynamically adjust client snaprate" );
anCVarSystem net_effectiveSnapRateDebug( "net_effectiveSnapRateDebug", "0", CVAR_BOOL, "Debug" );

// Min buffered snapshot time to keep as a percentage of the effective snaprate
//	-ie we want to keep 50% of the amount of time difference between last two snaps.
//	-we need to scale this because we may get throttled at the snaprate may change
//  -Acts as a buffer to absorb spikes
anCVarSystem net_minBufferedSnapPCT_Static( "net_minBufferedSnapPCT_Static", "1.0", CVAR_FLOAT, "Min amount of snapshot buffer time we want need to buffer" );
anCVarSystem net_maxBufferedSnapMS( "net_maxBufferedSnapMS", "336", CVAR_INTEGER, "Max time to allow for interpolation cushion" );
anCVarSystem net_minBufferedSnapWinPCT_Static( "net_minBufferedSnapWinPCT_Static", "1.0", CVAR_FLOAT, "Min amount of snapshot buffer time we want need to buffer" );

// Factor at which we catch speed up interpolation if we fall behind our optimal interpolation window
//  -This is a static factor. We may experiment with a dynamic one that would be faster the farther you are from the ideal window
anCVarSystem net_interpolationCatchupRate( "net_interpolationCatchupRate", "1.3", CVAR_FLOAT, "Scale interpolationg rate when we fall behind" );
anCVarSystem net_interpolationFallbackRate( "net_interpolationFallbackRate", "0.95", CVAR_FLOAT, "Scale interpolationg rate when we fall behind" );
anCVarSystem net_interpolationBaseRate( "net_interpolationBaseRate", "1.0", CVAR_FLOAT, "Scale interpolationg rate when we fall behind" );

// Enabled a dynamic ideal snap buffer window: we will scale the distance and size
anCVarSystem net_optimalDynamic( "net_optimalDynamic", "1", CVAR_BOOL, "How fast to add to our optimal time buffer when we are playing snapshots faster than server is feeding them to us" );

// These values are used instead if net_optimalDynamic is 0 (don't scale by actual snap rate/interval)
anCVarSystem net_optimalSnapWindow( "net_optimalSnapWindow", "112", CVAR_FLOAT, "" );
anCVarSystem net_optimalSnapTime( "net_optimalSnapTime", "112", CVAR_FLOAT, "How fast to add to our optimal time buffer when we are playing snapshots faster than server is feeding them to us" );

// this is at what percentage of being ahead of the interpolation buffer that we start slowing down (we ramp down from 1.0 to 0.0 starting here)
// this is a percentage of the total cushion time.
anCVarSystem net_interpolationSlowdownStart( "net_interpolationSlowdownStart", "0.5", CVAR_FLOAT, "Scale interpolation rate when we fall behind" );


// Extrapolation is now disabled
anCVarSystem net_maxExtrapolationInMS( "net_maxExtrapolationInMS", "0", CVAR_INTEGER, "Max time in MS that extrapolation is allowed to occur." );

static const int SNAP_USERCMDS = 8192;

/*
===============
anCommonLocal::SendSnapshots
===============
*/
int anCommonLocal::GetSnapRate() {
	return net_snapRate.GetInteger();
}

/*
===============
anCommonLocal::SendSnapshots
===============
*/
void anCommonLocal::SendSnapshots() {
	if ( !mapSpawned ) {
		return;
	}
	int currentTime = Sys_Milliseconds();
	if ( currentTime < nextSnapshotSendTime ) {
		return;
	}
	idLobbyBase & lobby = session->GetActingGameStateLobbyBase();
	if ( !lobby.IsHost() ) {
		return;
	}
	if ( !lobby.HasActivePeers() ) {
		return;
	}
	ARCSnapShot ss;
	session->SendSnapshot( ss );
	nextSnapshotSendTime = MSEC_ALIGN_TO_FRAME( currentTime + net_snapRate.GetInteger() );
}

/*
===============
anCommonLocal::NetReceiveSnapshot
===============
*/
void anCommonLocal::NetReceiveSnapshot( class ARCSnapShot & ss ) {
	ss.SetRecvTime( Sys_Milliseconds() );
	// If we are about to overwrite the oldest snap, then force a read, which will cause a pop on screen, but we have to do this.
	if ( writeSnapshotIndex - readSnapshotIndex >= RECEIVE_SNAPSHOT_BUFFER_SIZE ) {
		anLibrary::Printf( "Overwritting oldest snapshot %d with new snapshot %d\n", readSnapshotIndex, writeSnapshotIndex );
		assert( writeSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE == readSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE );
		ProcessNextSnapshot();
	}

	receivedSnaps[ writeSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE ] = ss;
	writeSnapshotIndex++;

	// Force read the very first 2 snapshots
	if ( readSnapshotIndex < 2 ) {
		ProcessNextSnapshot();
	}
}

/*
===============
anCommonLocal::SendUsercmd
===============
*/
void anCommonLocal::SendUsercmds( int localClientNum ) {
	if ( !mapSpawned ) {
		return;
	}
	int currentTime = Sys_Milliseconds();
	if ( currentTime < nextUsercmdSendTime ) {
		return;
	}
	idLobbyBase & lobby = session->GetActingGameStateLobbyBase();
	if ( lobby.IsHost() ) {
		return;
	}
	// We always send the last NUM_USERCMD_SEND usercmds
	// Which may result in duplicate usercmds being sent in the case of a low net_ucmdRate
	// But the LZW compressor means the extra usercmds are not large and the redundancy can smooth packet loss
	byte buffer[idPacketProcessor::MAX_FINAL_PACKET_SIZE];
	anBitMessage msg( buffer, sizeof( buffer ) );
	idSerializer ser( msg, true );
	usercmd_t empty;
	usercmd_t * last = &empty;

	usercmd_t * cmdBuffer[NUM_USERCMD_SEND];
	const int numCmds = userCmdMgr.GetPlayerCmds( localClientNum, cmdBuffer, NUM_USERCMD_SEND );
	msg.WriteByte( numCmds );
	for ( int i = 0; i < numCmds; i++ ) {
		cmdBuffer[i]->Serialize( ser, *last );

		last = cmdBuffer[i];
	}
	session->SendUsercmds( msg );

	nextUsercmdSendTime = MSEC_ALIGN_TO_FRAME( currentTime + net_ucmdRate.GetInteger() );
}

/*
===============
anCommonLocal::NetReceiveUsercmds
===============
*/
void anCommonLocal::NetReceiveUsercmds( int peer, anBitMessage & msg ) {
	int clientNum = Game()->MapPeerToClient( peer );
	if ( clientNum == -1 ) {
		anLibrary::Warning( "NetReceiveUsercmds: Could not find client for peer %d", peer );
		return;
	}

	NetReadUsercmds( clientNum, msg );
}

/*
===============
anCommonLocal::NetReceiveReliable
===============
*/
void anCommonLocal::NetReceiveReliable( int peer, int type, anBitMessage & msg ) {
	int clientNum = Game()->MapPeerToClient( peer );
	// Only servers care about the client num. Band-aid for problems related to the host's peerIndex being -1 on clients.
	if ( common->IsServer() && clientNum == -1 ) {
		anLibrary::Warning( "NetReceiveReliable: Could not find client for peer %d", peer );
		return;
	}

	const byte * msgData = msg.GetReadData() + msg.GetReadCount();
	int msgSize = msg.GetRemainingData();
	reliableMsg_t & reliable = reliableQueue.Alloc();
	reliable.client = clientNum;
	reliable.type = type;
	reliable.dataSize = msgSize;
	reliable.data = (byte *)Mem_Alloc( msgSize, TAG_NETWORKING );
	memcpy( reliable.data, msgData, msgSize );
}

/*
========================
anCommonLocal::ProcessSnapshot
========================
*/
void anCommonLocal::ProcessSnapshot( ARCSnapShot & ss ) {
	int time = Sys_Milliseconds();

	snapTime = time;
	snapPrevious			= snapCurrent;
	snapCurrent.serverTime	= ss.GetTime();
	snapRate = snapCurrent.serverTime - snapPrevious.serverTime;

	static int lastReceivedLocalTime = 0;
	int timeSinceLastSnap = ( time - lastReceivedLocalTime );
	if ( net_debug_snapShotTime.GetBool() ) {
		anLibrary::Printf( "^2ProcessSnapshot. delta serverTime: %d  delta localTime: %d \n", ( snapCurrent.serverTime-snapPrevious.serverTime ), timeSinceLastSnap );
	}
	lastReceivedLocalTime = time;
	// Read usercmds from other players
	for ( int p = 0; p < MAX_PLAYERS; p++ ) {
		if ( p == game->GetLocalClientNum() ) {
			continue;
		}
		anBitMessage msg;
		if ( ss.GetObjectMsgByID( SNAP_USERCMDS + p, msg ) ) {
			NetReadUsercmds( p, msg );
		}
	}

	// Set server game time here so that it accurately reflects the time when this frame was saved out, in case any serialize function needs it.
	int oldTime = Game()->GetServerGameTimeMs();
	Game()->SetServerGameTimeMs( snapCurrent.serverTime );

	Game()->ClientReadSnapshot( ss ); //, &oldss );

	// Restore server game time
	Game()->SetServerGameTimeMs( oldTime );

	snapTimeDelta = ss.GetRecvTime() - oldss.GetRecvTime();
	oldss = ss;
}

/*
========================
anCommonLocal::NetReadUsercmds
========================
*/
void anCommonLocal::NetReadUsercmds( int clientNum, anBitMessage & msg ) {
	if ( clientNum == -1 ) {
		anLibrary::Warning( "NetReadUsercmds: Trying to read commands from invalid clientNum %d", clientNum );
		return;
	}

	// TODO: This shouldn't actually happen. Figure out why it does.
	// Seen on clients when another client leaves a match.
	if ( msg.GetReadData() == nullptr ) {
		return;
	}

	idSerializer ser( msg, false );

	usercmd_t fakeCmd;
	usercmd_t * base = &fakeCmd;

	usercmd_t lastCmd;

	bool										gotNewCmd = false;
	arcStaticList< usercmd_t, NUM_USERCMD_RELAY >	newCmdBuffer;

	usercmd_t baseCmd = userCmdMgr.NewestUserCmdForPlayer( clientNum );
	int curMilliseconds = baseCmd.clientGameMilliseconds;

	const int numCmds = msg.ReadByte();

	for ( int i = 0; i < numCmds; i++ ) {
		usercmd_t newCmd;
		newCmd.Serialize( ser, *base );

		lastCmd = newCmd;
		base = &lastCmd;

		int newMilliseconds = newCmd.clientGameMilliseconds;

		if ( newMilliseconds > curMilliseconds ) {
			if ( verify( i < NUM_USERCMD_RELAY ) ) {
				newCmdBuffer.Append( newCmd );
				gotNewCmd = true;
				curMilliseconds = newMilliseconds;
			}
		}
	}

	// Push the commands into the buffer.
	for ( int i = 0; i < newCmdBuffer.Num(); ++i ) {
		userCmdMgr.PutUserCmdForPlayer( clientNum, newCmdBuffer[i] );
	}
}

/*
========================
anCommonLocal::CalcSnapTimeBuffered
Return the amount of game time left of buffered snapshots
totalBufferedTime - total amount of snapshot time (includng what we've already past in current interpolate)
totalRecvTime - total real time ( sys_milliseconds) all of totalBufferedTime was received over
========================
*/
int anCommonLocal::CalcSnapTimeBuffered( int & totalBufferedTime, int & totalRecvTime ) {
	totalBufferedTime = snapRate;
	totalRecvTime = snapTimeDelta;

	// oldSS = last ss we deserialized
	int lastBuffTime = oldss.GetTime();
	int lastRecvTime = oldss.GetRecvTime();

	// receivedSnaps[readSnapshotIndex % RECEIVE_SNAPSHOT_BUFFER_SIZE] = next buffered snapshot we haven't processed yet (might not exist)
	for ( int i = readSnapshotIndex; i < writeSnapshotIndex; i++ ) {
		int buffTime = receivedSnaps[i % RECEIVE_SNAPSHOT_BUFFER_SIZE].GetTime();
		int recvTime = receivedSnaps[i % RECEIVE_SNAPSHOT_BUFFER_SIZE].GetRecvTime();

		totalBufferedTime += buffTime - lastBuffTime;
		totalRecvTime += recvTime - lastRecvTime;

		lastRecvTime = recvTime;
		lastBuffTime = buffTime;
	}

	totalRecvTime = Max( 1, totalRecvTime );
	totalRecvTime = static_cast<float>( initialBaseTicksPerSec ) * static_cast<float>( totalRecvTime / 1000.0f ); // convert realMS to gameMS

	// remove time we've already interpolated over
	int timeLeft = totalBufferedTime - Min< int >( snapRate, snapCurrentTime );

	//anLibrary::Printf( "CalcSnapTimeBuffered. timeLeft: %d totalRecvTime: %d, totalTimeBuffered: %d\n", timeLeft, totalRecvTime, totalBufferedTime );
	return timeLeft;
}

/*
========================
anCommonLocal::InterpolateSnapshot
========================
*/
void anCommonLocal::InterpolateSnapshot( netTimes_t & prev, netTimes_t & next, float fraction, bool predict ) {
	int serverTime = Lerp( prev.serverTime, next.serverTime, fraction );

	Game()->SetServerGameTimeMs( serverTime );		// Set the global server time to the interpolated time of the server
	Game()->SetInterpolation( fraction, serverTime, prev.serverTime, next.serverTime );

	//Game()->RunFrame( &userCmdMgr, &ret, true );

}

/*
========================
anCommonLocal::ExecuteReliableMessages
========================
*/
void anCommonLocal::ExecuteReliableMessages() {
	// Process any reliable messages we've received
	for ( int i = 0; i < reliableQueue.Num(); i++ ) {
		reliableMsg_t & reliable = reliableQueue[i];
		game->ProcessReliableMessage( reliable.client, reliable.type, anBitMessage( (const byte *)reliable.data, reliable.dataSize ) );
		Mem_Free( reliable.data );
	}
	reliableQueue.Clear();

}

/*
========================
anCommonLocal::ResetNetworkingState
========================
*/
void anCommonLocal::ResetNetworkingState() {
	snapTime		= 0;
	snapTimeWrite	= 0;
	snapCurrentTime	= 0;
	snapCurrentResidual = 0.0f;

	snapTimeBuffered	= 0.0f;
	effectiveSnapRate	= 0.0f;
	totalBufferedTime	= 0;
	totalRecvTime		= 0;

	readSnapshotIndex	= 0;
	writeSnapshotIndex	= 0;
	snapRate			= 100000;
	optimalTimeBuffered	= 0.0f;
	optimalPCTBuffer	= 0.5f;
	optimalTimeBufferedWindow = 0.0;

	// Clear snapshot queue
	for ( int i = 0; i < RECEIVE_SNAPSHOT_BUFFER_SIZE; i++ ) {
		receivedSnaps[i].Clear();
	}

	userCmdMgr.SetDefaults();

	// Make sure our current snap state is cleared so state from last game doesn't carry over into new game
	oldss.Clear();

	gameFrame = 0;
	clientPrediction = 0;
	nextUsercmdSendTime = 0;
	nextSnapshotSendTime = 0;
}
