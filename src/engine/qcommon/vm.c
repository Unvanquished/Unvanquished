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

// vm.c -- virtual machine

/*


intermix code and data
symbol table

a dll has one imported function: VM_SystemCall
and one exported function: Perform


*/

#ifdef _MSC_VER
#include "../../libs/msinttypes/inttypes.h"
#else
#include <inttypes.h>
#endif

#include "vm_local.h"
#include "vm_traps.h"

#ifdef USE_LLVM
#include "vm_llvm.h"
#endif

vm_t       *currentVM = NULL;
vm_t       *lastVM = NULL;
int        vm_debugLevel;

// used by Com_Error to get rid of running VMs before longjmp
static int forced_unload;

#define MAX_VM 3
vm_t       vmTable[ MAX_VM ];

void       VM_VmInfo_f( void );
void       VM_VmProfile_f( void );

#if 0 // 64bit!
// converts a VM pointer to a C pointer and
// checks to make sure that the range is acceptable
void    *VM_VM2C( vmptr_t p, int length )
{
	return ( void * ) p;
}

#endif

void VM_Debug( int level )
{
	vm_debugLevel = level;
}

/*
==============
VM_Init
==============
*/
void VM_Init( void )
{
	// vm_* 0 means native libraries (.so, .dll, etc.) are used
	// vm_* 1 means virtual machines (.qvm, etc.) are used through an interpreter
	// vm_* 2 means virtual machines are used with JIT compiling
	Cvar_Get( "vm_cgame", "0", CVAR_ARCHIVE );
	Cvar_Get( "vm_game", "0", CVAR_ARCHIVE );
	Cvar_Get( "vm_ui", "0", CVAR_ARCHIVE );

	Cmd_AddCommand( "vmprofile", VM_VmProfile_f );
	Cmd_AddCommand( "vminfo", VM_VmInfo_f );

	Com_Memset( vmTable, 0, sizeof( vmTable ) );
}

/*
===============
VM_ValueToSymbol

Assumes a program counter value
===============
*/
const char *VM_ValueToSymbol( vm_t *vm, int value )
{
	vmSymbol_t  *sym;
	static char text[ MAX_TOKEN_CHARS ];

	sym = vm->symbols;

	if ( !sym )
	{
		return "NO SYMBOLS";
	}

	// find the symbol
	while ( sym->next && sym->next->symValue <= value )
	{
		sym = sym->next;
	}

	if ( value == sym->symValue )
	{
		return sym->symName;
	}

	Com_sprintf( text, sizeof( text ), "%s+%i", sym->symName, value - sym->symValue );

	return text;
}

/*
===============
VM_ValueToFunctionSymbol

For profiling, find the symbol behind this value
===============
*/
vmSymbol_t *VM_ValueToFunctionSymbol( vm_t *vm, int value )
{
	vmSymbol_t        *sym;
	static vmSymbol_t nullSym;

	sym = vm->symbols;

	if ( !sym )
	{
		return &nullSym;
	}

	while ( sym->next && sym->next->symValue <= value )
	{
		sym = sym->next;
	}

	return sym;
}

/*
===============
VM_SymbolToValue
===============
*/
int VM_SymbolToValue( vm_t *vm, const char *symbol )
{
	vmSymbol_t *sym;

	for ( sym = vm->symbols; sym; sym = sym->next )
	{
		if ( !strcmp( symbol, sym->symName ) )
		{
			return sym->symValue;
		}
	}

	return 0;
}

/*
=====================
VM_SymbolForCompiledPointer
=====================
*/
#if 0 // 64bit!
const char *VM_SymbolForCompiledPointer( vm_t *vm, void *code )
{
	int i;

	if ( code < ( void * ) vm->codeBase )
	{
		return "Before code block";
	}

	if ( code >= ( void * )( vm->codeBase + vm->codeLength ) )
	{
		return "After code block";
	}

	// find which original instruction it is after
	for ( i = 0; i < vm->codeLength; i++ )
	{
		if ( ( void * ) vm->instructionPointers[ i ] > code )
		{
			break;
		}
	}

	i--;

	// now look up the bytecode instruction pointer
	return VM_ValueToSymbol( vm, i );
}

#endif

