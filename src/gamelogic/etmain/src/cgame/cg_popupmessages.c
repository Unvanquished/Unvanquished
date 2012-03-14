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

#define NUM_PM_STACK_ITEMS     32
#define MAX_VISIBLE_ITEMS      5

#define NUM_PM_STACK_ITEMS_BIG 8 // Gordon: we shouldn't need many of these

typedef struct pmStackItem_s    pmListItem_t;
typedef struct pmStackItemBig_s pmListItemBig_t;

struct pmStackItem_s
{
	popupMessageType_t type;
	qboolean           inuse;
	int                time;
	char               message[ 128 ];
	qhandle_t          shader;

	pmListItem_t       *next;
};

struct pmStackItemBig_s
{
	popupMessageBigType_t type;
	qboolean              inuse;
	int                   time;
	char                  message[ 128 ];
	qhandle_t             shader;

	pmListItemBig_t       *next;
};

pmListItem_t    cg_pmStack[ NUM_PM_STACK_ITEMS ];
pmListItem_t    *cg_pmOldList;
pmListItem_t    *cg_pmWaitingList;
pmListItemBig_t *cg_pmWaitingListBig;

pmListItemBig_t cg_pmStackBig[ NUM_PM_STACK_ITEMS_BIG ];

const char      *cg_skillRewards[ SK_NUM_SKILLS ][ NUM_SKILL_LEVELS - 1 ] =
{
	{ "Binoculars",                               "Improved Physical Fitness",                 "Improved Health",                       "Trap Awareness"           }, // battle sense
	{ "Improved use of Explosive Ammunition",     "Improved Dexterity",                        "Improved Construction and Destruction", "a Flak Jacket"            }, // explosives & construction
	{ "Medic Ammo",                               "Improved Resources",                        "Full Revive",                           "Adrenalin Self"           }, // first aid
	{ "Improved Resources",                       "Improved Signals",                          "Improved Air and Ground Support",       "Enemy Recognition"        }, // signals
	{ "Improved use of Light Weapon Ammunition",  "Faster Reload",                             "Improved Light Weapon Handling",        "Dual-Wield Pistols"       }, // light weapons
	{ "Improved Projectile Resources",            "Heavy Weapon Proficiency",                  "Improved Dexterity",                    "Improved Weapon Handling" }, // heavy weapons
	{ "Improved use of Scoped Weapon Ammunition", "Improved use of Sabotage and Misdirection", "Breath Control",                        "Assassin"                 } // scoped weapons & military intelligence
};

void            CG_PMItemBigSound ( pmListItemBig_t *item );

void CG_InitPMGraphics ( void )
{
	cgs.media.pmImages[ PM_DYNAMITE ] = trap_R_RegisterShaderNoMip ( "gfx/limbo/cm_dynamite" );
	cgs.media.pmImages[ PM_CONSTRUCTION ] = trap_R_RegisterShaderNoMip ( "sprites/voiceChat" );
	cgs.media.pmImages[ PM_MINES ] = trap_R_RegisterShaderNoMip ( "sprites/voiceChat" );
	cgs.media.pmImages[ PM_DEATH ] = trap_R_RegisterShaderNoMip ( "gfx/hud/pm_death" );
	cgs.media.pmImages[ PM_MESSAGE ] = trap_R_RegisterShaderNoMip ( "sprites/voiceChat" );
	cgs.media.pmImages[ PM_OBJECTIVE ] = trap_R_RegisterShaderNoMip ( "sprites/objective" );
	cgs.media.pmImages[ PM_DESTRUCTION ] = trap_R_RegisterShaderNoMip ( "sprites/voiceChat" );
	cgs.media.pmImages[ PM_TEAM ] = trap_R_RegisterShaderNoMip ( "sprites/voiceChat" );

	cgs.media.pmImageAlliesConstruct = trap_R_RegisterShaderNoMip ( "gfx/hud/pm_constallied" );
	cgs.media.pmImageAxisConstruct = trap_R_RegisterShaderNoMip ( "gfx/hud/pm_constaxis" );
	cgs.media.pmImageAlliesMine = trap_R_RegisterShaderNoMip ( "gfx/hud/pm_mineallied" );
	cgs.media.pmImageAxisMine = trap_R_RegisterShaderNoMip ( "gfx/hud/pm_mineaxis" );
	cgs.media.hintKey = trap_R_RegisterShaderNoMip ( "gfx/hud/keyboardkey_old" );
}

