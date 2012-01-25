/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - QPel interpolation -
 *
 *  Copyright(C) 2003 Pascal Massimino <skal@planet-d.net>
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
 * $Id: qpel.c,v 1.9 2008/11/26 01:04:34 Isibaar Exp $
 *
 ****************************************************************************/

#ifndef XVID_AUTO_INCLUDE

#include <stdio.h>

#include "../portab.h"
#include "qpel.h"

/* Quarterpel FIR definition
 ****************************************************************************/

static const int32_t FIR_Tab_8[9][8] = {
	{ 14, -3,  2, -1,  0,  0,  0,  0 },
	{ 23, 19, -6,  3, -1,  0,  0,  0 },
	{ -7, 20, 20, -6,  3, -1,  0,  0 },
	{  3, -6, 20, 20, -6,  3, -1,  0 },
	{ -1,  3, -6, 20, 20, -6,  3, -1 },
	{  0, -1,  3, -6, 20, 20, -6,  3 },
	{  0,  0, -1,  3, -6, 20, 20, -7 },
	{  0,  0,  0, -1,  3, -6, 19, 23 },
	{  0,  0,  0,  0, -1,  2, -3, 14 }
};

static const int32_t FIR_Tab_16[17][16] = {
	{ 14, -3,  2, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{ 23, 19, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{ -7, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{ -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0,  0 },
	{  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0,  0 },
	{  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0,  0 },
	{  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0,  0 },
	{  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0,  0 },
	{  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1,  0 },
	{  0,  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3, -1 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -6,  3 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 20, 20, -7 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  3, -6, 19, 23 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  2, -3, 14 }
};

/* Implementation
 ****************************************************************************/

#define XVID_AUTO_INCLUDE
/* First auto include this file to generate reference code for SIMD versions
 * This set of functions are good for educational purpose, because they're
 * straightforward to understand, use loops and so on... But obviously they
 * sux when it comes to speed */
#define REFERENCE_CODE

/* 16x? filters */

#define SIZE  16
#define TABLE FIR_Tab_16

#define STORE(d,s)  (d) = (s)
#define FUNC_H      H_Pass_16_C_ref
#define FUNC_V      V_Pass_16_C_ref
#define FUNC_HA     H_Pass_Avrg_16_C_ref
#define FUNC_VA     V_Pass_Avrg_16_C_ref
#define FUNC_HA_UP  H_Pass_Avrg_Up_16_C_ref
#define FUNC_VA_UP  V_Pass_Avrg_Up_16_C_ref

#include "qpel.c"   /* self-include ourself */

/* note: B-frame always uses Rnd=0... */
#define STORE(d,s)  (d) = ( (s)+(d)+1 ) >> 1
#define FUNC_H      H_Pass_16_Add_C_ref
#define FUNC_V      V_Pass_16_Add_C_ref
#define FUNC_HA     H_Pass_Avrg_16_Add_C_ref
#define FUNC_VA     V_Pass_Avrg_16_Add_C_ref
#define FUNC_HA_UP  H_Pass_Avrg_Up_16_Add_C_ref
#define FUNC_VA_UP  V_Pass_Avrg_Up_16_Add_C_ref

#include "qpel.c"   /* self-include ourself */

#undef SIZE
#undef TABLE

/* 8x? filters */

#define SIZE  8
#define TABLE FIR_Tab_8

#define STORE(d,s)  (d) = (s)
#define FUNC_H      H_Pass_8_C_ref
#define FUNC_V      V_Pass_8_C_ref
#define FUNC_HA     H_Pass_Avrg_8_C_ref
#define FUNC_VA     V_Pass_Avrg_8_C_ref
#define FUNC_HA_UP  H_Pass_Avrg_Up_8_C_ref
#define FUNC_VA_UP  V_Pass_Avrg_Up_8_C_ref

#include "qpel.c"   /* self-include ourself */

/* note: B-frame always uses Rnd=0... */
#define STORE(d,s)  (d) = ( (s)+(d)+1 ) >> 1
#define FUNC_H      H_Pass_8_Add_C_ref
#define FUNC_V      V_Pass_8_Add_C_ref
#define FUNC_HA     H_Pass_Avrg_8_Add_C_ref
#define FUNC_VA     V_Pass_Avrg_8_Add_C_ref
#define FUNC_HA_UP  H_Pass_Avrg_Up_8_Add_C_ref
#define FUNC_VA_UP  V_Pass_Avrg_Up_8_Add_C_ref

#include "qpel.c"   /* self-include ourself */

#undef SIZE
#undef TABLE

/* Then we define more optimized C version where loops are unrolled, where
 * FIR coeffcients are not read from memory but are hardcoded in instructions
 * They should be faster */
#undef REFERENCE_CODE

/* 16x? filters */

#define SIZE  16

#define STORE(d,s)  (d) = (s)
#define FUNC_H      H_Pass_16_C
#define FUNC_V      V_Pass_16_C
#define FUNC_HA     H_Pass_Avrg_16_C
#define FUNC_VA     V_Pass_Avrg_16_C
#define FUNC_HA_UP  H_Pass_Avrg_Up_16_C
#define FUNC_VA_UP  V_Pass_Avrg_Up_16_C

#include "qpel.c"   /* self-include ourself */

/* note: B-frame always uses Rnd=0... */
#define STORE(d,s)  (d) = ( (s)+(d)+1 ) >> 1
#define FUNC_H      H_Pass_16_Add_C
#define FUNC_V      V_Pass_16_Add_C
#define FUNC_HA     H_Pass_Avrg_16_Add_C
#define FUNC_VA     V_Pass_Avrg_16_Add_C
#define FUNC_HA_UP  H_Pass_Avrg_Up_16_Add_C
#define FUNC_VA_UP  V_Pass_Avrg_Up_16_Add_C

#include "qpel.c"   /* self-include ourself */

#undef SIZE
#undef TABLE

/* 8x? filters */

#define SIZE  8
#define TABLE FIR_Tab_8

#define STORE(d,s)  (d) = (s)
#define FUNC_H      H_Pass_8_C
#define FUNC_V      V_Pass_8_C
#define FUNC_HA     H_Pass_Avrg_8_C
#define FUNC_VA     V_Pass_Avrg_8_C
#define FUNC_HA_UP  H_Pass_Avrg_Up_8_C
#define FUNC_VA_UP  V_Pass_Avrg_Up_8_C

#include "qpel.c"   /* self-include ourself */

/* note: B-frame always uses Rnd=0... */
#define STORE(d,s)  (d) = ( (s)+(d)+1 ) >> 1
#define FUNC_H      H_Pass_8_Add_C
#define FUNC_V      V_Pass_8_Add_C
#define FUNC_HA     H_Pass_Avrg_8_Add_C
#define FUNC_VA     V_Pass_Avrg_8_Add_C
#define FUNC_HA_UP  H_Pass_Avrg_Up_8_Add_C
#define FUNC_VA_UP  V_Pass_Avrg_Up_8_Add_C

#include "qpel.c"   /* self-include ourself */

#undef SIZE
#undef TABLE
#undef XVID_AUTO_INCLUDE

/* Global scope hooks
 ****************************************************************************/

XVID_QP_FUNCS *xvid_QP_Funcs = NULL;
XVID_QP_FUNCS *xvid_QP_Add_Funcs = NULL;

/* Reference plain C impl. declaration
 ****************************************************************************/

XVID_QP_FUNCS xvid_QP_Funcs_C_ref = {
	H_Pass_16_C_ref, H_Pass_Avrg_16_C_ref, H_Pass_Avrg_Up_16_C_ref,
	V_Pass_16_C_ref, V_Pass_Avrg_16_C_ref, V_Pass_Avrg_Up_16_C_ref,

	H_Pass_8_C_ref, H_Pass_Avrg_8_C_ref, H_Pass_Avrg_Up_8_C_ref,
	V_Pass_8_C_ref, V_Pass_Avrg_8_C_ref, V_Pass_Avrg_Up_8_C_ref
};

XVID_QP_FUNCS xvid_QP_Add_Funcs_C_ref = {
	H_Pass_16_Add_C_ref, H_Pass_Avrg_16_Add_C_ref, H_Pass_Avrg_Up_16_Add_C_ref,
	V_Pass_16_Add_C_ref, V_Pass_Avrg_16_Add_C_ref, V_Pass_Avrg_Up_16_Add_C_ref,

	H_Pass_8_Add_C_ref, H_Pass_Avrg_8_Add_C_ref, H_Pass_Avrg_Up_8_Add_C_ref,
	V_Pass_8_Add_C_ref, V_Pass_Avrg_8_Add_C_ref, V_Pass_Avrg_Up_8_Add_C_ref
};

/* Plain C impl. declaration (faster than ref one)
 ****************************************************************************/

XVID_QP_FUNCS xvid_QP_Funcs_C = {
	H_Pass_16_C, H_Pass_Avrg_16_C, H_Pass_Avrg_Up_16_C,
	V_Pass_16_C, V_Pass_Avrg_16_C, V_Pass_Avrg_Up_16_C,

	H_Pass_8_C, H_Pass_Avrg_8_C, H_Pass_Avrg_Up_8_C,
	V_Pass_8_C, V_Pass_Avrg_8_C, V_Pass_Avrg_Up_8_C
};

XVID_QP_FUNCS xvid_QP_Add_Funcs_C = {
	H_Pass_16_Add_C, H_Pass_Avrg_16_Add_C, H_Pass_Avrg_Up_16_Add_C,
	V_Pass_16_Add_C, V_Pass_Avrg_16_Add_C, V_Pass_Avrg_Up_16_Add_C,

	H_Pass_8_Add_C, H_Pass_Avrg_8_Add_C, H_Pass_Avrg_Up_8_Add_C,
	V_Pass_8_Add_C, V_Pass_Avrg_8_Add_C, V_Pass_Avrg_Up_8_Add_C
};

/* mmx impl. declaration (see. qpel_mmx.asm
 ****************************************************************************/

#if defined (ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Avrg_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Avrg_Up_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Avrg_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Avrg_Up_16_mmx);

extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_8_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Avrg_8_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Avrg_Up_8_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_8_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Avrg_8_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Avrg_Up_8_mmx);

extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Add_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Avrg_Add_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Avrg_Up_Add_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Add_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Avrg_Add_16_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Avrg_Up_Add_16_mmx);

extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_8_Add_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Avrg_8_Add_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_H_Pass_Avrg_Up_8_Add_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_8_Add_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Avrg_8_Add_mmx);
extern XVID_QP_PASS_SIGNATURE(xvid_V_Pass_Avrg_Up_8_Add_mmx);

