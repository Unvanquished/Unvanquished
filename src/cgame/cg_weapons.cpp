/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
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

// cg_weapons.c -- events and effects dealing with weapons

#include "cg_local.h"

static refSkeleton_t gunSkeleton;
static refSkeleton_t oldGunSkeleton;

/*
=================
CG_RegisterUpgrade

The server says this item is used on this level
=================
*/
void CG_RegisterUpgrade( int upgradeNum )
{
	upgradeInfo_t *upgradeInfo;
	const char    *icon;

	if ( upgradeNum <= UP_NONE || upgradeNum >= UP_NUM_UPGRADES )
	{
		CG_Error( "CG_RegisterUpgrade: out of range: %d", upgradeNum );
	}

	upgradeInfo = &cg_upgrades[ upgradeNum ];

	if ( upgradeInfo->registered )
	{
		CG_Printf( "CG_RegisterUpgrade: already registered: (%d) %s\n", upgradeNum,
		           BG_Upgrade( upgradeNum )->name );
		return;
	}

	upgradeInfo->registered = true;

	if ( !BG_Upgrade( upgradeNum )->name[ 0 ] )
	{
		CG_Error( "Couldn't find upgrade %i", upgradeNum );
	}

	upgradeInfo->humanName = BG_Upgrade( upgradeNum )->humanName;

	icon = BG_Upgrade( upgradeNum )->icon;
	if ( icon )
	{
		upgradeInfo->upgradeIcon = trap_R_RegisterShader( icon, RSF_DEFAULT );
	}
}

/*
=======================
CG_LoadCustomCrosshairs

Load custom crosshairs specified by the user
=======================
*/

static void CG_LoadCustomCrosshairs()
{
	const char         *text_p, *token;
	char         text[ 20000 ];
	int          len;
	fileHandle_t f;
	weapon_t     weapon;

	len = trap_FS_FOpenFile( cg_crosshairFile.string, &f, FS_READ );

	if ( len < 0 )
	{
		return;
	}

	if ( len == 0 || len + 1 >= (int) sizeof( text ) )
	{
		CG_Printf( len == 0 ? "File %s is empty\n" : "File %s is too long\n", cg_crosshairFile.string );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	while ( 1 )
	{
		token = COM_Parse2( &text_p );

		if ( !*token )
		{
			break;
		}

		if ( ( weapon = BG_WeaponNumberByName( token ) ) )
		{
			token = COM_Parse( &text_p );

			if ( !Q_stricmp( token, "crosshair" ) )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				cg_weapons[ weapon ].crossHair = trap_R_RegisterShader( token, RSF_DEFAULT );

				if ( !cg_weapons[ weapon ].crossHair )
				{
					CG_Printf( S_ERROR "weapon crosshair not found %s\n", token );
				}

				continue;
			}
			else if ( !Q_stricmp( token, "crosshairIndicator" ) )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				cg_weapons[ weapon ].crossHairIndicator = trap_R_RegisterShader( token, RSF_DEFAULT );

				if ( !cg_weapons[ weapon ].crossHairIndicator )
				{
					CG_Printf( S_ERROR "weapon crosshair indicator not found %s\n", token );
				}

				continue;
			}
			else if ( !Q_stricmp( token, "crosshairSize" ) )
			{
				int size;

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

				cg_weapons[ weapon ].crossHairSize = size;

				continue;
			}
			else
			{
				CG_Printf( S_ERROR "Unexpected keyword %s in crosshair file\n", token );
				break;
			}
		}
		else
		{
			CG_Printf( S_ERROR "Unknown weapon %s in crosshair file\n", token );
			break;
		}
	}
}

/*
===============
CG_InitUpgrades

Precaches upgrades
===============
*/
void CG_InitUpgrades()
{
	int i;

	Com_Memset( cg_upgrades, 0, sizeof( cg_upgrades ) );

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		CG_RegisterUpgrade( i );
	}

	if ( cg_crosshairFile.string[0] )
	{
		CG_LoadCustomCrosshairs();
	}
}

 /*
============================
CG_ParseWeaponAnimationFile

Reads the animation.cfg for weapons
============================
*/

static bool CG_ParseWeaponAnimationFile( const char *filename, weaponInfo_t *wi )
{
	const char         *text_p;
	int          len;
	int          i;
	char         *token;
	float        fps;
	char         text[ 20000 ];
	fileHandle_t f;
	animation_t  *animations;

	animations = wi->animations;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len < 0 )
	{
		return false;
	}

	if ( len == 0 || len + 1 >= (int) sizeof( text ) )
	{
		CG_Printf( len == 0 ? "File %s is empty\n" : "File %s is too long\n", filename );
		trap_FS_FCloseFile( f );
		return false;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	for ( i = WANIM_NONE + 1; i < MAX_WEAPON_ANIMATIONS; i++ )
	{
		token = COM_Parse2( &text_p );
		animations[ i ].firstFrame = atoi( token );

		token = COM_Parse2( &text_p );
		animations[ i ].numFrames = atoi( token );
		animations[ i ].reversed = false;
		animations[ i ].flipflop = false;

		if ( animations[ i ].numFrames < 0 )
		{
			animations[ i ].numFrames *= -1;
			animations[ i ].reversed = true;
		}

		token = COM_Parse2( &text_p );
		animations[ i ].loopFrames = atoi( token );

		token = COM_Parse2( &text_p );
		fps = atof( token );

		if ( fps == 0 )
		{
			fps = 1;
		}

		animations[ i ].frameLerp = 1000 / fps;
		animations[ i ].initialLerp = 1000 / fps;
	}

	if ( i != MAX_WEAPON_ANIMATIONS )
	{
		CG_Printf( "Error parsing weapon animation file: %s\n", filename );
		return false;
	}

	return true;
}

