/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// cl_parse.c  -- parse a message received from the server

#include "client.h"

static const char *const svc_strings[ 256 ] =
{
	"svc_bad",

	"svc_nop",
	"svc_gamestate",
	"svc_configstring",
	"svc_baseline",
	"svc_serverCommand",
	"svc_download",
	"svc_snapshot",
	"svc_voip",
	"svc_extension",
	"svc_EOF"
};

static void SHOWNET( msg_t *msg, const char *s )
{
	if ( cl_shownet->integer >= 2 )
	{
		Com_Printf( "%3i:%s\n", msg->readcount - 1, s );
	}
}

/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity( msg_t *msg, clSnapshot_t *frame, int newnum, entityState_t *old, bool unchanged )
{
	entityState_t *state;

	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	state = &cl.parseEntities[ cl.parseEntitiesNum & ( MAX_PARSE_ENTITIES - 1 ) ];

	if ( unchanged )
	{
		*state = *old;
	}
	else
	{
		MSG_ReadDeltaEntity( msg, old, state, newnum );
	}

	if ( state->number == ( MAX_GENTITIES - 1 ) )
	{
		return; // entity was delta removed
	}

	cl.parseEntitiesNum++;
	frame->numEntities++;
}

/*
==================
CL_ParsePacketEntities

==================
*/
void CL_ParsePacketEntities( msg_t *msg, const clSnapshot_t *oldframe, clSnapshot_t *newframe )
{
	unsigned int  newnum;
	entityState_t *oldstate;
	unsigned int  oldindex, oldnum;

	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	oldstate = nullptr;

	if ( !oldframe )
	{
		static const clSnapshot_t nullframe = { false };

		oldframe = &nullframe;
		oldnum = MAX_GENTITIES; // guaranteed out of range
	}
	else
	{
		if ( oldindex >= oldframe->numEntities )
		{
			oldnum = MAX_GENTITIES;
		}
		else
		{
			oldstate = &cl.parseEntities[
			             ( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 ) ];
			oldnum = oldstate->number;
		}
	}

	while ( 1 )
	{
		// read the entity index number
		newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );

		if ( newnum >= ( MAX_GENTITIES - 1 ) )
		{
			break;
		}

		if ( msg->readcount > msg->cursize )
		{
			Com_Error( ERR_DROP, "CL_ParsePacketEntities: end of message" );
		}

		while ( oldnum < newnum )
		{
			// one or more entities from the old packet are unchanged
			if ( cl_shownet->integer == 3 )
			{
				Com_Printf( "%3i:  unchanged: %i\n", msg->readcount, oldnum );
			}

			CL_DeltaEntity( msg, newframe, oldnum, oldstate, true );

			oldindex++;

			if ( oldindex >= oldframe->numEntities )
			{
				oldnum = MAX_GENTITIES;
			}
			else
			{
				oldstate = &cl.parseEntities[
				             ( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 ) ];
				oldnum = oldstate->number;
			}
		}

		if ( oldnum == newnum )
		{
			// delta from previous state
			if ( cl_shownet->integer == 3 )
			{
				Com_Printf( "%3i:  delta: %i\n", msg->readcount, newnum );
			}

			CL_DeltaEntity( msg, newframe, newnum, oldstate, false );

			oldindex++;

			if ( oldindex >= oldframe->numEntities )
			{
				oldnum = MAX_GENTITIES;
			}
			else
			{
				oldstate = &cl.parseEntities[
				             ( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 ) ];
				oldnum = oldstate->number;
			}

			continue;
		}

		if ( oldnum > newnum )
		{
			// delta from baseline
			if ( cl_shownet->integer == 3 )
			{
				Com_Printf( "%3i:  baseline: %i\n", msg->readcount, newnum );
			}

			CL_DeltaEntity( msg, newframe, newnum, &cl.entityBaselines[ newnum ], false );
			continue;
		}
	}

	// any remaining entities in the old frame are copied over
	while ( oldnum != MAX_GENTITIES )
	{
		// one or more entities from the old packet are unchanged
		if ( cl_shownet->integer == 3 )
		{
			Com_Printf( "%3i:  unchanged: %i\n", msg->readcount, oldnum );
		}

		CL_DeltaEntity( msg, newframe, oldnum, oldstate, true );

		oldindex++;

		if ( oldindex >= oldframe->numEntities )
		{
			oldnum = MAX_GENTITIES;
		}
		else
		{
			oldstate = &cl.parseEntities[
			             ( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 ) ];
			oldnum = oldstate->number;
		}
	}

	if ( cl_shownuments->integer )
	{
		Com_Printf( "Entities in packet: %i\n", newframe->numEntities );
	}
}