XVID_QP_FUNCS xvid_QP_Funcs_mmx = {
	xvid_H_Pass_16_mmx, xvid_H_Pass_Avrg_16_mmx, xvid_H_Pass_Avrg_Up_16_mmx,
	xvid_V_Pass_16_mmx, xvid_V_Pass_Avrg_16_mmx, xvid_V_Pass_Avrg_Up_16_mmx,

	xvid_H_Pass_8_mmx, xvid_H_Pass_Avrg_8_mmx, xvid_H_Pass_Avrg_Up_8_mmx,
	xvid_V_Pass_8_mmx, xvid_V_Pass_Avrg_8_mmx, xvid_V_Pass_Avrg_Up_8_mmx
};

XVID_QP_FUNCS xvid_QP_Add_Funcs_mmx = {
	xvid_H_Pass_Add_16_mmx, xvid_H_Pass_Avrg_Add_16_mmx, xvid_H_Pass_Avrg_Up_Add_16_mmx,
	xvid_V_Pass_Add_16_mmx, xvid_V_Pass_Avrg_Add_16_mmx, xvid_V_Pass_Avrg_Up_Add_16_mmx,

	xvid_H_Pass_8_Add_mmx, xvid_H_Pass_Avrg_8_Add_mmx, xvid_H_Pass_Avrg_Up_8_Add_mmx,
	xvid_V_Pass_8_Add_mmx, xvid_V_Pass_Avrg_8_Add_mmx, xvid_V_Pass_Avrg_Up_8_Add_mmx,
};
#endif /* ARCH_IS_IA32 */


/* altivec impl. declaration (see qpel_altivec.c)
 ****************************************************************************/

#ifdef ARCH_IS_PPC

