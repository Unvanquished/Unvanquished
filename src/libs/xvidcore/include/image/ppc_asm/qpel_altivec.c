/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - QPel interpolation with altivec optimization -
 *
 *  Copyright(C) 2004 Christoph NŠgeli <chn@kbw.ch>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: qpel_altivec.c,v 1.2 2004/12/09 23:02:54 edgomez Exp $
 *
 ****************************************************************************/



#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "../../portab.h"

#undef DEBUG
#include <stdio.h>

static const vector signed char FIR_Tab_16[17] = {
	(vector signed char)AVV( 14, -3,  2, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 ),
	(vector signed char)AVV( 23, 19, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 ),
	(vector signed char)AVV( -7, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 ),
	(vector signed char)AVV(  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0 ),
	(vector signed char)AVV( -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0,  0 ),
	(vector signed char)AVV(  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0 ),
	(vector signed char)AVV(  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0 ),
	(vector signed char)AVV(  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0 ),
	(vector signed char)AVV(  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0 ),
	(vector signed char)AVV(  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0 ),
	(vector signed char)AVV(  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0 ),
	(vector signed char)AVV(  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0 ),
	(vector signed char)AVV(  0,  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1 ),
	(vector signed char)AVV(  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3 ),
	(vector signed char)AVV(  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -7 ),
	(vector signed char)AVV(  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 19, 23 ),
	(vector signed char)AVV(  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  2, -3, 14 )
};

static const vector signed short FIR_Tab_8[9] = {
	(vector signed short)AVV( 14, -3,  2, -1,  0,  0,  0,  0 ),
	(vector signed short)AVV( 23, 19, -6,  3, -1,  0,  0,  0 ),
	(vector signed short)AVV( -7, 20, 20, -6,  3, -1,  0,  0 ),
	(vector signed short)AVV(  3, -6, 20, 20, -6,  3, -1,  0 ),
	(vector signed short)AVV( -1,  3, -6, 20, 20, -6,  3, -1 ),
	(vector signed short)AVV(  0, -1,  3, -6, 20, 20, -6,  3 ),
	(vector signed short)AVV(  0,  0, -1,  3, -6, 20, 20, -7 ),
	(vector signed short)AVV(  0,  0,  0, -1,  3, -6, 19, 23 ),
	(vector signed short)AVV(  0,  0,  0,  0, -1,  2, -3, 14 )
};

/* Processing with FIR_Tab */
#define PROCESS_FIR_16(x) \
	firs = FIR_Tab_16[x];\
	\
	tmp = vec_splat(vec_src,(x));\
	sums1 = vec_mladd( (vector signed short)vec_mergeh(ox00, tmp), vec_unpackh(firs), sums1 );\
	sums2 = vec_mladd( (vector signed short)vec_mergel(ox00, tmp), vec_unpackl(firs), sums2 )

#define PROCESS_FIR_8(x) \
	firs = FIR_Tab_8[x];\
	tmp = (vector signed short)vec_mergeh( ox00, vec_splat(vec_src,x) );\
	sums = vec_mladd(firs,tmp,sums)

#define NOTHING() \
	/* Nothing here */

#pragma mark -

/* "Postprocessing" macros */

#define AVRG_16() \
	sums1 = (vector signed short)vec_mergeh(ox00,tmp);\
	sums2 = (vector signed short)vec_mergel(ox00,tmp);\
	sums1 = vec_add(sums1, (vector signed short)vec_mergeh(ox00,vec_src) );\
	sums2 = vec_add(sums2, (vector signed short)vec_mergel(ox00,vec_src) );\
	tmp = (vector unsigned char)vec_splat_u16(1);\
	sums1 = vec_add(sums1, (vector signed short)tmp);\
	sums2 = vec_add(sums2, (vector signed short)tmp);\
	sums1 = vec_sub(sums1, vec_rnd);\
	sums2 = vec_sub(sums2, vec_rnd);\
	sums1 = vec_sra(sums1, (vector unsigned short)tmp);\
	sums2 = vec_sra(sums2, (vector unsigned short)tmp);\
	tmp = vec_packsu(sums1,sums2)

#define AVRG_UP_16_H() \
	vec_src = vec_perm(vec_ld(1,Src),vec_ld(17,Src),vec_lvsl(1,Src));\
	AVRG_16()

#define AVRG_UP_16_V() \
	((unsigned char*)&vec_src)[0] = Src[16 * BpS];\
	vec_src = vec_perm(vec_src,vec_src,vec_lvsl(1,(unsigned char*)0));\
	AVRG_16()


