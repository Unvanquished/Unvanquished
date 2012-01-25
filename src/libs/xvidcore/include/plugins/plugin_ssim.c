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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../portab.h"
#include "../xvid.h"
#include "plugin_ssim.h"
#include "../utils/emms.h"

/* needed for visualisation of the error map with X
 display.h borrowed from x264
#include "display.h"*/

typedef struct framestat_t framestat_t;

/*dev 1.0 gaussian weighting. the weight for the pixel x,y is w(x)*w(y)*/
static float mask8[8] = {
	0.0069815f, 0.1402264f, 1.0361408f, 2.8165226f, 
	2.8165226f, 1.0361408f, 0.1402264f, 0.0069815f
};

/* integer version. Norm: coeffs sums up to 4096.
  Define USE_INT_GAUSSIAN to use it as replacement to float version */

/* #define USE_INT_GAUSSIAN */
static const uint16_t imask8[8] = {
  4, 72, 530, 1442, 1442, 530, 72, 4
};
#define GACCUM(X)  ( ((X)+(1<<11)) >> 12 )


struct framestat_t{
	int type;
	int quant;
	float ssim_min;
	float ssim_max;
	float ssim_avg;
	framestat_t* next;
};


typedef int (*lumfunc)(uint8_t* ptr, int stride);
typedef void (*csfunc)(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);

int lum_8x8_mmx(uint8_t* ptr, int stride);
void consim_mmx(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);
void consim_sse2(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);

typedef struct{

	plg_ssim_param_t* param;

	/* for error map visualisation
	uint8_t* errmap;
	*/

	int grid;

	/*for average SSIM*/
	float ssim_sum;
	int frame_cnt;

	/*function pointers*/
	lumfunc func8x8;
	lumfunc func2x8;
	csfunc consim;

	/*stats - for debugging*/
	framestat_t* head;
	framestat_t* tail;
} ssim_data_t;

/* append the stats for another frame to the linked list*/
void framestat_append(ssim_data_t* ssim,int type, int quant, float min, float max, float avg){
	framestat_t* act;
	act = (framestat_t*) malloc(sizeof(framestat_t));
	act->type = type;
	act->quant = quant;
	act->ssim_min = min;
	act->ssim_max = max;
	act->ssim_avg = avg;
	act->next = NULL;

	if(ssim->head == NULL){
		ssim->head = act;
		ssim->tail = act;
	} else {
		ssim->tail->next = act;
		ssim->tail = act;
	}
}

/* destroy the whole list*/
void framestat_free(framestat_t* stat){
	if(stat != NULL){
		if(stat->next != NULL) framestat_free(stat->next);
		free(stat);	
	}
	return;
}

/*writeout the collected stats*/
void framestat_write(ssim_data_t* ssim, char* path){
	framestat_t* tmp = ssim->head;
	FILE* out = fopen(path,"w");
	if(out==NULL) printf("Cannot open %s in plugin_ssim\n",path);

	fprintf(out,"SSIM Error Metric\n");
	fprintf(out,"quant   avg     min     max\n");
	while(tmp->next->next != NULL){
		fprintf(out,"%3d     %1.3f   %1.3f   %1.3f\n",tmp->quant,tmp->ssim_avg,tmp->ssim_min,tmp->ssim_max);
		tmp = tmp->next;
	}
	fclose(out);
}

