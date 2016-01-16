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

/* Additional features that would be nice for this code:
        * Only display <gamepath>/<file>, i.e., etpro/etpro-3_0_1.pk3 in the UI.
        * Add server as referring URL
*/

#include <curl/curl.h>

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"

// initialize once
static int   dl_initialized = 0;

static CURLM *dl_multi = nullptr;
static CURL  *dl_request = nullptr;
static fileHandle_t dl_file;

/*
** Write to file
*/
static size_t DL_cb_FWriteFile( void *ptr, size_t size, size_t nmemb, void *stream )
{
	fileHandle_t file = ( fileHandle_t )( intptr_t ) stream;

	return FS_Write( ptr, size * nmemb, file );
}

/*
** Print progress
*/
static int DL_cb_Progress( void*, double, double dlnow, double, double )
{
	/* cl_downloadSize and cl_downloadTime are set by the Q3 protocol...
	   and it would probably be expensive to verify them here.   -zinx */

	Cvar_SetValue( "cl_downloadCount", ( float ) dlnow );
	return 0;
}

void DL_InitDownload()
{
	if ( dl_initialized )
	{
		return;
	}

	/* Make sure curl has initialized, so the cleanup doesn't get confused */
	curl_global_init( CURL_GLOBAL_ALL );

	dl_multi = curl_multi_init();

	Com_DPrintf( "Client download subsystem initialized\n" );
	dl_initialized = 1;
}

/*
================
DL_Shutdown

================
*/
void DL_Shutdown()
{
	if ( !dl_initialized )
	{
		return;
	}

	curl_multi_cleanup( dl_multi );
	dl_multi = nullptr;

	curl_global_cleanup();

	dl_initialized = 0;
}

/*
===============
inspired from http://www.w3.org/Library/Examples/LoadToFile.c
setup the download, return once we have a connection
===============
*/
int DL_BeginDownload( const char *localName, const char *remoteName )
{
	char referer[ MAX_STRING_CHARS + URI_SCHEME_LENGTH ];

	if ( dl_request )
	{
		curl_multi_remove_handle( dl_multi, dl_request );
		curl_easy_cleanup( dl_request );
		dl_request = nullptr;
	}

	if ( dl_file )
	{
		FS_FCloseFile( dl_file );
		dl_file = 0;
	}

	if ( !localName || !remoteName )
	{
		Com_DPrintf( "Empty download URL or empty local file name\n" );
		return 0;
	}

	dl_file = FS_SV_FOpenFileWrite( localName );

	if ( !dl_file )
	{
		Com_Printf( "ERROR: DL_BeginDownload unable to open '%s' for writing\n", localName );
		return 0;
	}

	DL_InitDownload();

	strcpy( referer, URI_SCHEME );
	Q_strncpyz( referer + URI_SCHEME_LENGTH, Cvar_VariableString( "cl_currentServerIP" ), MAX_STRING_CHARS );

	dl_request = curl_easy_init();
	curl_easy_setopt( dl_request, CURLOPT_USERAGENT, va( "%s %s", PRODUCT_NAME "/" PRODUCT_VERSION, curl_version() ) );
	curl_easy_setopt( dl_request, CURLOPT_REFERER, referer );
	curl_easy_setopt( dl_request, CURLOPT_URL, remoteName );
	curl_easy_setopt( dl_request, CURLOPT_WRITEFUNCTION, DL_cb_FWriteFile );
	curl_easy_setopt( dl_request, CURLOPT_WRITEDATA, ( void * )( intptr_t ) dl_file );
	curl_easy_setopt( dl_request, CURLOPT_PROGRESSFUNCTION, DL_cb_Progress );
	curl_easy_setopt( dl_request, CURLOPT_NOPROGRESS, 0 );
	curl_easy_setopt( dl_request, CURLOPT_FAILONERROR, 1 );

	curl_multi_add_handle( dl_multi, dl_request );

	Cvar_Set( "cl_downloadName", remoteName );

	return 1;
}

// (maybe this should be CL_DL_DownloadLoop)
dlStatus_t DL_DownloadLoop()
{
	CURLMcode  status;
	CURLMsg    *msg;
	int        dls = 0;
	const char *err = nullptr;

	if ( !dl_request )
	{
		Com_DPrintf( "DL_DownloadLoop: unexpected call with dl_request == NULL\n" );
		return dlStatus_t::DL_DONE;
	}

	if ( ( status = curl_multi_perform( dl_multi, &dls ) ) == CURLM_CALL_MULTI_PERFORM && dls )
	{
		return dlStatus_t::DL_CONTINUE;
	}

	while ( ( msg = curl_multi_info_read( dl_multi, &dls ) ) && msg->easy_handle != dl_request )
	{
		;
	}

	if ( !msg || msg->msg != CURLMSG_DONE )
	{
		return dlStatus_t::DL_CONTINUE;
	}

	if ( msg->data.result != CURLE_OK )
	{
#ifdef __MACOS__ // ���
		err = "unknown curl error.";
#else
		err = curl_easy_strerror( msg->data.result );
#endif
	}
	else
	{
		err = nullptr;
	}

	curl_multi_remove_handle( dl_multi, dl_request );
	curl_easy_cleanup( dl_request );

	FS_FCloseFile( dl_file );
	dl_file = 0;

	dl_request = nullptr;

	Cvar_Set( "ui_dl_running", "0" );

	if ( err )
	{
		Com_DPrintf( "DL_DownloadLoop: request terminated with failure status '%s'\n", err );
		return dlStatus_t::DL_FAILED;
	}

	return dlStatus_t::DL_DONE;
}