extern XVID_QP_PASS_SIGNATURE(H_Pass_16_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(H_Pass_Avrg_16_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(H_Pass_Avrg_Up_16_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_16_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_Avrg_16_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_Avrg_Up_16_Altivec_C);

extern XVID_QP_PASS_SIGNATURE(H_Pass_8_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(H_Pass_Avrg_8_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(H_Pass_Avrg_Up_8_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_8_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_Avrg_8_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_Avrg_Up_8_Altivec_C);


extern XVID_QP_PASS_SIGNATURE(H_Pass_16_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(H_Pass_Avrg_16_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(H_Pass_Avrg_Up_16_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_16_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_Avrg_16_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_Avrg_Up_16_Add_Altivec_C);

extern XVID_QP_PASS_SIGNATURE(H_Pass_8_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(H_Pass_Avrg_8_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(H_Pass_Avrg_Up_8_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_8_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_Avrg_8_Add_Altivec_C);
extern XVID_QP_PASS_SIGNATURE(V_Pass_Avrg_Up_8_Add_Altivec_C);

XVID_QP_FUNCS xvid_QP_Funcs_Altivec_C = {
	H_Pass_16_Altivec_C, H_Pass_Avrg_16_Altivec_C, H_Pass_Avrg_Up_16_Altivec_C,
	V_Pass_16_Altivec_C, V_Pass_Avrg_16_Altivec_C, V_Pass_Avrg_Up_16_Altivec_C,

	H_Pass_8_Altivec_C, H_Pass_Avrg_8_Altivec_C, H_Pass_Avrg_Up_8_Altivec_C,
	V_Pass_8_Altivec_C, V_Pass_Avrg_8_Altivec_C, V_Pass_Avrg_Up_8_Altivec_C
};

XVID_QP_FUNCS xvid_QP_Add_Funcs_Altivec_C = {
	H_Pass_16_Add_Altivec_C, H_Pass_Avrg_16_Add_Altivec_C, H_Pass_Avrg_Up_16_Add_Altivec_C,
	V_Pass_16_Add_Altivec_C, V_Pass_Avrg_16_Add_Altivec_C, V_Pass_Avrg_Up_16_Add_Altivec_C,

	H_Pass_8_Add_Altivec_C, H_Pass_Avrg_8_Add_Altivec_C, H_Pass_Avrg_Up_8_Add_Altivec_C,
	V_Pass_8_Add_Altivec_C, V_Pass_Avrg_8_Add_Altivec_C, V_Pass_Avrg_Up_8_Add_Altivec_C
};

#endif /* ARCH_IS_PPC */

/* tables for ASM
 ****************************************************************************/


#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
/* These symbols will be used outside this file, so tell the compiler
 * they're global. */
extern uint16_t xvid_Expand_mmx[256][4]; /* 8b -> 64b expansion table */

extern int16_t xvid_FIR_1_0_0_0[256][4];
extern int16_t xvid_FIR_3_1_0_0[256][4];
extern int16_t xvid_FIR_6_3_1_0[256][4];
extern int16_t xvid_FIR_14_3_2_1[256][4];
extern int16_t xvid_FIR_20_6_3_1[256][4];
extern int16_t xvid_FIR_20_20_6_3[256][4];
extern int16_t xvid_FIR_23_19_6_3[256][4];
extern int16_t xvid_FIR_7_20_20_6[256][4];
extern int16_t xvid_FIR_6_20_20_6[256][4];
extern int16_t xvid_FIR_6_20_20_7[256][4];
extern int16_t xvid_FIR_3_6_20_20[256][4];
extern int16_t xvid_FIR_3_6_19_23[256][4];
extern int16_t xvid_FIR_1_3_6_20[256][4];
extern int16_t xvid_FIR_1_2_3_14[256][4];
extern int16_t xvid_FIR_0_1_3_6[256][4];
extern int16_t xvid_FIR_0_0_1_3[256][4];
extern int16_t xvid_FIR_0_0_0_1[256][4];
#endif

/* Arrays definitions, according to the target platform */

#if !defined(ARCH_IS_X86_64) && !defined(ARCH_IS_IA32)
/* Only ia32/ia64 will use these tables outside this file so mark them
* static for all other archs */
#define __SCOPE static
__SCOPE int16_t xvid_FIR_1_0_0_0[256][4];
__SCOPE int16_t xvid_FIR_3_1_0_0[256][4];
__SCOPE int16_t xvid_FIR_6_3_1_0[256][4];
__SCOPE int16_t xvid_FIR_14_3_2_1[256][4];
__SCOPE int16_t xvid_FIR_20_6_3_1[256][4];
__SCOPE int16_t xvid_FIR_20_20_6_3[256][4];
__SCOPE int16_t xvid_FIR_23_19_6_3[256][4];
__SCOPE int16_t xvid_FIR_7_20_20_6[256][4];
__SCOPE int16_t xvid_FIR_6_20_20_6[256][4];
__SCOPE int16_t xvid_FIR_6_20_20_7[256][4];
__SCOPE int16_t xvid_FIR_3_6_20_20[256][4];
__SCOPE int16_t xvid_FIR_3_6_19_23[256][4];
__SCOPE int16_t xvid_FIR_1_3_6_20[256][4];
__SCOPE int16_t xvid_FIR_1_2_3_14[256][4];
__SCOPE int16_t xvid_FIR_0_1_3_6[256][4];
__SCOPE int16_t xvid_FIR_0_0_1_3[256][4];
__SCOPE int16_t xvid_FIR_0_0_0_1[256][4];
#endif

static void Init_FIR_Table(int16_t Tab[][4],
                           int A, int B, int C, int D)
{
	int i;
	for(i=0; i<256; ++i) {
		Tab[i][0] = i*A;
		Tab[i][1] = i*B;
		Tab[i][2] = i*C;
		Tab[i][3] = i*D;
	}
}


void xvid_Init_QP(void)
{
#if defined (ARCH_IS_IA32) || defined (ARCH_IS_X86_64)
	int i;

	for(i=0; i<256; ++i) {
		xvid_Expand_mmx[i][0] = i;
		xvid_Expand_mmx[i][1] = i;
		xvid_Expand_mmx[i][2] = i;
		xvid_Expand_mmx[i][3] = i;
	}
#endif

	/* Alternate way of filtering (cf. USE_TABLES flag in qpel_mmx.asm) */

	Init_FIR_Table(xvid_FIR_1_0_0_0,   -1,  0,  0,  0);
	Init_FIR_Table(xvid_FIR_3_1_0_0,    3, -1,  0,  0);
	Init_FIR_Table(xvid_FIR_6_3_1_0,   -6,  3, -1,  0);
	Init_FIR_Table(xvid_FIR_14_3_2_1,  14, -3,  2, -1);
	Init_FIR_Table(xvid_FIR_20_6_3_1,  20, -6,  3, -1);
	Init_FIR_Table(xvid_FIR_20_20_6_3, 20, 20, -6,  3);
	Init_FIR_Table(xvid_FIR_23_19_6_3, 23, 19, -6,  3);
	Init_FIR_Table(xvid_FIR_7_20_20_6, -7, 20, 20, -6);
	Init_FIR_Table(xvid_FIR_6_20_20_6, -6, 20, 20, -6);
	Init_FIR_Table(xvid_FIR_6_20_20_7, -6, 20, 20, -7);
	Init_FIR_Table(xvid_FIR_3_6_20_20,  3, -6, 20, 20);
	Init_FIR_Table(xvid_FIR_3_6_19_23,  3, -6, 19, 23);
	Init_FIR_Table(xvid_FIR_1_3_6_20,  -1,  3, -6, 20);
	Init_FIR_Table(xvid_FIR_1_2_3_14,  -1,  2, -3, 14);
	Init_FIR_Table(xvid_FIR_0_1_3_6,    0, -1,  3, -6);
	Init_FIR_Table(xvid_FIR_0_0_1_3,    0,  0, -1,  3);
	Init_FIR_Table(xvid_FIR_0_0_0_1,    0,  0,  0, -1);

}

#endif /* !XVID_AUTO_INCLUDE */

#if defined(XVID_AUTO_INCLUDE) && defined(REFERENCE_CODE)

/*****************************************************************************
 * "reference" filters impl. in plain C
 ****************************************************************************/

static
void FUNC_H(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t Rnd)
{
	while(H-->0) {
		int32_t i, k;
		int32_t Sums[SIZE] = { 0 };
		for(i=0; i<=SIZE; ++i)
			for(k=0; k<SIZE; ++k)
				Sums[k] += TABLE[i][k] * Src[i];

		for(i=0; i<SIZE; ++i) {
			int32_t C = ( Sums[i] + 16-Rnd ) >> 5;
			if (C<0) C = 0; else if (C>255) C = 255;
			STORE(Dst[i], C);
		}
		Src += BpS;
		Dst += BpS;
	}
}

static
void FUNC_V(uint8_t *Dst, const uint8_t *Src, int32_t W, int32_t BpS, int32_t Rnd)
{
	while(W-->0) {
		int32_t i, k;
		int32_t Sums[SIZE] = { 0 };
		const uint8_t *S = Src++;
		uint8_t *D = Dst++;
		for(i=0; i<=SIZE; ++i) {
			for(k=0; k<SIZE; ++k)
				Sums[k] += TABLE[i][k] * S[0];
			S += BpS;
		}

		for(i=0; i<SIZE; ++i) {
			int32_t C = ( Sums[i] + 16-Rnd )>>5;
			if (C<0) C = 0; else if (C>255) C = 255;
			STORE(D[0], C);
			D += BpS;
		}
	}
}

static
void FUNC_HA(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t Rnd)
{
	while(H-->0) {
		int32_t i, k;
		int32_t Sums[SIZE] = { 0 };
		for(i=0; i<=SIZE; ++i)
			for(k=0; k<SIZE; ++k)
				Sums[k] += TABLE[i][k] * Src[i];

		for(i=0; i<SIZE; ++i) {
			int32_t C = ( Sums[i] + 16-Rnd ) >> 5;
			if (C<0) C = 0; else if (C>255) C = 255;
			C = (C+Src[i]+1-Rnd) >> 1;
			STORE(Dst[i], C);
		}
		Src += BpS;
		Dst += BpS;
	}
}

static
void FUNC_HA_UP(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t Rnd)
{
	while(H-->0) {
		int32_t i, k;
		int32_t Sums[SIZE] = { 0 };
		for(i=0; i<=SIZE; ++i)
			for(k=0; k<SIZE; ++k)
				Sums[k] += TABLE[i][k] * Src[i];

		for(i=0; i<SIZE; ++i) {
			int32_t C = ( Sums[i] + 16-Rnd ) >> 5;
			if (C<0) C = 0; else if (C>255) C = 255;
			C = (C+Src[i+1]+1-Rnd) >> 1;
			STORE(Dst[i], C);
		}
		Src += BpS;
		Dst += BpS;
	}
}

static
void FUNC_VA(uint8_t *Dst, const uint8_t *Src, int32_t W, int32_t BpS, int32_t Rnd)
{
	while(W-->0) {
		int32_t i, k;
		int32_t Sums[SIZE] = { 0 };
		const uint8_t *S = Src;
		uint8_t *D = Dst;

		for(i=0; i<=SIZE; ++i) {
			for(k=0; k<SIZE; ++k)
				Sums[k] += TABLE[i][k] * S[0];
			S += BpS;
		}

		S = Src;
		for(i=0; i<SIZE; ++i) {
			int32_t C = ( Sums[i] + 16-Rnd )>>5;
			if (C<0) C = 0; else if (C>255) C = 255;
			C = ( C+S[0]+1-Rnd ) >> 1;
			STORE(D[0], C);
			D += BpS;
			S += BpS;
		}
		Src++;
		Dst++;
	}
}

static
void FUNC_VA_UP(uint8_t *Dst, const uint8_t *Src, int32_t W, int32_t BpS, int32_t Rnd)
{
	while(W-->0) {
		int32_t i, k;
		int32_t Sums[SIZE] = { 0 };
		const uint8_t *S = Src;
		uint8_t *D = Dst;

		for(i=0; i<=SIZE; ++i) {
			for(k=0; k<SIZE; ++k)
				Sums[k] += TABLE[i][k] * S[0];
			S += BpS;
		}

		S = Src + BpS;
		for(i=0; i<SIZE; ++i) {
			int32_t C = ( Sums[i] + 16-Rnd )>>5;
			if (C<0) C = 0; else if (C>255) C = 255;
			C = ( C+S[0]+1-Rnd ) >> 1;
			STORE(D[0], C);
			D += BpS;
			S += BpS;
		}
		Dst++;
		Src++;
	}
}

#undef STORE
#undef FUNC_H
#undef FUNC_V
#undef FUNC_HA
#undef FUNC_VA
#undef FUNC_HA_UP
#undef FUNC_VA_UP

#elif defined(XVID_AUTO_INCLUDE) && !defined(REFERENCE_CODE)

/*****************************************************************************
 * "fast" filters impl. in plain C
 ****************************************************************************/

#define CLIP_STORE(D,C) \
  if (C<0) C = 0; else if (C>(255<<5)) C = 255; else C = C>>5;  \
  STORE(D, C)

static void
FUNC_H(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t RND)
{
#if (SIZE==16)
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[0] +23*Src[1] - 7*Src[2] + 3*Src[3] -   Src[4];
    CLIP_STORE(Dst[ 0],C);
    C = 16-RND - 3*(Src[0]-Src[4]) +19*Src[1] +20*Src[2] - 6*Src[3] - Src[5];
    CLIP_STORE(Dst[ 1],C);
    C = 16-RND + 2*Src[0] - 6*(Src[1]+Src[4]) +20*(Src[2]+Src[3]) + 3*Src[5] - Src[6];
    CLIP_STORE(Dst[ 2],C);
    C = 16-RND - (Src[0]+Src[7 ]) + 3*(Src[ 1]+Src[ 6])-6*(Src[ 2]+Src[ 5]) + 20*(Src[ 3]+Src[ 4]);
    CLIP_STORE(Dst[ 3],C);
    C = 16-RND - (Src[1]+Src[8 ]) + 3*(Src[ 2]+Src[ 7])-6*(Src[ 3]+Src[ 6]) + 20*(Src[ 4]+Src[ 5]);
    CLIP_STORE(Dst[ 4],C);
    C = 16-RND - (Src[2]+Src[9 ]) + 3*(Src[ 3]+Src[ 8])-6*(Src[ 4]+Src[ 7]) + 20*(Src[ 5]+Src[ 6]);
    CLIP_STORE(Dst[ 5],C);
    C = 16-RND - (Src[3]+Src[10]) + 3*(Src[ 4]+Src[ 9])-6*(Src[ 5]+Src[ 8]) + 20*(Src[ 6]+Src[ 7]);
    CLIP_STORE(Dst[ 6],C);
    C = 16-RND - (Src[4]+Src[11]) + 3*(Src[ 5]+Src[10])-6*(Src[ 6]+Src[ 9]) + 20*(Src[ 7]+Src[ 8]);
    CLIP_STORE(Dst[ 7],C);
    C = 16-RND - (Src[5]+Src[12]) + 3*(Src[ 6]+Src[11])-6*(Src[ 7]+Src[10]) + 20*(Src[ 8]+Src[ 9]);
    CLIP_STORE(Dst[ 8],C);
    C = 16-RND - (Src[6]+Src[13]) + 3*(Src[ 7]+Src[12])-6*(Src[ 8]+Src[11]) + 20*(Src[ 9]+Src[10]);
    CLIP_STORE(Dst[ 9],C);
    C = 16-RND - (Src[7]+Src[14]) + 3*(Src[ 8]+Src[13])-6*(Src[ 9]+Src[12]) + 20*(Src[10]+Src[11]);
    CLIP_STORE(Dst[10],C);
    C = 16-RND - (Src[8]+Src[15]) + 3*(Src[ 9]+Src[14])-6*(Src[10]+Src[13]) + 20*(Src[11]+Src[12]);
    CLIP_STORE(Dst[11],C);
    C = 16-RND - (Src[9]+Src[16]) + 3*(Src[10]+Src[15])-6*(Src[11]+Src[14]) + 20*(Src[12]+Src[13]);
    CLIP_STORE(Dst[12],C);
    C = 16-RND - Src[10] +3*Src[11] -6*(Src[12]+Src[15]) + 20*(Src[13]+Src[14]) +2*Src[16];
    CLIP_STORE(Dst[13],C);
    C = 16-RND - Src[11] +3*(Src[12]-Src[16]) -6*Src[13] + 20*Src[14] + 19*Src[15];
    CLIP_STORE(Dst[14],C);
    C = 16-RND - Src[12] +3*Src[13] -7*Src[14] + 23*Src[15] + 14*Src[16];
    CLIP_STORE(Dst[15],C);
    Src += BpS;
    Dst += BpS;
  }
#else
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[0] +23*Src[1] - 7*Src[2] + 3*Src[3] -   Src[4];
    CLIP_STORE(Dst[0],C);
    C = 16-RND - 3*(Src[0]-Src[4]) +19*Src[1] +20*Src[2] - 6*Src[3] - Src[5];
    CLIP_STORE(Dst[1],C);
    C = 16-RND + 2*Src[0] - 6*(Src[1]+Src[4]) +20*(Src[2]+Src[3]) + 3*Src[5] - Src[6];
    CLIP_STORE(Dst[2],C);
    C = 16-RND - (Src[0]+Src[7]) + 3*(Src[1]+Src[6])-6*(Src[2]+Src[5]) + 20*(Src[3]+Src[4]);
    CLIP_STORE(Dst[3],C);
    C = 16-RND - (Src[1]+Src[8]) + 3*(Src[2]+Src[7])-6*(Src[3]+Src[6]) + 20*(Src[4]+Src[5]);
    CLIP_STORE(Dst[4],C);
    C = 16-RND - Src[2] +3*Src[3] -6*(Src[4]+Src[7]) + 20*(Src[5]+Src[6]) +2*Src[8];
    CLIP_STORE(Dst[5],C);
    C = 16-RND - Src[3] +3*(Src[4]-Src[8]) -6*Src[5] + 20*Src[6] + 19*Src[7];
    CLIP_STORE(Dst[6],C);
    C = 16-RND - Src[4] +3*Src[5] -7*Src[6] + 23*Src[7] + 14*Src[8];
    CLIP_STORE(Dst[7],C);
    Src += BpS;
    Dst += BpS;
  }
#endif
}
#undef CLIP_STORE

#define CLIP_STORE(i,C) \
  if (C<0) C = 0; else if (C>(255<<5)) C = 255; else C = C>>5;  \
  C = (C+Src[i]+1-RND) >> 1;  \
  STORE(Dst[i], C)

static void
FUNC_HA(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t RND)
{
#if (SIZE==16)
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[0] +23*Src[1] - 7*Src[2] + 3*Src[3] -   Src[4];
    CLIP_STORE(0,C);
    C = 16-RND - 3*(Src[0]-Src[4]) +19*Src[1] +20*Src[2] - 6*Src[3] - Src[5];
    CLIP_STORE( 1,C);
    C = 16-RND + 2*Src[0] - 6*(Src[1]+Src[4]) +20*(Src[2]+Src[3]) + 3*Src[5] - Src[6];
    CLIP_STORE( 2,C);
    C = 16-RND - (Src[0]+Src[7 ]) + 3*(Src[ 1]+Src[ 6])-6*(Src[ 2]+Src[ 5]) + 20*(Src[ 3]+Src[ 4]);
    CLIP_STORE( 3,C);
    C = 16-RND - (Src[1]+Src[8 ]) + 3*(Src[ 2]+Src[ 7])-6*(Src[ 3]+Src[ 6]) + 20*(Src[ 4]+Src[ 5]);
    CLIP_STORE( 4,C);
    C = 16-RND - (Src[2]+Src[9 ]) + 3*(Src[ 3]+Src[ 8])-6*(Src[ 4]+Src[ 7]) + 20*(Src[ 5]+Src[ 6]);
    CLIP_STORE( 5,C);
    C = 16-RND - (Src[3]+Src[10]) + 3*(Src[ 4]+Src[ 9])-6*(Src[ 5]+Src[ 8]) + 20*(Src[ 6]+Src[ 7]);
    CLIP_STORE( 6,C);
    C = 16-RND - (Src[4]+Src[11]) + 3*(Src[ 5]+Src[10])-6*(Src[ 6]+Src[ 9]) + 20*(Src[ 7]+Src[ 8]);
    CLIP_STORE( 7,C);
    C = 16-RND - (Src[5]+Src[12]) + 3*(Src[ 6]+Src[11])-6*(Src[ 7]+Src[10]) + 20*(Src[ 8]+Src[ 9]);
    CLIP_STORE( 8,C);
    C = 16-RND - (Src[6]+Src[13]) + 3*(Src[ 7]+Src[12])-6*(Src[ 8]+Src[11]) + 20*(Src[ 9]+Src[10]);
    CLIP_STORE( 9,C);
    C = 16-RND - (Src[7]+Src[14]) + 3*(Src[ 8]+Src[13])-6*(Src[ 9]+Src[12]) + 20*(Src[10]+Src[11]);
    CLIP_STORE(10,C);
    C = 16-RND - (Src[8]+Src[15]) + 3*(Src[ 9]+Src[14])-6*(Src[10]+Src[13]) + 20*(Src[11]+Src[12]);
    CLIP_STORE(11,C);
    C = 16-RND - (Src[9]+Src[16]) + 3*(Src[10]+Src[15])-6*(Src[11]+Src[14]) + 20*(Src[12]+Src[13]);
    CLIP_STORE(12,C);
    C = 16-RND - Src[10] +3*Src[11] -6*(Src[12]+Src[15]) + 20*(Src[13]+Src[14]) +2*Src[16];
    CLIP_STORE(13,C);
    C = 16-RND - Src[11] +3*(Src[12]-Src[16]) -6*Src[13] + 20*Src[14] + 19*Src[15];
    CLIP_STORE(14,C);
    C = 16-RND - Src[12] +3*Src[13] -7*Src[14] + 23*Src[15] + 14*Src[16];
    CLIP_STORE(15,C);
    Src += BpS;
    Dst += BpS;
  }
#else
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[0] +23*Src[1] - 7*Src[2] + 3*Src[3] -   Src[4];
    CLIP_STORE(0,C);
    C = 16-RND - 3*(Src[0]-Src[4]) +19*Src[1] +20*Src[2] - 6*Src[3] - Src[5];
    CLIP_STORE(1,C);
    C = 16-RND + 2*Src[0] - 6*(Src[1]+Src[4]) +20*(Src[2]+Src[3]) + 3*Src[5] - Src[6];
    CLIP_STORE(2,C);
    C = 16-RND - (Src[0]+Src[7]) + 3*(Src[1]+Src[6])-6*(Src[2]+Src[5]) + 20*(Src[3]+Src[4]);
    CLIP_STORE(3,C);
    C = 16-RND - (Src[1]+Src[8]) + 3*(Src[2]+Src[7])-6*(Src[3]+Src[6]) + 20*(Src[4]+Src[5]);
    CLIP_STORE(4,C);
    C = 16-RND - Src[2] +3*Src[3] -6*(Src[4]+Src[7]) + 20*(Src[5]+Src[6]) +2*Src[8];
    CLIP_STORE(5,C);
    C = 16-RND - Src[3] +3*(Src[4]-Src[8]) -6*Src[5] + 20*Src[6] + 19*Src[7];
    CLIP_STORE(6,C);
    C = 16-RND - Src[4] +3*Src[5] -7*Src[6] + 23*Src[7] + 14*Src[8];
    CLIP_STORE(7,C);
    Src += BpS;
    Dst += BpS;
  }
#endif
}
#undef CLIP_STORE

#define CLIP_STORE(i,C) \
  if (C<0) C = 0; else if (C>(255<<5)) C = 255; else C = C>>5;  \
  C = (C+Src[i+1]+1-RND) >> 1;  \
  STORE(Dst[i], C)

static void
FUNC_HA_UP(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t RND)
{
#if (SIZE==16)
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[0] +23*Src[1] - 7*Src[2] + 3*Src[3] -   Src[4];
    CLIP_STORE(0,C);
    C = 16-RND - 3*(Src[0]-Src[4]) +19*Src[1] +20*Src[2] - 6*Src[3] - Src[5];
    CLIP_STORE( 1,C);
    C = 16-RND + 2*Src[0] - 6*(Src[1]+Src[4]) +20*(Src[2]+Src[3]) + 3*Src[5] - Src[6];
    CLIP_STORE( 2,C);
    C = 16-RND - (Src[0]+Src[7 ]) + 3*(Src[ 1]+Src[ 6])-6*(Src[ 2]+Src[ 5]) + 20*(Src[ 3]+Src[ 4]);
    CLIP_STORE( 3,C);
    C = 16-RND - (Src[1]+Src[8 ]) + 3*(Src[ 2]+Src[ 7])-6*(Src[ 3]+Src[ 6]) + 20*(Src[ 4]+Src[ 5]);
    CLIP_STORE( 4,C);
    C = 16-RND - (Src[2]+Src[9 ]) + 3*(Src[ 3]+Src[ 8])-6*(Src[ 4]+Src[ 7]) + 20*(Src[ 5]+Src[ 6]);
    CLIP_STORE( 5,C);
    C = 16-RND - (Src[3]+Src[10]) + 3*(Src[ 4]+Src[ 9])-6*(Src[ 5]+Src[ 8]) + 20*(Src[ 6]+Src[ 7]);
    CLIP_STORE( 6,C);
    C = 16-RND - (Src[4]+Src[11]) + 3*(Src[ 5]+Src[10])-6*(Src[ 6]+Src[ 9]) + 20*(Src[ 7]+Src[ 8]);
    CLIP_STORE( 7,C);
    C = 16-RND - (Src[5]+Src[12]) + 3*(Src[ 6]+Src[11])-6*(Src[ 7]+Src[10]) + 20*(Src[ 8]+Src[ 9]);
    CLIP_STORE( 8,C);
    C = 16-RND - (Src[6]+Src[13]) + 3*(Src[ 7]+Src[12])-6*(Src[ 8]+Src[11]) + 20*(Src[ 9]+Src[10]);
    CLIP_STORE( 9,C);
    C = 16-RND - (Src[7]+Src[14]) + 3*(Src[ 8]+Src[13])-6*(Src[ 9]+Src[12]) + 20*(Src[10]+Src[11]);
    CLIP_STORE(10,C);
    C = 16-RND - (Src[8]+Src[15]) + 3*(Src[ 9]+Src[14])-6*(Src[10]+Src[13]) + 20*(Src[11]+Src[12]);
    CLIP_STORE(11,C);
    C = 16-RND - (Src[9]+Src[16]) + 3*(Src[10]+Src[15])-6*(Src[11]+Src[14]) + 20*(Src[12]+Src[13]);
    CLIP_STORE(12,C);
    C = 16-RND - Src[10] +3*Src[11] -6*(Src[12]+Src[15]) + 20*(Src[13]+Src[14]) +2*Src[16];
    CLIP_STORE(13,C);
    C = 16-RND - Src[11] +3*(Src[12]-Src[16]) -6*Src[13] + 20*Src[14] + 19*Src[15];
    CLIP_STORE(14,C);
    C = 16-RND - Src[12] +3*Src[13] -7*Src[14] + 23*Src[15] + 14*Src[16];
    CLIP_STORE(15,C);
    Src += BpS;
    Dst += BpS;
  }
#else
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[0] +23*Src[1] - 7*Src[2] + 3*Src[3] -   Src[4];
    CLIP_STORE(0,C);
    C = 16-RND - 3*(Src[0]-Src[4]) +19*Src[1] +20*Src[2] - 6*Src[3] - Src[5];
    CLIP_STORE(1,C);
    C = 16-RND + 2*Src[0] - 6*(Src[1]+Src[4]) +20*(Src[2]+Src[3]) + 3*Src[5] - Src[6];
    CLIP_STORE(2,C);
    C = 16-RND - (Src[0]+Src[7]) + 3*(Src[1]+Src[6])-6*(Src[2]+Src[5]) + 20*(Src[3]+Src[4]);
    CLIP_STORE(3,C);
    C = 16-RND - (Src[1]+Src[8]) + 3*(Src[2]+Src[7])-6*(Src[3]+Src[6]) + 20*(Src[4]+Src[5]);
    CLIP_STORE(4,C);
    C = 16-RND - Src[2] +3*Src[3] -6*(Src[4]+Src[7]) + 20*(Src[5]+Src[6]) +2*Src[8];
    CLIP_STORE(5,C);
    C = 16-RND - Src[3] +3*(Src[4]-Src[8]) -6*Src[5] + 20*Src[6] + 19*Src[7];
    CLIP_STORE(6,C);
    C = 16-RND - Src[4] +3*Src[5] -7*Src[6] + 23*Src[7] + 14*Src[8];
    CLIP_STORE(7,C);
    Src += BpS;
    Dst += BpS;
  }
#endif
}
#undef CLIP_STORE

//////////////////////////////////////////////////////////
// vertical passes
//////////////////////////////////////////////////////////
// Note: for vertical passes, width (W) needs only be 8 or 16.

#define CLIP_STORE(D,C) \
  if (C<0) C = 0; else if (C>(255<<5)) C = 255; else C = C>>5;  \
  STORE(D, C)

static void
FUNC_V(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t RND)
{
#if (SIZE==16)
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[BpS*0] +23*Src[BpS*1] - 7*Src[BpS*2] + 3*Src[BpS*3] -   Src[BpS*4];
    CLIP_STORE(Dst[BpS* 0],C);
    C = 16-RND - 3*(Src[BpS*0]-Src[BpS*4]) +19*Src[BpS*1] +20*Src[BpS*2] - 6*Src[BpS*3] - Src[BpS*5];
    CLIP_STORE(Dst[BpS* 1],C);
    C = 16-RND + 2*Src[BpS*0] - 6*(Src[BpS*1]+Src[BpS*4]) +20*(Src[BpS*2]+Src[BpS*3]) + 3*Src[BpS*5] - Src[BpS*6];
    CLIP_STORE(Dst[BpS* 2],C);
    C = 16-RND - (Src[BpS*0]+Src[BpS*7 ]) + 3*(Src[BpS* 1]+Src[BpS* 6])-6*(Src[BpS* 2]+Src[BpS* 5]) + 20*(Src[BpS* 3]+Src[BpS* 4]);
    CLIP_STORE(Dst[BpS* 3],C);
    C = 16-RND - (Src[BpS*1]+Src[BpS*8 ]) + 3*(Src[BpS* 2]+Src[BpS* 7])-6*(Src[BpS* 3]+Src[BpS* 6]) + 20*(Src[BpS* 4]+Src[BpS* 5]);
    CLIP_STORE(Dst[BpS* 4],C);
    C = 16-RND - (Src[BpS*2]+Src[BpS*9 ]) + 3*(Src[BpS* 3]+Src[BpS* 8])-6*(Src[BpS* 4]+Src[BpS* 7]) + 20*(Src[BpS* 5]+Src[BpS* 6]);
    CLIP_STORE(Dst[BpS* 5],C);
    C = 16-RND - (Src[BpS*3]+Src[BpS*10]) + 3*(Src[BpS* 4]+Src[BpS* 9])-6*(Src[BpS* 5]+Src[BpS* 8]) + 20*(Src[BpS* 6]+Src[BpS* 7]);
    CLIP_STORE(Dst[BpS* 6],C);
    C = 16-RND - (Src[BpS*4]+Src[BpS*11]) + 3*(Src[BpS* 5]+Src[BpS*10])-6*(Src[BpS* 6]+Src[BpS* 9]) + 20*(Src[BpS* 7]+Src[BpS* 8]);
    CLIP_STORE(Dst[BpS* 7],C);
    C = 16-RND - (Src[BpS*5]+Src[BpS*12]) + 3*(Src[BpS* 6]+Src[BpS*11])-6*(Src[BpS* 7]+Src[BpS*10]) + 20*(Src[BpS* 8]+Src[BpS* 9]);
    CLIP_STORE(Dst[BpS* 8],C);
    C = 16-RND - (Src[BpS*6]+Src[BpS*13]) + 3*(Src[BpS* 7]+Src[BpS*12])-6*(Src[BpS* 8]+Src[BpS*11]) + 20*(Src[BpS* 9]+Src[BpS*10]);
    CLIP_STORE(Dst[BpS* 9],C);
    C = 16-RND - (Src[BpS*7]+Src[BpS*14]) + 3*(Src[BpS* 8]+Src[BpS*13])-6*(Src[BpS* 9]+Src[BpS*12]) + 20*(Src[BpS*10]+Src[BpS*11]);
    CLIP_STORE(Dst[BpS*10],C);
    C = 16-RND - (Src[BpS*8]+Src[BpS*15]) + 3*(Src[BpS* 9]+Src[BpS*14])-6*(Src[BpS*10]+Src[BpS*13]) + 20*(Src[BpS*11]+Src[BpS*12]);
    CLIP_STORE(Dst[BpS*11],C);
    C = 16-RND - (Src[BpS*9]+Src[BpS*16]) + 3*(Src[BpS*10]+Src[BpS*15])-6*(Src[BpS*11]+Src[BpS*14]) + 20*(Src[BpS*12]+Src[BpS*13]);
    CLIP_STORE(Dst[BpS*12],C);
    C = 16-RND - Src[BpS*10] +3*Src[BpS*11] -6*(Src[BpS*12]+Src[BpS*15]) + 20*(Src[BpS*13]+Src[BpS*14]) +2*Src[BpS*16];
    CLIP_STORE(Dst[BpS*13],C);
    C = 16-RND - Src[BpS*11] +3*(Src[BpS*12]-Src[BpS*16]) -6*Src[BpS*13] + 20*Src[BpS*14] + 19*Src[BpS*15];
    CLIP_STORE(Dst[BpS*14],C);
    C = 16-RND - Src[BpS*12] +3*Src[BpS*13] -7*Src[BpS*14] + 23*Src[BpS*15] + 14*Src[BpS*16];
    CLIP_STORE(Dst[BpS*15],C);
    Src += 1;
    Dst += 1;
  }
#else
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[BpS*0] +23*Src[BpS*1] - 7*Src[BpS*2] + 3*Src[BpS*3] -   Src[BpS*4];
    CLIP_STORE(Dst[BpS*0],C);
    C = 16-RND - 3*(Src[BpS*0]-Src[BpS*4]) +19*Src[BpS*1] +20*Src[BpS*2] - 6*Src[BpS*3] - Src[BpS*5];
    CLIP_STORE(Dst[BpS*1],C);
    C = 16-RND + 2*Src[BpS*0] - 6*(Src[BpS*1]+Src[BpS*4]) +20*(Src[BpS*2]+Src[BpS*3]) + 3*Src[BpS*5] - Src[BpS*6];
    CLIP_STORE(Dst[BpS*2],C);
    C = 16-RND - (Src[BpS*0]+Src[BpS*7]) + 3*(Src[BpS*1]+Src[BpS*6])-6*(Src[BpS*2]+Src[BpS*5]) + 20*(Src[BpS*3]+Src[BpS*4]);
    CLIP_STORE(Dst[BpS*3],C);
    C = 16-RND - (Src[BpS*1]+Src[BpS*8]) + 3*(Src[BpS*2]+Src[BpS*7])-6*(Src[BpS*3]+Src[BpS*6]) + 20*(Src[BpS*4]+Src[BpS*5]);
    CLIP_STORE(Dst[BpS*4],C);
    C = 16-RND - Src[BpS*2] +3*Src[BpS*3] -6*(Src[BpS*4]+Src[BpS*7]) + 20*(Src[BpS*5]+Src[BpS*6]) +2*Src[BpS*8];
    CLIP_STORE(Dst[BpS*5],C);
    C = 16-RND - Src[BpS*3] +3*(Src[BpS*4]-Src[BpS*8]) -6*Src[BpS*5] + 20*Src[BpS*6] + 19*Src[BpS*7];
    CLIP_STORE(Dst[BpS*6],C);
    C = 16-RND - Src[BpS*4] +3*Src[BpS*5] -7*Src[BpS*6] + 23*Src[BpS*7] + 14*Src[BpS*8];
    CLIP_STORE(Dst[BpS*7],C);
    Src += 1;
    Dst += 1;
  }
#endif
}
#undef CLIP_STORE

