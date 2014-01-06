/*
===========================================================================
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_local.h"

static entityPos_t entityPositions;

#define HUMAN_SCANNER_UPDATE_PERIOD 700

/*
=============
CG_UpdateRadarVisibility

Update radar visibility of all entities
=============
*/
static void CG_UpdateRadarVisibility( void ) {
	centity_t *cent = NULL;
	int       i;
	int       msec;

	msec = cg.time - cg.oldTime;

	for ( i = 0; i < cg.snap->numEntities; i++ )
	{
		cent = &cg_entities[ cg.snap->entities[ i ].number ];

		if ( cent->currentState.eType == ET_BUILDABLE &&
		     !( cent->currentState.eFlags & EF_DEAD ) ) {
			const buildableAttributes_t *bld = BG_Buildable( cent->currentState.modelindex );
			float fadeOut = bld->radarFadeOut;

			if( fadeOut <= 0.0f ||
			    cent->currentState.modelindex2 == cg.predictedPlayerState.persistant[ PERS_TEAM ] ||
			    cent->buildableAnim != bld->idleAnim ) {
				cent->radarVisibility = 1.0f;
			} else {
				cent->radarVisibility = MAX( cent->radarVisibility - fadeOut * msec, 0.0f );
			}
		} else if ( cent->currentState.eType == ET_PLAYER ) {
			float fadeOut = BG_Class( (cent->currentState.misc >> 8) & 0xff )->radarFadeOut;
			clientInfo_t *ci = &cgs.clientinfo[ cent->currentState.clientNum ];

			if ( !(cent->currentState.eFlags & EF_POWER_AVAILABLE ) ) {
				cent->radarVisibility = 1.0f;
			} else if ( ci->nonsegmented || ci->gender == GENDER_NEUTER ) {
				switch( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) {
				case NSPA_STAND:
				case NSPA_DEATH1:
				case NSPA_DEAD1:
				case NSPA_DEATH2:
				case NSPA_DEAD2:
				case NSPA_DEATH3:
				case NSPA_DEAD3:
					cent->radarVisibility = MAX( cent->radarVisibility - fadeOut * msec, 0.0f );
					break;
				default:
					cent->radarVisibility = 1.0f;
					break;
				}
			} else {
				switch( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) {
				case LEGS_IDLE:
				case LEGS_IDLECR:
				case BOTH_DEATH1:
				case BOTH_DEAD1:
				case BOTH_DEATH2:
				case BOTH_DEAD2:
				case BOTH_DEATH3:
				case BOTH_DEAD3:
					switch( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) {
					case BOTH_DEATH1:
					case BOTH_DEAD1:
					case BOTH_DEATH2:
					case BOTH_DEAD2:
					case BOTH_DEATH3:
					case BOTH_DEAD3:
					case TORSO_STAND:
					case TORSO_STAND_BLASTER:
						cent->radarVisibility = MAX( cent->radarVisibility - fadeOut * msec, 0.0f );
						break;
					default:
						cent->radarVisibility = 1.0f;
						break;
					}
					break;
				default:
					cent->radarVisibility = 1.0f;
					break;
				}
			}
		}
	}
}

