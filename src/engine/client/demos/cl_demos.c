/*
===========================================================================
Copyright (C) 2006 Sjoerd van der Berg

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cl_demos.c -- Enhanced demo player and server demo recorder

#include "../client.h"
#include "../../server/server.h"
#include "cl_demos.h"

#define DEMOLISTSIZE 1024

#define DEMO_ERROR_NONE		0
#define DEMO_ERROR_NOEXIST	1
#define DEMO_ERROR_VERSION	2

typedef struct {
	char demoName[ MAX_OSPATH ];
	char projectName[ MAX_OSPATH ];
} demoListEntry_t;

static demoListEntry_t	demoList[DEMOLISTSIZE];
static int				demoListIndex, demoListCount;
static demo_t			demo;
static byte				demoBuffer[128*1024];
static entityState_t	demoNullEntityState;
static playerState_t	demoNullPlayerState;

static const char *demoHeader = Q3_VERSION "Demo";

static void demoFrameAddString( demoString_t *string, int num, const char *newString) {
	int			dataLeft, len;
	int			i;
	char		cache[sizeof(string->data)];

	if (!newString[0]) {
		string->offsets[num] = 0;
		return;
	}
	len = strlen( newString ) + 1;
	dataLeft = sizeof(string->data) - string->used;
	if (len <= dataLeft) {
		Com_Memcpy( string->data + string->used, newString, len );
		string->offsets[num] = string->used;
		string->used += len;
		return;
	}
	cache[0] = 0;
	string->used = 1;
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		const char * s;
		s = (i == num) ? newString : string->data + string->offsets[i];
		if (!s[0]) {
			string->offsets[i] = 0;
			continue;
		}
		dataLeft = sizeof(cache) - string->used;
		len = strlen( s ) + 1;
		if ( len >= dataLeft) {
			Com_Printf( "Adding configstring %i with %s overflowed buffer", i, s );
			return;
		}
		Com_Memcpy( cache + string->used, s, len );
		string->offsets[i] = string->used;
		string->used += len;
	}
	Com_Memcpy( string->data, cache, string->used );
}

static void demoFrameUnpack( msg_t *msg, demoFrame_t *oldFrame, demoFrame_t *newFrame ) {
	int last;
	qboolean isDelta = MSG_ReadBits( msg, 1 ) ? qfalse : qtrue;
	if (!isDelta)
		oldFrame = 0;

	newFrame->serverTime = MSG_ReadLong( msg );
	/* Read config strings */
	newFrame->string.data[0] = 0;
	newFrame->string.used = 1;
	last = 0;
	/* Extract config strings */
	while ( 1 ) {
		int i, num = MSG_ReadShort( msg );
		if (!isDelta ) {
			for (i = last;i<num;i++)
				newFrame->string.offsets[i] = 0;
		} else {
			for (i = last;i<num;i++)
				demoFrameAddString( &newFrame->string, i, oldFrame->string.data + oldFrame->string.offsets[i] );
		}
		if (num < MAX_CONFIGSTRINGS) {
			demoFrameAddString( &newFrame->string, num, MSG_ReadBigString( msg ) );
		} else {
			break;
		}
		last = num + 1;
	}
    /* Extract player states */
	Com_Memset( newFrame->clientData, 0, sizeof( newFrame->clientData ));
	last = MSG_ReadByte( msg );
	while (last < MAX_CLIENTS) {
		playerState_t *oldPlayer, *newPlayer;
		newFrame->clientData[last] = 1;
		oldPlayer = isDelta && oldFrame->clientData[last] ? &oldFrame->clients[last] : &demoNullPlayerState;
		newPlayer = &newFrame->clients[last];
		MSG_ReadDeltaPlayerstate( msg, oldPlayer, newPlayer );
		last = MSG_ReadByte( msg );
	}
	/* Extract entity states */
	last = 0;
	while ( 1 ) {
		int i, num = MSG_ReadBits( msg, GENTITYNUM_BITS );
		entityState_t *oldEntity, *newEntity;	
		if ( isDelta ) {
			for (i = last;i<num;i++)
				if (oldFrame->entities[i].number == i)
					newFrame->entities[i] = oldFrame->entities[i];
				else
					newFrame->entities[i].number = MAX_GENTITIES - 1;
		} else {
			for (i = last;i<num;i++)
				newFrame->entities[i].number = MAX_GENTITIES - 1;
		}
		if (num < MAX_GENTITIES - 1) {
			if (isDelta) {
				oldEntity = &oldFrame->entities[num];
				if (oldEntity->number != num)
					oldEntity = &demoNullEntityState;
			} else {
				oldEntity = &demoNullEntityState;
			}
			newEntity = &newFrame->entities[i];
			MSG_ReadDeltaEntity( msg, oldEntity, newEntity, num );
		} else
			break;
		last = num + 1;
	}
	/* Read the area mask */
	newFrame->areaUsed = MSG_ReadByte( msg );
	MSG_ReadData( msg, newFrame->areamask, newFrame->areaUsed );
	/* Read the command string data */
	newFrame->commandUsed = MSG_ReadLong( msg );
	MSG_ReadData( msg, newFrame->commandData, newFrame->commandUsed );
}

