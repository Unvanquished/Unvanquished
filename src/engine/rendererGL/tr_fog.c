/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

// tr_fog.c

#include "tr_local.h"

//static qboolean        fogIsOn = qfalse;

/*
=================
R_Fog (void)
=================
*/
void RB_Fog( glfog_t *curfog )
{
//	static glfog_t  setfog;

	GLimp_LogComment( "--- RB_Fog() ---\n" );

#if 0

	if ( !r_wolfFog->integer )
	{
		RB_FogOff();
		return;
	}

	if ( !curfog->registered )
	{
		//----(SA)
		RB_FogOff();
		return;
	}

	//----(SA) assme values of '0' for these parameters means 'use default'
	if ( !curfog->density )
	{
		curfog->density = 1;
	}

	if ( !curfog->hint )
	{
		curfog->hint = GL_DONT_CARE;
	}

	if ( !curfog->mode )
	{
		curfog->mode = GL_LINEAR;
	}

	//----(SA)  end

	RB_FogOn();

	// only send changes if necessary

//  if(curfog->mode != setfog.mode || !setfog.registered) {
	glFogi( GL_FOG_MODE, curfog->mode );
//      setfog.mode = curfog->mode;
//  }
//  if(curfog->color[0] != setfog.color[0] || curfog->color[1] != setfog.color[1] || curfog->color[2] != setfog.color[2] || !setfog.registered) {
	glFogfv( GL_FOG_COLOR, curfog->color );
//      VectorCopy(setfog.color, curfog->color);
//  }
//  if(curfog->density != setfog.density || !setfog.registered) {
	glFogf( GL_FOG_DENSITY, curfog->density );
//      setfog.density = curfog->density;
//  }
//  if(curfog->hint != setfog.hint || !setfog.registered) {
	glHint( GL_FOG_HINT, curfog->hint );
//      setfog.hint = curfog->hint;
//  }
//  if(curfog->start != setfog.start || !setfog.registered) {
	glFogf( GL_FOG_START, curfog->start );
//      setfog.start = curfog->start;
//  }

	if ( r_zfar->value )
	{
		// (SA) allow override for helping level designers test fog distances
//      if(setfog.end != r_zfar->value || !setfog.registered) {
		glFogf( GL_FOG_END, r_zfar->value );
//          setfog.end = r_zfar->value;
//      }
	}
	else
	{
//      if(curfog->end != setfog.end || !setfog.registered) {
		glFogf( GL_FOG_END, curfog->end );
//          setfog.end = curfog->end;
//      }
	}

// TTimo - from SP NV fog code
	// NV fog mode
	if ( glConfig.NVFogAvailable )
	{
		glFogi( GL_FOG_DISTANCE_MODE_NV, glConfig.NVFogMode );
	}

// end

	setfog.registered = qtrue;

	GL_ClearColor( curfog->color[ 0 ], curfog->color[ 1 ], curfog->color[ 2 ], curfog->color[ 3 ] );
#endif
}

void RB_FogOff()
{
	GLimp_LogComment( "--- RB_FogOff() ---\n" );

#if 0

	if ( !fogIsOn )
	{
		return;
	}

	glDisable( GL_FOG );
	fogIsOn = qfalse;
#endif
}

void RB_FogOn()
{
	GLimp_LogComment( "--- RB_FogOn() ---\n" );

#if 0

	if ( fogIsOn )
	{
		return;
	}

//  if(r_uiFullScreen->integer) {   // don't fog in the menu
//      R_FogOff();
//      return;
//  }

	if ( !r_wolfFog->integer )
	{
		return;
	}

//  if(backEnd.viewParms.isGLFogged) {
//      if(!(backEnd.viewParms.glFog.registered))
//          return;
//  }

	if ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL )
	{
		// don't force world fog on portal sky
		if ( !( tr.glfogsettings[ FOG_PORTALVIEW ].registered ) )
		{
			return;
		}
	}
	else if ( !tr.glfogNum )
	{
		return;
	}

	glEnable( GL_FOG );
	fogIsOn = qtrue;
