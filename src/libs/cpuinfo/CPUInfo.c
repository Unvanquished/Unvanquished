/*
	CPUInfo - obtains all sorts of information about your CPU.

	The idea is to try and obtain as much usable info as possible about the
	CPU, and presenting it in a non-vendor specific way, which is also easy to
	use in your programs to select the optimum code, and optimum compatibility.

	Some functions are just helper-functions.
	I'll briefly describe the interesting functions.

	GetFamily() will determine the family that the CPU belongs to:

	0: 8086
	2: 80286
	3: 80386
	4: 80486
	5: Pentium
	6: Pentium Pro/II/III/various Celerons
	15: Pentium 4

	(or whatever other families there may be).

	GetFPU() will determine whether or not there is an FPU available.
	If there is an FPU, in most cases it will be of the same family as the CPU.
	There is one known exception: 80386 can use either a 80287 or 80387 FPU.
	The routine will therefore do an extra test on a 80386, to see which FPU is
	installed.

	GetClockSpeed() is rather obvious I suppose.
	It does not work in all cases though. It will try to read the clockspeed
	from the Windows registry. If that fails, it will try to measure the time
	using the timestamp counter, if	one is present (this is set to take 1500
	ms, which gives accurate results).
	If there is no timestamp counter, then there is no good way to determine
	the clockspeed,	and a value of 0 is returned.

	HasCPUID() will test if the cpuid instruction is supported.
	Two versions of the code are included. One uses an exception handler, and
	tries to execute cpuid, in order to determine its presence.
	This is the default routine.
	The alternative routine tests a bit in the eflags register to see if cpuid
	is supported. This routine is not used, because according to some AMD
	manual, this does not work in all cases, and the first routine was
	recommended.

	GetCPUInfo() will try to fill the CPUINFO structure with as much info as it
	can get from your PC. In the worst case, this may only be the CPU family,
	presence of an FPU, and if present, its family (which will typically happen
	on 80486 or older CPUs). If the clockspeed happens to be set in the Windows
	registry, it will get that aswell.
	On CPUs with cpuid (some 80486, and Pentium and newer CPUs), it will at
	least get family, fpu, model, stepping, vendor ID, feature flags and
	clockspeed.
	In all cases, the feature flags for all found features are set (regardless
	of whether cpuid actually reports the feature flags or not). This means
	that the FPU feature flag will be set on a 386 with FPU, or a 80486dx for
	example. It also means that on CPUs with SSE, the extended AMD feature flag
	for MMX Extensions is set, because these are built into SSE aswell.
	Many bits in the extended AMD features field also mirror Intel features.
	On non-AMD CPUs, these features will also be set.

	All this should allow you to simply check a feature flag, regardless of
	brand, type or model of a CPU.

	The cache-information is also presented in a non-vendor specific format.
	AMD and Intel both have very different ways of reporting the cache
	configuration. I chose a format that is similar to what AMD reports, and
	I translate the Intel cache-descriptors to that format.
	Not all information may be reported on Intel CPUs, and some CPUs may report
	descriptors that are not known yet, so not all cache-information may always
	be present. If all fields for a certain cache are 0, this could either
	mean that there is no cache, or that the information was not reported in a
	known format.
	Similarly, it is not known yet how AMD CPUs would report L3-cache at this
	time.

	There are also a few simple Has<feature>() functions. They simply check the
	bit in the proper features field of the CPUINFO that is passed to them, and
	return true if the feature is available, or false if it is not.

	All this should allow you to write very simple feature-detection code.
	An example:

	CPUINFO cpuInfo;

	GetCPUInfo( &cpuInfo );

	if ( HasSSE2( &cpuInfo ) )
	{
		// run SSE2 code
	}
	else if ( HasFPU( &cpuInfo ) )
	{
		// run FPU code
	}
	else
	{
		// Inform the user to upgrade his Flintstone-box
	}

	Scali - 2003

	This code is released under the BSD license. See COPYRIGHT.txt for more information.
*/

#ifdef _WINDOWS
	#include <windows.h>
#else
	#include <unistd.h>
#endif
#ifdef _MSC_VER
	#include <intrin.h>
#endif
#include <memory.h>
#include "CPUInfo.h"

const char* typeStrings[] = { "Original OEM Processor",
						"Intel OverDrive(R) Processor",
						"Dual processor",
						"Intel reserved." };

