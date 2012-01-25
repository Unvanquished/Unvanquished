/*
 * Copyright (c) 2001 Michel Lespinasse
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * XviD integration by Christoph NŠgeli <chn@kbw.ch>
 *
 * This file is a direct copy of the altivec idct module from the libmpeg2
 * project with some minor changes to fit in XviD.
 */


#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "../../portab.h"

#define IDCT_Vectors \
    vector signed short vx0, vx1, vx2, vx3, vx4, vx5, vx6, vx7;			\
    vector signed short vy0, vy1, vy2, vy3, vy4, vy5, vy6, vy7;			\
    vector signed short a0, a1, a2, ma2, c4, mc4, zero, bias;                   \
    vector signed short t0, t1, t2, t3, t4, t5, t6, t7, t8;                     \
    vector unsigned short shift

static const vector signed short constants [5] = {
    (vector signed short)AVV(23170, 13573, 6518, 21895, -23170, -21895, 32, 31),
    (vector signed short)AVV(16384, 22725, 21407, 19266, 16384, 19266, 21407, 22725),
    (vector signed short)AVV(16069, 22289, 20995, 18895, 16069, 18895, 20995, 22289),
    (vector signed short)AVV(21407, 29692, 27969, 25172, 21407, 25172, 27969, 29692),
    (vector signed short)AVV(13623, 18895, 17799, 16019, 13623, 16019, 17799, 18895)
};

