;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MPEG4 Quantization H263 implementation / MMX optimized -
; *
; *  Copyright(C) 2001-2003 Peter Ross <pross@xvid.org>
; *               2002-2003 Pascal Massimino <skal@planet-d.net>
; *               2004      Jean-Marc Bastide <jmtest@voila.fr>
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
; * $Id: quantize_h263_mmx.asm,v 1.11.2.4 2009/09/16 17:11:39 Isibaar Exp $
; *
; ****************************************************************************/

; enable dequant saturate [-2048,2047], test purposes only.
%define SATURATE

%include "nasm.inc"

;=============================================================================
; Read only Local data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
plus_one:
	times 8 dw 1

;-----------------------------------------------------------------------------
;
; quant table
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_quant:
%assign quant 0
%rep 32
	times 4 dw quant
	%assign quant quant+1
%endrep

;-----------------------------------------------------------------------------
;
; subtract by Q/2 table
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
mmx_sub:
%assign quant 1
%rep 31
	times 4 dw  quant / 2
	%assign quant quant+1
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
%assign quant 1
%rep 31
	times 4 dw  (1<<16) / (quant*2) + 1
	%assign quant quant+1
%endrep

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal quant_h263_intra_mmx
cglobal quant_h263_intra_sse2
cglobal quant_h263_inter_mmx
cglobal quant_h263_inter_sse2
cglobal dequant_h263_intra_mmx
cglobal dequant_h263_intra_xmm
cglobal dequant_h263_intra_sse2
cglobal dequant_h263_inter_mmx
cglobal dequant_h263_inter_xmm
cglobal dequant_h263_inter_sse2

