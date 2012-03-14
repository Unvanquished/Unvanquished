/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

// cg_weapons.c -- events and effects dealing with weapons

#include "cg_local.h"

/*
=================
CG_RegisterUpgrade

The server says this item is used on this level
=================
*/
void CG_RegisterUpgrade( int upgradeNum )
{
	upgradeInfo_t *upgradeInfo;
	char          *icon;

	upgradeInfo = &cg_upgrades[ upgradeNum ];

	if ( upgradeNum == 0 )
	{
		return;
	}

	if ( upgradeInfo->registered )
	{
		return;
	}

	memset( upgradeInfo, 0, sizeof( *upgradeInfo ) );
	upgradeInfo->registered = qtrue;

	if ( !BG_FindNameForUpgrade( upgradeNum ) )
	{
		CG_Error( "Couldn't find upgrade %i", upgradeNum );
	}

	upgradeInfo->humanName = BG_FindHumanNameForUpgrade( upgradeNum );

	//la la la la la, i'm not listening!
	if ( upgradeNum == UP_GRENADE )
	{
		upgradeInfo->upgradeIcon = cg_weapons[ WP_GRENADE ].weaponIcon;
	}
	else if ( ( icon = BG_FindIconForUpgrade( upgradeNum ) ) )
	{
		upgradeInfo->upgradeIcon = trap_R_RegisterShader( icon );
	}
}

/*
===============
CG_InitUpgrades

Precaches upgrades
===============
*/
void CG_InitUpgrades( void )
{
	int i;

	memset( cg_upgrades, 0, sizeof( cg_upgrades ) );

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		CG_RegisterUpgrade( i );
	}
}

