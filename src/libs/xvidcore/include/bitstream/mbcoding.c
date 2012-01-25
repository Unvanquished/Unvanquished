/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - MB coding -
 *
 *  Copyright (C) 2002 Michael Militzer <isibaar@xvid.org>
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
 * $Id: mbcoding.c,v 1.57 2008/11/26 01:04:34 Isibaar Exp $
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../portab.h"
#include "../global.h"
#include "bitstream.h"
#include "zigzag.h"
#include "vlc_codes.h"
#include "mbcoding.h"

#include "../utils/mbfunctions.h"

#ifdef _DEBUG
# include "../motion/estimation.h"
# include "../motion/motion_inlines.h"
# include <assert.h>
#endif


#define LEVELOFFSET 32

/* Initialized once during xvid_global call
 * RO access is thread safe */
static REVERSE_EVENT DCT3D[2][4096];
static VLC coeff_VLC[2][2][64][64];

/* not really MB related, but VLCs are only available here */
void bs_put_spritetrajectory(Bitstream * bs, const int val)
{
	const int code = sprite_trajectory_code[val+16384].code;
	const int len = sprite_trajectory_code[val+16384].len;
	const int code2 = sprite_trajectory_len[len].code;
	const int len2 = sprite_trajectory_len[len].len;

#if 0
	printf("GMC=%d Code/Len  = %d / %d ",val, code,len);
	printf("Code2 / Len2 = %d / %d \n",code2,len2);
#endif

	BitstreamPutBits(bs, code2, len2);
	if (len) BitstreamPutBits(bs, code, len);
}

int bs_get_spritetrajectory(Bitstream * bs)
{
	int i;
	for (i = 0; i < 12; i++)
	{
		if (BitstreamShowBits(bs, sprite_trajectory_len[i].len) == sprite_trajectory_len[i].code)
		{
			BitstreamSkip(bs, sprite_trajectory_len[i].len);
			return i;
		}
	}
	return -1;
}

void
init_vlc_tables(void)
{
	uint32_t i, j, k, intra, last, run,  run_esc, level, level_esc, escape, escape_len, offset;
	int32_t l;

	for (intra = 0; intra < 2; intra++)
		for (i = 0; i < 4096; i++)
			DCT3D[intra][i].event.level = 0;

	for (intra = 0; intra < 2; intra++) {
		for (last = 0; last < 2; last++) {
			for (run = 0; run < 63 + last; run++) {
				for (level = 0; level < (uint32_t)(32 << intra); level++) {
					offset = !intra * LEVELOFFSET;
					coeff_VLC[intra][last][level + offset][run].len = 128;
				}
			}
		}
	}

	for (intra = 0; intra < 2; intra++) {
		for (i = 0; i < 102; i++) {
			offset = !intra * LEVELOFFSET;

			for (j = 0; j < (uint32_t)(1 << (12 - coeff_tab[intra][i].vlc.len)); j++) {
				DCT3D[intra][(coeff_tab[intra][i].vlc.code << (12 - coeff_tab[intra][i].vlc.len)) | j].len	 = coeff_tab[intra][i].vlc.len;
				DCT3D[intra][(coeff_tab[intra][i].vlc.code << (12 - coeff_tab[intra][i].vlc.len)) | j].event = coeff_tab[intra][i].event;
			}

			coeff_VLC[intra][coeff_tab[intra][i].event.last][coeff_tab[intra][i].event.level + offset][coeff_tab[intra][i].event.run].code
				= coeff_tab[intra][i].vlc.code << 1;
			coeff_VLC[intra][coeff_tab[intra][i].event.last][coeff_tab[intra][i].event.level + offset][coeff_tab[intra][i].event.run].len
				= coeff_tab[intra][i].vlc.len + 1;

			if (!intra) {
				coeff_VLC[intra][coeff_tab[intra][i].event.last][offset - coeff_tab[intra][i].event.level][coeff_tab[intra][i].event.run].code
					= (coeff_tab[intra][i].vlc.code << 1) | 1;
				coeff_VLC[intra][coeff_tab[intra][i].event.last][offset - coeff_tab[intra][i].event.level][coeff_tab[intra][i].event.run].len
					= coeff_tab[intra][i].vlc.len + 1;
			}
		}
	}

	for (intra = 0; intra < 2; intra++) {
		for (last = 0; last < 2; last++) {
			for (run = 0; run < 63 + last; run++) {
				for (level = 1; level < (uint32_t)(32 << intra); level++) {

					if (level <= max_level[intra][last][run] && run <= max_run[intra][last][level])
					    continue;

					offset = !intra * LEVELOFFSET;
                    level_esc = level - max_level[intra][last][run];
					run_esc = run - 1 - max_run[intra][last][level];

					if (level_esc <= max_level[intra][last][run] && run <= max_run[intra][last][level_esc]) {
						escape     = ESCAPE1;
						escape_len = 7 + 1;
						run_esc    = run;
					} else {
						if (run_esc <= max_run[intra][last][level] && level <= max_level[intra][last][run_esc]) {
							escape     = ESCAPE2;
							escape_len = 7 + 2;
							level_esc  = level;
						} else {
							if (!intra) {
								coeff_VLC[intra][last][level + offset][run].code
									= (ESCAPE3 << 21) | (last << 20) | (run << 14) | (1 << 13) | ((level & 0xfff) << 1) | 1;
								coeff_VLC[intra][last][level + offset][run].len = 30;
									coeff_VLC[intra][last][offset - level][run].code
									= (ESCAPE3 << 21) | (last << 20) | (run << 14) | (1 << 13) | ((-(int32_t)level & 0xfff) << 1) | 1;
								coeff_VLC[intra][last][offset - level][run].len = 30;
							}
							continue;
						}
					}

					coeff_VLC[intra][last][level + offset][run].code
						= (escape << coeff_VLC[intra][last][level_esc + offset][run_esc].len)
						|  coeff_VLC[intra][last][level_esc + offset][run_esc].code;
					coeff_VLC[intra][last][level + offset][run].len
						= coeff_VLC[intra][last][level_esc + offset][run_esc].len + escape_len;

					if (!intra) {
						coeff_VLC[intra][last][offset - level][run].code
							= (escape << coeff_VLC[intra][last][level_esc + offset][run_esc].len)
							|  coeff_VLC[intra][last][level_esc + offset][run_esc].code | 1;
						coeff_VLC[intra][last][offset - level][run].len
							= coeff_VLC[intra][last][level_esc + offset][run_esc].len + escape_len;
					}
				}

				if (!intra) {
					coeff_VLC[intra][last][0][run].code
						= (ESCAPE3 << 21) | (last << 20) | (run << 14) | (1 << 13) | ((-32 & 0xfff) << 1) | 1;
					coeff_VLC[intra][last][0][run].len = 30;
				}
			}
		}
	}

	/* init sprite_trajectory tables
	 * even if GMC is not specified (it might be used later...) */

	sprite_trajectory_code[0+16384].code = 0;
	sprite_trajectory_code[0+16384].len = 0;
	for (k=0;k<14;k++) {
		int limit = (1<<k);

		for (l=-(2*limit-1); l <= -limit; l++) {
			sprite_trajectory_code[l+16384].code = (2*limit-1)+l;
			sprite_trajectory_code[l+16384].len = k+1;
		}

		for (l=limit; l<= 2*limit-1; l++) {
			sprite_trajectory_code[l+16384].code = l;
			sprite_trajectory_code[l+16384].len = k+1;
		}
	}
}

static __inline void
CodeVector(Bitstream * bs,
		   int32_t value,
		   int32_t f_code)
{

	const int scale_factor = 1 << (f_code - 1);
	const int cmp = scale_factor << 5;

	if (value < (-1 * cmp))
		value += 64 * scale_factor;

	if (value > (cmp - 1))
		value -= 64 * scale_factor;

	if (value == 0) {
		BitstreamPutBits(bs, mb_motion_table[32].code,
						 mb_motion_table[32].len);
	} else {
		uint16_t length, code, mv_res, sign;

		length = 16 << f_code;
		f_code--;

		sign = (value < 0);

		if (value >= length)
			value -= 2 * length;
		else if (value < -length)
			value += 2 * length;

		if (sign)
			value = -value;

		value--;
		mv_res = value & ((1 << f_code) - 1);
		code = ((value - mv_res) >> f_code) + 1;

		if (sign)
			code = -code;

		code += 32;
		BitstreamPutBits(bs, mb_motion_table[code].code,
						 mb_motion_table[code].len);

		if (f_code)
			BitstreamPutBits(bs, mv_res, f_code);
	}

}

static __inline void
CodeCoeffInter(Bitstream * bs,
		  const int16_t qcoeff[64],
		  const uint16_t * zigzag)
{
	uint32_t i, run, prev_run, code, len;
	int32_t level, prev_level, level_shifted;

	i	= 0;
	run = 0;

	while (!(level = qcoeff[zigzag[i++]]))
		run++;

	prev_level = level;
	prev_run   = run;
	run = 0;

	while (i < 64)
	{
		if ((level = qcoeff[zigzag[i++]]) != 0)
		{
			level_shifted = prev_level + 32;
			if (!(level_shifted & -64))
			{
				code = coeff_VLC[0][0][level_shifted][prev_run].code;
				len	 = coeff_VLC[0][0][level_shifted][prev_run].len;
			}
			else
			{
				code = (ESCAPE3 << 21) | (prev_run << 14) | (1 << 13) | ((prev_level & 0xfff) << 1) | 1;
				len  = 30;
			}
			BitstreamPutBits(bs, code, len);
			prev_level = level;
			prev_run   = run;
			run = 0;
		}
		else
			run++;
	}

	level_shifted = prev_level + 32;
	if (!(level_shifted & -64))
	{
		code = coeff_VLC[0][1][level_shifted][prev_run].code;
		len	 = coeff_VLC[0][1][level_shifted][prev_run].len;
	}
	else
	{
		code = (ESCAPE3 << 21) | (1 << 20) | (prev_run << 14) | (1 << 13) | ((prev_level & 0xfff) << 1) | 1;
		len  = 30;
	}
	BitstreamPutBits(bs, code, len);
}