#define CLIP_STORE(i,C) \
  if (C<0) C = 0; else if (C>(255<<5)) C = 255; else C = C>>5;  \
  C = (C+Src[BpS*i]+1-RND) >> 1;  \
  STORE(Dst[BpS*i], C)

static void
FUNC_VA(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t RND)
{
#if (SIZE==16)
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[BpS*0] +23*Src[BpS*1] - 7*Src[BpS*2] + 3*Src[BpS*3] -   Src[BpS*4];
    CLIP_STORE(0,C);
    C = 16-RND - 3*(Src[BpS*0]-Src[BpS*4]) +19*Src[BpS*1] +20*Src[BpS*2] - 6*Src[BpS*3] - Src[BpS*5];
    CLIP_STORE( 1,C);
    C = 16-RND + 2*Src[BpS*0] - 6*(Src[BpS*1]+Src[BpS*4]) +20*(Src[BpS*2]+Src[BpS*3]) + 3*Src[BpS*5] - Src[BpS*6];
    CLIP_STORE( 2,C);
    C = 16-RND - (Src[BpS*0]+Src[BpS*7 ]) + 3*(Src[BpS* 1]+Src[BpS* 6])-6*(Src[BpS* 2]+Src[BpS* 5]) + 20*(Src[BpS* 3]+Src[BpS* 4]);
    CLIP_STORE( 3,C);
    C = 16-RND - (Src[BpS*1]+Src[BpS*8 ]) + 3*(Src[BpS* 2]+Src[BpS* 7])-6*(Src[BpS* 3]+Src[BpS* 6]) + 20*(Src[BpS* 4]+Src[BpS* 5]);
    CLIP_STORE( 4,C);
    C = 16-RND - (Src[BpS*2]+Src[BpS*9 ]) + 3*(Src[BpS* 3]+Src[BpS* 8])-6*(Src[BpS* 4]+Src[BpS* 7]) + 20*(Src[BpS* 5]+Src[BpS* 6]);
    CLIP_STORE( 5,C);
    C = 16-RND - (Src[BpS*3]+Src[BpS*10]) + 3*(Src[BpS* 4]+Src[BpS* 9])-6*(Src[BpS* 5]+Src[BpS* 8]) + 20*(Src[BpS* 6]+Src[BpS* 7]);
    CLIP_STORE( 6,C);
    C = 16-RND - (Src[BpS*4]+Src[BpS*11]) + 3*(Src[BpS* 5]+Src[BpS*10])-6*(Src[BpS* 6]+Src[BpS* 9]) + 20*(Src[BpS* 7]+Src[BpS* 8]);
    CLIP_STORE( 7,C);
    C = 16-RND - (Src[BpS*5]+Src[BpS*12]) + 3*(Src[BpS* 6]+Src[BpS*11])-6*(Src[BpS* 7]+Src[BpS*10]) + 20*(Src[BpS* 8]+Src[BpS* 9]);
    CLIP_STORE( 8,C);
    C = 16-RND - (Src[BpS*6]+Src[BpS*13]) + 3*(Src[BpS* 7]+Src[BpS*12])-6*(Src[BpS* 8]+Src[BpS*11]) + 20*(Src[BpS* 9]+Src[BpS*10]);
    CLIP_STORE( 9,C);
    C = 16-RND - (Src[BpS*7]+Src[BpS*14]) + 3*(Src[BpS* 8]+Src[BpS*13])-6*(Src[BpS* 9]+Src[BpS*12]) + 20*(Src[BpS*10]+Src[BpS*11]);
    CLIP_STORE(10,C);
    C = 16-RND - (Src[BpS*8]+Src[BpS*15]) + 3*(Src[BpS* 9]+Src[BpS*14])-6*(Src[BpS*10]+Src[BpS*13]) + 20*(Src[BpS*11]+Src[BpS*12]);
    CLIP_STORE(11,C);
    C = 16-RND - (Src[BpS*9]+Src[BpS*16]) + 3*(Src[BpS*10]+Src[BpS*15])-6*(Src[BpS*11]+Src[BpS*14]) + 20*(Src[BpS*12]+Src[BpS*13]);
    CLIP_STORE(12,C);
    C = 16-RND - Src[BpS*10] +3*Src[BpS*11] -6*(Src[BpS*12]+Src[BpS*15]) + 20*(Src[BpS*13]+Src[BpS*14]) +2*Src[BpS*16];
    CLIP_STORE(13,C);
    C = 16-RND - Src[BpS*11] +3*(Src[BpS*12]-Src[BpS*16]) -6*Src[BpS*13] + 20*Src[BpS*14] + 19*Src[BpS*15];
    CLIP_STORE(14,C);
    C = 16-RND - Src[BpS*12] +3*Src[BpS*13] -7*Src[BpS*14] + 23*Src[BpS*15] + 14*Src[BpS*16];
    CLIP_STORE(15,C);
    Src += 1;
    Dst += 1;
  }