/*
================
CL_ParseSnapshot

If the snapshot is parsed properly, it will be copied to
cl.snap and saved in cl.snapshots[].  If the snapshot is invalid
for any reason, no changes to the state will be made at all.
================
*/
void CL_ParseSnapshot( msg_t *msg )
{
	int          len;
	clSnapshot_t *old;
	clSnapshot_t newSnap;
	int          deltaNum;
	int          oldMessageNum;
	int          i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.reliableAcknowledge = MSG_ReadLong( msg );

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	Com_Memset( &newSnap, 0, sizeof( newSnap ) );

	// we will have read any new server commands in this
	// message before we got to svc_snapshot
	newSnap.serverCommandNum = clc.serverCommandSequence;

	newSnap.serverTime = MSG_ReadLong( msg );

	// if we were just unpaused, we can only *now* really let the
	// change come into effect or the client hangs.
	cl_paused->modified = 0;

	newSnap.messageNum = clc.serverMessageSequence;

	deltaNum = MSG_ReadByte( msg );

	if ( !deltaNum )
	{
		newSnap.deltaNum = -1;
	}
	else
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}

	newSnap.snapFlags = MSG_ReadByte( msg );

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if ( newSnap.deltaNum <= 0 )
	{
		newSnap.valid = true; // uncompressed frame
		old = nullptr;

		if ( clc.demorecording )
		{
			clc.demowaiting = false; // we can start recording now
//          if(cl_autorecord->integer) {
//              Cvar_Set( "g_synchronousClients", "0" );
//          }
		}
		else
		{
			if ( cl_autorecord->integer /*&& Cvar_VariableValue( "g_synchronousClients") */ )
			{
				char    name[ 256 ];
				char    mapname[ MAX_QPATH ];
				char    *period;
				qtime_t time;

				Com_RealTime( &time );

				Q_strncpyz( mapname, cl.mapname, MAX_QPATH );

				for ( period = mapname; *period; period++ )
				{
					if ( *period == '.' )
					{
						*period = '\0';
						break;
					}
				}

				for ( period = mapname; *period; period++ )
				{
					if ( *period == '/' )
					{
						break;
					}
				}

				if ( *period )
				{
					period++;
				}

				Com_sprintf( name, sizeof( name ), "demos/%s_%04i-%02i-%02i_%02i%02i%02i.dm_%d", period,
				             1900 + time.tm_year, time.tm_mon + 1, time.tm_mday,
				             time.tm_hour, time.tm_min, time.tm_sec,
				             PROTOCOL_VERSION );

				CL_Record( name );
			}
		}
	}
	else
	{
		old = &cl.snapshots[ newSnap.deltaNum & PACKET_MASK ];

		if ( !old->valid )
		{
			// should never happen
			Com_Printf( "Delta from invalid frame (not supposed to happen!).\n" );
		}
		else if ( old->messageNum != newSnap.deltaNum )
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_DPrintf( "Delta frame too old.\n" );
		}
		else if ( cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES - 128 )
		{
			Com_DPrintf( "Delta parseEntitiesNum too old.\n" );
		}
		else
		{
			newSnap.valid = true; // valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte( msg );

	if ( len > sizeof( newSnap.areamask ) )
	{
		Com_Error( ERR_DROP, "CL_ParseSnapshot: Invalid size %d for areamask.", len );
	}

	MSG_ReadData( msg, &newSnap.areamask, len );

	// read playerinfo
	SHOWNET( msg, "playerstate" );

	if ( old )
	{
		MSG_ReadDeltaPlayerstate( msg, &old->ps, &newSnap.ps );
	}
	else
	{
		MSG_ReadDeltaPlayerstate( msg, nullptr, &newSnap.ps );
	}

	// read packet entities
	SHOWNET( msg, "packet entities" );
	CL_ParsePacketEntities( msg, old, &newSnap );

	// if not valid, dump the entire thing now that it has
	// been properly read
	if ( !newSnap.valid )
	{
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.snap.messageNum + 1;

	if ( newSnap.messageNum - oldMessageNum >= PACKET_BACKUP )
	{
		oldMessageNum = newSnap.messageNum - ( PACKET_BACKUP - 1 );
	}

	for ( ; oldMessageNum < newSnap.messageNum; oldMessageNum++ )
	{
		cl.snapshots[ oldMessageNum & PACKET_MASK ].valid = false;
	}

	// copy to the current good spot
	cl.snap = newSnap;
	cl.snap.ping = 999;

	// calculate ping time
	for ( i = 0; i < PACKET_BACKUP; i++ )
	{
		packetNum = ( clc.netchan.outgoingSequence - 1 - i ) & PACKET_MASK;

		if ( cl.snap.ps.commandTime >= cl.outPackets[ packetNum ].p_serverTime )
		{
			cl.snap.ping = cls.realtime - cl.outPackets[ packetNum ].p_realtime;
			break;
		}
	}

	// save the frame off in the backup array for later delta comparisons
	cl.snapshots[ cl.snap.messageNum & PACKET_MASK ] = cl.snap;

	if ( cl_shownet->integer == 3 )
	{
		Com_Printf( "   snapshot:%i  delta:%i  ping:%i\n", cl.snap.messageNum, cl.snap.deltaNum, cl.snap.ping );
	}

	cl.newSnapshots = true;
}

//=====================================================================

/*
==================
CL_SystemInfoChanged

The systeminfo configstring has been changed, so parse
new information out of it.  This will happen at every
gamestate, and possibly during gameplay.
==================
*/
void CL_SystemInfoChanged()
{
	const char *systemInfo;
	const char *s;
	char       key[ BIG_INFO_KEY ];
	char       value[ BIG_INFO_VALUE ];

	systemInfo = cl.gameState[ CS_SYSTEMINFO ].c_str();
	// NOTE TTimo:
	// when the serverId changes, any further messages we send to the server will use this new serverId
	// show_bug.cgi?id=475
	// in some cases, outdated cp commands might get sent with this news serverId
	cl.serverId = atoi( Info_ValueForKey( systemInfo, "sv_serverid" ) );

	// load paks sent by the server, but not if we are running a local server
	if (!com_sv_running->integer) {
		FS::PakPath::ClearPaks();
		if (!FS_LoadServerPaks(Info_ValueForKey(systemInfo, "sv_paks"), clc.demoplaying)) {
			if (!cl_allowDownload->integer) {
				Com_Error(ERR_DROP, "Client is missing paks but downloads are disabled");
			} else if (clc.demoplaying) {
				Com_Error(ERR_DROP, "Client is missing paks needed by the demo");
			}
		}
	}

	// don't set any vars when playing a demo
	if ( clc.demoplaying )
	{
		return;
	}

#ifdef USE_VOIP
	s = Info_ValueForKey( systemInfo, "sv_voip" );
	clc.voipEnabled = atoi( s );
#endif

	// scan through all the variables in the systeminfo and locally set cvars to match
	s = systemInfo;

	while ( s )
	{
		Info_NextPair( &s, key, value );

		if ( !key[ 0 ] )
		{
			break;
		}

		Cvar_Set( key, value );
	}
}

/*
==================
CL_ParseGamestate
==================
*/
void CL_ParseGamestate( msg_t *msg )
{
	int           i;
	entityState_t *es;
	int           newnum;
	entityState_t nullstate;
	int           cmd;

	Con_Close();

	clc.connectPacketCount = 0;

	// wipe local client state
	CL_ClearState();

	// a gamestate always marks a server command sequence
	clc.serverCommandSequence = MSG_ReadLong( msg );

	// parse all the configstrings and baselines
	while ( 1 )
	{
		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF )
		{
			break;
		}

		if ( cmd == svc_configstring )
		{
			i = MSG_ReadShort( msg );

			if ( i < 0 || i >= MAX_CONFIGSTRINGS )
			{
				Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
			}

            const char* str = MSG_ReadBigString( msg );
            std::string s = str;
			cl.gameState[i] = str;
		}
		else if ( cmd == svc_baseline )
		{
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );

			if ( newnum < 0 || newnum >= MAX_GENTITIES )
			{
				Com_Error( ERR_DROP, "Baseline number out of range: %i", newnum );
			}

			memset( &nullstate, 0, sizeof( nullstate ) );
			es = &cl.entityBaselines[ newnum ];
			MSG_ReadDeltaEntity( msg, &nullstate, es, newnum );
		}
		else
		{
			Com_Error( ERR_DROP, "CL_ParseGamestate: bad command byte" );
		}
	}

	clc.clientNum = MSG_ReadLong( msg );
	// read the checksum feed
	clc.checksumFeed = MSG_ReadLong( msg );

	// parse serverId and other cvars
	CL_SystemInfoChanged();

	// This used to call CL_StartHunkUsers, but now we enter the download state before loading the
	// cgame
	CL_InitDownloads();

	// make sure the game starts
	Cvar_Set( "cl_paused", "0" );
}

