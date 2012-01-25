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
// * $Id: calc_delta_3.s,v 1.1.14.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  calc_delta_3.s, IA-64 halfpel refinement                                  
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		
// *                                                                            
// ****************************************************************************

	;; 
(fb)	mov iMinSAD = mpr[8]
(fb)	mov currX = backupX
(fb)	mov currY = backupY
	mov ret0 = 2 
(non0_0)	add sc[0] = iFcode, sc[0]
(non0_1)	add sc[1] = iFcode, sc[1]
	;;
(non0_0)	add ret0 = ret0, sc[0]
	;; 
(non0_1)	add ret0 = ret0, sc[1]
	;; 

	setf.sig fmv = ret0
	;; 
	xmpy.l fmv = fmv, fQuant
	;; 
