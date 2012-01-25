;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - 3dne pipeline optimized  8x8 block-based halfpel interpolation -
; *
; *  Copyright(C) 2002 Jaan Kalda
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

; these 3dne functions are compatible with iSSE, but are optimized specifically
; for K7 pipelines

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
mmx_one:
	times 8 db 1

ALIGN SECTION_ALIGN
mm_minusone:
	dd -1,-1

;=============================================================================
; Macros
;=============================================================================

%macro nop4 0
DB 08Dh,074h,026h,0
%endmacro

;=============================================================================
; Macros
;=============================================================================

TEXT

cglobal interpolate8x8_halfpel_h_3dne
cglobal interpolate8x8_halfpel_v_3dne
cglobal interpolate8x8_halfpel_hv_3dne

cglobal interpolate8x4_halfpel_h_3dne
cglobal interpolate8x4_halfpel_v_3dne
cglobal interpolate8x4_halfpel_hv_3dne

;-----------------------------------------------------------------------------
;
; void interpolate8x8_halfpel_h_3dne(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;-----------------------------------------------------------------------------

%macro COPY_H_SSE_RND0 1
%if (%1)
  movq mm0, [_EAX]
%else
  movq mm0, [_EAX+0]
  ; ---
  ; nasm >0.99.x rejects the original statement:
  ;   movq mm0, [dword _EAX]
  ; as it is ambiguous. for this statement nasm <0.99.x would
  ; generate "movq mm0,[_EAX+0]"
  ; ---
%endif
  pavgb mm0, [_EAX+1]
  movq mm1, [_EAX+TMP1]
  pavgb mm1, [_EAX+TMP1+1]
  lea _EAX, [_EAX+2*TMP1]
  movq [TMP0], mm0
  movq [TMP0+TMP1], mm1
%endmacro

%macro COPY_H_SSE_RND1 0
  movq mm0, [_EAX]
  movq mm1, [_EAX+TMP1]
  movq mm4, mm0
  movq mm5, mm1
  movq mm2, [_EAX+1]
  movq mm3, [_EAX+TMP1+1]
  pavgb mm0, mm2
  pxor mm2, mm4
  pavgb mm1, mm3
  lea _EAX, [_EAX+2*TMP1]
  pxor mm3, mm5
  pand mm2, mm7
  pand mm3, mm7
  psubb mm0, mm2
  movq [TMP0], mm0
  psubb mm1, mm3
  movq [TMP0+TMP1], mm1
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_h_3dne:

  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; stride
  dec PTR_TYPE prm4; rounding

  jz near .rounding1
  mov TMP0, prm1 ; Dst

  COPY_H_SSE_RND0 0
  lea TMP0,[TMP0+2*TMP1]
  COPY_H_SSE_RND0 1
  lea TMP0,[TMP0+2*TMP1]
  COPY_H_SSE_RND0 1
  lea TMP0,[TMP0+2*TMP1]
  COPY_H_SSE_RND0 1
  ret

.rounding1:
 ; we use: (i+j)/2 = ( i+j+1 )/2 - (i^j)&1
  mov TMP0, prm1 ; Dst
  movq mm7, [mmx_one]
  COPY_H_SSE_RND1
  lea TMP0, [TMP0+2*TMP1]
  COPY_H_SSE_RND1
  lea TMP0,[TMP0+2*TMP1]
  COPY_H_SSE_RND1
  lea TMP0,[TMP0+2*TMP1]
  COPY_H_SSE_RND1
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x8_halfpel_v_3dne(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_v_3dne:

  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; stride
  dec PTR_TYPE prm4; rounding

    ; we process 2 line at a time

  jz near .rounding1
  pxor mm2,mm2
  movq mm0, [_EAX]
  movq mm1, [_EAX+TMP1]
  por mm2, [_EAX+2*TMP1]
  mov TMP0, prm1 ; Dst
  lea _EAX, [_EAX+2*TMP1]
  pxor mm4, mm4
  pavgb mm0, mm1
  pavgb mm1, mm2
  movq [byte TMP0], mm0
  movq [TMP0+TMP1], mm1
  pxor mm6, mm6
  add _EAX, TMP1
  lea TMP0, [TMP0+2*TMP1]
  movq mm3, [byte _EAX]
  por mm4, [_EAX+TMP1]
  lea _EAX, [_EAX+2*TMP1]
  pavgb mm2, mm3
  pavgb mm3, mm4
  movq [TMP0], mm2
  movq [TMP0+TMP1], mm3
  lea TMP0, [byte TMP0+2*TMP1]
  movq mm5, [byte _EAX]
  por mm6, [_EAX+TMP1]
  lea _EAX, [_EAX+2*TMP1]
  pavgb mm4, mm5
  pavgb mm5, mm6
  movq [TMP0], mm4
  movq [TMP0+TMP1], mm5
  lea TMP0, [TMP0+2*TMP1]
  movq mm7, [_EAX]
  movq mm0, [_EAX+TMP1]
  pavgb mm6, mm7
  pavgb mm7, mm0
  movq [TMP0], mm6
  movq [TMP0+TMP1], mm7
  ret

ALIGN SECTION_ALIGN
.rounding1:
  pcmpeqb mm0, mm0
  psubusb mm0, [_EAX]
  add _EAX, TMP1
  mov TMP0, prm1 ; Dst
  push _ESI
  pcmpeqb mm1, mm1
  pcmpeqb mm2, mm2
  lea _ESI, [mm_minusone]
  psubusb mm1, [byte _EAX]
  psubusb mm2, [_EAX+TMP1]
  lea _EAX, [_EAX+2*TMP1]
  movq mm6, [_ESI]
  movq mm7, [_ESI]
  pavgb mm0, mm1
  pavgb mm1, mm2
  psubusb mm6, mm0
  psubusb mm7, mm1
  movq [TMP0], mm6
  movq [TMP0+TMP1], mm7
  lea TMP0, [TMP0+2*TMP1]
  pcmpeqb mm3, mm3
  pcmpeqb mm4, mm4
  psubusb mm3, [_EAX]
  psubusb mm4, [_EAX+TMP1]
  lea _EAX, [_EAX+2*TMP1]
  pavgb mm2, mm3
  pavgb mm3, mm4
  movq mm0, [_ESI]
  movq mm1, [_ESI]
  psubusb mm0, mm2
  psubusb mm1, mm3
  movq [TMP0], mm0
  movq [TMP0+TMP1], mm1
  lea TMP0,[TMP0+2*TMP1]

  pcmpeqb mm5, mm5
  pcmpeqb mm6, mm6
  psubusb mm5, [_EAX]
  psubusb mm6, [_EAX+TMP1]
  lea _EAX, [_EAX+2*TMP1]
  pavgb mm4, mm5
  pavgb mm5, mm6
  movq mm2, [_ESI]
  movq mm3, [_ESI]
  psubusb mm2, mm4
  psubusb mm3, mm5
  movq [TMP0], mm2
  movq [TMP0+TMP1], mm3
  lea TMP0, [TMP0+2*TMP1]
  pcmpeqb mm7, mm7
  pcmpeqb mm0, mm0
  psubusb mm7, [_EAX]
  psubusb mm0, [_EAX+TMP1]
  pavgb mm6, mm7
  pavgb mm7, mm0
  movq mm4, [_ESI]
  movq mm5, [_ESI]
  psubusb mm4, mm6
  pop _ESI
  psubusb mm5, mm7
  movq [TMP0], mm4
  movq [TMP0+TMP1], mm5
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x8_halfpel_hv_3dne(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;-----------------------------------------------------------------------------

; The trick is to correct the result of 'pavgb' with some combination of the
; lsb's of the 4 input values i,j,k,l, and their intermediate 'pavgb' (s and t).
; The boolean relations are:
;   (i+j+k+l+3)/4 = (s+t+1)/2 - (ij&kl)&st
;   (i+j+k+l+2)/4 = (s+t+1)/2 - (ij|kl)&st
;   (i+j+k+l+1)/4 = (s+t+1)/2 - (ij&kl)|st
;   (i+j+k+l+0)/4 = (s+t+1)/2 - (ij|kl)|st
; with  s=(i+j+1)/2, t=(k+l+1)/2, ij = i^j, kl = k^l, st = s^t.

; Moreover, we process 2 lines at a times, for better overlapping (~15% faster).

%macro COPY_HV_SSE_RND0 0

  movq mm0, [_EAX+TMP1]
  movq mm1, [_EAX+TMP1+1]

  movq mm6, mm0
  pavgb mm0, mm1        ; mm0=(j+k+1)/2. preserved for next step
  lea _EAX, [_EAX+2*TMP1]
  pxor mm1, mm6         ; mm1=(j^k).     preserved for next step

  por mm3, mm1          ; ij |= jk
  movq mm6, mm2
  pxor mm6, mm0         ; mm6 = s^t
  pand mm3, mm6         ; (ij|jk) &= st
  pavgb mm2, mm0        ; mm2 = (s+t+1)/2
  movq mm6, [_EAX]
  pand mm3, mm7         ; mask lsb
  psubb mm2, mm3        ; apply.

  movq [TMP0], mm2

  movq mm2, [_EAX]
  movq mm3, [_EAX+1]
  pavgb mm2, mm3        ; preserved for next iteration
  pxor mm3, mm6         ; preserved for next iteration

  por mm1, mm3
  movq mm6, mm0
  pxor mm6, mm2
  pand mm1, mm6
  pavgb mm0, mm2

  pand mm1, mm7
  psubb mm0, mm1

  movq [TMP0+TMP1], mm0
%endmacro

%macro COPY_HV_SSE_RND1 0
  movq mm0, [_EAX+TMP1]
  movq mm1, [_EAX+TMP1+1]

  movq mm6, mm0
  pavgb mm0, mm1        ; mm0=(j+k+1)/2. preserved for next step
  lea _EAX,[_EAX+2*TMP1]
  pxor mm1, mm6         ; mm1=(j^k).     preserved for next step

  pand mm3, mm1
  movq mm6, mm2
  pxor mm6, mm0
  por mm3, mm6
  pavgb mm2, mm0
  movq mm6, [_EAX]
  pand mm3, mm7
  psubb mm2, mm3

  movq [TMP0], mm2

  movq mm2, [_EAX]
  movq mm3, [_EAX+1]
  pavgb mm2, mm3        ; preserved for next iteration
  pxor mm3, mm6         ; preserved for next iteration

  pand mm1, mm3
  movq mm6, mm0
  pxor mm6, mm2
  por mm1, mm6
  pavgb mm0, mm2
  pand mm1, mm7
  psubb mm0, mm1
  movq [TMP0+TMP1], mm0
%endmacro

ALIGN SECTION_ALIGN
interpolate8x8_halfpel_hv_3dne:
  mov _EAX, prm2     ; Src
  mov TMP1, prm3     ; stride
  dec PTR_TYPE prm4    ; rounding

    ; loop invariants: mm2=(i+j+1)/2  and  mm3= i^j
  movq mm2, [_EAX]
  movq mm3, [_EAX+1]
  movq mm6, mm2
  pavgb mm2, mm3
  pxor mm3, mm6         ; mm2/mm3 ready
  mov TMP0, prm1     ; Dst
  movq mm7, [mmx_one]

  jz near .rounding1
  lea _EBP,[byte _EBP]
  COPY_HV_SSE_RND0
  lea TMP0,[TMP0+2*TMP1]
  COPY_HV_SSE_RND0
  lea TMP0,[TMP0+2*TMP1]
  COPY_HV_SSE_RND0
  lea TMP0,[TMP0+2*TMP1]
  COPY_HV_SSE_RND0
  ret

ALIGN SECTION_ALIGN
.rounding1:
  COPY_HV_SSE_RND1
  lea TMP0,[TMP0+2*TMP1]
  COPY_HV_SSE_RND1
  lea TMP0,[TMP0+2*TMP1]
  COPY_HV_SSE_RND1
  lea TMP0,[TMP0+2*TMP1]
  COPY_HV_SSE_RND1
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x4_halfpel_h_3dne(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
interpolate8x4_halfpel_h_3dne:

  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; stride
  dec PTR_TYPE prm4; rounding

  jz .rounding1
  mov TMP0, prm1 ; Dst

  COPY_H_SSE_RND0 0
  lea TMP0,[TMP0+2*TMP1]
  COPY_H_SSE_RND0 1
  ret

.rounding1:
 ; we use: (i+j)/2 = ( i+j+1 )/2 - (i^j)&1
  mov TMP0, prm1 ; Dst
  movq mm7, [mmx_one]
  COPY_H_SSE_RND1
  lea TMP0, [TMP0+2*TMP1]
  COPY_H_SSE_RND1
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x4_halfpel_v_3dne(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
interpolate8x4_halfpel_v_3dne:

  mov _EAX, prm2 ; Src
  mov TMP1, prm3 ; stride
  dec PTR_TYPE prm4; rounding

    ; we process 2 line at a time

  jz .rounding1
  pxor mm2,mm2
  movq mm0, [_EAX]
  movq mm1, [_EAX+TMP1]
  por mm2, [_EAX+2*TMP1]      ; Something like preload (pipelining)
  mov TMP0, prm1 ; Dst
  lea _EAX, [_EAX+2*TMP1]
  pxor mm4, mm4
  pavgb mm0, mm1
  pavgb mm1, mm2
  movq [byte TMP0], mm0
  movq [TMP0+TMP1], mm1
  
  pxor mm6, mm6
  add _EAX, TMP1
  lea TMP0, [TMP0+2*TMP1]
  movq mm3, [byte _EAX]
  por mm4, [_EAX+TMP1]
  lea _EAX, [_EAX+2*TMP1]
  pavgb mm2, mm3
  pavgb mm3, mm4
  movq [TMP0], mm2
  movq [TMP0+TMP1], mm3
  
  ret

ALIGN SECTION_ALIGN
.rounding1:
  pcmpeqb mm0, mm0
  psubusb mm0, [_EAX]            ; _EAX==line0
  add _EAX, TMP1                  ; _EAX==line1
  mov TMP0, prm1 ; Dst

  push _ESI

  pcmpeqb mm1, mm1
  pcmpeqb mm2, mm2
  lea _ESI, [mm_minusone]
  psubusb mm1, [byte _EAX]       ; line1
  psubusb mm2, [_EAX+TMP1]        ; line2
  lea _EAX, [_EAX+2*TMP1]          ; _EAX==line3
  movq mm6, [_ESI]
  movq mm7, [_ESI]
  pavgb mm0, mm1
  pavgb mm1, mm2
  psubusb mm6, mm0
  psubusb mm7, mm1
  movq [TMP0], mm6               ; store line0
  movq [TMP0+TMP1], mm7           ; store line1
  
  lea TMP0, [TMP0+2*TMP1]
  pcmpeqb mm3, mm3
  pcmpeqb mm4, mm4
  psubusb mm3, [_EAX]            ; line3
  psubusb mm4, [_EAX+TMP1]        ; line4
  lea _EAX, [_EAX+2*TMP1]          ; _EAX==line 5
  pavgb mm2, mm3
  pavgb mm3, mm4
  movq mm0, [_ESI]
  movq mm1, [_ESI]
  psubusb mm0, mm2
  psubusb mm1, mm3
  movq [TMP0], mm0
  movq [TMP0+TMP1], mm1

  pop _ESI

  ret

ENDFUNC

;-----------------------------------------------------------------------------
;
; void interpolate8x4_halfpel_hv_3dne(uint8_t * const dst,
;                       const uint8_t * const src,
;                       const uint32_t stride,
;                       const uint32_t rounding);
;
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
interpolate8x4_halfpel_hv_3dne:
  mov _EAX, prm2     ; Src
  mov TMP1, prm3     ; stride
  dec PTR_TYPE prm4    ; rounding

    ; loop invariants: mm2=(i+j+1)/2  and  mm3= i^j
  movq mm2, [_EAX]
  movq mm3, [_EAX+1]
  movq mm6, mm2
  pavgb mm2, mm3
  pxor mm3, mm6         ; mm2/mm3 ready
  mov TMP0, prm1     ; Dst
  movq mm7, [mmx_one]

  jz near .rounding1
  lea _EBP,[byte _EBP]
  COPY_HV_SSE_RND0
  lea TMP0,[TMP0+2*TMP1]
  COPY_HV_SSE_RND0
  ret

ALIGN SECTION_ALIGN
.rounding1:
  COPY_HV_SSE_RND1
  lea TMP0,[TMP0+2*TMP1]
  COPY_HV_SSE_RND1
  ret
ENDFUNC

NON_EXEC_STACK