#endif
}

/*
==============
RE_SetFog

  if fogvar == FOG_CMD_SWITCHFOG {
        fogvar is the command
        var1 is the fog to switch to
        var2 is the time to transition
  }
  else {
        fogvar is the fog that's being set
        var1 is the near fog z value
        var2 is the far fog z value
        rgb = color
        density is density, and is used to derive the values of 'mode', 'drawsky', and 'clearscreen'
  }
==============
*/
void RE_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density )
{
	ri.Printf( PRINT_DEVELOPER, "RE_SetFog( fogvar = %i, var1 = %i, var2 = %i, r = %f, g = %f, b = %f, density = %f )\n",
	           fogvar, var1, var2, r, g, b, density );

	if ( fogvar != FOG_CMD_SWITCHFOG )
	{
		// just set the parameters and return
		if ( var1 == 0 && var2 == 0 )
		{
			// clear this fog
			tr.glfogsettings[ fogvar ].registered = qfalse;
			return;
		}

		tr.glfogsettings[ fogvar ].color[ 0 ] = r * tr.identityLight;
		tr.glfogsettings[ fogvar ].color[ 1 ] = g * tr.identityLight;
		tr.glfogsettings[ fogvar ].color[ 2 ] = b * tr.identityLight;
		tr.glfogsettings[ fogvar ].color[ 3 ] = 1;
		tr.glfogsettings[ fogvar ].start      = var1;
		tr.glfogsettings[ fogvar ].end        = var2;

		if ( density > 1 )
		{
			tr.glfogsettings[ fogvar ].mode        = GL_LINEAR;
			tr.glfogsettings[ fogvar ].drawsky     = qfalse;
			tr.glfogsettings[ fogvar ].clearscreen = qtrue;
			tr.glfogsettings[ fogvar ].density     = 1.0;
		}
		else
		{
			tr.glfogsettings[ fogvar ].mode        = GL_EXP;
			tr.glfogsettings[ fogvar ].drawsky     = qtrue;
			tr.glfogsettings[ fogvar ].clearscreen = qfalse;
			tr.glfogsettings[ fogvar ].density     = density;
		}

		tr.glfogsettings[ fogvar ].hint       = GL_DONT_CARE;
		tr.glfogsettings[ fogvar ].registered = qtrue;

		return;
	}

	// don't switch to invalid fogs
	if ( tr.glfogsettings[ var1 ].registered != qtrue )
	{
		return;
	}

	tr.glfogNum = var1;

	// transitioning to new fog, store the current values as the 'from'

	if ( tr.glfogsettings[ FOG_CURRENT ].registered )
	{
		memcpy( &tr.glfogsettings[ FOG_LAST ], &tr.glfogsettings[ FOG_CURRENT ], sizeof( glfog_t ) );
	}
	else
	{
		// if no current fog fall back to world fog
		// FIXME: handle transition if there is no FOG_MAP fog
		memcpy( &tr.glfogsettings[ FOG_LAST ], &tr.glfogsettings[ FOG_MAP ], sizeof( glfog_t ) );
	}

	memcpy( &tr.glfogsettings[ FOG_TARGET ], &tr.glfogsettings[ tr.glfogNum ], sizeof( glfog_t ) );

	// setup transition times
	tr.glfogsettings[ FOG_TARGET ].startTime  = tr.refdef.time;
	tr.glfogsettings[ FOG_TARGET ].finishTime = tr.refdef.time + var2;
}

