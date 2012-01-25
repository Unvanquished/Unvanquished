/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Colorspace conversion functions -
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
 * $Id: colorspace.c,v 1.14.2.1 2009/05/25 08:31:15 Isibaar Exp $
 *
 ****************************************************************************/

#include <string.h>				/* memcpy */

#include "../global.h"
#include "colorspace.h"

/* function pointers */

/* input */
packedFuncPtr rgb555_to_yv12;
packedFuncPtr rgb565_to_yv12;
packedFuncPtr rgb_to_yv12;
packedFuncPtr bgr_to_yv12;
packedFuncPtr bgra_to_yv12;
packedFuncPtr abgr_to_yv12;
packedFuncPtr rgba_to_yv12;
packedFuncPtr argb_to_yv12;
packedFuncPtr yuyv_to_yv12;
packedFuncPtr uyvy_to_yv12;

packedFuncPtr rgb555i_to_yv12;
packedFuncPtr rgb565i_to_yv12;
packedFuncPtr rgbi_to_yv12;
packedFuncPtr bgri_to_yv12;
packedFuncPtr bgrai_to_yv12;
packedFuncPtr abgri_to_yv12;
packedFuncPtr rgbai_to_yv12;
packedFuncPtr argbi_to_yv12;
packedFuncPtr yuyvi_to_yv12;
packedFuncPtr uyvyi_to_yv12;

/* output */
packedFuncPtr yv12_to_rgb555;
packedFuncPtr yv12_to_rgb565;
packedFuncPtr yv12_to_bgr;
packedFuncPtr yv12_to_bgra;
packedFuncPtr yv12_to_abgr;
packedFuncPtr yv12_to_rgb;
packedFuncPtr yv12_to_rgba;
packedFuncPtr yv12_to_argb;
packedFuncPtr yv12_to_yuyv;
packedFuncPtr yv12_to_uyvy;

packedFuncPtr yv12_to_rgb555i;
packedFuncPtr yv12_to_rgb565i;
packedFuncPtr yv12_to_bgri;
packedFuncPtr yv12_to_bgrai;
packedFuncPtr yv12_to_abgri;
packedFuncPtr yv12_to_rgbi;
packedFuncPtr yv12_to_rgbai;
packedFuncPtr yv12_to_argbi;
packedFuncPtr yv12_to_yuyvi;
packedFuncPtr yv12_to_uyvyi;

planarFuncPtr yv12_to_yv12;


static int32_t RGB_Y_tab[256];
static int32_t B_U_tab[256];
static int32_t G_U_tab[256];
static int32_t G_V_tab[256];
static int32_t R_V_tab[256];



/********** generic colorspace macro **********/


#define MAKE_COLORSPACE(NAME,SIZE,PIXELS,VPIXELS,FUNC,C1,C2,C3,C4) \
void	\
NAME(uint8_t * x_ptr, int x_stride,	\
				 uint8_t * y_ptr, uint8_t * u_ptr, uint8_t * v_ptr,	\
				 int y_stride, int uv_stride,	\
				 int width, int height, int vflip)	\
{	\
	int fixed_width = (width + 1) & ~1;				\
	int x_dif = x_stride - (SIZE)*fixed_width;		\
	int y_dif = y_stride - fixed_width;				\
	int uv_dif = uv_stride - (fixed_width / 2);		\
	int x, y;										\
	if (vflip) {								\
		x_ptr += (height - 1) * x_stride;			\
		x_dif = -(SIZE)*fixed_width - x_stride;		\
		x_stride = -x_stride;						\
	}												\
	for (y = 0; y < height; y+=(VPIXELS)) {			\
		FUNC##_ROW(SIZE,C1,C2,C3,C4);				\
		for (x = 0; x < fixed_width; x+=(PIXELS)) {	\
			FUNC(SIZE,C1,C2,C3,C4);				\
			x_ptr += (PIXELS)*(SIZE);				\
			y_ptr += (PIXELS);						\
			u_ptr += (PIXELS)/2;					\
			v_ptr += (PIXELS)/2;					\
		}											\
		x_ptr += x_dif + (VPIXELS-1)*x_stride;		\
		y_ptr += y_dif + (VPIXELS-1)*y_stride;		\
		u_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride;	\
		v_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride;	\
	}												\
}



