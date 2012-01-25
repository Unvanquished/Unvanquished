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

#include "ui_local.h"
#include "ui_shared.h"

qboolean        bg_loadscreeninited = qfalse;
fontInfo_t      bg_loadscreenfont1;
fontInfo_t      bg_loadscreenfont2;

void            UI_LoadPanel_RenderHeaderText(panel_button_t * button);
void            UI_LoadPanel_RenderLoadingText(panel_button_t * button);
void            UI_LoadPanel_RenderPercentageMeter(panel_button_t * button);

// panel_button_text_t FONTNAME = { SCALEX, SCALEY, COLOUR, STYLE, FONT };

panel_button_text_t missiondescriptionTxt = {
	0.2f, 0.2f,
	{0.0f, 0.0f, 0.0f, 1.f},
	0, 0,
	&bg_loadscreenfont2,
};

panel_button_text_t missiondescriptionHeaderTxt = {
	0.2f, 0.2f,
	{0.0f, 0.0f, 0.0f, 0.8f},
	0, ITEM_ALIGN_CENTER,
	&bg_loadscreenfont2,
};

panel_button_text_t campaignpheaderTxt = {
	0.2f, 0.2f,
	{1.0f, 1.0f, 1.0f, 0.6f},
	0, 0,
	&bg_loadscreenfont2,
};

panel_button_text_t campaignpTxt = {
	0.35f, 0.35f,
	{1.0f, 1.0f, 1.0f, 0.6f},
	0, 0,
	&bg_loadscreenfont2,
};