#define IDCT()\
    c4 = vec_splat       (constants[0], 0); \
    a0 = vec_splat       (constants[0], 1); \
    a1 = vec_splat       (constants[0], 2); \
    a2 = vec_splat       (constants[0], 3); \
    mc4 = vec_splat      (constants[0], 4); \
    ma2 = vec_splat      (constants[0], 5); \
    bias = (vector signed short)vec_splat((vector signed int)constants[0], 3);  \
    \
    zero = vec_splat_s16 (0);   \
    \
    vx0 = vec_adds (block[0], block[4]);        \
    vx4 = vec_subs (block[0], block[4]);        \
    t5 = vec_mradds (vx0, constants[1], zero);  \
    t0 = vec_mradds (vx4, constants[1], zero);  \
    \
    vx1 = vec_mradds (a1, block[7], block[1]);                  \
    vx7 = vec_mradds (a1, block[1], vec_subs (zero, block[7])); \
    t1 = vec_mradds (vx1, constants[2], zero);                  \
    t8 = vec_mradds (vx7, constants[2], zero);                  \
    \
    vx2 = vec_mradds (a0, block[6], block[2]);                  \
    vx6 = vec_mradds (a0, block[2], vec_subs (zero, block[6])); \
    t2 = vec_mradds (vx2, constants[3], zero);                  \
    t4 = vec_mradds (vx6, constants[3], zero);                  \
    \
    vx3 = vec_mradds (block[3], constants[4], zero);    \
    vx5 = vec_mradds (block[5], constants[4], zero);    \
    t7 = vec_mradds (a2, vx5, vx3);                     \
    t3 = vec_mradds (ma2, vx3, vx5);                    \
    \
    t6 = vec_adds (t8, t3); \
    t3 = vec_subs (t8, t3); \
    t8 = vec_subs (t1, t7); \
    t1 = vec_adds (t1, t7); \
    t6 = vec_mradds (a0, t6, t6);   \
    t1 = vec_mradds (a0, t1, t1);   \
    \
    t7 = vec_adds (t5, t2); \
    t2 = vec_subs (t5, t2); \
    t5 = vec_adds (t0, t4); \
    t0 = vec_subs (t0, t4); \
    t4 = vec_subs (t8, t3); \
    t3 = vec_adds (t8, t3); \
    \
    vy0 = vec_adds (t7, t1);    \
    vy7 = vec_subs (t7, t1);    \
    vy1 = vec_adds (t5, t3);    \
    vy6 = vec_subs (t5, t3);    \
    vy2 = vec_adds (t0, t4);    \
    vy5 = vec_subs (t0, t4);    \
    vy3 = vec_adds (t2, t6);    \
    vy4 = vec_subs (t2, t6);    \
    \
    vx0 = vec_mergeh (vy0, vy4);    \
    vx1 = vec_mergel (vy0, vy4);    \
    vx2 = vec_mergeh (vy1, vy5);    \
    vx3 = vec_mergel (vy1, vy5);    \
    vx4 = vec_mergeh (vy2, vy6);    \
    vx5 = vec_mergel (vy2, vy6);    \
    vx6 = vec_mergeh (vy3, vy7);    \
    vx7 = vec_mergel (vy3, vy7);    \
    \
    vy0 = vec_mergeh (vx0, vx4);    \
    vy1 = vec_mergel (vx0, vx4);    \
    vy2 = vec_mergeh (vx1, vx5);    \
    vy3 = vec_mergel (vx1, vx5);    \
    vy4 = vec_mergeh (vx2, vx6);    \
    vy5 = vec_mergel (vx2, vx6);    \
    vy6 = vec_mergeh (vx3, vx7);    \
    vy7 = vec_mergel (vx3, vx7);    \
    \
    vx0 = vec_mergeh (vy0, vy4);    \
    vx1 = vec_mergel (vy0, vy4);    \
    vx2 = vec_mergeh (vy1, vy5);    \
    vx3 = vec_mergel (vy1, vy5);    \
    vx4 = vec_mergeh (vy2, vy6);    \
    vx5 = vec_mergel (vy2, vy6);    \
    vx6 = vec_mergeh (vy3, vy7);    \
    vx7 = vec_mergel (vy3, vy7);    \
    \
    vx0 = vec_adds (vx0, bias); \
    t5 = vec_adds (vx0, vx4);   \
    t0 = vec_subs (vx0, vx4);   \
    \
    t1 = vec_mradds (a1, vx7, vx1);                     \
    t8 = vec_mradds (a1, vx1, vec_subs (zero, vx7));    \
    \
    t2 = vec_mradds (a0, vx6, vx2);                     \
    t4 = vec_mradds (a0, vx2, vec_subs (zero, vx6));    \
    \
    t7 = vec_mradds (a2, vx5, vx3);     \
    t3 = vec_mradds (ma2, vx3, vx5);    \
    \
    t6 = vec_adds (t8, t3); \
    t3 = vec_subs (t8, t3); \
    t8 = vec_subs (t1, t7); \
    t1 = vec_adds (t1, t7); \
    \
    t7 = vec_adds (t5, t2); \
    t2 = vec_subs (t5, t2); \
    t5 = vec_adds (t0, t4); \
    t0 = vec_subs (t0, t4); \
    t4 = vec_subs (t8, t3); \
    t3 = vec_adds (t8, t3); \
    \
    vy0 = vec_adds (t7, t1);        \
    vy7 = vec_subs (t7, t1);        \
    vy1 = vec_mradds (c4, t3, t5);  \
    vy6 = vec_mradds (mc4, t3, t5); \
    vy2 = vec_mradds (c4, t4, t0);  \
    vy5 = vec_mradds (mc4, t4, t0); \
    vy3 = vec_adds (t2, t6);        \
    vy4 = vec_subs (t2, t6);        \
    \
    shift = vec_splat_u16 (6);  \
    vx0 = vec_sra (vy0, shift); \
    vx1 = vec_sra (vy1, shift); \
    vx2 = vec_sra (vy2, shift); \
    vx3 = vec_sra (vy3, shift); \
    vx4 = vec_sra (vy4, shift); \
    vx5 = vec_sra (vy5, shift); \
    vx6 = vec_sra (vy6, shift); \
    vx7 = vec_sra (vy7, shift)

void
idct_altivec_c(vector short *const block)
{
    int i;
    int j;
    short block2[64];
    short *block_ptr;
    IDCT_Vectors;
    
    block_ptr = (short*)block;
    for (i = 0; i < 64; i++)
	block2[i] = block_ptr[i];

    for (i = 0; i < 8; i++)
	for (j = 0; j < 8; j++)
	    block_ptr[i+8*j] = block2[j+8*i] << 4;
    
    IDCT();
    
    block[0] = vx0;
    block[1] = vx1;
    block[2] = vx2;
    block[3] = vx3;
    block[4] = vx4;
    block[5] = vx5;
    block[6] = vx6;
    block[7] = vx7;
}
