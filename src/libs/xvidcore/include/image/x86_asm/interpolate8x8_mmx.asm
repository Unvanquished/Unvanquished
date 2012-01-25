;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - mmx 8x8 block-based halfpel interpolation -
; *
; *  Copyright(C) 2001 Peter Ross <pross@xvid.org>
; *               2002-2008 Michael Militzer <michael@xvid.org>
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
; ****************************************************************************/

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

;-----------------------------------------------------------------------------
; (16 - r) rounding table
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
rounding_lowpass_mmx:
	times 4 dw 16
	times 4 dw 15

;-----------------------------------------------------------------------------
; (1 - r) rounding table
;-----------------------------------------------------------------------------

rounding1_mmx:
	times 4 dw 1
	times 4 dw 0

;-----------------------------------------------------------------------------
; (2 - r) rounding table
;-----------------------------------------------------------------------------

rounding2_mmx:
	times 4 dw 2
	times 4 dw 1

mmx_one:
	times 8 db 1

mmx_two:
	times 8 db 2

mmx_three:
	times 8 db 3

mmx_five:
	times 4 dw 5

mmx_mask:
	times 8 db 254

mmx_mask2:
	times 8 db 252

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal interpolate8x8_halfpel_h_mmx
cglobal interpolate8x8_halfpel_v_mmx
cglobal interpolate8x8_halfpel_hv_mmx

cglobal interpolate8x4_halfpel_h_mmx
cglobal interpolate8x4_halfpel_v_mmx
cglobal interpolate8x4_halfpel_hv_mmx

cglobal interpolate8x8_avg4_mmx
cglobal interpolate8x8_avg2_mmx

cglobal interpolate8x8_6tap_lowpass_h_mmx
cglobal interpolate8x8_6tap_lowpass_v_mmx

cglobal interpolate8x8_halfpel_add_mmx
cglobal interpolate8x8_halfpel_h_add_mmx
cglobal interpolate8x8_halfpel_v_add_mmx
cglobal interpolate8x8_halfpel_hv_add_mmx

%macro  CALC_AVG 6
  punpcklbw %3, %6
  punpckhbw %4, %6

  paddusw %1, %3    ; mm01 += mm23
  paddusw %2, %4
  paddusw %1, %5    ; mm01 += rounding
  paddusw %2, %5

  psrlw %1, 1   ; mm01 >>= 1
  psrlw %2, 1
%endmacro


;-----------------------------------------------------------------------------
;
; void interpolate8x8_halfpel_h_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro COPY_H_MMX 0
  movq mm0, [TMP0]
  movq mm2, [TMP0 + 1]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm6        ; mm01 = [src]
  punpckhbw mm1, mm6        ; mm23 = [src + 1]

  CALC_AVG mm0, mm1, mm2, mm3, mm7, mm6

  packuswb mm0, mm1
  movq [_EAX], mm0           ; [dst] = mm01

  add TMP0, TMP1              ; src += stride
  add _EAX, TMP1              ; dst += stride
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_h_mmx:

  mov _EAX, prm4       ; rounding
  lea TMP0, [rounding1_mmx]
  movq mm7, [TMP0 + _EAX * 8]

  mov _EAX, prm1        ; dst
  mov TMP0, prm2        ; src
  mov TMP1, prm3        ; stride

  pxor mm6, mm6         ; zero

  COPY_H_MMX
  COPY_H_MMX
  COPY_H_MMX
  COPY_H_MMX
  COPY_H_MMX
  COPY_H_MMX
  COPY_H_MMX
  COPY_H_MMX

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void interpolate8x8_halfpel_v_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro COPY_V_MMX 0
  movq mm0, [TMP0]
  movq mm2, [TMP0 + TMP1]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm6    ; mm01 = [src]
  punpckhbw mm1, mm6    ; mm23 = [src + 1]

  CALC_AVG mm0, mm1, mm2, mm3, mm7, mm6

  packuswb mm0, mm1
  movq [_EAX], mm0      ; [dst] = mm01

  add TMP0, TMP1        ; src += stride
  add _EAX, TMP1        ; dst += stride
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_v_mmx:

  mov _EAX, prm4       ; rounding
  lea TMP0, [rounding1_mmx]
  movq mm7, [TMP0 + _EAX * 8]

  mov _EAX, prm1       ; dst
  mov TMP0, prm2       ; src
  mov TMP1, prm3       ; stride

  pxor mm6, mm6        ; zero


  COPY_V_MMX
  COPY_V_MMX
  COPY_V_MMX
  COPY_V_MMX
  COPY_V_MMX
  COPY_V_MMX
  COPY_V_MMX
  COPY_V_MMX

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void interpolate8x8_halfpel_hv_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;-----------------------------------------------------------------------------