/*
===============
CG_ParseWeaponModeSection

Parse a weapon mode section
===============
*/
static bool CG_ParseWeaponModeSection( weaponInfoMode_t *wim, const char **text_p )
{
	char *token;
	int  i;

	wim->flashDlight = 0;
	wim->flashDlightIntensity = 0;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( text_p );

		if ( !*token )
		{
			return false;
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
				CG_Printf( S_ERROR "muzzle particle system not found %s\n", token );
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
				CG_Printf( S_ERROR "impact particle system not found %s\n", token );
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

			wim->impactMark = trap_R_RegisterShader(token,
								RSF_DEFAULT);
			wim->impactMarkSize = size;

			if ( !wim->impactMark )
			{
				CG_Printf( S_ERROR "impact mark shader not found %s\n", token );
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

			wim->impactSound[ index ] = trap_S_RegisterSound( token, false );

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

			wim->impactFleshSound[ index ] = trap_S_RegisterSound( token, false );

			continue;
		}
		else if ( !Q_stricmp( token, "alwaysImpact" ) )
		{
			wim->alwaysImpact = true;

			continue;
		}
		else if ( !Q_stricmp( token, "flashDLight" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->flashDlight = atof( token );

			continue;
		}
		else if ( !Q_stricmp( token, "flashDLightInt" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->flashDlightIntensity = atof( token );

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
			wim->continuousFlash = true;

			continue;
		}
		else if ( !Q_stricmp( token, "firingSound" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->firingSound = trap_S_RegisterSound( token, false );

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

			wim->flashSound[ index ] = trap_S_RegisterSound( token, false );

			continue;
		}
		else if ( !Q_stricmp( token, "reloadSound" ) )
		{
			token = COM_Parse( text_p );

			if ( !token )
			{
				break;
			}

			wim->reloadSound = trap_S_RegisterSound( token, false );
			continue;
		}
		else if ( !Q_stricmp( token, "}" ) )
		{
			return true; //reached the end of this weapon section
		}
		else
		{
			CG_Printf( S_ERROR "unknown token '%s' in weapon section\n", token );
			return false;
		}
	}

	return false;
}

bool CG_RegisterWeaponAnimation( animation_t *anim, const char *filename, bool loop, bool reversed,
    bool clearOrigin )
{
	int frameRate;

	anim->handle = trap_R_RegisterAnimation( filename );

	if ( !anim->handle )
	{
		Com_Printf( "Failed to load animation file %s\n", filename );
		return false;
	}

	anim->firstFrame = 0;
	anim->numFrames = trap_R_AnimNumFrames( anim->handle );
	frameRate = trap_R_AnimFrameRate( anim->handle );

	if ( frameRate == 0 )
	{
		frameRate = 1;
	}

	anim->frameLerp = 1000 / frameRate;
	anim->initialLerp = 1000 / frameRate;

	if ( loop )
	{
		anim->loopFrames = anim->numFrames;
	}
	else
	{
		anim->loopFrames = 0;
	}

	anim->reversed = reversed;
	anim->clearOrigin = clearOrigin;

	return true;
}

/*
======================
CG_ParseWeaponFile

Parses a configuration file describing a weapon
======================
*/
static bool CG_ParseWeaponFile( const char *filename, int weapon, weaponInfo_t *wi )
{
	const char         *text_p;
	int          len;
	char         *token;
	char         text[ 20000 ];
	fileHandle_t f;
	weaponMode_t weaponMode = WPM_NONE;
	char         token2[ MAX_QPATH ];
	int          i;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len < 0 )
	{
		return false;
	}

	if ( len == 0 || len + 1 >= (int) sizeof( text ) )
	{
		trap_FS_FCloseFile( f );
		CG_Printf( len == 0 ? "File %s is empty\n" : "File %s is too long\n", filename );
		return false;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	wi->scale = 1.0f;

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( &text_p );

		if ( !*token )
		{
			break;
		}

		if ( !Q_stricmp( token, "{" ) )
		{
			if ( weaponMode == WPM_NONE )
			{
				CG_Printf( S_ERROR "weapon mode section started without a declaration\n" );
				return false;
			}
			else if ( !CG_ParseWeaponModeSection( &wi->wim[ weaponMode ], &text_p ) )
			{
				CG_Printf( S_ERROR "failed to parse weapon mode section\n" );
				return false;
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

			COM_StripExtension( token, token2 );

			if ( cg_highPolyWeaponModels.integer && ( wi->weaponModel = trap_R_RegisterModel( va( "%s_view.iqm", token2 ) ) ) )
			{
				wi->md5 = true;

				if ( !CG_RegisterWeaponAnimation( &wi->animations[ WANIM_IDLE ],
				                                  va( "%s_view.iqm:idle", token2 ), true, false, false ) )
				{
					//CG_Error( "could not find '%s'", path );
				}

				// default all weapon animations to the idle animation
				for ( i = 0; i < MAX_WEAPON_ANIMATIONS; i++ )
				{
					if ( i == WANIM_IDLE )
					{
						continue;
					}

					wi->animations[ i ] = wi->animations[ WANIM_IDLE ];
				}

				switch( weapon )
				{
					case WP_MACHINEGUN:
					case WP_SHOTGUN:
					case WP_MASS_DRIVER:
					case WP_PULSE_RIFLE:

						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_RAISE ],
													va( "%s_view.iqm:raise", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_DROP ],
													va( "%s_view.iqm:lower", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_RELOAD ],
													va( "%s_view.iqm:reload", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK1 ],
													va( "%s_view.iqm:fire", token2 ), false, false, false );
						break;

					case WP_LUCIFER_CANNON:
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK2 ],
													va( "%s_view.iqm:fire2", token2 ), false, false, false );
					case WP_BLASTER:
					case WP_PAIN_SAW:
					case WP_LAS_GUN:
					case WP_CHAINGUN:
					case WP_FLAMER:
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_RAISE ],
													va( "%s_view.iqm:raise", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_DROP ],
													va( "%s_view.iqm:lower", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_RELOAD ],
													va( "%s_view.iqm:reload", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK1 ],
													va( "%s_view.iqm:fire", token2 ), false, false, false );
						break;

					case WP_ALEVEL1:
					case WP_ALEVEL2:
					case WP_ALEVEL4:
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK1 ],
													va( "%s_view.iqm:fire", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK2 ],
													va( "%s_view.iqm:fire2", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK3 ],
													va( "%s_view.iqm:fire3", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK4 ],
													va( "%s_view.iqm:fire4", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK5 ],
													va( "%s_view.iqm:fire5", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK6 ],
													va( "%s_view.iqm:fire6", token2 ), false, false, false );
						break;

					case WP_ALEVEL2_UPG:
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK1 ],
													va( "%s_view.iqm:fire", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK2 ],
													va( "%s_view.iqm:fire2", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK3 ],
													va( "%s_view.iqm:fire3", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK4 ],
													va( "%s_view.iqm:fire4", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK5 ],
													va( "%s_view.iqm:fire5", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK6 ],
													va( "%s_view.iqm:fire6", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK7 ],
													va( "%s_view.iqm:fire7", token2 ), false, false, false );
				}
			}
			else if ( cg_highPolyWeaponModels.integer && ( wi->weaponModel = trap_R_RegisterModel( va( "%s_view.md5mesh", token2 ) ) ) )
			{
				wi->md5 = true;

				if ( !CG_RegisterWeaponAnimation( &wi->animations[ WANIM_IDLE ],
					va( "%s_view_idle.md5anim", token2 ), true, false, false ) )
				{
					//CG_Error( "could not find '%s'", path );
				}

				// default all weapon animations to the idle animation
				for ( i = 0; i < MAX_WEAPON_ANIMATIONS; i++ )
				{
					if ( i == WANIM_IDLE )
					{
						continue;
					}

					wi->animations[ i ] = wi->animations[ WANIM_IDLE ];
				}

				switch( weapon )
				{
					case WP_MACHINEGUN:
					case WP_SHOTGUN:
					case WP_MASS_DRIVER:
					case WP_PULSE_RIFLE:

						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_RAISE ],
									    va( "%s_view_raise.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_DROP ],
									    va( "%s_view_lower.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_RELOAD ],
									    va( "%s_view_reload.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK1 ],
									    va( "%s_view_fire.md5anim", token2 ), false, false, false );
						break;

					case WP_LUCIFER_CANNON:
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK2 ],
									    va( "%s_view_fire2.md5anim", token2 ), false, false, false );
					case WP_BLASTER:
					case WP_PAIN_SAW:
					case WP_LAS_GUN:
					case WP_CHAINGUN:
					case WP_FLAMER:
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_RAISE ],
									    va( "%s_view_raise.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_DROP ],
									    va( "%s_view_lower.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_RELOAD ],
									    va( "%s_view_reload.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK1 ],
									    va( "%s_view_fire.md5anim", token2 ), false, false, false );
						break;

					case WP_ALEVEL1:
					case WP_ALEVEL2:
					case WP_ALEVEL4:
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK1 ],
									    va( "%s_view_fire.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK2 ],
									    va( "%s_view_fire2.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK3 ],
									    va( "%s_view_fire3.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK4 ],
									    va( "%s_view_fire4.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK5 ],
									    va( "%s_view_fire5.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK6 ],
									    va( "%s_view_fire6.md5anim", token2 ), false, false, false );
						break;

					case WP_ALEVEL2_UPG:
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK1 ],
									    va( "%s_view_fire.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK2 ],
									    va( "%s_view_fire2.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK3 ],
									    va( "%s_view_fire3.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK4 ],
									    va( "%s_view_fire4.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK5 ],
									    va( "%s_view_fire5.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK6 ],
									    va( "%s_view_fire6.md5anim", token2 ), false, false, false );
						CG_RegisterWeaponAnimation( &wi->animations[ WANIM_ATTACK7 ],
									    va( "%s_view_fire7.md5anim", token2 ), false, false, false );
				}
			}
			else
			{
				wi->weaponModel = trap_R_RegisterModel( token );
			}

			if ( !wi->weaponModel )
			{
				CG_Printf( S_ERROR "weapon model not found %s\n", token );
			}

			COM_StripExtension( token, path );
			strcat( path, "_flash.md3" );
			wi->flashModel = trap_R_RegisterModel( path );

			COM_StripExtension( token, path );
			strcat( path, "_barrel.md3" );
			wi->barrelModel = trap_R_RegisterModel( path );

			COM_StripExtension( token, path );
			strcat( path, "_hand.md3" );
			wi->handsModel = trap_R_RegisterModel( path );

			continue;
		}
		else if ( !Q_stricmp( token, "weaponModel3rdPerson" ) )
		{
			char path[ MAX_QPATH ];

			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->weaponModel3rdPerson = trap_R_RegisterModel( token );

			if ( !wi->weaponModel3rdPerson )
			{
				CG_Printf( S_ERROR "3rd person weapon "
				           "model not found %s\n", token );
			}

			COM_StripExtension( token, path );
			strcat( path, "_flash.md3" );
			wi->flashModel3rdPerson = trap_R_RegisterModel( path );

			COM_StripExtension( token, path );
			strcat( path, "_barrel.md3" );
			wi->barrelModel3rdPerson = trap_R_RegisterModel( path );

			continue;
		}
		else if ( !Q_stricmp( token, "idleSound" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->readySound = trap_S_RegisterSound( token, false );

			continue;
		}
		else if ( !Q_stricmp( token, "icon" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->weaponIcon = wi->ammoIcon = trap_R_RegisterShader(token,
									      RSF_DEFAULT);

			if ( !wi->weaponIcon )
			{
				CG_Printf( S_ERROR "weapon icon not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "crosshair" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->crossHair = trap_R_RegisterShader( token, RSF_DEFAULT );

			if ( !wi->crossHair )
			{
				CG_Printf( S_ERROR "weapon crosshair not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "crosshairIndicator" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			wi->crossHairIndicator = trap_R_RegisterShader( token, RSF_DEFAULT );

			if ( !wi->crossHairIndicator )
			{
				CG_Printf( S_ERROR "weapon crosshair indicator not found %s\n", token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "crosshairSize" ) )
		{
			int size;

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

			wi->crossHairSize = size;

			continue;
		}
		else if ( !Q_stricmp( token, "disableIn3rdPerson" ) )
		{
			wi->disableIn3rdPerson = true;

			continue;
		}
		else if ( !Q_stricmp( token, "rotation" ) )
		{
			if ( !cg_highPolyWeaponModels.integer )
			{
				for ( i = 0; i < 3; i++ )
				{
					token = COM_ParseExt2( &text_p, false );
				}
				continue;
			}

			for ( i = 0; i < 3; i++ )
			{
				token = COM_ParseExt2( &text_p, false );

				if ( !token )
				{
					break;
				}

				wi->rotation[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "posOffs" ) )
		{
			if ( !cg_highPolyWeaponModels.integer )
			{
				for ( i = 0; i < 3; i++ )
				{
					token = COM_ParseExt2( &text_p, false );
				}
				continue;
			}
			for ( i = 0; i < 3; i++ )
			{
				token = COM_ParseExt2( &text_p, false );

				if ( !token )
				{
					break;
				}

				wi->posOffs[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "rotationBone" ) )
		{
			if ( !cg_highPolyWeaponModels.integer )
			{
				token = COM_Parse2( &text_p );
				continue;
			}

			token = COM_Parse2( &text_p );
			Q_strncpyz( wi->rotationBone, token, sizeof( wi->rotationBone ) );
			continue;
		}
		else if ( !Q_stricmp( token, "modelScale" ) )
		{
			if ( !cg_highPolyWeaponModels.integer )
			{
				token = COM_ParseExt2( &text_p, false );
				continue;
			}

			token = COM_ParseExt2( &text_p, false );

			if ( token )
			{
				wi->scale = atof( token );
			}

			continue;
		}

		Com_Printf( S_ERROR "unknown token '%s'\n", token );
		return false;
	}

	return true;
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

	if ( weaponNum <= WP_NONE || weaponNum >= WP_NUM_WEAPONS )
	{
		CG_Error( "CG_RegisterWeapon: out of range: %d", weaponNum );
	}

	weaponInfo = &cg_weapons[ weaponNum ];

	if ( weaponInfo->registered )
	{
		CG_Printf( "CG_RegisterWeapon: already registered: (%d) %s\n", weaponNum,
		           BG_Weapon( weaponNum )->name );
		return;
	}

	weaponInfo->registered = true;

	if ( !BG_Weapon( weaponNum )->name[ 0 ] )
	{
		CG_Error( "Couldn't find weapon %i", weaponNum );
	}

	Com_sprintf( path, MAX_QPATH, "models/weapons/%s/weapon.cfg", BG_Weapon( weaponNum )->name );

	weaponInfo->humanName = BG_Weapon( weaponNum )->humanName;

	if ( !CG_ParseWeaponFile( path, weaponNum, weaponInfo ) )
	{
		Com_Printf( S_ERROR "failed to parse %s\n", path );
	}

	if( !weaponInfo->md5 )
	{
		CG_ParseWeaponAnimationFile( va( "models/weapons/%s/animation.cfg", BG_Weapon( weaponNum )->name ), weaponInfo );
	}


	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );

	for ( i = 0; i < 3; i++ )
	{
		weaponInfo->weaponMidpoint[ i ] = mins[ i ] + 0.5 * ( maxs[ i ] - mins[ i ] );
	}
}

/*
===============
CG_InitWeapons

Precaches weapons
===============
*/
void CG_InitWeapons()
{
	int i;

	Com_Memset( cg_weapons, 0, sizeof( cg_weapons ) );

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
===============
CG_SetWeaponLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetWeaponLerpFrameAnimation( weapon_t weapon, lerpFrame_t *lf, int newAnimation )
{
	animation_t *anim;
	bool toggle = false;

	lf->animationNumber = newAnimation;
	toggle = newAnimation & ANIM_TOGGLEBIT;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_WEAPON_ANIMATIONS )
	{
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &cg_weapons[ weapon ].animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
	lf->frame = lf->oldFrame = 0;
	if ( cg_debugAnim.integer )
	{
		CG_Printf( "Anim: %i\n", newAnimation );
	}

	if ( /*&cg_weapons[ weapon ].md5 &&*/ !toggle && lf && lf->old_animation && lf->old_animation->handle )
	{
		if ( !trap_R_BuildSkeleton( &oldGunSkeleton, lf->old_animation->handle, lf->oldFrame, lf->frame, lf->backlerp, lf->old_animation->clearOrigin ) )
		{
			CG_Printf( "CG_SetWeaponLerpFrameAnimation: can't build old gunSkeleton\n" );
			return;
		}
	}
}

/*
===============
CG_WeaponAnimation
===============
*/
static void CG_WeaponAnimation( centity_t *cent, int *old, int *now, float *backLerp )
{
	lerpFrame_t   *lf = &cent->pe.weapon;
	entityState_t *es = &cent->currentState;

	// see if the animation sequence is switching
	if ( es->weaponAnim != lf->animationNumber || !lf->animation || ( cg_weapons[ es->weapon ].md5 && !lf->animation->handle ) )
	{
		CG_SetWeaponLerpFrameAnimation( (weapon_t) es->weapon, lf, es->weaponAnim );
	}

	CG_RunLerpFrame( lf, 1.0f );

	*old = lf->oldFrame;
	*now = lf->frame;
	*backLerp = lf->backlerp;

	if ( cg_weapons[ es->weapon ].md5 )
	{
		CG_BlendLerpFrame( lf );

		CG_BuildAnimSkeleton( lf, &gunSkeleton, &oldGunSkeleton );
	}
}

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame, int anim )
{
	if ( anim == -1 ) { return 0; }

	if ( !cg_highPolyPlayerModels.integer )
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
		if ( frame >= ci->animations[ TORSO_ATTACK_BLASTER ].firstFrame &&
		     frame < ci->animations[ TORSO_ATTACK_BLASTER ].firstFrame + 6 )
		{
			return 1 + frame - ci->animations[ TORSO_ATTACK_BLASTER ].firstFrame;
		}
	}
	else // MD5 animations all start at 0, so there is no way to differentiate them with first frame alone
	{
		// change weapon
		if ( anim == TORSO_DROP && frame < 9 )
		{
			return frame - 6;
		}

		// stand attack
		else if ( ( anim == TORSO_ATTACK || anim == TORSO_ATTACK_BLASTER ) && frame < 6 )
		{
			return 1 + frame;
		}
	}

	return 0;
}

/*
==============
WeaponOffsets
==============
*/
WeaponOffsets WeaponOffsets::operator+=( WeaponOffsets B )
{
	bob += B.bob;
	angvel += B.angvel;

	return *this;
}

WeaponOffsets WeaponOffsets::operator*( float B )
{
	WeaponOffsets R;

	R.bob = bob * B;
	R.angvel = angvel * B;

	return R;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t out_origin, vec3_t out_angles )
{
	const float
		limitX = 1.5f,
		scaleX = -2e-3,
		limitY = 1.5f,
		scaleY = 2e-3;
	Vec3 origin, angles, right, up;
	float        scale;
	//weaponInfo_t *weapon = cg_weapons + cg.predictedPlayerState.weapon;
	Filter<WeaponOffsets> &filter = cg.weaponOffsetsFilter;
	WeaponOffsets offsets;

	origin = Vec3::Load( cg.refdef.vieworg );
	angles = Vec3::Load( cg.refdefViewAngles );
	right = Vec3::Load( cg.refdef.viewaxis[ 1 ] );
	up = Vec3::Load( cg.refdef.viewaxis[ 2 ] );

	// on odd legs, invert some angles
	scale = ( cg.bobcycle & 1 ? -1 : 1 ) * cg.xyspeed;

	filter.SetWidth( 350 );

	// bobbing
	if( BG_Class( cg.predictedPlayerState.stats[ STAT_CLASS ] )->bob )
	{
		offsets.bob = Vec3(
			cg.xyspeed * cg.bobfracsin * 0.005,
			scale * cg.bobfracsin * 0.005,
			scale * cg.bobfracsin * 0.01 );
	}

	// weapon inertia
	offsets.angles = angles;

	if( !filter.IsEmpty( ) )
	{
		auto last = filter.Last( );
		float dt;

		dt = ( cg.time - last.first ) * 0.001f;

		offsets.angvel = angles.Apply2( AngleDelta, last.second.angles ) / dt;
	}

	// accumulate and get the smoothed out values

	filter.Accumulate( cg.time, offsets );
	offsets = filter.GaussianMA( cg.time );

	// offset angles and origin

	angles += offsets.bob;
	origin += up * atan( offsets.angvel[ 0 ] * scaleY ) * limitY;
	origin += right * atan( offsets.angvel[ 1 ] * scaleX ) * limitX;

	// FIXME: is this of any use?
	/*if( !weapon->md5 && !weapon->noDrift )
	{
		int delta;
		float fracsin;

		delta = cg.time - cg.landTime;

		if ( delta < LAND_DEFLECT_TIME )
		{
			origin += Vec3( 0, 0, cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME );
		}
		else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME )
		{
			origin += Vec3( 0, 0, cg.landChange * 0.25 *
			          ( LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta ) / LAND_RETURN_TIME );
		}

		// idle drift
		scale = cg.xyspeed + 40;
		fracsin = sin( cg.time * 0.001 );
		angles += Vec3( scale * fracsin * 0.01 );
	}*/

	origin.Store( out_origin );
	angles.Store( out_angles );
}

/*
======================
CG_MachinegunSpinAngle
======================
*/
#define   SPIN_SPEED 0.9
#define   COAST_TIME 1000
static float CG_MachinegunSpinAngle( centity_t *cent, bool firing )
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
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = firing;
	}

	return angle;
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is nullptr)
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
	bool     noGunModel;
	bool     firing;

	weaponNum = (weapon_t) cent->currentState.weapon;
	weaponMode = (weaponMode_t) cent->currentState.generic1;

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	if ( ( ( cent->currentState.eFlags & EF_FIRING ) && weaponMode == WPM_PRIMARY ) ||
	     ( ( cent->currentState.eFlags & EF_FIRING2 ) && weaponMode == WPM_SECONDARY ) ||
	     ( ( cent->currentState.eFlags & EF_FIRING3 ) && weaponMode == WPM_TERTIARY ) )
	{
		firing = true;
	}
	else
	{
		firing = false;
	}

	weapon = &cg_weapons[ weaponNum ];

	if ( !weapon->registered )
	{
		Com_Printf( S_WARNING "CG_AddPlayerWeapon: weapon %d (%s) "
		            "is not registered\n", weaponNum, BG_Weapon( weaponNum )->name );
		return;
	}

	// add the weapon
	Com_Memset( &gun, 0, sizeof( gun ) );
	Com_Memset( &barrel, 0, sizeof( barrel ) );
	Com_Memset( &flash, 0, sizeof( flash ) );

	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.renderfx = parent->renderfx;

	if ( ps )
	{
		gun.shaderRGBA[ 0 ] = 255;
		gun.shaderRGBA[ 1 ] = 255;
		gun.shaderRGBA[ 2 ] = 255;
		gun.shaderRGBA[ 3 ] = 255;

		//set weapon[1/2]Time when respective buttons change state
		if (cg.weapon1Firing != !!(cg.predictedPlayerState.eFlags & EF_FIRING))
		{
			cg.weapon1Time = cg.time;
			cg.weapon1Firing = !!(cg.predictedPlayerState.eFlags & EF_FIRING);
		}

		if (cg.weapon2Firing != !!(cg.predictedPlayerState.eFlags & EF_FIRING2))
		{
			cg.weapon2Time = cg.time;
			cg.weapon2Firing = !!(cg.predictedPlayerState.eFlags & EF_FIRING2);
		}

		if (cg.weapon3Firing != !!(cg.predictedPlayerState.eFlags & EF_FIRING3))
		{
			cg.weapon3Time = cg.time;
			cg.weapon3Firing = !!(cg.predictedPlayerState.eFlags & EF_FIRING3);
		}
	}

	if ( !ps )
	{
		gun.hModel = weapon->weaponModel3rdPerson;

		if ( !gun.hModel )
		{
			gun.hModel = weapon->weaponModel;
		}
	}
	else
	{
		gun.hModel = weapon->weaponModel;
	}

	noGunModel = ( ( !ps || cg.renderingThirdPerson ) && weapon->disableIn3rdPerson ) || !gun.hModel;

	if ( !ps )
	{
		// add weapon ready sound
		if ( firing && weapon->wim[ weaponMode ].firingSound )
		{
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
			                        weapon->wim[ weaponMode ].firingSound );
		}
		else if ( weapon->readySound )
		{
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
		}
	}

	// Lucifer cannon charge warning beep
	if ( weaponNum == WP_LUCIFER_CANNON && ( cent->currentState.eFlags & EF_WARN_CHARGE ) )
	{
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin,
		                        vec3_origin, ps ? cgs.media.lCannonWarningSound :
		                        cgs.media.lCannonWarningSound2 );
	}

	if ( !noGunModel )
	{
		CG_PositionEntityOnTag( &gun, parent, parent->hModel, "tag_weapon" );
		if ( ps )
		{
			CG_WeaponAnimation( cent, &gun.oldframe, &gun.frame, &gun.backlerp );
		}

		if ( weapon->md5 )
		{

			gun.skeleton = gunSkeleton;

			if ( weapon->rotationBone[ 0 ] && ps )
			{
				int    boneIndex = trap_R_BoneIndex( gun.hModel, weapon->rotationBone );
				quat_t rotation;
				matrix_t mat;
				vec3_t   nBounds[ 2 ];
				vec3_t   p1, p2;

				if ( boneIndex < 0 )
				{
					Com_Printf( S_WARNING "Cannot find bone index %s, using root bone\n",
								weapon->rotationBone );
					weapon->rotationBone[ 0 ] = '\0'; // avoid repeated warnings
					boneIndex = 0;
				}

				QuatFromAngles( rotation, weapon->rotation[ 0 ], weapon->rotation[ 1 ], weapon->rotation[ 2 ] );
				QuatMultiply0( gun.skeleton.bones[ boneIndex ].t.rot, rotation );

				// Update bounds to reflect rotation
				MatrixFromAngles( mat, weapon->rotation[ 0 ], weapon->rotation[ 1 ], weapon->rotation[ 2 ] );

				MatrixTransformNormal( mat, gun.skeleton.bounds[ 0 ], p1 );
				MatrixTransformNormal( mat, gun.skeleton.bounds[ 1 ], p2 );

				ClearBounds( nBounds[ 0 ], nBounds[ 1 ] );
				AddPointToBounds( p1, nBounds[ 0 ], nBounds[ 1 ] );
				AddPointToBounds( p2, nBounds[ 0 ], nBounds[ 1 ] );

				BoundsAdd( gun.skeleton.bounds[ 0 ], gun.skeleton.bounds[ 1 ], nBounds[ 0 ], nBounds[ 1 ] );
			}

			CG_TransformSkeleton( &gun.skeleton, weapon->scale );
		}

		trap_R_AddRefEntityToScene( &gun );

		if ( !ps )
		{
			barrel.hModel = weapon->barrelModel3rdPerson;

			if ( !barrel.hModel )
			{
				barrel.hModel = weapon->barrelModel;
			}
		}
		else
		{
			barrel.hModel = weapon->barrelModel;
		}

		// add the spinning barrel
		if ( barrel.hModel )
		{
			VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
			barrel.renderfx = parent->renderfx;

			angles[ YAW ] = 0;
			angles[ PITCH ] = 0;
			angles[ ROLL ] = CG_MachinegunSpinAngle( cent, firing );
			AnglesToAxis( angles, barrel.axis );

			CG_PositionRotatedEntityOnTag( &barrel, &gun, gun.hModel, "tag_barrel" );

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
				CG_SetAttachmentTag( &cent->muzzlePS->attachment, parent, parent->hModel, "tag_weapon" );
			}
			else
			{
				CG_SetAttachmentTag( &cent->muzzlePS->attachment, &gun, gun.hModel, "tag_flash" );
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

	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.renderfx = parent->renderfx;

	if ( !ps )
	{
		flash.hModel = weapon->flashModel3rdPerson;

		if ( !flash.hModel )
		{
			flash.hModel = weapon->flashModel;
		}
	}
	else
	{
		flash.hModel = weapon->flashModel;
	}

	if ( flash.hModel )
	{
		angles[ YAW ] = 0;
		angles[ PITCH ] = 0;
		angles[ ROLL ] = crandom() * 10;
		AnglesToAxis( angles, flash.axis );

		if ( noGunModel )
		{
			CG_PositionRotatedEntityOnTag( &flash, parent, parent->hModel, "tag_weapon" );
		}
		else
		{
			CG_PositionRotatedEntityOnTag( &flash, &gun, gun.hModel, "tag_flash" );
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
					CG_SetAttachmentTag( &cent->muzzlePS->attachment, parent, parent->hModel, "tag_weapon" );
				}
				else
				{
					CG_SetAttachmentTag( &cent->muzzlePS->attachment, &gun, gun.hModel, "tag_flash" );
				}

				CG_SetAttachmentCent( &cent->muzzlePS->attachment, cent );
				CG_AttachToTag( &cent->muzzlePS->attachment );
			}

			cent->muzzlePsTrigger = false;
		}

		// make a dlight for the flash
		if ( weapon->wim[ weaponMode ].flashDlightColor[ 0 ] ||
		     weapon->wim[ weaponMode ].flashDlightColor[ 1 ] ||
		     weapon->wim[ weaponMode ].flashDlightColor[ 2 ] )
		{
			trap_R_AddLightToScene( flash.origin, weapon->wim[ weaponMode ].flashDlight,
			                        weapon->wim[ weaponMode ].flashDlightIntensity,
			                        weapon->wim[ weaponMode ].flashDlightColor[ 0 ],
			                        weapon->wim[ weaponMode ].flashDlightColor[ 1 ],
			                        weapon->wim[ weaponMode ].flashDlightColor[ 2 ], 0, 0 );
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/

#define WEAPON_CLICK_REPEAT 500

void CG_AddViewWeapon( playerState_t *ps )
{
	static refEntity_t  hand; // static for proper alignment in QVMs
	centity_t    *cent;
	clientInfo_t *ci;
	float        fovOffset;
	vec3_t       angles;
	weaponInfo_t *wi;
	weapon_t     weapon = (weapon_t) ps->weapon;
	weaponMode_t weaponMode = (weaponMode_t) ps->generic1;
	bool     drawGun = true;

	// no weapon carried - can't draw it
	if ( weapon == WP_NONE )
	{
		return;
	}

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	wi = &cg_weapons[ weapon ];

	switch ( cg_drawGun.integer )
	{
		case 0:
			drawGun = false;
			break;

		case 1:
			if ( BG_Weapon( weapon )->team == TEAM_ALIENS )
			{
				drawGun = false;
			}
			break;
	}


	if ( !wi->registered )
	{
		Com_Printf( S_WARNING "CG_AddViewWeapon: weapon %d (%s) "
		            "is not registered\n", weapon, BG_Weapon( weapon )->name );
		return;
	}

	cent = &cg.predictedPlayerEntity; // &cg_entities[cg.snap->ps.clientNum];

	if ( ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION )
	{
		return;
	}

	// draw a prospective buildable infront of the player
	if ( ( ps->stats[ STAT_BUILDABLE ] & SB_BUILDABLE_MASK ) > BA_NONE )
	{
		CG_GhostBuildable( ps->stats[ STAT_BUILDABLE ] );
	}

	// no gun if in third person view
	if ( cg.renderingThirdPerson )
	{
		return;
	}

	// allow the gun to be completely removed
	if ( !drawGun )
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

			cent->muzzlePsTrigger = false;
		}

		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun )
	{
		return;
	}

	fovOffset = -0.03f * cg.refdef.fov_y;

	Com_Memset( &hand, 0, sizeof( hand ) );

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, ( cg_gun_x.value + fovOffset + wi->posOffs[ 0 ] ), cg.refdef.viewaxis[ 0 ], hand.origin );
	if( cg_mirrorgun.integer )
		VectorMA( hand.origin, -( cg_gun_y.value + wi->posOffs[ 1 ] ), cg.refdef.viewaxis[ 1 ], hand.origin );
	else
		VectorMA( hand.origin, ( cg_gun_y.value + wi->posOffs[ 1 ] ), cg.refdef.viewaxis[ 1 ], hand.origin );
	VectorMA( hand.origin, ( cg_gun_z.value + wi->posOffs[ 2 ] ), cg.refdef.viewaxis[ 2 ], hand.origin );

	// Lucifer Cannon vibration effect
	if ( weapon == WP_LUCIFER_CANNON && ps->stats[ STAT_MISC ] > 0 )
	{
		float fraction;

		fraction = ( float ) ps->stats[ STAT_MISC ] / LCANNON_CHARGE_TIME_MAX;
		VectorMA( hand.origin, random() * fraction, cg.refdef.viewaxis[ 0 ],
		          hand.origin );
		VectorMA( hand.origin, random() * fraction, cg.refdef.viewaxis[ 1 ],
		          hand.origin );
	}

	AnglesToAxis( angles, hand.axis );
	if( cg_mirrorgun.integer ) {
		hand.axis[1][0] = - hand.axis[1][0];
		hand.axis[1][1] = - hand.axis[1][1];
		hand.axis[1][2] = - hand.axis[1][2];
	}

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer )
	{
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	}
	else
	{
		// get clientinfo for animation map
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame, !wi->md5 ? cent->pe.torso.animationNumber & ~ANIM_TOGGLEBIT : -1 );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame, !wi->md5 ? cent->pe.torso.animationNumber & ~ANIM_TOGGLEBIT : -1 );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = wi->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;
	if( cg_mirrorgun.integer )
		hand.renderfx |= RF_SWAPCULL;

	// add everything onto the hand
	if ( weapon )
	{
		CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity );
	}
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
static bool CG_WeaponSelectable( weapon_t weapon )
{
	if ( !BG_InventoryContainsWeapon( weapon, cg.snap->ps.stats ) )
	{
		return false;
	}

	return true;
}

/*
===============
CG_UpgradeSelectable
===============
*/
static bool CG_UpgradeSelectable( upgrade_t upgrade )
{
	if ( !BG_InventoryContainsUpgrade( upgrade, cg.snap->ps.stats ) )
	{
		return false;
	}

	return BG_Upgrade( upgrade )->usable;
}

#define ICON_BORDER 4

/*
===================
CG_DrawItemSelect
===================
*/
void CG_DrawHumanInventory()
{
	int           i;
	int           items[ 64 ];
	int           colinfo[ 64 ];
	int           numItems = 0;
	playerState_t *ps;
	static char   RML[ MAX_STRING_CHARS ];

	enum
	{
		USABLE,
		NO_AMMO,
		NOT_USABLE
	};

	ps = &cg.snap->ps;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[ STAT_HEALTH ] <= 0 )
	{
		return;
	}

	if ( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
	{
		// first make sure that whatever it selected is actually selectable
		if ( cg.weaponSelect < 32 )
		{
			if ( !CG_WeaponSelectable( (weapon_t) cg.weaponSelect ) )
			{
				CG_NextWeapon_f();
			}
		}
		else
		{
			if ( !CG_UpgradeSelectable( (upgrade_t) ( cg.weaponSelect - 32 ) ) )
			{
				CG_NextWeapon_f();
			}
		}
	}

	// put all weapons in the items list
	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( !BG_InventoryContainsWeapon( i, cg.snap->ps.stats ) )
		{
			continue;
		}

		if ( !ps->ammo && !ps->clips && !BG_Weapon( i )->infiniteAmmo )
		{
			colinfo[ numItems ] = NO_AMMO;
		}
		else
		{
			colinfo[ numItems ] = USABLE;
		}

		if ( !cg_weapons[ i ].registered )
		{
			Com_Printf( S_WARNING "CG_DrawItemSelect: weapon %d (%s) "
			            "is not registered\n", i, BG_Weapon( i )->name );
			continue;
		}

		items[ numItems ] = i;
		numItems++;
	}

	// put all upgrades in the weapons list
	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( !BG_InventoryContainsUpgrade( i, cg.snap->ps.stats ) )
		{
			continue;
		}

		colinfo[ numItems ] = 0;

		if ( !BG_Upgrade( i )->usable )
		{
			colinfo[ numItems ] = NOT_USABLE;
		}

		if ( !cg_upgrades[ i ].registered )
		{
			Com_Printf( S_WARNING "CG_DrawItemSelect: upgrade %d (%s) "
			            "is not registered\n", i, BG_Upgrade( i )->name );
			continue;
		}

		items[ numItems ] = i + 32;
		numItems++;
	}

	// reset buffer
	RML[ 0 ] = '\0';

	Q_strncpyz( RML, "<div class='item_select'>", sizeof( RML ) );

	// Build RML string
	for ( i = 0; i < numItems; ++i )
	{
		const char *rmlClass;
		const char *src =  CG_GetShaderNameFromHandle( items[ i ] < 32 ? cg_weapons[ items[ i ] ].weaponIcon : cg_upgrades[ items[ i ] - 32 ].upgradeIcon );

		switch( colinfo[ i ] )
		{
			case USABLE:
				rmlClass = "usable";
				break;

			case NO_AMMO:
				rmlClass = "no_ammo";
				break;

			case NOT_USABLE:
			default:
				rmlClass = "inactive";
				break;
		}

		if ( cg.weaponSelect == items[ i ] || cg.weaponSelect - 32 == items[ i ] )
		{
			rmlClass = va( "%s selected", rmlClass );
		}


		Q_strcat( RML, sizeof( RML ), va( "<img class='%s' src='/%s' />", rmlClass, src ) );
	}
	Q_strcat( RML, sizeof( RML ), "</div>" );

	Rocket_SetInnerRML( RML, false );
}

/*
===================
CG_DrawItemSelectText
===================
*/
void CG_DrawItemSelectText()
{
	const char *name;
	float      alpha;

	alpha = CG_FadeAlpha( cg.weaponSelectTime, WEAPON_SELECT_TIME );

	if ( !alpha )
	{
		Rocket_SetInnerRML( "&nbsp;", false );
		return;
	}



	// draw the selected name
	if ( cg.weaponSelect < 32 )
	{
		if ( cg_weapons[ cg.weaponSelect ].registered &&
		     BG_InventoryContainsWeapon( cg.weaponSelect, cg.snap->ps.stats ) )
		{
			if ( ( name = cg_weapons[ cg.weaponSelect ].humanName ) )
			{
				Rocket_SetInnerRML( name, false );
			}
		}
	}
	else
	{
		if ( cg_upgrades[ cg.weaponSelect - 32 ].registered &&
		     BG_InventoryContainsUpgrade( cg.weaponSelect - 32, cg.snap->ps.stats ) )
		{
			if ( ( name = cg_upgrades[ cg.weaponSelect - 32 ].humanName ) )
			{
				Rocket_SetInnerRML( name, false );
			}
		}
	}

	Rocket_SetPropertyById( "", "opacity", va( "%f", alpha ) );
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f()
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
	original = cg.weaponSelect;

	for ( i = 0; i < 64; i++ )
	{
		cg.weaponSelect++;

		if ( cg.weaponSelect == 64 )
		{
			cg.weaponSelect = 0;
		}

		if ( cg.weaponSelect < 32 )
		{
			if ( CG_WeaponSelectable( (weapon_t) cg.weaponSelect ) )
			{
				break;
			}
		}
		else
		{
			if ( CG_UpgradeSelectable( (upgrade_t) ( cg.weaponSelect - 32 ) ) )
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
void CG_PrevWeapon_f()
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
	original = cg.weaponSelect;

	for ( i = 0; i < 64; i++ )
	{
		cg.weaponSelect--;

		if ( cg.weaponSelect == -1 )
		{
			cg.weaponSelect = 63;
		}

		if ( cg.weaponSelect < 32 )
		{
			if ( CG_WeaponSelectable( (weapon_t) cg.weaponSelect ) )
			{
				break;
			}
		}
		else
		{
			if ( CG_UpgradeSelectable( (upgrade_t) ( cg.weaponSelect - 32 ) ) )
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
void CG_Weapon_f()
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

TODO: Put this into cg_event_weapon.c?

===================================================================================================
*/

static bool CalcMuzzlePoint( int entityNum, vec3_t muzzle )
{
	centity_t *cent;

	if ( entityNum < 0 || entityNum >= MAX_ENTITIES )
	{
		return false;
	}

	if ( entityNum == cg.predictedPlayerState.clientNum )
	{
		cent = &cg.predictedPlayerEntity;
	}
	else if ( !( cent = &cg_entities[ entityNum ] ) || !cent->currentValid )
	{
		return false;
	}

	if ( cent->muzzlePS && CG_Attached( &cent->muzzlePS->attachment ) )
	{
		CG_AttachmentPoint( &cent->muzzlePS->attachment, muzzle );
	}
	else
	{
		return false;
	}

	return true;
}

static void PlayHitSound( vec3_t origin, const sfxHandle_t *impactSound )
{
	int c;

	for ( c = 0; c < 4; c++ )
	{
		if ( !impactSound[ c ] )
		{
			break;
		}
	}

	if ( c > 0 )
	{
		c = rand() % c;

		if ( impactSound[ c ] )
		{
			trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, impactSound[ c ] );
		}
	}
}

static void DrawGenericHitEffect( vec3_t origin, vec3_t normal, qhandle_t psHandle, int psCharge )
{
	particleSystem_t *ps;

	if ( !psHandle )
	{
		return;
	}

	ps = CG_SpawnNewParticleSystem( psHandle );

	if ( !CG_IsParticleSystemValid( &ps ) )
	{
		return;
	}

	CG_SetAttachmentPoint( &ps->attachment, origin );
	CG_SetParticleSystemNormal( ps, normal );
	CG_AttachToPoint( &ps->attachment );

	ps->charge = psCharge;
}

static void DrawEntityHitEffect( vec3_t origin, vec3_t normal, int targetNum )
{
	team_t           team;
	qhandle_t        psHandle;
	particleSystem_t *ps;
	centity_t        *target;

	target = &cg_entities[ targetNum ];

	if ( cg_blood.integer && target->currentState.eType == ET_PLAYER )
	{
		team = cgs.clientinfo[ targetNum ].team;

		if ( team == TEAM_ALIENS )
		{
			psHandle = cgs.media.alienBleedPS;
		}
		else if ( team == TEAM_HUMANS )
		{
			psHandle = cgs.media.humanBleedPS;
		}
		else
		{
			return;
		}
	}
	else if ( target->currentState.eType == ET_BUILDABLE )
	{
		team = BG_Buildable( target->currentState.modelindex )->team;

		if ( team == TEAM_ALIENS )
		{
			psHandle = cgs.media.alienBuildableBleedPS;
		}
		else if ( team == TEAM_HUMANS )
		{
			psHandle = cgs.media.humanBuildableBleedPS;
		}
		else
		{
			return;
		}
	}
	else
	{
		return;
	}

	ps = CG_SpawnNewParticleSystem( psHandle );

	if ( CG_IsParticleSystemValid( &ps ) )
	{
		CG_SetAttachmentPoint( &ps->attachment, origin );
		CG_SetAttachmentCent( &ps->attachment, &cg_entities[ targetNum ] );
		CG_AttachToPoint( &ps->attachment );

		CG_SetParticleSystemNormal( ps, normal );
	}
}

#define TRACER_MIN_DISTANCE 100.0f

// TODO: Use a trail system to make the effect framerate independent.
static void DrawTracer( vec3_t source, vec3_t dest, float chance, float length, float width )
{
	vec3_t     forward, right;
	polyVert_t verts[ 4 ];
	vec3_t     line;
	float      distance, begin, end;
	vec3_t     start, finish;

	if ( random() >= chance )
	{
		return;
	}

	VectorSubtract( dest, source, forward );
	distance = VectorNormalize( forward );

	// Start at least a little away from the muzzle.
	if ( distance < TRACER_MIN_DISTANCE )
	{
		return;
	}

	// Guarantee a minimum length of 1/4 * length.
	begin = random() * Q_clamp( distance - 0.25f * length, TRACER_MIN_DISTANCE, distance );
	end   = Q_clamp( begin + length, begin, distance );

	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[ 0 ] = DotProduct( forward, cg.refdef.viewaxis[ 1 ] );
	line[ 1 ] = DotProduct( forward, cg.refdef.viewaxis[ 2 ] );

	VectorScale( cg.refdef.viewaxis[ 1 ], line[ 1 ], right );
	VectorMA( right, -line[ 0 ], cg.refdef.viewaxis[ 2 ], right );
	VectorNormalize( right );

	VectorMA( finish, width, right, verts[ 0 ].xyz );
	verts[ 0 ].st[ 0 ] = 0;
	verts[ 0 ].st[ 1 ] = 1;
	verts[ 0 ].modulate[ 0 ] = 255;
	verts[ 0 ].modulate[ 1 ] = 255;
	verts[ 0 ].modulate[ 2 ] = 255;
	verts[ 0 ].modulate[ 3 ] = 255;

	VectorMA( finish, -width, right, verts[ 1 ].xyz );
	verts[ 1 ].st[ 0 ] = 1;
	verts[ 1 ].st[ 1 ] = 0;
	verts[ 1 ].modulate[ 0 ] = 255;
	verts[ 1 ].modulate[ 1 ] = 255;
	verts[ 1 ].modulate[ 2 ] = 255;
	verts[ 1 ].modulate[ 3 ] = 255;

	VectorMA( start, -width, right, verts[ 2 ].xyz );
	verts[ 2 ].st[ 0 ] = 1;
	verts[ 2 ].st[ 1 ] = 1;
	verts[ 2 ].modulate[ 0 ] = 255;
	verts[ 2 ].modulate[ 1 ] = 255;
	verts[ 2 ].modulate[ 2 ] = 255;
	verts[ 2 ].modulate[ 3 ] = 255;

	VectorMA( start, width, right, verts[ 3 ].xyz );
	verts[ 3 ].st[ 0 ] = 0;
	verts[ 3 ].st[ 1 ] = 0;
	verts[ 3 ].modulate[ 0 ] = 255;
	verts[ 3 ].modulate[ 1 ] = 255;
	verts[ 3 ].modulate[ 2 ] = 255;
	verts[ 3 ].modulate[ 3 ] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );
}

/*
================
Performs the same traces the server did to locate local hit effects.
Keep this in sync with ShotgunPattern in g_weapon.c!
================
*/
static void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, int attackerNum )
{
	int     i;
	float   r, u, a;
	vec3_t  end;
	vec3_t  forward, right, up;
	trace_t tr;
	entityState_t dummy;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0; i < SHOTGUN_PELLETS; i++ )
	{
		r = Q_crandom( &seed ) * M_PI;
		a = Q_random( &seed ) * SHOTGUN_SPREAD * 16;

		u = sin( r ) * a;
		r = cos( r ) * a;

		VectorMA( origin, 8192 * 16, forward, end );
		VectorMA( end, r, right, end );
		VectorMA( end, u, up, end );

		CG_Trace( &tr, origin, nullptr, nullptr, end, attackerNum, MASK_SHOT, 0 );

		if ( !( tr.surfaceFlags & SURF_NOIMPACT ) )
		{
			dummy.weapon          = WP_SHOTGUN;
			dummy.generic1        = WPM_PRIMARY;
			dummy.eventParm       = DirToByte( tr.plane.normal );
			dummy.otherEntityNum  = tr.entityNum;
			dummy.otherEntityNum2 = attackerNum;
			dummy.torsoAnim       = 0; // Make sure it is not used uninitialized

			if ( cg_entities[ tr.entityNum ].currentState.eType == ET_PLAYER ||
			     cg_entities[ tr.entityNum ].currentState.eType == ET_BUILDABLE )
			{
				CG_HandleWeaponHitEntity( &dummy, tr.endpos );
			}
			else
			{
				CG_HandleWeaponHitWall( &dummy, tr.endpos );
			}
		}
	}
}

void CG_HandleFireWeapon( centity_t *cent, weaponMode_t weaponMode )
{
	entityState_t *es;
	int           c;
	weaponInfo_t  *wi;
	weapon_t      weaponNum;

	es = &cent->currentState;

	weaponNum = (weapon_t) es->weapon;

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
			cent->muzzlePsTrigger = true;
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
			trap_S_StartSound( nullptr, es->number, CHAN_WEAPON, wi->wim[ weaponMode ].flashSound[ c ] );
		}
	}
}