/*
=============
CG_UpdateEntityPositions

Update this client's perception of entity positions
=============
*/
void CG_UpdateEntityPositions( void )
{
	centity_t *cent = NULL;
	int       i;

	CG_UpdateRadarVisibility();

	if ( cg.predictedPlayerState.persistant[ PERS_TEAM ] == TEAM_HUMANS )
	{
		if ( entityPositions.lastUpdateTime + HUMAN_SCANNER_UPDATE_PERIOD > cg.time )
		{
			return;
		}
	}

	VectorCopy( cg.refdef.vieworg, entityPositions.origin );
	VectorCopy( cg.refdefViewAngles, entityPositions.vangles );
	entityPositions.lastUpdateTime = cg.time;

	entityPositions.numAlienBuildables = 0;
	entityPositions.numHumanBuildables = 0;
	entityPositions.numAlienClients = 0;
	entityPositions.numHumanClients = 0;

	for ( i = 0; i < cg.snap->numEntities; i++ )
	{
		cent = &cg_entities[ cg.snap->entities[ i ].number ];

		if( !trap_R_inPVVS( entityPositions.origin, cent->lerpOrigin ) )
			continue;

		if ( cent->currentState.eType == ET_BUILDABLE &&
		     !( cent->currentState.eFlags & EF_DEAD ) )
		{
			// add to list of item positions (for creep)
			if ( cent->currentState.modelindex2 == TEAM_ALIENS && entityPositions.numAlienBuildables < MAX_GENTITIES )
			{
				VectorCopy( cent->lerpOrigin, entityPositions.alienBuildablePos[ entityPositions.numAlienBuildables ] );
				entityPositions.alienBuildableIntensity[ entityPositions.numAlienBuildables ] = cent->radarVisibility;
				entityPositions.numAlienBuildables++;
			}
			else if ( cent->currentState.modelindex2 == TEAM_HUMANS && entityPositions.numHumanBuildables < MAX_GENTITIES )
			{
				VectorCopy( cent->lerpOrigin, entityPositions.humanBuildablePos[ entityPositions.numHumanBuildables ] );
				entityPositions.humanBuildableIntensity[ entityPositions.numHumanBuildables ] = cent->radarVisibility;
				entityPositions.numHumanBuildables++;
			}
		}
		else if ( cent->currentState.eType == ET_PLAYER )
		{
			int team = cent->currentState.misc & 0x00FF;

			if ( team == TEAM_ALIENS && entityPositions.numAlienClients < MAX_CLIENTS )
			{
				VectorCopy( cent->lerpOrigin, entityPositions.alienClientPos[ entityPositions.numAlienClients ] );
				entityPositions.alienClientIntensity[ entityPositions.numAlienClients ] = cent->radarVisibility;
				entityPositions.numAlienClients++;
			}
			else if ( team == TEAM_HUMANS && entityPositions.numHumanClients < MAX_CLIENTS )
			{
				VectorCopy( cent->lerpOrigin, entityPositions.humanClientPos[ entityPositions.numHumanClients ] );
				entityPositions.humanClientIntensity[ entityPositions.numHumanClients ] = cent->radarVisibility;
				entityPositions.numHumanClients++;
			}
		}
	}
}

#define STALKWIDTH ( 2.0f * cgDC.aspectScale )
#define BLIPX      ( 16.0f * cgDC.aspectScale )
#define BLIPY      8.0f
#define FAR_ALPHA  0.8f
#define NEAR_ALPHA 1.2f

/*
=============
CG_DrawBlips

Draw blips and stalks for the human scanner
=============
*/
static void CG_DrawBlips( rectDef_t *rect, vec3_t origin, vec4_t colour, qhandle_t shader )
{
	vec3_t drawOrigin;
	vec3_t up = { 0, 0, 1 };
	float  alphaMod = 1.0f;
	float  timeFractionSinceRefresh = 1.0f -
	                                  ( ( float )( cg.time - entityPositions.lastUpdateTime ) /
	                                    ( float ) HUMAN_SCANNER_UPDATE_PERIOD );
	vec4_t localColour;

	Vector4Copy( colour, localColour );

	RotatePointAroundVector( drawOrigin, up, origin, -entityPositions.vangles[ 1 ] - 90 );
	drawOrigin[ 0 ] /= ( 2 * RADAR_RANGE / rect->w );
	drawOrigin[ 1 ] /= ( 2 * RADAR_RANGE / rect->h );
	drawOrigin[ 2 ] /= ( 2 * RADAR_RANGE / rect->w );

	alphaMod = FAR_ALPHA +
	           ( ( drawOrigin[ 1 ] + ( rect->h / 2.0f ) ) / rect->h ) * ( NEAR_ALPHA - FAR_ALPHA );

	localColour[ 3 ] *= alphaMod;
	localColour[ 3 ] *= ( 0.5f + ( timeFractionSinceRefresh * 0.5f ) );

	if ( localColour[ 3 ] > 1.0f )
	{
		localColour[ 3 ] = 1.0f;
	}
	else if ( localColour[ 3 ] < 0.0f )
	{
		localColour[ 3 ] = 0.0f;
	}

	trap_R_SetColor( localColour );

	if ( drawOrigin[ 2 ] > 0 )
	{
		CG_DrawPic( rect->x + ( rect->w / 2 ) - ( STALKWIDTH / 2 ) - drawOrigin[ 0 ],
		            rect->y + ( rect->h / 2 ) + drawOrigin[ 1 ] - drawOrigin[ 2 ],
		            STALKWIDTH, drawOrigin[ 2 ], cgs.media.scannerLineShader );
	}
	else
	{
		CG_DrawPic( rect->x + ( rect->w / 2 ) - ( STALKWIDTH / 2 ) - drawOrigin[ 0 ],
		            rect->y + ( rect->h / 2 ) + drawOrigin[ 1 ],
		            STALKWIDTH, -drawOrigin[ 2 ], cgs.media.scannerLineShader );
	}

	CG_DrawPic( rect->x + ( rect->w / 2 ) - ( BLIPX / 2 ) - drawOrigin[ 0 ],
	            rect->y + ( rect->h / 2 ) - ( BLIPY / 2 ) + drawOrigin[ 1 ] - drawOrigin[ 2 ],
	            BLIPX, BLIPY, shader );
	trap_R_SetColor( NULL );
}