//=====================================================================

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload( msg_t *msg )
{
	int           size;
	unsigned char data[ MAX_MSGLEN ];
	int           block;

	if ( !*cls.downloadTempName )
	{
		Com_Printf( "Server sending download, but no download was requested\n" );
		CL_AddReliableCommand( "stopdl" );
		return;
	}

	// read the data
	block = MSG_ReadShort( msg );

	// TTimo - www dl
	// if we haven't acked the download redirect yet
	if ( block == -1 )
	{
		if ( !clc.bWWWDl )
		{
			// server is sending us a www download
			Q_strncpyz( cls.originalDownloadName, cls.downloadName, sizeof( cls.originalDownloadName ) );
			Q_strncpyz( cls.downloadName, MSG_ReadString( msg ), sizeof( cls.downloadName ) );
			clc.downloadSize = MSG_ReadLong( msg );
			clc.downloadFlags = MSG_ReadLong( msg );

			Cvar_SetValue( "cl_downloadSize", clc.downloadSize );
			Com_DPrintf( "Server redirected download: %s\n", cls.downloadName );
			clc.bWWWDl = true; // activate wwwdl client loop
			CL_AddReliableCommand( "wwwdl ack" );
			cls.state = CA_DOWNLOADING;

			// make sure the server is not trying to redirect us again on a bad checksum
			if ( strstr( clc.badChecksumList, va( "@%s", cls.originalDownloadName ) ) )
			{
				Com_Printf( "refusing redirect to %s by server (bad checksum)\n", cls.downloadName );
				CL_AddReliableCommand( "wwwdl fail" );
				clc.bWWWDlAborting = true;
				return;
			}

			if ( !DL_BeginDownload( cls.downloadTempName, cls.downloadName, com_developer->integer ) )
			{
				// setting bWWWDl to false after sending the wwwdl fail doesn't work
				// not sure why, but I suspect we have to eat all remaining block -1 that the server has sent us
				// still leave a flag so that CL_WWWDownload is inactive
				// we count on server sending us a gamestate to start up clean again
				CL_AddReliableCommand( "wwwdl fail" );
				clc.bWWWDlAborting = true;
				Com_Printf( "Failed to initialize download for '%s'\n", cls.downloadName );
			}

			// Check for a disconnected download
			// we'll let the server disconnect us when it gets the bbl8r message
			if ( clc.downloadFlags & ( 1 << DL_FLAG_DISCON ) )
			{
				CL_AddReliableCommand( "wwwdl bbl8r" );
				cls.bWWWDlDisconnected = true;
			}

			return;
		}
		else
		{
			// server keeps sending that message till we ack it, eat and ignore
			//MSG_ReadLong( msg );
			MSG_ReadString( msg );
			MSG_ReadLong( msg );
			MSG_ReadLong( msg );
			return;
		}
	}

	if ( !block )
	{
		// block zero is special, contains file size
		clc.downloadSize = MSG_ReadLong( msg );

		Cvar_SetValue( "cl_downloadSize", clc.downloadSize );

		if ( clc.downloadSize < 0 )
		{
			Com_Error( ERR_DROP, "%s", MSG_ReadString( msg ) );
		}
	}

	size = MSG_ReadShort( msg );

	if ( size < 0 || size > sizeof( data ) )
	{
		Com_Error( ERR_DROP, "CL_ParseDownload: Invalid size %d for download chunk.", size );
	}

	MSG_ReadData( msg, data, size );

	if ( clc.downloadBlock != block )
	{
		Com_DPrintf( "CL_ParseDownload: Expected block %d, got %d\n", clc.downloadBlock, block );
		return;
	}

	// open the file if not opened yet
	if ( !clc.download )
	{
		clc.download = FS_SV_FOpenFileWrite( cls.downloadTempName );

		if ( !clc.download )
		{
			Com_Printf( "Could not create %s\n", cls.downloadTempName );
			CL_AddReliableCommand( "stopdl" );
			CL_NextDownload();
			return;
		}
	}

	if ( size )
	{
		FS_Write( data, size, clc.download );
	}

	CL_AddReliableCommand( va( "nextdl %d", clc.downloadBlock ) );
	clc.downloadBlock++;

	clc.downloadCount += size;

	// So UI gets access to it
	Cvar_SetValue( "cl_downloadCount", clc.downloadCount );

	if ( !size )
	{
		// A zero length block means EOF
		if ( clc.download )
		{
			FS_FCloseFile( clc.download );
			clc.download = 0;

			// rename the file
			FS_SV_Rename( cls.downloadTempName, cls.downloadName );
		}

		*cls.downloadTempName = *cls.downloadName = 0;
		Cvar_Set( "cl_downloadName", "" );

		// send intentions now
		// We need this because without it, we would hold the last nextdl and then start
		// loading right away.  If we take a while to load, the server is happily trying
		// to send us that last block over and over.
		// Write it twice to help make sure we acknowledge the download
		CL_WritePacket();
		CL_WritePacket();

		// get another file if needed
		CL_NextDownload();
	}
}