void CG_HandleFireShotgun( entityState_t *es )
{
	vec3_t v;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );

	ShotgunPattern( es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum );
}

void CG_HandleWeaponHitEntity( entityState_t *es, vec3_t origin )
{
	weapon_t         weapon;
	weaponMode_t     weaponMode;
	int              victimNum, attackerNum, psCharge;
	vec3_t           normal, muzzle;
	weaponInfoMode_t *wim;
	centity_t        *victim;

	// retrieve data from event
	weapon      = (weapon_t) es->weapon;
	weaponMode  = (weaponMode_t) es->generic1;
	victimNum   = es->otherEntityNum;
	attackerNum = es->otherEntityNum2;
	psCharge    = es->torsoAnim;
	ByteToDir( es->eventParm, normal );

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	wim    = &cg_weapons[ weapon ].wim[ weaponMode ];
	victim = &cg_entities[ victimNum ];

	// generic hit effect
	if ( wim->alwaysImpact )
	{
		DrawGenericHitEffect( origin, normal, wim->impactParticleSystem, psCharge );
	}

	// entity hit effect
	DrawEntityHitEffect( origin, normal, victimNum );

	// sound
	if ( victim->currentState.eType == ET_PLAYER )
	{
		PlayHitSound( origin, wim->impactFleshSound );
	}
	else if ( victim->currentState.eType == ET_BUILDABLE &&
			  BG_Buildable( victim->currentState.modelindex )->team == TEAM_ALIENS )
	{
		PlayHitSound( origin, wim->impactFleshSound );
	}
	else
	{
		PlayHitSound( origin, wim->impactSound );
	}

	// tracer
	if ( CalcMuzzlePoint( attackerNum, muzzle ) )
	{
		DrawTracer( muzzle, origin, cg_tracerChance.value, cg_tracerLength.value,
		            cg_tracerWidth.value );
	}
}

