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
void PRINTF_LIKE(2) PC_SourceError( int handle, const char *format, ... )
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
bool Float_Parse( const char **p, float *f )
{
	char *token;
	token = COM_ParseExt( p, false );

	if ( token && token[ 0 ] != 0 )
	{
		*f = atof( token );
		return true;
	}
	else
	{
		return false;
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
static bool PC_Expression_Parse( int handle, float *f )
{
	pc_token_t  token;
	int         unmatchedParentheses = 0;
	exprList_t  stack, fifo;
	exprToken_t *value;
	bool    expectingNumber = true;

#define FULL( a )  ( a.b >= ( MAX_EXPR_ELEMENTS - 1 ) )
#define EMPTY( a ) ( a.f > a.b )

#define PUSH_VAL( a, v ) \
  { \
    if( FULL( a ) ) { \
      return false; } \
    a.b++; \
    a.l[ a.b ].type = EXPR_VALUE; \
    a.l[ a.b ].u.val = v; \
  }

#define PUSH_OP( a, o ) \
  { \
    if( FULL( a ) ) { \
      return false; } \
    a.b++; \
    a.l[ a.b ].type = EXPR_OPERATOR; \
    a.l[ a.b ].u.op = o; \
  }

#define POP_STACK( a ) \
  { \
    if( EMPTY( a ) ) { \
      return false; } \
    value = &a.l[ a.b ]; \
    a.b--; \
  }

#define PEEK_STACK_OP( a )  ( a.l[ a.b ].u.op )
#define PEEK_STACK_VAL( a ) ( a.l[ a.b ].u.val )

#define POP_FIFO( a ) \
  { \
    if( EMPTY( a ) ) { \
      return false; } \
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
				return false;
			}

			token.floatvalue = -token.floatvalue;
		}

		if ( token.type == TT_NUMBER )
		{
			if ( !expectingNumber )
			{
				return false;
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
						return false;
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
						return false;
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
					return false;
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

	return true;

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
bool PC_Float_Parse( int handle, float *f )
{
	pc_token_t token;
	int        negative = false;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return false;
	}

	if ( token.string[ 0 ] == '(' )
	{
		return PC_Expression_Parse( handle, f );
	}

	if ( token.string[ 0 ] == '-' )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return false;
		}

		negative = true;
	}

	if ( token.type != TT_NUMBER )
	{
		PC_SourceError( handle, "expected float but found %s", token.string );
		return false;
	}

	if ( negative )
	{
		*f = -token.floatvalue;
	}
	else
	{
		*f = token.floatvalue;
	}

	return true;
}

/*
=================
Color_Parse
=================
*/
bool Color_Parse( const char **p, Color::Color* c )
{
	int   i;
	float f;

	for ( i = 0; i < 4; i++ )
	{
		if ( !Float_Parse( p, &f ) )
		{
			return false;
		}

		c->ToArray()[ i ] = f;
	}

	return true;
}

/*
=================
PC_Color_Parse
=================
*/
bool PC_Color_Parse( int handle, Color::Color *c )
{
	int   i;
	float f;

	for ( i = 0; i < 4; i++ )
	{
		if ( !PC_Float_Parse( handle, &f ) )
		{
			return false;
		}

		c->ToArray()[ i ] = f;
	}

	return true;
}

/*
=================
Int_Parse
=================
*/
bool Int_Parse( const char **p, int *i )
{
	char *token;
	token = COM_ParseExt( p, false );

	if ( token && token[ 0 ] != 0 )
	{
		*i = atoi( token );
		return true;
	}
	else
	{
		return false;
	}
}

/*
=================
PC_Int_Parse
=================
*/
bool PC_Int_Parse( int handle, int *i )
{
	pc_token_t token;
	int        negative = false;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return false;
	}

	if ( token.string[ 0 ] == '(' )
	{
		float f;

		if ( PC_Expression_Parse( handle, &f ) )
		{
			*i = ( int ) f;
			return true;
		}
		else
		{
			return false;
		}
	}

	if ( token.string[ 0 ] == '-' )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return false;
		}

		negative = true;
	}

	if ( token.type != TT_NUMBER )
	{
		PC_SourceError( handle, "expected integer but found %s", token.string );
		return false;
	}

	*i = token.intvalue;

	if ( negative )
	{
		*i = -*i;
	}

	return true;
}

/*
=================
Rect_Parse
=================
*/
bool Rect_Parse( const char **p, rectDef_t *r )
{
	if ( Float_Parse( p, &r->x ) )
	{
		if ( Float_Parse( p, &r->y ) )
		{
			if ( Float_Parse( p, &r->w ) )
			{
				if ( Float_Parse( p, &r->h ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

/*
=================
PC_Rect_Parse
=================
*/
bool PC_Rect_Parse( int handle, rectDef_t *r )
{
	if ( PC_Float_Parse( handle, &r->x ) )
	{
		if ( PC_Float_Parse( handle, &r->y ) )
		{
			if ( PC_Float_Parse( handle, &r->w ) )
			{
				if ( PC_Float_Parse( handle, &r->h ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

/*
=================
String_Parse
=================
*/
bool String_Parse( const char **p, const char **out )
{
	char *token;

	token = COM_ParseExt( p, false );

	if ( token && token[ 0 ] != 0 )
	{
		* ( out ) = BG_strdup( token );
		return true;
	}

	return false;
}

/*
=================
PC_String_Parse
=================
*/
bool PC_String_Parse( int handle, const char **out )
{
	pc_token_t token;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return false;
	}

	* ( out ) = BG_strdup( token.string );

	return true;
}

bool PC_String_ParseTranslate( int handle, const char **out )
{
	pc_token_t token;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return false;
	}

	* ( out ) = BG_strdup( _(token.string) );

	return true;
}


/*
================
PC_Char_Parse
================
*/
bool PC_Char_Parse( int handle, char *out )
{
	pc_token_t token;

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return false;
	}

	*out = token.string[ 0 ];

	return true;
}

/*
=================
PC_Script_Parse
=================
*/
bool PC_Script_Parse( int handle, const char **out )
{
	char       script[ 1024 ];
	pc_token_t token;

	memset( script, 0, sizeof( script ) );
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if ( !trap_Parse_ReadToken( handle, &token ) )
	{
		return false;
	}

	if ( Q_stricmp( token.string, "{" ) != 0 )
	{
		return false;
	}

	while ( 1 )
	{
		if ( !trap_Parse_ReadToken( handle, &token ) )
		{
			return false;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 )
		{
			*out = BG_strdup( script );
			return true;
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

	return false;
}
