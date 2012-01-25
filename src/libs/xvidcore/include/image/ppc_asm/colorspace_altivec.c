/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Colorspace conversion functions with altivec optimization -
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
 * $Id: colorspace_altivec.c,v 1.4 2005/03/18 18:01:34 edgomez Exp $
 *
 ****************************************************************************/

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "../../portab.h"
#include "../colorspace.h"

#undef DEBUG
#include <stdio.h>


/********** generic altivec RGB to YV12 colorspace macro **********/

#define MAKE_COLORSPACE_ALTIVEC_FROM_RGB(NAME,SIZE,PIXELS,VPIXELS,FUNC,C1,C2,C3,C4) \
void    \
NAME(uint8_t *x_ptr, int x_stride,      \
                                uint8_t *y_ptr, uint8_t *u_ptr, uint8_t *v_ptr, \
                                int y_stride, int uv_stride,    \
                                int width, int height, int vflip) \
{   \
    int fixed_width = (width + 15) & ~15;   \
    int x_dif = x_stride - (SIZE) * fixed_width; \
    int y_dif = y_stride - fixed_width; \
    int uv_dif = uv_stride - (fixed_width / 2); \
    int x, y;   \
    unsigned prefetch_constant; \
    \
    register vector unsigned int shift_consts[4]; \
    \
    vector unsigned char y_add; \
    vector unsigned char u_add; \
    vector unsigned char v_add; \
    \
    vector unsigned short vec_fix_ins[3];   \
    \
    vec_st(vec_ldl(0, &g_vec_fix_ins[0]), 0, &vec_fix_ins[0]);  \
    vec_st(vec_ldl(0, &g_vec_fix_ins[1]), 0, &vec_fix_ins[1]);  \
    vec_st(vec_ldl(0, &g_vec_fix_ins[2]), 0, &vec_fix_ins[2]);  \
    \
    shift_consts[0] = vec_add(vec_splat_u32(12), vec_splat_u32(12));    \
    shift_consts[1] = vec_add(vec_splat_u32(8), vec_splat_u32(8));  \
    shift_consts[2] = vec_splat_u32(8); \
    shift_consts[3] = vec_splat_u32(0); \
    \
    prefetch_constant = build_prefetch(16, 2, (short)x_stride); \
    vec_dstt(x_ptr, prefetch_constant, 0);  \
    vec_dstt(x_ptr + (x_stride << 1), prefetch_constant, 1);    \
    \
    *((unsigned char*)&y_add) = Y_ADD_IN;   \
    *((unsigned char*)&u_add) = U_ADD_IN;   \
    *((unsigned char*)&v_add) = V_ADD_IN;   \
    \
    y_add = vec_splat(y_add, 0);    \
    u_add = vec_splat(u_add, 0);    \
    v_add = vec_splat(v_add, 0);    \
    \
    if(vflip) { \
        x_ptr += (height - 1) * x_stride;   \
        x_dif = -(SIZE) * fixed_width - x_stride;    \
        x_stride = -x_stride;   \
    }   \
    \
    for(y = 0; y < height; y += (VPIXELS)) {    \
        FUNC##_ROW(SIZE,C1,C2,C3,C4);   \
        for(x = 0; x < fixed_width; x += (PIXELS)) {    \
            FUNC(SIZE,C1,C2,C3,C4); \
            x_ptr += (PIXELS)*(SIZE);   \
            y_ptr += (PIXELS);  \
            u_ptr += (PIXELS)/2;    \
            v_ptr += (PIXELS)/2;    \
        }   \
        x_ptr += x_dif + (VPIXELS-1) * x_stride; \
        y_ptr += y_dif + (VPIXELS-1) * y_stride; \
        u_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride;    \
        v_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride;    \
    }   \
    vec_dssall(); \
}


/********** generic altivec YUV to YV12 colorspace macro **********/

