;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - K7 optimized SAD operators -
; *
; *  Copyright(C) 2001 Peter Ross <pross@xvid.org>
; *               2002 Pascal Massimino <skal@planet-d.net>
; *
; *  This program is free software; you can redistribute it and/or modify it
; *  under the terms of the GNU General Public License as published by
; *  the Free Software Foundation; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; * $Id: sad_mmx.asm,v 1.20.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
mmx_one:
	times 4	dw 1

;=============================================================================
; Helper macros
;=============================================================================

%macro SAD_16x16_MMX 0
  movq mm0, [_EAX]
  movq mm1, [TMP1]

  movq mm2, [_EAX+8]
  movq mm3, [TMP1+8]

  movq mm4, mm0
  psubusb mm0, mm1
  lea _EAX, [_EAX+TMP0]
  movq mm5, mm2
  psubusb mm2, mm3

  psubusb mm1, mm4
  psubusb mm3, mm5
  por mm0, mm1
  por mm2, mm3

  movq mm1, mm0
  punpcklbw mm0,mm7
  movq mm3, mm2
  punpckhbw mm1,mm7
  lea TMP1, [TMP1+TMP0]
  punpcklbw mm2,mm7
  paddusw mm0, mm1
  punpckhbw mm3,mm7
  paddusw mm6, mm0
  paddusw mm2, mm3
  paddusw mm6, mm2

%endmacro

%macro SAD_8x8_MMX	0
  movq mm0, [_EAX]
  movq mm1, [TMP1]

  movq mm2, [_EAX+TMP0]
  movq mm3, [TMP1+TMP0]

  lea _EAX,[_EAX+2*TMP0]
  lea TMP1,[TMP1+2*TMP0]

  movq mm4, mm0
  psubusb mm0, mm1
  movq mm5, mm2
  psubusb mm2, mm3

  psubusb mm1, mm4
  psubusb mm3, mm5
  por mm0, mm1
  por mm2, mm3

  movq mm1,mm0
  punpcklbw mm0,mm7
  movq mm3,mm2
  punpckhbw mm1,mm7
  punpcklbw mm2,mm7
  paddusw mm0,mm1
  punpckhbw mm3,mm7
  paddusw mm6,mm0
  paddusw mm2,mm3
  paddusw mm6,mm2
%endmacro


%macro SADV_16x16_MMX 0
  movq mm0, [_EAX]
  movq mm1, [TMP1]

  movq mm2, [_EAX+8]
  movq mm4, mm0
  movq mm3, [TMP1+8]
  psubusb mm0, mm1

  psubusb mm1, mm4
  lea _EAX,[_EAX+TMP0]
  por mm0, mm1

  movq mm4, mm2
  psubusb mm2, mm3

  psubusb mm3, mm4
  por mm2, mm3
  
  movq mm1,mm0
  punpcklbw mm0,mm7
  movq mm3,mm2
  punpckhbw mm1,mm7
  punpcklbw mm2,mm7
  paddusw mm0,mm1
  punpckhbw mm3,mm7
  paddusw mm5, mm0
  paddusw mm2,mm3
  lea TMP1,[TMP1+TMP0]
  paddusw mm6, mm2
%endmacro

%macro SADBI_16x16_MMX 2    ; SADBI_16x16_MMX( int_ptr_offset, bool_increment_ptr );

  movq mm0, [TMP1+%1]
  movq mm2, [_EBX+%1]
  movq mm1, mm0
  movq mm3, mm2

%if %2 != 0
  add TMP1, TMP0
%endif

  punpcklbw mm0, mm7
  punpckhbw mm1, mm7
  punpcklbw mm2, mm7
  punpckhbw mm3, mm7

%if %2 != 0
  add _EBX, TMP0
%endif

  paddusw mm0, mm2              ; mm01 = ref1 + ref2
  paddusw mm1, mm3
  paddusw mm0, [mmx_one]        ; mm01 += 1
  paddusw mm1, [mmx_one]
  psrlw mm0, 1                  ; mm01 >>= 1
  psrlw mm1, 1

  movq mm2, [_EAX+%1]
  movq mm3, mm2
  punpcklbw mm2, mm7            ; mm23 = src
  punpckhbw mm3, mm7

%if %2 != 0
  add _EAX, TMP0
%endif

  movq mm4, mm0
  movq mm5, mm1
  psubusw mm0, mm2
  psubusw mm1, mm3
  psubusw mm2, mm4
  psubusw mm3, mm5
  por mm0, mm2                  ; mm01 = ABS(mm01 - mm23)
  por mm1, mm3

  paddusw mm6, mm0              ; mm6 += mm01
  paddusw mm6, mm1

%endmacro

%macro MEAN_16x16_MMX 0
  movq mm0, [_EAX]
  movq mm2, [_EAX+8]
  lea _EAX, [_EAX+TMP0]
  movq mm1, mm0
  punpcklbw mm0, mm7
  movq mm3, mm2
  punpckhbw mm1, mm7
  paddw mm5, mm0
  punpcklbw mm2, mm7
  paddw mm6, mm1
  punpckhbw mm3, mm7
  paddw mm5, mm2
  paddw mm6, mm3