static void demoFramePack( msg_t *msg, const demoFrame_t *newFrame, const demoFrame_t *oldFrame ) {
	int i;
	/* Full or delta frame marker */
	MSG_WriteBits( msg, oldFrame ? 0 : 1, 1 );
	MSG_WriteLong( msg, newFrame->serverTime );
	/* Add the config strings */
	for (i = 0;i<MAX_CONFIGSTRINGS;i++) {
		const char *oldString = !oldFrame ? "" : &oldFrame->string.data[oldFrame->string.offsets[i]];
		const char *newString = newFrame->string.data + newFrame->string.offsets[i];
		if (strcmp( oldString, newString)) {
			MSG_WriteShort( msg, i );
			MSG_WriteBigString( msg, newString );
		}
	}
	MSG_WriteShort( msg, MAX_CONFIGSTRINGS );
	/* Add the playerstates */
	for (i=0; i<MAX_CLIENTS; i++) {
		const playerState_t *oldPlayer, *newPlayer;
		if (!newFrame->clientData[i])
			continue;
		oldPlayer = (!oldFrame || !oldFrame->clientData[i]) ? &demoNullPlayerState : &oldFrame->clients[i];
		newPlayer = &newFrame->clients[i];
		MSG_WriteByte( msg, i );
		MSG_WriteDeltaPlayerstate( msg, (playerState_t *)oldPlayer, (playerState_t *)newPlayer );
	}
	MSG_WriteByte( msg, MAX_CLIENTS );
	/* Add the entities */
	for (i=0; i<MAX_GENTITIES-1; i++) {
		const entityState_t *oldEntity, *newEntity;
		newEntity = &newFrame->entities[i];
		if (oldFrame) {
			oldEntity = &oldFrame->entities[i];
			if (oldEntity->number == (MAX_GENTITIES -1))
				oldEntity = 0;
		} else {
			oldEntity = 0;
		}
		if (newEntity->number != i || newEntity->number >= (MAX_GENTITIES -1)) {
			newEntity = 0;
		} else {
			if (!oldEntity) {
				oldEntity = &demoNullEntityState;
			}
		}
		MSG_WriteDeltaEntity( msg, (entityState_t *)oldEntity, (entityState_t *)newEntity, qfalse );
	}
	MSG_WriteBits( msg, (MAX_GENTITIES-1), GENTITYNUM_BITS );
	/* Add the area mask */
	MSG_WriteByte( msg, newFrame->areaUsed );
	MSG_WriteData( msg, newFrame->areamask, newFrame->areaUsed );
	/* Add the command string data */
	MSG_WriteLong( msg, newFrame->commandUsed );
	MSG_WriteData( msg, newFrame->commandData, newFrame->commandUsed );
}

static void demoFrameInterpolate( demoFrame_t frames[], int frameCount, int index ) {
	int i;
	demoFrame_t *workFrame;

	workFrame = &frames[index % frameCount];
//	return;

	for (i=0; i<MAX_CLIENTS; i++) {
		entityState_t *workEntity;
		workEntity = &workFrame->entities[i];
		if (workEntity->number != ENTITYNUM_NONE && !workFrame->entityData[i]) {
			int m;
			demoFrame_t *prevFrame, *nextFrame;
			prevFrame = nextFrame = 0;
			for (m = index - 1; (m > (index - frameCount)) && m >0 ; m--) {
				demoFrame_t *testFrame = &frames[ m % frameCount];
				if ( !testFrame->entityData[i])
					continue;
				if (!testFrame->serverTime || testFrame->serverTime >= workFrame->serverTime)
					break;
				if ( testFrame->entities[i].number != i)
					break;
				if ( (testFrame->entities[i].eFlags ^ workEntity->eFlags) & EF_TELEPORT_BIT )
					break;
				prevFrame = testFrame;
				break;
			}
			for (m = index + 1; (m < (index + frameCount)); m++) {
				demoFrame_t *testFrame = &frames[ m % frameCount];
				if ( !testFrame->entityData[i])
					continue;
				if (!testFrame->serverTime || testFrame->serverTime <= workFrame->serverTime)
					break;
				if ( testFrame->entities[i].number != i)
					break;
				if ( (testFrame->entities[i].eFlags ^ workEntity->eFlags) & EF_TELEPORT_BIT )
					break;
				nextFrame = testFrame;
				break;
			}
			if (prevFrame && nextFrame) {
				const entityState_t *prevEntity = &prevFrame->entities[i];
				const entityState_t *nextEntity = &nextFrame->entities[i];
				float lerp;
				int posDelta;
				float posLerp;
				int	 prevTime, nextTime;
                  
				prevTime = prevFrame->serverTime;
				nextTime = nextFrame->serverTime;
				lerp = (workFrame->serverTime - prevTime) / (float)( nextTime - prevTime );
				posDelta = nextEntity->pos.trTime - prevEntity->pos.trTime;
				if ( posDelta ) {
					workEntity->pos.trTime = prevEntity->pos.trTime + posDelta * lerp;
					posLerp = (workEntity->pos.trTime - prevEntity->pos.trTime) / (float) posDelta;
				} else {
					posLerp = lerp;
				}
				LerpOrigin( prevEntity->pos.trBase, nextEntity->pos.trBase, workEntity->pos.trBase, posLerp );
				LerpOrigin( prevEntity->pos.trDelta, nextEntity->pos.trDelta, workEntity->pos.trDelta, posLerp );

				LerpAngles( prevEntity->apos.trBase, nextEntity->apos.trBase, workEntity->apos.trBase, lerp );
			}
		}
	}
}

static void demoLogFile( fileHandle_t fileHandle, const char *fmt, ... ) {
	va_list		argptr;
	int			len;
	char		buf[1024];

	va_start (argptr,fmt);
	len = vsnprintf (buf, sizeof( buf),fmt,argptr);
	va_end (argptr);

	FS_Write( buf, len, fileHandle );
	FS_Write( "\n", 1, fileHandle );
}