%macro COPY_HV_MMX 0
    ; current row
  movq mm0, [TMP0]
  movq mm2, [TMP0 + 1]

  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm6        ; mm01 = [src]
  punpcklbw mm2, mm6        ; mm23 = [src + 1]
  punpckhbw mm1, mm6
  punpckhbw mm3, mm6

  paddusw mm0, mm2          ; mm01 += mm23
  paddusw mm1, mm3

    ; next row
  movq mm4, [TMP0 + TMP1]
  movq mm2, [TMP0 + TMP1 + 1]

  movq mm5, mm4
  movq mm3, mm2

  punpcklbw mm4, mm6        ; mm45 = [src + stride]
  punpcklbw mm2, mm6        ; mm23 = [src + stride + 1]
  punpckhbw mm5, mm6
  punpckhbw mm3, mm6

  paddusw mm4, mm2          ; mm45 += mm23
  paddusw mm5, mm3

    ; add current + next row
  paddusw mm0, mm4          ; mm01 += mm45
  paddusw mm1, mm5
  paddusw mm0, mm7          ; mm01 += rounding2
  paddusw mm1, mm7

  psrlw mm0, 2              ; mm01 >>= 2
  psrlw mm1, 2

  packuswb mm0, mm1
  movq [_EAX], mm0           ; [dst] = mm01

  add TMP0, TMP1             ; src += stride
  add _EAX, TMP1             ; dst += stride
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_hv_mmx:

  mov _EAX, prm4    ; rounding
  lea TMP0, [rounding2_mmx]
  movq mm7, [TMP0 + _EAX * 8]

  mov _EAX, prm1    ; dst
  mov TMP0, prm2    ; src

  pxor mm6, mm6     ; zero

  mov TMP1, prm3    ; stride

  COPY_HV_MMX
  COPY_HV_MMX
  COPY_HV_MMX
  COPY_HV_MMX
  COPY_HV_MMX
  COPY_HV_MMX
  COPY_HV_MMX
  COPY_HV_MMX

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x4_halfpel_h_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
interpolate8x4_halfpel_h_mmx:

  mov _EAX, prm4        ; rounding
  lea TMP0, [rounding1_mmx]
  movq mm7, [TMP0 + _EAX * 8]

  mov _EAX, prm1        ; dst
  mov TMP0, prm2        ; src
  mov TMP1, prm3        ; stride

  pxor mm6, mm6         ; zero

  COPY_H_MMX
  COPY_H_MMX
  COPY_H_MMX
  COPY_H_MMX

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void interpolate8x4_halfpel_v_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
interpolate8x4_halfpel_v_mmx:

  mov _EAX, prm4       ; rounding
  lea TMP0, [rounding1_mmx]
  movq mm7, [TMP0 + _EAX * 8]

  mov _EAX, prm1       ; dst
  mov TMP0, prm2       ; src
  mov TMP1, prm3       ; stride

  pxor mm6, mm6        ; zero


  COPY_V_MMX
  COPY_V_MMX
  COPY_V_MMX
  COPY_V_MMX

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void interpolate8x4_halfpel_hv_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
interpolate8x4_halfpel_hv_mmx:

  mov _EAX, prm4    ; rounding
  lea TMP0, [rounding2_mmx]
  movq mm7, [TMP0 + _EAX * 8]

  mov _EAX, prm1    ; dst
  mov TMP0, prm2    ; src

  pxor mm6, mm6     ; zero

  mov TMP1, prm3    ; stride

  COPY_HV_MMX
  COPY_HV_MMX
  COPY_HV_MMX
  COPY_HV_MMX

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x8_avg2_mmx(uint8_t const *dst,
;                              const uint8_t * const src1,
;                              const uint8_t * const src2,
;                              const uint32_t stride,
;                              const uint32_t rounding,
;                              const uint32_t height);
;
;-----------------------------------------------------------------------------

