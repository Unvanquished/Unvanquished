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

/*
TTimo - 12/13/2002
libwww Bindings
indent -kr -ut -ts2 -i2 <file>
*/

#include "WWWLib.h" /* Global Library Include file */
#include "WWWMIME.h" /* MIME parser/generator */
#include "WWWNews.h" /* News access module */
#include "WWWHTTP.h" /* HTTP access module */
#include "WWWFTP.h"
#include "WWWFile.h"
#include "WWWGophe.h"
#include "WWWInit.h"

#include "HTAABrow.h"
#include "HTReqMan.h"

#include "dl_public.h"
#include "dl_local.h"

// initialize once
// NOTE: anything planned for shutdown? an HTLibTerminate call?
static int dl_initialized = 0;

// safe guard to make sure we have only one download going at a time
// libwww would allow multiple requests, but our glue code kinda doesn't atm
static int dl_running = 0;

// track special case behaviour for ftp downloads
// show_bug.cgi?id=605
static int       dl_is_ftp = 0;

static int       terminate_status = HT_UNDEF;

static HTRequest *dl_request = NULL;

/*
**  We get called here from the event loop when we are done
**  loading. Here we terminate the program as we have nothing
**  better to do.
*/
int terminate_handler( HTRequest *request, HTResponse *response, void *param, int status )
{
	// (LoadToFile.c had HTRequest_delete and HTProfile_delete calls here)

	// the status param tells us about the download .. > 0 success <= 0 failure
	terminate_status = status;

	HTEventList_stopLoop();

	return HT_OK;
}

/*
** Print progress
*/
BOOL HTAlertCallback_progress( HTRequest *request, HTAlertOpcode op,
                               int msgnum, const char *dfault, void *input, HTAlertPar *reply )
{
	if ( op == HT_PROG_READ )
	{
		if ( !dl_is_ftp )
		{
			Cvar_SetValue( "cl_downloadCount", ( float ) HTRequest_bytesRead( request ) );
		}
		else
		{
			/* show_bug.cgi?id=605 */
			if ( !HTNet_rawBytesCount( request->net ) )
			{
				Com_DPrintf( "Force raw byte count on request->net %p\n", request->net );
				HTFTP_setRawBytesCount( request );
			}

			Cvar_SetValue( "cl_downloadCount", ( float ) HTFTP_getDNetRawBytesCount( request ) );
		}
	}

	return YES;
}

/*
** answer questions
** NOTE: tried to get those message from HTDialog, but that didn't work out?
** i.e. HTDialog_errorMessage
*/
BOOL HTAlertCallback_confirm( HTRequest *request, HTAlertOpcode op,
                              int msgnum, const char *dfault, void *input, HTAlertPar *reply )
{
	// some predefined messages we know the answer to
	if ( msgnum == HT_MSG_FILE_REPLACE )
	{
		Com_Printf(_( "Replace existing download target file\n" ));
		return YES;
	}

	// anything else, means we abort
	Com_Printf(_( "Aborting, unknown libwww confirm message id: %d\n"), msgnum );
	HTEventList_stopLoop();
	return NO;
}

/*
** get prompted - various situations:
**    HT_A_PROMPT   = 0x4<<16, * Want full dialog *
**    HT_A_SECRET   = 0x8<<16, * Secret dialog (e.g. password) *
**    HT_A_USER_PW  = 0x10<<16 * Atomic userid and password *
*/
BOOL HTAlertCallback_prompt( HTRequest *request, HTAlertOpcode op,
                             int msgnum, const char *dfault, void *input, HTAlertPar *reply )
{
	Com_Printf(_( "Aborting, libwww prompt message id: %d (prompted for a login/password?)\n"), msgnum );
	HTEventList_stopLoop();
	return NO;
}

void DL_InitDownload( void )
{
	if ( dl_initialized )
	{
		return;
	}

	/* Initiate W3C Reference Library with a client profile */
	HTProfile_newNoCacheClient( PRODUCT_NAME, PRODUCT_VERSION );

	// if you leave the default (interactive)
	// then prompts can happen:
	// This file already exists - replace existing file? (y/n) Can't open output file
	// and cause a www download to fail
	HTAlertInit();
	HTAlert_setInteractive( YES );

	/* And the traces... */
#ifndef NDEBUG
	// see HTHome.c, you can specify the flags - empty string gets you all traces
	HTSetTraceMessageMask( "" );
#endif

	/* Need our own trace and print functions */
	HTPrint_setCallback( Com_VPrintf );
	HTTrace_setCallback( Com_VPrintf );

	/* Add our own filter to terminate the application */
	HTNet_addAfter( terminate_handler, NULL, NULL, HT_ALL, HT_FILTER_LAST );

	HTAlert_add( HTAlertCallback_progress, HT_A_PROGRESS );
	HTAlert_add( HTAlertCallback_confirm, HT_A_CONFIRM );
	HTAlert_add( HTAlertCallback_prompt, HT_A_PROMPT | HT_A_SECRET | HT_A_USER_PW );

	Com_DPrintf(_( "Client download subsystem initialized\n" ));
	dl_initialized = 1;
}

/*
================
DL_Shutdown

================
*/
void DL_Shutdown( void )
{
	if ( !dl_initialized )
	{
		return;
	}

	/*
	   this is unstable - since it's at shutdown, better leak
	   show_bug.cgi?id=611
	 */
#ifndef NDEBUG
	HTLibTerminate();
#endif

	dl_initialized = 0;
}