#ifdef USE_VOIP
static
bool CL_ShouldIgnoreVoipSender( int sender )
{
	if ( !cl_voip->integer )
	{
		return true; // VoIP is disabled.
	}
	else if ( ( sender == clc.clientNum ) && ( !clc.demoplaying ) )
	{
		return true; // ignore own voice (unless playing back a demo).
	}
	else if ( clc.voipMuteAll )
	{
		return true; // all channels are muted with extreme prejudice.
	}
	else if ( clc.voipIgnore[ sender ] )
	{
		return true; // just ignoring this guy.
	}
	else if ( clc.voipGain[ sender ] == 0.0f )
	{
		return true; // too quiet to play.
	}

	return false;
}

/*
=====================
CL_PlayVoip

Play raw data
=====================
*/

static void CL_PlayVoip( int sender, int samplecnt, const byte *data, int flags )
{
	if ( flags & VOIP_DIRECT )
	{
		Audio::StreamData( sender + 1, data, samplecnt, clc.speexSampleRate, 2, 1, clc.voipGain[ sender ], -1 );
	}

	if ( flags & VOIP_SPATIAL )
	{
		Audio::StreamData( sender + MAX_CLIENTS + 1, data, samplecnt, clc.speexSampleRate, 2, 1, 1.0f, sender );
	}
}

