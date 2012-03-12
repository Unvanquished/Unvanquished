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

/******************************************************************************
***** teh firetams! (sic)
****/

#include "cg_local.h"

/******************************************************************************
***** Defines, constants, etc
****/

static int      sortedFireTeamClients[MAX_CLIENTS];

/******************************************************************************
***** Support Routines
****/

int QDECL CG_SortFireTeam(const void *a, const void *b)
{
	clientInfo_t   *ca, *cb;
	int             cna, cnb;

	cna = *(int *)a;
	cnb = *(int *)b;

	ca = &cgs.clientinfo[cna];
	cb = &cgs.clientinfo[cnb];

	// Not on our team, so shove back
	if(!CG_IsOnSameFireteam(cnb, cg.clientNum))
	{
		return -1;
	}
	if(!CG_IsOnSameFireteam(cna, cg.clientNum))
	{
		return 1;
	}

	// Leader comes first
	if(CG_IsFireTeamLeader(cna))
	{
		return -1;
	}
	if(CG_IsFireTeamLeader(cnb))
	{
		return 1;
	}

	// Then higher ranks
	if(ca->rank > cb->rank)
	{
		return -1;
	}
	if(cb->rank > ca->rank)
	{
		return 1;
	}

	// Then score
/*	if ( ca->score > cb->score ) {
		return -1;
	}
	if ( cb->score > ca->score ) {
		return 1;
											}*/// not atm

	return 0;
}

// Sorts client's fireteam by leader then rank
void CG_SortClientFireteam()
{
	int             i;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		sortedFireTeamClients[i] = i;
	}

	qsort(sortedFireTeamClients, MAX_CLIENTS, sizeof(sortedFireTeamClients[0]), CG_SortFireTeam);

/*	for(i = 0; i < MAX_CLIENTS; i++) {
		CG_Printf( "%i ", sortedFireTeamClients[i] );
	}

	CG_Printf( "\n" );*/
}

// Parses fireteam servercommand
void CG_ParseFireteams()
{
	int             i, j;
	char           *s;
	const char     *p;
	int             clnts[2];

	qboolean        onFireteam2;
	qboolean        isLeader2;

//  qboolean onFireteam =   CG_IsOnFireteam( cg.clientNum ) ? qtrue : qfalse;
//  qboolean isLeader =     CG_IsFireTeamLeader( cg.clientNum ) ? qtrue : qfalse;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		cgs.clientinfo[i].fireteamData = NULL;
	}

	for(i = 0; i < MAX_FIRETEAMS; i++)
	{
		char            hexbuffer[11] = "0x00000000";

		p = CG_ConfigString(CS_FIRETEAMS + i);

/*		s = Info_ValueForKey(p, "n");
		if(!s || !*s) {
			cg.fireTeams[i].inuse = qfalse;
			continue;
		} else {
			cg.fireTeams[i].inuse = qtrue;
		}*/

//      Q_strncpyz(cg.fireTeams[i].name, s, 32);
//      CG_Printf("Fireteam: %s\n", cg.fireTeams[i].name);

		j = atoi(Info_ValueForKey(p, "id"));
		if(j == -1)
		{
			cg.fireTeams[i].inuse = qfalse;
			continue;
		}
		else
		{
			cg.fireTeams[i].inuse = qtrue;
			cg.fireTeams[i].ident = j;
		}

		s = Info_ValueForKey(p, "l");
		cg.fireTeams[i].leader = atoi(s);

		s = Info_ValueForKey(p, "c");
		Q_strncpyz(hexbuffer + 2, s, 9);
		sscanf(hexbuffer, "%x", &clnts[1]);
		Q_strncpyz(hexbuffer + 2, s + 8, 9);
		sscanf(hexbuffer, "%x", &clnts[0]);

		for(j = 0; j < MAX_CLIENTS; j++)
		{
			if(COM_BitCheck(clnts, j))
			{
				cg.fireTeams[i].joinOrder[j] = qtrue;
				cgs.clientinfo[j].fireteamData = &cg.fireTeams[i];
//              CG_Printf("%s\n", cgs.clientinfo[j].name);
			}
			else
			{
				cg.fireTeams[i].joinOrder[j] = qfalse;
			}
		}
	}

	CG_SortClientFireteam();

	onFireteam2 = CG_IsOnFireteam(cg.clientNum) ? qtrue : qfalse;
	isLeader2 = CG_IsFireTeamLeader(cg.clientNum) ? qtrue : qfalse;
}