const Translate cacheTranslations[] = 
{
	// TLBInfo:		{ entries, associativity }
	// CacheInfo:	{ lineSize, linesPerTag, associativity, size }
	{ 0x01, L1CODETLB, { 32, 4 }, { 0 } },
	{ 0x02, L1CODELARGETLB, { 2, 4 }, { 0 } },
	{ 0x03, L1DATATLB, { 64, 4 }, { 0 } },
	{ 0x04, L1DATALARGETLB, { 8, 4 }, { 0 } },
	{ 0x05, L1DATALARGETLB, { 32, 4 }, { 0 } },
	{ 0x06, L1CODE, { 0 }, { 32, 1, 4, 8 } },
	{ 0x08, L1CODE, { 0 }, { 32, 1, 4, 16 } },
	{ 0x09, L1CODE, { 0 }, { 64, 1, 4, 32 } },
	{ 0x0A, L1DATA, { 0 }, { 32, 1, 2, 8 } },
	{ 0x0C, L1DATA, { 0 }, { 32, 1, 4, 16 } },
	{ 0x0D, L1DATA, { 0 }, { 64, 1, 4, 16 } },
	{ 0x21, L2DATA, { 0 }, { 64, 1, 8, 256 } },
	{ 0x22, L3DATA, { 0 }, { 64, 1, 4, 512 } },
	{ 0x23, L3DATA, { 0 }, { 64, 1, 8, 1024 } },
	{ 0x25, L3DATA, { 0 }, { 64, 1, 8, 2048 } },
	{ 0x29, L3DATA, { 0 }, { 64, 1, 8, 4096 } },
	{ 0x2C, L1DATA, { 0 }, { 64, 1, 8, 32 } },
	{ 0x30, L1CODE, { 0 }, { 64, 1, 8, 32 } },
	{ 0x39, L2DATA, { 0 }, { 64, 1, 4, 128 } },
	{ 0x3A, L2DATA, { 0 }, { 64, 1, 6, 192 } },
	{ 0x3B, L2DATA, { 0 }, { 64, 1, 2, 128 } },
	{ 0x3C, L2DATA, { 0 }, { 64, 1, 4, 256 } },
	{ 0x3D, L2DATA, { 0 }, { 64, 1, 6, 384 } },
	{ 0x3E, L2DATA, { 0 }, { 64, 1, 4, 512 } },
	//{ 0x40, "No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache" },
	{ 0x41, L2DATA, { 0 }, { 32, 1, 4, 128 } },
	{ 0x42, L2DATA, { 0 }, { 32, 1, 4, 256 } },
	{ 0x43, L2DATA, { 0 }, { 32, 1, 4, 512 } },
	{ 0x44, L2DATA, { 0 }, { 32, 1, 4, 1024 } },
	{ 0x45, L2DATA, { 0 }, { 32, 1, 4, 2048 } },
	{ 0x49, L2DATA, { 0 }, { 61, 1, 4, 4096 } },
	{ 0x50, L1CODETLBANDLARGE, { 64, 255 }, { 0 } }, // ??
	{ 0x51, L1CODETLBANDLARGE, { 128, 255 }, { 0 } }, // ??
	{ 0x52, L1CODETLBANDLARGE, { 256, 255 }, { 0 } }, // ??
	{ 0x56, L1DATATLB, { 16, 4 }, { 0 } },
	{ 0x57, L1DATATLB, { 16, 4 }, { 0 } },
	{ 0x5B, L1DATATLBANDLARGE, { 64, 255 }, { 0 } }, // ??
	{ 0x5C, L1DATATLBANDLARGE, { 128, 255 }, { 0 } }, // ??
	{ 0x5D, L1DATATLBANDLARGE, { 256, 255 }, { 0 } }, // ??
	{ 0x60, L1DATA, { 0 }, { 64, 1, 8, 16 } },
	{ 0x66, L1DATA, { 0 }, { 64, 1, 4, 8 } },
	{ 0x67, L1DATA, { 0 }, { 64, 1, 4, 16 } },
	{ 0x68, L1DATA, { 0 }, { 64, 1, 4, 32 } },
	{ 0x70, L1CODETRACE, { 0 }, { 0, 1, 8, 12} },
	{ 0x71, L1CODETRACE, { 0 }, { 0, 1, 8, 16} },
	{ 0x72, L1CODETRACE, { 0 }, { 0, 1, 8, 32} },
	{ 0x78, L2DATA, { 0 }, { 64, 1, 8, 1024 } },
	{ 0x79, L2DATA, { 0 }, { 64, 1, 8, 128 } },
	{ 0x7A, L2DATA, { 0 }, { 64, 128/64, 8, 256 } }, // ??
	{ 0x7B, L2DATA, { 0 }, { 64, 128/64, 8, 512 } }, // ??
	{ 0x7C, L2DATA, { 0 }, { 64, 128/64, 8, 1024 } }, // ??
	{ 0x7D, L2DATA, { 0 }, { 64, 128/64, 8, 2048 } }, // ??
	{ 0x82, L2DATA, { 0 }, { 32, 1, 8, 256 } },
	{ 0x83, L2DATA, { 0 }, { 32, 1, 8, 512 } },
	{ 0x84, L2DATA, { 0 }, { 32, 1, 8, 1024 } },
	{ 0x85, L2DATA, { 0 }, { 32, 1, 8, 2048 } },
	{ 0x86, L2DATA, { 0 }, { 64, 1, 4, 512 } },
	{ 0x87, L2DATA, { 0 }, { 64, 1, 8, 1024 } },
	{ 0xB0, L1CODETLB, { 128, 4 }, { 0 } },
	{ 0xB1, L1CODELARGETLB, { 4, 4 }, { 0 } },
	{ 0xB3, L1DATALARGETLB, { 128, 4 }, { 0 } },
	{ 0xB4, L2DATATLB, { 256, 4 }, { 0 } }
};

/*
0h L2/L3 cache or TLB is disabled.
1h Direct mapped.
2h 2-way associative.
4h 4-way associative.
6h 8-way associative.
8h 16-way associative.
Ah 32-way associative.
Bh 48-way associative.
Ch 64-way associative.
Dh 96-way associative.
Eh 128-way associative.
Fh Fully associative.
*/

const unsigned char associativityTable[] = { 0, 1, 2, 0, 4, 0, 8, 0, 16, 0, 32, 48, 64, 96, 128, 255 };