/*
==============
R_SetFrameFog
==============
*/
void R_SetFrameFog()
{
	if ( !tr.world )
	{
		return;
	}

	// Arnout: new style global fog transitions
	if ( tr.world->globalFogTransEndTime )
	{
		if ( tr.world->globalFogTransEndTime >= tr.refdef.time )
		{
			float lerpPos;
			int   fadeTime;

			fadeTime = tr.world->globalFogTransEndTime - tr.world->globalFogTransStartTime;
			lerpPos  = ( float )( tr.refdef.time - tr.world->globalFogTransStartTime ) / ( float )fadeTime;

			if ( lerpPos > 1 )
			{
				lerpPos = 1;
			}

			tr.world->fogs[ tr.world->globalFog ].fogParms.color[ 0 ] =
			  tr.world->globalTransStartFog[ 0 ] +
			  ( ( tr.world->globalTransEndFog[ 0 ] - tr.world->globalTransStartFog[ 0 ] ) * lerpPos );
			tr.world->fogs[ tr.world->globalFog ].fogParms.color[ 1 ] =
			  tr.world->globalTransStartFog[ 1 ] +
			  ( ( tr.world->globalTransEndFog[ 1 ] - tr.world->globalTransStartFog[ 1 ] ) * lerpPos );
			tr.world->fogs[ tr.world->globalFog ].fogParms.color[ 2 ] =
			  tr.world->globalTransStartFog[ 2 ] +
			  ( ( tr.world->globalTransEndFog[ 2 ] - tr.world->globalTransStartFog[ 2 ] ) * lerpPos );

			/*
			tr.world->fogs[tr.world->globalFog].fogParms.colorInt =
			        ColorBytes4(tr.world->fogs[tr.world->globalFog].fogParms.color[0] * tr.identityLight,
			                                tr.world->fogs[tr.world->globalFog].fogParms.color[1] * tr.identityLight,
			                                tr.world->fogs[tr.world->globalFog].fogParms.color[2] * tr.identityLight, 1.0);
			*/

			tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque =
			  tr.world->globalTransStartFog[ 3 ] +
			  ( ( tr.world->globalTransEndFog[ 3 ] - tr.world->globalTransStartFog[ 3 ] ) * lerpPos );
			tr.world->fogs[ tr.world->globalFog ].tcScale                 =
			  1.0f / ( tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque * 8 );
		}
		else
		{
			// transition complete
			VectorCopy( tr.world->globalTransEndFog, tr.world->fogs[ tr.world->globalFog ].fogParms.color );

			/*
			tr.world->fogs[tr.world->globalFog].fogParms.colorInt =
			        ColorBytes4(tr.world->globalTransEndFog[0] * tr.identityLight, tr.world->globalTransEndFog[1] * tr.identityLight,
			                                tr.world->globalTransEndFog[2] * tr.identityLight, 1.0);
			*/
			tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque = tr.world->globalTransEndFog[ 3 ];
			tr.world->fogs[ tr.world->globalFog ].tcScale                 =
			  1.0f / ( tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque * 8 );

			tr.world->globalFogTransEndTime                               = 0;
		}
	}

	if ( r_speeds->integer == RSPEEDS_FOG )
	{
		if ( !tr.glfogsettings[ FOG_TARGET ].registered )
		{
			ri.Printf( PRINT_ALL, "no fog - calc zFar: %0.1f\n", tr.viewParms.zFar );
			return;
		}
	}

	// DHM - Nerve :: If fog is not valid, don't use it
	if ( !tr.glfogsettings[ FOG_TARGET ].registered )
	{
		return;
	}

	// still fading
	if ( tr.glfogsettings[ FOG_TARGET ].finishTime && tr.glfogsettings[ FOG_TARGET ].finishTime >= tr.refdef.time )
	{
		float lerpPos;
		int   fadeTime;

		// transitioning from density to distance
		if ( tr.glfogsettings[ FOG_LAST ].mode == GL_EXP && tr.glfogsettings[ FOG_TARGET ].mode == GL_LINEAR )
		{
			// for now just fast transition to the target when dissimilar fogs are
			memcpy( &tr.glfogsettings[ FOG_CURRENT ], &tr.glfogsettings[ FOG_TARGET ], sizeof( glfog_t ) );
			tr.glfogsettings[ FOG_TARGET ].finishTime = 0;
		}
		// transitioning from distance to density
		else if ( tr.glfogsettings[ FOG_LAST ].mode == GL_LINEAR && tr.glfogsettings[ FOG_TARGET ].mode == GL_EXP )
		{
			memcpy( &tr.glfogsettings[ FOG_CURRENT ], &tr.glfogsettings[ FOG_TARGET ], sizeof( glfog_t ) );
			tr.glfogsettings[ FOG_TARGET ].finishTime = 0;
		}
		// transitioning like fog modes
		else
		{
			fadeTime = tr.glfogsettings[ FOG_TARGET ].finishTime - tr.glfogsettings[ FOG_TARGET ].startTime;

			if ( fadeTime <= 0 )
			{
				fadeTime = 1;   // avoid divide by zero
			}

			lerpPos = ( float )( tr.refdef.time - tr.glfogsettings[ FOG_TARGET ].startTime ) / ( float )fadeTime;

			if ( lerpPos > 1 )
			{
				lerpPos = 1;
			}

			// lerp near/far
			tr.glfogsettings[ FOG_CURRENT ].start =
			  tr.glfogsettings[ FOG_LAST ].start + ( ( tr.glfogsettings[ FOG_TARGET ].start - tr.glfogsettings[ FOG_LAST ].start ) * lerpPos );
			tr.glfogsettings[ FOG_CURRENT ].end   =
			  tr.glfogsettings[ FOG_LAST ].end + ( ( tr.glfogsettings[ FOG_TARGET ].end - tr.glfogsettings[ FOG_LAST ].end ) * lerpPos );

			// lerp color
			tr.glfogsettings[ FOG_CURRENT ].color[ 0 ] =
			  tr.glfogsettings[ FOG_LAST ].color[ 0 ] +
			  ( ( tr.glfogsettings[ FOG_TARGET ].color[ 0 ] - tr.glfogsettings[ FOG_LAST ].color[ 0 ] ) * lerpPos );
			tr.glfogsettings[ FOG_CURRENT ].color[ 1 ] =
			  tr.glfogsettings[ FOG_LAST ].color[ 1 ] +
			  ( ( tr.glfogsettings[ FOG_TARGET ].color[ 1 ] - tr.glfogsettings[ FOG_LAST ].color[ 1 ] ) * lerpPos );
			tr.glfogsettings[ FOG_CURRENT ].color[ 2 ] =
			  tr.glfogsettings[ FOG_LAST ].color[ 2 ] +
			  ( ( tr.glfogsettings[ FOG_TARGET ].color[ 2 ] - tr.glfogsettings[ FOG_LAST ].color[ 2 ] ) * lerpPos );

			tr.glfogsettings[ FOG_CURRENT ].density     = tr.glfogsettings[ FOG_TARGET ].density;
			tr.glfogsettings[ FOG_CURRENT ].mode        = tr.glfogsettings[ FOG_TARGET ].mode;
			tr.glfogsettings[ FOG_CURRENT ].registered  = qtrue;

			// if either fog in the transition clears the screen, clear the background this frame to avoid hall of mirrors
			tr.glfogsettings[ FOG_CURRENT ].clearscreen = ( tr.glfogsettings[ FOG_TARGET ].clearscreen ||
			    tr.glfogsettings[ FOG_LAST ].clearscreen );
		}
	}
	else
	{
		// probably usually not necessary to copy the whole thing.
		// potential FIXME: since this is the most common occurance, diff first and only set changes
		memcpy( &tr.glfogsettings[ FOG_CURRENT ], &tr.glfogsettings[ FOG_TARGET ], sizeof( glfog_t ) );
	}

	// shorten the far clip if the fog opaque distance is closer than the procedural farcip dist
	if ( tr.glfogsettings[ FOG_CURRENT ].mode == GL_LINEAR )
	{
		if ( tr.glfogsettings[ FOG_CURRENT ].end < tr.viewParms.zFar )
		{
			tr.viewParms.zFar = tr.glfogsettings[ FOG_CURRENT ].end;
		}
	}

//  else
//      tr.glfogsettings[FOG_CURRENT].end = 5;

	if ( r_speeds->integer == RSPEEDS_FOG )
	{
		if ( tr.glfogsettings[ FOG_CURRENT ].mode == GL_LINEAR )
		{
			ri.Printf( PRINT_ALL, "farclip fog - den: %0.1f  calc zFar: %0.1f  fog zfar: %0.1f\n",
			           tr.glfogsettings[ FOG_CURRENT ].density, tr.viewParms.zFar, tr.glfogsettings[ FOG_CURRENT ].end );
		}
		else
		{
			ri.Printf( PRINT_ALL, "density fog - den: %0.4f  calc zFar: %0.1f  fog zFar: %0.1f\n",
			           tr.glfogsettings[ FOG_CURRENT ].density, tr.viewParms.zFar, tr.glfogsettings[ FOG_CURRENT ].end );
		}
	}
}

