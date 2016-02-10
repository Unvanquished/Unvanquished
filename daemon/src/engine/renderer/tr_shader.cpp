/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_shader.c -- this file deals with the parsing and definition of shaders
#include "tr_local.h"
#include "gl_shader.h"

static const int MAX_SHADERTABLE_HASH = 1024;
static shaderTable_t *shaderTableHashTable[ MAX_SHADERTABLE_HASH ];

static const int FILE_HASH_SIZE       = 1024;
static shader_t      *shaderHashTable[ FILE_HASH_SIZE ];

static const int MAX_SHADERTEXT_HASH  = 2048;
static const char          **shaderTextHashTable[ MAX_SHADERTEXT_HASH ];

static char          *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderTable_t table;
static shaderStage_t stages[ MAX_SHADER_STAGES ];
static shader_t      shader;
static texModInfo_t  texMods[ MAX_SHADER_STAGES ][ TR_MAX_TEXMODS ];

// ydnar: these are here because they are only referenced while parsing a shader
static char          implicitMap[ MAX_QPATH ];
static unsigned      implicitStateBits;
static cullType_t    implicitCullType;

static char          whenTokens[ MAX_STRING_CHARS ];

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname, const int size )
{
	int  i;

	long hash;
	char letter;

	hash = 0;
	i = 0;

	while ( fname[ i ] != '\0' )
	{
		letter = Str::ctolower( fname[ i ] );

		if ( letter == '.' )
		{
			break; // don't include extension
		}

		if ( letter == '\\' )
		{
			letter = '/'; // damn path names
		}

		if ( letter == PATH_SEP )
		{
			letter = '/'; // damn path names
		}

		hash += ( long )( letter ) * ( i + 119 );
		i++;
	}

	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
	hash &= ( size - 1 );
	return hash;
}

void R_RemapShader( const char *shaderName, const char *newShaderName, const char* )
{
	char      strippedName[ MAX_QPATH ];
	int       hash;
	shader_t  *sh, *sh2;
	qhandle_t h;

	sh = R_FindShaderByName( shaderName );

	if ( sh == nullptr || sh == tr.defaultShader )
	{
		h = RE_RegisterShader( shaderName, RSF_DEFAULT );
		sh = R_GetShaderByHandle( h );
	}

	if ( sh == nullptr || sh == tr.defaultShader )
	{
		Log::Warn("R_RemapShader: shader %s not found", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );

	if ( sh2 == nullptr || sh2 == tr.defaultShader )
	{
		h = RE_RegisterShader( newShaderName, RSF_DEFAULT );
		sh2 = R_GetShaderByHandle( h );
	}

	if ( sh2 == nullptr || sh2 == tr.defaultShader )
	{
		Log::Warn("R_RemapShader: new shader %s not found", newShaderName );
		return;
	}

	if ( sh->autoSpriteMode != sh2->autoSpriteMode ) {
		Log::Warn("R_RemapShader: shaders %s and %s have different autoSprite modes", shaderName, newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension3( shaderName, strippedName, sizeof( strippedName ) );
	hash = generateHashValue( strippedName, FILE_HASH_SIZE );

	for ( sh = shaderHashTable[ hash ]; sh; sh = sh->next )
	{
		if ( Q_stricmp( sh->name, strippedName ) == 0 )
		{
			if ( sh != sh2 )
			{
				sh->remappedShader = sh2;
			}
			else
			{
				sh->remappedShader = nullptr;
			}
		}
	}
}

/*
===============
ParseVector
===============
*/
static bool ParseVector( const char **text, int count, float *v )
{
	char *token;
	int  i;

	token = COM_ParseExt2( text, false );

	if ( strcmp( token, "(" ) )
	{
		Log::Warn("missing parenthesis in shader '%s'", shader.name );
		return false;
	}

	for ( i = 0; i < count; i++ )
	{
		token = COM_ParseExt2( text, false );

		if ( !token[ 0 ] )
		{
			Log::Warn("missing vector element in shader '%s'", shader.name );
			return false;
		}

		v[ i ] = atof( token );
	}

	token = COM_ParseExt2( text, false );

	if ( strcmp( token, ")" ) )
	{
		Log::Warn("missing parenthesis in shader '%s'", shader.name );
		return false;
	}

	return true;
}

const opstring_t opStrings[] = {
	{"bad",                opcode_t::OP_BAD},

	{"&&",                 opcode_t::OP_LAND},
	{"||",                 opcode_t::OP_LOR},
	{">=",                 opcode_t::OP_GE},
	{"<=",                 opcode_t::OP_LE},
	{"==",                 opcode_t::OP_LEQ},
	{"!=",                 opcode_t::OP_LNE},

	{"+",                  opcode_t::OP_ADD},
	{"-",                  opcode_t::OP_SUB},
	{"/",                  opcode_t::OP_DIV},
	{"%",                  opcode_t::OP_MOD},
	{"*",                  opcode_t::OP_MUL},
	{"neg",                opcode_t::OP_NEG},

	{"<",                  opcode_t::OP_LT},
	{">",                  opcode_t::OP_GT},

	{"(",                  opcode_t::OP_LPAREN},
	{")",                  opcode_t::OP_RPAREN},
	{"[",                  opcode_t::OP_LBRACKET},
	{"]",                  opcode_t::OP_RBRACKET},

	{"c",                  opcode_t::OP_NUM},
	{"time",               opcode_t::OP_TIME},
	{"parm0",              opcode_t::OP_PARM0},
	{"parm1",              opcode_t::OP_PARM1},
	{"parm2",              opcode_t::OP_PARM2},
	{"parm3",              opcode_t::OP_PARM3},
	{"parm4",              opcode_t::OP_PARM4},
	{"parm5",              opcode_t::OP_PARM5},
	{"parm6",              opcode_t::OP_PARM6},
	{"parm7",              opcode_t::OP_PARM7},
	{"parm8",              opcode_t::OP_PARM8},
	{"parm9",              opcode_t::OP_PARM9},
	{"parm10",             opcode_t::OP_PARM10},
	{"parm11",             opcode_t::OP_PARM11},
	{"global0",            opcode_t::OP_GLOBAL0},
	{"global1",            opcode_t::OP_GLOBAL1},
	{"global2",            opcode_t::OP_GLOBAL2},
	{"global3",            opcode_t::OP_GLOBAL3},
	{"global4",            opcode_t::OP_GLOBAL4},
	{"global5",            opcode_t::OP_GLOBAL5},
	{"global6",            opcode_t::OP_GLOBAL6},
	{"global7",            opcode_t::OP_GLOBAL7},
	{"fragmentShaders",    opcode_t::OP_FRAGMENTSHADERS},
	{"frameBufferObjects", opcode_t::OP_FRAMEBUFFEROBJECTS},
	{"sound",              opcode_t::OP_SOUND},
	{"distance",           opcode_t::OP_DISTANCE},

	{"table",              opcode_t::OP_TABLE},

	{nullptr,              opcode_t::OP_BAD},
};

const char* GetOpName(opcode_t type)
{
	return opStrings[ Util::ordinal(type) ].s;
}

static void GetOpType( char *token, expOperation_t *op )
{
	const opstring_t *opString;
	char          tableName[ MAX_QPATH ];
	int           hash;
	shaderTable_t *tb;

	if ( ( token[ 0 ] >= '0' && token[ 0 ] <= '9' ) ||
	     //(token[0] == '-' && token[1] >= '0' && token[1] <= '9')   ||
	     //(token[0] == '+' && token[1] >= '0' && token[1] <= '9')   ||
	     ( token[ 0 ] == '.' && token[ 1 ] >= '0' && token[ 1 ] <= '9' ) )
	{
		op->type = opcode_t::OP_NUM;
		return;
	}

	Q_strncpyz( tableName, token, sizeof( tableName ) );
	hash = generateHashValue( tableName, MAX_SHADERTABLE_HASH );

	for ( tb = shaderTableHashTable[ hash ]; tb; tb = tb->next )
	{
		if ( Q_stricmp( tb->name, tableName ) == 0 )
		{
			// match found
			op->type = opcode_t::OP_TABLE;
			op->value = tb->index;
			return;
		}
	}

	for ( opString = opStrings; opString->s; opString++ )
	{
		if ( !Q_stricmp( token, opString->s ) )
		{
			op->type = opString->type;
			return;
		}
	}

	op->type = opcode_t::OP_BAD;
}

static bool IsOperand( opcode_t oc )
{
	switch ( oc )
	{
		case opcode_t::OP_NUM:
		case opcode_t::OP_TIME:
		case opcode_t::OP_PARM0:
		case opcode_t::OP_PARM1:
		case opcode_t::OP_PARM2:
		case opcode_t::OP_PARM3:
		case opcode_t::OP_PARM4:
		case opcode_t::OP_PARM5:
		case opcode_t::OP_PARM6:
		case opcode_t::OP_PARM7:
		case opcode_t::OP_PARM8:
		case opcode_t::OP_PARM9:
		case opcode_t::OP_PARM10:
		case opcode_t::OP_PARM11:
		case opcode_t::OP_GLOBAL0:
		case opcode_t::OP_GLOBAL1:
		case opcode_t::OP_GLOBAL2:
		case opcode_t::OP_GLOBAL3:
		case opcode_t::OP_GLOBAL4:
		case opcode_t::OP_GLOBAL5:
		case opcode_t::OP_GLOBAL6:
		case opcode_t::OP_GLOBAL7:
		case opcode_t::OP_FRAGMENTSHADERS:
		case opcode_t::OP_FRAMEBUFFEROBJECTS:
		case opcode_t::OP_SOUND:
		case opcode_t::OP_DISTANCE:
			return true;

		default:
			return false;
	}
}

static bool IsOperator( opcode_t oc )
{
	switch ( oc )
	{
		case opcode_t::OP_LAND:
		case opcode_t::OP_LOR:
		case opcode_t::OP_GE:
		case opcode_t::OP_LE:
		case opcode_t::OP_LEQ:
		case opcode_t::OP_LNE:
		case opcode_t::OP_ADD:
		case opcode_t::OP_SUB:
		case opcode_t::OP_DIV:
		case opcode_t::OP_MOD:
		case opcode_t::OP_MUL:
		case opcode_t::OP_NEG:
		case opcode_t::OP_LT:
		case opcode_t::OP_GT:
		case opcode_t::OP_TABLE:
			return true;

		default:
			return false;
	}
}

static int GetOpPrecedence( opcode_t oc )
{
	switch ( oc )
	{
		case opcode_t::OP_LOR:
			return 1;

		case opcode_t::OP_LAND:
			return 2;

		case opcode_t::OP_LEQ:
		case opcode_t::OP_LNE:
			return 3;

		case opcode_t::OP_GE:
		case opcode_t::OP_LE:
		case opcode_t::OP_LT:
		case opcode_t::OP_GT:
			return 4;

		case opcode_t::OP_ADD:
		case opcode_t::OP_SUB:
			return 5;

		case opcode_t::OP_DIV:
		case opcode_t::OP_MOD:
		case opcode_t::OP_MUL:
			return 6;

		case opcode_t::OP_NEG:
			return 7;

		case opcode_t::OP_TABLE:
			return 8;

		default:
			return 0;
	}
}

static char    *ParseExpressionElement( const char **data_p )
{
	int         c = 0, len;
	const char        *data;
	const char  *const *punc;
	static char token[ MAX_TOKEN_CHARS ];

	// multiple character punctuation tokens
	static const char *const punctuation[] =
	{
		"&&", "||", "<=", ">=", "==", "!=", nullptr
	};

	if ( !data_p )
	{
		ri.Error( errorParm_t::ERR_FATAL, "ParseExpressionElement: NULL data_p" );
	}

	data = *data_p;
	len = 0;
	token[ 0 ] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = nullptr;
		return token;
	}

	// skip whitespace
	while ( 1 )
	{
		// skip whitespace
		while ( ( c = *data ) <= ' ' )
		{
			if ( !c )
			{
				*data_p = nullptr;
				return token;
			}
			else if ( c == '\n' )
			{
				data++;
				*data_p = data;
				return token;
			}
			else
			{
				data++;
			}
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[ 1 ] == '/' )
		{
			data += 2;

			while ( *data && *data != '\n' )
			{
				data++;
			}
		}
		// skip /* */ comments
		else if ( c == '/' && data[ 1 ] == '*' )
		{
			data += 2;

			while ( *data && ( *data != '*' || data[ 1 ] != '/' ) )
			{
				data++;
			}

			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			// a real token to parse
			break;
		}
	}

	// handle quoted strings
	if ( c == '\"' )
	{
		data++;

		while ( 1 )
		{
			c = *data++;

			if ( ( c == '\\' ) && ( *data == '\"' ) )
			{
				// allow quoted strings to use \" to indicate the " character
				data++;
			}
			else if ( c == '\"' || !c )
			{
				token[ len ] = 0;
				*data_p = ( char * ) data;
				return token;
			}

			if ( len < MAX_TOKEN_CHARS - 1 )
			{
				token[ len ] = c;
				len++;
			}
		}
	}

	// check for a number
	if ( ( c >= '0' && c <= '9' ) ||
	     ( c == '.' && data[ 1 ] >= '0' && data[ 1 ] <= '9' ) )
	{
		do
		{
			if ( len < MAX_TOKEN_CHARS - 1 )
			{
				token[ len ] = c;
				len++;
			}

			data++;

			c = *data;
		}
		while ( ( c >= '0' && c <= '9' ) || c == '.' );

		// parse the exponent
		if ( c == 'e' || c == 'E' )
		{
			if ( len < MAX_TOKEN_CHARS - 1 )
			{
				token[ len ] = c;
				len++;
			}

			data++;
			c = *data;

			if ( c == '-' || c == '+' )
			{
				if ( len < MAX_TOKEN_CHARS - 1 )
				{
					token[ len ] = c;
					len++;
				}

				data++;
				c = *data;
			}

			do
			{
				if ( len < MAX_TOKEN_CHARS - 1 )
				{
					token[ len ] = c;
					len++;
				}

				data++;

				c = *data;
			}
			while ( c >= '0' && c <= '9' );
		}

		if ( len == MAX_TOKEN_CHARS )
		{
			len = 0;
		}

		token[ len ] = 0;

		*data_p = ( char * ) data;
		return token;
	}

	// check for a regular word
	if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c == '_' ) )
	{
		do
		{
			if ( len < MAX_TOKEN_CHARS - 1 )
			{
				token[ len ] = c;
				len++;
			}

			data++;

			c = *data;
		}
		while ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c == '_' ) || ( c >= '0' && c <= '9' ) );

		if ( len == MAX_TOKEN_CHARS )
		{
			len = 0;
		}

		token[ len ] = 0;

		*data_p = ( char * ) data;
		return token;
	}

	// check for multi-character punctuation token
	for ( punc = punctuation; *punc; punc++ )
	{
		int l;
		int j;

		l = strlen( *punc );

		for ( j = 0; j < l; j++ )
		{
			if ( data[ j ] != ( *punc ) [ j ] )
			{
				break;
			}
		}

		if ( j == l )
		{
			// a valid multi-character punctuation
			memcpy( token, *punc, l );
			token[ l ] = 0;
			data += l;
			*data_p = ( char * ) data;
			return token;
		}
	}

	// single character punctuation
	token[ 0 ] = *data;
	token[ 1 ] = 0;
	data++;
	*data_p = ( char * ) data;

	return token;
}

