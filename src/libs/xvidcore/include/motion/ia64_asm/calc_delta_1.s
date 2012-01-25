// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 halfpel refinement -
// *
// *  Copyright(C) 2002 Johannes Singler, Daniel Winkler
// *
// *  This program is free software; you can redistribute it and/or modify it
// *  under the terms of the GNU General Public License as published by
// *  the Free Software Foundation; either version 2 of the License, or
// *  (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// *  GNU General Public License for more details.
// *
// *  You should have received a copy of the GNU General Public License
// *  along with this program; if not, write to the Free Software
// *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
// *
// * $Id: calc_delta_1.s,v 1.1.14.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  calc_delta_1.s, IA-64 halfpel refinement                                  
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		
// *                                                                            
// ****************************************************************************

	;; 
	getf.sig ret0 = fmv
	add mpr[0] = mpr[0], mpr[1]
	add mpr[2] = mpr[2], mpr[3]
	add mpr[4] = mpr[4], mpr[5]
	add mpr[6] = mpr[6], mpr[7]
	;; 
	add mpr[0] = mpr[0], mpr[2]
	add mpr[4] = mpr[4], mpr[6]
	mov component[0] = dx
	mov component[1] = dy
	
	cmp.ne non0_2, p0 = 0, dx
	cmp.gt neg_2, p0 = 0, dx
	
	.pred.rel "mutex", p32, p36	//non0_0, neg_0
 
	cmp.ne non0_3, p0 = 0, dy
	cmp.gt neg_3, p0 = 0, dy
	;;
	.pred.rel "mutex", p33, p37	//non0_1, neg_1

	add iSAD = iSAD, ret0
	add mpr[8] = mpr[0], mpr[4]
(neg_2)	sub component[0] = 0, component[0]	//abs
(neg_3)	sub component[1] = 0, component[1]	//abs
	;; 