void demoConvert( const char *oldName, const char *newBaseName, qboolean smoothen ) {
	fileHandle_t	oldHandle = 0;
	fileHandle_t	newHandle = 0;
	int				temp;
	int				oldSize;
	int				msgSequence;
	msg_t			oldMsg;
	byte			oldData[ MAX_MSGLEN ];
	int				oldTime, nextTime, fullTime;
	int				clientNum;
	demoFrame_t		*workFrame;
	int				parseEntitiesNum = 0;
	demoConvert_t	*convert;
	char			bigConfigString[BIG_INFO_STRING];
	int				bigConfigNumber;
	const char		*s;
	clSnapshot_t	*oldSnap = 0;
	clSnapshot_t	*newSnap;
	int				levelCount = 0;
	char			newName[MAX_OSPATH];

	oldSize = FS_FOpenFileRead( oldName, &oldHandle, qtrue );
	if (!oldHandle) {
		Com_Printf("Failed to open %s for conversion.", oldName);
		return;
	}
	/* Alloc some memory */
	convert = (demoConvert_t *)Z_Malloc( sizeof( demoConvert_t) );
	/* Initialize the first workframe's strings */
	while (oldSize > 0) {
		MSG_Init( &oldMsg, oldData, sizeof( oldData ) );
		/* Read the sequence number */
		if (FS_Read( &convert->messageNum, 4, oldHandle) != 4)
			goto conversionerror;
		convert->messageNum = LittleLong( convert->messageNum );
		oldSize -= 4;
		/* Read the message size */
		if (FS_Read( &oldMsg.cursize,4, oldHandle) != 4)
			goto conversionerror;
		oldSize -= 4;
		oldMsg.cursize = LittleLong( oldMsg.cursize );
		/* Negative size signals end of demo */
		if (oldMsg.cursize < 0)
			break;
		if ( oldMsg.cursize > oldMsg.maxsize ) 
			goto conversionerror;
		/* Read the actual message */
		if (FS_Read( oldMsg.data, oldMsg.cursize, oldHandle ) != oldMsg.cursize)
			goto conversionerror;
		oldSize -= oldMsg.cursize;
		// init the bitstream
		MSG_BeginReading( &oldMsg );
		// Skip the reliable sequence acknowledge number
		MSG_ReadLong( &oldMsg );
		//
		// parse the message
		//
		while ( 1 ) {
			byte cmd;
			if ( oldMsg.readcount > oldMsg.cursize ) {
				Com_Printf ("Demo conversion, read past end of server message.\n");
				goto conversionerror;
			}
            cmd = MSG_ReadByte( &oldMsg );
			// See if this is an extension command after the EOF, which means we
			//  got data that a legacy client should ignore.
			if ( ( cmd == svc_EOF ) && ( MSG_LookaheadByte( &oldMsg ) == svc_extension ) ){
				MSG_ReadByte( &oldMsg );  // throw the svc_extension byte away.
				cmd = MSG_ReadByte( &oldMsg );  // something legacy clients can't do!
				// sometimes you get a svc_extension at end of stream...dangling
				//  bits in the huffman decoder giving a bogus value?
				if ( cmd == -1 ) {
					cmd = svc_EOF;
				}
			}
			if ( cmd == svc_EOF) {
                break;
			}
			workFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES ];
			// other commands
			switch ( cmd ) {
			default:
				Com_Error (ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
				break;			
			case svc_nop:
				break;
			case svc_serverCommand:
				temp = MSG_ReadLong( &oldMsg );
				s = MSG_ReadString( &oldMsg );
				if (temp<=msgSequence)
					break;
//				Com_Printf( " server command %s\n", s );
				msgSequence = temp;
				Cmd_TokenizeString( s );
	
				if ( !Q_stricmp( Cmd_Argv(0), "bcs0" ) ) {
					bigConfigNumber = atoi( Cmd_Argv(1) );
					Q_strncpyz( bigConfigString, Cmd_Argv(2), sizeof( bigConfigString ));
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "bcs1" ) ) {
					Q_strcat( bigConfigString, sizeof( bigConfigString ), Cmd_Argv(2));
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "bcs2" ) ) {
					Q_strcat( bigConfigString, sizeof( bigConfigString ), Cmd_Argv(2));
					demoFrameAddString( &workFrame->string, bigConfigNumber, bigConfigString );
					break;
				}
				if ( !Q_stricmp( Cmd_Argv(0), "cs" ) ) {
					int num = atoi( Cmd_Argv(1) );
					s = Cmd_ArgsFrom( 2 );
					demoFrameAddString( &workFrame->string, num, Cmd_ArgsFrom( 2 ) );
					break;
				}
				if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
					int len = strlen( s ) + 1;
					char *dst;
					if (workFrame->commandUsed + len + 1 > sizeof( workFrame->commandData)) {
						Com_Printf("Overflowed state command data.\n");
						goto conversionerror;
					}
					dst = workFrame->commandData + workFrame->commandUsed;
					*dst = clientNum;
					Com_Memcpy( dst+1, s, len );
					workFrame->commandUsed += len + 1;
				}
				break;
			case svc_gamestate:
				if (newHandle) {
					FS_FCloseFile( newHandle );
					newHandle = 0;
				}
				if (levelCount) {
					Com_sprintf( newName, sizeof( newName ), "%s.%d.mme", newBaseName, levelCount );
				} else {
					Com_sprintf( newName, sizeof( newName ), "%s.mme", newBaseName );
				}
				fullTime = -1;
				clientNum = -1;
				oldTime = -1;
				Com_Memset( convert, 0, sizeof( *convert ));
				convert->frames[0].string.used = 1;
				levelCount++;
				newHandle = FS_FOpenFileWrite( newName );
				if (!newHandle) {
					Com_Printf("Failed to open %s for target conversion target.\n", newName);
					goto conversionerror;
					return;
				} else {
					FS_Write ( demoHeader, strlen( demoHeader ), newHandle );
				}
				Com_sprintf( newName, sizeof( newName ), "%s.txt", newBaseName );
				workFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES ];
				msgSequence = MSG_ReadLong( &oldMsg );
				while( 1 ) {
					cmd = MSG_ReadByte( &oldMsg );
					if (cmd == svc_EOF)
						break;
					if ( cmd == svc_configstring) {
						int		num;
						const char *s;
						num = MSG_ReadShort( &oldMsg );
						s = MSG_ReadBigString( &oldMsg );
						demoFrameAddString( &workFrame->string, num, s );
					} else if ( cmd == svc_baseline ) {
						int num = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
						if ( num < 0 || num >= MAX_GENTITIES ) {
							Com_Printf( "Baseline number out of range: %i.\n", num );
							goto conversionerror;
						}
						MSG_ReadDeltaEntity( &oldMsg, &demoNullEntityState, &convert->entityBaselines[num], num );
					} else {
						Com_Printf( "Unknown block while converting demo gamestate.\n" );
						goto conversionerror;
					}
				}
				clientNum = MSG_ReadLong( &oldMsg );
				/* Skip the checksum feed */
				MSG_ReadLong( &oldMsg );
				break;
			case svc_snapshot:
				nextTime = MSG_ReadLong( &oldMsg );
				/* Delta number, not needed */
				newSnap = &convert->snapshots[convert->messageNum & PACKET_MASK];
				Com_Memset (newSnap, 0, sizeof(*newSnap));
				newSnap->deltaNum = MSG_ReadByte( &oldMsg );
				newSnap->messageNum = convert->messageNum;
				if (!newSnap->deltaNum) {
					newSnap->deltaNum = -1;
					newSnap->valid = qtrue;		// uncompressed frame
					oldSnap  = NULL;
				} else {
					newSnap->deltaNum = newSnap->messageNum - newSnap->deltaNum;
					oldSnap = &convert->snapshots[newSnap->deltaNum & PACKET_MASK];
					if (!oldSnap->valid) {
						Com_Printf( "Delta snapshot without base.\n" );
						goto conversionerror;
					} else if (oldSnap->messageNum != newSnap->deltaNum) {
						// The frame that the server did the delta from
						// is too old, so we can't reconstruct it properly.
						Com_Printf ("Delta frame too old.\n");
					} else if ( parseEntitiesNum - oldSnap->parseEntitiesNum > MAX_PARSE_ENTITIES-128 ) {
						Com_Printf ("Delta parseEntitiesNum too old.\n");
					} else {
						newSnap->valid = qtrue;	// valid delta parse
					}
				}

				/* Snapflags, not needed */
				newSnap->snapFlags = MSG_ReadByte( &oldMsg );
				// read areamask
				workFrame->areaUsed = MSG_ReadByte( &oldMsg );
				MSG_ReadData( &oldMsg, workFrame->areamask, workFrame->areaUsed );
				if (clientNum <0 || clientNum >= MAX_CLIENTS) {
					Com_Printf("Got snapshot with invalid client.\n");
					goto conversionerror;
				}
				MSG_ReadDeltaPlayerstate( &oldMsg, oldSnap ? &oldSnap->ps : &demoNullPlayerState, &newSnap->ps );
				/* Read the individual entities */
				newSnap->parseEntitiesNum = parseEntitiesNum;
				newSnap->numEntities = 0;
				Com_Memset( workFrame->entityData, 0, sizeof( workFrame->entityData ));

				/* The beast that is entity parsing */
				{
				int			newnum;
				entityState_t	*oldstate, *newstate;
				int			oldindex = 0;
				int			oldnum;
				newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
				while ( 1 ) {
					// read the entity index number
					if (oldSnap && oldindex < oldSnap->numEntities) {
						oldstate = &convert->parseEntities[(oldSnap->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES-1)];
						oldnum = oldstate->number;
					} else {
						oldstate = 0;
						oldnum = MAX_GENTITIES;
					}
					newstate = &convert->parseEntities[parseEntitiesNum];
					if ( !oldstate && (newnum == (MAX_GENTITIES-1))) {
						break;
					} else if ( oldnum < newnum ) {
						*newstate = *oldstate;
						oldindex++;
					} else if (oldnum == newnum) {
						oldindex++;
						MSG_ReadDeltaEntity( &oldMsg, oldstate, newstate, newnum );
						if ( newstate->number != MAX_GENTITIES-1)
							workFrame->entityData[ newstate->number ] = 1;
						newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
					} else if (oldnum > newnum) {
						MSG_ReadDeltaEntity( &oldMsg, &convert->entityBaselines[newnum], newstate , newnum );
						if ( newstate->number != MAX_GENTITIES-1)
							workFrame->entityData[ newstate->number ] = 1;
						newnum = MSG_ReadBits( &oldMsg, GENTITYNUM_BITS );
					}
					if (newstate->number == MAX_GENTITIES-1)
						continue;
					parseEntitiesNum++;
					parseEntitiesNum &= (MAX_PARSE_ENTITIES-1);
					newSnap->numEntities++;
				}}
				/* Stop processing this further since it's an invalid snap due to lack of delta data */
				if (!newSnap->valid)
					break;

				/* Skipped snapshots will be set invalid in the circular buffer */
				if ( newSnap->messageNum - convert->lastMessageNum >= PACKET_BACKUP ) {
					convert->lastMessageNum = newSnap->messageNum - ( PACKET_BACKUP - 1 );
				}
				for ( ; convert->lastMessageNum < newSnap->messageNum ; convert->lastMessageNum++ ) {
					convert->snapshots[convert->lastMessageNum & PACKET_MASK].valid = qfalse;
				}
				convert->lastMessageNum = newSnap->messageNum + 1;

				/* compress the frame into the new format */
				if (nextTime > oldTime) {
					demoFrame_t *cleanFrame;
					int writeIndex;
					for (temp = 0;temp<newSnap->numEntities;temp++) {
						int p = (newSnap->parseEntitiesNum+temp) & (MAX_PARSE_ENTITIES-1);
						entityState_t *newState = &convert->parseEntities[p];
						workFrame->entities[newState->number] = *newState;
					}
					workFrame->clientData[clientNum] = 1;
					workFrame->clients[clientNum] = newSnap->ps;
					workFrame->serverTime = nextTime;

					/* Which frame from the cache to save */
					writeIndex = convert->frameIndex - (DEMOCONVERTFRAMES/2);
					if (writeIndex >= 0) {
						const demoFrame_t *newFrame;
						msg_t writeMsg;
						// init the message
						MSG_Init( &writeMsg, demoBuffer, sizeof (demoBuffer));
						MSG_Clear( &writeMsg );
						MSG_Bitstream( &writeMsg );
						newFrame = &convert->frames[ writeIndex  % DEMOCONVERTFRAMES];
						if ( smoothen )
							demoFrameInterpolate( convert->frames, DEMOCONVERTFRAMES, writeIndex );
						if ( nextTime > fullTime || writeIndex <= 0 ) {
							/* Plan the next time for a full write */
							fullTime = nextTime + 2000;
							demoFramePack( &writeMsg, newFrame, 0 );
						} else {
							const demoFrame_t *oldFrame = &convert->frames[ ( writeIndex -1 ) % DEMOCONVERTFRAMES];
							demoFramePack( &writeMsg, newFrame, oldFrame );
						}
						/* Write away the new data in the msg queue */
						temp = LittleLong( writeMsg.cursize );
						FS_Write (&temp, 4, newHandle );
						FS_Write ( writeMsg.data , writeMsg.cursize, newHandle );
					}

					/* Clean up the upcoming frame for all new changes */
					convert->frameIndex++;
					cleanFrame = &convert->frames[ convert->frameIndex % DEMOCONVERTFRAMES];
					cleanFrame->serverTime = 0;
					for (temp = 0;temp<MAX_GENTITIES;temp++)
						cleanFrame->entities[temp].number = MAX_GENTITIES-1;
					Com_Memset( cleanFrame->clientData, 0, sizeof ( cleanFrame->clientData ));
					Com_Memcpy( cleanFrame->string.data, workFrame->string.data, workFrame->string.used );
					Com_Memcpy( cleanFrame->string.offsets, workFrame->string.offsets, sizeof( workFrame->string.offsets ));
					cleanFrame->string.used = workFrame->string.used;
					cleanFrame->commandUsed = 0;
					/* keep track of this last frame's time */
					oldTime = nextTime;
				}
				break;
			case svc_download:
				// read block number
				temp = MSG_ReadShort ( &oldMsg );
				if (temp == -1) {	//-1 block, www download
					MSG_ReadString( &oldMsg );
					MSG_ReadLong( &oldMsg );
					MSG_ReadLong( &oldMsg );
				}
				if (!temp)	//0 block, read file size
					MSG_ReadLong( &oldMsg );
				// read block size
				temp = MSG_ReadShort ( &oldMsg );
				// read the data block
				for ( ;temp>0;temp--)
					MSG_ReadByte( &oldMsg );
				break;
			}
		}
	}
