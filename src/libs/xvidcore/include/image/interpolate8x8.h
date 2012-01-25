/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Interpolation related header  -
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
 * $Id: interpolate8x8.h,v 1.17 2008/11/26 01:04:34 Isibaar Exp $
 *
 ****************************************************************************/

#ifndef _INTERPOLATE8X8_H_
#define _INTERPOLATE8X8_H_

#include "../utils/mem_transfer.h"

typedef void (INTERPOLATE8X8) (uint8_t * const dst,
							   const uint8_t * const src,
							   const uint32_t stride,
							   const uint32_t rounding);
typedef INTERPOLATE8X8 *INTERPOLATE8X8_PTR;

typedef void (INTERPOLATE8X4) (uint8_t * const dst,
							   const uint8_t * const src,
							   const uint32_t stride,
							   const uint32_t rounding);
typedef INTERPOLATE8X4 *INTERPOLATE8X4_PTR;

typedef void (INTERPOLATE8X8_AVG2) (uint8_t *dst,
									const uint8_t *src1,
									const uint8_t *src2,
									const uint32_t stride,
									const uint32_t rounding,
									const uint32_t height);
typedef INTERPOLATE8X8_AVG2 *INTERPOLATE8X8_AVG2_PTR;

typedef void (INTERPOLATE8X8_AVG4) (uint8_t *dst,
									const uint8_t *src1,
									const uint8_t *src2,
									const uint8_t *src3,
									const uint8_t *src4,
									const uint32_t stride,
									const uint32_t rounding);
typedef INTERPOLATE8X8_AVG4 *INTERPOLATE8X8_AVG4_PTR;

typedef void (INTERPOLATE_LOWPASS) (uint8_t *dst,
									   uint8_t *src,
									   int32_t stride,
									   int32_t rounding);

typedef INTERPOLATE_LOWPASS *INTERPOLATE_LOWPASS_PTR;

typedef void (INTERPOLATE_LOWPASS_HV) (uint8_t *dst1,
										  uint8_t *dst2,
										  uint8_t *src,
										  int32_t stride,
										  int32_t rounding);

typedef INTERPOLATE_LOWPASS_HV *INTERPOLATE_LOWPASS_HV_PTR;

typedef void (INTERPOLATE8X8_6TAP_LOWPASS) (uint8_t *dst,
									        uint8_t *src,
									        int32_t stride,
									        int32_t rounding);

typedef INTERPOLATE8X8_6TAP_LOWPASS *INTERPOLATE8X8_6TAP_LOWPASS_PTR;

/* These function do: dst = interpolate(src) */
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_h;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_v;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_hv;

extern INTERPOLATE8X4_PTR interpolate8x4_halfpel_h;
extern INTERPOLATE8X4_PTR interpolate8x4_halfpel_v;
extern INTERPOLATE8X4_PTR interpolate8x4_halfpel_hv;

/* These functions do: dst = (dst+interpolate(src) + 1)/2
 * Suitable for direct/interpolated bvop prediction block
 * building w/o the need for intermediate interpolated result
 * storing/reading
 * NB: the rounding applies to the interpolation, but not
 *     the averaging step which will always use rounding=0 */
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_add;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_h_add;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_v_add;
extern INTERPOLATE8X8_PTR interpolate8x8_halfpel_hv_add;

extern INTERPOLATE8X8_AVG2_PTR interpolate8x8_avg2;
extern INTERPOLATE8X8_AVG4_PTR interpolate8x8_avg4;

extern INTERPOLATE_LOWPASS_PTR interpolate8x8_lowpass_h;
extern INTERPOLATE_LOWPASS_PTR interpolate8x8_lowpass_v;

extern INTERPOLATE_LOWPASS_PTR interpolate16x16_lowpass_h;
extern INTERPOLATE_LOWPASS_PTR interpolate16x16_lowpass_v;

extern INTERPOLATE_LOWPASS_HV_PTR interpolate8x8_lowpass_hv;
extern INTERPOLATE_LOWPASS_HV_PTR interpolate16x16_lowpass_hv;

extern INTERPOLATE8X8_6TAP_LOWPASS_PTR interpolate8x8_6tap_lowpass_h;
extern INTERPOLATE8X8_6TAP_LOWPASS_PTR interpolate8x8_6tap_lowpass_v;

INTERPOLATE8X8 interpolate8x8_halfpel_h_c;
INTERPOLATE8X8 interpolate8x8_halfpel_v_c;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_c;

INTERPOLATE8X4 interpolate8x4_halfpel_h_c;
INTERPOLATE8X4 interpolate8x4_halfpel_v_c;
INTERPOLATE8X4 interpolate8x4_halfpel_hv_c;

INTERPOLATE8X8 interpolate8x8_halfpel_add_c;
INTERPOLATE8X8 interpolate8x8_halfpel_h_add_c;
INTERPOLATE8X8 interpolate8x8_halfpel_v_add_c;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_add_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
INTERPOLATE8X8 interpolate8x8_halfpel_h_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_v_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_mmx;