#else
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[BpS*0] +23*Src[BpS*1] - 7*Src[BpS*2] + 3*Src[BpS*3] -   Src[BpS*4];
    CLIP_STORE(0,C);
    C = 16-RND - 3*(Src[BpS*0]-Src[BpS*4]) +19*Src[BpS*1] +20*Src[BpS*2] - 6*Src[BpS*3] - Src[BpS*5];
    CLIP_STORE(1,C);
    C = 16-RND + 2*Src[BpS*0] - 6*(Src[BpS*1]+Src[BpS*4]) +20*(Src[BpS*2]+Src[BpS*3]) + 3*Src[BpS*5] - Src[BpS*6];
    CLIP_STORE(2,C);
    C = 16-RND - (Src[BpS*0]+Src[BpS*7]) + 3*(Src[BpS*1]+Src[BpS*6])-6*(Src[BpS*2]+Src[BpS*5]) + 20*(Src[BpS*3]+Src[BpS*4]);
    CLIP_STORE(3,C);
    C = 16-RND - (Src[BpS*1]+Src[BpS*8]) + 3*(Src[BpS*2]+Src[BpS*7])-6*(Src[BpS*3]+Src[BpS*6]) + 20*(Src[BpS*4]+Src[BpS*5]);
    CLIP_STORE(4,C);
    C = 16-RND - Src[BpS*2] +3*Src[BpS*3] -6*(Src[BpS*4]+Src[BpS*7]) + 20*(Src[BpS*5]+Src[BpS*6]) +2*Src[BpS*8];
    CLIP_STORE(5,C);
    C = 16-RND - Src[BpS*3] +3*(Src[BpS*4]-Src[BpS*8]) -6*Src[BpS*5] + 20*Src[BpS*6] + 19*Src[BpS*7];
    CLIP_STORE(6,C);
    C = 16-RND - Src[BpS*4] +3*Src[BpS*5] -7*Src[BpS*6] + 23*Src[BpS*7] + 14*Src[BpS*8];
    CLIP_STORE(7,C);
    Src += 1;
    Dst += 1;
  }