conversionerror:
	FS_FCloseFile( oldHandle );
	FS_FCloseFile( newHandle );
	Z_Free( convert );
	return;
}

static void demoPlayAddCommand( demoPlay_t *play, const char *cmd ) {
	int len = strlen ( cmd ) + 1;
	int index = (++play->commandCount) % DEMO_PLAY_CMDS;
	int freeWrite;
	/* First clear previous command */
	int nextStart = play->commandStart[ index ];
	if (play->commandFree > nextStart) {
		int freeNext = sizeof( play->commandData ) - play->commandFree;
		if ( len <= freeNext ) {
			freeWrite = play->commandFree;
			play->commandFree += len;
		} else {
			if ( len > nextStart )
					Com_Error( ERR_DROP, "Ran out of server command space");
			freeWrite = 0;
			play->commandFree = len;
		}
	} else {
		int left = nextStart - play->commandFree;
		if ( len > left )
			Com_Error( ERR_DROP, "Ran out of server command space");
		freeWrite = play->commandFree;
		play->commandFree = (play->commandFree + len) % sizeof( play->commandData );
	}
	play->commandStart[ index ] = freeWrite;
	Com_Memcpy( play->commandData + freeWrite, cmd, len );
}

static void demoPlaySynch( demoPlay_t *play, demoFrame_t *frame) {
	int i;
	int startCount = play->commandCount;
	int totalLen = 0;
	for (i = 0;i<MAX_CONFIGSTRINGS;i++) {
		char *oldString = cl.gameState.stringData + cl.gameState.stringOffsets[i];
		char *newString = frame->string.data + frame->string.offsets[i];
		if (!strcmp( oldString, newString ))
			continue;
		totalLen += strlen( newString );
		demoPlayAddCommand( play, va("cs %d \"%s\"", i, newString) );
	}
	/* Copy the new server commands */
	for (i=0;i<frame->commandUsed;) {
		char *line = frame->commandData + i;
		int len, cmdClient;

		cmdClient = *line++;
		len = strlen( line ) + 1;
		i += len + 1;
		if ( cmdClient != play->clientNum ) 
			continue;
		demoPlayAddCommand( play, line );
		totalLen += strlen( line );
	}
	if (play->commandCount - startCount > DEMO_PLAY_CMDS) {
		Com_Error( ERR_DROP, "frame added more than %d commands", DEMO_PLAY_CMDS);
	}
//	Com_Printf("Added %d commands, length %d\n", play->commandCount - startCount, totalLen );
}


