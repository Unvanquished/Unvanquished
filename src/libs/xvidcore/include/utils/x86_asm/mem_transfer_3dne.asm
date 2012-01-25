;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - 8<->16 bit transfer functions -
; *
; *  Copyright (C) 2002 Jaan Kalda
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
; * $Id: mem_transfer_3dne.asm,v 1.11.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

; these 3dne functions are compatible with iSSE, but are optimized specifically
; for K7 pipelines

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
mm_zero:
	dd 0,0
;=============================================================================
; Macros
;=============================================================================

%ifdef ARCH_IS_X86_64
%define nop4
%else
%macro nop4 0
	db 08Dh, 074h, 026h, 0
%endmacro
%endif

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal transfer_8to16copy_3dne
cglobal transfer_16to8copy_3dne
cglobal transfer_8to16sub_3dne
cglobal transfer_8to16subro_3dne
cglobal transfer_8to16sub2_3dne
cglobal transfer_16to8add_3dne
cglobal transfer8x8_copy_3dne
cglobal transfer8x4_copy_3dne

;-----------------------------------------------------------------------------
;
; void transfer_8to16copy_3dne(int16_t * const dst,
;							const uint8_t * const src,
;							uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
transfer_8to16copy_3dne:

  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; Stride
  mov TMP0, prm1 ; Dst
  punpcklbw mm0, [byte _EAX]
  punpcklbw mm1, [_EAX+4]
  movq mm2, [_EAX+TMP1]
  movq mm3, [_EAX+TMP1]
  pxor mm7, mm7
  lea _EAX, [_EAX+2*TMP1]
  punpcklbw mm2, mm7
  punpckhbw mm3, mm7
  psrlw mm0, 8
  psrlw mm1, 8
  punpcklbw mm4, [_EAX]
  punpcklbw mm5, [_EAX+TMP1+4]
  movq [byte TMP0+0*64], mm0
  movq [TMP0+0*64+8], mm1
  punpcklbw mm6, [_EAX+TMP1]
  punpcklbw mm7, [_EAX+4]
  lea _EAX, [byte _EAX+2*TMP1]
  psrlw mm4, 8
  psrlw mm5, 8
  punpcklbw mm0, [_EAX]
  punpcklbw mm1, [_EAX+TMP1+4]
  movq [TMP0+0*64+16], mm2
  movq [TMP0+0*64+24], mm3
  psrlw mm6, 8
  psrlw mm7, 8
  punpcklbw mm2, [_EAX+TMP1]
  punpcklbw mm3, [_EAX+4]
  lea _EAX, [byte _EAX+2*TMP1]
  movq [byte TMP0+0*64+32], mm4
  movq [TMP0+0*64+56], mm5
  psrlw mm0, 8
  psrlw mm1, 8
  punpcklbw mm4, [_EAX]
  punpcklbw mm5, [_EAX+TMP1+4]
  movq [byte TMP0+0*64+48], mm6
  movq [TMP0+0*64+40], mm7
  psrlw mm2, 8
  psrlw mm3, 8
  punpcklbw mm6, [_EAX+TMP1]
  punpcklbw mm7, [_EAX+4]
  movq [byte TMP0+1*64], mm0
  movq [TMP0+1*64+24], mm1
  psrlw mm4, 8
  psrlw mm5, 8
  movq [TMP0+1*64+16], mm2
  movq [TMP0+1*64+8], mm3
  psrlw mm6, 8
  psrlw mm7, 8
  movq [byte TMP0+1*64+32], mm4
  movq [TMP0+1*64+56], mm5
  movq [byte TMP0+1*64+48], mm6
  movq [TMP0+1*64+40], mm7
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void transfer_16to8copy_3dne(uint8_t * const dst,
;							const int16_t * const src,
;							uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
transfer_16to8copy_3dne:

  mov _EAX, prm2 ; Src
  mov TMP0, prm1 ; Dst
  mov TMP1, prm3 ; Stride

  movq mm0, [byte _EAX+0*32]
  packuswb mm0, [_EAX+0*32+8]
  movq mm1, [_EAX+0*32+16]
  packuswb mm1, [_EAX+0*32+24]
  movq mm5, [_EAX+2*32+16]
  movq mm2, [_EAX+1*32]
  packuswb mm2, [_EAX+1*32+8]
  movq mm3, [_EAX+1*32+16]
  packuswb mm3, [_EAX+1*32+24]
  movq mm6, [_EAX+3*32]
  movq mm4, [_EAX+2*32]
  packuswb mm4, [_EAX+2*32+8]
  packuswb mm5, [_EAX+2*32+24]
  movq mm7, [_EAX+3*32+16]
  packuswb mm7, [_EAX+3*32+24]
  packuswb mm6, [_EAX+3*32+8]
  movq [TMP0], mm0
  lea _EAX, [3*TMP1]
  add _EAX, TMP0
  movq [TMP0+TMP1], mm1
  movq [TMP0+2*TMP1], mm2
  movq [byte _EAX], mm3
  movq [TMP0+4*TMP1], mm4
  lea TMP0, [byte TMP0+4*TMP1]
  movq [_EAX+2*TMP1], mm5
  movq [_EAX+4*TMP1], mm7
  movq [TMP0+2*TMP1], mm6
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void transfer_8to16sub_3dne(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * const ref,
;				const uint32_t stride);
;
;-----------------------------------------------------------------------------

; when second argument == 1, reference (ebx) block is to current (_EAX)
%macro COPY_8_TO_16_SUB 2
  movq mm1, [_EAX]      ; cur
  movq mm0, mm1
  movq mm4, [TMP0]      ; ref
  movq mm6, mm4
%if %2 == 1
  movq [_EAX], mm4
%endif
  punpckhbw mm1, mm7
  punpckhbw mm6, mm7
  punpcklbw mm4, mm7
ALIGN SECTION_ALIGN
  movq mm2, [byte _EAX+TMP1]
  punpcklbw mm0, mm7
  movq mm3, [byte _EAX+TMP1]
  punpcklbw mm2, mm7
  movq mm5, [byte TMP0+TMP1]  ; ref
  punpckhbw mm3, mm7
%if %2 == 1
  movq [byte _EAX+TMP1], mm5
%endif
  psubsw mm1, mm6

  movq mm6, mm5
  psubsw mm0, mm4
%if (%1 < 3)
  lea _EAX,[_EAX+2*TMP1]
  lea TMP0,[TMP0+2*TMP1]
%else
  mov TMP0,[_ESP]
  add _ESP,byte PTR_SIZE
%endif
  movq [_EDI+%1*32+ 8], mm1
  movq [byte _EDI+%1*32+ 0], mm0 ; dst
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  psubsw mm3, mm6
  movq [_EDI+%1*32+16], mm2
  movq [_EDI+%1*32+24], mm3
%endmacro

ALIGN SECTION_ALIGN
transfer_8to16sub_3dne:
  mov _EAX, prm2 ; Cur
  mov TMP0, prm3 ; Ref
  mov TMP1, prm4 ; Stride

  push _EDI
%ifdef ARCH_IS_X86_64
  mov _EDI, prm1
%else
  mov _EDI, [_ESP+4+4] ; Dst
%endif

  pxor mm7, mm7
  nop
ALIGN SECTION_ALIGN
  COPY_8_TO_16_SUB 0, 1
  COPY_8_TO_16_SUB 1, 1
  COPY_8_TO_16_SUB 2, 1
  COPY_8_TO_16_SUB 3, 1
  mov _EDI, TMP0
  ret
ENDFUNC

ALIGN SECTION_ALIGN
transfer_8to16subro_3dne:
  mov _EAX, prm2 ; Cur
  mov TMP0, prm3 ; Ref
  mov TMP1, prm4 ; Stride

  push _EDI
%ifdef ARCH_IS_X86_64
  mov _EDI, prm1
%else
  mov _EDI, [_ESP+4+ 4] ; Dst
%endif

  pxor mm7, mm7
  nop
ALIGN SECTION_ALIGN
  COPY_8_TO_16_SUB 0, 0
  COPY_8_TO_16_SUB 1, 0
  COPY_8_TO_16_SUB 2, 0
  COPY_8_TO_16_SUB 3, 0
  mov _EDI, TMP0
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void transfer_8to16sub2_3dne(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * ref1,
;				const uint8_t * ref2,
;				const uint32_t stride)
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_16_SUB2_SSE 1
  db 0Fh, 6Fh, 44h, 20h, 00  ;movq mm0, [byte _EAX]      ; cur
  punpcklbw mm0, mm7
  movq mm2, [byte _EAX+TMP1]
  punpcklbw mm2, mm7
  db 0Fh, 6Fh, 4ch, 20h, 00 ;movq mm1, [byte _EAX]
  punpckhbw mm1, mm7
  movq mm3, [byte _EAX+TMP1]
  punpckhbw mm3, mm7

  movq mm4, [byte _EBX]      ; ref1
  pavgb mm4, [byte _ESI]     ; ref2
  movq [_EAX], mm4
  movq mm5, [_EBX+TMP1]  ; ref
  pavgb mm5, [_ESI+TMP1] ; ref2
  movq [_EAX+TMP1], mm5
  movq mm6, mm4
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
%if (%1 < 3)
  lea _ESI,[_ESI+2*TMP1]
  lea _EBX,[byte _EBX+2*TMP1]
  lea _EAX,[_EAX+2*TMP1]
%else
  mov _ESI,[_ESP]
  mov _EBX,[_ESP+PTR_SIZE]
  add _ESP,byte 2*PTR_SIZE
%endif
  psubsw mm0, mm4
  psubsw mm1, mm6
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  psubsw mm3, mm6
  movq [byte TMP0+%1*32+ 0], mm0 ; dst
  movq [TMP0+%1*32+ 8], mm1
  movq [TMP0+%1*32+16], mm2
  movq [TMP0+%1*32+24], mm3
%endmacro

ALIGN SECTION_ALIGN
transfer_8to16sub2_3dne:
  mov TMP1d, prm5d ; Stride
  mov TMP0, prm1   ; Dst
  mov _EAX, prm2   ; Cur
  push _EBX
  lea _EBP,[byte _EBP]

%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref1
%endif

  push _ESI
  pxor mm7, mm7
  
%ifdef ARCH_IS_X86_64
  mov _ESI, prm4
%else
  mov _ESI, [_ESP+8+16] ; Ref2
%endif
  
  nop4
  COPY_8_TO_16_SUB2_SSE 0
  COPY_8_TO_16_SUB2_SSE 1
  COPY_8_TO_16_SUB2_SSE 2
  COPY_8_TO_16_SUB2_SSE 3

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void transfer_16to8add_3dne(uint8_t * const dst,
;						const int16_t * const src,
;						uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro COPY_16_TO_8_ADD 1
  movq mm0, [byte TMP0]
  punpcklbw mm0, mm7
  movq mm2, [byte TMP0+TMP1]
  punpcklbw mm2, mm7
  movq mm1, [byte TMP0]
  punpckhbw mm1, mm7
  movq mm3, [byte TMP0+TMP1]
  punpckhbw mm3, mm7
  paddsw mm0, [byte _EAX+%1*32+ 0]
  paddsw mm1, [_EAX+%1*32+ 8]
  paddsw mm2, [_EAX+%1*32+16]
  paddsw mm3, [_EAX+%1*32+24]
  packuswb mm0, mm1
  packuswb mm2, mm3
  mov _ESP, _ESP
  movq [byte TMP0], mm0
  movq [TMP0+TMP1], mm2
%endmacro


ALIGN SECTION_ALIGN
transfer_16to8add_3dne:
  mov TMP0, prm1 ; Dst
  mov TMP1, prm3 ; Stride
  mov _EAX, prm2 ; Src
  pxor mm7, mm7
  nop

  COPY_16_TO_8_ADD 0
  lea TMP0,[byte TMP0+2*TMP1]
  COPY_16_TO_8_ADD 1
  lea TMP0,[byte TMP0+2*TMP1]
  COPY_16_TO_8_ADD 2
  lea TMP0,[byte TMP0+2*TMP1]
  COPY_16_TO_8_ADD 3
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void transfer8x8_copy_3dne(uint8_t * const dst,
;					const uint8_t * const src,
;					const uint32_t stride);
;
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_8 0
  movq mm0, [byte  _EAX]
  movq mm1, [_EAX+TMP1]
  movq [byte TMP0], mm0
  lea _EAX,[byte _EAX+2*TMP1]
  movq [TMP0+TMP1], mm1
%endmacro

ALIGN SECTION_ALIGN
transfer8x8_copy_3dne:
  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; Stride
  mov TMP0, prm1 ; Dst

  COPY_8_TO_8
  lea TMP0,[byte TMP0+2*TMP1]
  COPY_8_TO_8
  lea TMP0,[byte TMP0+2*TMP1]
  COPY_8_TO_8
  lea TMP0,[byte TMP0+2*TMP1]
  COPY_8_TO_8
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void transfer8x4_copy_3dne(uint8_t * const dst,
;					const uint8_t * const src,
;					const uint32_t stride);
;
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
transfer8x4_copy_3dne:
  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; Stride
  mov TMP0, prm1 ; Dst

  COPY_8_TO_8
  lea TMP0,[byte TMP0+2*TMP1]
  COPY_8_TO_8
  ret
ENDFUNC

NON_EXEC_STACK