/*
===============
ParseExpression
===============
*/
static void ParseExpression( const char **text, expression_t *exp )
{
	int            i;
	char           *token;

	expOperation_t op, op2;

	expOperation_t inFixOps[ MAX_EXPRESSION_OPS ];
	int            numInFixOps;

	// convert stack
	expOperation_t tmpOps[ MAX_EXPRESSION_OPS ];
	int            numTmpOps;

	numInFixOps = 0;
	numTmpOps = 0;

	exp->numOps = 0;

	// push left parenthesis on the stack
	op.type = opcode_t::OP_LPAREN;
	op.value = 0;
	inFixOps[ numInFixOps++ ] = op;

	while ( 1 )
	{
		token = ParseExpressionElement( text );

		if ( token[ 0 ] == 0 || token[ 0 ] == ',' )
		{
			break;
		}

		if ( numInFixOps == MAX_EXPRESSION_OPS )
		{
			Log::Warn("too many arithmetic expression operations in shader '%s'", shader.name );
			SkipRestOfLine( text );
			return;
		}

		GetOpType( token, &op );

		switch ( op.type )
		{
			case opcode_t::OP_BAD:
				Log::Warn("unknown token '%s' for arithmetic expression in shader '%s'", token,
				           shader.name );
				break;

			case opcode_t::OP_LBRACKET:
				inFixOps[ numInFixOps++ ] = op;

				// add extra (
				op2.type = opcode_t::OP_LPAREN;
				op2.value = 0;
				inFixOps[ numInFixOps++ ] = op2;
				break;

			case opcode_t::OP_RBRACKET:
				// add extra )
				op2.type = opcode_t::OP_RPAREN;
				op2.value = 0;
				inFixOps[ numInFixOps++ ] = op2;

				inFixOps[ numInFixOps++ ] = op;
				break;

			case opcode_t::OP_NUM:
				op.value = atof( token );
				inFixOps[ numInFixOps++ ] = op;
				break;

			case opcode_t::OP_TABLE:
				// value already set by GetOpType
				inFixOps[ numInFixOps++ ] = op;
				break;

			default:
				op.value = 0;
				inFixOps[ numInFixOps++ ] = op;
				break;
		}
	}

	// push right parenthesis on the stack
	op.type = opcode_t::OP_RPAREN;
	op.value = 0;
	inFixOps[ numInFixOps++ ] = op;

	for ( i = 0; i < ( numInFixOps - 1 ); i++ )
	{
		op = inFixOps[ i ];
		op2 = inFixOps[ i + 1 ];

		// convert OP_SUBs that should be unary into OP_NEG
		if ( op2.type == opcode_t::OP_SUB && op.type != opcode_t::OP_RPAREN && op.type != opcode_t::OP_TABLE && !IsOperand( op.type ) )
		{
			inFixOps[ i + 1 ].type = opcode_t::OP_NEG;
		}
	}

	// http://cis.stvincent.edu/swd/stl/stacks/stacks.html
	// http://www.qiksearch.com/articles/cs/infix-postfix/
	// http://www.experts-exchange.com/Programming/Programming_Languages/C/Q_20394130.html

	//
	// convert infix representation to postfix
	//

	for ( i = 0; i < numInFixOps; i++ )
	{
		op = inFixOps[ i ];

		// if current operator in infix is digit
		if ( IsOperand( op.type ) )
		{
			exp->ops[ exp->numOps++ ] = op;
		}
		// if current operator in infix is left parenthesis
		else if ( op.type == opcode_t::OP_LPAREN )
		{
			tmpOps[ numTmpOps++ ] = op;
		}
		// if current operator in infix is operator
		else if ( IsOperator( op.type ) )
		{
			while ( true )
			{
				if ( !numTmpOps )
				{
					Log::Warn("invalid infix expression in shader '%s'", shader.name );
					return;
				}
				else
				{
					// get top element
					op2 = tmpOps[ numTmpOps - 1 ];

					if ( IsOperator( op2.type ) )
					{
						if ( GetOpPrecedence( op2.type ) >= GetOpPrecedence( op.type ) )
						{
							exp->ops[ exp->numOps++ ] = op2;
							numTmpOps--;
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
			}

			tmpOps[ numTmpOps++ ] = op;
		}
		// if current operator in infix is right parenthesis
		else if ( op.type == opcode_t::OP_RPAREN )
		{
			while ( true )
			{
				if ( !numTmpOps )
				{
					Log::Warn("invalid infix expression in shader '%s'", shader.name );
					return;
				}
				else
				{
					// get top element
					op2 = tmpOps[ numTmpOps - 1 ];

					if ( op2.type != opcode_t::OP_LPAREN )
					{
						exp->ops[ exp->numOps++ ] = op2;
						numTmpOps--;
					}
					else
					{
						numTmpOps--;
						break;
					}
				}
			}
		}
	}

	// everything went ok
	exp->active = true;
}

/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "GT0" ) )
	{
		return GLS_ATEST_GT_0;
	}
	else if ( !Q_stricmp( funcname, "LT128" ) )
	{
		return GLS_ATEST_LT_128;
	}
	else if ( !Q_stricmp( funcname, "GE128" ) )
	{
		return GLS_ATEST_GE_128;
	}
	else if ( !Q_stricmp( funcname, "LTENT" ) )
	{
		return GLS_ATEST_LT_ENT;
	}
	else if ( !Q_stricmp( funcname, "GTENT" ) )
	{
		return GLS_ATEST_GT_ENT;
	}

	Log::Warn("invalid alphaFunc name '%s' in shader '%s'", funcname, shader.name );
	return 0;
}

/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_SRCBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	Log::Warn("unknown blend mode '%s' in shader '%s', substituting GL_ONE", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_DSTBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	Log::Warn("unknown blend mode '%s' in shader '%s', substituting GL_ONE", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "sin" ) )
	{
		return genFunc_t::GF_SIN;
	}
	else if ( !Q_stricmp( funcname, "square" ) )
	{
		return genFunc_t::GF_SQUARE;
	}
	else if ( !Q_stricmp( funcname, "triangle" ) )
	{
		return genFunc_t::GF_TRIANGLE;
	}
	else if ( !Q_stricmp( funcname, "sawtooth" ) )
	{
		return genFunc_t::GF_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "inversesawtooth" ) )
	{
		return genFunc_t::GF_INVERSE_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "noise" ) )
	{
		return genFunc_t::GF_NOISE;
	}

	Log::Warn("invalid genfunc name '%s' in shader '%s'", funcname, shader.name );
	return genFunc_t::GF_SIN;
}

/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( const char **text, waveForm_t *wave )
{
	char *token;

	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("missing waveform parm in shader '%s'", shader.name );
		return;
	}

	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("missing waveform parm in shader '%s'", shader.name );
		return;
	}

	wave->base = atof( token );

	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("missing waveform parm in shader '%s'", shader.name );
		return;
	}

	wave->amplitude = atof( token );

	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("missing waveform parm in shader '%s'", shader.name );
		return;
	}

	wave->phase = atof( token );

	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("missing waveform parm in shader '%s'", shader.name );
		return;
	}

	wave->frequency = atof( token );
}

/*
===================
ParseTexMod
===================
*/
static bool ParseTexMod( const char **text, shaderStage_t *stage )
{
	const char   *token;
	texModInfo_t *tmi;

	if ( stage->bundle[ 0 ].numTexMods == TR_MAX_TEXMODS )
	{
		ri.Error(errorParm_t::ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
	}

	tmi = &stage->bundle[ 0 ].texMods[ stage->bundle[ 0 ].numTexMods ];
	stage->bundle[ 0 ].numTexMods++;

	token = COM_ParseExt2( text, false );

	// turb
	if ( !Q_stricmp( token, "turb" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing tcMod turb parms in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.base = atof( token );
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing tcMod turb in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.amplitude = atof( token );
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing tcMod turb in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.phase = atof( token );
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing tcMod turb in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.frequency = atof( token );

		tmi->type = texMod_t::TMOD_TURBULENT;
	}
	// scale
	else if ( !Q_stricmp( token, "scale" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing scale parms in shader '%s'", shader.name );
			return false;
		}

		tmi->scale[ 0 ] = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing scale parms in shader '%s'", shader.name );
			return false;
		}

		tmi->scale[ 1 ] = atof( token );
		tmi->type = texMod_t::TMOD_SCALE;
	}
	// scroll
	else if ( !Q_stricmp( token, "scroll" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing scale scroll parms in shader '%s'", shader.name );
			return false;
		}

		tmi->scroll[ 0 ] = atof( token );
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing scale scroll parms in shader '%s'", shader.name );
			return false;
		}

		tmi->scroll[ 1 ] = atof( token );
		tmi->type = texMod_t::TMOD_SCROLL;
	}
	// stretch
	else if ( !Q_stricmp( token, "stretch" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing stretch parms in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.func = NameToGenFunc( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing stretch parms in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.base = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing stretch parms in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.amplitude = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing stretch parms in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.phase = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing stretch parms in shader '%s'", shader.name );
			return false;
		}

		tmi->wave.frequency = atof( token );

		tmi->type = texMod_t::TMOD_STRETCH;
	}
	// transform
	else if ( !Q_stricmp( token, "transform" ) )
	{
		MatrixIdentity( tmi->matrix );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing transform parms in shader '%s'", shader.name );
			return false;
		}

		tmi->matrix[ 0 ] = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing transform parms in shader '%s'", shader.name );
			return false;
		}

		tmi->matrix[ 1 ] = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing transform parms in shader '%s'", shader.name );
			return false;
		}

		tmi->matrix[ 4 ] = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing transform parms in shader '%s'", shader.name );
			return false;
		}

		tmi->matrix[ 5 ] = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing transform parms in shader '%s'", shader.name );
			return false;
		}

		tmi->matrix[ 12 ] = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing transform parms in shader '%s'", shader.name );
			return false;
		}

		tmi->matrix[ 13 ] = atof( token );

		tmi->type = texMod_t::TMOD_TRANSFORM;
	}
	// rotate
	else if ( !Q_stricmp( token, "rotate" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing tcMod rotate parms in shader '%s'", shader.name );
			return false;
		}

		tmi->rotateSpeed = atof( token );
		tmi->type = texMod_t::TMOD_ROTATE;
	}
	// entityTranslate
	else if ( !Q_stricmp( token, "entityTranslate" ) )
	{
		tmi->type = texMod_t::TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		Log::Warn("unknown tcMod '%s' in shader '%s'", token, shader.name );
		return false;
	}

	return true;
}

static bool ParseMap( const char **text, char *buffer, int bufferSize )
{
	int  len;
	char *token;

	// examples
	// map textures/caves/tembrick1crum_local.tga
	// addnormals (textures/caves/tembrick1crum_local.tga, heightmap (textures/caves/tembrick1crum_bmp.tga, 3 ))
	// heightmap( textures/hell/hellbones_d07bbump.tga, 8)

	while ( 1 )
	{
		token = COM_ParseExt2( text, false );

		if ( !token[ 0 ] )
		{
			// end of line
			break;
		}

		Q_strcat( buffer, bufferSize, token );
		Q_strcat( buffer, bufferSize, " " );
	}

	if ( !buffer[ 0 ] )
	{
		Log::Warn("'map' missing parameter in shader '%s'", shader.name );
		return false;
	}

	len = strlen( buffer );
	buffer[ len - 1 ] = 0; // replace last ' ' with tailing zero

	return true;
}

static bool LoadMap( shaderStage_t *stage, const char *buffer )
{
	char         *token;
	int          imageBits = 0;
	filterType_t filterType;
	wrapType_t   wrapType;
	const char         *buffer_p = &buffer[ 0 ];

	if ( !buffer || !buffer[ 0 ] )
	{
		Log::Warn("NULL parameter for LoadMap in shader '%s'", shader.name );
		return false;
	}

	token = COM_ParseExt2( &buffer_p, false );

	if ( !Q_stricmp( token, "$whiteimage" ) || !Q_stricmp( token, "$white" ) || !Q_stricmp( token, "_white" ) ||
	     !Q_stricmp( token, "*white" ) )
	{
		stage->bundle[ 0 ].image[ 0 ] = tr.whiteImage;
		return true;
	}
	else if ( !Q_stricmp( token, "$blackimage" ) || !Q_stricmp( token, "$black" ) || !Q_stricmp( token, "_black" ) ||
	          !Q_stricmp( token, "*black" ) )
	{
		stage->bundle[ 0 ].image[ 0 ] = tr.blackImage;
		return true;
	}
	else if ( !Q_stricmp( token, "$flatimage" ) || !Q_stricmp( token, "$flat" ) || !Q_stricmp( token, "_flat" ) ||
	          !Q_stricmp( token, "*flat" ) )
	{
		stage->bundle[ 0 ].image[ 0 ] = tr.flatImage;
		return true;
	}

	else if ( !Q_stricmp( token, "$lightmap" ) )
	{
		stage->type = stageType_t::ST_LIGHTMAP;
		return true;
	}

	// determine image options
	if ( stage->overrideNoPicMip || shader.noPicMip || stage->highQuality || stage->forceHighQuality )
	{
		imageBits |= IF_NOPICMIP;
	}

	if ( stage->type == stageType_t::ST_NORMALMAP || stage->type == stageType_t::ST_HEATHAZEMAP || stage->type == stageType_t::ST_LIQUIDMAP )
	{
		imageBits |= IF_NORMALMAP;
	}

	if ( stage->type == stageType_t::ST_NORMALMAP && shader.parallax )
	{
		imageBits |= IF_DISPLACEMAP;
	}

	if ( stage->uncompressed || stage->highQuality || stage->forceHighQuality || shader.uncompressed )
	{
		imageBits |= IF_NOCOMPRESSION;
	}

	if ( stage->stateBits & ( GLS_ATEST_BITS ) )
	{
		imageBits |= IF_ALPHATEST;
	}

	if ( stage->overrideFilterType )
	{
		filterType = stage->filterType;
	}
	else
	{
		filterType = shader.filterType;
	}

	if ( stage->overrideWrapType )
	{
		wrapType = stage->wrapType;
	}
	else
	{
		wrapType = shader.wrapType;
	}

	// try to load the image
	stage->bundle[ 0 ].image[ 0 ] = R_FindImageFile( buffer, imageBits, filterType, wrapType );

	if ( !stage->bundle[ 0 ].image[ 0 ] )
	{
		Log::Warn("R_FindImageFile could not find image '%s' in shader '%s'", buffer, shader.name );
		return false;
	}

	return true;
}

/*
===================
ParseClampType
===================
*/
static bool ParseClampType( char *token, wrapType_t *clamp )
{
	bool s = true, t = true;
	wrapTypeEnum_t type;

	// handle prefixing with 'S' or 'T'
	switch ( token[ 0 ] & 0xDF )
	{
	case 'S': t = false; ++token; break;
	case 'T': s = false; ++token; break;
	}

	// get the clamp type
	if      ( !Q_stricmp( token, "clamp"          ) ) { type = wrapTypeEnum_t::WT_CLAMP; }
	else if ( !Q_stricmp( token, "edgeClamp"      ) ) { type = wrapTypeEnum_t::WT_EDGE_CLAMP; }
	else if ( !Q_stricmp( token, "zeroClamp"      ) ) { type = wrapTypeEnum_t::WT_ZERO_CLAMP; }
	else if ( !Q_stricmp( token, "alphaZeroClamp" ) ) { type = wrapTypeEnum_t::WT_ALPHA_ZERO_CLAMP; }
	else if ( !Q_stricmp( token, "noClamp"        ) ) { type = wrapTypeEnum_t::WT_REPEAT; }
	else // not recognised
	{
		return false;
	}

	if (s) { clamp->s = type; }
	if (t) { clamp->t = type; }

	return true;
}