#define MAKE_COLORSPACE_ALTIVEC_FROM_YUV(NAME,SIZE,PIXELS,VPIXELS,FUNC,C1,C2,C3,C4) \
void    \
NAME(uint8_t *x_ptr, int x_stride,  \
                                uint8_t *y_ptr, uint8_t *u_ptr, uint8_t *v_ptr, \
                                int y_stride, int uv_stride,    \
                                int width, int height, int vflip)   \
{   \
    int fixed_width = (width + 15) & ~15;   \
    int x_dif = x_stride - (SIZE)*fixed_width; \
    int y_dif = y_stride - fixed_width;     \
    int uv_dif = uv_stride - (fixed_width / 2); \
    int x, y;   \
    \
    unsigned prefetch_constant; \
    \
    vector unsigned int p0, p1;         \
    vector unsigned char lum0, lum1;    \
    vector unsigned char u0, u1;        \
    vector unsigned char v0, v1;        \
    vector unsigned char t;             \
    \
    prefetch_constant = build_prefetch(16, 2, (short)x_stride); \
    vec_dstt(x_ptr, prefetch_constant, 0);                      \
    vec_dstt(x_ptr + (x_stride << 1), prefetch_constant, 1);    \
    \
    if(vflip) { \
        x_ptr += (height - 1) * x_stride;       \
        x_dif = -(SIZE)*fixed_width - x_stride;    \
        x_stride = -x_stride;                   \
    }   \
    \
    for(y = 0; y < height; y += (VPIXELS)) {    \
        FUNC##_ROW(SIZE,C1,C2,C3,C4);   \
        for(x = 0; x < fixed_width; x += (PIXELS)) {   \
            FUNC(SIZE,C1,C2,C3,C4); \
            x_ptr += (PIXELS)*(SIZE);    \
            y_ptr += (PIXELS);    \
            u_ptr += (PIXELS)/2;     \
            v_ptr += (PIXELS)/2;     \
        }   \
        x_ptr += x_dif + (VPIXELS-1) * x_stride;  \
        y_ptr += y_dif + (VPIXELS-1) * y_stride;  \
        u_ptr += uv_dif + ((VPIXELS/2)-1) * uv_stride;    \
        v_ptr += uv_dif + ((VPIXELS/2)-1) * uv_stride;    \
    }   \
    vec_dssall();   \
}


/********** generic altivec YV12 to YUV colorspace macro **********/