panel_button_t  loadScreenMap = {
	"gfx/loading/camp_map",
	NULL,
	{0, 0, 440, 480},			// shouldn't this be square??
	{0, 0, 0, 0, 0, 0, 0, 0},
	NULL,						/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

panel_button_t  loadScreenBack = {
	"gfx/loading/camp_side",
	NULL,
	{440, 0, 200, 480},
	{0, 0, 0, 0, 0, 0, 0, 0},
	NULL,						/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

/*panel_button_t loadingPanelHeaderText = {
	NULL,
	"***TOP SECRET***",
	{ 440, 72, 200, 32 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	&missiondescriptionHeaderTxt,
	NULL,
	NULL,
	BG_PanelButtonsRender_Text,
	NULL,
};*/

panel_button_t  loadingPanelText = {
	NULL,
	NULL,
	{460, 72, 160, 244},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&missiondescriptionTxt,		/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	UI_LoadPanel_RenderLoadingText,
	NULL,
};

/*panel_button_t campaignheaderPanelText = {
	NULL,
	"Connecting to:",
	{ 456, 24, 152, 232 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	&campaignpheaderTxt,
	NULL,
	NULL,
	BG_PanelButtonsRender_Text,
	NULL,
};
*/
panel_button_t  campaignPanelText = {
	NULL,
	NULL,						//"CONNECTING...",
	{470, 33, 152, 232},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&campaignpTxt,				/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	UI_LoadPanel_RenderHeaderText,
	NULL,
};

panel_button_t *loadpanelButtons[] = {
	&loadScreenMap, &loadScreenBack,


	&loadingPanelText,			/*&loadingPanelHeaderText, */

	/*&campaignheaderPanelText, */ &campaignPanelText,

	NULL,
};

/*
================
CG_DrawConnectScreen
================
*/
static qboolean connect_ownerdraw;
void UI_DrawLoadPanel(qboolean forcerefresh, qboolean ownerdraw, qboolean uihack)
{
	static qboolean inside = qfalse;

	if(inside)
	{
		if(!uihack && trap_Cvar_VariableValue("ui_connecting"))
		{
			trap_Cvar_Set("ui_connecting", "0");
		}
		return;
	}

	connect_ownerdraw = ownerdraw;

	inside = qtrue;

	if(!bg_loadscreeninited)
	{
		trap_R_RegisterFont("ariblk.ttf", 27, &bg_loadscreenfont1);
		trap_R_RegisterFont("courbd.ttf", 30, &bg_loadscreenfont2);

		BG_PanelButtonsSetup(loadpanelButtons);

		bg_loadscreeninited = qtrue;
	}

	BG_PanelButtonsRender(loadpanelButtons);

	if(forcerefresh)
	{
		//trap_UpdateScreen();
	}

	if(!uihack && trap_Cvar_VariableValue("ui_connecting"))
	{
		trap_Cvar_Set("ui_connecting", "0");
	}

	inside = qfalse;
}

#define STARTANGLE 40
void UI_LoadPanel_RenderPercentageMeter(panel_button_t * button)
{
	float           hunkfrac;
	float           w, h;
	vec2_t          org;
	polyVert_t      verts[4];

	org[0] = button->rect.x;
	org[1] = button->rect.y;
	w = button->rect.w;
	h = button->rect.h;

	hunkfrac = 0.f;
	AdjustFrom640(&org[0], &org[1], &w, &h);
	SetupRotatedThing(verts, org, w, h, DEG2RAD((180 - STARTANGLE) - ((180 - (2 * STARTANGLE)) * hunkfrac)));

	trap_R_Add2dPolys(verts, 4, button->hShaderNormal);
}

/*
void UI_LoadPanel_RenderCampaignNameText( panel_button_t* button ) {
	uiClientState_t	cstate;
	char *s;

	trap_GetClientState( &cstate );

	s = Q_strupr( cstate.servername );

	Text_Paint_Ext( button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 14, 0, button->font->font );
}*/

void MiniAngleToAxis(vec_t angle, vec2_t axes[2])
{
	axes[0][0] = (vec_t) sin(-angle);
	axes[0][1] = -(vec_t) cos(-angle);

	axes[1][0] = -axes[0][1];
	axes[1][1] = axes[0][0];
}

void SetupRotatedThing(polyVert_t * verts, vec2_t org, float w, float h, vec_t angle)
{
	vec2_t          axes[2];

	MiniAngleToAxis(angle, axes);

	verts[0].xyz[0] = org[0] - (w * 0.5f) * axes[0][0];
	verts[0].xyz[1] = org[1] - (w * 0.5f) * axes[0][1];
	verts[0].xyz[2] = 0;
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	verts[1].xyz[0] = verts[0].xyz[0] + w * axes[0][0];
	verts[1].xyz[1] = verts[0].xyz[1] + w * axes[0][1];
	verts[1].xyz[2] = 0;
	verts[1].st[0] = 1;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	verts[2].xyz[0] = verts[1].xyz[0] + h * axes[1][0];
	verts[2].xyz[1] = verts[1].xyz[1] + h * axes[1][1];
	verts[2].xyz[2] = 0;
	verts[2].st[0] = 1;
	verts[2].st[1] = 0;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	verts[3].xyz[0] = verts[2].xyz[0] - w * axes[0][0];
	verts[3].xyz[1] = verts[2].xyz[1] - w * axes[0][1];
	verts[3].xyz[2] = 0;
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;
}

void UI_LoadPanel_RenderHeaderText(panel_button_t * button)
{
	uiClientState_t cstate;
	char            downloadName[MAX_INFO_VALUE];

	trap_GetClientState(&cstate);

	trap_Cvar_VariableStringBuffer("cl_downloadName", downloadName, sizeof(downloadName));

	if((cstate.connState == CA_DISCONNECTED || cstate.connState == CA_CONNECTED) && *downloadName)
	{
		button->text = "DOWNLOADING...";
	}
	else
	{
		button->text = "CONNECTING...";
	}

	BG_PanelButtonsRender_Text(button);
}

#define ESTIMATES 80
const char     *UI_DownloadInfo(const char *downloadName)
{
	static char     dlText[] = "Downloading:";
	static char     etaText[] = "Estimated time left:";
	static char     xferText[] = "Transfer rate:";
	static int      tleEstimates[ESTIMATES] = { 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
		60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
		60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
		60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60
	};
	static int      tleIndex = 0;

	char            dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int             downloadSize, downloadCount, downloadTime;
	int             xferRate;
	const char     *s, *ds;

	downloadSize = trap_Cvar_VariableValue("cl_downloadSize");
	downloadCount = trap_Cvar_VariableValue("cl_downloadCount");
	downloadTime = trap_Cvar_VariableValue("cl_downloadTime");

	if(downloadSize > 0)
	{
		ds = va("%s (%d%%)", downloadName, (int)((float)downloadCount * 100.0f / (float)downloadSize));
	}
	else
	{
		ds = downloadName;
	}

	UI_ReadableSize(dlSizeBuf, sizeof dlSizeBuf, downloadCount);
	UI_ReadableSize(totalSizeBuf, sizeof totalSizeBuf, downloadSize);

	if(downloadCount < 4096 || !downloadTime)
	{
		s = va("%s\n %s\n%s\n\n%s\n estimating...\n\n%s\n\n%s copied", dlText, ds, totalSizeBuf, etaText, xferText, dlSizeBuf);
		return s;
	}
	else
	{
		if((uiInfo.uiDC.realTime - downloadTime) / 1000)
		{
			xferRate = downloadCount / ((uiInfo.uiDC.realTime - downloadTime) / 1000);
		}
		else
		{
			xferRate = 0;
		}
		UI_ReadableSize(xferRateBuf, sizeof xferRateBuf, xferRate);

		// Extrapolate estimated completion time
		if(downloadSize && xferRate)
		{
			int             n = downloadSize / xferRate;	// estimated time for entire d/l in secs
			int             timeleft = 0, i;

			// We do it in K (/1024) because we'd overflow around 4MB
			tleEstimates[tleIndex] = (n - (((downloadCount / 1024) * n) / (downloadSize / 1024)));
			tleIndex++;
			if(tleIndex >= ESTIMATES)
			{
				tleIndex = 0;
			}

			for(i = 0; i < ESTIMATES; i++)
				timeleft += tleEstimates[i];

			timeleft /= ESTIMATES;

			UI_PrintTime(dlTimeBuf, sizeof dlTimeBuf, timeleft);
		}
		else
		{
			dlTimeBuf[0] = '\0';
		}

		if(xferRate)
		{
			s = va("%s\n %s\n%s\n\n%s\n %s\n\n%s\n %s/sec\n\n%s copied", dlText, ds, totalSizeBuf,
				   etaText, dlTimeBuf, xferText, xferRateBuf, dlSizeBuf);
		}
		else
		{
			if(downloadSize)
			{
				s = va("%s\n %s\n%s\n\n%s\n estimating...\n\n%s\n\n%s copied", dlText, ds, totalSizeBuf,
					   etaText, xferText, dlSizeBuf);
			}
			else
			{
				s = va("%s\n %s\n\n%s\n estimating...\n\n%s\n\n%s copied", dlText, ds, etaText, xferText, dlSizeBuf);
			}
		}

		return s;
	}

	return "";
}

void UI_LoadPanel_RenderLoadingText(panel_button_t * button)
{
	uiClientState_t cstate;
	char            downloadName[MAX_INFO_VALUE];
	char            buff[2560];
	static connstate_t lastConnState;
	static char     lastLoadingText[MAX_INFO_VALUE];
	char           *p, *s = "";
	float           y;

	trap_GetClientState(&cstate);

	Com_sprintf(buff, sizeof(buff), "Connecting to:\n %s^*\n\n%s", cstate.servername,
				Info_ValueForKey(cstate.updateInfoString, "motd"));

	//Com_sprintf( buff, sizeof(buff), "%s^*", cstate.servername, Info_ValueForKey( cstate.updateInfoString, "motd" ) );

	trap_Cvar_VariableStringBuffer("cl_downloadName", downloadName, sizeof(downloadName));

	if(lastConnState > cstate.connState)
	{
		lastLoadingText[0] = '\0';
	}
	lastConnState = cstate.connState;

	if(!connect_ownerdraw)
	{
		if(!trap_Cvar_VariableValue("ui_connecting"))
		{
			switch (cstate.connState)
			{
				case CA_CONNECTING:
					s = va(trap_TranslateString("Awaiting connection...%i"), cstate.connectPacketCount);
					break;
				case CA_CHALLENGING:
					s = va(trap_TranslateString("Awaiting challenge...%i"), cstate.connectPacketCount);
					break;
				case CA_DISCONNECTED:
				case CA_CONNECTED:
					if(*downloadName || cstate.connState == CA_DISCONNECTED)
					{
						s = (char *)UI_DownloadInfo(downloadName);
					}
					else
					{
						s = trap_TranslateString("Awaiting gamestate...");
					}
					break;
				case CA_LOADING:
				case CA_PRIMED:
				default:
					break;
			}
		}
		else if(trap_Cvar_VariableValue("ui_dl_running"))
		{
			// only toggle during a disconnected download
			s = (char *)UI_DownloadInfo(downloadName);
		}

		Q_strcat(buff, sizeof(buff), va("\n\n%s^*", s));

		if(cstate.connState < CA_CONNECTED && *cstate.messageString)
		{
			Q_strcat(buff, sizeof(buff), va("\n\n%s^*", cstate.messageString));
		}
	}

	BG_FitTextToWidth_Ext(buff, button->font->scalex, button->rect.w, sizeof(buff), button->font->font);

	//UI_DrawRect( button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorRed );

	y = button->rect.y + 12;

	s = p = buff;

	while(*p)
	{
		if(*p == '\n')
		{
			*p++ = '\0';
			Text_Paint_Ext(button->rect.x + 4, y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 0, 0,
						   button->font->font);
			y += 8;
			s = p;
		}
		else
		{
			p++;
		}
	}
}
