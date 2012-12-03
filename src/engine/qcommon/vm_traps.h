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

#ifndef __VM_TRAPS_H
#define __VM_TRAPS_H

// Major: API breakage
#define SYSCALL_ABI_VERSION_MAJOR 4
// Minor: API extension
#define SYSCALL_ABI_VERSION_MINOR 1

// First VM-specific call no.
#define FIRST_VM_SYSCALL 256

// Calls common to all VMs. Call nos. must be between 0 and FIRST_VM_SYSCALL
// Comments in this enum are used by the QVM API scanner
typedef enum sharedImport_s
{
  TRAP_MEMSET,               // = memset
  TRAP_MEMCPY,               // = memcpy
  TRAP_MEMCMP,               // = memcmp
  TRAP_STRNCPY,              // = strncpy
  TRAP_SIN,                  // = sin
  TRAP_COS,                  // = cos
  TRAP_ASIN,                 // = asin
//.TRAP_ACOS,                 // = acos
  TRAP_TAN,                  // = tanf
  TRAP_ATAN,                 // = atanf
  TRAP_ATAN2,                // = atan2
  TRAP_SQRT,                 // = sqrt
  TRAP_FLOOR,                // = floor
  TRAP_CEIL,                 // = ceil

  TRAP_MATRIXMULTIPLY = 128, // unused
  TRAP_ANGLEVECTORS,         // unused
  TRAP_PERPENDICULARVECTOR,  // unused

  TRAP_VERSION = 255
} sharedTraps_t;

void trap_SyscallABIVersion( int, int );

#endif