const Descriptor cacheDescriptors[] = 
{
	{ 0x01, "Instruction TLB: 4K-Byte Pages, 4-way set associative, 32 entries" },
	{ 0x02, "Instruction TLB: 4M-Byte Pages, 4-way set associative, 2 entries" },
	{ 0x03, "Data TLB: 4K-Byte Pages, 4-way set associative, 64 entries" },
	{ 0x04, "Data TLB: 4M-Byte Pages, 4-way set associative, 8 entries" },
	{ 0x05, "Data TLB: 4-MB Pages, 4-way set associative, 32 entries" },
	{ 0x06, "1st-level instruction cache: 8K Bytes, 4-way set associative, 32 byte line size" },
	{ 0x08, "1st-level instruction cache: 16K Bytes, 4-way set associative, 32 byte line size" },
	{ 0x0A, "1st-level data cache: 8K Bytes, 2-way set associative, 32 byte line size" },
	{ 0x0C, "1st-level data cache: 16K Bytes, 4-way set associative, 32 byte line size" },
	{ 0x22, "3rd-level cache: 512K Bytes, 4-way set associative, 64 byte line size, 128 byte sector size" },
	{ 0x23, "3rd-level cache: 1M Bytes, 8-way set associative, 64 byte line size, 128 byte sector size" },
	{ 0x25, "3rd-level cache: 2M Bytes, 8-way set associative, 64 byte line size, 128 byte sector size" },
	{ 0x2C, "1st-level data cache: 32K Bytes, 8-way set associative, 64 byte line size" },
	{ 0x30, "1st-level instruction cache: 32K Bytes, 8-way set associative, 64 byte line size" },
	{ 0x3C, "2nd-level cache: 256K Bytes, 4-way set associative, 64 byte line size" },
	{ 0x40, "No 2nd-level cache or, if processor contains a valid 2nd-level cache, no 3rd-level cache" },
	{ 0x41, "2nd-level cache: 128K Bytes, 4-way set associative, 32 byte line size" },
	{ 0x42, "2nd-level cache: 256K Bytes, 4-way set associative, 32 byte line size" },
	{ 0x43, "2nd-level cache: 512K Bytes, 4-way set associative, 32 byte line size" },
	{ 0x44, "2nd-level cache: 1M Byte, 4-way set associative, 32 byte line size" },
	{ 0x45, "2nd-level cache: 2M Byte, 4-way set associative, 32 byte line size" },
	{ 0x49, "2nd-level cache: 4-MB, 16-way set associative, 64-byte line size" },
	{ 0x50, "Instruction TLB: 4-KByte and 2-MByte or 4-MByte pages, 64 entries" },
	{ 0x51, "Instruction TLB: 4-KByte and 2-MByte or 4-MByte pages, 128 entries" },
	{ 0x56, "L1 Data TLB: 4-MB pages, 4-way set associative, 16 entries" },
	{ 0x52, "Instruction TLB: 4-KByte and 2-MByte or 4-MByte pages, 256 entries" },
	{ 0x57, "L1 Data TLB: 4-KB pages, 4-way set associative, 16 entries" },
	{ 0x5B, "Data TLB: 4-KByte and 4-MByte pages, 64 entries" },
	{ 0x5C, "Data TLB: 4-KByte and 4-MByte pages, 128 entries" },
	{ 0x5D, "Data TLB: 4-KByte and 4-MByte pages, 256 entries" },
	{ 0x66, "1st-level data cache: 8KB, 4-way set associative, 64 byte line size" },
	{ 0x67, "1st-level data cache: 16KB, 4-way set associative, 64 byte line size" },
	{ 0x68, "1st-level data cache: 32KB, 4-way set associative, 64 byte line size" },
	{ 0x70, "Trace cache: 12K-micro-op, 8-way set associative" },
	{ 0x71, "Trace cache: 16K-micro-op, 8-way set associative" },
	{ 0x72, "Trace cache: 32K-micro-op, 8-way set associative" },
	{ 0x78, "2nd-level cache: 1M Byte, 8-way set associative, 64 byte line size" },
	{ 0x79, "2nd-level cache: 128KB, 8-way set associative, 64 byte line size, 128 byte sector size" },
	{ 0x7A, "2nd-level cache: 256KB, 8-way set associative, 64 byte line size, 128 byte sector size" },
	{ 0x7B, "2nd-level cache: 512KB, 8-way set associative, 64 byte line size, 128 byte sector size" },
	{ 0x7C, "2nd-level cache: 1MB, 8-way set associative, 64 byte line size, 128 byte sector size" },
	{ 0x7D, "2nd-level cache: 2MB, 8-way set associative, 64 byte line size, 128 byte sector size" },
	{ 0x82, "2nd-level cache: 256K Bytes, 8-way set associative, 32 byte line size" },
	{ 0x83, "2nd-level cache: 512K Bytes, 8-way set associative, 32 byte line size" },
	{ 0x84, "2nd-level cache: 1M Byte, 8-way set associative, 32 byte line size" },
	{ 0x85, "2nd-level cache: 2M Byte, 8-way set associative, 32 byte line size" },
	{ 0x86, "2nd-level cache: 512K Bytes, 4-way set associative, 64 byte line size" },
	{ 0x87, "2nd-level cache: 1M Bytes, 8-way set associative, 64 byte line size" },
	{ 0xB0, "Instruction TLB: 4M-Byte Pages, 4-way set associative, 128 entries" },
	{ 0xB3, "Data TLB: 4M-Byte Pages, 4-way set associative, 128 entries" },
	{ 0xB4, "Data TLB: 4-KB Pages, 4-way set associative, 256 entries" },
	{ 0xF0, "64-byte Prefetching" }
};