void CG_InitPM ( void )
{
	memset ( &cg_pmStack, 0, sizeof ( cg_pmStack ) );
	memset ( &cg_pmStackBig, 0, sizeof ( cg_pmStackBig ) );

	cg_pmOldList = NULL;
	cg_pmWaitingList = NULL;
	cg_pmWaitingListBig = NULL;
}

#define PM_FADETIME     2500
#define PM_WAITTIME     2000

#define PM_FADETIME_BIG 1000
#define PM_WAITTIME_BIG 3500

int CG_TimeForPopup ( popupMessageType_t type )
{
	switch ( type )
	{
		default:
			return 1000;
	}
}

int CG_TimeForBigPopup ( popupMessageBigType_t type )
{
	switch ( type )
	{
		default:
			return 2500;
	}
}

void CG_AddToListFront ( pmListItem_t **list, pmListItem_t *item )
{
	item->next = *list;
	*list = item;
}

void CG_UpdatePMLists ( void )
{
	pmListItem_t    *listItem;
	pmListItem_t    *lastItem;
	pmListItemBig_t *listItem2;

	if ( ( listItem = cg_pmWaitingList ) )
	{
		int t = ( CG_TimeForPopup ( listItem->type ) + listItem->time );

		if ( cg.time > t )
		{
			if ( listItem->next )
			{
				// there's another item waiting to come on, so move to old list
				cg_pmWaitingList = listItem->next;
				cg_pmWaitingList->time = cg.time; // set time we popped up at

				CG_AddToListFront ( &cg_pmOldList, listItem );
			}
			else
			{
				if ( cg.time > t + PM_WAITTIME + PM_FADETIME )
				{
					// we're gone completely
					cg_pmWaitingList = NULL;
					listItem->inuse = qfalse;
					listItem->next = NULL;
				}
				else
				{
					// just sit where we are, no pressure to do anything...
				}
			}
		}
	}

	listItem = cg_pmOldList;
	lastItem = NULL;

	while ( listItem )
	{
		int t = ( CG_TimeForPopup ( listItem->type ) + listItem->time + PM_WAITTIME + PM_FADETIME );

		if ( cg.time > t )
		{
			// nuke this, and everything below it (though there shouldn't BE anything below us anyway)
			pmListItem_t *next;

			if ( !lastItem )
			{
				// we're the top of the old list, so set to NULL
				cg_pmOldList = NULL;
			}
			else
			{
				lastItem->next = NULL;
			}

			do
			{
				next = listItem->next;

				listItem->next = NULL;
				listItem->inuse = qfalse;
			}
			while ( ( listItem = next ) );

			break;
		}

		lastItem = listItem;
		listItem = listItem->next;
	}

	if ( ( listItem2 = cg_pmWaitingListBig ) )
	{
		int t = CG_TimeForBigPopup ( listItem2->type ) + listItem2->time;

		if ( cg.time > t )
		{
			if ( listItem2->next )
			{
				// there's another item waiting to come on, so kill us and shove the next one to the front
				cg_pmWaitingListBig = listItem2->next;
				cg_pmWaitingListBig->time = cg.time; // set time we popped up at

				CG_PMItemBigSound ( cg_pmWaitingListBig );

				listItem2->inuse = qfalse;
				listItem2->next = NULL;
			}
			else
			{
				if ( cg.time > t + PM_WAITTIME + PM_FADETIME )
				{
					// we're gone completely
					cg_pmWaitingListBig = NULL;
					listItem2->inuse = qfalse;
					listItem2->next = NULL;
				}
				else
				{
					// just sit where we are, no pressure to do anything...
				}
			}
		}
	}
}

