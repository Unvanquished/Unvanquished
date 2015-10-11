/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

static int GCD( int a, int b )
{
	int c;

	while ( b != 0 )
	{
		c = a % b;
		a = b;
		b = c;
	}

	return a;
}

static const char *DisplayAspectString( int w, int h )
{
	int gcd = GCD( w, h );

	w /= gcd;
	h /= gcd;

	// For some reason 8:5 is usually referred to as 16:10
	if ( w == 8 && h == 5 )
	{
		w = 16;
		h = 10;
	}

	return va( "%d:%d", w, h );
}

static void CG_Rocket_DFResolution( int handle, const char *data )
{
	int w = atoi( Info_ValueForKey(data, "1" ) );
	int h = atoi( Info_ValueForKey(data, "2" ) );

	if ( w == -1 || h == -1 )
	{
		Rocket_DataFormatterFormattedData( handle, "Custom", false );
		return;
	}
	char *aspectRatio = BG_strdup( DisplayAspectString( w, h ) );
	Rocket_DataFormatterFormattedData( handle, va( "%dx%d ( %s )", w, h, aspectRatio ), false );
	BG_Free( aspectRatio );
}

static void CG_Rocket_DFServerPing( int handle, const char *data )
{
	const char *str = Info_ValueForKey( data, "1" );
	Rocket_DataFormatterFormattedData( handle, *str && Q_isnumeric( *str ) ? va( "%s ms", Info_ValueForKey( data, "1" ) ) : "", false );
}

static void CG_Rocket_DFServerPlayers( int handle, const char *data )
{
	char max[ 4 ];
	Q_strncpyz( max, Info_ValueForKey( data, "3" ), sizeof( max ) );
	Rocket_DataFormatterFormattedData( handle, va( "%s + (%s) / %s", Info_ValueForKey( data, "1" ), Info_ValueForKey( data, "2" ), max ), true );
}

static void CG_Rocket_DFPlayerName( int handle, const char *data )
{
	Rocket_DataFormatterFormattedData( handle, va("<div class=\"playername\">%s</div>", CG_Rocket_QuakeToRML( cgs.clientinfo[ atoi( Info_ValueForKey( data, "1" ) ) ].name ) ) , false );
}

static void CG_Rocket_DFUpgradeName( int handle, const char *data )
{
	Rocket_DataFormatterFormattedData( handle, BG_Upgrade( atoi( Info_ValueForKey( data, "1" ) ) )->humanName, true );
}

static void CG_Rocket_DFVotePlayer( int handle, const char *data )
{
	Rocket_DataFormatterFormattedData( handle, va("<button onClick=\"Events.pushevent('exec set ui_dialogCvar1 %s;exec rocket ui/dialogs/editplayer.rml load; exec rocket editplayer show', event)\">vote/moderate</button>", cgs.clientinfo[ atoi( Info_ValueForKey( data, "1" ) ) ].name ) , false );
}

static void CG_Rocket_DFVoteMap( int handle, const char *data )
{
	int mapIndex = atoi( Info_ValueForKey( data, "1" ) );
	if ( mapIndex < rocketInfo.data.mapCount )
	{
		Rocket_DataFormatterFormattedData( handle, va("<button onClick=\"Events.pushevent('exec set ui_dialogCvar1 %s;hide maps;exec rocket ui/dialogs/mapdialog.rml load; exec rocket mapdialog show', event)\" class=\"maps\"><div class=\"levelname\">%s</div> <img class=\"levelshot\"src='/meta/%s/%s'/><div class=\"hovertext\">Start Vote</div> </button>", rocketInfo.data.mapList[ mapIndex ].mapLoadName, CG_Rocket_QuakeToRML( rocketInfo.data.mapList[ mapIndex ].mapName ), rocketInfo.data.mapList[ mapIndex ].mapLoadName, rocketInfo.data.mapList[ mapIndex ].mapLoadName ) , false );
	}
}

static void CG_Rocket_DFWeaponName( int handle, const char *data )
{
	Rocket_DataFormatterFormattedData( handle, BG_Weapon( atoi( Info_ValueForKey( data, "1" ) ) )->humanName, true );
}

static void CG_Rocket_DFClassName( int handle, const char *data )
{
	Rocket_DataFormatterFormattedData( handle, BG_Class( atoi( Info_ValueForKey( data, "1" ) ) )->name, true );
}

static void CG_Rocket_DFServerLabel( int handle, const char *data )
{
	const char *str = Info_ValueForKey( data, "1" );
	Rocket_DataFormatterFormattedData( handle, *data ? ++str : "&nbsp;", false );
}

