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

#include "cg_local.h"

panel_button_text_t fireteamTitleFont =
{
	0.19f,                    0.19f,
	{ 0.6f,                   0.6f, 0.6f,  1.f }
	,
	0,                        0,
	&cgs.media.limboFont1_lo,
};

panel_button_text_t fireteamFont =
{
	0.2f,                    0.2f,
	{ 0.6f,                  0.6f, 0.6f,    1.f }
	,
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

panel_button_t      fireteamTopBorder =
{
	NULL,
	NULL,
	{ 10,                     129,        204,       136 }
	,
	{ 1,                      255 * .5f,  255 * .5f, 255 * .5f, 255 * .5f, 1, 0, 0}
	,
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

panel_button_t      fireteamTopBorderBack =
{
	"white",
	NULL,
	{ 11,                     130, 202, 134 }
	,
	{ 1,                      0,  0,   0, 255 * 0.75f, 0, 0, 0}
	,
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

panel_button_t      fireteamTopBorderInner =
{
	"white",
	NULL,
	{ 12,                     131, 200, 12 }
	,
	{ 1,                      41,  51,  43, 204, 0, 0, 0}
	,
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

panel_button_t      fireteamTopBorderInnerText =
{
	NULL,
	NULL,
	{ 15,                           141,  200, 12 }
	,
	{ 0,                            0,    0,   0, 0, 0, 0, 0}
	,
	&fireteamTitleFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_Fireteams_MenuTitleText_Draw,
	NULL,
};

panel_button_t      fireteamMenuItemText =
{
	NULL,
	NULL,
	{ 16,                      153,  128, 12 }
	,
	{ 0,                       0,    0,   0, 0, 0, 0, 0}
	,
	&fireteamFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_Fireteams_MenuText_Draw,
	NULL,
};

panel_button_t      *fireteamButtons[] =
{
	&fireteamTopBorderBack, &fireteamTopBorder, &fireteamTopBorderInner, &fireteamTopBorderInnerText,

	&fireteamMenuItemText,

	NULL
};

const char          *ftMenuRootStrings[] =
{
	"Soldier",
	"Medic",
	"Engineer",
	"Field Ops",
	"Covert Ops",
	"General",
	"Attack",
	"Fall Back",
	NULL
};

const char          *ftMenuRootStringsMsg[] =
{
	"",
	"",
	"",
	"",
	"",
	"",
	"FTAttack",
	"FTFallBack",
	NULL
};

const char          *ftMenuRootStringsAlphachars[] =
{
	"S",
	"M",
	"E",
	"F",
	"C",
	"G",
	"A",
	"B",
	NULL
};

const char          *ftMenuSoliderStrings[] =
{
	"Cover Me",
	"Covering Fire",
	"Mortar",
	NULL
};

const char          *ftMenuSoliderStringsAlphachars[] =
{
	"C",
	"F",
	"M",
	NULL
};

const char          *ftMenuSoliderStringsMsg[] =
{
	"FTCoverMe",
	"FTCoveringFire",
	"FTMortar",
	NULL
};

const char          *ftMenuMedicStrings[] =
{
	"Heal Squad",
	"Heal Me",
	"Revive Team Mate",
	"Revive Me",
	NULL
};

const char          *ftMenuMedicStringsAlphachars[] =
{
	"H",
	"M",
	"R",
	"E",
	NULL
};

const char          *ftMenuMedicStringsMsg[] =
{
	"FTHealSquad",
	"FTHealMe",
	"FTReviveTeamMate",
	"FTReviveMe",
	NULL
};

const char          *ftMenuEngineerStrings[] =
{
	"Destroy Objective",
	"Repair Objective",
	"Construct Objective",
	"Disarm Dynamite",
	"Deploy Landmines",
	"Disarm Landmines",
	NULL
};

const char          *ftMenuEngineerStringsAlphachars[] =
{
	"D",
	"R",
	"C",
	"A",
	"L",
	"M",
	NULL
};

const char          *ftMenuEngineerStringsMsg[] =
{
	"FTDestroyObjective",
	"FTRepairObjective",
	"FTConstructObjective",
	"FTDisarmDynamite",
	"FTDeployLandmines",
	"FTDisarmLandmines",
	NULL
};

const char          *ftMenuFieldOpsStrings[] =
{
	"Call Air-Strike",
	"Call Artillery",
	"Resupply Squad",
	"Resupply Me",
	NULL
};

const char          *ftMenuFieldOpsStringsAlphachars[] =
{
	"A",
	"T",
	"R",
	"S",
	NULL
};

const char          *ftMenuFieldOpsStringsMsg[] =
{
	"FTCallAirStrike",
	"FTCallArtillery",
	"FTResupplySquad",
	"FTResupplyMe",
	NULL
};

const char          *ftMenuCovertOpsStrings[] =
{
	"Explore Area",
	"Destroy Objective",
	"Infiltrate",
	"Go Undercover",
	"Provide Sniper Cover",
	NULL
};

const char          *ftMenuCovertOpsStringsAlphachars[] =
{
	"E",
	"D",
	"I",
	"U",
	"S",
	NULL
};

const char          *ftMenuCovertOpsStringsMsg[] =
{
	"FTExploreArea",
	"FTSatchelObjective",
	"FTInfiltrate",
	"FTGoUndercover",
	"FTProvideSniperCover",
	NULL
};

const char          **ftMenuStrings[] =
{
	ftMenuSoliderStrings,
	ftMenuMedicStrings,
	ftMenuEngineerStrings,
	ftMenuFieldOpsStrings,
	ftMenuCovertOpsStrings,
};

const char          **ftMenuStringsAlphachars[] =
{
	ftMenuSoliderStringsAlphachars,
	ftMenuMedicStringsAlphachars,
	ftMenuEngineerStringsAlphachars,
	ftMenuFieldOpsStringsAlphachars,
	ftMenuCovertOpsStringsAlphachars,
};

const char          **ftMenuStringsMsg[] =
{
	ftMenuSoliderStringsMsg,
	ftMenuMedicStringsMsg,
	ftMenuEngineerStringsMsg,
	ftMenuFieldOpsStringsMsg,
	ftMenuCovertOpsStringsMsg,
};

void CG_Fireteams_MenuTitleText_Draw( panel_button_t *button )
{
	switch( cgs.ftMenuMode )
	{
		case 0:
			CG_Text_Paint_Ext( button->rect.x, button->rect.y + button->data[ 0 ], button->font->scalex, button->font->scaley,
			                   button->font->colour, "MESSAGE", 0, 0, button->font->style, button->font->font );
			break;

		case 1:
			CG_Text_Paint_Ext( button->rect.x, button->rect.y + button->data[ 0 ], button->font->scalex, button->font->scaley,
			                   button->font->colour, "FIRETEAMS", 0, 0, button->font->style, button->font->font );
			break;

		case 2:
			CG_Text_Paint_Ext( button->rect.x, button->rect.y + button->data[ 0 ], button->font->scalex, button->font->scaley,
			                   button->font->colour, "JOIN", 0, 0, button->font->style, button->font->font );
			break;

		case 3:
			CG_Text_Paint_Ext( button->rect.x, button->rect.y + button->data[ 0 ], button->font->scalex, button->font->scaley,
			                   button->font->colour, "PROPOSE", 0, 0, button->font->style, button->font->font );
			break;

		case 4:
			switch( cgs.ftMenuPos )
			{
				case 2:
					CG_Text_Paint_Ext( button->rect.x, button->rect.y + button->data[ 0 ], button->font->scalex,
					                   button->font->scaley, button->font->colour, "INVITE", 0, 0, button->font->style,
					                   button->font->font );
					break;

				case 3:
					CG_Text_Paint_Ext( button->rect.x, button->rect.y + button->data[ 0 ], button->font->scalex,
					                   button->font->scaley, button->font->colour, "KICK", 0, 0, button->font->style,
					                   button->font->font );
					break;

				case 4:
					CG_Text_Paint_Ext( button->rect.x, button->rect.y + button->data[ 0 ], button->font->scalex,
					                   button->font->scaley, button->font->colour, "WARN", 0, 0, button->font->style,
					                   button->font->font );
					break;
			}
	}
}

const char *ftOffMenuList[] =
{
	"Apply",
	"Create",
	NULL,
};

const char *ftOffMenuListAlphachars[] =
{
	"A",
	"C",
	NULL,
};

const char *ftOnMenuList[] =
{
	"Propose",
	"Leave",
	NULL,
};

const char *ftOnMenuListAlphachars[] =
{
	"P",
	"L",
	NULL,
};

const char *ftLeaderMenuList[] =
{
	"Disband",
	"Leave",
	"Invite",
	"Kick",
	"Warn",
	NULL,
};

const char *ftLeaderMenuListAlphachars[] =
{
	"D",
	"L",
	"I",
	"K",
	"W",
	NULL,
};

int CG_CountFireteamsByTeam( team_t t )
{
	int cnt = 0;
	int i;

	if( t != TEAM_AXIS && t != TEAM_ALLIES )
	{
		return 0;
	}

	for( i = 0; i < MAX_FIRETEAMS; i++ )
	{
		if( !cg.fireTeams[ i ].inuse )
		{
			continue;
		}

		if( cgs.clientinfo[ cg.fireTeams[ i ].leader ].team != t )
		{
			continue;
		}

		cnt++;
	}

	return cnt;
}

void CG_DrawFireteamsByTeam( panel_button_t *button, team_t t )
{
	float      y = button->rect.y;
	const char *str;
	int        i;

	if( t != TEAM_AXIS && t != TEAM_ALLIES )
	{
		return;
	}

	for( i = 0; i < MAX_FIRETEAMS; i++ )
	{
		if( !cg.fireTeams[ i ].inuse )
		{
			continue;
		}

		if( cgs.clientinfo[ cg.fireTeams[ i ].leader ].team != t )
		{
			continue;
		}

		if( cg_quickMessageAlt.integer )
		{
			str = va( "%i. %s", ( cg.fireTeams[ i ].ident + 1 ) % 10, bg_fireteamNames[ cg.fireTeams[ i ].ident ] );
		}
		else
		{
			str = va( "%c. %s", 'A' + cg.fireTeams[ i ].ident, bg_fireteamNames[ cg.fireTeams[ i ].ident ] );
		}

		CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0,
		                   button->font->style, button->font->font );

		y += button->rect.h;
	}
}

int CG_CountPlayersSF( void )
{
	int i, cnt = 0;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( i == cg.clientNum )
		{
			continue;
		}

		if( !cgs.clientinfo[ i ].infoValid )
		{
			continue;
		}

		if( cgs.clientinfo[ i ].team != cgs.clientinfo[ cg.clientNum ].team )
		{
			continue;
		}

		if( CG_IsOnFireteam( i ) != CG_IsOnFireteam( cg.clientNum ) )
		{
			continue;
		}

		cnt++;
	}

	return cnt;
}

int CG_CountPlayersNF( void )
{
	int i, cnt = 0;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( i == cg.clientNum )
		{
			continue;
		}

		if( !cgs.clientinfo[ i ].infoValid )
		{
			continue;
		}

		if( cgs.clientinfo[ i ].team != cgs.clientinfo[ cg.clientNum ].team )
		{
			continue;
		}

		if( CG_IsOnFireteam( i ) )
		{
			continue;
		}

		cnt++;
	}

	return cnt;
}

int CG_PlayerSFFromPos( int pos, int *pageofs )
{
	int x, i;

	if( !CG_IsOnFireteam( cg.clientNum ) )
	{
		*pageofs = 0;
		return -1;
	}

	x = CG_CountPlayersSF();

	if( x < ( ( *pageofs ) * 8 ) )
	{
		*pageofs = 0;
	}

	x = 0;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( i == cg.clientNum )
		{
			continue;
		}

		if( !cgs.clientinfo[ i ].infoValid )
		{
			continue;
		}

		if( cgs.clientinfo[ i ].team != cgs.clientinfo[ cg.clientNum ].team )
		{
			continue;
		}

		if( CG_IsOnFireteam( i ) != CG_IsOnFireteam( cg.clientNum ) )
		{
			continue;
		}

		if( x >= ( ( *pageofs ) * 8 ) && x < ( ( *pageofs + 1 ) * 8 ) )
		{
			int ofs = x - ( ( *pageofs ) * 8 );

			if( pos == ofs )
			{
				return i;
			}
		}

		x++;
	}

	return -1;
}