pmListItemBig_t *CG_FindFreePMItem2 ( void )
{
	int i = 0;

	for ( ; i < NUM_PM_STACK_ITEMS_BIG; i++ )
	{
		if ( !cg_pmStackBig[ i ].inuse )
		{
			return &cg_pmStackBig[ i ];
		}
	}

	return NULL;
}

pmListItem_t   *CG_FindFreePMItem ( void )
{
	pmListItem_t *listItem;
	pmListItem_t *lastItem;

	int          i = 0;

	for ( ; i < NUM_PM_STACK_ITEMS; i++ )
	{
		if ( !cg_pmStack[ i ].inuse )
		{
			return &cg_pmStack[ i ];
		}
	}

	// no totally free items, so just grab the last item in the oldlist
	if ( ( lastItem = listItem = cg_pmOldList ) )
	{
		while ( listItem->next )
		{
			lastItem = listItem;
			listItem = listItem->next;
		}

		if ( lastItem == cg_pmOldList )
		{
			cg_pmOldList = NULL;
		}
		else
		{
			lastItem->next = NULL;
		}

		listItem->inuse = qfalse;

		return listItem;
	}
	else
	{
		// there is no old list... PANIC!
		return NULL;
	}
}

void CG_AddPMItem ( popupMessageType_t type, const char *message, qhandle_t shader )
{
	pmListItem_t *listItem;
	char         *end;

	if ( !message || !*message )
	{
		return;
	}

	if ( ( unsigned ) type >= PM_NUM_TYPES )
	{
		CG_Printf ( "Invalid popup type: %d\n", type );
		return;
	}

	listItem = CG_FindFreePMItem();

	if ( !listItem )
	{
		return;
	}

	if ( shader )
	{
		listItem->shader = shader;
	}
	else
	{
		listItem->shader = cgs.media.pmImages[ type ];
	}

	listItem->inuse = qtrue;
	listItem->type = type;
	Q_strncpyz ( listItem->message, message, sizeof ( cg_pmStack[ 0 ].message ) );

	// rain - moved this: print and THEN chop off the newline, as the
	// console deals with newlines perfectly.  We do chop off the newline
	// at the end, if any, though.
	if ( listItem->message[ strlen ( listItem->message ) - 1 ] == '\n' )
	{
		listItem->message[ strlen ( listItem->message ) - 1 ] = 0;
	}

	trap_Print ( va ( "%s\n", listItem->message ) );

	// rain - added parens
	while ( ( end = strchr ( listItem->message, '\n' ) ) )
	{
		*end = '\0';
	}

	// rain - don't eat popups for empty lines
	if ( *listItem->message == '\0' )
	{
		return;
	}

	if ( !cg_pmWaitingList )
	{
		cg_pmWaitingList = listItem;
		listItem->time = cg.time;
	}
	else
	{
		pmListItem_t *loop = cg_pmWaitingList;

		while ( loop->next )
		{
			loop = loop->next;
		}

		loop->next = listItem;
	}
}

void CG_PMItemBigSound ( pmListItemBig_t *item )
{
	if ( !cg.snap )
	{
		return;
	}

	switch ( item->type )
	{
		case PM_RANK:
			trap_S_StartSound ( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.sndRankUp );
			break;

		case PM_SKILL:
			trap_S_StartSound ( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.media.sndSkillUp );
			break;

		default:
			break;
	}
}