static void CG_Rocket_DFCMArmouryBuyWeapon( int handle, const char *data )
{
	weapon_t weapon = (weapon_t) atoi( Info_ValueForKey( data, "1" ) );
	const char *Class = "";
	const char *Icon = "";
	const char *action = "";
	playerState_t *ps = &cg.snap->ps;
	int credits = ps->persistant[ PERS_CREDIT ];
	weapon_t currentweapon = BG_PrimaryWeapon( ps->stats );
	credits += BG_Weapon( currentweapon )->price;

	if( BG_InventoryContainsWeapon( weapon, cg.predictedPlayerState.stats ) ){
		Class = "active";
		action =  va( "onClick='Cmd.exec(\"sell %s\")'", BG_Weapon( weapon )->name );
		//Check mark icon. UTF-8 encoding of \uf00c
		Icon = "<icon class=\"current\">\xEF\x80\x8C</icon>";
	}
	else if ( !BG_WeaponUnlocked( weapon ) || BG_WeaponDisabled( weapon ) )
	{
		Class = "locked";
		//Padlock icon. UTF-8 encoding of \uf023
		Icon = "<icon>\xEF\x80\xA3</icon>";
	}

	else if(BG_Weapon( weapon )->price > credits){

		Class = "expensive";
		//$1 bill icon. UTF-8 encoding of \uf0d6
		Icon = "<icon>\xEF\x83\x96</icon>";
	}
	else
	{
		Class = "available";
		action =  va( "onClick='Cmd.exec(\"buy +%s\")'", BG_Weapon( weapon )->name );
	}

	Rocket_DataFormatterFormattedData( handle, va( "<button class='armourybuy %s' onMouseover='Events.pushevent(\"setDS armouryBuyList weapons %s\", event)' %s>%s<img src='/%s'/></button>", Class, Info_ValueForKey( data, "2" ), action, Icon, CG_GetShaderNameFromHandle( cg_weapons[ weapon ].ammoIcon )), false );
}

static void CG_Rocket_DFCMArmouryBuyUpgrade( int handle, const char *data )
{
	upgrade_t upgrade = (upgrade_t) atoi( Info_ValueForKey( data, "1" ) );
	const char *Class = "";
	const char *Icon = "";
	const char *action = "";
	playerState_t *ps = &cg.snap->ps;
	int credits = ps->persistant[ PERS_CREDIT ];

	if( BG_InventoryContainsUpgrade( upgrade, cg.predictedPlayerState.stats ) ){
		Class = "active";
		action =  va( "onClick='Cmd.exec(\"sell %s\")'", BG_Upgrade( upgrade )->name );
		//Check mark icon. UTF-8 encoding of \uf00c
		Icon = "<icon class=\"current\">\xEF\x80\x8C</icon>";
	}

	else if ( !BG_UpgradeUnlocked( upgrade ) || BG_UpgradeDisabled( upgrade ) )
	{
		Class = "locked";
		//Padlock icon. UTF-8 encoding of \uf023
		Icon = "<icon>\xEF\x80\xA3</icon>";
	}
	else if(BG_Upgrade( upgrade )->price > credits){

		Class = "expensive";
		//$1 bill icon. UTF-8 encoding of \uf0d6
		Icon = "<icon>\xEF\x83\x96</icon>";
	}
	else
	{
		Class = "available";
		action =  va( "onClick='Cmd.exec(\"buy +%s\")'", BG_Upgrade( upgrade )->name );
	}

	Rocket_DataFormatterFormattedData( handle, va( "<button class='armourybuy %s' onMouseover='Events.pushevent(\"setDS armouryBuyList upgrades %s\", event)' %s>%s<img src='/%s'/></button>", Class, Info_ValueForKey( data, "2" ), action, Icon, CG_GetShaderNameFromHandle( cg_upgrades[ upgrade ].upgradeIcon)), false );
}

static void CG_Rocket_DFGWeaponDamage( int handle, const char *data )
{
	weapon_t weapon = (weapon_t) atoi( Info_ValueForKey( data, "1" ) );
	int      width = 0;

	switch( weapon )
	{
		case WP_HBUILD: width = 0; break;
		case WP_MACHINEGUN: width = 10; break;
		case WP_PAIN_SAW: width = 90; break;
		case WP_SHOTGUN: width = 40; break;
		case WP_LAS_GUN: width = 30; break;
		case WP_MASS_DRIVER: width = 50; break;
		case WP_CHAINGUN: width = 60; break;
		case WP_FLAMER: width = 70; break;
		case WP_PULSE_RIFLE: width = 70; break;
		case WP_LUCIFER_CANNON: width = 100; break;
		default: width = 0; break;
	}

	Rocket_DataFormatterFormattedData( handle, va( "<div class=\"barValue\" style=\"width:%d%%;\"></div>", width ), false );
}

