;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - 3dne Quantization/Dequantization -
; *
; *  Copyright (C) 2002-2003 Peter Ross <pross@xvid.org>
; *                2002      Jaan Kalda
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
; * $Id: quantize_mpeg_xmm.asm,v 1.10.2.3 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

; _3dne functions are compatible with iSSE, but are optimized specifically
; for K7 pipelines

%define SATURATE

%include "nasm.inc"

;=============================================================================
; Local data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
mmzero:
	dd 0,0
mmx_one:
	times 4 dw 1

;-----------------------------------------------------------------------------
; divide by 2Q table
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_divs:		;i>2
%assign i 1
%rep 31
	times 4 dw  ((1 << 15) / i + 1)
	%assign i i+1
%endrep

ALIGN SECTION_ALIGN
mmx_div:		;quant>2
	times 4 dw 65535 ; the div by 2 formula will overflow for the case
	                 ; quant=1 but we don't care much because quant=1
	                 ; is handled by a different piece of code that
	                 ; doesn't use this table.
%assign quant 2
%rep 31
	times 4 dw  ((1 << 16) / quant + 1)
	%assign quant quant+1
%endrep

%macro FIXX 1
dw (1 << 16) / (%1) + 1
%endmacro

%ifndef ARCH_IS_X86_64
%define nop4	db	08Dh, 074h, 026h,0
%define nop7    db      08dh, 02ch, 02dh,0,0,0,0
%else
%define nop4 
%define nop7
%endif
%define nop3	add	_ESP, byte 0
%define nop2	mov	_ESP, _ESP 
%define nop6	add	_EBP, dword 0

;-----------------------------------------------------------------------------
; quantd table
;-----------------------------------------------------------------------------

%define VM18P	3
%define VM18Q	4

ALIGN SECTION_ALIGN
quantd:
%assign i 1
%rep 31
	times 4 dw  (((VM18P*i) + (VM18Q/2)) / VM18Q)
	%assign i i+1
%endrep

;-----------------------------------------------------------------------------
; multiple by 2Q table
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_mul_quant:
%assign i 1
%rep 31
	times 4 dw  i
	%assign i i+1
%endrep

;-----------------------------------------------------------------------------
; saturation limits
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_32767_minus_2047:
	times 4 dw (32767-2047)
mmx_32768_minus_2048:
	times 4 dw (32768-2048)
mmx_2047:
	times 4 dw 2047
mmx_minus_2048:
	times 4 dw (-2048)
zero:
	times 4 dw 0

int_div:
dd 0
%assign i 1
%rep 255
	dd  (1 << 17) / ( i) + 1
	%assign i i+1
%endrep

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal quant_mpeg_inter_xmm
cglobal dequant_mpeg_intra_3dne
cglobal dequant_mpeg_inter_3dne