void CG_HandleWeaponHitWall( entityState_t *es, vec3_t origin )
{
	weapon_t         weapon;
	weaponMode_t     weaponMode;
	int              attackerNum, psCharge;
	vec3_t           normal, muzzle;
	weaponInfoMode_t *wim;

	// retrieve data from event
	weapon      = (weapon_t) es->weapon;
	weaponMode  = (weaponMode_t) es->generic1;
	//victimNum   = es->otherEntityNum;
	attackerNum = es->otherEntityNum2;
	psCharge    = es->torsoAnim;
	ByteToDir( es->eventParm, normal );

	if ( weaponMode <= WPM_NONE || weaponMode >= WPM_NUM_WEAPONMODES )
	{
		weaponMode = WPM_PRIMARY;
	}

	wim = &cg_weapons[ weapon ].wim[ weaponMode ];

	// generic hit effect
	DrawGenericHitEffect( origin, normal, wim->impactParticleSystem, psCharge );

	// sound
	PlayHitSound( origin, wim->impactSound );

	// mark
	if ( wim->impactMark && wim->impactMarkSize > 0.0f )
	{
		CG_ImpactMark( wim->impactMark, origin, normal, random() * 360, 1, 1, 1, 1, false, wim->impactMarkSize, false );
	}

	// tracer
	if ( CalcMuzzlePoint( attackerNum, muzzle ) )
	{
		DrawTracer( muzzle, origin, cg_tracerChance.value, cg_tracerLength.value,
		            cg_tracerWidth.value );
	}
}