void CG_AddPMItemBig ( popupMessageBigType_t type, const char *message, qhandle_t shader )
{
	pmListItemBig_t *listItem = CG_FindFreePMItem2();

	if ( !listItem )
	{
		return;
	}

	if ( shader )
	{
		listItem->shader = shader;
	}
	else
	{
		listItem->shader = cgs.media.pmImages[ type ];
	}

	listItem->inuse = qtrue;
	listItem->type = type;
	listItem->next = NULL;
	Q_strncpyz ( listItem->message, message, sizeof ( cg_pmStackBig[ 0 ].message ) );

	if ( !cg_pmWaitingListBig )
	{
		cg_pmWaitingListBig = listItem;
		listItem->time = cg.time;

		CG_PMItemBigSound ( listItem );
	}
	else
	{
		pmListItemBig_t *loop = cg_pmWaitingListBig;

		while ( loop->next )
		{
			loop = loop->next;
		}

		loop->next = listItem;
	}
}

#define PM_ICON_SIZE_NORMAL 20
#define PM_ICON_SIZE_SMALL  12
void CG_DrawPMItems ( void )
{
	vec4_t       colour = { 0.f, 0.f, 0.f, 1.f };
	vec4_t       colourText = { 1.f, 1.f, 1.f, 1.f };
	float        t;
	int          i, size;
	pmListItem_t *listItem = cg_pmOldList;
	float        y = 360;

	if ( cg_drawSmallPopupIcons.integer )
	{
		size = PM_ICON_SIZE_SMALL;

		y += 4;
	}
	else
	{
		size = PM_ICON_SIZE_NORMAL;
	}

	if ( cg.snap->ps.persistant[ PERS_RESPAWNS_LEFT ] >= 0 )
	{
		y -= 20;
	}

	if ( !cg_pmWaitingList )
	{
		return;
	}

	t = cg_pmWaitingList->time + CG_TimeForPopup ( cg_pmWaitingList->type ) + PM_WAITTIME;

	if ( cg.time > t )
	{
		colourText[ 3 ] = colour[ 3 ] = 1 - ( ( cg.time - t ) / ( float ) PM_FADETIME );
	}

	trap_R_SetColor ( colourText );
	CG_DrawPic ( 4, y, size, size, cg_pmWaitingList->shader );
	trap_R_SetColor ( NULL );
	CG_Text_Paint_Ext ( 4 + size + 2, y + 12, 0.2f, 0.2f, colourText, cg_pmWaitingList->message, 0, 0, 0, &cgs.media.limboFont2 );

	for ( i = 0; i < 4 && listItem; i++, listItem = listItem->next )
	{
		y -= size + 2;

		t = listItem->time + CG_TimeForPopup ( listItem->type ) + PM_WAITTIME;

		if ( cg.time > t )
		{
			colourText[ 3 ] = colour[ 3 ] = 1 - ( ( cg.time - t ) / ( float ) PM_FADETIME );
		}
		else
		{
			colourText[ 3 ] = colour[ 3 ] = 1.f;
		}

		trap_R_SetColor ( colourText );
		CG_DrawPic ( 4, y, size, size, listItem->shader );
		trap_R_SetColor ( NULL );
		CG_Text_Paint_Ext ( 4 + size + 2, y + 12, 0.2f, 0.2f, colourText, listItem->message, 0, 0, 0, &cgs.media.limboFont2 );
	}
}

void CG_DrawPMItemsBig ( void )
{
	vec4_t colour = { 0.f, 0.f, 0.f, 1.f };
	vec4_t colourText = { 1.f, 1.f, 1.f, 1.f };
	float  t;
	float  y = 270;
	float  w;

	if ( !cg_pmWaitingListBig )
	{
		return;
	}

	t = cg_pmWaitingListBig->time + CG_TimeForBigPopup ( cg_pmWaitingListBig->type ) + PM_WAITTIME_BIG;

	if ( cg.time > t )
	{
		colourText[ 3 ] = colour[ 3 ] = 1 - ( ( cg.time - t ) / ( float ) PM_FADETIME_BIG );
	}

	trap_R_SetColor ( colourText );
	CG_DrawPic ( 640 - 56, y, 48, 48, cg_pmWaitingListBig->shader );
	trap_R_SetColor ( NULL );

	w = CG_Text_Width_Ext ( cg_pmWaitingListBig->message, 0.22f, 0, &cgs.media.limboFont2 );
	CG_Text_Paint_Ext ( 640 - 4 - w, y + 56, 0.22f, 0.24f, colourText, cg_pmWaitingListBig->message, 0, 0, 0,
	                    &cgs.media.limboFont2 );
}

