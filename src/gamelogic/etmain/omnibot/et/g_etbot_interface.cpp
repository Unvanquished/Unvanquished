/*
* ET <-> Omni-Bot interface source file.
*
*/

#include <sstream>
#include <iomanip>

extern "C"
{
#include "g_etbot_interface.h"
	qboolean G_IsOnFireteam( int entityNum, fireteamData_t **teamNum );
};

#include "../common/BotExports.h"
#include "ET_Config.h"
#include "ET_Messages.h"

#define OMNIBOT_MIN_ENG 2
#define OMNIBOT_MIN_MED 2
#define OMNIBOT_MIN_FOP 2
#define OMNIBOT_MIN_SOL 2
#define OMNIBOT_MIN_COP 1

void Bot_Event_EntityCreated( gentity_t *pEnt );

bool IsBot( gentity_t *e )
{
	return e->r.svFlags & SVF_BOT ? true : false;
}

//////////////////////////////////////////////////////////////////////////

const int MAX_SMOKEGREN_CACHE = 32;
gentity_t *g_SmokeGrenadeCache[ MAX_SMOKEGREN_CACHE ] = { 0 };

struct BotEntity
{
	obint16 m_HandleSerial;
	bool    m_NewEntity : 1;
	bool    m_Used : 1;
};

BotEntity m_EntityHandles[ MAX_GENTITIES ];

//////////////////////////////////////////////////////////////////////////

// utils partly taken from id code
#define WC_WEAPON_TIME_LEFT level.time - ps->classWeaponTime
#define WC_SOLDIER_TIME     level.soldierChargeTime         [ team - TEAM_AXIS ]
#define WC_ENGINEER_TIME    level.engineerChargeTime        [ team - TEAM_AXIS ]
#define WC_FIELDOPS_TIME    level.lieutenantChargeTime      [ team - TEAM_AXIS ]
#define WC_MEDIC_TIME       level.medicChargeTime           [ team - TEAM_AXIS ]
#define WC_COVERTOPS_TIME   level.covertopsChargeTime       [ team - TEAM_AXIS ]

//////////////////////////////////////////////////////////////////////////

gentity_t *INDEXENT( const int _gameId )
{
	if( _gameId >= 0 && _gameId < MAX_GENTITIES )
	{
		switch( _gameId )
		{
			case ENTITYNUM_WORLD: // world ent not marked inuse for some reason
				return &g_entities[ ENTITYNUM_WORLD ];

			default:
				return g_entities[ _gameId ].inuse ? &g_entities[ _gameId ] : 0;
		}
	}

	return 0;
}

int ENTINDEX( gentity_t *_ent )
{
	return _ent - g_entities;
}

gentity_t *EntityFromHandle( GameEntity _ent )
{
	obint16 index = _ent.GetIndex();

	if( m_EntityHandles[ index ].m_HandleSerial == _ent.GetSerial() && g_entities[ index ].inuse )
	{
		return &g_entities[ index ];
	}

	if( index == ENTITYNUM_WORLD )
	{
		return &g_entities[ ENTITYNUM_WORLD ];
	}

	return NULL;
}

GameEntity HandleFromEntity( gentity_t *_ent )
{
	if( _ent )
	{
		return GameEntity( _ent - g_entities, m_EntityHandles[ _ent - g_entities ].m_HandleSerial );
	}
	else
	{
		return GameEntity();
	}
}

//////////////////////////////////////////////////////////////////////////
enum { MaxDeferredGoals = 64 };

MapGoalDef g_DeferredGoals[ MaxDeferredGoals ];
int        g_NumDeferredGoals = 0;
bool       g_GoalSubmitReady = false;

void AddDeferredGoal( gentity_t *ent )
{
	if( g_NumDeferredGoals >= MaxDeferredGoals )
	{
		G_Error( "Deferred Goal Buffer Full!" );
		return;
	}

	MapGoalDef &goaldef = g_DeferredGoals[ g_NumDeferredGoals++ ];
	MapGoalDef &goaldef2 = g_DeferredGoals[ g_NumDeferredGoals++ ];

	switch( ent->s.eType )
	{
		case ET_MG42_BARREL:
			{
				goaldef.Props.SetString( "Type", "mountmg42" );
				goaldef.Props.SetEntity( "Entity", HandleFromEntity( ent ) );
				goaldef.Props.SetInt( "Team", ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ) );
				goaldef.Props.SetString( "TagName", _GetEntityName( ent ) );
				goaldef.Props.SetInt( "InterfaceGoal", 1 );

				// cs: this was done in et_goalmanager before
				goaldef2.Props.SetString( "Type", "repairmg42" );
				goaldef2.Props.SetEntity( "Entity", HandleFromEntity( ent ) );
				goaldef2.Props.SetInt( "Team", ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ) );
				goaldef2.Props.SetString( "TagName", _GetEntityName( ent ) );
				goaldef2.Props.SetInt( "InterfaceGoal", 1 );
				break;
			}

		default:
			{
				break;
			}
	}
}

void SendDeferredGoals()
{
	if( g_GoalSubmitReady )
	{
		for( int i = 0; i < g_NumDeferredGoals; ++i )
		{
			g_BotFunctions.pfnAddGoal( g_DeferredGoals[ i ] );
		}

		g_NumDeferredGoals = 0;
	}
}

void UpdateGoalEntity( gentity_t *oldent, gentity_t *newent )
{
	if( g_GoalSubmitReady )
	{
		g_BotFunctions.pfnUpdateEntity( HandleFromEntity( oldent ), HandleFromEntity( newent ) );
	}
}

void DeleteMapGoal( char *name )
{
	g_BotFunctions.pfnDeleteGoal( name );
}

struct mg42s_t
{
	gentity_t *ent;
	vec3_t    position;
	char      name[ 64 ];
	char      newname[ 64 ];
	bool      buildable;
};

mg42s_t mg42s[ 64 ];
int     numofmg42s = 0;
bool    havemg42s = true;

void GetMG42s()
{
	if( !numofmg42s && havemg42s )
	{
		gentity_t *trav = NULL;
		char      *name;

		while( ( trav = G_Find( trav, FOFS( classname ), "misc_mg42" ) ) )
		{
			name = ( char * ) _GetEntityName( trav );
			mg42s[ numofmg42s ].ent = trav;
			GetEntityCenter( trav, mg42s[ numofmg42s ].position );

			if( name )
			{
				strcpy( mg42s[ numofmg42s ].name, name );
			}
			else
			{
				mg42s[ numofmg42s ].name[ 0 ] = ( char ) NULL;
			}

			mg42s[ numofmg42s ].buildable = false;
			numofmg42s++;
		}
	}

	if( !numofmg42s )
	{
		havemg42s = false;
	}
}

void UpdateMG42( gentity_t *ent )
{
	vec3_t entpos;

	GetEntityCenter( ent, entpos );

	for( int i = 0; i < numofmg42s; ++i )
	{
		if( mg42s[ i ].position[ 0 ] == entpos[ 0 ] &&
		    mg42s[ i ].position[ 1 ] == entpos[ 1 ] )
		{
			mg42s[ i ].ent = ent;
		}
	}
}

void CheckForMG42( gentity_t *ent, const char *newname )
{
	if( !numofmg42s )
	{
		return;
	}

	vec3_t entpos;

	GetEntityCenter( ent, entpos );

	for( int i = 0; i < numofmg42s; ++i )
	{
		if( ( fabs( mg42s[ i ].position[ 0 ] - entpos[ 0 ] ) < 100.0 ) &&
		    ( fabs( mg42s[ i ].position[ 1 ] - entpos[ 1 ] ) < 100.0 ) )
		{
			mg42s[ i ].buildable = true;
			strcpy( mg42s[ i ].newname, newname );
		}
	}

	return;
}

void GetEntityCenter( gentity_t *ent, vec3_t _pos )
{
	_pos[ 0 ] = ent->r.currentOrigin[ 0 ] + ( ( ent->r.maxs[ 0 ] + ent->r.mins[ 0 ] ) * 0.5f );
	_pos[ 1 ] = ent->r.currentOrigin[ 1 ] + ( ( ent->r.maxs[ 1 ] + ent->r.mins[ 1 ] ) * 0.5f );
	_pos[ 2 ] = ent->r.currentOrigin[ 2 ] + ( ( ent->r.maxs[ 2 ] + ent->r.mins[ 2 ] ) * 0.5f );
}

//////////////////////////////////////////////////////////////////////////

// Important note:
// These weaponcharged values are intentionally set a little bit lower
// so the bots can start heading towards a goal a bit ahead of time rather than waiting
// for a full charge. Have a look at NQ case WP_LANDMINE as an exmaple.
// Exception:
// In case of no goal is used (f.e. the riffle WP_GPG40 & WP_M7)
// we do an exact match instead of lowered value
// Note:
// There is a seperate goal for soldiers with panzer/bazooka to go defend at
static qboolean weaponCharged( playerState_t *ps, team_t team, int weapon, int *skill )
{
	switch( weapon )
	{
#ifdef NOQUARTER

			// IRATA added BAZOOKA
		case WP_BAZOOKA:

#endif
		case WP_PANZERFAUST:
			if( ps->eFlags & EF_PRONE )
			{
				return qfalse;
			}

			if( skill[ SK_HEAVY_WEAPONS ] >= 1 )
			{
				if( WC_WEAPON_TIME_LEFT < WC_SOLDIER_TIME * 0.66f )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < WC_SOLDIER_TIME )
			{
				return qfalse;
			}

			break;

		case WP_MORTAR:
		case WP_MORTAR_SET:
#ifdef NOQUARTER
		case WP_MORTAR2:
		case WP_MORTAR2_SET:
#endif
			if( skill[ SK_HEAVY_WEAPONS ] >= 1 )
			{
				if( WC_WEAPON_TIME_LEFT < WC_SOLDIER_TIME * 0.33f )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < WC_SOLDIER_TIME * 0.5f )
			{
				return qfalse;
			}

			break;

		case WP_SMOKE_BOMB:
		case WP_SATCHEL:
			if( skill[ SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS ] >= 2 )
			{
				if( WC_WEAPON_TIME_LEFT < WC_COVERTOPS_TIME * 0.66f )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < WC_COVERTOPS_TIME )
			{
				return qfalse;
			}

			break;

		case WP_LANDMINE:
#ifdef NOQUARTER

			// IRATA NQ: see bg_misc charge cost
			// { 0.5f, 0.5f, 0.5f, .33f, .33f, .33f, .33f, .25f, .25f, .25f}; <--
			if( skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ] >= 2 && skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ] <= 5 )
			{
				if( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.33f ) )
				{
					return qfalse;
				}
			}

			if( skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ] >= 6 && skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ] <= 9 )
			{
				if( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.25f ) )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.5f ) )
			{
				return qfalse;
			}

#else

			if( skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ] >= 2 )
			{
				if( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.33f ) )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.5f ) )
			{
				return qfalse;
			}

#endif
			break;

		case WP_DYNAMITE:
			if( skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ] >= 3 )
			{
				if( WC_WEAPON_TIME_LEFT < ( WC_ENGINEER_TIME * 0.66f ) )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < WC_ENGINEER_TIME )
			{
				return qfalse;
			}

			break;

		case WP_MEDKIT:
			if( skill[ SK_FIRST_AID ] >= 2 )
			{
				if( WC_WEAPON_TIME_LEFT < WC_MEDIC_TIME * 0.15f )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < WC_MEDIC_TIME * 0.25f )
			{
				return qfalse;
			}

			break;

		case WP_AMMO:
			if( skill[ SK_SIGNALS ] >= 1 )
			{
				if( WC_WEAPON_TIME_LEFT < WC_FIELDOPS_TIME * 0.15f )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < WC_FIELDOPS_TIME * 0.25f )
			{
				return qfalse;
			}

			break;

		case WP_SMOKE_MARKER:
			if( skill[ SK_SIGNALS ] >= 2 )
			{
				if( WC_WEAPON_TIME_LEFT < WC_FIELDOPS_TIME * 0.66f )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < WC_FIELDOPS_TIME )
			{
				return qfalse;
			}

			break;

		case WP_MEDIC_ADRENALINE:
			if( WC_WEAPON_TIME_LEFT < WC_MEDIC_TIME )
			{
				return qfalse;
			}

			break;

		case WP_BINOCULARS:
			switch( ps->stats[ STAT_PLAYER_CLASS ] )
			{
				case PC_FIELDOPS:
					if( skill[ SK_SIGNALS ] >= 2 )
					{
						if( WC_WEAPON_TIME_LEFT <= WC_FIELDOPS_TIME * 0.66f )
						{
							return qfalse;
						}
					}
					else if( WC_WEAPON_TIME_LEFT <= WC_FIELDOPS_TIME )
					{
						return qfalse;
					}

					break;

				default:
					return qfalse;
			}

			break;

		case WP_GPG40:
		case WP_M7:
#ifdef NOQUARTER

			// IRATA NQ: see bg_misc charge cost
			//{ .50f, .50f, .50f, .50f, .50f, .50f, .50f, .50f, .35f, .35f};
			if( skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ] >= 8 )
			{
				if( WC_WEAPON_TIME_LEFT < WC_ENGINEER_TIME * 0.35f )
				{
					return qfalse;
				}
			}
			else if( WC_WEAPON_TIME_LEFT < WC_ENGINEER_TIME * 0.5f )
			{
				return qfalse;
			}

#else

			if( WC_WEAPON_TIME_LEFT < WC_ENGINEER_TIME * 0.5f )
			{
				return qfalse;
			}

#endif
			break;
	}

	return qtrue;
}

// cs: added ignore ent so the bot wont count itself.
// this fixes soldier counts since latchPlayerType defaults to 0
static int CountPlayerClass( team_t team, int playerClass, int ignore )
{
	int num = 0;
	int j;

	for( int i = 0; i < level.numConnectedClients; ++i )
	{
		if( i == ignore )
		{
			continue;
		}

		j = level.sortedClients[ i ];

		if( level.clients[ j ].sess.sessionTeam != team )
		{
			continue;
		}

		if( level.clients[ j ].sess.latchPlayerType != playerClass )
		{
			continue;
		}

		num++;
	}

	return num;
}

int Bot_TeamGameToBot( int team )
{
	switch( team )
	{
		case TEAM_AXIS:
			return ET_TEAM_AXIS;

		case TEAM_ALLIES:
			return ET_TEAM_ALLIES;

		case TEAM_SPECTATOR:
			return OB_TEAM_SPECTATOR;

		default:
			return ET_TEAM_NONE;
	}
}

static int playerClassBotToGame( int playerClass )
{
	switch( playerClass )
	{
		case ET_CLASS_SOLDIER:
			return PC_SOLDIER;

		case ET_CLASS_MEDIC:
			return PC_MEDIC;

		case ET_CLASS_ENGINEER:
			return PC_ENGINEER;

		case ET_CLASS_FIELDOPS:
			return PC_FIELDOPS;

		case ET_CLASS_COVERTOPS:
			return PC_COVERTOPS;

		default:
			return -1;
	}
}

int Bot_PlayerClassGameToBot( int playerClass )
{
	switch( playerClass )
	{
		case PC_SOLDIER:
			return ET_CLASS_SOLDIER;

		case PC_MEDIC:
			return ET_CLASS_MEDIC;

		case PC_ENGINEER:
			return ET_CLASS_ENGINEER;

		case PC_FIELDOPS:
			return ET_CLASS_FIELDOPS;

		case PC_COVERTOPS:
			return ET_CLASS_COVERTOPS;

		default:
			return ET_CLASS_NULL;
	}
}

static int _weaponBotToGame( int weapon )
{
	switch( weapon )
	{
		case ET_WP_ADRENALINE:
			return WP_MEDIC_ADRENALINE;

		case ET_WP_AKIMBO_COLT:
			return WP_AKIMBO_COLT;

		case ET_WP_AKIMBO_LUGER:
			return WP_AKIMBO_LUGER;

		case ET_WP_AKIMBO_SILENCED_COLT:
			return WP_AKIMBO_SILENCEDCOLT;

		case ET_WP_AKIMBO_SILENCED_LUGER:
			return WP_AKIMBO_SILENCEDLUGER;

		case ET_WP_AMMO_PACK:
			return WP_AMMO;

		case ET_WP_BINOCULARS:
			return WP_BINOCULARS;

		case ET_WP_CARBINE:
			return WP_CARBINE;

		case ET_WP_COLT:
			return WP_COLT;

		case ET_WP_DYNAMITE:
			return WP_DYNAMITE;

		case ET_WP_FG42:
			return WP_FG42;

		case ET_WP_FG42_SCOPE:
			return WP_FG42SCOPE;

		case ET_WP_FLAMETHROWER:
			return WP_FLAMETHROWER;

		case ET_WP_GARAND:
			return WP_GARAND;

		case ET_WP_GARAND_SCOPE:
			return WP_GARAND_SCOPE;

		case ET_WP_GPG40:
			return WP_GPG40;

		case ET_WP_GREN_ALLIES:
			return WP_GRENADE_PINEAPPLE;

		case ET_WP_GREN_AXIS:
			return WP_GRENADE_LAUNCHER;

		case ET_WP_K43:
			return WP_K43;

		case ET_WP_K43_SCOPE:
			return WP_K43_SCOPE;

		case ET_WP_KAR98:
			return WP_KAR98;

		case ET_WP_KNIFE:
			return WP_KNIFE;

		case ET_WP_LANDMINE:
			return WP_LANDMINE;

		case ET_WP_LUGER:
			return WP_LUGER;

		case ET_WP_M7:
			return WP_M7;

		case ET_WP_MEDKIT:
			return WP_MEDKIT;

		case ET_WP_MOBILE_MG42:
			return WP_MOBILE_MG42;

		case ET_WP_MOBILE_MG42_SET:
			return WP_MOBILE_MG42_SET;

		case ET_WP_MORTAR:
			return WP_MORTAR;

		case ET_WP_MORTAR_SET:
			return WP_MORTAR_SET;

		case ET_WP_MP40:
			return WP_MP40;

		case ET_WP_PANZERFAUST:
			return WP_PANZERFAUST;

		case ET_WP_PLIERS:
			return WP_PLIERS;

		case ET_WP_SATCHEL:
			return WP_SATCHEL;

		case ET_WP_SATCHEL_DET:
			return WP_SATCHEL_DET;

		case ET_WP_SILENCED_COLT:
			return WP_SILENCED_COLT;

		case ET_WP_SILENCED_LUGER:
			return WP_SILENCER;

		case ET_WP_SMOKE_GRENADE:
			return WP_SMOKE_BOMB;

		case ET_WP_SMOKE_MARKER:
			return WP_SMOKE_MARKER;

		case ET_WP_STEN:
			return WP_STEN;

		case ET_WP_SYRINGE:
			return WP_MEDIC_SYRINGE;

		case ET_WP_THOMPSON:
			return WP_THOMPSON;
#ifdef JAYMOD_name

		case 75:
			return WP_POISON_SYRINGE;

		case 76:
			return WP_ADRENALINE_SHARE;

		case 77:
			return WP_M97;

		case 78:
			return WP_POISON_GAS;

		case 79:
			return WP_LANDMINE_BBETTY;

		case 80:
			return WP_LANDMINE_PGAS;
#endif
#ifdef NOQUARTER

		case 85:
			return WP_STG44;

		case 86:
			return WP_BAR;

		case 87:
			return WP_BAR_SET;

		case 88:
			return WP_MOBILE_BROWNING;

		case 89:
			return WP_MOBILE_BROWNING_SET;

		case 90:
			return WP_SHOTGUN;

		case 91:
			return WP_MP34;

		case 92:
			return WP_MORTAR2;

		case 93:
			return WP_MORTAR2_SET;

		case 94:
			return WP_KNIFE_KABAR;

		case 95:
			return WP_STEN_MKII;

		case 96:
			return WP_BAZOOKA;

		case 98:
			return WP_VENOM;

		case 99:
			return WP_POISON_SYRINGE;

		case 100:
			return WP_FOOTKICK;
#endif

		default:
			return WP_NONE;
	}
}

int Bot_WeaponGameToBot( int weapon )
{
	switch( weapon )
	{
		case WP_MEDIC_ADRENALINE:
			return ET_WP_ADRENALINE;

		case WP_AKIMBO_COLT:
			return ET_WP_AKIMBO_COLT;

		case WP_AKIMBO_LUGER:
			return ET_WP_AKIMBO_LUGER;

		case WP_AKIMBO_SILENCEDCOLT:
			return ET_WP_AKIMBO_SILENCED_COLT;

		case WP_AKIMBO_SILENCEDLUGER:
			return ET_WP_AKIMBO_SILENCED_LUGER;

		case WP_AMMO:
			return ET_WP_AMMO_PACK;

		case WP_BINOCULARS:
			return ET_WP_BINOCULARS;

		case WP_CARBINE:
			return ET_WP_CARBINE;

		case WP_COLT:
			return ET_WP_COLT;

		case WP_DYNAMITE:
			return ET_WP_DYNAMITE;

		case WP_FG42:
			return ET_WP_FG42;

		case WP_FG42SCOPE:
			return ET_WP_FG42_SCOPE;

		case WP_FLAMETHROWER:
			return ET_WP_FLAMETHROWER;

		case WP_GARAND:
			return ET_WP_GARAND;

		case WP_GARAND_SCOPE:
			return ET_WP_GARAND_SCOPE;

		case WP_GPG40:
			return ET_WP_GPG40;

		case WP_GRENADE_PINEAPPLE:
			return ET_WP_GREN_ALLIES;

		case WP_GRENADE_LAUNCHER:
			return ET_WP_GREN_AXIS;

		case WP_K43:
			return ET_WP_K43;

		case WP_K43_SCOPE:
			return ET_WP_K43_SCOPE;

		case WP_KAR98:
			return ET_WP_KAR98;

		case WP_KNIFE:
			return ET_WP_KNIFE;

		case WP_LANDMINE:
			return ET_WP_LANDMINE;

		case WP_LUGER:
			return ET_WP_LUGER;

		case WP_M7:
			return ET_WP_M7;

		case WP_MEDKIT:
			return ET_WP_MEDKIT;

		case WP_MOBILE_MG42:
			return ET_WP_MOBILE_MG42;

		case WP_MOBILE_MG42_SET:
			return ET_WP_MOBILE_MG42_SET;

		case WP_MORTAR:
			return ET_WP_MORTAR;

		case WP_MORTAR_SET:
			return ET_WP_MORTAR_SET;

		case WP_MP40:
			return ET_WP_MP40;

		case WP_PANZERFAUST:
			return ET_WP_PANZERFAUST;

		case WP_PLIERS:
			return ET_WP_PLIERS;

		case WP_SATCHEL:
			return ET_WP_SATCHEL;

		case WP_SATCHEL_DET:
			return ET_WP_SATCHEL_DET;

		case WP_SILENCED_COLT:
			return ET_WP_SILENCED_COLT;

		case WP_SILENCER:
			return ET_WP_SILENCED_LUGER;

		case WP_SMOKE_BOMB:
			return ET_WP_SMOKE_GRENADE;

		case WP_SMOKE_MARKER:
			return ET_WP_SMOKE_MARKER;

		case WP_STEN:
			return ET_WP_STEN;

		case WP_MEDIC_SYRINGE:
			return ET_WP_SYRINGE;

		case WP_THOMPSON:
			return ET_WP_THOMPSON;
#ifdef JAYMOD_name

		case WP_POISON_SYRINGE:
			return 75;

		case WP_ADRENALINE_SHARE:
			return 76;

		case WP_M97:
			return 77;

		case WP_POISON_GAS:
			return 78;

		case WP_LANDMINE_BBETTY:
			return 79;

		case WP_LANDMINE_PGAS:
			return 80;
#endif
#ifdef NOQUARTER

		case WP_STG44:
			return 85;

		case WP_BAR:
			return 86;

		case WP_BAR_SET:
			return 87;

		case WP_MOBILE_BROWNING:
			return ET_WP_MOBILE_MG42; //cs: was 88

		case WP_MOBILE_BROWNING_SET:
			return ET_WP_MOBILE_MG42_SET; //cs: was 89

		case WP_SHOTGUN:
			return 90;

		case WP_MP34:
			return 91;

		case WP_MORTAR2:
			return ET_WP_MORTAR; //cs: was 92

		case WP_MORTAR2_SET:
			return ET_WP_MORTAR_SET; //cs: was 93

		case WP_KNIFE_KABAR:
			return ET_WP_KNIFE; //cs: was 94

		case WP_STEN_MKII:
			return 95;

		case WP_BAZOOKA:
			return 96;

		case WP_VENOM:
			return 98;

		case WP_POISON_SYRINGE:
			return 99;

		case WP_FOOTKICK:
			return 100;
#endif

		default:
			return ET_WP_NONE;
	}
}

static int Bot_HintGameToBot( gentity_t *_ent )
{
	if( _ent && _ent->client )
	{
		switch( _ent->client->ps.serverCursorHint )
		{
			case HINT_PLAYER:
				return CURSOR_HINT_PLAYER;

			case HINT_ACTIVATE:
				return CURSOR_HINT_ACTIVATE;

			case HINT_DOOR:
				return CURSOR_HINT_DOOR;

			case HINT_DOOR_ROTATING:
				return CURSOR_HINT_DOOR_ROTATING;

			case HINT_DOOR_LOCKED:
				return CURSOR_HINT_DOOR_LOCKED;

			case HINT_DOOR_ROTATING_LOCKED:
				return CURSOR_HINT_DOOR_LOCKED;

			case HINT_MG42:
				return CURSOR_HINT_MG42;

			case HINT_BREAKABLE:
				return CURSOR_HINT_BREAKABLE;

			case HINT_BREAKABLE_DYNAMITE:
				return CURSOR_HINT_BREAKABLE_DYNAMITE;

			case HINT_CHAIR:
				return CURSOR_HINT_CHAIR;

			case HINT_ALARM:
				return CURSOR_HINT_ALARM;

			case HINT_HEALTH:
				return CURSOR_HINT_HEALTH;
#ifndef NOQUARTER

			case HINT_TREASURE:
				return CURSOR_HINT_TREASURE;
				break;
#endif

			case HINT_KNIFE:
				return CURSOR_HINT_KNIFE;

			case HINT_LADDER:
				return CURSOR_HINT_LADDER;

			case HINT_BUTTON:
				return CURSOR_HINT_BUTTON;

			case HINT_WATER:
				return CURSOR_HINT_WATER;
#ifndef NOQUARTER

			case HINT_CAUTION:
				return CURSOR_HINT_CAUTION;

			case HINT_DANGER:
				return CURSOR_HINT_DANGER;

			case HINT_SECRET:
				return CURSOR_HINT_SECRET;

			case HINT_QUESTION:
				return CURSOR_HINT_QUESTION;

			case HINT_EXCLAMATION:
				return CURSOR_HINT_EXCLAMATION;

			case HINT_CLIPBOARD:
				return CURSOR_HINT_CLIPBOARD;
#endif

			case HINT_WEAPON:
				return CURSOR_HINT_WEAPON;

			case HINT_AMMO:
				return CURSOR_HINT_AMMO;
#ifndef NOQUARTER

			case HINT_ARMOR:
				return CURSOR_HINT_ARMOR;
#endif

			case HINT_POWERUP:
				return CURSOR_HINT_POWERUP;
#ifndef NOQUARTER

			case HINT_HOLDABLE:
				return CURSOR_HINT_HOLDABLE;
#endif

			case HINT_INVENTORY:
				return CURSOR_HINT_INVENTORY;

			case HINT_SCENARIC:
				return CURSOR_HINT_SCENARIC;

			case HINT_EXIT:
				return CURSOR_HINT_EXIT;

			case HINT_NOEXIT:
				return CURSOR_HINT_NOEXIT;

			case HINT_PLYR_FRIEND:
				return CURSOR_HINT_PLYR_FRIEND;

			case HINT_PLYR_NEUTRAL:
				return CURSOR_HINT_PLYR_NEUTRAL;

			case HINT_PLYR_ENEMY:
				return CURSOR_HINT_PLYR_ENEMY;

			case HINT_PLYR_UNKNOWN:
				return CURSOR_HINT_PLYR_UNKNOWN;

			case HINT_BUILD:
				return CURSOR_HINT_BUILD;

			case HINT_DISARM:
				return CURSOR_HINT_DISARM;

			case HINT_REVIVE:
				return CURSOR_HINT_REVIVE;

			case HINT_DYNAMITE:
				return CURSOR_HINT_DYNAMITE;

			case HINT_CONSTRUCTIBLE:
				return CURSOR_HINT_CONSTRUCTIBLE;

			case HINT_UNIFORM:
				return CURSOR_HINT_UNIFORM;

			case HINT_LANDMINE:
				return CURSOR_HINT_LANDMINE;

			case HINT_TANK:
				return CURSOR_HINT_TANK;

			case HINT_SATCHELCHARGE:
				return CURSOR_HINT_SATCHELCHARGE;
#ifndef NOQUARTER

			case HINT_LOCKPICK:
				return CURSOR_HINT_LOCKPICK;
#endif

			default:
				return CURSOR_HINT_NONE;
		}
	}

	return CURSOR_HINT_NONE;
}

