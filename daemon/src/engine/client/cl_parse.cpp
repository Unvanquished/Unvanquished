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
	"svc_EOF"
};

static void SHOWNET( msg_t *msg, const char *s )
{
	if ( cl_shownet->integer >= 2 )
	{
		Log::Notice( "%3i:%s\n", msg->readcount - 1, s );
	}
}

/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/

// TODO(kangz) if we can make sure that the baseline entities have the correct entity
// number, then we could grab the entity number from old directly, simplifying code a bit.
void CL_DeltaEntity( msg_t *msg, clSnapshot_t *snapshot, int entityNum, const entityState_t &oldEntity)
{
    entityState_t entity;
    MSG_ReadDeltaEntity(msg, &oldEntity, &entity, entityNum);

    if (entity.number != MAX_GENTITIES - 1) {
        snapshot->entities.push_back(entity);
    }
}

/*
==================
CL_ParsePacketEntities

==================
*/
void CL_ParsePacketEntities( msg_t *msg, const clSnapshot_t *oldSnapshot, clSnapshot_t *newSnapshot )
{
    // The entity packet contains the delta between the two snapshots, with data only
    // for entities that were created, changed or removed. Entities entries are in
    // order of increasing entity number, as are entities in a snapshot. Using this we
    // have an efficient algorithm to create the new snapshot, that goes over the old
    // snapshot once from the beginning to the end.

    // If we don't have an old snapshot or it is empty, we'll recreate all entities
    // from the baseline entities as setting oldEntityNum to MAX_GENTITIES will force
    // us to only do step (3) below.
    unsigned int oldEntityNum = MAX_GENTITIES;
    if (oldSnapshot && oldSnapshot->entities.size() > 0){
        oldEntityNum = oldSnapshot->entities[0].number;
    }

    // Likewise when we don't have an old snapshot, oldEntities just has to be an empty
    // vector so that we skip step (4)
    std::vector<entityState_t> dummyEntities;
    auto& oldEntities = oldSnapshot? oldSnapshot->entities : dummyEntities;
    auto& newEntities = newSnapshot->entities;

    unsigned int numEntities = MSG_ReadShort(msg);
    newEntities.reserve(numEntities);

	unsigned oldIndex = 0;

    while (true) {
        unsigned int newEntityNum = MSG_ReadBits(msg, GENTITYNUM_BITS);

        if (msg->readcount > msg->cursize) {
            Sys::Drop("CL_ParsePacketEntities: Unexpected end of message");
        }

        if (newEntityNum == MAX_GENTITIES - 1) {
            break;
        }

        // (1) all entities that weren't specified between the previous newEntityNum and
        // the current one are unchanged and just copied over.
        while (oldEntityNum < newEntityNum) {
            newEntities.push_back(oldEntities[oldIndex]);

            oldIndex ++;
            if (oldIndex >= oldEntities.size()) {
                oldEntityNum = MAX_GENTITIES;
            } else {
                oldEntityNum = oldEntities[oldIndex].number;
            }
        }

        // (2) there is an entry for an entity in the old snapshot, apply the delta
        if (oldEntityNum == newEntityNum) {
            CL_DeltaEntity(msg, newSnapshot, newEntityNum, oldEntities[oldIndex]);

            oldIndex ++;
            if (oldIndex >= oldEntities.size()) {
                oldEntityNum = MAX_GENTITIES;
            } else {
                oldEntityNum = oldEntities[oldIndex].number;
            }
        } else {
            // (3) the entry isn't in the old snapshot, so the entity will be specified
            // from the baseline
            ASSERT_GT(oldEntityNum, newEntityNum);

            CL_DeltaEntity(msg, newSnapshot, newEntityNum, cl.entityBaselines[newEntityNum]);
        }
    }

    // (4) All remaining entities in the oldSnapshot are unchanged and copied over
    while (oldIndex < oldEntities.size()) {
        newEntities.push_back(oldEntities[oldIndex]);
        oldIndex ++;
    }

    ASSERT_EQ(numEntities, newEntities.size());
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
			Log::Notice( "Delta from invalid frame (not supposed to happen!).\n" );
		}
		else if ( old->messageNum != newSnap.deltaNum )
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Log::Debug( "Delta frame too old." );
		}
		else
		{
			newSnap.valid = true; // valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte( msg );

	if ( len > (int) sizeof( newSnap.areamask ) )
	{
		Com_Error( errorParm_t::ERR_DROP, "CL_ParseSnapshot: Invalid size %d for areamask.", len );
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
		Log::Notice( "   snapshot:%i  delta:%i  ping:%i\n", cl.snap.messageNum, cl.snap.deltaNum, cl.snap.ping );
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
				Com_Error(errorParm_t::ERR_DROP, "Client is missing paks but downloads are disabled");
			} else if (clc.demoplaying) {
				Com_Error(errorParm_t::ERR_DROP, "Client is missing paks needed by the demo");
			}
		}
	}

	// don't set any vars when playing a demo
	if ( clc.demoplaying )
	{
		return;
	}

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
				Com_Error( errorParm_t::ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
			}

			const char* str = MSG_ReadBigString( msg );
			cl.gameState[i] = str;
		}
		else if ( cmd == svc_baseline )
		{
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );

			if ( newnum < 0 || newnum >= MAX_GENTITIES )
			{
				Com_Error( errorParm_t::ERR_DROP, "Baseline number out of range: %i", newnum );
			}

			memset( &nullstate, 0, sizeof( nullstate ) );
			es = &cl.entityBaselines[ newnum ];
			MSG_ReadDeltaEntity( msg, &nullstate, es, newnum );
		}
		else
		{
			Com_Error( errorParm_t::ERR_DROP, "CL_ParseGamestate: bad command byte" );
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
		Log::Notice( "Server sending download, but no download was requested\n" );
        // Eat the packet anyway
	    block = MSG_ReadShort( msg );
        if (block == -1) {
			MSG_ReadString( msg );
			MSG_ReadLong( msg );
			MSG_ReadLong( msg );
        } else if (block != 0) {
            size = MSG_ReadShort( msg );
            if ( size < 0 || size > (int) sizeof( data ) )
            {
                Com_Error( errorParm_t::ERR_DROP, "CL_ParseDownload: Invalid size %d for download chunk.", size );
            }
	        MSG_ReadData( msg, data, size );
        }

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

            downloadLogger.Debug("Server sent us a new WWW DL '%s', size %i, flags %i",
                                 cls.downloadName, clc.downloadSize, clc.downloadFlags);

			Cvar_SetValue( "cl_downloadSize", clc.downloadSize );
			clc.bWWWDl = true; // activate wwwdl client loop
			CL_AddReliableCommand( "wwwdl ack" );
			cls.state = connstate_t::CA_DOWNLOADING;

			// make sure the server is not trying to redirect us again on a bad checksum
			if ( strstr( clc.badChecksumList, va( "@%s", cls.originalDownloadName ) ) )
			{
				Log::Notice( "refusing redirect to %s by server (bad checksum)\n", cls.downloadName );
				CL_AddReliableCommand( "wwwdl fail" );
				clc.bWWWDlAborting = true;
				return;
			}

			if ( !DL_BeginDownload( cls.downloadTempName, cls.downloadName ) )
			{
				// setting bWWWDl to false after sending the wwwdl fail doesn't work
				// not sure why, but I suspect we have to eat all remaining block -1 that the server has sent us
				// still leave a flag so that CL_WWWDownload is inactive
				// we count on server sending us a gamestate to start up clean again
				CL_AddReliableCommand( "wwwdl fail" );
				clc.bWWWDlAborting = true;
				Log::Notice( "Failed to initialize download for '%s'\n", cls.downloadName );
			}

			// Check for a disconnected download
			// we'll let the server disconnect us when it gets the bbl8r message
			if ( clc.downloadFlags & DL_FLAG_DISCON )
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

        downloadLogger.Debug("Starting new direct download of size %i for '%s'", clc.downloadSize, cls.downloadTempName);
		Cvar_SetValue( "cl_downloadSize", clc.downloadSize );

		if ( clc.downloadSize < 0 )
		{
			Com_Error( errorParm_t::ERR_DROP, "%s", MSG_ReadString( msg ) );
		}
	}

	size = MSG_ReadShort( msg );

	if ( size < 0 || size > (int) sizeof( data ) )
	{
		Com_Error( errorParm_t::ERR_DROP, "CL_ParseDownload: Invalid size %d for download chunk.", size );
	}

    downloadLogger.Debug("Received block of size %i", size);

	MSG_ReadData( msg, data, size );

	if ( clc.downloadBlock != block )
	{
		downloadLogger.Debug( "CL_ParseDownload: Expected block %i, got %i", clc.downloadBlock, block );
		return;
	}

	// open the file if not opened yet
	if ( !clc.download )
	{
		clc.download = FS_SV_FOpenFileWrite( cls.downloadTempName );

		if ( !clc.download )
		{
			Log::Notice( "Could not create %s\n", cls.downloadTempName );
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
        downloadLogger.Debug("Received EOF, closing '%s'", cls.downloadTempName);
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
CL_ParseBinaryMessage
=====================
*/
void CL_ParseBinaryMessage(msg_t *msg)
{
	MSG_BeginReadingUncompressed(msg);
	int ssize = msg->cursize - msg->readcount;
	if (ssize <= 0 || ssize > MAX_BINARY_MESSAGE) {
		return;
	}
	CL_CGameBinaryMessageReceived(msg->data + msg->readcount, size_t(ssize), cl.snap.serverTime);
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
		Log::Notice("%i ", msg->cursize );
	}
	else if ( cl_shownet->integer >= 2 )
	{
		Log::Notice( "------------------\n" );
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
			Com_Error( errorParm_t::ERR_DROP, "CL_ParseServerMessage: read past end of server message" );
		}

		cmd = MSG_ReadByte( msg );

		if ( cmd < 0 || cmd == svc_EOF )
		{
			SHOWNET( msg, "END OF MESSAGE" );
			break;
		}

		if ( cl_shownet->integer >= 2 )
		{
			if ( !svc_strings[ cmd ] )
			{
				Log::Notice( "%3i:BAD CMD %i\n", msg->readcount - 1, cmd );
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
				Com_Error( errorParm_t::ERR_DROP, "CL_ParseServerMessage: Illegible server message %d", cmd );

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
		}
	}
	CL_ParseBinaryMessage( msg );
}
