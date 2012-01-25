/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

OpenWolf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

OpenWolf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

#define INT64_C(C)  ((int64_t) C ## LL) 
#define UINT64_C(C) ((uint64_t) C ## ULL) 

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/PassManager.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Support/StandardPasses.h>

using namespace llvm;

#ifdef __cplusplus
extern "C" {
#endif

static ExecutionEngine *engine = NULL;

void *VM_LoadLLVM( vm_t *vm, intptr_t (*systemcalls)(intptr_t, ...) ) {
	char name[MAX_QPATH];
	char filename[MAX_QPATH];
	char *bytes;
	std::string error;

	COM_StripExtension3( vm->name, name, sizeof(name) );
	Com_sprintf( filename, sizeof(filename), "%sllvm.bc", name );
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

	PassManager p;
	llvm::Pass *InliningPass = createFunctionInliningPass(275);
	p.add( new TargetData( module ) );
	createStandardModulePasses(&p, 3, false, true, true, true, false, InliningPass);
	p.run( *module );

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
			Com_Printf("Couldn't create ExecutionEngine: %s\n", str.c_str());
			return NULL;
		}		
	} else {
		engine->addModule( module );
	}

	Function *dllEntry_ptr = module->getFunction( std::string("dllEntry") );
	if ( !dllEntry_ptr ) {
		Com_Printf("Couldn't get function address of dllEntry\n");
		return NULL;
	}
	void  (*dllEntry)( intptr_t (*syscallptr)(intptr_t, ...) ) = (void (*)(intptr_t (*syscallptr)(intptr_t, ...)))engine->getPointerToFunction( dllEntry_ptr );
	dllEntry( systemcalls );

	Function *vmMain_ptr = module->getFunction( std::string("vmMain") );
	if ( !vmMain_ptr ) {
		Com_Printf("Couldn't get function address of vmMain\n");
		return NULL;
	}
	intptr_t(*fp)(int, ...) = (intptr_t(*)(int, ...))engine->getPointerToFunction( vmMain_ptr );

	vm->entryPoint = fp;

	if ( com_developer->integer ) {
		Com_Printf("Loaded LLVM %s with module==%p\n", name, module);
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