/*
===============
CG_ParseWeaponModeSection

Parse a weapon mode section
===============
*/
static qboolean CG_ParseWeaponModeSection( weaponInfoMode_t *wim, char **text_p )
{
	char *token;
	int  i;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( text_p );

		if ( !token )
		{
			break;
		}

		if ( !Q_stricmp( token, "" ) )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, "missileModel" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileModel = trap_R_RegisterModel( token );

			if ( !wim->missileModel )
			{
				CG_Printf( S_COLOR_RED "ERROR: missile model not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "missileSprite" ) )
		{
			int size = 0;

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			size = atoi( token );

			if ( size < 0 )
			{
				size = 0;
			}

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileSprite     = trap_R_RegisterShader( token );
			wim->missileSpriteSize = size;
			wim->usesSpriteMissle  = qtrue;

			if ( !wim->missileSprite )
			{
				CG_Printf( S_COLOR_RED "ERROR: missile sprite not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "missileRotates" ) )
		{
			wim->missileRotates = qtrue;

			continue;
		}
		else if ( !Q_stricmp( token, "missileAnimates" ) )
		{
			wim->missileAnimates = qtrue;

			token                = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileAnimStartFrame = atoi( token );

			token                      = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileAnimNumFrames = atoi( token );

			token                     = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileAnimFrameRate = atoi( token );

			token                     = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileAnimLooping = atoi( token );

			continue;
		}
		else if ( !Q_stricmp( token, "missileParticleSystem" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileParticleSystem = CG_RegisterParticleSystem( token );

			if ( !wim->missileParticleSystem )
			{
				CG_Printf( S_COLOR_RED "ERROR: missile particle system not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "missileTrailSystem" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileTrailSystem = CG_RegisterTrailSystem( token );

			if ( !wim->missileTrailSystem )
			{
				CG_Printf( S_COLOR_RED "ERROR: missile trail system not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "muzzleParticleSystem" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->muzzleParticleSystem = CG_RegisterParticleSystem( token );

			if ( !wim->muzzleParticleSystem )
			{
				CG_Printf( S_COLOR_RED "ERROR: muzzle particle system not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "impactParticleSystem" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->impactParticleSystem = CG_RegisterParticleSystem( token );

			if ( !wim->impactParticleSystem )
			{
				CG_Printf( S_COLOR_RED "ERROR: impact particle system not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "impactMark" ) )
		{
			int size = 0;

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			size = atoi( token );

			if ( size < 0 )
			{
				size = 0;
			}

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->impactMark     = trap_R_RegisterShader( token );
			wim->impactMarkSize = size;

			if ( !wim->impactMark )
			{
				CG_Printf( S_COLOR_RED "ERROR: impact mark shader not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "impactSound" ) )
		{
			int index = 0;

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			index = atoi( token );

			if ( index < 0 )
			{
				index = 0;
			}
			else if ( index > 3 )
			{
				index = 3;
			}

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->impactSound[ index ] = trap_S_RegisterSound( token, qfalse );

			continue;
		}
		else if ( !Q_stricmp( token, "impactFleshSound" ) )
		{
			int index = 0;

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			index = atoi( token );

			if ( index < 0 )
			{
				index = 0;
			}
			else if ( index > 3 )
			{
				index = 3;
			}

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->impactFleshSound[ index ] = trap_S_RegisterSound( token, qfalse );

			continue;
		}
		else if ( !Q_stricmp( token, "alwaysImpact" ) )
		{
			wim->alwaysImpact = qtrue;

			continue;
		}
		else if ( !Q_stricmp( token, "flashDLightColor" ) )
		{
			for ( i = 0; i < 3; i++ )
			{
				token = COM_Parse( text_p );

				if ( !token )
				{
					break;
				}

				wim->flashDlightColor[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "continuousFlash" ) )
		{
			wim->continuousFlash = qtrue;

			continue;
		}
		else if ( !Q_stricmp( token, "missileDlightColor" ) )
		{
			for ( i = 0; i < 3; i++ )
			{
				token = COM_Parse( text_p );

				if ( !token )
				{
					break;
				}

				wim->missileDlightColor[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "missileDlight" ) )
		{
			int size = 0;

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			size = atoi( token );

			if ( size < 0 )
			{
				size = 0;
			}

			wim->missileDlight = size;

			continue;
		}
		else if ( !Q_stricmp( token, "firingSound" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->firingSound = trap_S_RegisterSound( token, qfalse );

			continue;
		}
		else if ( !Q_stricmp( token, "missileSound" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->missileSound = trap_S_RegisterSound( token, qfalse );

			continue;
		}
		else if ( !Q_stricmp( token, "flashSound" ) )
		{
			int index = 0;

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			index = atoi( token );

			if ( index < 0 )
			{
				index = 0;
			}
			else if ( index > 3 )
			{
				index = 3;
			}

			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->flashSound[ index ] = trap_S_RegisterSound( token, qfalse );

			continue;
		}
		else if ( !Q_stricmp( token, "}" ) )
		{
			return qtrue; //reached the end of this weapon section
		}
		else
		{
			CG_Printf( S_COLOR_RED "ERROR: unknown token '%s' in weapon section\n", token );
			return qfalse;
		}
	}

	return qfalse;
}

/*
======================
CG_ParseWeaponFile

Parses a configuration file describing a weapon
======================
*/
static qboolean CG_ParseWeaponFile( const char *filename, weaponInfo_t *wi )
{
	char         *text_p;
	int          len;
	char         *token;
	char         text[ 20000 ];
	fileHandle_t f;
	weaponMode_t weaponMode = WPM_NONE;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len <= 0 )
	{
		return qfalse;
	}

	if ( len >= sizeof( text ) - 1 )
	{
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( &text_p );

		if ( !token )
		{
			break;
		}

		if ( !Q_stricmp( token, "" ) )
		{
			break;
		}

		if ( !Q_stricmp( token, "{" ) )
		{
			if ( weaponMode == WPM_NONE )
			{
				CG_Printf( S_COLOR_RED "ERROR: weapon mode section started without a declaration\n" );
				return qfalse;
			}
			else if ( !CG_ParseWeaponModeSection( &wi->wim[ weaponMode ], &text_p ) )
			{
				CG_Printf( S_COLOR_RED "ERROR: failed to parse weapon mode section\n" );
				return qfalse;
			}

			//start parsing ejectors again
			weaponMode = WPM_NONE;

			continue;
		}
		else if ( !Q_stricmp( token, "primary" ) )
		{
			weaponMode = WPM_PRIMARY;
			continue;
		}
		else if ( !Q_stricmp( token, "secondary" ) )
		{
			weaponMode = WPM_SECONDARY;
			continue;
		}
		else if ( !Q_stricmp( token, "tertiary" ) )
		{
			weaponMode = WPM_TERTIARY;
			continue;
		}
		else if ( !Q_stricmp( token, "weaponModel" ) )
		{
			char path[ MAX_QPATH ];

			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->weaponModel = trap_R_RegisterModel( token );

			if ( !wi->weaponModel )
			{
				CG_Printf( S_COLOR_RED "ERROR: weapon model not found %s\n", token );
			}

			strcpy( path, token );
			COM_StripExtension( path, path );
			strcat( path, "_flash.md3" );
			wi->flashModel = trap_R_RegisterModel( path );

			strcpy( path, token );
			COM_StripExtension( path, path );
			strcat( path, "_barrel.md3" );
			wi->barrelModel = trap_R_RegisterModel( path );

			strcpy( path, token );
			COM_StripExtension( path, path );
			strcat( path, "_hand.md3" );
			wi->handsModel = trap_R_RegisterModel( path );

			if ( !wi->handsModel )
			{
				wi->handsModel = trap_R_RegisterModel( "models/weapons2/shotgun/shotgun_hand.md3" );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "idleSound" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->readySound = trap_S_RegisterSound( token, qfalse );

			continue;
		}
		else if ( !Q_stricmp( token, "icon" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->weaponIcon = wi->ammoIcon = trap_R_RegisterShader( token );

			if ( !wi->weaponIcon )
			{
				CG_Printf( S_COLOR_RED "ERROR: weapon icon not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "crosshair" ) )
		{
			int size = 0;

			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			size = atoi( token );

			if ( size < 0 )
			{
				size = 0;
			}

			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->crossHair     = trap_R_RegisterShader( token );
			wi->crossHairSize = size;

			if ( !wi->crossHair )
			{
				CG_Printf( S_COLOR_RED "ERROR: weapon crosshair not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "disableIn3rdPerson" ) )
		{
			wi->disableIn3rdPerson = qtrue;

			continue;
		}

		Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
		return qfalse;
	}

	return qtrue;
}

/*
=================
CG_RegisterWeapon
=================
*/
void CG_RegisterWeapon( int weaponNum )
{
	weaponInfo_t *weaponInfo;
	char         path[ MAX_QPATH ];
	vec3_t       mins, maxs;
	int          i;

	weaponInfo = &cg_weapons[ weaponNum ];

	if ( weaponNum == 0 )
	{
		return;
	}

	if ( weaponInfo->registered )
	{
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	if ( !BG_FindNameForWeapon( weaponNum ) )
	{
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}

	Com_sprintf( path, MAX_QPATH, "models/weapons/%s/weapon.cfg", BG_FindNameForWeapon( weaponNum ) );

	weaponInfo->humanName = BG_FindHumanNameForWeapon( weaponNum );

	if ( !CG_ParseWeaponFile( path, weaponInfo ) )
	{
		Com_Printf( S_COLOR_RED "ERROR: failed to parse %s\n", path );
	}

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );

	for ( i = 0; i < 3; i++ )
	{
		weaponInfo->weaponMidpoint[ i ] = mins[ i ] + 0.5 * ( maxs[ i ] - mins[ i ] );
	}

	//FIXME:
	for ( i = WPM_NONE + 1; i < WPM_NUM_WEAPONMODES; i++ )
	{
		weaponInfo->wim[ i ].loopFireSound = qfalse;
	}
}

/*
===============
CG_InitWeapons

Precaches weapons
===============
*/
void CG_InitWeapons( void )
{
	int i;

	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		CG_RegisterWeapon( i );
	}

	cgs.media.level2ZapTS = CG_RegisterTrailSystem( "models/weapons/lev2zap/lightning" );
}

/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame )
{
	// change weapon
	if ( frame >= ci->animations[ TORSO_DROP ].firstFrame &&
	     frame < ci->animations[ TORSO_DROP ].firstFrame + 9 )
	{
		return frame - ci->animations[ TORSO_DROP ].firstFrame + 6;
	}

	// stand attack
	if ( frame >= ci->animations[ TORSO_ATTACK ].firstFrame &&
	     frame < ci->animations[ TORSO_ATTACK ].firstFrame + 6 )
	{
		return 1 + frame - ci->animations[ TORSO_ATTACK ].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[ TORSO_ATTACK2 ].firstFrame &&
	     frame < ci->animations[ TORSO_ATTACK2 ].firstFrame + 6 )
	{
		return 1 + frame - ci->animations[ TORSO_ATTACK2 ].firstFrame;
	}

	return 0;
}

/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles )
{
	float scale;
	int   delta;
	float fracsin;
	float bob;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 )
	{
		scale = -cg.xyspeed;
	}
	else
	{
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	//TA: bob amount is class dependant
	bob = BG_FindBobForClass( cg.predictedPlayerState.stats[ STAT_PCLASS ] );

	if ( bob != 0 )
	{
		angles[ ROLL ]  += scale * cg.bobfracsin * 0.005;
		angles[ YAW ]   += scale * cg.bobfracsin * 0.01;
		angles[ PITCH ] += cg.xyspeed * cg.bobfracsin * 0.005;
	}

	// drop the weapon when landing
	if ( !BG_ClassHasAbility( cg.predictedPlayerState.stats[ STAT_PCLASS ], SCA_NOWEAPONDRIFT ) )
	{
		delta = cg.time - cg.landTime;

		if ( delta < LAND_DEFLECT_TIME )
		{
			origin[ 2 ] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
		}
		else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME )
		{
			origin[ 2 ] += cg.landChange * 0.25 *
			               ( LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta ) / LAND_RETURN_TIME;
		}

		// idle drift
		scale            = cg.xyspeed + 40;
		fracsin          = sin( cg.time * 0.001 );
		angles[ ROLL ]  += scale * fracsin * 0.01;
		angles[ YAW ]   += scale * fracsin * 0.01;
		angles[ PITCH ] += scale * fracsin * 0.01;
	}
}

/*
======================
CG_MachinegunSpinAngle
======================
*/
#define   SPIN_SPEED 0.9
#define   COAST_TIME 1000
static float CG_MachinegunSpinAngle( centity_t *cent, qboolean firing )
{
	int   delta;
	float angle;
	float speed;

	delta = cg.time - cent->pe.barrelTime;

	if ( cent->pe.barrelSpinning )
	{
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	}
	else
	{
		if ( delta > COAST_TIME )
		{
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + ( float )( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !firing )
	{
		cent->pe.barrelTime     = cg.time;
		cent->pe.barrelAngle    = AngleMod( angle );
		cent->pe.barrelSpinning = firing;
	}

	return angle;
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent )
{
	refEntity_t  gun;
	refEntity_t  barrel;
	refEntity_t  flash;
	vec3_t       angles;
	weapon_t     weaponNum;
	weaponMode_t weaponMode;
	weaponInfo_t *weapon;
	qboolean     noGunModel;
	qboolean     firing;

	weaponNum  = cent->currentState.weapon;
	weaponMode = cent->currentState.generic1;

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	if ( ( ( cent->currentState.eFlags & EF_FIRING ) && weaponMode == WPM_PRIMARY ) ||
	     ( ( cent->currentState.eFlags & EF_FIRING2 ) && weaponMode == WPM_SECONDARY ) ||
	     ( ( cent->currentState.eFlags & EF_FIRING3 ) && weaponMode == WPM_TERTIARY ) )
	{
		firing = qtrue;
	}
	else
	{
		firing = qfalse;
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[ weaponNum ];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx    = parent->renderfx;

	// set custom shading for railgun refire rate
	if ( ps )
	{
		gun.shaderRGBA[ 0 ] = 255;
		gun.shaderRGBA[ 1 ] = 255;
		gun.shaderRGBA[ 2 ] = 255;
		gun.shaderRGBA[ 3 ] = 255;

		//set weapon[1/2]Time when respective buttons change state
		if ( cg.weapon1Firing != ( cg.predictedPlayerState.eFlags & EF_FIRING ) )
		{
			cg.weapon1Time   = cg.time;
			cg.weapon1Firing = ( cg.predictedPlayerState.eFlags & EF_FIRING );
		}

		if ( cg.weapon2Firing != ( cg.predictedPlayerState.eFlags & EF_FIRING2 ) )
		{
			cg.weapon2Time   = cg.time;
			cg.weapon2Firing = ( cg.predictedPlayerState.eFlags & EF_FIRING2 );
		}

		if ( cg.weapon3Firing != ( cg.predictedPlayerState.eFlags & EF_FIRING3 ) )
		{
			cg.weapon3Time   = cg.time;
			cg.weapon3Firing = ( cg.predictedPlayerState.eFlags & EF_FIRING3 );
		}
	}

	gun.hModel = weapon->weaponModel;

	noGunModel = ( ( !ps || cg.renderingThirdPerson ) && weapon->disableIn3rdPerson ) || !gun.hModel;

	if ( !ps )
	{
		// add weapon ready sound
		if ( firing && weapon->wim[ weaponMode ].firingSound )
		{
			/*trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
			                        weapon->wim[ weaponMode ].firingSound );*/
		}
		else if ( weapon->readySound )
		{
			//trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
		}
	}

	if ( !noGunModel )
	{
		CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon" );

		trap_R_AddRefEntityToScene( &gun );

		// add the spinning barrel
		if ( weapon->barrelModel )
		{
			memset( &barrel, 0, sizeof( barrel ) );
			VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
			barrel.shadowPlane = parent->shadowPlane;
			barrel.renderfx    = parent->renderfx;

			barrel.hModel      = weapon->barrelModel;
			angles[ YAW ]      = 0;
			angles[ PITCH ]    = 0;
			angles[ ROLL ]     = CG_MachinegunSpinAngle( cent, firing );
			AnglesToAxis( angles, barrel.axis );

			CG_PositionRotatedEntityOnTag( &barrel, &gun, weapon->weaponModel, "tag_barrel" );

			trap_R_AddRefEntityToScene( &barrel );
		}
	}

	if ( CG_IsParticleSystemValid( &cent->muzzlePS ) )
	{
		if ( ps || cg.renderingThirdPerson ||
		     cent->currentState.number != cg.predictedPlayerState.clientNum )
		{
			if ( noGunModel )
			{
				CG_SetAttachmentTag( &cent->muzzlePS->attachment, *parent, parent->hModel, "tag_weapon" );
			}
			else
			{
				CG_SetAttachmentTag( &cent->muzzlePS->attachment, gun, weapon->weaponModel, "tag_flash" );
			}
		}

		//if the PS is infinite disable it when not firing
		if ( !firing && CG_IsParticleSystemInfinite( cent->muzzlePS ) )
		{
			CG_DestroyParticleSystem( &cent->muzzlePS );
		}
	}

	// add the flash
	if ( !weapon->wim[ weaponMode ].continuousFlash || !firing )
	{
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME )
		{
			return;
		}
	}

	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx    = parent->renderfx;

	flash.hModel      = weapon->flashModel;

	if ( flash.hModel )
	{
		angles[ YAW ]   = 0;
		angles[ PITCH ] = 0;
		angles[ ROLL ]  = crandom() * 10;
		AnglesToAxis( angles, flash.axis );

		if ( noGunModel )
		{
			CG_PositionRotatedEntityOnTag( &flash, parent, parent->hModel, "tag_weapon" );
		}
		else
		{
			CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash" );
		}

		trap_R_AddRefEntityToScene( &flash );
	}

	if ( ps || cg.renderingThirdPerson ||
	     cent->currentState.number != cg.predictedPlayerState.clientNum )
	{
		if ( weapon->wim[ weaponMode ].muzzleParticleSystem && cent->muzzlePsTrigger )
		{
			cent->muzzlePS = CG_SpawnNewParticleSystem( weapon->wim[ weaponMode ].muzzleParticleSystem );

			if ( CG_IsParticleSystemValid( &cent->muzzlePS ) )
			{
				if ( noGunModel )
				{
					CG_SetAttachmentTag( &cent->muzzlePS->attachment, *parent, parent->hModel, "tag_weapon" );
				}
				else
				{
					CG_SetAttachmentTag( &cent->muzzlePS->attachment, gun, weapon->weaponModel, "tag_flash" );
				}

				CG_SetAttachmentCent( &cent->muzzlePS->attachment, cent );
				CG_AttachToTag( &cent->muzzlePS->attachment );
			}

			cent->muzzlePsTrigger = qfalse;
		}

		// make a dlight for the flash
		if ( weapon->wim[ weaponMode ].flashDlightColor[ 0 ] ||
		     weapon->wim[ weaponMode ].flashDlightColor[ 1 ] ||
		     weapon->wim[ weaponMode ].flashDlightColor[ 2 ] )
		{
			trap_R_AddLightToScene( flash.origin, 320, 1.25 + ( rand() & 31 ) / 128,
			                        weapon->wim[ weaponMode ].flashDlightColor[ 0 ],
			                        weapon->wim[ weaponMode ].flashDlightColor[ 1 ],
			                        weapon->wim[ weaponMode ].flashDlightColor[ 2 ],
			                        0,
			                        0 );
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps )
{
	refEntity_t  hand;
	centity_t    *cent;
	clientInfo_t *ci;
	float        fovOffset;
	vec3_t       angles;
	weaponInfo_t *wi;
	weapon_t     weapon     = ps->weapon;
	weaponMode_t weaponMode = ps->generic1;

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	CG_RegisterWeapon( weapon );
	wi   = &cg_weapons[ weapon ];
	cent = &cg.predictedPlayerEntity; // &cg_entities[cg.snap->ps.clientNum];

	if ( ( ps->persistant[ PERS_TEAM ] == TEAM_SPECTATOR ) ||
	     ( ps->stats[ STAT_STATE ] & SS_INFESTING ) ||
	     ( ps->stats[ STAT_STATE ] & SS_HOVELING ) )
	{
		return;
	}

	//TA: no weapon carried - can't draw it
	if ( weapon == WP_NONE )
	{
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION )
	{
		return;
	}

	//TA: draw a prospective buildable infront of the player
	if ( ( ps->stats[ STAT_BUILDABLE ] & ~SB_VALID_TOGGLEBIT ) > BA_NONE )
	{
		CG_GhostBuildable( ps->stats[ STAT_BUILDABLE ] & ~SB_VALID_TOGGLEBIT );
	}

	if ( weapon == WP_LUCIFER_CANNON && ps->stats[ STAT_MISC ] > 0 )
	{
		if ( ps->stats[ STAT_MISC ] > ( LCANNON_TOTAL_CHARGE - ( LCANNON_TOTAL_CHARGE / 3 ) ) )
		{
			//trap_S_AddLoopingSound( ps->clientNum, ps->origin, vec3_origin, cgs.media.lCannonWarningSound );
		}
	}

	// no gun if in third person view
	if ( cg.renderingThirdPerson )
	{
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer )
	{
		vec3_t origin;

		VectorCopy( cg.refdef.vieworg, origin );
		VectorMA( origin, -8, cg.refdef.viewaxis[ 2 ], origin );

		if ( cent->muzzlePS )
		{
			CG_SetAttachmentPoint( &cent->muzzlePS->attachment, origin );
		}

		//check for particle systems
		if ( wi->wim[ weaponMode ].muzzleParticleSystem && cent->muzzlePsTrigger )
		{
			cent->muzzlePS = CG_SpawnNewParticleSystem( wi->wim[ weaponMode ].muzzleParticleSystem );

			if ( CG_IsParticleSystemValid( &cent->muzzlePS ) )
			{
				CG_SetAttachmentPoint( &cent->muzzlePS->attachment, origin );
				CG_SetAttachmentCent( &cent->muzzlePS->attachment, cent );
				CG_AttachToPoint( &cent->muzzlePS->attachment );
			}

			cent->muzzlePsTrigger = qfalse;
		}

		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun )
	{
		return;
	}

	// drop gun lower at higher fov
	//if ( cg_fov.integer > 90 ) {
	//TA: the client side variable isn't used ( shouldn't iD have done this anyway? )
	if ( cg.refdef.fov_y > 90 )
	{
		fovOffset = -0.4 * ( cg.refdef.fov_y - 90 );
	}
	else
	{
		fovOffset = 0;
	}

	memset( &hand, 0, sizeof( hand ) );

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[ 0 ], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[ 1 ], hand.origin );
	VectorMA( hand.origin, ( cg_gun_z.value + fovOffset ), cg.refdef.viewaxis[ 2 ], hand.origin );

	if ( weapon == WP_LUCIFER_CANNON && ps->stats[ STAT_MISC ] > 0 )
	{
		float fraction = ( float )ps->stats[ STAT_MISC ] / ( float )LCANNON_TOTAL_CHARGE;

		VectorMA( hand.origin, random() * fraction, cg.refdef.viewaxis[ 0 ], hand.origin );
		VectorMA( hand.origin, random() * fraction, cg.refdef.viewaxis[ 1 ], hand.origin );
	}

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer )
	{
		// development tool
		hand.frame    = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}
	else
	{
		// get clientinfo for animation map
		ci            = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame    = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel   = wi->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( weapon_t weapon )
{
	//int ammo, clips;
	//
	//BG_UnpackAmmoArray( i, cg.snap->ps.ammo, cg.snap->ps.powerups, &ammo, &clips );
	//
	//TA: this is a pain in the ass
	//if( !ammo && !clips && !BG_FindInfinteAmmoForWeapon( i ) )
	//  return qfalse;

	if ( !BG_InventoryContainsWeapon( weapon, cg.snap->ps.stats ) )
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_UpgradeSelectable
===============
*/
static qboolean CG_UpgradeSelectable( upgrade_t upgrade )
{
	if ( !BG_InventoryContainsUpgrade( upgrade, cg.snap->ps.stats ) )
	{
		return qfalse;
	}

	return qtrue;
}

#define ICON_BORDER 4

/*
===================
CG_DrawItemSelect
===================
*/
void CG_DrawItemSelect( rectDef_t *rect, vec4_t color )
{
	int           i;
	int           x        = rect->x;
	int           y        = rect->y;
	int           width    = rect->w;
	int           height   = rect->h;
	int           iconsize;
	int           items[ 64 ];
	int           numItems = 0, selectedItem = 0;
	int           length;
	int           selectWindow;
	qboolean      vertical;
	centity_t     *cent;
	playerState_t *ps;

	cent = &cg_entities[ cg.snap->ps.clientNum ];
	ps   = &cg.snap->ps;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 )
	{
		return;
	}

	if ( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
	{
		// first make sure that whatever it selected is actually selectable
		if ( cg.weaponSelect <= 32 && !CG_WeaponSelectable( cg.weaponSelect ) )
		{
			CG_NextWeapon_f();
		}
		else if ( cg.weaponSelect > 32 && !CG_UpgradeSelectable( cg.weaponSelect ) )
		{
			CG_NextWeapon_f();
		}
	}

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	if ( height > width )
	{
		vertical = qtrue;
		iconsize = width;
		length   = height / width;
	}
	else if ( height <= width )
	{
		vertical = qfalse;
		iconsize = height;
		length   = width / height;
	}

	selectWindow = length / 2;

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( !BG_InventoryContainsWeapon( i, cg.snap->ps.stats ) )
		{
			continue;
		}

		if ( i == cg.weaponSelect )
		{
			selectedItem = numItems;
		}

		CG_RegisterWeapon( i );
		items[ numItems ] = i;
		numItems++;
	}

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( !BG_InventoryContainsUpgrade( i, cg.snap->ps.stats ) )
		{
			continue;
		}

		if ( i == cg.weaponSelect - 32 )
		{
			selectedItem = numItems;
		}

		CG_RegisterUpgrade( i );
		items[ numItems ] = i + 32;
		numItems++;
	}

	for ( i = 0; i < length; i++ )
	{
		int displacement = i - selectWindow;
		int item         = displacement + selectedItem;

		if ( ( item >= 0 ) && ( item < numItems ) )
		{
			trap_R_SetColor( color );

			if ( items[ item ] <= 32 )
			{
				CG_DrawPic( x, y, iconsize, iconsize, cg_weapons[ items[ item ] ].weaponIcon );
			}
			else if ( items[ item ] > 32 )
			{
				CG_DrawPic( x, y, iconsize, iconsize, cg_upgrades[ items[ item ] - 32 ].upgradeIcon );
			}

			trap_R_SetColor( NULL );
		}

		if ( vertical )
		{
			y += iconsize;
		}
		else
		{
			x += iconsize;
		}
	}
}

/*
===================
CG_DrawItemSelectText
===================
*/
void CG_DrawItemSelectText( rectDef_t *rect, float scale, int textStyle )
{
	int   x, w;
	char  *name;
	float *color;

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );

	if ( !color )
	{
		return;
	}

	trap_R_SetColor( color );

	// draw the selected name
	if ( cg.weaponSelect <= 32 )
	{
		if ( cg_weapons[ cg.weaponSelect ].registered &&
		     BG_InventoryContainsWeapon( cg.weaponSelect, cg.snap->ps.stats ) )
		{
			if ( ( name = cg_weapons[ cg.weaponSelect ].humanName ) )
			{
				w = CG_Text_Width( name, scale, 0 );
				x = rect->x + rect->w / 2;
				CG_Text_Paint( x - w / 2, rect->y + rect->h, scale, color, name, 0, 0, textStyle );
			}
		}
	}
	else if ( cg.weaponSelect > 32 )
	{
		if ( cg_upgrades[ cg.weaponSelect - 32 ].registered &&
		     BG_InventoryContainsUpgrade( cg.weaponSelect - 32, cg.snap->ps.stats ) )
		{
			if ( ( name = cg_upgrades[ cg.weaponSelect - 32 ].humanName ) )
			{
				w = CG_Text_Width( name, scale, 0 );
				x = rect->x + rect->w / 2;
				CG_Text_Paint( x - w / 2, rect->y + rect->h, scale, color, name, 0, 0, textStyle );
			}
		}
	}

	trap_R_SetColor( NULL );
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void )
{
	int i;
	int original;

	if ( !cg.snap )
	{
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		trap_SendClientCommand( "followprev\n" );
		return;
	}

	cg.weaponSelectTime = cg.time;
	original            = cg.weaponSelect;

	for ( i = 0; i < 64; i++ )
	{
		cg.weaponSelect++;

		if ( cg.weaponSelect == 64 )
		{
			cg.weaponSelect = 0;
		}

		if ( cg.weaponSelect <= 32 )
		{
			if ( CG_WeaponSelectable( cg.weaponSelect ) )
			{
				break;
			}
		}
		else if ( cg.weaponSelect > 32 )
		{
			if ( CG_UpgradeSelectable( cg.weaponSelect - 32 ) )
			{
				break;
			}
		}
	}

	if ( i == 64 )
	{
		cg.weaponSelect = original;
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void )
{
	int i;
	int original;

	if ( !cg.snap )
	{
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		trap_SendClientCommand( "follownext\n" );
		return;
	}

	cg.weaponSelectTime = cg.time;
	original            = cg.weaponSelect;

	for ( i = 0; i < 64; i++ )
	{
		cg.weaponSelect--;

		if ( cg.weaponSelect == -1 )
		{
			cg.weaponSelect = 63;
		}

		if ( cg.weaponSelect <= 32 )
		{
			if ( CG_WeaponSelectable( cg.weaponSelect ) )
			{
				break;
			}
		}
		else if ( cg.weaponSelect > 32 )
		{
			if ( CG_UpgradeSelectable( cg.weaponSelect - 32 ) )
			{
				break;
			}
		}
	}

	if ( i == 64 )
	{
		cg.weaponSelect = original;
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void )
{
	int num;

	if ( !cg.snap )
	{
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > 31 )
	{
		return;
	}

	cg.weaponSelectTime = cg.time;

	if ( !BG_InventoryContainsWeapon( num, cg.snap->ps.stats ) )
	{
		return; // don't have the weapon
	}

	cg.weaponSelect = num;
}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, weaponMode_t weaponMode )
{
	entityState_t *es;
	int           c;
	weaponInfo_t  *wi;
	weapon_t      weaponNum;

	es        = &cent->currentState;

	weaponNum = es->weapon;

	if ( weaponNum == WP_NONE )
	{
		return;
	}

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	if ( weaponNum >= WP_NUM_WEAPONS )
	{
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}

	wi = &cg_weapons[ weaponNum ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	if ( wi->wim[ weaponMode ].muzzleParticleSystem )
	{
		if ( !CG_IsParticleSystemValid( &cent->muzzlePS ) ||
		     !CG_IsParticleSystemInfinite( cent->muzzlePS ) )
		{
			cent->muzzlePsTrigger = qtrue;
		}
	}

	// play a sound
	for ( c = 0; c < 4; c++ )
	{
		if ( !wi->wim[ weaponMode ].flashSound[ c ] )
		{
			break;
		}
	}

	if ( c > 0 )
	{
		c = rand() % c;

		if ( wi->wim[ weaponMode ].flashSound[ c ] )
		{
			trap_S_StartSound( NULL, es->number, CHAN_WEAPON, wi->wim[ weaponMode ].flashSound[ c ] );
		}
	}
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( weapon_t weaponNum, weaponMode_t weaponMode, int clientNum,
                        vec3_t origin, vec3_t dir, impactSound_t soundType )
{
	qhandle_t    mark    = 0;
	qhandle_t    ps      = 0;
	int          c;
	float        radius  = 1.0f;
	weaponInfo_t *weapon = &cg_weapons[ weaponNum ];

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	mark   = weapon->wim[ weaponMode ].impactMark;
	radius = weapon->wim[ weaponMode ].impactMarkSize;
	ps     = weapon->wim[ weaponMode ].impactParticleSystem;

	if ( soundType == IMPACTSOUND_FLESH )
	{
		//flesh sound
		for ( c = 0; c < 4; c++ )
		{
			if ( !weapon->wim[ weaponMode ].impactFleshSound[ c ] )
			{
				break;
			}
		}

		if ( c > 0 )
		{
			c = rand() % c;

			if ( weapon->wim[ weaponMode ].impactFleshSound[ c ] )
			{
				trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, weapon->wim[ weaponMode ].impactFleshSound[ c ] );
			}
		}
	}
	else
	{
		//normal sound
		for ( c = 0; c < 4; c++ )
		{
			if ( !weapon->wim[ weaponMode ].impactSound[ c ] )
			{
				break;
			}
		}

		if ( c > 0 )
		{
			c = rand() % c;

			if ( weapon->wim[ weaponMode ].impactSound[ c ] )
			{
				trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, weapon->wim[ weaponMode ].impactSound[ c ] );
			}
		}
	}

	//create impact particle system
	if ( ps )
	{
		particleSystem_t *partSystem = CG_SpawnNewParticleSystem( ps );

		if ( CG_IsParticleSystemValid( &partSystem ) )
		{
			CG_SetAttachmentPoint( &partSystem->attachment, origin );
			CG_SetParticleSystemNormal( partSystem, dir );
			CG_AttachToPoint( &partSystem->attachment );
		}
	}

	//
	// impact mark
	//
	if ( radius > 0.0f )
	{
		CG_ImpactMark( mark, origin, dir, random() * 360, 1, 1, 1, 1, qfalse, radius, qfalse );
	}
}

/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( weapon_t weaponNum, weaponMode_t weaponMode,
                          vec3_t origin, vec3_t dir, int entityNum )
{
	vec3_t       normal;
	weaponInfo_t *weapon = &cg_weapons[ weaponNum ];

	VectorCopy( dir, normal );
	VectorInverse( normal );

	CG_Bleed( origin, normal, entityNum );

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	if ( weapon->wim[ weaponMode ].alwaysImpact )
	{
		CG_MissileHitWall( weaponNum, weaponMode, 0, origin, dir, IMPACTSOUND_FLESH );
	}
}

/*
============================================================================

BULLETS

============================================================================
*/

/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest )
{
	vec3_t     forward, right;
	polyVert_t verts[ 4 ];
	vec3_t     line;
	float      len, begin, end;
	vec3_t     start, finish;
	vec3_t     midpoint;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 )
	{
		return;
	}

	begin = 50 + random() * ( len - 60 );
	end   = begin + cg_tracerLength.value;

	if ( end > len )
	{
		end = len;
	}

	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[ 0 ] = DotProduct( forward, cg.refdef.viewaxis[ 1 ] );
	line[ 1 ] = DotProduct( forward, cg.refdef.viewaxis[ 2 ] );

	VectorScale( cg.refdef.viewaxis[ 1 ], line[ 1 ], right );
	VectorMA( right, -line[ 0 ], cg.refdef.viewaxis[ 2 ], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[ 0 ].xyz );
	verts[ 0 ].st[ 0 ]       = 0;
	verts[ 0 ].st[ 1 ]       = 1;
	verts[ 0 ].modulate[ 0 ] = 255;
	verts[ 0 ].modulate[ 1 ] = 255;
	verts[ 0 ].modulate[ 2 ] = 255;
	verts[ 0 ].modulate[ 3 ] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[ 1 ].xyz );
	verts[ 1 ].st[ 0 ]       = 1;
	verts[ 1 ].st[ 1 ]       = 0;
	verts[ 1 ].modulate[ 0 ] = 255;
	verts[ 1 ].modulate[ 1 ] = 255;
	verts[ 1 ].modulate[ 2 ] = 255;
	verts[ 1 ].modulate[ 3 ] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[ 2 ].xyz );
	verts[ 2 ].st[ 0 ]       = 1;
	verts[ 2 ].st[ 1 ]       = 1;
	verts[ 2 ].modulate[ 0 ] = 255;
	verts[ 2 ].modulate[ 1 ] = 255;
	verts[ 2 ].modulate[ 2 ] = 255;
	verts[ 2 ].modulate[ 3 ] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[ 3 ].xyz );
	verts[ 3 ].st[ 0 ]       = 0;
	verts[ 3 ].st[ 1 ]       = 0;
	verts[ 3 ].modulate[ 0 ] = 255;
	verts[ 3 ].modulate[ 1 ] = 255;
	verts[ 3 ].modulate[ 2 ] = 255;
	verts[ 3 ].modulate[ 3 ] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );

	midpoint[ 0 ] = ( start[ 0 ] + finish[ 0 ] ) * 0.5;
	midpoint[ 1 ] = ( start[ 1 ] + finish[ 1 ] ) * 0.5;
	midpoint[ 2 ] = ( start[ 2 ] + finish[ 2 ] ) * 0.5;

	// add the tracer sound
	trap_S_StartSound( midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound );
}

/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle )
{
	vec3_t    forward;
	centity_t *cent;
	int       anim;

	if ( entityNum == cg.snap->ps.clientNum )
	{
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[ 2 ] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[ entityNum ];

	if ( !cent->currentValid )
	{
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR )
	{
		muzzle[ 2 ] += CROUCH_VIEWHEIGHT;
	}
	else
	{
		muzzle[ 2 ] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;
}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum )
{
	vec3_t start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && cg_tracerChance.value > 0 )
	{
		if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) )
		{
			// draw a tracer
			if ( random() < cg_tracerChance.value )
			{
				CG_Tracer( start, end );
			}
		}
	}

	// impact splash and mark
	if ( flesh )
	{
		CG_Bleed( end, normal, fleshEntityNum );
	}
	else
	{
		CG_MissileHitWall( WP_MACHINEGUN, WPM_PRIMARY, 0, end, normal, IMPACTSOUND_DEFAULT );
	}
}

/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void CG_ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, int otherEntNum )
{
	int     i;
	float   r, u;
	vec3_t  end;
	vec3_t  forward, right, up;
	trace_t tr;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0; i < SHOTGUN_PELLETS; i++ )
	{
		r = Q_crandom( &seed ) * SHOTGUN_SPREAD * 16;
		u = Q_crandom( &seed ) * SHOTGUN_SPREAD * 16;
		VectorMA( origin, 8192 * 16, forward, end );
		VectorMA( end, r, right, end );
		VectorMA( end, u, up, end );

		CG_Trace( &tr, origin, NULL, NULL, end, otherEntNum, MASK_SHOT );

		if ( !( tr.surfaceFlags & SURF_NOIMPACT ) )
		{
			if ( cg_entities[ tr.entityNum ].currentState.eType == ET_PLAYER )
			{
				CG_MissileHitPlayer( WP_SHOTGUN, WPM_PRIMARY, tr.endpos, tr.plane.normal, tr.entityNum );
			}
			else if ( tr.surfaceFlags & SURF_METAL )
			{
				CG_MissileHitWall( WP_SHOTGUN, WPM_PRIMARY, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL );
			}
			else
			{
				CG_MissileHitWall( WP_SHOTGUN, WPM_PRIMARY, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT );
			}
		}
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire( entityState_t *es )
{
	vec3_t v;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );

	CG_ShotgunPattern( es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum );
}