/*writeout the collected stats in octave readable format*/
void framestat_write_oct(ssim_data_t* ssim, char* path){
	framestat_t* tmp;
	FILE* out = fopen(path,"w");
	if(out==NULL) printf("Cannot open %s in plugin_ssim\n",path);

	fprintf(out,"quant = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		fprintf(out,"%d, ",tmp->quant);
		tmp = tmp->next;
	}
	fprintf(out,"%d];\n\n",tmp->quant);
	
	fprintf(out,"ssim_min = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		fprintf(out,"%f, ",tmp->ssim_min);
		tmp = tmp->next;
	}
	fprintf(out,"%f];\n\n",tmp->ssim_min);

	fprintf(out,"ssim_max = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		fprintf(out,"%f, ",tmp->ssim_max);
		tmp = tmp->next;
	}
	fprintf(out,"%f];\n\n",tmp->ssim_max);

	fprintf(out,"ssim_avg = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		fprintf(out,"%f, ",tmp->ssim_avg);
		tmp = tmp->next;
	}
	fprintf(out,"%f];\n\n",tmp->ssim_avg);

	fprintf(out,"ivop = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		if(tmp->type == XVID_TYPE_IVOP){
			fprintf(out,"%d, ",tmp->quant);
			fprintf(out,"%f, ",tmp->ssim_avg);
			fprintf(out,"%f, ",tmp->ssim_min);
			fprintf(out,"%f; ",tmp->ssim_max);
		}
		tmp = tmp->next;
	}
	fprintf(out,"%d, ",tmp->quant);
	fprintf(out,"%f, ",tmp->ssim_avg);
	fprintf(out,"%f, ",tmp->ssim_min);
	fprintf(out,"%f];\n\n",tmp->ssim_max);

	fprintf(out,"pvop = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		if(tmp->type == XVID_TYPE_PVOP){
			fprintf(out,"%d, ",tmp->quant);
			fprintf(out,"%f, ",tmp->ssim_avg);
			fprintf(out,"%f, ",tmp->ssim_min);
			fprintf(out,"%f; ",tmp->ssim_max);
		}
		tmp = tmp->next;
	}
	fprintf(out,"%d, ",tmp->quant);
	fprintf(out,"%f, ",tmp->ssim_avg);
	fprintf(out,"%f, ",tmp->ssim_min);
	fprintf(out,"%f];\n\n",tmp->ssim_max);

	fprintf(out,"bvop = [");
	tmp = ssim->head;
	while(tmp->next->next != NULL){
		if(tmp->type == XVID_TYPE_BVOP){
			fprintf(out,"%d, ",tmp->quant);
			fprintf(out,"%f, ",tmp->ssim_avg);
			fprintf(out,"%f, ",tmp->ssim_min);
			fprintf(out,"%f; ",tmp->ssim_max);
		}
		tmp = tmp->next;
	}
	fprintf(out,"%d, ",tmp->quant);
	fprintf(out,"%f, ",tmp->ssim_avg);
	fprintf(out,"%f, ",tmp->ssim_min);
	fprintf(out,"%f];\n\n",tmp->ssim_max);

	fclose(out);
}

/*calculate the luminance of a 8x8 block*/
int lum_8x8_c(uint8_t* ptr, int stride){
	int mean=0,i,j;
	for(i=0;i< 8;i++)
		for(j=0;j< 8;j++){
			mean += ptr[i*stride + j];
		}
	return mean;
}

int lum_8x8_gaussian(uint8_t* ptr, int stride){
	float mean=0,sum;
	int i,j;
	for(i=0;i<8;i++){
		sum = 0;
		for(j=0;j<8;j++)
			sum += ptr[i*stride + j]*mask8[j];
		
		sum *=mask8[i];
		mean += sum;
	}
	return (int) (mean + 0.5);
}

int lum_8x8_gaussian_int(uint8_t* ptr, int stride){
	uint32_t mean;
	int i,j;
	mean = 0;
	for(i=0;i<8;i++){
		uint32_t sum = 0;
		for(j=0;j<8;j++)
			sum += ptr[i*stride + j]*imask8[j];
		
		sum = GACCUM(sum) * imask8[i];
		mean += sum;
	}
	return (int)GACCUM(mean);
}

/*calculate the difference between two blocks next to each other on a row*/
int lum_2x8_c(uint8_t* ptr, int stride){
	int mean=0,i;
	/*Luminance*/
	for(i=0;i< 8;i++){
		mean -= *(ptr-1);
		mean += *(ptr+ 8 - 1);
		ptr+=stride;
	}
	return mean;
}

/*calculate contrast and correlation of the two blocks*/
void consim_gaussian(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr){
	unsigned int valo, valc,i,j,str;
	float devo=0, devc=0, corr=0,sumo,sumc,sumcorr;
	str = stride - 8;
	for(i=0;i< 8;i++){
		sumo = 0;
		sumc = 0;
		sumcorr = 0;
		for(j=0;j< 8;j++){
			valo = *ptro;
			valc = *ptrc;
			sumo += valo*valo*mask8[j];
			sumc += valc*valc*mask8[j];
			sumcorr += valo*valc*mask8[j];
			ptro++;
			ptrc++;
		}

	devo += sumo*mask8[i];
	devc += sumc*mask8[i];
	corr += sumcorr*mask8[i];
	ptro += str;
	ptrc += str;
	}

	*pdevo = (int) ((devo - ((lumo*lumo + 32) >> 6)) + 0.5);
	*pdevc = (int) ((devc - ((lumc*lumc + 32) >> 6)) + 0.5);
	*pcorr = (int) ((corr - ((lumo*lumc + 32) >> 6)) + 0.5);
};

