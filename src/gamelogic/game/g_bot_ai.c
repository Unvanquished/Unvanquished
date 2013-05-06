/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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
#include "g_bot_ai.h"
#include "g_bot_util.h"

/*
=======================
Behavior Tree Creation
=======================
*/

static void BotInitNode( AINode_t type, AINodeRunner func, void *node )
{
	AIGenericNode_t *n = ( AIGenericNode_t * ) node;
	n->type = type;
	n->run = func;
}

#define allocNode( T ) ( T * ) BG_Alloc( sizeof( T ) );
#define stringify( v ) va( #v " %d", v )
#define D( T ) trap_Parse_AddGlobalDefine( stringify( T ) )

// FIXME: copied from parse.c
#define P_LOGIC_AND        5
#define P_LOGIC_OR         6
#define P_LOGIC_GEQ        7
#define P_LOGIC_LEQ        8
#define P_LOGIC_EQ         9
#define P_LOGIC_UNEQ       10

#define P_LOGIC_NOT        36
#define P_LOGIC_GREATER    37
#define P_LOGIC_LESS       38

typedef struct pc_token_list_s
{
	pc_token_t token;
	struct pc_token_list_s *prev;
	struct pc_token_list_s *next;
} pc_token_list;

static pc_token_list *CreateTokenList( int handle )
{
	pc_token_t token;
	char filename[ MAX_QPATH ];
	pc_token_list *current = NULL;
	pc_token_list *root = NULL;

	while ( trap_Parse_ReadToken( handle, &token ) )
	{
		pc_token_list *list = ( pc_token_list * ) BG_Alloc( sizeof( pc_token_list ) );
		
		if ( current )
		{
			list->prev = current;
			current->next = list;
		}
		else
		{
			list->prev = list;
			root = list;
		}
		
		current = list;
		current->next = NULL;

		current->token = token;
		trap_Parse_SourceFileAndLine( handle, filename, &current->token.line );
	}

	return root;
}

static void FreeTokenList( pc_token_list *list )
{
	pc_token_list *current = list;
	while( current )
	{
		pc_token_list *node = current;
		current = current->next;

		BG_Free( node );
	}
}

static AIGenericNode_t *ReadNode( pc_token_list **tokenlist );
static void FreeNode( AIGenericNode_t *node );
static void FreeNodeList( AINodeList_t *node );
static void FreeConditionNode( AIConditionNode_t *node );

static void CheckToken( const char *tokenValueName, const char *nodename, const pc_token_t *token, int requiredType )
{
	if ( token->type != requiredType )
	{
		BotDPrintf( S_COLOR_RED "ERROR: Invalid %s %s after %s on line %d\n", tokenValueName, token->string, nodename, token->line );
	}
}

static AIValue_t AIBoxFloat( float f )
{
	AIValue_t t;
	t.expType = EX_VALUE;
	t.valType = VALUE_FLOAT;
	t.l.floatValue = f;
	return t;
}

static AIValue_t AIBoxInt( int i )
{
	AIValue_t t;
	t.expType = EX_VALUE;
	t.valType = VALUE_INT;
	t.l.intValue = i;
	return t;
}

static AIValue_t AIBoxToken( const pc_token_t *token )
{
	if ( ( float ) token->intvalue != token->floatvalue )
	{
		return AIBoxFloat( token->floatvalue );
	}
	else
	{
		return AIBoxInt( token->intvalue );
	}
}

static float AIUnBoxFloat( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return v.l.floatValue;
		case VALUE_INT:
			return ( float ) v.l.intValue;
	}
	return 0.0f;
}

static int AIUnBoxInt( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return ( int ) v.l.floatValue;
		case VALUE_INT:
			return v.l.intValue;
	}
	return 0;
}

static double AIUnBoxDouble( AIValue_t v )
{
	switch ( v.valType )
	{
		case VALUE_FLOAT:
			return ( double ) v.l.floatValue;
		case VALUE_INT:
			return v.l.intValue;
	}
	return 0.0;
}

// functions that are used to provide values to the behavior tree
static AIValue_t buildingIsDamaged( gentity_t *self, const AIValue_t *params )
{
	return AIBoxInt( self->botMind->closestDamagedBuilding.ent != NULL );
}

static AIValue_t haveWeapon( gentity_t *self, const AIValue_t *params )
{
	return AIBoxInt( BG_InventoryContainsWeapon( AIUnBoxInt( params[ 0 ] ), self->client->ps.stats ) );
}

static AIValue_t alertedToEnemy( gentity_t *self, const AIValue_t *params )
{
	qboolean success;
	if ( level.time - self->botMind->timeFoundEnemy < g_bot_reactiontime.integer )
	{
		success = qfalse;
	}
	else if ( !self->botMind->bestEnemy.ent )
	{
		success = qfalse;
	}
	else
	{
		success = qtrue;
	}

	return AIBoxInt( success );
}

static AIValue_t botTeam( gentity_t *self, const AIValue_t *params )
{
	return AIBoxInt( self->client->ps.stats[ STAT_TEAM ] );
}

static AIValue_t currentWeapon( gentity_t *self, const AIValue_t *params )
{
	return AIBoxInt( self->client->ps.weapon == AIUnBoxInt( params[ 0 ] ) );
}

static AIValue_t haveUpgrade( gentity_t *self, const AIValue_t *params )
{
	int upgrade = AIUnBoxInt( params[ 0 ] );
	return AIBoxInt( !BG_UpgradeIsActive( upgrade, self->client->ps.stats ) && BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) );
}
static AIValue_t botHealth( gentity_t *self, const AIValue_t *params )
{
	float health = self->health;
	float maxHealth = BG_Class( self->client->ps.stats[ STAT_CLASS ] )->health;
	return AIBoxFloat( health / maxHealth );
}

static AIValue_t botAmmo( gentity_t *self, const AIValue_t *params )
{
	return AIBoxFloat( PercentAmmoRemaining( BG_PrimaryWeapon( self->client->ps.stats ), &self->client->ps ) );
}

static AIValue_t teamateHasWeapon( gentity_t *self, const AIValue_t *params )
{
	return AIBoxInt( BotTeamateHasWeapon( self, AIUnBoxInt( params[ 0 ] ) ) );
}

static AIValue_t distanceTo( gentity_t *self, const AIValue_t *params )
{
	int e = AIUnBoxInt( params[ 0 ] );
	float distance = 0;
	if ( e < BA_NUM_BUILDABLES )
	{
		distance = self->botMind->closestBuildings[ e ].distance;
	}
	else if ( e == E_GOAL )
	{
		distance = DistanceToGoal( self );
	}
	else if ( e == E_ENEMY )
	{
		distance = self->botMind->bestEnemy.distance;
	}
	else if ( e == E_DAMAGEDBUILDING )
	{
		distance = self->botMind->closestDamagedBuilding.distance;
	}
	return AIBoxFloat( distance );
}

static AIValue_t baseRushScore( gentity_t *self, const AIValue_t *params )
{
	return AIBoxFloat( BotGetBaseRushScore( self ) );
}

static AIValue_t healScore( gentity_t *self, const AIValue_t *params )
{
	return AIBoxFloat( BotGetHealScore( self ) );
}

static AIValue_t botClass( gentity_t *self, const AIValue_t *params )
{
	return AIBoxInt( self->client->ps.stats[ STAT_CLASS ] );
}

