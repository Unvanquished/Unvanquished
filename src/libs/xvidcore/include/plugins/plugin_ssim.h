
/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - SSIM plugin: computes the SSIM metric  -
 *
 *  Copyright(C) 2005 Johannes Reinhardt <Johannes.Reinhardt@gmx.de>
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
 *
 *
 ****************************************************************************/

#ifndef SSIM_H
#define SSIM_H

/*Plugin for calculating and dumping the ssim quality metric according to 

http://www.cns.nyu.edu/~lcv/ssim/

there is a accurate (but very slow) implementation, using a 8x8 gaussian 
weighting window, that is quite close to the paper, and a faster unweighted 
implementation*/

typedef struct{
	/*stat output*/
	int b_printstat;
	char* stat_path;
	
	/*visualize*/
	int b_visualize;

	/*accuracy
	0 	gaussian weigthed (original, as in paper, very slow)
	<=4	unweighted, 1 slow 4 fastest*/
	int acc;

    int cpu_flags; /* XVID_CPU_XXX flags */

} plg_ssim_param_t;


int plugin_ssim(void * handle, int opt, void * param1, void * param2);

#endif
