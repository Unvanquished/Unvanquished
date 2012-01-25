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
 * $Id: qpel.h,v 1.8 2008/11/26 01:04:34 Isibaar Exp $
 *
 ****************************************************************************/

#ifndef _XVID_QPEL_H_
#define _XVID_QPEL_H_

#include "interpolate8x8.h"
#include "../utils/mem_transfer.h"

/*****************************************************************************
 * Signatures
 ****************************************************************************/

#define XVID_QP_PASS_SIGNATURE(NAME)  \
  void (NAME)(uint8_t *dst, const uint8_t *src, int32_t length, int32_t BpS, int32_t rounding)

typedef  XVID_QP_PASS_SIGNATURE(XVID_QP_PASS);

/* We put everything in a single struct so it can easily be passed
 * to prediction functions as a whole... */

typedef struct _XVID_QP_FUNCS {

	/* filter for QPel 16x? prediction */

	XVID_QP_PASS *H_Pass;
	XVID_QP_PASS *H_Pass_Avrg;
	XVID_QP_PASS *H_Pass_Avrg_Up;
	XVID_QP_PASS *V_Pass;
	XVID_QP_PASS *V_Pass_Avrg;
	XVID_QP_PASS *V_Pass_Avrg_Up;

    /* filter for QPel 8x? prediction */

	XVID_QP_PASS *H_Pass_8;
	XVID_QP_PASS *H_Pass_Avrg_8;
	XVID_QP_PASS *H_Pass_Avrg_Up_8;
	XVID_QP_PASS *V_Pass_8;
	XVID_QP_PASS *V_Pass_Avrg_8;
	XVID_QP_PASS *V_Pass_Avrg_Up_8;
} XVID_QP_FUNCS;

/*****************************************************************************
 * fwd dcl
 ****************************************************************************/
extern void xvid_Init_QP();

extern XVID_QP_FUNCS xvid_QP_Funcs_C_ref;       /* for P-frames */
extern XVID_QP_FUNCS xvid_QP_Add_Funcs_C_ref;   /* for B-frames */

extern XVID_QP_FUNCS xvid_QP_Funcs_C;       /* for P-frames */
extern XVID_QP_FUNCS xvid_QP_Add_Funcs_C;   /* for B-frames */

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern XVID_QP_FUNCS xvid_QP_Funcs_mmx;
extern XVID_QP_FUNCS xvid_QP_Add_Funcs_mmx;
#endif

#ifdef ARCH_IS_PPC
extern XVID_QP_FUNCS xvid_QP_Funcs_Altivec_C;
extern XVID_QP_FUNCS xvid_QP_Add_Funcs_Altivec_C;
#endif

extern XVID_QP_FUNCS *xvid_QP_Funcs;      /* <- main pointer for enc/dec structure */
extern XVID_QP_FUNCS *xvid_QP_Add_Funcs;  /* <- main pointer for enc/dec structure */

/*****************************************************************************
 * macros
 ****************************************************************************/

/*****************************************************************************

    Passes to be performed

 case 0:         copy
 case 2:         h-pass
 case 1/3:       h-pass + h-avrg
 case 8:                           v-pass
 case 10:        h-pass          + v-pass
 case 9/11:      h-pass + h-avrg + v-pass
 case 4/12:                        v-pass + v-avrg
 case 6/14:      h-pass          + v-pass + v-avrg
 case 5/13/7/15: h-pass + h-avrg + v-pass + v-avrg

 ****************************************************************************/