/*
===================
ParseStage
===================
*/
static bool ParseStage( shaderStage_t *stage, const char **text )
{
	char         *token;
	int          colorMaskBits = 0;
	int          depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits =
	                               0, polyModeBits = 0;
	bool     depthMaskExplicit = false;
	int          imageBits = 0;
	filterType_t filterType;
	char         buffer[ 1024 ] = "";
	bool     loadMap = false;

	while ( 1 )
	{
		token = COM_ParseExt2( text, true );

		if ( !token[ 0 ] )
		{
			Log::Warn("no matching '}' found" );
			return false;
		}

		if ( token[ 0 ] == '}' )
		{
			break;
		}
		// if(<condition>)
		else if ( !Q_stricmp( token, "if" ) )
		{
			ParseExpression( text, &stage->ifExp );
		}
		// map <name>
		else if ( !Q_stricmp( token, "map" ) )
		{
			if ( !ParseMap( text, buffer, sizeof( buffer ) ) )
			{
				return false;
			}
			else
			{
				loadMap = true;
			}
		}
		// lightmap <name>
		else if ( !Q_stricmp( token, "lightmap" ) )
		{
			if ( !ParseMap( text, buffer, sizeof( buffer ) ) )
			{
				return false;
			}
			else
			{
				loadMap = true;
			}
		}
		// clampmap <name>
		else if ( !Q_stricmp( token, "clampmap" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parameter for 'clampmap' keyword in shader '%s'", shader.name );
				return false;
			}

			imageBits = 0;

			if ( stage->overrideNoPicMip || shader.noPicMip )
			{
				imageBits |= IF_NOPICMIP;
			}

			if ( stage->overrideFilterType )
			{
				filterType = stage->filterType;
			}
			else
			{
				filterType = shader.filterType;
			}

			stage->bundle[ 0 ].image[ 0 ] = R_FindImageFile( token, imageBits, filterType, wrapTypeEnum_t::WT_CLAMP );

			if ( !stage->bundle[ 0 ].image[ 0 ] )
			{
				Log::Warn("R_FindImageFile could not find '%s' in shader '%s'", token, shader.name );
				return false;
			}
		}
		// animMap <frequency> <image1> .... <imageN>
		else if ( !Q_stricmp( token, "animMap" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parameter for 'animMmap' keyword in shader '%s'", shader.name );
				return false;
			}

			stage->bundle[ 0 ].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 )
			{
				int num;

				token = COM_ParseExt2( text, false );

				if ( !token[ 0 ] )
				{
					break;
				}

				num = stage->bundle[ 0 ].numImages;

				if ( num < MAX_IMAGE_ANIMATIONS )
				{
					stage->bundle[ 0 ].image[ num ] = R_FindImageFile( token, IF_NONE, filterType_t::FT_DEFAULT, wrapTypeEnum_t::WT_REPEAT );

					if ( !stage->bundle[ 0 ].image[ num ] )
					{
						Log::Warn("R_FindImageFile could not find '%s' in shader '%s'", token,
						           shader.name );
						return false;
					}

					stage->bundle[ 0 ].numImages++;
				}
			}
		}
		else if ( !Q_stricmp( token, "videoMap" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parameter for 'videoMap' keyword in shader '%s'", shader.name );
				return false;
			}

			stage->bundle[ 0 ].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 512, 512, ( CIN_loop | CIN_silent | CIN_shader ) );

			if ( stage->bundle[ 0 ].videoMapHandle != -1 )
			{
				stage->bundle[ 0 ].isVideoMap = true;
				stage->bundle[ 0 ].image[ 0 ] = tr.scratchImage[ stage->bundle[ 0 ].videoMapHandle ];
			}
		}
		// cubeMap <map>
		else if ( !Q_stricmp( token, "cubeMap" ) || !Q_stricmp( token, "cameraCubeMap" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parameter for 'cubeMap' keyword in shader '%s'", shader.name );
				return false;
			}

			imageBits = 0;

			if ( stage->overrideNoPicMip || shader.noPicMip )
			{
				imageBits |= IF_NOPICMIP;
			}

			if ( stage->overrideFilterType )
			{
				filterType = stage->filterType;
			}
			else
			{
				filterType = shader.filterType;
			}

			stage->bundle[ 0 ].image[ 0 ] = R_FindCubeImage( token, imageBits, filterType, wrapTypeEnum_t::WT_EDGE_CLAMP );

			if ( !stage->bundle[ 0 ].image[ 0 ] )
			{
				Log::Warn("R_FindCubeImage could not find '%s' in shader '%s'", token, shader.name );
				return false;
			}
		}
		// alphafunc <func>
		else if ( !Q_stricmp( token, "alphaFunc" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parameter for 'alphaFunc' keyword in shader '%s'", shader.name );
				return false;
			}

			atestBits = NameToAFunc( token );
		}
		// alphaTest <exp>
		else if ( !Q_stricmp( token, "alphaTest" ) )
		{
			atestBits = GLS_ATEST_GE_128;
			ParseExpression( text, &stage->alphaTestExp );
		}
		// depthFunc <func>
		else if ( !Q_stricmp( token, "depthfunc" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parameter for 'depthfunc' keyword in shader '%s'", shader.name );
				return false;
			}

			if ( !Q_stricmp( token, "lequal" ) )
			{
				depthFuncBits = 0;
			}
			else if ( !Q_stricmp( token, "equal" ) )
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else
			{
				Log::Warn("unknown depthfunc '%s' in shader '%s'", token, shader.name );
				continue;
			}
		}
		// ignoreAlphaTest
		else if ( !Q_stricmp( token, "ignoreAlphaTest" ) )
		{
			depthFuncBits = 0;
		}
		// nearest
		else if ( !Q_stricmp( token, "nearest" ) )
		{
			stage->overrideFilterType = true;
			stage->filterType = filterType_t::FT_NEAREST;
		}
		// linear
		else if ( !Q_stricmp( token, "linear" ) )
		{
			stage->overrideFilterType = true;
			stage->filterType = filterType_t::FT_LINEAR;

			stage->overrideNoPicMip = true;
		}
		// noPicMip
		else if ( !Q_stricmp( token, "noPicMip" ) )
		{
			stage->overrideNoPicMip = true;
		}
		// clamp, edgeClamp etc.
		else if ( ParseClampType( token, &stage->wrapType ) )
		{
			stage->overrideWrapType = true;
		}
		// uncompressed
		else if ( !Q_stricmp( token, "uncompressed" ) )
		{
			stage->uncompressed = true;
		}
		// highQuality
		else if ( !Q_stricmp( token, "highQuality" ) )
		{
			stage->highQuality = true;
			stage->overrideNoPicMip = true;
		}
		// forceHighQuality
		else if ( !Q_stricmp( token, "forceHighQuality" ) )
		{
			stage->forceHighQuality = true;
			stage->overrideNoPicMip = true;
		}
		// ET fog
		else if ( !Q_stricmp( token, "fog" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing parm for fog in shader '%s'", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "on" ) )
			{
				stage->noFog = false;
			}
			else
			{
				stage->noFog = true;
			}
		}
		else if ( !Q_stricmp( token, "depthFade" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parameter for 'depthFade' keyword in shader '%s'", shader.name );
				return false;
			}

			stage->hasDepthFade = true;
			stage->depthFadeValue = atof( token );

			if ( stage->depthFadeValue <= 0.0f )
			{
				Log::Warn("depthFade parameter <= 0 in '%s'", shader.name );
				stage->depthFadeValue = 1.0f;
			}

		}
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		else if ( !Q_stricmp( token, "blendfunc" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing parm for blendFunc in shader '%s'", shader.name );
				continue;
			}

			// check for "simple" blends first
			if ( !Q_stricmp( token, "add" ) )
			{
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			else if ( !Q_stricmp( token, "filter" ) )
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if ( !Q_stricmp( token, "blend" ) )
			{
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
			else
			{
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( token );

				token = COM_ParseExt2( text, false );

				if ( token[ 0 ] == 0 )
				{
					Log::Warn("missing parm for blendFunc in shader '%s'", shader.name );
					continue;
				}

				blendDstBits = NameToDstBlendMode( token );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit )
			{
				depthMaskBits = 0;
			}
		}
		// blend <srcFactor> , <dstFactor>
		// or blend <add | filter | blend>
		// or blend <diffusemap | bumpmap | specularmap>
		else if ( !Q_stricmp( token, "blend" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing parm for blend in shader '%s'", shader.name );
				continue;
			}

			// check for "simple" blends first
			if ( !Q_stricmp( token, "add" ) )
			{
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			else if ( !Q_stricmp( token, "filter" ) )
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if ( !Q_stricmp( token, "modulate" ) )
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if ( !Q_stricmp( token, "blend" ) )
			{
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
			else if ( !Q_stricmp( token, "none" ) )
			{
				blendSrcBits = GLS_SRCBLEND_ZERO;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			// check for other semantic meanings
			else if ( !Q_stricmp( token, "diffuseMap" ) )
			{
				stage->type = stageType_t::ST_DIFFUSEMAP;
			}
			else if ( !Q_stricmp( token, "bumpMap" ) )
			{
				stage->type = stageType_t::ST_NORMALMAP;
			}
			else if ( !Q_stricmp( token, "specularMap" ) )
			{
				stage->type = stageType_t::ST_SPECULARMAP;
			}
			else if ( !Q_stricmp( token, "materialMap" ) )
			{
				stage->type = stageType_t::ST_MATERIALMAP;
			}
			else if ( !Q_stricmp( token, "glowMap" ) )
			{
				stage->type = stageType_t::ST_GLOWMAP;
			}
			else
			{
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( token );

				token = COM_ParseExt2( text, false );

				if ( token[ 0 ] != ',' )
				{
					Log::Warn("expecting ',', found '%s' instead for blend in shader '%s'", token,
					           shader.name );
					continue;
				}

				token = COM_ParseExt2( text, false );

				if ( token[ 0 ] == 0 )
				{
					Log::Warn("missing parm for blend in shader '%s'", shader.name );
					continue;
				}

				blendDstBits = NameToDstBlendMode( token );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit && stage->type == stageType_t::ST_COLORMAP )
			{
				depthMaskBits = 0;
			}
		}
		// stage <type>
		else if ( !Q_stricmp( token, "stage" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing parameters for stage in shader '%s'", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "colorMap" ) )
			{
				stage->type = stageType_t::ST_COLORMAP;
			}
			else if ( !Q_stricmp( token, "diffuseMap" ) )
			{
				stage->type = stageType_t::ST_DIFFUSEMAP;
			}
			else if ( !Q_stricmp( token, "normalMap" ) || !Q_stricmp( token, "bumpMap" ) )
			{
				stage->type = stageType_t::ST_NORMALMAP;
			}
			else if ( !Q_stricmp( token, "specularMap" ) )
			{
				stage->type = stageType_t::ST_SPECULARMAP;
			}
			else if ( !Q_stricmp( token, "materialMap" ) )
			{
				stage->type = stageType_t::ST_MATERIALMAP;
			}
			else if ( !Q_stricmp( token, "glowMap" ) )
			{
				stage->type = stageType_t::ST_GLOWMAP;
			}
			else if ( !Q_stricmp( token, "reflectionMap" ) )
			{
				stage->type = stageType_t::ST_REFLECTIONMAP;
			}
			else if ( !Q_stricmp( token, "refractionMap" ) )
			{
				stage->type = stageType_t::ST_REFRACTIONMAP;
			}
			else if ( !Q_stricmp( token, "dispersionMap" ) )
			{
				stage->type = stageType_t::ST_DISPERSIONMAP;
			}
			else if ( !Q_stricmp( token, "skyboxMap" ) )
			{
				stage->type = stageType_t::ST_SKYBOXMAP;
			}
			else if ( !Q_stricmp( token, "screenMap" ) )
			{
				stage->type = stageType_t::ST_SCREENMAP;
			}
			else if ( !Q_stricmp( token, "portalMap" ) )
			{
				stage->type = stageType_t::ST_PORTALMAP;
			}
			else if ( !Q_stricmp( token, "heathazeMap" ) )
			{
				stage->type = stageType_t::ST_HEATHAZEMAP;
			}
			else if ( !Q_stricmp( token, "liquidMap" ) )
			{
				stage->type = stageType_t::ST_LIQUIDMAP;
			}
			else if ( !Q_stricmp( token, "attenuationMapXY" ) )
			{
				stage->type = stageType_t::ST_ATTENUATIONMAP_XY;
			}
			else if ( !Q_stricmp( token, "attenuationMapZ" ) )
			{
				stage->type = stageType_t::ST_ATTENUATIONMAP_Z;
			}
			else
			{
				Log::Warn("unknown stage parameter '%s' in shader '%s'", token, shader.name );
				continue;
			}
		}
		// rgbGen
		else if ( !Q_stricmp( token, "rgbGen" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing parameters for rgbGen in shader '%s'", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->rgbWave );
				stage->rgbGen = colorGen_t::CGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				vec3_t color;

				ParseVector( text, 3, color );
				stage->constantColor = Color::Adapt( color );

				stage->rgbGen = colorGen_t::CGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->rgbGen = colorGen_t::CGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "identityLighting" ) )
			{
				stage->rgbGen = colorGen_t::CGEN_IDENTITY_LIGHTING;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->rgbGen = colorGen_t::CGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->rgbGen = colorGen_t::CGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->rgbGen = colorGen_t::CGEN_VERTEX;

				if ( stage->alphaGen == alphaGen_t::AGEN_IDENTITY )
				{
					stage->alphaGen = alphaGen_t::AGEN_VERTEX;
				}
			}
			else if ( !Q_stricmp( token, "exactVertex" ) )
			{
				stage->rgbGen = colorGen_t::CGEN_VERTEX;
			}
			else if ( !Q_stricmp( token, "lightingDiffuse" ) )
			{
				//Log::Warn("obsolete rgbGen lightingDiffuse keyword not supported in shader '%s'", shader.name);
				stage->type = stageType_t::ST_DIFFUSEMAP;
				stage->rgbGen = colorGen_t::CGEN_IDENTITY_LIGHTING;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->rgbGen = colorGen_t::CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				Log::Warn("unknown rgbGen parameter '%s' in shader '%s'", token, shader.name );
				continue;
			}
		}
		// rgb <arithmetic expression>
		else if ( !Q_stricmp( token, "rgb" ) )
		{
			stage->rgbGen = colorGen_t::CGEN_CUSTOM_RGB;
			ParseExpression( text, &stage->rgbExp );
		}
		// red <arithmetic expression>
		else if ( !Q_stricmp( token, "red" ) )
		{
			stage->rgbGen = colorGen_t::CGEN_CUSTOM_RGBs;
			ParseExpression( text, &stage->redExp );
		}
		// green <arithmetic expression>
		else if ( !Q_stricmp( token, "green" ) )
		{
			stage->rgbGen = colorGen_t::CGEN_CUSTOM_RGBs;
			ParseExpression( text, &stage->greenExp );
		}
		// blue <arithmetic expression>
		else if ( !Q_stricmp( token, "blue" ) )
		{
			stage->rgbGen = colorGen_t::CGEN_CUSTOM_RGBs;
			ParseExpression( text, &stage->blueExp );
		}
		// colored
		else if ( !Q_stricmp( token, "colored" ) )
		{
			stage->rgbGen = colorGen_t::CGEN_ENTITY;
			stage->alphaGen = alphaGen_t::AGEN_ENTITY;
		}
		// vertexColor
		else if ( !Q_stricmp( token, "vertexColor" ) )
		{
			stage->rgbGen = colorGen_t::CGEN_VERTEX;

			if ( stage->alphaGen == alphaGen_t::AGEN_IDENTITY )
			{
				stage->alphaGen = alphaGen_t::AGEN_VERTEX;
			}
		}
		// inverseVertexColor
		else if ( !Q_stricmp( token, "inverseVertexColor" ) )
		{
			stage->rgbGen = colorGen_t::CGEN_ONE_MINUS_VERTEX;

			if ( stage->alphaGen == alphaGen_t::AGEN_IDENTITY )
			{
				stage->alphaGen = alphaGen_t::AGEN_ONE_MINUS_VERTEX;
			}
		}
		// alphaGen
		else if ( !Q_stricmp( token, "alphaGen" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing parameters for alphaGen in shader '%s'", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = alphaGen_t::AGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				token = COM_ParseExt2( text, false );
				stage->constantColor.SetAlpha( 255 * atof( token ) );
				stage->alphaGen = alphaGen_t::AGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->alphaGen = alphaGen_t::AGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->alphaGen = alphaGen_t::AGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->alphaGen = alphaGen_t::AGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->alphaGen = alphaGen_t::AGEN_VERTEX;
			}
			else if ( !Q_stricmp( token, "lightingSpecular" ) )
			{
				Log::Warn("alphaGen lightingSpecular keyword not supported in shader '%s'", shader.name );
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->alphaGen = alphaGen_t::AGEN_ONE_MINUS_VERTEX;
			}
			else if ( !Q_stricmp( token, "portal" ) )
			{
				Log::Warn("alphaGen portal keyword not supported in shader '%s'", shader.name );
				//stage->type = ST_PORTALMAP;
				stage->alphaGen = alphaGen_t::AGEN_CONST;
				stage->constantColor.SetAlpha( 0 );
				SkipRestOfLine( text );
			}
			else
			{
				Log::Warn("unknown alphaGen parameter '%s' in shader '%s'", token, shader.name );
				continue;
			}
		}
		// alpha <arithmetic expression>
		else if ( !Q_stricmp( token, "alpha" ) )
		{
			stage->alphaGen = alphaGen_t::AGEN_CUSTOM;
			ParseExpression( text, &stage->alphaExp );
		}
		// color <exp>, <exp>, <exp>, <exp>
		else if ( !Q_stricmp( token, "color" ) )
		{
			stage->rgbGen = colorGen_t::CGEN_CUSTOM_RGBs;
			stage->alphaGen = alphaGen_t::AGEN_CUSTOM;
			ParseExpression( text, &stage->redExp );
			ParseExpression( text, &stage->greenExp );
			ParseExpression( text, &stage->blueExp );
			ParseExpression( text, &stage->alphaExp );
		}
		// tcGen <function>
		else if ( !Q_stricmp( token, "texGen" ) || !Q_stricmp( token, "tcGen" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing texGen parm in shader '%s'", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "environment" ) )
			{
				//Log::Warn("texGen environment keyword not supported in shader '%s'", shader.name);
				stage->tcGen_Environment = true;
			}
			else if ( !Q_stricmp( token, "lightmap" ) )
			{
				stage->tcGen_Lightmap = true;
			}
			else if ( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )
			{
			}
			else if ( !Q_stricmp( token, "reflect" ) )
			{
				stage->type = stageType_t::ST_REFLECTIONMAP;
			}
			else if ( !Q_stricmp( token, "skybox" ) )
			{
				stage->type = stageType_t::ST_SKYBOXMAP;
			}
			else
			{
				Log::Warn("unknown tcGen keyword '%s' not supported in shader '%s'", token, shader.name );
				SkipRestOfLine( text );
			}
		}
		// tcMod <type> <...>
		else if ( !Q_stricmp( token, "tcMod" ) )
		{
			if ( !ParseTexMod( text, stage ) )
			{
				return false;
			}
		}
		// scroll
		else if ( !Q_stricmp( token, "scroll" ) || !Q_stricmp( token, "translate" ) )
		{
			texModInfo_t *tmi;

			if ( stage->bundle[ 0 ].numTexMods == TR_MAX_TEXMODS )
			{
				ri.Error(errorParm_t::ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
			}

			tmi = &stage->bundle[ 0 ].texMods[ stage->bundle[ 0 ].numTexMods ];
			stage->bundle[ 0 ].numTexMods++;

			ParseExpression( text, &tmi->sExp );
			ParseExpression( text, &tmi->tExp );

			tmi->type = texMod_t::TMOD_SCROLL2;
		}
		// scale
		else if ( !Q_stricmp( token, "scale" ) )
		{
			texModInfo_t *tmi;

			if ( stage->bundle[ 0 ].numTexMods == TR_MAX_TEXMODS )
			{
				ri.Error(errorParm_t::ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
			}

			tmi = &stage->bundle[ 0 ].texMods[ stage->bundle[ 0 ].numTexMods ];
			stage->bundle[ 0 ].numTexMods++;

			ParseExpression( text, &tmi->sExp );
			ParseExpression( text, &tmi->tExp );

			tmi->type = texMod_t::TMOD_SCALE2;
		}
		// centerScale
		else if ( !Q_stricmp( token, "centerScale" ) )
		{
			texModInfo_t *tmi;

			if ( stage->bundle[ 0 ].numTexMods == TR_MAX_TEXMODS )
			{
				ri.Error(errorParm_t::ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
			}

			tmi = &stage->bundle[ 0 ].texMods[ stage->bundle[ 0 ].numTexMods ];
			stage->bundle[ 0 ].numTexMods++;

			ParseExpression( text, &tmi->sExp );
			ParseExpression( text, &tmi->tExp );

			tmi->type = texMod_t::TMOD_CENTERSCALE;
		}
		// shear
		else if ( !Q_stricmp( token, "shear" ) )
		{
			texModInfo_t *tmi;

			if ( stage->bundle[ 0 ].numTexMods == TR_MAX_TEXMODS )
			{
				ri.Error(errorParm_t::ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
			}

			tmi = &stage->bundle[ 0 ].texMods[ stage->bundle[ 0 ].numTexMods ];
			stage->bundle[ 0 ].numTexMods++;

			ParseExpression( text, &tmi->sExp );
			ParseExpression( text, &tmi->tExp );

			tmi->type = texMod_t::TMOD_SHEAR;
		}
		// rotate
		else if ( !Q_stricmp( token, "rotate" ) )
		{
			texModInfo_t *tmi;

			if ( stage->bundle[ 0 ].numTexMods == TR_MAX_TEXMODS )
			{
				ri.Error(errorParm_t::ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
			}

			tmi = &stage->bundle[ 0 ].texMods[ stage->bundle[ 0 ].numTexMods ];
			stage->bundle[ 0 ].numTexMods++;

			ParseExpression( text, &tmi->rExp );

			tmi->type = texMod_t::TMOD_ROTATE2;
		}
		// depthwrite
		else if ( !Q_stricmp( token, "depthwrite" ) )
		{
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = true;
			continue;
		}
		// maskRed
		else if ( !Q_stricmp( token, "maskRed" ) )
		{
			colorMaskBits |= GLS_REDMASK_FALSE;
		}
		// maskGreen
		else if ( !Q_stricmp( token, "maskGreen" ) )
		{
			colorMaskBits |= GLS_GREENMASK_FALSE;
		}
		// maskBlue
		else if ( !Q_stricmp( token, "maskBlue" ) )
		{
			colorMaskBits |= GLS_BLUEMASK_FALSE;
		}
		// maskAlpha
		else if ( !Q_stricmp( token, "maskAlpha" ) )
		{
			colorMaskBits |= GLS_ALPHAMASK_FALSE;
		}
		// maskColor
		else if ( !Q_stricmp( token, "maskColor" ) )
		{
			colorMaskBits |= GLS_REDMASK_FALSE | GLS_GREENMASK_FALSE | GLS_BLUEMASK_FALSE;
		}
		// maskColorAlpha
		else if ( !Q_stricmp( token, "maskColorAlpha" ) )
		{
			colorMaskBits |= GLS_REDMASK_FALSE | GLS_GREENMASK_FALSE | GLS_BLUEMASK_FALSE | GLS_ALPHAMASK_FALSE;
		}
		// maskDepth
		else if ( !Q_stricmp( token, "maskDepth" ) )
		{
			depthMaskBits &= ~GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = false;
		}
		// wireFrame
		else if ( !Q_stricmp( token, "wireFrame" ) )
		{
			polyModeBits |= GLS_POLYMODE_LINE;
		}
		else if ( !Q_stricmp( token, "specularExponentMin" ) )
		{
			ParseExpression( text, &stage->specularExponentMin );
		}
		else if ( !Q_stricmp( token, "specularExponentMax" ) )
		{
			ParseExpression( text, &stage->specularExponentMax );
		}
		// refractionIndex <arithmetic expression>
		else if ( !Q_stricmp( token, "refractionIndex" ) )
		{
			ParseExpression( text, &stage->refractionIndexExp );
		}
		// fresnelPower <arithmetic expression>
		else if ( !Q_stricmp( token, "fresnelPower" ) )
		{
			ParseExpression( text, &stage->fresnelPowerExp );
		}
		// fresnelScale <arithmetic expression>
		else if ( !Q_stricmp( token, "fresnelScale" ) )
		{
			ParseExpression( text, &stage->fresnelScaleExp );
		}
		// fresnelBias <arithmetic expression>
		else if ( !Q_stricmp( token, "fresnelBias" ) )
		{
			ParseExpression( text, &stage->fresnelBiasExp );
		}
		// normalScale <arithmetic expression>
		else if ( !Q_stricmp( token, "normalScale" ) )
		{
			ParseExpression( text, &stage->normalScaleExp );
		}
		// fogDensity <arithmetic expression>
		else if ( !Q_stricmp( token, "fogDensity" ) )
		{
			ParseExpression( text, &stage->fogDensityExp );
		}
		// depthScale <arithmetic expression>
		else if ( !Q_stricmp( token, "depthScale" ) )
		{
			ParseExpression( text, &stage->depthScaleExp );
		}
		// deformMagnitude <arithmetic expression>
		else if ( !Q_stricmp( token, "deformMagnitude" ) )
		{
			ParseExpression( text, &stage->deformMagnitudeExp );
		}
		// wrapAroundLighting <arithmetic expression>
		else if ( !Q_stricmp( token, "wrapAroundLighting" ) )
		{
			ParseExpression( text, &stage->wrapAroundLightingExp );
		}
		else
		{
			Log::Warn("unknown shader stage parameter '%s' in shader '%s'", token, shader.name );
			SkipRestOfLine( text );
			return false;
		}
	}

	// parsing succeeded
	stage->active = true;

	// if cgen isn't explicitly specified, use either identity or identitylighting
	if ( stage->rgbGen == colorGen_t::CGEN_BAD )
	{
		if ( blendSrcBits == 0 || blendSrcBits == GLS_SRCBLEND_ONE || blendSrcBits == GLS_SRCBLEND_SRC_ALPHA )
		{
			stage->rgbGen = colorGen_t::CGEN_IDENTITY_LIGHTING;
		}
		else
		{
			stage->rgbGen = colorGen_t::CGEN_IDENTITY;
		}
	}

	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ZERO ) )
	{
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// tell shader if this stage has an alpha test
	if ( atestBits & GLS_ATEST_BITS )
	{
		shader.alphaTest = true;
	}

	// check that depthFade and depthWrite are mutually exclusive
	if ( depthMaskBits && stage->hasDepthFade ) {
		Log::Warn( "depth fade conflicts with depth mask in shader '%s'\n", shader.name );
		stage->hasDepthFade = false;
	}

	// compute state bits
	stage->stateBits = colorMaskBits | depthMaskBits | blendSrcBits | blendDstBits | atestBits | depthFuncBits | polyModeBits;

	// load image
	if ( loadMap && !LoadMap( stage, buffer ) )
	{
		return false;
	}

	return true;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes rotgrow <growSpeed> <rotSlices> <rotSpeed>
deformVertexes autoSprite
deformVertexes autoSprite2
===============
*/
static void ParseDeform( const char **text )
{
	char          *token;
	deformStage_t *ds;

	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("missing deform parm in shader '%s'", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS )
	{
		Log::Warn("MAX_SHADER_DEFORMS in '%s'", shader.name );
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if ( !Q_stricmp( token, "autosprite" ) )
	{
		shader.autoSpriteMode = 1;
		shader.numDeforms--;
		return;
	}

	if ( !Q_stricmp( token, "autosprite2" ) )
	{
		shader.autoSpriteMode = 2;
		shader.numDeforms--;
		return;
	}

	if ( !Q_stricmp( token, "bulge" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing deformVertexes bulge parm in shader '%s'", shader.name );
			return;
		}

		ds->bulgeWidth = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing deformVertexes bulge parm in shader '%s'", shader.name );
			return;
		}

		ds->bulgeHeight = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing deformVertexes bulge parm in shader '%s'", shader.name );
			return;
		}

		ds->bulgeSpeed = atof( token );

		ds->deformation = deform_t::DEFORM_BULGE;
		return;
	}

	if ( !Q_stricmp( token, "wave" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing deformVertexes parm in shader '%s'", shader.name );
			return;
		}

		if ( atof( token ) != 0 )
		{
			ds->deformationSpread = 1.0f / atof( token );
		}
		else
		{
			ds->deformationSpread = 100.0f;
			Log::Warn("illegal div value of 0 in deformVertexes command for shader '%s'", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = deform_t::DEFORM_WAVE;
		return;
	}

	if ( !Q_stricmp( token, "normal" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing deformVertexes parm in shader '%s'", shader.name );
			return;
		}

		ds->deformationWave.amplitude = atof( token );

		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing deformVertexes parm in shader '%s'", shader.name );
			return;
		}

		ds->deformationWave.frequency = atof( token );

		ds->deformation = deform_t::DEFORM_NORMALS;
		return;
	}

	if ( !Q_stricmp( token, "move" ) )
	{
		int i;

		for ( i = 0; i < 3; i++ )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing deformVertexes parm in shader '%s'", shader.name );
				return;
			}

			ds->moveVector[ i ] = atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = deform_t::DEFORM_MOVE;
		return;
	}

	if ( !Q_stricmp( token, "rotgrow" ) )
	{
		int i;

		for ( i = 0; i < 3; i++ )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing deformVertexes parm in shader '%s'", shader.name );
				return;
			}

			ds->moveVector[ i ] = atof( token );
		}

		ds->deformation = deform_t::DEFORM_ROTGROW;
		return;
	}

	if ( !Q_stricmp( token, "sprite" ) )
	{
		shader.autoSpriteMode = 1;
		shader.numDeforms--;
		return;
	}

	if ( !Q_stricmp( token, "flare" ) )
	{
		token = COM_ParseExt2( text, false );

		if ( token[ 0 ] == 0 )
		{
			Log::Warn("missing deformVertexes flare parm in shader '%s'", shader.name );
			return;
		}

		ds->flareSize = atof( token );
		return;
	}

	Log::Warn("unknown deformVertexes subtype '%s' found in shader '%s'", token, shader.name );
}

/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( const char **text )
{
	char *token;
	char prefix[ MAX_QPATH ];

	// outerbox
	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("'skyParms' missing parameter in shader '%s'", shader.name );
		return;
	}

	if ( strcmp( token, "-" ) )
	{
		Q_strncpyz( prefix, token, sizeof( prefix ) );

		shader.sky.outerbox = R_FindCubeImage( prefix, IF_NONE, filterType_t::FT_DEFAULT, wrapTypeEnum_t::WT_EDGE_CLAMP );

		if ( !shader.sky.outerbox )
		{
			Log::Warn("could not find cubemap '%s' for outer skybox in shader '%s'", prefix, shader.name );
			shader.sky.outerbox = tr.blackCubeImage;
		}
	}

	// cloudheight
	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("'skyParms' missing parameter in shader '%s'", shader.name );
		return;
	}

	shader.sky.cloudHeight = atof( token );

	if ( !shader.sky.cloudHeight )
	{
		shader.sky.cloudHeight = 512;
	}

	R_InitSkyTexCoords( shader.sky.cloudHeight );

	// innerbox
	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("'skyParms' missing parameter in shader '%s'", shader.name );
		return;
	}

	if ( strcmp( token, "-" ) )
	{
		Q_strncpyz( prefix, token, sizeof( prefix ) );

		shader.sky.innerbox = R_FindCubeImage( prefix, IF_NONE, filterType_t::FT_DEFAULT, wrapTypeEnum_t::WT_EDGE_CLAMP );

		if ( !shader.sky.innerbox )
		{
			Log::Warn("could not find cubemap '%s' for inner skybox in shader '%s'", prefix, shader.name );
			shader.sky.innerbox = tr.blackCubeImage;
		}
	}

	shader.isSky = true;
}

/*
=================
ParseSort
=================
*/
static void ParseSort( const char **text )
{
	char *token;

	token = COM_ParseExt2( text, false );

	if ( token[ 0 ] == 0 )
	{
		Log::Warn("missing sort parameter in shader '%s'", shader.name );
		return;
	}

	if ( !Q_stricmp( token, "portal" ) || !Q_stricmp( token, "subview" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_PORTAL);
	}
	else if ( !Q_stricmp( token, "sky" ) || !Q_stricmp( token, "environment" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_ENVIRONMENT_FOG);
	}
	else if ( !Q_stricmp( token, "opaque" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_OPAQUE);
	}
	else if ( !Q_stricmp( token, "decal" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_DECAL);
	}
	else if ( !Q_stricmp( token, "seeThrough" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_SEE_THROUGH);
	}
	else if ( !Q_stricmp( token, "banner" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_BANNER);
	}
	else if ( !Q_stricmp( token, "underwater" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_UNDERWATER);
	}
	else if ( !Q_stricmp( token, "far" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_FAR);
	}
	else if ( !Q_stricmp( token, "medium" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_MEDIUM);
	}
	else if ( !Q_stricmp( token, "close" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_CLOSE);
	}
	else if ( !Q_stricmp( token, "additive" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_BLEND1);
	}
	else if ( !Q_stricmp( token, "almostNearest" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_ALMOST_NEAREST);
	}
	else if ( !Q_stricmp( token, "nearest" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_NEAREST);
	}
	else if ( !Q_stricmp( token, "postProcess" ) )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_POST_PROCESS);
	}
	else
	{
		shader.sort = atof( token );
	}
}

// this table is also present in xmap

struct infoParm_t
{
	const char *name;
	int  clearSolid, surfaceFlags, contents;
};

// *INDENT-OFF*
static const infoParm_t infoParms[] =
{
	// server relevant contents

	{ "clipmissile",        1,                         0,                      CONTENTS_MISSILECLIP                   }, // impact only specific weapons (rl, gl)
	{ "water",              1,                         0,                      CONTENTS_WATER                         },
	{ "slag",               1,                         0,                      CONTENTS_SLIME                         }, // uses the CONTENTS_SLIME flag, but the shader reference is changed to 'slag'
	// to idendify that this doesn't work the same as 'slime' did.

	{ "lava",               1,                         0,                      CONTENTS_LAVA                          }, // very damaging
	{ "playerclip",         1,                         0,                      CONTENTS_PLAYERCLIP                    },
	{ "monsterclip",        1,                         0,                      CONTENTS_MONSTERCLIP                   },
	{ "nodrop",             1,                         0,                      int(CONTENTS_NODROP)                   }, // don't drop items or leave bodies (death fog, lava, etc)
	{ "nonsolid",           1,                         SURF_NONSOLID,          0                                      }, // clears the solid flag

	{ "blood",              1,                         0,                      CONTENTS_WATER                         },

	// utility relevant attributes
	{ "origin",             1,                         0,                      CONTENTS_ORIGIN                        }, // center of rotating brushes

	{ "trans",              0,                         0,                      CONTENTS_TRANSLUCENT                   }, // don't eat contained surfaces
	{ "translucent",        0,                         0,                      CONTENTS_TRANSLUCENT                   }, // don't eat contained surfaces

	{ "detail",             0,                         0,                      CONTENTS_DETAIL                        }, // don't include in structural bsp
	{ "structural",         0,                         0,                      CONTENTS_STRUCTURAL                    }, // force into structural bsp even if trnas
	{ "areaportal",         1,                         0,                      CONTENTS_AREAPORTAL                    }, // divides areas
	{ "clusterportal",      1,                         0,                      CONTENTS_CLUSTERPORTAL                 }, // for bots
	{ "donotenter",         1,                         0,                      CONTENTS_DONOTENTER                    }, // for bots

	{ "donotenterlarge",    1,                         0,                      CONTENTS_DONOTENTER_LARGE              }, // for larger bots
	{ "fog",                1,                         0,                      CONTENTS_FOG                           }, // carves surfaces entering
	{ "sky",                0,                         SURF_SKY,               0                                      }, // emit light from an environment map
	{ "lightfilter",        0,                         SURF_LIGHTFILTER,       0                                      }, // filter light going through it
	{ "alphashadow",        0,                         SURF_ALPHASHADOW,       0                                      }, // test light on a per-pixel basis
	{ "hint",               0,                         SURF_HINT,              0                                      }, // use as a primary splitter

	// server attributes
	{ "slick",              0,                         SURF_SLICK,             0                                      },

	{ "noimpact",           0,                         SURF_NOIMPACT,          0                                      }, // don't make impact explosions or marks

	{ "nomarks",            0,                         SURF_NOMARKS,           0                                      }, // don't make impact marks, but still explode
	{ "nooverlays",         0,                         SURF_NOMARKS,           0                                      }, // don't make impact marks, but still explode

	{ "ladder",             0,                         SURF_LADDER,            0                                      },

	{ "nodamage",           0,                         SURF_NODAMAGE,          0                                      },

	{ "monsterslick",       0,                         SURF_MONSTERSLICK,      0                                      }, // surf only slick for monsters

	{ "glass",              0,                         SURF_GLASS,             0                                      }, //----(SA) added
	{ "splash",             0,                         SURF_SPLASH,            0                                      }, //----(SA) added

	// steps
	{ "metal",              0,                         SURF_METAL,             0                                      },
	{ "metalsteps",         0,                         SURF_METAL,             0                                      },

	{ "nosteps",            0,                         SURF_NOSTEPS,           0                                      },
	{ "woodsteps",          0,                         SURF_WOOD,              0                                      },
	{ "grasssteps",         0,                         SURF_GRASS,             0                                      },
	{ "gravelsteps",        0,                         SURF_GRAVEL,            0                                      },
	{ "carpetsteps",        0,                         SURF_CARPET,            0                                      },
	{ "snowsteps",          0,                         SURF_SNOW,              0                                      },
	{ "roofsteps",          0,                         SURF_ROOF,              0                                      }, // tile roof

	{ "rubble",             0,                         SURF_RUBBLE,            0                                      },

	// drawsurf attributes
	{ "nodraw",             0,                         SURF_NODRAW,            0                                      }, // don't generate a drawsurface (or a lightmap)
	{ "pointlight",         0,                         SURF_POINTLIGHT,        0                                      }, // sample lighting at vertexes
	{ "nolightmap",         0,                         SURF_NOLIGHTMAP,        0                                      }, // don't generate a lightmap
	{ "nodlight",           0,                         0,                      0                                      }, // OBSELETE: don't ever add dynamic lights

	// monster ai
	{ "monsterslicknorth",  0,                         SURF_MONSLICK_N,        0                                      },
	{ "monsterslickeast",   0,                         SURF_MONSLICK_E,        0                                      },
	{ "monsterslicksouth",  0,                         SURF_MONSLICK_S,        0                                      },
	{ "monsterslickwest",   0,                         SURF_MONSLICK_W,        0                                      },

	// unsupported Doom3 surface types for sound effects and blood splats
	{ "metal",              0,                         SURF_METAL,             0                                      },

	{ "stone",              0,                         0,                      0                                      },
	{ "wood",               0,                         SURF_WOOD,              0                                      },
	{ "cardboard",          0,                         0,                      0                                      },
	{ "liquid",             0,                         0,                      0                                      },
	{ "glass",              0,                         0,                      0                                      },
	{ "plastic",            0,                         0,                      0                                      },
	{ "ricochet",           0,                         0,                      0                                      },
	{ "surftype10",         0,                         0,                      0                                      },
	{ "surftype11",         0,                         0,                      0                                      },
	{ "surftype12",         0,                         0,                      0                                      },
	{ "surftype13",         0,                         0,                      0                                      },
	{ "surftype14",         0,                         0,                      0                                      },
	{ "surftype15",         0,                         0,                      0                                      },

	// other unsupported Doom3 surface types
	{ "trigger",            0,                         0,                      0                                      },
	{ "flashlight_trigger", 0,                         0,                      0                                      },
	{ "aassolid",           0,                         0,                      0                                      },
	{ "aasobstacle",        0,                         0,                      0                                      },
	{ "nullNormal",         0,                         0,                      0                                      },
	{ "discrete",           0,                         0,                      0                                      },
};
// *INDENT-ON*

/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static bool SurfaceParm( const char *token )
{
	int numInfoParms = ARRAY_LEN( infoParms );
	int i;

	for ( i = 0; i < numInfoParms; i++ )
	{
		if ( !Q_stricmp( token, infoParms[ i ].name ) )
		{
			shader.surfaceFlags |= infoParms[ i ].surfaceFlags;
			shader.contentFlags |= infoParms[ i ].contents;
			return true;
		}
	}

	return false;
}

static void ParseSurfaceParm( const char **text )
{
	char *token;

	token = COM_ParseExt2( text, false );
	SurfaceParm( token );
}

static void ParseDiffuseMap( shaderStage_t *stage, const char **text )
{
	char buffer[ 1024 ] = "";

	stage->active = true;
	stage->type = stageType_t::ST_DIFFUSEMAP;
	stage->rgbGen = colorGen_t::CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;

	if ( !r_compressDiffuseMaps->integer )
	{
		stage->uncompressed = true;
	}

	if ( ParseMap( text, buffer, sizeof( buffer ) ) )
	{
		LoadMap( stage, buffer );
	}
}

static void ParseNormalMap( shaderStage_t *stage, const char **text )
{
	char buffer[ 1024 ] = "";

	stage->active = true;
	stage->type = stageType_t::ST_NORMALMAP;
	stage->rgbGen = colorGen_t::CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;

	if ( !r_compressNormalMaps->integer )
	{
		stage->uncompressed = true;
	}

	if ( r_highQualityNormalMapping->integer )
	{
		stage->overrideFilterType = true;
		stage->filterType = filterType_t::FT_LINEAR;

		stage->overrideNoPicMip = true;
	}

	if ( ParseMap( text, buffer, sizeof( buffer ) ) )
	{
		LoadMap( stage, buffer );
	}
}

static void ParseSpecularMap( shaderStage_t *stage, const char **text )
{
	char buffer[ 1024 ] = "";

	stage->active = true;
	stage->type = stageType_t::ST_SPECULARMAP;
	stage->rgbGen = colorGen_t::CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;

	if ( !r_compressSpecularMaps->integer )
	{
		stage->uncompressed = true;
	}

	if ( ParseMap( text, buffer, sizeof( buffer ) ) )
	{
		LoadMap( stage, buffer );
	}
}

static void ParseMaterialMap( shaderStage_t *stage, const char **text )
{
	char buffer[ 1024 ] = "";

	stage->active = true;
	stage->type = stageType_t::ST_MATERIALMAP;
	stage->rgbGen = colorGen_t::CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;

	if ( !r_compressSpecularMaps->integer )
	{
		stage->uncompressed = true;
	}

	if ( ParseMap( text, buffer, sizeof( buffer ) ) )
	{
		LoadMap( stage, buffer );
	}
}

static void ParseGlowMap( shaderStage_t *stage, const char **text )
{
	char buffer[ 1024 ] = "";

	stage->active = true;
	stage->type = stageType_t::ST_GLOWMAP;
	stage->rgbGen = colorGen_t::CGEN_IDENTITY;
	stage->stateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE; // blend add

	if ( ParseMap( text, buffer, sizeof( buffer ) ) )
	{
		LoadMap( stage, buffer );
	}
}

static void ParseReflectionMap( shaderStage_t *stage, const char **text )
{
	char buffer[ 1024 ] = "";

	stage->active = true;
	stage->type = stageType_t::ST_REFLECTIONMAP;
	stage->rgbGen = colorGen_t::CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;
	stage->overrideWrapType = true;
	stage->wrapType = wrapTypeEnum_t::WT_EDGE_CLAMP;

	if ( ParseMap( text, buffer, sizeof( buffer ) ) )
	{
		LoadMap( stage, buffer );
	}
}

static void ParseReflectionMapBlended( shaderStage_t *stage, const char **text )
{
	char buffer[ 1024 ] = "";

	stage->active = true;
	stage->type = stageType_t::ST_REFLECTIONMAP;
	stage->rgbGen = colorGen_t::CGEN_IDENTITY;
	stage->stateBits = GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE;
	stage->overrideWrapType = true;
	stage->wrapType = wrapTypeEnum_t::WT_EDGE_CLAMP;

	if ( ParseMap( text, buffer, sizeof( buffer ) ) )
	{
		LoadMap( stage, buffer );
	}
}

static void ParseLightFalloffImage( shaderStage_t *stage, const char **text )
{
	char buffer[ 1024 ] = "";

	stage->active = true;
	stage->type = stageType_t::ST_ATTENUATIONMAP_Z;
	stage->rgbGen = colorGen_t::CGEN_IDENTITY;
	stage->stateBits = GLS_DEFAULT;
	stage->overrideWrapType = true;
	stage->wrapType = wrapTypeEnum_t::WT_EDGE_CLAMP;

	if ( ParseMap( text, buffer, sizeof( buffer ) ) )
	{
		LoadMap( stage, buffer );
	}
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static bool ParseShader( const char *_text )
{
	const char **text;
	char *token;
	int  s;

	s = 0;
	text = &_text;

	token = COM_ParseExt2( text, true );

	if ( token[ 0 ] != '{' )
	{
		Log::Warn("no preceding '}' in shader %s", shader.name );
		return false;
	}

	while ( 1 )
	{
		token = COM_ParseExt2( text, true );

		if ( !token[ 0 ] )
		{
			Log::Warn("no concluding '}' in shader %s", shader.name );
			return false;
		}

		// end of shader definition
		if ( token[ 0 ] == '}' )
		{
			break;
		}
		// stage definition
		else if ( token[ 0 ] == '{' )
		{
			if ( s >= ( MAX_SHADER_STAGES - 1 ) )
			{
				Log::Warn("too many stages in shader %s", shader.name );
				return false;
			}

			if ( !ParseStage( &stages[ s ], text ) )
			{
				return false;
			}

			stages[ s ].active = true;
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_strnicmp( token, "qer", 3 ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		// skip description
		else if ( !Q_stricmp( token, "description" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		// skip renderbump
		else if ( !Q_stricmp( token, "renderbump" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		// skip guiSurf
		else if ( !Q_stricmp( token, "guiSurf" ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		// sun parms
		else if ( !Q_stricmp( token, "xmap_sun" ) ||
		          !Q_stricmp( token, "q3map_sun" ) )
		{
			float a, b;

			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parm for 'xmap_sun' keyword in shader '%s'", shader.name );
				continue;
			}

			tr.sunLight[ 0 ] = atof( token );

			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parm for 'xmap_sun' keyword in shader '%s'", shader.name );
				continue;
			}

			tr.sunLight[ 1 ] = atof( token );

			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parm for 'xmap_sun' keyword in shader '%s'", shader.name );
				continue;
			}

			tr.sunLight[ 2 ] = atof( token );

			VectorNormalize( tr.sunLight );

			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parm for 'xmap_sun' keyword in shader '%s'", shader.name );
				continue;
			}

			a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight );

			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parm for 'xmap_sun' keyword in shader '%s'", shader.name );
				continue;
			}

			a = atof( token );
			a = a / 180 * M_PI;

			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parm for 'xmap_sun' keyword in shader '%s'", shader.name );
				continue;
			}

			b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[ 0 ] = cos( a ) * cos( b );
			tr.sunDirection[ 1 ] = sin( a ) * cos( b );
			tr.sunDirection[ 2 ] = sin( b );
			continue;
		}
		// noShadows
		else if ( !Q_stricmp( token, "noShadows" ) )
		{
			shader.noShadows = true;
			continue;
		}
		// translucent
		else if ( !Q_stricmp( token, "translucent" ) )
		{
			shader.translucent = true;
			continue;
		}
		// forceOpaque
		else if ( !Q_stricmp( token, "forceOpaque" ) )
		{
			shader.forceOpaque = true;
			continue;
		}
		// forceSolid
		else if ( !Q_stricmp( token, "forceSolid" ) || !Q_stricmp( token, "solid" ) )
		{
			continue;
		}
		else if ( !Q_stricmp( token, "deformVertexes" ) || !Q_stricmp( token, "deform" ) )
		{
			ParseDeform( text );
			continue;
		}
		else if ( !Q_stricmp( token, "tesssize" ) )
		{
			SkipRestOfLine( text );
			continue;
		}

		// skip noFragment
		if ( !Q_stricmp( token, "noFragment" ) )
		{
			continue;
		}
		// skip stuff that only the xmap needs
		else if ( !Q_strnicmp( token, "xmap", 4 ) || !Q_strnicmp( token, "q3map", 5 ) )
		{
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only xmap or the server needs
		else if ( !Q_stricmp( token, "surfaceParm" ) )
		{
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( !Q_stricmp( token, "nomipmap" ) || !Q_stricmp( token, "nomipmaps" ) )
		{
			shader.filterType = filterType_t::FT_LINEAR;
			shader.noPicMip = true;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) )
		{
			shader.noPicMip = true;
			continue;
		}
		// RF, allow each shader to permit compression if available
		else if ( !Q_stricmp( token, "allowcompress" ) )
		{
			shader.uncompressed = false;
			continue;
		}
		else if ( !Q_stricmp( token, "nocompress" ) )
		{
			shader.uncompressed = true;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) )
		{
			shader.polygonOffset = true;

			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] )
			{
				shader.polygonOffsetValue = atof( token );
			}

			continue;
		}
		// parallax mapping
		else if ( !Q_stricmp( token, "parallax" ) )
		{
			shader.parallax = true;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) )
		{
			shader.entityMergable = true;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) )
		{
			/*
			Log::Warn("fogParms keyword not supported in shader '%s'", shader.name);
			SkipRestOfLine(text);

			*/

			if ( !ParseVector( text, 3, shader.fogParms.color ) )
			{
				return false;
			}

			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing parm for 'fogParms' keyword in shader '%s'", shader.name );
				continue;
			}

			shader.fogParms.depthForOpaque = atof( token );

			shader.sort = Util::ordinal(shaderSort_t::SS_FOG);

			// skip any old gradient directions
			SkipRestOfLine( text );
			continue;
		}
		// noFog
		else if ( !Q_stricmp( token, "noFog" ) )
		{
			shader.noFog = true;
			continue;
		}
		// portal
		else if ( !Q_stricmp( token, "portal" ) )
		{
			shader.sort = Util::ordinal(shaderSort_t::SS_PORTAL);
			shader.isPortal = true;

			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] )
			{
				shader.portalRange = atof( token );
			}
			else
			{
				shader.portalRange = 256;
			}

			continue;
		}
		// portal or mirror
		else if ( !Q_stricmp( token, "mirror" ) )
		{
			shader.sort = Util::ordinal(shaderSort_t::SS_PORTAL);
			shader.isPortal = true;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !Q_stricmp( token, "skyparms" ) )
		{
			ParseSkyParms( text );
			continue;
		}
		// ET sunshader <name>
		else if ( !Q_stricmp( token, "sunshader" ) )
		{
			int tokenLen;

			token = COM_ParseExt2( text, false );

			if ( !token[ 0 ] )
			{
				Log::Warn("missing shader name for 'sunshader'" );
				continue;
			}

			/*
			RB: don't call tr.sunShader = R_FindShader(token, SHADER_3D_STATIC, RSF_DEFAULT);
			        because it breaks the computation of the current shader
			*/
			tokenLen = strlen( token ) + 1;
			tr.sunShaderName = (char*) ri.Hunk_Alloc( sizeof( char ) * tokenLen, ha_pref::h_low );
			Q_strncpyz( tr.sunShaderName, token, tokenLen );
		}
		// light <value> determines flaring in xmap, not needed here
		else if ( !Q_stricmp( token, "light" ) )
		{
			token = COM_ParseExt2( text, false );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull" ) )
		{
			token = COM_ParseExt2( text, false );

			if ( token[ 0 ] == 0 )
			{
				Log::Warn("missing cull parms in shader '%s'", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twoSided" ) || !Q_stricmp( token, "disable" ) )
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if ( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				Log::Warn("invalid cull parm '%s' in shader '%s'", token, shader.name );
			}

			continue;
		}
		// twoSided
		else if ( !Q_stricmp( token, "twoSided" ) )
		{
			shader.cullType = CT_TWO_SIDED;
			continue;
		}
		// backSided
		else if ( !Q_stricmp( token, "backSided" ) )
		{
			shader.cullType = CT_BACK_SIDED;
			continue;
		}
		// clamp, edgeClamp etc.
		else if ( ParseClampType( token, &shader.wrapType ) )
		{
			continue;
		}
		// sort
		else if ( !Q_stricmp( token, "sort" ) )
		{
			ParseSort( text );
			continue;
		}
		// ydnar: implicit default mapping to eliminate redundant/incorrect explicit shader stages
		else if ( !Q_strnicmp( token, "implicit", 8 ) )
		{
			bool GL1 = false;

			// set implicit mapping state
			if ( !Q_stricmp( token, "implicitBlend" ) )
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				implicitCullType = CT_TWO_SIDED;
			}
			else if ( !Q_stricmp( token, "implicitMask" ) )
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_128;
				implicitCullType = CT_TWO_SIDED;
			}
			else if ( !Q_stricmp( token, "implicitMapGL1" ) )
			{
				GL1 = true;
			}
			else // "implicitMap"
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE;
				implicitCullType = CT_FRONT_SIDED;
			}

			// get image
			token = COM_ParseExt( text, false );

			if ( !GL1 )
			{
				if ( token[ 0 ] != '\0' )
				{
					Q_strncpyz( implicitMap, token, sizeof( implicitMap ) );
				}
				else
				{
					implicitMap[ 0 ] = '-';
					implicitMap[ 1 ] = '\0';
				}
			}

			continue;
		}
		// diffuseMap <image>
		else if ( !Q_stricmp( token, "diffuseMap" ) )
		{
			ParseDiffuseMap( &stages[ s ], text );
			s++;
			continue;
		}
		// normalMap <image>
		else if ( !Q_stricmp( token, "normalMap" ) || !Q_stricmp( token, "bumpMap" ) )
		{
			ParseNormalMap( &stages[ s ], text );
			s++;
			continue;
		}
		// specularMap <image>
		else if ( !Q_stricmp( token, "specularMap" ) )
		{
			ParseSpecularMap( &stages[ s ], text );
			s++;
			continue;
		}
		// materialMap <image>
		else if ( !Q_stricmp( token, "materialMap" ) )
		{
			ParseMaterialMap( &stages[ s ], text );
			s++;
			continue;
		}
		// glowMap <image>
		else if ( !Q_stricmp( token, "glowMap" ) )
		{
			ParseGlowMap( &stages[ s ], text );
			s++;
			continue;
		}
		// reflectionMap <image>
		else if ( !Q_stricmp( token, "reflectionMap" ) )
		{
			ParseReflectionMap( &stages[ s ], text );
			s++;
			continue;
		}
		// reflectionMapBlended <image>
		else if ( !Q_stricmp( token, "reflectionMapBlended" ) )
		{
			ParseReflectionMapBlended( &stages[ s ], text );
			s++;
			continue;
		}
		// lightFalloffImage <image>
		else if ( !Q_stricmp( token, "lightFalloffImage" ) )
		{
			ParseLightFalloffImage( &stages[ s ], text );
			s++;
			continue;
		}
		// Doom 3 DECAL_MACRO
		else if ( !Q_stricmp( token, "DECAL_MACRO" ) )
		{
			shader.polygonOffset = true;
			shader.polygonOffsetValue = 1;
			shader.sort = Util::ordinal(shaderSort_t::SS_DECAL);
			SurfaceParm( "discrete" );
			SurfaceParm( "noShadows" );
			continue;
		}
		// Prey DECAL_ALPHATEST_MACRO
		else if ( !Q_stricmp( token, "DECAL_ALPHATEST_MACRO" ) )
		{
			// what's different?
			shader.polygonOffset = true;
			shader.polygonOffsetValue = 1;
			shader.sort = Util::ordinal(shaderSort_t::SS_DECAL);
			SurfaceParm( "discrete" );
			SurfaceParm( "noShadows" );
			continue;
		}
		// when <state> <shader name>
		else if ( !Q_stricmp( token, "when" ) )
		{
			int i;
			const char *p;
			int index = 0;

			token = COM_ParseExt2( text, false );

			for ( i = 1, p = whenTokens; i < MAX_ALTSHADERS && *p; ++i, p += strlen( p ) + 1 )
			{
				if ( !Q_stricmp( token, p ) )
				{
					index = i;
					break;
				}
			}

			if ( index == 0 )
			{
				Log::Warn("unknown parameter '%s' for 'when' in '%s'", token, shader.name );
			}
			else
			{
				int tokenLen;

				token = COM_ParseExt( text, false );

				if ( !token[ 0 ] )
				{
					Log::Warn("missing shader name for 'when'" );
					continue;
				}

				tokenLen = strlen( token ) + 1;
				shader.altShader[ index ].index = 0;
				shader.altShader[ index ].name = ( char* )ri.Hunk_Alloc( sizeof( char ) * tokenLen, ha_pref::h_low );
				Q_strncpyz( shader.altShader[ index ].name, token, tokenLen );
			}
		}
		else if ( SurfaceParm( token ) )
		{
			continue;
		}
		else
		{
			Log::Warn("unknown general shader parameter '%s' in '%s'", token, shader.name );
			return false;
		}
	}

	// ignore shaders that don't have any stages, unless it is a sky or fog
	if ( s == 0 && !shader.forceOpaque && !shader.isSky && !( shader.contentFlags & CONTENTS_FOG ) && implicitMap[ 0 ] == '\0' )
	{
		return false;
	}

	return true;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