;-----------------------------------------------------------------------------
;
; uint32_t quant_mpeg_inter_xmm(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
quant_mpeg_inter_xmm:
  mov _EAX, prm2       ; data
  mov TMP0, prm3       ; quant
  mov TMP1, prm1       ; coeff
  push _ESI
  push _EDI
  push _EBX
  nop
%ifdef ARCH_IS_X86_64
  mov _EDI, prm4
%else
  mov _EDI, [_ESP + 12 + 16]
%endif

  mov _ESI, -14
  mov _EBX, _ESP
  sub _ESP, byte 24
  lea _EBX, [_ESP+8]
  and _EBX, byte -8 ;ALIGN 8
  pxor mm0, mm0
  pxor mm3, mm3
  movq [byte _EBX],mm0
  movq [_EBX+8],mm0

  cmp TMP0, byte 1
  je near .q1loop
  cmp TMP0, byte 19
  jg near .lloop
  nop

ALIGN SECTION_ALIGN
.loop:
  movq mm1, [_EAX + 8*_ESI+112]       ; mm0 = [1st]
  psubw mm0, mm1 ;-mm1
  movq mm4, [_EAX + 8*_ESI + 120] ;
  psubw mm3, mm4 ;-mm4
  pmaxsw mm0, mm1 ;|src|
  pmaxsw mm3, mm4
  nop2
  psraw mm1, 15         ;sign src
  psraw mm4, 15
  psllw mm0, 4          ; level << 4
  psllw mm3, 4          ;
  paddw mm0, [_EDI + 640 + 8*_ESI+112]
  paddw mm3, [_EDI + 640 + 8*_ESI+120]
  movq mm5, [_EDI + 896 + 8*_ESI+112]
  movq mm7, [_EDI + 896 + 8*_ESI+120]
  pmulhuw mm5, mm0
  pmulhuw mm7, mm3
  mov _ESP, _ESP
  movq mm2, [_EDI + 512 + 8*_ESI+112]
  movq mm6, [_EDI + 512 + 8*_ESI+120]
  pmullw mm2, mm5
  pmullw mm6, mm7
  psubw mm0, mm2
  psubw mm3, mm6
  movq mm2, [byte _EBX]
%ifdef ARCH_IS_X86_64
  lea r9, [mmx_divs]
  movq mm6, [r9 + TMP0 * 8 - 8]
%else
  movq mm6, [mmx_divs + TMP0 * 8 - 8]
%endif
  pmulhuw mm0, [_EDI + 768 + 8*_ESI+112]
  pmulhuw mm3, [_EDI + 768 + 8*_ESI+120]
  paddw mm2, [_EBX+8]    ;sum
  paddw mm5, mm0
  paddw mm7, mm3
  pxor mm0, mm0
  pxor mm3, mm3
  pmulhuw mm5, mm6      ; mm0 = (mm0 / 2Q) >> 16
  pmulhuw mm7, mm6      ;  (level ) / quant (0<quant<32)
  add _ESI, byte 2
  paddw mm2, mm5        ;sum += x1
  movq [_EBX], mm7       ;store x2
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
  movq [_EBX+8],mm2 ;store sum
  movq [TMP1 + 8*_ESI+112-16], mm5
  movq [TMP1 + 8*_ESI +120-16], mm7
  jng near .loop

.done:
; calculate  data[0] // (int32_t)dcscalar)
  paddw mm2, [_EBX]
  mov _EBX, [_ESP+24]
  mov _EDI, [_ESP+PTR_SIZE+24]
  mov _ESI, [_ESP+2*PTR_SIZE+24]
  add _ESP, byte 3*PTR_SIZE+24
  pmaddwd mm2, [mmx_one]
  punpckldq mm0, mm2 ;get low dw to mm0:high
  paddd mm0,mm2
  punpckhdq mm0, mm0 ;get result to low
  movd eax, mm0

  ret

ALIGN SECTION_ALIGN
.q1loop:
  movq mm1, [_EAX + 8*_ESI+112]       ; mm0 = [1st]
  psubw mm0, mm1                    ;-mm1
  movq mm4, [_EAX + 8*_ESI+120]
  psubw mm3, mm4                    ;-mm4
  pmaxsw mm0, mm1                   ;|src|
  pmaxsw mm3, mm4
  nop2
  psraw mm1, 15                             ; sign src
  psraw mm4, 15
  psllw mm0, 4                              ; level << 4
  psllw mm3, 4
  paddw mm0, [_EDI + 640 + 8*_ESI+112]    ;mm0 is to be divided
  paddw mm3, [_EDI + 640 + 8*_ESI+120]    ; inter1 contains fix for division by 1
  movq mm5, [_EDI + 896 + 8*_ESI+112] ;with rounding down
  movq mm7, [_EDI + 896 + 8*_ESI+120]
  pmulhuw mm5, mm0
  pmulhuw mm7, mm3                          ;mm7: first approx of division
  mov _ESP, _ESP
  movq mm2, [_EDI + 512 + 8*_ESI+112]
  movq mm6, [_EDI + 512 + 8*_ESI+120]      ; divs for q<=16
  pmullw mm2, mm5                           ;test value <= original
  pmullw mm6, mm7
  psubw mm0, mm2                            ;mismatch
  psubw mm3, mm6
  movq mm2, [byte _EBX]
  pmulhuw mm0, [_EDI + 768 + 8*_ESI+112]  ;correction
  pmulhuw mm3, [_EDI + 768 + 8*_ESI+120]
  paddw mm2, [_EBX+8]    ;sum
  paddw mm5, mm0        ;final result
  paddw mm7, mm3
  pxor mm0, mm0
  pxor mm3, mm3
  psrlw mm5, 1          ;  (level ) /2  (quant = 1)
  psrlw mm7, 1
  add _ESI, byte 2
  paddw mm2, mm5        ;sum += x1
  movq [_EBX], mm7      ;store x2
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
  movq [_EBX+8], mm2    ;store sum
  movq [TMP1 + 8*_ESI+112-16], mm5
  movq [TMP1 + 8*_ESI +120-16], mm7
  jng near .q1loop
  jmp near .done

ALIGN SECTION_ALIGN
.lloop:
  movq mm1, [_EAX + 8*_ESI+112]       ; mm0 = [1st]
  psubw mm0,mm1         ;-mm1
  movq mm4, [_EAX + 8*_ESI+120]
  psubw mm3,mm4         ;-mm4
  pmaxsw mm0,mm1        ;|src|
  pmaxsw mm3,mm4
  nop2
  psraw mm1,15          ;sign src
  psraw mm4,15
  psllw mm0, 4          ; level << 4
  psllw mm3, 4          ;
  paddw mm0, [_EDI + 640 + 8*_ESI+112] ;mm0 is to be divided inter1 contains fix for division by 1
  paddw mm3, [_EDI + 640 + 8*_ESI+120]
  movq mm5,[_EDI + 896 + 8*_ESI+112]
  movq mm7,[_EDI + 896 + 8*_ESI+120]
  pmulhuw mm5,mm0
  pmulhuw mm7,mm3       ;mm7: first approx of division
  mov _ESP,_ESP
  movq mm2,[_EDI + 512 + 8*_ESI+112]
  movq mm6,[_EDI + 512 + 8*_ESI+120]
  pmullw mm2,mm5        ;test value <= original
  pmullw mm6,mm7
  psubw mm0,mm2         ;mismatch
  psubw mm3,mm6
  movq mm2,[byte _EBX]
%ifdef ARCH_IS_X86_64
  lea r9, [mmx_div]
  movq mm6, [r9 + TMP0 * 8 - 8]
%else
  movq mm6,[mmx_div + TMP0 * 8 - 8]  ; divs for q<=16
%endif
  pmulhuw mm0,[_EDI + 768 + 8*_ESI+112] ;correction
  pmulhuw mm3,[_EDI + 768 + 8*_ESI+120]
  paddw mm2,[_EBX+8]    ;sum
  paddw mm5,mm0         ;final result
  paddw mm7,mm3
  pxor mm0,mm0
  pxor mm3,mm3
  pmulhuw mm5, mm6      ; mm0 = (mm0 / 2Q) >> 16
  pmulhuw mm7, mm6      ;  (level ) / quant (0<quant<32)
  add _ESI,byte 2
  psrlw mm5, 1          ; (level ) / (2*quant)
  paddw mm2,mm5         ;sum += x1
  psrlw mm7, 1
  movq [_EBX],mm7       ;store x2
  pxor mm5, mm1         ; mm0 *= sign(mm0)
  pxor mm7, mm4         ;
  psubw mm5, mm1        ; undisplace
  psubw mm7, mm4        ;
  movq [_EBX+8], mm2    ;store sum
  movq [TMP1 + 8*_ESI+112-16], mm5
  movq [TMP1 + 8*_ESI +120-16], mm7
  jng near .lloop
  jmp near .done
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t dequant_mpeg_intra_3dne(int16_t *data,
;                                  const int16_t const *coeff,
;                                  const uint32_t quant,
;                                  const uint32_t dcscalar,
;                                  const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

  ;   Note: in order to saturate 'easily', we pre-shift the quantifier
  ; by 4. Then, the high-word of (coeff[]*matrix[i]*quant) are used to
  ; build a saturating mask. It is non-zero only when an overflow occured.
  ; We thus avoid packing/unpacking toward double-word.
  ; Moreover, we perform the mult (matrix[i]*quant) first, instead of, e.g.,
  ; (coeff[i]*matrix[i]). This is less prone to overflow if coeff[] are not
  ; checked. Input ranges are: coeff in [-127,127], inter_matrix in [1..255],a
  ; and quant in [1..31].
  ;

%macro DEQUANT4INTRAMMX 1
  movq mm1, [byte TMP0+ 16 * %1] ; mm0 = c  = coeff[i]
  movq mm4, [TMP0+ 16 * %1 +8]   ; mm3 = c' = coeff[i+1]
  psubw mm0, mm1
  psubw mm3, mm4
  pmaxsw mm0, mm1
  pmaxsw mm3, mm4
  psraw mm1, 15
  psraw mm4, 15
%if %1
  movq mm2, [_EAX+8] ;preshifted quant
  movq mm7, [_EAX+8]
%endif
  pmullw mm2, [TMP1 + 16 * %1 ]     ; matrix[i]*quant
  pmullw mm7, [TMP1 + 16 * %1 +8]   ; matrix[i+1]*quant
  movq mm5, mm0
  movq mm6, mm3
  pmulhw mm0, mm2   ; high of coeff*(matrix*quant)
  pmulhw mm3, mm7   ; high of coeff*(matrix*quant)
  pmullw mm2, mm5   ; low  of coeff*(matrix*quant)
  pmullw mm7, mm6   ; low  of coeff*(matrix*quant)
  pcmpgtw mm0, [_EAX]
  pcmpgtw mm3, [_EAX]
  paddusw mm2, mm0
  paddusw mm7, mm3
  psrlw mm2, 5
  psrlw mm7, 5
  pxor mm2, mm1     ; start negating back
  pxor mm7, mm4     ; start negating back
  psubusw mm1, mm0
  psubusw mm4, mm3
  movq mm0, [_EAX]   ;zero
  movq mm3, [_EAX]   ;zero
  psubw mm2, mm1    ; finish negating back
  psubw mm7, mm4    ; finish negating back
  movq [byte _EDI + 16 * %1], mm2   ; data[i]
  movq [_EDI + 16 * %1  +8], mm7   ; data[i+1]
%endmacro

ALIGN SECTION_ALIGN
dequant_mpeg_intra_3dne:
  mov _EAX, prm3  ; quant
%ifdef ARCH_IS_X86_64
  lea TMP0, [mmx_mul_quant]
  movq mm7, [TMP0 + _EAX*8 - 8]
%else
  movq mm7, [mmx_mul_quant  + _EAX*8 - 8]
%endif
  mov TMP0, prm2  ; coeff
  psllw mm7, 2    ; << 2. See comment.
  mov TMP1, prm5 ; mpeg_quant_matrices	
  push _EBX
  movsx _EBX, word [TMP0]
  pxor mm0, mm0
  pxor mm3, mm3
  push _ESI
  lea _EAX, [_ESP-28]
  sub _ESP, byte 32
  and _EAX, byte -8  ;points to qword ALIGNed space on stack
  movq [_EAX], mm0
  movq [_EAX+8], mm7
%ifdef ARCH_IS_X86_64
  imul _EBX, prm4    ; dcscalar
%else
  imul _EBX, [_ESP+16+8+32]    ; dcscalar
%endif
  movq mm2, mm7
  push _EDI

%ifdef ARCH_IS_X86_64
  mov _EDI, prm1  ; data
%else
  mov _EDI, [_ESP+4+12+32]
%endif

ALIGN SECTION_ALIGN

  DEQUANT4INTRAMMX 0

  mov _ESI, -2048
  nop
  cmp _EBX, _ESI

  DEQUANT4INTRAMMX 1

  cmovl _EBX, _ESI
  neg _ESI
  sub _ESI, byte 1 ;2047

  DEQUANT4INTRAMMX 2

  cmp _EBX, _ESI
  cmovg _EBX, _ESI
  lea _EBP, [byte _EBP]

  DEQUANT4INTRAMMX 3

  mov _ESI, [_ESP+32+PTR_SIZE]
  mov [byte _EDI], bx
  mov _EBX, [_ESP+32+2*PTR_SIZE]

  DEQUANT4INTRAMMX 4
  DEQUANT4INTRAMMX 5
  DEQUANT4INTRAMMX 6
  DEQUANT4INTRAMMX 7

  pop _EDI

  add _ESP, byte 32+2*PTR_SIZE

  xor _EAX, _EAX
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t dequant_mpeg_inter_3dne(int16_t * data,
;                                  const int16_t * const coeff,
;                                  const uint32_t quant,
;                                  const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

    ; Note:  We use (2*c + sgn(c) - sgn(-c)) as multiplier
    ; so we handle the 3 cases: c<0, c==0, and c>0 in one shot.
    ; sgn(x) is the result of 'pcmpgtw 0,x':  0 if x>=0, -1 if x<0.
    ; It's mixed with the extraction of the absolute value.

ALIGN SECTION_ALIGN
dequant_mpeg_inter_3dne:
  mov _EAX, prm3        ; quant
%ifdef ARCH_IS_X86_64
  lea TMP0, [mmx_mul_quant]
  movq mm7, [TMP0  + _EAX*8 - 8]
%else
  movq mm7, [mmx_mul_quant  + _EAX*8 - 8]
%endif
  mov TMP1, prm1        ; data
  mov TMP0, prm2        ; coeff
  mov _EAX, -14
  paddw mm7, mm7    ; << 1
  pxor mm6, mm6     ; mismatch sum
  push _ESI
  push _EDI
  lea _ESI, [mmzero]
  pxor mm1, mm1
  pxor mm3, mm3
%ifdef ARCH_IS_X86_64
  mov _EDI, prm4
%else
  mov _EDI, [_ESP + 8 + 16] ; mpeg_quant_matrices
%endif
  nop
  nop4

ALIGN SECTION_ALIGN
.loop:
  movq mm0, [TMP0+8*_EAX + 7*16   ]   ; mm0 = coeff[i]
  pcmpgtw mm1, mm0  ; mm1 = sgn(c)    (preserved)
  movq mm2, [TMP0+8*_EAX + 7*16 +8]   ; mm2 = coeff[i+1]
  pcmpgtw mm3, mm2  ; mm3 = sgn(c')   (preserved)
  paddsw mm0, mm1   ; c += sgn(c)
  paddsw mm2, mm3   ; c += sgn(c')
  paddw mm0, mm0    ; c *= 2
  paddw mm2, mm2    ; c'*= 2

  movq mm4, [_ESI]
  movq mm5, [_ESI]
  psubw mm4, mm0    ; -c
  psubw mm5, mm2    ; -c'

  psraw mm4, 16     ; mm4 = sgn(-c)
  psraw mm5, 16     ; mm5 = sgn(-c')
  psubsw mm0, mm4   ; c  -= sgn(-c)
  psubsw mm2, mm5   ; c' -= sgn(-c')
  pxor mm0, mm1     ; finish changing sign if needed
  pxor mm2, mm3     ; finish changing sign if needed

 ; we're short on register, here. Poor pairing...

  movq mm4, mm7     ; (matrix*quant)
  nop
  pmullw mm4, [_EDI + 512 + 8*_EAX + 7*16]
  movq mm5, mm4
  pmulhw mm5, mm0   ; high of c*(matrix*quant)
  pmullw mm0, mm4   ; low  of c*(matrix*quant)

  movq mm4, mm7     ; (matrix*quant)
  pmullw mm4, [_EDI + 512 + 8*_EAX + 7*16 + 8]
  add _EAX, byte 2

  pcmpgtw mm5, [_ESI]
  paddusw mm0, mm5
  psrlw mm0, 5
  pxor mm0, mm1     ; start restoring sign
  psubusw mm1, mm5

  movq mm5, mm4
  pmulhw mm5, mm2   ; high of c*(matrix*quant)
  pmullw mm2, mm4   ; low  of c*(matrix*quant)
  psubw mm0, mm1    ; finish restoring sign

  pcmpgtw mm5, [_ESI]
  paddusw mm2, mm5
  psrlw mm2, 5
  pxor mm2, mm3     ; start restoring sign
  psubusw mm3, mm5
  psubw mm2, mm3    ; finish restoring sign
  movq mm1, [_ESI]
  movq mm3, [byte _ESI]
  pxor mm6, mm0                             ; mismatch control
  movq [TMP1 + 8*_EAX + 7*16 -2*8   ], mm0    ; data[i]
  pxor mm6, mm2                             ; mismatch control
  movq [TMP1 + 8*_EAX + 7*16 -2*8 +8], mm2    ; data[i+1]

  jng .loop
  nop

 ; mismatch control

  pshufw mm0, mm6, 01010101b
  pshufw mm1, mm6, 10101010b
  pshufw mm2, mm6, 11111111b
  pxor mm6, mm0
  pxor mm1, mm2
  pxor mm6, mm1
  movd eax, mm6
  pop _EDI
  and _EAX, byte 1
  xor _EAX, byte 1
  mov _ESI, [_ESP]
  add _ESP, byte PTR_SIZE
  xor word [TMP1 + 2*63], ax

  xor _EAX, _EAX
  ret
ENDFUNC

NON_EXEC_STACK