#endif
}
#undef CLIP_STORE

#define CLIP_STORE(i,C) \
  if (C<0) C = 0; else if (C>(255<<5)) C = 255; else C = C>>5;  \
  C = (C+Src[BpS*i+BpS]+1-RND) >> 1;  \
  STORE(Dst[BpS*i], C)

static void
FUNC_VA_UP(uint8_t *Dst, const uint8_t *Src, int32_t H, int32_t BpS, int32_t RND)
{
#if (SIZE==16)
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[BpS*0] +23*Src[BpS*1] - 7*Src[BpS*2] + 3*Src[BpS*3] -   Src[BpS*4];
    CLIP_STORE(0,C);
    C = 16-RND - 3*(Src[BpS*0]-Src[BpS*4]) +19*Src[BpS*1] +20*Src[BpS*2] - 6*Src[BpS*3] - Src[BpS*5];
    CLIP_STORE( 1,C);
    C = 16-RND + 2*Src[BpS*0] - 6*(Src[BpS*1]+Src[BpS*4]) +20*(Src[BpS*2]+Src[BpS*3]) + 3*Src[BpS*5] - Src[BpS*6];
    CLIP_STORE( 2,C);
    C = 16-RND - (Src[BpS*0]+Src[BpS*7 ]) + 3*(Src[BpS* 1]+Src[BpS* 6])-6*(Src[BpS* 2]+Src[BpS* 5]) + 20*(Src[BpS* 3]+Src[BpS* 4]);
    CLIP_STORE( 3,C);
    C = 16-RND - (Src[BpS*1]+Src[BpS*8 ]) + 3*(Src[BpS* 2]+Src[BpS* 7])-6*(Src[BpS* 3]+Src[BpS* 6]) + 20*(Src[BpS* 4]+Src[BpS* 5]);
    CLIP_STORE( 4,C);
    C = 16-RND - (Src[BpS*2]+Src[BpS*9 ]) + 3*(Src[BpS* 3]+Src[BpS* 8])-6*(Src[BpS* 4]+Src[BpS* 7]) + 20*(Src[BpS* 5]+Src[BpS* 6]);
    CLIP_STORE( 5,C);
    C = 16-RND - (Src[BpS*3]+Src[BpS*10]) + 3*(Src[BpS* 4]+Src[BpS* 9])-6*(Src[BpS* 5]+Src[BpS* 8]) + 20*(Src[BpS* 6]+Src[BpS* 7]);
    CLIP_STORE( 6,C);
    C = 16-RND - (Src[BpS*4]+Src[BpS*11]) + 3*(Src[BpS* 5]+Src[BpS*10])-6*(Src[BpS* 6]+Src[BpS* 9]) + 20*(Src[BpS* 7]+Src[BpS* 8]);
    CLIP_STORE( 7,C);
    C = 16-RND - (Src[BpS*5]+Src[BpS*12]) + 3*(Src[BpS* 6]+Src[BpS*11])-6*(Src[BpS* 7]+Src[BpS*10]) + 20*(Src[BpS* 8]+Src[BpS* 9]);
    CLIP_STORE( 8,C);
    C = 16-RND - (Src[BpS*6]+Src[BpS*13]) + 3*(Src[BpS* 7]+Src[BpS*12])-6*(Src[BpS* 8]+Src[BpS*11]) + 20*(Src[BpS* 9]+Src[BpS*10]);
    CLIP_STORE( 9,C);
    C = 16-RND - (Src[BpS*7]+Src[BpS*14]) + 3*(Src[BpS* 8]+Src[BpS*13])-6*(Src[BpS* 9]+Src[BpS*12]) + 20*(Src[BpS*10]+Src[BpS*11]);
    CLIP_STORE(10,C);
    C = 16-RND - (Src[BpS*8]+Src[BpS*15]) + 3*(Src[BpS* 9]+Src[BpS*14])-6*(Src[BpS*10]+Src[BpS*13]) + 20*(Src[BpS*11]+Src[BpS*12]);
    CLIP_STORE(11,C);
    C = 16-RND - (Src[BpS*9]+Src[BpS*16]) + 3*(Src[BpS*10]+Src[BpS*15])-6*(Src[BpS*11]+Src[BpS*14]) + 20*(Src[BpS*12]+Src[BpS*13]);
    CLIP_STORE(12,C);
    C = 16-RND - Src[BpS*10] +3*Src[BpS*11] -6*(Src[BpS*12]+Src[BpS*15]) + 20*(Src[BpS*13]+Src[BpS*14]) +2*Src[BpS*16];
    CLIP_STORE(13,C);
    C = 16-RND - Src[BpS*11] +3*(Src[BpS*12]-Src[BpS*16]) -6*Src[BpS*13] + 20*Src[BpS*14] + 19*Src[BpS*15];
    CLIP_STORE(14,C);
    C = 16-RND - Src[BpS*12] +3*Src[BpS*13] -7*Src[BpS*14] + 23*Src[BpS*15] + 14*Src[BpS*16];
    CLIP_STORE(15,C);
    Src += 1;
    Dst += 1;
  }