int CG_PlayerNFFromPos( int pos, int *pageofs )
{
	int x, i;

	if( !CG_IsOnFireteam( cg.clientNum ) )
	{
		*pageofs = 0;
		return -1;
	}

	x = CG_CountPlayersNF();

	if( x < ( ( *pageofs ) * 8 ) )
	{
		*pageofs = 0;
	}

	x = 0;

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		if( i == cg.clientNum )
		{
			continue;
		}

		if( !cgs.clientinfo[ i ].infoValid )
		{
			continue;
		}

		if( cgs.clientinfo[ i ].team != cgs.clientinfo[ cg.clientNum ].team )
		{
			continue;
		}

		if( CG_IsOnFireteam( i ) )
		{
			continue;
		}

		if( x >= ( ( *pageofs ) * 8 ) && x < ( ( *pageofs + 1 ) * 8 ) )
		{
			int ofs = x - ( ( *pageofs ) * 8 );

			if( pos == ofs )
			{
				return i;
			}
		}

		x++;
	}

	return -1;
}

void CG_DrawPlayerSF( panel_button_t *button, int *pageofs )
{
	float      y = button->rect.y;
	const char *str;
	int        i, x;

	for( i = 0; i < 8; i++ )
	{
		x = CG_PlayerSFFromPos( i, pageofs );

		if( x == -1 )
		{
			break;
		}

		if( cg_quickMessageAlt.integer )
		{
			str = va( "%i. %s", ( i + 1 ) % 10, cgs.clientinfo[ x ].name );
		}
		else
		{
			str = va( "%c. %s", 'A' + i, cgs.clientinfo[ x ].name );
		}

		CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0,
		                   button->font->style, button->font->font );

		y += button->rect.h;
	}

	if( *pageofs )
	{
		if( cg_quickMessageAlt.integer )
		{
			str = va( "%i. %s", ( 8 + 1 ) % 10, "Previous" );
		}
		else
		{
			str = va( "%c. %s", 'P', "Previous" );
		}

		CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0,
		                   button->font->style, button->font->font );

		y += button->rect.h;
	}

	if( CG_CountPlayersSF() > ( *pageofs + 1 ) * 8 )
	{
		if( cg_quickMessageAlt.integer )
		{
			str = va( "%i. %s", ( 9 + 1 ) % 10, "Next" );
		}
		else
		{
			str = va( "%c. %s", 'N', "Next" );
		}

		CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0,
		                   button->font->style, button->font->font );

		y += button->rect.h;
	}
}