// functions accessible to the behavior tree for use in condition nodes
static const struct AIConditionMap_s
{
	const char    *name;
	AIValueType_t retType;
	AIFunc        func;
	int           nparams;
} conditionFuncs[] =
{
	{ "alertedToEnemy",    VALUE_INT,   alertedToEnemy,    0 },
	{ "baseRushScore",     VALUE_FLOAT, baseRushScore,     0 },
	{ "buildingIsDamaged", VALUE_INT,   buildingIsDamaged, 0 },
	{ "class",             VALUE_INT,   botClass,          0 },
	{ "distanceTo",        VALUE_FLOAT, distanceTo,        1 },
	{ "haveUpgrade",       VALUE_INT,   haveUpgrade,       1 },
	{ "haveWeapon",        VALUE_INT,   haveWeapon,        1 },
	{ "healScore",         VALUE_FLOAT, healScore,         0 },
	{ "percentHealth",     VALUE_FLOAT, botHealth,         0 },
	{ "team",              VALUE_INT,   botTeam,           0 },
	{ "teamateHasWeapon",  VALUE_INT,   teamateHasWeapon,  1 },
	{ "weapon",            VALUE_INT,   currentWeapon,     0 }
};

static const struct AIOpMap_s
{
	const char            *str;
	int                   tokenSubtype;
	AIOpType_t            opType;
} conditionOps[] =
{
	{ ">=", P_LOGIC_GEQ,     OP_GREATERTHANEQUAL },
	{ ">",  P_LOGIC_GREATER, OP_GREATERTHAN      },
	{ "<=", P_LOGIC_LEQ,     OP_LESSTHANEQUAL    },
	{ "<",  P_LOGIC_LESS,    OP_LESSTHAN         },
	{ "==", P_LOGIC_EQ,      OP_EQUAL            },
	{ "!=", P_LOGIC_UNEQ,    OP_NEQUAL           },
	{ "!",  P_LOGIC_NOT,     OP_NOT              },
	{ "&&", P_LOGIC_AND,     OP_AND              },
	{ "||", P_LOGIC_OR,      OP_OR               }
};

static AIOpType_t opTypeFromToken( pc_token_t *token )
{
	int i;
	if ( token->type != TT_PUNCTUATION )
	{
		return OP_NONE;
	}

	for ( i = 0; i < ARRAY_LEN( conditionOps ); i++ )
	{
		if ( token->subtype == conditionOps[ i ].tokenSubtype )
		{
			return conditionOps[ i ].opType;
		}
	}
	return OP_NONE;
}

static const char *opTypeToString( AIOpType_t op )
{
	int i;
	for ( i = 0; i < ARRAY_LEN( conditionOps ); i++ )
	{
		if ( conditionOps[ i ].opType == op )
		{
			return conditionOps[ i ].str;
		}
	}
	return NULL;
}

static qboolean isBinaryOp( AIOpType_t op )
{
	switch ( op )
	{
		case OP_GREATERTHAN:
		case OP_GREATERTHANEQUAL:
		case OP_LESSTHAN:
		case OP_LESSTHANEQUAL:
		case OP_EQUAL:
		case OP_NEQUAL:
		case OP_AND:
		case OP_OR:
			return qtrue;
		default: return qfalse;
	}
}

static qboolean isUnaryOp( AIOpType_t op )
{
	return op == OP_NOT;
}

// compare operator precedence
static int opCompare( AIOpType_t op1, AIOpType_t op2 )
{
	if ( op1 < op2 )
	{
		return 1;
	}
	else if ( op1 < op2 )
	{
		return -1;
	}
	return 0;
}

static pc_token_list *findCloseParen( pc_token_list *start, pc_token_list *end )
{
	pc_token_list *list = start;
	int depth = 0;

	while ( list != end )
	{
		if ( list->token.string[ 0 ] == '(' )
		{
			depth++;
		}

		if ( list->token.string[ 0 ] == ')' )
		{
			depth--;
		}

		if ( depth == 0 )
		{
			return list;
		}

		list = list->next;
	}

	return NULL;
}

qboolean expectToken( const char *s, pc_token_list **list, qboolean next )
{
	const pc_token_list *current = *list;

	if ( !current )
	{
		BotError( "Expected token %s but found end of file\n", s );
		return qfalse;
	}
	
	if ( Q_stricmp( current->token.string, s ) != 0 )
	{
		BotError( "Expected token %s but found %s on line %d\n", s, current->token.string, current->token.line );
		return qfalse;
	}
	
	if ( next )
	{
		*list = current->next;
	}
	return qtrue;
}

static AIOp_t *newOp( pc_token_list *list )
{
	pc_token_list *current = list;
	AIOp_t *ret = NULL;

	AIOpType_t op = opTypeFromToken( &current->token );

	if ( isBinaryOp( op ) )
	{
		AIBinaryOp_t *b = ( AIBinaryOp_t * ) BG_Alloc( sizeof( *b ) );
		b->opType = op;
		ret = ( AIOp_t * ) b;
	}
	else if ( isUnaryOp( op ) )
	{
		AIUnaryOp_t *u = ( AIUnaryOp_t * ) BG_Alloc( sizeof( *u ) );
		u->opType = op;
		ret = ( AIOp_t * ) u;
	}

	return ret;
}

static AIValue_t *newValueLiteral( pc_token_list **list )
{
	AIValue_t *ret;
	pc_token_list *current = *list;
	pc_token_t *token = &current->token;

	ret = ( AIValue_t * ) BG_Alloc( sizeof( *ret ) );

	*ret = AIBoxToken( token );

	*list = current->next;
	return ret;
}

static AIValueFunc_t *newValueFunc( pc_token_list **list )
{
	AIValueFunc_t *ret = NULL;
	AIValueFunc_t v;
	pc_token_list *current = *list;
	pc_token_list *parenBegin = NULL;
	pc_token_list *parenEnd = NULL;
	pc_token_list *parse = NULL;
	int numParams = 0;
	struct AIConditionMap_s *f;

	memset( &v, 0, sizeof( v ) );

	f = bsearch( current->token.string, conditionFuncs, ARRAY_LEN( conditionFuncs ), sizeof( *conditionFuncs ), cmdcmp );

	if ( !f )
	{
		BotError( "Unknown function: %s on line %d\n", current->token.string, current->token.line );
		*list = current->next;
		return NULL;
	}

	v.expType = EX_FUNC;
	v.retType = f->retType;
	v.func =    f->func;
	v.nparams = f->nparams;

	parenBegin = current->next;

	// if the function has no parameters, allow it to be used without parenthesis
	if ( v.nparams == 0 && parenBegin->token.string[ 0 ] != '(' )
	{
		ret = ( AIValueFunc_t * ) BG_Alloc( sizeof( *ret ) );
		memcpy( ret, &v, sizeof( *ret ) );

		*list = current->next;
		return ret;
	}

	// functions should always be proceeded by a '(' if they have parameters
	if ( !expectToken( "(", &parenBegin, qfalse ) )
	{
		*list = current;
		return NULL;
	}

	// find the end parenthesis around the function's args
	parenEnd = findCloseParen( parenBegin, NULL );

	if ( !parenEnd )
	{
		BotError( "could not find matching ')' for '(' on line %d", parenBegin->token.line );
		*list = parenBegin->next;
		return NULL;
	}

	// count the number of parameters
	parse = parenBegin->next;

	while ( parse != parenEnd )
	{
		if ( parse->token.type == TT_NUMBER )
		{
			numParams++;
		}
		else if ( parse->token.string[ 0 ] != ',' )
		{
			BotError( "Invalid token %s in parameter list on line %d\n", parse->token.string, parse->token.line );
			*list = parenEnd->next; // skip invalid function expression
			return NULL;
		}
		parse = parse->next;
	}

	// warn if too many or too few parameters
	if ( numParams < v.nparams )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: too few parameters for %s on line %d\n", current->token.string, current->token.line );
	}

	if ( numParams > v.nparams )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: too many parameters for %s on line %d\n", current->token.string, current->token.line );
	}

	// create the value op
	ret = ( AIValueFunc_t * ) BG_Alloc( sizeof( *ret ) );

	// copy the members
	memcpy( ret, &v, sizeof( *ret ) );

	if ( ret->nparams )
	{
		// add the parameters
		ret->params = ( AIValue_t * ) BG_Alloc( sizeof( *ret->params ) * ret->nparams );

		numParams = 0;
		parse = parenBegin->next;
		while ( parse != parenEnd )
		{
			if ( parse->token.type == TT_NUMBER )
			{
				ret->params[ numParams ] = AIBoxToken( &parse->token );
				numParams++;
			}
			parse = parse->next;
		}
	}
	*list = parenEnd->next;
	return ret;
}