static void __inline 
interpolate16x16_quarterpel(uint8_t * const cur,
								uint8_t * const refn,
								uint8_t * const refh,
								uint8_t * const refv,
								uint8_t * const refhv,
								const uint32_t x, const uint32_t y,
								const int32_t dx,  const int dy,
								const uint32_t stride,
								const uint32_t rounding)
{
	const uint8_t *src;
	uint8_t *dst;
	uint8_t *tmp;
	int32_t quads;
	const XVID_QP_FUNCS *Ops;

	int32_t x_int, y_int;

	const int32_t xRef = (int)x*4 + dx;
	const int32_t yRef = (int)y*4 + dy;

	Ops = xvid_QP_Funcs;
	quads = (dx&3) | ((dy&3)<<2);

	x_int = xRef >> 2;
	y_int = yRef >> 2;

	dst = cur + y * stride + x;
	src = refn + y_int * (int)stride + x_int;

	tmp = refh; /* we need at least a 16 x stride scratch block */

	switch(quads) {
	case 0:
		transfer8x8_copy(dst, src, stride);
		transfer8x8_copy(dst+8, src+8, stride);
		transfer8x8_copy(dst+8*stride, src+8*stride, stride);
		transfer8x8_copy(dst+8*stride+8, src+8*stride+8, stride);
		break;
	case 1:
		Ops->H_Pass_Avrg(dst, src, 16, stride, rounding);
		break;
	case 2:
		Ops->H_Pass(dst, src, 16, stride, rounding);
		break;
	case 3:
		Ops->H_Pass_Avrg_Up(dst, src, 16, stride, rounding);
		break;
	case 4:
		Ops->V_Pass_Avrg(dst, src, 16, stride, rounding);
		break;
	case 5:
		Ops->H_Pass_Avrg(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg(dst, tmp, 16, stride, rounding);
		break;
	case 6:
		Ops->H_Pass(tmp, src,	  17, stride, rounding);
		Ops->V_Pass_Avrg(dst, tmp, 16, stride, rounding);
		break;
	case 7:
		Ops->H_Pass_Avrg_Up(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg(dst, tmp, 16, stride, rounding);
		break;
	case 8:
		Ops->V_Pass(dst, src, 16, stride, rounding);
		break;
	case 9:
		Ops->H_Pass_Avrg(tmp, src, 17, stride, rounding);
		Ops->V_Pass(dst, tmp, 16, stride, rounding);
		break;
	case 10:
		Ops->H_Pass(tmp, src, 17, stride, rounding);
		Ops->V_Pass(dst, tmp, 16, stride, rounding);
		break;
	case 11:
		Ops->H_Pass_Avrg_Up(tmp, src, 17, stride, rounding);
		Ops->V_Pass(dst, tmp, 16, stride, rounding);
		break;
	case 12:
		Ops->V_Pass_Avrg_Up(dst, src, 16, stride, rounding);
		break;
	case 13:
		Ops->H_Pass_Avrg(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg_Up(dst, tmp, 16, stride, rounding);
		break;
	case 14:
		Ops->H_Pass(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg_Up( dst, tmp, 16, stride, rounding);
		break;
	case 15:
		Ops->H_Pass_Avrg_Up(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg_Up(dst, tmp, 16, stride, rounding);
		break;
	}
}

static void __inline 
interpolate16x16_add_quarterpel(uint8_t * const cur,
								uint8_t * const refn,
								uint8_t * const refh,
								uint8_t * const refv,
								uint8_t * const refhv,
								const uint32_t x, const uint32_t y,
								const int32_t dx,  const int dy,
								const uint32_t stride,
								const uint32_t rounding)
{
	const uint8_t *src;
	uint8_t *dst;
	uint8_t *tmp;
	int32_t quads;
	const XVID_QP_FUNCS *Ops;
	const XVID_QP_FUNCS *Ops_Copy;

	int32_t x_int, y_int;

	const int32_t xRef = (int)x*4 + dx;
	const int32_t yRef = (int)y*4 + dy;

	Ops = xvid_QP_Add_Funcs;
	Ops_Copy = xvid_QP_Funcs;
	quads = (dx&3) | ((dy&3)<<2);

	x_int = xRef >> 2;
	y_int = yRef >> 2;

	dst = cur + y * stride + x;
	src = refn + y_int * (int)stride + x_int;

	tmp = refh; /* we need at least a 16 x stride scratch block */

	switch(quads) {
	case 0:
		/* NB: there is no halfpel involved ! the name's function can be
		 *     misleading */
		interpolate8x8_halfpel_add(dst, src, stride, rounding);
		interpolate8x8_halfpel_add(dst+8, src+8, stride, rounding);
		interpolate8x8_halfpel_add(dst+8*stride, src+8*stride, stride, rounding);
		interpolate8x8_halfpel_add(dst+8*stride+8, src+8*stride+8, stride, rounding);
		break;
	case 1:
		Ops->H_Pass_Avrg(dst, src, 16, stride, rounding);
		break;
	case 2:
		Ops->H_Pass(dst, src, 16, stride, rounding);
		break;
	case 3:
		Ops->H_Pass_Avrg_Up(dst, src, 16, stride, rounding);
		break;
	case 4:
		Ops->V_Pass_Avrg(dst, src, 16, stride, rounding);
		break;
	case 5:
		Ops_Copy->H_Pass_Avrg(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg(dst, tmp, 16, stride, rounding);
		break;
	case 6:
		Ops_Copy->H_Pass(tmp, src,	  17, stride, rounding);
		Ops->V_Pass_Avrg(dst, tmp, 16, stride, rounding);
		break;
	case 7:
		Ops_Copy->H_Pass_Avrg_Up(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg(dst, tmp, 16, stride, rounding);
		break;
	case 8:
		Ops->V_Pass(dst, src, 16, stride, rounding);
		break;
	case 9:
		Ops_Copy->H_Pass_Avrg(tmp, src, 17, stride, rounding);
		Ops->V_Pass(dst, tmp, 16, stride, rounding);
		break;
	case 10:
		Ops_Copy->H_Pass(tmp, src, 17, stride, rounding);
		Ops->V_Pass(dst, tmp, 16, stride, rounding);
		break;
	case 11:
		Ops_Copy->H_Pass_Avrg_Up(tmp, src, 17, stride, rounding);
		Ops->V_Pass(dst, tmp, 16, stride, rounding);
		break;
	case 12:
		Ops->V_Pass_Avrg_Up(dst, src, 16, stride, rounding);
		break;
	case 13:
		Ops_Copy->H_Pass_Avrg(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg_Up(dst, tmp, 16, stride, rounding);
		break;
	case 14:
		Ops_Copy->H_Pass(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg_Up( dst, tmp, 16, stride, rounding);
		break;
	case 15:
		Ops_Copy->H_Pass_Avrg_Up(tmp, src, 17, stride, rounding);
		Ops->V_Pass_Avrg_Up(dst, tmp, 16, stride, rounding);
		break;
	}
}

static void __inline
interpolate16x8_quarterpel(uint8_t * const cur,
							   uint8_t * const refn,
							   uint8_t * const refh,
							   uint8_t * const refv,
							   uint8_t * const refhv,
							   const uint32_t x, const uint32_t y,
							   const int32_t dx,  const int dy,
							   const uint32_t stride,
							   const uint32_t rounding)
{
	const uint8_t *src;
	uint8_t *dst;
	uint8_t *tmp;
	int32_t quads;
	const XVID_QP_FUNCS *Ops;

	int32_t x_int, y_int;

	const int32_t xRef = (int)x*4 + dx;
	const int32_t yRef = (int)y*4 + dy;

	Ops = xvid_QP_Funcs;
	quads = (dx&3) | ((dy&3)<<2);

	x_int = xRef >> 2;
	y_int = yRef >> 2;

	dst = cur + y * stride + x;
	src = refn + y_int * (int)stride + x_int;

	tmp = refh; /* we need at least a 16 x stride scratch block */

	switch(quads) {
	case 0:
		transfer8x8_copy( dst, src, stride);
		transfer8x8_copy( dst+8, src+8, stride);
		break;
	case 1:
		Ops->H_Pass_Avrg(dst, src, 8, stride, rounding);
		break;
	case 2:
		Ops->H_Pass(dst, src, 8, stride, rounding);
		break;
	case 3:
		Ops->H_Pass_Avrg_Up(dst, src, 8, stride, rounding);
		break;
	case 4:
		Ops->V_Pass_Avrg_8(dst, src, 16, stride, rounding);
		break;
	case 5:
		Ops->H_Pass_Avrg(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 16, stride, rounding);
		break;
	case 6:
		Ops->H_Pass(tmp, src,	  9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 16, stride, rounding);
		break;
	case 7:
		Ops->H_Pass_Avrg_Up(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 16, stride, rounding);
		break;
	case 8:
		Ops->V_Pass_8(dst, src, 16, stride, rounding);
		break;
	case 9:
		Ops->H_Pass_Avrg(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 16, stride, rounding);
		break;
	case 10:
		Ops->H_Pass(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 16, stride, rounding);
		break;
	case 11:
		Ops->H_Pass_Avrg_Up(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 16, stride, rounding);
		break;
	case 12:
		Ops->V_Pass_Avrg_Up_8(dst, src, 16, stride, rounding);
		break;
	case 13:
		Ops->H_Pass_Avrg(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8(dst, tmp, 16, stride, rounding);
		break;
	case 14:
		Ops->H_Pass(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8( dst, tmp, 16, stride, rounding);
		break;
	case 15:
		Ops->H_Pass_Avrg_Up(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8(dst, tmp, 16, stride, rounding);
		break;
	}
}

static void __inline
interpolate8x8_quarterpel(uint8_t * const cur,
							  uint8_t * const refn,
							  uint8_t * const refh,
							  uint8_t * const refv,
							  uint8_t * const refhv,
							  const uint32_t x, const uint32_t y,
							  const int32_t dx,  const int dy,
							  const uint32_t stride,
							  const uint32_t rounding)
{
	const uint8_t *src;
	uint8_t *dst;
	uint8_t *tmp;
	int32_t quads;
	const XVID_QP_FUNCS *Ops;

	int32_t x_int, y_int;

	const int32_t xRef = (int)x*4 + dx;
	const int32_t yRef = (int)y*4 + dy;

	Ops = xvid_QP_Funcs;
	quads = (dx&3) | ((dy&3)<<2);

	x_int = xRef >> 2;
	y_int = yRef >> 2;

	dst = cur + y * stride + x;
	src = refn + y_int * (int)stride + x_int;

	tmp = refh; /* we need at least a 16 x stride scratch block */

	switch(quads) {
	case 0:
		transfer8x8_copy( dst, src, stride);
		break;
	case 1:
		Ops->H_Pass_Avrg_8(dst, src, 8, stride, rounding);
		break;
	case 2:
		Ops->H_Pass_8(dst, src, 8, stride, rounding);
		break;
	case 3:
		Ops->H_Pass_Avrg_Up_8(dst, src, 8, stride, rounding);
		break;
	case 4:
		Ops->V_Pass_Avrg_8(dst, src, 8, stride, rounding);
		break;
	case 5:
		Ops->H_Pass_Avrg_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 8, stride, rounding);
		break;
	case 6:
		Ops->H_Pass_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 8, stride, rounding);
		break;
	case 7:
		Ops->H_Pass_Avrg_Up_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 8, stride, rounding);
		break;
	case 8:
		Ops->V_Pass_8(dst, src, 8, stride, rounding);
		break;
	case 9:
		Ops->H_Pass_Avrg_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 8, stride, rounding);
		break;
	case 10:
		Ops->H_Pass_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 8, stride, rounding);
		break;
	case 11:
		Ops->H_Pass_Avrg_Up_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 8, stride, rounding);
		break;
	case 12:
		Ops->V_Pass_Avrg_Up_8(dst, src, 8, stride, rounding);
		break;
	case 13:
		Ops->H_Pass_Avrg_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8(dst, tmp, 8, stride, rounding);
		break;
	case 14:
		Ops->H_Pass_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8( dst, tmp, 8, stride, rounding);
		break;
	case 15:
		Ops->H_Pass_Avrg_Up_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8(dst, tmp, 8, stride, rounding);
		break;
	}
}

static void __inline
interpolate8x8_add_quarterpel(uint8_t * const cur,
							  uint8_t * const refn,
							  uint8_t * const refh,
							  uint8_t * const refv,
							  uint8_t * const refhv,
							  const uint32_t x, const uint32_t y,
							  const int32_t dx,  const int dy,
							  const uint32_t stride,
							  const uint32_t rounding)
{
	const uint8_t *src;
	uint8_t *dst;
	uint8_t *tmp;
	int32_t quads;
	const XVID_QP_FUNCS *Ops;
	const XVID_QP_FUNCS *Ops_Copy;

	int32_t x_int, y_int;

	const int32_t xRef = (int)x*4 + dx;
	const int32_t yRef = (int)y*4 + dy;

	Ops = xvid_QP_Add_Funcs;
	Ops_Copy = xvid_QP_Funcs;
	quads = (dx&3) | ((dy&3)<<2);

	x_int = xRef >> 2;
	y_int = yRef >> 2;

	dst = cur + y * stride + x;
	src = refn + y_int * (int)stride + x_int;

	tmp = refh; /* we need at least a 16 x stride scratch block */

	switch(quads) {
	case 0:
		/* Misleading function name, there is no halfpel involved
		 * just dst and src averaging with rounding=0 */
		interpolate8x8_halfpel_add(dst, src, stride, rounding);
		break;
	case 1:
		Ops->H_Pass_Avrg_8(dst, src, 8, stride, rounding);
		break;
	case 2:
		Ops->H_Pass_8(dst, src, 8, stride, rounding);
		break;
	case 3:
		Ops->H_Pass_Avrg_Up_8(dst, src, 8, stride, rounding);
		break;
	case 4:
		Ops->V_Pass_Avrg_8(dst, src, 8, stride, rounding);
		break;
	case 5:
		Ops_Copy->H_Pass_Avrg_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 8, stride, rounding);
		break;
	case 6:
		Ops_Copy->H_Pass_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 8, stride, rounding);
		break;
	case 7:
		Ops_Copy->H_Pass_Avrg_Up_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_8(dst, tmp, 8, stride, rounding);
		break;
	case 8:
		Ops->V_Pass_8(dst, src, 8, stride, rounding);
		break;
	case 9:
		Ops_Copy->H_Pass_Avrg_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 8, stride, rounding);
		break;
	case 10:
		Ops_Copy->H_Pass_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 8, stride, rounding);
		break;
	case 11:
		Ops_Copy->H_Pass_Avrg_Up_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_8(dst, tmp, 8, stride, rounding);
		break;
	case 12:
		Ops->V_Pass_Avrg_Up_8(dst, src, 8, stride, rounding);
		break;
	case 13:
		Ops_Copy->H_Pass_Avrg_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8(dst, tmp, 8, stride, rounding);
		break;
	case 14:
		Ops_Copy->H_Pass_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8( dst, tmp, 8, stride, rounding);
		break;
	case 15:
		Ops_Copy->H_Pass_Avrg_Up_8(tmp, src, 9, stride, rounding);
		Ops->V_Pass_Avrg_Up_8(dst, tmp, 8, stride, rounding);
		break;
	}
}

#endif  /* _XVID_QPEL_H_ */