void CG_DrawPlayerNF( panel_button_t *button, int *pageofs )
{
	float      y = button->rect.y;
	const char *str;
	int        i, x;

	for( i = 0; i < 8; i++ )
	{
		x = CG_PlayerNFFromPos( i, pageofs );

		if( x == -1 )
		{
			break;
		}

		if( cg_quickMessageAlt.integer )
		{
			str = va( "%i. %s", ( i + 1 ) % 10, cgs.clientinfo[ x ].name );
		}
		else
		{
			str = va( "%c. %s", 'A' + i, cgs.clientinfo[ x ].name );
		}

		CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0,
		                   button->font->style, button->font->font );

		y += button->rect.h;
	}

	if( *pageofs )
	{
		if( cg_quickMessageAlt.integer )
		{
			str = va( "%i. %s", ( 8 + 1 ) % 10, "Previous" );
		}
		else
		{
			str = va( "%c. %s", 'P', "Previous" );
		}

		CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0,
		                   button->font->style, button->font->font );

		y += button->rect.h;
	}

	if( CG_CountPlayersNF() > ( *pageofs + 1 ) * 8 )
	{
		if( cg_quickMessageAlt.integer )
		{
			str = va( "%i. %s", ( 9 + 1 ) % 10, "Next" );
		}
		else
		{
			str = va( "%c. %s", 'N', "Next" );
		}

		CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0, 0,
		                   button->font->style, button->font->font );

		y += button->rect.h;
	}
}