static void FreeOp( AIOp_t *op );
static void FreeValue( AIValue_t *v )
{
	if ( !v )
	{
		return;
	}

	BG_Free( v );
}
static void FreeValueFunc( AIValueFunc_t *v )
{
	if ( !v )
	{
		return;
	}

	BG_Free( v->params );
	BG_Free( v );
}
static void FreeExpression( AIExpType_t *exp )
{
	if ( !exp )
	{
		return;
	}

	if ( *exp == EX_FUNC )
	{
		AIValueFunc_t *v = ( AIValueFunc_t * ) exp;
		FreeValueFunc( v );
	}
	else if ( *exp == EX_VALUE )
	{
		AIValue_t *v = ( AIValue_t * ) exp;
		
		FreeValue( v );
	}
	else if ( *exp == EX_OP )
	{
		AIOp_t *op = ( AIOp_t * ) exp;

		FreeOp( op );
	}
}
static void FreeOp( AIOp_t *op )
{
	if ( !op )
	{
		return;
	}

	if ( isBinaryOp( op->opType ) )
	{
		AIBinaryOp_t *b = ( AIBinaryOp_t * ) op;
		FreeExpression( b->exp1 );
		FreeExpression( b->exp2 );
	}
	else if ( isUnaryOp( op->opType ) )
	{
		AIUnaryOp_t *u = ( AIUnaryOp_t * ) op;
		FreeExpression( u->exp );
	}

	BG_Free( op );
}

AIExpType_t *makeExpression( AIOp_t *op, AIExpType_t *exp1, AIExpType_t *exp2 )
{
	if ( isUnaryOp( op->opType ) )
	{
		AIUnaryOp_t *u = ( AIUnaryOp_t * ) op;
		u->exp = exp1;
	}
	else if ( isBinaryOp( op->opType ) )
	{
		AIBinaryOp_t *b = ( AIBinaryOp_t * ) op;
		b->exp1 = exp1;
		b->exp2 = exp2;
	}

	return ( AIExpType_t * ) op;
}

AIExpType_t *Primary( pc_token_list **list );
AIExpType_t *ReadConditionExpression( pc_token_list **list, AIOpType_t op2 )
{
	AIExpType_t *t;
	AIOpType_t  op;

	if ( !*list )
	{
		BotError( "Unexpected end of file\n" );
		return NULL;
	}

	t = Primary( list );

	if ( !t )
	{
		return NULL;
	}

	op = opTypeFromToken( &(*list)->token );

	while ( isBinaryOp( op ) && opCompare( op, op2 ) >= 0 )
	{
		AIExpType_t *t1;
		pc_token_list *prev = *list;
		AIOp_t *exp = newOp( *list );
		*list = (*list)->next;
		t1 = ReadConditionExpression( list, op );

		if ( !t1 )
		{
			BotError( "Missing right operand for %s on line %d\n", opTypeToString( op ), prev->token.line );
			FreeExpression( t );
			FreeOp( exp );
			return NULL;
		}

		t = makeExpression( exp, t, t1 );

		op = opTypeFromToken( &(*list)->token );
	}

	return t;
}

AIExpType_t *Primary( pc_token_list **list )
{
	pc_token_list *current = *list;
	AIExpType_t *tree = NULL;

	if ( isUnaryOp( opTypeFromToken( &current->token ) ) )
	{
		AIExpType_t *t;
		AIOp_t *op = newOp( current );
		*list = current->next;
		t = ReadConditionExpression( list, op->opType );

		if ( !t )
		{
			BotError( "Missing right operand for %s on line %d\n", opTypeToString( op->opType ), current->token.line );
			FreeOp( op );
			return NULL;
		}

		tree = makeExpression( op, t, NULL );
	}
	else if ( current->token.string[0] == '(' )
	{
		*list = current->next;
		tree = ReadConditionExpression( list, OP_NONE );
		if ( !expectToken( ")", list, qtrue ) )
		{
			return NULL;
		}
	}
	else if ( current->token.type == TT_NUMBER )
	{
		tree = ( AIExpType_t * ) newValueLiteral( list );
	}
	else if ( current->token.type == TT_NAME )
	{
		tree = ( AIExpType_t * ) newValueFunc( list );
	}
	else
	{
		BotError( "token %s on line %d is not valid\n", current->token.string, current->token.line );
	}
	return tree;
}


/*
======================
ReadConditionNode

Parses and creates an AIConditionNode_t from a token list
The token list pointer is modified to point to the beginning of the next node text block

A condition node has the form:
condition [expression] {
	child node
}

or the form:
condition [expression]

[expression] can be any valid set of boolean operations and values
======================
*/
static AIGenericNode_t *ReadConditionNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;

	AIConditionNode_t *condition;

	if ( !expectToken( "condition", &current, qtrue ) )
	{
		return NULL;
	}

	condition = allocNode( AIConditionNode_t );
	BotInitNode( CONDITION_NODE, BotConditionNode, condition );

	condition->exp = ReadConditionExpression( &current, OP_NONE );

	if ( !current )
	{
		*tokenlist = current;
		BotError( "Unexpected end of file\n" );
		FreeConditionNode( condition );
		return NULL;
	}

	if ( !condition->exp )
	{
		*tokenlist = current;
		FreeConditionNode( condition );
		return NULL;
	}

	if ( Q_stricmp( current->token.string, "{" ) )
	{
		// this condition node has no child nodes
		*tokenlist = current;
		return ( AIGenericNode_t * ) condition;
	}

	current = current->next;

	condition->child = ReadNode( &current );

	if ( !condition->child )
	{
		BotError( "Failed to parse child node of condition on line %d\n", (*tokenlist)->token.line );
		*tokenlist = current;
		FreeConditionNode( condition );
		return NULL;
	}

	if ( !expectToken( "}", &current, qtrue ) )
	{
		*tokenlist = current;
		FreeConditionNode( condition );
		return NULL;
	}

	*tokenlist = current;

	return ( AIGenericNode_t * ) condition;
}

