#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../lib/modedec.h"
#if !defined(OC_COLLECT_METRICS)
# error "Recompile libtheoraenc and encoder_example with -DOC_COLLECT_METRICS"
#endif
#define OC_COLLECT_NO_ENC_FUNCS (1)
#include "../lib/collect.c"



ogg_int16_t     OC_MODE_LOGQ_TMP[OC_LOGQ_BINS][3][2];
oc_mode_metrics OC_MODE_METRICS_SATD_TMP[OC_LOGQ_BINS-1][3][2][OC_COMP_BINS];
oc_mode_metrics OC_MODE_METRICS_SAD_TMP[OC_LOGQ_BINS-1][3][2][OC_COMP_BINS];

ogg_int16_t     OC_MODE_LOGQ_BASE[OC_LOGQ_BINS][3][2];
oc_mode_metrics OC_MODE_METRICS_SATD_BASE[OC_LOGQ_BINS-1][3][2][OC_COMP_BINS];
oc_mode_metrics OC_MODE_METRICS_SAD_BASE[OC_LOGQ_BINS-1][3][2][OC_COMP_BINS];


static int logq_cmp(ogg_int16_t(*a)[3][2],ogg_int16_t(*b)[3][2]){
  int pli;
  int qti;
  int qi;
  for(pli=0;pli<3;pli++){
    for(qti=0;qti<2;qti++){
      for(qi=0;qi<OC_LOGQ_BINS-1;qi++){
        if(a[qi][pli][qti]!=b[qi][pli][qti]){
          return EXIT_FAILURE;
        }
      }
    }
  }
  return EXIT_SUCCESS;
}