#define AVRG_8() \
	sums = (vector signed short)vec_mergeh(ox00, st);\
	sums = vec_add(sums, (vector signed short)vec_mergeh(ox00, vec_src));\
	st = (vector unsigned char)vec_splat_u16(1);\
	sums = vec_add(sums, (vector signed short)st);\
	sums = vec_sub(sums, vec_rnd);\
	sums = vec_sra(sums, (vector unsigned short)st);\
	st = vec_packsu(sums,sums)


#define AVRG_UP_8() \
	vec_src = vec_perm(vec_src, vec_src, vec_lvsl(1,(unsigned char*)0));\
	AVRG_8()

#pragma mark -

/* Postprocessing Macros for the Pass_16 Add functions */
#define ADD_16_H()\
	tmp = vec_avg(tmp, vec_perm(vec_ld(0,Dst),vec_ld(16,Dst),vec_lvsl(0,Dst)))

#define AVRG_ADD_16_H()\
	AVRG_16();\
	ADD_16_H()

#define AVRG_UP_ADD_16_H()\
	AVRG_UP_16_H();\
	ADD_16_H()

#define ADD_16_V() \
	for(j = 0; j < 16; j++)\
		((unsigned char*)&vec_src)[j] = Dst[j * BpS];\
	tmp = vec_avg(tmp, vec_src)

#define AVRG_ADD_16_V()\
	AVRG_16();\
	ADD_16_V()

#define AVRG_UP_ADD_16_V()\
	AVRG_UP_16_V();\
	ADD_16_V()

#pragma mark -

/* Postprocessing Macros for the Pass_8 Add functions */
#define ADD_8_H()\
	sums = (vector signed short)vec_mergeh(ox00, st);\
	tmp = (vector signed short)vec_mergeh(ox00,vec_perm(vec_ld(0,Dst),vec_ld(16,Dst),vec_lvsl(0,Dst)));\
	sums = vec_avg(sums,tmp);\
	st = vec_packsu(sums,sums)

#define AVRG_ADD_8_H()\
	AVRG_8();\
	ADD_8_H()

#define AVRG_UP_ADD_8_H()\
	AVRG_UP_8();\
	ADD_8_H()	


#define ADD_8_V()\
	for(j = 0; j < 8; j++)\
		((short*)&tmp)[j] = (short)Dst[j * BpS];\
	sums = (vector signed short)vec_mergeh(ox00,st);\
	sums = vec_avg(sums,tmp);\
	st = vec_packsu(sums,sums)

#define AVRG_ADD_8_V()\
	AVRG_8();\
	ADD_8_V()

#define AVRG_UP_ADD_8_V()\
	AVRG_UP_8();\
	ADD_8_V()

#pragma mark -

/* Load/Store Macros */
#define LOAD_H() \
	vec_src = vec_perm(vec_ld(0,Src),vec_ld(16,Src),vec_lvsl(0,Src))

#define LOAD_V_16() \
	for(j = 0; j < 16; j++)\
		((unsigned char*)&vec_src)[j] = Src[j * BpS]

#define LOAD_V_8() \
	for(j = 0; j <= 8; j++)\
		((unsigned char*)&vec_src)[j] = Src[j * BpS]

#define LOAD_UP_V_8() \
	for(j = 0; j <= 9; j++)\
		((unsigned char*)&vec_src)[j] = Src[j * BpS]

#define STORE_H_16() \
	mask = vec_lvsr(0,Dst);\
	tmp = vec_perm(tmp,tmp,mask);\
	mask = vec_perm(oxFF, ox00, mask);\
	tmp1 = vec_sel(tmp, vec_ld(0,Dst), mask);\
	vec_st(tmp1, 0, Dst);\
	tmp1 = vec_sel(vec_ld(16,Dst), tmp, mask);\
	vec_st(tmp1, 16, Dst)

#define STORE_V_16() \
	for(j = 0; j < 16; j++)\
		Dst[j * BpS] = ((unsigned char*)&tmp)[j]


#define STORE_H_8() \
	mask = vec_perm(mask_00ff, mask_00ff, vec_lvsr(0,Dst) );\
	st = vec_sel(st, vec_ld(0,Dst), mask);\
	vec_st(st, 0, Dst)

#define STORE_V_8() \
	for(j = 0; j < 8; j++)\
		Dst[j * BpS] = ((unsigned char*)&st)[j]


#pragma mark -

/* Additional variable declaration/initialization macros */

#define VARS_H_16()\
	register vector unsigned char mask;\
	register vector unsigned char tmp1


#define VARS_V()\
	register unsigned j


#define VARS_H_8()\
	register vector unsigned char mask_00ff;\
	register vector unsigned char mask;\
	mask_00ff = vec_pack(vec_splat_u16(0),vec_splat_u16(-1))

#pragma mark -