void consim_gaussian_int(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr)
{
	unsigned int valo, valc,i,j,str;
	uint32_t  devo=0, devc=0, corr=0;
	str = stride - 8;
	for(i=0;i< 8;i++){
		uint32_t sumo = 0;
		uint32_t sumc = 0;
		uint32_t sumcorr = 0;
		for(j=0;j< 8;j++){
			valo = *ptro;
			valc = *ptrc;
			sumo += valo*valo*imask8[j];
			sumc += valc*valc*imask8[j];
			sumcorr += valo*valc*imask8[j];
			ptro++;
			ptrc++;
		}

	devo += GACCUM(sumo)*imask8[i];
	devc += GACCUM(sumc)*imask8[i];
	corr += GACCUM(sumcorr)*imask8[i];
	ptro += str;
	ptrc += str;
	}

        devo = GACCUM(devo);
        devc = GACCUM(devc);
        corr = GACCUM(corr);
	*pdevo = (int) ((devo - ((lumo*lumo + 32) >> 6)) + 0.5);
	*pdevc = (int) ((devc - ((lumc*lumc + 32) >> 6)) + 0.5);
	*pcorr = (int) ((corr - ((lumo*lumc + 32) >> 6)) + 0.5);
};

/*calculate contrast and correlation of the two blocks*/
void consim_c(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr){
	unsigned int valo, valc, devo=0, devc=0, corr=0,i,j,str;
	str = stride - 8;
	for(i=0;i< 8;i++){
		for(j=0;j< 8;j++){
			valo = *ptro;
			valc = *ptrc;
			devo += valo*valo;
			devc += valc*valc;
			corr += valo*valc;
			ptro++;
			ptrc++;
		}
	ptro += str;
	ptrc += str;
	}

	*pdevo = devo - ((lumo*lumo + 32) >> 6);
	*pdevc = devc - ((lumc*lumc + 32) >> 6);
	*pcorr = corr - ((lumo*lumc + 32) >> 6);
};

/*calculate the final ssim value*/
static float calc_ssim(float meano, float meanc, float devo, float devc, float corr){
	static const float c1 = (0.01f*255)*(0.01f*255);
	static const float c2 = (0.03f*255)*(0.03f*255);
	/*printf("meano: %f meanc: %f devo: %f devc: %f corr: %f\n",meano,meanc,devo,devc,corr);*/
	return ((2.0f*meano*meanc + c1)*(corr/32.0f + c2))/((meano*meano + meanc*meanc + c1)*(devc/64.0f + devo/64.0f + c2));
}

static void ssim_after(xvid_plg_data_t* data, ssim_data_t* ssim){
	int i,j,c=0,opt;
	int width,height,str,ovr;
	unsigned char * ptr1,*ptr2;
	float isum=0, min=1.00,max=0.00, val;
	int meanc, meano;
	int devc, devo, corr;

	width = data->width - 8;
	height = data->height - 8;
	str = data->original.stride[0];
	if(str != data->current.stride[0]) printf("WARNING: Different strides in plugin_ssim original: %d current: %d\n",str,data->current.stride[0]);
	ovr = str - width + (width % ssim->grid);

	ptr1 = (unsigned char*) data->original.plane[0];
	ptr2 = (unsigned char*) data->current.plane[0];

	opt = ssim->grid == 1 && ssim->param->acc != 0;

	/*TODO: Thread*/
	for(i=0;i<height;i+=ssim->grid){
		/*begin of each row*/
		meano = meanc = devc = devo = corr = 0;
		meano = ssim->func8x8(ptr1,str);
		meanc = ssim->func8x8(ptr2,str);
		ssim->consim(ptr1,ptr2,str,meano,meanc,&devo,&devc,&corr);
		emms();

		val = calc_ssim((float) meano,(float) meanc,(float) devo,(float) devc,(float) corr);
		isum += val;
		c++;
		/* for visualisation
		if(ssim->param->b_visualize)
			ssim->errmap[i*width] = (uint8_t) 127*val;
		*/


		if(val < min) min = val;
		if(val > max) max = val;
		ptr1+=ssim->grid;
		ptr2+=ssim->grid;
		/*rest of each row*/
		for(j=ssim->grid;j<width;j+=ssim->grid){
 			if(opt){
 				meano += ssim->func2x8(ptr1,str);
 				meanc += ssim->func2x8(ptr2,str);
 			} else {
				meano = ssim->func8x8(ptr1,str);
				meanc = ssim->func8x8(ptr2,str);
			}
			ssim->consim(ptr1,ptr2,str,meano,meanc,&devo,&devc,&corr);
			emms();	

			val = calc_ssim((float) meano,(float) meanc,(float) devo,(float) devc,(float) corr);
			isum += val;
			c++;
			/* for visualisation
			if(ssim->param->b_visualize)
				ssim->errmap[i*width +j] = (uint8_t) 255*val;
			*/
			if(val < min) min = val;
			if(val > max) max = val;
			ptr1+=ssim->grid;
			ptr2+=ssim->grid;
		}
		ptr1 +=ovr;
		ptr2 +=ovr;
 	}
	isum/=c;
	ssim->ssim_sum += isum;
	ssim->frame_cnt++;

	if(ssim->param->stat_path != NULL)
		framestat_append(ssim,data->type,data->quant,min,max,isum);

/* for visualization
	if(ssim->param->b_visualize){
		disp_gray(0,ssim->errmap,width,height,width, "Error-Map");
		disp_gray(1,data->original.plane[0],data->width,data->height,data->original.stride[0],"Original");
		disp_gray(2,data->current.plane[0],data->width,data->height,data->original.stride[0],"Compressed");
		disp_sync();
	}
*/
	if(ssim->param->b_printstat){
		printf("       SSIM: avg: %1.3f min: %1.3f max: %1.3f\n",isum,min,max);
	}

}