// Fireteam that both specified clients are on, if they both are on the same team
fireteamData_t *CG_IsOnSameFireteam(int clientNum, int clientNum2)
{
	if(CG_IsOnFireteam(clientNum) == CG_IsOnFireteam(clientNum2))
	{
		return CG_IsOnFireteam(clientNum);
	}

	return NULL;
}

// Fireteam that specified client is leader of, or NULL if none
fireteamData_t *CG_IsFireTeamLeader(int clientNum)
{
	fireteamData_t *f;

	if(!(f = CG_IsOnFireteam(clientNum)))
	{
		return NULL;
	}

	if(f->leader != clientNum)
	{
		return NULL;
	}

	return f;
}

// Client, not on a fireteam, not sorted, but on your team
clientInfo_t   *CG_ClientInfoForPosition(int pos, int max)
{
	int             i, cnt = 0;

	for(i = 0; i < MAX_CLIENTS && cnt < max; i++)
	{
		if(cg.clientNum != i && cgs.clientinfo[i].infoValid && !CG_IsOnFireteam(i) &&
		   cgs.clientinfo[cg.clientNum].team == cgs.clientinfo[i].team)
		{
			if(cnt == pos)
			{
				return &cgs.clientinfo[i];
			}
			cnt++;
		}
	}

	return NULL;
}

// Fireteam, that's on your same team
fireteamData_t *CG_FireTeamForPosition(int pos, int max)
{
	int             i, cnt = 0;

	for(i = 0; i < MAX_FIRETEAMS && cnt < max; i++)
	{
		if(cg.fireTeams[i].inuse && cgs.clientinfo[cg.fireTeams[i].leader].team == cgs.clientinfo[cg.clientNum].team)
		{
			if(cnt == pos)
			{
				return &cg.fireTeams[i];
			}
			cnt++;
		}
	}

	return NULL;
}

// Client, not sorted by rank, on CLIENT'S fireteam
clientInfo_t   *CG_FireTeamPlayerForPosition(int pos, int max)
{
	int             i, cnt = 0;
	fireteamData_t *f = CG_IsOnFireteam(cg.clientNum);

	if(!f)
	{
		return NULL;
	}

	for(i = 0; i < MAX_CLIENTS && cnt < max; i++)
	{
		if(cgs.clientinfo[i].infoValid && cgs.clientinfo[cg.clientNum].team == cgs.clientinfo[i].team)
		{
			if(!(f == CG_IsOnFireteam(i)))
			{
				continue;
			}

			if(cnt == pos)
			{
				return &cgs.clientinfo[i];
			}
			cnt++;
		}
	}

	return NULL;
}

// Client, sorted by rank, on CLIENT'S fireteam
clientInfo_t   *CG_SortedFireTeamPlayerForPosition(int pos, int max)
{
	int             i, cnt = 0;
	fireteamData_t *f = CG_IsOnFireteam(cg.clientNum);

	if(!f)
	{
		return NULL;
	}

	for(i = 0; i < MAX_CLIENTS && cnt < max; i++)
	{
		if(!(f == CG_IsOnFireteam(sortedFireTeamClients[i])))
		{
			return NULL;
		}

		if(cnt == pos)
		{
			return &cgs.clientinfo[sortedFireTeamClients[i]];
		}
		cnt++;
	}

	return NULL;
}

/******************************************************************************
***** Main Functions
****/

