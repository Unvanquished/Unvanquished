/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Quantization matrix management header  -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
 *               2002 Peter Ross <pross@xvid.org>
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
 * $Id: quant_matrix.h,v 1.8 2006/07/10 08:09:59 syskin Exp $
 *
 ****************************************************************************/

#ifndef _QUANT_MATRIX_H_
#define _QUANT_MATRIX_H_

#include "../portab.h"

#define SCALEBITS 17

void init_mpeg_matrix(uint16_t * mpeg_quant_matrices);

int is_custom_intra_matrix(const uint16_t * mpeg_quant_matrices);
int is_custom_inter_matrix(const uint16_t * mpeg_quant_matrices);

void set_intra_matrix(uint16_t *mpeg_quant_matrices, const uint8_t * matrix);
void set_inter_matrix(uint16_t *mpeg_quant_matrices, const uint8_t * matrix);

void init_intra_matrix(uint16_t * mpeg_quant_matrices, uint32_t quant);

const uint16_t *get_intra_matrix(const uint16_t *mpeg_quant_matrices);
const uint16_t *get_inter_matrix(const uint16_t *mpeg_quant_matrices);

const uint8_t *get_default_intra_matrix(void);
const uint8_t *get_default_inter_matrix(void);

#endif							/* _QUANT_MATRIX_H_ */