const Descriptor brandDescriptors[] = 
{
	{ 0x00, "This processor does not support the brand identification feature" },
	{ 0x01, "Intel Celeron processor" },
	{ 0x02, "Intel Pentium III processor" },
	{ 0x03, "Intel Pentium III Xeon processor or Celeron, if family is 6, model is 11, and stepping is 1" },
	{ 0x04, "Intel Pentium III processor" },
	{ 0x06, "Mobile Intel Pentium III processor-M" },
	{ 0x06, "Mobile Intel Celeron processor" },
	{ 0x08, "Intel Pentium 4 processor (Willamette)" },
	{ 0x09, "Intel Pentium 4 processor (Northwood)" },
	{ 0x0A, "Intel Celeron processor" },
	{ 0x0B, "Intel Xeon processor or Xeon processor MP if family is 15, model is 1 and stepping is 3" },
	{ 0x0C, "Intel Xeon processor MP" },
	{ 0x0E, "Mobile Intel Pentium 4 processor-M or Xeon processor if family is 15 15, model is 1 and stepping is 3" },
	{ 0x0F, "Mobile Intel Celeron processor" },
	{ 0x13, "Mobile Intel Celeron processor" },
	{ 0x16, "Intel Pentium M processor" }
};

const char* descriptorStrings[256];
const char* brandStrings[256];
const Translate* cacheTranslators[256];
static CI_BOOL tablesBuilt = 0;

//static Registers regs;

void BuildTables()
{
	unsigned int i;

	// Set all entries to "unknown" first
	for (i = 0; i < lengthof(descriptorStrings); i++)
		descriptorStrings[i] = "Unknown cache descriptor";

	for (i = 0; i < lengthof(cacheDescriptors); i++)
		descriptorStrings[cacheDescriptors[i].descriptor] = cacheDescriptors[i].string;

	// Set all entries to "unknown" first
	for (i = 0; i < lengthof(brandStrings); i++)
		brandStrings[i] = "Unknown brand index";

	for (i = 0; i < lengthof(brandDescriptors); i++)
		brandStrings[brandDescriptors[i].descriptor] = brandDescriptors[i].string;

	// Set all entries to "unknown" first
	for (i = 0; i < lengthof(cacheTranslators); i++)
		cacheTranslators[i] = 0;

	for (i = 0; i < lengthof(cacheTranslations); i++)
		cacheTranslators[cacheTranslations[i].descriptor] = &cacheTranslations[i];

	tablesBuilt = CI_TRUE;
}

CI_BOOL HasFPU( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->features & CPUID_STD_FPU) != 0);
}

CI_BOOL HasMMX( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->features & CPUID_STD_MMX) != 0);
}

CI_BOOL HasMMXExt( CPUINFO* pCPUInfo )
{
	// AMD reports Extended MMX in a bit (SSE also supports it, field patched in GetCPUInfo)
	return ((pCPUInfo->extAMDFeatures & CPUID_EXT_AMD_MMXEXT) != 0);
}

CI_BOOL Has3DNow( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->extAMDFeatures & CPUID_EXT_AMD_3DNOW) != 0);
}

CI_BOOL Has3DNowExt( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->extAMDFeatures & CPUID_EXT_AMD_3DNOWEXT) != 0);
}

CI_BOOL HasSSE( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->features & CPUID_STD_SSE) != 0);
}

CI_BOOL HasSSE2( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->features & CPUID_STD_SSE2) != 0);
}

CI_BOOL HasSSE3( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->extFeatures & CPUID_EXT_SSE3) != 0);
}

CI_BOOL HasSSSE3( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->extFeatures & CPUID_EXT_SSSE3) != 0);
}

CI_BOOL HasSSE4_1( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->extFeatures & CPUID_EXT_SSE4_1) != 0);
}

CI_BOOL HasSSE4_2( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->extFeatures & CPUID_EXT_SSE4_2) != 0);
}

CI_BOOL HasHTT( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->features & CPUID_STD_HTT) != 0);
}

CI_BOOL HasSerial( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->features & CPUID_STD_PSN) != 0);
}

CI_BOOL Is64Bit( CPUINFO* pCPUInfo )
{
	return ((pCPUInfo->features & CPUID_EXT_EM64T) != 0);
}

void CPUID( Registers* pRegs, unsigned int index )
{
#ifdef _MSC_VER
/*
	__asm
	{
		mov		esi, [pRegs]
		mov		eax, [index]
		cpuid
		mov		[esi], eax
		mov		[esi+4], ebx
		mov		[esi+8], ecx
		mov		[esi+12],edx
	}
*/
	__cpuid( (int*)pRegs, index );
#else
#if defined(CI_X64)
	__asm ("cpuid"
	: "=a" (pRegs->eax), "=b" (pRegs->ebx), "=c" (pRegs->ecx), "=d" (pRegs->edx)
	: "a" (index)
	);
#else
	__asm (	"pushl	%%ebx	\n\t"
			"cpuid	\n\t"
			"movl	%%ebx, %1	\n\t"
			"popl	%%ebx	\n\t"
	: "=a" (pRegs->eax), "=r" (pRegs->ebx), "=c" (pRegs->ecx), "=d" (pRegs->edx)
	: "a" (index)
	);
#endif
#endif
}

uint64 RDTSC()
{
#ifdef _MSC_VER
	return __rdtsc();
#else
	uint64 ret;
	unsigned int* pRet = (unsigned int*)&ret;

	__asm ("rdtsc"
	: "=a" (pRet[0]), "=d" (pRet[1])
	);

	return ret;
#endif
}

/*CI_BOOL HasCPUID()
{
#if defined(CI_X64)
	return CI_TRUE;
#elif defined _MSC_VER
	__try
	{
		__asm
		{
			xor		eax, eax
			cpuid
		}

		return CI_TRUE;
	}
	__except( (GetExceptionCode() == 0xC000001D) )
	{
		return CI_FALSE;
	}
#else
	return CI_TRUE;
#endif
}*/