================
CollapseMultitexture
=================
*/
// *INDENT-OFF*
static void CollapseStages()
{
	int           i, j;

	bool      hasDiffuseStage;
	bool      hasNormalStage;
	bool      hasSpecularStage;
	bool      hasMaterialStage;
	bool      hasReflectionStage;
	bool      hasGlowStage;

	shaderStage_t tmpDiffuseStage;
	shaderStage_t tmpNormalStage;
	shaderStage_t tmpSpecularStage;
	shaderStage_t tmpMaterialStage;
	shaderStage_t tmpReflectionStage;
	shaderStage_t tmpGlowStage;

	shaderStage_t tmpColorStage;
	shaderStage_t tmpLightmapStage;

	shader_t      tmpShader;

	int           numStages = 0;
	shaderStage_t tmpStages[ MAX_SHADER_STAGES ];

	if ( !r_collapseStages->integer )
	{
		return;
	}

	Com_Memcpy( &tmpShader, &shader, sizeof( shader ) );

	Com_Memset( &tmpStages[ 0 ], 0, sizeof( stages ) );

	for ( j = 0; j < MAX_SHADER_STAGES; j++ )
	{
		hasDiffuseStage = false;
		hasNormalStage = false;
		hasSpecularStage = false;
		hasMaterialStage = false;
		hasReflectionStage = false;
		hasGlowStage = false;

		Com_Memset( &tmpDiffuseStage, 0, sizeof( shaderStage_t ) );
		Com_Memset( &tmpNormalStage, 0, sizeof( shaderStage_t ) );
		Com_Memset( &tmpSpecularStage, 0, sizeof( shaderStage_t ) );
		Com_Memset( &tmpGlowStage, 0, sizeof( shaderStage_t ) );

		Com_Memset( &tmpColorStage, 0, sizeof( shaderStage_t ) );
		Com_Memset( &tmpLightmapStage, 0, sizeof( shaderStage_t ) );

		if ( !stages[ j ].active )
		{
			continue;
		}

		if (
		  stages[ j ].type == stageType_t::ST_REFRACTIONMAP ||
		  stages[ j ].type == stageType_t::ST_DISPERSIONMAP ||
		  stages[ j ].type == stageType_t::ST_SKYBOXMAP ||
		  stages[ j ].type == stageType_t::ST_SCREENMAP ||
		  stages[ j ].type == stageType_t::ST_PORTALMAP ||
		  stages[ j ].type == stageType_t::ST_HEATHAZEMAP ||
		  stages[ j ].type == stageType_t::ST_LIQUIDMAP ||
		  stages[ j ].type == stageType_t::ST_ATTENUATIONMAP_XY ||
		  stages[ j ].type == stageType_t::ST_ATTENUATIONMAP_Z )
		{
			// only merge lighting relevant stages
			tmpStages[ numStages ] = stages[ j ];
			numStages++;
			continue;
		}

		for ( i = 0; i < 4; i++ )
		{
			if ( ( j + i ) >= MAX_SHADER_STAGES )
			{
				continue;
			}

			if ( !stages[ j + i ].active )
			{
				continue;
			}

			if ( stages[ j + i ].type == stageType_t::ST_DIFFUSEMAP && !hasDiffuseStage )
			{
				hasDiffuseStage = true;
				tmpDiffuseStage = stages[ j + i ];
			}
			else if ( stages[ j + i ].type == stageType_t::ST_NORMALMAP && !hasNormalStage )
			{
				hasNormalStage = true;
				tmpNormalStage = stages[ j + i ];
			}
			else if ( stages[ j + i ].type == stageType_t::ST_SPECULARMAP && !hasSpecularStage )
			{
				hasSpecularStage = true;
				tmpSpecularStage = stages[ j + i ];
			}
			else if ( stages[ j + i ].type == stageType_t::ST_MATERIALMAP && !hasMaterialStage )
			{
				hasMaterialStage = true;
				tmpMaterialStage = stages[ j + i ];
			}
			else if ( stages[ j + i ].type == stageType_t::ST_REFLECTIONMAP && !hasReflectionStage )
			{
				hasReflectionStage = true;
				tmpReflectionStage = stages[ j + i ];
			}
			else if ( stages[ j + i ].type == stageType_t::ST_GLOWMAP && !hasGlowStage )
			{
				hasGlowStage = true;
				tmpGlowStage = stages[ j + i ];
			}
		}

		// NOTE: Tr3B - merge as many stages as possible
		if( hasSpecularStage && hasMaterialStage ) {
			Log::Warn( "specularMap disabled in favor of materialMap in shader '%s'\n", shader.name );
			hasSpecularStage = false;
		}

		// try to merge diffuse/normal/specular/glow
		if ( hasDiffuseStage         &&
		     hasNormalStage          &&
		     hasSpecularStage        &&
		     hasGlowStage
		   )
		{
			tmpShader.collapseType = collapseType_t::COLLAPSE_lighting_DBSG;

			tmpStages[ numStages ] = tmpDiffuseStage;
			tmpStages[ numStages ].type = stageType_t::ST_COLLAPSE_lighting_DBSG;

			tmpStages[ numStages ].bundle[ TB_NORMALMAP ] = tmpNormalStage.bundle[ 0 ];

			tmpStages[ numStages ].bundle[ TB_SPECULARMAP ] = tmpSpecularStage.bundle[ 0 ];
			tmpStages[ numStages ].specularExponentMin = tmpSpecularStage.specularExponentMin;
			tmpStages[ numStages ].specularExponentMax = tmpSpecularStage.specularExponentMax;

			tmpStages[ numStages ].bundle[ TB_GLOWMAP ] = tmpGlowStage.bundle[ 0 ];
			numStages++;
			j += 3;
			continue;
		}
		// try to merge diffuse/normal/specular
		else if ( hasDiffuseStage         &&
		          hasNormalStage          &&
		          hasSpecularStage
		   )
		{
			tmpShader.collapseType = collapseType_t::COLLAPSE_lighting_DBS;

			tmpStages[ numStages ] = tmpDiffuseStage;
			tmpStages[ numStages ].type = stageType_t::ST_COLLAPSE_lighting_DBS;

			tmpStages[ numStages ].bundle[ TB_NORMALMAP ] = tmpNormalStage.bundle[ 0 ];

			tmpStages[ numStages ].bundle[ TB_SPECULARMAP ] = tmpSpecularStage.bundle[ 0 ];
			tmpStages[ numStages ].specularExponentMin = tmpSpecularStage.specularExponentMin;
			tmpStages[ numStages ].specularExponentMax = tmpSpecularStage.specularExponentMax;

			numStages++;
			j += 2;
			continue;
		}
		// try to merge diffuse/normal/material/glow
		else if ( hasDiffuseStage         &&
			  hasNormalStage          &&
			  hasMaterialStage        &&
			  hasGlowStage
		   )
		{
			tmpShader.collapseType = collapseType_t::COLLAPSE_lighting_DBMG;

			tmpStages[ numStages ] = tmpDiffuseStage;
			tmpStages[ numStages ].type = stageType_t::ST_COLLAPSE_lighting_DBMG;

			tmpStages[ numStages ].bundle[ TB_NORMALMAP ] = tmpNormalStage.bundle[ 0 ];

			tmpStages[ numStages ].bundle[ TB_MATERIALMAP ] = tmpMaterialStage.bundle[ 0 ];

			tmpStages[ numStages ].bundle[ TB_GLOWMAP ] = tmpGlowStage.bundle[ 0 ];
			numStages++;
			j += 3;
			continue;
		}
		// try to merge diffuse/normal/material
		else if ( hasDiffuseStage         &&
		          hasNormalStage          &&
		          hasMaterialStage
		   )
		{
			tmpShader.collapseType = collapseType_t::COLLAPSE_lighting_DBM;

			tmpStages[ numStages ] = tmpDiffuseStage;
			tmpStages[ numStages ].type = stageType_t::ST_COLLAPSE_lighting_DBM;

			tmpStages[ numStages ].bundle[ TB_NORMALMAP ] = tmpNormalStage.bundle[ 0 ];

			tmpStages[ numStages ].bundle[ TB_MATERIALMAP ] = tmpMaterialStage.bundle[ 0 ];

			numStages++;
			j += 2;
			continue;
		}
		// try to merge diffuse/normal/glow
		else if ( hasDiffuseStage         &&
		          hasNormalStage          &&
		          hasGlowStage
		        )
		{
			tmpShader.collapseType = collapseType_t::COLLAPSE_lighting_DBG;

			tmpStages[ numStages ] = tmpDiffuseStage;
			tmpStages[ numStages ].type = stageType_t::ST_COLLAPSE_lighting_DBG;

			tmpStages[ numStages ].bundle[ TB_NORMALMAP ] = tmpNormalStage.bundle[ 0 ];
			tmpStages[ numStages ].bundle[ TB_GLOWMAP ] = tmpGlowStage.bundle[ 0 ];
			numStages++;
			j += 2;
			continue;
		}
		// try to merge diffuse/normal
		else if ( hasDiffuseStage         &&
		          hasNormalStage
		        )
		{
			tmpShader.collapseType = collapseType_t::COLLAPSE_lighting_DB;

			tmpStages[ numStages ] = tmpDiffuseStage;
			tmpStages[ numStages ].type = stageType_t::ST_COLLAPSE_lighting_DB;

			tmpStages[ numStages ].bundle[ TB_NORMALMAP ] = tmpNormalStage.bundle[ 0 ];

			numStages++;
			j += 1;
			continue;
		}
		// try to merge env/normal
		else if ( hasReflectionStage &&
		          hasNormalStage
		        )
		{
			tmpShader.collapseType = collapseType_t::COLLAPSE_reflection_CB;

			tmpStages[ numStages ] = tmpReflectionStage;
			tmpStages[ numStages ].type = stageType_t::ST_COLLAPSE_reflection_CB;

			tmpStages[ numStages ].bundle[ TB_NORMALMAP ] = tmpNormalStage.bundle[ 0 ];

			numStages++;
			j += 1;
			continue;
		}
		// if there was no merge option just copy stage
		else
		{
			tmpStages[ numStages ] = stages[ j ];
			numStages++;
		}
	}

	// clear unused stages
	Com_Memset( &tmpStages[ numStages ], 0, sizeof( stages[ 0 ] ) * ( MAX_SHADER_STAGES - numStages ) );
	tmpShader.numStages = numStages;

	// copy result
	Com_Memcpy( &stages[ 0 ], &tmpStages[ 0 ], sizeof( stages ) );
	Com_Memcpy( &shader, &tmpShader, sizeof( shader ) );
}