/*
===============
ParseHex
===============
*/
int     ParseHex( const char *text )
{
	int value;
	int c;

	value = 0;

	while ( ( c = *text++ ) != 0 )
	{
		if ( c >= '0' && c <= '9' )
		{
			value = value * 16 + c - '0';
			continue;
		}

		if ( c >= 'a' && c <= 'f' )
		{
			value = value * 16 + 10 + c - 'a';
			continue;
		}

		if ( c >= 'A' && c <= 'F' )
		{
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}

/*
===============
VM_LoadSymbols
===============
*/
void VM_LoadSymbols( vm_t *vm )
{
	union
	{
		char *c;
		void *v;
	} mapfile;

	char       *text_p, *token;
	char       name[ MAX_QPATH ];
	char       symbols[ MAX_QPATH ];
	vmSymbol_t **prev, *sym;
	int        count;
	int        value;
	int        chars;
	int        segment;
	int        numInstructions;

	// don't load symbols if not developer
	if ( !com_developer->integer )
	{
		return;
	}

	COM_StripExtension3( vm->name, name, sizeof( name ) );
	Com_sprintf( symbols, sizeof( symbols ), "vm/%s.map", name );
	FS_ReadFile( symbols, &mapfile.v );

	if ( !mapfile.c )
	{
		Com_Printf( "Couldn't load symbol file: %s\n", symbols );
		return;
	}

	numInstructions = vm->instructionCount;

	// parse the symbols
	text_p = mapfile.c;
	prev = &vm->symbols;
	count = 0;

	while ( 1 )
	{
		token = COM_Parse( &text_p );

		if ( !token[ 0 ] )
		{
			break;
		}

		segment = ParseHex( token );

		if ( segment )
		{
			COM_Parse( &text_p );
			COM_Parse( &text_p );
			continue; // only load code segment values
		}

		token = COM_Parse( &text_p );

		if ( !token[ 0 ] )
		{
			Com_Printf( "WARNING: incomplete line at end of file\n" );
			break;
		}

		value = ParseHex( token );

		token = COM_Parse( &text_p );

		if ( !token[ 0 ] )
		{
			Com_Printf( "WARNING: incomplete line at end of file\n" );
			break;
		}

		chars = strlen( token );
		sym = Hunk_Alloc( sizeof( *sym ) + chars, h_high );
		*prev = sym;
		prev = &sym->next;
		sym->next = NULL;

		// convert value from an instruction number to a code offset
		if ( value >= 0 && value < numInstructions )
		{
			value = vm->instructionPointers[ value ];
		}

		sym->symValue = value;
		Q_strncpyz( sym->symName, token, chars + 1 );

		count++;
	}

	vm->numSymbols = count;
	Com_Printf( "%i symbols parsed from %s\n", count, symbols );
	FS_FreeFile( mapfile.v );
}

/*
============
VM_InitSanity
VM_SetSanity
VM_CheckSanity

Overflow? We should be safe where there are bounds checks, but not all are
checked. What isn't should be caught here (if there's a write).
============
*/
static void VM_InitSanity( vm_t *vm )
{
	if ( vm->dataMask )
	{
		int i;

		for ( i = 1; i < sizeof( vm->sanity ); ++i )
		{
			vm->dataBase[ vm->dataMask + 1 + i ] =
			  vm->sanity[ i ] = 1 + ( rand () % 255 );
		}
	}

	vm->versionChecked = qfalse;
}

#ifndef NDEBUG
#define VM_Insanity(c,n) Com_Error( ERR_DROP, "And it's a good night from vm. [%ld %s]", (long)(c), (n) );
#else
#define VM_Insanity(c,n) Com_Error( ERR_DROP, "And it's a good night from vm." );
#endif

void VM_SetSanity( vm_t* vm, intptr_t call )
{
	if ( !vm->versionChecked && call != TRAP_VERSION )
	{
		Com_Error( ERR_DROP, "VM code must invoke a version check" );
	}
	else if ( call == TRAP_VERSION )
	{
		vm->versionChecked = qtrue;
	}

	if ( vm->dataMask )
	{
		int i = rand();

		if ( i % sizeof( vm->sanity ) )
		{
			vm->dataBase[ vm->dataMask + 1 + ( i % sizeof( vm->sanity ) ) ] =
			  vm->sanity[ i % sizeof( vm->sanity ) ] ^=
			    (byte) ( i / sizeof( vm->sanity ) ) ^
			    (byte) ( i / sizeof( vm->sanity ) / 257 );

			if ( !vm->clean )
			{
				VM_Insanity( call, vm->name );
			}
		}
	}
}

void VM_CheckSanity( vm_t *vm, intptr_t call )
{
	if ( vm->dataMask && memcmp( vm->dataBase + vm->dataMask + 1, vm->sanity, sizeof( vm->sanity ) ) )
	{
		VM_Insanity( call, vm->name );
	}
}

/*
============
VM_DllSyscall

Dlls will call this directly

 rcg010206 The horror; the horror.

  The syscall mechanism relies on stack manipulation to get its args.
   This is likely due to C's inability to pass "..." parameters to
   a function in one clean chunk. On PowerPC Linux, these parameters
   are not necessarily passed on the stack, so while (&arg[0] == arg)
   is true, (&arg[1] == 2nd function parameter) is not necessarily
   accurate, as arg's value might have been stored to the stack or
   other piece of scratch memory to give it a valid address, but the
   next parameter might still be sitting in a register.

  Quake's syscall system also assumes that the stack grows downward,
   and that any needed types can be squeezed, safely, into a signed int.

  This hack below copies all needed values for an argument to a
   array in memory, so that Quake can get the correct values. This can
   also be used on systems where the stack grows upwards, as the
   presumably standard and safe stdargs.h macros are used.

  As for having enough space in a signed int for your datatypes, well,
   it might be better to wait for DOOM 3 before you start porting.  :)

  The original code, while probably still inherently dangerous, seems
   to work well enough for the platforms it already works on. Rather
   than add the performance hit for those platforms, the original code
   is still in use there.

  For speed, we just grab 15 arguments, and don't worry about exactly
   how many the syscall actually needs; the extra is thrown away.

============
*/
intptr_t QDECL VM_DllSyscall( intptr_t arg, ... )
{
	intptr_t ret;
#if !id386 || defined __clang__
	// rcg010206 - see commentary above
	intptr_t args[ 16 ];
	int      i;
	va_list  ap;

	args[ 0 ] = arg;

	va_start( ap, arg );

	for ( i = 1; i < ARRAY_LEN( args ); i++ )
	{
		args[ i ] = va_arg( ap, intptr_t );
	}

	va_end( ap );

	VM_SetSanity( currentVM, arg );

	if ( arg < FIRST_VM_SYSCALL )
	{
		ret = VM_SystemCall( args ); // all VMs
	}
	else
	{
		ret = currentVM->systemCall( args );
	}
#else // original id code (almost)
	VM_SetSanity( currentVM, arg );

	if ( arg < FIRST_VM_SYSCALL )
	{
		ret = VM_SystemCall( &arg ); // all VMs
	}
	else
	{
		ret = currentVM->systemCall( &arg );
	}
#endif
	VM_CheckSanity( currentVM, arg );
	return ret;
}

/*
=================
VM_LoadQVM

Load a .qvm file
=================
*/
vmHeader_t *VM_LoadQVM( vm_t *vm, qboolean alloc )
{
	int  dataLength;
	int  i;
	char filename[ MAX_QPATH ];
	union
	{
		vmHeader_t *h;
		void       *v;
	} header;

	// load the image
	Com_sprintf( filename, sizeof( filename ), "vm/%s.qvm", vm->name );
	Com_DPrintf( "Loading vm file %s…\n", filename );

	i = FS_ReadFileCheck( filename, &header.v );

	if ( !header.h )
	{
		Com_Printf( "Failed.\n" );
		VM_Free( vm );
		return NULL;
	}

	vm->clean = i >= 0;

	// show where the QVM was loaded from
	if ( com_developer->integer )
	{
		Cmd_ExecuteString( va( "which %s\n", filename ) );
	}

	if ( LittleLong( header.h->vmMagic ) == VM_MAGIC_VER2 )
	{
		Com_DPrintf( "…which has vmMagic VM_MAGIC_VER2\n" );

		// byte swap the header
		for ( i = 0; i < sizeof( vmHeader_t ) / 4; i++ )
		{
			( ( int * ) header.h ) [ i ] = LittleLong( ( ( int * ) header.h ) [ i ] );
		}

		// validate
		if ( header.h->jtrgLength < 0
		     || header.h->bssLength < 0
		     || header.h->dataLength < 0
		     || header.h->litLength < 0
		     || header.h->codeLength <= 0 )
		{
			VM_Free( vm );
			Com_Error( ERR_FATAL, "%s has bad header", filename );
		}
	}
	else if ( LittleLong( header.h->vmMagic ) == VM_MAGIC )
	{
		// byte swap the header
		// sizeof( vmHeader_t ) - sizeof( int ) is the 1.32b vm header size
		for ( i = 0; i < ( sizeof( vmHeader_t ) - sizeof( int ) ) / 4; i++ )
		{
			( ( int * ) header.h ) [ i ] = LittleLong( ( ( int * ) header.h ) [ i ] );
		}

		// validate
		if ( header.h->bssLength < 0
		     || header.h->dataLength < 0
		     || header.h->litLength < 0
		     || header.h->codeLength <= 0 )
		{
			VM_Free( vm );
			Com_Error( ERR_FATAL, "%s has bad header", filename );
		}
	}
	else
	{
		VM_Free( vm );
		Com_Error( ERR_FATAL, "%s does not have a recognisable "
		           "magic number in its header", filename );
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header.h->dataLength + header.h->litLength +
	             header.h->bssLength;

	for ( i = 0; dataLength > ( 1 << i ); i++ )
	{
	}

	dataLength = 1 << i;

	if ( alloc )
	{
		// allocate zero filled space for initialized and uninitialized data
		vm->dataBase = Hunk_Alloc( dataLength + VM_DATA_PADDING, h_high );
		vm->dataMask = dataLength - 1;
	}
	else
	{
		// clear the data, but make sure we're not clearing more than allocated
		if ( vm->dataMask + 1 != dataLength )
		{
			VM_Free(vm);
			FS_FreeFile(header.v);

			Com_DPrintf( S_COLOR_YELLOW "Warning: Data region size of %s not matching after"
						"VM_Restart()\n", filename );
			return NULL;
		}

		Com_Memset( vm->dataBase, 0, dataLength );
	}

	// copy the intialized data
	Com_Memcpy( vm->dataBase, ( byte * ) header.h + header.h->dataOffset,
	            header.h->dataLength + header.h->litLength );

	// byte swap the longs
	for ( i = 0; i < header.h->dataLength; i += 4 )
	{
		* ( int * )( vm->dataBase + i ) = LittleLong( * ( int * )( vm->dataBase + i ) );
	}

	if ( header.h->vmMagic == VM_MAGIC_VER2 )
	{
		int previousNumJumpTableTargets = vm->numJumpTableTargets;

		header.h->jtrgLength &= ~0x03;

		vm->numJumpTableTargets = header.h->jtrgLength >> 2;
		Com_DPrintf( "Loading %d jump table targets\n", vm->numJumpTableTargets );

		if ( alloc )
		{
			vm->jumpTableTargets = Hunk_Alloc( header.h->jtrgLength, h_high );
		}
		else
		{
			if( vm->numJumpTableTargets != previousNumJumpTableTargets )
			{
				VM_Free( vm );
				FS_FreeFile( header.v );

				Com_DPrintf( S_COLOR_YELLOW "Warning: Jump table size of %s not matching after"
				            "VM_Restart()\n", filename );
				return NULL;
			}

			Com_Memset( vm->jumpTableTargets, 0, header.h->jtrgLength );
		}

		Com_Memcpy( vm->jumpTableTargets, ( byte * ) header.h + header.h->dataOffset +
		            header.h->dataLength + header.h->litLength, header.h->jtrgLength );

		// byte swap the longs
		for ( i = 0; i < header.h->jtrgLength; i += 4 )
		{
			* ( int * )( vm->jumpTableTargets + i ) = LittleLong( * ( int * )( vm->jumpTableTargets + i ) );
		}
	}

	return header.h;
}

/*
=================
VM_Restart

Reload the data, but leave everything else in place
This allows a server to do a map_restart without changing memory allocation
=================
*/
vm_t *VM_Restart( vm_t *vm )
{
	vmHeader_t *header;

	// DLLs can't be restarted in place
	if ( vm->dllHandle
#if USE_LLVM
	     || vm->llvmModuleProvider
#endif
	   )
	{
		char     name[ MAX_QPATH ];
		intptr_t ( *systemCall )( intptr_t * parms );

		systemCall = vm->systemCall;
		Q_strncpyz( name, vm->name, sizeof( name ) );

		VM_Free( vm );

		vm = VM_Create( name, systemCall, VMI_NATIVE );
		return vm;
	}

	// load the image
	Com_Printf( "VM_Restart()\n" );

	if ( !( header = VM_LoadQVM( vm, qfalse ) ) )
	{
		Com_Error( ERR_DROP, "VM_Restart failed" );
		return NULL;
	}

	// free the original file
	FS_FreeFile( header );

	return vm;
}

/*
================
VM_Create

If image ends in .qvm it will be interpreted, otherwise
it will attempt to load as a system dll
================
*/
#define STACK_SIZE 0x20000

vm_t *VM_Create( const char *module, intptr_t ( *systemCalls )( intptr_t * ),
                 vmInterpret_t interpret )
{
	vm_t       *vm;
	vmHeader_t *header;
	int        i, remaining;
	qboolean   onlyQVM = !!Cvar_VariableValue( "sv_pure" );

#ifdef DEDICATED
	onlyQVM &= strcmp( module, "game" );
#endif

	if ( !module || !module[ 0 ] || !systemCalls )
	{
		Com_Error( ERR_FATAL, "VM_Create: bad parms" );
	}

	remaining = Hunk_MemoryRemaining();

	// see if we already have the VM
	for ( i = 0; i < MAX_VM; i++ )
	{
		if ( !Q_stricmp( vmTable[ i ].name, module ) )
		{
			vm = &vmTable[ i ];
			return vm;
		}
	}

	// find a free vm
	for ( i = 0; i < MAX_VM; i++ )
	{
		if ( !vmTable[ i ].name[ 0 ] )
		{
			break;
		}
	}

	if ( i == MAX_VM )
	{
		Com_Error( ERR_FATAL, "VM_Create: no free vm_t" );
	}

	vm = &vmTable[ i ];

	Q_strncpyz( vm->name, module, sizeof( vm->name ) );
	vm->systemCall = systemCalls;

	if ( interpret == VMI_NATIVE && !onlyQVM )
	{
		// try to load as a system dll
		vm->dllHandle = Sys_LoadDll( module, &vm->entryPoint, VM_DllSyscall );

		if ( vm->dllHandle )
		{
			vm->systemCall = systemCalls;
			return vm;
		}

		Com_DPrintf( "Failed loading dll, trying next\n" );
#if USE_LLVM
		interpret = VMI_BYTECODE;
#endif
	}

#if USE_LLVM
	if ( !onlyQVM )
	{
		// try to load the LLVM
		Com_DPrintf( "Loading llvm file %s.\n", vm->name );
		vm->llvmModuleProvider = VM_LoadLLVM( vm, VM_DllSyscall );

		if ( vm->llvmModuleProvider )
		{
			return vm;
		}

		Com_DPrintf( "Failed to load llvm, looking for qvm.\n" );
	}
#endif // USE_LLVM

	// load the image
	interpret = VMI_COMPILED;

	if ( !( header = VM_LoadQVM( vm, qtrue ) ) )
	{
		return NULL;
	}

	// allocate space for the jump targets, which will be filled in by the compile/prep functions
	vm->instructionCount = header->instructionCount;
	vm->instructionPointers = Hunk_Alloc( vm->instructionCount * sizeof( *vm->instructionPointers ), h_high );

	// copy or compile the instructions
	vm->codeLength = header->codeLength;

	vm->compiled = qfalse;

#ifdef NO_VM_COMPILED

	if ( interpret >= VMI_COMPILED )
	{
		Com_DPrintf( "Architecture doesn't have a bytecode compiler, using interpreter\n" );
		interpret = VMI_BYTECODE;
	}

#else

	if ( interpret >= VMI_COMPILED )
	{
		vm->compiled = qtrue;
		VM_Compile( vm, header );
	}

#endif

	// VM_Compile may have reset vm->compiled if compilation failed
	if ( !vm->compiled )
	{
		VM_PrepareInterpreter( vm, header );
	}

	// free the original file
	FS_FreeFile( header );

	// load the map file
	VM_LoadSymbols( vm );

	// the stack is implicitly at the end of the image
	vm->programStack = vm->dataMask + 1;
	vm->stackBottom = vm->programStack - PROGRAM_STACK_SIZE;

	Com_DPrintf( "%s loaded in %d bytes on the hunk\n", module, remaining - Hunk_MemoryRemaining() );

	VM_InitSanity( vm );

	return vm;
}

/*
==============
VM_Free
==============
*/
void VM_Free( vm_t *vm )
{
	if ( vm->dllHandle )
	{
		Sys_UnloadDll( vm->dllHandle );
		Com_Memset( vm, 0, sizeof( *vm ) );
	}

#if USE_LLVM

	if ( vm->llvmModuleProvider )
	{
		VM_UnloadLLVM( vm->llvmModuleProvider );
		Com_Memset( vm, 0, sizeof( *vm ) );
	}

#endif

#if 0 // now automatically freed by hunk

	if ( vm->codeBase )
	{
		Z_Free( vm->codeBase );
	}

	if ( vm->dataBase )
	{
		Z_Free( vm->dataBase );
	}

	if ( vm->instructionPointers )
	{
		Z_Free( vm->instructionPointers );
	}

#endif
	Com_Memset( vm, 0, sizeof( *vm ) );

	currentVM = NULL;
	lastVM = NULL;
}

void VM_Clear( void )
{
	int i;

	for ( i = 0; i < MAX_VM; i++ )
	{
		VM_Free( &vmTable[ i ] );
	}
}

void VM_Forced_Unload_Start( void )
{
	forced_unload = 1;
}

void VM_Forced_Unload_Done( void )
{
	forced_unload = 0;
}

void *VM_ArgPtr( intptr_t intValue )
{
	if ( !intValue )
	{
		return NULL;
	}

	// currentVM is missing on reconnect
	if ( currentVM == NULL )
	{
		return NULL;
	}

	if ( currentVM->entryPoint )
	{
		return ( void * )( currentVM->dataBase + intValue );
	}
	else
	{
		return ( void * )( currentVM->dataBase + ( intValue & currentVM->dataMask ) );
	}
}

void *VM_ExplicitArgPtr( vm_t *vm, intptr_t intValue )
{
	if ( !intValue )
	{
		return NULL;
	}

	// currentVM is missing on reconnect here as well?
	if ( currentVM == NULL )
	{
		return NULL;
	}

	//
	if ( vm->entryPoint )
	{
		return ( void * )( vm->dataBase + intValue );
	}
	else
	{
		return ( void * )( vm->dataBase + ( intValue & vm->dataMask ) );
	}
}

/*
==============
VM_Call


Upon a system call, the stack will look like:

sp+32 parm1
sp+28 parm0
sp+24 return value
sp+20 return address
sp+16 local1
sp+14 local0
sp+12 arg1
sp+8  arg0
sp+4  return stack
sp    return address

An interpreted function will immediately execute
an OP_ENTER instruction, which will subtract space for
locals from sp
==============
*/

intptr_t        QDECL VM_Call( vm_t *vm, int callnum, ... )
{
	vm_t     *oldVM;
	intptr_t r;
	int      i;

	if ( !vm || !vm->name[ 0 ] )
	{
		Com_Error( ERR_FATAL, "VM_Call with NULL vm" );
	}

	oldVM = currentVM;
	currentVM = vm;
	lastVM = vm;

	if ( vm_debugLevel )
	{
		Com_Printf( "VM_Call( %d )\n", callnum );
	}

	++vm->callLevel;

	// if we have a native library loaded, call it directly
	if ( vm->entryPoint )
	{
		//rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
		int     args[ 16 ];
		va_list ap;
		va_start( ap, callnum );

		for ( i = 0; i < ARRAY_LEN( args ); i++ )
		{
			args[ i ] = va_arg( ap, int );
		}

		va_end( ap );

		r = vm->entryPoint( callnum, args[ 0 ], args[ 1 ], args[ 2 ], args[ 3 ],
		                    args[ 4 ], args[ 5 ], args[ 6 ], args[ 7 ],
		                    args[ 8 ], args[ 9 ], args[ 10 ], args[ 11 ], args[ 12 ], args[ 13 ], args[ 14 ], args[ 15 ] );
	}
	else
	{
#if ( id386 || idsparc ) && !defined __clang__ // calling convention doesn't need conversion in some cases
#ifndef NO_VM_COMPILED

		if ( vm->compiled )
		{
			r = VM_CallCompiled( vm, ( int * ) &callnum );
		}
		else
#endif
			r = VM_CallInterpreted( vm, ( int * ) &callnum );

#else
		struct
		{
			int callnum;
			int args[ 10 ];
		} a;

		va_list ap;

		a.callnum = callnum;
		va_start( ap, callnum );

		for ( i = 0; i < ARRAY_LEN( a.args ); i++ )
		{
			a.args[ i ] = va_arg( ap, int );
		}

		va_end( ap );
#ifndef NO_VM_COMPILED

		if ( vm->compiled )
		{
			r = VM_CallCompiled( vm, &a.callnum );
		}
		else
#endif
			r = VM_CallInterpreted( vm, &a.callnum );

#endif
	}

	--vm->callLevel;

	if ( oldVM != NULL )
	{
		currentVM = oldVM;
	}

	return r;
}

//=================================================================

static int QDECL VM_ProfileSort( const void *a, const void *b )
{
	vmSymbol_t *sa, *sb;

	sa = * ( vmSymbol_t ** ) a;
	sb = * ( vmSymbol_t ** ) b;

	if ( sa->profileCount < sb->profileCount )
	{
		return -1;
	}

	if ( sa->profileCount > sb->profileCount )
	{
		return 1;
	}

	return 0;
}

/*
==============
VM_VmProfile_f

==============
*/
void VM_VmProfile_f( void )
{
	vm_t       *vm;
	vmSymbol_t **sorted, *sym;
	int        i;
	double     total;

	if ( !lastVM )
	{
		return;
	}

	vm = lastVM;

	if ( !vm->numSymbols )
	{
		return;
	}

	sorted = Z_Malloc( vm->numSymbols * sizeof( *sorted ) );
	sorted[ 0 ] = vm->symbols;
	total = sorted[ 0 ]->profileCount;

	for ( i = 1; i < vm->numSymbols; i++ )
	{
		sorted[ i ] = sorted[ i - 1 ]->next;
		total += sorted[ i ]->profileCount;
	}

	qsort( sorted, vm->numSymbols, sizeof( *sorted ), VM_ProfileSort );

	for ( i = 0; i < vm->numSymbols; i++ )
	{
		int perc;

		sym = sorted[ i ];

		perc = 100 * ( float ) sym->profileCount / total;
		Com_Printf( "%2i%% %9i %s\n", perc, sym->profileCount, sym->symName );
		sym->profileCount = 0;
	}

	Com_Printf( "    %9.0f total\n", total );

	Z_Free( sorted );
}

/*
==============
VM_VmInfo_f

==============
*/
void VM_VmInfo_f( void )
{
	vm_t *vm;
	int  i;

	Com_Printf(_( "Registered virtual machines:\n" ));

	for ( i = 0; i < MAX_VM; i++ )
	{
		vm = &vmTable[ i ];

		if ( !vm->name[ 0 ] )
		{
			break;
		}

		Com_Printf( "%s : ", vm->name );

		if ( vm->dllHandle )
		{
			Com_Printf( "native\n" );
			continue;
		}

#if USE_LLVM

		if ( vm->llvmModuleProvider )
		{
			Com_Printf( "llvm\n" );
			continue;
		}

#endif // USE_LLVM

		if ( vm->compiled )
		{
			Com_Printf( "compiled on load\n" );
		}
		else
		{
			Com_Printf( "interpreted\n" );
		}

		Com_Printf( "    code length : %7i\n", vm->codeLength );
		Com_Printf( "    table length: %7i\n", vm->instructionCount * 4 );
		Com_Printf( "    data length : %7i\n", vm->dataMask + 1 );
	}
}

/*
===============
VM_LogSyscalls

Insert calls to this while debugging the vm compiler
===============
*/
void VM_LogSyscalls( int *args )
{
	static  int  callnum;
	static  FILE *f;

	if ( !f )
	{
		f = fopen( "syscalls.log", "w" );
	}

	callnum++;
	fprintf( f, "%i: %p (%i) = %i %i %i %i\n", callnum, ( void * )( args - ( int * ) currentVM->dataBase ),
	         args[ 0 ], args[ 1 ], args[ 2 ], args[ 3 ], args[ 4 ] );
}

/*
=================
VM_CheckBlock
Address+offset validation
=================
*/
void VM_CheckBlock( intptr_t buf, size_t n, const char *fail )
{
	intptr_t dataMask = currentVM->dataMask;

	if ( dataMask &&
	     ( ( buf & dataMask ) != buf || ( ( buf + n ) & dataMask ) != buf + n ) )
	{
		Com_Error( ERR_DROP, "%s out of range! [%lx, %lx, %lx]", fail, (unsigned long)buf, (unsigned long)n, (unsigned long)dataMask );
	}
}

void VM_CheckBlockPair( intptr_t dest, intptr_t src, size_t dn, size_t sn, const char *fail )
{
	intptr_t dataMask = currentVM->dataMask;

	if ( dataMask &&
	     ( ( dest & dataMask ) != dest
	       || ( src & dataMask ) != src
	       || ( ( dest + dn ) & dataMask ) != dest + dn
	       || ( ( src + sn ) & dataMask ) != src + sn ) )
	{
		Com_Error( ERR_DROP, "%s out of range!", fail );
	}
}

/*
=================
VM_BlockCopy
Executes a block copy operation within currentVM data space
=================
*/
void VM_BlockCopy( unsigned int dest, unsigned int src, size_t n )
{
	VM_CheckBlockPair( dest, src, n, n, "OP_BLOCK_COPY" );
	Com_Memcpy( currentVM->dataBase + dest, currentVM->dataBase + src, n );
}

/*
=================
VM_SystemCall
System calls common to all VMs
=================
*/
static int FloatAsInt( float f )
{
	floatint_t fi;

	fi.f = f;
	return fi.i;
}

intptr_t VM_SystemCall( intptr_t *args )
{
	switch( args[ 0 ] )
	{
		case TRAP_MEMSET:
			VM_CheckBlock( args[ 1 ], args[ 3 ], "MEMSET" );
			memset( VMA( 1 ), args[ 2 ], args[ 3 ] );
			return 0;

		case TRAP_MEMCPY:
			VM_CheckBlockPair( args[ 1 ], args[ 2 ], args[ 3 ], args[ 3 ], "MEMCPY" );
			memcpy( VMA( 1 ), VMA( 2 ), args[ 3 ] );
			return 0;

		case TRAP_MEMCMP:
			VM_CheckBlockPair( args[ 1 ], args[ 2 ], args[ 3 ], args[ 3 ], "MEMCMP" );
			return memcmp( VMA( 1 ), VMA( 2 ), args[ 3 ] );

		case TRAP_STRNCPY:
			VM_CheckBlockPair( args[ 1 ], args[ 2 ], args[ 3 ], strlen( VMA( 2 ) ) + 1, "STRNCPY" );
			return ( intptr_t ) strncpy( VMA( 1 ), VMA( 2 ), args[ 3 ] );

		case TRAP_SIN:
			return FloatAsInt( sin( VMF( 1 ) ) );

		case TRAP_COS:
			return FloatAsInt( cos( VMF( 1 ) ) );

		case TRAP_TAN:
			return FloatAsInt( tan( VMF( 1 ) ) );

		case TRAP_ASIN:
			return FloatAsInt( asin( VMF( 1 ) ) );

//		case TRAP_ACOS:
//			return FloatAsInt( acos( VMF( 1 ) ) );

		case TRAP_ATAN:
			return FloatAsInt( atan( VMF( 1 ) ) );

		case TRAP_ATAN2:
			return FloatAsInt( atan2( VMF( 1 ), VMF( 2 ) ) );

		case TRAP_SQRT:
			return FloatAsInt( sqrt( VMF( 1 ) ) );

		case TRAP_FLOOR:
			return FloatAsInt( floor( VMF( 1 ) ) );

		case TRAP_CEIL:
			return FloatAsInt( ceil( VMF( 1 ) ) );

		case TRAP_MATRIXMULTIPLY:
			AxisMultiply( VMA( 1 ), VMA( 2 ), VMA( 3 ) );
			return 0;

		case TRAP_ANGLEVECTORS:
			AngleVectors( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ) );
			return 0;

		case TRAP_PERPENDICULARVECTOR:
			PerpendicularVector( VMA( 1 ), VMA( 2 ) );
			return 0;

		case TRAP_VERSION:
			if ( args[ 1 ] != SYSCALL_ABI_VERSION_MAJOR || args[ 2 ] > SYSCALL_ABI_VERSION_MINOR )
			{
				Com_Error( ERR_DROP, "Syscall ABI mismatch" );
			}
			return 0;

		default:
			Com_Error( ERR_DROP, "Bad game system trap: %ld", ( long int ) args[ 0 ] );
	}

	return -1;
}