CI_BOOL HasCPUID()
{
#if defined(CI_X64)
	return CI_TRUE;
#elif defined _MSC_VER
	CI_BOOL cpuid_support = CI_TRUE;

	__asm
	{
		pushfd                       // Get original EFLAGS
		pop     eax
		mov     ecx, eax
		xor     eax, 200000h         // Flip ID bit in EFLAGS
		push    eax                  // Save new EFLAGS value on stack
		popfd                        // Replace current EFLAGS value
		pushfd                       // Get new EFLAGS
		pop     eax                  // Store new EFLAGS in EAX
		xor     eax, ecx             // Can not toggle ID bit,
		jz      support              // Processor=80486
		mov     [cpuid_support], 1   // We have CPUID support
support:
	}

	return cpuid_support;
#elif defined(__GNUC__) && defined(__x86_64__)
	CI_BOOL cpuid_support = CI_TRUE;

	__asm__
	(	"pushfq                         \n\t"   // Get original EFLAGS
		"popl       %%rax               \n\t"
		"movl       %%rax, %%rcx        \n\t"
		"xorl       $0x200000, %%eax    \n\t"   // Flip ID bit in EFLAGS
		"pushl      %%rax               \n\t"   // Save new EFLAGS value on stack
		"popfl                          \n\t"   // Replace current EFLAGS value
		"pushfl                         \n\t"   // Get new EFLAGS
		"popl       %%rax               \n\t"   // Store new EFLAGS in EAX
		"xorl       %%ecx, %%eax        \n\t"   // Can not toggle ID bit,
		"jnz        1f                  \n\t"   // Processor=80486
		"movl       $1, %0              \n\t"   // We have CPUID support
"1:                                     \n\t"
	: "=m" (cpuid_support)
	:
	: "%rax", "%rcx"
	);

	return cpuid_support;
#elif defined(__GNUC__) && defined(i386)
	CI_BOOL cpuid_support = CI_TRUE;
	
	__asm__
	(	"pushfl                         \n\t"	// Get original EFLAGS
		"popl       %%eax               \n\t"
		"movl       %%eax, %%ecx        \n\t"
		"xorl       $0x200000, %%eax	\n\t"	// Flip ID bit in EFLAGS
		"pushl      %%eax               \n\t"	// Save new EFLAGS value on stack
		"popfl                          \n\t"	// Replace current EFLAGS value
		"pushfl                         \n\t"	// Get new EFLAGS
		"popl       %%eax               \n\t"	// Store new EFLAGS in EAX
		"xorl       %%ecx, %%eax        \n\t"	// Can not toggle ID bit,
		"jnz        1f                  \n\t"	// Processor=80486
		"movl       $1, %0              \n\t"	// We have CPUID support
"1:                                     \n\t"
	: "=m" (cpuid_support)
	: 
	: "%eax", "%ecx"
	);

	return cpuid_support;
#endif
}

CI_BOOL Is8086()
{
#if defined(CI_X64)
	return CI_FALSE;
#elif defined _MSC_VER
	CI_BOOL cpu_type;

	__asm
	{
		pushf					// Push original FLAGS
		pop		ax				// Get original FLAGS
		mov		cx, ax			// Save original FLAGS
		and		ax, 0FFFh		// Clear bits 12-15 in FLAGS
		push	ax				// Save new FLAGS value on stack
		popf					// Replace current FLAGS value
		pushf					// Get new FLAGS
		pop		ax				// Store new FLAGS in AX
		and		ax, 0F000h		// If bits 12-15 are set, then
		cmp		ax, 0F000h		//   processor is an 8086/8088
		mov		[cpu_type], 1	// Turn on 8086/8088 flag
		je		end_8086		// Jump if processor is 8086/
								//   8088
		mov		[cpu_type], 0
end_8086:
		push 	cx
		popf
	}
	
	return cpu_type;
#else
	return CI_FALSE;
#endif
}

CI_BOOL Is80286()
{
#if defined(CI_X64)
	return CI_FALSE;
#elif defined _MSC_VER
	CI_BOOL cpu_type;

	__asm
	{
		pushf
		pop		cx
		mov		bx, cx
		or		cx, 0F000h			// Try to set bits 12-15
		push		cx				// Save new FLAGS value on stack
		popf						// Replace current FLAGS value
		pushf						// Get new FLAGS
		pop		ax					// Store new FLAGS in AX
		and		ax, 0F000h			// If bits 12-15 are clear

		mov		[cpu_type], 1		// Processor=80286, turn on 
									//   80286 flag

		jz		end_80286			// If no bits set, processor is 
									//   80286

		mov		[cpu_type], 0
end_80286:
		push	bx
		popf
	}
	
	return cpu_type;
#else
	return CI_FALSE;
#endif
}

CI_BOOL Is80386()
{
#if defined(CI_X64)
	return CI_FALSE;
#elif defined _MSC_VER
	CI_BOOL cpu_type;

	__asm
	{
		mov 	bx, sp
		and		sp, NOT 3
		pushfd					// Push original EFLAGS 
		pop		eax				// Get original EFLAGS
		mov		ecx, eax		// Save original EFLAGS
		xor		eax, 40000h		// Flip AC bit in EFLAGS

		push	eax				// Save new EFLAGS value on
								//   stack

		popfd					// Replace current EFLAGS value
		pushfd					// Get new EFLAGS
		pop		eax				// Store new EFLAGS in EAX

		xor		eax, ecx		// Can't toggle AC bit, 
								//   processor=80386

		mov		[cpu_type], 1	// Turn on 80386 processor flag
		jz		end_80386		// Jump if 80386 processor
		mov		[cpu_type], 0
end_80386:
		push	ecx
		popfd
		mov		sp, bx
	}

	return cpu_type;
#else
	return CI_FALSE;
#endif
}