/*
======================
ReadActionNode

Parses and creates an AIGenericNode_t with the type ACTION_NODE from a token list
The token list pointer is modified to point to the beginning of the next node text block after reading

An action node has the form:
action [type]

Where [type] defines which action to execute
======================
*/
static AIGenericNode_t *ReadActionNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;

	AIGenericNode_t *node = NULL;

	if ( !expectToken( "action", &current, qtrue ) )
	{
		return NULL;
	}

	if ( !current )
	{
		BotError( "Unexpected end of file after line %d\n", (*tokenlist)->token.line );
		return NULL;
	}

	if ( !Q_stricmp( current->token.string, "heal" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionHeal, node );
	}
	else if ( !Q_stricmp( current->token.string, "fight" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionFight, node );
	}
	else if ( !Q_stricmp( current->token.string, "roam" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionRoam, node );
	}
	else if ( !Q_stricmp( current->token.string, "equip" ) )
	{
		AIBuyNode_t *realAction = allocNode( AIBuyNode_t );
		realAction->numUpgrades = 0;
		realAction->weapon = WP_NONE;

		BotInitNode( ACTION_NODE, BotActionBuy, realAction );
		node = ( AIGenericNode_t * ) realAction;
	}
	else if ( !Q_stricmp( current->token.string, "buy" ) )
	{
		AIBuyNode_t *realAction = allocNode( AIBuyNode_t );

		current = current->next;

		CheckToken( "weapon", "action buy", &current->token, TT_NUMBER );

		realAction->weapon = ( weapon_t ) current->token.intvalue;
		realAction->numUpgrades = 0;

		BotInitNode( ACTION_NODE, BotActionBuy, realAction );
		node = ( AIGenericNode_t * ) realAction;
	}
	else if ( !Q_stricmp( current->token.string, "flee" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionFlee, node );
	}
	else if ( !Q_stricmp( current->token.string, "repair" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionRepair, node );
	}
	else if ( !Q_stricmp( current->token.string, "evolve" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionEvolve, node );
	}
	else if ( !Q_stricmp( current->token.string, "rush" ) )
	{
		node = allocNode( AIGenericNode_t );
		BotInitNode( ACTION_NODE, BotActionRush, node );
	}
	else
	{
		BotError( "Invalid token %s on line %d\n", current->token.string, current->token.line );
	}

	*tokenlist = current->next;
	return ( AIGenericNode_t * ) node;
}

/*
======================
ReadNodeList

Parses and creates an AINodeList_t from a token list
The token list pointer is modified to point to the beginning of the next node text block after reading

A node list has one of these forms:
selector sequence {
[ one or more child nodes ]
}

selector priority {
[ one or more child nodes ]
}

selector {
[ one or more child nodes ]
}
======================
*/
static AIGenericNode_t *ReadNodeList( pc_token_list **tokenlist )
{
	AINodeList_t *list;
	pc_token_list *current = *tokenlist;

	if ( !expectToken( "selector", &current, qtrue ) )
	{
		return NULL;
	}

	list = allocNode( AINodeList_t );

	if ( !Q_stricmp( current->token.string, "sequence" ) )
	{
		BotInitNode( SELECTOR_NODE, BotSequenceNode, list );
		current = current->next;
	}
	else if ( !Q_stricmp( current->token.string, "priority" ) )
	{
		BotInitNode( SELECTOR_NODE, BotPriorityNode, list );
		current = current->next;
	}
	else if ( !Q_stricmp( current->token.string, "{" ) )
	{
		BotInitNode( SELECTOR_NODE, BotSelectorNode, list );
	}
	else
	{
		BotError( "Invalid token %s on line %d\n", current->token.string, current->token.line );
		FreeNodeList( list );
		*tokenlist = current;
		return NULL;
	}

	if ( !expectToken( "{", &current, qtrue ) )
	{
		FreeNodeList( list );
		return NULL;
	}

	while ( Q_stricmp( current->token.string, "}" ) )
	{
		AIGenericNode_t *node = ReadNode( &current );

		if ( node && list->numNodes >= MAX_NODE_LIST )
		{
			BotError( "Max selector children limit exceeded at line %d\n", (*tokenlist)->token.line );
			FreeNode( node );
			FreeNodeList( list );
			*tokenlist = current;
			return NULL;
		}
		else if ( node )
		{
			list->list[ list->numNodes ] = node;
			list->numNodes++;
		}

		if ( !node )
		{
			*tokenlist = current;
			FreeNodeList( list );
			return NULL;
		}

		if ( !current )
		{
			*tokenlist = current;
			return ( AIGenericNode_t * ) list;
		}
	}

	*tokenlist = current->next;
	return ( AIGenericNode_t * ) list;
}

/*
======================
ReadNode

Parses and creates an AIGenericNode_t from a token list
The token list pointer is modified to point to the next node text block after reading

This function delegates the reading to the sub functions
ReadNodeList, ReadActionNode, and ReadConditionNode depending on the first token in the list
======================
*/

static AIGenericNode_t *ReadNode( pc_token_list **tokenlist )
{
	pc_token_list *current = *tokenlist;
	AIGenericNode_t *node;

	if ( !Q_stricmp( current->token.string, "selector" ) )
	{
		node = ReadNodeList( &current );
	}
	else if ( !Q_stricmp( current->token.string, "action" ) )
	{
		node = ReadActionNode( &current );
	}
	else if ( !Q_stricmp( current->token.string, "condition" ) )
	{
		node = ReadConditionNode( &current );
	}
	else
	{
		BotError( "Invalid token on line %d found: %s\n", current->token.line, current->token.string );
		node = NULL;
	}

	*tokenlist = current;
	return node;
}

void InitTreeList( AITreeList_t *list )
{
	list->trees = ( AIBehaviorTree_t ** ) BG_Alloc( sizeof( AIBehaviorTree_t * ) * 10 );
	list->maxTrees = 10;
	list->numTrees = 0;
}

void AddTreeToList( AITreeList_t *list, AIBehaviorTree_t *tree )
{
	if ( list->maxTrees == list->numTrees )
	{
		AIBehaviorTree_t **trees = ( AIBehaviorTree_t ** ) BG_Alloc( sizeof( AIBehaviorTree_t * ) * list->maxTrees );
		list->maxTrees *= 2;
		memcpy( trees, list->trees, sizeof( AIBehaviorTree_t * ) * list->numTrees );
		BG_Free( list->trees );
		list->trees = trees;
	}

	list->trees[ list->numTrees ] = tree;
	list->numTrees++;
}

void RemoveTreeFromList( AITreeList_t *list, AIBehaviorTree_t *tree )
{
	int i;

	for ( i = 0; i < list->numTrees; i++ )
	{
		AIBehaviorTree_t *testTree = list->trees[ i ];
		if ( !Q_stricmp( testTree->name, tree->name ) )
		{
			FreeBehaviorTree( testTree );
			memmove( &list->trees[ i ], &list->trees[ i + 1 ], sizeof( AIBehaviorTree_t * ) * ( list->numTrees - i - 1 ) );
			list->numTrees--;
		}
	}
}

void FreeTreeList( AITreeList_t *list )
{
	int i;
	for ( i = 0; i < list->numTrees; i++ )
	{
		AIBehaviorTree_t *tree = list->trees[ i ];
		FreeBehaviorTree( tree );
	}

	BG_Free( list->trees );
	list->trees = NULL;
	list->maxTrees = 0;
	list->numTrees = 0;
}

