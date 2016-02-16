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

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "server.h"

/*
=================
SV_Netchan_FreeQueue
=================
*/
void SV_Netchan_FreeQueue( client_t *client )
{
	netchan_buffer_t *netbuf, *next;

	for ( netbuf = client->netchan_start_queue; netbuf; netbuf = next )
	{
		next = netbuf->next;
		Z_Free( netbuf );
	}

	client->netchan_start_queue = nullptr;
	client->netchan_end_queue = client->netchan_start_queue;
}

/*
=================
SV_Netchan_TransmitNextFragment
=================
*/
void SV_Netchan_TransmitNextFragment( client_t *client )
{
	Netchan_TransmitNextFragment( &client->netchan );

	while ( !client->netchan.unsentFragments && client->netchan_start_queue )
	{
		// make sure the netchan queue has been properly initialized (you never know)
		//% if (!client->netchan_end_queue) {
		//%     Com_Error(ERR_DROP, "netchan queue is not properly initialized in SV_Netchan_TransmitNextFragment");
		//% }
		// the last fragment was transmitted, check whether we have queued messages
		netchan_buffer_t *netbuf = client->netchan_start_queue;

		// pop from queue
		client->netchan_start_queue = netbuf->next;

		if ( !client->netchan_start_queue )
		{
			client->netchan_end_queue = nullptr;
		}

		Netchan_Transmit( &client->netchan, netbuf->msg.cursize, netbuf->msg.data );

		Z_Free( netbuf );
	}
}

/*
===============
SV_WriteBinaryMessage
===============
*/
static void SV_WriteBinaryMessage( msg_t *msg, client_t *cl )
{
	if ( !cl->binaryMessageLength )
	{
		return;
	}

	MSG_Uncompressed( msg );

	if ( ( msg->cursize + cl->binaryMessageLength ) >= msg->maxsize )
	{
		cl->binaryMessageOverflowed = true;
		return;
	}

	MSG_WriteData( msg, cl->binaryMessage, cl->binaryMessageLength );
	cl->binaryMessageLength = 0;
	cl->binaryMessageOverflowed = false;
}

/*
===============
SV_Netchan_Transmit

TTimo
show_bug.cgi?id=462
if there are some unsent fragments (which may happen if the snapshots
and the gamestate are fragmenting, and collide on send for instance)
then buffer them and make sure they get sent in correct order
================
*/
void SV_Netchan_Transmit( client_t *client, msg_t *msg )
{
	//int length, const byte *data ) {
	MSG_WriteByte( msg, svc_EOF );
	SV_WriteBinaryMessage( msg, client );

	if ( client->netchan.unsentFragments )
	{
		netchan_buffer_t *netbuf;

		//Log::Debug("SV_Netchan_Transmit: there are unsent fragments remaining");
		netbuf = ( netchan_buffer_t * ) Z_Malloc( sizeof( netchan_buffer_t ) );

		MSG_Copy( &netbuf->msg, netbuf->msgBuffer, sizeof( netbuf->msgBuffer ), msg );

		// copy the command, since the command number used for encryption is
		// already compressed in the buffer, and receiving a new command would
		// otherwise lose the proper encryption key
		strcpy( netbuf->lastClientCommandString, client->lastClientCommandString );

		netbuf->next = nullptr;

		if ( !client->netchan_start_queue )
		{
			client->netchan_start_queue = netbuf;
		}
		else
		{
			client->netchan_end_queue->next = netbuf;
		}

		client->netchan_end_queue = netbuf;

		// emit the next fragment of the current message for now
		Netchan_TransmitNextFragment( &client->netchan );
	}
	else
	{
		Netchan_Transmit( &client->netchan, msg->cursize, msg->data );
	}
}