/*
=====================
CL_ParseVoip

A VoIP message has been received from the server
=====================
*/
static
void CL_ParseVoip( msg_t *msg )
{
    const int decodedSize = 4096 * sizeof(short); //FIXME: don't hardcode.

	const int    sender = MSG_ReadShort( msg );
	const int    generation = MSG_ReadByte( msg );
	const int    sequence = MSG_ReadLong( msg );
	const int    frames = MSG_ReadByte( msg );
	const int    packetsize = MSG_ReadShort( msg );
	const int    flags = MSG_ReadBits( msg, VOIP_FLAGCNT );
	char         encoded[ 1024 ];
	int          seqdiff = sequence - clc.voipIncomingSequence[ sender ];
	int          written = 0;
	int          i;

	Com_DPrintf( "VoIP: %d-byte packet from client %d\n", packetsize, sender );

	if ( sender < 0 )
	{
		return; // short/invalid packet, bail.
	}
	else if ( generation < 0 )
	{
		return; // short/invalid packet, bail.
	}
	else if ( sequence < 0 )
	{
		return; // short/invalid packet, bail.
	}
	else if ( frames < 0 )
	{
		return; // short/invalid packet, bail.
	}
	else if ( packetsize < 0 )
	{
		return; // short/invalid packet, bail.
	}

	if ( packetsize > sizeof( encoded ) )  // overlarge packet?
	{
		int bytesleft = packetsize;

		while ( bytesleft )
		{
			int br = bytesleft;

			if ( br > sizeof( encoded ) )
			{
				br = sizeof( encoded );
			}

			MSG_ReadData( msg, encoded, br );
			bytesleft -= br;
		}

		return; // overlarge packet, bail.
	}

	if ( !clc.speexInitialized )
	{
		MSG_ReadData( msg, encoded, packetsize );  // skip payload.
		return; // can't handle VoIP without libspeex!
	}
	else if ( sender >= MAX_CLIENTS )
	{
		MSG_ReadData( msg, encoded, packetsize );  // skip payload.
		return; // bogus sender.
	}
	else if ( CL_ShouldIgnoreVoipSender( sender ) )
	{
		MSG_ReadData( msg, encoded, packetsize );  // skip payload.
		return; // Channel is muted, bail.
	}

	// !!! FIXME: make sure data is narrowband? Does decoder handle this?

	Com_DPrintf( "VoIP: packet accepted!\n" );

	// This is a new "generation" ... a new recording started, reset the bits.
	if ( generation != clc.voipIncomingGeneration[ sender ] )
	{
		Com_DPrintf( "VoIP: new generation %d!\n", generation );
		speex_bits_reset( &clc.speexDecoderBits[ sender ] );
		clc.voipIncomingGeneration[ sender ] = generation;
		seqdiff = 0;
	}
	else if ( seqdiff < 0 ) // we're ahead of the sequence?!
	{
		// This shouldn't happen unless the packet is corrupted or something.
		Com_DPrintf( "VoIP: misordered sequence! %d < %d!\n",
		             sequence, clc.voipIncomingSequence[ sender ] );
		// reset the bits just in case.
		speex_bits_reset( &clc.speexDecoderBits[ sender ] );
		seqdiff = 0;
	}
	else if ( seqdiff * clc.speexFrameSize * 2 >= decodedSize ) // more than 2 seconds of audio dropped?
	{
		// just start over.
		Com_DPrintf( "VoIP: Dropped way too many (%d) frames from client #%d\n",
		             seqdiff, sender );
		speex_bits_reset( &clc.speexDecoderBits[ sender ] );
		seqdiff = 0;
	}

    short *decoded = new short[decodedSize/sizeof(short)];

	if ( seqdiff != 0 )
	{
		Com_DPrintf( "VoIP: Dropped %d frames from client #%d\n",
		             seqdiff, sender );

		// tell speex that we're missing frames...
		for ( i = 0; i < seqdiff; i++ )
		{
			assert( ( written + clc.speexFrameSize ) * 2 < decodedSize );
			speex_decode_int( clc.speexDecoder[ sender ], nullptr, decoded + written );
			written += clc.speexFrameSize;
		}
	}

	for ( i = 0; i < frames; i++ )
	{
		const int len = MSG_ReadByte( msg );

		if ( len < 0 )
		{
			Com_DPrintf( "VoIP: Short packet!\n" );
			break;
		}

		MSG_ReadData( msg, encoded, len );

		// shouldn't happen, but just in case...
		if ( ( written + clc.speexFrameSize ) * 2 > decodedSize )
		{
			Com_DPrintf( "VoIP: playback %d bytes, %d samples, %d frames\n",
			             written * 2, written, i );

			CL_PlayVoip( sender, written, ( const byte * ) decoded, flags );
			written = 0;
            decoded = new short[decodedSize / sizeof(short)];
		}

		speex_bits_read_from( &clc.speexDecoderBits[ sender ], encoded, len );
		speex_decode_int( clc.speexDecoder[ sender ],
		                  &clc.speexDecoderBits[ sender ], decoded + written );

		written += clc.speexFrameSize;
	}

	Com_DPrintf( "VoIP: playback %d bytes, %d samples, %d frames\n",
	             written * 2, written, i );

	if ( written > 0 )
	{
		CL_PlayVoip( sender, written, ( const byte * ) decoded, flags );
	}

	cls.voipSender = sender;

	clc.voipIncomingSequence[ sender ] = sequence + frames;
}

