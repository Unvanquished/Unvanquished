/*
===========================================================================
Copyright (C) 2011-2012 Unvanquished Development

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

#ifdef __cplusplus
extern "C" {
#endif
#include "vm_local.h"
#ifdef __cplusplus
}
#endif

#ifdef USE_LLVM

#include "vm_llvm.h"
#include "../sys/sys_loadlib.h"
#include <stdint.h>
#include <ctype.h>
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/PassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>


using namespace llvm;

#ifdef __cplusplus
extern "C" {
#endif

static ExecutionEngine *engine = NULL;

static void *VM_LookupSym( const std::string& symbol )
{
#define EXPORTFUNC(fn) { #fn, (void *) fn }
	static const struct {
		const char *name;
		void       *func;
	} lookup[] = {
		EXPORTFUNC( Q_snprintf ),
		EXPORTFUNC( Q_strncpyz ),
		EXPORTFUNC( Q_vsnprintf ),
		EXPORTFUNC( atan2 ),
		EXPORTFUNC( ceil ),
		EXPORTFUNC( cos ),
		EXPORTFUNC( floor ),
		EXPORTFUNC( floorf ),
		EXPORTFUNC( memcpy ),
		EXPORTFUNC( memmove ),
		EXPORTFUNC( memset ),
		EXPORTFUNC( powf ),
		EXPORTFUNC( realloc ), // FIXME - don't really want this one exported
		EXPORTFUNC( sin ),
		EXPORTFUNC( strncpy ),
		{}
	};

	if ( com_developer->integer ) {
		Com_Printf( "LLVM: Look up symbol %s\n", symbol.c_str() );
	}

	// our symbols
	for (int i = 0; lookup[ i ].name; ++i) {
		if ( symbol == lookup[ i ].name ) {
			return lookup[ i ].func;
		}
	}

	// return Sys_LoadFunction( NULL, symbol.c_str() );

	// this is about to crash and burn, so report the symbol
	Com_Printf( "LLVM: ^3Unparsed symbol %s^7\n", symbol.c_str() );
	return NULL;
}

typedef void (*dllEntry_t)(intptr_t (*)(intptr_t, ...));

void *VM_LoadLLVM( vm_t *vm, intptr_t (*systemcalls)(intptr_t, ...) ) {
	char name[MAX_QPATH];
	char filename[MAX_QPATH];
	char *bytes;
	std::string error;

	COM_StripExtension( vm->name, name );

#if defined(__x86_64__) || defined( __WIN64__ )
	Com_sprintf( filename, sizeof(filename), "vm/%s_64.bc", name );
#elif defined(__i386__) || defined(_WIN32)
	Com_sprintf( filename, sizeof(filename), "vm/%s_32.bc", name );
#else
	Com_sprintf( filename, sizeof(filename), "vm/%s.bc", name );
#endif

	int len = FS_ReadFile( filename, (void **)&bytes );

	if ( !bytes ) {
		Com_Printf( "Couldn't load llvm file: %s\n", filename );
		return NULL;
	}

	MemoryBuffer *buffer = MemoryBuffer::getMemBuffer(StringRef(bytes, len));
	Module *module = ParseBitcodeFile( buffer, getGlobalContext(), &error );
	delete buffer;
	FS_FreeFile( bytes );

	if ( !module ) {
		Com_Printf( "Couldn't parse llvm file: %s: %s\n", filename, error.c_str() );
		return NULL;
	}

	PassManagerBuilder PMBuilder;
	PMBuilder.OptLevel = 3;
	PMBuilder.SizeLevel = false;
	PMBuilder.DisableSimplifyLibCalls = false;
	PMBuilder.DisableUnitAtATime = false;
	PMBuilder.DisableUnrollLoops = false;

	PMBuilder.Inliner = createFunctionInliningPass(275);

	PassManager * PerModulePasses = new PassManager();
	PerModulePasses->add(new TargetData(module));

	PassManager *MPM = PerModulePasses;
	PMBuilder.populateModulePassManager(*MPM);

	if ( !engine ) {
		InitializeNativeTarget();

		std::string str;
		/* LLVM has gone through some churn since the initial version of the patch was
		 * written in 2009. For some unknown reason, the last argument of create() has
		 * to be false, otherwise LLVM will seg fault when JIT compiling vmMain in the
		 * call to engine->getPointerToFunction below. This is OK though, since the
		 * parameter is only there for backwards compatibility and they plan to reverse
		 * its default to false at some point in the future.  */
		engine = ExecutionEngine::create( module, false, &str, CodeGenOpt::Default, false );
		if ( !engine ) {
			Com_Printf( "Couldn't create ExecutionEngine: %s\n", str.c_str());
			return NULL;
		}
		engine->DisableSymbolSearching();
		engine->InstallLazyFunctionCreator( VM_LookupSym );
	} else {
		engine->addModule( module );
	}

	Function *func = module->getFunction("dllEntry");
	dllEntry_t dllEntry = (dllEntry_t)engine->getPointerToFunction(func);


	dllEntry(systemcalls);

	func = module->getFunction(std::string("vmMain"));
	intptr_t(*fp) (int,...) = (intptr_t(*)(int,...))engine->getPointerToFunction(func);

	vm->entryPoint = fp;

	if ( com_developer->integer ) {
		Com_Printf( "Loaded LLVM %s with module==%p\n", name, module);
	}

	return module;
}

void VM_UnloadLLVM( void *llvmModule ) {
	if ( !llvmModule ) {
		Com_Printf( "VM_UnloadLLVM called with NULL pointer\n" );
		return;
	}

	if ( com_developer->integer ) {
		Com_Printf( "Unloading LLVM with module==%p\n", llvmModule );
	}

	if ( !engine->removeModule( (Module *)llvmModule ) ) {
		Com_Printf( "Couldn't remove llvm\n" );
		return;
	}
	delete (Module *)llvmModule;
}

#ifdef __cplusplus
}
#endif

#endif
