/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Timer related header (used for internal debugging)  -
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
 * $Id: timer.h,v 1.11 2005/11/22 10:23:01 suxen_drol Exp $
 *
 ****************************************************************************/

#ifndef _ENCORE_TIMER_H
#define _ENCORE_TIMER_H

#if defined(_PROFILING_)

#include "../portab.h"

uint64_t count_frames;

extern void start_timer(void);
extern void start_global_timer(void);
extern void stop_dct_timer(void);
extern void stop_idct_timer(void);
extern void stop_motion_timer(void);
extern void stop_comp_timer(void);
extern void stop_edges_timer(void);
extern void stop_inter_timer(void);
extern void stop_quant_timer(void);
extern void stop_iquant_timer(void);
extern void stop_conv_timer(void);
extern void stop_transfer_timer(void);
extern void stop_coding_timer(void);
extern void stop_prediction_timer(void);
extern void stop_interlacing_timer(void);
extern void stop_global_timer(void);
extern void init_timer(void);
extern void write_timer(void);

#else

static __inline void
start_timer(void)
{
}
static __inline void
start_global_timer(void)
{
}
static __inline void
stop_dct_timer(void)
{
}
static __inline void
stop_idct_timer(void)
{
}
static __inline void
stop_motion_timer(void)
{
}
static __inline void
stop_comp_timer(void)
{
}
static __inline void
stop_edges_timer(void)
{
}
static __inline void
stop_inter_timer(void)
{
}
static __inline void
stop_quant_timer(void)
{
}
static __inline void
stop_iquant_timer(void)
{
}
static __inline void
stop_conv_timer(void)
{
}
static __inline void
stop_transfer_timer(void)
{
}
static __inline void
init_timer(void)
{
}
static __inline void
write_timer(void)
{
}
static __inline void
stop_coding_timer(void)
{
}
static __inline void
stop_interlacing_timer(void)
{
}
static __inline void
stop_prediction_timer(void)
{
}
static __inline void
stop_global_timer(void)
{
}

#endif

#endif							/* _TIMER_H_ */
