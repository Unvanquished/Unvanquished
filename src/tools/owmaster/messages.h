/*
	messages.h

	Message management for dpmaster

	Copyright (C) 2004  Mathieu Olivier

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _MESSAGES_H_
#define _MESSAGES_H_


// ---------- Public functions ---------- //

// Parse a packet to figure out what to do with it
void HandleMessage (const char* msg, size_t length,
					const struct sockaddr_in* address);


#endif  // #ifndef _MESSAGES_H_