/********** colorspace input (xxx_to_yv12) functions **********/

/*	rgb -> yuv def's

	this following constants are "official spec"
	Video Demystified" (ISBN 1-878707-09-4)

	rgb<->yuv _is_ lossy, since most programs do the conversion differently

	SCALEBITS/FIX taken from  ffmpeg
*/

#define Y_R_IN			0.257
#define Y_G_IN			0.504
#define Y_B_IN			0.098
#define Y_ADD_IN		16

#define U_R_IN			0.148
#define U_G_IN			0.291
#define U_B_IN			0.439
#define U_ADD_IN		128

#define V_R_IN			0.439
#define V_G_IN			0.368
#define V_B_IN			0.071
#define V_ADD_IN		128

#define SCALEBITS_IN		13	
#define FIX_IN(x)		((uint16_t) ((x) * (1L<<SCALEBITS_IN) + 0.5))
#define FIX_ROUND		(1L<<(SCALEBITS_IN-1))

/* rgb16/rgb16i input */

#define MK_RGB555_B(RGB)  ((RGB) << 3) & 0xf8
#define MK_RGB555_G(RGB)  ((RGB) >> 2) & 0xf8
#define MK_RGB555_R(RGB)  ((RGB) >> 7) & 0xf8

#define MK_RGB565_B(RGB)  ((RGB) << 3) & 0xf8
#define MK_RGB565_G(RGB)  ((RGB) >> 3) & 0xfc
#define MK_RGB565_R(RGB)  ((RGB) >> 8) & 0xf8


#define READ_RGB16_Y(ROW, UVID, C1,C2,C3,C4)	\
	rgb = *(uint16_t *) (x_ptr + ((ROW)*x_stride) + 0);	\
	b##UVID += b = C1##_B(rgb);				\
	g##UVID += g = C1##_G(rgb);				\
	r##UVID += r = C1##_R(rgb);				\
	y_ptr[(ROW)*y_stride+0] =				\
		(uint8_t) ((FIX_IN(Y_R_IN) * r + FIX_IN(Y_G_IN) * g +	\
					FIX_IN(Y_B_IN) * b + FIX_ROUND) >> SCALEBITS_IN) + Y_ADD_IN;	\
	rgb = *(uint16_t *) (x_ptr + ((ROW)*x_stride) + 2);	\
	b##UVID += b = C1##_B(rgb);				\
	g##UVID += g = C1##_G(rgb);				\
	r##UVID += r = C1##_R(rgb);				\
	y_ptr[(ROW)*y_stride+1] =				\
		(uint8_t) ((FIX_IN(Y_R_IN) * r + FIX_IN(Y_G_IN) * g +			\
					FIX_IN(Y_B_IN) * b + FIX_ROUND) >> SCALEBITS_IN) + Y_ADD_IN;