qboolean Simple_EmplacedGunIsRepairable( gentity_t *ent )
{
	if( Q_stricmp( ent->classname, "misc_mg42" ) /*&& Q_stricmp( ent->classname, "misc_aagun" )*/ )
	{
		return qfalse;
	}

	if( ent->s.frame == 0 )
	{
		return qfalse;
	}

	return qtrue;
}

static int _choosePriWeap( gentity_t *bot, int playerClass, int team )
{
	int iSelected = 0;

	do
	{
		switch( playerClass )
		{
			case ET_CLASS_SOLDIER:
				{
#ifdef NOQUARTER

					if( jp_insanity.integer & JP_INSANITY_VENOM && ( rand() % 6 ) == 5 )
					{
						return 98;
					}
					else if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							86, // BAR
							96, // BAZOOKA
							//88, // BROWNING
							ET_WP_MOBILE_MG42,
							ET_WP_FLAMETHROWER,
							ET_WP_MORTAR,
							ET_WP_THOMPSON
						};

						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							85, // STG44
							ET_WP_PANZERFAUST,
							ET_WP_MOBILE_MG42,
							ET_WP_FLAMETHROWER,
							//92 // MORTAR2
							ET_WP_MORTAR,
							ET_WP_MP40
						};

						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}

#else
					int wpns[] =
					{
						// add shit as needed
						ET_WP_THOMPSON / ET_WP_MP40, // pointless?
						ET_WP_PANZERFAUST,
						ET_WP_MOBILE_MG42,
						ET_WP_FLAMETHROWER,
						ET_WP_MORTAR,
#ifdef JAYMOD_name
						77 //WP_M97
#endif
					};
					int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
					iSelected = wpns[ rInt ];
					break;
#endif
				}

			case ET_CLASS_MEDIC:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_THOMPSON,
#ifdef JAYMOD_name
							77 //WP_M97
#endif
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_MP40,
#ifdef JAYMOD_name
							77 //WP_M97
#endif
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			case ET_CLASS_ENGINEER:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_THOMPSON,
							ET_WP_CARBINE,
#ifdef JAYMOD_name
							77 //WP_M97
#endif
#ifdef NOQUARTER
							90 //SHOTGUN
#endif
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_MP40,
							ET_WP_KAR98,
#ifdef JAYMOD_name
							77 //WP_M97
#endif
#ifdef NOQUARTER
							90 //SHOTGUN
#endif
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			case ET_CLASS_FIELDOPS:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_THOMPSON,
#ifdef JAYMOD_name
							77 //WP_M97
#endif
#ifdef NOQUARTER
							90 //SHOTGUN
#endif
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_MP40,
#ifdef JAYMOD_name
							77 //WP_M97
#endif
#ifdef NOQUARTER
							90 //SHOTGUN
#endif
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			case ET_CLASS_COVERTOPS:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_STEN,
#ifdef NOQUARTER
							86, //BAR
#else
							ET_WP_FG42,
#endif
							ET_WP_GARAND
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
#ifdef NOQUARTER
							91, //MP34
#else
							ET_WP_STEN,
#endif
							ET_WP_FG42,
							ET_WP_K43
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			default:
				iSelected = ET_WP_NONE;
				break;
		}

#ifdef NOQUARTER
	}
	while( G_IsWeaponDisabled( bot, ( weapon_t ) _weaponBotToGame( iSelected ), qtrue ) );

#elif defined( ETPUB_VERSION )
	}

	while( G_IsWeaponDisabled( bot, ( weapon_t ) _weaponBotToGame( iSelected ), bot->client->sess.sessionTeam, qtrue ) );

#else

	} while( G_IsWeaponDisabled( bot, ( weapon_t ) _weaponBotToGame( iSelected ) ) );

#endif

	return iSelected;
}

static int _chooseSecWeap( gentity_t *bot, int playerClass, int team )
{
	int iSelected = ET_WP_NONE;

// IRATA NQ like _chooseSecWeap
#ifdef NOQUARTER

	switch( playerClass )
	{
		case ET_CLASS_SOLDIER:
		case ET_CLASS_MEDIC:
		case ET_CLASS_ENGINEER:
		case ET_CLASS_FIELDOPS:
			{
				if( team == ET_TEAM_ALLIES )
				{
					if( bot->client->sess.skill[ SK_LIGHT_WEAPONS ] < 4 )
					{
						iSelected = ET_WP_COLT;
					}
					else
					{
						int wpns[] =
						{
							ET_WP_COLT, // simple noob bots ...
							ET_WP_AKIMBO_COLT,
							ET_WP_AKIMBO_COLT
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
					}
				}
				else
				{
					if( bot->client->sess.skill[ SK_LIGHT_WEAPONS ] < 4 )
					{
						iSelected = ET_WP_LUGER;
					}
					else
					{
						int wpns[] =
						{
							ET_WP_LUGER, // simple noob bots ...
							ET_WP_AKIMBO_LUGER,
							ET_WP_AKIMBO_LUGER
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
					}
				}
			}
			break;

		case ET_CLASS_COVERTOPS:
			{
				if( team == ET_TEAM_ALLIES )
				{
					if( bot->client->sess.skill[ SK_LIGHT_WEAPONS ] < 4 )
					{
						iSelected = ET_WP_SILENCED_COLT;
					}
					else
					{
						int wpns[] =
						{
							ET_WP_SILENCED_COLT, // simple noob bots ...
							ET_WP_AKIMBO_SILENCED_COLT,
							ET_WP_AKIMBO_SILENCED_COLT
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
					}
				}
				else
				{
					if( bot->client->sess.skill[ SK_LIGHT_WEAPONS ] < 4 )
					{
						iSelected = ET_WP_SILENCED_LUGER;
					}
					else
					{
						int wpns[] =
						{
							ET_WP_SILENCED_LUGER, // simple noob bots ...
							ET_WP_AKIMBO_SILENCED_LUGER,
							ET_WP_AKIMBO_SILENCED_LUGER
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
					}
				}
			}
			break;

		default:
			iSelected = ET_WP_NONE;
			break;
	}

// common
// IRATA @ botteam:
// In fact G_IsWeaponDisabled won't return true for secondary weapons it only checks for HW in most mods & vanilla
// I did fix inconsistency of return & break usage
#else

	do
	{
		switch( playerClass )
		{
			case ET_CLASS_SOLDIER:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_COLT,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_LUGER,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			case ET_CLASS_MEDIC:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_COLT,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_LUGER,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			case ET_CLASS_ENGINEER:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_COLT,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_LUGER,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			case ET_CLASS_FIELDOPS:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_COLT,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_LUGER,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			case ET_CLASS_COVERTOPS:
				{
					if( team == ET_TEAM_ALLIES )
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_SILENCED_COLT,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
					else
					{
						int wpns[] =
						{
							// add shit as needed
							ET_WP_SILENCED_LUGER,
						};
						int rInt = rand() % ( sizeof( wpns ) / sizeof( wpns[ 0 ] ) );
						iSelected = wpns[ rInt ];
						break;
					}
				}

			default:
				iSelected = ET_WP_NONE;
				break;
		}

#ifdef ETPUB_VERSION
	}
	while( G_IsWeaponDisabled( bot, ( weapon_t ) _weaponBotToGame( iSelected ), bot->client->sess.sessionTeam, qtrue ) );

#else
	}

	while( G_IsWeaponDisabled( bot, ( weapon_t ) _weaponBotToGame( iSelected ) ) );

#endif // ETPUB_VERSION
#endif // NOQUARTER

	return iSelected;
}

static void ReTransmitWeapons( const gentity_t *bot )
{
	if( !bot || !bot->client )
	{
		return;
	}

	Bot_Event_ResetWeapons( bot - g_entities );

	for( int weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; ++weapon )
	{
		if( COM_BitCheck( bot->client->ps.weapons, weapon ) )
		{
			Bot_Event_AddWeapon( bot - g_entities, Bot_WeaponGameToBot( weapon ) );
		}
	}
}

#define MAX_SMOKE_RADIUS         320.0
#define MAX_SMOKE_RADIUS_TIME    10000.0
#define UNAFFECTED_BY_SMOKE_DIST SQR(100)

gentity_t *Bot_EntInvisibleBySmokeBomb( vec3_t start, vec3_t end )
{
	// if the target is close enough, vision is not affected by smoke bomb
	if( DistanceSquared( start, end ) < UNAFFECTED_BY_SMOKE_DIST )
	{
		//pfnPrintMessage("within unaffected dist");
		return 0;
	}

	gentity_t *ent = NULL;
	vec3_t smokeCenter;
	float smokeRadius;

	//while (ent = G_FindSmokeBomb( ent ))
	for( int i = 0; i < MAX_SMOKEGREN_CACHE; ++i )
	{
		ent = g_SmokeGrenadeCache[ i ];

		if( !ent )
		{
			continue;
		}

		if( ent->s.effect1Time == 16 )
		{
			//pfnPrintMessage("smoke not up yet");
			// xkan, the smoke has not really started yet, see weapon_smokeBombExplode
			// and CG_RenderSmokeGrenadeSmoke
			continue;
		}

		// check the distance
		VectorCopy( ent->s.pos.trBase, smokeCenter );
		// raise the center to better match the position of the smoke, see
		// CG_SpawnSmokeSprite().
		smokeCenter[ 2 ] += 32;
		// smoke sprite has a maximum radius of 640/2. and it takes a while for it to
		// reach that size, so adjust the radius accordingly.
		smokeRadius = MAX_SMOKE_RADIUS * ( ( level.time - ent->grenadeExplodeTime ) / MAX_SMOKE_RADIUS_TIME );

		if( smokeRadius > MAX_SMOKE_RADIUS )
		{
			smokeRadius = MAX_SMOKE_RADIUS;
		}

		// if distance from line is short enough, vision is blocked by smoke

		if( DistanceFromLineSquared( smokeCenter, start, end ) < ( smokeRadius * smokeRadius ) )
		{
			//// if smoke is farther, we're less sensitive to being hidden.

			//const float DistToEnd = VectorDistanceSquared(start,end);
			//const float DistToSmoke = VectorDistanceSquared(start,smokeCenter);

			//g_InterfaceFunctions->DebugLine(smokeCenter, end,obColor(0,255,0),0.2f);
			//pfnPrintMessage("hid by smoke");
			return ent;
		}

		//pfnAddTempDisplayLine(smokeCenter, start, fColorRed);
		//g_InterfaceFunctions->DebugLine(smokeCenter, end,obColor(255,0,0),0.2f);
	}

	return 0;
}

void Bot_Util_AddGoal( const char *_type, gentity_t *_ent, int _team, const char *_tag, const char *_extrakey = 0, obUserData *_extraval = 0 )
{
	if( IsOmnibotLoaded() )
	{
		MapGoalDef goaldef;

		goaldef.Props.SetString( "Type", _type );
		goaldef.Props.SetEntity( "Entity", HandleFromEntity( _ent ) );
		goaldef.Props.SetInt( "Team", _team );
		goaldef.Props.SetString( "TagName", _tag );
		goaldef.Props.SetInt( "InterfaceGoal", 1 );

		if( _extrakey && _extraval )
		{
			goaldef.Props.Set( _extrakey, *_extraval );
		}

		g_BotFunctions.pfnAddGoal( goaldef );
	}
}

static int _GetEntityTeam( gentity_t *_ent )
{
	// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
	if( _ent->client && ( _ent - g_entities ) < MAX_CLIENTS )
	{
		//t = ET_PLAYER;
		return Bot_TeamGameToBot( _ent->client->sess.sessionTeam );
	}

	int t = _ent->s.eType;

	switch( t )
	{
		case ET_PLAYER:
			return Bot_TeamGameToBot( _ent->client->sess.sessionTeam );

		case ET_CORPSE:
			return Bot_TeamGameToBot( BODY_TEAM( _ent ) );

		case ET_MISSILE:
			if( _ent->s.weapon == WP_LANDMINE || _ent->s.weapon == WP_DYNAMITE )
			{
				return Bot_TeamGameToBot( G_LandmineTeam( _ent ) );
			}

			// Let this fall through
		default:
			return Bot_TeamGameToBot( _ent->s.teamNum );
	}

	return 0;
}

static int _GetEntityClass( gentity_t *_ent )
{
	// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
	int t = _ent->s.eType;

	if( _ent->client && ( _ent - g_entities ) < MAX_CLIENTS )
	{
		t = ET_PLAYER;
	}

	switch( t )
	{
		case ET_GENERAL:
			{
				if( !Q_stricmp( _ent->classname, "func_invisible_user" ) )
				{
					return ENT_CLASS_GENERIC_BUTTON;
				}
				else if( !Q_stricmp( _ent->classname, "func_button" ) )
				{
					return ENT_CLASS_GENERIC_BUTTON;
				}
				else if( !Q_stricmp( _ent->classname, "misc_mg42" ) )
				{
					return ET_CLASSEX_MG42MOUNT;
				}
				else if( !Q_stricmp( _ent->classname, "props_chair_hiback" ) )
				{
					return ET_CLASSEX_BROKENCHAIR;
				}
				else if( !Q_stricmp( _ent->classname, "props_chair" ) )
				{
					return ET_CLASSEX_BROKENCHAIR;
				}
				else if( !Q_stricmp( _ent->classname, "props_chair_side" ) )
				{
					return ET_CLASSEX_BROKENCHAIR;
				}

				// cs: waypoint tool, don't merge the spawns
				else if( !Q_stricmp( _ent->classname, "info_player_deathmatch" ) ||
				         !Q_stricmp( _ent->classname, "team_CTF_redspawn" ) ||
				         !Q_stricmp( _ent->classname, "team_CTF_bluespawn" ) ||
				         !Q_stricmp( _ent->classname, "info_player_spawn" ) )
				{
					return ENT_CLASS_GENERIC_PLAYERSTART;
				}

				break;
			}

		case ET_INVISIBLE:
			{
				if( _ent->client )
				{
					return ENT_CLASS_GENERIC_SPECTATOR;
				}

				break;
			}

		case ET_PLAYER:
			{
				if( !_ent->client || ( _ent->entstate == STATE_INVISIBLE ) ||
				    ( _ent->client->sess.sessionTeam != TEAM_AXIS &&
				      _ent->client->sess.sessionTeam != TEAM_ALLIES ) )
				{
					return ENT_CLASS_GENERIC_SPECTATOR;
				}

				// Special case for dead players that haven't respawned.

				/*if(_ent->health <= 0 && _ent->client->ps.pm_type == PM_DEAD)
				return ET_CLASSEX_INJUREDPLAYER;*/

				// for scripted class changes, count latched in warmup or if dead

				/*if ( g_gamestate.integer > GS_PLAYING
				|| _ent->client->ps.pm_type > PM_SPECTATOR
				|| _ent->client->ps.pm_flags & PMF_LIMBO )
				return Bot_PlayerClassGameToBot(_ent->client->sess.latchPlayerType);*/

				return Bot_PlayerClassGameToBot( _ent->client->sess.latchPlayerType );
			}

		case ET_ITEM:
			{
				if( !Q_strncmp( _ent->classname, "item_health", strlen( "item_health" ) ) )
				{
					return ENT_CLASS_GENERIC_HEALTH;
				}
				else if( !Q_strncmp( _ent->classname, "weapon_magicammo", strlen( "weapon_magicammo" ) ) )
				{
					return ENT_CLASS_GENERIC_AMMO;
				}
				else if( !Q_stricmp( _ent->classname, "item_treasure" ) )
				{
					return ET_CLASSEX_TREASURE;
				}
				else if( _ent->item && _ent->item->giType == IT_WEAPON )
				{
					return ET_CLASSEX_WEAPON + Bot_WeaponGameToBot( _ent->item->giTag );
				}

				break;
			}

		case ET_CORPSE:
			{
				return ET_CLASSEX_CORPSE;
			}

		case ET_MISSILE:
			{
				// Register certain weapons as threats to avoid or whatever.
				switch( _ent->s.weapon )
				{
					case WP_GRENADE_LAUNCHER:
					case WP_GRENADE_PINEAPPLE:
						return ET_CLASSEX_GRENADE;

					case WP_PANZERFAUST:
						return ET_CLASSEX_ROCKET;
#ifdef NOQUARTER

					case WP_BAZOOKA:
						return ET_CLASSEX_ROCKET;
#endif

					case WP_ARTY:
						return ET_CLASSEX_ARTY;

					case WP_DYNAMITE:
						return ET_CLASSEX_DYNAMITE;

					case WP_SMOKE_MARKER:
						return ET_CLASSEX_SMOKEMARKER;

					case WP_SMOKE_BOMB:
						return ET_CLASSEX_SMOKEBOMB;

					case WP_LANDMINE:
						return ET_CLASSEX_MINE;

					case WP_SATCHEL:
						return ET_CLASSEX_SATCHEL;

					case WP_M7:
						return ET_CLASSEX_M7_GRENADE;

					case WP_GPG40:
						return ET_CLASSEX_GPG40_GRENADE;

					case WP_MORTAR_SET:
						return ET_CLASSEX_MORTAR;
#ifdef NOQUARTER

					case WP_MORTAR2_SET:
						return ET_CLASSEX_MORTAR;
#endif

					default:
						if( !Q_strncmp( _ent->classname, "air strike", sizeof( "air strike" ) ) )
						{
							return ET_CLASSEX_AIRSTRIKE;
						}
				}

				break;
			}

		case ET_FLAMETHROWER_CHUNK:
			{
				return ET_CLASSEX_FLAMECHUNK;
				break;
			}

		case ET_MOVER:
			{
				if( !Q_stricmp( _ent->classname, "script_mover" ) )
				{
					if( _ent->count > 0 )
					{
						return ( _ent->spawnflags & 4 ) ? ET_CLASSEX_VEHICLE_HVY : ET_CLASSEX_VEHICLE;
					}

					//if(_ent->model2)
					return ET_CLASSEX_VEHICLE_NODAMAGE;
				}

				/*else if (!Q_stricmp(pCurrent->classname, "props_flamebarrel"))
				{
				info.m_EntityClass = ET_CLASSEX_BREAKABLE;
				info.m_EntityCategory = ENT_CAT_SHOOTABLE;
				}
				else if (!Q_stricmp(pCurrent->classname, "props_statue"))
				{
				info.m_EntityClass = ET_CLASSEX_BREAKABLE;
				info.m_EntityCategory = ENT_CAT_SHOOTABLE;
				}*/
				else if( !Q_stricmp( _ent->classname, "props_chair_hiback" ) )
				{
					if( ( _ent->health > 0 ) && ( _ent->takedamage == qtrue ) )
					{
						return ET_CLASSEX_BREAKABLE;
					}
				}
				else if( !Q_stricmp( _ent->classname, "props_chair" ) )
				{
					if( ( _ent->health > 0 ) && ( _ent->takedamage == qtrue ) )
					{
						return ET_CLASSEX_BREAKABLE;
					}
				}
				else if( !Q_stricmp( _ent->classname, "props_chair_side" ) )
				{
					if( ( _ent->health > 0 ) && ( _ent->takedamage == qtrue ) )
					{
						return ET_CLASSEX_BREAKABLE;
					}
				}

				break;
			}

		case ET_MG42_BARREL:
			{
				return ET_CLASSEX_MG42MOUNT;
			}

			//case ET_AAGUN:
			//  {
			//    if((pCurrent->health > 0) &&
			//    (pCurrent->entstate != STATE_INVISIBLE) &&
			//    (pCurrent->entstate != STATE_UNDERCONSTRUCTION))
			//    {
			//    }
			//    break;
			//  }
		case ET_EXPLOSIVE:
			{
				if( !( _ent->spawnflags & EXPLOSIVE_TANK ) &&
				    ( _ent->constructibleStats.weaponclass != 1 ) &&
				    ( _ent->constructibleStats.weaponclass != 2 ) ) // &&
					//(_ent->health > 0) && (_ent->takedamage == qtrue))
				{
					return ET_CLASSEX_BREAKABLE;
				}

				break;
			}

		case ET_CONSTRUCTIBLE:
			{
				//if (G_ConstructionIsPartlyBuilt(pCurrent) &&
				//  !(pCurrent->spawnflags & CONSTRUCTIBLE_INVULNERABLE) &&
				//   (pCurrent->constructibleStats.weaponclass == 0))
				//{
				//  info.m_EntityClass = ET_CLASSEX_BREAKABLE;
				//  info.m_EntityCategory = ENT_CAT_SHOOTABLE;
				//}
				//else
				//{
				//continue;
				//}
				break;
			}

		case ET_HEALER:
			{
				return ET_CLASSEX_HEALTHCABINET;
			}

		case ET_SUPPLIER:
			{
				return ET_CLASSEX_AMMOCABINET;
			}

		case ET_OID_TRIGGER:
			{
				return ENT_CLASS_GENERIC_GOAL;
			}

		default:
			break;
	};

	return 0;
}

qboolean _TankIsMountable( gentity_t *_ent )
{
	if( !( _ent->spawnflags & 128 ) )
	{
		return qfalse;
	}

	if( level.disableTankEnter )
	{
		return qfalse;
	}

	/*if( G_TankIsOccupied( ent ) )
	return qfalse;*/
	if( g_OmniBotFlags.integer & OBF_DONT_MOUNT_TANKS )
	{
		return qfalse;
	}

	if( _ent->health <= 0 )
	{
		return qfalse;
	}

	return qtrue;
}

qboolean _EmplacedGunIsMountable( gentity_t *_ent )
{
	if( g_OmniBotFlags.integer & OBF_DONT_MOUNT_GUNS )
	{
		return qfalse;
	}

	return qtrue;
}

void Omnibot_Load_PrintMsg( const char *_msg )
{
	G_Printf( "Omni-bot: %s%s\n", S_COLOR_GREEN, _msg );
}

void Omnibot_Load_PrintErr( const char *_msg )
{
	G_Printf( "Omni-bot: %s%s\n", S_COLOR_RED, _msg );
}

int g_LastScreenMessageTime = 0;

int obUtilBotContentsFromGameContents( int _contents )
{
	int iBotContents = 0;

	if( _contents & CONTENTS_SOLID )
	{
		iBotContents |= CONT_SOLID;
	}

	if( _contents & CONTENTS_WATER )
	{
		iBotContents |= CONT_WATER;
	}

	if( _contents & CONTENTS_SLIME )
	{
		iBotContents |= CONT_SLIME;
	}

	if( _contents & CONTENTS_FOG )
	{
		iBotContents |= CONT_FOG;
	}

	if( _contents & CONTENTS_TELEPORTER )
	{
		iBotContents |= CONT_TELEPORTER;
	}

	if( _contents & CONTENTS_MOVER )
	{
		iBotContents |= CONT_MOVER;
	}

	if( _contents & CONTENTS_TRIGGER )
	{
		iBotContents |= CONT_TRIGGER;
	}

	if( _contents & CONTENTS_LAVA )
	{
		iBotContents |= CONT_LAVA;
	}

	if( _contents & CONTENTS_PLAYERCLIP )
	{
		iBotContents |= CONT_PLYRCLIP;
	}

	return iBotContents;
}

int obUtilBotSurfaceFromGameSurface( int _surfaceflags )
{
	int iBotSurface = 0;

	if( _surfaceflags & SURF_SLICK )
	{
		iBotSurface |= SURFACE_SLICK;
	}

	if( _surfaceflags & SURF_LADDER )
	{
		iBotSurface |= SURFACE_LADDER;
	}

	return iBotSurface;
}

void Bot_Util_CheckForGoalEntity( GameEntity _ent )
{
	if( IsOmnibotLoaded() )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt->inuse )
		{
			switch( pEnt->s.eType )
			{
				case ET_ITEM:
					{
						char buffer[ 256 ] = { 0 };
						const char *pGoalName = _GetEntityName( pEnt );

						if( !Q_stricmp( pEnt->classname, "team_CTF_redflag" ) )
						{
							// allies flag
							if( pEnt->s.otherEntityNum != -1 )
							{
								pGoalName = _GetEntityName( &g_entities[ pEnt->s.otherEntityNum ] );
							}

							sprintf( buffer, "%s_dropped", pGoalName ? pGoalName : "allies_flag" );
							Bot_Util_AddGoal( "flag", pEnt, ( 1 << ET_TEAM_ALLIES ), buffer );
							Bot_Util_AddGoal( "flagreturn", pEnt, ( 1 << ET_TEAM_AXIS ), buffer );
						}
						else if( !Q_stricmp( pEnt->classname, "team_CTF_blueflag" ) )
						{
							// axis flag
							if( pEnt->s.otherEntityNum != -1 )
							{
								pGoalName = _GetEntityName( &g_entities[ pEnt->s.otherEntityNum ] );
							}

							sprintf( buffer, "%s_dropped", pGoalName ? pGoalName : "axis_flag" );
							Bot_Util_AddGoal( "flag", pEnt, ( 1 << ET_TEAM_AXIS ), buffer );
							Bot_Util_AddGoal( "flagreturn", pEnt, ( 1 << ET_TEAM_ALLIES ), buffer );
						}

						break;
					}

				default:
					break;
			}
		}
	}
}

// helper stuff
qboolean InFieldOfVision( vec3_t viewangles, float fov, vec3_t angles )
{
	float diff, angle;

	for( int i = 0; i < 2; ++i )
	{
		angle = AngleMod( viewangles[ i ] );
		angles[ i ] = AngleMod( angles[ i ] );
		diff = angles[ i ] - angle;

		if( angles[ i ] > angle )
		{
			if( diff > 180.0 ) { diff -= 360.0; }
		}
		else
		{
			if( diff < -180.0 ) { diff += 360.0; }
		}

		if( diff > 0 )
		{
			if( diff > fov * 0.5 ) { return qfalse; }
		}
		else
		{
			if( diff < -fov * 0.5 ) { return qfalse; }
		}
	}

	return qtrue;
}

class ETInterface : public IEngineInterface
{
public:
	int AddBot( const MessageHelper &_data )
	{
		// wait until everything settles before adding bots on dedicated servers
		if( g_dedicated.integer && ( level.time - level.startTime < 10000 ) )
		{
			return -1;
		}

		OB_GETMSG( Msg_Addbot );

		int num = trap_BotAllocateClient( 0 );

		if( num < 0 )
		{
			PrintError( "Could not add bot!" );
			PrintError( "No free slots!" );
			return -1;
		}

		char userinfo[ MAX_INFO_STRING ] = { 0 };

		std::stringstream guid;
		guid << "OMNIBOT" << std::setw( 2 ) << std::setfill( '0' ) << num << std::right << std::setw( 23 ) << "";

		gentity_t *bot = &g_entities[ num ];

		Info_SetValueForKey( userinfo, "name", pMsg->m_Name );
		Info_SetValueForKey( userinfo, "rate", "25000" );
		Info_SetValueForKey( userinfo, "snaps", "20" );
		Info_SetValueForKey( userinfo, "ip", "localhost" );
		Info_SetValueForKey( userinfo, "cl_guid", guid.str().c_str() );

		trap_SetUserinfo( num, userinfo );

		const char *s = 0;

		if( ( s = ClientConnect( num, qtrue, qtrue ) ) != 0 )
		{
			PrintError( va( "Could not connect bot: %s", s ) );
			num = -1;
		}

		// bad hack to prevent unhandled errors being returned as successful connections
		return bot && bot->inuse ? num : -1;
	}

	void RemoveBot( const MessageHelper &_data )
	{
		OB_GETMSG( Msg_Kickbot );

		if( pMsg->m_GameId != Msg_Kickbot::InvalidGameId )
		{
			if( pMsg->m_GameId >= 0 && pMsg->m_GameId < MAX_CLIENTS )
			{
				gentity_t *ent = &g_entities[ pMsg->m_GameId ];

				if( IsBot( ent ) )
				{
					trap_DropClient( pMsg->m_GameId, "disconnected", 0 );
				}
			}
		}
		else
		{
			char cleanNetName[ MAX_NETNAME ];
			char cleanName[ MAX_NAME_LENGTH ];

			Q_strncpyz( cleanName, pMsg->m_Name, MAX_NAME_LENGTH );
			Q_CleanStr( cleanName );

			for( int i = 0; i < g_maxclients.integer; ++i )
			{
				if( !g_entities[ i ].inuse )
				{
					continue;
				}

				if( !IsBot( &g_entities[ i ] ) )
				{
					continue;
				}

				// clean stuff
				Q_strncpyz( cleanNetName, g_entities[ i ].client->pers.netname, MAX_NETNAME );
				Q_CleanStr( cleanNetName );

				if( !Q_stricmp( cleanNetName, cleanName ) )
				{
					trap_DropClient( i, "disconnected", 0 );
				}
			}
		}
	}

	obResult ChangeTeam( int _client, int _newteam, const MessageHelper *_data )
	{
#ifdef NOQUARTER
		const char *teamName;
#else
		char *teamName;
#endif
		gentity_t *bot = &g_entities[ _client ];

		// find a team if we didn't get one and we need one ;-)
		if( _newteam != ET_TEAM_ALLIES && _newteam != ET_TEAM_AXIS )
		{
			if( ( _newteam == RANDOM_TEAM ) ||
			    ( bot->client->sess.sessionTeam != TEAM_AXIS &&
			      bot->client->sess.sessionTeam != TEAM_ALLIES ) )
			{
				if( TeamCount( _client, TEAM_ALLIES ) <= TeamCount( _client, TEAM_AXIS ) )
				{
					_newteam = ET_TEAM_ALLIES;
				}
				else
				{
					_newteam = ET_TEAM_AXIS;
				}
			}
			else
			{
				// remember old team
				_newteam = Bot_TeamGameToBot( bot->client->sess.sessionTeam );
			}
		}

		if( _newteam == ET_TEAM_AXIS )
		{
			teamName = "axis";
		}
		else
		{
			teamName = "allies";
		}

		// always go to spectator first to solve problems on map restarts
		//SetTeam(bot, "spectator", qtrue, -1, -1, qfalse);

		Msg_PlayerChooseEquipment *pMsg = 0;

		if( _data ) { _data->Get2( pMsg ); }

		if( pMsg )
		{
			if( pMsg->m_WeaponChoice[ 0 ] )
			{
				bot->client->sess.latchPlayerWeapon = _weaponBotToGame( pMsg->m_WeaponChoice[ 0 ] );
			}

			if( pMsg->m_WeaponChoice[ 1 ] )
			{
				bot->client->sess.latchPlayerWeapon2 = _weaponBotToGame( pMsg->m_WeaponChoice[ 1 ] );
			}

#ifdef NOQUARTER

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon, qtrue ) )
			{
				bot->client->sess.latchPlayerWeapon = 0;
			}

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon2, qtrue ) )
			{
				bot->client->sess.latchPlayerWeapon2 = 0;
			}

