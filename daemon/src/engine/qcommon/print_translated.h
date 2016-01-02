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

// Function body for translated text printing from cgame/ui & server code
// Macros required:
//   TRANSLATE_FUNC            - name of the translation function, called from this code
//   PLURAL_TRANSLATE_FUNC     - name of the translation function for plurals, called from this code
// If needed:
//   Cmd_Argv
//   Cmd_Argc

#if !defined TRANSLATE_FUNC && !defined PLURAL_TRANSLATE_FUNC
#error No translation function? Fail!
#endif

static const char *TranslateText_Internal( bool plural, int firstTextArg )
{
	static char str[ MAX_STRING_CHARS ];
	char        buf[ MAX_STRING_CHARS ];
	const char  *in;
	int         c, i = 0, totalArgs;

	totalArgs = Cmd_Argc();

	if ( plural )
	{
		int        number = atoi( Cmd_Argv( firstTextArg ) );
		const char *text = Cmd_Argv( ++firstTextArg );

		Q_strncpyz( buf, PLURAL_TRANSLATE_FUNC( text, text, number ), sizeof( buf ) );
	}
	else
	{
		Q_strncpyz( buf, TRANSLATE_FUNC( Cmd_Argv( firstTextArg ) ), sizeof( buf ) );
	}

	in = buf;
	memset( &str, 0, sizeof( str ) );

	while( *in )
	{
		c = *in;

		if( c == '$' )
		{
			const char *number = ++in;

			if( *in == '$' )
			{
				goto literal;
			}

			while( *in )
			{
				if( Str::cisdigit( *in ) )
				{
					in++;

					if( *in == 't' && *(in+1) == '$' )
					{
						int num = atoi( number );

						if( num >= 0 && num < totalArgs )
						{
							const char *translated = TRANSLATE_FUNC( Cmd_Argv( num + firstTextArg ) );
							int         length = strlen( translated );

							i += length;

							if( i >= MAX_STRING_CHARS )
							{
								Com_Printf( "%s", str );
								memset( &str, 0, sizeof( str ) );
								i = length;
							}

							Q_strcat( str, sizeof( str ), translated );
						}

						in += 2;
						break;
					}
					else if( *in == '$' )
					{
						int num = atoi( number );

						if( num >= 0 && num < totalArgs )
						{
							const char *translated = TRANSLATE_FUNC( Cmd_Argv( num + firstTextArg ) );
							int         length = strlen( translated );

							i += length;

							if( i >= MAX_STRING_CHARS )
							{
								Com_Printf( "%s", str );
								memset( &str, 0, sizeof( str ) );
								i = length;
							}

							Q_strcat( str, sizeof( str ), translated );
						}

						in++;
						break;
					}
				}
				else
				{
					// invalid sequence
					c = '$';
					goto literal;
				}
			}
		}
		else
		{
			// arrive here for literal '$' or invalid sequence
			// in has not yet been incremented
			// c contains the character to be appended
			literal:

			if( i < MAX_STRING_CHARS )
			{
				str[ i++ ] = c;
			}
			else
			{
				Com_Printf( "%s", str );
				memset( &str, 0, sizeof( str ) );
				str[ 0 ] = c;
				i = 1;
			}

			++in;
		}
	}

	return str;
}
