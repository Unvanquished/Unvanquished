;/**************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - 3dne Quantization/Dequantization -
; *
; *  Copyright(C) 2002-2003 Jaan Kalda
; *
; *  This program is free software ; you can r_EDIstribute it and/or modify
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
; * $Id: quantize_h263_3dne.asm,v 1.9.2.3 2009/09/16 17:11:39 Isibaar Exp $
; *
; *************************************************************************/
;
; these 3dne functions are compatible with iSSE, but are optimized specifically for
; K7 pipelines

; enable dequant saturate [-2048,2047], test purposes only.
%define SATURATE

%include "nasm.inc"

;=============================================================================
; Local data
;=============================================================================

DATA

align SECTION_ALIGN
int_div:
	dd 0
%assign i 1
%rep 255
	dd  (1 << 16) / (i) + 1
	%assign i i+1
%endrep

ALIGN SECTION_ALIGN
plus_one:
	times 8 dw 1

;-----------------------------------------------------------------------------
; subtract by Q/2 table
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_sub:
%assign i 1
%rep 31
	times 4 dw i / 2
	%assign i i+1
%endrep


;-----------------------------------------------------------------------------
;
; divide by 2Q table
;
; use a shift of 16 to take full advantage of _pmulhw_
; for q=1, _pmulhw_ will overflow so it is treated seperately
; (3dnow2 provides _pmulhuw_ which wont cause overflow)
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_div:
%assign i 1
%rep 31
	times 4 dw  (1 << 16) / (i * 2) + 1
	%assign i i+1
%endrep

;-----------------------------------------------------------------------------
; add by (odd(Q) ? Q : Q - 1) table
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_add:
%assign i 1
%rep 31
	%if i % 2 != 0
	times 4 dw i
	%else
	times 4 dw i - 1
	%endif
	%assign i i+1
%endrep

;-----------------------------------------------------------------------------
; multiple by 2Q table
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_mul:
%assign i 1
%rep 31
	times 4 dw i * 2
	%assign i i+1
%endrep

;-----------------------------------------------------------------------------
; saturation limits
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_32768_minus_2048:
	times 4 dw (32768-2048)
mmx_32767_minus_2047:
	times 4 dw (32767-2047)

ALIGN SECTION_ALIGN
mmx_2047:
	times 4 dw 2047

ALIGN SECTION_ALIGN
mmzero:
	dd 0, 0
int2047:
	dd 2047
int_2048:
	dd -2048

;=============================================================================
; Code
;=============================================================================

TEXT