/*
======================
ReadBehaviorTree

Load a behavior tree of the given name from a file
======================
*/
AIBehaviorTree_t * ReadBehaviorTree( const char *name, AITreeList_t *list )
{
	int i;
	char treefilename[ MAX_QPATH ];
	int handle;
	pc_token_list *tokenlist;
	AIBehaviorTree_t *tree;
	pc_token_list *current;
	AIGenericNode_t *node;

	// check if this behavior tree has already been loaded
	for ( i = 0; i < list->numTrees; i++ )
	{
		AIBehaviorTree_t *tree = list->trees[ i ];
		if ( !Q_stricmp( tree->name, name ) )
		{
			return tree;
		}
	}

	// add preprocessor defines for use in the behavior tree
	// add upgrades
	D( UP_LIGHTARMOUR );
	D( UP_HELMET );
	D( UP_MEDKIT );
	D( UP_BATTPACK );
	D( UP_JETPACK );
	D( UP_BATTLESUIT );
	D( UP_GRENADE );

	// add weapons
	D( WP_MACHINEGUN );
	D( WP_PAIN_SAW );
	D( WP_SHOTGUN );
	D( WP_LAS_GUN );
	D( WP_MASS_DRIVER );
	D( WP_CHAINGUN );
	D( WP_FLAMER );
	D( WP_PULSE_RIFLE );
	D( WP_LUCIFER_CANNON );
	D( WP_GRENADE );
	D( WP_HBUILD );

	// add teams
	D( TEAM_ALIENS );
	D( TEAM_HUMANS );

	// add AIEntitys
	D( E_NONE );
	D( E_A_SPAWN );
	D( E_A_OVERMIND );
	D( E_A_BARRICADE );
	D( E_A_ACIDTUBE );
	D( E_A_TRAPPER );
	D( E_A_BOOSTER );
	D( E_A_HIVE );
	D( E_H_SPAWN );
	D( E_H_MGTURRET );
	D( E_H_TESLAGEN );
	D( E_H_ARMOURY );
	D( E_H_DCC );
	D( E_H_MEDISTAT );
	D( E_H_REACTOR );
	D( E_H_REPEATER );
	D( E_GOAL );
	D( E_ENEMY );
	D( E_DAMAGEDBUILDING );

	// add player classes
	D( PCL_NONE );
	D( PCL_ALIEN_BUILDER0 );
	D( PCL_ALIEN_BUILDER0_UPG );
	D( PCL_ALIEN_LEVEL0 );
	D( PCL_ALIEN_LEVEL1 );
	D( PCL_ALIEN_LEVEL1_UPG );
	D( PCL_ALIEN_LEVEL2 );
	D( PCL_ALIEN_LEVEL2_UPG );
	D( PCL_ALIEN_LEVEL3 );
	D( PCL_ALIEN_LEVEL3_UPG );
	D( PCL_ALIEN_LEVEL4 );
	D( PCL_HUMAN );
	D( PCL_HUMAN_BSUIT );
	
	Q_strncpyz( treefilename, va( "bots/%s.bt", name ), sizeof( treefilename ) );

	handle = trap_Parse_LoadSource( treefilename );
	if ( !handle )
	{
		G_Printf( S_COLOR_RED "ERROR: Cannot load behavior tree %s: File not found\n", treefilename );
		return NULL;
	}

	tokenlist = CreateTokenList( handle );
	
	tree = ( AIBehaviorTree_t * ) BG_Alloc( sizeof( AIBehaviorTree_t ) );

	Q_strncpyz( tree->name, name, sizeof( tree->name ) );

	current = tokenlist;

	node = ReadNode( &current );
	if ( node )
	{
		tree->root = ( AINode_t * ) node;
	}
	else
	{
		BG_Free( tree );
		tree = NULL;
	}

	if ( tree )
	{
		AddTreeToList( list, tree );
	}

	FreeTokenList( tokenlist );
	trap_Parse_FreeSource( handle );
	return tree;
}

static void FreeConditionNode( AIConditionNode_t *node )
{
	FreeNode( node->child );
	FreeExpression( node->exp );
	BG_Free( node );
}

static void FreeNodeList( AINodeList_t *node )
{
	int i;
	for ( i = 0; i < node->numNodes; i++ )
	{
		FreeNode( node->list[ i ] );
	}
	BG_Free( node );
}

static void FreeNode( AIGenericNode_t *node )
{
	if ( !node )
	{
		return;
	}

	if ( node->type == SELECTOR_NODE )
	{
		FreeNodeList( ( AINodeList_t * ) node );
	}
	else if ( node->type == CONDITION_NODE )
	{
		FreeConditionNode( ( AIConditionNode_t * ) node );
	}
	else if ( node->type == ACTION_NODE )
	{
		BG_Free( node );
	}
}

void FreeBehaviorTree( AIBehaviorTree_t *tree )
{
	if ( tree )
	{
		FreeNode( ( AIGenericNode_t * ) tree->root );

		BG_Free( tree );
	}
	else
	{
		G_Printf( "WARNING: Attempted to free NULL behavior tree\n" );
	}
}

static qboolean NodeIsRunning( gentity_t *self, AIGenericNode_t *node )
{
	int i;
	for ( i = 0; i < self->botMind->numRunningNodes; i++ )
	{
		if ( self->botMind->runningNodes[ i ] == node )
		{
			return qtrue;
		}
	}
	return qfalse;
}

/*
	Behavior tree control-flow nodes
*/
AINodeStatus_t BotSelectorNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *selector = ( AINodeList_t * ) node;
	int i;

	i = 0;

	// find a previously running node and start there
	for ( i = selector->numNodes - 1; i > 0; i-- )
	{
		if ( NodeIsRunning( self, selector->list[ i ] ) )
		{
			break;
		}
	}

	for ( ; i < selector->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, selector->list[ i ] );
		if ( status == STATUS_FAILURE )
		{
			continue;
		}
		return status;
	}
	return STATUS_FAILURE;
}

AINodeStatus_t BotSequenceNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *sequence = ( AINodeList_t * ) node;
	int i = 0;

	// find a previously running node and start there
	for ( i = sequence->numNodes - 1; i > 0; i-- )
	{
		if ( NodeIsRunning( self, sequence->list[ i ] ) )
		{
			break;
		}
	}

	for ( ; i < sequence->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, sequence->list[ i ] );
		if ( status == STATUS_FAILURE )
		{
			return STATUS_FAILURE;
		}

		if ( status == STATUS_RUNNING )
		{
			return STATUS_RUNNING;
		}
	}
	return STATUS_SUCCESS;
}

AINodeStatus_t BotPriorityNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *priority = ( AINodeList_t * ) node;
	int i;

	for ( i = 0; i < priority->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, priority->list[ i ] );
		if ( status == STATUS_FAILURE )
		{
			continue;
		}
		return status;
	}
	return STATUS_FAILURE;
}

AINodeStatus_t BotParallelNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeList_t *parallel = ( AINodeList_t * ) node;
	int i = 0;
	int numFailure = 0;

	for ( ; i < parallel->numNodes; i++ )
	{
		AINodeStatus_t status = BotEvaluateNode( self, parallel->list[ i ] );

		if ( status == STATUS_FAILURE )
		{
			numFailure++;

			if ( numFailure < parallel->maxFail )
			{
				continue;
			}
		}

		return status;
	}
	return STATUS_FAILURE;
}
qboolean EvalConditionExpression( gentity_t *self, AIExpType_t *exp );

double EvalFunc( gentity_t *self, AIExpType_t *exp )
{
	AIValueFunc_t *v = ( AIValueFunc_t * ) exp;

	return AIUnBoxDouble( v->func( self, v->params ) );
}

