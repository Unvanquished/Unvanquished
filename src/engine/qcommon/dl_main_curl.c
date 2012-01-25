/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
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

#include "../qcommon/q_shared.h"
#include "qcommon.h"
#include "dl_public.h"

#define APP_NAME        "ID_DOWNLOAD"
#define APP_VERSION     "2.0"

// initialize once
static int      dl_initialized = 0;

static CURLM   *dl_multi = NULL;
static CURL    *dl_request = NULL;
static FILE    *dl_file = NULL;

/*
** Write to file
*/
static size_t DL_cb_FWriteFile(void *ptr, size_t size, size_t nmemb, void *stream)
{
	FILE           *file = (FILE *) stream;

	return fwrite(ptr, size, nmemb, file);
}

/*
** Print progress
*/
static int DL_cb_Progress(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	/* cl_downloadSize and cl_downloadTime are set by the Q3 protocol...
	   and it would probably be expensive to verify them here.   -zinx */

	Cvar_SetValue("cl_downloadCount", (float)dlnow);
	return 0;
}

void DL_InitDownload()
{
	if(dl_initialized)
	{
		return;
	}

	/* Make sure curl has initialized, so the cleanup doesn't get confused */
	curl_global_init(CURL_GLOBAL_ALL);

	dl_multi = curl_multi_init();

	Com_Printf("Client download subsystem initialized\n");
	dl_initialized = 1;
}

/*
================
DL_Shutdown

================
*/
void DL_Shutdown()
{
	if(!dl_initialized)
	{
		return;
	}

	curl_multi_cleanup(dl_multi);
	dl_multi = NULL;

	curl_global_cleanup();

	dl_initialized = 0;
}

/*
===============
inspired from http://www.w3.org/Library/Examples/LoadToFile.c
setup the download, return once we have a connection
===============
*/
int DL_BeginDownload(const char *localName, const char *remoteName, int debug)
{
	char            referer[MAX_STRING_CHARS + 5 /*"ET://" */ ];

	if(dl_request)
	{
		Com_Printf("ERROR: DL_BeginDownload called with a download request already active\n");
		return 0;
	}

	if(!localName || !remoteName)
	{
		Com_DPrintf("Empty download URL or empty local file name\n");
		return 0;
	}

	FS_CreatePath(localName);
	dl_file = fopen(localName, "wb+");
	if(!dl_file)
	{
		Com_Printf("ERROR: DL_BeginDownload unable to open '%s' for writing\n", localName);
		return 0;
	}

	DL_InitDownload();

	/* ET://ip:port */
	strcpy(referer, "ET://");
	Q_strncpyz(referer + 5, Cvar_VariableString("cl_currentServerIP"), MAX_STRING_CHARS);

	dl_request = curl_easy_init();
	curl_easy_setopt(dl_request, CURLOPT_USERAGENT, va("%s %s", APP_NAME "/" APP_VERSION, curl_version()));
	curl_easy_setopt(dl_request, CURLOPT_REFERER, referer);
	curl_easy_setopt(dl_request, CURLOPT_URL, remoteName);
	curl_easy_setopt(dl_request, CURLOPT_WRITEFUNCTION, DL_cb_FWriteFile);
	curl_easy_setopt(dl_request, CURLOPT_WRITEDATA, (void *)dl_file);
	curl_easy_setopt(dl_request, CURLOPT_PROGRESSFUNCTION, DL_cb_Progress);
	curl_easy_setopt(dl_request, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(dl_request, CURLOPT_FAILONERROR, 1);

	curl_multi_add_handle(dl_multi, dl_request);

	Cvar_Set("cl_downloadName", remoteName);

	return 1;
}

// (maybe this should be CL_DL_DownloadLoop)
dlStatus_t DL_DownloadLoop()
{
	CURLMcode       status;
	CURLMsg        *msg;
	int             dls = 0;
	const char     *err = NULL;

	if(!dl_request)
	{
		Com_DPrintf("DL_DownloadLoop: unexpected call with dl_request == NULL\n");
		return DL_DONE;
	}

	if((status = curl_multi_perform(dl_multi, &dls)) == CURLM_CALL_MULTI_PERFORM && dls)
	{
		return DL_CONTINUE;
	}

	while((msg = curl_multi_info_read(dl_multi, &dls)) && msg->easy_handle != dl_request)
		;

	if(!msg || msg->msg != CURLMSG_DONE)
	{
		return DL_CONTINUE;
	}

	if(msg->data.result != CURLE_OK)
	{
#ifdef __MACOS__				// ���
		err = "unknown curl error.";
#else
		err = curl_easy_strerror(msg->data.result);
#endif
	}
	else
	{
		err = NULL;
	}

	curl_multi_remove_handle(dl_multi, dl_request);
	curl_easy_cleanup(dl_request);

	fclose(dl_file);
	dl_file = NULL;

	dl_request = NULL;

	Cvar_Set("ui_dl_running", "0");

	if(err)
	{
		Com_DPrintf("DL_DownloadLoop: request terminated with failure status '%s'\n", err);
		return DL_FAILED;
	}

	return DL_DONE;
}