#elif defined( ETPUB_VERSION )

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon, bot->client->sess.sessionTeam, qtrue ) )
			{
				bot->client->sess.latchPlayerWeapon = 0;
			}

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon2, bot->client->sess.sessionTeam, qtrue ) )
			{
				bot->client->sess.latchPlayerWeapon2 = 0;
			}

#else

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon ) )
			{
				bot->client->sess.latchPlayerWeapon = 0;
			}

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon2 ) )
			{
				bot->client->sess.latchPlayerWeapon2 = 0;
			}

#endif
		}

		{
			const int iBotTeam = Bot_TeamGameToBot( bot->client->sess.sessionTeam );
			const int iBotClass = Bot_PlayerClassGameToBot( bot->client->sess.latchPlayerType );

			//G_IsWeaponDisabled ?
			if( !bot->client->sess.latchPlayerWeapon || iBotTeam != _newteam )
			{
				bot->client->sess.latchPlayerWeapon = _weaponBotToGame( _choosePriWeap( bot, iBotClass, _newteam ) );
			}

			if( !bot->client->sess.latchPlayerWeapon2 || iBotTeam != _newteam )
			{
				bot->client->sess.latchPlayerWeapon2 = _weaponBotToGame( _chooseSecWeap( bot, iBotClass, _newteam ) );
			}
		}

		//if (!SetTeam(bot, teamName, qtrue, -1, -1, qfalse)) {
		//  pfnPrintError("Bot could not join team!");
		//  return 0;
		//}

		// if SetTeam() fails, be sure to at least send a note to the bot about the current team
		// (else this won't be neccessary because on respawn messages will be sent automatically)
		if( !SetTeam( bot, teamName, qtrue, ( weapon_t ) - 1, ( weapon_t ) - 1, qfalse ) )
		{
			// also retransmit weapons stuff
			//ReTransmitWeapons(bot);
		}

		return Success;
	}

	obResult ChangeClass( int _client, int _newclass, const MessageHelper *_data )
	{
		gentity_t *bot = &g_entities[ _client ];

		// find playerclass if we didn't got one
		if( _newclass <= ET_CLASS_NULL || _newclass >= ET_CLASS_MAX )
		{
			if( ( _newclass == RANDOM_CLASS ) || ( bot->client->sess.latchPlayerType < 0 ) ||
			    ( bot->client->sess.latchPlayerType >= NUM_PLAYER_CLASSES ) )
			{
				team_t predictedTeam = bot->client->sess.sessionTeam;

				if( predictedTeam != TEAM_ALLIES && predictedTeam != TEAM_AXIS )
				{
					if( TeamCount( _client, TEAM_ALLIES ) <= TeamCount( _client, TEAM_AXIS ) )
					{
						predictedTeam = TEAM_ALLIES;
					}
					else
					{
						predictedTeam = TEAM_AXIS;
					}
				}

				// cs: make sure one of each if min is greater than zero
				int engineers = CountPlayerClass( predictedTeam, PC_ENGINEER, _client );
				int medics = CountPlayerClass( predictedTeam, PC_MEDIC, _client );
				int fieldops = CountPlayerClass( predictedTeam, PC_FIELDOPS, _client );
				int soldiers = CountPlayerClass( predictedTeam, PC_SOLDIER, _client );
				int covops = CountPlayerClass( predictedTeam, PC_COVERTOPS, _client );

				if( OMNIBOT_MIN_ENG > 0 && engineers == 0 )
				{
					_newclass = ET_CLASS_ENGINEER;
				}
				else if( OMNIBOT_MIN_MED > 0 && medics == 0 )
				{
					_newclass = ET_CLASS_MEDIC;
				}
				else if( OMNIBOT_MIN_FOP > 0 && fieldops == 0 )
				{
					_newclass = ET_CLASS_FIELDOPS;
				}
				else if( OMNIBOT_MIN_SOL > 0 && soldiers == 0 )
				{
					_newclass = ET_CLASS_SOLDIER;
				}
				else if( OMNIBOT_MIN_COP > 0 && covops == 0 )
				{
					_newclass = ET_CLASS_COVERTOPS;
				}
				else if( engineers < OMNIBOT_MIN_ENG )
				{
					_newclass = ET_CLASS_ENGINEER;
				}
				else if( medics < OMNIBOT_MIN_MED )
				{
					_newclass = ET_CLASS_MEDIC;
				}
				else if( fieldops < OMNIBOT_MIN_FOP )
				{
					_newclass = ET_CLASS_FIELDOPS;
				}
				else if( soldiers < OMNIBOT_MIN_SOL )
				{
					_newclass = ET_CLASS_SOLDIER;
				}
				else if( covops < OMNIBOT_MIN_COP )
				{
					_newclass = ET_CLASS_COVERTOPS;
				}
				else
				{
					_newclass = Bot_PlayerClassGameToBot( rand() % NUM_PLAYER_CLASSES );
				}

				// old stuff

				/*if (CountPlayerClass(predictedTeam, PC_ENGINEER) < OMNIBOT_MIN_ENG) {
				_newclass = ET_CLASS_ENGINEER;
				} else if (CountPlayerClass(predictedTeam, PC_MEDIC) < OMNIBOT_MIN_MED) {
				_newclass = ET_CLASS_MEDIC;
				} else if (CountPlayerClass(predictedTeam, PC_FIELDOPS) < OMNIBOT_MIN_FOP) {
				_newclass = ET_CLASS_FIELDOPS;
				} else if (CountPlayerClass(predictedTeam, PC_SOLDIER) < OMNIBOT_MIN_SOL) {
				_newclass = ET_CLASS_SOLDIER;
				} else if (CountPlayerClass(predictedTeam, PC_COVERTOPS) < OMNIBOT_MIN_COP) {
				_newclass = ET_CLASS_COVERTOPS;
				} else {
				_newclass = Bot_PlayerClassGameToBot(rand() % NUM_PLAYER_CLASSES);
				}*/
			}
			else
			{
				_newclass = Bot_PlayerClassGameToBot( bot->client->sess.latchPlayerType );
			}
		}

		int team = Bot_TeamGameToBot( bot->client->sess.sessionTeam );
		bot->client->sess.latchPlayerType = playerClassBotToGame( _newclass );

		Msg_PlayerChooseEquipment *pMsg = 0;

		if( _data ) { _data->Get2( pMsg ); }

		if( pMsg )
		{
			if( pMsg->m_WeaponChoice[ 0 ] )
			{
				bot->client->sess.latchPlayerWeapon = _weaponBotToGame( pMsg->m_WeaponChoice[ 0 ] );
			}

			if( pMsg->m_WeaponChoice[ 1 ] )
			{
				bot->client->sess.latchPlayerWeapon2 = _weaponBotToGame( pMsg->m_WeaponChoice[ 1 ] );
			}

#ifdef NOQUARTER

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon, qtrue ) )
			{
				bot->client->sess.latchPlayerWeapon = 0;
			}

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon2, qtrue ) )
			{
				bot->client->sess.latchPlayerWeapon2 = 0;
			}

#elif defined( ETPUB_VERSION )

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon, bot->client->sess.sessionTeam, qtrue ) )
			{
				bot->client->sess.latchPlayerWeapon = 0;
			}

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon2, bot->client->sess.sessionTeam, qtrue ) )
			{
				bot->client->sess.latchPlayerWeapon2 = 0;
			}

#else

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon ) )
			{
				bot->client->sess.latchPlayerWeapon = 0;
			}

			if( G_IsWeaponDisabled( bot, ( weapon_t ) bot->client->sess.latchPlayerWeapon2 ) )
			{
				bot->client->sess.latchPlayerWeapon2 = 0;
			}

