/*
===========================================================================
Copyright (C) 2011-2014 Unvanquished Development

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

#include "../qcommon/qcommon.h"
#include "../qcommon/q_shared.h"
#include "sdl2_compat.h"

#if !defined(WIN32) && !defined(MACOS_X)
# include <X11/Xlib.h>
#endif

/*
==================
Sys_GetClipboardText

We support SDL 1.2 where SDL 2 isn't available as standard.
This, in practice, means Debian wheezy and other GNU/Linux of similar age.

However, using our own code allows Shift-Insert to use the correct source
(the primary selection). SDL 2 only supports the clipboard.
==================
*/
char *Sys_GetClipboardText( clipboard_t clip )
{
# if defined(WIN32) && !defined(MACOS_X)
	static struct {
		Display *display;
		Window  window;
		Atom    utf8;
	} x11 = { NULL };

	// derived from SDL clipboard code (http://hg.assembla.com/SDL_Clipboard)
	Window        owner;
	Atom          selection;
	Atom          converted;
	Atom          selectionType;
	int           selectionFormat;
	unsigned long nbytes;
	unsigned long overflow;
	char          *src;

	if ( x11.display == NULL )
	{
		SDL_SysWMinfo info;

		SDL_VERSION( &info.version );
		if ( SDL_GetWindowWMInfo( (SDL_Window*) IN_GetWindow(), &info ) != 1 || info.subsystem != SDL_SYSWM_X11 )
		{
#  if SDL_VERSION_ATLEAST(2, 0, 0)
			// fallback if SDL 2 is available
			char *buffer = SDL_GetClipboardText();

			if( buffer )
			{
				char *data = ( char * ) Z_Malloc( sizeof( buffer ) );
				Q_strncpyz( data, buffer, sizeof( buffer ) );
				SDL_free( buffer );
				return data;
			}
#  else
			// otherwise, whinge about no X
			Com_Printf("Not on X11? (%d)\n",info.subsystem);
#  endif
			return NULL;
		}

		x11.display = info.info.x11.display;
		x11.window = info.info.x11.window;
		x11.utf8 = XInternAtom( x11.display, "UTF8_STRING", False );

		SDL_EventState( SDL_SYSWMEVENT, SDL_ENABLE );
		//SDL_SetEventFilter( Sys_ClipboardFilter );
	}

	XLockDisplay( x11.display );

	switch ( clip )
	{
	// preference order; we use fall-through
	default: // SELECTION_CLIPBOARD
		selection = XInternAtom( x11.display, "CLIPBOARD", False );
		owner = XGetSelectionOwner( x11.display, selection );
		if ( owner != None && owner != x11.window )
		{
			break;
		}

	case SELECTION_PRIMARY:
		selection = XA_PRIMARY;
		owner = XGetSelectionOwner( x11.display, selection );
		if ( owner != None && owner != x11.window )
		{
			break;
		}

	case SELECTION_SECONDARY:
		selection = XA_SECONDARY;
		owner = XGetSelectionOwner( x11.display, selection );
	}

	converted = XInternAtom( x11.display, "UNVANQUISHED_SELECTION", False );
	XUnlockDisplay( x11.display );

	if ( owner == None || owner == x11.window )
	{
		selection = XA_CUT_BUFFER0;
		owner = RootWindow( x11.display, DefaultScreen( x11.display ) );
	}
	else
	{
		SDL_Event event;

		XLockDisplay( x11.display );
		owner = x11.window;
		//FIXME: when we can respond to clipboard requests, don't alter selection
		XConvertSelection( x11.display, selection, x11.utf8, converted, owner, CurrentTime );
		XUnlockDisplay( x11.display );

		for (;;)
		{
			SDL_WaitEvent( &event );

			if ( event.type == SDL_SYSWMEVENT )
			{
#  if SDL_VERSION_ATLEAST(2, 0, 0)
				XEvent xevent = event.syswm.msg->msg.x11.event;
#  else
				XEvent xevent = event.syswm.msg->event.xevent;
#  endif
				if ( xevent.type == SelectionNotify &&
				     xevent.xselection.requestor == owner )
				{
					break;
				}
			}
		}
	}

	XLockDisplay( x11.display );

	if ( XGetWindowProperty( x11.display, owner, converted, 0, INT_MAX / 4,
	                         False, x11.utf8, &selectionType, &selectionFormat,
	                         &nbytes, &overflow, (unsigned char **) &src ) == Success )
	{
		char *dest = NULL;

		if ( selectionType == x11.utf8 )
		{
			dest = (char*) Z_Malloc( nbytes + 1 );
			memcpy( dest, src, nbytes );
			dest[ nbytes ] = 0;
		}
		XFree( src );

		XUnlockDisplay( x11.display );
		return dest;
	}

	XUnlockDisplay( x11.display );
# elif SDL_VERSION_ATLEAST(2, 0, 0)
	char *buffer = SDL_GetClipboardText();
	char *data = NULL;

	if( !buffer )
	{
		return NULL;
	}

	data = ( char * ) Z_Malloc( sizeof( buffer ) );
	Q_strncpyz( data, buffer, sizeof( buffer ) );
	SDL_free( buffer );

	return data;
# endif
	return NULL;
}