#define READ_RGB16_UV(UV_ROW,UVID)	\
	u_ptr[(UV_ROW)*uv_stride] =														\
		(uint8_t) ((-FIX_IN(U_R_IN) * r##UVID - FIX_IN(U_G_IN) * g##UVID +			\
					FIX_IN(U_B_IN) * b##UVID + 4*FIX_ROUND) >> (SCALEBITS_IN + 2)) + U_ADD_IN;	\
	v_ptr[(UV_ROW)*uv_stride] =														\
		(uint8_t) ((FIX_IN(V_R_IN) * r##UVID - FIX_IN(V_G_IN) * g##UVID -			\
					FIX_IN(V_B_IN) * b##UVID + 4*FIX_ROUND) >> (SCALEBITS_IN + 2)) + V_ADD_IN;

#define RGB16_TO_YV12_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */
#define RGB16_TO_YV12(SIZE,C1,C2,C3,C4)	\
	uint32_t rgb, r, g, b, r0, g0, b0;	\
	r0 = g0 = b0 = 0;					\
	READ_RGB16_Y (0, 0, C1,C2,C3,C4)	\
	READ_RGB16_Y (1, 0, C1,C2,C3,C4)	\
	READ_RGB16_UV(0, 0)


#define RGB16I_TO_YV12_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */
#define RGB16I_TO_YV12(SIZE,C1,C2,C3,C4)	\
	uint32_t rgb, r, g, b, r0, g0, b0, r1, g1, b1;	\
	r0 = g0 = b0 = r1 = g1 = b1 = 0;	\
	READ_RGB16_Y (0, 0, C1,C2,C3,C4)	\
	READ_RGB16_Y (1, 1, C1,C2,C3,C4)	\
	READ_RGB16_Y (2, 0, C1,C2,C3,C4)	\
	READ_RGB16_Y (3, 1, C1,C2,C3,C4)	\
	READ_RGB16_UV(0, 0)					\
	READ_RGB16_UV(1, 1)


/* rgb/rgbi input */

#define READ_RGB_Y(SIZE, ROW, UVID, C1,C2,C3,C4)	\
	r##UVID += r = x_ptr[(ROW)*x_stride+(C1)];						\
	g##UVID += g = x_ptr[(ROW)*x_stride+(C2)];						\
	b##UVID += b = x_ptr[(ROW)*x_stride+(C3)];						\
	y_ptr[(ROW)*y_stride+0] =									\
		(uint8_t) ((FIX_IN(Y_R_IN) * r + FIX_IN(Y_G_IN) * g +	\
					FIX_IN(Y_B_IN) * b + FIX_ROUND) >> SCALEBITS_IN) + Y_ADD_IN;	\
	r##UVID += r = x_ptr[(ROW)*x_stride+(SIZE)+(C1)];				\
	g##UVID += g = x_ptr[(ROW)*x_stride+(SIZE)+(C2)];				\
	b##UVID += b = x_ptr[(ROW)*x_stride+(SIZE)+(C3)];				\
	y_ptr[(ROW)*y_stride+1] =									\
		(uint8_t) ((FIX_IN(Y_R_IN) * r + FIX_IN(Y_G_IN) * g +	\
					FIX_IN(Y_B_IN) * b + FIX_ROUND) >> SCALEBITS_IN) + Y_ADD_IN;

#define READ_RGB_UV(UV_ROW,UVID)	\
	u_ptr[(UV_ROW)*uv_stride] =														\
		(uint8_t) ((-FIX_IN(U_R_IN) * r##UVID - FIX_IN(U_G_IN) * g##UVID +			\
					FIX_IN(U_B_IN) * b##UVID + 4*FIX_ROUND) >> (SCALEBITS_IN + 2)) + U_ADD_IN;	\
	v_ptr[(UV_ROW)*uv_stride] =														\
		(uint8_t) ((FIX_IN(V_R_IN) * r##UVID - FIX_IN(V_G_IN) * g##UVID -			\
					FIX_IN(V_B_IN) * b##UVID + 4*FIX_ROUND) >> (SCALEBITS_IN + 2)) + V_ADD_IN;


#define RGB_TO_YV12_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */
#define RGB_TO_YV12(SIZE,C1,C2,C3,C4)	\
	uint32_t r, g, b, r0, g0, b0;		\
	r0 = g0 = b0 = 0;					\
	READ_RGB_Y(SIZE, 0, 0, C1,C2,C3,C4)	\
	READ_RGB_Y(SIZE, 1, 0, C1,C2,C3,C4)	\
	READ_RGB_UV(     0, 0)

#define RGBI_TO_YV12_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */
#define RGBI_TO_YV12(SIZE,C1,C2,C3,C4)	\
	uint32_t r, g, b, r0, g0, b0, r1, g1, b1;	\
	r0 = g0 = b0 = r1 = g1 = b1 = 0;	\
	READ_RGB_Y(SIZE, 0, 0, C1,C2,C3,C4)	\
	READ_RGB_Y(SIZE, 1, 1, C1,C2,C3,C4)	\
	READ_RGB_Y(SIZE, 2, 0, C1,C2,C3,C4)	\
	READ_RGB_Y(SIZE, 3, 1, C1,C2,C3,C4)	\
	READ_RGB_UV(     0, 0)				\
	READ_RGB_UV(     1, 1)


/* yuyv/yuyvi input */

#define READ_YUYV_Y(ROW,C1,C2,C3,C4)	\
	y_ptr[(ROW)*y_stride+0] = x_ptr[(ROW)*x_stride+(C1)];	\
	y_ptr[(ROW)*y_stride+1] = x_ptr[(ROW)*x_stride+(C3)];
#define READ_YUYV_UV(UV_ROW,ROW1,ROW2,C1,C2,C3,C4) \
	u_ptr[(UV_ROW)*uv_stride] = (x_ptr[(ROW1)*x_stride+(C2)] + x_ptr[(ROW2)*x_stride+(C2)] + 1) / 2;	\
	v_ptr[(UV_ROW)*uv_stride] = (x_ptr[(ROW1)*x_stride+(C4)] + x_ptr[(ROW2)*x_stride+(C4)] + 1) / 2;

#define YUYV_TO_YV12_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */
#define YUYV_TO_YV12(SIZE,C1,C2,C3,C4)	\
	READ_YUYV_Y (0,      C1,C2,C3,C4)	\
	READ_YUYV_Y (1,      C1,C2,C3,C4)	\
	READ_YUYV_UV(0, 0,1, C1,C2,C3,C4)

#define YUYVI_TO_YV12_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */
#define YUYVI_TO_YV12(SIZE,C1,C2,C3,C4)	\
	READ_YUYV_Y (0, C1,C2,C3,C4)	\
	READ_YUYV_Y (1, C1,C2,C3,C4)	\
	READ_YUYV_Y (2, C1,C2,C3,C4)	\
	READ_YUYV_Y (3, C1,C2,C3,C4)	\
	READ_YUYV_UV(0, 0,2, C1,C2,C3,C4)	\
	READ_YUYV_UV(1, 1,3, C1,C2,C3,C4)


MAKE_COLORSPACE(rgb555_to_yv12_c,  2,2,2, RGB16_TO_YV12,  MK_RGB555, 0,0,0)
MAKE_COLORSPACE(rgb565_to_yv12_c,  2,2,2, RGB16_TO_YV12,  MK_RGB565, 0,0,0)
MAKE_COLORSPACE(bgr_to_yv12_c,     3,2,2, RGB_TO_YV12,    2,1,0, 0)
MAKE_COLORSPACE(bgra_to_yv12_c,    4,2,2, RGB_TO_YV12,    2,1,0, 0)
MAKE_COLORSPACE(rgb_to_yv12_c,     3,2,2, RGB_TO_YV12,    0,1,2, 0)
MAKE_COLORSPACE(abgr_to_yv12_c,    4,2,2, RGB_TO_YV12,    3,2,1, 0)
MAKE_COLORSPACE(rgba_to_yv12_c,    4,2,2, RGB_TO_YV12,    0,1,2, 0)
MAKE_COLORSPACE(argb_to_yv12_c,    4,2,2, RGB_TO_YV12,    1,2,3, 0)
MAKE_COLORSPACE(yuyv_to_yv12_c,    2,2,2, YUYV_TO_YV12,   0,1,2,3)
MAKE_COLORSPACE(uyvy_to_yv12_c,    2,2,2, YUYV_TO_YV12,   1,0,3,2)

MAKE_COLORSPACE(rgb555i_to_yv12_c, 2,2,4, RGB16I_TO_YV12, MK_RGB555, 0,0,0)
MAKE_COLORSPACE(rgb565i_to_yv12_c, 2,2,4, RGB16I_TO_YV12, MK_RGB565, 0,0,0)
MAKE_COLORSPACE(bgri_to_yv12_c,    3,2,4, RGBI_TO_YV12,   2,1,0, 0)
MAKE_COLORSPACE(bgrai_to_yv12_c,   4,2,4, RGBI_TO_YV12,   2,1,0, 0)
MAKE_COLORSPACE(abgri_to_yv12_c,   4,2,4, RGBI_TO_YV12,   3,2,1, 0)
MAKE_COLORSPACE(rgbi_to_yv12_c,    3,2,4, RGBI_TO_YV12,   0,1,2, 0)
MAKE_COLORSPACE(rgbai_to_yv12_c,   4,2,4, RGBI_TO_YV12,   0,1,2, 0)
MAKE_COLORSPACE(argbi_to_yv12_c,   4,2,4, RGBI_TO_YV12,   1,2,3, 0)
MAKE_COLORSPACE(yuyvi_to_yv12_c,   2,2,4, YUYVI_TO_YV12,  0,1,2,3)
MAKE_COLORSPACE(uyvyi_to_yv12_c,   2,2,4, YUYVI_TO_YV12,  1,0,3,2)


/********** colorspace output (yv12_to_xxx) functions **********/

/* yuv -> rgb def's */

#define RGB_Y_OUT		1.164
#define B_U_OUT			2.018
#define Y_ADD_OUT		16

#define G_U_OUT			0.391
#define G_V_OUT			0.813
#define U_ADD_OUT		128

#define R_V_OUT			1.596
#define V_ADD_OUT		128

#define SCALEBITS_OUT		13
#define FIX_OUT(x)		((uint16_t) ((x) * (1L<<SCALEBITS_OUT) + 0.5))


/* rgb16/rgb16i output */

#define MK_RGB555(R,G,B)	\
	((MAX(0,MIN(255, R)) << 7) & 0x7c00) | \
	((MAX(0,MIN(255, G)) << 2) & 0x03e0) | \
	((MAX(0,MIN(255, B)) >> 3) & 0x001f)

#define MK_RGB565(R,G,B)	\
	((MAX(0,MIN(255, R)) << 8) & 0xf800) | \
	((MAX(0,MIN(255, G)) << 3) & 0x07e0) | \
	((MAX(0,MIN(255, B)) >> 3) & 0x001f)

#define WRITE_RGB16(ROW,UV_ROW,C1)	\
	rgb_y = RGB_Y_tab[ y_ptr[y_stride*(ROW) + 0] ];						\
	b[ROW] = (b[ROW] & 0x7) + ((rgb_y + b_u##UV_ROW) >> SCALEBITS_OUT);	\
	g[ROW] = (g[ROW] & 0x7) + ((rgb_y - g_uv##UV_ROW) >> SCALEBITS_OUT);	\
	r[ROW] = (r[ROW] & 0x7) + ((rgb_y + r_v##UV_ROW) >> SCALEBITS_OUT);		\
	*(uint16_t *) (x_ptr+((ROW)*x_stride)+0) = C1(r[ROW], g[ROW], b[ROW]);	\
	rgb_y = RGB_Y_tab[ y_ptr[y_stride*(ROW) + 1] ];				\
	b[ROW] = (b[ROW] & 0x7) + ((rgb_y + b_u##UV_ROW) >> SCALEBITS_OUT);		\
	g[ROW] = (g[ROW] & 0x7) + ((rgb_y - g_uv##UV_ROW) >> SCALEBITS_OUT);	\
	r[ROW] = (r[ROW] & 0x7) + ((rgb_y + r_v##UV_ROW) >> SCALEBITS_OUT);		\
	*(uint16_t *) (x_ptr+((ROW)*x_stride)+2) = C1(r[ROW], g[ROW], b[ROW]);

#define YV12_TO_RGB16_ROW(SIZE,C1,C2,C3,C4) \
	int r[2], g[2], b[2];					\
	r[0] = r[1] = g[0] = g[1] = b[0] = b[1] = 0;
#define YV12_TO_RGB16(SIZE,C1,C2,C3,C4)		\
	int rgb_y; 												\
	int b_u0 = B_U_tab[ u_ptr[0] ];								\
	int g_uv0 = G_U_tab[ u_ptr[0] ] + G_V_tab[ v_ptr[0] ];		\
	int r_v0 = R_V_tab[ v_ptr[0] ];								\
	WRITE_RGB16(0, 0, C1)										\
	WRITE_RGB16(1, 0, C1)

#define YV12_TO_RGB16I_ROW(SIZE,C1,C2,C3,C4) \
	int r[4], g[4], b[4];					\
	r[0] = r[1] = r[2] = r[3] = 0;			\
	g[0] = g[1] = g[2] = g[3] = 0;			\
	b[0] = b[1] = b[2] = b[3] = 0;
#define YV12_TO_RGB16I(SIZE,C1,C2,C3,C4)		\
	int rgb_y; 													\
	int b_u0 = B_U_tab[ u_ptr[0] ];								\
	int g_uv0 = G_U_tab[ u_ptr[0] ] + G_V_tab[ v_ptr[0] ];		\
	int r_v0 = R_V_tab[ v_ptr[0] ];								\
    int b_u1 = B_U_tab[ u_ptr[uv_stride] ];						\
	int g_uv1 = G_U_tab[ u_ptr[uv_stride] ] + G_V_tab[ v_ptr[uv_stride] ];	\
	int r_v1 = R_V_tab[ v_ptr[uv_stride] ];						\
    WRITE_RGB16(0, 0, C1)										\
	WRITE_RGB16(1, 1, C1)										\
    WRITE_RGB16(2, 0, C1)										\
	WRITE_RGB16(3, 1, C1)										\


/* rgb/rgbi output */

#define WRITE_RGB(SIZE,ROW,UV_ROW,C1,C2,C3,C4)	\
	rgb_y = RGB_Y_tab[ y_ptr[(ROW)*y_stride + 0] ];						\
	x_ptr[(ROW)*x_stride+(C3)] = MAX(0, MIN(255, (rgb_y + b_u##UV_ROW) >> SCALEBITS_OUT));	\
	x_ptr[(ROW)*x_stride+(C2)] = MAX(0, MIN(255, (rgb_y - g_uv##UV_ROW) >> SCALEBITS_OUT));	\
	x_ptr[(ROW)*x_stride+(C1)] = MAX(0, MIN(255, (rgb_y + r_v##UV_ROW) >> SCALEBITS_OUT));	\
	if ((SIZE)>3) x_ptr[(ROW)*x_stride+(C4)] = 0;									\
	rgb_y = RGB_Y_tab[ y_ptr[(ROW)*y_stride + 1] ];									\
	x_ptr[(ROW)*x_stride+(SIZE)+(C3)] = MAX(0, MIN(255, (rgb_y + b_u##UV_ROW) >> SCALEBITS_OUT));	\
	x_ptr[(ROW)*x_stride+(SIZE)+(C2)] = MAX(0, MIN(255, (rgb_y - g_uv##UV_ROW) >> SCALEBITS_OUT));	\
	x_ptr[(ROW)*x_stride+(SIZE)+(C1)] = MAX(0, MIN(255, (rgb_y + r_v##UV_ROW) >> SCALEBITS_OUT));	\
	if ((SIZE)>3) x_ptr[(ROW)*x_stride+(SIZE)+(C4)] = 0;


#define YV12_TO_RGB_ROW(SIZE,C1,C2,C3,C4) 	/* nothing */
#define YV12_TO_RGB(SIZE,C1,C2,C3,C4)				\
	int rgb_y;												\
	int b_u0 = B_U_tab[ u_ptr[0] ];							\
	int g_uv0 = G_U_tab[ u_ptr[0] ] + G_V_tab[ v_ptr[0] ];	\
	int r_v0 = R_V_tab[ v_ptr[0] ];							\
	WRITE_RGB(SIZE, 0, 0, C1,C2,C3,C4)						\
	WRITE_RGB(SIZE, 1, 0, C1,C2,C3,C4)

#define YV12_TO_RGBI_ROW(SIZE,C1,C2,C3,C4) 	/* nothing */
#define YV12_TO_RGBI(SIZE,C1,C2,C3,C4)				\
	int rgb_y;												\
	int b_u0 = B_U_tab[ u_ptr[0] ];							\
	int g_uv0 = G_U_tab[ u_ptr[0] ] + G_V_tab[ v_ptr[0] ];	\
	int r_v0 = R_V_tab[ v_ptr[0] ];							\
    int b_u1 = B_U_tab[ u_ptr[uv_stride] ];					\
	int g_uv1 = G_U_tab[ u_ptr[uv_stride] ] + G_V_tab[ v_ptr[uv_stride] ];	\
	int r_v1 = R_V_tab[ v_ptr[uv_stride] ];					\
	WRITE_RGB(SIZE, 0, 0, C1,C2,C3,C4)		\
	WRITE_RGB(SIZE, 1, 1, C1,C2,C3,C4)		\
	WRITE_RGB(SIZE, 2, 0, C1,C2,C3,C4)		\
	WRITE_RGB(SIZE, 3, 1, C1,C2,C3,C4)


/* yuyv/yuyvi output */

#define WRITE_YUYV(ROW,UV_ROW,C1,C2,C3,C4)	\
	x_ptr[(ROW)*x_stride+(C1)] = y_ptr[   (ROW)*y_stride +0];	\
	x_ptr[(ROW)*x_stride+(C2)] = u_ptr[(UV_ROW)*uv_stride+0];	\
	x_ptr[(ROW)*x_stride+(C3)] = y_ptr[   (ROW)*y_stride +1];	\
	x_ptr[(ROW)*x_stride+(C4)] = v_ptr[(UV_ROW)*uv_stride+0];	\

#define YV12_TO_YUYV_ROW(SIZE,C1,C2,C3,C4) 	/* nothing */
#define YV12_TO_YUYV(SIZE,C1,C2,C3,C4)	\
	WRITE_YUYV(0, 0, C1,C2,C3,C4)		\
	WRITE_YUYV(1, 0, C1,C2,C3,C4)

#define YV12_TO_YUYVI_ROW(SIZE,C1,C2,C3,C4) /* nothing */
#define YV12_TO_YUYVI(SIZE,C1,C2,C3,C4)	\
	WRITE_YUYV(0, 0, C1,C2,C3,C4)		\
	WRITE_YUYV(1, 1, C1,C2,C3,C4)		\
	WRITE_YUYV(2, 0, C1,C2,C3,C4)		\
	WRITE_YUYV(3, 1, C1,C2,C3,C4)


MAKE_COLORSPACE(yv12_to_rgb555_c,  2,2,2, YV12_TO_RGB16,  MK_RGB555, 0,0,0)
MAKE_COLORSPACE(yv12_to_rgb565_c,  2,2,2, YV12_TO_RGB16,  MK_RGB565, 0,0,0)
MAKE_COLORSPACE(yv12_to_bgr_c,     3,2,2, YV12_TO_RGB,    2,1,0,0)
MAKE_COLORSPACE(yv12_to_bgra_c,    4,2,2, YV12_TO_RGB,	  2,1,0,3)
MAKE_COLORSPACE(yv12_to_abgr_c,    4,2,2, YV12_TO_RGB,    3,2,1,0)
MAKE_COLORSPACE(yv12_to_rgb_c,     3,2,2, YV12_TO_RGB,    0,1,2,0)
MAKE_COLORSPACE(yv12_to_rgba_c,    4,2,2, YV12_TO_RGB,    0,1,2,3)
MAKE_COLORSPACE(yv12_to_argb_c,    4,2,2, YV12_TO_RGB,    1,2,3,0)
MAKE_COLORSPACE(yv12_to_yuyv_c,    2,2,2, YV12_TO_YUYV,   0,1,2,3)
MAKE_COLORSPACE(yv12_to_uyvy_c,    2,2,2, YV12_TO_YUYV,   1,0,3,2)

MAKE_COLORSPACE(yv12_to_rgb555i_c, 2,2,4, YV12_TO_RGB16I, MK_RGB555, 0,0,0)
MAKE_COLORSPACE(yv12_to_rgb565i_c, 2,2,4, YV12_TO_RGB16I, MK_RGB565, 0,0,0)
MAKE_COLORSPACE(yv12_to_bgri_c,    3,2,4, YV12_TO_RGBI,   2,1,0, 0)
MAKE_COLORSPACE(yv12_to_bgrai_c,   4,2,4, YV12_TO_RGBI,	  2,1,0,3)
MAKE_COLORSPACE(yv12_to_abgri_c,   4,2,4, YV12_TO_RGBI,   3,2,1,0)
MAKE_COLORSPACE(yv12_to_rgbi_c,    3,2,4, YV12_TO_RGBI,   0,1,2,0)
MAKE_COLORSPACE(yv12_to_rgbai_c,   4,2,4, YV12_TO_RGBI,   0,1,2,3)
MAKE_COLORSPACE(yv12_to_argbi_c,   4,2,4, YV12_TO_RGBI,   1,2,3,0)
MAKE_COLORSPACE(yv12_to_yuyvi_c,   2,2,4, YV12_TO_YUYVI,  0,1,2,3)
MAKE_COLORSPACE(yv12_to_uyvyi_c,   2,2,4, YV12_TO_YUYVI,  1,0,3,2)



/* yv12 to yv12 copy function */

void
yv12_to_yv12_c(uint8_t * y_dst, uint8_t * u_dst, uint8_t * v_dst,
				int y_dst_stride, int uv_dst_stride,
				uint8_t * y_src, uint8_t * u_src, uint8_t * v_src,
				int y_src_stride, int uv_src_stride,
				int width, int height, int vflip)
{
	int width2 = width / 2;
	int height2 = height / 2;
	int y;
	const int with_uv = (u_src!=0 && v_src!=0);

	if (vflip) {
		y_src += (height - 1) * y_src_stride;
		if (with_uv) {
			u_src += (height2 - 1) * uv_src_stride;
			v_src += (height2 - 1) * uv_src_stride;
		}
		y_src_stride = -y_src_stride;
		uv_src_stride = -uv_src_stride;
	}

	for (y = height; y; y--) {
		memcpy(y_dst, y_src, width);
		y_src += y_src_stride;
		y_dst += y_dst_stride;
	}

	if (with_uv) {
		for (y = height2; y; y--) {
			memcpy(u_dst, u_src, width2);
			memcpy(v_dst, v_src, width2);
			u_src += uv_src_stride;
			u_dst += uv_dst_stride;
			v_src += uv_src_stride;
			v_dst += uv_dst_stride;
		}
	}
	else {
		for (y = height2; y; y--) {
			memset(u_dst, 0x80, width2);
			memset(v_dst, 0x80, width2);
			u_dst += uv_dst_stride;
			v_dst += uv_dst_stride;
		}
	}
}



/* initialize rgb lookup tables */

void
colorspace_init(void)
{
	int32_t i;

	for (i = 0; i < 256; i++) {
		RGB_Y_tab[i] = FIX_OUT(RGB_Y_OUT) * (i - Y_ADD_OUT);
		B_U_tab[i] = FIX_OUT(B_U_OUT) * (i - U_ADD_OUT);
		G_U_tab[i] = FIX_OUT(G_U_OUT) * (i - U_ADD_OUT);
		G_V_tab[i] = FIX_OUT(G_V_OUT) * (i - V_ADD_OUT);
		R_V_tab[i] = FIX_OUT(R_V_OUT) * (i - V_ADD_OUT);
	}
}