static void CG_Rocket_DFGWeaponRateOfFire( int handle, const char *data )
{
	weapon_t weapon = (weapon_t) atoi( Info_ValueForKey( data, "1" ) );
	int      width = 0;

	switch( weapon )
	{
		case WP_HBUILD: width = 0; break;
		case WP_MACHINEGUN: width = 70; break;
		case WP_PAIN_SAW: width = 100; break;
		case WP_SHOTGUN: width = 100; break;
		case WP_LAS_GUN: width = 40; break;
		case WP_MASS_DRIVER: width = 20; break;
		case WP_CHAINGUN: width = 80; break;
		case WP_FLAMER: width = 70; break;
		case WP_PULSE_RIFLE: width = 70; break;
		case WP_LUCIFER_CANNON: width = 10; break;
		default: width = 0; break;
	}

	Rocket_DataFormatterFormattedData( handle, va( "<div class=\"barValue\" style=\"width:%d%%;\"></div>", width ), false );
}

static void CG_Rocket_DFGWeaponRange( int handle, const char *data )
{
	weapon_t weapon = (weapon_t) atoi( Info_ValueForKey( data, "1" ) );
	int      width = 0;

	switch( weapon )
	{
		case WP_HBUILD: width = 0; break;
		case WP_MACHINEGUN: width = 75; break;
		case WP_PAIN_SAW: width = 10; break;
		case WP_SHOTGUN: width = 30; break;
		case WP_LAS_GUN: width = 100; break;
		case WP_MASS_DRIVER: width = 100; break;
		case WP_CHAINGUN: width = 50; break;
		case WP_FLAMER: width = 25; break;
		case WP_PULSE_RIFLE: width = 80; break;
		case WP_LUCIFER_CANNON: width = 75; break;
		default: width = 0; break;
	}

	Rocket_DataFormatterFormattedData( handle, va( "<div class=\"barValue\" style=\"width:%d%%;\"></div>", width ), false );
}

static void CG_Rocket_DFLevelShot( int handle, const char *data )
{
	Rocket_DataFormatterFormattedData( handle, va( "<img class=\"levelshot\" src=\"/levelshots/%s\"/>", Info_ValueForKey( data, "1" ) ), false );
}

static score_t *ScoreFromClientNum( int clientNum )
{
	int i = 0;

	for ( i = 0; i < cg.numScores; ++i )
	{
		if ( cg.scores[ i ].client == clientNum )
		{
			return &cg.scores[ i ];
		}
	}

	return nullptr;
}

static void CG_Rocket_DFGearOrReady( int handle, const char *data )
{
	int clientNum = atoi( Info_ValueForKey( data, "1" ) );
	if ( cg.intermissionStarted )
	{
		if ( CG_ClientIsReady( clientNum ) )
		{
			Rocket_DataFormatterFormattedData( handle, "[check]", true );
		}
		else
		{
			Rocket_DataFormatterFormattedData( handle, "", false );
		}
	}
	else
	{
		score_t *s = ScoreFromClientNum( clientNum );
		const char *rml = "";

		if ( s && s->team == cg.predictedPlayerState.persistant[ PERS_TEAM ] && s->weapon != WP_NONE )
		{
			rml = va( "<img src='/%s'/>", CG_GetShaderNameFromHandle( cg_weapons[ s->weapon ].weaponIcon ) );
		}

		if ( s && s->team == cg.predictedPlayerState.persistant[ PERS_TEAM ] && s->team == TEAM_HUMANS && s->upgrade != UP_NONE )
		{
			rml = va( "%s<img src='/%s'/>", rml, CG_GetShaderNameFromHandle( cg_upgrades[ s->upgrade ].upgradeIcon ) );
		}

		Rocket_DataFormatterFormattedData( handle, rml, false );
	}
}