static void demoPlayForwardFrame( demoPlay_t *play ) {
	int			blockSize;
	msg_t		msg;
	demoFrame_t *copyFrame;

	if (play->filePos + 4 > play->fileSize) {
		play->lastFrame = qtrue;
		return;
	}
	play->lastFrame = qfalse;
	play->filePos += 4;
	FS_Read( &blockSize, 4, play->fileHandle );
	blockSize = LittleLong( blockSize );
	FS_Read( demoBuffer, blockSize, play->fileHandle );
	play->filePos += blockSize;
	MSG_Init( &msg, demoBuffer, sizeof(demoBuffer) );
	MSG_BeginReading( &msg );
	msg.cursize = blockSize;
	copyFrame = play->frame;
	play->frame = play->nextFrame;
	play->nextFrame = copyFrame;
	demoFrameUnpack( &msg, play->frame, play->nextFrame );
	play->frameNumber++;
}

static void demoPlaySetIndex( demoPlay_t *play, int index ) {
	int wantPos;
	if (index <0 || index >= play->fileIndexCount) {
		Com_Printf("demoFile index Out of range search\n");
		index = 0;
	}
	wantPos = play->fileIndex[index].pos;
#if 0
	if ( !play->fileHandle || play->filePos > wantPos) {
		trap_FS_FCloseFile( play->fileHandle );
		trap_FS_FOpenFile( play->fileName, &play->fileHandle, FS_READ );
		play->filePos = 0;
	}
	if (!play->fileHandle)
		Com_Error(0, "Failed to open demo file \n");
	while (play->filePos < wantPos) {
		int toRead = wantPos - play->filePos;
		if (toRead > sizeof(demoBuffer))
			toRead = sizeof(demoBuffer);
		play->filePos += toRead;
		trap_FS_Read( demoBuffer, toRead, play->fileHandle );
	}
#else
	FS_Seek( play->fileHandle, wantPos, FS_SEEK_SET );
	play->filePos = wantPos;
#endif

	demoPlayForwardFrame( play );
	demoPlayForwardFrame( play );

	play->frameNumber = play->fileIndex[index].frame;
}