#endif
		}

		{
			const int iBotClass = Bot_PlayerClassGameToBot( bot->client->sess.latchPlayerType );

			if( !bot->client->sess.latchPlayerWeapon || bot->client->sess.latchPlayerType != bot->client->sess.playerType )
			{
				bot->client->sess.latchPlayerWeapon = _weaponBotToGame( _choosePriWeap( bot, iBotClass, team ) );
			}

			if( !bot->client->sess.latchPlayerWeapon2 || bot->client->sess.latchPlayerType != bot->client->sess.playerType )
			{
				bot->client->sess.latchPlayerWeapon2 = _weaponBotToGame( _chooseSecWeap( bot, iBotClass, team ) );
			}
		}

		// commit suicide to ensure new class is used
		// CS: wait until 2 seconds before next spawn
		if( bot->client->sess.latchPlayerType != bot->client->sess.playerType )
		{
			//round end.
			if( bot->client->ps.pm_flags & PMF_TIME_LOCKPLAYER )
			{
				Cmd_Kill_f( bot );
			}
			else if( !( bot->client->ps.pm_flags & PMF_LIMBO ) )
			{
				bot->client->sess.botSuicide = qtrue;
			}
		}
		else
		{
			// also retransmit weapons stuff
			ReTransmitWeapons( bot );
		}

		return Success;
	}

	bool DebugLine( const float _start[ 3 ], const float _end[ 3 ], const obColor &_color, float _time )
	{
		// for dedicated servers we tell the bot we can handle this function, so it doesn't open
		// an IPC channel.
		if( g_dedicated.integer )
		{
			return true;
		}

		return false;
	}

	bool DebugRadius( const float _pos[ 3 ], const float _radius, const obColor &_color, float _time )
	{
		// for dedicated servers we tell the bot we can handle this function, so it doesn't open
		// an IPC channel.
		if( g_dedicated.integer )
		{
			return true;
		}

		return false;
	}

	void UpdateBotInput( int _client, const ClientInput &_input )
	{
		static usercmd_t cmd;
		gentity_t *bot = &g_entities[ _client ];

		// only causes problems
		bot->client->ps.pm_flags &= ~PMF_RESPAWNED;

		memset( &cmd, 0, sizeof( cmd ) );

		cmd.identClient = _client;
		cmd.serverTime = level.time;

		// Set the weapon
		cmd.weapon = _weaponBotToGame( _input.m_CurrentWeapon );

#ifdef NOQUARTER

		// cs: nq bots need to select alt versions of mobile mg and mortar
		if( bot->client->sess.sessionTeam == TEAM_ALLIES )
		{
			if( cmd.weapon == WP_MOBILE_MG42 )
			{
				cmd.weapon = WP_MOBILE_BROWNING;
			}
			else if( cmd.weapon == WP_MOBILE_MG42_SET )
			{
				cmd.weapon = WP_MOBILE_BROWNING_SET;
			}
			else if( cmd.weapon == WP_KNIFE )
			{
				cmd.weapon = WP_KNIFE_KABAR;
			}
		}
		else if( bot->client->sess.sessionTeam == TEAM_AXIS )
		{
			if( cmd.weapon == WP_MORTAR )
			{
				cmd.weapon = WP_MORTAR2;
			}
			else if( cmd.weapon == WP_MORTAR_SET )
			{
				cmd.weapon = WP_MORTAR2_SET;
			}
		}

#endif //NOQUARTER

		// dont choose scoped directly.
		switch( cmd.weapon )
		{
			case WP_GARAND_SCOPE:
				cmd.weapon = WP_GARAND;
				break;

			case WP_FG42SCOPE:
				cmd.weapon = WP_FG42;
				break;

			case WP_K43_SCOPE:
				cmd.weapon = WP_K43;
				break;
		}

		if( cmd.weapon == WP_BINOCULARS )
		{
			cmd.wbuttons |= WBUTTON_ZOOM;
		}

		// If trying to switch to rifle nade from anything other than the base rifle, switch to base first
#ifdef NOQUARTER

		if( cmd.weapon == WP_GPG40 && bot->client->ps.weapon == WP_GPG40 /*&& bot->client->ps.weapon != WP_KAR98*/ )
		{
			const int ammo = bot->client->ps.ammoclip[ WeaponTable[ WP_GPG40 ].clipindex ];

			if( ammo == 0 && bot->client->ps.weaponstate == WEAPON_READY )
			{
				cmd.weapon = WP_KAR98;
			}
		}
		else if( cmd.weapon == WP_M7 && bot->client->ps.weapon == WP_M7 /*&& bot->client->ps.weapon != WP_CARBINE*/ )
		{
			const int ammo = bot->client->ps.ammoclip[ WeaponTable[ WP_M7 ].clipindex ];

			if( ammo == 0 && bot->client->ps.weaponstate == WEAPON_READY )
			{
				cmd.weapon = WP_CARBINE;
			}
		}
		else if( cmd.weapon == WP_FOOTKICK )
		{
			// convert from weapon request to command
			cmd.buttons |= BUTTON_GESTURE;
		}

#else

		if( cmd.weapon == WP_GPG40 && bot->client->ps.weapon == WP_GPG40 /*&& bot->client->ps.weapon != WP_KAR98*/ )
		{
			const int ammo = bot->client->ps.ammoclip[ BG_FindClipForWeapon( WP_GPG40 ) ];

			if( ammo == 0 && bot->client->ps.weaponstate == WEAPON_READY )
			{
				cmd.weapon = WP_KAR98;
			}
		}
		else if( cmd.weapon == WP_M7 && bot->client->ps.weapon == WP_M7 /*&& bot->client->ps.weapon != WP_CARBINE*/ )
		{
			const int ammo = bot->client->ps.ammoclip[ BG_FindClipForWeapon( WP_M7 ) ];

			if( ammo == 0 && bot->client->ps.weaponstate == WEAPON_READY )
			{
				cmd.weapon = WP_CARBINE;
			}
		}

#endif

		// Process the bot keypresses.
		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_RESPAWN ) )
		{
			cmd.buttons |= BUTTON_ACTIVATE;
		}

		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_ATTACK1 ) )
		{
			cmd.buttons |= BUTTON_ATTACK;
		}

		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_WALK ) )
		{
			cmd.buttons |= BUTTON_WALKING;
		}
		else if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_SPRINT ) )
		{
			cmd.buttons |= BUTTON_SPRINT;
		}

		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_USE ) )
		{
			cmd.buttons |= BUTTON_ACTIVATE;
		}

		// wbuttons
		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_ATTACK2 ) )
		{
			cmd.wbuttons |= WBUTTON_ATTACK2;
		}

		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_DROP ) )
		{
			cmd.wbuttons |= WBUTTON_DROP;
		}

		// if we have prone held
		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_PRONE ) )
		{
			// and we're not prone, go prone
			if( !( bot->client->ps.eFlags & EF_PRONE ) )
			{
				cmd.wbuttons |= WBUTTON_PRONE;
			}
		}
		else
		{
			// if prone is not held, and we're prone, exit prone
			if( bot->client->ps.eFlags & EF_PRONE )
			{
				cmd.wbuttons |= WBUTTON_PRONE;
			}
		}

		// if we have aim held
		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_AIM ) )
		{
			//if(!(bot->s.eFlags & EF_ZOOMING))
			switch( bot->client->ps.weapon )
			{
				case WP_GARAND:
				case WP_GARAND_SCOPE:
					cmd.weapon = WP_GARAND_SCOPE;
					break;

				case WP_FG42:
				case WP_FG42SCOPE:
					cmd.weapon = WP_FG42SCOPE;
					break;

				case WP_K43:
				case WP_K43_SCOPE:
					cmd.weapon = WP_K43_SCOPE;
					break;

				default:
					cmd.wbuttons |= WBUTTON_ZOOM;
					break;
			}
		}
		else
		{
			// if aim not held and we're zooming, zoom out.
			//if(bot->s.eFlags & EF_ZOOMING)
			//{
			//  trap_EA_Command(_client, "-zoom");
			//  //cmd.wbuttons |= WBUTTON_ZOOM;
			//}
		}

		if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_LEANLEFT ) )
		{
			cmd.wbuttons |= WBUTTON_LEANLEFT;
		}
		else if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_LEANRIGHT ) )
		{
			cmd.wbuttons |= WBUTTON_LEANRIGHT;
		}

		if( bot->client->ps.weaponstate == WEAPON_DROPPING_TORELOAD ||
		    bot->client->ps.weaponstate == WEAPON_RELOADING ||
		    bot->client->ps.weaponstate == WEAPON_RAISING_TORELOAD )
		{
			// keep the same weapon while reloading
			cmd.weapon = bot->client->ps.weapon;
		}
		else if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_RELOAD ) )
		{
			// watch for gpg40 and m7 as these need to be switched for reloading

			/*switch(_input.m_CurrentWeapon)
			{
			case ET_WP_GPG40:
			cmd.weapon = _weaponBotToGame(ET_WP_KAR98);
			break;
			case ET_WP_M7:
			cmd.weapon = _weaponBotToGame(ET_WP_CARBINE);
			break;
			case ET_WP_GARAND_SCOPE:
			cmd.weapon = _weaponBotToGame(ET_WP_GARAND);
			break;
			case ET_WP_FG42_SCOPE:
			cmd.weapon = _weaponBotToGame(ET_WP_FG42);
			break;
			case ET_WP_K43_SCOPE:
			cmd.weapon = _weaponBotToGame(ET_WP_K43);
			break;
			default:
			break;
			}*/
			cmd.wbuttons |= WBUTTON_RELOAD;
		}

		// don't process view angles and moving stuff when dead
		if( bot->client->ps.pm_type >= PM_DEAD || bot->client->ps.pm_flags & ( PMF_LIMBO | PMF_TIME_LOCKPLAYER ) )
		{
			// cant move in these states
			cmd.buttons &= ~BUTTON_ATTACK;
			cmd.wbuttons &= ~WBUTTON_ATTACK2;
#ifdef NOQUARTER
			cmd.buttons &= ~BUTTON_GESTURE;
#endif
		}
		else
		{
			float fMaxSpeed = 127.f;
			vec3_t angles, bodyangles, forward, right;

			// Convert the bots vector to angles and set the view angle to the orientation
			vectoangles( _input.m_Facing, angles );
			SetClientViewAngle( bot, angles );

			if( cmd.buttons & BUTTON_WALKING )
			{
				fMaxSpeed = 64.f;
			}

			// Convert the move direction into forward and right moves to
			// take the bots orientation into account.

			// flatten the view angles so we get a proper fwd,right vector as relevent to movement.
			VectorCopy( angles, bodyangles );
			bodyangles[ PITCH ] = 0;

			AngleVectors( bodyangles, forward, right, NULL );
			const float fwd = DotProduct( forward, _input.m_MoveDir );
			const float rght = DotProduct( right, _input.m_MoveDir );

			cmd.forwardmove = ( char )( fwd * fMaxSpeed );
			cmd.rightmove = ( char )( rght * fMaxSpeed );

			if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_FWD ) || _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_MOVEUP ) )
			{
				cmd.forwardmove = ( char ) fMaxSpeed;
			}

			if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_BACK ) || _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_MOVEDN ) )
			{
				cmd.forwardmove = ( char ) - fMaxSpeed;
			}

			if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_RSTRAFE ) )
			{
				cmd.rightmove = ( char ) fMaxSpeed;
			}

			if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_LSTRAFE ) )
			{
				cmd.rightmove = ( char ) - fMaxSpeed;
			}

			if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_JUMP ) )
			{
				cmd.upmove = ( char ) fMaxSpeed;
			}

			if( _input.m_ButtonFlags.CheckFlag( BOT_BUTTON_CROUCH ) )
			{
				cmd.upmove = ( char ) - fMaxSpeed;
			}
		}

		trap_BotUserCommand( _client, &cmd );
	}

	void BotCommand( int _client, const char *_cmd )
	{
		trap_EA_Command( _client, ( char * ) _cmd );
	}

	obBool IsInPVS( const float _pos[ 3 ], const float _target[ 3 ] )
	{
		return trap_InPVS( _pos, _target ) ? True : False;
	}

	obResult TraceLine( obTraceResult &_result, const float _start[ 3 ], const float _end[ 3 ],
	                    const AABB *_pBBox, int _mask, int _user, obBool _bUsePVS )
	{
		qboolean bInPVS = _bUsePVS ? trap_InPVS( _start, _end ) : qtrue;

		if( bInPVS )
		{
			int iMask = 0;

			// Set up the collision masks
			if( _mask & TR_MASK_ALL )
			{
				iMask = MASK_ALL;
			}
			else
			{
				if( _mask & TR_MASK_SOLID )
				{
					iMask |= MASK_SOLID;
				}

				if( _mask & TR_MASK_PLAYER )
				{
					iMask |= MASK_PLAYERSOLID;
				}

				if( _mask & TR_MASK_SHOT )
				{
					iMask |= MASK_SHOT;
				}

				if( _mask & TR_MASK_OPAQUE )
				{
					iMask |= MASK_OPAQUE;
				}

				if( _mask & TR_MASK_WATER )
				{
					iMask |= MASK_WATER;
				}

				if( _mask & TR_MASK_PLAYERCLIP )
				{
					iMask |= CONTENTS_PLAYERCLIP;
				}

				if( _mask & ( TR_MASK_FLOODFILL | TR_MASK_FLOODFILLENT ) )
				{
					iMask |= CONTENTS_PLAYERCLIP | CONTENTS_SOLID;
				}

				if( _mask & TR_MASK_SMOKEBOMB )
				{
					gentity_t *pSmokeBlocker = Bot_EntInvisibleBySmokeBomb( ( float * ) _start, ( float * ) _end );

					if( pSmokeBlocker )
					{
						_result.m_Fraction = 0.0f;
						_result.m_HitEntity = HandleFromEntity( pSmokeBlocker );
						return Success;
					}
				}
			}

			trace_t tr;

			if( _mask & TR_MASK_FLOODFILL )
			{
				trap_TraceNoEnts( &tr, _start,
				                  _pBBox ? _pBBox->m_Mins : NULL,
				                  _pBBox ? _pBBox->m_Maxs : NULL,
				                  _end, _user, iMask );
			}
			else
			{
				trap_Trace( &tr, _start,
				            _pBBox ? _pBBox->m_Mins : NULL,
				            _pBBox ? _pBBox->m_Maxs : NULL,
				            _end, _user, iMask );
			}

			if( ( tr.entityNum != ENTITYNUM_WORLD ) && ( tr.entityNum != ENTITYNUM_NONE ) )
			{
				_result.m_HitEntity = HandleFromEntity( &g_entities[ tr.entityNum ] );
			}
			else
			{
				_result.m_HitEntity.Reset();
			}

			//_result.m_iUser1 = tr.surfaceFlags;

			// Fill in the bot traceflag.
			_result.m_Fraction = tr.fraction;
			_result.m_StartSolid = tr.startsolid;
			_result.m_Endpos[ 0 ] = tr.endpos[ 0 ];
			_result.m_Endpos[ 1 ] = tr.endpos[ 1 ];
			_result.m_Endpos[ 2 ] = tr.endpos[ 2 ];
			_result.m_Normal[ 0 ] = tr.plane.normal[ 0 ];
			_result.m_Normal[ 1 ] = tr.plane.normal[ 1 ];
			_result.m_Normal[ 2 ] = tr.plane.normal[ 2 ];
			_result.m_Contents = obUtilBotContentsFromGameContents( tr.contents );
			_result.m_Surface = obUtilBotSurfaceFromGameSurface( tr.surfaceFlags );
		}
		else
		{
			// Not in PVS
			_result.m_Fraction = 0.0f;
			_result.m_HitEntity.Reset();
		}

		return bInPVS ? Success : OutOfPVS;
	}

	int GetPointContents( const float _pos[ 3 ] )
	{
		vec3_t vpos = { _pos[ 0 ], _pos[ 1 ], _pos[ 2 ] };
		int iContents = trap_PointContents( vpos, -1 );
		return obUtilBotContentsFromGameContents( iContents );
	}

	GameEntity GetLocalGameEntity()
	{
		return EntityFromID( 0 );
	}

	GameEntity FindEntityInSphere( const float _pos[ 3 ], float _radius, GameEntity _pStart, int classId )
	{
		// square it to avoid the square root in the distance check.
		gentity_t *pStartEnt = _pStart.IsValid() ? EntityFromHandle( _pStart ) : 0;

		const char *pClassName = 0;
		int iPlayerClass = 0;
		int iSpawnFlags = 0;

#ifdef NOQUARTER
		int iHash = 0;

		switch( classId )
		{
			case ET_CLASS_SOLDIER:
			case ET_CLASS_MEDIC:
			case ET_CLASS_ENGINEER:
			case ET_CLASS_FIELDOPS:
			case ET_CLASS_COVERTOPS:
			case ET_CLASS_ANY:
				iPlayerClass = classId != ET_CLASS_ANY ? classId : 0;
				pClassName = "player";
				iHash = PLAYER_HASH;
				break;

				//////////////////////////////////////////////////////////////////////////
			case ET_CLASSEX_MG42MOUNT:
				pClassName = "misc_mg42";
				iHash = MISC_MG42_HASH;
				break;

			case ET_CLASSEX_DYNAMITE:
				pClassName = "dynamite";
				iHash = DYNAMITE_HASH;
				break;

			case ET_CLASSEX_MINE:
				pClassName = "landmine";
				iHash = LANDMINE_HASH;
				break;

			case ET_CLASSEX_SATCHEL:
				pClassName = "satchel_charge";
				iHash = SATCHEL_CHARGE_HASH;
				break;

			case ET_CLASSEX_SMOKEBOMB:
				pClassName = "smoke_bomb";
				iHash = SMOKE_BOMB_HASH;
				break;

			case ET_CLASSEX_SMOKEMARKER:
				pClassName = "air strike";
				iHash = AIR_STRIKE_HASH;
				break;

			case ET_CLASSEX_VEHICLE:
			case ET_CLASSEX_VEHICLE_HVY:
				iSpawnFlags = classId == ET_CLASSEX_VEHICLE_HVY ? 4 : 0;
				pClassName = "script_mover";
				iHash = SCRIPT_MOVER_HASH;
				break;

			case ET_CLASSEX_BREAKABLE:
				break;

			case ET_CLASSEX_CORPSE:
				pClassName = "corpse";
				iHash = CORPSE_HASH;
				break;

			case ET_CLASSEX_GRENADE:
				pClassName = "grenade";
				iHash = GRENADE_HASH;
				break;

			case ET_CLASSEX_ROCKET:
				pClassName = "rocket";
				iHash = ROCKET_HASH;
				break;

			case ET_CLASSEX_MORTAR:
				pClassName = "mortar_grenade";
				iHash = MORTAR_GRENADE_HASH;
				break;

			case ET_CLASSEX_ARTY:
				pClassName = "air strike";
				iHash = AIR_STRIKE_HASH;
				break;

			case ET_CLASSEX_AIRSTRIKE:
				pClassName = "air strike";
				iHash = AIR_STRIKE_HASH;
				break;

			case ET_CLASSEX_FLAMECHUNK:
				pClassName = "flamechunk";
				iHash = FLAMECHUNK_HASH;
				break;

			case ET_CLASSEX_M7_GRENADE:
				pClassName = "m7_grenade";
				iHash = M7_GRENADE_HASH;
				break;

			case ET_CLASSEX_GPG40_GRENADE:
				pClassName = "gpg40_grenade";
				iHash = GPG40_GRENADE_HASH;
				break;

			case ET_CLASSEX_HEALTHCABINET:
				pClassName = "misc_cabinet_health";
				iHash = MISC_CABINET_HEALTH_HASH;
				break;

			case ET_CLASSEX_AMMOCABINET:
				pClassName = "misc_cabinet_supply";
				iHash = MISC_CABINET_SUPPLY_HASH;
				break;
		}

		if( iHash )
		{
			float fSqRad = _radius * _radius;
			vec3_t toent;

			while( ( pStartEnt = G_FindByClassnameFast( pStartEnt, pClassName, iHash ) ) != NULL )
			{
				if( iPlayerClass && pStartEnt->client &&
				    pStartEnt->client->sess.sessionTeam != iPlayerClass )
				{
					continue;
				}

				if( iSpawnFlags && !( pStartEnt->spawnflags & iSpawnFlags ) )
				{
					continue;
				}

				// don't detect unusable corpses. these ents hang around until the body queue slot is re-used
				if( classId == ET_CLASSEX_CORPSE &&
				    ( !pStartEnt->physicsObject ||
				      ( pStartEnt->activator && pStartEnt->activator->client->ps.powerups[ PW_OPS_DISGUISED ] ) ) )
				{
					continue;
				}

				VectorSubtract( _pos, pStartEnt->r.currentOrigin, toent );

				if( VectorLengthSquared( toent ) < fSqRad )
				{
					break;
				}
			}

			return HandleFromEntity( pStartEnt );
		}

#else // not NoQuarter

		switch( classId )
		{
			case ET_CLASS_SOLDIER:
			case ET_CLASS_MEDIC:
			case ET_CLASS_ENGINEER:
			case ET_CLASS_FIELDOPS:
			case ET_CLASS_COVERTOPS:
			case ET_CLASS_ANY:
				iPlayerClass = classId != ET_CLASS_ANY ? classId : 0;
				pClassName = "player";
				break;

				//////////////////////////////////////////////////////////////////////////
			case ET_CLASSEX_MG42MOUNT:
				pClassName = "misc_mg42";
				break;

			case ET_CLASSEX_DYNAMITE:
				pClassName = "dynamite";
				break;

			case ET_CLASSEX_MINE:
				pClassName = "landmine";
				break;

			case ET_CLASSEX_SATCHEL:
				pClassName = "satchel_charge";
				break;

			case ET_CLASSEX_SMOKEBOMB:
				pClassName = "smoke_bomb";
				break;

			case ET_CLASSEX_SMOKEMARKER:
				pClassName = "air strike";
				break;

			case ET_CLASSEX_VEHICLE:
			case ET_CLASSEX_VEHICLE_HVY:
				iSpawnFlags = classId == ET_CLASSEX_VEHICLE_HVY ? 4 : 0;
				pClassName = "script_mover";
				break;

			case ET_CLASSEX_BREAKABLE:
				break;

			case ET_CLASSEX_CORPSE:
				pClassName = "corpse";
				break;

			case ET_CLASSEX_GRENADE:
				pClassName = "grenade";
				break;

			case ET_CLASSEX_ROCKET:
				pClassName = "rocket";
				break;

			case ET_CLASSEX_MORTAR:
				pClassName = "mortar_grenade";
				break;

			case ET_CLASSEX_ARTY:
				pClassName = "air strike";
				break;

			case ET_CLASSEX_AIRSTRIKE:
				pClassName = "air strike";
				break;

			case ET_CLASSEX_FLAMECHUNK:
				pClassName = "flamechunk";
				break;

			case ET_CLASSEX_M7_GRENADE:
				pClassName = "m7_grenade";
				break;

			case ET_CLASSEX_GPG40_GRENADE:
				pClassName = "gpg40_grenade";
				break;

			case ET_CLASSEX_HEALTHCABINET:
				pClassName = "misc_cabinet_health";
				break;

			case ET_CLASSEX_AMMOCABINET:
				pClassName = "misc_cabinet_supply";
				break;
		}

		if( pClassName )
		{
			float fSqRad = _radius * _radius;
			vec3_t toent;

			while( ( pStartEnt = G_Find( pStartEnt, FOFS( classname ), pClassName ) ) != NULL )
			{
				if( iPlayerClass && pStartEnt->client &&
				    pStartEnt->client->sess.sessionTeam != iPlayerClass )
				{
					continue;
				}

				if( iSpawnFlags && !( pStartEnt->spawnflags & iSpawnFlags ) )
				{
					continue;
				}

				// don't detect unusable corpses. these ents hang around until the body queue slot is re-used
				if( classId == ET_CLASSEX_CORPSE &&
				    ( !pStartEnt->physicsObject ||
				      ( pStartEnt->activator && pStartEnt->activator->client->ps.powerups[ PW_OPS_DISGUISED ] ) ) )
				{
					continue;
				}

				VectorSubtract( _pos, pStartEnt->r.currentOrigin, toent );

				if( VectorLengthSquared( toent ) < fSqRad )
				{
					break;
				}
			}

			return HandleFromEntity( pStartEnt );
		}

#endif
		return GameEntity();
	}

	int GetEntityClass( const GameEntity _ent )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );
		return pEnt && pEnt->inuse ? _GetEntityClass( pEnt ) : ET_TEAM_NONE;
	}

	obResult GetEntityCategory( const GameEntity _ent, BitFlag32 &_category )
	{
		obResult res = Success;
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( !pEnt )
		{
			return InvalidEntity;
		}

		// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
		int t = pEnt->s.eType;

		if( pEnt->client && ( pEnt - g_entities ) < MAX_CLIENTS )
		{
			t = ET_PLAYER;
		}

		switch( t )
		{
			case ET_GENERAL:
				{
					if( !Q_stricmp( pEnt->classname, "func_invisible_user" ) )
					{
						// The damage flags tells us the type.
						switch( pEnt->s.dmgFlags )
						{
							case HINT_BUTTON:
								_category.SetFlag( ENT_CAT_TRIGGER );
								_category.SetFlag( ENT_CAT_STATIC );
								break;
						}
					}
					else if( !Q_stricmp( pEnt->classname, "func_button" ) )
					{
						_category.SetFlag( ENT_CAT_TRIGGER );
						_category.SetFlag( ENT_CAT_STATIC );
						// continue for now so it doesnt get regged
						//continue;
					}
					else if( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
					{
						if( ( pEnt->health > 0 ) &&
						    ( pEnt->entstate != STATE_INVISIBLE ) &&
						    ( pEnt->entstate != STATE_UNDERCONSTRUCTION ) )
						{
							_category.SetFlag( ENT_CAT_MOUNTEDWEAPON );
							_category.SetFlag( ENT_CAT_SHOOTABLE );
						}
					}
					// cs: waypoint tool, don't merge the spawns
					else if( !Q_stricmp( pEnt->classname, "info_player_deathmatch" ) ||
					         !Q_stricmp( pEnt->classname, "team_CTF_redspawn" ) ||
					         !Q_stricmp( pEnt->classname, "team_CTF_bluespawn" ) ||
					         !Q_stricmp( pEnt->classname, "info_player_spawn" ) )
					{
						// don't fill up the bots sensory mem at start with these
						_category.SetFlag( ENT_CAT_INTERNAL );
					}
					else
					{
						res = InvalidEntity;
					}

					break;
				}

			case ET_PLAYER:
				{
					if( !pEnt->client || ( pEnt->entstate == STATE_INVISIBLE ) ||
					    ( pEnt->client->ps.pm_type == PM_SPECTATOR ) ||
					    ( pEnt->client->sess.sessionTeam != TEAM_AXIS &&
					      pEnt->client->sess.sessionTeam != TEAM_ALLIES ) )
					{
						res = InvalidEntity;
						break;
					}

					// Special case for dead players that haven't respawned.
					if( pEnt->health <= GIB_HEALTH )
					{
						_category.SetFlag( ENT_CAT_MISC );
						break;
					}

					if( pEnt->health > GIB_HEALTH )
					{
						if( !pEnt->client->ps.powerups[ PW_INVULNERABLE ] )
						{
							_category.SetFlag( ENT_CAT_SHOOTABLE );
						}

						_category.SetFlag( ENT_CAT_PLAYER );
						break;
					}
				}

			case ET_ITEM:
				{
					if( !( pEnt->r.contents & CONTENTS_ITEM ) )
					{
						res = InvalidEntity;
					}
					else if( !Q_strncmp( pEnt->classname, "item_health", strlen( "item_health" ) ) )
					{
						_category.SetFlag( ENT_CAT_PICKUP );
						_category.SetFlag( ENT_CAT_PICKUP_HEALTH );
					}
					else if( !Q_strncmp( pEnt->classname, "weapon_magicammo", strlen( "weapon_magicammo" ) ) )
					{
						_category.SetFlag( ENT_CAT_PICKUP );
						_category.SetFlag( ENT_CAT_PICKUP_AMMO );
					}
					else if( !Q_stricmp( pEnt->classname, "item_treasure" ) )
					{
						_category.SetFlag( ENT_CAT_PICKUP );
					}
					else if( pEnt->item && pEnt->item->giType == IT_WEAPON )
					{
						_category.SetFlag( ENT_CAT_PICKUP );
						_category.SetFlag( ENT_CAT_PICKUP_WEAPON );
					}
					else
					{
						res = InvalidEntity;
					}

					break;
				}

			case ET_CORPSE:
				{
					_category.SetFlag( ENT_CAT_MISC );
					break;
				}

			case ET_MISSILE:
				{
					// Register certain weapons as threats to avoid or whatever.
					switch( pEnt->s.weapon )
					{
						case WP_GRENADE_LAUNCHER:
						case WP_GRENADE_PINEAPPLE:
						case WP_PANZERFAUST:
						case WP_ARTY:
						case WP_DYNAMITE:
						case WP_SMOKE_MARKER:
						case WP_SATCHEL:
						case WP_M7:
						case WP_GPG40:
						case WP_MORTAR_SET:
#ifdef NOQUARTER
						case WP_MORTAR2_SET:
						case WP_BAZOOKA:
#endif
							_category.SetFlag( ENT_CAT_AVOID );
							_category.SetFlag( ENT_CAT_PROJECTILE );
							break;

						case WP_LANDMINE:
#ifdef JAYMOD_name
						case 79: //WP_LANDMINE_BBETTY
						case 80: //WP_LANDMINE_PGAS
#endif
							_category.SetFlag( ENT_CAT_AVOID );
							_category.SetFlag( ET_ENT_CAT_MINE );
							_category.SetFlag( ENT_CAT_OBSTACLE );
							break;

						case WP_SMOKE_BOMB:
							_category.SetFlag( ENT_CAT_PROJECTILE );
							break;

						default:
							if( !Q_strncmp( pEnt->classname, "air strike", sizeof( "air strike" ) ) )
							{
								_category.SetFlag( ENT_CAT_AVOID );
								_category.SetFlag( ENT_CAT_PROJECTILE );
								break;
							}
							else
							{
								res = InvalidEntity;
							}
					}

					break;
				}

			case ET_FLAMETHROWER_CHUNK:
				{
					_category.SetFlag( ENT_CAT_AVOID );
					_category.SetFlag( ENT_CAT_PROJECTILE );
					break;
				}

			case ET_MOVER:
				{
					if( !Q_stricmp( pEnt->classname, "script_mover" ) )
					{
						_category.SetFlag( ENT_CAT_OBSTACLE );

						if( pEnt->model2 )
						{
							_category.SetFlag( ENT_CAT_VEHICLE );
						}
						else
						{
							_category.SetFlag( ENT_CAT_MOVER );
						}

						_category.SetFlag( ENT_CAT_STATIC );

						if( pEnt->health > 0 )
						{
							_category.SetFlag( ENT_CAT_SHOOTABLE );
						}
					}

					/*else if (!Q_stricmp(pCurrent->classname, "props_flamebarrel"))
					{
					info.m_EntityClass = ET_CLASSEX_BREAKABLE;
					info.m_EntityCategory = ENT_CAT_SHOOTABLE;
					}
					else if (!Q_stricmp(pCurrent->classname, "props_statue"))
					{
					info.m_EntityClass = ET_CLASSEX_BREAKABLE;
					info.m_EntityCategory = ENT_CAT_SHOOTABLE;
					}*/
					else if( !Q_stricmp( pEnt->classname, "props_chair_hiback" ) )
					{
						if( ( pEnt->health > 0 ) && ( pEnt->takedamage == qtrue ) )
						{
							_category.SetFlag( ENT_CAT_SHOOTABLE );
						}
					}
					else if( !Q_stricmp( pEnt->classname, "props_chair" ) )
					{
						if( ( pEnt->health > 0 ) && ( pEnt->takedamage == qtrue ) )
						{
							_category.SetFlag( ENT_CAT_SHOOTABLE );
						}
					}
					else if( !Q_stricmp( pEnt->classname, "props_chair_side" ) )
					{
						if( ( pEnt->health > 0 ) && ( pEnt->takedamage == qtrue ) )
						{
							_category.SetFlag( ENT_CAT_SHOOTABLE );
						}
					}
					else
					{
						res = InvalidEntity;
					}

					break;
				}

			case ET_MG42_BARREL:
				{
					if( ( pEnt->health > 0 ) &&
					    ( pEnt->entstate != STATE_INVISIBLE ) &&
					    ( pEnt->entstate != STATE_UNDERCONSTRUCTION ) )
					{
						_category.SetFlag( ENT_CAT_MOUNTEDWEAPON );
						_category.SetFlag( ENT_CAT_SHOOTABLE );
					}
					else
					{
						res = InvalidEntity;
					}

					break;
				}

				/*case ET_AAGUN:
				{
				if((pCurrent->health > 0) &&
				(pCurrent->entstate != STATE_INVISIBLE) &&
				(pCurrent->entstate != STATE_UNDERCONSTRUCTION))
				{
				}
				break;
				}*/
			case ET_EXPLOSIVE:
				{
					if( !( pEnt->spawnflags & EXPLOSIVE_TANK ) &&
					    ( pEnt->constructibleStats.weaponclass != 1 ) &&
					    ( pEnt->constructibleStats.weaponclass != 2 ) ) // &&
						//(pEnt->health > 0) && (pEnt->takedamage == qtrue))
					{
						_category.SetFlag( ENT_CAT_OBSTACLE );
						_category.SetFlag( ENT_CAT_SHOOTABLE );
					}
					else
					{
						res = InvalidEntity;
					}

					break;
				}

				//case ET_CONSTRUCTIBLE:
				//{
				//if (G_ConstructionIsPartlyBuilt(pCurrent) &&
				//  !(pCurrent->spawnflags & CONSTRUCTIBLE_INVULNERABLE) &&
				//   (pCurrent->constructibleStats.weaponclass == 0))
				//{
				//  info.m_EntityClass = ET_CLASSEX_BREAKABLE;
				//  info.m_EntityCategory = ENT_CAT_SHOOTABLE;
				//}
				//else
				//{
				//}
				//break;
				//}
			case ET_HEALER:
				{
					_category.SetFlag( ENT_CAT_OBSTACLE );
					_category.SetFlag( ENT_CAT_PICKUP );
					break;
				}

			case ET_SUPPLIER:
				{
					_category.SetFlag( ENT_CAT_OBSTACLE );
					_category.SetFlag( ENT_CAT_PICKUP );
					break;
				}

			default:
				res = InvalidEntity;
				break; // ignore this type.
		};

		return res;
	}

	obResult GetEntityFlags( const GameEntity _ent, BitFlag64 &_flags )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt && pEnt->inuse )
		{
			// Set any flags.
			if( pEnt->health <= 0 )
			{
				_flags.SetFlag( ENT_FLAG_DEAD );
			}

			if( pEnt->client && !IsBot( pEnt ) )
			{
				_flags.SetFlag( ENT_FLAG_HUMANCONTROLLED );
			}

			if( pEnt->waterlevel >= 3 )
			{
				_flags.SetFlag( ENT_FLAG_UNDERWATER );
			}
			else if( pEnt->waterlevel > 0 )
			{
				_flags.SetFlag( ENT_FLAG_INWATER );
			}

			if( pEnt->s.eFlags & EF_ZOOMING )
			{
				_flags.SetFlag( ENT_FLAG_ZOOMING );
				_flags.SetFlag( ENT_FLAG_AIMING );
			}

			if( pEnt->s.eFlags & EF_MG42_ACTIVE )
			{
				_flags.SetFlag( ET_ENT_FLAG_MNT_MG42 );
				_flags.SetFlag( ET_ENT_FLAG_MOUNTED );
			}

			if( pEnt->s.eFlags & EF_MOUNTEDTANK )
			{
				_flags.SetFlag( ET_ENT_FLAG_MNT_TANK );
				_flags.SetFlag( ET_ENT_FLAG_MOUNTED );
			}

#ifndef NOQUARTER

			if( pEnt->s.eFlags & EF_AAGUN_ACTIVE )
			{
				_flags.SetFlag( ET_ENT_FLAG_MNT_AAGUN );
				_flags.SetFlag( ET_ENT_FLAG_MOUNTED );
			}

#endif

			if( pEnt->s.eType == ET_HEALER || pEnt->s.eType == ET_SUPPLIER )
			{
				if( pEnt->entstate == STATE_INVISIBLE )
				{
					_flags.SetFlag( ENT_FLAG_DISABLED );
				}
			}

			if( pEnt->s.eType == ET_MOVER )
			{
				_flags.SetFlag( ENT_FLAG_VISTEST );

				if( _TankIsMountable( pEnt ) )
				{
					_flags.SetFlag( ET_ENT_FLAG_ISMOUNTABLE );
				}

				if( G_TankIsOccupied( pEnt ) )
				{
					_flags.SetFlag( ET_ENT_FLAG_MOUNTED );
				}
			}

			if( pEnt->s.eType == ET_CONSTRUCTIBLE )
			{
				if( !G_ConstructionIsFullyBuilt( pEnt ) )
				{
					_flags.SetFlag( ENT_FLAG_DEAD );
				}
				else if( G_ConstructionIsFullyBuilt( pEnt ) )
				{
					_flags.SetFlag( ENT_FLAG_DEAD, false );
				}
			}

			if( ( pEnt->s.eType == ET_MG42_BARREL ) ||
			    ( pEnt->s.eType == ET_GENERAL && !Q_stricmp( pEnt->classname, "misc_mg42" ) ) )
			{
				_flags.SetFlag( ENT_FLAG_DEAD, Simple_EmplacedGunIsRepairable( pEnt ) != 0 );

				_flags.SetFlag( ENT_FLAG_VISTEST );

				if( _EmplacedGunIsMountable( pEnt ) )
				{
					_flags.SetFlag( ET_ENT_FLAG_ISMOUNTABLE );
				}

				if( pEnt->r.ownerNum != pEnt->s.number )
				{
					gentity_t *owner = &g_entities[ pEnt->r.ownerNum ];

					if( owner && owner->active && owner->client && owner->s.eFlags & EF_MG42_ACTIVE )
					{
						_flags.SetFlag( ET_ENT_FLAG_MOUNTED );
					}
				}
			}

			if( pEnt->client )
			{
#ifdef NOQUARTER

				if( pEnt->client->ps.eFlags & EF_POISONED )
				{
					_flags.SetFlag( ET_ENT_FLAG_POISONED );
				}

#elif defined ETPUB_VERSION

				if( pEnt->client->pmext.poisoned )
				{
					_flags.SetFlag( ET_ENT_FLAG_POISONED );
				}

#elif defined JAYMOD_name

				if( G_IsPoisoned( pEnt ) )
				{
					_flags.SetFlag( ET_ENT_FLAG_POISONED );
				}

#endif

				if( pEnt->client->ps.pm_flags & PMF_LADDER )
				{
					_flags.SetFlag( ENT_FLAG_ONLADDER );
				}

				if( pEnt->client->ps.eFlags & EF_PRONE )
				{
					_flags.SetFlag( ENT_FLAG_PRONED );
				}

				if( pEnt->client->ps.pm_flags & PMF_DUCKED )
				{
					_flags.SetFlag( ENT_FLAG_CROUCHED );
				}

				if( pEnt->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					_flags.SetFlag( ENT_FLAG_ONGROUND );
				}

				if( pEnt->client->ps.weaponstate == WEAPON_RELOADING )
				{
					_flags.SetFlag( ENT_FLAG_RELOADING );
				}

				if( pEnt->client->ps.powerups[ PW_OPS_DISGUISED ] )
				{
					_flags.SetFlag( ET_ENT_FLAG_DISGUISED );
				}

				if( pEnt->client->ps.powerups[ PW_REDFLAG ] || pEnt->client->ps.powerups[ PW_BLUEFLAG ] )
				{
					_flags.SetFlag( ET_ENT_FLAG_CARRYINGGOAL );
				}

				if( pEnt->client->ps.pm_flags & PMF_LIMBO )
				{
					_flags.SetFlag( ET_ENT_FLAG_INLIMBO );
				}

				switch( pEnt->client->ps.weapon )
				{
					case WP_GARAND_SCOPE:
					case WP_FG42SCOPE:
					case WP_K43_SCOPE:
						_flags.SetFlag( ENT_FLAG_ZOOMING );
						break;
				}

				if( pEnt->s.eFlags & EF_ZOOMING )
				{
					_flags.SetFlag( ENT_FLAG_ZOOMING );
				}
			}

			// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
			int t = pEnt->s.eType;

			if( pEnt->client && ( pEnt - g_entities ) < MAX_CLIENTS )
			{
				t = ET_PLAYER;
			}

			switch( t )
			{
				case ET_PLAYER:
					{
						_flags.SetFlag( ENT_FLAG_VISTEST );

						if( pEnt->health <= 0 )
						{
							if( !pEnt->r.linked || BODY_TEAM( pEnt ) >= 4 || BODY_VALUE( pEnt ) >= 250 || pEnt->health < GIB_HEALTH )
							{
								_flags.SetFlag( ENT_FLAG_DISABLED );
							}
							else if( g_OmniBotFlags.integer & OBF_GIBBING )
							{
								// for gibbing
								_flags.SetFlag( ENT_FLAG_DEAD, false );
								_flags.SetFlag( ENT_FLAG_PRONED );
							}
						}

						break;
					}

				case ET_CORPSE:
					{
						_flags.SetFlag( ENT_FLAG_VISTEST );

						if( !pEnt->r.linked || BODY_TEAM( pEnt ) >= 4 || BODY_VALUE( pEnt ) >= 250 || pEnt->health < GIB_HEALTH )
						{
							_flags.SetFlag( ENT_FLAG_DISABLED );
						}

						break;
					}

				case ET_ITEM:
					{
						_flags.SetFlag( ENT_FLAG_VISTEST );

						if( !( pEnt->r.contents & CONTENTS_ITEM ) )
						{
							_flags.SetFlag( ENT_FLAG_DISABLED );
						}

						break;
					}

				case ET_HEALER:
					{
						break;
					}

				case ET_SUPPLIER:
					{
						break;
					}

				case ET_EXPLOSIVE:
					{
						_flags.SetFlag( ENT_FLAG_VISTEST );
						break;
					}

				case ET_MISSILE:
					{
						// Register certain weapons as threats to avoid or whatever.
						switch( pEnt->s.weapon )
						{
							case WP_GRENADE_PINEAPPLE:
							case WP_GRENADE_LAUNCHER:
							case WP_PANZERFAUST:
							case WP_ARTY:
							case WP_DYNAMITE:
							case WP_SMOKE_MARKER:
							case WP_LANDMINE:
							case WP_SATCHEL:
							case WP_M7:
							case WP_GPG40:
							case WP_MORTAR_SET:
#ifdef NOQUARTER
							case WP_MORTAR2_SET:
							case WP_BAZOOKA:
#endif
							case WP_SMOKE_BOMB:

							default:
								_flags.SetFlag( ENT_FLAG_VISTEST );
						}

						break;
					}

				case ET_GENERAL:
				case ET_MG42_BARREL:
					{
						if( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
						{
							if( ( pEnt->health < 0 ) ||
							    ( pEnt->entstate == STATE_INVISIBLE ) )
							{
								//_flags.SetFlag(ENT_FLAG_VISTEST);
								_flags.SetFlag( ENT_FLAG_DEAD );
							}
						}

						break;
					}
			}
		}

		return Success;
	}

	obResult GetEntityPowerups( const GameEntity _ent, BitFlag64 &_flags )
	{
		return Success;
	}

	obResult GetEntityEyePosition( const GameEntity _ent, float _pos[ 3 ] )
	{
		if( GetEntityPosition( _ent, _pos ) == Success )
		{
			gentity_t *pEnt = EntityFromHandle( _ent );

			if( pEnt && pEnt->client )
			{
				_pos[ 2 ] += pEnt->client->ps.viewheight;
			}

			return Success;
		}

		return InvalidEntity;
	}

	obResult GetEntityBonePosition( const GameEntity _ent, int _boneid, float _pos[ 3 ] )
	{
		// ET doesnt really support bones
		return GetEntityPosition( _ent, _pos );
	}

	obResult GetEntityOrientation( const GameEntity _ent, float _fwd[ 3 ], float _right[ 3 ], float _up[ 3 ] )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt && pEnt->inuse )
		{
			if( pEnt->client )
			{
				AngleVectors( pEnt->client->ps.viewangles, _fwd, _right, _up );
			}
			else
			{
				AngleVectors( pEnt->r.currentAngles, _fwd, _right, _up );
			}

			return Success;
		}

		return InvalidEntity;
	}

	obResult GetEntityVelocity( const GameEntity _ent, float _velocity[ 3 ] )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt && pEnt->inuse )
		{
			// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
			int t = pEnt->s.eType;

			if( pEnt->client && ( pEnt - g_entities ) < MAX_CLIENTS )
			{
				t = ET_PLAYER;
			}

			if( t == ET_MOVER )
			{
				// Calculate the velocity ourselves. for some reason mover velocity
				// isn't in s.pos.trDelta
				const float fDeltaTime = 50.0f; // get this dynamically?
				_velocity[ 0 ] = ( pEnt->r.currentOrigin[ 0 ] - pEnt->oldOrigin[ 0 ] ) * fDeltaTime;
				_velocity[ 1 ] = ( pEnt->r.currentOrigin[ 1 ] - pEnt->oldOrigin[ 1 ] ) * fDeltaTime;
				_velocity[ 2 ] = ( pEnt->r.currentOrigin[ 2 ] - pEnt->oldOrigin[ 2 ] ) * fDeltaTime;
			}
			else
			{
				_velocity[ 0 ] = pEnt->s.pos.trDelta[ 0 ];
				_velocity[ 1 ] = pEnt->s.pos.trDelta[ 1 ];
				_velocity[ 2 ] = pEnt->s.pos.trDelta[ 2 ];
			}

			return Success;
		}

		return InvalidEntity;
	}

	obResult GetEntityPosition( const GameEntity _ent, float _pos[ 3 ] )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt && pEnt->inuse )
		{
			if( !pEnt->client )
			{
				vec3_t axis[ 3 ];
				//AngleVectors( pEnt->r.currentAngles, axis[0], axis[1], axis[2] );
				AnglesToAxis( pEnt->r.currentAngles, axis );

				vec3_t boxCenter;
				boxCenter[ 0 ] = ( ( pEnt->r.maxs[ 0 ] + pEnt->r.mins[ 0 ] ) * 0.5f );
				boxCenter[ 1 ] = ( ( pEnt->r.maxs[ 1 ] + pEnt->r.mins[ 1 ] ) * 0.5f );
				boxCenter[ 2 ] = ( ( pEnt->r.maxs[ 2 ] + pEnt->r.mins[ 2 ] ) * 0.5f );

				vec3_t out;
				VectorCopy( pEnt->r.currentOrigin, out );

				for( int i = 0; i < 3; ++i )
				{
					vec3_t tmp;
					VectorScale( axis[ i ], boxCenter[ i ], tmp );
					VectorAdd( out, tmp, out );
				}

				VectorCopy( out, _pos );
				return Success;
			}

			// Clients return normal position.
			_pos[ 0 ] = pEnt->r.currentOrigin[ 0 ];
			_pos[ 1 ] = pEnt->r.currentOrigin[ 1 ];
			_pos[ 2 ] = pEnt->r.currentOrigin[ 2 ];

			return Success;
		}

		return InvalidEntity;
	}

	obResult GetEntityLocalAABB( const GameEntity _ent, AABB &_aabb )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt && pEnt->inuse )
		{
			/*if(!pEnt->r.linked && pEnt->parent && pEnt->parent->s.eType == ET_OID_TRIGGER)
			pEnt = pEnt->parent;*/

			if( pEnt->s.eType == ET_CONSTRUCTIBLE )
			{
				gentity_t *pAxis = G_ConstructionForTeam( pEnt->parent ? pEnt->parent : pEnt, TEAM_AXIS );
				gentity_t *pAlly = G_ConstructionForTeam( pEnt->parent ? pEnt->parent : pEnt, TEAM_ALLIES );

				if( pAxis /*&& pAxis->entstate == STATE_DEFAULT*/ )
				{
					pEnt = pAxis;
				}
				else if( pAlly /*&& pAlly->entstate == STATE_DEFAULT*/ )
				{
					pEnt = pAlly;
				}
			}

			//if(!Q_stricmp(pEnt->classname, "func_explosive") ||
			//  !Q_stricmpn(pEnt->classname, "trigger_",8))
			//{
			//  // find the midpt
			//  vec3_t pos;
			//  pos[0] = pEnt->r.currentOrigin[0] + ((pEnt->r.maxs[0] + pEnt->r.mins[0]) * 0.5f);
			//  pos[1] = pEnt->r.currentOrigin[1] + ((pEnt->r.maxs[1] + pEnt->r.mins[1]) * 0.5f);
			//  pos[2] = pEnt->r.currentOrigin[2] + ((pEnt->r.maxs[2] + pEnt->r.mins[2]) * 0.5f);

			//  // figure out the local bounds from there
			//  _aabb.m_Mins[0] = pEnt->r.mins[0] - pos[0];
			//  _aabb.m_Mins[1] = pEnt->r.mins[1] - pos[1];
			//  _aabb.m_Mins[2] = pEnt->r.mins[2] - pos[2];
			//  _aabb.m_Maxs[0] = pEnt->r.maxs[0] - pos[0];
			//  _aabb.m_Maxs[1] = pEnt->r.maxs[1] - pos[1];
			//  _aabb.m_Maxs[2] = pEnt->r.maxs[2] - pos[2];
			//  return Success;
			//}

			//if(!Q_stricmp(pEnt->classname, "script_mover"))
			//{
			//  vec3_t fwd,rgt,up,mins,maxs;
			//  AngleVectors(pEnt->r.currentAngles, fwd, rgt, up);
			//  VectorCopy(pEnt->r.currentOrigin,mins);
			//  VectorCopy(pEnt->r.currentOrigin,maxs);

			//  VectorSubtract(mins,x,mins);

			//  VectorAdd();

			//  // start at the origin
			//  _aabb.m_Mins[0] = _aabb.m_Maxs[0] = pEnt->r.currentOrigin[0];
			//  _aabb.m_Mins[0] = _aabb.m_Maxs[0] = pEnt->r.currentOrigin[0];
			//  _aabb.m_Mins[0] = _aabb.m_Maxs[0] = pEnt->r.currentOrigin[0];

			//  // find the mins/maxs
			//  _aabb.m_Mins[0] = pEnt->r.currentOrigin[0] + pEnt->r.mins[0] * fwd[0];
			//  _aabb.m_Mins[1] = pEnt->r.currentOrigin[1] + pEnt->r.mins[1] * fwd[1];
			//  _aabb.m_Mins[1] = pEnt->r.currentOrigin[1] + pEnt->r.mins[2] * fwd[2];

			//  _aabb.m_Mins[0] = pEnt->r.mins[0] * fwd;
			//  _aabb.m_Mins[1] = pEnt->r.mins[1] - pos[1];
			//  _aabb.m_Mins[2] = pEnt->r.mins[2] - pos[2];
			//  _aabb.m_Maxs[0] = pEnt->r.maxs[0] - pos[0];
			//  _aabb.m_Maxs[1] = pEnt->r.maxs[1] - pos[1];
			//  _aabb.m_Maxs[2] = pEnt->r.maxs[2] - pos[2];

			//  return Success;
			//}

			_aabb.m_Mins[ 0 ] = pEnt->r.mins[ 0 ];
			_aabb.m_Mins[ 1 ] = pEnt->r.mins[ 1 ];
			_aabb.m_Mins[ 2 ] = pEnt->r.mins[ 2 ];
			_aabb.m_Maxs[ 0 ] = pEnt->r.maxs[ 0 ];
			_aabb.m_Maxs[ 1 ] = pEnt->r.maxs[ 1 ];
			_aabb.m_Maxs[ 2 ] = pEnt->r.maxs[ 2 ];

			// hack for bad abs bounds
			if( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
			{
				if( _aabb.IsZero() )
				{
					const float default_box_mins[] = { -8, -8, -8 };
					const float default_box_maxs[] = { 8, 8, 48 };
					_aabb.Set( default_box_mins, default_box_maxs );
				}
			}

			return Success;
		}

		return InvalidEntity;
	}

	obResult GetEntityWorldAABB( const GameEntity _ent, AABB &_aabb )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt && pEnt->inuse )
		{
			/*if(!pEnt->r.linked && pEnt->parent && pEnt->parent->s.eType == ET_OID_TRIGGER)
			pEnt = pEnt->parent;*/

			if( pEnt->s.eType == ET_CONSTRUCTIBLE )
			{
				gentity_t *pAxis = G_ConstructionForTeam( pEnt->parent ? pEnt->parent : pEnt, TEAM_AXIS );
				gentity_t *pAlly = G_ConstructionForTeam( pEnt->parent ? pEnt->parent : pEnt, TEAM_ALLIES );

				if( pAxis /*&& pAxis->entstate == STATE_DEFAULT*/ )
				{
					pEnt = pAxis;
				}
				else if( pAlly /*&& pAlly->entstate == STATE_DEFAULT*/ )
				{
					pEnt = pAlly;
				}
			}

			_aabb.m_Mins[ 0 ] = pEnt->r.absmin[ 0 ];
			_aabb.m_Mins[ 1 ] = pEnt->r.absmin[ 1 ];
			_aabb.m_Mins[ 2 ] = pEnt->r.absmin[ 2 ];
			_aabb.m_Maxs[ 0 ] = pEnt->r.absmax[ 0 ];
			_aabb.m_Maxs[ 1 ] = pEnt->r.absmax[ 1 ];
			_aabb.m_Maxs[ 2 ] = pEnt->r.absmax[ 2 ];

			// raise player bounds slightly since it appears to be in the ground a bit
			if( pEnt->client )
			{
				_aabb.m_Mins[ 2 ] += 2.f;
				_aabb.m_Maxs[ 2 ] += 2.f;
			}

			// hack for bad abs bounds
			if( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
			{
				if( _aabb.IsZero() || !_aabb.Contains( pEnt->r.currentOrigin ) )
				{
					float pos[ 3 ] = { 0, 0, 0 };
					const float default_box_mins[] = { -8, -8, -8 };
					const float default_box_maxs[] = { 8, 8, 48 };
					GetEntityPosition( _ent, pos );
					_aabb.Set( default_box_mins, default_box_maxs );
					_aabb.SetCenter( pos );
				}
			}

			return Success;
		}

		return InvalidEntity;
	}

	obResult GetEntityWorldOBB( const GameEntity _ent, float *_center, float *_axis0, float *_axis1, float *_axis2, float *_extents )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt )
		{
			vec3_t axis[ 3 ];
			AnglesToAxis( pEnt->r.currentAngles, axis );

			vec3_t boxCenter;
			boxCenter[ 0 ] = ( ( pEnt->r.maxs[ 0 ] + pEnt->r.mins[ 0 ] ) * 0.5f );
			boxCenter[ 1 ] = ( ( pEnt->r.maxs[ 1 ] + pEnt->r.mins[ 1 ] ) * 0.5f );
			boxCenter[ 2 ] = ( ( pEnt->r.maxs[ 2 ] + pEnt->r.mins[ 2 ] ) * 0.5f );

			vec3_t out;
			VectorCopy( pEnt->r.currentOrigin, out );

			for( int i = 0; i < 3; ++i )
			{
				vec3_t tmp;
				VectorScale( axis[ i ], boxCenter[ i ], tmp );
				VectorAdd( out, tmp, out );
			}

			VectorCopy( out, _center );
			VectorCopy( axis[ 0 ], _axis0 );
			VectorCopy( axis[ 1 ], _axis1 );
			VectorCopy( axis[ 2 ], _axis2 );
			_extents[ 0 ] = ( pEnt->r.maxs[ 0 ] - pEnt->r.mins[ 0 ] ) * 0.5f;
			_extents[ 1 ] = ( pEnt->r.maxs[ 1 ] - pEnt->r.mins[ 1 ] ) * 0.5f;
			_extents[ 2 ] = ( pEnt->r.maxs[ 2 ] - pEnt->r.mins[ 2 ] ) * 0.5f;

			return Success;
		}

		return InvalidEntity;
	}

	obResult GetEntityGroundEntity( const GameEntity _ent, GameEntity &moveent )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt )
		{
			if( pEnt->s.groundEntityNum > 0 && pEnt->s.groundEntityNum < ENTITYNUM_MAX_NORMAL )
			{
				moveent = HandleFromEntity( &g_entities[ pEnt->s.groundEntityNum ] );
			}

			return Success;
		}

		return InvalidEntity;
	}

	GameEntity GetEntityOwner( const GameEntity _ent )
	{
		GameEntity owner;

		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt && pEnt->inuse )
		{
			// hack, when the game joins clients again after warmup, they are temporarily ET_GENERAL entities(LAME)
			int t = pEnt->s.eType;

			if( pEnt->client && ( pEnt - g_entities ) < MAX_CLIENTS )
			{
				t = ET_PLAYER;
			}

			switch( t )
			{
				case ET_ITEM:
					{
						if( !Q_stricmp( pEnt->classname, "team_CTF_redflag" ) ||
						    !Q_stricmp( pEnt->classname, "team_CTF_blueflag" ) )
						{
							int iFlagEntNum = pEnt - g_entities;

							for( int i = 0; i < g_maxclients.integer; ++i )
							{
								if( g_entities[ i ].client && g_entities[ i ].client->flagParent == iFlagEntNum )
								{
									owner = HandleFromEntity( &g_entities[ i ] );
								}
							}
						}

						break;
					}

				case ET_GENERAL:
				case ET_MG42_BARREL:
					{
						if( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
						{
							if( pEnt->r.ownerNum != pEnt->s.number )
							{
								gentity_t *pOwner = &g_entities[ pEnt->r.ownerNum ];

								if( pOwner && pOwner->active && pOwner->client && pOwner->s.eFlags & EF_MG42_ACTIVE )
								{
									owner = HandleFromEntity( pOwner );
								}
							}
						}

						break;
					}

				default:

					// -1 means theres no owner.
					if( pEnt->r.ownerNum < MAX_GENTITIES )
					{
						owner = HandleFromEntity( &g_entities[ pEnt->r.ownerNum ] );
					}
			}
		}

		return owner;
	}

	int GetEntityTeam( const GameEntity _ent )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );
		return pEnt && pEnt->inuse ? _GetEntityTeam( pEnt ) : ET_TEAM_NONE;
	}

	const char *GetEntityName( const GameEntity _ent )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt )
		{
			return _GetEntityName( pEnt );
		}

		return NULL;
	}

	obResult GetCurrentWeaponClip( const GameEntity _ent, FireMode _mode, int &_curclip, int &_maxclip )
	{
		gentity_t *bot = EntityFromHandle( _ent );

		if( bot && bot->inuse && bot->client )
		{
			int iWeapon = bot->client->ps.weapon;
#ifdef NOQUARTER
			_curclip = bot->client->ps.ammoclip[ WeaponTable[( weapon_t ) iWeapon ].clipindex ];
#else
			_curclip = bot->client->ps.ammoclip[ BG_FindClipForWeapon( ( weapon_t ) iWeapon ) ];
#endif

			// sanity check for non-clipped weapons
			switch( iWeapon )
			{
				case WP_MEDIC_ADRENALINE:
				case WP_AMMO:
				case WP_BINOCULARS:
				case WP_DYNAMITE:
				case WP_FLAMETHROWER:
				case WP_GRENADE_PINEAPPLE:
				case WP_GRENADE_LAUNCHER:
				case WP_KNIFE:
				case WP_LANDMINE:
				case WP_MEDKIT:
				case WP_MORTAR:
				case WP_MORTAR_SET:
				case WP_PANZERFAUST:
#ifdef NOQUARTER
				case WP_MORTAR2:
				case WP_MORTAR2_SET:
				case WP_BAZOOKA:
#endif
				case WP_PLIERS:
				case WP_SATCHEL:
				case WP_SATCHEL_DET:
				case WP_SMOKE_BOMB:
				case WP_SMOKE_MARKER:
				case WP_MEDIC_SYRINGE:
					_maxclip = 0;
					break;

				default:
#ifdef NOQUARTER
					_maxclip = GetWeaponTableData( iWeapon )->maxclip;
#else
					_maxclip = GetAmmoTableData( iWeapon )->maxclip;
#endif
			}

			return Success;
		}

		return InvalidEntity;
	}

	obResult GetCurrentAmmo( const GameEntity _ent, int _weaponId, FireMode _mode, int &_cur, int &_max )
	{
		gentity_t *bot = EntityFromHandle( _ent );

		if( bot && bot->inuse && bot->client )
		{
			int maxclip = 0;
			int ammoIndex = 0;

			_weaponId = _weaponBotToGame( _weaponId );

#ifdef NOQUARTER

			// need to translate for correct ammo ...
			if( bot->client->sess.sessionTeam == TEAM_ALLIES )
			{
				switch( _weaponId )
				{
					case WP_MOBILE_MG42:
						_weaponId = WP_MOBILE_BROWNING;
						break;

					case WP_MOBILE_MG42_SET:
						_weaponId = WP_MOBILE_BROWNING_SET;
						break;

					default:
						break;
				}
			}
			else if( bot->client->sess.sessionTeam == TEAM_AXIS )
			{
				switch( _weaponId )
				{
					case WP_MORTAR:
						_weaponId = WP_MORTAR2;
						break;

					case WP_MORTAR_SET:
						_weaponId = WP_MORTAR2_SET;
						break;

					default:
						break;
				}
			}

#endif

			ammoIndex = BG_FindAmmoForWeapon( ( weapon_t ) _weaponId );

#ifdef NOQUARTER
			_cur = bot->client->ps.ammoclip[ WeaponTable[( weapon_t ) _weaponId ].clipindex ] +
			       bot->client->ps.ammo[ WeaponTable[( weapon_t ) _weaponId ].ammoindex ];
#else
			_cur = bot->client->ps.ammoclip[ BG_FindClipForWeapon( ( weapon_t ) _weaponId ) ] +
			       bot->client->ps.ammo[ ammoIndex ];
#endif

			// sanity check for non-clipped weapons
			switch( _weaponId )
			{
				case WP_MEDIC_ADRENALINE:
				case WP_AMMO:
				case WP_BINOCULARS:
				case WP_DYNAMITE:
				case WP_FLAMETHROWER:
				case WP_GRENADE_PINEAPPLE:
				case WP_GRENADE_LAUNCHER:
				case WP_KNIFE:
				case WP_LANDMINE:
				case WP_MEDKIT:
				case WP_MORTAR:
				case WP_MORTAR_SET:
				case WP_PANZERFAUST:
#ifdef NOQUARTER
				case WP_MORTAR2:
				case WP_MORTAR2_SET:
				case WP_BAZOOKA:
#endif
				case WP_PLIERS:
				case WP_SATCHEL:
				case WP_SATCHEL_DET:
				case WP_SMOKE_BOMB:
				case WP_SMOKE_MARKER:
				case WP_MEDIC_SYRINGE:
					maxclip = 0;
					break;

				default:
#ifdef NOQUARTER
					maxclip = GetWeaponTableData( ammoIndex )->maxclip;
#else
					maxclip = GetAmmoTableData( ammoIndex )->maxclip;
#endif
			}

#ifdef NOQUARTER
			_max = maxclip + GetWeaponTableData( ammoIndex )->maxammo;
#else
			_max = maxclip + BG_MaxAmmoForWeapon( ( weapon_t ) _weaponId, bot->client->sess.skill );
#endif
			return Success;
		}

		return InvalidEntity;
	}

	int GetGameTime()
	{
		return level.time;
	}

	void GetGoals()
	{
		g_GoalSubmitReady = true;
		gentity_t *e;

		SendDeferredGoals();
		GetMG42s();

		for( int i = MAX_CLIENTS; i < level.num_entities; ++i )
		{
			e = &g_entities[ i ];

			if( !e->inuse )
			{
				continue;
			}

			const char *pGoalName = _GetEntityName( e );

			switch( e->s.eType )
			{
				case ET_ITEM:
					{
						if( !Q_stricmp( e->classname, "team_CTF_redflag" ) )
						{
							// allies flag
							Bot_Util_AddGoal( "flag", e, ( 1 << ET_TEAM_ALLIES ), pGoalName );
						}
						else if( !Q_stricmp( e->classname, "team_CTF_blueflag" ) )
						{
							// axis flag
							Bot_Util_AddGoal( "flag", e, ( 1 << ET_TEAM_AXIS ), pGoalName );
						}

						break;
					}

				case ET_OID_TRIGGER:
					{
						// detect constructibles
						char *pTmp = 0;
						gentity_t *eAxis = G_ConstructionForTeam( e, TEAM_AXIS );
						gentity_t *eAlly = G_ConstructionForTeam( e, TEAM_ALLIES );
						const char *pAllyGoalName = 0, *pAxisGoalName = 0;
						//gentity_t *mg42ent;

						//////////////////////////////////////////////////////////////////////////
						if( eAxis )
						{
							/*if(e->target_ent)
							pAxisGoalName = _GetEntityName(e->target_ent);*/
							//if(!pAxisGoalName)
							//  pAxisGoalName = _GetEntityName(eAxis);
							if( !pAxisGoalName )
							{
								pAxisGoalName = _GetEntityName( e );
							}

							if( !pAxisGoalName && e->parent )
							{
								pAxisGoalName = _GetEntityName( e->parent );
							}

							// detect fakeobj's
							if( strncmp( eAxis->targetname, "fakeobj", 7 ) == 0 )
							{
								continue;
							}
						}

						//////////////////////////////////////////////////////////////////////////
						if( eAlly )
						{
							/*if(e->target_ent)
							pAllyGoalName = _GetEntityName(e->target_ent);*/
							//if(!pAllyGoalName)
							//  pAllyGoalName = _GetEntityName(eAlly);
							if( !pAllyGoalName )
							{
								pAllyGoalName = _GetEntityName( e );
							}

							if( !pAllyGoalName && e->parent )
							{
								pAllyGoalName = _GetEntityName( e->parent );
							}

							// detect fakeobj's
							if( strncmp( eAlly->targetname, "fakeobj", 7 ) == 0 )
							{
								continue;
							}
						}

						//////////////////////////////////////////////////////////////////////////
						//if(pAxisGoalName && pAllyGoalName)
						//{
						//  if(!Q_stricmp(pAxisGoalName, pAllyGoalName))
						//  {
						//    PrintMessage("Goals Have Same Name!!");
						//    PrintMessage(pAxisGoalName);
						//  }
						//}
						//////////////////////////////////////////////////////////////////////////
						if( eAxis && eAlly && !Q_stricmp( pAxisGoalName, pAllyGoalName ) )
						{
							//bool bMover = pAxisGoalName ? strstr(pAxisGoalName, "_construct")!=0 : false;
							bool bMover = e->target ? strstr( e->target, "_construct" ) != 0 : false;
							obUserData ud = obUserData( bMover ? 1 : 0 );
							Bot_Util_AddGoal( "build", e, ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ), pAxisGoalName, "Mobile", &ud );
						}
						else
						{
							if( eAxis )
							{
								//bool bMover = pAxisGoalName ? strstr(pAxisGoalName, "_construct")!=0 : false;
								bool bMover = e->target ? strstr( e->target, "_construct" ) != 0 : false;
								obUserData ud = obUserData( bMover ? 1 : 0 );
								Bot_Util_AddGoal( "build", e, ( 1 << ET_TEAM_AXIS ), pAxisGoalName, "Mobile", &ud );
							}

							if( eAlly )
							{
								//bool bMover = pAllyGoalName ? strstr(pAllyGoalName, "_construct")!=0 : false;
								bool bMover = e->target ? strstr( e->target, "_construct" ) != 0 : false;
								obUserData ud = obUserData( bMover ? 1 : 0 );
								Bot_Util_AddGoal( "build", e, ( 1 << ET_TEAM_ALLIES ), pAllyGoalName, "Mobile", &ud );
							}
						}

						//////////////////////////////////////////////////////////////////////////

						// register constructions as dyno targets, too
						// check if this is an enemy construction and check spawnflags if this is a dynamite target
						if( !e->target_ent )
						{
							continue;
						}

						// If we didn't find a name from the entity, try to get one from its target_ent

						/*pAxisGoalName = pAxisGoalName ? pAxisGoalName : _GetEntityName(e->target_ent);
						pAllyGoalName = pAllyGoalName ? pAllyGoalName : _GetEntityName(e->target_ent);*/
						//pAxisGoalName = _GetEntityName(e->target_ent) ? _GetEntityName(e->target_ent) : pAxisGoalName;
						//pAllyGoalName = _GetEntityName(e->target_ent) ? _GetEntityName(e->target_ent) : pAllyGoalName;
						pAxisGoalName = _GetEntityName( e ) ? _GetEntityName( e ) : pAxisGoalName;
						pAllyGoalName = _GetEntityName( e ) ? _GetEntityName( e ) : pAllyGoalName;

						//////////////////////////////////////////////////////////////////////////
						// Is this a movable?, skip it if so.
						if( e->target_ent->targetname &&
						    ( pTmp = strstr( e->target_ent->targetname, "_construct" ) ) != NULL )
						{
							char strName[ 256 ];
							Q_strncpyz( strName, e->target_ent->targetname, 256 );
							strName[ pTmp - e->target_ent->targetname ] = 0;
							gentity_t *pMover = G_FindByTargetname( NULL, strName );

							if( pMover &&
							    pMover->s.eType == ET_MOVER &&
							    !Q_stricmp( pMover->classname, "script_mover" ) )
							{
								continue;
							}
						}

						//////////////////////////////////////////////////////////////////////////

						if( e->target_ent->s.eType == ET_CONSTRUCTIBLE )
						{
							obUserData bud;
							bud.DataType = obUserData::dtInt;
							bud.udata.m_Int = 0;

							switch( e->target_ent->constructibleStats.weaponclass )
							{
								case -1: // Grenade, fall through and also register as satchel and dyno.
								case 1: // Satchelable, it should fall through and also register as dyno.
									bud.udata.m_Int |= XPLO_TYPE_SATCHEL;

								case 2: // Dynamite
									bud.udata.m_Int |= XPLO_TYPE_DYNAMITE;
									break;

								default:

									// < 0 is apparently satchel only.
									if( e->target_ent->constructibleStats.weaponclass < 0 )
									{
										bud.udata.m_Int |= XPLO_TYPE_SATCHEL;
									}

									break;
							}

							// NOTE: sometimes goals get registered that don't have 1 or 2 weaponclass
							// is this correct? for now let's not register them and see what happens.
							if( bud.udata.m_Int != 0 )
							{
								// If there's a goal for each team, and it has the same name, just register 1 goal
								if( eAxis && eAlly && !Q_stricmp( pAxisGoalName, pAllyGoalName ) )
								{
									Bot_Util_AddGoal( "plant", e->target_ent,
									                  ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ),
									                  pAxisGoalName,
									                  "ExplosiveType",
									                  &bud );
									CheckForMG42( e->target_ent, pAxisGoalName );
								}
								else
								{
									if( eAxis )
									{
										Bot_Util_AddGoal( "plant", e->target_ent,
										                  ( 1 << ET_TEAM_ALLIES ),
										                  pAxisGoalName,
										                  "ExplosiveType",
										                  &bud );
										CheckForMG42( e->target_ent, pAxisGoalName );
									}

									if( eAlly )
									{
										Bot_Util_AddGoal( "plant", e->target_ent,
										                  ( 1 << ET_TEAM_AXIS ),
										                  pAllyGoalName,
										                  "ExplosiveType",
										                  &bud );
										CheckForMG42( e->target_ent, pAllyGoalName );
									}
								}
							}
							else // for grenadetarget
							{
								if( eAxis )
								{
									Bot_Util_AddGoal( "explode",
									                  e->target_ent,
									                  ( 1 << ET_TEAM_ALLIES ),
									                  pAxisGoalName,
									                  "ExplosiveType",
									                  &bud );
								}

								if( eAlly )
								{
									Bot_Util_AddGoal( "explode",
									                  e->target_ent,
									                  ( 1 << ET_TEAM_AXIS ),
									                  pAllyGoalName,
									                  "ExplosiveType",
									                  &bud );
								}
							}
						}
						// check for objective targets
						else if( e->target_ent->s.eType == ET_EXPLOSIVE )
						{
							obUserData bud;

							switch( e->target_ent->constructibleStats.weaponclass )
							{
								case 1:
									{
										// SATCHEL
										bud.udata.m_Int |= XPLO_TYPE_SATCHEL;
										// note ALLIED/AXIS stuff is reversed here (ET is strange sometimes)

										/*if (e->spawnflags & ALLIED_OBJECTIVE)
										Bot_Util_AddGoal("satcheltarget",
										e->target_ent,
										(1<<ET_TEAM_AXIS),
										pAxisGoalName);
										if (e->spawnflags & AXIS_OBJECTIVE)
										Bot_Util_AddGoal("satcheltarget",
										e->target_ent,
										(1<<ET_TEAM_ALLIES),
										pAllyGoalName);*/
									}

									// fall through to register dyna too.
								case 2:
									{
										// DYNAMITE
										bud.udata.m_Int |= XPLO_TYPE_DYNAMITE;

										// note ALLIED/AXIS stuff is reversed here (ET is strange sometimes)
										if( e->spawnflags & ALLIED_OBJECTIVE )
										{
											Bot_Util_AddGoal( "plant",
											                  e->target_ent,
											                  ( 1 << ET_TEAM_AXIS ),
											                  pAxisGoalName,
											                  "ExplosiveType",
											                  &bud );
										}

										if( e->spawnflags & AXIS_OBJECTIVE )
										{
											Bot_Util_AddGoal( "plant",
											                  e->target_ent,
											                  ( 1 << ET_TEAM_ALLIES ),
											                  pAllyGoalName,
											                  "ExplosiveType",
											                  &bud );
										}

										break;
									}
							}
						}

						break;
					}

					//case ET_GENERAL:
				case ET_MG42_BARREL:
					{
						//    if(!Q_stricmp(e->classname, "misc_mg42"))
						//    {
						//      int team = 0;
						//      team |= (1 << ET_TEAM_ALLIES);
						//      team |= (1 << ET_TEAM_AXIS);
						//      pGoalName = _GetEntityName(e);
						//      Bot_Util_AddGoal(e, ET_GOAL_MG42MOUNT, team, pGoalName, NULL);
						//    }
						UpdateMG42( e );

						break;
					}

				case ET_MOVER:
					{
						int team = 0;

						if( e->spawnflags & 32 )
						{
							team = ( 1 << ET_TEAM_ALLIES );
						}
						else if( e->spawnflags & 64 )
						{
							team = ( 1 << ET_TEAM_AXIS );
						}
						else
						{
							team |= ( 1 << ET_TEAM_ALLIES );
							team |= ( 1 << ET_TEAM_AXIS );
						}

						pGoalName = _GetEntityName( e );

						if( pGoalName && !Q_stricmp( e->classname, "script_mover" ) &&
						    e->think != G_FreeEntity )
						{
							Bot_Util_AddGoal( "mover", e, team, pGoalName );
						}

						break;
					}

				case ET_CONSTRUCTIBLE:
					{
						if( e->classname )
						{
							//pfnPrintMessage("constructible");
						}

						break;
					}

					//case ET_CABINET_H:
					//  {
					//    int team = 0;
					//    team |= (1 << ET_TEAM_ALLIES);
					//    team |= (1 << ET_TEAM_AXIS);
					//    pGoalName = _GetEntityName(e);
					//    //Bot_Util_AddGoal(e, GOAL_HEALTH, team, pGoalName, NULL);
					//    break;
					//  }
					//case ET_CABINET_A:
					//  {
					//    int team = 0;
					//    team |= (1 << ET_TEAM_ALLIES);
					//    team |= (1 << ET_TEAM_AXIS);
					//    pGoalName = _GetEntityName(e);
					//    //Bot_Util_AddGoal(e, GOAL_AMMO, team, pGoalName, NULL);
					//    break;
					//  }
				case ET_HEALER:
					{
						obUserData bud( e->damage );  // heal rate
						int team = ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS );
						pGoalName = _GetEntityName( e->target_ent ) ? _GetEntityName( e->target_ent ) : _GetEntityName( e );
						Bot_Util_AddGoal( "healthcab", e, team, pGoalName, "HealRate", &bud );
						break;
					}

				case ET_SUPPLIER:
					{
						obUserData bud( e->damage );  // heal rate
						int team = ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS );
						pGoalName = _GetEntityName( e->target_ent ) ? _GetEntityName( e->target_ent ) : _GetEntityName( e );
						Bot_Util_AddGoal( "ammocab", e, team, pGoalName, "AmmoRate", &bud );
						break;
					}

				case ET_TRAP:
					{
						if( !Q_stricmp( e->classname, "team_WOLF_checkpoint" ) )
						{
							Bot_Util_AddGoal( "checkpoint", e, 0, pGoalName );
						}

						break;
					}

				case ET_EXPLOSIVE:
					{
						//CS: hack - weaponclass not set yet, using xpbonus
						if( !Q_stricmp( e->classname, "func_explosive" ) && e->constructibleStats.constructxpbonus == 5 )
						{
							pGoalName = _GetEntityName( e );
							Bot_Util_AddGoal( "explosive", e, 0, pGoalName );
						}

						break;
					}

				default:
					{
						break;
					}
			}
		}

		for( int i = 0; i < numofmg42s; ++i )
		{
			char strName[ 256 ];

			if( mg42s[ i ].buildable )
			{
				strcpy( strName, mg42s[ i ].newname );
			}
			else
			{
				strcpy( strName, mg42s[ i ].name );
			}

			Bot_Util_AddGoal( "mountmg42", mg42s[ i ].ent,
			                  ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ), strName );

			// cs: this was done in et_goalmanager before
			Bot_Util_AddGoal( "repairmg42", mg42s[ i ].ent,
			                  ( 1 << ET_TEAM_ALLIES ) | ( 1 << ET_TEAM_AXIS ), strName );
		}
	}

	void GetPlayerInfo( obPlayerInfo &info )
	{
		info.m_AvailableTeams |= ( 1 << ET_TEAM_ALLIES );
		info.m_AvailableTeams |= ( 1 << ET_TEAM_AXIS );

		info.m_MaxPlayers = level.maxclients;

		GameEntity ge;

		for( int i = 0; i < g_maxclients.integer; ++i )
		{
			if( !g_entities[ i ].inuse )
			{
				continue;
			}

			if( !g_entities[ i ].client )
			{
				continue;
			}

			if( g_entities[ i ].client->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			ge = HandleFromEntity( &g_entities[ i ] );

			info.m_Players[ i ].m_Team = GetEntityTeam( ge );
			info.m_Players[ i ].m_Class = GetEntityClass( ge );
			info.m_Players[ i ].m_Controller = IsBot( &g_entities[ i ] ) ?
			                                   obPlayerInfo::Bot : obPlayerInfo::Human;
		}
	}

	obResult InterfaceSendMessage( const MessageHelper &_data, const GameEntity _ent )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		switch( _data.GetMessageId() )
		{
				// general messages
			case GEN_MSG_ISALIVE:
				{
					OB_GETMSG( Msg_IsAlive );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client && pEnt->health > 0 &&
						    pEnt->client->ps.pm_type == PM_NORMAL )
						{
							pMsg->m_IsAlive = True;
						}
					}

					break;
				}

			case GEN_MSG_ISRELOADING:
				{
					OB_GETMSG( Msg_Reloading );

					if( pMsg )
					{
						if( ( pEnt && pEnt->inuse && pEnt->client && pEnt->client->ps.weaponstate >= WEAPON_RAISING ) &&
						    ( pEnt->client->ps.weaponstate <= WEAPON_RELOADING ) )
						{
							pMsg->m_Reloading = True;
						}
					}

					break;
				}

			case GEN_MSG_ISREADYTOFIRE:
				{
					OB_GETMSG( Msg_ReadyToFire );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client )
						{
							if( pEnt->client->ps.weaponstate == WEAPON_READY || pEnt->client->ps.weaponstate == WEAPON_FIRING )
							{
								pMsg->m_Ready = True;
							}
						}
					}

					break;
				}

			case GEN_MSG_GETEQUIPPEDWEAPON:
				{
					OB_GETMSG( WeaponStatus );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client )
						{
							pMsg->m_WeaponId = Bot_WeaponGameToBot( pEnt->client->ps.weapon );
						}
						else
						{
							pMsg->m_WeaponId = 0;
						}
					}

					break;
				}

			case GEN_MSG_GETMOUNTEDWEAPON:
				{
					OB_GETMSG( WeaponStatus );

					if( pMsg && pEnt && pEnt->inuse && pEnt->client )
					{
						pMsg->m_WeaponId = BG_PlayerMounted( pEnt->s.eFlags ) ? ET_WP_MOUNTABLE_MG42 : ET_WP_NONE;
					}

					break;
				}

			case GEN_MSG_GETWEAPONLIMITS:
				{
					OB_GETMSG( WeaponLimits );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client )
						{
							if( pMsg->m_WeaponId == ET_WP_MOUNTABLE_MG42 &&
							    BG_PlayerMounted( pEnt->client->ps.eFlags ) )
							{
								pMsg->m_Limited = True;
								AngleVectors( pEnt->client->pmext.centerangles, pMsg->m_CenterFacing, NULL, NULL );

								if( pEnt->client->ps.eFlags & EF_MOUNTEDTANK )
								{
									// seems tanks have complete horizonal movement, and fixed vertical
									pMsg->m_MinYaw = -360;
									pMsg->m_MaxYaw = -360;
									pMsg->m_MinPitch = 14;
									pMsg->m_MaxPitch = 50;
								}
								else
								{
									pMsg->m_MinYaw = -pEnt->client->pmext.harc;
									pMsg->m_MaxYaw = pEnt->client->pmext.harc;
									pMsg->m_MinPitch = -pEnt->client->pmext.varc;
									pMsg->m_MaxPitch = pEnt->client->pmext.varc;
								}
							}

							if( pMsg->m_WeaponId == ET_WP_MOBILE_MG42 || pMsg->m_WeaponId == ET_WP_MOBILE_MG42_SET )
							{
								AngleVectors( pEnt->client->pmext.mountedWeaponAngles, pMsg->m_CenterFacing, NULL, NULL );
								pMsg->m_Limited = True;
								pMsg->m_MinYaw = -20.f;
								pMsg->m_MaxYaw = 20.f;
								pMsg->m_MinPitch = -20.f;
								pMsg->m_MaxPitch = 20.f;
							}
						}
						else
						{
							pMsg->m_Limited = False;
						}
					}

					//////////////////////////////////////////////////////////////////////////
					break;
				}

			case GEN_MSG_GETHEALTHARMOR:
				{
					OB_GETMSG( Msg_HealthArmor );

					if( pMsg )
					{
						// No Armor in ET
						pMsg->m_CurrentArmor = pMsg->m_MaxArmor = 0;

						if( pEnt )
						{
							switch( pEnt->s.eType )
							{
								case ET_MOVER:
								case ET_CONSTRUCTIBLE:
									pMsg->m_CurrentHealth = pEnt->takedamage ? pEnt->health > 0 ? pEnt->health : 0 : 0;
									pMsg->m_MaxHealth = pEnt->count;
									break;

								case ET_GENERAL:
								case ET_MG42_BARREL:
									if( !Q_stricmp( pEnt->classname, "misc_mg42" ) )
									{
										if( Simple_EmplacedGunIsRepairable( pEnt ) )
										{
											pMsg->m_CurrentHealth = 0;
											pMsg->m_MaxHealth = MG42_MULTIPLAYER_HEALTH;
										}
										else
										{
											if( pEnt->mg42BaseEnt > 0 )
											{
												gentity_t *pBase = &g_entities[ pEnt->mg42BaseEnt ];
												pMsg->m_CurrentHealth = pBase->health;
												pMsg->m_MaxHealth = MG42_MULTIPLAYER_HEALTH;
											}
											else
											{
												// just in case
												pMsg->m_CurrentHealth = pEnt->health;
												pMsg->m_MaxHealth = MG42_MULTIPLAYER_HEALTH;
											}
										}
									}

									break;

								default:
									if( pEnt->client )
									{
										pMsg->m_CurrentHealth = pEnt->client->ps.stats[ STAT_HEALTH ];
										pMsg->m_MaxHealth = pEnt->client->ps.stats[ STAT_MAX_HEALTH ];
									}
									else
									{
										pMsg->m_CurrentHealth = pEnt->takedamage ? pEnt->health : 0;
										pMsg->m_MaxHealth = 0;
									}
							}
						}
					}

					break;
				}

			case GEN_MSG_GETMAXSPEED:
				{
					OB_GETMSG( Msg_PlayerMaxSpeed );

					if( pMsg && pEnt )
					{
						pMsg->m_MaxSpeed = ( float ) g_movespeed.integer;
					}

					break;
				}

			case GEN_MSG_ISALLIED:
				{
					OB_GETMSG( Msg_IsAllied );

					if( pMsg )
					{
						gentity_t *pEntOther = EntityFromHandle( pMsg->m_TargetEntity );

						if( pEntOther && pEnt )
						{
							if( ENTINDEX( pEntOther ) == ENTITYNUM_WORLD )
							{
								pMsg->m_IsAllied = True;
							}
							else
							{
								pMsg->m_IsAllied = _GetEntityTeam( pEnt ) == _GetEntityTeam( pEntOther ) ? True : False;
							}
						}
					}

					break;
				}

			case GEN_MSG_ISOUTSIDE:
				{
					OB_GETMSG( Msg_IsOutside );

					if( pMsg )
					{
						trace_t tr;
						vec3_t end = { pMsg->m_Position[ 0 ], pMsg->m_Position[ 1 ], ( pMsg->m_Position[ 2 ] + 4096 ) };
						trap_Trace( &tr, pMsg->m_Position, NULL, NULL, end, -1, MASK_SOLID );

						if( ( tr.fraction < 1.0 ) && !( tr.surfaceFlags & SURF_NOIMPACT ) )
						{
							pMsg->m_IsOutside = False;
						}
						else
						{
							pMsg->m_IsOutside = True;
						}
					}

					break;
				}

			case GEN_MSG_CHANGENAME:
				{
					OB_GETMSG( Msg_ChangeName );

					if( pMsg && pEnt && pEnt->client )
					{
						char userinfo[ MAX_INFO_STRING ];
						trap_GetUserinfo( pEnt - g_entities, userinfo, MAX_INFO_STRING );
						Info_SetValueForKey( userinfo, "name", pMsg->m_NewName );
						trap_SetUserinfo( pEnt - g_entities, userinfo );
						ClientUserinfoChanged( pEnt - g_entities );
					}

					break;
				}

			case GEN_MSG_ENTITYKILL:
				{
					OB_GETMSG( Msg_KillEntity );

					if( pMsg && pMsg->m_WhoToKill.IsValid() && g_cheats.integer )
					{
						gentity_t *pWho = EntityFromHandle( pMsg->m_WhoToKill );

						if( pWho )
						{
							G_Damage( pWho, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
						}
					}

					break;
				}

			case GEN_MSG_SERVERCOMMAND:
				{
					OB_GETMSG( Msg_ServerCommand );

					if( pMsg && pMsg->m_Command[ 0 ] /*&& g_cheats.integer*/ )
					{
						trap_SendConsoleCommand( EXEC_NOW, pMsg->m_Command );
					}

					break;
				}

			case GEN_MSG_GETFLAGSTATE:
				{
					OB_GETMSG( Msg_FlagState );

					if( pMsg )
					{
						/* CS: flagreturn position is not updated while its carried.
						if(pEnt)
						{
						if(!(pEnt->flags & FL_DROPPED_ITEM) && !(pEnt->flags & FL_NODRAW))
						pMsg->m_FlagState = S_FLAG_AT_BASE;
						else if(pEnt->flags & FL_NODRAW)
						pMsg->m_FlagState = S_FLAG_CARRIED;
						else if(pEnt->flags & FL_DROPPED_ITEM)
						pMsg->m_FlagState = S_FLAG_DROPPED;
						}
						*/
					}

					break;
				}

			case GEN_MSG_GETCONTROLLINGTEAM:
				{
					ControllingTeam *pMsg = _data.Get< ControllingTeam >();

					if( pMsg )
					{
						if( pEnt && pEnt->s.eType == ET_TRAP )
						{
							pMsg->m_ControllingTeam = Bot_TeamGameToBot( pEnt->count );
						}
					}

					break;
				}

			case GEN_MSG_GAMESTATE:
				{
					OB_GETMSG( Msg_GameState );

					if( pMsg )
					{
						if( level.framenum < GAME_INIT_FRAMES )
						{
							pMsg->m_GameState = GAME_STATE_INVALID;
							break;
						}

						int iTimeLimit = ( int )( g_timelimit.value * 60000 );
						int iMatchTime = level.timeCurrent - level.startTime;
						pMsg->m_TimeLeft = ( iTimeLimit - iMatchTime ) / 1000.f;

						switch( g_gamestate.integer )
						{
							case GS_PLAYING:
								pMsg->m_GameState = GAME_STATE_PLAYING;
								break;

							case GS_WARMUP_COUNTDOWN:
								pMsg->m_GameState = GAME_STATE_WARMUP_COUNTDOWN;
								break;

							case GS_WARMUP:
								pMsg->m_GameState = GAME_STATE_WARMUP;
								break;

							case GS_INTERMISSION:
								pMsg->m_GameState = GAME_STATE_INTERMISSION;
								break;

							case GS_WAITING_FOR_PLAYERS:
								pMsg->m_GameState = GAME_STATE_WAITINGFORPLAYERS;
								break;

							default:
								pMsg->m_GameState = GAME_STATE_INVALID;
								break;
						}
					}

					break;
				}

			case GEN_MSG_ENTITYSTAT:
				{
					OB_GETMSG( Msg_EntityStat );

					if( pMsg )
					{
						if( pEnt && pEnt->client && !strcmp( pMsg->m_StatName, "kills" ) )
						{
							pMsg->m_Result = obUserData( pEnt->client->sess.kills );
						}
						else if( pEnt && pEnt->client && !strcmp( pMsg->m_StatName, "deaths" ) )
						{
							pMsg->m_Result = obUserData( pEnt->client->sess.deaths );
						}
						else if( pEnt && pEnt->client && !strcmp( pMsg->m_StatName, "xp" ) )
						{
							pMsg->m_Result = obUserData( pEnt->client->ps.stats[ STAT_XP ] );
						}
					}

					break;
				}

			case GEN_MSG_TEAMSTAT:
				{
					OB_GETMSG( Msg_TeamStat );

					/*if(pMsg)
					{
					        if(!strcmp(pMsg->m_StatName, "score"))
					        pMsg->m_Result = obUserData(0);
					        else if(!strcmp(pMsg->m_StatName, "deaths"))
					        pMsg->m_Result = obUserData(0);
					}
					*/
					break;
				}

			case GEN_MSG_WPCHARGED:
				{
					OB_GETMSG( WeaponCharged );

					if( pMsg && pEnt && pEnt->inuse && pEnt->client )
					{
#ifdef NOQUARTER

						if( pMsg->m_Weapon == ET_WP_BINOCULARS && ( pEnt->client->ps.ammo[ WP_ARTY ] & NO_ARTY ) )
						{
							pMsg->m_IsCharged = False;
						}
						else if( pMsg->m_Weapon == ET_WP_SMOKE_MARKER && ( pEnt->client->ps.ammo[ WP_ARTY ] & NO_AIRSTRIKE ) )
						{
							pMsg->m_IsCharged = False;
						}
						else
						{
#endif
							pMsg->m_IsCharged =
							  ( weaponCharged( &pEnt->client->ps, pEnt->client->sess.sessionTeam,
							                   _weaponBotToGame( pMsg->m_Weapon ), pEnt->client->sess.skill ) == qtrue ) ? True : False;
#ifdef NOQUARTER
						}

#endif
					}

					break;
				}

			case GEN_MSG_WPHEATLEVEL:
				{
					OB_GETMSG( WeaponHeatLevel );

					if( pMsg && pEnt && pEnt->inuse && pEnt->client )
					{
						pMsg->m_CurrentHeat = pEnt->client->ps.weapHeat[ WP_DUMMY_MG42 ];
						pMsg->m_MaxHeat = MAX_MG42_HEAT;
					}

					break;
				}

			case GEN_MSG_MOVERAT:
				{
					OB_GETMSG( Msg_MoverAt );

					/*
					if(pMsg)
					{
					        Vector org(
					        pMsg->m_Position[0],
					        pMsg->m_Position[1],
					        pMsg->m_Position[2]);
					        Vector under(
					        pMsg->m_Under[0],
					        pMsg->m_Under[1],
					        pMsg->m_Under[2]);

					        trace_t tr;
					        UTIL_TraceLine(org, under, MASK_SOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);

					        if(tr.DidHitNonWorldEntity() && !tr.m_pEnt->IsPlayer()&&!startsolid)
					        {
					        pMsg->m_Entity = HandleFromEntity(tr.m_pEnt);

					}
					}*/
					break;
				}

			case GEN_MSG_GOTOWAYPOINT:
				{
					OB_GETMSG( Msg_GotoWaypoint );

					if( pMsg && pMsg->m_Origin && g_cheats.integer )
					{
						char *cmd = va( "setviewpos %f %f %f %f", pMsg->m_Origin[ 0 ], pMsg->m_Origin[ 1 ], pMsg->m_Origin[ 2 ], 0.f );
						trap_SendConsoleCommand( EXEC_NOW, cmd );
						return Success;
					}

					break;
				}

				//////////////////////////////////////////////////////////////////////////
			case ET_MSG_GOTOLIMBO:
				{
					OB_GETMSG( ET_GoLimbo );

					if( pMsg )
					{
						// Dens: don't forget to look if the pEnt SHOULD really be in limbo
						// this check prevenst extra bodies and weird spectator behaviour
						int limbo_health = GIB_HEALTH;

						if( pEnt && pEnt->inuse && pEnt->client && pEnt->health > limbo_health &&
						    pEnt->client->ps.pm_type == PM_DEAD && !( pEnt->client->ps.pm_flags & PMF_LIMBO ) )
						{
							limbo( pEnt, qtrue );
							pMsg->m_GoLimbo = True;
						}
						else
						{
							pMsg->m_GoLimbo = False;
						}
					}

					break;
				}

			case ET_MSG_ISMEDICNEAR:
				{
					OB_GETMSG( ET_MedicNear );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client &&
						    pEnt->client->ps.pm_type == PM_DEAD && !( pEnt->client->ps.pm_flags & PMF_LIMBO ) )
						{
							pMsg->m_MedicNear = ( pEnt->client->ps.viewlocked == 7 ) ? True : False;
						}
						else
						{
							pMsg->m_MedicNear = Invalid;
						}
					}

					break;
				}

			case ET_MSG_ISWAITINGFORMEDIC:
				{
					OB_GETMSG( ET_WaitingForMedic );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client &&
						    ( pEnt->health <= 60 || pEnt->client->ps.pm_type == PM_DEAD ) &&
						    !( pEnt->client->ps.pm_flags & PMF_LIMBO ) )
						{
							pMsg->m_WaitingForMedic = True;
						}
						else if( pEnt && pEnt->inuse && pEnt->client &&
						         pEnt->client->ps.pm_type == PM_NORMAL )
						{
							pMsg->m_WaitingForMedic = False;
						}
						else
						{
							pMsg->m_WaitingForMedic = Invalid;
						}
					}

					break;
				}

			case ET_MSG_REINFORCETIME:
				{
					OB_GETMSG( ET_ReinforceTime );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client )
						{
							if( pEnt->client->sess.sessionTeam == TEAM_AXIS )
							{
								pMsg->m_ReinforceTime = g_redlimbotime.integer -
								                        ( ( level.dwRedReinfOffset + level.timeCurrent - level.startTime ) %
								                          g_redlimbotime.integer );
							}
							else if( pEnt->client->sess.sessionTeam == TEAM_ALLIES )
							{
								pMsg->m_ReinforceTime = g_bluelimbotime.integer -
								                        ( ( level.dwBlueReinfOffset + level.timeCurrent - level.startTime ) %
								                          g_bluelimbotime.integer );
							}
						}
					}

					break;
				}

			case ET_MSG_GETGUNHEALTH:
				{
					OB_GETMSG( ET_MG42Health );

					if( pMsg )
					{
						gentity_t *pGunEntity = EntityFromHandle( pMsg->m_MG42Entity );

						if( pGunEntity && pGunEntity->inuse && pGunEntity->r.linked &&
						    pGunEntity->entstate == STATE_DEFAULT )
						{
							if( pGunEntity->mg42BaseEnt != -1 )
							{
								pMsg->m_Health = g_entities[ pGunEntity->mg42BaseEnt ].health;
							}
							else
							{
								pMsg->m_Health = pGunEntity->health;
							}
						}
						else
						{
							pMsg->m_Health = -1;
						}
					}

					break;
				}

			case ET_MSG_GETGUNHEAT:
				{
					OB_GETMSG( ET_WeaponHeatLevel );

					if( pMsg )
					{
						gentity_t *pGunEntity = EntityFromHandle( pMsg->m_Entity );

						if( pEnt && pEnt->client && pGunEntity )
						{
							pMsg->m_Current = pEnt->client->ps.weapHeat[ WP_DUMMY_MG42 ];
							pMsg->m_Max = ( int ) MAX_MG42_HEAT;
						}
						else
						{
							pMsg->m_Current = -1;
							pMsg->m_Max = -1;
						}
					}

					break;
				}

			case ET_MSG_ISGUNMOUNTED:
				{
					OB_GETMSG( ET_MG42MountedPlayer );

					if( pMsg )
					{
						gentity_t *pGunEntity = EntityFromHandle( pMsg->m_MG42Entity );

						if( pGunEntity && pGunEntity->inuse && ( pGunEntity->r.ownerNum < level.maxclients ) )
						{
							pMsg->m_MountedEntity = HandleFromEntity( &g_entities[ pGunEntity->r.ownerNum ] );
						}
						else
						{
							pMsg->m_MountedEntity.Reset();
						}
					}

					break;
				}

			case ET_MSG_ISGUNREPAIRABLE:
				{
					OB_GETMSG( ET_MG42MountedRepairable );

					if( pMsg )
					{
						gentity_t *pGunEntity = EntityFromHandle( pMsg->m_MG42Entity );

						if( pEnt && pGunEntity && pGunEntity->inuse )
						{
							pMsg->m_Repairable = G_EmplacedGunIsRepairable( pGunEntity, pEnt ) == qtrue ? True : False;
						}
						else
						{
							pMsg->m_Repairable = False;
						}
					}

					break;
				}

			case ET_MSG_MOUNTEDMG42INFO:
				{
					OB_GETMSG( ET_MG42Info );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client && BG_PlayerMounted( pEnt->client->ps.eFlags ) )
						{
							AngleVectors( pEnt->client->pmext.centerangles, pMsg->m_CenterFacing, NULL, NULL );

							//AngleVectors(pEnt->client->pmext.mountedWeaponAngles, pMsg->, NULL, NULL);
							if( pEnt->client->ps.eFlags & EF_MOUNTEDTANK )
							{
								// seems tanks have complete horizonal movement, and fixed vertical
								pMsg->m_MinHorizontalArc = -360;
								pMsg->m_MaxHorizontalArc = -360;
								pMsg->m_MinVerticalArc = 14;
								pMsg->m_MaxVerticalArc = 50;
							}
							else
							{
								pMsg->m_MinHorizontalArc = -pEnt->client->pmext.harc;
								pMsg->m_MaxHorizontalArc = pEnt->client->pmext.harc;
								pMsg->m_MinVerticalArc = -pEnt->client->pmext.varc;
								pMsg->m_MaxVerticalArc = pEnt->client->pmext.varc;
							}
						}
					}

					break;
				}

			case ET_MSG_WPOVERHEATED:
				{
					OB_GETMSG( ET_WeaponOverheated );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client )
						{
							if( pMsg->m_Weapon != ET_WP_MOUNTABLE_MG42 )
							{
								int iCurHeat = pEnt->client->ps.weapHeat[ _weaponBotToGame( pMsg->m_Weapon ) ];
#ifdef NOQUARTER
								int iMaxHeat = GetWeaponTableData( _weaponBotToGame( pMsg->m_Weapon ) )->maxHeat;
#else
								int iMaxHeat = GetAmmoTableData( _weaponBotToGame( pMsg->m_Weapon ) )->maxHeat;
#endif
								pMsg->m_IsOverheated = iMaxHeat ? ( ( iCurHeat >= iMaxHeat ) ? True : False ) : False;
							}
							else
							{
								pMsg->m_IsOverheated = ( pEnt->s.eFlags & EF_OVERHEATING ) ? True : False;
							}
						}
					}

					break;
				}

			case ET_MSG_PICKWEAPON:
			case ET_MSG_PICKWEAPON2:
				{
					OB_GETMSG( ET_SelectWeapon );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client )
						{
							if( pMsg->m_Selection == ET_WP_GPG40 )
							{
								pMsg->m_Selection = ET_WP_KAR98;
							}
							else if( pMsg->m_Selection == ET_WP_M7 )
							{
								pMsg->m_Selection = ET_WP_CARBINE;
							}

							if( _data.GetMessageId() == ET_MSG_PICKWEAPON2 )
							{
								pEnt->client->sess.playerWeapon2 = _weaponBotToGame( pMsg->m_Selection );
								pEnt->client->sess.latchPlayerWeapon2 = _weaponBotToGame( pMsg->m_Selection );
							}
							else
							{
								//CS: /kill 2 seconds before next spawn
								if( !( pEnt->client->ps.pm_flags & PMF_LIMBO ) && pEnt->client->sess.playerWeapon != _weaponBotToGame( pMsg->m_Selection ) )
								{
									pEnt->client->sess.botSuicide = qtrue;
								}

#ifdef NOQUARTER

								// dupe weapons now have same id for NQ
								if( pEnt->client->sess.sessionTeam == TEAM_ALLIES && pMsg->m_Selection == ET_WP_MOBILE_MG42 )
								{
									pEnt->client->sess.playerWeapon = WP_MOBILE_BROWNING;
									pEnt->client->sess.latchPlayerWeapon = WP_MOBILE_BROWNING;
								}
								else if( pEnt->client->sess.sessionTeam == TEAM_AXIS && pMsg->m_Selection == ET_WP_MORTAR )
								{
									pEnt->client->sess.playerWeapon = WP_MORTAR2;
									pEnt->client->sess.latchPlayerWeapon = WP_MORTAR2;
								}
								else
								{
									pEnt->client->sess.playerWeapon = _weaponBotToGame( pMsg->m_Selection );
									pEnt->client->sess.latchPlayerWeapon = _weaponBotToGame( pMsg->m_Selection );
								}

#else // !NOQUARTER
								pEnt->client->sess.playerWeapon = _weaponBotToGame( pMsg->m_Selection );
								pEnt->client->sess.latchPlayerWeapon = _weaponBotToGame( pMsg->m_Selection );
#endif
							}

							pMsg->m_Good = True;
						}
						else
						{
							pMsg->m_Good = False;
						}
					}

					break;
				}

			case ET_MSG_GETHINT:
				{
					OB_GETMSG( ET_CursorHint );

					if( pMsg )
					{
						if( pEnt && pEnt->client )
						{
							pMsg->m_Type = Bot_HintGameToBot( pEnt );
							pMsg->m_Value = pEnt->client->ps.serverCursorHintVal;
						}
					}

					break;
				}

			case ET_MSG_CHANGESPAWNPOINT:
				{
					OB_GETMSG( ET_SpawnPoint );

					if( pMsg )
					{
						if( pEnt && pEnt->client )
						{
							SetPlayerSpawn( pEnt, pMsg->m_SpawnPoint, qtrue );
						}
					}

					break;
				}

			case ET_MSG_GHASFLAG:
				{
					OB_GETMSG( ET_HasFlag );

					if( pMsg )
					{
						if( pEnt && pEnt->inuse && pEnt->client && ( pEnt->health >= 0 ) )
						{
							if( pEnt->client->ps.powerups[ PW_REDFLAG ] || pEnt->client->ps.powerups[ PW_BLUEFLAG ] )
							{
								pMsg->m_HasFlag = True;
							}
						}
					}

					break;
				}

			case ET_MSG_GCONSTRUCTABLE:
				{
					OB_GETMSG( ET_ConstructionState );

					if( pMsg )
					{
						gentity_t *pConstructable = EntityFromHandle( pMsg->m_Constructable );

						if( pEnt && pEnt->inuse && pConstructable && pConstructable->inuse )
						{
							if( G_IsConstructible( pEnt->client->sess.sessionTeam, pConstructable ) )
							{
								pMsg->m_State = CONST_UNBUILT;
							}
							else if( ( pConstructable = G_ConstructionForTeam( pConstructable, pEnt->client->sess.sessionTeam ) ) &&
							         G_ConstructionIsFullyBuilt( pConstructable ) )
							{
								pMsg->m_State = CONST_BUILT;
							}
							else
							{
								pMsg->m_State = CONST_INVALID;
							}
						}
						else
						{
							pMsg->m_State = CONST_INVALID;
						}
					}

					break;
				}

			case ET_MSG_GDYNDESTROYABLE:
				{
					OB_GETMSG( ET_Destroyable );

					if( pMsg )
					{
						gentity_t *pDestroyable = EntityFromHandle( pMsg->m_Entity );

						if( pEnt && pEnt->inuse && pDestroyable && pDestroyable->inuse )
						{
							if( pDestroyable->s.eType == ET_OID_TRIGGER )
							{
								pDestroyable = pDestroyable->target_ent;
							}

							// uhh, whoops. oid without a target ent. happens on map urbanterritory.
							if( !pDestroyable )
							{
								pMsg->m_State = CONST_NOTDESTROYABLE;
								//G_Printf("no target ent for oid\n");
								break;
							}

							if( pDestroyable->s.eType == ET_CONSTRUCTIBLE )
							{
								//qboolean d = G_ConstructionIsDestroyable(pDestroyable);

								if( pDestroyable->spawnflags & CONSTRUCTIBLE_INVULNERABLE )
								{
									pMsg->m_State = CONST_INVALID;
								}
								else if( G_ConstructionIsPartlyBuilt( pDestroyable ) && pDestroyable->s.teamNum != pEnt->client->sess.sessionTeam )
								{
									pMsg->m_State = CONST_DESTROYABLE;
								}
								else if( pDestroyable->parent && pDestroyable->parent->s.eType == ET_OID_TRIGGER )
								{
									gentity_t *pCurrent = pDestroyable->parent->chain;

									while( pCurrent )
									{
										if( G_ConstructionIsPartlyBuilt( pCurrent ) &&
										    pCurrent->s.teamNum != pEnt->client->sess.sessionTeam )
										{
											pMsg->m_State = CONST_DESTROYABLE;
											break;
										}

										pCurrent = pCurrent->chain;

										if( pCurrent == pDestroyable->parent->chain )
										{
											break;
										}
									}
								}
								else
								{
									pMsg->m_State = CONST_NOTDESTROYABLE;
								}
							}
							else if( pDestroyable->s.eType == ET_EXPLOSIVE && pDestroyable->parent && pDestroyable->parent->s.eType == ET_OID_TRIGGER &&
							         ( ( ( pDestroyable->parent->spawnflags & ALLIED_OBJECTIVE ) && pEnt->client->sess.sessionTeam == TEAM_AXIS ) ||
							           ( ( pDestroyable->parent->spawnflags & AXIS_OBJECTIVE ) && pEnt->client->sess.sessionTeam == TEAM_ALLIES ) ) )
							{
								if( pDestroyable->health > 0 )
								{
									pMsg->m_State = CONST_DESTROYABLE;
								}
								else
								{
									pMsg->m_State = CONST_NOTDESTROYABLE;
								}
							}
						}
					}

					break;
				}

			case ET_MSG_GEXPLOSIVESTATE:
				{
					OB_GETMSG( ET_ExplosiveState );

					if( pMsg )
					{
						gentity_t *pExplo = EntityFromHandle( pMsg->m_Explosive );

						if( pExplo && pExplo->inuse )
						{
							if( pExplo->s.eType == ET_MISSILE )
							{
								switch( pExplo->s.weapon )
								{
									case WP_DYNAMITE:
									case WP_LANDMINE:
										pMsg->m_State = ( pExplo->s.teamNum < 4 ) ? XPLO_ARMED : XPLO_UNARMED;
										break;

									case WP_SATCHEL:
										pMsg->m_State = ( pExplo->health >= 250 ) ? XPLO_UNARMED : XPLO_ARMED;
										break;
								}
							}
						}
					}

					break;
				}

			case ET_MSG_GCANBEGRABBED:
				{
					OB_GETMSG( ET_CanBeGrabbed );

					if( pMsg )
					{
						//CS: testing flagstate
						//pMsg->m_CanBeGrabbed = True;

						gentity_t *pFlagEnt = EntityFromHandle( pMsg->m_Entity );

						if( pEnt && pEnt->client && pFlagEnt )
						{
							// DUPLICATE ERROR CHECK, so BG_CanItemBeGrabbed doesn't screw up.
							if( pFlagEnt->s.modelindex < 1 || pFlagEnt->s.modelindex >= bg_numItems )
							{
								//Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
								pMsg->m_CanBeGrabbed = Invalid;
							}
							else
							{
								pMsg->m_CanBeGrabbed =
								  BG_CanItemBeGrabbed( &pFlagEnt->s, &pEnt->client->ps,
								                       pEnt->client->sess.skill, pEnt->client->sess.sessionTeam ) ? True : False;

								// When flags aren't dropped, we need to reject some pickup cases
								// This is because the return flag goal uses this function to see if they can
								// grab/return a flag
								if( pMsg->m_CanBeGrabbed == True && !( pFlagEnt->flags & FL_DROPPED_ITEM ) )
								{
									if( ( pEnt->client->sess.sessionTeam == TEAM_AXIS ) &&
									    !Q_stricmp( pFlagEnt->classname, "team_CTF_redflag" ) )
									{
										GameEntity owner = GetEntityOwner( pMsg->m_Entity );

										if( !owner.IsValid() )
										{
											pMsg->m_CanBeGrabbed = False;
										}
									}
									else if( ( pEnt->client->sess.sessionTeam == TEAM_ALLIES ) &&
									         !Q_stricmp( pFlagEnt->classname, "team_CTF_blueflag" ) )
									{
										GameEntity owner = GetEntityOwner( pMsg->m_Entity );

										if( !owner.IsValid() )
										{
											pMsg->m_CanBeGrabbed = False;
										}
									}
								}
							}
						}
					}

					break;
				}

			case ET_MSG_GNUMTEAMMINES:
				{
					OB_GETMSG( ET_TeamMines );

					if( pMsg )
					{
						if( pEnt && pEnt->client )
						{
							pMsg->m_Current = G_CountTeamLandmines( pEnt->client->sess.sessionTeam );
#ifdef NOQUARTER
							pMsg->m_Max = team_maxLandmines.integer;
#elif defined ETPUB_VERSION
							pMsg->m_Max = g_maxTeamLandmines.integer;
#else
							pMsg->m_Max = MAX_TEAM_LANDMINES;
#endif
						}
					}

					break;
				}

			case ET_MSG_CABINETDATA:
				{
					OB_GETMSG( ET_CabinetData );

					if( pMsg )
					{
						if( pEnt && ( pEnt->s.eType == ET_HEALER || pEnt->s.eType == ET_SUPPLIER ) )
						{
							pMsg->m_Rate = pEnt->damage;
							pMsg->m_CurrentAmount = pEnt->health;
							pMsg->m_MaxAmount = pEnt->count;
						}
						else
						{
							return InvalidEntity;
						}
					}

					break;
				}

			case ET_MSG_SKILLLEVEL:
				{
					if( pEnt && pEnt->client )
					{
						OB_GETMSG( ET_PlayerSkills );
						pMsg->m_Skill[ ET_SKILL_BATTLE_SENSE ] = pEnt->client->sess.skill[ SK_BATTLE_SENSE ];
						pMsg->m_Skill[ ET_SKILL_ENGINEERING ] = pEnt->client->sess.skill[ SK_EXPLOSIVES_AND_CONSTRUCTION ];
						pMsg->m_Skill[ ET_SKILL_FIRST_AID ] = pEnt->client->sess.skill[ SK_FIRST_AID ];
						pMsg->m_Skill[ ET_SKILL_SIGNALS ] = pEnt->client->sess.skill[ SK_SIGNALS ];
						pMsg->m_Skill[ ET_SKILL_LIGHT_WEAPONS ] = pEnt->client->sess.skill[ SK_LIGHT_WEAPONS ];
						pMsg->m_Skill[ ET_SKILL_HEAVY_WEAPONS ] = pEnt->client->sess.skill[ SK_HEAVY_WEAPONS ];
						pMsg->m_Skill[ ET_SKILL_COVERTOPS ] = pEnt->client->sess.skill[ SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS ];
					}
					else
					{
						return InvalidEntity;
					}

					break;
				}

			case ET_MSG_FIRETEAM_CREATE:
				{
					if( pEnt && pEnt->client )
					{
						trap_EA_Command( pEnt - g_entities, ( char * ) "fireteam create" );
					}

					break;
				}

			case ET_MSG_FIRETEAM_DISBAND:
				{
					if( pEnt && pEnt->client )
					{
						trap_EA_Command( pEnt - g_entities, ( char * ) "fireteam disband" );
					}

					break;
				}

			case ET_MSG_FIRETEAM_LEAVE:
				{
					if( pEnt && pEnt->client )
					{
						trap_EA_Command( pEnt - g_entities, ( char * ) "fireteam leave" );
					}

					break;
				}

			case ET_MSG_FIRETEAM_APPLY:
				{
					if( pEnt && pEnt->client )
					{
						OB_GETMSG( ET_FireTeamApply );
						trap_EA_Command( pEnt - g_entities, va( "fireteam apply %i", pMsg->m_FireTeamNum ) );
					}

					break;
				}

			case ET_MSG_FIRETEAM_INVITE:
				{
					if( pEnt && pEnt->client )
					{
						OB_GETMSG( ET_FireTeam );
						gentity_t *targ = EntityFromHandle( pMsg->m_Target );

						if( targ )
						{
							trap_EA_Command( pEnt - g_entities, va( "fireteam invite %i", ( targ - g_entities ) + 1 ) );
						}
					}

					break;
				}

			case ET_MSG_FIRETEAM_WARN:
				{
					if( pEnt && pEnt->client )
					{
						OB_GETMSG( ET_FireTeam );
						gentity_t *targ = EntityFromHandle( pMsg->m_Target );

						if( targ )
						{
							trap_EA_Command( pEnt - g_entities, va( "fireteam warn %i", ( targ - g_entities ) + 1 ) );
						}
					}

					break;
				}

			case ET_MSG_FIRETEAM_KICK:
				{
					if( pEnt && pEnt->client )
					{
						OB_GETMSG( ET_FireTeam );
						gentity_t *targ = EntityFromHandle( pMsg->m_Target );

						if( targ )
						{
							trap_EA_Command( pEnt - g_entities, va( "fireteam kick %i", ( targ - g_entities ) + 1 ) );
						}
					}

					break;
				}

			case ET_MSG_FIRETEAM_PROPOSE:
				{
					if( pEnt && pEnt->client )
					{
						OB_GETMSG( ET_FireTeam );
						gentity_t *targ = EntityFromHandle( pMsg->m_Target );

						if( targ )
						{
							trap_EA_Command( pEnt - g_entities, va( "fireteam propose %i", ( targ - g_entities ) + 1 ) );
						}
					}

					break;
				}

			case ET_MSG_FIRETEAM_INFO:
				{
					if( pEnt && pEnt->client )
					{
						OB_GETMSG( ET_FireTeamInfo );
						fireteamData_t *ft = 0;

						if( G_IsOnFireteam( pEnt - g_entities, &ft ) )
						{
							pMsg->m_FireTeamNum = ft->ident;
							pMsg->m_Leader = HandleFromEntity( &g_entities[( int ) ft->joinOrder[ 0 ] ] );
							pMsg->m_InFireTeam = True;

							int mbrnum = 0;

							for( int i = 0; i < g_maxclients.integer; ++i )
							{
								pMsg->m_Members[ mbrnum ].Reset();

								if( ft->joinOrder[ i ] != -1 )
								{
									GameEntity ge = HandleFromEntity( &g_entities[( int ) ft->joinOrder[ i ] ] );

									if( ge.IsValid() )
									{
										pMsg->m_Members[ mbrnum++ ] = ge;
									}
								}
							}
						}
					}

					break;
				}

			case ET_MSG_GETGAMETYPE:
				{
					OB_GETMSG( ET_GameType );

					if( pMsg )
					{
						pMsg->m_GameType = g_gametype.integer;
					}

					break;
				}

			case ET_MSG_SETCVAR:
				{
					OB_GETMSG( ET_CvarSet );

					if( pMsg )
					{
						trap_Cvar_Set( pMsg->m_Cvar, pMsg->m_Value );
					}

					break;
				}

			case ET_MSG_GETCVAR:
				{
					OB_GETMSG( ET_CvarGet );

					if( pMsg )
					{
						pMsg->m_Value =
						  trap_Cvar_VariableIntegerValue( pMsg->m_Cvar );
					}

					break;
				}

			case ET_MSG_DISABLEBOTPUSH:
				{
					OB_GETMSG( ET_DisableBotPush );

					if( pMsg )
					{
						if( pEnt && pEnt->client )
						{
							if( pMsg->m_Push > 0 )
							{
								pEnt->client->sess.botPush = qfalse;
							}
							else
							{
								pEnt->client->sess.botPush = qtrue;
							}
						}
					}

					break;
				}

			default:
				{
					return UnknownMessageType;
				}
		}

		return Success;
	}

	void PrintError( const char *_error )
	{
		if( _error )
		{
			G_Printf( "%s%s\n", S_COLOR_RED, _error );
		}
	}

	void PrintMessage( const char *_msg )
	{
		if( _msg )
		{
			// et console doesn't support tabs, so
			const int BufferSize = 1024;
			char tmpbuffer[ BufferSize ] = {};
			const char *src = _msg;
			char *dest = tmpbuffer;

			while( *src != 0 )
			{
				if( *src == '\t' )
				{
					for( int i = 0; i < 4; ++i )
					{
						*dest++ = ' ';
					}

					src++;
				}
				else
				{
					*dest++ = *src++;
				}
			}

			G_Printf( "%s%s\n", S_COLOR_GREEN, tmpbuffer );
		}
	}