;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_intra_3dne(int16_t * coeff,
;                                const int16_t const * data,
;                                const uint32_t quant,
;                                const uint32_t dcscalar,
;                                const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------
;This is Athlon-optimized code (ca 70 clk per call)

%macro quant_intra1  1
  psubw mm1, mm0    ;A3
  psubw mm3, mm2    ;B3
%if (%1)
  psubw mm5, mm4    ;C8
  psubw mm7, mm6    ;D8
%endif

ALIGN SECTION_ALIGN
  movq   mm4, [_ECX + %1 * 32 +16] ;C1
  pmaxsw mm1, mm0   ;A4
  movq   mm6, [_ECX + %1 * 32 +24] ;D1
  pmaxsw mm3, mm2   ;B4


  psraw mm0, 15     ;A5
  psraw mm2, 15     ;B5
%if (%1)
  movq [_EDX + %1 * 32 + 16-32], mm5 ;C9
  movq [_EDX + %1 * 32 + 24-32], mm7 ;D9
%endif

  psrlw mm1, 1      ;A6
  psrlw mm3, 1      ;B6
  movq mm5, [_EBX]   ;C2
  movq mm7, [_EBX]   ;D2

  pxor mm1, mm0 ;A7
  pxor mm3, mm2 ;B7

  psubw mm5, mm4    ;C3
  psubw mm7, mm6    ;D3
  psubw mm1, mm0    ;A8
  psubw mm3, mm2    ;B8

%if (%1 == 0)
  push _EBP
  movq mm0, [_ECX + %1 * 32 +32]
%elif (%1 < 3)
  movq   mm0, [_ECX + %1 * 32 +32]    ;A1
%endif
  pmaxsw mm5, mm4   ;C4
%if (%1 < 3)
  movq   mm2, [_ECX + %1 * 32 +8+32]  ;B1
%else
  cmp _ESP, _ESP
%endif
  pmaxsw mm7, mm6   ;D4

  psraw mm4, 15     ;C5
  psraw mm6, 15     ;D5
  movq [byte _EDX + %1 * 32], mm1    ;A9
  movq [_EDX + %1 * 32+8], mm3       ;B9


  psrlw mm5, 1      ;C6
  psrlw mm7, 1      ;D6
%if (%1 < 3)
  movq mm1, [_EBX]   ;A2
  movq mm3, [_EBX]   ;B2
%endif
%if (%1 == 3)
%ifdef ARCH_IS_X86_64
  lea r9, [int_div]
  imul eax, dword [r9+4*_EDI]
%else
  imul _EAX, [int_div+4*_EDI]
%endif
%endif
  pxor mm5, mm4 ;C7
  pxor mm7, mm6 ;D7
%endm


%macro quant_intra  1
    ; Rules for athlon:
        ; 1) schedule latencies
        ; 2) add/mul and load/store in 2:1 proportion
        ; 3) avoid spliting >3byte instructions over 8byte boundaries

  psubw mm1, mm0    ;A3
  psubw mm3, mm2    ;B3
%if (%1)
  psubw mm5, mm4    ;C8
  psubw mm7, mm6    ;D8
%endif

ALIGN SECTION_ALIGN
  movq   mm4, [_ECX + %1 * 32 +16] ;C1
  pmaxsw mm1, mm0   ;A4
  movq   mm6, [_ECX + %1 * 32 +24] ;D1
  pmaxsw mm3, mm2   ;B4


  psraw mm0, 15     ;A5
  psraw mm2, 15     ;B5
%if (%1)
  movq [_EDX + %1 * 32 + 16-32], mm5 ;C9
  movq [_EDX + %1 * 32 + 24-32], mm7 ;D9
%endif

  pmulhw mm1, [_ESI] ;A6
  pmulhw mm3, [_ESI] ;B6
  movq mm5, [_EBX]   ;C2
  movq mm7, [_EBX]   ;D2

  nop
  nop
  pxor mm1, mm0 ;A7
  pxor mm3, mm2 ;B7

  psubw mm5, mm4    ;C3
  psubw mm7, mm6    ;D3
  psubw mm1, mm0    ;A8
  psubw mm3, mm2    ;B8


%if (%1 < 3)
  movq    mm0, [_ECX + %1 * 32 +32]    ;A1
%endif
  pmaxsw mm5, mm4     ;C4
%if (%1 < 3)
  movq mm2, [_ECX + %1 * 32 +8+32]  ;B1
%else
  cmp _ESP, _ESP
%endif
  pmaxsw mm7,mm6        ;D4

  psraw mm4, 15     ;C5
  psraw mm6, 15     ;D5
  movq [byte _EDX + %1 * 32], mm1 ;A9
  movq [_EDX + %1 * 32+8], mm3     ;B9


  pmulhw mm5, [_ESI] ;C6
  pmulhw mm7, [_ESI] ;D6
%if (%1 < 3)
  movq mm1, [_EBX]   ;A2
  movq mm3, [_EBX]   ;B2
%endif
%if (%1 == 0)
  push _EBP
%elif (%1 < 3)
  nop
%endif
  nop
%if (%1 == 3)
%ifdef ARCH_IS_X86_64
  lea r9, [int_div]
  imul eax, dword [r9+4*_EDI]
%else
  imul _EAX, [int_div+4*_EDI]
%endif
%endif
  pxor mm5, mm4 ;C7
  pxor mm7, mm6 ;D7