static __inline void
CodeCoeffIntra(Bitstream * bs,
		  const int16_t qcoeff[64],
		  const uint16_t * zigzag)
{
	uint32_t i, abs_level, run, prev_run, code, len;
	int32_t level, prev_level;

	i	= 1;
	run = 0;

	while (i<64 && !(level = qcoeff[zigzag[i++]]))
		run++;

	prev_level = level;
	prev_run   = run;
	run = 0;

	while (i < 64)
	{
		if ((level = qcoeff[zigzag[i++]]) != 0)
		{
			abs_level = abs(prev_level);
			abs_level = abs_level < 64 ? abs_level : 0;
			code	  = coeff_VLC[1][0][abs_level][prev_run].code;
			len		  = coeff_VLC[1][0][abs_level][prev_run].len;
			if (len != 128)
				code |= (prev_level < 0);
			else
			{
		        code = (ESCAPE3 << 21) | (prev_run << 14) | (1 << 13) | ((prev_level & 0xfff) << 1) | 1;
				len  = 30;
			}
			BitstreamPutBits(bs, code, len);
			prev_level = level;
			prev_run   = run;
			run = 0;
		}
		else
			run++;
	}

	abs_level = abs(prev_level);
	abs_level = abs_level < 64 ? abs_level : 0;
	code	  = coeff_VLC[1][1][abs_level][prev_run].code;
	len		  = coeff_VLC[1][1][abs_level][prev_run].len;
	if (len != 128)
		code |= (prev_level < 0);
	else
	{
		code = (ESCAPE3 << 21) | (1 << 20) | (prev_run << 14) | (1 << 13) | ((prev_level & 0xfff) << 1) | 1;
		len  = 30;
	}
	BitstreamPutBits(bs, code, len);
}



/* returns the number of bits required to encode qcoeff */

int
CodeCoeffIntra_CalcBits(const int16_t qcoeff[64], const uint16_t * zigzag)
{
	int bits = 0;
	uint32_t i, abs_level, run, prev_run, len;
	int32_t level, prev_level;

	i	= 1;
	run = 0;

	while (i<64 && !(level = qcoeff[zigzag[i++]]))
		run++;

	if (i >= 64) return 0;	/* empty block */

	prev_level = level;
	prev_run   = run;
	run = 0;

	while (i < 64)
	{
		if ((level = qcoeff[zigzag[i++]]) != 0)
		{
			abs_level = abs(prev_level);
			abs_level = abs_level < 64 ? abs_level : 0;
			len		  = coeff_VLC[1][0][abs_level][prev_run].len;
			bits      += len!=128 ? len : 30;

			prev_level = level;
			prev_run   = run;
			run = 0;
		}
		else
			run++;
	}

	abs_level = abs(prev_level);
	abs_level = abs_level < 64 ? abs_level : 0;
	len		  = coeff_VLC[1][1][abs_level][prev_run].len;
	bits      += len!=128 ? len : 30;

	return bits;
}

int
CodeCoeffInter_CalcBits(const int16_t qcoeff[64], const uint16_t * zigzag)
{
	uint32_t i, run, prev_run, len;
	int32_t level, prev_level, level_shifted;
	int bits = 0;

	i	= 0;
	run = 0;

	while (!(level = qcoeff[zigzag[i++]]))
		run++;

	prev_level = level;
	prev_run   = run;
	run = 0;

	while (i < 64) {
		if ((level = qcoeff[zigzag[i++]]) != 0) {
			level_shifted = prev_level + 32;
			if (!(level_shifted & -64))
				len	 = coeff_VLC[0][0][level_shifted][prev_run].len;
			else
				len  = 30;

			bits += len;
			prev_level = level;
			prev_run   = run;
			run = 0;
		}
		else
			run++;
	}

	level_shifted = prev_level + 32;
	if (!(level_shifted & -64))
		len	 = coeff_VLC[0][1][level_shifted][prev_run].len;
	else
		len  = 30;
	bits += len;

	return bits;
}

static const int iDQtab[5] = {
	1, 0, -1 /* no change */, 2, 3
};
#define DQ_VALUE2INDEX(value)  iDQtab[(value)+2]


static __inline void
CodeBlockIntra(const FRAMEINFO * const frame,
			   const MACROBLOCK * pMB,
			   int16_t qcoeff[6 * 64],
			   Bitstream * bs,
			   Statistics * pStat)
{

	uint32_t i, mcbpc, cbpy, bits;

	cbpy = pMB->cbp >> 2;

	/* write mcbpc */
	if (frame->coding_type == I_VOP) {
		mcbpc = ((pMB->mode >> 1) & 3) | ((pMB->cbp & 3) << 2);
		BitstreamPutBits(bs, mcbpc_intra_tab[mcbpc].code,
						 mcbpc_intra_tab[mcbpc].len);
	} else {
		mcbpc = (pMB->mode & 7) | ((pMB->cbp & 3) << 3);
		BitstreamPutBits(bs, mcbpc_inter_tab[mcbpc].code,
						 mcbpc_inter_tab[mcbpc].len);
	}

	/* ac prediction flag */
	if (pMB->acpred_directions[0])
		BitstreamPutBits(bs, 1, 1);
	else
		BitstreamPutBits(bs, 0, 1);

	/* write cbpy */
	BitstreamPutBits(bs, xvid_cbpy_tab[cbpy].code, xvid_cbpy_tab[cbpy].len);

	/* write dquant */
	if (pMB->mode == MODE_INTRA_Q)
		BitstreamPutBits(bs, DQ_VALUE2INDEX(pMB->dquant), 2);

	/* write interlacing */
	if (frame->vol_flags & XVID_VOL_INTERLACING) {
		BitstreamPutBit(bs, pMB->field_dct);
	}
	/* code block coeffs */
	for (i = 0; i < 6; i++) {
		if (i < 4)
			BitstreamPutBits(bs, dcy_tab[qcoeff[i * 64 + 0] + 255].code,
							 dcy_tab[qcoeff[i * 64 + 0] + 255].len);
		else
			BitstreamPutBits(bs, dcc_tab[qcoeff[i * 64 + 0] + 255].code,
							 dcc_tab[qcoeff[i * 64 + 0] + 255].len);

		if (pMB->cbp & (1 << (5 - i))) {
			const uint16_t *scan_table =
				frame->vop_flags & XVID_VOP_ALTERNATESCAN ?
				scan_tables[2] : scan_tables[pMB->acpred_directions[i]];

			bits = BitstreamPos(bs);

			CodeCoeffIntra(bs, &qcoeff[i * 64], scan_table);

			bits = BitstreamPos(bs) - bits;
			pStat->iTextBits += bits;
		}
	}

}


static void
CodeBlockInter(const FRAMEINFO * const frame,
			   const MACROBLOCK * pMB,
			   int16_t qcoeff[6 * 64],
			   Bitstream * bs,
			   Statistics * pStat)
{

	int32_t i;
	uint32_t bits, mcbpc, cbpy;

	mcbpc = (pMB->mode & 7) | ((pMB->cbp & 3) << 3);
	cbpy = 15 - (pMB->cbp >> 2);

	/* write mcbpc */
	BitstreamPutBits(bs, mcbpc_inter_tab[mcbpc].code,
					 mcbpc_inter_tab[mcbpc].len);

	if ( (frame->coding_type == S_VOP) && (pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q) )
		BitstreamPutBit(bs, pMB->mcsel);		/* mcsel: '0'=local motion, '1'=GMC */

	/* write cbpy */
	BitstreamPutBits(bs, xvid_cbpy_tab[cbpy].code, xvid_cbpy_tab[cbpy].len);

	/* write dquant */
	if (pMB->mode == MODE_INTER_Q)
		BitstreamPutBits(bs, DQ_VALUE2INDEX(pMB->dquant), 2);

	/* interlacing */
	if (frame->vol_flags & XVID_VOL_INTERLACING) {
		if (pMB->cbp) {
			BitstreamPutBit(bs, pMB->field_dct);
			DPRINTF(XVID_DEBUG_MB,"codep: field_dct: %i\n", pMB->field_dct);
		}

		/* if inter block, write field ME flag */
		if ((pMB->mode == MODE_INTER || pMB->mode == MODE_INTER_Q) && (pMB->mcsel == 0)) {
			BitstreamPutBit(bs, 0 /*pMB->field_pred*/); /* not implemented yet */
			DPRINTF(XVID_DEBUG_MB,"codep: field_pred: %i\n", pMB->field_pred);

			/* write field prediction references */
#if 0 /* Remove the #if once field_pred is supported */
			if (pMB->field_pred) {
				BitstreamPutBit(bs, pMB->field_for_top);
				BitstreamPutBit(bs, pMB->field_for_bot);
			}
#endif
		}
	}

	bits = BitstreamPos(bs);

	/* code motion vector(s) if motion is local  */
	if (!pMB->mcsel)
		for (i = 0; i < (pMB->mode == MODE_INTER4V ? 4 : 1); i++) {
			CodeVector(bs, pMB->pmvs[i].x, frame->fcode);
			CodeVector(bs, pMB->pmvs[i].y, frame->fcode);

#ifdef _DEBUG
			if (i == 0) /* for simplicity */ {
				int coded_length = BitstreamPos(bs) - bits;
				int estimated_length = d_mv_bits(pMB->pmvs[i].x, pMB->pmvs[i].y, zeroMV, frame->fcode, 0);
				assert(estimated_length == coded_length);
				d_mv_bits(pMB->pmvs[i].x, pMB->pmvs[i].y, zeroMV, frame->fcode, 0);
			}
#endif
		}

	bits = BitstreamPos(bs) - bits;
	pStat->iMVBits += bits;

	bits = BitstreamPos(bs);

	/* code block coeffs */
	for (i = 0; i < 6; i++)
		if (pMB->cbp & (1 << (5 - i))) {
			const uint16_t *scan_table =
				frame->vop_flags & XVID_VOP_ALTERNATESCAN ?
				scan_tables[2] : scan_tables[0];

			CodeCoeffInter(bs, &qcoeff[i * 64], scan_table);
		}

	bits = BitstreamPos(bs) - bits;
	pStat->iTextBits += bits;
}


void
MBCoding(const FRAMEINFO * const frame,
		 MACROBLOCK * pMB,
		 int16_t qcoeff[6 * 64],
		 Bitstream * bs,
		 Statistics * pStat)
{
	if (frame->coding_type != I_VOP)
			BitstreamPutBit(bs, 0);	/* not_coded */

	if (frame->vop_flags & XVID_VOP_GREYSCALE) {
		pMB->cbp &= 0x3C;		/* keep only bits 5-2 */
		qcoeff[4*64+0]=0;		/* for INTRA DC value is saved */
		qcoeff[5*64+0]=0;
	}

	if (pMB->mode == MODE_INTRA || pMB->mode == MODE_INTRA_Q)
		CodeBlockIntra(frame, pMB, qcoeff, bs, pStat);
	else
		CodeBlockInter(frame, pMB, qcoeff, bs, pStat);

}