const char     *CG_GetPMItemText ( centity_t *cent )
{
	switch ( cent->currentState.effect1Time )
	{
		case PM_DYNAMITE:
			switch ( cent->currentState.effect2Time )
			{
				case 0:
					return va ( "Planted at %s.", CG_ConfigString ( CS_OID_TRIGGERS + cent->currentState.effect3Time ) );

				case 1:
					return va ( "Defused at %s.", CG_ConfigString ( CS_OID_TRIGGERS + cent->currentState.effect3Time ) );
			}

			break;

		case PM_CONSTRUCTION:
			switch ( cent->currentState.effect2Time )
			{
				case -1:
					return CG_ConfigString ( CS_STRINGS + cent->currentState.effect3Time );

				case 0:
					return va ( "%s has been constructed.", CG_ConfigString ( CS_OID_TRIGGERS + cent->currentState.effect3Time ) );
			}

			break;

		case PM_DESTRUCTION:
			switch ( cent->currentState.effect2Time )
			{
				case 0:
					return va ( "%s has been damaged.", CG_ConfigString ( CS_OID_TRIGGERS + cent->currentState.effect3Time ) );

				case 1:
					return va ( "%s has been destroyed.", CG_ConfigString ( CS_OID_TRIGGERS + cent->currentState.effect3Time ) );
			}

			break;

		case PM_MINES:

			// CHRUKER: b009 - Prevent spectators from being informed when a mine is spotted
			if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_SPECTATOR )
			{
				return NULL;
			}

			if ( cgs.clientinfo[ cg.clientNum ].team == cent->currentState.effect2Time )
			{
				return NULL;
			}

			return va ( "Spotted by %s^7 at %s", cgs.clientinfo[ cent->currentState.effect3Time ].name,
			            BG_GetLocationString ( cent->currentState.origin ) );

		case PM_OBJECTIVE:
			switch ( cent->currentState.density )
			{
				case 0:
					return va ( "%s have stolen %s!", cent->currentState.effect2Time == TEAM_ALLIES ? "Allies" : "Axis",
					            CG_ConfigString ( CS_STRINGS + cent->currentState.effect3Time ) );

				case 1:
					return va ( "%s have returned %s!", cent->currentState.effect2Time == TEAM_ALLIES ? "Allies" : "Axis",
					            CG_ConfigString ( CS_STRINGS + cent->currentState.effect3Time ) );
			}

			break;

		case PM_TEAM:
			switch ( cent->currentState.density )
			{
				case 0: // joined
					{
						const char *teamstr = NULL;

						switch ( cent->currentState.effect2Time )
						{
							case TEAM_AXIS:
								teamstr = "Axis team";
								break;

							case TEAM_ALLIES:
								teamstr = "Allied team";
								break;

							default:
								teamstr = "Spectators";
								break;
						}

						return va ( "%s^7 has joined the %s^7!", cgs.clientinfo[ cent->currentState.effect3Time ].name, teamstr );
					}

				case 1:
					return va ( "%s^7 disconnected", cgs.clientinfo[ cent->currentState.effect3Time ].name );
			}
	}

	return NULL;
}