INTERPOLATE8X4 interpolate8x4_halfpel_h_mmx;
INTERPOLATE8X4 interpolate8x4_halfpel_v_mmx;
INTERPOLATE8X4 interpolate8x4_halfpel_hv_mmx;

INTERPOLATE8X8 interpolate8x8_halfpel_add_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_h_add_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_v_add_mmx;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_add_mmx;

INTERPOLATE8X8 interpolate8x8_halfpel_h_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_v_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_xmm;

INTERPOLATE8X4 interpolate8x4_halfpel_h_xmm;
INTERPOLATE8X4 interpolate8x4_halfpel_v_xmm;
INTERPOLATE8X4 interpolate8x4_halfpel_hv_xmm;

INTERPOLATE8X8 interpolate8x8_halfpel_add_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_h_add_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_v_add_xmm;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_add_xmm;

INTERPOLATE8X8 interpolate8x8_halfpel_h_3dn;
INTERPOLATE8X8 interpolate8x8_halfpel_v_3dn;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_3dn;

INTERPOLATE8X4 interpolate8x4_halfpel_h_3dn;
INTERPOLATE8X4 interpolate8x4_halfpel_v_3dn;
INTERPOLATE8X4 interpolate8x4_halfpel_hv_3dn;

INTERPOLATE8X8 interpolate8x8_halfpel_h_3dne;
INTERPOLATE8X8 interpolate8x8_halfpel_v_3dne;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_3dne;

INTERPOLATE8X4 interpolate8x4_halfpel_h_3dne;
INTERPOLATE8X4 interpolate8x4_halfpel_v_3dne;
INTERPOLATE8X4 interpolate8x4_halfpel_hv_3dne;
#endif

#ifdef ARCH_IS_IA64
INTERPOLATE8X8 interpolate8x8_halfpel_h_ia64;
INTERPOLATE8X8 interpolate8x8_halfpel_v_ia64;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_ia64;
#endif

#ifdef ARCH_IS_PPC
INTERPOLATE8X8 interpolate8x8_halfpel_h_altivec_c;
INTERPOLATE8X8 interpolate8x8_halfpel_v_altivec_c;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_altivec_c;

INTERPOLATE8X8 interpolate8x8_halfpel_add_altivec_c;
INTERPOLATE8X8 interpolate8x8_halfpel_h_add_altivec_c;
INTERPOLATE8X8 interpolate8x8_halfpel_v_add_altivec_c;
INTERPOLATE8X8 interpolate8x8_halfpel_hv_add_altivec_c;
#endif

INTERPOLATE8X8_AVG2 interpolate8x8_avg2_c;
INTERPOLATE8X8_AVG4 interpolate8x8_avg4_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
INTERPOLATE8X8_AVG2 interpolate8x8_avg2_mmx;
INTERPOLATE8X8_AVG4 interpolate8x8_avg4_mmx;
#endif

#ifdef ARCH_IS_PPC
INTERPOLATE8X8_AVG2 interpolate8x8_avg2_altivec_c;
INTERPOLATE8X8_AVG4 interpolate8x8_avg4_altivec_c;
#endif

INTERPOLATE_LOWPASS interpolate8x8_lowpass_h_c;
INTERPOLATE_LOWPASS interpolate8x8_lowpass_v_c;

INTERPOLATE_LOWPASS interpolate16x16_lowpass_h_c;
INTERPOLATE_LOWPASS interpolate16x16_lowpass_v_c;

INTERPOLATE_LOWPASS_HV interpolate8x8_lowpass_hv_c;
INTERPOLATE_LOWPASS_HV interpolate16x16_lowpass_hv_c;

INTERPOLATE8X8_6TAP_LOWPASS interpolate8x8_6tap_lowpass_h_c;
INTERPOLATE8X8_6TAP_LOWPASS interpolate8x8_6tap_lowpass_v_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
INTERPOLATE8X8_6TAP_LOWPASS interpolate8x8_6tap_lowpass_h_mmx;
INTERPOLATE8X8_6TAP_LOWPASS interpolate8x8_6tap_lowpass_v_mmx;
#endif

#ifdef ARCH_IS_PPC
INTERPOLATE8X8_6TAP_LOWPASS interpolate8x8_6tap_lowpass_h_altivec_c;
#endif