%endmacro


ALIGN SECTION_ALIGN
cglobal quant_h263_intra_3dne
quant_h263_intra_3dne:

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
  add _ESP, PTR_SIZE
%ifndef WINDOWS
  push prm6
  push prm5
%endif
  push prm4
  push prm3
  push prm2
  push prm1
  sub _ESP, PTR_SIZE
  mov [_ESP], TMP0
%endif  

  mov _EAX, [_ESP + 3*PTR_SIZE]       ; quant
  mov _ECX, [_ESP + 2*PTR_SIZE]       ; data
  mov _EDX, [_ESP + 1*PTR_SIZE]       ; coeff
  cmp al, 1
  pxor mm1, mm1
  pxor mm3, mm3
  movq mm0, [_ECX]           ; mm0 = [1st]
  movq mm2, [_ECX + 8]
  push _ESI
%ifdef ARCH_IS_X86_64
  lea _ESI, [mmx_div]
  lea _ESI, [_ESI + _EAX*8 - 8]
%else
  lea _ESI, [mmx_div + _EAX*8 - 8]
%endif

  push _EBX
  lea _EBX, [mmzero]
  push _EDI
  jz near .q1loop

  quant_intra 0
  mov _EBP, [_ESP + (4+4)*PTR_SIZE]   ; dcscalar
                                    ; NB -- there are 3 pushes in the function preambule and one more
                                    ; in "quant_intra 0", thus an added offset of 16 bytes
  movsx _EAX, word [byte _ECX]        ; DC

  quant_intra 1
  mov _EDI, _EAX
  sar _EDI, 31                       ; sign(DC)
  shr _EBP, byte 1                   ; _EBP = dcscalar/2

  quant_intra 2
  sub _EAX, _EDI                      ; DC (+1)
  xor _EBP, _EDI                      ; sign(DC) dcscalar /2  (-1)
  mov _EDI, [_ESP + (4+4)*PTR_SIZE]   ; dscalar
  lea _EAX, [byte _EAX + _EBP]         ; DC + sign(DC) dcscalar/2
  mov _EBP, [byte _ESP]

  quant_intra 3
  psubw mm5, mm4                    ;C8
  mov _ESI, [_ESP + 3*PTR_SIZE]       ; pop back the register value
  mov _EDI, [_ESP + 1*PTR_SIZE]       ; pop back the register value
  sar _EAX, 16
  lea _EBX, [byte _EAX + 1]           ; workaround for _EAX < 0
  cmovs _EAX, _EBX                    ; conditionnaly move the corrected value
  mov [_EDX], ax                     ; coeff[0] = ax
  mov _EBX, [_ESP + 2*PTR_SIZE]       ; pop back the register value
  add _ESP, byte 4*PTR_SIZE          ; "quant_intra 0" pushed _EBP, but we don't restore that one, just correct the stack offset by 16
  psubw mm7, mm6                    ;D8
  movq [_EDX + 3 * 32 + 16], mm5     ;C9
  movq [_EDX + 3 * 32 + 24], mm7     ;D9

  xor _EAX, _EAX

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
%ifndef WINDOWS
  add _ESP, 6*PTR_SIZE
%else
  add _ESP, 4*PTR_SIZE
%endif
  mov [_ESP], TMP0
%endif  

  ret

ALIGN SECTION_ALIGN