%endmacro

%macro ABS_16x16_MMX 0
  movq mm0, [_EAX]
  movq mm2, [_EAX+8]
  lea _EAX, [_EAX+TMP0]
  movq mm1, mm0
  movq mm3, mm2
  punpcklbw mm0, mm7
  punpcklbw mm2, mm7
  punpckhbw mm1, mm7
  punpckhbw mm3, mm7
  movq mm4, mm6
  psubusw mm4, mm0

  psubusw mm0, mm6
  por mm0, mm4
  movq mm4, mm6
  psubusw mm4, mm1
  psubusw mm1, mm6
  por mm1, mm4

  movq mm4, mm6
  psubusw mm4, mm2
  psubusw mm2, mm6
  por mm2, mm4
  movq mm4, mm6
  psubusw mm4, mm3
  psubusw mm3, mm6
  por mm3, mm4

  paddw mm0, mm1
  paddw mm2, mm3
  paddw mm5, mm0
  paddw mm5, mm2
%endmacro

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal sad16_mmx
cglobal sad16v_mmx
cglobal sad8_mmx
cglobal sad16bi_mmx
cglobal sad8bi_mmx
cglobal dev16_mmx
cglobal sse8_16bit_mmx
cglobal sse8_8bit_mmx
	
;-----------------------------------------------------------------------------
;
; uint32_t sad16_mmx(const uint8_t * const cur,
;					 const uint8_t * const ref,
;					 const uint32_t stride,
;					 const uint32_t best_sad);
;
; (early termination ignore; slows this down)
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad16_mmx:

  mov _EAX, prm1 ; Src1
  mov TMP1, prm2 ; Src2
  mov TMP0, prm3 ; Stride

  pxor mm6, mm6 ; accum
  pxor mm7, mm7 ; zero

  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX

  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX
  SAD_16x16_MMX

  pmaddwd mm6, [mmx_one] ; collapse
  movq mm7, mm6
  psrlq mm7, 32
  paddd mm6, mm7

  movd eax, mm6

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sad8_mmx(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad8_mmx:

  mov _EAX, prm1 ; Src1
  mov TMP1, prm2 ; Src2
  mov TMP0, prm3 ; Stride

  pxor mm6, mm6 ; accum
  pxor mm7, mm7 ; zero

  SAD_8x8_MMX
  SAD_8x8_MMX
  SAD_8x8_MMX
  SAD_8x8_MMX

  pmaddwd mm6, [mmx_one] ; collapse
  movq mm7, mm6
  psrlq mm7, 32
  paddd mm6, mm7

  movd eax, mm6

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sad16v_mmx(const uint8_t * const cur,
;				      const uint8_t * const ref,
;					  const uint32_t stride,
;					  int32_t *sad);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad16v_mmx:

  mov _EAX, prm1 ; Src1
  mov TMP1, prm2 ; Src2
  mov TMP0, prm3 ; Stride

  push _EBX
  push _EDI
%ifdef ARCH_IS_X86_64
  mov _EBX, prm4
%else
  mov _EBX, [_ESP + 8 + 16] ; sad ptr
%endif

  pxor mm5, mm5 ; accum
  pxor mm6, mm6 ; accum
  pxor mm7, mm7 ; zero

  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX

  pmaddwd mm5, [mmx_one] ; collapse
  pmaddwd mm6, [mmx_one] ; collapse

  movq mm2, mm5
  movq mm3, mm6

  psrlq mm2, 32
  psrlq mm3, 32

  paddd mm5, mm2
  paddd mm6, mm3

  movd [_EBX], mm5
  movd [_EBX + 4], mm6

  paddd mm5, mm6

  movd edi, mm5

  pxor mm5, mm5
  pxor mm6, mm6

  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX
  SADV_16x16_MMX

  pmaddwd mm5, [mmx_one] ; collapse
  pmaddwd mm6, [mmx_one] ; collapse

  movq mm2, mm5
  movq mm3, mm6

  psrlq mm2, 32
  psrlq mm3, 32

  paddd mm5, mm2
  paddd mm6, mm3

  movd [_EBX + 8], mm5
  movd [_EBX + 12], mm6

  paddd mm5, mm6

  movd eax, mm5

  add _EAX, _EDI 

  pop _EDI
  pop _EBX

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sad16bi_mmx(const uint8_t * const cur,
; const uint8_t * const ref1,
; const uint8_t * const ref2,
; const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad16bi_mmx:
  mov _EAX, prm1 ; Src
  mov TMP1, prm2 ; Ref1
  mov TMP0, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3 ; Ref2
%else
  mov _EBX, [_ESP+4+12] ; Ref2
%endif

  pxor mm6, mm6 ; accum2
  pxor mm7, mm7
.Loop:
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1

  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1
  SADBI_16x16_MMX 0, 0
  SADBI_16x16_MMX 8, 1

  pmaddwd mm6, [mmx_one] ; collapse
  movq mm7, mm6
  psrlq mm7, 32
  paddd mm6, mm7

  movd eax, mm6
  pop _EBX

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sad8bi_mmx(const uint8_t * const cur,
; const uint8_t * const ref1,
; const uint8_t * const ref2,
; const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad8bi_mmx:
  mov _EAX, prm1 ; Src
  mov TMP1, prm2 ; Ref1
  mov TMP0, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref2
%endif

  pxor mm6, mm6 ; accum2
  pxor mm7, mm7
.Loop:
  SADBI_16x16_MMX 0, 1
  SADBI_16x16_MMX 0, 1
  SADBI_16x16_MMX 0, 1
  SADBI_16x16_MMX 0, 1
  SADBI_16x16_MMX 0, 1
  SADBI_16x16_MMX 0, 1
  SADBI_16x16_MMX 0, 1
  SADBI_16x16_MMX 0, 1

  pmaddwd mm6, [mmx_one] ; collapse
  movq mm7, mm6
  psrlq mm7, 32
  paddd mm6, mm7

  movd eax, mm6
  pop _EBX
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t dev16_mmx(const uint8_t * const cur,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
dev16_mmx:
  mov _EAX, prm1 ; Src
  mov TMP0, prm2 ; Stride

  pxor mm7, mm7 ; zero
  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX

  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX
  MEAN_16x16_MMX

  paddusw mm6, mm5
  pmaddwd mm6, [mmx_one]    ; collapse
  movq mm5, mm6
  psrlq mm5, 32
  paddd mm6, mm5

  psllq mm6, 32             ; blank upper dword
  psrlq mm6, 32 + 8         ;    /= (16*16)

  punpckldq mm6, mm6
  packssdw mm6, mm6

    ; mm6 contains the mean
    ; mm5 is the new accum

  pxor mm5, mm5
  mov _EAX, prm1         ; Src

  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX

  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX
  ABS_16x16_MMX

  pmaddwd mm5, [mmx_one]    ; collapse
  movq mm6, mm5
  psrlq mm6, 32
  paddd mm6, mm5

  movd eax, mm6

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sse8_16bit_mmx(const int16_t *b1,
;                         const int16_t *b2,
;                         const uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro ROW_SSE_16bit_MMX 2
  movq mm0, [%1]
  movq mm1, [%1+8]
  psubw mm0, [%2]
  psubw mm1, [%2+8]
  pmaddwd mm0, mm0
  pmaddwd mm1, mm1
  paddd mm2, mm0
  paddd mm2, mm1
%endmacro
	
sse8_16bit_mmx:

  ;; Load the function params
  mov _EAX, prm1
  mov TMP0, prm2
  mov TMP1, prm3

  ;; Reset the sse accumulator
  pxor mm2, mm2

  ;; Let's go
%rep 8
  ROW_SSE_16bit_MMX _EAX, TMP0
  lea _EAX, [_EAX+TMP1]
  lea TMP0, [TMP0+TMP1]
%endrep

  ;; Finish adding each dword of the accumulator
  movq mm3, mm2
  psrlq mm2, 32
  paddd mm2, mm3
  movd eax, mm2

  ;; All done
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sse8_8bit_mmx(const int8_t *b1,
;                        const int8_t *b2,
;                        const uint32_t stride);
;
;-----------------------------------------------------------------------------

%macro ROW_SSE_8bit_MMX 2
  movq mm0, [%1] ; load a row
  movq mm2, [%2] ; load a row

  movq mm1, mm0  ; copy row
  movq mm3, mm2  ; copy row

  punpcklbw mm0, mm7 ; turn the 4low elements into 16bit
  punpckhbw mm1, mm7 ; turn the 4high elements into 16bit

  punpcklbw mm2, mm7 ; turn the 4low elements into 16bit
  punpckhbw mm3, mm7 ; turn the 4high elements into 16bit

  psubw mm0, mm2 ; low  part of src-dst
  psubw mm1, mm3 ; high part of src-dst

  pmaddwd mm0, mm0 ; compute the square sum
  pmaddwd mm1, mm1 ; compute the square sum

  paddd mm6, mm0 ; add to the accumulator
  paddd mm6, mm1 ; add to the accumulator
%endmacro

sse8_8bit_mmx:

  ;; Load the function params
  mov _EAX, prm1
  mov TMP0, prm2
  mov TMP1, prm3

  ;; Reset the sse accumulator
  pxor mm6, mm6

  ;; Used to interleave 8bit data with 0x00 values
  pxor mm7, mm7

  ;; Let's go
%rep 8
  ROW_SSE_8bit_MMX _EAX, TMP0
  lea _EAX, [_EAX+TMP1]
  lea TMP0, [TMP0+TMP1]
%endrep

  ;; Finish adding each dword of the accumulator
  movq mm7, mm6
  psrlq mm6, 32
  paddd mm6, mm7
  movd eax, mm6

  ;; All done
  ret
ENDFUNC

NON_EXEC_STACK