void CG_HandleMissileHitEntity( entityState_t *es, vec3_t origin )
{
	const missileAttributes_t *ma;
	int                       victimNum, psCharge;
	vec3_t                    normal;
	centity_t                 *victim;

	// retrieve data from event
	ma          = BG_Missile( es->weapon );
	victimNum   = es->otherEntityNum;
	//attackerNum = es->otherEntityNum2;
	psCharge    = es->torsoAnim;
	ByteToDir( es->eventParm, normal );

	victim      = &cg_entities[ victimNum ];

	// generic hit effect
	if ( ma->alwaysImpact )
	{
		DrawGenericHitEffect( origin, normal, ma->impactParticleSystem, psCharge );
	}

	// entity hit effect
	DrawEntityHitEffect( origin, normal, victimNum );

	// sound
	if ( victim->currentState.eType == ET_PLAYER )
	{
		PlayHitSound( origin, ma->impactFleshSound );
	}
	else if ( victim->currentState.eType == ET_BUILDABLE &&
			  BG_Buildable( victim->currentState.modelindex )->team == TEAM_ALIENS )
	{
		PlayHitSound( origin, ma->impactFleshSound );
	}
	else
	{
		PlayHitSound( origin, ma->impactSound );
	}
}

void CG_HandleMissileHitWall( entityState_t *es, vec3_t origin )
{
	const missileAttributes_t *ma;
	int                       psCharge;
	vec3_t                    normal;

	// retrieve data from event
	ma          = BG_Missile( es->weapon );
	//victimNum   = es->otherEntityNum;
	//attackerNum = es->otherEntityNum2;
	psCharge    = es->torsoAnim;
	ByteToDir( es->eventParm, normal );

	// generic hit effect
	DrawGenericHitEffect( origin, normal, ma->impactParticleSystem, psCharge );

	// sound
	PlayHitSound( origin, ma->impactSound );

	// mark
	if ( ma->impactMark && ma->impactMarkSize > 0.0f )
	{
		CG_ImpactMark( ma->impactMark, origin, normal, random() * 360, 1, 1, 1, 1, false,
		               ma->impactMarkSize, false );
	}
}