// using double because it has enough precision to exactly represent both a float and an int
double EvalValue( gentity_t *self, AIExpType_t *exp )
{
	AIValue_t *v = ( AIValue_t * ) exp;

	if ( *exp == EX_FUNC )
	{
		return EvalFunc( self, exp ); 
	}

	if ( *exp != EX_VALUE )
	{
		return ( double ) EvalConditionExpression( self, exp );
	}

	return AIUnBoxDouble( *v );
}

qboolean EvaluateBinaryOp( gentity_t *self, AIExpType_t *exp )
{
	AIBinaryOp_t *o = ( AIBinaryOp_t * ) exp;
	qboolean      ret = qfalse;
	
	switch ( o->opType )
	{
		case OP_LESSTHAN:
			return EvalValue( self, o->exp1 ) < EvalValue( self, o->exp2 );
		case OP_LESSTHANEQUAL:
			return EvalValue( self, o->exp1 ) <= EvalValue( self, o->exp2 );
		case OP_GREATERTHAN:
			return EvalValue( self, o->exp1 ) > EvalValue( self, o->exp2 );
		case OP_GREATERTHANEQUAL:
			return EvalValue( self, o->exp1 ) >= EvalValue( self, o->exp2 );
		case OP_EQUAL:
			return EvalValue( self, o->exp1 ) == EvalValue( self, o->exp2 );
		case OP_NEQUAL:
			return EvalValue( self, o->exp1 ) != EvalValue( self, o->exp2 );
		case OP_AND:
			return EvalConditionExpression( self, o->exp1 ) && EvalConditionExpression( self, o->exp2 );
		case OP_OR:
			return EvalConditionExpression( self, o->exp1 ) || EvalConditionExpression( self, o->exp2 );
		default:
			return qfalse;
	}
}

qboolean EvaluateUnaryOp( gentity_t *self, AIExpType_t *exp )
{
	AIUnaryOp_t *o = ( AIUnaryOp_t * ) exp;
	return !EvalConditionExpression( self, o->exp );
}

qboolean EvalConditionExpression( gentity_t *self, AIExpType_t *exp )
{
	if ( *exp == EX_OP )
	{
		AIOp_t *op = ( AIOp_t * ) exp;

		if ( isBinaryOp( op->opType ) )
		{
			return EvaluateBinaryOp( self, exp );
		}
		else if ( isUnaryOp( op->opType ) )
		{
			return EvaluateUnaryOp( self, exp );
		}
	}
	else if ( *exp  == EX_VALUE )
	{
		return EvalValue( self, exp ) != 0.0;
	}
	else if ( *exp == EX_FUNC )
	{
		return EvalFunc( self, exp ) != 0.0;
	}

	return qfalse;
}

AINodeStatus_t BotConditionNode( gentity_t *self, AIGenericNode_t *node )
{
	qboolean success = qfalse;

	AIConditionNode_t *con = ( AIConditionNode_t * ) node;

	success = EvalConditionExpression( self, con->exp );
	if ( success )
	{
		if ( con->child )
		{
			return BotEvaluateNode( self, con->child );
		}
		else
		{
			return STATUS_SUCCESS;
		}
	}

	return STATUS_FAILURE;
}

AINodeStatus_t BotEvaluateNode( gentity_t *self, AIGenericNode_t *node )
{
	AINodeStatus_t status = node->run( self, node );

	// maintain current node data for action nodes
	// they use this to determine if they need to pathfind again
	if ( node->type == ACTION_NODE )
	{
		if ( status == STATUS_RUNNING )
		{
			self->botMind->currentNode = node;
		}

		if ( self->botMind->currentNode == node && status != STATUS_RUNNING )
		{
			self->botMind->currentNode = NULL;
		}
	}

	// reset running information on node success so sequences and selectors reset their state
	if ( NodeIsRunning( self, node ) && status == STATUS_SUCCESS )
	{
		memset( self->botMind->runningNodes, 0, sizeof( self->botMind->runningNodes ) );
		self->botMind->numRunningNodes = 0;
	}

	// store running information for sequence nodes and selector nodes
	if ( status == STATUS_RUNNING )
	{
		if ( self->botMind->numRunningNodes == MAX_NODE_DEPTH )
		{
			G_Printf( "ERROR: MAX_NODE_DEPTH exceeded\n" );
			return status;
		}

		// clear out previous running list when we hit a running leaf node
		// this insures that only 1 node in a sequence or selector has the running state
		if ( node->type == ACTION_NODE )
		{
			memset( self->botMind->runningNodes, 0, sizeof( self->botMind->runningNodes ) );
			self->botMind->numRunningNodes = 0;
		}

		self->botMind->runningNodes[ self->botMind->numRunningNodes++ ] = node;
	}

	return status;
}

/*
	Behavior tree action nodes
*/
AINodeStatus_t BotActionFight( gentity_t *self, AIGenericNode_t *node )
{
	team_t myTeam = ( team_t ) self->client->ps.stats[ STAT_TEAM ];

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->bestEnemy.ent ) )
		{
			return STATUS_FAILURE;
		}
		else
		{
			return STATUS_RUNNING;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( BotGetTargetTeam( self->botMind->goal ) == myTeam || BotGetTargetTeam( self->botMind->goal ) == TEAM_NONE )
	{
		self->botMind->bestEnemy.ent = NULL;
		return STATUS_FAILURE;
	}

	// the enemy has died
	if ( self->botMind->goal.ent->health <= 0 )
	{
		self->botMind->bestEnemy.ent = NULL;
		return STATUS_SUCCESS;
	}

	if ( self->botMind->goal.ent->client && self->botMind->goal.ent->client->sess.spectatorState != SPECTATOR_NOT )
	{
		self->botMind->bestEnemy.ent = NULL;
		return STATUS_FAILURE;
	}

	if ( WeaponIsEmpty( ( weapon_t )self->client->ps.weapon, self->client->ps ) && myTeam == TEAM_HUMANS )
	{
		G_ForceWeaponChange( self, WP_BLASTER );
	}

	if ( self->client->ps.weapon == WP_HBUILD )
	{
		G_ForceWeaponChange( self, WP_BLASTER );
	}

	//aliens have radar so they will always 'see' the enemy if they are in radar range
	if ( myTeam == TEAM_ALIENS && DistanceToGoalSquared( self ) <= Square( ALIENSENSE_RANGE ) )
	{
		self->botMind->enemyLastSeen = level.time;
	}

	if ( !BotTargetIsVisible( self, self->botMind->goal, CONTENTS_SOLID ) )
	{
		botTarget_t proposedTarget;
		BotSetTarget( &proposedTarget, self->botMind->bestEnemy.ent, NULL );

		//we can see another enemy (not our target)
		if ( self->botMind->bestEnemy.ent && self->botMind->goal.ent != self->botMind->bestEnemy.ent && BotPathIsWalkable( self, proposedTarget ) )
		{
			//change targets
			BotChangeGoal( self, proposedTarget );
			return STATUS_RUNNING;
		}
		else if ( level.time - self->botMind->enemyLastSeen > g_bot_chasetime.integer )
		{
			self->botMind->bestEnemy.ent = NULL;
			return STATUS_FAILURE;
		}
		else
		{
			BotMoveToGoal( self );
			return STATUS_RUNNING;
		}
	}
	else
	{
		qboolean inAttackRange = BotTargetInAttackRange( self, self->botMind->goal );
		self->botMind->enemyLastSeen = level.time;

		if ( ( inAttackRange && myTeam == TEAM_HUMANS ) || self->botMind->directPathToGoal )
		{
			botRouteTarget_t routeTarget;
			BotAimAtEnemy( self );

			BotTargetToRouteTarget( self, self->botMind->goal, &routeTarget );

			//update the path corridor
			trap_BotUpdatePath( self->s.number, &routeTarget, NULL, &self->botMind->directPathToGoal );

			BotMoveInDir( self, MOVE_FORWARD );

			if ( inAttackRange || self->client->ps.weapon == WP_PAIN_SAW )
			{
				BotFireWeaponAI( self );
			}

			if ( myTeam == TEAM_HUMANS )
			{
				if ( self->botMind->botSkill.level >= 3 && DistanceToGoalSquared( self ) < Square( MAX_HUMAN_DANCE_DIST )
				        && ( DistanceToGoalSquared( self ) > Square( MIN_HUMAN_DANCE_DIST ) || self->botMind->botSkill.level < 5 )
				        && self->client->ps.weapon != WP_PAIN_SAW )
				{
					BotMoveInDir( self, MOVE_BACKWARD );
				}
				else if ( DistanceToGoalSquared( self ) <= Square( MIN_HUMAN_DANCE_DIST ) ) //we wont hit this if skill < 5
				{
					//we will be moving toward enemy, strafe too
					//the result: we go around the enemy
					BotAlternateStrafe( self );

					if ( self->client->ps.weapon != WP_PAIN_SAW )
					{
						BotDodge( self );
					}
				}
				else if ( DistanceToGoalSquared( self ) >= Square( MAX_HUMAN_DANCE_DIST ) && self->client->ps.weapon != WP_PAIN_SAW )
				{
					if ( DistanceToGoalSquared( self ) - Square( MAX_HUMAN_DANCE_DIST ) < 100 )
					{
						BotStandStill( self );
					}

					BotStrafeDodge( self );
				}

				if ( inAttackRange && BotGetTargetType( self->botMind->goal ) == ET_BUILDABLE )
				{
					BotStandStill( self );
				}

				BotSprint( self, qtrue );
			}
			else if ( myTeam == TEAM_ALIENS )
			{
				BotClassMovement( self, inAttackRange );
			}
		}
		else
		{
			BotMoveToGoal( self );
		}
	}
	return STATUS_RUNNING;
}