unsigned char GetFamily()
{
	// Can we get the family directly from the CPU?
	if ( HasCPUID() )
	{
		unsigned char family;
		Registers regs;

		CPUID( &regs, 1 );

		family = ((regs.eax >> 8) & 0xF);

		// Get extended family info
		if (family == 0xF)
			family += (regs.eax >> 20) & 0xFF;

		return family;
	}
	else	// No, must be a 486 or lower then...
	{
		if ( Is8086() )
			return 0;
		if ( Is80286() )
			return 2;
		if ( Is80386() )
			return 3;

		// Must be 486
		return 4;
	}
}

unsigned char GetFPU( unsigned char family )
{
#if defined(CI_X64)
	return family;
#elif defined _MSC_VER
	CI_BOOL hasFPU;
	unsigned short controlWord;

	// Check if controlword works, then FPU is present
	__asm
	{
		fninit
		mov		[controlWord], 5A5Ah
		fnstsw	[controlWord]
		mov		ax, [controlWord]
		cmp		al, 0
		mov		[hasFPU], 0
		jnz		endDetectFPU
		fnstcw	[controlWord]
		mov		ax, [controlWord]
		and		ax, 103Fh
		cmp		ax, 3Fh
		mov		[hasFPU], 0
		jnz		endDetectFPU
		mov		[hasFPU], 1
endDetectFPU:
	}

	if (!hasFPU)
		return NOFPU;

	if (family != 3)
		return family;

	// 386 could possibly have a 287 or 387... 287 cannot store sign of infinity
	__asm
	{
		wait
		fld1
		wait
		fldz
		wait
		fdivp	st(1), st
		wait
		fld	st
		wait
		fchs
		wait
		fcomp	//p	st(1), st(0)
		fstsw	[controlWord]
		mov		ax, [controlWord]
		mov		[family], 2
		sahf
		jz		endDetect387
		mov		[family], 3
endDetect387:
	}

	return family;
#else
	return family;
#endif
}

#ifndef _WINDOWS
void Sleep(unsigned int dwMilliseconds)
{
	usleep(dwMilliseconds * 1000L);
}
#endif

unsigned int GetClockSpeed()
{
#define interval 1500
	Registers regs;
	uint64 time;

#ifdef _WINDOWS
	HKEY hKey;

	// See if we can get it from the Windows registry first, fastest (and most reliable?) way
	if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey ) ) 
	{
		unsigned int ProcSpeed;
		unsigned int buflen;

		ProcSpeed = 0;
		buflen = sizeof( ProcSpeed );

		if ( RegQueryValueEx( hKey, "~MHz", NULL, NULL, (LPBYTE)&ProcSpeed, (LPDWORD)&buflen ) == ERROR_SUCCESS )
		{
			RegCloseKey( hKey );
			return ProcSpeed;
		}

		RegCloseKey( hKey );
	}
#endif

	// Check for rdtsc support
	if (!HasCPUID())
		return 0;

	CPUID( &regs, 1 );

	// No timestamp counter? Then we can't measure the clockspeed
	if (!(regs.edx & CPUID_STD_TSC) )
		return 0;

	// Measure clockspeed
	Sleep( 0 );
	
	/*__asm
	{
		rdtsc
		mov dword ptr [time], eax
		mov dword ptr [time+4], edx
	}*/
	time = RDTSC();

	Sleep ( interval );

	/*__asm
	{
		rdtsc
		sub eax, dword ptr [time]
		sbb edx, dword ptr [time+4]
		mov dword ptr [time], eax
		mov dword ptr [time+4], edx
	}*/
	time = RDTSC() - time;

	return (unsigned int)(time/(interval*1000L));
}

void TranslateDescriptor( CPUINFO* pCPUInfo, unsigned char a )
{
	const Translate* pTrans = cacheTranslators[a];

	// Unknown descriptor?
	if (pTrans == NULL)
		return;

	switch (pTrans->type)
	{
	case L1CODELARGETLB:
		pCPUInfo->L1CodeLargeTLB = pTrans->tlbInfo;
		break;
	case L1DATALARGETLB:
		pCPUInfo->L1DataLargeTLB = pTrans->tlbInfo;
		break;
	case L1CODETLBANDLARGE:
		pCPUInfo->L1CodeLargeTLB = pTrans->tlbInfo;
		// Fall-through
	case L1CODETLB:
		pCPUInfo->L1CodeTLB = pTrans->tlbInfo;
		break;
	case L1DATATLBANDLARGE:
		pCPUInfo->L1DataLargeTLB = pTrans->tlbInfo;
		// Fall-through
	case L1DATATLB:
		pCPUInfo->L1DataTLB = pTrans->tlbInfo;
		break;
	case L2CODELARGETLB:
		pCPUInfo->L2CodeLargeTLB = pTrans->tlbInfo;
		break;
	case L2DATALARGETLB:
		pCPUInfo->L2DataLargeTLB = pTrans->tlbInfo;
		break;
	case L2CODETLB:
		pCPUInfo->L2CodeTLB = pTrans->tlbInfo;
		break;
	case L2DATATLB:
		pCPUInfo->L2DataTLB = pTrans->tlbInfo;
		break;
	case L1CODETRACE:
		pCPUInfo->traceCache = 1;
		// Fall-through
	case L1CODE:
		pCPUInfo->L1Code = pTrans->cacheInfo;
		break;
	case L1DATA:
		pCPUInfo->L1Data = pTrans->cacheInfo;
		break;
	case L2DATA:
		pCPUInfo->L2Data = pTrans->cacheInfo;
		break;
	case L3DATA:
		pCPUInfo->L3Data = pTrans->cacheInfo;
		break;
	}
}