/*
====================
RE_SetGlobalFog
        rgb = colour
        depthForOpaque is depth for opaque

        the restore flag can be used to restore the original level fog
        duration can be set to fade over a certain period
====================
*/
void RE_SetGlobalFog( qboolean restore, int duration, float r, float g, float b, float depthForOpaque )
{
	ri.Printf( PRINT_DEVELOPER, "RE_SetGlobalFog( restore = %i, duration = %i, r = %f, g = %f, b = %f, depthForOpaque = %f )\n",
	           restore, duration, r, g, b, depthForOpaque );

	if ( restore )
	{
		if ( duration > 0 )
		{
			VectorCopy( tr.world->fogs[ tr.world->globalFog ].fogParms.color, tr.world->globalTransStartFog );
			tr.world->globalTransStartFog[ 3 ] = tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque;

			Vector4Copy( tr.world->globalOriginalFog, tr.world->globalTransEndFog );

			tr.world->globalFogTransStartTime = tr.refdef.time;
			tr.world->globalFogTransEndTime   = tr.refdef.time + duration;
		}
		else
		{
			VectorCopy( tr.world->globalOriginalFog, tr.world->fogs[ tr.world->globalFog ].fogParms.color );

			/*
			tr.world->fogs[tr.world->globalFog].fogParms.colorInt =
			        ColorBytes4(tr.world->globalOriginalFog[0] * tr.identityLight, tr.world->globalOriginalFog[1] * tr.identityLight,
			                                tr.world->globalOriginalFog[2] * tr.identityLight, 1.0);
			*/
			tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque = tr.world->globalOriginalFog[ 3 ];
			tr.world->fogs[ tr.world->globalFog ].tcScale                 =
			  1.0f / ( tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque );
		}
	}
	else
	{
		if ( duration > 0 )
		{
			VectorCopy( tr.world->fogs[ tr.world->globalFog ].fogParms.color, tr.world->globalTransStartFog );
			tr.world->globalTransStartFog[ 3 ] = tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque;

			VectorSet( tr.world->globalTransEndFog, r, g, b );
			tr.world->globalTransEndFog[ 3 ]   = depthForOpaque < 1 ? 1 : depthForOpaque;

			tr.world->globalFogTransStartTime  = tr.refdef.time;
			tr.world->globalFogTransEndTime    = tr.refdef.time + duration;
		}
		else
		{
			VectorSet( tr.world->fogs[ tr.world->globalFog ].fogParms.color, r, g, b );

			/*
			tr.world->fogs[tr.world->globalFog].fogParms.colorInt = ColorBytes4(r * tr.identityLight,
			                                                                                                                                                        g * tr.identityLight,
			                                                                                                                                                        b * tr.identityLight, 1.0);
			                                                                                                                                                        */
			tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque = depthForOpaque < 1 ? 1 : depthForOpaque;
			tr.world->fogs[ tr.world->globalFog ].tcScale                 =
			  1.0f / ( tr.world->fogs[ tr.world->globalFog ].fogParms.depthForOpaque );
		}
	}
}