AINodeStatus_t BotActionFlee( gentity_t *self, AIGenericNode_t *node )
{
	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoal( self, BotGetRetreatTarget( self ) ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) < Square( 70 ) )
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BotMoveToGoal( self );
	}

	return STATUS_RUNNING;
}

AINodeStatus_t BotActionRoam( gentity_t *self, AIGenericNode_t *node )
{
	// we are just starting to roam, get a target location
	if ( node != self->botMind->currentNode )
	{
		botTarget_t target = BotGetRoamTarget( self );
		if ( !BotChangeGoal( self, target ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( self->botMind->directPathToGoal && DistanceToGoal2DSquared( self ) < Square( 70 ) )
	{
		return STATUS_SUCCESS;
	}
	else
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

botTarget_t BotGetMoveToTarget( gentity_t *self, AIMoveToNode_t *node )
{
	botTarget_t target;
	gentity_t *ent = NULL;

	if ( node->ent < BA_NUM_BUILDABLES )
	{
		ent = self->botMind->closestBuildings[ node->ent ].ent;
	}
	else if ( node->ent == E_ENEMY )
	{
		ent = self->botMind->bestEnemy.ent;
	}
	else if ( node->ent == E_DAMAGEDBUILDING )
	{
		ent = self->botMind->closestDamagedBuilding.ent;
	}

	BotSetTarget( &target, ent, NULL );
	return target;
}

AINodeStatus_t BotActionMoveTo( gentity_t *self, AIGenericNode_t *node )
{
	float radius;
	AIMoveToNode_t *moveTo = ( AIMoveToNode_t * ) node;
	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoal( self, BotGetMoveToTarget( self, moveTo ) ) )
		{
			return STATUS_FAILURE;
		}
		else
		{
			return STATUS_RUNNING;
		}
	}

	if ( self->botMind->goal.ent )
	{
		// died
		if ( self->botMind->goal.ent->health < 0 )
		{
			return STATUS_FAILURE;
		}
	}

	BotMoveToGoal( self );

	if ( moveTo->range == -1 )
	{
		radius = BotGetGoalRadius( self );
	}
	else
	{
		radius = moveTo->range;
	}

	if ( DistanceToGoal2DSquared( self ) <= Square( radius ) && self->botMind->directPathToGoal )
	{
		return STATUS_SUCCESS;
	}

	return STATUS_RUNNING;
}

AINodeStatus_t BotActionRush( gentity_t *self, AIGenericNode_t *node )
{
	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoal( self, BotGetRushTarget( self ) ) )
		{
			return STATUS_FAILURE;
		}
		else
		{
			return STATUS_RUNNING;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) > Square( 100 ) )
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

AINodeStatus_t BotActionHeal( gentity_t *self, AIGenericNode_t *node )
{
	if ( self->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
	{
		return BotActionHealH( self, node );
	}
	else
	{
		return BotActionHealA( self, node );
	}
}

/*
	alien specific actions
*/
AINodeStatus_t BotActionEvolve ( gentity_t *self, AIGenericNode_t *node )
{
	AINodeStatus_t status = STATUS_FAILURE;
	if ( !g_bot_evolve.integer )
	{
		return status;
	}

	if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL4 ) && g_bot_level4.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL4 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL3_UPG ) && g_bot_level3upg.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL3_UPG ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL3 ) && ( !BG_ClassAllowedInStage( PCL_ALIEN_LEVEL3_UPG, ( stage_t ) g_alienStage.integer ) || !g_bot_level2upg.integer || !g_bot_level3upg.integer ) && g_bot_level3.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL3 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL2_UPG ) && g_bot_level2upg.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL2_UPG ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL2 ) && ( g_humanStage.integer == 0 || !g_bot_level2upg.integer )  && g_bot_level2.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL2 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL1_UPG ) && g_humanStage.integer == 0 && g_bot_level1upg.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL1_UPG ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL1 ) && g_humanStage.integer == 0 && g_bot_level1.integer )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL1 ) )
		{
			status = STATUS_SUCCESS;
		}
	}
	else if ( BotCanEvolveToClass( self, PCL_ALIEN_LEVEL0 ) )
	{
		if ( BotEvolveToClass( self, PCL_ALIEN_LEVEL0 ) )
		{
			status = STATUS_SUCCESS;
		}
	}

	return status;
}

AINodeStatus_t BotActionHealA( gentity_t *self, AIGenericNode_t *node )
{
	const int maxHealth = BG_Class( ( class_t )self->client->ps.stats[STAT_CLASS] )->health;
	gentity_t *healTarget = NULL;
	float distToHealer = 0;

	if ( self->botMind->closestBuildings[BA_A_BOOSTER].ent )
	{
		healTarget = self->botMind->closestBuildings[BA_A_BOOSTER].ent;
	}
	else if ( self->botMind->closestBuildings[BA_A_OVERMIND].ent )
	{
		healTarget = self->botMind->closestBuildings[BA_A_OVERMIND].ent;
	}
	else if ( self->botMind->closestBuildings[BA_A_SPAWN].ent )
	{
		healTarget = self->botMind->closestBuildings[BA_A_SPAWN].ent;
	}

	if ( !healTarget )
	{
		return STATUS_FAILURE;
	}

	if ( self->client->ps.stats[STAT_TEAM] != TEAM_ALIENS )
	{
		return STATUS_FAILURE;
	}

	//we are fully healed
	if ( maxHealth == self->client->ps.stats[STAT_HEALTH] )
	{
		return STATUS_SUCCESS;
	}

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoalEntity( self, healTarget ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	//target has died, signal goal is unusable
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) > Square( 70 ) )
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}