int main(int _argc,const char **_argv){
  FILE *fmetrics;
  int   want_base;
  int   have_base;
  int   want_output;
  int   have_output;
  int   processing_options;
  int   i;
  want_base=have_base=want_output=have_output=0;
  processing_options=1;
  for(i=1;i<_argc;i++){
    int pli;
    int qti;
    int qi;
    int si;
    if(processing_options&&!want_base&&!want_output){
      if(strcmp(_argv[i],"-b")==0){
        want_base=1;
        continue;
      }
      if(strcmp(_argv[i],"-o")==0){
        want_output=1;
        if(have_output)break;
        continue;
      }
      if(strcmp(_argv[i],"--")==0){
        processing_options=0;
        continue;
      }
    }
    if(want_output){
      OC_MODE_METRICS_FILENAME=_argv[i];
      have_output=1;
      want_output=0;
      continue;
    }
    fmetrics=fopen(_argv[i],"rb");
    if(fmetrics==NULL){
      fprintf(stderr,"Error opening '%s' for reading.\n",_argv[i]);
      return EXIT_FAILURE;
    }
    if(want_base){
      if(fread(OC_MODE_LOGQ_BASE,sizeof(OC_MODE_LOGQ_BASE),1,fmetrics)<1){
        fprintf(stderr,"Error reading quantizer bins from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(fread(OC_MODE_METRICS_SATD_BASE,sizeof(OC_MODE_METRICS_SATD_BASE),1,fmetrics)<1){
        fprintf(stderr,"Error reading SATD metrics from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(fread(OC_MODE_METRICS_SAD_BASE,sizeof(OC_MODE_METRICS_SAD_BASE),1,fmetrics)<1){
        fprintf(stderr,"Error reading SAD metrics from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(OC_HAS_MODE_METRICS){
        if(logq_cmp(OC_MODE_LOGQ,OC_MODE_LOGQ_BASE)){
          fprintf(stderr,
           "Error: quantizer bins in '%s' do not match previous files.\n",
           _argv[i]);
          return EXIT_FAILURE;
        }
      }
      want_base=0;
      have_base=1;
    }
    else if(!OC_HAS_MODE_METRICS){
      if(fread(OC_MODE_LOGQ,sizeof(OC_MODE_LOGQ),1,fmetrics)<1){
        fprintf(stderr,"Error reading quantizer bins from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(fread(OC_MODE_METRICS_SATD,sizeof(OC_MODE_METRICS_SATD),1,fmetrics)<1){
        fprintf(stderr,"Error reading SATD metrics from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(fread(OC_MODE_METRICS_SAD,sizeof(OC_MODE_METRICS_SAD),1,fmetrics)<1){
        fprintf(stderr,"Error reading SAD metrics from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(have_base){
        if(logq_cmp(OC_MODE_LOGQ,OC_MODE_LOGQ_BASE)){
          fprintf(stderr,
           "Error: quantizer bins in '%s' do not match previous files.\n",
           _argv[i]);
          return EXIT_FAILURE;
        }
      }
      OC_HAS_MODE_METRICS=1;
    }
    else{
      if(fread(OC_MODE_LOGQ_TMP,sizeof(OC_MODE_LOGQ_TMP),1,fmetrics)<1){
        fprintf(stderr,"Error reading quantizer bins from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(fread(OC_MODE_METRICS_SATD_TMP,sizeof(OC_MODE_METRICS_SATD_TMP),1,fmetrics)<1){
        fprintf(stderr,"Error reading SATD metrics from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(fread(OC_MODE_METRICS_SAD_TMP,sizeof(OC_MODE_METRICS_SAD_TMP),1,fmetrics)<1){
        fprintf(stderr,"Error reading SAD metrics from '%s'.\n",_argv[i]);
        return EXIT_FAILURE;
      }
      if(logq_cmp(OC_MODE_LOGQ,OC_MODE_LOGQ_TMP)){
        fprintf(stderr,
         "Error: quantizer bins in '%s' do not match previous files.\n",
         _argv[i]);
        return EXIT_FAILURE;
      }
      for(pli=0;pli<3;pli++){
        for(qti=0;qti<2;qti++){
          for(qi=0;qi<OC_LOGQ_BINS-1;qi++){
            for(si=0;si<OC_COMP_BINS;si++){
              oc_mode_metrics m[3];
              *(m+0)=*(OC_MODE_METRICS_SATD[qi][pli][qti]+si);
              *(m+1)=*(OC_MODE_METRICS_SATD_TMP[qi][pli][qti]+si);
              /*Subtract out the contribution from the base.*/
              if(have_base){
                m[2].w=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].w;
                m[2].s=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].s;
                m[2].q=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].q;
                m[2].r=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].r;
                m[2].d=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].d;
                m[2].s2=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].s2;
                m[2].sq=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].sq;
                m[2].q2=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].q2;
                m[2].sr=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].sr;
                m[2].qr=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].qr;
                m[2].r2=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].r2;
                m[2].sd=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].sd;
                m[2].qd=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].qd;
                m[2].d2=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].d2;
                m[2].s2q=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].s2q;
                m[2].sq2=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].sq2;
                m[2].sqr=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].sqr;
                m[2].sqd=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].sqd;
                m[2].s2q2=-OC_MODE_METRICS_SATD_BASE[qi][pli][qti][si].s2q2;
              }
              oc_mode_metrics_merge(OC_MODE_METRICS_SATD[qi][pli][qti]+si,
               m,2+have_base);
            }
            for(si=0;si<OC_COMP_BINS;si++){
              oc_mode_metrics m[3];
              *(m+0)=*(OC_MODE_METRICS_SAD[qi][pli][qti]+si);
              *(m+1)=*(OC_MODE_METRICS_SAD_TMP[qi][pli][qti]+si);
              /*Subtract out the contribution from the base.*/
              if(have_base){
                m[2].w=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].w;
                m[2].s=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].s;
                m[2].q=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].q;
                m[2].r=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].r;
                m[2].d=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].d;
                m[2].s2=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].s2;
                m[2].sq=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].sq;
                m[2].q2=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].q2;
                m[2].sr=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].sr;
                m[2].qr=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].qr;
                m[2].r2=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].r2;
                m[2].sd=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].sd;
                m[2].qd=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].qd;
                m[2].d2=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].d2;
                m[2].s2q=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].s2q;
                m[2].sq2=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].sq2;
                m[2].sqr=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].sqr;
                m[2].sqd=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].sqd;
                m[2].s2q2=-OC_MODE_METRICS_SAD_BASE[qi][pli][qti][si].s2q2;
              }
              oc_mode_metrics_merge(OC_MODE_METRICS_SAD[qi][pli][qti]+si,
               m,2+have_base);
            }
          }
        }
      }
    }
    fclose(fmetrics);
  }
  /*If the user specified -b but no base stats file, report an error.*/
  if(want_base){
    fprintf(stderr,"Error: missing base file argument.\n");
    OC_HAS_MODE_METRICS=0;
  }
  /*If the user specified -o with no file name, or multiple -o's, report an
     error.*/
  if(OC_HAS_MODE_METRICS&&want_output){
    if(have_output)fprintf(stderr,"Error: multiple output file arguments.\n");
    else fprintf(stderr,"Error: missing output file argument.\n");
    OC_HAS_MODE_METRICS=0;
  }
  if(!OC_HAS_MODE_METRICS){
    fprintf(stderr,"Usage: %s "
     "[-b <base.stats>] <in1.stats> [<in2.stats> ...] [-o <out.stats>]\n",
     _argv[0]);
    return EXIT_FAILURE;
  }
  /*Claim not to have metrics yet so update starts fresh.*/
  OC_HAS_MODE_METRICS=0;
  oc_mode_metrics_update(OC_MODE_METRICS_SATD,100,0,
   OC_MODE_RD_SATD,OC_SATD_SHIFT,OC_MODE_RD_WEIGHT_SATD);
  oc_mode_metrics_update(OC_MODE_METRICS_SAD,100,0,
   OC_MODE_RD_SAD,OC_SAD_SHIFT,OC_MODE_RD_WEIGHT_SAD);
  oc_mode_metrics_print(stdout);
  if(have_output)oc_mode_metrics_dump();
  return EXIT_SUCCESS;
}
