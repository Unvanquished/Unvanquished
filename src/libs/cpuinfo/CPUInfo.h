/*
	Datastructures used with CPUInfo to store CPU features and characteristics
	in a (mostly) vendor-independent way.

	Scali - 2003

	This code is released under the BSD license. See COPYRIGHT.txt for more information.
*/

#ifndef CPUINFO_H
#define CPUINFO_H

#include "CPUFlags.h"

#define lengthof(x) (sizeof(x)/sizeof(x[0]))

#define NOFPU 0xFF

// Handle some compiler/architecture-specific issues
#ifdef _MSC_VER
	typedef unsigned __int64 uint64;
	typedef signed __int64 int64;
#else
	typedef unsigned long long uint64;
	typedef signed long long int64;
#endif

#if defined(_M_X64) || defined(__x86_64__)
	#define CI_X64
#endif

typedef unsigned char CI_BOOL;
#define CI_FALSE (CI_BOOL)0
#define CI_TRUE !CI_FALSE

typedef struct
{
	unsigned int lineSize;			// in bytes
	unsigned int linesPerTag;
	unsigned int associativity;
	unsigned int size;				// in KB
} CacheInfo;

typedef struct
{
	unsigned int entries;
	unsigned int associativity;
} TLBInfo;

typedef struct
{
	unsigned char family;
	unsigned char fpu;
	unsigned char model;
	unsigned char amdFamily;
	unsigned char amdModel;
	unsigned char amdStepping;
	unsigned char stepping;
	unsigned char type;
	unsigned char brandIndex;	// The order of these
	unsigned char CLFLUSH;		// 4 members of the struct
	unsigned char logicalCPUs;	// are important, since they
	unsigned char APICID;		// map to a single register!
	unsigned int features;
	unsigned int extFeatures;
	unsigned int extIntelFeatures;
	unsigned int extAMDFeatures;
	unsigned int maxInput;
	unsigned int maxExtInput;
	unsigned int clockspeed;
	uint64 serial;
	char vendorID[13];
	char brandString[49];
	unsigned char cacheDescriptors[64];
	CI_BOOL traceCache;	// Boolean flag
	TLBInfo L1CodeLargeTLB;
	TLBInfo L1DataLargeTLB;
	TLBInfo L1CodeTLB;
	TLBInfo L1DataTLB;
	TLBInfo L2CodeLargeTLB;
	TLBInfo L2DataLargeTLB;
	TLBInfo L2CodeTLB;
	TLBInfo L2DataTLB;
	CacheInfo L1Code;
	CacheInfo L1Data;
	CacheInfo L2Data;
	CacheInfo L3Data;
} CPUINFO;

typedef struct
{
	unsigned char descriptor;
	const char* string;
} Descriptor;

enum
{
	L1CODELARGETLB,
	L1DATALARGETLB,
	L1CODETLB,
	L1CODETLBANDLARGE,
	L1DATATLB,
	L1DATATLBANDLARGE,
	L2CODELARGETLB,
	L2DATALARGETLB,
	L2CODETLB,
	L2DATATLB,
	L1CODE,
	L1CODETRACE,
	L1DATA,
	L2DATA,
	L3DATA
};

typedef struct
{
	unsigned char descriptor;
	unsigned char type;
	TLBInfo tlbInfo;
	CacheInfo cacheInfo;
} Translate;

typedef struct
{
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;
} Registers;

#ifdef __cplusplus
extern "C"
{
#endif
void BuildTables();
CI_BOOL HasFPU( CPUINFO* pCPUInfo );
CI_BOOL HasMMX( CPUINFO* pCPUInfo );
CI_BOOL HasMMXExt( CPUINFO* pCPUInfo );
CI_BOOL Has3DNow( CPUINFO* pCPUInfo );
CI_BOOL Has3DNowExt( CPUINFO* pCPUInfo );
CI_BOOL HasSSE( CPUINFO* pCPUInfo );
CI_BOOL HasSSE2( CPUINFO* pCPUInfo );
CI_BOOL HasSSE3( CPUINFO* pCPUInfo );
CI_BOOL HasSSSE3( CPUINFO* pCPUInfo );
CI_BOOL HasSSE4_1( CPUINFO* pCPUInfo );
CI_BOOL HasSSE4_2( CPUINFO* pCPUInfo );
CI_BOOL HasHTT( CPUINFO* pCPUInfo );
CI_BOOL HasSerial( CPUINFO* pCPUInfo );
CI_BOOL Is64Bit( CPUINFO* pCPUInfo );
void CPUID( Registers* pRegs, unsigned int index );
CI_BOOL HasCPUID();
uint64 RDTSC();
CI_BOOL Is8086();
CI_BOOL Is80286();
CI_BOOL Is80386();
unsigned char GetFamily();
unsigned char GetFPU( unsigned char family );
unsigned int GetClockSpeed();
void TranslateDescriptor( CPUINFO* pCPUInfo, unsigned char a );
void StoreDescriptors( CPUINFO* pCPUInfo, unsigned int d, unsigned char **desc );
void GetCPUInfo( CPUINFO* pCPUInfo, CI_BOOL getSpeed );

extern const char* brandStrings[];
extern const char* descriptorStrings[];
extern const char* typeStrings[];
#ifdef __cplusplus
}
#endif

#endif /* CPUINFO_H */