#define MAKE_COLORSPACE_ALTIVEC_TO_YUV(NAME,SIZE,PIXELS,VPIXELS,FUNC,C1,C2,C3,C4) \
void    \
NAME(uint8_t *x_ptr, int x_stride,  \
                                uint8_t *y_ptr, uint8_t *u_ptr, uint8_t *v_ptr, \
                                int y_stride, int uv_stride,    \
                                int width, int height, int vflip)   \
{ \
    int fixed_width = (width + 15) & ~15;       \
    int x_dif = x_stride - (SIZE)*fixed_width;  \
    int y_dif = y_stride - fixed_width;         \
    int uv_dif = uv_stride - (fixed_width / 2); \
    int x, y;                                   \
    \
    vector unsigned char y_vec;         \
    vector unsigned char u_vec;         \
    vector unsigned char v_vec;         \
    vector unsigned char p0, p1, ptmp;  \
    vector unsigned char mask;          \
    vector unsigned char mask_stencil;  \
    vector unsigned char t;             \
    vector unsigned char m4;            \
    vector unsigned char vec4;          \
    \
    unsigned prefetch_constant_y; \
    unsigned prefetch_constant_uv; \
    \
    prefetch_constant_y = build_prefetch(16, 4, (short)y_stride);   \
    prefetch_constant_uv = build_prefetch(16, 2, (short)uv_stride); \
    \
    vec_dstt(y_ptr, prefetch_constant_y, 0);    \
    vec_dstt(u_ptr, prefetch_constant_uv, 1);   \
    vec_dstt(v_ptr, prefetch_constant_uv, 2);   \
    \
    mask_stencil = (vector unsigned char)vec_mergeh( (vector unsigned short)vec_mergeh(vec_splat_u8(-1), vec_splat_u8(0)), vec_splat_u16(0) ); \
    m4 = vec_sr(vec_lvsl(0, (unsigned char*)0), vec_splat_u8(2)); \
    vec4 = vec_splat_u8(4);                                       \
    \
    if(vflip) { \
        x_ptr += (height - 1) * x_stride;       \
        x_dif = -(SIZE)*fixed_width - x_stride; \
        x_stride = -x_stride;                   \
    } \
    \
    for(y = 0; y < height; y += (VPIXELS)) {    \
        FUNC##_ROW(SIZE,C1,C2,C3,C4); \
        for(x = 0; x < fixed_width; x += (PIXELS)) { \
            FUNC(SIZE,C1,C2,C3,C4);     \
            x_ptr += (PIXELS)*(SIZE);   \
            y_ptr += (PIXELS);          \
            u_ptr += (PIXELS)/2;        \
            v_ptr += (PIXELS)/2;        \
        } \
        x_ptr += x_dif + (VPIXELS-1) * x_stride;        \
        y_ptr += y_dif + (VPIXELS-1) * y_stride;        \
        u_ptr += uv_dif + ((VPIXELS/2)-1) * uv_stride;  \
        v_ptr += uv_dif + ((VPIXELS/2)-1) * uv_stride;  \
    } \
    vec_dssall(); \
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

#define SCALEBITS_IN	8
#define FIX_IN(x)		((uint16_t) ((x) * (1L<<SCALEBITS_IN) + 0.5))


static inline unsigned
build_prefetch(unsigned char block_size, unsigned char block_count, short stride)
{    
    return ((block_size << 24) | (block_count << 16) | stride);
}

const static vector unsigned short g_vec_fix_ins [3] = {
    (vector unsigned short)AVV( SCALEBITS_IN, FIX_IN(Y_R_IN), FIX_IN(Y_G_IN), FIX_IN(Y_B_IN), 0, 0, 0, 0),
    (vector unsigned short)AVV( SCALEBITS_IN + 2, -FIX_IN(U_R_IN), -FIX_IN(U_G_IN), FIX_IN(U_B_IN), 0, 0, 0, 0),
    (vector unsigned short)AVV( SCALEBITS_IN + 2, FIX_IN(V_R_IN), -FIX_IN(V_G_IN), -FIX_IN(V_B_IN), 0, 0, 0, 0)
};

/* RGB Input */
#define READ_RGB_Y_ALTIVEC(SIZE,ROW,UVID,C1,C2,C3,C4)   \
        p0 = vec_ld(0, (unsigned int*)(x_ptr + (ROW) * x_stride));  \
        p1 = vec_ld(16, (unsigned int*)(x_ptr + (ROW) * x_stride)); \
        \
        mask = vec_mergeh((vector unsigned char)shift_consts[3], vec_splat_u8(-1));   \
        mask = (vector unsigned char)vec_mergeh((vector unsigned short)shift_consts[3], (vector unsigned short)mask);   \
        \
        t0 = vec_sr(p0, shift_consts[C1]);  \
        t0 = vec_sel(shift_consts[3], t0, (vector unsigned int)mask);  \
        t1 = vec_sr(p1, shift_consts[C1]);  \
        t1 = vec_sel(shift_consts[3], t1, (vector unsigned int)mask);  \
        r = vec_pack(t0, t1);  \
        r##UVID = vec_add(r##UVID, r);  \
        \
        t0 = vec_sr(p0, shift_consts[C2]);  \
        t0 = vec_sel(shift_consts[3], t0, (vector unsigned int)mask);  \
        t1 = vec_sr(p1, shift_consts[C2]);  \
        t1 = vec_sel(shift_consts[3], t1, (vector unsigned int)mask);  \
        g = vec_pack(t0, t1);  \
        g##UVID = vec_add(g##UVID, g);  \
        \
        t0 = vec_sr(p0, shift_consts[C3]);  \
        t0 = vec_sel(shift_consts[3], t0, (vector unsigned int)mask);  \
        t1 = vec_sr(p1, shift_consts[C3]);  \
        t1 = vec_sel(shift_consts[3], t1, (vector unsigned int)mask);  \
        b = vec_pack(t0, t1);  \
        b##UVID = vec_add(b##UVID, b);  \
        \
        lum = vec_mladd(r, vec_splat(vec_fix_ins[0], 1), (vector unsigned short)shift_consts[3]);  \
        lum = vec_mladd(g, vec_splat(vec_fix_ins[0], 2), lum); \
        lum = vec_mladd(b, vec_splat(vec_fix_ins[0], 3), lum); \
        lum = vec_sr(lum, vec_splat(vec_fix_ins[0], 0));    \
        y_vec = vec_pack(lum, (vector unsigned short)shift_consts[3]);  \
        y_vec = vec_add(y_vec, y_add);  \
        \
        mask = vec_pack((vector unsigned short)shift_consts[3], vec_splat_u16(-1)); \
        mask = vec_perm(mask, mask, vec_lvsl(0, y_ptr + (ROW)*y_stride));    \
        y_vec = vec_perm(y_vec, y_vec, vec_lvsl(0, y_ptr + (ROW)*y_stride)); \
        y_vec = vec_sel(y_vec, vec_ld(0, y_ptr + (ROW)*y_stride), mask); \
        vec_st(y_vec, 0, y_ptr + (ROW)*y_stride)

#define READ_RGB_UV_ALTIVEC(UV_ROW,UVID) \
        r##UVID = (vector unsigned short)vec_sum4s((vector signed short)r##UVID, (vector signed int)shift_consts[3]); \
        g##UVID = (vector unsigned short)vec_sum4s((vector signed short)g##UVID, (vector signed int)shift_consts[3]); \
        b##UVID = (vector unsigned short)vec_sum4s((vector signed short)b##UVID, (vector signed int)shift_consts[3]); \
        \
        t3 = vec_mulo((vector signed short)r##UVID, (vector signed short)vec_splat(vec_fix_ins[1], 1)); \
        t3 = vec_add(t3, vec_mulo((vector signed short)g##UVID, (vector signed short)vec_splat(vec_fix_ins[1], 2))); \
        t3 = vec_add(t3, vec_mulo((vector signed short)b##UVID, (vector signed short)vec_splat(vec_fix_ins[1], 3))); \
        t3 = vec_sr(t3, (vector unsigned int)vec_mergeh((vector unsigned short)shift_consts[3], vec_splat(vec_fix_ins[1], 0))); \
        \
        u_vec = vec_pack(vec_pack((vector unsigned int)t3, shift_consts[3]), (vector unsigned short)shift_consts[3]);  \
        u_vec = vec_add(u_vec, u_add);  \
        \
        mask = vec_pack(vec_splat_u16(-1), (vector unsigned short)shift_consts[3]); \
        mask = (vector unsigned char)vec_pack((vector unsigned int)mask, shift_consts[3]); \
        mask = vec_perm(mask, mask, vec_lvsr(0, u_ptr + (UV_ROW)*uv_stride));    \
        u_vec = vec_perm(u_vec, u_vec, vec_lvsr(0, u_ptr + (UV_ROW)*uv_stride)); \
        u_vec = vec_sel(vec_ld(0, u_ptr + (UV_ROW)*uv_stride), u_vec, mask); \
        vec_st(u_vec, 0, u_ptr + (UV_ROW)*uv_stride);    \
        \
        t3 = vec_mulo((vector signed short)r##UVID, (vector signed short)vec_splat(vec_fix_ins[2], 1));  \
        t3 = vec_add(t3, vec_mulo((vector signed short)g##UVID, (vector signed short)vec_splat(vec_fix_ins[2], 2))); \
        t3 = vec_add(t3, vec_mulo((vector signed short)b##UVID, (vector signed short)vec_splat(vec_fix_ins[2], 3))); \
        t3 = vec_sr(t3, (vector unsigned int)vec_mergeh((vector unsigned short)shift_consts[3], vec_splat(vec_fix_ins[2], 0))); \
        \
        v_vec = vec_pack(vec_pack((vector unsigned int)t3, shift_consts[3]), (vector unsigned short)shift_consts[3]);  \
        v_vec = vec_add(v_vec, v_add);  \
        \
        mask = vec_pack(vec_splat_u16(-1), (vector unsigned short)shift_consts[3]); \
        mask = (vector unsigned char)vec_pack((vector unsigned int)mask, shift_consts[3]); \
        mask = vec_perm(mask, mask, vec_lvsr(0, v_ptr + (UV_ROW) * uv_stride));    \
        v_vec = vec_perm(v_vec, v_vec, vec_lvsr(0, v_ptr + (UV_ROW) * uv_stride)); \
        v_vec = vec_sel(vec_ld(0, v_ptr + (UV_ROW) * uv_stride), v_vec, mask);  \
        vec_st(v_vec, 0, v_ptr + (UV_ROW) * uv_stride)


#define RGB_TO_YV12_ALTIVEC_ROW(SIZE,C1,C2,C3,C4) \
	/* nothing */

#define RGB_TO_YV12_ALTIVEC(SIZE,C1,C2,C3,C4)	\
            vector unsigned int p0, p1; \
            vector unsigned int t0, t1;         \
            vector unsigned short r, g, b, r0, g0, b0;  \
            vector unsigned short lum;    \
            vector unsigned char mask;  \
            vector unsigned char y_vec;    \
            vector unsigned char u_vec; \
            vector unsigned char v_vec; \
            vector signed int t3;   \
            \
            vec_dstt(x_ptr, prefetch_constant, 0);  \
            vec_dstt(x_ptr + (x_stride << 1), prefetch_constant, 1);    \
            \
            r0 = g0 = b0 = (vector unsigned short)shift_consts[3];  \
            \
            READ_RGB_Y_ALTIVEC(SIZE, 0, 0, C1, C2, C3, C4); \
            READ_RGB_Y_ALTIVEC(SIZE, 1, 0, C1, C2, C3, C4); \
            READ_RGB_UV_ALTIVEC(0, 0)


/* YUV input */

#define READ_YUYV_Y_ALTIVEC(ROW,C1,C2,C3,C4)    \
    p0 = vec_ld(0, (unsigned int*)(x_ptr + (ROW)*x_stride));  \
    p1 = vec_ld(16, (unsigned int*)(x_ptr + (ROW)*x_stride)); \
    \
    t = vec_lvsl(0, (unsigned char*)0); \
    t = vec_sl(t, vec_splat_u8(2));     \
    t = vec_add(t, vec_splat_u8(C1));   \
    \
    lum0 = (vector unsigned char)vec_perm(p0, p0, t);   \
    lum1 = (vector unsigned char)vec_perm(p1, p1, t);   \
    \
    t = vec_lvsl(0, (unsigned char*)0); \
    t = vec_sl(t, vec_splat_u8(2));     \
    t = vec_add(t, vec_splat_u8(C3));    \
    \
    lum0 = vec_mergeh(lum0, (vector unsigned char)vec_perm(p0, p0, t)); \
    lum1 = vec_mergeh(lum1, (vector unsigned char)vec_perm(p1, p1, t)); \
    \
    lum0 = vec_sel(lum0, lum1, vec_pack(vec_splat_u16(0), vec_splat_u16(-1)));  \
    vec_st(lum0, 0, y_ptr + (ROW)*y_stride);    \
    \
    t = vec_lvsl(0, (unsigned char*)0); \
    t = vec_sl(t, vec_splat_u8(2));     \
    t = vec_add(t, vec_splat_u8(C2));    \
    \
    lum0 = (vector unsigned char)vec_perm(p0, p0, t); \
    lum1 = (vector unsigned char)vec_perm(p1, p1, t); \
    lum1 = vec_perm(lum1, lum1, vec_lvsr(4, (unsigned char*)0));    \
    t = vec_pack(vec_pack(vec_splat_u32(0), vec_splat_u32(-1)), vec_splat_u16(-1)); \
    u##ROW = vec_sel(lum0, lum1, t);    \
    \
    t = vec_lvsl(0, (unsigned char*)0); \
    t = vec_sl(t, vec_splat_u8(2));     \
    t = vec_add(t, vec_splat_u8(C4));   \
    \
    lum0 = (vector unsigned char)vec_perm(p0, p0, t);   \
    lum1 = (vector unsigned char)vec_perm(p1, p1, t);   \
    lum1 = vec_perm(lum1, lum1, vec_lvsr(4, (unsigned char*)0));    \
    t = vec_pack(vec_pack(vec_splat_u32(0), vec_splat_u32(-1)), vec_splat_u16(-1)); \
    v##ROW = vec_sel(lum0, lum1, t);

#define READ_YUYV_UV_ALTIVEC(UV_ROW,ROW1,ROW2,C1,C2,C3,C4)  \
    u##ROW1 = vec_avg(u##ROW1, u##ROW2);    \
    t = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));  \
    t = vec_perm(t, t, vec_lvsl(0, u_ptr + (UV_ROW)*uv_stride));    \
    u##ROW1 = vec_perm(u##ROW1, u##ROW1, vec_lvsl(0, u_ptr + (UV_ROW)*uv_stride));  \
    u##ROW1 = vec_sel(u##ROW1, vec_ld(0, u_ptr + (UV_ROW)*uv_stride), t);   \
    vec_st(u##ROW1, 0, u_ptr + (UV_ROW)*uv_stride); \
    \
    v##ROW1 = vec_avg(v##ROW1, v##ROW2);    \
    t = vec_pack(vec_splat_u16(0), vec_splat_u16(-1));  \
    t = vec_perm(t, t, vec_lvsl(0, v_ptr + (UV_ROW)*uv_stride));    \
    v##ROW1 = vec_perm(v##ROW1, v##ROW1, vec_lvsl(0, v_ptr + (UV_ROW)*uv_stride));  \
    v##ROW1 = vec_sel(v##ROW1, vec_ld(0, v_ptr + (UV_ROW)*uv_stride), t);   \
    vec_st(v##ROW1, 0, v_ptr + (UV_ROW)*uv_stride);


#define YUYV_TO_YV12_ALTIVEC_ROW(SIZE,C1,C2,C3,C4)  \
        /*nothing*/

#define YUYV_TO_YV12_ALTIVEC(SIZE,C1,C2,C3,C4)  \
        vec_dstt(x_ptr, prefetch_constant, 0);  \
        vec_dstt(x_ptr + (x_stride << 1), prefetch_constant, 1);    \
        \
        READ_YUYV_Y_ALTIVEC (0,         C1,C2,C3,C4)    \
        READ_YUYV_Y_ALTIVEC (1,         C1,C2,C3,C4)    \
        READ_YUYV_UV_ALTIVEC(0, 0, 1,   C1,C2,C3,C4)

MAKE_COLORSPACE_ALTIVEC_FROM_RGB(bgra_to_yv12_altivec_c, 4, 8, 2, RGB_TO_YV12_ALTIVEC, 2, 1, 0, 0)
MAKE_COLORSPACE_ALTIVEC_FROM_RGB(abgr_to_yv12_altivec_c, 4, 8, 2, RGB_TO_YV12_ALTIVEC, 3, 2, 1, 0)
MAKE_COLORSPACE_ALTIVEC_FROM_RGB(rgba_to_yv12_altivec_c, 4, 8, 2, RGB_TO_YV12_ALTIVEC, 0, 1, 2, 0)
MAKE_COLORSPACE_ALTIVEC_FROM_RGB(argb_to_yv12_altivec_c, 4, 8, 2, RGB_TO_YV12_ALTIVEC, 1, 2, 3, 0)

MAKE_COLORSPACE_ALTIVEC_FROM_YUV(yuyv_to_yv12_altivec_c, 2, 16, 2, YUYV_TO_YV12_ALTIVEC, 0, 1, 2, 3)
MAKE_COLORSPACE_ALTIVEC_FROM_YUV(uyvy_to_yv12_altivec_c, 2, 16, 2, YUYV_TO_YV12_ALTIVEC, 1, 0, 3, 2)
    

#define WRITE_YUYV_ALTIVEC(ROW, UV_ROW, C1,C2,C3,C4) \
    p0 = vec_splat_u8(0);   \
    p1 = vec_splat_u8(0);   \
    \
    y_vec = vec_perm(vec_ld(0, y_ptr + (ROW)*y_stride), vec_ld(16, y_ptr + (ROW)*y_stride), vec_lvsl(0, y_ptr + (ROW)*y_stride)); \
    /* C1 */ \
    t = vec_perm(y_vec, y_vec, vec_sl(vec_lvsl(0, (unsigned char*)0), vec_splat_u8(1)));   \
    mask = vec_perm(mask_stencil, mask_stencil, vec_lvsr(C1, (unsigned char*)0));           \
    \
    p0 = vec_sel(p0, vec_perm(t, t, m4), mask);                   \
	ptmp = vec_perm(t,t, vec_add(m4, vec4));\
    p1 = vec_sel(p1, ptmp, mask);    \
    \
    /* C3 */ \
	ptmp = vec_add(vec_sl(vec_lvsl(0, (unsigned char*)0), vec_splat_u8(1)), vec_splat_u8(1));	\
    t = vec_perm(y_vec, y_vec, ptmp); \
    mask = vec_perm(mask_stencil, mask_stencil, vec_lvsr(C3, (unsigned char*)0));                                   \
    \
    p0 = vec_sel(p0, vec_perm(t, t, m4), mask);                   \
	ptmp = vec_perm(t, t, vec_add(m4, vec4));	\
    p1 = vec_sel(p1, ptmp, mask);    \
    \
    /* C2 */ \
    u_vec = vec_perm(vec_ld(0,u_ptr), vec_ld(16, u_ptr), vec_lvsl(0, u_ptr));       \
    mask = vec_perm(mask_stencil, mask_stencil, vec_lvsr(C2, (unsigned char*)0));   \
    \
    p0 = vec_sel(p0, vec_perm(u_vec, u_vec, m4), mask);                 \
	ptmp = vec_perm(u_vec, u_vec, vec_add(m4, vec4)); \
    p1 = vec_sel(p1, ptmp, mask);  \
    \
    /* C4 */ \
    v_vec = vec_perm(vec_ld(0, v_ptr), vec_ld(16, v_ptr), vec_lvsl(0, v_ptr));      \
    mask = vec_perm(mask_stencil, mask_stencil, vec_lvsr(C4, (unsigned char*)0));   \
    \
    p0 = vec_sel(p0, vec_perm(v_vec, v_vec, m4), mask);                 \
	ptmp = vec_perm(v_vec, v_vec, vec_add(m4, vec4));	\
    p1 = vec_sel(p1, ptmp, mask);  \
    \
    vec_st(p0, 0, x_ptr + (ROW)*x_stride);   \
    vec_st(p1, 16, x_ptr + (ROW)*x_stride)


#define YV12_TO_YUYV_ALTIVEC_ROW(SIZE,C1,C2,C3,C4) \
        /*nothing*/

#define YV12_TO_YUYV_ALTIVEC(SIZE,C1,C2,C3,C4)  \
        vec_dstt(y_ptr, prefetch_constant_y, 0);    \
        vec_dstt(u_ptr, prefetch_constant_uv, 1);   \
        vec_dstt(v_ptr, prefetch_constant_uv, 2);   \
        \
        WRITE_YUYV_ALTIVEC(0, 0, C1,C2,C3,C4);   \
        WRITE_YUYV_ALTIVEC(1, 0, C1,C2,C3,C4)


MAKE_COLORSPACE_ALTIVEC_TO_YUV(yv12_to_yuyv_altivec_unaligned_c, 2, 16, 2, YV12_TO_YUYV_ALTIVEC, 0, 1, 2, 3)
MAKE_COLORSPACE_ALTIVEC_TO_YUV(yv12_to_uyvy_altivec_unaligned_c, 2, 16, 2, YV12_TO_YUYV_ALTIVEC, 1, 0, 3, 2)


/* This intermediate functions are used because gcc v3.3 seems to produces an invalid register usage with the fallback directly integrated in the altivec routine (!!!) */

#define CHECK_COLORSPACE_ALTIVEC_TO_YUV(NAME,FAST,FALLBACK) \
void \
NAME(uint8_t *x_ptr, int x_stride,  \
                                uint8_t *y_ptr, uint8_t *u_ptr, uint8_t *v_ptr, \
                                int y_stride, int uv_stride,    \
                                int width, int height, int vflip)   \
{\
	if( ((uint32_t)x_ptr & 15) | (x_stride & 15) )\
		FALLBACK(x_ptr, x_stride, y_ptr, u_ptr, v_ptr, y_stride, uv_stride, width, height, vflip);\
	else\
		FAST(x_ptr, x_stride, y_ptr, u_ptr, v_ptr, y_stride, uv_stride, width, height, vflip);\
}

CHECK_COLORSPACE_ALTIVEC_TO_YUV(yv12_to_yuyv_altivec_c, yv12_to_yuyv_altivec_unaligned_c, yv12_to_yuyv_c)
CHECK_COLORSPACE_ALTIVEC_TO_YUV(yv12_to_uyvy_altivec_c, yv12_to_uyvy_altivec_unaligned_c, yv12_to_uyvy_c)
