/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - MB coding header  -
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
 * $Id: mbcoding.h,v 1.13 2004/07/10 17:49:31 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _MB_CODING_H_
#define _MB_CODING_H_

#include "../portab.h"
#include "../global.h"
#include "vlc_codes.h"
#include "bitstream.h"

void init_vlc_tables(void);

int check_resync_marker(Bitstream * bs, int addbits);

void bs_put_spritetrajectory(Bitstream * bs, const int val);
int bs_get_spritetrajectory(Bitstream * bs);

int get_mcbpc_intra(Bitstream * bs);
int get_mcbpc_inter(Bitstream * bs);
int get_cbpy(Bitstream * bs,
			 int intra);
int get_mv(Bitstream * bs,
		   int fcode);

int get_dc_dif(Bitstream * bs,
			   uint32_t dc_size);
int get_dc_size_lum(Bitstream * bs);
int get_dc_size_chrom(Bitstream * bs);

void get_intra_block(Bitstream * bs,
					 int16_t * block,
					 int direction,
					 int coeff);
void get_inter_block_h263(
		Bitstream * bs,
		int16_t * block,
		int direction,
		const int quant,
		const uint16_t *matrix);

void get_inter_block_mpeg(
		Bitstream * bs,
		int16_t * block,
		int direction,
		const int quant,
		const uint16_t *matrix);


void MBCodingBVOP(const FRAMEINFO * const frame,
				  const MACROBLOCK * mb,
				  const int16_t qcoeff[6 * 64],
				  const int32_t fcode,
				  const int32_t bcode,
				  Bitstream * bs,
				  Statistics * pStat);


static __inline void
MBSkip(Bitstream * bs)
{
	BitstreamPutBit(bs, 1);	/* not coded */
}

int CodeCoeffIntra_CalcBits(const int16_t qcoeff[64], const uint16_t * zigzag);
int CodeCoeffInter_CalcBits(const int16_t qcoeff[64], const uint16_t * zigzag);

#endif							/* _MB_CODING_H_ */