%macro AVG2_MMX_RND0 0
  movq mm0, [_EAX]           ; src1 -> mm0
  movq mm1, [_EBX]           ; src2 -> mm1

  movq mm4, [_EAX+TMP1]
  movq mm5, [_EBX+TMP1]

  movq mm2, mm0             ; src1 -> mm2
  movq mm3, mm1             ; src2 -> mm3

  pand mm2, mm7             ; isolate the lsb
  pand mm3, mm7             ; isolate the lsb

  por mm2, mm3              ; ODD(src1) OR ODD(src2) -> mm2

  movq mm3, mm4
  movq mm6, mm5

  pand mm3, mm7
  pand mm6, mm7

  por mm3, mm6

  pand mm0, [mmx_mask]
  pand mm1, [mmx_mask]
  pand mm4, [mmx_mask]
  pand mm5, [mmx_mask]

  psrlq mm0, 1              ; src1 / 2
  psrlq mm1, 1              ; src2 / 2

  psrlq mm4, 1
  psrlq mm5, 1

  paddb mm0, mm1            ; src1/2 + src2/2 -> mm0
  paddb mm0, mm2            ; correct rounding error

  paddb mm4, mm5
  paddb mm4, mm3

  lea _EAX, [_EAX+2*TMP1]
  lea _EBX, [_EBX+2*TMP1]

  movq [TMP0], mm0           ; (src1 + src2 + 1) / 2 -> dst
  movq [TMP0+TMP1], mm4
%endmacro

%macro AVG2_MMX_RND1 0
  movq mm0, [_EAX]           ; src1 -> mm0
  movq mm1, [_EBX]           ; src2 -> mm1

  movq mm4, [_EAX+TMP1]
  movq mm5, [_EBX+TMP1]

  movq mm2, mm0             ; src1 -> mm2
  movq mm3, mm1             ; src2 -> mm3

  pand mm2, mm7             ; isolate the lsb
  pand mm3, mm7             ; isolate the lsb

  pand mm2, mm3             ; ODD(src1) AND ODD(src2) -> mm2

  movq mm3, mm4
  movq mm6, mm5

  pand mm3, mm7
  pand mm6, mm7

  pand mm3, mm6

  pand mm0, [mmx_mask]
  pand mm1, [mmx_mask]
  pand mm4, [mmx_mask]
  pand mm5, [mmx_mask]

  psrlq mm0, 1              ; src1 / 2
  psrlq mm1, 1              ; src2 / 2

  psrlq mm4, 1
  psrlq mm5, 1

  paddb mm0, mm1            ; src1/2 + src2/2 -> mm0
  paddb mm0, mm2            ; correct rounding error

  paddb mm4, mm5
  paddb mm4, mm3

  lea _EAX, [_EAX+2*TMP1]
  lea _EBX, [_EBX+2*TMP1]

  movq [TMP0], mm0           ; (src1 + src2 + 1) / 2 -> dst
  movq [TMP0+TMP1], mm4
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_avg2_mmx:

  mov eax, prm5d   ; rounding
  test _EAX, _EAX

  jnz near .rounding1

  mov eax, prm6d   ; height -> _EAX
  sub _EAX, 8
  test _EAX, _EAX

  mov TMP0, prm1   ; dst -> edi
  mov _EAX, prm2   ; src1 -> esi
  mov TMP1, prm4   ; stride -> TMP1

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [esp + 4 + 12]   ; src2 -> eax
%endif

  movq mm7, [mmx_one]

  jz near .start0

  AVG2_MMX_RND0
  lea TMP0, [TMP0+2*TMP1]

.start0:

  AVG2_MMX_RND0
  lea TMP0, [TMP0+2*TMP1]
  AVG2_MMX_RND0
  lea TMP0, [TMP0+2*TMP1]
  AVG2_MMX_RND0
  lea TMP0, [TMP0+2*TMP1]
  AVG2_MMX_RND0

  pop _EBX
  ret