//bool PrintScreenText(const float _pos[3], float _duration, const obColor &_color, const char *_msg)
//{
//	if(_msg && (g_LastScreenMessageTime != level.time))
//	{
//		trap_SendServerCommand(-1, va("cp \"%s\"", _msg));
//		g_LastScreenMessageTime = level.time;
//	}

//	//gentity_t *pEnt = &level.gentities[0];

//	//// for dedicated servers we quit early because there can't possibly be a client 0 that is local client
//	//if(g_dedicated.integer)
//	// return;

//	//if(pEnt && pEnt->inuse && pEnt->client && pEnt->client->pers.localClient && !IsBot(pEnt) && _msg)
//	//{
//	// if(_pos)
//	// {
//	//   trap_SendServerCommand(0, va("wp \"%s\" %f %f %f %f", _msg, _pos[0], _pos[1], _pos[2], _duration));
//	// }
//	// else
//	// {
//	//   if(g_LastScreenMessageTime != level.time)
//	//   {
//	//     trap_SendServerCommand(0, va("cp \"%s\"", _msg));
//	//     g_LastScreenMessageTime = level.time;
//	//   }
//	// }
//	//}
//}

	const char *GetMapName()
	{
		return level.rawmapname;
	}

	void GetMapExtents( AABB &_aabb )
	{
		if( level.mapcoordsValid )
		{
			_aabb.m_Mins[ 0 ] = level.mapcoordsMins[ 0 ] /** 2.F*/;
			_aabb.m_Mins[ 1 ] = level.mapcoordsMins[ 1 ] /** 2.F*/;
			_aabb.m_Mins[ 2 ] = -65535.0f;
			_aabb.m_Maxs[ 0 ] = level.mapcoordsMaxs[ 0 ] /** 2.F*/;
			_aabb.m_Maxs[ 1 ] = level.mapcoordsMaxs[ 1 ] /** 2.F*/;
			_aabb.m_Maxs[ 2 ] = 65535.0f;

			for( int i = 0; i < 3; ++i )
			{
				if( _aabb.m_Mins[ i ] > _aabb.m_Maxs[ i ] )
				{
					float t = _aabb.m_Mins[ i ];
					_aabb.m_Mins[ i ] = _aabb.m_Maxs[ i ];
					_aabb.m_Maxs[ i ] = t;
				}
			}
		}
		else
		{
			memset( &_aabb, 0, sizeof( AABB ) );
		}
	}

	GameEntity EntityByName( const char *_name )
	{
		gentity_t *pEnt = G_FindByTargetname( NULL, _name );
		return HandleFromEntity( pEnt );
	}

	GameEntity EntityFromID( const int _gameId )
	{
		gentity_t *pEnt = INDEXENT( _gameId );
		return pEnt ? HandleFromEntity( pEnt ) : GameEntity();
	}

	int IDFromEntity( const GameEntity _ent )
	{
		gentity_t *pEnt = EntityFromHandle( _ent );

		if( pEnt )
		{
			gentity_t *pStart = g_entities;
			int iIndex = pEnt - pStart;
			assert( iIndex >= 0 );
			return ( iIndex < MAX_GENTITIES ) ? iIndex : -1;
		}

		return -1;
	}

	bool DoesEntityStillExist( const GameEntity &_hndl )
	{
		return _hndl.IsValid() ? EntityFromHandle( _hndl ) != NULL : false;
	}

	int GetAutoNavFeatures( AutoNavFeature *_feature, int _max )
	{
		int iNumFeatures = 0;

		for( int i = MAX_CLIENTS; i < level.num_entities; ++i )
		{
			gentity_t *e = &g_entities[ i ];

			if( !e->inuse )
			{
				continue;
			}

			////////////////////////////////////////////////////////////////////////
			_feature[ iNumFeatures ].m_Type = 0;
			_feature[ iNumFeatures ].m_TravelTime = 0;
			_feature[ iNumFeatures ].m_ObstacleEntity = false;

			for( int i = 0; i < 3; ++i )
			{
				_feature[ iNumFeatures ].m_Position[ i ] = e->r.currentOrigin[ i ];
				_feature[ iNumFeatures ].m_TargetPosition[ i ] = e->r.currentOrigin[ i ];
				_feature[ iNumFeatures ].m_Bounds.m_Mins[ 0 ] = 0.f;
				_feature[ iNumFeatures ].m_Bounds.m_Maxs[ 0 ] = 0.f;
				AngleVectors( e->s.angles, _feature[ iNumFeatures ].m_Facing, NULL, NULL );
			}

			_feature[ iNumFeatures ].m_Bounds.m_Mins[ 0 ] = e->r.absmin[ 0 ];
			_feature[ iNumFeatures ].m_Bounds.m_Mins[ 1 ] = e->r.absmin[ 1 ];
			_feature[ iNumFeatures ].m_Bounds.m_Mins[ 2 ] = e->r.absmin[ 2 ];
			_feature[ iNumFeatures ].m_Bounds.m_Maxs[ 0 ] = e->r.absmax[ 0 ];
			_feature[ iNumFeatures ].m_Bounds.m_Maxs[ 1 ] = e->r.absmax[ 1 ];
			_feature[ iNumFeatures ].m_Bounds.m_Maxs[ 2 ] = e->r.absmax[ 2 ];

			//////////////////////////////////////////////////////////////////////////
			if( e->classname )
			{
				if( !Q_stricmp( e->classname, "team_CTF_redspawn" ) )
				{
					_feature[ iNumFeatures ].m_Type = ENT_CLASS_GENERIC_PLAYERSTART_TEAM1;
				}
				else if( !Q_stricmp( e->classname, "team_CTF_bluespawn" ) )
				{
					_feature[ iNumFeatures ].m_Type = ENT_CLASS_GENERIC_PLAYERSTART_TEAM2;
				}
				else if( !Q_stricmp( e->classname, "info_player_deathmatch" ) ||
				         !Q_stricmp( e->classname, "info_player_spawn" ) )
				{
					_feature[ iNumFeatures ].m_Type = ENT_CLASS_GENERIC_PLAYERSTART;
				}
				else if( !Q_stricmp( e->classname, "target_teleporter" ) )
				{
					_feature[ iNumFeatures ].m_Type = ENT_CLASS_GENERIC_TELEPORTER;
					gentity_t *pTarget = G_PickTarget( e->target );

					if( pTarget )
					{
						_feature[ iNumFeatures ].m_TargetPosition[ 0 ] = pTarget->r.currentOrigin[ 0 ];
						_feature[ iNumFeatures ].m_TargetPosition[ 1 ] = pTarget->r.currentOrigin[ 1 ];
						_feature[ iNumFeatures ].m_TargetPosition[ 2 ] = pTarget->r.currentOrigin[ 2 ];
					}
				}
				else if( !Q_stricmp( e->classname, "team_CTF_redflag" ) )
				{
					_feature[ iNumFeatures ].m_Type = ENT_CLASS_GENERIC_FLAG;
				}
				else if( !Q_stricmp( e->classname, "team_CTF_blueflag" ) )
				{
					_feature[ iNumFeatures ].m_Type = ENT_CLASS_GENERIC_FLAG;
				}
				else if( !Q_stricmp( e->classname, "misc_mg42" ) || !Q_stricmp( e->classname, "misc_mg42base" ) )
				{
					_feature[ iNumFeatures ].m_Type = ET_CLASSEX_MG42MOUNT;
					_feature[ iNumFeatures ].m_ObstacleEntity = true;
				}
				else if( !Q_stricmp( e->classname, "misc_cabinet_health" ) )
				{
					_feature[ iNumFeatures ].m_Type = ET_CLASSEX_HEALTHCABINET;
					_feature[ iNumFeatures ].m_ObstacleEntity = true;
				}
				else if( !Q_stricmp( e->classname, "misc_cabinet_supply" ) )
				{
					_feature[ iNumFeatures ].m_Type = ET_CLASSEX_AMMOCABINET;
					_feature[ iNumFeatures ].m_ObstacleEntity = true;
				}
			}

			if( _feature[ iNumFeatures ].m_Type != 0 )
			{
				iNumFeatures++;
			}
		}

		return iNumFeatures;
	}

	const char *GetGameName()
	{
		return GAME_VERSION;
	}

	const char *GetModName()
	{
		return OMNIBOT_MODNAME;
	}

	const char *GetModVers()
	{
		return OMNIBOT_MODVERSION;
	}

	const char *GetBotPath()
	{
		//return Omnibot_GetLibraryPath();

		char            basepath[ MAX_OSPATH ];
		static char botpath[ MAX_OSPATH ];

		trap_Cvar_VariableStringBuffer( "fs_basepath", basepath, sizeof( basepath ) );

		Com_sprintf( botpath, sizeof( botpath ), "%s\\omni-bot", basepath );

		return Omnibot_FixPath( botpath );
	}

	const char *GetLogPath()
	{
		static char logpath[ 512 ];
		trap_Cvar_VariableStringBuffer( "fs_homepath", logpath, sizeof( logpath ) );
		return Omnibot_FixPath( logpath );
	}
};