/* Function macros */

#define MAKE_PASS_16(NAME, POSTPROC, ADDITIONAL_VARS, LOAD_SOURCE, STORE_DEST, NEXT_PIXEL, NEXT_LINE) \
void \
NAME(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t Rnd) \
{\
	register vector signed short sums1,sums2;\
	register vector unsigned char ox00;\
	register vector unsigned char oxFF;\
	register vector signed char firs;\
	vector signed short vec_rnd;\
	vector signed short s16Rnd;\
	vector unsigned char vec_src;\
	vector unsigned char tmp;\
	\
	register unsigned c;\
	\
	ADDITIONAL_VARS();\
	\
	ox00 = vec_splat_u8(0);\
	oxFF = vec_splat_u8(-1);\
	\
	*((short*)&vec_rnd) = (short)Rnd;\
	vec_rnd = vec_splat(vec_rnd,0);\
	s16Rnd = vec_add(vec_splat_s16(8),vec_splat_s16(8));\
	s16Rnd = vec_sub(s16Rnd, vec_rnd);\
	\
	c = ((1 << 24) | (16 << 16) | BpS);\
	\
	while(H-- > 0) {\
		\
		vec_dst(Src, c, 2);\
		\
		sums1 = s16Rnd;\
		sums2 = s16Rnd;\
		\
		LOAD_SOURCE();\
		\
		PROCESS_FIR_16(0);\
		PROCESS_FIR_16(1);\
		PROCESS_FIR_16(2);\
		PROCESS_FIR_16(3);\
		\
		PROCESS_FIR_16(4);\
		PROCESS_FIR_16(5);\
		PROCESS_FIR_16(6);\
		PROCESS_FIR_16(7);\
		\
		PROCESS_FIR_16(8);\
		PROCESS_FIR_16(9);\
		PROCESS_FIR_16(10);\
		PROCESS_FIR_16(11);\
		\
		PROCESS_FIR_16(12);\
		PROCESS_FIR_16(13);\
		PROCESS_FIR_16(14);\
		PROCESS_FIR_16(15);\
		\
		firs = FIR_Tab_16[16];\
		*((uint8_t*)&tmp) = Src[16*NEXT_PIXEL];\
		tmp = vec_splat(tmp,0);\
		\
		sums1 = vec_mladd( (vector signed short)vec_mergeh(ox00,tmp),vec_unpackh(firs),sums1 );\
		sums2 = vec_mladd( (vector signed short)vec_mergel(ox00,tmp),vec_unpackl(firs),sums2 );\
		\
		tmp = (vector unsigned char)vec_splat_u16(5);\
		sums1 = vec_sra(sums1,(vector unsigned short)tmp);\
		sums2 = vec_sra(sums2,(vector unsigned short)tmp);\
		tmp = vec_packsu(sums1,sums2);\
		\
		POSTPROC();\
		\
		STORE_DEST();\
		\
		Src += NEXT_LINE;\
		Dst += NEXT_LINE;\
	}\
	vec_dss(2);\
}

#define MAKE_PASS_8(NAME,POSTPROC,ADDITIONAL_VARS, LOAD_SOURCE, STORE_DEST, INC) \
void \
NAME(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t Rnd)\
{\
	register vector signed short sums;\
	register vector signed short firs;\
	register vector unsigned char ox00;\
	vector signed short tmp;\
	vector signed short vec_rnd;\
	vector signed short vec_rnd16;\
	vector unsigned char vec_src;\
	vector unsigned char st;\
	\
	ADDITIONAL_VARS();\
	\
	ox00 = vec_splat_u8(0);\
	\
	*((short*)&vec_rnd) = (short)Rnd;\
	vec_rnd = vec_splat(vec_rnd,0);\
	vec_rnd16 = vec_sub( vec_add(vec_splat_s16(8),vec_splat_s16(8)), vec_rnd );\
	\
	while(H-- > 0) {\
		\
		sums = vec_rnd16;\
		LOAD_SOURCE();\
		\
		PROCESS_FIR_8(0);\
		PROCESS_FIR_8(1);\
		PROCESS_FIR_8(2);\
		PROCESS_FIR_8(3);\
		\
		PROCESS_FIR_8(4);\
		PROCESS_FIR_8(5);\
		PROCESS_FIR_8(6);\
		PROCESS_FIR_8(7);\
		\
		PROCESS_FIR_8(8);\
		\
		sums = vec_sra(sums, vec_splat_u16(5));\
		st = vec_packsu(sums,sums);\
		\
		POSTPROC();\
		\
		STORE_DEST();\
		\
		Src += INC;\
		Dst += INC;\
	}\
}


/* Create the actual Functions
 ***************************************/

