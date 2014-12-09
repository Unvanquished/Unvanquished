/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_demos.h"

demoListPoint_t *listPointAlloc( int size ) {
	demoListPoint_t * point;

	size += sizeof( demoListPoint_t );
	point = (demoListPoint_t *)malloc( size );
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, size );
	return point;
}

void listPointFree( demoListPoint_t *listStart[], demoListPoint_t *point ) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		listStart[0] = point->next;

	if (point->next ) 
		point->next->prev = point->prev;

	free( point );
}

demoListPoint_t *listPointSynch( demoListPoint_t *listStart[], int time ) {
	demoListPoint_t *point = listStart[0];
	if (!point)
		return 0;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

demoListPoint_t *ListPointAdd( demoListPoint_t *listStart[], int time, int flags, int size ) {
	demoListPoint_t *point = listPointSynch( listStart, time );
	demoListPoint_t *newPoint;
	if (!point || point->time > time) {
		newPoint = listPointAlloc( size );
		newPoint->next = listStart[0];
		if (listStart[0])
			listStart[0]->prev = newPoint;
		listStart[0] = newPoint;
		newPoint->prev = 0;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = listPointAlloc( size );
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	newPoint->time = time;
	newPoint->flags = flags;
	return newPoint;
}

void listPointMatch( const demoListPoint_t *point, int mask, const demoListPoint_t *match[4] ) {
	const demoListPoint_t *p;

	p = point;
	while (p && !(p->flags & mask))
		p = p->prev;

	if (!p) {
		match[0] = 0;
		match[1] = 0;
		p = point;
		while (p && !(p->flags & mask))
			p = p->next;
		if (!p) {
			match[2] = 0;
			match[3] = 0;
			return;
		}
	} else {
		match[1] = p;
		p = p->prev;
		while (p && !(p->flags & mask))
			p = p->prev;
		match[0] = p;
	}
	p = point->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[2] = p;
	if (p)
		p = p->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[3] = p;
}

void listMatchAt( demoListPoint_t *listStart[], int time, int mask, const demoListPoint_t *match[4], int times[4] ) {
	const demoListPoint_t *p;

	p = listStart[0];
	match[0] = match[1] = 0;
	while( p ) {
		if (p->time > time)
			break;
		if (p->flags & mask) {
			match[0] = match[1];
			match[1] = p;
		}
		p = p->next;
	}
	while (p && !(p->flags & mask))
		p = p->next;
	match[2] = p;
	if (p)
		p = p->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[3] = p;
}