// *INDENT-ON*

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
static void FixRenderCommandList( int newShader )
{
	renderCommandList_t *cmdList = &backEndData[ tr.smpFrame ]->commands;

	if ( cmdList )
	{
		const void *curCmd = cmdList->cmds;

		while ( 1 )
		{
			switch ( * ( const int * ) curCmd )
			{
				case Util::ordinal(renderCommand_t::RC_SET_COLOR):
					{
						const setColorCommand_t *sc_cmd = ( const setColorCommand_t * ) curCmd;

						curCmd = ( const void * )( sc_cmd + 1 );
						break;
					}

				case Util::ordinal(renderCommand_t::RC_STRETCH_PIC):
					{
						const stretchPicCommand_t *sp_cmd = ( const stretchPicCommand_t * ) curCmd;

						curCmd = ( const void * )( sp_cmd + 1 );
						break;
					}

				case Util::ordinal(renderCommand_t::RC_DRAW_VIEW):
					{
						int                     i;
						drawSurf_t              *drawSurf;

						const drawViewCommand_t *dv_cmd = ( const drawViewCommand_t * ) curCmd;

						for ( i = 0, drawSurf = dv_cmd->viewParms.drawSurfs; i < dv_cmd->viewParms.numDrawSurfs; i++, drawSurf++ )
						{
							if ( drawSurf->shaderNum() >= newShader )
							{
								drawSurf->setSort( drawSurf->shaderNum() + 1, drawSurf->lightmapNum(), drawSurf->entityNum(), drawSurf->fogNum(), drawSurf->index() );
							}
						}

						curCmd = ( const void * )( dv_cmd + 1 );
						break;
					}

				case Util::ordinal(renderCommand_t::RC_DRAW_BUFFER):
					{
						const drawBufferCommand_t *db_cmd = ( const drawBufferCommand_t * ) curCmd;

						curCmd = ( const void * )( db_cmd + 1 );
						break;
					}

				case Util::ordinal(renderCommand_t::RC_SWAP_BUFFERS):
					{
						const swapBuffersCommand_t *sb_cmd = ( const swapBuffersCommand_t * ) curCmd;

						curCmd = ( const void * )( sb_cmd + 1 );
						break;
					}

				case Util::ordinal(renderCommand_t::RC_END_OF_LIST):
				default:
					return;
			}
		}
	}
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted relative to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader()
{
	int      i;
	float    sort;
	shader_t *newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for ( i = tr.numShaders - 2; i >= 0; i-- )
	{
		if ( tr.sortedShaders[ i ]->sort <= sort )
		{
			break;
		}

		tr.sortedShaders[ i + 1 ] = tr.sortedShaders[ i ];
		tr.sortedShaders[ i + 1 ]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList( i + 1 );

	newShader->sortedIndex = i + 1;
	tr.sortedShaders[ i + 1 ] = newShader;
}

/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader()
{
	shader_t *newShader;
	int      i, b;
	int      size, hash;

	if ( tr.numShaders == MAX_SHADERS )
	{
		Log::Warn("GeneratePermanentShader - MAX_SHADERS hit" );
		return tr.defaultShader;
	}

	newShader = (shader_t*) ri.Hunk_Alloc( sizeof( shader_t ), ha_pref::h_low );

	*newShader = shader;

	if ( shader.sort <= Util::ordinal(shaderSort_t::SS_OPAQUE) )
	{
		newShader->fogPass = fogPass_t::FP_EQUAL;
	}
	else if ( shader.contentFlags & CONTENTS_FOG )
	{
		newShader->fogPass = fogPass_t::FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0; i < newShader->numStages; i++ )
	{
		if ( !stages[ i ].active )
		{
			break;
		}

		newShader->stages[ i ] = (shaderStage_t*) ri.Hunk_Alloc( sizeof( stages[ i ] ), ha_pref::h_low );
		*newShader->stages[ i ] = stages[ i ];

		for ( b = 0; b < MAX_TEXTURE_BUNDLES; b++ )
		{
			size = newShader->stages[ i ]->bundle[ b ].numTexMods * sizeof( texModInfo_t );
			newShader->stages[ i ]->bundle[ b ].texMods = (texModInfo_t*) ri.Hunk_Alloc( size, ha_pref::h_low );
			Com_Memcpy( newShader->stages[ i ]->bundle[ b ].texMods, stages[ i ].bundle[ b ].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue( newShader->name, FILE_HASH_SIZE );
	newShader->next = shaderHashTable[ hash ];
	shaderHashTable[ hash ] = newShader;

	return newShader;
}

/*
====================
GeneratePermanentShaderTable
====================
*/
static void GeneratePermanentShaderTable( float *values, int numValues )
{
	shaderTable_t *newTable;
	int           i;
	int           hash;

	if ( tr.numTables == MAX_SHADER_TABLES )
	{
		Log::Warn("GeneratePermanentShaderTables - MAX_SHADER_TABLES hit" );
		return;
	}

	newTable = (shaderTable_t*) ri.Hunk_Alloc( sizeof( shaderTable_t ), ha_pref::h_low );

	*newTable = table;

	tr.shaderTables[ tr.numTables ] = newTable;
	newTable->index = tr.numTables;

	tr.numTables++;

	newTable->numValues = numValues;
	newTable->values = (float*) ri.Hunk_Alloc( sizeof( float ) * numValues, ha_pref::h_low );

	for ( i = 0; i < numValues; i++ )
	{
		newTable->values[ i ] = values[ i ];
	}

	hash = generateHashValue( newTable->name, MAX_SHADERTABLE_HASH );
	newTable->next = shaderTableHashTable[ hash ];
	shaderTableHashTable[ hash ] = newTable;
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader()
{
	int      stage, i;
	shader_t *ret;

	// set sky stuff appropriate
	if ( shader.isSky )
	{
		if ( shader.noFog )
		{
			shader.sort = Util::ordinal(shaderSort_t::SS_ENVIRONMENT_NOFOG);
		}
		else
		{
			shader.sort = Util::ordinal(shaderSort_t::SS_ENVIRONMENT_FOG);
		}
	}

	if ( shader.forceOpaque )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_OPAQUE);
	}

	// set polygon offset
	if ( shader.polygonOffset && !shader.sort )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_DECAL);
	}

	// all light materials need at least one z attenuation stage as first stage
	if ( shader.type == shaderType_t::SHADER_LIGHT )
	{
		if ( stages[ 0 ].type != stageType_t::ST_ATTENUATIONMAP_Z )
		{
			// move up subsequent stages
			memmove( &stages[ 1 ], &stages[ 0 ], sizeof( stages[ 0 ] ) * ( MAX_SHADER_STAGES - 1 ) );

			stages[ 0 ].active = true;
			stages[ 0 ].type = stageType_t::ST_ATTENUATIONMAP_Z;
			stages[ 0 ].rgbGen = colorGen_t::CGEN_IDENTITY;
			stages[ 0 ].stateBits = GLS_DEFAULT;
			stages[ 0 ].overrideWrapType = true;
			stages[ 0 ].wrapType = wrapTypeEnum_t::WT_EDGE_CLAMP;

			LoadMap( &stages[ 0 ], "lights/squarelight1a.tga" );
		}

		// force following shader stages to be xy attenuation stages
		for ( stage = 1; stage < MAX_SHADER_STAGES; stage++ )
		{
			shaderStage_t *pStage = &stages[ stage ];

			if ( !pStage->active )
			{
				break;
			}

			pStage->type = stageType_t::ST_ATTENUATIONMAP_XY;
		}
	}

	// set appropriate stage information
	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = &stages[ stage ];

		if ( !pStage->active )
		{
			break;
		}

		// check for a missing texture
		switch ( pStage->type )
		{
			case stageType_t::ST_LIQUIDMAP:
			case stageType_t::ST_LIGHTMAP:
				// skip
				break;

			case stageType_t::ST_COLORMAP:
			default:
				{
					if ( !pStage->bundle[ 0 ].image[ 0 ] )
					{
						Log::Warn("Shader %s has a colormap stage with no image", shader.name );
						pStage->active = false;
						continue;
					}

					break;
				}

			case stageType_t::ST_DIFFUSEMAP:
				{
					if ( !shader.isSky )
					{
						shader.interactLight = true;
					}

					if ( !pStage->bundle[ 0 ].image[ 0 ] )
					{
						Log::Warn("Shader %s has a diffusemap stage with no image", shader.name );
						pStage->bundle[ 0 ].image[ 0 ] = tr.defaultImage;
					}

					break;
				}

			case stageType_t::ST_NORMALMAP:
				{
					if ( !shader.isSky )
					{
						shader.interactLight = true;
					}

					if ( !pStage->bundle[ 0 ].image[ 0 ] )
					{
						Log::Warn("Shader %s has a normalmap stage with no image", shader.name );
						pStage->bundle[ 0 ].image[ 0 ] = tr.flatImage;
					}

					break;
				}

			case stageType_t::ST_SPECULARMAP:
				{
					if ( !pStage->bundle[ 0 ].image[ 0 ] )
					{
						Log::Warn("Shader %s has a specularmap stage with no image", shader.name );
						pStage->bundle[ 0 ].image[ 0 ] = tr.blackImage;
					}

					break;
				}

			case stageType_t::ST_ATTENUATIONMAP_XY:
				{
					if ( !pStage->bundle[ 0 ].image[ 0 ] )
					{
						Log::Warn("Shader %s has a xy attenuationmap stage with no image", shader.name );
						pStage->active = false;
						continue;
					}

					break;
				}

			case stageType_t::ST_ATTENUATIONMAP_Z:
				{
					if ( !pStage->bundle[ 0 ].image[ 0 ] )
					{
						Log::Warn("Shader %s has a z attenuationmap stage with no image", shader.name );
						pStage->active = false;
						continue;
					}

					break;
				}
		}

		if ( shader.forceOpaque )
		{
			pStage->stateBits |= GLS_DEPTHMASK_TRUE;
		}

		if ( shader.isSky && pStage->noFog )
		{
			shader.sort = Util::ordinal(shaderSort_t::SS_ENVIRONMENT_NOFOG);
		}

		// determine sort order and fog color adjustment
		if ( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
		     ( stages[ 0 ].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) )
		{
			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort )
			{
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE )
				{
					shader.sort = Util::ordinal(shaderSort_t::SS_SEE_THROUGH);
				}
				else
				{
					shader.sort = Util::ordinal(shaderSort_t::SS_BLEND0);
				}
			}
		}
	}

	shader.numStages = stage;

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort )
	{
		if ( shader.translucent && !shader.forceOpaque )
		{
			shader.sort = Util::ordinal(shaderSort_t::SS_DECAL);
		}
		else
		{
			shader.sort = Util::ordinal(shaderSort_t::SS_OPAQUE);
		}
	}

	// HACK: allow alpha tested surfaces to create shadowmaps
	if ( r_shadows->integer >= Util::ordinal(shadowingMode_t::SHADOWING_ESM16) )
	{
		if ( shader.noShadows && shader.alphaTest )
		{
			shader.noShadows = false;
		}
	}

	// look for multitexture potential
	CollapseStages();

	// fogonly shaders don't have any stage passes
	if ( shader.numStages == 0 && !shader.isSky )
	{
		shader.sort = Util::ordinal(shaderSort_t::SS_FOG);
	}

	if ( shader.numDeforms > 0 ) {
		for( i = 0; i < shader.numStages; i++ ) {
			stages[i].deformIndex = gl_shaderManager.getDeformShaderIndex( shader.deforms, shader.numDeforms );
		}
	}

	ret = GeneratePermanentShader();

	// generate depth-only shader if necessary
	if( !shader.isSky &&
	    shader.numStages > 0 &&
	    (stages[0].stateBits & GLS_DEPTHMASK_TRUE) &&
	    !(stages[0].stateBits & GLS_DEPTHFUNC_EQUAL) &&
	    !(shader.type == shaderType_t::SHADER_2D) &&
	    !shader.polygonOffset ) {
		// keep only the first stage
		stages[1].active = false;
		shader.numStages = 1;
		strcat(shader.name, "$depth");

		if( stages[0].stateBits & GLS_ATEST_BITS ) {
			// alpha test requires a custom depth shader
			shader.sort = Util::ordinal( shaderSort_t::SS_DEPTH );
			stages[0].stateBits &= ~GLS_SRCBLEND_BITS & ~GLS_DSTBLEND_BITS;
			stages[0].stateBits |= GLS_COLORMASK_BITS;
			stages[0].type = stageType_t::ST_COLORMAP;

			ret->depthShader = GeneratePermanentShader();
		} else if ( shader.cullType == 0 &&
			    shader.numDeforms == 0 &&
			    tr.defaultShader ) {
			// can use the default depth shader
			ret->depthShader = tr.defaultShader->depthShader;
		} else {
			// requires a custom depth shader, but can skip
			// the texturing
			shader.sort = Util::ordinal( shaderSort_t::SS_DEPTH );
			stages[0].stateBits &= ~GLS_SRCBLEND_BITS & ~GLS_DSTBLEND_BITS;
			stages[0].stateBits |= GLS_COLORMASK_BITS;
			stages[0].type = stageType_t::ST_COLORMAP;

			stages[0].bundle[0].image[0] = tr.whiteImage;
			stages[0].bundle[0].numTexMods = 0;
			stages[0].tcGen_Environment = false;
			stages[0].tcGen_Lightmap = false;
			stages[0].rgbGen = colorGen_t::CGEN_IDENTITY;
			stages[0].alphaGen = alphaGen_t::AGEN_IDENTITY;

			ret->depthShader = GeneratePermanentShader();
		}
		// disable depth writes in the main pass
		ret->stages[0]->stateBits &= ~GLS_DEPTHMASK_TRUE;
	} else {
		ret->depthShader = NULL;
	}

	// load all altShaders recursively
	for ( i = 1; i < MAX_ALTSHADERS; ++i )
	{
		if ( ret->altShader[ i ].name )
		{
			// flags were previously stashed in altShader[0].index
			shader_t *sh = R_FindShader( ret->altShader[ i ].name, ret->type, (RegisterShaderFlags_t)ret->altShader[ 0 ].index );

			ret->altShader[ i ].index = sh->defaultShader ? 0 : sh->index;
		}
	}

	return ret;
}