void CG_Fireteams_MenuText_Draw( panel_button_t *button )
{
	float y = button->rect.y;
	int   i;

	switch( cgs.ftMenuMode )
	{
		case 0:
			if( cgs.ftMenuPos == -1 )
			{
				for( i = 0; ftMenuRootStrings[ i ]; i++ )
				{
					const char *str;

					if( i < 5 )
					{
						if( !CG_FireteamHasClass( i, qtrue ) )
						{
							continue;
						}
					}

					if( cg_quickMessageAlt.integer )
					{
						str = va( "%i. %s", ( i + 1 ) % 10, ftMenuRootStrings[ i ] );
					}
					else
					{
						str = va( "%s. %s", ftMenuRootStringsAlphachars[ i ], ftMenuRootStrings[ i ] );
					}

					CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0,
					                   0, button->font->style, button->font->font );

					y += button->rect.h;
				}
			}
			else
			{
				if( cgs.ftMenuPos < 0 || cgs.ftMenuPos > 4 )
				{
					return;
				}
				else
				{
					const char **strings = ftMenuStrings[ cgs.ftMenuPos ];

					for( i = 0; strings[ i ]; i++ )
					{
						const char *str;

						if( cg_quickMessageAlt.integer )
						{
							str = va( "%i. %s", ( i + 1 ) % 10, strings[ i ] );
						}
						else
						{
							str = va( "%s. %s", ( ftMenuStringsAlphachars[ cgs.ftMenuPos ] ) [ i ], strings[ i ] );
						}

						CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour,
						                   str, 0, 0, button->font->style, button->font->font );

						y += button->rect.h;
					}
				}
			}

			break;

		case 1:
			if( !CG_IsOnFireteam( cg.clientNum ) )
			{
				for( i = 0; ftOffMenuList[ i ]; i++ )
				{
					const char *str;

					if( i == 0 && !CG_CountFireteamsByTeam( cgs.clientinfo[ cg.clientNum ].team ) )
					{
						continue;
					}

					if( cg_quickMessageAlt.integer )
					{
						str = va( "%i. %s", ( i + 1 ) % 10, ftOffMenuList[ i ] );
					}
					else
					{
						str = va( "%s. %s", ftOffMenuListAlphachars[ i ], ftOffMenuList[ i ] );
					}

					CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, str, 0,
					                   0, button->font->style, button->font->font );

					y += button->rect.h;
				}
			}
			else
			{
				if( !CG_IsFireTeamLeader( cg.clientNum ) )
				{
					for( i = 0; ftOnMenuList[ i ]; i++ )
					{
						const char *str;

						if( i == 0 && !CG_CountPlayersNF() )
						{
							continue;
						}

						if( cg_quickMessageAlt.integer )
						{
							str = va( "%i. %s", ( i + 1 ) % 10, ftOnMenuList[ i ] );
						}
						else
						{
							str = va( "%s. %s", ftOnMenuListAlphachars[ i ], ftOnMenuList[ i ] );
						}

						CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour,
						                   str, 0, 0, button->font->style, button->font->font );

						y += button->rect.h;
					}
				}
				else
				{
					for( i = 0; ftLeaderMenuList[ i ]; i++ )
					{
						const char *str;

						if( i == 2 && !CG_CountPlayersNF() )
						{
							continue;
						}

						if( ( i == 3 || i == 4 ) && !CG_CountPlayersSF() )
						{
							continue;
						}

						if( cg_quickMessageAlt.integer )
						{
							str = va( "%i. %s", ( i + 1 ) % 10, ftLeaderMenuList[ i ] );
						}
						else
						{
							str = va( "%s. %s", ftLeaderMenuListAlphachars[ i ], ftLeaderMenuList[ i ] );
						}

						CG_Text_Paint_Ext( button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour,
						                   str, 0, 0, button->font->style, button->font->font );

						y += button->rect.h;
					}
				}
			}

			break;

		case 2:
			if( !CG_CountFireteamsByTeam( cgs.clientinfo[ cg.clientNum ].team ) || CG_IsOnFireteam( cg.clientNum ) )
			{
				cgs.ftMenuMode = 1;
				break;
			}

			CG_DrawFireteamsByTeam( button, cgs.clientinfo[ cg.clientNum ].team );
			break;

		case 3:
			if( !CG_CountPlayersNF() )
			{
				cgs.ftMenuMode = 1;
				break;
			}

			CG_DrawPlayerNF( button, &cgs.ftMenuModeEx );
			break;

		case 4:
			switch( cgs.ftMenuPos )
			{
				case 2:
					if( !CG_CountPlayersNF() )
					{
						cgs.ftMenuMode = 1;
						break;
					}

					CG_DrawPlayerNF( button, &cgs.ftMenuModeEx );
					break;

				case 3:
				case 4:
					if( !CG_CountPlayersSF() )
					{
						cgs.ftMenuMode = 1;
						break;
					}

					CG_DrawPlayerSF( button, &cgs.ftMenuModeEx );
					break;
			}

			break;
	}
}

