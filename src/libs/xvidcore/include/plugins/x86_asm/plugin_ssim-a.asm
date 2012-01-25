;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - optimized SSIM routines -
; *
; *  Copyright(C) 2006 Johannes Reinhardt <johannes.reinhardt@gmx.de>
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
; *
; ***************************************************************************/

BITS 32

%include "nasm.inc"

%macro ACC_ROW 2
	movq %1,[    TMP0]
	movq %2,[TMP0+TMP1]
	psadbw %1,mm0
	psadbw %2,mm0
        lea TMP0, [TMP0+2*TMP1]
        paddw  %1, %2
%endmacro

%macro CONSIM_1x8_SSE2 0
	movdqu xmm0,[TMP0]
	movdqu xmm1,[TMP1]

	;unpack to words
	punpcklbw xmm0,xmm2
	punpcklbw xmm1,xmm2

	movaps xmm3,xmm0
	movaps xmm4,xmm1

	pmaddwd xmm0,xmm0;orig
	pmaddwd xmm1,xmm1;comp
	pmaddwd xmm3,xmm4;corr

	paddd xmm5,xmm0
	paddd xmm6,xmm1
	paddd xmm7,xmm3
%endmacro

%macro CONSIM_1x8_MMX 0
 	movq mm0,[TMP0];orig
 	movq mm1,[TMP1];comp

 	;unpack low half of qw to words
 	punpcklbw mm0,mm2
 	punpcklbw mm1,mm2

 	movq mm3,mm0
 	pmaddwd	mm3,mm0
 	paddd mm5,mm3;

 	movq mm4,mm1
 	pmaddwd	mm4,mm1
 	paddd mm6,mm4;

	pmaddwd mm1,mm0
	paddd mm7,mm1

 	movq mm0,[TMP0];orig
 	movq mm1,[TMP1];comp

 	;unpack high half of qw to words
 	punpckhbw mm0,mm2
 	punpckhbw mm1,mm2

 	movq mm3,mm0
 	pmaddwd	mm3,mm0
 	paddd mm5,mm3;

 	movq mm4,mm1
 	pmaddwd	mm4,mm1
 	paddd mm6,mm4;

	pmaddwd mm1,mm0
	paddd mm7,mm1
%endmacro

%macro CONSIM_WRITEOUT 3
	mov eax,prm4d;lumo
	mul eax; lumo^2
        add eax, 32
	shr eax, 6; 64*lum0^2
	movd TMP0d,%1
	sub TMP0d, eax

	mov TMP1,prm6; pdevo
	mov dword [TMP1],TMP0d

	mov eax,prm5d ;lumc
	mul eax; lumc^2
        add eax, 32
	shr eax, 6; 64*lumc^2
	movd TMP0d,%2
	sub TMP0d, eax

	mov TMP1,prm7; pdevc
	mov dword [TMP1],TMP0d

	mov eax,prm4d;lumo
	mul prm5d; lumo*lumc, should fit in _EAX
        add eax, 32
	shr eax, 6; 64*lumo*lumc
	movd TMP0d,%3
	sub TMP0d, eax

	mov TMP1,prm8; pcorr
	mov dword [TMP1],TMP0d
%endmacro


TEXT

cglobal lum_8x8_mmx
cglobal consim_sse2
cglobal consim_mmx

;int lum_8x8_c(uint8_t* ptr, uint32_t stride)

ALIGN SECTION_ALIGN
lum_8x8_mmx:
	mov TMP0, prm1 ;ptr
	mov TMP1, prm2 ;stride

	pxor mm0,mm0

	ACC_ROW mm1, mm2
	
	ACC_ROW mm3, mm4

	ACC_ROW mm5, mm6

	ACC_ROW mm7, mm4

	paddw mm1, mm3
	paddw mm5, mm7
	paddw mm1, mm5

	movd eax,mm1
	ret
ENDFUNC

ALIGN SECTION_ALIGN
consim_sse2:
	PUSH_XMM6_XMM7
	mov TMP0,prm1 ;ptro
	mov TMP1,prm2 ;ptrc
	mov _EAX, prm3 ;stride

	pxor xmm2,xmm2;null vektor
	pxor xmm5,xmm5;devo
	pxor xmm6,xmm6;devc
	pxor xmm7,xmm7;corr

	;broadcast lumo/c
	punpcklbw xmm6,xmm6
	punpcklwd xmm6,xmm6
	pshufd xmm6,xmm6,00000000b;or shufps
	punpcklbw xmm7,xmm7
	punpcklwd xmm7,xmm7
	pshufd xmm7,xmm7,00000000b

	CONSIM_1x8_SSE2
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_SSE2
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_SSE2
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_SSE2
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_SSE2
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_SSE2
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_SSE2
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_SSE2

	;accumulate xmm5-7
	pshufd     xmm0, xmm5, 0x0E
	paddd      xmm5, xmm0
	pshufd     xmm0, xmm5, 0x01
	paddd      xmm5, xmm0

	pshufd     xmm1, xmm6, 0x0E
	paddd      xmm6, xmm1
	pshufd     xmm1, xmm6, 0x01
	paddd      xmm6, xmm1

	pshufd     xmm2, xmm7, 0x0E
	paddd      xmm7, xmm2
	pshufd     xmm2, xmm7, 0x01
	paddd      xmm7, xmm2

	CONSIM_WRITEOUT xmm5,xmm6,xmm7

	POP_XMM6_XMM7
	ret
ENDFUNC





ALIGN SECTION_ALIGN
consim_mmx:
	mov TMP0,prm1 ;ptro
	mov TMP1,prm2 ;ptrc
	mov _EAX,prm3;stride
	pxor mm2,mm2;null
	pxor mm5,mm5;devo
	pxor mm6,mm6;devc
	pxor mm7,mm7;corr

	CONSIM_1x8_MMX
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_MMX
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_MMX
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_MMX
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_MMX
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_MMX
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_MMX
	add TMP0,_EAX
	add TMP1,_EAX
	CONSIM_1x8_MMX

	movq mm0,mm5
	psrlq mm0,32
	paddd mm5,mm0
	movq mm1,mm6
	psrlq mm1,32
	paddd mm6,mm1
	movq mm2,mm7
	psrlq mm2,32
	paddd mm7,mm2

	CONSIM_WRITEOUT mm5,mm6,mm7
	ret
ENDFUNC

NON_EXEC_STACK
