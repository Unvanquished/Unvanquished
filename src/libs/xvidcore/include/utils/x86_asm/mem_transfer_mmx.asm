;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - 8<->16 bit transfer functions -
; *
; *  Copyright (C) 2001 Peter Ross <pross@xvid.org>
; *                2001-2008 Michael Militzer <michael@xvid.org>
; *                2002 Pascal Massimino <skal@planet-d.net>
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
; * $Id: mem_transfer_mmx.asm,v 1.20.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
mmx_one:
	dw 1, 1, 1, 1

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal transfer_8to16copy_mmx
cglobal transfer_16to8copy_mmx
cglobal transfer_8to16sub_mmx
cglobal transfer_8to16subro_mmx
cglobal transfer_8to16sub2_mmx
cglobal transfer_8to16sub2_xmm
cglobal transfer_8to16sub2ro_xmm
cglobal transfer_16to8add_mmx
cglobal transfer8x8_copy_mmx
cglobal transfer8x4_copy_mmx

;-----------------------------------------------------------------------------
;
; void transfer_8to16copy_mmx(int16_t * const dst,
;							const uint8_t * const src,
;							uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_16 1
  movq mm0, [_EAX]
  movq mm1, [_EAX+TMP1]
  movq mm2, mm0
  movq mm3, mm1
  punpcklbw mm0, mm7
  movq [TMP0+%1*32], mm0
  punpcklbw mm1, mm7
  movq [TMP0+%1*32+16], mm1
  punpckhbw mm2, mm7
  punpckhbw mm3, mm7
  lea _EAX, [_EAX+2*TMP1]
  movq [TMP0+%1*32+8], mm2
  movq [TMP0+%1*32+24], mm3
%endmacro

ALIGN SECTION_ALIGN
transfer_8to16copy_mmx:

  mov TMP0, prm1 ; Dst
  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; Stride
  pxor mm7, mm7

  COPY_8_TO_16 0
  COPY_8_TO_16 1
  COPY_8_TO_16 2
  COPY_8_TO_16 3
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void transfer_16to8copy_mmx(uint8_t * const dst,
;							const int16_t * const src,
;							uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro COPY_16_TO_8 1
  movq mm0, [_EAX+%1*32]
  movq mm1, [_EAX+%1*32+8]
  packuswb mm0, mm1
  movq [TMP0], mm0
  movq mm2, [_EAX+%1*32+16]
  movq mm3, [_EAX+%1*32+24]
  packuswb mm2, mm3
  movq [TMP0+TMP1], mm2
%endmacro

ALIGN SECTION_ALIGN
transfer_16to8copy_mmx:

  mov TMP0, prm1 ; Dst
  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; Stride

  COPY_16_TO_8 0
  lea TMP0,[TMP0+2*TMP1]
  COPY_16_TO_8 1
  lea TMP0,[TMP0+2*TMP1]
  COPY_16_TO_8 2
  lea TMP0,[TMP0+2*TMP1]
  COPY_16_TO_8 3
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void transfer_8to16sub_mmx(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * const ref,
;				const uint32_t stride);
;
;-----------------------------------------------------------------------------

; when second argument == 1, reference (ebx) block is to current (_EAX)
%macro COPY_8_TO_16_SUB 2
  movq mm0, [_EAX]      ; cur
  movq mm2, [_EAX+TMP1]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  movq mm4, [_EBX]      ; ref
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  movq mm5, [_EBX+TMP1]  ; ref

  movq mm6, mm4
%if %2 == 1
  movq [_EAX], mm4
  movq [_EAX+TMP1], mm5
%endif
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea _EAX, [_EAX+2*TMP1]
  psubsw mm3, mm6
  lea _EBX,[_EBX+2*TMP1]

  movq [TMP0+%1*32+ 0], mm0 ; dst
  movq [TMP0+%1*32+ 8], mm1
  movq [TMP0+%1*32+16], mm2
  movq [TMP0+%1*32+24], mm3
%endmacro

ALIGN SECTION_ALIGN
transfer_8to16sub_mmx:
  mov TMP0, prm1 ; Dst
  mov _EAX, prm2 ; Cur
  mov TMP1, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref
%endif
  pxor mm7, mm7

  COPY_8_TO_16_SUB 0, 1
  COPY_8_TO_16_SUB 1, 1
  COPY_8_TO_16_SUB 2, 1
  COPY_8_TO_16_SUB 3, 1

  pop _EBX
  ret
ENDFUNC


ALIGN SECTION_ALIGN
transfer_8to16subro_mmx:
  mov TMP0, prm1 ; Dst
  mov _EAX, prm2 ; Cur
  mov TMP1, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref
%endif
  pxor mm7, mm7

  COPY_8_TO_16_SUB 0, 0
  COPY_8_TO_16_SUB 1, 0
  COPY_8_TO_16_SUB 2, 0
  COPY_8_TO_16_SUB 3, 0

  pop _EBX
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void transfer_8to16sub2_mmx(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * ref1,
;				const uint8_t * ref2,
;				const uint32_t stride)
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_16_SUB2_MMX 1
  movq mm0, [_EAX]      ; cur
  movq mm2, [_EAX+TMP1]

  ; mm4 <- (ref1+ref2+1) / 2
  movq mm4, [_EBX]      ; ref1
  movq mm1, [_ESI]      ; ref2
  movq mm6, mm4
  movq mm3, mm1
  punpcklbw mm4, mm7
  punpcklbw mm1, mm7
  punpckhbw mm6, mm7
  punpckhbw mm3, mm7
  paddusw mm4, mm1
  paddusw mm6, mm3
  paddusw mm4, [mmx_one]
  paddusw mm6, [mmx_one]
  psrlw mm4, 1
  psrlw mm6, 1
  packuswb mm4, mm6
  movq [_EAX], mm4

    ; mm5 <- (ref1+ref2+1) / 2
  movq mm5, [_EBX+TMP1]  ; ref1
  movq mm1, [_ESI+TMP1]  ; ref2
  movq mm6, mm5
  movq mm3, mm1
  punpcklbw mm5, mm7
  punpcklbw mm1, mm7
  punpckhbw mm6, mm7
  punpckhbw mm3, mm7
  paddusw mm5, mm1
  paddusw mm6, mm3
  paddusw mm5, [mmx_one]
  paddusw mm6, [mmx_one]
  lea _ESI, [_ESI+2*TMP1]
  psrlw mm5, 1
  psrlw mm6, 1
  packuswb mm5, mm6
  movq [_EAX+TMP1], mm5

  movq mm1, mm0
  movq mm3, mm2
  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7

  movq mm6, mm4
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea _EAX, [_EAX+2*TMP1]
  psubsw mm3, mm6
  lea _EBX, [_EBX+2*TMP1]

  movq [TMP0+%1*32+ 0], mm0 ; dst
  movq [TMP0+%1*32+ 8], mm1
  movq [TMP0+%1*32+16], mm2
  movq [TMP0+%1*32+24], mm3
%endmacro

ALIGN SECTION_ALIGN
transfer_8to16sub2_mmx:
  mov TMP0, prm1   ; Dst
  mov TMP1d, prm5d ; Stride
  mov _EAX, prm2   ; Cur

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref1
%endif

  push _ESI
%ifdef ARCH_IS_X86_64
  mov _ESI, prm4
%else
  mov _ESI, [_ESP+8+16] ; Ref2
%endif

  pxor mm7, mm7

  COPY_8_TO_16_SUB2_MMX 0
  COPY_8_TO_16_SUB2_MMX 1
  COPY_8_TO_16_SUB2_MMX 2
  COPY_8_TO_16_SUB2_MMX 3

  pop _ESI
  pop _EBX
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void transfer_8to16sub2_xmm(int16_t * const dct,
;				uint8_t * const cur,
;				const uint8_t * ref1,
;				const uint8_t * ref2,
;				const uint32_t stride)
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_16_SUB2_SSE 1
  movq mm0, [_EAX]      ; cur
  movq mm2, [_EAX+TMP1]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  movq mm4, [_EBX]     ; ref1
  pavgb mm4, [_ESI]     ; ref2
  movq [_EAX], mm4
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  movq mm5, [_EBX+TMP1] ; ref
  pavgb mm5, [_ESI+TMP1] ; ref2
  movq [_EAX+TMP1], mm5

  movq mm6, mm4
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  lea _ESI, [_ESI+2*TMP1]
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea _EAX, [_EAX+2*TMP1]
  psubsw mm3, mm6
  lea _EBX, [_EBX+2*TMP1]

  movq [TMP0+%1*32+ 0], mm0 ; dst
  movq [TMP0+%1*32+ 8], mm1
  movq [TMP0+%1*32+16], mm2
  movq [TMP0+%1*32+24], mm3
%endmacro

ALIGN SECTION_ALIGN
transfer_8to16sub2_xmm:
  mov TMP0, prm1   ; Dst
  mov _EAX, prm2   ; Cur
  mov TMP1d, prm5d ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3 ; Ref1
%else
  mov _EBX, [_ESP+4+12] ; Ref1
%endif

  push _ESI
%ifdef ARCH_IS_X86_64
  mov _ESI, prm4 ; Ref1
%else
  mov _ESI, [_ESP+8+16] ; Ref2
%endif

  pxor mm7, mm7

  COPY_8_TO_16_SUB2_SSE 0
  COPY_8_TO_16_SUB2_SSE 1
  COPY_8_TO_16_SUB2_SSE 2
  COPY_8_TO_16_SUB2_SSE 3

  pop _ESI
  pop _EBX
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void transfer_8to16sub2ro_xmm(int16_t * const dct,
;				const uint8_t * const cur,
;				const uint8_t * ref1,
;				const uint8_t * ref2,
;				const uint32_t stride)
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_16_SUB2RO_SSE 1
  movq mm0, [_EAX]      ; cur
  movq mm2, [_EAX+TMP1]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  movq mm4, [_EBX]     ; ref1
  pavgb mm4, [_ESI]     ; ref2
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  movq mm5, [_EBX+TMP1] ; ref
  pavgb mm5, [_ESI+TMP1] ; ref2

  movq mm6, mm4
  punpcklbw mm4, mm7
  punpckhbw mm6, mm7
  psubsw mm0, mm4
  psubsw mm1, mm6
  lea _ESI, [_ESI+2*TMP1]
  movq mm6, mm5
  punpcklbw mm5, mm7
  punpckhbw mm6, mm7
  psubsw mm2, mm5
  lea _EAX, [_EAX+2*TMP1]
  psubsw mm3, mm6
  lea _EBX, [_EBX+2*TMP1]

  movq [TMP0+%1*32+ 0], mm0 ; dst
  movq [TMP0+%1*32+ 8], mm1
  movq [TMP0+%1*32+16], mm2
  movq [TMP0+%1*32+24], mm3
%endmacro

ALIGN SECTION_ALIGN
transfer_8to16sub2ro_xmm:
  pxor mm7, mm7
  mov TMP0, prm1   ; Dst
  mov _EAX, prm2   ; Cur
  mov TMP1d, prm5d ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref1
%endif

  push _ESI
%ifdef ARCH_IS_X86_64
  mov _ESI, prm4
%else
  mov _ESI, [_ESP+8+16] ; Ref2
%endif

  COPY_8_TO_16_SUB2RO_SSE 0
  COPY_8_TO_16_SUB2RO_SSE 1
  COPY_8_TO_16_SUB2RO_SSE 2
  COPY_8_TO_16_SUB2RO_SSE 3

  pop _ESI
  pop _EBX
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void transfer_16to8add_mmx(uint8_t * const dst,
;						const int16_t * const src,
;						uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro COPY_16_TO_8_ADD 1
  movq mm0, [TMP0]
  movq mm2, [TMP0+TMP1]
  movq mm1, mm0
  movq mm3, mm2
  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  paddsw mm0, [_EAX+%1*32+ 0]
  paddsw mm1, [_EAX+%1*32+ 8]
  paddsw mm2, [_EAX+%1*32+16]
  paddsw mm3, [_EAX+%1*32+24]
  packuswb mm0, mm1
  movq [TMP0], mm0
  packuswb mm2, mm3
  movq [TMP0+TMP1], mm2
%endmacro


ALIGN SECTION_ALIGN
transfer_16to8add_mmx:
  mov TMP0, prm1 ; Dst
  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; Stride
  pxor mm7, mm7

  COPY_16_TO_8_ADD 0
  lea TMP0,[TMP0+2*TMP1]
  COPY_16_TO_8_ADD 1
  lea TMP0,[TMP0+2*TMP1]
  COPY_16_TO_8_ADD 2
  lea TMP0,[TMP0+2*TMP1]
  COPY_16_TO_8_ADD 3
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void transfer8x8_copy_mmx(uint8_t * const dst,
;					const uint8_t * const src,
;					const uint32_t stride);
;
;
;-----------------------------------------------------------------------------

%macro COPY_8_TO_8 0
  movq mm0, [_EAX]
  movq mm1, [_EAX+TMP1]
  movq [TMP0], mm0
  lea _EAX, [_EAX+2*TMP1]
  movq [TMP0+TMP1], mm1
%endmacro

ALIGN SECTION_ALIGN
transfer8x8_copy_mmx:
  mov TMP0, prm1 ; Dst
  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; Stride

  COPY_8_TO_8
  lea TMP0,[TMP0+2*TMP1]
  COPY_8_TO_8
  lea TMP0,[TMP0+2*TMP1]
  COPY_8_TO_8
  lea TMP0,[TMP0+2*TMP1]
  COPY_8_TO_8
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void transfer8x4_copy_mmx(uint8_t * const dst,
;					const uint8_t * const src,
;					const uint32_t stride);
;
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
transfer8x4_copy_mmx:
  mov TMP0, prm1 ; Dst
  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; Stride

  COPY_8_TO_8
  lea TMP0,[TMP0+2*TMP1]
  COPY_8_TO_8
  ret
ENDFUNC

NON_EXEC_STACK
