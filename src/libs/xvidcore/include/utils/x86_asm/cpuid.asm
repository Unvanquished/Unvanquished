;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - CPUID check processors capabilities -
; *
; *  Copyright (C) 2001-2008 Michael Militzer <michael@xvid.org>
; *
; *  This program is free software ; you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
; *  the Free Software Foundation ; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program ; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; * $Id: cpuid.asm,v 1.15.2.3 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Constants
;=============================================================================

%define CPUID_TSC               0x00000010
%define CPUID_MMX               0x00800000
%define CPUID_SSE               0x02000000
%define CPUID_SSE2              0x04000000
%define CPUID_SSE3              0x00000001
%define CPUID_SSE41             0x00080000

%define EXT_CPUID_3DNOW         0x80000000
%define EXT_CPUID_AMD_3DNOWEXT  0x40000000
%define EXT_CPUID_AMD_MMXEXT    0x00400000

;;; NB: Make sure these defines match the ones defined in xvid.h
%define XVID_CPU_MMX      (1<< 0)
%define XVID_CPU_MMXEXT   (1<< 1)
%define XVID_CPU_SSE      (1<< 2)
%define XVID_CPU_SSE2     (1<< 3)
%define XVID_CPU_SSE3     (1<< 8)
%define XVID_CPU_SSE41    (1<< 9)
%define XVID_CPU_3DNOW    (1<< 4)
%define XVID_CPU_3DNOWEXT (1<< 5)
%define XVID_CPU_TSC      (1<< 6)

;=============================================================================
; Read only data
;=============================================================================

ALIGN SECTION_ALIGN

DATA

vendorAMD:
		db "AuthenticAMD"

;=============================================================================
; Macros
;=============================================================================

%macro  CHECK_FEATURE         4
  mov eax, %1
  and eax, %4
  neg eax 
  sbb eax, eax 
  and eax, %2
  or %3, eax 
%endmacro

;=============================================================================
; Code
;=============================================================================

%ifdef ARCH_IS_X86_64
%define XVID_PUSHFD pushfq
%define XVID_POPFD  popfq
%else
%define XVID_PUSHFD pushfd
%define XVID_POPFD  popfd
%endif

TEXT

; int check_cpu_feature(void)

cglobal check_cpu_features
check_cpu_features:

  push _EBX
  push _ESI
  push _EDI
  push _EBP

  sub _ESP, 12             ; Stack space for vendor name
  
  xor ebp, ebp 

	; CPUID command ?
  XVID_PUSHFD 
  pop _EAX
  mov ecx, eax 
  xor eax, 0x200000
  push _EAX
  XVID_POPFD 
  XVID_PUSHFD 
  pop _EAX
  cmp eax, ecx 

  jz near .cpu_quit		; no CPUID command -> exit

	; get vendor string, used later
  xor eax, eax 
  cpuid
  mov [_ESP], ebx        ; vendor string
  mov [_ESP+4], edx
  mov [_ESP+8], ecx
  test eax, eax 

  jz near .cpu_quit

  mov eax, 1
  cpuid

 ; RDTSC command ?
  CHECK_FEATURE CPUID_TSC, XVID_CPU_TSC, ebp, edx 

  ; MMX support ?
  CHECK_FEATURE CPUID_MMX, XVID_CPU_MMX, ebp, edx 

  ; SSE support ?
  CHECK_FEATURE CPUID_SSE, (XVID_CPU_MMXEXT|XVID_CPU_SSE), ebp, edx 

  ; SSE2 support?
  CHECK_FEATURE CPUID_SSE2, XVID_CPU_SSE2, ebp, edx 

  ; SSE3 support?
  CHECK_FEATURE CPUID_SSE3, XVID_CPU_SSE3, ebp, ecx 

  ; SSE41 support?
  CHECK_FEATURE CPUID_SSE41, XVID_CPU_SSE41, ebp, ecx 

  ; extended functions?
  mov eax, 0x80000000
  cpuid
  cmp eax, 0x80000000
  jbe near .cpu_quit

  mov eax, 0x80000001
  cpuid

 ; AMD cpu ?
  lea _ESI, [vendorAMD]
  lea _EDI, [_ESP]
  mov ecx, 12
  cld
  repe cmpsb
  jnz .cpu_quit

  ; 3DNow! support ?
  CHECK_FEATURE EXT_CPUID_3DNOW, XVID_CPU_3DNOW, ebp, edx 

  ; 3DNOW extended ?
  CHECK_FEATURE EXT_CPUID_AMD_3DNOWEXT, XVID_CPU_3DNOWEXT, ebp, edx 

  ; extended MMX ?
  CHECK_FEATURE EXT_CPUID_AMD_MMXEXT, XVID_CPU_MMXEXT, ebp, edx 

.cpu_quit:

  mov eax, ebp 

  add _ESP, 12

  pop _EBP
  pop _EDI
  pop _ESI
  pop _EBX

  ret
ENDFUNC

; sse/sse2 operating support detection routines
; these will trigger an invalid instruction signal if not supported.
ALIGN SECTION_ALIGN
cglobal sse_os_trigger
sse_os_trigger:
  xorps xmm0, xmm0
  ret
ENDFUNC


ALIGN SECTION_ALIGN
cglobal sse2_os_trigger
sse2_os_trigger:
  xorpd xmm0, xmm0
  ret
ENDFUNC


; enter/exit mmx state
ALIGN SECTION_ALIGN
cglobal emms_mmx
emms_mmx:
  emms
  ret
ENDFUNC

; faster enter/exit mmx state
ALIGN SECTION_ALIGN
cglobal emms_3dn
emms_3dn:
  femms
  ret
ENDFUNC

%ifdef ARCH_IS_X86_64
%ifdef WINDOWS
cglobal prime_xmm
prime_xmm:
  movdqa xmm6, [prm1]
  movdqa xmm7, [prm1+16]
  ret
ENDFUNC

cglobal get_xmm
get_xmm:
  movdqa [prm1], xmm6
  movdqa [prm1+16], xmm7
  ret
ENDFUNC
%endif
%endif


NON_EXEC_STACK