.q1loop:
  quant_intra1 0
  mov _EBP, [_ESP + (4+4)*PTR_SIZE]   ; dcscalar
  movsx _EAX, word [byte _ECX]        ; DC

  quant_intra1 1
  mov _EDI, _EAX
  sar _EDI, 31                       ; sign(DC)
  shr _EBP, byte 1                   ; _EBP = dcscalar /2

  quant_intra1 2
  sub _EAX, _EDI                      ; DC (+1)
  xor _EBP, _EDI                      ; sign(DC) dcscalar /2  (-1)
  mov _EDI, [_ESP + (4+4)*PTR_SIZE]   ; dcscalar
  lea _EAX, [byte _EAX + _EBP]         ; DC + sign(DC) dcscalar /2
  mov _EBP, [byte _ESP]

  quant_intra1 3
  psubw mm5, mm4                    ;C8
  mov _ESI, [_ESP + 3*PTR_SIZE]       ; pop back the register value
  mov _EDI, [_ESP + 1*PTR_SIZE]       ; pop back the register value
  sar _EAX, 16
  lea _EBX, [byte _EAX + 1]           ; workaround for _EAX < 0
  cmovs _EAX, _EBX                    ; conditionnaly move the corrected value
  mov [_EDX], ax                     ; coeff[0] = ax
  mov _EBX, [_ESP + 2*PTR_SIZE]       ; pop back the register value
  add _ESP, byte 4*PTR_SIZE          ; "quant_intra 0" pushed _EBP, but we don't restore that one, just correct the stack offset by 16
  psubw mm7, mm6                    ;D8
  movq [_EDX + 3 * 32 + 16], mm5     ;C9
  movq [_EDX + 3 * 32 + 24], mm7     ;D9

  xor _EAX, _EAX

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
%ifndef WINDOWS
  add _ESP, 6*PTR_SIZE
%else
  add _ESP, 4*PTR_SIZE
%endif
  mov [_ESP], TMP0