//========================================================================================

//bani - dynamic shader list
struct dynamicshader_t
{
	char            *shadertext;
	dynamicshader_t *next;
};

static dynamicshader_t *dshader = nullptr;

/*
====================
RE_LoadDynamicShader

bani - load a new dynamic shader

if shadertext is nullptr, looks for matching shadername and removes it

returns true if request was successful, false if the gods were angered
====================
*/
bool RE_LoadDynamicShader( const char *shadername, const char *shadertext )
{
	const char      *func_err = "RE_LoadDynamicShader";
	dynamicshader_t *dptr, *lastdptr;
	const char            *q, *token;

	Log::Warn("RE_LoadDynamicShader( name = '%s', text = '%s' )", shadername, shadertext );

	if ( !shadername && shadertext )
	{
		Log::Warn("%s called with NULL shadername and non-NULL shadertext:\n%s", func_err, shadertext );
		return false;
	}

	if ( shadername && strlen( shadername ) >= MAX_QPATH )
	{
		Log::Warn("%s shadername %s exceeds MAX_QPATH", func_err, shadername );
		return false;
	}

	//empty the whole list
	if ( !shadername && !shadertext )
	{
		dptr = dshader;

		while ( dptr )
		{
			lastdptr = dptr->next;
			ri.Free( dptr->shadertext );
			ri.Free( dptr );
			dptr = lastdptr;
		}

		dshader = nullptr;
		return true;
	}

	//walk list for existing shader to delete, or end of the list
	dptr = dshader;
	lastdptr = nullptr;

	while ( dptr )
	{
		q = dptr->shadertext;

		token = COM_ParseExt( &q, true );

		if ( ( token[ 0 ] != 0 ) && !Q_stricmp( token, shadername ) )
		{
			//request to nuke this dynamic shader
			if ( !shadertext )
			{
				if ( !lastdptr )
				{
					dshader = nullptr;
				}
				else
				{
					lastdptr->next = dptr->next;
				}

				ri.Free( dptr->shadertext );
				ri.Free( dptr );
				return true;
			}

			Log::Warn("%s shader %s already exists!", func_err, shadername );
			return false;
		}

		lastdptr = dptr;
		dptr = dptr->next;
	}

	//can't add a new one with empty shadertext
	if ( !shadertext || !strlen( shadertext ) )
	{
		Log::Warn("%s new shader %s has NULL shadertext!", func_err, shadername );
		return false;
	}

	//create a new shader
	dptr = ( dynamicshader_t * ) ri.Z_Malloc( sizeof( *dptr ) );

	if ( !dptr )
	{
		Com_Error( errorParm_t::ERR_FATAL, "Couldn't allocate struct for dynamic shader %s", shadername );
	}

	if ( lastdptr )
	{
		lastdptr->next = dptr;
	}

	dptr->shadertext = (char*) ri.Z_Malloc( strlen( shadertext ) + 1 );

	if ( !dptr->shadertext )
	{
		Com_Error( errorParm_t::ERR_FATAL, "Couldn't allocate buffer for dynamic shader %s", shadername );
	}

	Q_strncpyz( dptr->shadertext, shadertext, strlen( shadertext ) + 1 );
	dptr->next = nullptr;

	if ( !dshader )
	{
		dshader = dptr;
	}

	return true;
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return nullptr if not found

If found, it will return a valid shader
=====================
*/
static const char    *FindShaderInShaderText( const char *shaderName )
{
	const char *token, *p;

	int  i, hash;

	hash = generateHashValue( shaderName, MAX_SHADERTEXT_HASH );

	for ( i = 0; shaderTextHashTable[ hash ][ i ]; i++ )
	{
		p = shaderTextHashTable[ hash ][ i ];
		token = COM_ParseExt2( &p, true );

		if ( !Q_stricmp( token, shaderName ) )
		{
			return p;
		}
	}

	// if the shader is not in the table, it must not exist
	return nullptr;
}

/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t       *R_FindShaderByName( const char *name )
{
	char     strippedName[ MAX_QPATH ];
	int      hash;
	shader_t *sh;

	if ( ( name == nullptr ) || ( name[ 0 ] == 0 ) )
	{
		// bk001205
		return tr.defaultShader;
	}

	COM_StripExtension3( name, strippedName, sizeof( strippedName ) );

	hash = generateHashValue( strippedName, FILE_HASH_SIZE );

	// see if the shader is already loaded
	for ( sh = shaderHashTable[ hash ]; sh; sh = sh->next )
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with type == SHADER_3D_DYNAMIC, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( Q_stricmp( sh->name, strippedName ) == 0 )
		{
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If type == SHADER_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If type == SHADER_3D_DYNAMIC, then the image will have
dynamic diffuse lighting applied to it, as appropriate for most
entity skin surfaces.

If type == SHADER_3D_STATIC, then the image will use
the vertex rgba modulate values, as appropriate for misc_model
pre-lit surfaces.
===============
*/
shader_t       *R_FindShader( const char *name, shaderType_t type,
			      RegisterShaderFlags_t flags )
{
	char     strippedName[ MAX_QPATH ];
	char     fileName[ MAX_QPATH ];
	int      i, hash, bits;
	const char     *shaderText;
	image_t  *image;
	shader_t *sh;

	if ( name[ 0 ] == 0 )
	{
		return tr.defaultShader;
	}

	COM_StripExtension3( name, strippedName, sizeof( strippedName ) );

	hash = generateHashValue( strippedName, FILE_HASH_SIZE );

	// see if the shader is already loaded
	for ( sh = shaderHashTable[ hash ]; sh; sh = sh->next )
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with type == SHADER_3D_DYNAMIC, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( ( sh->type == type || sh->defaultShader ) && !Q_stricmp( sh->name, strippedName ) )
		{
			// match found
			return sh;
		}
	}

	shader.altShader[ 0 ].index = flags; // save for later use (in case of alternativer shaders)

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer )
	{
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz( shader.name, strippedName, sizeof( shader.name ) );
	shader.type = type;

	for ( i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		stages[ i ].bundle[ 0 ].texMods = texMods[ i ];
	}

	// ydnar: default to no implicit mappings
	implicitMap[ 0 ] = '\0';
	implicitStateBits = GLS_DEFAULT;
	if( shader.type == shaderType_t::SHADER_2D )
	{
		implicitCullType = CT_TWO_SIDED;
	}
	else
	{
		implicitCullType = CT_FRONT_SIDED;
	}

	if( flags & RSF_SPRITE ) {
		shader.autoSpriteMode = 1;
	}

	// attempt to define shader from an explicit parameter file
	shaderText = FindShaderInShaderText( strippedName );

	if ( shaderText )
	{
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if ( r_printShaders->integer )
		{
			Log::Notice("...loading explicit shader '%s'", strippedName );
		}

		if ( !ParseShader( shaderText ) )
		{
			// had errors, so use default shader
			shader.defaultShader = true;
			sh = FinishShader();
			return sh;
		}

		// ydnar: allow implicit mappings
		if ( implicitMap[ 0 ] == '\0' )
		{
			sh = FinishShader();
			return sh;
		}
	}

	// ydnar: allow implicit mapping ('-' = use shader name)
	if ( implicitMap[ 0 ] == '\0' || implicitMap[ 0 ] == '-' )
	{
		Q_strncpyz( fileName, strippedName, sizeof( fileName ) );
	}
	else
	{
		Q_strncpyz( fileName, implicitMap, sizeof( fileName ) );
	}

	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file
	bits = IF_NONE;
	if( flags & RSF_NOMIP )
		bits |= IF_NOPICMIP;
	else
		shader.noPicMip = true;

	if( flags & RSF_NOLIGHTSCALE )
		bits |= IF_NOLIGHTSCALE | IF_NOCOMPRESSION;

	// choosing filter based on the NOMIP flag seems strange,
	// maybe it should be changed to type == SHADER_2D
	if( !(bits & RSF_NOMIP) ) {
		image = R_FindImageFile( fileName, bits, filterType_t::FT_DEFAULT,
					 wrapTypeEnum_t::WT_REPEAT );
	} else {
		image = R_FindImageFile( fileName, bits, filterType_t::FT_LINEAR,
					 wrapTypeEnum_t::WT_CLAMP );
	}

	if ( !image )
	{
		Log::Debug("Couldn't find image file for shader %s", name );
		shader.defaultShader = true;
		return FinishShader();
	}

	// set implicit cull type
	if ( implicitCullType && !shader.cullType )
	{
		shader.cullType = implicitCullType;
	}

	// create the default shading commands
	switch ( shader.type )
	{
		case shaderType_t::SHADER_2D:
			{
				// stageType_t::GUI elements
				stages[ 0 ].bundle[ 0 ].image[ 0 ] = image;
				stages[ 0 ].active = true;
				stages[ 0 ].rgbGen = colorGen_t::CGEN_VERTEX;
				stages[ 0 ].alphaGen = alphaGen_t::AGEN_VERTEX;
				stages[ 0 ].stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				break;
			}

		case shaderType_t::SHADER_3D_DYNAMIC:
			{
				// dynamic colors at vertexes
				stages[ 0 ].type = stageType_t::ST_DIFFUSEMAP;
				stages[ 0 ].bundle[ 0 ].image[ 0 ] = image;
				stages[ 0 ].active = true;
				stages[ 0 ].rgbGen = colorGen_t::CGEN_IDENTITY_LIGHTING;
				stages[ 0 ].stateBits = implicitStateBits;
				break;
			}

		case shaderType_t::SHADER_3D_STATIC:
			{
				// explicit colors at vertexes
				stages[ 0 ].type = stageType_t::ST_DIFFUSEMAP;
				stages[ 0 ].bundle[ 0 ].image[ 0 ] = image;
				stages[ 0 ].active = true;
				stages[ 0 ].rgbGen = colorGen_t::CGEN_VERTEX;
				stages[ 0 ].stateBits = implicitStateBits;
				break;
			}

		case shaderType_t::SHADER_LIGHT:
			{
				stages[ 0 ].type = stageType_t::ST_ATTENUATIONMAP_Z;
				stages[ 0 ].bundle[ 0 ].image[ 0 ] = tr.noFalloffImage; // FIXME should be attenuationZImage
				stages[ 0 ].active = true;
				stages[ 0 ].rgbGen = colorGen_t::CGEN_IDENTITY;
				stages[ 0 ].stateBits = GLS_DEFAULT;

				stages[ 1 ].type = stageType_t::ST_ATTENUATIONMAP_XY;
				stages[ 1 ].bundle[ 0 ].image[ 0 ] = image;
				stages[ 1 ].active = true;
				stages[ 1 ].rgbGen = colorGen_t::CGEN_IDENTITY;
				stages[ 1 ].stateBits = GLS_DEFAULT;
				break;
			}

		default:
			break;
	}

	return FinishShader();
}

qhandle_t RE_RegisterShaderFromImage( const char *name, image_t *image )
{
	int      i, hash;
	shader_t *sh;

	hash = generateHashValue( name, FILE_HASH_SIZE );

	// see if the shader is already loaded
	for ( sh = shaderHashTable[ hash ]; sh; sh = sh->next )
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with type == SHADER_3D_DYNAMIC, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( ( sh->type == shaderType_t::SHADER_2D || sh->defaultShader ) && !Q_stricmp( sh->name, name ) )
		{
			// match found
			return sh->index;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer )
	{
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz( shader.name, name, sizeof( shader.name ) );
	shader.type = shaderType_t::SHADER_2D;
	shader.cullType = CT_TWO_SIDED;

	for ( i = 0; i < MAX_SHADER_STAGES; i++ )
	{
		stages[ i ].bundle[ 0 ].texMods = texMods[ i ];
	}

	// create the default shading commands

	// GUI elements
	stages[ 0 ].bundle[ 0 ].image[ 0 ] = image;
	stages[ 0 ].active = true;
	stages[ 0 ].rgbGen = colorGen_t::CGEN_VERTEX;
	stages[ 0 ].alphaGen = alphaGen_t::AGEN_VERTEX;
	stages[ 0 ].stateBits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;

	sh = FinishShader();
	return sh->index;
}

/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name, RegisterShaderFlags_t flags )
{
	shader_t *sh;

	if ( strlen( name ) >= MAX_QPATH )
	{
		Log::Notice( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name,
			   (flags & RSF_LIGHT_ATTENUATION) ? shaderType_t::SHADER_LIGHT : shaderType_t::SHADER_2D,
			   flags );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader )
	{
		return 0;
	}

	return sh->index;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t       *R_GetShaderByHandle( qhandle_t hShader )
{
	if ( hShader < 0 )
	{
		Log::Warn("R_GetShaderByHandle: out of range hShader '%d'", hShader );  // bk: FIXME name
		return tr.defaultShader;
	}

	if ( hShader >= tr.numShaders )
	{
		Log::Warn("R_GetShaderByHandle: out of range hShader '%d'", hShader );
		return tr.defaultShader;
	}

	return tr.shaders[ hShader ];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void R_ShaderList_f()
{
	int      i;
	int      count;
	shader_t *shader;
	const char *s = nullptr;

	Log::Notice("-----------------------" );

	if ( ri.Cmd_Argc() > 1 )
	{
		s = ri.Cmd_Argv( 1 );
	}

	count = 0;

	for ( i = 0; i < tr.numShaders; i++ )
	{
		if ( ri.Cmd_Argc() > 2 )
		{
			shader = tr.sortedShaders[ i ];
		}
		else
		{
			shader = tr.shaders[ i ];
		}

		if ( s && !Com_Filter( s, shader->name, false ) )
		{
			continue;
		}

		std::string str;

		switch ( shader->type )
		{
			case shaderType_t::SHADER_2D:
				str += "2D   ";
				break;

			case shaderType_t::SHADER_3D_DYNAMIC:
				str += "3D_D ";
				break;

			case shaderType_t::SHADER_3D_STATIC:
				str += "3D_S ";
				break;

			case shaderType_t::SHADER_LIGHT:
				str += "ATTN ";
				break;
		}

		if ( shader->collapseType == collapseType_t::COLLAPSE_lighting_DB )
		{
			str += "lighting_DB    ";
		}
		else if ( shader->collapseType == collapseType_t::COLLAPSE_lighting_DBS )
		{
			str += "lighting_DBS   ";
		}
		else if ( shader->collapseType == collapseType_t::COLLAPSE_lighting_DBM )
		{
			str += "lighting_DBM   ";
		}
		else if ( shader->collapseType == collapseType_t::COLLAPSE_reflection_CB )
		{
			str += "reflection_CB  ";
		}
		else
		{
			str += "               ";
		}

		if ( shader->sort == Util::ordinal(shaderSort_t::SS_BAD) )
		{
			str += "SS_BAD              ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_PORTAL) )
		{
			str += "SS_PORTAL           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_DEPTH) )
		{
			str += "SS_DEPTH            ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_ENVIRONMENT_FOG) )
		{
			str += "SS_ENVIRONMENT_FOG  ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_ENVIRONMENT_NOFOG) )
		{
			str += "SS_ENVIRONMENT_NOFOG";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_OPAQUE) )
		{
			str += "SS_OPAQUE           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_DECAL) )
		{
			str += "SS_DECAL            ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_SEE_THROUGH) )
		{
			str += "SS_SEE_THROUGH      ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_BANNER) )
		{
			str += "SS_BANNER           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_FOG) )
		{
			str += "SS_FOG              ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_UNDERWATER) )
		{
			str += "SS_UNDERWATER       ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_WATER) )
		{
			str += "SS_WATER            ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_FAR) )
		{
			str += "SS_FAR              ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_MEDIUM) )
		{
			str += "SS_MEDIUM           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_CLOSE) )
		{
			str += "SS_CLOSE            ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_BLEND0) )
		{
			str += "SS_BLEND0           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_BLEND1) )
		{
			str += "SS_BLEND1           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_BLEND2) )
		{
			str += "SS_BLEND2           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_BLEND3) )
		{
			str += "SS_BLEND3           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_BLEND6) )
		{
			str += "SS_BLEND6           ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_ALMOST_NEAREST) )
		{
			str += "SS_ALMOST_NEAREST   ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_NEAREST) )
		{
			str += "SS_NEAREST          ";
		}
		else if ( shader->sort == Util::ordinal(shaderSort_t::SS_POST_PROCESS) )
		{
			str += "SS_POST_PROCESS     ";
		}
		else
		{
			str += "                    ";
		}

		if ( shader->interactLight )
		{
			str += "IA ";
		}
		else
		{
			str += "   ";
		}

		if ( shader->defaultShader )
		{
			Log::Notice("%s: %s (DEFAULTED)", str.c_str(), shader->name );
		}
		else
		{
			Log::Notice("%s: %s", str.c_str(), shader->name );
		}

		count++;
	}

	Log::Notice("%i total shaders", count );
	Log::Notice("------------------" );
}

void R_ShaderExp_f()
{
	int          i;
	int          len;
	char         buffer[ 1024 ] = "";
	const char         *buffer_p = &buffer[ 0 ];
	expression_t exp;

	strcpy( shader.name, "dummy" );

	Log::Notice("-----------------------" );

	len = sizeof( buffer );

	for ( i = 1; i < ri.Cmd_Argc() && len > 0; i++ )
	{
		if ( i > 1 )
		{
			strncat( buffer, " ", len-- );
		}

		strncat( buffer, ri.Cmd_Argv( i ), len );
		len -= strlen( ri.Cmd_Argv( i ) );
	}

	ParseExpression( &buffer_p, &exp );

	Log::Notice("%i total ops", exp.numOps );
	Log::Notice("%f result", RB_EvalExpression( &exp, 0 ) );
	Log::Notice("------------------" );
}

/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
static const int MAX_SHADER_FILES = 4096;
static void ScanAndLoadShaderFiles()
{
	char **shaderFiles;
	char *buffers[ MAX_SHADER_FILES ];
	const char *p;
	int  numShaderFiles;
	int  i;
	const char *oldp, *token;
	char *textEnd;
	const char **hashMem;
	int  shaderTextHashTableSizes[ MAX_SHADERTEXT_HASH ], hash, size;
	char filename[ MAX_QPATH ];
	long sum = 0, summand;

	Log::Debug("----- ScanAndLoadShaderFiles -----" );

	// scan for shader files
	shaderFiles = ri.FS_ListFiles( "scripts", ".shader", &numShaderFiles );

	if ( !shaderFiles || !numShaderFiles )
	{
		Log::Warn("no shader files found" );
	}

	if ( numShaderFiles > MAX_SHADER_FILES )
	{
		numShaderFiles = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaderFiles; i++ )
	{
		Com_sprintf( filename, sizeof( filename ), "scripts/%s", shaderFiles[ i ] );

		Log::Debug("...loading '%s'", filename );
		summand = ri.FS_ReadFile( filename, ( void ** ) &buffers[ i ] );

		if ( !buffers[ i ] )
		{
			ri.Error(errorParm_t::ERR_DROP, "Couldn't load %s", filename );
		}

		p = buffers[ i ];

		while ( 1 )
		{
			token = COM_ParseExt2( &p, true );

			if ( !*token )
			{
				break;
			}

			// Step over the "table" and the name
			if ( !Q_stricmp( token, "table" ) )
			{
				token = COM_ParseExt2( &p, true );

				if ( !*token )
				{
					break;
				}
			}

			token = COM_ParseExt2( &p, true );

			if ( token[ 0 ] != '{' || token[ 1 ] != '\0' || !SkipBracedSection_Depth( &p, 1 ) )
			{
				Log::Warn("Bad shader file %s has incorrect syntax.", filename );
				ri.FS_FreeFile( buffers[ i ] );
				buffers[ i ] = nullptr;
				break;
			}
		}

		if ( buffers[ i ] )
		{
			sum += summand;
		}
	}

	// build single large buffer
	s_shaderText = (char*) ri.Hunk_Alloc( sum + numShaderFiles * 2, ha_pref::h_low );
	s_shaderText[ 0 ] = '\0';
	textEnd = s_shaderText;

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaderFiles - 1; i >= 0; i-- )
	{
		if ( !buffers[ i ] )
		{
			continue;
		}

		strcat( textEnd, buffers[ i ] );
		strcat( textEnd, "\n" );
		textEnd += strlen( textEnd );
		ri.FS_FreeFile( buffers[ i ] );
	}

	// ydnar: unixify all shaders
	COM_FixPath( s_shaderText );

	COM_Compress( s_shaderText );

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	Com_Memset( shaderTextHashTableSizes, 0, sizeof( shaderTextHashTableSizes ) );
	size = 0;

	p = s_shaderText;

	// look for shader names
	while ( 1 )
	{
		token = COM_ParseExt2( &p, true );

		if ( token[ 0 ] == 0 )
		{
			break;
		}

		// skip shader tables
		if ( !Q_stricmp( token, "table" ) )
		{
			// skip table name
			token = COM_ParseExt2( &p, true );

			SkipBracedSection( &p );
		}
		else
		{
			hash = generateHashValue( token, MAX_SHADERTEXT_HASH );
			shaderTextHashTableSizes[ hash ]++;
			size++;
			SkipBracedSection( &p );
		}
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = (const char**) ri.Hunk_Alloc( size * sizeof( char * ), ha_pref::h_low );

	for ( i = 0; i < MAX_SHADERTEXT_HASH; i++ )
	{
		shaderTextHashTable[ i ] = hashMem;
		hashMem += shaderTextHashTableSizes[ i ] + 1;
	}

	Com_Memset( shaderTextHashTableSizes, 0, sizeof( shaderTextHashTableSizes ) );

	p = s_shaderText;

	// look for shader names
	while ( 1 )
	{
		oldp = p;
		token = COM_ParseExt( &p, true );

		if ( token[ 0 ] == 0 )
		{
			break;
		}

		// parse shader tables
		if ( !Q_stricmp( token, "table" ) )
		{
			int           depth;
			float         values[ FUNCTABLE_SIZE ];
			int           numValues;
			shaderTable_t *tb;
			bool      alreadyCreated;

			Com_Memset( &table, 0, sizeof( table ) );

			token = COM_ParseExt2( &p, true );

			Q_strncpyz( table.name, token, sizeof( table.name ) );

			// check if already created
			alreadyCreated = false;
			hash = generateHashValue( table.name, MAX_SHADERTABLE_HASH );

			for ( tb = shaderTableHashTable[ hash ]; tb; tb = tb->next )
			{
				if ( Q_stricmp( tb->name, table.name ) == 0 )
				{
					// match found
					alreadyCreated = true;
					break;
				}
			}

			depth = 0;
			numValues = 0;

			do
			{
				token = COM_ParseExt2( &p, true );

				if ( !Q_stricmp( token, "snap" ) )
				{
					table.snap = true;
				}
				else if ( !Q_stricmp( token, "clamp" ) )
				{
					table.clamp = true;
				}
				else if ( token[ 0 ] == '{' )
				{
					depth++;
				}
				else if ( token[ 0 ] == '}' )
				{
					depth--;
				}
				else if ( token[ 0 ] == ',' )
				{
					continue;
				}
				else
				{
					if ( numValues == FUNCTABLE_SIZE )
					{
						Log::Warn("FUNCTABLE_SIZE hit" );
						break;
					}

					values[ numValues++ ] = atof( token );
				}
			}
			while ( depth && p );

			if ( !alreadyCreated )
			{
				Log::Debug("...generating '%s'", table.name );
				GeneratePermanentShaderTable( values, numValues );
			}
		}
		else
		{
			hash = generateHashValue( token, MAX_SHADERTEXT_HASH );
			shaderTextHashTable[ hash ][ shaderTextHashTableSizes[ hash ]++ ] = oldp;

			SkipBracedSection( &p );
		}
	}
}

/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders()
{
	Log::Debug("----- CreateInternalShaders -----" );

	tr.numShaders = 0;

	// init the default shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	shader.type = shaderType_t::SHADER_3D_DYNAMIC;
	stages[ 0 ].type = stageType_t::ST_DIFFUSEMAP;
	stages[ 0 ].bundle[ 0 ].image[ 0 ] = tr.defaultImage;
	stages[ 0 ].active = true;
	stages[ 0 ].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();
}

static void CreateExternalShaders()
{
	Log::Debug("----- CreateExternalShaders -----" );

	tr.flareShader = R_FindShader( "flareShader", shaderType_t::SHADER_3D_DYNAMIC, RSF_DEFAULT );
	tr.sunShader = R_FindShader( "sun", shaderType_t::SHADER_3D_DYNAMIC, RSF_DEFAULT );

	tr.defaultPointLightShader = R_FindShader( "lights/defaultPointLight", shaderType_t::SHADER_LIGHT, RSF_DEFAULT );
	tr.defaultProjectedLightShader = R_FindShader( "lights/defaultProjectedLight", shaderType_t::SHADER_LIGHT, RSF_DEFAULT );
	tr.defaultDynamicLightShader = R_FindShader( "lights/defaultDynamicLight", shaderType_t::SHADER_LIGHT, RSF_DEFAULT );
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders()
{
	Com_Memset( shaderTableHashTable, 0, sizeof( shaderTableHashTable ) );
	Com_Memset( shaderHashTable, 0, sizeof( shaderHashTable ) );

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();
}

/*
==================
R_SetAltShaderKeywords
==================
*/
void R_SetAltShaderTokens( const char *list )
{
	char *p;

	memset( whenTokens, 0, sizeof( whenTokens ) );
	Q_strncpyz( whenTokens, list, sizeof( whenTokens ) - 1 ); // will have double-NUL termination

	p = whenTokens - 1;

	while ( ( p = strchr( p + 1, ',' ) ) )
	{
		*p = 0;
	}
}

/*
==================
RE_GetShaderNameFromHandle
==================
*/

const char *RE_GetShaderNameFromHandle( qhandle_t shader )
{
	return R_GetShaderByHandle( shader )->name;
}