/* These functions assume no alignment */
MAKE_PASS_16(H_Pass_16_Altivec_C, NOTHING, VARS_H_16, LOAD_H, STORE_H_16, 1, BpS)
MAKE_PASS_16(H_Pass_Avrg_16_Altivec_C, AVRG_16, VARS_H_16, LOAD_H, STORE_H_16, 1, BpS)
MAKE_PASS_16(H_Pass_Avrg_Up_16_Altivec_C, AVRG_UP_16_H, VARS_H_16, LOAD_H, STORE_H_16, 1, BpS)

MAKE_PASS_16(V_Pass_16_Altivec_C, NOTHING, VARS_V, LOAD_V_16, STORE_V_16, BpS, 1)
MAKE_PASS_16(V_Pass_Avrg_16_Altivec_C, AVRG_16, VARS_V, LOAD_V_16, STORE_V_16, BpS, 1)
MAKE_PASS_16(V_Pass_Avrg_Up_16_Altivec_C, AVRG_UP_16_V, VARS_V, LOAD_V_16, STORE_V_16, BpS, 1)


/* These functions assume:
 *	Dst: 8 Byte aligned
 *	BpS: Multiple of 8
 */
MAKE_PASS_8(H_Pass_8_Altivec_C, NOTHING, VARS_H_8, LOAD_H, STORE_H_8, BpS)
MAKE_PASS_8(H_Pass_Avrg_8_Altivec_C, AVRG_8, VARS_H_8, LOAD_H, STORE_H_8, BpS)
MAKE_PASS_8(H_Pass_Avrg_Up_8_Altivec_C, AVRG_UP_8, VARS_H_8, LOAD_H, STORE_H_8, BpS)

/* These functions assume no alignment */
MAKE_PASS_8(V_Pass_8_Altivec_C, NOTHING, VARS_V, LOAD_V_8, STORE_V_8, 1)
MAKE_PASS_8(V_Pass_Avrg_8_Altivec_C, AVRG_8, VARS_V, LOAD_V_8, STORE_V_8, 1)
MAKE_PASS_8(V_Pass_Avrg_Up_8_Altivec_C, AVRG_UP_8, VARS_V, LOAD_UP_V_8, STORE_V_8, 1)


/* These functions assume no alignment */
MAKE_PASS_16(H_Pass_16_Add_Altivec_C, ADD_16_H, VARS_H_16, LOAD_H, STORE_H_16, 1, BpS)
MAKE_PASS_16(H_Pass_Avrg_16_Add_Altivec_C, AVRG_ADD_16_H, VARS_H_16, LOAD_H, STORE_H_16, 1, BpS)
MAKE_PASS_16(H_Pass_Avrg_Up_16_Add_Altivec_C, AVRG_UP_ADD_16_H, VARS_H_16, LOAD_H, STORE_H_16, 1, BpS)

MAKE_PASS_16(V_Pass_16_Add_Altivec_C, ADD_16_V, VARS_V, LOAD_V_16, STORE_V_16, BpS, 1)
MAKE_PASS_16(V_Pass_Avrg_16_Add_Altivec_C, AVRG_ADD_16_V, VARS_V, LOAD_V_16, STORE_V_16, BpS, 1)
MAKE_PASS_16(V_Pass_Avrg_Up_16_Add_Altivec_C, AVRG_UP_ADD_16_V, VARS_V, LOAD_V_16, STORE_V_16, BpS, 1)


/* These functions assume:
 *	Dst: 8 Byte aligned
 *	BpS: Multiple of 8
 */
MAKE_PASS_8(H_Pass_8_Add_Altivec_C, ADD_8_H, VARS_H_8, LOAD_H, STORE_H_8, BpS)
MAKE_PASS_8(H_Pass_Avrg_8_Add_Altivec_C, AVRG_ADD_8_H, VARS_H_8, LOAD_H, STORE_H_8, BpS)
MAKE_PASS_8(H_Pass_Avrg_Up_8_Add_Altivec_C, AVRG_UP_ADD_8_H, VARS_H_8, LOAD_H, STORE_H_8, BpS)


/* These functions assume no alignment */
MAKE_PASS_8(V_Pass_8_Add_Altivec_C, ADD_8_V, VARS_V, LOAD_V_8, STORE_V_8, 1)
MAKE_PASS_8(V_Pass_Avrg_8_Add_Altivec_C, AVRG_ADD_8_V, VARS_V, LOAD_V_8, STORE_V_8, 1)
MAKE_PASS_8(V_Pass_Avrg_Up_8_Add_Altivec_C, AVRG_UP_ADD_8_V, VARS_V, LOAD_UP_V_8, STORE_V_8, 1)
