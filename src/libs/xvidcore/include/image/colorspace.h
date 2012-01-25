/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Colorspace related header  -
 *
 *  Copyright(C) 2001-2003 Peter Ross <pross@xvid.org>
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
 * $Id: colorspace.h,v 1.10 2008/11/26 01:04:34 Isibaar Exp $
 *
 ****************************************************************************/

#ifndef _COLORSPACE_H
#define _COLORSPACE_H

#include "../portab.h"

/* initialize tables */

void colorspace_init(void);



/* colorspace conversion function (encoder) */

typedef void (packedFunc) (uint8_t * x_ptr,
								 int x_stride,
								 uint8_t * y_src,
								 uint8_t * v_src,
								 uint8_t * u_src,
								 int y_stride,
								 int uv_stride,
								 int width,
								 int height,
								 int vflip);

typedef packedFunc *packedFuncPtr;


/* xxx_to_yv12 colorspace conversion functions (encoder) */

extern packedFuncPtr rgb555_to_yv12;
extern packedFuncPtr rgb565_to_yv12;
extern packedFuncPtr rgb_to_yv12;
extern packedFuncPtr bgr_to_yv12;
extern packedFuncPtr bgra_to_yv12;
extern packedFuncPtr abgr_to_yv12;
extern packedFuncPtr rgba_to_yv12;
extern packedFuncPtr argb_to_yv12;
extern packedFuncPtr yuyv_to_yv12;
extern packedFuncPtr uyvy_to_yv12;

extern packedFuncPtr rgb555i_to_yv12;
extern packedFuncPtr rgb565i_to_yv12;
extern packedFuncPtr rgbi_to_yv12;
extern packedFuncPtr bgri_to_yv12;
extern packedFuncPtr bgrai_to_yv12;
extern packedFuncPtr abgri_to_yv12;
extern packedFuncPtr rgbai_to_yv12;
extern packedFuncPtr argbi_to_yv12;
extern packedFuncPtr yuyvi_to_yv12;
extern packedFuncPtr uyvyi_to_yv12;

/* plain c */
packedFunc rgb555_to_yv12_c;
packedFunc rgb565_to_yv12_c;
packedFunc rgb_to_yv12_c;
packedFunc bgr_to_yv12_c;
packedFunc bgra_to_yv12_c;
packedFunc abgr_to_yv12_c;
packedFunc rgba_to_yv12_c;
packedFunc argb_to_yv12_c;
packedFunc yuyv_to_yv12_c;
packedFunc uyvy_to_yv12_c;

packedFunc rgb555i_to_yv12_c;
packedFunc rgb565i_to_yv12_c;
packedFunc rgbi_to_yv12_c;
packedFunc bgri_to_yv12_c;
packedFunc bgrai_to_yv12_c;
packedFunc abgri_to_yv12_c;
packedFunc rgbai_to_yv12_c;
packedFunc argbi_to_yv12_c;
packedFunc yuyvi_to_yv12_c;
packedFunc uyvyi_to_yv12_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
/* mmx */
packedFunc bgr_to_yv12_mmx;
packedFunc rgb_to_yv12_mmx;
packedFunc bgra_to_yv12_mmx;
packedFunc rgba_to_yv12_mmx;
packedFunc yuyv_to_yv12_mmx;
packedFunc uyvy_to_yv12_mmx;

/* 3dnow */
packedFunc yuyv_to_yv12_3dn;
packedFunc uyvy_to_yv12_3dn;

/* xmm */
packedFunc yuyv_to_yv12_xmm;
packedFunc uyvy_to_yv12_xmm;
#endif

#ifdef ARCH_IS_PPC
packedFunc bgra_to_yv12_altivec_c;
packedFunc abgr_to_yv12_altivec_c;
packedFunc rgba_to_yv12_altivec_c;
packedFunc argb_to_yv12_altivec_c;

packedFunc yuyv_to_yv12_altivec_c;
packedFunc uyvy_to_yv12_altivec_c;
#endif

/* yv12_to_xxx colorspace conversion functions (decoder) */

extern packedFuncPtr yv12_to_rgb555;
extern packedFuncPtr yv12_to_rgb565;
extern packedFuncPtr yv12_to_rgb;
extern packedFuncPtr yv12_to_bgr;
extern packedFuncPtr yv12_to_bgra;
extern packedFuncPtr yv12_to_abgr;
extern packedFuncPtr yv12_to_rgba;
extern packedFuncPtr yv12_to_argb;
extern packedFuncPtr yv12_to_yuyv;
extern packedFuncPtr yv12_to_uyvy;

extern packedFuncPtr yv12_to_rgb555i;
extern packedFuncPtr yv12_to_rgb565i;
extern packedFuncPtr yv12_to_rgbi;
extern packedFuncPtr yv12_to_bgri;
extern packedFuncPtr yv12_to_bgrai;
extern packedFuncPtr yv12_to_abgri;
extern packedFuncPtr yv12_to_rgbai;
extern packedFuncPtr yv12_to_argbi;
extern packedFuncPtr yv12_to_yuyvi;
extern packedFuncPtr yv12_to_uyvyi;

/* plain c */
packedFunc yv12_to_rgb555_c;
packedFunc yv12_to_rgb565_c;
packedFunc yv12_to_rgb_c;
packedFunc yv12_to_bgr_c;
packedFunc yv12_to_bgra_c;
packedFunc yv12_to_abgr_c;
packedFunc yv12_to_rgba_c;
packedFunc yv12_to_argb_c;
packedFunc yv12_to_yuyv_c;
packedFunc yv12_to_uyvy_c;

packedFunc yv12_to_rgb555i_c;
packedFunc yv12_to_rgb565i_c;
packedFunc yv12_to_rgbi_c;
packedFunc yv12_to_bgri_c;
packedFunc yv12_to_bgrai_c;
packedFunc yv12_to_abgri_c;
packedFunc yv12_to_rgbai_c;
packedFunc yv12_to_argbi_c;
packedFunc yv12_to_yuyvi_c;
packedFunc yv12_to_uyvyi_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
/* mmx */
packedFunc yv12_to_bgr_mmx;
packedFunc yv12_to_bgra_mmx;
packedFunc yv12_to_yuyv_mmx;
packedFunc yv12_to_uyvy_mmx;

packedFunc yv12_to_yuyvi_mmx;
packedFunc yv12_to_uyvyi_mmx;
#endif

#ifdef ARCH_IS_PPC
packedFunc yv12_to_yuyv_altivec_c;
packedFunc yv12_to_uyvy_altivec_c;
#endif

typedef void (planarFunc) (
				uint8_t * y_dst, uint8_t * u_dst, uint8_t * v_dst,
				int y_dst_stride, int uv_dst_stride,
				uint8_t * y_src, uint8_t * u_src, uint8_t * v_src,
				int y_src_stride, int uv_src_stride,
				int width, int height, int vflip);
typedef planarFunc *planarFuncPtr;

extern planarFuncPtr yv12_to_yv12;

planarFunc yv12_to_yv12_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
planarFunc yv12_to_yv12_mmx;
planarFunc yv12_to_yv12_xmm;
#endif


#endif							/* _COLORSPACE_H_ */