static int demoPlaySeek( demoPlay_t *play, int seekTime ) {
	int i;

	seekTime += play->startTime;

	if ( seekTime < 0)
		seekTime = 0;
	
	if (seekTime < play->frame->serverTime || (seekTime > play->frame->serverTime + 1000)) {
		for (i=0;i<(play->fileIndexCount-1);i++) {
			if (play->fileIndex[i].time <= seekTime && 
				play->fileIndex[i+1].time > seekTime)
				goto foundit;
		}
		i = play->fileIndexCount-1;
foundit:
		demoPlaySetIndex( play, i);
	}
	while (!play->lastFrame && ( seekTime > play->nextFrame->serverTime)) {
		demoPlayForwardFrame( play  );
	}
	return seekTime;
}


static demoPlay_t *demoPlayOpen( const char* fileName ) {
	demoPlay_t	*play;
	fileHandle_t fileHandle;
	int	fileSize, filePos;
	int i;

	msg_t msg;
	fileSize = FS_FOpenFileRead( fileName, &fileHandle, qtrue );
	if (fileHandle<=0) {
		Com_Printf("Failed to open demo file %s \n", fileName );
		return 0;
	}
	filePos = strlen( demoHeader );
	i = FS_Read( &demoBuffer, filePos, fileHandle );
	if ( i != filePos || Q_strncmp( (const char *)demoBuffer, demoHeader, filePos )) {
		Com_Printf("demo file %s is wrong version\n", fileName );
		FS_FCloseFile( fileHandle );
		return 0;
	}
	play = (demoPlay_t *)Z_Malloc( sizeof( demoPlay_t ));
	Q_strncpyz( play->fileName, fileName, sizeof( play->fileName ));
	play->fileSize = fileSize;
	play->frame = &play->storageFrame[0];
	play->nextFrame = &play->storageFrame[1];
	for (i=0;i<DEMO_PLAY_CMDS;i++)
		play->commandStart[i] = i;
	play->commandFree = DEMO_PLAY_CMDS;

	for ( ; filePos<fileSize; ) {
		int blockSize, isFull, serverTime;
		FS_Read( &blockSize, 4, fileHandle );
		blockSize = LittleLong( blockSize );
		if (blockSize > sizeof(demoBuffer)) {
			Com_Printf( "Block too large to be read in.\n");
			goto errorreturn;
		}
		if ( blockSize + filePos > fileSize) {
			Com_Printf( "Block would read past the end of the file.\n");
			goto errorreturn;
		}
		FS_Read( demoBuffer, blockSize, fileHandle );
		MSG_Init( &msg, demoBuffer, sizeof(demoBuffer) );
		MSG_BeginReading( &msg );
		msg.cursize = blockSize;	
		isFull = MSG_ReadBits( &msg, 1 );
		serverTime = MSG_ReadLong( &msg );
		if (!play->startTime)
			play->startTime = serverTime;
		if (isFull) {
			if (play->fileIndexCount < DEMO_MAX_INDEX) {
				play->fileIndex[play->fileIndexCount].pos = filePos;
				play->fileIndex[play->fileIndexCount].frame = play->totalFrames;
				play->fileIndex[play->fileIndexCount].time = serverTime;
				play->fileIndexCount++;
			}
		}
		play->endTime = serverTime;
		filePos += 4 + blockSize;
		play->totalFrames++;
	}
	play->fileHandle = fileHandle;
	demoPlaySetIndex( play, 0 );
	play->clientNum = -1;
	for( i=0;i<MAX_CLIENTS;i++)
		if (play->frame->clientData[i]) {
			play->clientNum = i;
			break;
		}
	return play;
errorreturn:
	Z_Free( play );
	FS_FCloseFile( fileHandle );
	return 0;
}

