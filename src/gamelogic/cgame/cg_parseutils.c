/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "cg_local.h"

/*
=================
PC_SourceWarning
=================
*/
void PRINTF_LIKE(2) PC_SourceWarning( int handle, char *format, ... )
{
	int         line;
	char        filename[ 128 ];
	va_list     argptr;
	static char string[ 4096 ];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	filename[ 0 ] = '\0';
	line = 0;
	trap_Parse_SourceFileAndLine( handle, filename, &line );

	Com_Printf( S_WARNING "%s, line %d: %s\n", filename, line, string );
}

/*
=================
PC_SourceError
=================
*/
void PRINTF_LIKE(2) PC_SourceError( int handle, char *format, ... )
{
	int         line;
	char        filename[ 128 ];
	va_list     argptr;
	static char string[ 4096 ];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	filename[ 0 ] = '\0';
	line = 0;
	trap_Parse_SourceFileAndLine( handle, filename, &line );

	Com_Printf( S_ERROR "%s, line %d: %s\n", filename, line, string );
}

/*
=================
LerpColor
=================
*/
void LerpColor( vec4_t a, vec4_t b, vec4_t c, float t )
{
	int i;

	// lerp and clamp each component

	for ( i = 0; i < 4; i++ )
	{
		c[ i ] = a[ i ] + t * ( b[ i ] - a[ i ] );

		if ( c[ i ] < 0 )
		{
			c[ i ] = 0;
		}
		else if ( c[ i ] > 1.0 )
		{
			c[ i ] = 1.0;
		}
	}
}