#endif

/*
=====================
CL_ParseCommandString

Command strings are just saved off until cgame asks for them
when it transitions a snapshot
=====================
*/
void CL_ParseCommandString( msg_t *msg )
{
	char *s;
	int  seq;
	int  index;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed stored it off
	if ( clc.serverCommandSequence >= seq )
	{
		return;
	}

	clc.serverCommandSequence = seq;

	index = seq & ( MAX_RELIABLE_COMMANDS - 1 );
	Q_strncpyz( clc.serverCommands[ index ], s, sizeof( clc.serverCommands[ index ] ) );
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage( msg_t *msg )
{
	int cmd;
//	msg_t           msgback;

//	msgback = *msg;

	if ( cl_shownet->integer == 1 )
	{
		Com_Printf("%i ", msg->cursize );
	}
	else if ( cl_shownet->integer >= 2 )
	{
		Com_Printf( "------------------\n" );
	}

	MSG_Bitstream( msg );

	// get the reliable sequence acknowledge number
	clc.reliableAcknowledge = MSG_ReadLong( msg );

	//
	if ( clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS )
	{
		clc.reliableAcknowledge = clc.reliableSequence;
	}

	//
	// parse the message
	//
	while ( 1 )
	{
		if ( msg->readcount > msg->cursize )
		{
			Com_Error( ERR_DROP, "CL_ParseServerMessage: read past end of server message" );
		}

		cmd = MSG_ReadByte( msg );

		// See if this is an extension command after the EOF, which means we
		//  got data that a legacy client should ignore.
		if ( ( cmd == svc_EOF ) && ( MSG_LookaheadByte( msg ) == svc_extension ) )
		{
			SHOWNET( msg, "EXTENSION" );
			MSG_ReadByte( msg );  // throw the svc_extension byte away.
			cmd = MSG_ReadByte( msg );  // something legacy clients can't do!

			// sometimes you get a svc_extension at end of stream...dangling
			//  bits in the huffman decoder giving a bogus value?
			if ( cmd == -1 )
			{
				cmd = svc_EOF;
			}
		}

		if ( cmd == svc_EOF )
		{
			SHOWNET( msg, "END OF MESSAGE" );
			break;
		}

		if ( cl_shownet->integer >= 2 )
		{
			if ( !svc_strings[ cmd ] )
			{
				Com_Printf( "%3i:BAD CMD %i\n", msg->readcount - 1, cmd );
			}
			else
			{
				SHOWNET( msg, svc_strings[ cmd ] );
			}
		}

		// other commands
		switch ( cmd )
		{
			default:
				Com_Error( ERR_DROP, "CL_ParseServerMessage: Illegible server message %d", cmd );

			case svc_nop:
				break;

			case svc_serverCommand:
				CL_ParseCommandString( msg );
				break;

			case svc_gamestate:
				CL_ParseGamestate( msg );
				break;

			case svc_snapshot:
				CL_ParseSnapshot( msg );
				break;

			case svc_download:
				CL_ParseDownload( msg );
				break;

			case svc_voip:
#ifdef USE_VOIP
				CL_ParseVoip( msg );
#endif
				break;
		}
	}

}