static void demoPlayStop( demoPlay_t *play ) {
	FS_FCloseFile( play->fileHandle );
	Z_Free( play );
}

extern void CL_ConfigstringModified( Cmd::Args& csCmd );
qboolean demoGetServerCommand( int cmdNumber ) {
	demoPlay_t *play = demo.play.handle;
	int index = cmdNumber % DEMO_PLAY_CMDS;
	const char *cmd;

	if (cmdNumber < play->commandCount - DEMO_PLAY_CMDS || cmdNumber > play->commandCount )
		return qfalse;
	
	Cmd_TokenizeString( play->commandData + play->commandStart[index] );

	cmd = Cmd_Argv( 0 );
	if ( !strcmp( cmd, "cs" ) ) {
		Cmd::Args args(play->commandData + play->commandStart[index]);
		CL_ConfigstringModified(args);
	}
	return qtrue;
}

int demoSeek( int seekTime ) {
	return demoPlaySeek( demo.play.handle, seekTime );
}

void demoRenderFrame( void ) {
	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cls.realtime, 2 );	
}

void demoGetSnapshotNumber( int *snapNumber, int *serverTime ) {
	demoPlay_t *play = demo.play.handle;

	*snapNumber = play->frameNumber + (play->lastFrame ? 0 : 1);
	*serverTime = play->lastFrame ?  play->frame->serverTime : play->nextFrame->serverTime;
}

qboolean demoGetSnapshot( int snapNumber, snapshot_t *snap ) {
	demoPlay_t *play = demo.play.handle;
	demoFrame_t *frame;
	int i;

	if (snapNumber < play->frameNumber)
		return qfalse;
	if (snapNumber > play->frameNumber + 1)
		return qfalse;
	if (snapNumber == play->frameNumber + 1 && play->lastFrame)
		return qfalse;

	frame = snapNumber == play->frameNumber ? play->frame : play->nextFrame;

	demoPlaySynch( play, frame );
	snap->serverCommandSequence = play->commandCount;
	snap->serverTime = frame->serverTime;
	if (play->clientNum >=0 && play->clientNum < MAX_CLIENTS) {
		snap->ps = frame->clients[ play->clientNum ];
	} else
		Com_Memset( &snap->ps, 0, sizeof(snap->ps) );

	snap->numEntities = 0;
	for (i=0;i<MAX_GENTITIES-1 && snap->numEntities < MAX_ENTITIES_IN_SNAPSHOT;i++) {
		entityState_t *newEntity;
		if (frame->entities[i].number != i)
			continue;
		/* Skip your own entity if there ever comes server side recording */
		if (frame->entities[i].number == snap->ps.clientNum)
			continue;
		newEntity = &snap->entities[snap->numEntities++];
		*newEntity = frame->entities[i];
	}
	snap->snapFlags = 0;
	snap->ping = 0;
	snap->numServerCommands = 0;
	Com_Memcpy( snap->areamask, frame->areamask, frame->areaUsed );
	return qtrue;
}

void CL_DemoSetCGameTime( void ) {
	/* We never timeout */
	clc.lastPacketTime = cls.realtime;
}

void CL_DemoShutDown( void ) {
	if ( demo.play.handle ) {
		FS_FCloseFile( demo.play.handle->fileHandle );
		Z_Free( demo.play.handle );
		demo.play.handle = 0;
	}
}

void demoStop( void ) {
	if (demo.play.handle) {
		demoPlayStop( demo.play.handle );
	}
	Com_Memset( &demo.play, 0, sizeof( demo.play ));
}

int demoInfo( mmeDemoInfo_t* info ) {
	demoPlay_t* play = demo.play.handle;
	if ( !play )
		return -1;

	info->clientNum = play->clientNum;
	if ( play->frame ) {
		info->currentTime = play->frame->serverTime;
	} else {
		info->currentTime = 0;
	}
	info->currentFrame = play->frameNumber;
	info->totalTime = play->endTime;
	info->totalFrames = play->totalFrames;
	return 0;
}