%endif  

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_inter_3dne(int16_t * coeff,
;                                const int16_t const * data,
;                                const uint32_t quant,
;                                const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------
;This is Athlon-optimized code (ca 90 clk per call)
;Optimized by Jaan, 30 Nov 2002


%macro quantinter 1
  movq mm1, [_EAX]               ;A2
  psraw mm3, 15                 ;B6
%if (%1)
  psubw mm2, mm6                ;C10
%endif
  psubw mm1, mm0                ;A3
  pmulhw mm4, mm7               ;B7
  movq mm6, [_ECX + %1*24+16]    ;C1
  pmaxsw mm1, mm0               ;A4
  paddw mm5, mm4                ;B8
%if (%1)
  movq [_EDX + %1*24+16-24], mm2 ;C11
%endif
  psubusw mm1, [_EBX]            ;A5 mm0 -= sub (unsigned, dont go < 0)
  pxor mm4, mm3                 ;B9
  movq mm2, [_EAX]               ;C2
  psraw mm0, 15                 ;A6
  psubw mm4, mm3                ;B10
  psubw mm2, mm6                ;C3
  pmulhw mm1, mm7               ;A7 mm0 = (mm0 / 2Q) >> 24
  movq mm3, [_ECX + %1*24+8] ;B1
  pmaxsw mm2, mm6               ;C4
  paddw mm5, mm1                ;A8 sum += mm0
%if (%1)
  movq [_EDX + %1*24+8-24], mm4  ;B11
%else
  movq [_EDX + 120], mm4         ;B11
%endif
  psubusw mm2, [_EBX]            ;C5
  pxor mm1, mm0                 ;A9 mm0 *= sign(mm0)
  movq mm4, [_EAX]               ;B2
  psraw mm6, 15                 ;C6
  psubw mm1, mm0                ;A10 undisplace
  psubw mm4, mm3                ;B3
  pmulhw mm2, mm7               ;C7
  movq mm0, [_ECX + %1*24+24]    ;A1 mm0 = [1st]
  pmaxsw mm4, mm3               ;B4
  paddw mm5, mm2                ;C8
  movq [byte _EDX + %1*24], mm1  ;A11
  psubusw mm4, [_EBX]            ;B5
  pxor mm2, mm6                 ;C9
%endmacro

%macro quantinter1 1
  movq mm0, [byte _ECX + %1*16]  ;mm0 = [1st]
  movq mm3, [_ECX + %1*16+8] ;
  movq mm1, [_EAX]
  movq mm4, [_EAX]
  psubw mm1, mm0
  psubw mm4, mm3
  pmaxsw mm1, mm0
  pmaxsw mm4, mm3
  psubusw mm1, mm6              ; mm0 -= sub (unsigned, dont go < 0)
  psubusw mm4, mm6              ;
  psraw mm0, 15
  psraw mm3, 15
  psrlw mm1, 1                  ; mm0 = (mm0 / 2Q) >> 16
  psrlw mm4, 1                  ;
  paddw mm5, mm1                ; sum += mm0
  pxor mm1, mm0                 ; mm0 *= sign(mm0)
  paddw mm5, mm4
  pxor mm4, mm3                 ;
  psubw mm1, mm0                ; undisplace
  psubw mm4, mm3
  cmp _ESP, _ESP
  movq [byte _EDX + %1*16], mm1
  movq [_EDX + %1*16+8], mm4
%endmacro

ALIGN SECTION_ALIGN
cglobal quant_h263_inter_3dne
quant_h263_inter_3dne:

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
  add _ESP, PTR_SIZE
%ifndef WINDOWS
  push prm6
  push prm5
%endif
  push prm4
  push prm3
  push prm2
  push prm1
  sub _ESP, PTR_SIZE
  mov [_ESP], TMP0
%endif  

  mov _EDX, [_ESP  + 1*PTR_SIZE]      ; coeff
  mov _ECX, [_ESP  + 2*PTR_SIZE]      ; data
  mov _EAX, [_ESP  + 3*PTR_SIZE]      ; quant
  push _EBX

  pxor mm5, mm5                     ; sum
  nop
%ifdef ARCH_IS_X86_64
  lea _EBX, [mmx_div]
  movq mm7, [_EBX + _EAX * 8 - 8]
  lea _EBX, [mmx_sub]
  lea _EBX, [_EBX + _EAX * 8 - 8]
%else
  lea _EBX,[mmx_sub + _EAX * 8 - 8]   ; sub
  movq mm7, [mmx_div + _EAX * 8 - 8] ; divider
%endif

  cmp al, 1
  lea _EAX, [mmzero]
  jz near .q1loop
  cmp _ESP, _ESP
ALIGN SECTION_ALIGN
  movq mm3, [_ECX + 120]     ;B1
  pxor mm4, mm4             ;B2
  psubw mm4, mm3            ;B3
  movq mm0, [_ECX]           ;A1 mm0 = [1st]
  pmaxsw mm4, mm3           ;B4
  psubusw mm4, [_EBX]        ;B5

  quantinter 0
  quantinter 1
  quantinter 2
  quantinter 3
  quantinter 4

  psraw mm3, 15             ;B6
  psubw mm2, mm6            ;C10
  pmulhw mm4, mm7           ;B7
  paddw mm5, mm4            ;B8
  pxor mm4, mm3             ;B9
  psubw mm4, mm3            ;B10
  movq [_EDX + 4*24+16], mm2 ;C11
  pop _EBX
  movq [_EDX + 4*24+8], mm4  ;B11
  pmaddwd mm5, [plus_one]
  movq mm0, mm5
  punpckhdq mm5, mm5
  paddd mm0, mm5
  movd eax, mm0             ; return sum

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
%ifndef WINDOWS
  add _ESP, 6*PTR_SIZE
%else
  add _ESP, 4*PTR_SIZE
%endif
  mov [_ESP], TMP0
%endif  

  ret

ALIGN SECTION_ALIGN
.q1loop:
  movq mm6, [byte _EBX]

  quantinter1 0
  quantinter1 1
  quantinter1 2
  quantinter1 3
  quantinter1 4
  quantinter1 5
  quantinter1 6
  quantinter1 7

  pmaddwd mm5, [plus_one]
  movq mm0, mm5
  psrlq mm5, 32
  paddd mm0, mm5
  movd eax, mm0 ; return sum

  pop _EBX

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
%ifndef WINDOWS
  add _ESP, 6*PTR_SIZE
%else
  add _ESP, 4*PTR_SIZE
%endif
  mov [_ESP], TMP0
%endif  

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_intra_3dne(int16_t *data,
;                                  const int16_t const *coeff,
;                                  const uint32_t quant,
;                                  const uint32_t dcscalar,
;                                  const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

  ; this is the same as dequant_inter_3dne, except that we're
  ; saturating using 'pminsw' (saves 2 cycles/loop => ~5% faster)

;This is Athlon-optimized code (ca 106 clk per call)

%macro dequant 1
  movq mm1, [_ECX+%1*24]         ; c  = coeff[i] ;A2
  psubw mm0, mm1                ;-c     ;A3 (1st dep)
%if (%1)
  paddw mm4, mm6                ;C11 mm6 free (4th+)
%endif
  pmaxsw mm0, mm1               ;|c|        ;A4 (2nd)
%if (%1)
  mov _EBP, _EBP
  pminsw mm4, [_EBX]             ;C12 saturates to +2047 (5th+) later
%endif
  movq mm6, [_ESI]               ;0      ;A5  mm6 in use
  pandn mm7, [_EAX]              ;B9 offset = isZero ? 0 : quant_add (2nd)
%if (%1)
  pxor mm5, mm4                 ;C13 (6th+) 1later
%endif
  movq mm4, [_ESI]               ;C1 ;0
  mov _ESP, _ESP
  pcmpeqw mm6, [_ECX+%1*24]      ;A6 (c ==0) ? -1 : 0 (1st)
ALIGN SECTION_ALIGN
  psraw mm1, 15                 ; sign(c)   ;A7 (2nd)
%if (%1)
  movq [_EDX+%1*24+16-24], mm5   ; C14 (7th) 2later
%endif
  paddw mm7, mm3                ;B10  offset +negate back (3rd)
  pmullw mm0, [_EDI]             ;*= 2Q  ;A8 (3rd+)
  paddw mm2, mm7                ;B11 mm7 free (4th+)
  lea _EBP, [byte _EBP]
  movq mm5, [_ECX+%1*24+16]      ;C2 ; c  = coeff[i]
  psubw mm4, mm5                ;-c         ;C3 (1st dep)
  pandn mm6, [_EAX]              ;A9 offset = isZero ? 0 : quant_add (2nd)
  pminsw mm2, [_EBX]             ;B12 saturates to +2047 (5th+)
  pxor mm3, mm2                 ;B13 (6th+)
  movq mm2, [byte _ESI]          ;B1 ;0
%if (%1)
  movq [_EDX+%1*24+8-24], mm3    ;B14 (7th)
%else
  movq [_EDX+120], mm3
%endif
  pmaxsw mm4, mm5               ;|c|        ;C4 (2nd)
  paddw mm6, mm1                ;A10  offset +negate back (3rd)
  movq mm3, [_ECX+%1*24 + 8]     ;B2 ; c  = coeff[i]
  psubw mm2, mm3                ;-c     ;B3 (1st dep)
  paddw mm0, mm6                ;A11 mm6 free (4th+)
  movq mm6, [byte _ESI]          ;0          ;C5  mm6 in use
  pcmpeqw mm6, [_ECX+%1*24+16]   ;C6 (c ==0) ? -1 : 0 (1st)
  pminsw mm0, [_EBX]             ;A12 saturates to +2047 (5th+)
  pmaxsw mm2, mm3               ;|c|        ;B4 (2nd)
  pxor mm1, mm0                 ;A13 (6th+)
  pmullw mm4, [_EDI]             ;*= 2Q  ;C8 (3rd+)
  psraw mm5, 15                 ; sign(c)   ;C7 (2nd)
  movq mm7, [byte _ESI]          ;0          ;B5 mm7 in use
  pcmpeqw mm7, [_ECX+%1*24 + 8]  ;B6 (c ==0) ? -1 : 0 (1st)
%if (%1 < 4)
  movq mm0, [byte _ESI]          ;A1 ;0
%endif
  pandn mm6, [byte _EAX]         ;C9 offset = isZero ? 0 : quant_add (2nd)
  psraw mm3, 15                 ;sign(c)    ;B7 (2nd)
  movq [byte _EDX+%1*24], mm1    ;A14 (7th)
  paddw mm6, mm5                ;C10  offset +negate back (3rd)
  pmullw mm2, [_EDI]             ;*= 2Q  ;B8 (3rd+)
  mov _ESP, _ESP
%endmacro


ALIGN SECTION_ALIGN
cglobal dequant_h263_intra_3dne
dequant_h263_intra_3dne:

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
  add _ESP, PTR_SIZE
%ifndef WINDOWS
  push prm6
  push prm5
%endif
  push prm4
  push prm3
  push prm2
  push prm1
  sub _ESP, PTR_SIZE
  mov [_ESP], TMP0
%endif  

  mov _ECX, [_ESP+ 2*PTR_SIZE]        ; coeff
  mov _EAX, [_ESP+ 3*PTR_SIZE]        ; quant
  pxor mm0, mm0
  pxor mm2, mm2
  push _EDI
  push _EBX
%ifdef ARCH_IS_X86_64
  lea _EDI, [mmx_mul]
  lea _EDI, [_EDI + _EAX*8 - 8]    ; 2*quant
%else
  lea _EDI, [mmx_mul + _EAX*8 - 8]    ; 2*quant
%endif
  push _EBP
  lea _EBX, [mmx_2047]
  movsx _EBP, word [_ECX]
%ifdef ARCH_IS_X86_64
  lea r9, [mmx_add]
  lea _EAX, [r9 + _EAX*8 - 8]    ; quant or quant-1
%else
  lea _EAX, [mmx_add + _EAX*8 - 8]    ; quant or quant-1
%endif
  push _ESI
  lea _ESI, [mmzero]
  pxor mm7, mm7
  movq mm3, [_ECX+120]               ;B2 ; c  = coeff[i]
  pcmpeqw mm7, [_ECX+120]            ;B6 (c ==0) ? -1 : 0 (1st)

  imul _EBP, [_ESP+(4+4)*PTR_SIZE]    ; dcscalar
  psubw mm2, mm3                    ;-c         ;B3 (1st dep)
  pmaxsw mm2, mm3                   ;|c|        ;B4 (2nd)
  pmullw mm2, [_EDI]                 ;*= 2Q  ;B8 (3rd+)
  psraw mm3, 15                     ; sign(c)   ;B7 (2nd)
  mov _EDX, [_ESP+ (1+4)*PTR_SIZE]    ; data

ALIGN SECTION_ALIGN
  dequant 0

  cmp _EBP, -2048
  mov _ESP, _ESP

  dequant 1

  cmovl _EBP, [int_2048]
  nop

  dequant 2

  cmp _EBP, 2047
  mov _ESP, _ESP

  dequant 3

  cmovg _EBP, [int2047]
  nop

  dequant 4

  paddw mm4, mm6            ;C11 mm6 free (4th+)
  pminsw mm4, [_EBX]         ;C12 saturates to +2047 (5th+)
  pandn mm7, [_EAX]          ;B9 offset = isZero ? 0 : quant_add (2nd)
  mov _EAX, _EBP
  mov _ESI, [_ESP]
  mov _EBP, [_ESP+PTR_SIZE]
  pxor mm5, mm4             ;C13 (6th+)
  paddw mm7, mm3            ;B10  offset +negate back (3rd)
  movq [_EDX+4*24+16], mm5   ;C14 (7th)
  paddw mm2, mm7            ;B11 mm7 free (4th+)
  pminsw mm2, [_EBX]         ;B12 saturates to +2047 (5th+)
  mov _EBX, [_ESP+2*PTR_SIZE]
  mov _EDI, [_ESP+3*PTR_SIZE]
  add _ESP, byte 4*PTR_SIZE
  pxor mm3, mm2             ;B13 (6th+)
  movq [_EDX+4*24+8], mm3    ;B14 (7th)
  mov [_EDX], ax

  xor _EAX, _EAX

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
%ifndef WINDOWS
  add _ESP, 6*PTR_SIZE
%else
  add _ESP, 4*PTR_SIZE
%endif
  mov [_ESP], TMP0
%endif  

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_inter_3dne(int16_t * data,
;                                  const int16_t * const coeff,
;                                  const uint32_t quant,
;                                  const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

; this is the same as dequant_inter_3dne,
; except that we're saturating using 'pminsw' (saves 2 cycles/loop)
; This is Athlon-optimized code (ca 100 clk per call)

ALIGN SECTION_ALIGN
cglobal dequant_h263_inter_3dne
dequant_h263_inter_3dne:

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
  add _ESP, PTR_SIZE
%ifndef WINDOWS
  push prm6
  push prm5
%endif
  push prm4
  push prm3
  push prm2
  push prm1
  sub _ESP, PTR_SIZE
  mov [_ESP], TMP0
%endif  

  mov _ECX, [_ESP+ 2*PTR_SIZE]        ; coeff
  mov _EAX, [_ESP+ 3*PTR_SIZE]        ; quant
  pxor mm0, mm0
  pxor mm2, mm2
  push _EDI
  push _EBX
  push _ESI
%ifdef ARCH_IS_X86_64
  lea _EDI, [mmx_mul]
  lea _EDI, [_EDI + _EAX*8 - 8]    ; 2*quant
%else
  lea _EDI, [mmx_mul + _EAX*8 - 8]    ; 2*quant
%endif
  lea _EBX, [mmx_2047]
  pxor mm7, mm7
  movq mm3, [_ECX+120]               ;B2 ; c  = coeff[i]
  pcmpeqw mm7, [_ECX+120]            ;B6 (c ==0) ? -1 : 0 (1st)
%ifdef ARCH_IS_X86_64
  lea r9, [mmx_add]
  lea _EAX, [r9 + _EAX*8 - 8]    ; quant or quant-1
%else
  lea _EAX, [mmx_add + _EAX*8 - 8]    ; quant or quant-1
%endif
  psubw mm2, mm3                    ;-c ;B3 (1st dep)
  lea _ESI, [mmzero]
  pmaxsw mm2, mm3                   ;|c|        ;B4 (2nd)
  pmullw mm2, [_EDI]                 ;*= 2Q      ;B8 (3rd+)
  psraw mm3, 15                     ; sign(c)   ;B7 (2nd)
  mov _EDX, [_ESP+ (1+3)*PTR_SIZE]    ; data

ALIGN SECTION_ALIGN

  dequant 0
  dequant 1
  dequant 2
  dequant 3
  dequant 4

  paddw mm4, mm6            ;C11 mm6 free (4th+)
  pminsw mm4, [_EBX]         ;C12 saturates to +2047 (5th+)
  pandn mm7, [_EAX]          ;B9 offset = isZero ? 0 : quant_add (2nd)
  mov _ESI, [_ESP]
  pxor mm5, mm4             ;C13 (6th+)
  paddw mm7, mm3            ;B10  offset +negate back (3rd)
  movq [_EDX+4*24+16], mm5   ;C14 (7th)
  paddw mm2, mm7            ;B11 mm7 free (4th+)
  pminsw mm2, [_EBX]         ;B12 saturates to +2047 (5th+)
  mov _EBX, [_ESP+PTR_SIZE]
  mov _EDI, [_ESP+2*PTR_SIZE]
  add _ESP, byte 3*PTR_SIZE
  pxor mm3, mm2             ;B13 (6th+)
  movq [_EDX+4*24+8], mm3    ;B14 (7th)

  xor _EAX, _EAX

%ifdef ARCH_IS_X86_64
  mov TMP0, [_ESP]
%ifndef WINDOWS
  add _ESP, 6*PTR_SIZE
%else
  add _ESP, 4*PTR_SIZE
%endif
  mov [_ESP], TMP0
%endif  

  ret
ENDFUNC

NON_EXEC_STACK