#define BLIPX2 ( 36.0f * cgDC.aspectScale )
#define BLIPY2 36.0f

/*
=============
CG_DrawDir

Draw dot marking the direction to an enemy
=============
*/
static void CG_DrawDir( rectDef_t *rect, vec3_t origin, vec4_t colour, qhandle_t shader )
{
	vec3_t        drawOrigin;
	vec3_t        noZOrigin;
	vec3_t        normal, antinormal, normalDiff;
	vec3_t        view, noZview;
	vec3_t        up = { 0.0f, 0.0f,   1.0f };
	vec3_t        top = { 0.0f, -1.0f,  0.0f };
	float         angle;
	playerState_t *ps = &cg.snap->ps;

	BG_GetClientNormal( ps, normal );

	AngleVectors( entityPositions.vangles, view, NULL, NULL );

	ProjectPointOnPlane( noZOrigin, origin, normal );
	ProjectPointOnPlane( noZview, view, normal );
	VectorNormalize( noZOrigin );
	VectorNormalize( noZview );

	//calculate the angle between the images of the blip and the view
	angle = RAD2DEG( acos( DotProduct( noZOrigin, noZview ) ) );
	CrossProduct( noZOrigin, noZview, antinormal );
	VectorNormalize( antinormal );

	//decide which way to rotate
	VectorSubtract( normal, antinormal, normalDiff );

	if ( VectorLength( normalDiff ) < 1.0f )
	{
		angle = 360.0f - angle;
	}

	RotatePointAroundVector( drawOrigin, up, top, angle );

	trap_R_SetColor( colour );
	CG_DrawPic( rect->x + ( rect->w / 2 ) - ( BLIPX2 / 2 ) - drawOrigin[ 0 ] * ( rect->w / 2 ),
	            rect->y + ( rect->h / 2 ) - ( BLIPY2 / 2 ) + drawOrigin[ 1 ] * ( rect->h / 2 ),
	            BLIPX2, BLIPY2, shader ); // cgs.media.scannerBlipShader
	trap_R_SetColor( NULL );
}

/*
=============
CG_AlienSense
=============
*/
void CG_AlienSense( rectDef_t *rect )
{
	int    i;
	vec3_t origin;
	vec3_t relOrigin;
	vec4_t  color_human = { 0.04f, 0.71f, 0.88f, 1.0f };
	vec4_t  color_alien = { 0.75f, 0.00f, 0.00f, 1.0f };
	float  length;

	VectorCopy( entityPositions.origin, origin );

	//draw human buildables
	for ( i = 0; i < entityPositions.numHumanBuildables; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.humanBuildablePos[ i ], origin, relOrigin );
		length = VectorLength( relOrigin );

		if ( length < ALIENSENSE_RANGE )
		{
			color_human[3] = 1.f - length / ALIENSENSE_RANGE;
			color_human[3] *= entityPositions.humanBuildableIntensity[ i ];
			CG_DrawDir( rect, relOrigin, color_human, cgs.media.scannerBlipBldgShader );
		}
	}

	//draw human clients
	for ( i = 0; i < entityPositions.numHumanClients; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.humanClientPos[ i ], origin, relOrigin );
		length = VectorLength( relOrigin );

		if ( length < ALIENSENSE_RANGE )
		{
			color_human[3] = 1.f - length / ALIENSENSE_RANGE;
			color_human[3] *= entityPositions.humanClientIntensity[ i ];
			CG_DrawDir( rect, relOrigin, color_human, cgs.media.scannerBlipShader );
		}
	}

	//draw alien buildables
	for ( i = 0; i < entityPositions.numAlienBuildables; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.alienBuildablePos[ i ], origin, relOrigin );
		length = VectorLength( relOrigin );

		if ( length < ALIENSENSE_RANGE )
		{
			color_alien[3] = 1.f - length / ALIENSENSE_RANGE;
			color_alien[3] *= entityPositions.alienBuildableIntensity[ i ];
			CG_DrawDir( rect, relOrigin, color_alien, cgs.media.scannerBlipBldgShader );
		}
	}

	//draw alien clients
	for ( i = 0; i < entityPositions.numAlienClients; i++ )
	{ 
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.alienClientPos[ i ], origin, relOrigin );

		length = VectorLength( relOrigin );

		if ( length < ALIENSENSE_RANGE )
		{
			color_alien[3] = 1.f - length / ALIENSENSE_RANGE;
			color_alien[3] *= entityPositions.alienClientIntensity[ i ];
			CG_DrawDir( rect, relOrigin, color_alien, cgs.media.scannerBlipShader );
		}
	}
}