;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_intra_mmx(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint32_t dcscalar,
;                               const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
quant_h263_intra_mmx:

  mov _EAX, prm2     ; data
  mov TMP0, prm4     ; dcscalar
  movsx _EAX, word [_EAX]  ; data[0]
  
  sar TMP0, 1              ; dcscalar /2
  mov TMP1, _EAX
  sar TMP1, 31             ; sgn(data[0])
  xor TMP0,TMP1            ; *sgn(data[0])
  sub _EAX,TMP1
  add _EAX,TMP0            ; + (dcscalar/2)*sgn(data[0])

  mov TMP0, prm3     ; quant
  lea TMP1, [mmx_div]
  movq mm7, [TMP1+TMP0 * 8 - 8]
%ifdef ARCH_IS_X86_64
%ifdef WINDOWS
  mov TMP1, prm2
%endif
%endif
  cdq 
  idiv prm4d         ; dcscalar
%ifdef ARCH_IS_X86_64
%ifdef WINDOWS
  mov prm2, TMP1
%endif
%endif
  cmp TMP0, 1
  mov TMP1, prm1     ; coeff
  je .low
 
  mov TMP0, prm2     ; data
  push _EAX          ; DC
  mov _EAX, TMP0

  mov TMP0,4

.loop:
  movq mm0, [_EAX]           ; data      
  pxor mm4,mm4
  movq mm1, [_EAX + 8]
  pcmpgtw mm4,mm0           ; (data<0)
  pxor mm5,mm5
  pmulhw mm0,mm7            ; /(2*quant)
  pcmpgtw mm5,mm1
  movq mm2, [_EAX+16] 
  psubw mm0,mm4             ;  +(data<0)
  pmulhw mm1,mm7
  pxor mm4,mm4
  movq mm3,[_EAX+24]
  pcmpgtw mm4,mm2
  psubw mm1,mm5 
  pmulhw mm2,mm7 
  pxor mm5,mm5
  pcmpgtw mm5,mm3
  pmulhw mm3,mm7
  psubw mm2,mm4
  psubw mm3,mm5  
  movq [TMP1], mm0
  lea _EAX, [_EAX+32]
  movq [TMP1 + 8], mm1
  movq [TMP1 + 16], mm2
  movq [TMP1 + 24], mm3
   
  dec TMP0
  lea TMP1, [TMP1+32]
  jne .loop
  jmp .end
   
.low:
  movd mm7,TMP0d

  mov TMP0, prm2
  push _EAX
  mov _EAX, TMP0

  mov TMP0,4
.loop_low:  
  movq mm0, [_EAX]           
  pxor mm4,mm4
  movq mm1, [_EAX + 8]
  pcmpgtw mm4,mm0
  pxor mm5,mm5
  psubw mm0,mm4
  pcmpgtw mm5,mm1
  psraw mm0,mm7
  psubw mm1,mm5 
  movq mm2,[_EAX+16]
  pxor mm4,mm4
  psraw mm1,mm7
  pcmpgtw mm4,mm2
  pxor mm5,mm5
  psubw mm2,mm4
  movq mm3,[_EAX+24]
  pcmpgtw mm5,mm3
  psraw mm2,mm7
  psubw mm3,mm5
  movq [TMP1], mm0
  psraw mm3,mm7
  movq [TMP1 + 8], mm1
  movq [TMP1+16],mm2
  lea _EAX, [_EAX+32]
  movq [TMP1+24],mm3
  
  dec TMP0
  lea TMP1, [TMP1+32]
  jne .loop_low
  
.end:

  pop _EAX

  mov TMP1, prm1     ; coeff
  mov [TMP1],ax  
  xor _EAX,_EAX       ; return 0

  ret
ENDFUNC
 

;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_intra_sse2(int16_t * coeff,
;                                const int16_t const * data,
;                                const uint32_t quant,
;                                const uint32_t dcscalar,
;                                const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
quant_h263_intra_sse2:
  PUSH_XMM6_XMM7
  mov _EAX, prm2     ; data
 
  movsx _EAX, word [_EAX]      ; data[0]
 
  mov TMP0,prm4     ; dcscalar
  mov TMP1,_EAX
  sar TMP0,1
  add _EAX,TMP0
  sub TMP1,TMP0
  cmovl _EAX,TMP1              ; +/- dcscalar/2
  mov TMP0, prm3    ; quant
  lea TMP1, [mmx_div]
  movq xmm7, [TMP1+TMP0 * 8 - 8]

%ifdef ARCH_IS_X86_64
%ifdef WINDOWS
  mov TMP1, prm2
%endif
%endif
  cdq 
  idiv prm4d  ; dcscalar
%ifdef ARCH_IS_X86_64
%ifdef WINDOWS
  mov prm2, TMP1
%endif
%endif
  cmp TMP0, 1
  mov TMP1, prm1     ; coeff
  je near .low
  
  mov TMP0, prm2
  push _EAX ; DC
  mov _EAX, TMP0

  mov TMP0,2
  movlhps xmm7,xmm7

.loop:
  movdqa xmm0, [_EAX]           
  pxor xmm4,xmm4
  movdqa xmm1, [_EAX + 16]
  pcmpgtw xmm4,xmm0 
  pxor xmm5,xmm5
  pmulhw xmm0,xmm7
  pcmpgtw xmm5,xmm1
  movdqa xmm2, [_EAX+32] 
  psubw xmm0,xmm4
  pmulhw xmm1,xmm7
  pxor xmm4,xmm4
  movdqa xmm3,[_EAX+48]
  pcmpgtw xmm4,xmm2
  psubw xmm1,xmm5 
  pmulhw xmm2,xmm7 
  pxor xmm5,xmm5
  pcmpgtw xmm5,xmm3
  pmulhw xmm3,xmm7
  psubw xmm2,xmm4
  psubw xmm3,xmm5  
  movdqa [TMP1], xmm0
  lea _EAX, [_EAX+64]
  movdqa [TMP1 + 16], xmm1
  movdqa [TMP1 + 32], xmm2
  movdqa [TMP1 + 48], xmm3
   
  dec TMP0
  lea TMP1, [TMP1+64]
  jne .loop
  jmp .end
   
.low:
  movd xmm7,TMP0d

  mov TMP0, prm2
  push _EAX ; DC
  mov _EAX, TMP0

  mov TMP0,2
.loop_low:  
  movdqa xmm0, [_EAX]           
  pxor xmm4,xmm4
  movdqa xmm1, [_EAX + 16]
  pcmpgtw xmm4,xmm0
  pxor xmm5,xmm5
  psubw xmm0,xmm4
  pcmpgtw xmm5,xmm1
  psraw xmm0,xmm7
  psubw xmm1,xmm5 
  movdqa xmm2,[_EAX+32]
  pxor xmm4,xmm4
  psraw xmm1,xmm7
  pcmpgtw xmm4,xmm2
  pxor xmm5,xmm5
  psubw xmm2,xmm4
  movdqa xmm3,[_EAX+48]
  pcmpgtw xmm5,xmm3
  psraw xmm2,xmm7
  psubw xmm3,xmm5
  movdqa [TMP1], xmm0
  psraw xmm3,xmm7
  movdqa [TMP1+16], xmm1
  movdqa [TMP1+32],xmm2
  lea _EAX, [_EAX+64]
  movdqa [TMP1+48],xmm3
  
  dec TMP0
  lea TMP1, [TMP1+64]
  jne .loop_low
  
.end:

  pop _EAX

  mov TMP1, prm1     ; coeff
  mov [TMP1],ax  
  xor _EAX,_EAX            ; return 0
  POP_XMM6_XMM7
  ret
ENDFUNC
 
;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_inter_mmx(int16_t * coeff,
;                               const int16_t const * data,
;                               const uint32_t quant,
;                               const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------
  
ALIGN SECTION_ALIGN
quant_h263_inter_mmx:

  mov TMP1, prm1           ; coeff
  mov _EAX, prm3           ; quant

  pxor mm5, mm5                     ; sum
  lea TMP0, [mmx_sub]
  movq mm6, [TMP0 + _EAX * 8 - 8] ; sub

  cmp al, 1
  jz near .q1routine

  lea TMP0, [mmx_div]
  movq mm7, [TMP0 + _EAX * 8 - 8] ; divider

  xor TMP0, TMP0
  mov _EAX, prm2           ; data

ALIGN SECTION_ALIGN
.loop:
  movq mm0, [_EAX + 8*TMP0]           ; mm0 = [1st]
  movq mm3, [_EAX + 8*TMP0 + 8]
  pxor mm1, mm1                     ; mm1 = 0
  pxor mm4, mm4                     ;
  pcmpgtw mm1, mm0                  ; mm1 = (0 > mm0)
  pcmpgtw mm4, mm3                  ;
  pxor mm0, mm1                     ; mm0 = |mm0|
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; displace
  psubw mm3, mm4                    ;
  psubusw mm0, mm6                  ; mm0 -= sub (unsigned, dont go < 0)
  psubusw mm3, mm6                  ;
  pmulhw mm0, mm7                   ; mm0 = (mm0 / 2Q) >> 16
  pmulhw mm3, mm7                   ;
  paddw mm5, mm0                    ; sum += mm0
  pxor mm0, mm1                     ; mm0 *= sign(mm0)
  paddw mm5, mm3                    ;
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; undisplace
  psubw mm3, mm4
  movq [TMP1 + 8*TMP0], mm0
  movq [TMP1 + 8*TMP0 + 8], mm3

  add TMP0, 2
  cmp TMP0, 16
  jnz .loop

.done:
  pmaddwd mm5, [plus_one]
  movq mm0, mm5
  psrlq mm5, 32
  paddd mm0, mm5

  movd eax, mm0     ; return sum

  ret

.q1routine:
  xor TMP0, TMP0
  mov _EAX, prm2           ; data

ALIGN SECTION_ALIGN
.q1loop:
  movq mm0, [_EAX + 8*TMP0]           ; mm0 = [1st]
  movq mm3, [_EAX + 8*TMP0+ 8]        ;
  pxor mm1, mm1                     ; mm1 = 0
  pxor mm4, mm4                     ;
  pcmpgtw mm1, mm0                  ; mm1 = (0 > mm0)
  pcmpgtw mm4, mm3                  ;
  pxor mm0, mm1                     ; mm0 = |mm0|
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; displace
  psubw mm3, mm4                    ;
  psubusw mm0, mm6                  ; mm0 -= sub (unsigned, dont go < 0)
  psubusw mm3, mm6                  ;
  psrlw mm0, 1                      ; mm0 >>= 1   (/2)
  psrlw mm3, 1                      ;
  paddw mm5, mm0                    ; sum += mm0
  pxor mm0, mm1                     ; mm0 *= sign(mm0)
  paddw mm5, mm3                    ;
  pxor mm3, mm4                     ;
  psubw mm0, mm1                    ; undisplace
  psubw mm3, mm4
  movq [TMP1 + 8*TMP0], mm0
  movq [TMP1 + 8*TMP0 + 8], mm3

  add TMP0, 2
  cmp TMP0, 16
  jnz .q1loop

  jmp .done
ENDFUNC



;-----------------------------------------------------------------------------
;
; uint32_t quant_h263_inter_sse2(int16_t * coeff,
;                                const int16_t const * data,
;                                const uint32_t quant,
;                                const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
quant_h263_inter_sse2:
  PUSH_XMM6_XMM7
  
  mov TMP1, prm1      ; coeff
  mov _EAX, prm3      ; quant

  pxor xmm5, xmm5                           ; sum

  lea TMP0, [mmx_sub]
  movq mm0, [TMP0 + _EAX*8 - 8]             ; sub
  movq2dq xmm6, mm0                         ; load into low 8 bytes
  movlhps xmm6, xmm6                        ; duplicate into high 8 bytes

  cmp al, 1
  jz near .qes2_q1_routine

.qes2_not1:
  lea TMP0, [mmx_div]
  movq mm0, [TMP0 + _EAX*8 - 8]          ; divider

  xor TMP0, TMP0
  mov _EAX, prm2      ; data

  movq2dq xmm7, mm0
  movlhps xmm7, xmm7

ALIGN SECTION_ALIGN
.qes2_loop:
  movdqa xmm0, [_EAX + TMP0*8]               ; xmm0 = [1st]
  movdqa xmm3, [_EAX + TMP0*8 + 16]          ; xmm3 = [2nd]
  pxor xmm1, xmm1
  pxor xmm4, xmm4
  pcmpgtw xmm1, xmm0
  pcmpgtw xmm4, xmm3
  pxor xmm0, xmm1
  pxor xmm3, xmm4
  psubw xmm0, xmm1
  psubw xmm3, xmm4
  psubusw xmm0, xmm6
  psubusw xmm3, xmm6
  pmulhw xmm0, xmm7
  pmulhw xmm3, xmm7
  paddw xmm5, xmm0
  pxor xmm0, xmm1
  paddw xmm5, xmm3
  pxor xmm3, xmm4
  psubw xmm0, xmm1
  psubw xmm3, xmm4
  movdqa [TMP1 + TMP0*8], xmm0
  movdqa [TMP1 + TMP0*8 + 16], xmm3

  add TMP0, 4
  cmp TMP0, 16
  jnz .qes2_loop

.qes2_done:
  movdqu xmm6, [plus_one]
  pmaddwd xmm5, xmm6
  movhlps xmm6, xmm5
  paddd xmm5, xmm6
  movdq2q mm0, xmm5

  movq mm5, mm0
  psrlq mm5, 32
  paddd mm0, mm5

  movd eax, mm0         ; return sum

  POP_XMM6_XMM7
  ret

.qes2_q1_routine:
  xor TMP0, TMP0
  mov _EAX, prm2      ; data

ALIGN SECTION_ALIGN
.qes2_q1loop:
  movdqa xmm0, [_EAX + TMP0*8]        ; xmm0 = [1st]
  movdqa xmm3, [_EAX + TMP0*8 + 16]   ; xmm3 = [2nd]
  pxor xmm1, xmm1
  pxor xmm4, xmm4
  pcmpgtw xmm1, xmm0
  pcmpgtw xmm4, xmm3
  pxor xmm0, xmm1
  pxor xmm3, xmm4
  psubw xmm0, xmm1
  psubw xmm3, xmm4
  psubusw xmm0, xmm6
  psubusw xmm3, xmm6
  psrlw xmm0, 1
  psrlw xmm3, 1
  paddw xmm5, xmm0
  pxor xmm0, xmm1
  paddw xmm5, xmm3
  pxor xmm3, xmm4
  psubw xmm0, xmm1
  psubw xmm3, xmm4
  movdqa [TMP1 + TMP0*8], xmm0
  movdqa [TMP1 + TMP0*8 + 16], xmm3

  add TMP0, 4
  cmp TMP0, 16
  jnz .qes2_q1loop
  jmp .qes2_done
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_intra_mmx(int16_t *data,
;                                 const int16_t const *coeff,
;                                 const uint32_t quant,
;                                 const uint32_t dcscalar,
;                                 const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
dequant_h263_intra_mmx:

  mov TMP0, prm3                 ; quant
  mov _EAX, prm2                 ; coeff
  pcmpeqw mm0,mm0
  lea TMP1, [mmx_quant]
  movq mm6, [TMP1 + TMP0*8] ; quant
  shl TMP0,31                    ; quant & 1 ? 0 : - 1
  movq mm7,mm6
  movq mm5,mm0
  movd mm1,TMP0d
  mov TMP1, prm1                 ; data
  psllw mm0,mm1
  paddw mm7,mm7                  ; 2*quant
  paddw mm6,mm0                  ; quant-1
  psllw mm5,12
  mov TMP0,8
  psrlw mm5,1

.loop: 
  movq mm0,[_EAX] 
  pxor mm2,mm2
  pxor mm4,mm4
  pcmpgtw mm2,mm0
  pcmpeqw mm4,mm0
  pmullw mm0,mm7      ; * 2 * quant  
  movq mm1,[_EAX+8]
  psubw mm0,mm2 
  pxor mm2,mm6
  pxor mm3,mm3
  pandn mm4,mm2
  pxor mm2,mm2
  pcmpgtw mm3,mm1
  pcmpeqw mm2,mm1
  pmullw mm1,mm7
  paddw mm0,mm4
  psubw mm1,mm3
  pxor mm3,mm6
  pandn mm2,mm3
  paddsw mm0, mm5        ; saturate
  paddw mm1,mm2
   
  paddsw mm1, mm5
  psubsw mm0, mm5
  psubsw mm1, mm5
  psubsw mm0, mm5
  psubsw mm1, mm5
  paddsw mm0, mm5
  paddsw mm1, mm5
  
  movq [TMP1],mm0
  lea _EAX,[_EAX+16]
  movq [TMP1+8],mm1
 
  dec TMP0
  lea TMP1,[TMP1+16]
  jne .loop
  
   ; deal with DC
  mov _EAX, prm2               ; coeff
  movd mm1,prm4d                ; dcscalar
  movd mm0,[_EAX]                   ; coeff[0]
  pmullw mm0,mm1                   ; * dcscalar
  mov TMP1, prm1               ; data
  paddsw mm0, mm5                  ; saturate +
  psubsw mm0, mm5
  psubsw mm0, mm5                  ; saturate -
  paddsw mm0, mm5
  movd eax,mm0
  mov [TMP1], ax

  xor _EAX, _EAX                    ; return 0
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_intra_xmm(int16_t *data,
;                                 const int16_t const *coeff,
;                                 const uint32_t quant,
;                                 const uint32_t dcscalar,
;                                 const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

  
ALIGN SECTION_ALIGN 
dequant_h263_intra_xmm:

  mov TMP0, prm3                 ; quant
  mov _EAX, prm2                 ; coeff

  movd mm6,TMP0d                  ; quant
  pcmpeqw mm0,mm0
  pshufw mm6,mm6,0               ; all quant
  shl TMP0,31
  movq mm5,mm0
  movq mm7,mm6
  movd mm1,TMP0d
  mov TMP1, prm1                 ; data
  psllw mm0,mm1                  ; quant & 1 ? 0 : - 1
  movq mm4,mm5
  paddw mm7,mm7                  ; quant*2
  paddw mm6,mm0                  ; quant-1
  psrlw mm4,5                    ; mm4=2047
  mov TMP0,8
  pxor mm5,mm4                   ; mm5=-2048
  
.loop:
  movq mm0,[_EAX] 
  pxor mm2,mm2
  pxor mm3,mm3

  pcmpgtw mm2,mm0
  pcmpeqw mm3,mm0     ; if coeff==0...
  pmullw mm0,mm7      ; * 2 * quant
  movq mm1,[_EAX+8]
  
  psubw mm0,mm2
  pxor mm2,mm6
  pandn mm3,mm2       ; ...then data=0
  pxor mm2,mm2
  paddw mm0,mm3
  pxor mm3,mm3
  pcmpeqw mm2,mm1
  pcmpgtw mm3,mm1
  pmullw mm1,mm7
   
  pminsw mm0,mm4
  psubw mm1,mm3
  pxor mm3,mm6
  pandn mm2,mm3
  paddw mm1,mm2
  
  pmaxsw mm0,mm5
  pminsw mm1,mm4
  movq [TMP1],mm0
  pmaxsw mm1,mm5
  lea _EAX,[_EAX+16]
  movq [TMP1+8],mm1
  
  dec TMP0
  lea TMP1,[TMP1+16]
  jne .loop
  
   ; deal with DC
  mov _EAX, prm2                ; coeff
  movd mm1,prm4d                 ; dcscalar
  movd mm0, [_EAX]
  pmullw mm0, mm1            
  mov TMP1, prm1                ; data
  pminsw mm0,mm4
  pmaxsw mm0,mm5
  movd eax, mm0
  mov [TMP1], ax

  xor _EAX, _EAX                ; return 0
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_intra_sse2(int16_t *data,
;                                  const int16_t const *coeff,
;                                  const uint32_t quant,
;                                  const uint32_t dcscalar,
;                                  const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN 
dequant_h263_intra_sse2:
  PUSH_XMM6_XMM7
  
  mov TMP0, prm3                 ; quant
  mov _EAX, prm2                 ; coeff
 
  movd xmm6,TMP0d                     ; quant

  shl TMP0,31
  pshuflw xmm6,xmm6,0
  pcmpeqw xmm0,xmm0
  movlhps xmm6,xmm6                 ; all quant
  movd xmm1,TMP0d
  movdqa xmm5,xmm0
  movdqa xmm7,xmm6
  mov TMP1, prm1                 ; data
  paddw xmm7,xmm7                   ; quant *2
  psllw xmm0,xmm1                   ; quant & 1 ? 0 : - 1 
  movdqa xmm4,xmm5
  paddw xmm6,xmm0                   ; quant-1
  psrlw xmm4,5                      ; 2047
  mov TMP0,4
  pxor xmm5,xmm4                    ; mm5=-2048
  
.loop:
  movdqa xmm0,[_EAX] 
  pxor xmm2,xmm2
  pxor xmm3,xmm3

  pcmpgtw xmm2,xmm0
  pcmpeqw xmm3,xmm0
  pmullw xmm0,xmm7      ; * 2 * quant
  movdqa xmm1,[_EAX+16]
  
  psubw xmm0,xmm2
  pxor xmm2,xmm6
  pandn xmm3,xmm2
  pxor xmm2,xmm2
  paddw xmm0,xmm3
  pxor xmm3,xmm3
  pcmpeqw xmm2,xmm1
  pcmpgtw xmm3,xmm1
  pmullw xmm1,xmm7
   
  pminsw xmm0,xmm4
  psubw xmm1,xmm3
  pxor xmm3,xmm6
  pandn xmm2,xmm3
  paddw xmm1,xmm2
  
  pmaxsw xmm0,xmm5
  pminsw xmm1,xmm4
  movdqa [TMP1],xmm0
  pmaxsw xmm1,xmm5
  lea _EAX,[_EAX+32]
  movdqa [TMP1+16],xmm1
 
  dec TMP0
  lea TMP1,[TMP1+32]
  jne .loop
    
   ; deal with DC

  mov _EAX, prm2             ; coeff
  movsx _EAX,word [_EAX]
  imul prm4d                 ; dcscalar
  mov TMP1, prm1             ; data
  movd xmm0,eax
  pminsw xmm0,xmm4
  pmaxsw xmm0,xmm5
  movd eax,xmm0
  
  mov [TMP1], ax

  xor _EAX, _EAX                  ; return 0

  POP_XMM6_XMM7
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32t dequant_h263_inter_mmx(int16_t * data,
;                                const int16_t * const coeff,
;                                const uint32_t quant,
;                                const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
dequant_h263_inter_mmx:

  mov TMP0, prm3                 ; quant
  mov _EAX, prm2                 ; coeff
  pcmpeqw mm0,mm0
  lea TMP1, [mmx_quant]
  movq mm6, [TMP1 + TMP0*8]      ; quant
  shl TMP0,31                    ; odd/even
  movq mm7,mm6
  movd mm1,TMP0d
  mov TMP1, prm1                 ; data
  movq mm5,mm0
  psllw mm0,mm1                  ; quant & 1 ? 0 : - 1
  paddw mm7,mm7                  ; quant*2
  paddw mm6,mm0                  ; quant & 1 ? quant : quant - 1
  psllw mm5,12
  mov TMP0,8
  psrlw mm5,1                    ; 32767-2047 (32768-2048)

.loop:
  movq mm0,[_EAX] 
  pxor mm4,mm4
  pxor mm2,mm2
  pcmpeqw mm4,mm0     ; if coeff==0...
  pcmpgtw mm2,mm0
  pmullw mm0,mm7      ; * 2 * quant
  pxor mm3,mm3
  psubw mm0,mm2 
  movq mm1,[_EAX+8]
  pxor mm2,mm6
  pcmpgtw mm3,mm1
  pandn mm4,mm2      ; ... then data==0
  pmullw mm1,mm7
  pxor mm2,mm2
  pcmpeqw mm2,mm1
  psubw mm1,mm3
  pxor mm3,mm6
  pandn mm2,mm3
  paddw mm0,mm4
  paddw mm1,mm2
    
  paddsw mm0, mm5        ; saturate
  paddsw mm1, mm5
  psubsw mm0, mm5
  psubsw mm1, mm5
  psubsw mm0, mm5
  psubsw mm1, mm5
  paddsw mm0, mm5
  paddsw mm1, mm5
  
  movq [TMP1],mm0
  lea _EAX,[_EAX+16]
  movq [TMP1+8],mm1
 
  dec TMP0
  lea TMP1,[TMP1+16]
  jne .loop
  
  xor _EAX, _EAX              ; return 0
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_inter_xmm(int16_t * data,
;                                 const int16_t * const coeff,
;                                 const uint32_t quant,
;                                 const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------
ALIGN SECTION_ALIGN 
dequant_h263_inter_xmm:

  mov TMP0, prm3                 ; quant
  mov _EAX, prm2                 ; coeff
  pcmpeqw mm0,mm0
  lea TMP1, [mmx_quant]
  movq mm6, [TMP1 + TMP0*8]      ; quant
  shl TMP0,31
  movq mm5,mm0
  movd mm1,TMP0d
  movq mm7,mm6
  psllw mm0,mm1
  mov TMP1, prm1                 ; data
  movq mm4,mm5
  paddw mm7,mm7
  paddw mm6,mm0                     ; quant-1

  psrlw mm4,5
  mov TMP0,8
  pxor mm5,mm4                      ; mm5=-2048
   
.loop:
  movq mm0,[_EAX] 
  pxor mm3,mm3
  pxor mm2,mm2
  pcmpeqw mm3,mm0
  pcmpgtw mm2,mm0
  pmullw mm0,mm7                    ; * 2 * quant
  pandn mm3,mm6
  movq mm1,[_EAX+8]
  psubw mm0,mm2 
  pxor mm2,mm3
  pxor mm3,mm3
  paddw mm0,mm2
  pxor mm2,mm2
  pcmpgtw mm3,mm1
  pcmpeqw mm2,mm1
  pmullw mm1,mm7
  pandn mm2,mm6
  psubw mm1,mm3
  pxor mm3,mm2
  paddw mm1,mm3
  
  pminsw mm0,mm4
  pminsw mm1,mm4
  pmaxsw mm0,mm5
  pmaxsw mm1,mm5
    
  movq [TMP1],mm0
  lea _EAX,[_EAX+16]
  movq [TMP1+8],mm1
 
  dec TMP0
  lea TMP1,[TMP1+16]
  jne .loop

  xor _EAX, _EAX              ; return 0
  ret
ENDFUNC

 
;-----------------------------------------------------------------------------
;
; uint32_t dequant_h263_inter_sse2(int16_t * data,
;                                  const int16_t * const coeff,
;                                  const uint32_t quant,
;                                  const uint16_t *mpeg_matrices);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
dequant_h263_inter_sse2:
  PUSH_XMM6_XMM7
  
  mov TMP0, prm3                 ; quant
  mov _EAX, prm2                 ; coeff

  lea TMP1, [mmx_quant]
  movq xmm6, [TMP1 + TMP0*8]    ; quant
  inc TMP0
  pcmpeqw xmm5,xmm5
  and TMP0,1
  movlhps xmm6,xmm6
  movd xmm0,TMP0d
  movdqa xmm7,xmm6
  pshuflw xmm0,xmm0,0
  movdqa xmm4,xmm5
  mov TMP1, prm1                 ; data
  movlhps xmm0,xmm0
  paddw xmm7,xmm7
  psubw xmm6,xmm0
  psrlw xmm4,5   ; 2047
  mov TMP0,4
  pxor xmm5,xmm4 ; mm5=-2048
  
.loop:
  movdqa xmm0,[_EAX] 
  pxor xmm3,xmm3
  pxor xmm2,xmm2
  pcmpeqw xmm3,xmm0
  pcmpgtw xmm2,xmm0
  pmullw xmm0,xmm7      ; * 2 * quant
  pandn xmm3,xmm6
  movdqa xmm1,[_EAX+16]
  psubw xmm0,xmm2 
  pxor xmm2,xmm3
  pxor xmm3,xmm3
  paddw xmm0,xmm2
  pxor xmm2,xmm2
  pcmpgtw xmm3,xmm1
  pcmpeqw xmm2,xmm1
  pmullw xmm1,xmm7
  pandn xmm2,xmm6
  psubw xmm1,xmm3
  pxor xmm3,xmm2
  paddw xmm1,xmm3
  
  pminsw xmm0,xmm4
  pminsw xmm1,xmm4
  pmaxsw xmm0,xmm5
  pmaxsw xmm1,xmm5
    
  movdqa [TMP1],xmm0
  lea _EAX,[_EAX+32]
  movdqa [TMP1+16],xmm1
 
  dec TMP0
  lea TMP1,[TMP1+32]
  jne .loop

  xor _EAX, _EAX              ; return 0

  POP_XMM6_XMM7
  ret
ENDFUNC

NON_EXEC_STACK