static int ssim_create(xvid_plg_create_t* create, void** handle){
	ssim_data_t* ssim;
	plg_ssim_param_t* param;
	param = (plg_ssim_param_t*) malloc(sizeof(plg_ssim_param_t));
	*param = *((plg_ssim_param_t*) create->param);
	ssim = (ssim_data_t*) malloc(sizeof(ssim_data_t));

	ssim->func8x8 = lum_8x8_c;
	ssim->func2x8 = lum_2x8_c;
	ssim->consim = consim_c;

	ssim->param = param;

	ssim->grid = param->acc;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
	{
        int cpu_flags = (param->cpu_flags & XVID_CPU_FORCE) ? param->cpu_flags : check_cpu_features();

		if((cpu_flags & XVID_CPU_MMX) && (param->acc > 0)){
			ssim->func8x8 = lum_8x8_mmx;
			ssim->consim = consim_mmx;
		}
		if((cpu_flags & XVID_CPU_SSE2) && (param->acc > 0)){
			ssim->consim = consim_sse2;
		}
	}
#endif

	/*gaussian weigthing not implemented*/
#if !defined(USE_INT_GAUSSIAN)
	if(ssim->grid == 0){
		ssim->grid = 1;
		ssim->func8x8 = lum_8x8_gaussian;
		ssim->func2x8 = NULL;
		ssim->consim = consim_gaussian;
	}
#else
	if(ssim->grid == 0){
		ssim->grid = 1;
		ssim->func8x8 = lum_8x8_gaussian_int;
		ssim->func2x8 = NULL;
		ssim->consim = consim_gaussian_int;
	}
#endif
	if(ssim->grid > 4) ssim->grid = 4;

	ssim->ssim_sum = 0.0;
	ssim->frame_cnt = 0;

/* for visualization
	if(param->b_visualize){
		//error map
		ssim->errmap = (uint8_t*) malloc(sizeof(uint8_t)*(create->width-8)*(create->height-8));
	} else {
		ssim->errmap = NULL;
	};
*/

	/*stats*/
	ssim->head=NULL;
	ssim->tail=NULL;

	*(handle) = (void*) ssim;

	return 0;
}	

int xvid_plugin_ssim(void * handle, int opt, void * param1, void * param2){
	ssim_data_t* ssim;
	switch(opt){
		case(XVID_PLG_INFO):
 			((xvid_plg_info_t*) param1)->flags = XVID_REQORIGINAL;
			break;
		case(XVID_PLG_CREATE):
			ssim_create((xvid_plg_create_t*) param1,(void**) param2);
			break;
		case(XVID_PLG_BEFORE):
		case(XVID_PLG_FRAME):
			break;
		case(XVID_PLG_AFTER):
			ssim_after((xvid_plg_data_t*) param1, (ssim_data_t*) handle);
			break;
		case(XVID_PLG_DESTROY):
			ssim = (ssim_data_t*) handle;
			printf("Average SSIM: %f\n",ssim->ssim_sum/ssim->frame_cnt);
			if(ssim->param->stat_path != NULL)
				framestat_write(ssim,ssim->param->stat_path);
			framestat_free(ssim->head);
			/*free(ssim->errmap);*/
			free(ssim->param);	
			free(ssim);
			break;
		default:
			break;
	}
	return 0;
};