static void CG_Rocket_DFCMAlienBuildables( int handle, const char *data )
{
	buildable_t buildable = ( buildable_t ) atoi( Info_ValueForKey( data, "1" ) );
	const char *Class = "";
	const char *Icon = "";
	const char *action = "";
	int value, valueMarked;

	value = cg.snap->ps.persistant[ PERS_BP ];
	valueMarked = cg.snap->ps.persistant[ PERS_MARKEDBP ];

	if ( BG_BuildableDisabled( buildable ) || !BG_BuildableUnlocked( buildable ) )
	{
		Class = "locked";
		//Padlock icon. UTF-8 encoding of \uf023
		Icon = "<icon>\xEF\x80\xA3</icon>";
	}
	else if ( BG_Buildable( buildable )->buildPoints > value + valueMarked )
	{
		Class = "expensive";
		//$1 bill icon. UTF-8 encoding of \uf0d6
		Icon = "<icon>\xEF\x83\x96</icon>";
	}
	else
	{
		Class = "available";
		action = va( "onClick='Cmd.exec(\"build %s\") Events.pushevent(\"hide %s\", event)'", BG_Buildable( buildable )->name, rocketInfo.menu[ ROCKETMENU_ALIENBUILD ].id );
	}

	Rocket_DataFormatterFormattedData( handle, va( "<button class='%s' onMouseover='Events.pushevent(\"setDS alienBuildList default %s\", event)' %s>%s<img src='/%s'/></button>", Class, Info_ValueForKey( data, "2" ), action, Icon, CG_GetShaderNameFromHandle( cg_buildables[ buildable ].buildableIcon ) ), false );
}

static void CG_Rocket_DFCMHumanBuildables( int handle, const char *data )
{
	buildable_t buildable = ( buildable_t ) atoi( Info_ValueForKey( data, "1" ) );
	const char *Class = "";
	const char *Icon = "";
	const char *action = "";
	int value, valueMarked;

	value = cg.snap->ps.persistant[ PERS_BP ];
	valueMarked = cg.snap->ps.persistant[ PERS_MARKEDBP ];

	if ( BG_BuildableDisabled( buildable ) || !BG_BuildableUnlocked( buildable ) )
	{
		Class = "locked";
		//Padlock icon. UTF-8 encoding of \uf023
		Icon = "<icon>\xEF\x80\xA3</icon>";
	}
	else if ( BG_Buildable( buildable )->buildPoints > value + valueMarked )
	{
		Class = "expensive";
		//$1 bill icon. UTF-8 encoding of \uf0d6
		Icon = "<icon>\xEF\x83\x96</icon>";
	}
	else
	{
		Class = "available";
		action = va( "onClick='Cmd.exec(\"build %s\") Events.pushevent(\"hide %s\", event)'", BG_Buildable( buildable )->name, rocketInfo.menu[ ROCKETMENU_HUMANBUILD ].id );
	}

	Rocket_DataFormatterFormattedData( handle, va( "<button class='%s' onMouseover='Events.pushevent(\"setDS humanBuildList default %s\", event)' %s>%s<img src='/%s'/></button>", Class, Info_ValueForKey( data, "2" ), action, Icon, CG_GetShaderNameFromHandle( cg_buildables[ buildable ].buildableIcon ) ), false );
}

static void CG_Rocket_DFCMAlienEvolve( int handle, const char *data )
{
	class_t alienClass = (class_t) atoi( Info_ValueForKey( data, "1" ) );
	const char *Class = "";
	const char *Icon = "";
	const char *action = "";
	int cost = BG_ClassCanEvolveFromTo( cg.predictedPlayerState.stats[ STAT_CLASS ], alienClass, cg.predictedPlayerState.persistant[ PERS_CREDIT ] );

	if( cg.predictedPlayerState.stats[ STAT_CLASS ] == alienClass )
	{
		Class = "active";
		//Check mark icon. UTF-8 encoding of \uf00c
		Icon = "<icon class=\"current\">\xEF\x80\x8C</icon>";
	}
	else if ( !BG_ClassUnlocked( alienClass ) || BG_ClassDisabled( alienClass ) )
	{
		Class = "locked";
		//Padlock icon. UTF-8 encoding of \uf023
		Icon = "<icon>\xEF\x80\xA3</icon>";
	}
	else if ( cost == -1 )
	{

		Class = "expensive";
		//$1 bill icon. UTF-8 encoding of \uf0d6
		Icon = "<icon>\xEF\x83\x96</icon>";
	}
	else
	{
		Class = "available";
		action =  va( "onClick='Cmd.exec(\"class %s\") Events.pushevent(\"hide %s\", event)'", BG_Class( alienClass )->name, rocketInfo.menu[ ROCKETMENU_ALIENEVOLVE ].id );
	}

	Rocket_DataFormatterFormattedData( handle, va( "<button class='alienevo %s' onMouseover='Events.pushevent(\"setDS alienEvolveList alienClasss %s\", event)' %s>%s<img src='/%s'/></button>", Class, Info_ValueForKey( data, "2" ), action, Icon, CG_GetShaderNameFromHandle( cg_classes[ alienClass ].classIcon )), false );
}

