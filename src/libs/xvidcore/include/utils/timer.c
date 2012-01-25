/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Timer functions (used for internal debugging)  -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
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
 * $Id: timer.c,v 1.9 2004/03/22 22:36:24 edgomez Exp $
 *
 ****************************************************************************/

#include <stdio.h>
#include <time.h>
#include "timer.h"

#if defined(_PROFILING_)

struct ts
{
	int64_t current;
	int64_t global;
	int64_t overall;
	int64_t dct;
	int64_t idct;
	int64_t quant;
	int64_t iquant;
	int64_t motion;
	int64_t comp;
	int64_t edges;
	int64_t inter;
	int64_t conv;
	int64_t trans;
	int64_t prediction;
	int64_t coding;
	int64_t interlacing;
};

struct ts tim;

double frequency = 0.0;

/*
    determine cpu frequency
	not very precise but sufficient
*/
double
get_freq()
{
	int64_t x, y;
	int32_t i;

	i = time(NULL);

	while (i == time(NULL));

	x = read_counter();
	i++;

	while (i == time(NULL));

	y = read_counter();

	return (double) (y - x) / 1000.;
}

/* set everything to zero */
void
init_timer()
{
	frequency = get_freq();

	count_frames = 0;

	tim.dct = tim.quant = tim.idct = tim.iquant = tim.motion = tim.conv =
		tim.edges = tim.inter = tim.interlacing = tim.trans = tim.trans =
		tim.coding = tim.global = tim.overall = 0;
}

void
start_timer()
{
	tim.current = read_counter();
}

void
start_global_timer()
{
	tim.global = read_counter();
}

void
stop_dct_timer()
{
	tim.dct += (read_counter() - tim.current);
}

void
stop_idct_timer()
{
	tim.idct += (read_counter() - tim.current);
}

void
stop_quant_timer()
{
	tim.quant += (read_counter() - tim.current);
}

void
stop_iquant_timer()
{
	tim.iquant += (read_counter() - tim.current);
}

void
stop_motion_timer()
{
	tim.motion += (read_counter() - tim.current);
}

void
stop_comp_timer()
{
	tim.comp += (read_counter() - tim.current);
}

void
stop_edges_timer()
{
	tim.edges += (read_counter() - tim.current);
}

void
stop_inter_timer()
{
	tim.inter += (read_counter() - tim.current);
}

void
stop_conv_timer()
{
	tim.conv += (read_counter() - tim.current);
}

void
stop_transfer_timer()
{
	tim.trans += (read_counter() - tim.current);
}

void
stop_prediction_timer()
{
	tim.prediction += (read_counter() - tim.current);
}

void
stop_coding_timer()
{
	tim.coding += (read_counter() - tim.current);
}

void
stop_interlacing_timer()
{
	tim.interlacing += (read_counter() - tim.current);
}

void
stop_global_timer()
{
	tim.overall += (read_counter() - tim.global);
}

/*
    write log file with some timer information
*/
void
write_timer()
{
	float dct_per, quant_per, idct_per, iquant_per, mot_per, comp_per,
		interlacing_per;
	float edges_per, inter_per, conv_per, trans_per, pred_per, cod_per,
		measured;
	int64_t sum_ticks = 0;

	count_frames++;

	// only write log file every 50 processed frames //
	if (count_frames % 50) {
		FILE *fp;

		fp = fopen("encoder.log", "w+");

		dct_per =
			(float) (((float) ((float) tim.dct / (float) tim.overall)) *
					 100.0);
		quant_per =
			(float) (((float) ((float) tim.quant / (float) tim.overall)) *
					 100.0);
		idct_per =
			(float) (((float) ((float) tim.idct / (float) tim.overall)) *
					 100.0);
		iquant_per =
			(float) (((float) ((float) tim.iquant / (float) tim.overall)) *
					 100.0);
		mot_per =
			(float) (((float) ((float) tim.motion / (float) tim.overall)) *
					 100.0);
		comp_per =
			(float) (((float) ((float) tim.comp / (float) tim.overall)) *
					 100.0);
		edges_per =
			(float) (((float) ((float) tim.edges / (float) tim.overall)) *
					 100.0);
		inter_per =
			(float) (((float) ((float) tim.inter / (float) tim.overall)) *
					 100.0);
		conv_per =
			(float) (((float) ((float) tim.conv / (float) tim.overall)) *
					 100.0);
		trans_per =
			(float) (((float) ((float) tim.trans / (float) tim.overall)) *
					 100.0);
		pred_per =
			(float) (((float) ((float) tim.prediction / (float) tim.overall)) *
					 100.0);
		cod_per =
			(float) (((float) ((float) tim.coding / (float) tim.overall)) *
					 100.0);
		interlacing_per =
			(float) (((float) ((float) tim.interlacing / (float) tim.overall))
					 * 100.0);

		sum_ticks =
			tim.coding + tim.conv + tim.dct + tim.idct + tim.interlacing +
			tim.edges + tim.inter + tim.iquant + tim.motion + tim.trans +
			tim.quant + tim.trans;

		measured =
			(float) (((float) ((float) sum_ticks / (float) tim.overall)) *
					 100.0);

		fprintf(fp,
				"DCT:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"Quant:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"IDCT:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"IQuant:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"Mot estimation:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"Mot compensation:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"Edges:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"Interpolation:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"RGB2YUV:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"Transfer:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"Prediction:\nTotal time: %f ms (%3f percent of total encoding time)\n\n"
				"Coding:\nTotal time: %f ms (%3f percent of total encoding time)\n\n\n"
				"Interlacing:\nTotal time: %f ms (%3f percent of total encoding time)\n\n\n"
				"Overall encoding time: %f ms, we measured %f ms (%3f percent)\n",
				(float) (tim.dct / frequency), dct_per,
				(float) (tim.quant / frequency), quant_per,
				(float) (tim.idct / frequency), idct_per,
				(float) (tim.iquant / frequency), iquant_per,
				(float) (tim.motion / frequency), mot_per,
				(float) (tim.comp / frequency), comp_per,
				(float) (tim.edges / frequency), edges_per,
				(float) (tim.inter / frequency), inter_per,
				(float) (tim.conv / frequency), conv_per,
				(float) (tim.trans / frequency), trans_per,
				(float) (tim.prediction / frequency), pred_per,
				(float) (tim.coding / frequency), cod_per,
				(float) (tim.interlacing / frequency), interlacing_per,
				(float) (tim.overall / frequency),
				(float) (sum_ticks / frequency), measured);

		fclose(fp);
	}
}

#endif