/*
=================
Float_Parse
=================
*/
qboolean Float_Parse( char **p, float *f )
{
	char *token;
	token = COM_ParseExt( p, qfalse );

	if ( token && token[ 0 ] != 0 )
	{
		*f = atof( token );
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

#define MAX_EXPR_ELEMENTS 32

typedef enum
{
  EXPR_OPERATOR,
  EXPR_VALUE
}

exprType_t;

typedef struct exprToken_s
{
	exprType_t type;
	union
	{
		char  op;
		float val;
	} u;
}

exprToken_t;

typedef struct exprList_s
{
	exprToken_t l[ MAX_EXPR_ELEMENTS ];
	int         f, b;
}

exprList_t;

/*
=================
OpPrec

Return a value reflecting operator precedence
=================
*/
static INLINE int OpPrec( char op )
{
	switch ( op )
	{
		case '*':
			return 4;

		case '/':
			return 3;

		case '+':
			return 2;

		case '-':
			return 1;

		case '(':
			return 0;

		default:
			return -1;
	}
}

/*
=================
PC_Expression_Parse
=================
*/
static qboolean PC_Expression_Parse( int handle, float *f )
{
	pc_token_t  token;
	int         unmatchedParentheses = 0;
	exprList_t  stack, fifo;
	exprToken_t *value;
	qboolean    expectingNumber = qtrue;

#define FULL( a )  ( a.b >= ( MAX_EXPR_ELEMENTS - 1 ) )
#define EMPTY( a ) ( a.f > a.b )

#define PUSH_VAL( a, v ) \
  { \
    if( FULL( a ) ) { \
      return qfalse; } \
    a.b++; \
    a.l[ a.b ].type = EXPR_VALUE; \
    a.l[ a.b ].u.val = v; \
  }

#define PUSH_OP( a, o ) \
  { \
    if( FULL( a ) ) { \
      return qfalse; } \
    a.b++; \
    a.l[ a.b ].type = EXPR_OPERATOR; \
    a.l[ a.b ].u.op = o; \
  }

#define POP_STACK( a ) \
  { \
    if( EMPTY( a ) ) { \
      return qfalse; } \
    value = &a.l[ a.b ]; \
    a.b--; \
  }

#define PEEK_STACK_OP( a )  ( a.l[ a.b ].u.op )
#define PEEK_STACK_VAL( a ) ( a.l[ a.b ].u.val )

#define POP_FIFO( a ) \
  { \
    if( EMPTY( a ) ) { \
      return qfalse; } \
    value = &a.l[ a.f ]; \
    a.f++; \
  }

	stack.f = fifo.f = 0;
	stack.b = fifo.b = -1;

	while ( trap_Parse_ReadToken( handle, &token ) )
	{
		if ( !unmatchedParentheses && token.string[ 0 ] == ')' )
		{
			break;
		}

		// Special case to catch negative numbers
		if ( expectingNumber && token.string[ 0 ] == '-' )
		{
			if ( !trap_Parse_ReadToken( handle, &token ) )
			{
				return qfalse;
			}

			token.floatvalue = -token.floatvalue;
		}

		if ( token.type == TT_NUMBER )
		{
			if ( !expectingNumber )
			{
				return qfalse;
			}

			expectingNumber = !expectingNumber;

			PUSH_VAL( fifo, token.floatvalue );
		}
		else
		{
			switch ( token.string[ 0 ] )
			{
				case '(':
					unmatchedParentheses++;
					PUSH_OP( stack, '(' );
					break;

				case ')':
					unmatchedParentheses--;

					if ( unmatchedParentheses < 0 )
					{
						return qfalse;
					}

					while ( !EMPTY( stack ) && PEEK_STACK_OP( stack ) != '(' )
					{
						POP_STACK( stack );
						PUSH_OP( fifo, value->u.op );
					}

					// Pop the '('
					POP_STACK( stack );

					break;

				case '*':
				case '/':
				case '+':
				case '-':
					if ( expectingNumber )
					{
						return qfalse;
					}

					expectingNumber = !expectingNumber;

					if ( EMPTY( stack ) )
					{
						PUSH_OP( stack, token.string[ 0 ] );
					}
					else
					{
						while ( !EMPTY( stack ) && OpPrec( token.string[ 0 ] ) < OpPrec( PEEK_STACK_OP( stack ) ) )
						{
							POP_STACK( stack );
							PUSH_OP( fifo, value->u.op );
						}

						PUSH_OP( stack, token.string[ 0 ] );
					}

					break;

				default:
					// Unknown token
					return qfalse;
			}
		}
	}

	while ( !EMPTY( stack ) )
	{
		POP_STACK( stack );
		PUSH_OP( fifo, value->u.op );
	}

	while ( !EMPTY( fifo ) )
	{
		POP_FIFO( fifo );

		if ( value->type == EXPR_VALUE )
		{
			PUSH_VAL( stack, value->u.val );
		}
		else if ( value->type == EXPR_OPERATOR )
		{
			char  op = value->u.op;
			float operand1, operand2, result;

			POP_STACK( stack );
			operand2 = value->u.val;
			POP_STACK( stack );
			operand1 = value->u.val;

			switch ( op )
			{
				case '*':
					result = operand1 * operand2;
					break;

				case '/':
					result = operand1 / operand2;
					break;

				case '+':
					result = operand1 + operand2;
					break;

				case '-':
					result = operand1 - operand2;
					break;

				default:
					Com_Error( ERR_FATAL, "Unknown operator '%c' in postfix string", op );
			}

			PUSH_VAL( stack, result );
		}
	}

	POP_STACK( stack );

	*f = value->u.val;

	return qtrue;

#undef FULL
#undef EMPTY
#undef PUSH_VAL
#undef PUSH_OP
#undef POP_STACK
#undef PEEK_STACK_OP
#undef PEEK_STACK_VAL
#undef POP_FIFO
}

/*
=================
PC_Float_Parse
=================
*/
qboolean PC_Float_Parse( int handle, float *f )
{
	pc_token_t token;
	int        negative = qfalse;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( token.string[ 0 ] == '(' )
	{
		return PC_Expression_Parse( handle, f );
	}

	if ( token.string[ 0 ] == '-' )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		negative = qtrue;
	}

	if ( token.type != TT_NUMBER )
	{
		PC_SourceError( handle, "expected float but found %s", token.string );
		return qfalse;
	}

	if ( negative )
	{
		*f = -token.floatvalue;
	}
	else
	{
		*f = token.floatvalue;
	}

	return qtrue;
}

/*
=================
Color_Parse
=================
*/
qboolean Color_Parse( char **p, vec4_t *c )
{
	int   i;
	float f;

	for ( i = 0; i < 4; i++ )
	{
		if ( !Float_Parse( p, &f ) )
		{
			return qfalse;
		}

		( *c ) [ i ] = f;
	}

	return qtrue;
}

/*
=================
PC_Color_Parse
=================
*/
qboolean PC_Color_Parse( int handle, vec4_t *c )
{
	int   i;
	float f;

	for ( i = 0; i < 4; i++ )
	{
		if ( !PC_Float_Parse( handle, &f ) )
		{
			return qfalse;
		}

		( *c ) [ i ] = f;
	}

	return qtrue;
}

/*
=================
Int_Parse
=================
*/
qboolean Int_Parse( char **p, int *i )
{
	char *token;
	token = COM_ParseExt( p, qfalse );

	if ( token && token[ 0 ] != 0 )
	{
		*i = atoi( token );
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
=================
PC_Int_Parse
=================
*/
qboolean PC_Int_Parse( int handle, int *i )
{
	pc_token_t token;
	int        negative = qfalse;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( token.string[ 0 ] == '(' )
	{
		float f;

		if ( PC_Expression_Parse( handle, &f ) )
		{
			*i = ( int ) f;
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	if ( token.string[ 0 ] == '-' )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		negative = qtrue;
	}

	if ( token.type != TT_NUMBER )
	{
		PC_SourceError( handle, "expected integer but found %s", token.string );
		return qfalse;
	}

	*i = token.intvalue;

	if ( negative )
	{
		*i = -*i;
	}

	return qtrue;
}

/*
=================
Rect_Parse
=================
*/
qboolean Rect_Parse( char **p, rectDef_t *r )
{
	if ( Float_Parse( p, &r->x ) )
	{
		if ( Float_Parse( p, &r->y ) )
		{
			if ( Float_Parse( p, &r->w ) )
			{
				if ( Float_Parse( p, &r->h ) )
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

/*
=================
PC_Rect_Parse
=================
*/
qboolean PC_Rect_Parse( int handle, rectDef_t *r )
{
	if ( PC_Float_Parse( handle, &r->x ) )
	{
		if ( PC_Float_Parse( handle, &r->y ) )
		{
			if ( PC_Float_Parse( handle, &r->w ) )
			{
				if ( PC_Float_Parse( handle, &r->h ) )
				{
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

/*
=================
String_Parse
=================
*/
qboolean String_Parse( char **p, const char **out )
{
	char *token;

	token = COM_ParseExt( p, qfalse );

	if ( token && token[ 0 ] != 0 )
	{
		* ( out ) = BG_strdup( token );
		return qtrue;
	}

	return qfalse;
}

/*
=================
PC_String_Parse
=================
*/
qboolean PC_String_Parse( int handle, const char **out )
{
	pc_token_t token;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	* ( out ) = BG_strdup( token.string );

	return qtrue;
}

qboolean PC_String_ParseTranslate( int handle, const char **out )
{
	pc_token_t token;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	* ( out ) = BG_strdup( _(token.string) );

	return qtrue;
}


/*
================
PC_Char_Parse
================
*/
qboolean PC_Char_Parse( int handle, char *out )
{
	pc_token_t token;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	*out = token.string[ 0 ];

	return qtrue;
}

/*
=================
PC_Script_Parse
=================
*/
qboolean PC_Script_Parse( int handle, const char **out )
{
	char       script[ 1024 ];
	pc_token_t token;

	memset( script, 0, sizeof( script ) );
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return qfalse;
	}

	if ( Q_stricmp( token.string, "{" ) != 0 )
	{
		return qfalse;
	}

	while ( 1 )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return qfalse;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 )
		{
			*out = BG_strdup( script );
			return qtrue;
		}

		if ( token.string[ 1 ] != '\0' )
		{
			Q_strcat( script, 1024, va( "\"%s\"", token.string ) );
		}
		else
		{
			Q_strcat( script, 1024, token.string );
		}

		Q_strcat( script, 1024, " " );
	}

	return qfalse;
}