#define FT_BAR_YSPACING 2.f
#define FT_BAR_HEIGHT 10.f
void CG_DrawFireTeamOverlay(rectDef_t * rect)
{
	int             x = rect->x;
	int             y = rect->y + 1;	// +1, jitter it into place in 1024 :)
	float           h;
	clientInfo_t   *ci = NULL;
	char            buffer[64];
	fireteamData_t *f = NULL;
	int             i;
	vec4_t          clr1 = { .16f, .2f, .17f, .8f };
	vec4_t          clr2 = { 0.f, 0.f, 0.f, .2f };
	vec4_t          clr3 = { 0.25f, 0.f, 0.f, 153 / 255.f };
	vec4_t          tclr = { 0.6f, 0.6f, 0.6f, 1.0f };
	vec4_t          bgColor = { 0.0f, 0.0f, 0.0f, 0.6f };	// window
	vec4_t          borderColor = { 0.5f, 0.5f, 0.5f, 0.5f };	// window

	if(!(f = CG_IsOnFireteam(cg.clientNum)))
	{
		return;
	}

	h = 12 + 2 + 2;
	for(i = 0; i < 6; i++)
	{
		ci = CG_SortedFireTeamPlayerForPosition(i, 6);
		if(!ci)
		{
			break;;
		}

		h += FT_BAR_HEIGHT + FT_BAR_YSPACING;
	}

	CG_DrawRect(x, y, 204, h, 1, borderColor);
	CG_FillRect(x + 1, y + 1, 204 - 2, h - 2, bgColor);

	x += 2;
	y += 2;

	CG_FillRect(x, y, 204 - 4, 12, clr1);

	sprintf(buffer, "Fireteam: %s", bg_fireteamNames[f->ident]);
	Q_strupr(buffer);
	CG_Text_Paint_Ext(x + 3, y + FT_BAR_HEIGHT, .19f, .19f, tclr, buffer, 0, 0, 0, &cgs.media.limboFont1);

	x += 2;
	//y += 2;

	for(i = 0; i < 6; i++)
	{
		y += FT_BAR_HEIGHT + FT_BAR_YSPACING;
		x = rect->x + 2;

		ci = CG_SortedFireTeamPlayerForPosition(i, 6);
		if(!ci)
		{
			break;;
		}

		if(ci->selected)
		{
			CG_FillRect(x, y + FT_BAR_YSPACING, 204 - 4, FT_BAR_HEIGHT, clr3);
		}
		else
		{
			CG_FillRect(x, y + FT_BAR_YSPACING, 204 - 4, FT_BAR_HEIGHT, clr2);
		}

		x += 4;

		CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, tclr, BG_ClassLetterForNumber(ci->cls), 0, 0, ITEM_TEXTSTYLE_SHADOWED,
						  &cgs.media.limboFont2);
		x += 10;

		CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, tclr,
						  ci->team == TEAM_AXIS ? miniRankNames_Axis[ci->rank] : miniRankNames_Allies[ci->rank], 0, 0,
						  ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		x += 22;

		CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, tclr, ci->name, 0, 17, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		x += 90;

/*		CG_DrawPic(x + 2, y + 2, FT_BAR_HEIGHT - 4, FT_BAR_HEIGHT - 4, cgs.media.movementAutonomyIcons[0]);
		x += FT_BAR_HEIGHT;

		CG_DrawPic(x + 2, y + 2, FT_BAR_HEIGHT - 4, FT_BAR_HEIGHT - 4, cgs.media.weaponAutonomyIcons[0]);
		x += FT_BAR_HEIGHT;
		x += 4;*/

/*		if( isLeader ) {
			CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, tclr, va("%i", i+4), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2 );
		}*/
		x += 20;

		if(ci->health > 80)
		{
			CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, tclr, va("%i", ci->health < 0 ? 0 : ci->health), 0, 0,
							  ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		}
		else if(ci->health > 0)
		{
			CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, colorYellow, va("%i", ci->health < 0 ? 0 : ci->health), 0, 0,
							  ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		}
		else
		{
			CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, colorRed, va("%i", ci->health < 0 ? 0 : ci->health), 0, 0,
							  ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		}
		//x += 20;

		{
			vec2_t          loc;
			char           *s;

			loc[0] = ci->location[0];
			loc[1] = ci->location[1];

			s = va("^3(%s)", BG_GetLocationString(loc));

			x = rect->x + (204 - 4 - CG_Text_Width_Ext(s, .2f, 0, &cgs.media.limboFont2));

			CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, tclr, va("^3(%s)", BG_GetLocationString(loc)), 0, 0,
							  ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		}
	}
}