.rounding1:
  mov eax, prm6d        ; height -> _EAX
  sub _EAX, 8
  test _EAX, _EAX

  mov TMP0, prm1        ; dst -> edi
  mov _EAX, prm2        ; src1 -> esi
  mov TMP1, prm4        ; stride -> TMP1

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [esp + 4 + 12]   ; src2 -> eax
%endif

  movq mm7, [mmx_one]

  jz near .start1

  AVG2_MMX_RND1
  lea TMP0, [TMP0+2*TMP1]

.start1:

  AVG2_MMX_RND1
  lea TMP0, [TMP0+2*TMP1]
  AVG2_MMX_RND1
  lea TMP0, [TMP0+2*TMP1]
  AVG2_MMX_RND1
  lea TMP0, [TMP0+2*TMP1]
  AVG2_MMX_RND1

  pop _EBX
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void interpolate8x8_avg4_mmx(uint8_t const *dst,
;                              const uint8_t * const src1,
;                              const uint8_t * const src2,
;                              const uint8_t * const src3,
;                              const uint8_t * const src4,
;                              const uint32_t stride,
;                              const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro AVG4_MMX_RND0 0
  movq mm0, [_EAX]           ; src1 -> mm0
  movq mm1, [_EBX]           ; src2 -> mm1

  movq mm2, mm0
  movq mm3, mm1

  pand mm2, [mmx_three]
  pand mm3, [mmx_three]

  pand mm0, [mmx_mask2]
  pand mm1, [mmx_mask2]

  psrlq mm0, 2
  psrlq mm1, 2

  lea _EAX, [_EAX+TMP1]
  lea _EBX, [_EBX+TMP1]

  paddb mm0, mm1
  paddb mm2, mm3

  movq mm4, [_ESI]           ; src3 -> mm0
  movq mm5, [_EDI]           ; src4 -> mm1

  movq mm1, mm4
  movq mm3, mm5

  pand mm1, [mmx_three]
  pand mm3, [mmx_three]

  pand mm4, [mmx_mask2]
  pand mm5, [mmx_mask2]

  psrlq mm4, 2
  psrlq mm5, 2

  paddb mm4, mm5
  paddb mm0, mm4

  paddb mm1, mm3
  paddb mm2, mm1

  paddb mm2, [mmx_two]
  pand mm2, [mmx_mask2]

  psrlq mm2, 2
  paddb mm0, mm2

  lea _ESI, [_ESI+TMP1]
  lea _EDI, [_EDI+TMP1]

  movq [TMP0], mm0           ; (src1 + src2 + src3 + src4 + 2) / 4 -> dst
%endmacro

%macro AVG4_MMX_RND1 0
  movq mm0, [_EAX]           ; src1 -> mm0
  movq mm1, [_EBX]           ; src2 -> mm1

  movq mm2, mm0
  movq mm3, mm1

  pand mm2, [mmx_three]
  pand mm3, [mmx_three]

  pand mm0, [mmx_mask2]
  pand mm1, [mmx_mask2]

  psrlq mm0, 2
  psrlq mm1, 2

  lea _EAX,[_EAX+TMP1]
  lea _EBX,[_EBX+TMP1]

  paddb mm0, mm1
  paddb mm2, mm3

  movq mm4, [_ESI]           ; src3 -> mm0
  movq mm5, [_EDI]           ; src4 -> mm1

  movq mm1, mm4
  movq mm3, mm5

  pand mm1, [mmx_three]
  pand mm3, [mmx_three]

  pand mm4, [mmx_mask2]
  pand mm5, [mmx_mask2]

  psrlq mm4, 2
  psrlq mm5, 2

  paddb mm4, mm5
  paddb mm0, mm4

  paddb mm1, mm3
  paddb mm2, mm1

  paddb mm2, [mmx_one]
  pand mm2, [mmx_mask2]

  psrlq mm2, 2
  paddb mm0, mm2

  lea _ESI,[_ESI+TMP1]
  lea _EDI,[_EDI+TMP1]

  movq [TMP0], mm0           ; (src1 + src2 + src3 + src4 + 2) / 4 -> dst
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_avg4_mmx:

  mov eax, prm7d      ; rounding
  test _EAX, _EAX

  mov TMP0, prm1      ; dst -> edi
  mov _EAX, prm5      ; src4 -> edi
  mov TMP1d, prm6d    ; stride -> TMP1


  push _EBX
  push _EDI
  push _ESI

  mov _EDI, _EAX

%ifdef ARCH_IS_X86_64
  mov _EAX, prm2
  mov _EBX, prm3
  mov _ESI, prm4
%else
  mov _EAX, [esp + 12 +  8]      ; src1 -> esi
  mov _EBX, [esp + 12 + 12]      ; src2 -> _EAX
  mov _ESI, [esp + 12 + 16]      ; src3 -> esi
%endif

  movq mm7, [mmx_one]

  jnz near .rounding1

  AVG4_MMX_RND0
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND0
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND0
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND0
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND0
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND0
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND0
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND0

  pop _ESI
  pop _EDI
  pop _EBX
  ret

.rounding1:
  AVG4_MMX_RND1
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND1
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND1
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND1
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND1
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND1
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND1
  lea TMP0, [TMP0+TMP1]
  AVG4_MMX_RND1

  pop _ESI
  pop _EDI
  pop _EBX
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; void interpolate8x8_6tap_lowpass_h_mmx(uint8_t const *dst,
;                                        const uint8_t * const src,
;                                        const uint32_t stride,
;                                        const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro LOWPASS_6TAP_H_MMX 0
  movq mm0, [_EAX]
  movq mm2, [_EAX+1]

  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7

  punpckhbw mm1, mm7
  punpckhbw mm3, mm7

  paddw mm0, mm2
  paddw mm1, mm3

  psllw mm0, 2
  psllw mm1, 2

  movq mm2, [_EAX-1]
  movq mm4, [_EAX+2]

  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm7
  punpcklbw mm4, mm7

  punpckhbw mm3, mm7
  punpckhbw mm5, mm7

  paddw mm2, mm4
  paddw mm3, mm5

  psubsw mm0, mm2
  psubsw mm1, mm3

  pmullw mm0, [mmx_five]
  pmullw mm1, [mmx_five]

  movq mm2, [_EAX-2]
  movq mm4, [_EAX+3]

  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm7
  punpcklbw mm4, mm7

  punpckhbw mm3, mm7
  punpckhbw mm5, mm7

  paddw mm2, mm4
  paddw mm3, mm5

  paddsw mm0, mm2
  paddsw mm1, mm3

  paddsw mm0, mm6
  paddsw mm1, mm6

  psraw mm0, 5
  psraw mm1, 5

  lea _EAX, [_EAX+TMP1]
  packuswb mm0, mm1
  movq [TMP0], mm0
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_6tap_lowpass_h_mmx:

  mov _EAX, prm4           ; rounding

  lea TMP0, [rounding_lowpass_mmx]
  movq mm6, [TMP0 + _EAX * 8]

  mov TMP0, prm1           ; dst -> edi
  mov _EAX, prm2           ; src -> esi
  mov TMP1, prm3           ; stride -> edx

  pxor mm7, mm7

  LOWPASS_6TAP_H_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_H_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_H_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_H_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_H_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_H_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_H_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_H_MMX

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x8_6tap_lowpass_v_mmx(uint8_t const *dst,
;                                        const uint8_t * const src,
;                                        const uint32_t stride,
;                                        const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro LOWPASS_6TAP_V_MMX 0
  movq mm0, [_EAX]
  movq mm2, [_EAX+TMP1]

  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm7
  punpcklbw mm2, mm7

  punpckhbw mm1, mm7
  punpckhbw mm3, mm7

  paddw mm0, mm2
  paddw mm1, mm3

  psllw mm0, 2
  psllw mm1, 2

  movq mm4, [_EAX+2*TMP1]
  sub _EAX, _EBX
  movq mm2, [_EAX+2*TMP1]

  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm7
  punpcklbw mm4, mm7

  punpckhbw mm3, mm7
  punpckhbw mm5, mm7

  paddw mm2, mm4
  paddw mm3, mm5

  psubsw mm0, mm2
  psubsw mm1, mm3

  pmullw mm0, [mmx_five]
  pmullw mm1, [mmx_five]

  movq mm2, [_EAX+TMP1]
  movq mm4, [_EAX+2*_EBX]

  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm7
  punpcklbw mm4, mm7

  punpckhbw mm3, mm7
  punpckhbw mm5, mm7

  paddw mm2, mm4
  paddw mm3, mm5

  paddsw mm0, mm2
  paddsw mm1, mm3

  paddsw mm0, mm6
  paddsw mm1, mm6

  psraw mm0, 5
  psraw mm1, 5

  lea _EAX, [_EAX+4*TMP1]
  packuswb mm0, mm1
  movq [TMP0], mm0
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_6tap_lowpass_v_mmx:

  mov _EAX, prm4           ; rounding

  lea TMP0, [rounding_lowpass_mmx]
  movq mm6, [TMP0 + _EAX * 8]

  mov TMP0, prm1           ; dst -> edi
  mov _EAX, prm2           ; src -> esi
  mov TMP1, prm3           ; stride -> edx

  push _EBX

  mov _EBX, TMP1
  shl _EBX, 1
  add _EBX, TMP1

  pxor mm7, mm7

  LOWPASS_6TAP_V_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_V_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_V_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_V_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_V_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_V_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_V_MMX
  lea TMP0, [TMP0+TMP1]
  LOWPASS_6TAP_V_MMX

  pop _EBX
  ret
ENDFUNC

;===========================================================================
;
; The next functions combine both source halfpel interpolation step and the
; averaging (with rouding) step to avoid wasting memory bandwidth computing
; intermediate halfpel images and then averaging them.
;
;===========================================================================

%macro PROLOG0 0
  mov TMP0, prm1 ; Dst
  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; BpS
%endmacro

%macro PROLOG 2   ; %1: Rounder, %2 load Dst-Rounder
  pxor mm6, mm6
  movq mm7, [%1]    ; TODO: dangerous! (eax isn't checked)
%if %2
  movq mm5, [rounding1_mmx]
%endif

  PROLOG0
%endmacro

  ; performs: mm0 == (mm0+mm2)  mm1 == (mm1+mm3)
%macro MIX 0
  punpcklbw mm0, mm6
  punpcklbw mm2, mm6
  punpckhbw mm1, mm6
  punpckhbw mm3, mm6
  paddusw mm0, mm2
  paddusw mm1, mm3
%endmacro

%macro MIX_DST 0
  movq mm3, mm2
  paddusw mm0, mm7  ; rounder
  paddusw mm1, mm7  ; rounder
  punpcklbw mm2, mm6
  punpckhbw mm3, mm6
  psrlw mm0, 1
  psrlw mm1, 1

  paddusw mm0, mm2  ; mix Src(mm0/mm1) with Dst(mm2/mm3)
  paddusw mm1, mm3
  paddusw mm0, mm5
  paddusw mm1, mm5
  psrlw mm0, 1
  psrlw mm1, 1

  packuswb mm0, mm1
%endmacro

%macro MIX2 0
  punpcklbw mm0, mm6
  punpcklbw mm2, mm6
  paddusw mm0, mm2
  paddusw mm0, mm7
  punpckhbw mm1, mm6
  punpckhbw mm3, mm6
  paddusw mm1, mm7
  paddusw mm1, mm3
  psrlw mm0, 1
  psrlw mm1, 1

  packuswb mm0, mm1
%endmacro

;===========================================================================
;
; void interpolate8x8_halfpel_add_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================

%macro ADD_FF_MMX 1
  movq mm0, [_EAX]
  movq mm2, [TMP0]
  movq mm1, mm0
  movq mm3, mm2
%if (%1!=0)
  lea _EAX,[_EAX+%1*TMP1]
%endif
  MIX
  paddusw mm0, mm5  ; rounder
  paddusw mm1, mm5  ; rounder
  psrlw mm0, 1
  psrlw mm1, 1

  packuswb mm0, mm1
  movq [TMP0], mm0
%if (%1!=0)
  lea TMP0,[TMP0+%1*TMP1]
%endif
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_add_mmx:
  PROLOG rounding1_mmx, 1
  ADD_FF_MMX 1
  ADD_FF_MMX 1
  ADD_FF_MMX 1
  ADD_FF_MMX 1
  ADD_FF_MMX 1
  ADD_FF_MMX 1
  ADD_FF_MMX 1
  ADD_FF_MMX 0
  ret
ENDFUNC

;===========================================================================
;
; void interpolate8x8_halfpel_h_add_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================

%macro ADD_FH_MMX 0
  movq mm0, [_EAX]
  movq mm2, [_EAX+1]
  movq mm1, mm0
  movq mm3, mm2

  lea _EAX,[_EAX+TMP1]

  MIX
  movq mm2, [TMP0]   ; prepare mix with Dst[0]
  MIX_DST
  movq [TMP0], mm0
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_h_add_mmx:
  PROLOG rounding1_mmx, 1

  ADD_FH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_FH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_FH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_FH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_FH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_FH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_FH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_FH_MMX
  ret
ENDFUNC

;===========================================================================
;
; void interpolate8x8_halfpel_v_add_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================

%macro ADD_HF_MMX 0
  movq mm0, [_EAX]
  movq mm2, [_EAX+TMP1]
  movq mm1, mm0
  movq mm3, mm2

  lea _EAX,[_EAX+TMP1]

  MIX
  movq mm2, [TMP0]   ; prepare mix with Dst[0]
  MIX_DST
  movq [TMP0], mm0

%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_v_add_mmx:
  PROLOG rounding1_mmx, 1

  ADD_HF_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HF_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HF_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HF_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HF_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HF_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HF_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HF_MMX
  ret
ENDFUNC

; The trick is to correct the result of 'pavgb' with some combination of the
; lsb's of the 4 input values i,j,k,l, and their intermediate 'pavgb' (s and t).
; The boolean relations are:
;   (i+j+k+l+3)/4 = (s+t+1)/2 - (ij&kl)&st
;   (i+j+k+l+2)/4 = (s+t+1)/2 - (ij|kl)&st
;   (i+j+k+l+1)/4 = (s+t+1)/2 - (ij&kl)|st
;   (i+j+k+l+0)/4 = (s+t+1)/2 - (ij|kl)|st
; with  s=(i+j+1)/2, t=(k+l+1)/2, ij = i^j, kl = k^l, st = s^t.

; Moreover, we process 2 lines at a times, for better overlapping (~15% faster).

;===========================================================================
;
; void interpolate8x8_halfpel_hv_add_mmx(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;===========================================================================

%macro ADD_HH_MMX 0
  lea _EAX,[_EAX+TMP1]

    ; transfert prev line to mm0/mm1
  movq mm0, mm2
  movq mm1, mm3

    ; load new line in mm2/mm3
  movq mm2, [_EAX]
  movq mm4, [_EAX+1]
  movq mm3, mm2
  movq mm5, mm4

  punpcklbw mm2, mm6
  punpcklbw mm4, mm6
  paddusw mm2, mm4
  punpckhbw mm3, mm6
  punpckhbw mm5, mm6
  paddusw mm3, mm5

    ; mix current line (mm2/mm3) with previous (mm0,mm1); 
    ; we'll preserve mm2/mm3 for next line...

  paddusw mm0, mm2  
  paddusw mm1, mm3  

  movq mm4, [TMP0]   ; prepare mix with Dst[0]
  movq mm5, mm4

  paddusw mm0, mm7  ; finish mixing current line
  paddusw mm1, mm7

  punpcklbw mm4, mm6
  punpckhbw mm5, mm6

  psrlw mm0, 2
  psrlw mm1, 2

  paddusw mm0, mm4  ; mix Src(mm0/mm1) with Dst(mm2/mm3)
  paddusw mm1, mm5

  paddusw mm0, [rounding1_mmx]
  paddusw mm1, [rounding1_mmx]

  psrlw mm0, 1
  psrlw mm1, 1

  packuswb mm0, mm1

  movq [TMP0], mm0
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_hv_add_mmx:
  PROLOG rounding2_mmx, 0    ; mm5 is busy. Don't load dst-rounder

    ; preprocess first line
  movq mm0, [_EAX]
  movq mm2, [_EAX+1]
  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0, mm6
  punpcklbw mm2, mm6
  punpckhbw mm1, mm6
  punpckhbw mm3, mm6
  paddusw mm2, mm0
  paddusw mm3, mm1

   ; Input: mm2/mm3 contains the value (Src[0]+Src[1]) of previous line

  ADD_HH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HH_MMX
  lea TMP0,[TMP0+TMP1]
  ADD_HH_MMX

  ret
ENDFUNC

NON_EXEC_STACK