float CG_ChargeProgress()
{
	float progress;
	int   min = 0, max = 0;

	if ( cg.snap->ps.weapon ==  WP_ALEVEL1 )
	{
		min = 0;
		max = LEVEL1_POUNCE_COOLDOWN;
	}
	else if ( cg.snap->ps.weapon == WP_ALEVEL3 )
	{
		min = LEVEL3_POUNCE_TIME_MIN;
		max = LEVEL3_POUNCE_TIME;
	}
	else if ( cg.snap->ps.weapon == WP_ALEVEL3_UPG )
	{
		min = LEVEL3_POUNCE_TIME_MIN;
		max = LEVEL3_POUNCE_TIME_UPG;
	}
	else if ( cg.snap->ps.weapon == WP_ALEVEL4 )
	{
		if ( cg.predictedPlayerState.stats[ STAT_STATE ] & SS_CHARGING )
		{
			min = 0;
			max = LEVEL4_TRAMPLE_DURATION;
		}
		else
		{
			min = LEVEL4_TRAMPLE_CHARGE_MIN;
			max = LEVEL4_TRAMPLE_CHARGE_MAX;
		}
	}
	else if ( cg.snap->ps.weapon == WP_LUCIFER_CANNON )
	{
		min = LCANNON_CHARGE_TIME_MIN;
		max = LCANNON_CHARGE_TIME_MAX;
	}

	if ( max - min <= 0.0f )
	{
		return 0.0f;
	}

	progress = ( ( float ) cg.predictedPlayerState.stats[ STAT_MISC ] - min ) /
	( max - min );

	if ( progress > 1.0f )
	{
		return 1.0f;
	}

	if ( progress < 0.0f )
	{
		return 0.0f;
	}

	return progress;
}
