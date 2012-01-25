
option expr32
option casemap:none

; 
; 	Datastructures used with CPUInfo to store CPU features and characteristics
; 	in a (mostly) vendor-independent way.
; 
; 	Scali - 2003
; 

include CPUFlags.inc

NOFPU		EQU		0000000ffh

CacheInfo		STRUCT 4t
	lineSize		WORD		?	;  in bytes
	linesPerTag		WORD		?
	associativity	WORD		?
	cacheSize		WORD		?	;  in KB
CacheInfo		ENDS

TLBInfo		STRUCT 4t
	entries			WORD		?
	associativity	WORD		?
TLBInfo		ENDS

CPUINFO		STRUCT 4t
	family				BYTE		?
	fpu					BYTE		?
	model				BYTE		?
	amdFamily			BYTE		?
	amdModel			BYTE		?
	amdStepping			BYTE		?
	stepping			BYTE		?
	cputype				BYTE		?
	brandIndex			BYTE		?	;  The order of these
	mCLFLUSH			BYTE		?	;  4 members of the struct
	logicalCPUs			BYTE		?	;  are important, since they
	APICID				BYTE		?	;  map to a single register!
	features			WORD		?
	extFeatures			WORD		?
	extIntelFeatures	WORD		?
	extAMDFeatures		WORD		?
	maxInput			WORD		?
	maxExtInput			WORD		?
	clockspeed			WORD		?
	serial				QWORD		?
	vendorID			BYTE		13t DUP (?)
	brandString			BYTE		49t DUP (?)
	cacheDescriptors	BYTE		64t DUP (?)
	traceCache			BYTE		?	;  Boolean flag
	L1CodeLargeTLB		TLBInfo		<>
	L1DataLargeTLB		TLBInfo		<>
	L1CodeTLB			TLBInfo		<>
	L1DataTLB			TLBInfo		<>
	L2CodeLargeTLB		TLBInfo		<>
	L2DataLargeTLB		TLBInfo		<>
	L2CodeTLB			TLBInfo		<>
	L2DataTLB			TLBInfo		<>
	L1Code				CacheInfo	<>
	L1Data				CacheInfo	<>
	L2Data				CacheInfo	<>
	L3Data				CacheInfo	<>
CPUINFO		ENDS

Descriptor		STRUCT 4t
	descriptor	BYTE		?
	string		BYTE PTR	?
Descriptor		ENDS

L1CODELARGETLB		EQU		0t
L1DATALARGETLB		EQU		1t
L1CODETLB			EQU		2t
L1CODETLBANDLARGE	EQU		3t
L1DATATLB			EQU		4t
L1DATATLBANDLARGE	EQU		5t
L2CODELARGETLB		EQU		6t
L2DATALARGETLB		EQU		7t
L2CODETLB			EQU		8t
L2DATATLB			EQU		9t
L1CODE				EQU		10t
L1CODETRACE			EQU		11t
L1DATA				EQU		12t
L2DATA				EQU		13t
L3DATA				EQU		14t

Translate		STRUCT 4t
	descriptor	BYTE		?
	cacheType	BYTE		?
	tlbInfo		TLBInfo		<>
	cacheInfo	CacheInfo	<>
Translate		ENDS

Registers		STRUCT 4t
	regeax		WORD		?
	regebx		WORD		?
	regecx		WORD		?
	regedx		WORD		?
Registers		ENDS

BuildTables			PROTO C 
HasFPU				PROTO C :PTR CPUINFO
HasMMX				PROTO C :PTR CPUINFO
HasMMXExt			PROTO C :PTR CPUINFO
Has3DNow			PROTO C :PTR CPUINFO
Has3DNowExt			PROTO C :PTR CPUINFO
HasSSE				PROTO C :PTR CPUINFO
HasSSE2				PROTO C :PTR CPUINFO
HasSSE3				PROTO C :PTR CPUINFO
HasSSSE3			PROTO C :PTR CPUINFO
HasSSE4_1			PROTO C :PTR CPUINFO
HasSSE4_2			PROTO C :PTR CPUINFO
HasHTT				PROTO C :PTR CPUINFO
HasSerial			PROTO C :PTR CPUINFO
Is64Bit				PROTO C :PTR CPUINFO
HasCPUID			PROTO C 
Is8086				PROTO C 
Is80286				PROTO C 
Is80386				PROTO C 
GetFamily			PROTO C 
GetFPU				PROTO C :BYTE
GetClockSpeed		PROTO C 
TranslateDescriptor	PROTO C :PTR CPUINFO, :BYTE
StoreDescriptors	PROTO C :PTR CPUINFO, :WORD, :PTR PTR BYTE
GetCPUInfo			PROTO C :PTR CPUINFO, :BYTE

EXTERNDEF		C brandStrings:PTR BYTE
EXTERNDEF		C descriptorStrings:PTR BYTE
EXTERNDEF		C typeStrings:PTR BYTE