void CG_Fireteams_Setup( void )
{
	BG_PanelButtonsSetup( fireteamButtons );
}

void CG_Fireteams_KeyHandling( int key, qboolean down )
{
	if( down )
	{
		CG_FireteamCheckExecKey( key, qtrue );
	}
}

void CG_Fireteams_Draw( void )
{
	BG_PanelButtonsRender( fireteamButtons );
}

void CG_QuickFireteamMessage_f( void );

qboolean CG_FireteamCheckExecKey( int key, qboolean doaction )
{
	if( key == K_ESCAPE )
	{
		return qtrue;
	}

	if( ( key & K_CHAR_FLAG ) )
	{
		return qfalse;
	}

	key &= ~K_CHAR_FLAG;

	switch( cgs.ftMenuMode )
	{
		case 0:
			if( cgs.ftMenuPos == -1 )
			{
				if( cg_quickMessageAlt.integer )
				{
					if( key >= '0' && key <= '9' )
					{
						int i = ( ( key - '0' ) + 9 ) % 10;

						if( i < 5 )
						{
							if( !CG_FireteamHasClass( i, qtrue ) )
							{
								return qfalse;
							}
						}

						if( i > 7 )
						{
							return qfalse;
						}

						if( doaction )
						{
							if( i < 5 )
							{
								cgs.ftMenuPos = i;
							}
							else if( i == 5 )
							{
								CG_QuickFireteamMessage_f();
							}
							else
							{
								trap_SendClientCommand( va
								                        ( "vsay_buddy -1 %s %s", CG_BuildSelectedFirteamString(),
								                          ftMenuRootStringsMsg[ i ] ) );
								CG_EventHandling( CGAME_EVENT_NONE, qfalse );
							}
						}

						return qtrue;
					}
				}
				else
				{
					int i;

					if( key >= 'a' || key <= 'z' )
					{
						for( i = 0; ftMenuRootStrings[ i ]; i++ )
						{
							if( key == tolower( *ftMenuRootStringsAlphachars[ i ] ) )
							{
								if( i < 5 )
								{
									if( !CG_FireteamHasClass( i, qtrue ) )
									{
										return qfalse;
									}
								}

								if( doaction )
								{
									if( i < 5 )
									{
										cgs.ftMenuPos = i;
									}
									else if( i == 5 )
									{
										CG_QuickFireteamMessage_f();
									}
									else
									{
										trap_SendClientCommand( va
										                        ( "vsay_buddy -1 %s %s", CG_BuildSelectedFirteamString(),
										                          ftMenuRootStringsMsg[ i ] ) );
										CG_EventHandling( CGAME_EVENT_NONE, qfalse );
									}
								}

								return qtrue;
							}
						}
					}
				}
			}
			else
			{
				if( cgs.ftMenuPos < 0 || cgs.ftMenuPos > 4 )
				{
					return qfalse;
				}

				if( cg_quickMessageAlt.integer )
				{
					if( key >= '0' && key <= '9' )
					{
						int        i = ( ( key - '0' ) + 9 ) % 10;
						int        x;

						const char **strings = ftMenuStrings[ cgs.ftMenuPos ];

						for( x = 0; strings[ x ]; x++ )
						{
							if( x == i )
							{
								if( doaction )
								{
									trap_SendClientCommand( va
									                        ( "vsay_buddy %i %s %s", cgs.ftMenuPos, CG_BuildSelectedFirteamString(),
									                          ( ftMenuStringsMsg[ cgs.ftMenuPos ] ) [ i ] ) );
									CG_EventHandling( CGAME_EVENT_NONE, qfalse );
								}

								return qtrue;
							}
						}
					}
				}
				else
				{
					int        i;
					const char **strings = ftMenuStrings[ cgs.ftMenuPos ];

					if( key >= 'a' || key <= 'z' )
					{
						for( i = 0; strings[ i ]; i++ )
						{
							if( key == tolower( *ftMenuStringsAlphachars[ cgs.ftMenuPos ][ i ] ) )
							{
								if( doaction )
								{
									trap_SendClientCommand( va
									                        ( "vsay_buddy %i %s %s", cgs.ftMenuPos, CG_BuildSelectedFirteamString(),
									                          ( ftMenuStringsMsg[ cgs.ftMenuPos ] ) [ i ] ) );
									CG_EventHandling( CGAME_EVENT_NONE, qfalse );
								}

								return qtrue;
							}
						}
					}
				}
			}

			break;

		case 1:
			{
				int i = -1, x;

				if( cg_quickMessageAlt.integer )
				{
					if( key >= '0' && key <= '9' )
					{
						i = ( ( key - '0' ) + 9 ) % 10;
					}
				}
				else
				{
					const char **strings;

					if( !CG_IsOnFireteam( cg.clientNum ) )
					{
						strings = ftOffMenuListAlphachars;
					}
					else
					{
						if( !CG_IsFireTeamLeader( cg.clientNum ) )
						{
							strings = ftOnMenuListAlphachars;
						}
						else
						{
							strings = ftLeaderMenuListAlphachars;
						}
					}

					if( key >= 'a' || key <= 'z' )
					{
						for( x = 0; strings[ x ]; x++ )
						{
							if( key == tolower( *strings[ x ] ) )
							{
								i = x;
								break;
							}
						}
					}
				}

				if( i == -1 )
				{
					break;
				}

				if( !CG_IsOnFireteam( cg.clientNum ) )
				{
					if( i >= 2 )
					{
						break;
					}

					if( i == 0 && !CG_CountFireteamsByTeam( cgs.clientinfo[ cg.clientNum ].team ) )
					{
						return qfalse;
					}

					if( doaction )
					{
						if( i == 1 )
						{
							trap_SendConsoleCommand( "fireteam create\n" );
							CG_EventHandling( CGAME_EVENT_NONE, qfalse );
						}
						else
						{
							cgs.ftMenuMode = 2;
							cgs.ftMenuModeEx = 0;
							cgs.ftMenuPos = i;
						}
					}

					return qtrue;
				}
				else
				{
					if( !CG_IsFireTeamLeader( cg.clientNum ) )
					{
						if( i >= 2 )
						{
							break;
						}

						if( i == 0 && !CG_CountPlayersNF() )
						{
							break;
						}

						if( doaction )
						{
							if( i == 1 )
							{
								trap_SendConsoleCommand( "fireteam leave\n" );
								CG_EventHandling( CGAME_EVENT_NONE, qfalse );
							}
							else
							{
								cgs.ftMenuMode = 3;
								cgs.ftMenuModeEx = 0;
								cgs.ftMenuPos = i;
							}
						}

						return qtrue;
					}
					else
					{
						if( i >= 5 )
						{
							break;
						}

						if( i == 2 && !CG_CountPlayersNF() )
						{
							break;
						}

						if( ( i == 3 || i == 4 ) && !CG_CountPlayersSF() )
						{
							break;
						}

						if( doaction )
						{
							if( i == 0 )
							{
								trap_SendConsoleCommand( "fireteam disband\n" );
								CG_EventHandling( CGAME_EVENT_NONE, qfalse );
							}
							else if( i == 1 )
							{
								trap_SendConsoleCommand( "fireteam leave\n" );
								CG_EventHandling( CGAME_EVENT_NONE, qfalse );
							}
							else
							{
								cgs.ftMenuMode = 4;
								cgs.ftMenuModeEx = 0;
								cgs.ftMenuPos = i;
							}
						}

						return qtrue;
					}
				}
			}
			break;

		case 2:
			{
				int i;

				for( i = 0; i < MAX_FIRETEAMS; i++ )
				{
					if( !cg.fireTeams[ i ].inuse )
					{
						continue;
					}

					if( cgs.clientinfo[ cg.fireTeams[ i ].leader ].team != cgs.clientinfo[ cg.clientNum ].team )
					{
						continue;
					}

					if( cg_quickMessageAlt.integer )
					{
						if( key >= '0' && key <= '9' )
						{
							if( ( ( key - '0' ) + 9 ) % 10 == cg.fireTeams[ i ].ident )
							{
								if( doaction )
								{
									trap_SendConsoleCommand( va( "fireteam apply %i", i + 1 ) );
									CG_EventHandling( CGAME_EVENT_NONE, qfalse );
								}

								return qtrue;
							}
						}
					}
					else
					{
						if( key >= 'a' || key <= 'z' )
						{
							if( key - 'a' == cg.fireTeams[ i ].ident )
							{
								if( doaction )
								{
									trap_SendConsoleCommand( va( "fireteam apply %i", i + 1 ) );
									CG_EventHandling( CGAME_EVENT_NONE, qfalse );
								}

								return qtrue;
							}
						}
					}
				}
			}
			break;

		case 3:
			{
				int i = -1, x;

				if( cg_quickMessageAlt.integer )
				{
					if( key >= '0' && key <= '9' )
					{
						i = ( ( key - '0' ) + 9 ) % 10;
					}
				}
				else
				{
					if( key >= 'a' || key <= 'g' )
					{
						i = key - 'a';
					}

					if( key == 'n' )
					{
						i = 9;
					}

					if( key == 'p' )
					{
						i = 0;
					}
				}

				if( i == -1 )
				{
					break;
				}

				if( CG_CountPlayersNF() > ( cgs.ftMenuModeEx + 1 ) * 8 )
				{
					if( i == 0 )
					{
						cgs.ftMenuModeEx++;
					}
				}

				if( cgs.ftMenuModeEx )
				{
					if( i == 9 )
					{
						cgs.ftMenuModeEx--;
					}
				}

				x = CG_PlayerNFFromPos( i, &cgs.ftMenuModeEx );

				if( x != -1 )
				{
					if( doaction )
					{
						trap_SendConsoleCommand( va( "fireteam propose %i", x + 1 ) );
						CG_EventHandling( CGAME_EVENT_NONE, qfalse );
					}

					return qtrue;
				}

				break;
			}
			break;

		case 4:
			{
				int i = -1, x;

				if( cg_quickMessageAlt.integer )
				{
					if( key >= '0' && key <= '9' )
					{
						i = ( ( key - '0' ) + 9 ) % 10;
					}
				}
				else
				{
					if( key >= 'a' || key <= 'g' )
					{
						i = key - 'a';
					}

					if( key == 'n' )
					{
						i = 9;
					}

					if( key == 'p' )
					{
						i = 8;
					}
				}

				if( i == -1 )
				{
					break;
				}

				switch( cgs.ftMenuPos )
				{
					case 2:
						if( CG_CountPlayersNF() > ( cgs.ftMenuModeEx + 1 ) * 8 )
						{
							if( i == 9 )
							{
								if( doaction )
								{
									cgs.ftMenuModeEx++;
								}

								return qtrue;
							}
						}

						if( cgs.ftMenuModeEx )
						{
							if( i == 8 )
							{
								if( doaction )
								{
									cgs.ftMenuModeEx--;
								}

								return qtrue;
							}
						}

						x = CG_PlayerNFFromPos( i, &cgs.ftMenuModeEx );

						if( x != -1 )
						{
							if( doaction )
							{
								trap_SendConsoleCommand( va( "fireteam invite %i", x + 1 ) );
								CG_EventHandling( CGAME_EVENT_NONE, qfalse );
							}

							return qtrue;
						}

						break;

					case 3:
					case 4:
						if( CG_CountPlayersSF() > ( cgs.ftMenuModeEx + 1 ) * 8 )
						{
							if( i == 0 )
							{
								cgs.ftMenuModeEx++;
							}
						}

						if( cgs.ftMenuModeEx )
						{
							if( i == 9 )
							{
								cgs.ftMenuModeEx--;
							}
						}

						x = CG_PlayerSFFromPos( i, &cgs.ftMenuModeEx );

						if( x != -1 )
						{
							if( doaction )
							{
								switch( cgs.ftMenuPos )
								{
									case 4:
										trap_SendConsoleCommand( va( "fireteam warn %i", x + 1 ) );
										CG_EventHandling( CGAME_EVENT_NONE, qfalse );
										break;

									case 3:
										trap_SendConsoleCommand( va( "fireteam kick %i", x + 1 ) );
										CG_EventHandling( CGAME_EVENT_NONE, qfalse );
										break;
								}
							}

							return qtrue;
						}

						break;
				}
			}
			break;
	}

	return qfalse;
}