/***************************************************************
 * bframe encoding start
 ***************************************************************/

/*
	mbtype
	0	1b		direct(h263)		mvdb
	1	01b		interpolate mc+q	dbquant, mvdf, mvdb
	2	001b	backward mc+q		dbquant, mvdb
	3	0001b	forward mc+q		dbquant, mvdf
*/

static __inline void
put_bvop_mbtype(Bitstream * bs,
				int value)
{
	switch (value) {
		case MODE_FORWARD:
			BitstreamPutBit(bs, 0);
		case MODE_BACKWARD:
			BitstreamPutBit(bs, 0);
		case MODE_INTERPOLATE:
			BitstreamPutBit(bs, 0);
		case MODE_DIRECT:
			BitstreamPutBit(bs, 1);
		default:
			break;
	}
}

/*
	dbquant
	-2	10b
	0	0b
	+2	11b
*/

static __inline void
put_bvop_dbquant(Bitstream * bs,
				 int value)
{
	switch (value) {
	case 0:
		BitstreamPutBit(bs, 0);
		return;

	case -2:
		BitstreamPutBit(bs, 1);
		BitstreamPutBit(bs, 0);
		return;

	case 2:
		BitstreamPutBit(bs, 1);
		BitstreamPutBit(bs, 1);
		return;

	default:;					/* invalid */
	}
}



void
MBCodingBVOP(const FRAMEINFO * const frame,
			 const MACROBLOCK * mb,
			 const int16_t qcoeff[6 * 64],
			 const int32_t fcode,
			 const int32_t bcode,
			 Bitstream * bs,
			 Statistics * pStat)
{
	int vcode = fcode;
	unsigned int i;

	const uint16_t *scan_table =
		frame->vop_flags & XVID_VOP_ALTERNATESCAN ?
		scan_tables[2] : scan_tables[0];
	int bits;

/*	------------------------------------------------------------------
		when a block is skipped it is decoded DIRECT(0,0)
		hence is interpolated from forward & backward frames
	------------------------------------------------------------------ */

	if (mb->mode == MODE_DIRECT_NONE_MV) {
		BitstreamPutBit(bs, 1);	/* skipped */
		return;
	}

	BitstreamPutBit(bs, 0);		/* not skipped */

	if (mb->cbp == 0) {
		BitstreamPutBit(bs, 1);	/* cbp == 0 */
	} else {
		BitstreamPutBit(bs, 0);	/* cbp == xxx */
	}

	put_bvop_mbtype(bs, mb->mode);

	if (mb->cbp) {
		BitstreamPutBits(bs, mb->cbp, 6);
	}

	if (mb->mode != MODE_DIRECT && mb->cbp != 0) {
		put_bvop_dbquant(bs, 0);	/* todo: mb->dquant = 0 */
	}

	if (frame->vol_flags & XVID_VOL_INTERLACING) {
		if (mb->cbp) {
			BitstreamPutBit(bs, mb->field_dct);
			DPRINTF(XVID_DEBUG_MB,"codep: field_dct: %i\n", mb->field_dct);
		}

		/* if not direct block, write field ME flag */
		if (mb->mode != MODE_DIRECT) {
			BitstreamPutBit(bs, 0 /*mb->field_pred*/); /* field ME not implemented */
			
			/* write field prediction references */
#if 0 /* Remove the #if once field_pred is supported */
			if (mb->field_pred) {
				BitstreamPutBit(bs, mb->field_for_top);
				BitstreamPutBit(bs, mb->field_for_bot);
			}
#endif
		}
	}

	bits = BitstreamPos(bs);

	switch (mb->mode) {
		case MODE_INTERPOLATE:
			CodeVector(bs, mb->pmvs[1].x, vcode); /* forward vector of interpolate mode */
			CodeVector(bs, mb->pmvs[1].y, vcode);
		case MODE_BACKWARD:
			vcode = bcode;
		case MODE_FORWARD:
			CodeVector(bs, mb->pmvs[0].x, vcode);
			CodeVector(bs, mb->pmvs[0].y, vcode);
			break;
		case MODE_DIRECT:
			CodeVector(bs, mb->pmvs[3].x, 1);	/* fcode is always 1 for delta vector */
			CodeVector(bs, mb->pmvs[3].y, 1);	/* prediction is always (0,0) */
		default: break;
	}
	pStat->iMVBits += BitstreamPos(bs) - bits;

	bits = BitstreamPos(bs);
	for (i = 0; i < 6; i++) {
		if (mb->cbp & (1 << (5 - i))) {
			CodeCoeffInter(bs, &qcoeff[i * 64], scan_table);
		}
	}
	pStat->iTextBits += BitstreamPos(bs) - bits;
}



/***************************************************************
 * decoding stuff starts here                                  *
 ***************************************************************/


/*
 * for IVOP addbits == 0
 * for PVOP addbits == fcode - 1
 * for BVOP addbits == max(fcode,bcode) - 1
 * returns true or false
 */
int
check_resync_marker(Bitstream * bs, int addbits)
{
	uint32_t nbits;
	uint32_t code;
	uint32_t nbitsresyncmarker = NUMBITS_VP_RESYNC_MARKER + addbits;

	nbits = BitstreamNumBitsToByteAlign(bs);
	code = BitstreamShowBits(bs, nbits);

	if (code == (((uint32_t)1 << (nbits - 1)) - 1))
	{
		return BitstreamShowBitsFromByteAlign(bs, nbitsresyncmarker) == RESYNC_MARKER;
	}

	return 0;
}



int
get_mcbpc_intra(Bitstream * bs)
{

	uint32_t index;

	index = BitstreamShowBits(bs, 9);
	index >>= 3;

	BitstreamSkip(bs, mcbpc_intra_table[index].len);

	return mcbpc_intra_table[index].code;

}

int
get_mcbpc_inter(Bitstream * bs)
{

	uint32_t index;

	index = MIN(BitstreamShowBits(bs, 9), 256);

	BitstreamSkip(bs, mcbpc_inter_table[index].len);

	return mcbpc_inter_table[index].code;

}

int
get_cbpy(Bitstream * bs,
		 int intra)
{

	int cbpy;
	uint32_t index = BitstreamShowBits(bs, 6);

	BitstreamSkip(bs, cbpy_table[index].len);
	cbpy = cbpy_table[index].code;

	if (!intra)
		cbpy = 15 - cbpy;

	return cbpy;

}

static __inline int
get_mv_data(Bitstream * bs)
{

	uint32_t index;

	if (BitstreamGetBit(bs))
		return 0;

	index = BitstreamShowBits(bs, 12);

	if (index >= 512) {
		index = (index >> 8) - 2;
		BitstreamSkip(bs, TMNMVtab0[index].len);
		return TMNMVtab0[index].code;
	}

	if (index >= 128) {
		index = (index >> 2) - 32;
		BitstreamSkip(bs, TMNMVtab1[index].len);
		return TMNMVtab1[index].code;
	}

	index -= 4;

	BitstreamSkip(bs, TMNMVtab2[index].len);
	return TMNMVtab2[index].code;

}

int
get_mv(Bitstream * bs,
	   int fcode)
{

	int data;
	int res;
	int mv;
	int scale_fac = 1 << (fcode - 1);

	data = get_mv_data(bs);

	if (scale_fac == 1 || data == 0)
		return data;

	res = BitstreamGetBits(bs, fcode - 1);
	mv = ((abs(data) - 1) * scale_fac) + res + 1;

	return data < 0 ? -mv : mv;

}

int
get_dc_dif(Bitstream * bs,
		   uint32_t dc_size)
{

	int code = BitstreamGetBits(bs, dc_size);
	int msb = code >> (dc_size - 1);

	if (msb == 0)
		return (-1 * (code ^ ((1 << dc_size) - 1)));

	return code;

}

int
get_dc_size_lum(Bitstream * bs)
{

	int code, i;

	code = BitstreamShowBits(bs, 11);

	for (i = 11; i > 3; i--) {
		if (code == 1) {
			BitstreamSkip(bs, i);
			return i + 1;
		}
		code >>= 1;
	}

	BitstreamSkip(bs, dc_lum_tab[code].len);
	return dc_lum_tab[code].code;

}


int
get_dc_size_chrom(Bitstream * bs)
{

	uint32_t code, i;

	code = BitstreamShowBits(bs, 12);

	for (i = 12; i > 2; i--) {
		if (code == 1) {
			BitstreamSkip(bs, i);
			return i;
		}
		code >>= 1;
	}

	return 3 - BitstreamGetBits(bs, 2);

}

#define GET_BITS(cache, n) ((cache)>>(32-(n)))

static __inline int
get_coeff(Bitstream * bs,
		  int *run,
		  int *last,
		  int intra,
		  int short_video_header)
{

	uint32_t mode;
	int32_t level;
	REVERSE_EVENT *reverse_event;

	uint32_t cache = BitstreamShowBits(bs, 32);
	
	if (short_video_header)		/* inter-VLCs will be used for both intra and inter blocks */
		intra = 0;

	if (GET_BITS(cache, 7) != ESCAPE) {
		reverse_event = &DCT3D[intra][GET_BITS(cache, 12)];

		if ((level = reverse_event->event.level) == 0)
			goto error;

		*last = reverse_event->event.last;
		*run  = reverse_event->event.run;

		/* Don't forget to update the bitstream position */
		BitstreamSkip(bs, reverse_event->len+1);

		return (GET_BITS(cache, reverse_event->len+1)&0x01) ? -level : level;
	}

	/* flush 7bits of cache */
	cache <<= 7;

	if (short_video_header) {
		/* escape mode 4 - H.263 type, only used if short_video_header = 1  */
		*last =  GET_BITS(cache, 1);
		*run  = (GET_BITS(cache, 7) &0x3f);
		level = (GET_BITS(cache, 15)&0xff);

		if (level == 0 || level == 128)
			DPRINTF(XVID_DEBUG_ERROR, "Illegal LEVEL for ESCAPE mode 4: %d\n", level);

		/* We've "eaten" 22 bits */
		BitstreamSkip(bs, 22);

		return (level << 24) >> 24;
	}

	if ((mode = GET_BITS(cache, 2)) < 3) {
		const int skip[3] = {1, 1, 2};
		cache <<= skip[mode];

		reverse_event = &DCT3D[intra][GET_BITS(cache, 12)];

		if ((level = reverse_event->event.level) == 0)
			goto error;

		*last = reverse_event->event.last;
		*run  = reverse_event->event.run;

		if (mode < 2) {
			/* first escape mode, level is offset */
			level += max_level[intra][*last][*run];
		} else {
			/* second escape mode, run is offset */
			*run += max_run[intra][*last][level] + 1;
		}
		
		/* Update bitstream position */
		BitstreamSkip(bs, 7 + skip[mode] + reverse_event->len + 1);

		return (GET_BITS(cache, reverse_event->len+1)&0x01) ? -level : level;
	}

	/* third escape mode - fixed length codes */
	cache <<= 2;
	*last =  GET_BITS(cache, 1);
	*run  = (GET_BITS(cache, 7)&0x3f);
	level = (GET_BITS(cache, 20)&0xfff);
	
	/* Update bitstream position */
	BitstreamSkip(bs, 30);

	return (level << 20) >> 20;

  error:
	*run = 64;
	return 0;
}