static __inline void
interpolate8x4_switch(uint8_t * const cur,
					  const uint8_t * const refn,
					  const uint32_t x,
					  const uint32_t y,
					  const int32_t dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{

	const uint8_t * const src = refn + (int)((y + (dy>>1)) * stride + x + (dx>>1));
	uint8_t * const dst = cur + (int)(y * stride + x);

	switch (((dx & 1) << 1) + (dy & 1))	
	{ /* ((dx%2)?2:0)+((dy%2)?1:0) */
	case 0:
		transfer8x4_copy(dst, src, stride);
		break;
	case 1:
		interpolate8x4_halfpel_v(dst, src, stride, rounding);
		break;
	case 2:
		interpolate8x4_halfpel_h(dst, src, stride, rounding);
		break;
	default:
		interpolate8x4_halfpel_hv(dst, src, stride, rounding);
		break;
	}
}

static __inline void
interpolate8x8_switch(uint8_t * const cur,
					  const uint8_t * const refn,
					  const uint32_t x,
					  const uint32_t y,
					  const int32_t dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{

	const uint8_t * const src = refn + (int)((y + (dy>>1)) * stride + x + (dx>>1));
	uint8_t * const dst = cur + (int)(y * stride + x);

	switch (((dx & 1) << 1) + (dy & 1))	{ /* ((dx%2)?2:0)+((dy%2)?1:0) */
	case 0:
		transfer8x8_copy(dst, src, stride);
		break;
	case 1:
		interpolate8x8_halfpel_v(dst, src, stride, rounding);
		break;
	case 2:
		interpolate8x8_halfpel_h(dst, src, stride, rounding);
		break;
	default:
		interpolate8x8_halfpel_hv(dst, src, stride, rounding);
		break;
	}
}

static __inline void
interpolate8x8_add_switch(uint8_t * const cur,
					  const uint8_t * const refn,
					  const uint32_t x,
					  const uint32_t y,
					  const int32_t dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{

	const uint8_t * const src = refn + (int)((y + (dy>>1)) * stride + x + (dx>>1));
	uint8_t * const dst = cur + (int)(y * stride + x);

	switch (((dx & 1) << 1) + (dy & 1))	{ /* ((dx%2)?2:0)+((dy%2)?1:0) */
	case 0:
		interpolate8x8_halfpel_add(dst, src, stride, rounding);
		break;
	case 1:
		interpolate8x8_halfpel_v_add(dst, src, stride, rounding);
		break;
	case 2:
		interpolate8x8_halfpel_h_add(dst, src, stride, rounding);
		break;
	default:
		interpolate8x8_halfpel_hv_add(dst, src, stride, rounding);
		break;
	}
}

static __inline void
interpolate16x16_switch(uint8_t * const cur,
					  const uint8_t * const refn,
					  const uint32_t x,
					  const uint32_t y,
					  const int32_t dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{
	interpolate8x8_switch(cur, refn, x,   y,   dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x+8, y,   dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x,   y+8, dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x+8, y+8, dx, dy, stride, rounding);
}

static __inline void
interpolate16x16_add_switch(uint8_t * const cur,
					  const uint8_t * const refn,
					  const uint32_t x,
					  const uint32_t y,
					  const int32_t dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{
	interpolate8x8_add_switch(cur, refn, x,   y,   dx, dy, stride, rounding);
	interpolate8x8_add_switch(cur, refn, x+8, y,   dx, dy, stride, rounding);
	interpolate8x8_add_switch(cur, refn, x,   y+8, dx, dy, stride, rounding);
	interpolate8x8_add_switch(cur, refn, x+8, y+8, dx, dy, stride, rounding);
}

static __inline void
interpolate32x32_switch(uint8_t * const cur,
					  const uint8_t * const refn,
					  const uint32_t x,
					  const uint32_t y,
					  const int32_t dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{
	interpolate16x16_switch(cur, refn, x,    y,    dx, dy, stride, rounding);
	interpolate16x16_switch(cur, refn, x+16, y,    dx, dy, stride, rounding);
	interpolate16x16_switch(cur, refn, x,    y+16, dx, dy, stride, rounding);
	interpolate16x16_switch(cur, refn, x+16, y+16, dx, dy, stride, rounding);
}

static __inline void
interpolate32x32_add_switch(uint8_t * const cur,
					  const uint8_t * const refn,
					  const uint32_t x,
					  const uint32_t y,
					  const int32_t dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{
	interpolate16x16_add_switch(cur, refn, x,    y,    dx, dy, stride, rounding);
	interpolate16x16_add_switch(cur, refn, x+16, y,    dx, dy, stride, rounding);
	interpolate16x16_add_switch(cur, refn, x,    y+16, dx, dy, stride, rounding);
	interpolate16x16_add_switch(cur, refn, x+16, y+16, dx, dy, stride, rounding);
}

static __inline uint8_t *
interpolate8x8_switch2(uint8_t * const buffer,
					  const uint8_t * const refn,
					  const int x,
					  const int y,
					  const int dx,
					  const int dy,
					  const uint32_t stride,
					  const uint32_t rounding)
{

	const uint8_t * const src = refn + (int)((y + (dy>>1)) * stride + x + (dx>>1));

	switch (((dx & 1) << 1) + (dy & 1))	{ /* ((dx%2)?2:0)+((dy%2)?1:0) */
	case 0:
		return (uint8_t *)src;
	case 1:
		interpolate8x8_halfpel_v(buffer, src, stride, rounding);
		break;
	case 2:
		interpolate8x8_halfpel_h(buffer, src, stride, rounding);
		break;
	default:
		interpolate8x8_halfpel_hv(buffer, src, stride, rounding);
		break;
	}
	return buffer;
}

#endif