void Bot_Interface_InitHandles()
{
	for( int i = 0; i < MAX_GENTITIES; ++i )
	{
		m_EntityHandles[ i ].m_HandleSerial = 1;
		m_EntityHandles[ i ].m_NewEntity = false;
		m_EntityHandles[ i ].m_Used = false;
	}
}

int Bot_Interface_Init()
{
	if( g_OmniBotEnable.integer == 0 )
	{
		G_Printf( "%s%s\n", S_COLOR_GREEN,
		          "Omni-bot is currently disabled with \"omnibot_enable 0\"" );
		return 1;
	}

#ifdef _DEBUG
	trap_Cvar_Set( "sv_cheats", "1" );
	trap_Cvar_Update( &g_cheats );
#endif

	g_GoalSubmitReady = false;

	g_InterfaceFunctions = new ETInterface;
	eomnibot_error err = Omnibot_LoadLibrary( ET_VERSION_LATEST,
	                     "omnibot_et", Omnibot_FixPath( g_OmniBotPath.string ) );

	if( err == BOT_ERROR_NONE )
	{
		return true;
	}

	return false;
}

int Bot_Interface_Shutdown()
{
	if( IsOmnibotLoaded() )
	{
		g_BotFunctions.pfnShutdown();
	}

	Omnibot_FreeLibrary();
	return 1;
}