qboolean demoPlay( const char *fileName ) {
	demo.play.handle = demoPlayOpen( fileName );
	if (demo.play.handle) {
		const char *info, *mapname;
		demoPlay_t *play = demo.play.handle;
		clc.demoplaying = qtrue;
		clc.newDemoPlayer = qtrue;
		clc.serverMessageSequence = 0;
		clc.lastExecutedServerCommand = 0;
		Com_Printf("Opened %s, which has %d seconds and %d frames\n", fileName, (play->endTime - play->startTime) / 1000, play->totalFrames );
		Con_Close();
		
		// wipe local client state
		CL_ClearState();
		cls.state = CA_LOADING;
		// Pump the loop, this may change gamestate!
		Com_EventLoop();
		// starting to load a map so we get out of full screen ui mode
		Cvar_Set("r_uiFullScreen", "0");
		info = play->frame->string.data + play->frame->string.offsets[ CS_SERVERINFO ];
		mapname = Info_ValueForKey( info, "mapname" );
		FS_LoadPak( va( "map-%s", mapname ) );
		// flush client memory and start loading stuff
		// this will also (re)load the UI
		// if this is a local client then only the client part of the hunk
		// will be cleared, note that this is done after the hunk mark has been set
		CL_FlushMemory();
		// initialize the CGame
		cls.cgameStarted = qtrue;
		// Create the gamestate
		Com_Memcpy( cl.gameState.stringOffsets, play->frame->string.offsets, sizeof( play->frame->string.offsets ));
		Com_Memcpy( cl.gameState.stringData, play->frame->string.data, play->frame->string.used );
		cl.gameState.dataCount = play->frame->string.used;
		CL_InitCGame();
		cls.state = CA_ACTIVE;
		return qtrue;
	} else {
		return qfalse;
	}
}

void CL_MMEDemo_f( void ) {
	const char *cmd = Cmd_Argv( 1 );

	if (Cmd_Argc() == 2) {
		char mmeName[MAX_OSPATH];
		Com_sprintf (mmeName, MAX_OSPATH, "mmedemos/%s.mme", cmd);
		if (FS_FileExists( mmeName )) {
			demoPlay( mmeName );
		} else {
			Com_Printf("%s not found.\n", mmeName );
		}
		return;
	}
	if (!Q_stricmp( cmd, "convert")) {
		demoConvert( Cmd_Argv( 2 ), Cmd_Argv( 3 ), mme_demoSmoothen->integer );
	} else if (!Q_stricmp( cmd, "play")) {
		demoPlay( Cmd_Argv( 2 ) );
	} else {
		Com_Printf("That does not compute...%s\n", cmd );
	}
}

void CL_DemoList_f(void) {
	int len, i;
	char *buf;
	char word[MAX_OSPATH];
	int	index;
	qboolean readName;
	qboolean haveQuote;

	demoListCount = 0;
	demoListIndex = 0;
	haveQuote = qfalse;

	if (Cmd_Argc() < 2) {
		Com_Printf( "Usage demoList filename.\n");
		Com_Printf( "That file should have lines with demoname projectname." );
		Com_Printf( "These will be played after each other." );
	}
	if (!FS_FileExists( Cmd_Argv(1))) {
		Com_Printf( "Listfile %s doesn't exist\n", Cmd_Argv(1));
		return;
	}
	len = FS_ReadFile( Cmd_Argv(1), (void **)&buf);
	if (!buf) {
		Com_Printf("file %s couldn't be opened\n", Cmd_Argv(1));
		return;
	}
	i = 0;
	index = 0;
	readName = qtrue;
	while( i < len) {
		switch (buf[i]) {
		case '\r':
			break;
		case '"':
			if (!haveQuote) {
				haveQuote = qtrue;
				break;
			}
		case '\n':
		case ' ':
		case '\t':
			if (haveQuote && buf[i] != '"') {
				if (index < (sizeof(word)-1)) {
	              word[index++] = buf[i];  
				}
				break;
			}
			if (!index)
				break;
			haveQuote = qfalse;
			word[index++] = 0;
			if (readName) {
				Com_Memcpy( demoList[demoListCount].demoName, word, index );
				readName = qfalse;
			} else {
				if (demoListCount < DEMOLISTSIZE) {
					Com_Memcpy( demoList[demoListCount].projectName, word, index );
					demoListCount++;
				}
				readName = qtrue;
			}
			index = 0;
			break;
		default:
			if (index < (sizeof(word)-1)) {
              word[index++] = buf[i];  
			}
			break;
		}
		i++;
	}
	/* Handle a final line if any */
	if (!readName && index && demoListCount < DEMOLISTSIZE) {
		word[index++] = 0;
		Com_Memcpy( demoList[demoListCount].projectName, word, index );
		demoListCount++;
	}

	FS_FreeFile ( buf );
	demoListIndex = 0;
}

void demoRecordServerFrame( fileHandle_t *fileHandle ) {



};

using namespace Cmd;
void CL_DemoListNext_f(void) {
	if ( demoListIndex < demoListCount ) {
		const demoListEntry_t *entry = &demoList[demoListIndex++];
		Cvar_Set( "mme_demoStartProject", entry->projectName );
		Com_Printf( "Starting demo %s with project %s\n",
			entry->demoName, entry->projectName );
		//entTODO: find out how to use new cmd exec to support demolist
//		Cmd::BufferCommandText( va( "demo \"%s\"\n", entry->demoName ));
//		Cmd_ExecuteString( va( "demo \"%s\"\n", entry->demoName ));
//		Cmd::ExecuteCommand( va( "demo \"%s\"\n", entry->demoName ));
	} else if (demoListCount) {
		Com_Printf( "DemoList:Finished playing %d demos\n", demoListCount );
		demoListCount = 0;
		demoListIndex = 0;
		//entTODO: find out how to use new cmd exec to support demolist
//		if ( mme_demoListQuit->integer )
//			Cbuf_ExecuteText( EXEC_APPEND, "quit" );
	}
}