/*
	human specific actions
*/
AINodeStatus_t BotActionHealH( gentity_t *self, AIGenericNode_t *node )
{
	vec3_t targetPos;
	vec3_t myPos;

	if ( self->client->ps.stats[STAT_TEAM] != TEAM_HUMANS )
	{
		return STATUS_FAILURE;
	}

	//we are fully healed
	if ( BG_Class( ( class_t )self->client->ps.stats[STAT_CLASS] )->health <= self->health && BG_InventoryContainsUpgrade( UP_MEDKIT, self->client->ps.stats ) )
	{
		return STATUS_SUCCESS;
	}

	if ( self->botMind->currentNode != node )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->closestBuildings[ BA_H_MEDISTAT ].ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	//safety check
	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	//the medi has died so signal that the goal is unusable
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	//this medi is no longer powered so signal that the goal is unusable
	if ( !self->botMind->goal.ent->powered )
	{
		return STATUS_FAILURE;
	}

	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorCopy( self->s.origin, myPos );
	targetPos[2] += BG_BuildableModelConfig( BA_H_MEDISTAT )->maxs[2];
	myPos[2] += self->r.mins[2]; //mins is negative

	//keep moving to the medi until we are on top of it
	if ( DistanceSquared( myPos, targetPos ) > Square( BG_BuildableModelConfig( BA_H_MEDISTAT )->mins[1] ) )
	{
		BotMoveToGoal( self );
	}
	return STATUS_RUNNING;
}
AINodeStatus_t BotActionRepair( gentity_t *self, AIGenericNode_t *node )
{
	vec3_t forward;
	vec3_t targetPos;
	vec3_t selfPos;

	if ( node != self->botMind->currentNode )
	{
		if ( !BotChangeGoalEntity( self, self->botMind->closestDamagedBuilding.ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( self->botMind->goal.ent->health >= BG_Buildable( ( buildable_t )self->botMind->goal.ent->s.modelindex )->health )
	{
		return STATUS_SUCCESS;
	}

	if ( self->client->ps.weapon != WP_HBUILD )
	{
		G_ForceWeaponChange( self, WP_HBUILD );
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	BotGetTargetPos( self->botMind->goal, targetPos );
	VectorMA( self->s.origin, self->r.maxs[1], forward, selfPos );

	//move to the damaged building until we are in range
	if ( !BotTargetIsVisible( self, self->botMind->goal, MASK_SHOT ) || DistanceToGoalSquared( self ) > Square( 100 ) )
	{
		BotMoveToGoal( self );
	}
	else
	{
		//aim at the buildable
		BotSlowAim( self, targetPos, 0.5 );
		BotAimAtLocation( self, targetPos );
		// we automatically heal a building if close enough and aiming at it
	}
	return STATUS_RUNNING;
}
AINodeStatus_t BotActionBuy( gentity_t *self, AIGenericNode_t *node )
{
	AIBuyNode_t *buy = ( AIBuyNode_t * ) node;

	weapon_t weapon = buy->weapon;
	upgrade_t *upgrades = buy->upgrades;
	int numUpgrades = buy->numUpgrades;
	int i;

	if ( weapon == WP_NONE && numUpgrades == 0 )
	{
		BotGetDesiredBuy( self, &weapon, upgrades, &numUpgrades );
	}

	if ( !g_bot_buy.integer )
	{
		return STATUS_FAILURE;
	}
	if ( BotGetEntityTeam( self ) != TEAM_HUMANS )
	{
		return STATUS_FAILURE;
	}
	if ( numUpgrades && upgrades[0] == UP_AMMO && BG_Weapon( (weapon_t)self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy )
	{
		if ( !self->botMind->closestBuildings[ BA_H_ARMOURY ].ent &&
		     !self->botMind->closestBuildings[ BA_H_REPEATER ].ent &&
		     !self->botMind->closestBuildings[ BA_H_REACTOR ].ent )
		{
			return STATUS_FAILURE; //wanted ammo for energy? no armoury, repeater or reactor, so fail
		}
	}
	else if ( !self->botMind->closestBuildings[BA_H_ARMOURY].ent )
	{
		return STATUS_FAILURE;    //no armoury, so fail
	}

	//check if we already have everything
	if ( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) || weapon == WP_NONE )
	{
		int numContain = 0;

		//cant buy more than 3 upgrades
		if ( numUpgrades > 3 )
		{
			return STATUS_FAILURE;
		}

		if ( numUpgrades == 0 )
		{
			return STATUS_FAILURE;
		}

		for ( i = 0; i < numUpgrades; i++ )
		{
			if ( BG_InventoryContainsUpgrade( upgrades[i], self->client->ps.stats ) )
			{
				numContain++;
			}
		}
		//we have every upgrade we want to buy
		if ( numContain == numUpgrades )
		{
			return STATUS_FAILURE;
		}
	}

	if ( self->botMind->currentNode != node )
	{
		//default to armoury
		const botEntityAndDistance_t *ent = &self->botMind->closestBuildings[ BA_H_ARMOURY ];

		//wanting energy ammo only? look for closer repeater or reactor
		if ( numUpgrades && upgrades[0] == UP_AMMO && BG_Weapon( (weapon_t)self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy)
		{
#define DISTANCE(obj) ( self->botMind->closestBuildings[ obj ].ent ? self->botMind->closestBuildings[ obj ].distance : INT_MAX )
			//repeater closest? use that
			if ( DISTANCE( BA_H_REPEATER ) < DISTANCE( BA_H_REACTOR ) )
			{
				if ( DISTANCE( BA_H_REPEATER ) < DISTANCE( BA_H_ARMOURY ) )
				{
					ent = &self->botMind->closestBuildings[ BA_H_REPEATER ];
				}
			}
			//reactor closest? use that
			else if ( DISTANCE( BA_H_REACTOR ) < DISTANCE( BA_H_ARMOURY ) )
			{
				ent = &self->botMind->closestBuildings[ BA_H_REACTOR ];
			}
		}
		if ( !BotChangeGoalEntity( self, ent->ent ) )
		{
			return STATUS_FAILURE;
		}
	}

	if ( !BotTargetIsEntity( self->botMind->goal ) )
	{
		return STATUS_FAILURE;
	}
	if ( self->botMind->goal.ent->health <= 0 )
	{
		return STATUS_FAILURE;
	}

	if ( DistanceToGoalSquared( self ) > 100 * 100 )
	{
		BotMoveToGoal( self );
		return STATUS_RUNNING;
	}
	else
	{

		if ( numUpgrades && upgrades[0] != UP_AMMO )
		{
			BotSellAll( self );
		}
		else if ( weapon != WP_NONE )
		{
			BotSellWeapons( self );
		}

		BotBuyWeapon( self, weapon );

		for ( i = 0; i < numUpgrades; i++ )
		{
			BotBuyUpgrade( self, upgrades[i] );
		}

		// make sure that we're not using the blaster
		G_ForceWeaponChange( self, weapon );
		
		return STATUS_SUCCESS;
	}
}