void StoreDescriptors( CPUINFO* pCPUInfo, unsigned int d, unsigned char **desc )
{
	unsigned int i;
	unsigned char* pdesc = *desc;

	// High bit clear indicates valid descriptors
	if (d & 0x80000000)
		return;

	for (i = 0; i < 4; i++)
	{
		unsigned char a = d;
		if (a != 0)
		{
			// store in table
			*pdesc++ = a;

			// Translate to regular values
			TranslateDescriptor( pCPUInfo, a );
		}

		d >>= 8;
	}

	*desc = pdesc;
}

void GetCPUInfo( CPUINFO* pCPUInfo, CI_BOOL getSpeed )
{
	Registers regs;
	unsigned int* ID;
	unsigned int* serial;
	unsigned int i;
	unsigned char* desc;
	unsigned int* brand;

	if (!tablesBuilt)
		BuildTables();

	memset( pCPUInfo, 0, sizeof(CPUINFO) );

	// If there is no CPUID, we can only figure out family and FPU, and possibly clockspeed
	if (!HasCPUID())
	{
		unsigned char fpu;

		pCPUInfo->family = GetFamily();

		fpu = GetFPU( pCPUInfo->family );
		if (fpu != NOFPU)
		{
			pCPUInfo->fpu = fpu;
			pCPUInfo->features |= CPUID_STD_FPU;

			// Also set the bit in extended AMD features field
			pCPUInfo->extAMDFeatures |= CPUID_STD_FPU;
		}

		if (getSpeed)
			pCPUInfo->clockspeed = GetClockSpeed();

		// There is no HTT available, set logical CPUs to 1, not 0
		pCPUInfo->logicalCPUs = 1;

		return;
	}

	// Get vendor ID
	CPUID( &regs, 0 );

	ID = (unsigned int*)pCPUInfo->vendorID;
	ID[0] = regs.ebx;
	ID[1] = regs.edx;
	ID[2] = regs.ecx;

	pCPUInfo->maxInput = regs.eax;

	// Process all info from the highest index down
	switch (regs.eax)
	{
	// Anything higher should run everything
	default:
	case 4:
		{
			// Deterministic cache info
			CPUID( &regs, 4 );

		}
	case 3:
		{
			// Get Serial
			CPUID( &regs, 3 );

			serial = (unsigned int*)&pCPUInfo->serial;
			serial[0] = regs.ecx;
			serial[1] = regs.edx;
		}
	case 2:
		{
			// Get cache/tlb info
			CPUID( &regs, 2 );

			i = regs.eax & 0xFF;

			// Remove counter from register
			regs.eax &= ~0xFF;

			desc = pCPUInfo->cacheDescriptors;

			// Get all cache-info
			while (i--)
			{
				StoreDescriptors( pCPUInfo, regs.eax, &desc );
				StoreDescriptors( pCPUInfo, regs.ebx, &desc );
				StoreDescriptors( pCPUInfo, regs.ecx, &desc );
				StoreDescriptors( pCPUInfo, regs.edx, &desc );

				CPUID( &regs, 2 );
			}

		}
	case 1:
		// Get processor type and feature info
		CPUID( &regs, 1 );

		// Store brand index and related info to the 4 bytes
		*((unsigned int*)&pCPUInfo->brandIndex) = regs.ebx;

		pCPUInfo->features = regs.edx;
		pCPUInfo->extFeatures = regs.ecx;

		pCPUInfo->stepping = regs.eax & 0xF;
		regs.eax >>= 4;

		pCPUInfo->model = regs.eax & 0xF;
		regs.eax >>= 4;

		pCPUInfo->family = regs.eax & 0xF;
		regs.eax >>= 4;

		pCPUInfo->type = regs.eax & 0x3;

		// Get extended model info
		if (pCPUInfo->family == 0xF || pCPUInfo->family == 0x6)
			pCPUInfo->model += (regs.eax & 0xF0);
		regs.eax >>= 8;

		// Get extended family info
		if (pCPUInfo->family == 0xF)
			pCPUInfo->family += regs.eax & 0xFF;

		// CPU and FPU are the same on any CPU with cpuid
		if ( HasFPU( pCPUInfo ) )
			pCPUInfo->fpu = pCPUInfo->family;
	case 0:;
	}

	// Get extended info
	CPUID( &regs, 0x80000000 );

	brand = (unsigned int*)pCPUInfo->brandString;

	// Do nothing if the CPU doesn't support extended info
	if ((regs.eax & 0x80000000) == 0)
		regs.eax = 0x80000000;

	pCPUInfo->maxExtInput = regs.eax;

	switch (regs.eax)
	{
	// Anything higher should run everything
	default:
	// 0x80000008-0x80000005 are for AMD cache info
	case 0x80000006:
		CPUID( &regs,  0x80000006 );

		// AMD uses this field for cache info
		if ( memcmp( pCPUInfo->vendorID, "AuthenticAMD", 12 ) == 0 )
		{
			pCPUInfo->L2DataLargeTLB.associativity = associativityTable[regs.eax >> 28];
			pCPUInfo->L2DataLargeTLB.entries = (regs.eax >> 16) & 0x3FF;

			pCPUInfo->L2CodeLargeTLB.associativity = associativityTable[(regs.eax >> 12) & 0xF];
			pCPUInfo->L2CodeLargeTLB.entries = regs.eax & 0x3FF;

			pCPUInfo->L2DataTLB.associativity = associativityTable[regs.ebx >> 28];
			pCPUInfo->L2DataTLB.entries = (regs.ebx >> 16) & 0x3FF;

			pCPUInfo->L2CodeTLB.associativity = associativityTable[(regs.ebx >> 12) & 0xF];
			pCPUInfo->L2CodeTLB.entries = regs.ebx & 0x3FF;

			pCPUInfo->L2Data.size = regs.ecx >> 16;
			pCPUInfo->L2Data.associativity = associativityTable[(regs.ecx >> 12) & 0xF];
			pCPUInfo->L2Data.linesPerTag = (regs.ecx >> 8) & 0xF;
			pCPUInfo->L2Data.lineSize = regs.ecx & 0xFF;
		}
		else if ( memcmp( pCPUInfo->vendorID, "GenuineIntel", 12 ) == 0 )
		{
			pCPUInfo->L2Data.size = regs.ecx >> 16;
			pCPUInfo->L2Data.associativity = associativityTable[(regs.ecx >> 12) & 0xF];
			pCPUInfo->L2Data.linesPerTag = 1;//(regs.ecx >> 8) & 0xF;
			pCPUInfo->L2Data.lineSize = regs.ecx & 0xFF;

		}
	case 0x80000005:
		CPUID( &regs, 0x80000005 );

		// AMD uses this field for cache info
		if ( memcmp( pCPUInfo->vendorID, "AuthenticAMD", 12 ) == 0 )
		{
			pCPUInfo->L1DataLargeTLB.associativity = regs.eax >> 24;
			pCPUInfo->L1DataLargeTLB.entries = (regs.eax >> 16) & 0xFF;

			pCPUInfo->L1CodeLargeTLB.associativity = (regs.eax >> 8) & 0xFF;
			pCPUInfo->L1CodeLargeTLB.entries = regs.eax & 0xFF;

			pCPUInfo->L1DataTLB.associativity = regs.ebx >> 24;
			pCPUInfo->L1DataTLB.entries = (regs.ebx >> 16) & 0xFF;

			pCPUInfo->L1CodeTLB.associativity = (regs.ebx >> 8) & 0xFF;
			pCPUInfo->L1CodeTLB.entries = regs.ebx & 0xFF;
			
			pCPUInfo->L1Data.size = regs.ecx >> 24;
			pCPUInfo->L1Data.associativity = (regs.ecx >> 16) & 0xFF;
			pCPUInfo->L1Data.linesPerTag = (regs.ecx >> 8) & 0xFF;
			pCPUInfo->L1Data.lineSize = regs.ecx & 0xFF;

			pCPUInfo->L1Code.size = regs.edx >> 24;
			pCPUInfo->L1Code.associativity = (regs.edx >> 16) & 0xFF;
			pCPUInfo->L1Code.linesPerTag = (regs.edx >> 8) & 0xFF;
			pCPUInfo->L1Code.lineSize = regs.edx & 0xFF;
		}
		
		// Get brand string
	case 0x80000004:
		CPUID( &regs, 0x80000004 );
		brand[8]  = regs.eax;
		brand[9]  = regs.ebx;
		brand[10] = regs.ecx;
		brand[11] = regs.edx;
	case 0x80000003:
		CPUID( &regs, 0x80000003 );
		brand[4]  = regs.eax;
		brand[5]  = regs.ebx;
		brand[6]  = regs.ecx;
		brand[7]  = regs.edx;
	case 0x80000002:
		CPUID( &regs, 0x80000002 );
		brand[0]  = regs.eax;
		brand[1]  = regs.ebx;
		brand[2]  = regs.ecx;
		brand[3]  = regs.edx;
	case 0x80000001:
		CPUID( &regs, 0x80000001 );

		// AMD uses this field for their own family/model/stepping info, which does NOT match the regular info
		if ( memcmp( pCPUInfo->vendorID, "AuthenticAMD", 12 ) == 0 )
		{
			pCPUInfo->amdStepping = regs.eax & 0xF;
			regs.eax >>= 4;

			pCPUInfo->amdModel = regs.eax & 0xF;
			regs.eax >>= 4;

			pCPUInfo->amdFamily = regs.eax & 0xF;
			regs.eax >>= 4;

			// Get extended model info
			if (pCPUInfo->amdModel == 0xF)
				pCPUInfo->amdModel += (regs.eax & 0xF0);

			// Get extended family info
			if (pCPUInfo->amdFamily == 0xF)
				pCPUInfo->amdFamily += (regs.eax >> 4) & 0xFF;

			pCPUInfo->extAMDFeatures = regs.edx;
		}
		else
			pCPUInfo->extIntelFeatures = regs.edx;
	case 0x80000000:;
	}

	// If SSE is available, then so are the AMD Extended MMX functions
	if (pCPUInfo->features & CPUID_STD_SSE)
		pCPUInfo->extAMDFeatures |= CPUID_EXT_AMD_MMXEXT;

	// If there is no HTT available, set logical CPUs to 1, not 0
	if (!(pCPUInfo->features & CPUID_STD_HTT))
		pCPUInfo->logicalCPUs = 1;

	// Duplicate shared feature bits of AMD and Intel in AMD features (only bits 11, 19, 20, 22, 25, 26, 29, 30, 31 differ)
	pCPUInfo->extAMDFeatures |= (pCPUInfo->features & 0x19A7F7FF);

	if (getSpeed)
		pCPUInfo->clockspeed = GetClockSpeed();
}