void
get_intra_block(Bitstream * bs,
				int16_t * block,
				int direction,
				int coeff)
{

	const uint16_t *scan = scan_tables[direction];
	int level, run, last = 0;

	do {
		level = get_coeff(bs, &run, &last, 1, 0);
		coeff += run;
		if (coeff & ~63) {
			DPRINTF(XVID_DEBUG_ERROR,"fatal: invalid run or index");
			break;
		}

		block[scan[coeff]] = level;

		DPRINTF(XVID_DEBUG_COEFF,"block[%i] %i\n", scan[coeff], level);
#if 0
		DPRINTF(XVID_DEBUG_COEFF,"block[%i] %i %08x\n", scan[coeff], level, BitstreamShowBits(bs, 32));
#endif

		if (level < -2047 || level > 2047) {
			DPRINTF(XVID_DEBUG_ERROR,"warning: intra_overflow %i\n", level);
		}
		coeff++;
	} while (!last);

}

void
get_inter_block_h263(
		Bitstream * bs,
		int16_t * block,
		int direction,
		const int quant,
		const uint16_t *matrix)
{

	const uint16_t *scan = scan_tables[direction];
	const uint16_t quant_m_2 = quant << 1;
	const uint16_t quant_add = (quant & 1 ? quant : quant - 1);
	int p;
	int level;
	int run;
	int last = 0;

	p = 0;
	do {
		level = get_coeff(bs, &run, &last, 0, 0);
		p += run;
		if (p & ~63) {
			DPRINTF(XVID_DEBUG_ERROR,"fatal: invalid run or index");
			break;
		}

		if (level < 0) {
			level = level*quant_m_2 - quant_add;
			block[scan[p]] = (level >= -2048 ? level : -2048);
		} else {
			level = level * quant_m_2 + quant_add;
			block[scan[p]] = (level <= 2047 ? level : 2047);
		}		
		p++;
	} while (!last);
}

void
get_inter_block_mpeg(
		Bitstream * bs,
		int16_t * block,
		int direction,
		const int quant,
		const uint16_t *matrix)
{
	const uint16_t *scan = scan_tables[direction];
	uint32_t sum = 0;
	int p;
	int level;
	int run;
	int last = 0;

	p = 0;
	do {
		level = get_coeff(bs, &run, &last, 0, 0);
		p += run;
		if (p & ~63) {
			DPRINTF(XVID_DEBUG_ERROR,"fatal: invalid run or index");
			break;
		}

		if (level < 0) {
			level = ((2 * -level + 1) * matrix[scan[p]] * quant) >> 4;
			block[scan[p]] = (level <= 2048 ? -level : -2048);
		} else {
			level = ((2 *  level + 1) * matrix[scan[p]] * quant) >> 4;
			block[scan[p]] = (level <= 2047 ? level : 2047);
		}

		sum ^= block[scan[p]];
		
		p++;
	} while (!last);

	/*	mismatch control */
	if ((sum & 1) == 0) {
		block[63] ^= 1;
	}
}


/*****************************************************************************
 * VLC tables and other constant arrays
 ****************************************************************************/

VLC_TABLE const coeff_tab[2][102] =
{
	/* intra = 0 */
	{
		{{ 2,  2}, {0, 0, 1}},
		{{15,  4}, {0, 0, 2}},
		{{21,  6}, {0, 0, 3}},
		{{23,  7}, {0, 0, 4}},
		{{31,  8}, {0, 0, 5}},
		{{37,  9}, {0, 0, 6}},
		{{36,  9}, {0, 0, 7}},
		{{33, 10}, {0, 0, 8}},
		{{32, 10}, {0, 0, 9}},
		{{ 7, 11}, {0, 0, 10}},
		{{ 6, 11}, {0, 0, 11}},
		{{32, 11}, {0, 0, 12}},
		{{ 6,  3}, {0, 1, 1}},
		{{20,  6}, {0, 1, 2}},
		{{30,  8}, {0, 1, 3}},
		{{15, 10}, {0, 1, 4}},
		{{33, 11}, {0, 1, 5}},
		{{80, 12}, {0, 1, 6}},
		{{14,  4}, {0, 2, 1}},
		{{29,  8}, {0, 2, 2}},
		{{14, 10}, {0, 2, 3}},
		{{81, 12}, {0, 2, 4}},
		{{13,  5}, {0, 3, 1}},
		{{35,  9}, {0, 3, 2}},
		{{13, 10}, {0, 3, 3}},
		{{12,  5}, {0, 4, 1}},
		{{34,  9}, {0, 4, 2}},
		{{82, 12}, {0, 4, 3}},
		{{11,  5}, {0, 5, 1}},
		{{12, 10}, {0, 5, 2}},
		{{83, 12}, {0, 5, 3}},
		{{19,  6}, {0, 6, 1}},
		{{11, 10}, {0, 6, 2}},
		{{84, 12}, {0, 6, 3}},
		{{18,  6}, {0, 7, 1}},
		{{10, 10}, {0, 7, 2}},
		{{17,  6}, {0, 8, 1}},
		{{ 9, 10}, {0, 8, 2}},
		{{16,  6}, {0, 9, 1}},
		{{ 8, 10}, {0, 9, 2}},
		{{22,  7}, {0, 10, 1}},
		{{85, 12}, {0, 10, 2}},
		{{21,  7}, {0, 11, 1}},
		{{20,  7}, {0, 12, 1}},
		{{28,  8}, {0, 13, 1}},
		{{27,  8}, {0, 14, 1}},
		{{33,  9}, {0, 15, 1}},
		{{32,  9}, {0, 16, 1}},
		{{31,  9}, {0, 17, 1}},
		{{30,  9}, {0, 18, 1}},
		{{29,  9}, {0, 19, 1}},
		{{28,  9}, {0, 20, 1}},
		{{27,  9}, {0, 21, 1}},
		{{26,  9}, {0, 22, 1}},
		{{34, 11}, {0, 23, 1}},
		{{35, 11}, {0, 24, 1}},
		{{86, 12}, {0, 25, 1}},
		{{87, 12}, {0, 26, 1}},
		{{ 7,  4}, {1, 0, 1}},
		{{25,  9}, {1, 0, 2}},
		{{ 5, 11}, {1, 0, 3}},
		{{15,  6}, {1, 1, 1}},
		{{ 4, 11}, {1, 1, 2}},
		{{14,  6}, {1, 2, 1}},
		{{13,  6}, {1, 3, 1}},
		{{12,  6}, {1, 4, 1}},
		{{19,  7}, {1, 5, 1}},
		{{18,  7}, {1, 6, 1}},
		{{17,  7}, {1, 7, 1}},
		{{16,  7}, {1, 8, 1}},
		{{26,  8}, {1, 9, 1}},
		{{25,  8}, {1, 10, 1}},
		{{24,  8}, {1, 11, 1}},
		{{23,  8}, {1, 12, 1}},
		{{22,  8}, {1, 13, 1}},
		{{21,  8}, {1, 14, 1}},
		{{20,  8}, {1, 15, 1}},
		{{19,  8}, {1, 16, 1}},
		{{24,  9}, {1, 17, 1}},
		{{23,  9}, {1, 18, 1}},
		{{22,  9}, {1, 19, 1}},
		{{21,  9}, {1, 20, 1}},
		{{20,  9}, {1, 21, 1}},
		{{19,  9}, {1, 22, 1}},
		{{18,  9}, {1, 23, 1}},
		{{17,  9}, {1, 24, 1}},
		{{ 7, 10}, {1, 25, 1}},
		{{ 6, 10}, {1, 26, 1}},
		{{ 5, 10}, {1, 27, 1}},
		{{ 4, 10}, {1, 28, 1}},
		{{36, 11}, {1, 29, 1}},
		{{37, 11}, {1, 30, 1}},
		{{38, 11}, {1, 31, 1}},
		{{39, 11}, {1, 32, 1}},
		{{88, 12}, {1, 33, 1}},
		{{89, 12}, {1, 34, 1}},
		{{90, 12}, {1, 35, 1}},
		{{91, 12}, {1, 36, 1}},
		{{92, 12}, {1, 37, 1}},
		{{93, 12}, {1, 38, 1}},
		{{94, 12}, {1, 39, 1}},
		{{95, 12}, {1, 40, 1}}
	},
	/* intra = 1 */
	{
		{{ 2,  2}, {0, 0, 1}},
		{{15,  4}, {0, 0, 3}},
		{{21,  6}, {0, 0, 6}},
		{{23,  7}, {0, 0, 9}},
		{{31,  8}, {0, 0, 10}},
		{{37,  9}, {0, 0, 13}},
		{{36,  9}, {0, 0, 14}},
		{{33, 10}, {0, 0, 17}},
		{{32, 10}, {0, 0, 18}},
		{{ 7, 11}, {0, 0, 21}},
		{{ 6, 11}, {0, 0, 22}},
		{{32, 11}, {0, 0, 23}},
		{{ 6,  3}, {0, 0, 2}},
		{{20,  6}, {0, 1, 2}},
		{{30,  8}, {0, 0, 11}},
		{{15, 10}, {0, 0, 19}},
		{{33, 11}, {0, 0, 24}},
		{{80, 12}, {0, 0, 25}},
		{{14,  4}, {0, 1, 1}},
		{{29,  8}, {0, 0, 12}},
		{{14, 10}, {0, 0, 20}},
		{{81, 12}, {0, 0, 26}},
		{{13,  5}, {0, 0, 4}},
		{{35,  9}, {0, 0, 15}},
		{{13, 10}, {0, 1, 7}},
		{{12,  5}, {0, 0, 5}},
		{{34,  9}, {0, 4, 2}},
		{{82, 12}, {0, 0, 27}},
		{{11,  5}, {0, 2, 1}},
		{{12, 10}, {0, 2, 4}},
		{{83, 12}, {0, 1, 9}},
		{{19,  6}, {0, 0, 7}},
		{{11, 10}, {0, 3, 4}},
		{{84, 12}, {0, 6, 3}},
		{{18,  6}, {0, 0, 8}},
		{{10, 10}, {0, 4, 3}},
		{{17,  6}, {0, 3, 1}},
		{{ 9, 10}, {0, 8, 2}},
		{{16,  6}, {0, 4, 1}},
		{{ 8, 10}, {0, 5, 3}},
		{{22,  7}, {0, 1, 3}},
		{{85, 12}, {0, 1, 10}},
		{{21,  7}, {0, 2, 2}},
		{{20,  7}, {0, 7, 1}},
		{{28,  8}, {0, 1, 4}},
		{{27,  8}, {0, 3, 2}},
		{{33,  9}, {0, 0, 16}},
		{{32,  9}, {0, 1, 5}},
		{{31,  9}, {0, 1, 6}},
		{{30,  9}, {0, 2, 3}},
		{{29,  9}, {0, 3, 3}},
		{{28,  9}, {0, 5, 2}},
		{{27,  9}, {0, 6, 2}},
		{{26,  9}, {0, 7, 2}},
		{{34, 11}, {0, 1, 8}},
		{{35, 11}, {0, 9, 2}},
		{{86, 12}, {0, 2, 5}},
		{{87, 12}, {0, 7, 3}},
		{{ 7,  4}, {1, 0, 1}},
		{{25,  9}, {0, 11, 1}},
		{{ 5, 11}, {1, 0, 6}},
		{{15,  6}, {1, 1, 1}},
		{{ 4, 11}, {1, 0, 7}},
		{{14,  6}, {1, 2, 1}},
		{{13,  6}, {0, 5, 1}},
		{{12,  6}, {1, 0, 2}},
		{{19,  7}, {1, 5, 1}},
		{{18,  7}, {0, 6, 1}},
		{{17,  7}, {1, 3, 1}},
		{{16,  7}, {1, 4, 1}},
		{{26,  8}, {1, 9, 1}},
		{{25,  8}, {0, 8, 1}},
		{{24,  8}, {0, 9, 1}},
		{{23,  8}, {0, 10, 1}},
		{{22,  8}, {1, 0, 3}},
		{{21,  8}, {1, 6, 1}},
		{{20,  8}, {1, 7, 1}},
		{{19,  8}, {1, 8, 1}},
		{{24,  9}, {0, 12, 1}},
		{{23,  9}, {1, 0, 4}},
		{{22,  9}, {1, 1, 2}},
		{{21,  9}, {1, 10, 1}},
		{{20,  9}, {1, 11, 1}},
		{{19,  9}, {1, 12, 1}},
		{{18,  9}, {1, 13, 1}},
		{{17,  9}, {1, 14, 1}},
		{{ 7, 10}, {0, 13, 1}},
		{{ 6, 10}, {1, 0, 5}},
		{{ 5, 10}, {1, 1, 3}},
		{{ 4, 10}, {1, 2, 2}},
		{{36, 11}, {1, 3, 2}},
		{{37, 11}, {1, 4, 2}},
		{{38, 11}, {1, 15, 1}},
		{{39, 11}, {1, 16, 1}},
		{{88, 12}, {0, 14, 1}},
		{{89, 12}, {1, 0, 8}},
		{{90, 12}, {1, 5, 2}},
		{{91, 12}, {1, 6, 2}},
		{{92, 12}, {1, 17, 1}},
		{{93, 12}, {1, 18, 1}},
		{{94, 12}, {1, 19, 1}},
		{{95, 12}, {1, 20, 1}}
	}
};