#else
  while(H-->0) {
    int C;
    C = 16-RND +14*Src[BpS*0] +23*Src[BpS*1] - 7*Src[BpS*2] + 3*Src[BpS*3] -   Src[BpS*4];
    CLIP_STORE(0,C);
    C = 16-RND - 3*(Src[BpS*0]-Src[BpS*4]) +19*Src[BpS*1] +20*Src[BpS*2] - 6*Src[BpS*3] - Src[BpS*5];
    CLIP_STORE(1,C);
    C = 16-RND + 2*Src[BpS*0] - 6*(Src[BpS*1]+Src[BpS*4]) +20*(Src[BpS*2]+Src[BpS*3]) + 3*Src[BpS*5] - Src[BpS*6];
    CLIP_STORE(2,C);
    C = 16-RND - (Src[BpS*0]+Src[BpS*7]) + 3*(Src[BpS*1]+Src[BpS*6])-6*(Src[BpS*2]+Src[BpS*5]) + 20*(Src[BpS*3]+Src[BpS*4]);
    CLIP_STORE(3,C);
    C = 16-RND - (Src[BpS*1]+Src[BpS*8]) + 3*(Src[BpS*2]+Src[BpS*7])-6*(Src[BpS*3]+Src[BpS*6]) + 20*(Src[BpS*4]+Src[BpS*5]);
    CLIP_STORE(4,C);
    C = 16-RND - Src[BpS*2] +3*Src[BpS*3] -6*(Src[BpS*4]+Src[BpS*7]) + 20*(Src[BpS*5]+Src[BpS*6]) +2*Src[BpS*8];
    CLIP_STORE(5,C);
    C = 16-RND - Src[BpS*3] +3*(Src[BpS*4]-Src[BpS*8]) -6*Src[BpS*5] + 20*Src[BpS*6] + 19*Src[BpS*7];
    CLIP_STORE(6,C);
    C = 16-RND - Src[BpS*4] +3*Src[BpS*5] -7*Src[BpS*6] + 23*Src[BpS*7] + 14*Src[BpS*8];
    CLIP_STORE(7,C);
    Src += 1;
    Dst += 1;
  }
#endif
}
#undef CLIP_STORE

#undef STORE
#undef FUNC_H
#undef FUNC_V
#undef FUNC_HA
#undef FUNC_VA
#undef FUNC_HA_UP
#undef FUNC_VA_UP


#endif /* XVID_AUTO_INCLUDE && !defined(REF) */