static void CG_Rocket_DFCMBeacons( int handle, const char *data )
{
	beaconType_t bct = (beaconType_t)atoi( Info_ValueForKey( data, "1" ) );
	const beaconAttributes_t *ba;
	const char *icon, *action;

	ba = BG_Beacon( bct );

	if( !ba )
		return;

	icon = CG_GetShaderNameFromHandle( ba->icon[ 0 ][ 0 ] );
	action = va( "onClick='Cmd.exec(\"beacon %s\") Events.pushevent(\"hide ingame_beaconmenu\", event)'", ba->name );

	Rocket_DataFormatterFormattedData( handle, va( "<button class='beacons' onMouseover='Events.pushevent(\"setDS beacons default %s\", event)' %s><img src='/%s'/></button>", Info_ValueForKey( data, "2" ), action, icon ), false );
}

typedef struct
{
	const char *name;
	void ( *exec ) ( int handle, const char *data );
} dataFormatterCmd_t;

static const dataFormatterCmd_t dataFormatterCmdList[] =
{
	{ "ClassName", &CG_Rocket_DFClassName },
	{ "CMAlienBuildables", &CG_Rocket_DFCMAlienBuildables },
	{ "CMAlienEvolve", &CG_Rocket_DFCMAlienEvolve },
	{ "CMArmouryBuyUpgrades", &CG_Rocket_DFCMArmouryBuyUpgrade },
	{ "CMArmouryBuyWeapons", &CG_Rocket_DFCMArmouryBuyWeapon },
	{ "CMBeacons", &CG_Rocket_DFCMBeacons },
	{ "CMHumanBuildables", &CG_Rocket_DFCMHumanBuildables },
	{ "GearOrReady", &CG_Rocket_DFGearOrReady },
	{ "GWeaponDamage", &CG_Rocket_DFGWeaponDamage },
	{ "GWeaponRange", &CG_Rocket_DFGWeaponRange },
	{ "GWeaponRateOfFire", &CG_Rocket_DFGWeaponRateOfFire },
	{ "LevelShot", &CG_Rocket_DFLevelShot },
	{ "PlayerName", &CG_Rocket_DFPlayerName },
	{ "Resolution", &CG_Rocket_DFResolution },
	{ "ServerLabel", &CG_Rocket_DFServerLabel },
	{ "ServerPing", &CG_Rocket_DFServerPing },
	{ "ServerPlayers", &CG_Rocket_DFServerPlayers },
	{ "UpgradeName", &CG_Rocket_DFUpgradeName },
	{ "VoteMap", &CG_Rocket_DFVoteMap },
	{ "VotePlayer", &CG_Rocket_DFVotePlayer },
	{ "WeaponName", &CG_Rocket_DFWeaponName },
};

static const size_t dataFormatterCmdListCount = ARRAY_LEN( dataFormatterCmdList );

static int dataFormatterCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( dataFormatterCmd_t * ) b )->name );
}

void CG_Rocket_FormatData( int handle )
{
	static char name[ 200 ], data[ BIG_INFO_STRING ];
	dataFormatterCmd_t *cmd;

	Rocket_DataFormatterRawData( handle, name, sizeof( name ), data, sizeof( data ) );

	cmd = (dataFormatterCmd_t*) bsearch( name, dataFormatterCmdList, dataFormatterCmdListCount, sizeof( dataFormatterCmd_t ), dataFormatterCmdCmp );

	if ( cmd )
	{
		cmd->exec( handle, data );
	}
}

void CG_Rocket_RegisterDataFormatters()
{
	for ( unsigned i = 0; i < dataFormatterCmdListCount; i++ )
	{
		// Check that the commands are in increasing order so that it can be used by bsearch
		if ( i != 0 && Q_stricmp( dataFormatterCmdList[ i - 1 ].name, dataFormatterCmdList[ i ].name ) > 0 )
		{
			CG_Printf( "CGame Rocket dataFormatterCmdList is in the wrong order for %s and %s\n", dataFormatterCmdList[i - 1].name, dataFormatterCmdList[ i ].name );
		}

		Rocket_RegisterDataFormatter( dataFormatterCmdList[ i ].name );
	}
}