/* constants taken from momusys/vm_common/inlcude/max_level.h */
uint8_t const max_level[2][2][64] = {
	{
		/* intra = 0, last = 0 */
		{
			12, 6, 4, 3, 3, 3, 3, 2,
			2, 2, 2, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		/* intra = 0, last = 1 */
		{
			3, 2, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		}
	},
	{
		/* intra = 1, last = 0 */
		{
			27, 10, 5, 4, 3, 3, 3, 3,
			2, 2, 1, 1, 1, 1, 1, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		/* intra = 1, last = 1 */
		{
			8, 3, 2, 2, 2, 2, 2, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0
		}
	}
};

uint8_t const max_run[2][2][64] = {
	{
		/* intra = 0, last = 0 */
		{
			0, 26, 10, 6, 2, 1, 1, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		/* intra = 0, last = 1 */
		{
			0, 40, 1, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		}
	},
	{
		/* intra = 1, last = 0 */
		{
			0, 14, 9, 7, 3, 2, 1, 1,
			1, 1, 1, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		},
		/* intra = 1, last = 1 */
		{
			0, 20, 6, 1, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0,
		}
	}
};

/******************************************************************
 * encoder tables                                                 *
 ******************************************************************/

VLC sprite_trajectory_code[32768];

VLC sprite_trajectory_len[15] = {
	{ 0x00 , 2},
	{ 0x02 , 3}, { 0x03, 3}, { 0x04, 3}, { 0x05, 3}, { 0x06, 3},
	{ 0x0E , 4}, { 0x1E, 5}, { 0x3E, 6}, { 0x7E, 7}, { 0xFE, 8},
	{ 0x1FE, 9}, {0x3FE,10}, {0x7FE,11}, {0xFFE,12} };


/* DCT coefficients. Four tables, two for last = 0, two for last = 1.
   the sign bit must be added afterwards. */

/* MCBPC Indexing by cbpc in first two bits, mode in last two.
 CBPC as in table 4/H.263, MB type (mode): 3 = 01, 4 = 10.
 Example: cbpc = 01 and mode = 4 gives index = 0110 = 6. */

VLC mcbpc_intra_tab[15] = {
	{0x01, 9}, {0x01, 1}, {0x01, 4}, {0x00, 0},
	{0x00, 0}, {0x01, 3}, {0x01, 6}, {0x00, 0},
	{0x00, 0}, {0x02, 3}, {0x02, 6}, {0x00, 0},
	{0x00, 0}, {0x03, 3}, {0x03, 6}
};

/* MCBPC inter.
   Addressing: 5 bit ccmmm (cc = CBPC, mmm = mode (1-4 binary)) */

VLC mcbpc_inter_tab[29] = {
	{1, 1}, {3, 3}, {2, 3}, {3, 5}, {4, 6}, {1, 9}, {0, 0}, {0, 0},
	{3, 4}, {7, 7}, {5, 7}, {4, 8}, {4, 9}, {0, 0}, {0, 0}, {0, 0},
	{2, 4}, {6, 7}, {4, 7}, {3, 8}, {3, 9}, {0, 0}, {0, 0}, {0, 0},
	{5, 6}, {5, 9}, {5, 8}, {3, 7}, {2, 9}
};

const VLC xvid_cbpy_tab[16] = {
	{3, 4}, {5, 5}, {4, 5}, {9, 4}, {3, 5}, {7, 4}, {2, 6}, {11, 4},
	{2, 5}, {3, 6}, {5, 4}, {10, 4}, {4, 4}, {8, 4}, {6, 4}, {3, 2}
};

const VLC dcy_tab[511] = {
	{0x100, 15}, {0x101, 15}, {0x102, 15}, {0x103, 15},
	{0x104, 15}, {0x105, 15}, {0x106, 15}, {0x107, 15},
	{0x108, 15}, {0x109, 15}, {0x10a, 15}, {0x10b, 15},
	{0x10c, 15}, {0x10d, 15}, {0x10e, 15}, {0x10f, 15},
	{0x110, 15}, {0x111, 15}, {0x112, 15}, {0x113, 15},
	{0x114, 15}, {0x115, 15}, {0x116, 15}, {0x117, 15},
	{0x118, 15}, {0x119, 15}, {0x11a, 15}, {0x11b, 15},
	{0x11c, 15}, {0x11d, 15}, {0x11e, 15}, {0x11f, 15},
	{0x120, 15}, {0x121, 15}, {0x122, 15}, {0x123, 15},
	{0x124, 15}, {0x125, 15}, {0x126, 15}, {0x127, 15},
	{0x128, 15}, {0x129, 15}, {0x12a, 15}, {0x12b, 15},
	{0x12c, 15}, {0x12d, 15}, {0x12e, 15}, {0x12f, 15},
	{0x130, 15}, {0x131, 15}, {0x132, 15}, {0x133, 15},
	{0x134, 15}, {0x135, 15}, {0x136, 15}, {0x137, 15},
	{0x138, 15}, {0x139, 15}, {0x13a, 15}, {0x13b, 15},
	{0x13c, 15}, {0x13d, 15}, {0x13e, 15}, {0x13f, 15},
	{0x140, 15}, {0x141, 15}, {0x142, 15}, {0x143, 15},
	{0x144, 15}, {0x145, 15}, {0x146, 15}, {0x147, 15},
	{0x148, 15}, {0x149, 15}, {0x14a, 15}, {0x14b, 15},
	{0x14c, 15}, {0x14d, 15}, {0x14e, 15}, {0x14f, 15},
	{0x150, 15}, {0x151, 15}, {0x152, 15}, {0x153, 15},
	{0x154, 15}, {0x155, 15}, {0x156, 15}, {0x157, 15},
	{0x158, 15}, {0x159, 15}, {0x15a, 15}, {0x15b, 15},
	{0x15c, 15}, {0x15d, 15}, {0x15e, 15}, {0x15f, 15},
	{0x160, 15}, {0x161, 15}, {0x162, 15}, {0x163, 15},
	{0x164, 15}, {0x165, 15}, {0x166, 15}, {0x167, 15},
	{0x168, 15}, {0x169, 15}, {0x16a, 15}, {0x16b, 15},
	{0x16c, 15}, {0x16d, 15}, {0x16e, 15}, {0x16f, 15},
	{0x170, 15}, {0x171, 15}, {0x172, 15}, {0x173, 15},
	{0x174, 15}, {0x175, 15}, {0x176, 15}, {0x177, 15},
	{0x178, 15}, {0x179, 15}, {0x17a, 15}, {0x17b, 15},
	{0x17c, 15}, {0x17d, 15}, {0x17e, 15}, {0x17f, 15},
	{0x80, 13}, {0x81, 13}, {0x82, 13}, {0x83, 13},
	{0x84, 13}, {0x85, 13}, {0x86, 13}, {0x87, 13},
	{0x88, 13}, {0x89, 13}, {0x8a, 13}, {0x8b, 13},
	{0x8c, 13}, {0x8d, 13}, {0x8e, 13}, {0x8f, 13},
	{0x90, 13}, {0x91, 13}, {0x92, 13}, {0x93, 13},
	{0x94, 13}, {0x95, 13}, {0x96, 13}, {0x97, 13},
	{0x98, 13}, {0x99, 13}, {0x9a, 13}, {0x9b, 13},
	{0x9c, 13}, {0x9d, 13}, {0x9e, 13}, {0x9f, 13},
	{0xa0, 13}, {0xa1, 13}, {0xa2, 13}, {0xa3, 13},
	{0xa4, 13}, {0xa5, 13}, {0xa6, 13}, {0xa7, 13},
	{0xa8, 13}, {0xa9, 13}, {0xaa, 13}, {0xab, 13},
	{0xac, 13}, {0xad, 13}, {0xae, 13}, {0xaf, 13},
	{0xb0, 13}, {0xb1, 13}, {0xb2, 13}, {0xb3, 13},
	{0xb4, 13}, {0xb5, 13}, {0xb6, 13}, {0xb7, 13},
	{0xb8, 13}, {0xb9, 13}, {0xba, 13}, {0xbb, 13},
	{0xbc, 13}, {0xbd, 13}, {0xbe, 13}, {0xbf, 13},
	{0x40, 11}, {0x41, 11}, {0x42, 11}, {0x43, 11},
	{0x44, 11}, {0x45, 11}, {0x46, 11}, {0x47, 11},
	{0x48, 11}, {0x49, 11}, {0x4a, 11}, {0x4b, 11},
	{0x4c, 11}, {0x4d, 11}, {0x4e, 11}, {0x4f, 11},
	{0x50, 11}, {0x51, 11}, {0x52, 11}, {0x53, 11},
	{0x54, 11}, {0x55, 11}, {0x56, 11}, {0x57, 11},
	{0x58, 11}, {0x59, 11}, {0x5a, 11}, {0x5b, 11},
	{0x5c, 11}, {0x5d, 11}, {0x5e, 11}, {0x5f, 11},
	{0x20, 9}, {0x21, 9}, {0x22, 9}, {0x23, 9},
	{0x24, 9}, {0x25, 9}, {0x26, 9}, {0x27, 9},
	{0x28, 9}, {0x29, 9}, {0x2a, 9}, {0x2b, 9},
	{0x2c, 9}, {0x2d, 9}, {0x2e, 9}, {0x2f, 9},
	{0x10, 7}, {0x11, 7}, {0x12, 7}, {0x13, 7},
	{0x14, 7}, {0x15, 7}, {0x16, 7}, {0x17, 7},
	{0x10, 6}, {0x11, 6}, {0x12, 6}, {0x13, 6},
	{0x08, 4}, {0x09, 4}, {0x06, 3}, {0x03, 3},
	{0x07, 3}, {0x0a, 4}, {0x0b, 4}, {0x14, 6},
	{0x15, 6}, {0x16, 6}, {0x17, 6}, {0x18, 7},
	{0x19, 7}, {0x1a, 7}, {0x1b, 7}, {0x1c, 7},
	{0x1d, 7}, {0x1e, 7}, {0x1f, 7}, {0x30, 9},
	{0x31, 9}, {0x32, 9}, {0x33, 9}, {0x34, 9},
	{0x35, 9}, {0x36, 9}, {0x37, 9}, {0x38, 9},
	{0x39, 9}, {0x3a, 9}, {0x3b, 9}, {0x3c, 9},
	{0x3d, 9}, {0x3e, 9}, {0x3f, 9}, {0x60, 11},
	{0x61, 11}, {0x62, 11}, {0x63, 11}, {0x64, 11},
	{0x65, 11}, {0x66, 11}, {0x67, 11}, {0x68, 11},
	{0x69, 11}, {0x6a, 11}, {0x6b, 11}, {0x6c, 11},
	{0x6d, 11}, {0x6e, 11}, {0x6f, 11}, {0x70, 11},
	{0x71, 11}, {0x72, 11}, {0x73, 11}, {0x74, 11},
	{0x75, 11}, {0x76, 11}, {0x77, 11}, {0x78, 11},
	{0x79, 11}, {0x7a, 11}, {0x7b, 11}, {0x7c, 11},
	{0x7d, 11}, {0x7e, 11}, {0x7f, 11}, {0xc0, 13},
	{0xc1, 13}, {0xc2, 13}, {0xc3, 13}, {0xc4, 13},
	{0xc5, 13}, {0xc6, 13}, {0xc7, 13}, {0xc8, 13},
	{0xc9, 13}, {0xca, 13}, {0xcb, 13}, {0xcc, 13},
	{0xcd, 13}, {0xce, 13}, {0xcf, 13}, {0xd0, 13},
	{0xd1, 13}, {0xd2, 13}, {0xd3, 13}, {0xd4, 13},
	{0xd5, 13}, {0xd6, 13}, {0xd7, 13}, {0xd8, 13},
	{0xd9, 13}, {0xda, 13}, {0xdb, 13}, {0xdc, 13},
	{0xdd, 13}, {0xde, 13}, {0xdf, 13}, {0xe0, 13},
	{0xe1, 13}, {0xe2, 13}, {0xe3, 13}, {0xe4, 13},
	{0xe5, 13}, {0xe6, 13}, {0xe7, 13}, {0xe8, 13},
	{0xe9, 13}, {0xea, 13}, {0xeb, 13}, {0xec, 13},
	{0xed, 13}, {0xee, 13}, {0xef, 13}, {0xf0, 13},
	{0xf1, 13}, {0xf2, 13}, {0xf3, 13}, {0xf4, 13},
	{0xf5, 13}, {0xf6, 13}, {0xf7, 13}, {0xf8, 13},
	{0xf9, 13}, {0xfa, 13}, {0xfb, 13}, {0xfc, 13},
	{0xfd, 13}, {0xfe, 13}, {0xff, 13}, {0x180, 15},
	{0x181, 15}, {0x182, 15}, {0x183, 15}, {0x184, 15},
	{0x185, 15}, {0x186, 15}, {0x187, 15}, {0x188, 15},
	{0x189, 15}, {0x18a, 15}, {0x18b, 15}, {0x18c, 15},
	{0x18d, 15}, {0x18e, 15}, {0x18f, 15}, {0x190, 15},
	{0x191, 15}, {0x192, 15}, {0x193, 15}, {0x194, 15},
	{0x195, 15}, {0x196, 15}, {0x197, 15}, {0x198, 15},
	{0x199, 15}, {0x19a, 15}, {0x19b, 15}, {0x19c, 15},
	{0x19d, 15}, {0x19e, 15}, {0x19f, 15}, {0x1a0, 15},
	{0x1a1, 15}, {0x1a2, 15}, {0x1a3, 15}, {0x1a4, 15},
	{0x1a5, 15}, {0x1a6, 15}, {0x1a7, 15}, {0x1a8, 15},
	{0x1a9, 15}, {0x1aa, 15}, {0x1ab, 15}, {0x1ac, 15},
	{0x1ad, 15}, {0x1ae, 15}, {0x1af, 15}, {0x1b0, 15},
	{0x1b1, 15}, {0x1b2, 15}, {0x1b3, 15}, {0x1b4, 15},
	{0x1b5, 15}, {0x1b6, 15}, {0x1b7, 15}, {0x1b8, 15},
	{0x1b9, 15}, {0x1ba, 15}, {0x1bb, 15}, {0x1bc, 15},
	{0x1bd, 15}, {0x1be, 15}, {0x1bf, 15}, {0x1c0, 15},
	{0x1c1, 15}, {0x1c2, 15}, {0x1c3, 15}, {0x1c4, 15},
	{0x1c5, 15}, {0x1c6, 15}, {0x1c7, 15}, {0x1c8, 15},
	{0x1c9, 15}, {0x1ca, 15}, {0x1cb, 15}, {0x1cc, 15},
	{0x1cd, 15}, {0x1ce, 15}, {0x1cf, 15}, {0x1d0, 15},
	{0x1d1, 15}, {0x1d2, 15}, {0x1d3, 15}, {0x1d4, 15},
	{0x1d5, 15}, {0x1d6, 15}, {0x1d7, 15}, {0x1d8, 15},
	{0x1d9, 15}, {0x1da, 15}, {0x1db, 15}, {0x1dc, 15},
	{0x1dd, 15}, {0x1de, 15}, {0x1df, 15}, {0x1e0, 15},
	{0x1e1, 15}, {0x1e2, 15}, {0x1e3, 15}, {0x1e4, 15},
	{0x1e5, 15}, {0x1e6, 15}, {0x1e7, 15}, {0x1e8, 15},
	{0x1e9, 15}, {0x1ea, 15}, {0x1eb, 15}, {0x1ec, 15},
	{0x1ed, 15}, {0x1ee, 15}, {0x1ef, 15}, {0x1f0, 15},
	{0x1f1, 15}, {0x1f2, 15}, {0x1f3, 15}, {0x1f4, 15},
	{0x1f5, 15}, {0x1f6, 15}, {0x1f7, 15}, {0x1f8, 15},
	{0x1f9, 15}, {0x1fa, 15}, {0x1fb, 15}, {0x1fc, 15},
	{0x1fd, 15}, {0x1fe, 15}, {0x1ff, 15},
};

const VLC dcc_tab[511] = {
	{0x100, 16}, {0x101, 16}, {0x102, 16}, {0x103, 16},
	{0x104, 16}, {0x105, 16}, {0x106, 16}, {0x107, 16},
	{0x108, 16}, {0x109, 16}, {0x10a, 16}, {0x10b, 16},
	{0x10c, 16}, {0x10d, 16}, {0x10e, 16}, {0x10f, 16},
	{0x110, 16}, {0x111, 16}, {0x112, 16}, {0x113, 16},
	{0x114, 16}, {0x115, 16}, {0x116, 16}, {0x117, 16},
	{0x118, 16}, {0x119, 16}, {0x11a, 16}, {0x11b, 16},
	{0x11c, 16}, {0x11d, 16}, {0x11e, 16}, {0x11f, 16},
	{0x120, 16}, {0x121, 16}, {0x122, 16}, {0x123, 16},
	{0x124, 16}, {0x125, 16}, {0x126, 16}, {0x127, 16},
	{0x128, 16}, {0x129, 16}, {0x12a, 16}, {0x12b, 16},
	{0x12c, 16}, {0x12d, 16}, {0x12e, 16}, {0x12f, 16},
	{0x130, 16}, {0x131, 16}, {0x132, 16}, {0x133, 16},
	{0x134, 16}, {0x135, 16}, {0x136, 16}, {0x137, 16},
	{0x138, 16}, {0x139, 16}, {0x13a, 16}, {0x13b, 16},
	{0x13c, 16}, {0x13d, 16}, {0x13e, 16}, {0x13f, 16},
	{0x140, 16}, {0x141, 16}, {0x142, 16}, {0x143, 16},
	{0x144, 16}, {0x145, 16}, {0x146, 16}, {0x147, 16},
	{0x148, 16}, {0x149, 16}, {0x14a, 16}, {0x14b, 16},
	{0x14c, 16}, {0x14d, 16}, {0x14e, 16}, {0x14f, 16},
	{0x150, 16}, {0x151, 16}, {0x152, 16}, {0x153, 16},
	{0x154, 16}, {0x155, 16}, {0x156, 16}, {0x157, 16},
	{0x158, 16}, {0x159, 16}, {0x15a, 16}, {0x15b, 16},
	{0x15c, 16}, {0x15d, 16}, {0x15e, 16}, {0x15f, 16},
	{0x160, 16}, {0x161, 16}, {0x162, 16}, {0x163, 16},
	{0x164, 16}, {0x165, 16}, {0x166, 16}, {0x167, 16},
	{0x168, 16}, {0x169, 16}, {0x16a, 16}, {0x16b, 16},
	{0x16c, 16}, {0x16d, 16}, {0x16e, 16}, {0x16f, 16},
	{0x170, 16}, {0x171, 16}, {0x172, 16}, {0x173, 16},
	{0x174, 16}, {0x175, 16}, {0x176, 16}, {0x177, 16},
	{0x178, 16}, {0x179, 16}, {0x17a, 16}, {0x17b, 16},
	{0x17c, 16}, {0x17d, 16}, {0x17e, 16}, {0x17f, 16},
	{0x80, 14}, {0x81, 14}, {0x82, 14}, {0x83, 14},
	{0x84, 14}, {0x85, 14}, {0x86, 14}, {0x87, 14},
	{0x88, 14}, {0x89, 14}, {0x8a, 14}, {0x8b, 14},
	{0x8c, 14}, {0x8d, 14}, {0x8e, 14}, {0x8f, 14},
	{0x90, 14}, {0x91, 14}, {0x92, 14}, {0x93, 14},
	{0x94, 14}, {0x95, 14}, {0x96, 14}, {0x97, 14},
	{0x98, 14}, {0x99, 14}, {0x9a, 14}, {0x9b, 14},
	{0x9c, 14}, {0x9d, 14}, {0x9e, 14}, {0x9f, 14},
	{0xa0, 14}, {0xa1, 14}, {0xa2, 14}, {0xa3, 14},
	{0xa4, 14}, {0xa5, 14}, {0xa6, 14}, {0xa7, 14},
	{0xa8, 14}, {0xa9, 14}, {0xaa, 14}, {0xab, 14},
	{0xac, 14}, {0xad, 14}, {0xae, 14}, {0xaf, 14},
	{0xb0, 14}, {0xb1, 14}, {0xb2, 14}, {0xb3, 14},
	{0xb4, 14}, {0xb5, 14}, {0xb6, 14}, {0xb7, 14},
	{0xb8, 14}, {0xb9, 14}, {0xba, 14}, {0xbb, 14},
	{0xbc, 14}, {0xbd, 14}, {0xbe, 14}, {0xbf, 14},
	{0x40, 12}, {0x41, 12}, {0x42, 12}, {0x43, 12},
	{0x44, 12}, {0x45, 12}, {0x46, 12}, {0x47, 12},
	{0x48, 12}, {0x49, 12}, {0x4a, 12}, {0x4b, 12},
	{0x4c, 12}, {0x4d, 12}, {0x4e, 12}, {0x4f, 12},
	{0x50, 12}, {0x51, 12}, {0x52, 12}, {0x53, 12},
	{0x54, 12}, {0x55, 12}, {0x56, 12}, {0x57, 12},
	{0x58, 12}, {0x59, 12}, {0x5a, 12}, {0x5b, 12},
	{0x5c, 12}, {0x5d, 12}, {0x5e, 12}, {0x5f, 12},
	{0x20, 10}, {0x21, 10}, {0x22, 10}, {0x23, 10},
	{0x24, 10}, {0x25, 10}, {0x26, 10}, {0x27, 10},
	{0x28, 10}, {0x29, 10}, {0x2a, 10}, {0x2b, 10},
	{0x2c, 10}, {0x2d, 10}, {0x2e, 10}, {0x2f, 10},
	{0x10, 8}, {0x11, 8}, {0x12, 8}, {0x13, 8},
	{0x14, 8}, {0x15, 8}, {0x16, 8}, {0x17, 8},
	{0x08, 6}, {0x09, 6}, {0x0a, 6}, {0x0b, 6},
	{0x04, 4}, {0x05, 4}, {0x04, 3}, {0x03, 2},
	{0x05, 3}, {0x06, 4}, {0x07, 4}, {0x0c, 6},
	{0x0d, 6}, {0x0e, 6}, {0x0f, 6}, {0x18, 8},
	{0x19, 8}, {0x1a, 8}, {0x1b, 8}, {0x1c, 8},
	{0x1d, 8}, {0x1e, 8}, {0x1f, 8}, {0x30, 10},
	{0x31, 10}, {0x32, 10}, {0x33, 10}, {0x34, 10},
	{0x35, 10}, {0x36, 10}, {0x37, 10}, {0x38, 10},
	{0x39, 10}, {0x3a, 10}, {0x3b, 10}, {0x3c, 10},
	{0x3d, 10}, {0x3e, 10}, {0x3f, 10}, {0x60, 12},
	{0x61, 12}, {0x62, 12}, {0x63, 12}, {0x64, 12},
	{0x65, 12}, {0x66, 12}, {0x67, 12}, {0x68, 12},
	{0x69, 12}, {0x6a, 12}, {0x6b, 12}, {0x6c, 12},
	{0x6d, 12}, {0x6e, 12}, {0x6f, 12}, {0x70, 12},
	{0x71, 12}, {0x72, 12}, {0x73, 12}, {0x74, 12},
	{0x75, 12}, {0x76, 12}, {0x77, 12}, {0x78, 12},
	{0x79, 12}, {0x7a, 12}, {0x7b, 12}, {0x7c, 12},
	{0x7d, 12}, {0x7e, 12}, {0x7f, 12}, {0xc0, 14},
	{0xc1, 14}, {0xc2, 14}, {0xc3, 14}, {0xc4, 14},
	{0xc5, 14}, {0xc6, 14}, {0xc7, 14}, {0xc8, 14},
	{0xc9, 14}, {0xca, 14}, {0xcb, 14}, {0xcc, 14},
	{0xcd, 14}, {0xce, 14}, {0xcf, 14}, {0xd0, 14},
	{0xd1, 14}, {0xd2, 14}, {0xd3, 14}, {0xd4, 14},
	{0xd5, 14}, {0xd6, 14}, {0xd7, 14}, {0xd8, 14},
	{0xd9, 14}, {0xda, 14}, {0xdb, 14}, {0xdc, 14},
	{0xdd, 14}, {0xde, 14}, {0xdf, 14}, {0xe0, 14},
	{0xe1, 14}, {0xe2, 14}, {0xe3, 14}, {0xe4, 14},
	{0xe5, 14}, {0xe6, 14}, {0xe7, 14}, {0xe8, 14},
	{0xe9, 14}, {0xea, 14}, {0xeb, 14}, {0xec, 14},
	{0xed, 14}, {0xee, 14}, {0xef, 14}, {0xf0, 14},
	{0xf1, 14}, {0xf2, 14}, {0xf3, 14}, {0xf4, 14},
	{0xf5, 14}, {0xf6, 14}, {0xf7, 14}, {0xf8, 14},
	{0xf9, 14}, {0xfa, 14}, {0xfb, 14}, {0xfc, 14},
	{0xfd, 14}, {0xfe, 14}, {0xff, 14}, {0x180, 16},
	{0x181, 16}, {0x182, 16}, {0x183, 16}, {0x184, 16},
	{0x185, 16}, {0x186, 16}, {0x187, 16}, {0x188, 16},
	{0x189, 16}, {0x18a, 16}, {0x18b, 16}, {0x18c, 16},
	{0x18d, 16}, {0x18e, 16}, {0x18f, 16}, {0x190, 16},
	{0x191, 16}, {0x192, 16}, {0x193, 16}, {0x194, 16},
	{0x195, 16}, {0x196, 16}, {0x197, 16}, {0x198, 16},
	{0x199, 16}, {0x19a, 16}, {0x19b, 16}, {0x19c, 16},
	{0x19d, 16}, {0x19e, 16}, {0x19f, 16}, {0x1a0, 16},
	{0x1a1, 16}, {0x1a2, 16}, {0x1a3, 16}, {0x1a4, 16},
	{0x1a5, 16}, {0x1a6, 16}, {0x1a7, 16}, {0x1a8, 16},
	{0x1a9, 16}, {0x1aa, 16}, {0x1ab, 16}, {0x1ac, 16},
	{0x1ad, 16}, {0x1ae, 16}, {0x1af, 16}, {0x1b0, 16},
	{0x1b1, 16}, {0x1b2, 16}, {0x1b3, 16}, {0x1b4, 16},
	{0x1b5, 16}, {0x1b6, 16}, {0x1b7, 16}, {0x1b8, 16},
	{0x1b9, 16}, {0x1ba, 16}, {0x1bb, 16}, {0x1bc, 16},
	{0x1bd, 16}, {0x1be, 16}, {0x1bf, 16}, {0x1c0, 16},
	{0x1c1, 16}, {0x1c2, 16}, {0x1c3, 16}, {0x1c4, 16},
	{0x1c5, 16}, {0x1c6, 16}, {0x1c7, 16}, {0x1c8, 16},
	{0x1c9, 16}, {0x1ca, 16}, {0x1cb, 16}, {0x1cc, 16},
	{0x1cd, 16}, {0x1ce, 16}, {0x1cf, 16}, {0x1d0, 16},
	{0x1d1, 16}, {0x1d2, 16}, {0x1d3, 16}, {0x1d4, 16},
	{0x1d5, 16}, {0x1d6, 16}, {0x1d7, 16}, {0x1d8, 16},
	{0x1d9, 16}, {0x1da, 16}, {0x1db, 16}, {0x1dc, 16},
	{0x1dd, 16}, {0x1de, 16}, {0x1df, 16}, {0x1e0, 16},
	{0x1e1, 16}, {0x1e2, 16}, {0x1e3, 16}, {0x1e4, 16},
	{0x1e5, 16}, {0x1e6, 16}, {0x1e7, 16}, {0x1e8, 16},
	{0x1e9, 16}, {0x1ea, 16}, {0x1eb, 16}, {0x1ec, 16},
	{0x1ed, 16}, {0x1ee, 16}, {0x1ef, 16}, {0x1f0, 16},
	{0x1f1, 16}, {0x1f2, 16}, {0x1f3, 16}, {0x1f4, 16},
	{0x1f5, 16}, {0x1f6, 16}, {0x1f7, 16}, {0x1f8, 16},
	{0x1f9, 16}, {0x1fa, 16}, {0x1fb, 16}, {0x1fc, 16},
	{0x1fd, 16}, {0x1fe, 16}, {0x1ff, 16},
};


const VLC mb_motion_table[65] = {
	{0x05, 13}, {0x07, 13}, {0x05, 12}, {0x07, 12},
	{0x09, 12}, {0x0b, 12}, {0x0d, 12}, {0x0f, 12},
	{0x09, 11}, {0x0b, 11}, {0x0d, 11}, {0x0f, 11},
	{0x11, 11}, {0x13, 11}, {0x15, 11}, {0x17, 11},
	{0x19, 11}, {0x1b, 11}, {0x1d, 11}, {0x1f, 11},
	{0x21, 11}, {0x23, 11}, {0x13, 10}, {0x15, 10},
	{0x17, 10}, {0x07, 8}, {0x09, 8}, {0x0b, 8},
	{0x07, 7}, {0x03, 5}, {0x03, 4}, {0x03, 3},
	{0x01, 1}, {0x02, 3}, {0x02, 4}, {0x02, 5},
	{0x06, 7}, {0x0a, 8}, {0x08, 8}, {0x06, 8},
	{0x16, 10}, {0x14, 10}, {0x12, 10}, {0x22, 11},
	{0x20, 11}, {0x1e, 11}, {0x1c, 11}, {0x1a, 11},
	{0x18, 11}, {0x16, 11}, {0x14, 11}, {0x12, 11},
	{0x10, 11}, {0x0e, 11}, {0x0c, 11}, {0x0a, 11},
	{0x08, 11}, {0x0e, 12}, {0x0c, 12}, {0x0a, 12},
	{0x08, 12}, {0x06, 12}, {0x04, 12}, {0x06, 13},
	{0x04, 13}
};


/******************************************************************
 * decoder tables                                                 *
 ******************************************************************/

VLC const mcbpc_intra_table[64] = {
	{-1, 0}, {20, 6}, {36, 6}, {52, 6}, {4, 4},  {4, 4},  {4, 4},  {4, 4},
	{19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3}, {19, 3},
	{35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3}, {35, 3},
	{51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3}, {51, 3},
	{3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},
	{3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},
	{3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},
	{3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1},  {3, 1}
};

VLC const mcbpc_inter_table[257] = {
	{VLC_ERROR, 0}, {255, 9}, {52, 9}, {36, 9}, {20, 9}, {49, 9}, {35, 8}, {35, 8},
	{19, 8}, {19, 8}, {50, 8}, {50, 8}, {51, 7}, {51, 7}, {51, 7}, {51, 7},
	{34, 7}, {34, 7}, {34, 7}, {34, 7}, {18, 7}, {18, 7}, {18, 7}, {18, 7},
	{33, 7}, {33, 7}, {33, 7}, {33, 7}, {17, 7}, {17, 7}, {17, 7}, {17, 7},
	{4, 6}, {4, 6}, {4, 6}, {4, 6}, {4, 6}, {4, 6}, {4, 6}, {4, 6},
	{48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6}, {48, 6},
	{3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5},
	{3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5}, {3, 5},
	{32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
	{32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
	{32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
	{32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4}, {32, 4},
	{16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
	{16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
	{16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
	{16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4}, {16, 4},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3}, {2, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
	{0, 1}
};

VLC const cbpy_table[64] = {
	{-1, 0}, {-1, 0}, {6, 6},  {9, 6},  {8, 5},  {8, 5},  {4, 5},  {4, 5},
	{2, 5},  {2, 5},  {1, 5},  {1, 5},  {0, 4},  {0, 4},  {0, 4},  {0, 4},
	{12, 4}, {12, 4}, {12, 4}, {12, 4}, {10, 4}, {10, 4}, {10, 4}, {10, 4},
	{14, 4}, {14, 4}, {14, 4}, {14, 4}, {5, 4},  {5, 4},  {5, 4},  {5, 4},
	{13, 4}, {13, 4}, {13, 4}, {13, 4}, {3, 4},  {3, 4},  {3, 4},  {3, 4},
	{11, 4}, {11, 4}, {11, 4}, {11, 4}, {7, 4},  {7, 4},  {7, 4},  {7, 4},
	{15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2},
	{15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}, {15, 2}
};

VLC const TMNMVtab0[] = {
	{3, 4}, {-3, 4}, {2, 3}, {2, 3}, {-2, 3}, {-2, 3}, {1, 2},
	{1, 2}, {1, 2}, {1, 2}, {-1, 2}, {-1, 2}, {-1, 2}, {-1, 2}
};

VLC const TMNMVtab1[] = {
	{12, 10}, {-12, 10}, {11, 10}, {-11, 10},
	{10, 9}, {10, 9}, {-10, 9}, {-10, 9},
	{9, 9}, {9, 9}, {-9, 9}, {-9, 9},
	{8, 9}, {8, 9}, {-8, 9}, {-8, 9},
	{7, 7}, {7, 7}, {7, 7}, {7, 7},
	{7, 7}, {7, 7}, {7, 7}, {7, 7},
	{-7, 7}, {-7, 7}, {-7, 7}, {-7, 7},
	{-7, 7}, {-7, 7}, {-7, 7}, {-7, 7},
	{6, 7}, {6, 7}, {6, 7}, {6, 7},
	{6, 7}, {6, 7}, {6, 7}, {6, 7},
	{-6, 7}, {-6, 7}, {-6, 7}, {-6, 7},
	{-6, 7}, {-6, 7}, {-6, 7}, {-6, 7},
	{5, 7}, {5, 7}, {5, 7}, {5, 7},
	{5, 7}, {5, 7}, {5, 7}, {5, 7},
	{-5, 7}, {-5, 7}, {-5, 7}, {-5, 7},
	{-5, 7}, {-5, 7}, {-5, 7}, {-5, 7},
	{4, 6}, {4, 6}, {4, 6}, {4, 6},
	{4, 6}, {4, 6}, {4, 6}, {4, 6},
	{4, 6}, {4, 6}, {4, 6}, {4, 6},
	{4, 6}, {4, 6}, {4, 6}, {4, 6},
	{-4, 6}, {-4, 6}, {-4, 6}, {-4, 6},
	{-4, 6}, {-4, 6}, {-4, 6}, {-4, 6},
	{-4, 6}, {-4, 6}, {-4, 6}, {-4, 6},
	{-4, 6}, {-4, 6}, {-4, 6}, {-4, 6}
};

VLC const TMNMVtab2[] = {
	{32, 12}, {-32, 12}, {31, 12}, {-31, 12},
	{30, 11}, {30, 11}, {-30, 11}, {-30, 11},
	{29, 11}, {29, 11}, {-29, 11}, {-29, 11},
	{28, 11}, {28, 11}, {-28, 11}, {-28, 11},
	{27, 11}, {27, 11}, {-27, 11}, {-27, 11},
	{26, 11}, {26, 11}, {-26, 11}, {-26, 11},
	{25, 11}, {25, 11}, {-25, 11}, {-25, 11},
	{24, 10}, {24, 10}, {24, 10}, {24, 10},
	{-24, 10}, {-24, 10}, {-24, 10}, {-24, 10},
	{23, 10}, {23, 10}, {23, 10}, {23, 10},
	{-23, 10}, {-23, 10}, {-23, 10}, {-23, 10},
	{22, 10}, {22, 10}, {22, 10}, {22, 10},
	{-22, 10}, {-22, 10}, {-22, 10}, {-22, 10},
	{21, 10}, {21, 10}, {21, 10}, {21, 10},
	{-21, 10}, {-21, 10}, {-21, 10}, {-21, 10},
	{20, 10}, {20, 10}, {20, 10}, {20, 10},
	{-20, 10}, {-20, 10}, {-20, 10}, {-20, 10},
	{19, 10}, {19, 10}, {19, 10}, {19, 10},
	{-19, 10}, {-19, 10}, {-19, 10}, {-19, 10},
	{18, 10}, {18, 10}, {18, 10}, {18, 10},
	{-18, 10}, {-18, 10}, {-18, 10}, {-18, 10},
	{17, 10}, {17, 10}, {17, 10}, {17, 10},
	{-17, 10}, {-17, 10}, {-17, 10}, {-17, 10},
	{16, 10}, {16, 10}, {16, 10}, {16, 10},
	{-16, 10}, {-16, 10}, {-16, 10}, {-16, 10},
	{15, 10}, {15, 10}, {15, 10}, {15, 10},
	{-15, 10}, {-15, 10}, {-15, 10}, {-15, 10},
	{14, 10}, {14, 10}, {14, 10}, {14, 10},
	{-14, 10}, {-14, 10}, {-14, 10}, {-14, 10},
	{13, 10}, {13, 10}, {13, 10}, {13, 10},
	{-13, 10}, {-13, 10}, {-13, 10}, {-13, 10}
};

short const dc_threshold[] = {
	26708, 29545, 29472, 26223, 30580, 29281,  8293, 29545,
	25632, 29285, 30313, 25701, 26144, 28530,  8301, 26740,
	 8293, 20039,  8277, 20551,  8268, 30296, 17513, 25376,
	25711, 25445, 10272, 11825, 11825, 10544,  2606, 28505,
	29301, 29472, 26223, 30580, 29281,  8293, 26980, 29811,
	26994, 30050, 28532,  8306, 24936,  8307, 28532, 26400,
	30313,  8293, 25441, 25955, 29555, 29728,  8303, 29801,
	 8307, 28531, 29301, 25955, 25376, 25711, 11877,    10
};

VLC const dc_lum_tab[] = {
	{0, 0}, {4, 3}, {3, 3}, {0, 3},
	{2, 2}, {2, 2}, {1, 2}, {1, 2},
};