void CG_PlayPMItemSound ( centity_t *cent )
{
	switch ( cent->currentState.effect1Time )
	{
		case PM_DYNAMITE:
			switch ( cent->currentState.effect2Time )
			{
				case 0:
					if ( cent->currentState.teamNum == TEAM_AXIS )
					{
						CG_SoundPlaySoundScript ( "axis_hq_dynamite_planted", NULL, -1, qtrue );
					}
					else
					{
						CG_SoundPlaySoundScript ( "allies_hq_dynamite_planted", NULL, -1, qtrue );
					}

					break;

				case 1:
					if ( cent->currentState.teamNum == TEAM_AXIS )
					{
						CG_SoundPlaySoundScript ( "axis_hq_dynamite_defused", NULL, -1, qtrue );
					}
					else
					{
						CG_SoundPlaySoundScript ( "allies_hq_dynamite_defused", NULL, -1, qtrue );
					}

					break;
			}

			break;

		case PM_MINES:

			// CHRUKER: b009 - Prevent spectators from being informed when a mine is spotted
			if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_SPECTATOR )
			{
				break;
			}

			if ( cgs.clientinfo[ cg.clientNum ].team != cent->currentState.effect2Time )
			{
				// inverted teams
				if ( cent->currentState.effect2Time == TEAM_AXIS )
				{
					CG_SoundPlaySoundScript ( "allies_hq_mines_spotted", NULL, -1, qtrue );
				}
				else
				{
					CG_SoundPlaySoundScript ( "axis_hq_mines_spotted", NULL, -1, qtrue );
				}
			}

			break;

		case PM_OBJECTIVE:
			switch ( cent->currentState.density )
			{
				case 0:
					if ( cent->currentState.effect2Time == TEAM_AXIS )
					{
						CG_SoundPlaySoundScript ( "axis_hq_objective_taken", NULL, -1, qtrue );
					}
					else
					{
						CG_SoundPlaySoundScript ( "allies_hq_objective_taken", NULL, -1, qtrue );
					}

					break;

				case 1:
					if ( cent->currentState.effect2Time == TEAM_AXIS )
					{
						CG_SoundPlaySoundScript ( "axis_hq_objective_secure", NULL, -1, qtrue );
					}
					else
					{
						CG_SoundPlaySoundScript ( "allies_hq_objective_secure", NULL, -1, qtrue );
					}

					break;
			}

			break;

		default:
			break;
	}
}

qhandle_t CG_GetPMItemIcon ( centity_t *cent )
{
	switch ( cent->currentState.effect1Time )
	{
		case PM_CONSTRUCTION:
			if ( cent->currentState.density == TEAM_AXIS )
			{
				return cgs.media.pmImageAxisConstruct;
			}

			return cgs.media.pmImageAlliesConstruct;

		case PM_MINES:
			if ( cent->currentState.effect2Time == TEAM_AXIS )
			{
				return cgs.media.pmImageAlliesMine;
			}

			return cgs.media.pmImageAxisMine;

		default:
			return cgs.media.pmImages[ cent->currentState.effect1Time ];
	}

	return 0;
}

void CG_DrawKeyHint ( rectDef_t *rect, const char *binding )
{
	/*  int k1, k2;
	        char buffer[256];
	        char k[2] = { 0, 0 };
	        float w;

	        trap_Key_KeysForBinding( binding, &k1, &k2 );

	        if( k1 != -1 ) {
	                trap_Key_KeynumToStringBuf( k1, buffer, 256 );
	                if( strlen( buffer ) != 1 ) {
	                        if( k2 != -1 ) {
	                                trap_Key_KeynumToStringBuf( k2, buffer, 256 );
	                                if( strlen( buffer ) == 1 ) {
	                                        *k = toupper( *buffer );
	                                }
	                        }
	                } else {
	                        *k = toupper( *buffer );
	                }
	        }

	        if( !*k ) {
	                return;
	        }

	        CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.hintKey );

	        w = CG_Text_Width_Ext( k, 0.2f, 0, &cgs.media.limboFont1 );
	        CG_Text_Paint_Ext( rect->x + ((rect->w - w) * 0.5f), rect->y + 14, 0.2f, 0.2f, colorWhite, k, 0, 0, 0, &cgs.media.limboFont1 );*/
}