/*
=============
CG_Scanner
=============
*/
void CG_Scanner( rectDef_t *rect, qhandle_t shader, vec4_t color )
{
	int    i;
	vec3_t origin;
	vec3_t relOrigin;
	vec4_t  color_human = { 0.04f, 0.71f, 0.88f, 1.0f };
	vec4_t  color_alien = { 0.75f, 0.00f, 0.00f, 1.0f };

	VectorCopy( entityPositions.origin, origin );

	//draw human buildables below scanner plane
	color_human[3] = color[3];

	for ( i = 0; i < entityPositions.numHumanBuildables; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.humanBuildablePos[ i ], origin, relOrigin );

		if ( VectorLength( relOrigin ) < RADAR_RANGE && ( relOrigin[ 2 ] < 0 ) )
		{
			color_human[3] = color[3] * entityPositions.humanBuildableIntensity[ i ];
			CG_DrawBlips( rect, relOrigin, color_human, cgs.media.scannerBlipBldgShader );
		}
	}

	//draw alien buildables below scanner plane
	color_alien[3] = color[3];

	for ( i = 0; i < entityPositions.numAlienBuildables; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.alienBuildablePos[ i ], origin, relOrigin );

		if ( VectorLength( relOrigin ) < RADAR_RANGE && ( relOrigin[ 2 ] < 0 ) )
		{
			color_alien[3] = color[3] * entityPositions.alienBuildableIntensity[ i ];
			CG_DrawBlips( rect, relOrigin, color_alien, cgs.media.scannerBlipBldgShader );
		}
	}

	//draw human clients below scanner plane
	for ( i = 0; i < entityPositions.numHumanClients; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.humanClientPos[ i ], origin, relOrigin );

		if ( VectorLength( relOrigin ) < RADAR_RANGE && ( relOrigin[ 2 ] < 0 ) )
		{
			color_human[3] = color[3] * entityPositions.humanClientIntensity[ i ];

			CG_DrawBlips( rect, relOrigin, color_human, cgs.media.scannerBlipShader );
		}
	}

	//draw alien clients below scanner plane
	for ( i = 0; i < entityPositions.numAlienClients; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.alienClientPos[ i ], origin, relOrigin );

		if ( VectorLength( relOrigin ) < RADAR_RANGE && ( relOrigin[ 2 ] < 0 ) )
		{
			color_alien[3] = color[3] * entityPositions.alienClientIntensity[ i ];
			CG_DrawBlips( rect, relOrigin, color_alien, cgs.media.scannerBlipShader );
		}
	}

	if ( !cg_disableScannerPlane.integer )
	{
		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
	}

	//draw human buildables above scanner plane
	for ( i = 0; i < entityPositions.numHumanBuildables; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.humanBuildablePos[ i ], origin, relOrigin );

		if ( VectorLength( relOrigin ) < RADAR_RANGE && ( relOrigin[ 2 ] > 0 ) )
		{
			color_human[3] = 1.5f * color[3] * entityPositions.humanBuildableIntensity[ i ];
			CG_DrawBlips( rect, relOrigin, color_human, cgs.media.scannerBlipBldgShader );
		}
	}

	//draw alien buildables above scanner plane
	for ( i = 0; i < entityPositions.numAlienBuildables; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.alienBuildablePos[ i ], origin, relOrigin );

		if ( VectorLength( relOrigin ) < RADAR_RANGE && ( relOrigin[ 2 ] > 0 ) )
		{
			color_alien[3] = 1.5f * color[3] * entityPositions.alienBuildableIntensity[ i ];
			CG_DrawBlips( rect, relOrigin, color_alien, cgs.media.scannerBlipBldgShader );
		}
	}

	//draw human clients above scanner plane
	for ( i = 0; i < entityPositions.numHumanClients; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.humanClientPos[ i ], origin, relOrigin );

		if ( VectorLength( relOrigin ) < RADAR_RANGE && ( relOrigin[ 2 ] > 0 ) )
		{
			color_human[3] = 1.5f * color[3] * entityPositions.humanClientIntensity[ i ];
			CG_DrawBlips( rect, relOrigin, color_human, cgs.media.scannerBlipShader );
		}
	}

	//draw alien clients above scanner plane
	for ( i = 0; i < entityPositions.numAlienClients; i++ )
	{
		VectorClear( relOrigin );
		VectorSubtract( entityPositions.alienClientPos[ i ], origin, relOrigin );

		if ( VectorLength( relOrigin ) < RADAR_RANGE && ( relOrigin[ 2 ] > 0 ) )
		{
			color_alien[3] = 1.5f * color[3] * entityPositions.alienClientIntensity[ i ];
			CG_DrawBlips( rect, relOrigin, color_alien, cgs.media.scannerBlipShader );
		}
	}
}