//////////////////////////////////////////////////////////////////////////

void Bot_Interface_ConsoleCommand()
{
	enum { BuffSize = 32 };

	char buffer[ BuffSize ] = {};
	trap_Argv( 1, buffer, BuffSize );

	if( IsOmnibotLoaded() )
	{
		if( !Q_stricmp( buffer, "unload" ) )
		{
			Bot_Interface_Shutdown();
			return;
		}
		else if( !Q_stricmp( buffer, "reload" ) )
		{
			Bot_Interface_Shutdown();
			Bot_Interface_InitHandles();
			Bot_Interface_Init();
			return;
		}

		Arguments args;

		for( int i = 0; i < trap_Argc(); ++i )
		{
			trap_Argv( i, args.m_Args[ args.m_NumArgs++ ], Arguments::MaxArgLength );
		}

		g_BotFunctions.pfnConsoleCommand( args );
	}
	else
	{
		if( !Q_stricmp( buffer, "load" ) )
		{
			Bot_Interface_InitHandles();
			Bot_Interface_Init();
			return;
		}
		else
		{
			G_Printf( "%s%s\n", S_COLOR_RED, "Omni-bot not loaded." );
		}
	}
}

extern "C" void script_mover_spawn( gentity_t *ent );

void Bot_Interface_Update()
{
	if( IsOmnibotLoaded() )
	{
		char buf[ 1024 ] = { 0 };

//#ifdef _DEBUG
//		trap_Cvar_Set( "sv_cheats", "1" );
//		trap_Cvar_Update(&g_cheats);
//#endif

		/*if (level.framenum == GAME_INIT_FRAMES)
		Bot_Event_StartGame();*/

		//////////////////////////////////////////////////////////////////////////
		{
			// time triggers for Omni-bot
#ifdef NOQUARTER
			if( level.framenum % sv_fps.integer == 0 )
#else
			if( level.framenum % 20 == 0 )  //@sv_fps
#endif
			{
				if( !level.twoMinute && ( g_timelimit.value * 60000 - ( level.time - level.startTime ) ) < 120000 )
				{
					level.twoMinute = qtrue;
					Bot_Util_SendTrigger( NULL, NULL, "two minute warning.", "twominute" );
				}

				if( !level.thirtySecond && ( g_timelimit.value * 60000 - ( level.time - level.startTime ) ) < 30000 )
				{
					level.thirtySecond = qtrue;
					Bot_Util_SendTrigger( NULL, NULL, "thirty second warning.", "thirtysecond" );
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////
		{
			static float serverGravity = 0.0f;

			if( serverGravity != g_gravity.value )
			{
				Event_SystemGravity d = { -g_gravity.value };
				g_BotFunctions.pfnSendGlobalEvent( MessageHelper( GAME_GRAVITY, &d, sizeof( d ) ) );
				serverGravity = g_gravity.value;
			}

			static int cheatsEnabled = 0;

			if( g_cheats.integer != cheatsEnabled )
			{
				Event_SystemCheats d = { g_cheats.integer ? True : False };
				g_BotFunctions.pfnSendGlobalEvent( MessageHelper( GAME_CHEATS, &d, sizeof( d ) ) );
				cheatsEnabled = g_cheats.integer;
			}
		}

		int iNumBots = 0;

		for( int i = 0; i < g_maxclients.integer; ++i )
		{
			if( !g_entities[ i ].inuse )
			{
				continue;
			}

			if( !g_entities[ i ].client )
			{
				continue;
			}

			if( g_entities[ i ].client->pers.connected != CON_CONNECTED )
			{
				continue;
			}

			/*if(i==1)
			g_entities[i].flags |= FL_GODMODE;*/

			// Send a spectated message to bots that are being spectated.
			if( ( g_entities[ i ].client->sess.sessionTeam == TEAM_SPECTATOR ) &&
			    ( g_entities[ i ].client->sess.spectatorState == SPECTATOR_FOLLOW ) )
			{
				int iDestination = g_entities[ i ].client->sess.spectatorClient;
				Bot_Event_Spectated( iDestination, i );
			}

			// fake handle server commands (to prevent server command overflow)
			if( ( g_entities[ i ].inuse == qtrue ) && IsBot( &g_entities[ i ] ) )
			{
				++iNumBots;

				while( trap_BotGetServerCommand( i, buf, sizeof( buf ) ) )
				{
				}
			}
		}

		if( !( g_OmniBotFlags.integer & OBF_DONT_SHOW_BOTCOUNT ) )
		{
			if( g_OmniBotPlaying.integer != iNumBots )
			{
				g_OmniBotPlaying.integer = iNumBots;
				trap_Cvar_Set( "omnibot_playing", va( "%i", iNumBots ) );
			}
		}
		else
		{
			if( g_OmniBotPlaying.integer != -1 )
			{
				g_OmniBotPlaying.integer = -1;
				trap_Cvar_Set( "omnibot_playing", "-1" );
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Register any pending entity updates.
		for( int i = 0; i < MAX_GENTITIES; ++i )
		{
			if( m_EntityHandles[ i ].m_NewEntity && g_entities[ i ].inuse )
			{
				if( g_entities[ i ].think != script_mover_spawn )
				{
					m_EntityHandles[ i ].m_NewEntity = false;
					Bot_Event_EntityCreated( &g_entities[ i ] );
				}
			}
		}

		SendDeferredGoals();
		//////////////////////////////////////////////////////////////////////////
		// Call the libraries update.
		g_BotFunctions.pfnUpdate();
		//////////////////////////////////////////////////////////////////////////
	}
}

//////////////////////////////////////////////////////////////////////////

qboolean Bot_Util_AllowPush( int weaponId )
{
	switch( weaponId )
	{
		case WP_MORTAR_SET:
#ifdef NOQUARTER
		case WP_MORTAR2_SET:
		case WP_MOBILE_BROWNING_SET:
		case WP_BAR_SET:
#endif
		case WP_MOBILE_MG42_SET:
			return qfalse;
	}

	return qtrue;
}

//////////////////////////////////////////////////////////////////////////
const char *_GetEntityName( gentity_t *_ent )
{
	// For goal names.
	//if(_ent)
	//{
	//  if(_ent->scriptName)
	//    return _ent->scriptName;
	//  else if(_ent->targetname)
	//    return _ent->targetname;
	//  else if(_ent->message)
	//    return _ent->message;
	//}
	static char newentname[ 256 ];
	char *name;

	newentname[ 0 ] = '\0';

	if( _ent )
	{
		if( _ent->inuse && _ent->client )
		{
			if( _ent->client->pers.netname[ 0 ] )
			{
				return _ent->client->pers.netname;
			}
			else
			{
				static char     userinfo[ MAX_INFO_STRING ] = { 0 };
				trap_GetUserinfo( _ent - g_entities, userinfo, sizeof( userinfo ) );
				return Info_ValueForKey( userinfo, "name" );
			}
		}

		if( _ent->track )
		{
			strcpy( newentname, _ent->track );
		}
		else if( _ent->scriptName )
		{
			strcpy( newentname, _ent->scriptName );
		}
		else if( _ent->targetname )
		{
			strcpy( newentname, _ent->targetname );
		}
		else if( _ent->message )
		{
			strcpy( newentname, _ent->message );
		}

		name = newentname;

		Q_CleanStr( name );

		if( name )
		{
			char undschar[] = { '-', ( char ) NULL };
			char skipchar[] = { '[', ']', '#', '!', '*', '`',
			                    '^', '&', '<', '>', '+', '=', '|',  '\'', '%',
			                    '.', ':', '/', '(', ')', ( char ) NULL
			                  };
			char *curchar = NULL;
			char *tmp = NULL;
			char *tmpdst = NULL;
			tmp = name;
			tmpdst = name;

			while( *tmp )
			{
				curchar = undschar;

				while( *curchar )
				{
					if( *tmp == *curchar )
					{
						*tmp = '_';
						break;
					}

					curchar++;
				}

				curchar = skipchar;

				while( *curchar )
				{
					if( *tmp == *curchar )
					{
						tmp++;
						break;
					}

					curchar++;
				}

				*tmpdst = *tmp;
				tmp++;
				tmpdst++;
			}

			*tmpdst = '\0';

			if( !Q_stricmpn( "the ", name, 4 ) )
			{
				return name + 4;
			}

			return name;
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
qboolean Bot_Util_CheckForSuicide( gentity_t *ent )
{
	if( ent && ent->client )
	{
		// Omni-bot: used for class changes, bot will /kill 2 seconds before spawn
		if( ent->client->sess.botSuicide == qtrue )
		{
			if( ent->client->sess.sessionTeam == TEAM_AXIS )
			{
				if( ( g_redlimbotime.integer - ( ( level.dwRedReinfOffset + level.timeCurrent - level.startTime ) % g_redlimbotime.integer ) ) < 2000 )
				{
					Cmd_Kill_f( ent );
					ent->client->sess.botSuicide = qfalse;
					return qtrue;
				}
			}
			else if( ent->client->sess.sessionTeam == TEAM_ALLIES )
			{
				if( ( g_bluelimbotime.integer - ( ( level.dwBlueReinfOffset + level.timeCurrent - level.startTime ) % g_bluelimbotime.integer ) ) < 2000 )
				{
					Cmd_Kill_f( ent );
					ent->client->sess.botSuicide = qfalse;
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

//////////////////////////////////////////////////////////////////////////

void Bot_Event_ClientConnected( int _client, qboolean _isbot )
{
	if( IsOmnibotLoaded() )
	{
		Event_SystemClientConnected d;
		d.m_GameId = _client;
		d.m_IsBot = _isbot == qtrue ? True : False;
		g_BotFunctions.pfnSendGlobalEvent( MessageHelper( GAME_CLIENTCONNECTED, &d, sizeof( d ) ) );
	}
}

void Bot_Event_ClientDisConnected( int _client )
{
	if( IsOmnibotLoaded() )
	{
		Event_SystemClientDisConnected d = { _client };
		g_BotFunctions.pfnSendGlobalEvent( MessageHelper( GAME_CLIENTDISCONNECTED, &d, sizeof( d ) ) );
	}
}

void Bot_Event_ResetWeapons( int _client )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_RESETWEAPONS ) );
		}
	}
}

void Bot_Event_AddWeapon( int _client, int _weaponId )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			//////////////////////////////////////////////////////////////////////////
			int AddWeapon = _weaponId;

			switch( AddWeapon )
			{
				case ET_WP_GARAND:
					{
						if( COM_BitCheck( g_entities[ _client ].client->ps.weapons, WP_GARAND_SCOPE ) )
						{
							// remove the unscoped to give the scoped
							Event_RemoveWeapon d = { ET_WP_GARAND };
							g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_REMOVEWEAPON, &d, sizeof( d ) ) );

							AddWeapon = ET_WP_GARAND_SCOPE;
						}

						break;
					}

				case ET_WP_K43:
					{
						if( COM_BitCheck( g_entities[ _client ].client->ps.weapons, WP_K43_SCOPE ) )
						{
							// remove the unscoped to give the scoped
							Event_RemoveWeapon d = { ET_WP_K43 };
							g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_REMOVEWEAPON, &d, sizeof( d ) ) );

							AddWeapon = ET_WP_K43_SCOPE;
						}

						break;
					}

				case ET_WP_FG42:
					{
						if( COM_BitCheck( g_entities[ _client ].client->ps.weapons, WP_FG42SCOPE ) )
						{
							// remove the unscoped to give the scoped
							Event_RemoveWeapon d = { ET_WP_FG42 };
							g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_REMOVEWEAPON, &d, sizeof( d ) ) );

							AddWeapon = ET_WP_FG42_SCOPE;
						}

						break;
					}

				case ET_WP_GARAND_SCOPE:
					{
						// remove the unscoped
						Event_RemoveWeapon d = { ET_WP_GARAND };
						g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_REMOVEWEAPON, &d, sizeof( d ) ) );

						break;
					}

				case ET_WP_K43_SCOPE:
					{
						// remove the unscoped
						Event_RemoveWeapon d = { ET_WP_K43 };
						g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_REMOVEWEAPON, &d, sizeof( d ) ) );

						break;
					}

				case ET_WP_FG42_SCOPE:
					{
						// remove the unscoped
						Event_RemoveWeapon d = { ET_WP_FG42 };
						g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_REMOVEWEAPON, &d, sizeof( d ) ) );

						break;
					}
			}

			//////////////////////////////////////////////////////////////////////////

			Event_AddWeapon d = { AddWeapon };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_ADDWEAPON, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_RemoveWeapon( int _client, int _weaponId )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_RemoveWeapon d = { _weaponId };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_REMOVEWEAPON, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_TakeDamage( int _client, gentity_t *_ent )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_TakeDamage d = { HandleFromEntity( _ent ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( PERCEPT_FEEL_PAIN, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_Death( int _client, gentity_t *_killer, const char *_meansofdeath )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_Death d;
			d.m_WhoKilledMe = HandleFromEntity( _killer );
			Q_strncpyz( d.m_MeansOfDeath,
			            _meansofdeath ? _meansofdeath : "<unknown>", sizeof( d.m_MeansOfDeath ) );
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_DEATH, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_Healed( int _client, gentity_t *_whodoneit )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_Healed d = { HandleFromEntity( _whodoneit ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_HEALED, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_RecievedAmmo( int _client, gentity_t *_whodoneit )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_Ammo d = { HandleFromEntity( _whodoneit ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_RECIEVEDAMMO, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_Revived( int _client, gentity_t *_whodoneit )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_Revived d = { HandleFromEntity( _whodoneit ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_REVIVED, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_KilledSomeone( int _client, gentity_t *_victim, const char *_meansofdeath )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_KilledSomeone d;
			d.m_WhoIKilled = HandleFromEntity( _victim );
			Q_strncpyz( d.m_MeansOfDeath,
			            _meansofdeath ? _meansofdeath : "<unknown>",
			            sizeof( d.m_MeansOfDeath ) / sizeof( d.m_MeansOfDeath[ 0 ] ) );
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_KILLEDSOMEONE, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_FireWeapon( int _client, int _weaponId, gentity_t *_projectile )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_WeaponFire d = { _weaponId, Primary, HandleFromEntity( _projectile ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ACTION_WEAPON_FIRE, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_PreTriggerMine( int _client, gentity_t *_mine )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_TriggerMine_ET d = { HandleFromEntity( _mine ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_PRETRIGGER_MINE, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_PostTriggerMine( int _client, gentity_t *_mine )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_TriggerMine_ET d = { HandleFromEntity( _mine ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_POSTTRIGGER_MINE, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_MortarImpact( int _client, vec3_t _pos )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_MortarImpact_ET d = { { _pos[ 0 ], _pos[ 1 ], _pos[ 2 ] } };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_MORTAR_IMPACT, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_Spectated( int _client, int _who )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_Spectated d = { _who };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( MESSAGE_SPECTATED, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_ChatMessage( int _to, gentity_t *_source, int _type, const char *_message )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _to ] ) )
		{
			int iMsg = PERCEPT_HEAR_GLOBALCHATMSG;

			switch( _type )
			{
				case SAY_ALL:
					iMsg = PERCEPT_HEAR_GLOBALCHATMSG;
					break;

				case SAY_TEAM:
				case SAY_TEAMNL:
					iMsg = PERCEPT_HEAR_TEAMCHATMSG;
					break;

				case SAY_BUDDY:
					iMsg = PERCEPT_HEAR_PRIVCHATMSG;
					break;
			}

			Event_ChatMessage d;
			d.m_WhoSaidIt = HandleFromEntity( _source );
			Q_strncpyz( d.m_Message, _message ? _message : "<unknown>",
			            sizeof( d.m_Message ) / sizeof( d.m_Message[ 0 ] ) );
			g_BotFunctions.pfnSendEvent( _to, MessageHelper( iMsg, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_VoiceMacro( int _client, gentity_t *_source, int _type, const char *_message )
{
	if( IsOmnibotLoaded() )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			int iMessageId = PERCEPT_HEAR_GLOBALVOICEMACRO;

			if( _type == SAY_TEAM )
			{
				iMessageId = PERCEPT_HEAR_TEAMVOICEMACRO;
			}
			else if( _type == SAY_BUDDY )
			{
				iMessageId = PERCEPT_HEAR_PRIVATEVOICEMACRO;
			}

			Event_VoiceMacro d;
			d.m_WhoSaidIt = HandleFromEntity( _source );
			Q_strncpyz( d.m_MacroString, _message ? _message : "<unknown>",
			            sizeof( d.m_MacroString ) / sizeof( d.m_MacroString[ 0 ] ) );
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( iMessageId, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_Sound( gentity_t *_source, int _sndtype, const char *_name )
{
	if( IsOmnibotLoaded() )
	{
		Event_Sound d = {};
		d.m_Source = HandleFromEntity( _source );
		d.m_SoundType = _sndtype;
		g_InterfaceFunctions->GetEntityPosition( d.m_Source, d.m_Origin );
		Q_strncpyz( d.m_SoundName, _name ? _name : "<unknown>", sizeof( d.m_SoundName ) / sizeof( d.m_SoundName[ 0 ] ) );
		g_BotFunctions.pfnSendGlobalEvent( MessageHelper( GAME_SOUND, &d, sizeof( d ) ) );
	}
}

void Bot_Event_FireTeamCreated( int _client, int _fireteamnum )
{
	if( IsOmnibotLoaded() && IsBot( &g_entities[ _client ] ) )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_FireTeamCreated d = { _fireteamnum };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_FIRETEAM_CREATED, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_FireTeamDestroyed( int _client )
{
	if( IsOmnibotLoaded() && IsBot( &g_entities[ _client ] ) )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_FireTeamDisbanded d;
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_FIRETEAM_DISBANDED, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_JoinedFireTeam( int _client, gentity_t *leader )
{
	if( IsOmnibotLoaded() && IsBot( &g_entities[ _client ] ) )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_FireTeamJoined d = { HandleFromEntity( leader ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_FIRETEAM_JOINED, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_LeftFireTeam( int _client )
{
	if( IsOmnibotLoaded() && IsBot( &g_entities[ _client ] ) )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_FireTeamLeft d;
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_FIRETEAM_LEFT, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_InviteFireTeam( int _inviter, int _invitee )
{
	if( IsOmnibotLoaded() && IsBot( &g_entities[ _invitee ] ) )
	{
		if( IsBot( &g_entities[ _invitee ] ) )
		{
			Event_FireTeamInvited d = { HandleFromEntity( &g_entities[ _inviter ] ) };
			g_BotFunctions.pfnSendEvent( _invitee, MessageHelper( ET_EVENT_FIRETEAM_INVITED, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_FireTeam_Proposal( int _client, int _proposed )
{
	if( IsOmnibotLoaded() && IsBot( &g_entities[ _client ] ) )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_FireTeamProposal d = { HandleFromEntity( &g_entities[ _proposed ] ) };
			g_BotFunctions.pfnSendEvent( _client, MessageHelper( ET_EVENT_FIRETEAM_PROPOSAL, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_FireTeam_Warn( int _client, int _warned )
{
	if( IsOmnibotLoaded() && IsBot( &g_entities[ _client ] ) )
	{
		if( IsBot( &g_entities[ _client ] ) )
		{
			Event_FireTeamWarning d = { HandleFromEntity( &g_entities[ _client ] ) };
			g_BotFunctions.pfnSendEvent( _warned, MessageHelper( ET_EVENT_FIRETEAM_WARNED, &d, sizeof( d ) ) );
		}
	}
}

void Bot_Event_EntityCreated( gentity_t *pEnt )
{
	if( pEnt && IsOmnibotLoaded() )
	{
		// Get common properties.
		const int iEntNum = pEnt - g_entities;
		GameEntity ent = HandleFromEntity( pEnt );
		int iClass = g_InterfaceFunctions->GetEntityClass( ent );

		if( iClass )
		{
			Event_EntityCreated d;
			d.m_Entity = GameEntity( iEntNum, m_EntityHandles[ iEntNum ].m_HandleSerial );

			d.m_EntityClass = iClass;
			g_InterfaceFunctions->GetEntityCategory( ent, d.m_EntityCategory );
			g_BotFunctions.pfnSendGlobalEvent( MessageHelper( GAME_ENTITYCREATED, &d, sizeof( d ) ) );
			m_EntityHandles[ iEntNum ].m_Used = true;
		}

		Bot_Util_CheckForGoalEntity( ent );
	}

	//////////////////////////////////////////////////////////////////////////
	// Cache smoke bombs
	if( ( pEnt->s.eType == ET_MISSILE && pEnt->s.weapon == WP_SMOKE_BOMB ) )
	{
		for( int i = 0; i < MAX_SMOKEGREN_CACHE; ++i )
		{
			if( !g_SmokeGrenadeCache[ i ] )
			{
				g_SmokeGrenadeCache[ i ] = pEnt;
				break;
			}
		}
	}
}

extern "C"
{
	void Bot_Queue_EntityCreated( gentity_t *pEnt )
	{
		if( pEnt )
		{
			m_EntityHandles[ pEnt - g_entities ].m_NewEntity = true;
		}
	}

	void Bot_Event_EntityDeleted( gentity_t *pEnt )
	{
		if( pEnt )
		{
			const int iEntNum = pEnt - g_entities;

			if( IsOmnibotLoaded() )
			{
				Event_EntityDeleted d = { GameEntity( iEntNum, m_EntityHandles[ iEntNum ].m_HandleSerial ) };
				g_BotFunctions.pfnSendGlobalEvent( MessageHelper( GAME_ENTITYDELETED, &d, sizeof( d ) ) );
			}

			m_EntityHandles[ iEntNum ].m_Used = false;
			m_EntityHandles[ iEntNum ].m_NewEntity = false;

			while( ++m_EntityHandles[ iEntNum ].m_HandleSerial == 0 ) {}
		}

		for( int i = 0; i < MAX_SMOKEGREN_CACHE; ++i )
		{
			if( g_SmokeGrenadeCache[ i ] == pEnt )
			{
				g_SmokeGrenadeCache[ i ] = NULL;
			}
		}
	}

//////////////////////////////////////////////////////////////////////////

	void Bot_Util_SendTrigger( gentity_t *_ent, gentity_t *_activator, const char *_tagname, const char *_action )
	{
		if( IsOmnibotLoaded() )
		{
			TriggerInfo triggerInfo;
			triggerInfo.m_Entity = HandleFromEntity( _ent );
			Q_strncpyz( triggerInfo.m_TagName, _tagname, TriggerBufferSize );
			Q_strncpyz( triggerInfo.m_Action, _action, TriggerBufferSize );
			g_BotFunctions.pfnSendTrigger( triggerInfo );
		}
	}

	void Bot_AddDynamiteGoal( gentity_t *_ent, int _team, const char *_tag )
	{
		if( _team == TEAM_AXIS )
		{
			Bot_Util_AddGoal( "defuse", _ent, ( 1 << ET_TEAM_ALLIES ), _tag );
		}
		else
		{
			Bot_Util_AddGoal( "defuse", _ent, ( 1 << ET_TEAM_AXIS ), _tag );
		}
	}

	void Bot_AddFallenTeammateGoals( gentity_t *_teammate, int _team )
	{
		if( _team == TEAM_AXIS )
		{
			Bot_Util_AddGoal( "revive", _teammate, ( 1 << ET_TEAM_AXIS ), _GetEntityName( _teammate ) );
		}
		else if( _team == TEAM_ALLIES )
		{
			Bot_Util_AddGoal( "revive", _teammate, ( 1 << ET_TEAM_ALLIES ), _GetEntityName( _teammate ) );
		}
	}
};