/*
===============
inspired from http://www.w3.org/Library/Examples/LoadToFile.c
setup the download, return once we have a connection
===============
*/
int DL_BeginDownload( const char *localName, const char *remoteName, int debug )
{
	char *access = NULL;
	char *url = NULL;
	char *login = NULL;
	char *path = NULL;
	char *ptr = NULL;

	if ( dl_running )
	{
		Com_Printf(_( "ERROR: DL_BeginDownload called with a download request already active\n" ));
		return 0;
	}

	terminate_status = HT_UNDEF;

#ifdef HTDEBUG

	if ( debug )
	{
		WWWTRACE = SHOW_ALL_TRACE;
	}

#endif

	if ( !localName || !remoteName )
	{
		Com_DPrintf( "Empty download URL or empty local file name\n" );
		return 0;
	}

	DL_InitDownload();

	/* need access for ftp behaviour and HTTP Basic Auth */
	access = HTParse( remoteName, "", PARSE_ACCESS );

	/*
	   Set the timeout for how long we are going to wait for a response
	   This needs to be set and reset to 0 after dl each time
	   idcvs/2003-January/000449.html
	   http://lists.w3.org/Archives/Public/www-lib/2003AprJun/0033.html
	   In case of ftp download, we leave no timeout during connect phase cause of libwww bugs
	   show_bug.cgi?id=605
	 */
	if ( !Q_stricmp( access, "ftp" ) )
	{
		dl_is_ftp = 1;
		HTHost_setEventTimeout( -1 );
	}
	else
	{
		dl_is_ftp = 0;
		HTHost_setEventTimeout( 30000 );
	}

	dl_request = HTRequest_new();

	/* HTTP Basic Auth */
	if ( !Q_stricmp( access, "http" ) )
	{
		HTBasic *basic;

		login = HTParse( remoteName, "", PARSE_HOST );
		path = HTParse( remoteName, "", PARSE_PATH + PARSE_PUNCTUATION );
		ptr = strchr( login, '@' );

		if ( ptr )
		{
			/* Uid and/or passwd specified */
			char *passwd;

			*ptr = '\0';
			passwd = strchr( login, ':' );

			if ( passwd )
			{
				/* Passwd specified */
				*passwd++ = '\0';
				HTUnEscape( passwd );
			}

			HTUnEscape( login );
			/* proactively set the auth */
			basic = HTBasic_new();
			StrAllocCopy( basic->uid, login );
			StrAllocCopy( basic->pw, passwd );
			basic_credentials( dl_request, basic );
			HTBasic_delete( basic );
			/* correct the HTTP */
			url = HT_MALLOC( 7 + strlen( ptr + 1 ) + strlen( path ) + 1 );
			sprintf( url, "http://%s%s", ptr + 1, path );
			Com_DPrintf( "HTTP Basic Auth — %s %s %s\n", login, passwd, url );
			HT_FREE( login );
			HT_FREE( path );
		}
		else
		{
			StrAllocCopy( url, remoteName );
		}
	}
	else
	{
		StrAllocCopy( url, remoteName );
	}

	HT_FREE( access );

	FS_CreatePath( localName );

	/* Start the load */
	if ( HTLoadToFile( url, dl_request, localName ) != YES )
	{
		Com_DPrintf( "HTLoadToFile failed\n" );
		HT_FREE( url );
		HTProfile_delete();
		return 0;
	}

	HT_FREE( url );

	/* remove possible login/pass part for the ui */
	access = HTParse( remoteName, "", PARSE_ACCESS );
	login = HTParse( remoteName, "", PARSE_HOST );
	path = HTParse( remoteName, "", PARSE_PATH + PARSE_PUNCTUATION );
	ptr = strchr( login, '@' );

	if ( ptr )
	{
		/* Uid and/or passwd specified */
		Cvar_Set( "cl_downloadName", va( "%s://*:*%s%s", access, ptr, path ) );
	}
	else
	{
		Cvar_Set( "cl_downloadName", remoteName );
	}

	HT_FREE( path );
	HT_FREE( login );
	HT_FREE( access );

	if ( dl_is_ftp )
	{
		HTHost_setEventTimeout( 30000 );
	}

	/* Go into the event loop... */
	HTEventList_init( dl_request );

	dl_running = 1;

	return 1;
}

// (maybe this should be CL_DL_DownloadLoop)
dlStatus_t DL_DownloadLoop( void )
{
	if ( !dl_running )
	{
		Com_DPrintf( "DL_DownloadLoop: unexpected call with dl_running == qfalse\n" );
		return DL_DONE;
	}

	if ( HTEventList_pump() )
	{
		return DL_CONTINUE;
	}

	/*
	   reset the timeout so it doesn't trigger when no-one wants it
	   un-register the current timeout first so it doesn't explode in our hands
	 */
	HTEventList_unregisterAll();
	HTHost_setEventTimeout( -1 );

	/* NOTE: in some samples I've seen, this is in the terminate_handler */
	HTRequest_delete( dl_request );
	dl_request = NULL;

	dl_running = 0;
	Cvar_Set( "ui_dl_running", "0" );

	/* NOTE: there is HTEventList_status, but it says != HT_OK as soon as HTEventList_pump returns NO */
	if ( terminate_status < 0 )
	{
		Com_DPrintf( "DL_DownloadLoop: request terminated with failure status %d\n", terminate_status );
		return DL_FAILED;
	}

	return DL_DONE;
}