qboolean CG_FireteamGetBoxNeedsButtons()
{
	if(cgs.applicationEndTime > cg.time)
	{
		if(cgs.applicationClient < 0)
		{
			return qfalse;
		}
		return qtrue;
	}

	if(cgs.invitationEndTime > cg.time)
	{
		if(cgs.invitationClient < 0)
		{
			return qfalse;
		}
		return qtrue;
	}

	if(cgs.propositionEndTime > cg.time)
	{
		if(cgs.propositionClient < 0)
		{
			return qfalse;
		}
		return qtrue;
	}

	return qfalse;
}

const char     *CG_FireteamGetBoxText()
{
	if(cgs.applicationEndTime > cg.time)
	{
		if(cgs.applicationClient == -1)
		{
			return "Sent";
		}

		if(cgs.applicationClient == -2)
		{
			return "Failed";
		}

		if(cgs.applicationClient == -3)
		{
			return "Accepted";
		}

		if(cgs.applicationClient == -4)
		{
			return "Sent";
		}

		if(cgs.applicationClient < 0)
		{
			return NULL;
		}

		return va("Accept application from %s?", cgs.clientinfo[cgs.applicationClient].name);
	}

	if(cgs.invitationEndTime > cg.time)
	{
		if(cgs.invitationClient == -1)
		{
			return "Sent";
		}

		if(cgs.invitationClient == -2)
		{
			return "Failed";
		}

		if(cgs.invitationClient == -3)
		{
			return "Accepted";
		}

		if(cgs.invitationClient == -4)
		{
			return "Sent";
		}

		if(cgs.invitationClient < 0)
		{
			return NULL;
		}

		return va("Accept invitiation from %s?", cgs.clientinfo[cgs.invitationClient].name);
	}

	if(cgs.propositionEndTime > cg.time)
	{
		if(cgs.propositionClient == -1)
		{
			return "Sent";
		}

		if(cgs.propositionClient == -2)
		{
			return "Failed";
		}

		if(cgs.propositionClient == -3)
		{
			return "Accepted";
		}

		if(cgs.propositionClient == -4)
		{
			return "Sent";
		}

		if(cgs.propositionClient < 0)
		{
			return NULL;
		}

		return va("Accept %s's proposition to invite %s to join your fireteam?", cgs.clientinfo[cgs.propositionClient2].name,
				  cgs.clientinfo[cgs.propositionClient].name);
	}

	return NULL;
}

qboolean CG_FireteamHasClass(int classnum, qboolean selectedonly)
{
	fireteamData_t *ft;
	int             i;

	if(!(ft = CG_IsOnFireteam(cg.clientNum)))
	{
		return qfalse;
	}

	for(i = 0; i < MAX_CLIENTS; i++)
	{
/*		if( i == cgs.clientinfo ) {
			continue;
		}*/

		if(!cgs.clientinfo[i].infoValid)
		{
			continue;
		}

		if(ft != CG_IsOnFireteam(i))
		{
			continue;
		}

		if(cgs.clientinfo[i].cls != classnum)
		{
			continue;
		}

		if(selectedonly && !cgs.clientinfo[i].selected)
		{
			continue;
		}

		return qtrue;
	}

	return qfalse;
}

const char     *CG_BuildSelectedFirteamString(void)
{
	char            buffer[256];
	clientInfo_t   *ci;
	int             cnt = 0;
	int             i;

	*buffer = '\0';
	for(i = 0; i < 6; i++)
	{
		ci = CG_SortedFireTeamPlayerForPosition(i, 6);
		if(!ci)
		{
			break;
		}

		if(!ci->selected)
		{
			continue;
		}

		cnt++;

		Q_strcat(buffer, sizeof(buffer), va("%i ", ci->clientNum));
	}

	if(cnt == 0)
	{
		return "0";
	}

	if(!cgs.clientinfo[cg.clientNum].selected)
	{
		Q_strcat(buffer, sizeof(buffer), va("%i ", cg.clientNum));
		cnt++;
	}

	return va("%i %s", cnt, buffer);
}
