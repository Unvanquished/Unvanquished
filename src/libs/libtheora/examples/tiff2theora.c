/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2011                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function: example encoder application; makes an Ogg Theora
            file from a sequence of tiff images
  last mod: $Id$
             based on png2theora

 ********************************************************************/

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <tiffio.h>
#include <ogg/ogg.h>
#include "theora/theoraenc.h"

#define PROGRAM_NAME  "tiff2theora"
#define PROGRAM_VERSION  "1.1"

static const char *option_output = NULL;
static int video_fps_numerator = 24;
static int video_fps_denominator = 1;
static int video_aspect_numerator = 0;
static int video_aspect_denominator = 0;
static int video_rate = -1;
static int video_quality = -1;
ogg_uint32_t keyframe_frequency=0;
int buf_delay=-1;
int vp3_compatible=0;
static int chroma_format = TH_PF_420;

static FILE *twopass_file = NULL;
static  int twopass=0;
static  int passno;

static FILE *ogg_fp = NULL;
static ogg_stream_state ogg_os;
static ogg_packet op;
static ogg_page og;

static th_enc_ctx      *td;
static th_info          ti;

static char *input_filter;

const char *optstring = "o:hv:\4:\2:V:s:S:f:F:ck:d:\1\2\3\4\5\6";
struct option options [] = {
 {"output",required_argument,NULL,'o'},
 {"help",no_argument,NULL,'h'},
 {"chroma-444",no_argument,NULL,'\5'},
 {"chroma-422",no_argument,NULL,'\6'},
 {"video-rate-target",required_argument,NULL,'V'},
 {"video-quality",required_argument,NULL,'v'},
 {"aspect-numerator",required_argument,NULL,'s'},
 {"aspect-denominator",required_argument,NULL,'S'},
 {"framerate-numerator",required_argument,NULL,'f'},
 {"framerate-denominator",required_argument,NULL,'F'},
 {"vp3-compatible",no_argument,NULL,'c'},
 {"soft-target",no_argument,NULL,'\1'},
 {"keyframe-freq",required_argument,NULL,'k'},
 {"buf-delay",required_argument,NULL,'d'},
 {"two-pass",no_argument,NULL,'\2'},
 {"first-pass",required_argument,NULL,'\3'},
 {"second-pass",required_argument,NULL,'\4'},
 {NULL,0,NULL,0}
};

static void usage(void){
  fprintf(stderr,
          "%s %s\n"
          "Usage: %s [options] <input>\n\n"
          "The input argument uses C printf format to represent a list of files,\n"
          "  i.e. file-%%06d.tiff to look for files file000001.tiff to file9999999.tiff \n\n"
          "Options: \n\n"
          "  -o --output <filename.ogv>      file name for encoded output (required);\n"
          "  -v --video-quality <n>          Theora quality selector fro 0 to 10\n"
          "                                  (0 yields smallest files but lowest\n"
          "                                  video quality. 10 yields highest\n"
          "                                  fidelity but large files)\n\n"
          "  -V --video-rate-target <n>      bitrate target for Theora video\n\n"
          "     --soft-target                Use a large reservoir and treat the rate\n"
          "                                  as a soft target; rate control is less\n"
          "                                  strict but resulting quality is usually\n"
          "                                  higher/smoother overall. Soft target also\n"
          "                                  allows an optional -v setting to specify\n"
          "                                  a minimum allowed quality.\n\n"
          "     --two-pass                   Compress input using two-pass rate control\n"
          "                                  This option performs both passes automatically.\n\n"
          "     --first-pass <filename>      Perform first-pass of a two-pass rate\n"
          "                                  controlled encoding, saving pass data to\n"
          "                                  <filename> for a later second pass\n\n"
          "     --second-pass <filename>     Perform second-pass of a two-pass rate\n"
          "                                  controlled encoding, reading first-pass\n"
          "                                  data from <filename>.  The first pass\n"
          "                                  data must come from a first encoding pass\n"
          "                                  using identical input video to work\n"
          "                                  properly.\n\n"
          "   -k --keyframe-freq <n>         Keyframe frequency\n"
          "   -d --buf-delay <n>             Buffer delay (in frames). Longer delays\n"
          "                                  allow smoother rate adaptation and provide\n"
          "                                  better overall quality, but require more\n"
          "                                  client side buffering and add latency. The\n"
          "                                  default value is the keyframe interval for\n"
          "                                  one-pass encoding (or somewhat larger if\n"
          "                                  --soft-target is used) and infinite for\n"
          "                                  two-pass encoding.\n"
          "  --chroma-444                    Use 4:4:4 chroma subsampling\n"
          "  --chroma-422                    Use 4:2:2 chroma subsampling\n"
          "                                  (4:2:0 is default)\n\n"
          "  -s --aspect-numerator <n>       Aspect ratio numerator, default is 0\n"
          "  -S --aspect-denominator <n>     Aspect ratio denominator, default is 0\n"
          "  -f --framerate-numerator <n>    Frame rate numerator\n"
          "  -F --framerate-denominator <n>  Frame rate denominator\n"
          "                                  The frame rate nominator divided by this\n"
          "                                  determines the frame rate in units per tick\n"
          ,PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_NAME
  );
  exit(0);
}

#ifdef WIN32
int
alphasort (const void *a, const void *b)
{
  return strcoll ((*(const struct dirent **) a)->d_name,
                  (*(const struct dirent **) b)->d_name);
}

int
scandir (const char *dir, struct dirent ***namelist,
         int (*select)(const struct dirent *), int (*compar)(const void *, const void *))
{
  DIR *d;
  struct dirent *entry;
  register int i=0;
  size_t entrysize;

  if ((d=opendir(dir)) == NULL)
    return(-1);

  *namelist=NULL;
  while ((entry=readdir(d)) != NULL)
  {
    if (select == NULL || (select != NULL && (*select)(entry)))
    {
      *namelist=(struct dirent **)realloc((void *)(*namelist),
                 (size_t)((i+1)*sizeof(struct dirent *)));
      if (*namelist == NULL) return(-1);
      entrysize=sizeof(struct dirent)-sizeof(entry->d_name)+strlen(entry->d_name)+1;
      (*namelist)[i]=(struct dirent *)malloc(entrysize);
      if ((*namelist)[i] == NULL) return(-1);
        memcpy((*namelist)[i], entry, entrysize);
      i++;
    }
  }
  if (closedir(d)) return(-1);
  if (i == 0) return(-1);
  if (compar != NULL)
    qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), compar);

  return(i);
}
#endif

static int
theora_write_frame(th_ycbcr_buffer ycbcr, int last)
{
  ogg_packet op;
  ogg_page og;

  /* Theora is a one-frame-in,one-frame-out system; submit a frame
     for compression and pull out the packet */
  /* in two-pass mode's second pass, we need to submit first-pass data */
  if(passno==2){
    int ret;
    for(;;){
      static unsigned char buffer[80];
      static int buf_pos;
      int bytes;
      /*Ask the encoder how many bytes it would like.*/
      bytes=th_encode_ctl(td,TH_ENCCTL_2PASS_IN,NULL,0);
      if(bytes<0){
        fprintf(stderr,"Error submitting pass data in second pass.\n");
        exit(1);
      }
      /*If it's got enough, stop.*/
      if(bytes==0)break;
      /*Read in some more bytes, if necessary.*/
      if(bytes>80-buf_pos)bytes=80-buf_pos;
      if(bytes>0&&fread(buffer+buf_pos,1,bytes,twopass_file)<bytes){
        fprintf(stderr,"Could not read frame data from two-pass data file!\n");
        exit(1);
      }
      /*And pass them off.*/
      ret=th_encode_ctl(td,TH_ENCCTL_2PASS_IN,buffer,bytes);
      if(ret<0){
        fprintf(stderr,"Error submitting pass data in second pass.\n");
        exit(1);
      }
      /*If the encoder consumed the whole buffer, reset it.*/
      if(ret>=bytes)buf_pos=0;
      /*Otherwise remember how much it used.*/
      else buf_pos+=ret;
    }
  }

  if(th_encode_ycbcr_in(td, ycbcr)) {
    fprintf(stderr, "%s: error: could not encode frame\n",
      option_output);
    return 1;
  }

  /* in two-pass mode's first pass we need to extract and save the pass data */
  if(passno==1){
    unsigned char *buffer;
    int bytes = th_encode_ctl(td, TH_ENCCTL_2PASS_OUT, &buffer, sizeof(buffer));
    if(bytes<0){
      fprintf(stderr,"Could not read two-pass data from encoder.\n");
      exit(1);
    }
    if(fwrite(buffer,1,bytes,twopass_file)<bytes){
      fprintf(stderr,"Unable to write to two-pass data file.\n");
      exit(1);
    }
    fflush(twopass_file);
  }

  if(!th_encode_packetout(td, last, &op)) {
    fprintf(stderr, "%s: error: could not read packets\n",
      option_output);
    return 1;
  }

  if (passno!=1) {
    ogg_stream_packetin(&ogg_os, &op);
    while(ogg_stream_pageout(&ogg_os, &og)) {
      fwrite(og.header, og.header_len, 1, ogg_fp);
      fwrite(og.body, og.body_len, 1, ogg_fp);
    }
  }

  return 0;
}

static unsigned char
clamp(int d)
{
  if(d < 0)
    return 0;

  if(d > 255)
    return 255;

  return d;
}

static void
rgb_to_yuv(uint32 *raster,
           th_ycbcr_buffer ycbcr,
           unsigned int w, unsigned int h)
{
  unsigned int x;
  unsigned int y;

  unsigned int x1;
  unsigned int y1;

  unsigned long yuv_w;
  unsigned long yuv_h;

  unsigned char *yuv_y;
  unsigned char *yuv_u;
  unsigned char *yuv_v;

  yuv_w = ycbcr[0].width;
  yuv_h = ycbcr[0].height;

  yuv_y = ycbcr[0].data;
  yuv_u = ycbcr[1].data;
  yuv_v = ycbcr[2].data;

  /*This ignores gamma and RGB primary/whitepoint differences.
    It also isn't terribly fast (though a decent compiler will
    strength-reduce the division to a multiplication).*/
  /*TIFF is upside down relative to the libtheora api*/
  if (chroma_format == TH_PF_420) {
    for(y = 0; y < h; y += 2) {
      y1=y+(y+1<h);
      for(x = 0; x < w; x += 2) {
        x1=x+(x+1<w);
        uint8 r0 = TIFFGetR(raster[(h-y)*w + x]);
        uint8 g0 = TIFFGetG(raster[(h-y)*w + x]);
        uint8 b0 = TIFFGetB(raster[(h-y)*w + x]);
        uint8 r1 = TIFFGetR(raster[(h-y)*w + x1]);
        uint8 g1 = TIFFGetG(raster[(h-y)*w + x1]);
        uint8 b1 = TIFFGetB(raster[(h-y)*w + x1]);
        uint8 r2 = TIFFGetR(raster[(h-y1)*w + x]);
        uint8 g2 = TIFFGetG(raster[(h-y1)*w + x]);
        uint8 b2 = TIFFGetB(raster[(h-y1)*w + x]);
        uint8 r3 = TIFFGetR(raster[(h-y1)*w + x1]);
        uint8 g3 = TIFFGetG(raster[(h-y1)*w + x1]);
        uint8 b3 = TIFFGetB(raster[(h-y1)*w + x1]);

        yuv_y[x  + y * yuv_w]  = clamp((65481*r0+128553*g0+24966*b0+4207500)/255000);
        yuv_y[x1 + y * yuv_w]  = clamp((65481*r1+128553*g1+24966*b1+4207500)/255000);
        yuv_y[x  + y1 * yuv_w] = clamp((65481*r2+128553*g2+24966*b2+4207500)/255000);
        yuv_y[x1 + y1 * yuv_w] = clamp((65481*r3+128553*g3+24966*b3+4207500)/255000);

        yuv_u[(x >> 1) + (y >> 1) * ycbcr[1].stride] =
          clamp( ((-33488*r0-65744*g0+99232*b0+29032005)/4 +
                  (-33488*r0-65744*g0+99232*b0+29032005)/4 +
                  (-33488*r2-65744*g2+99232*b2+29032005)/4 +
                  (-33488*r3-65744*g3+99232*b3+29032005)/4)/225930);
        yuv_v[(x >> 1) + (y >> 1) * ycbcr[2].stride] =
          clamp( ((157024*r0-131488*g0-25536*b0+45940035)/4 +
                  (157024*r1-131488*g1-25536*b1+45940035)/4 +
                  (157024*r2-131488*g2-25536*b2+45940035)/4 +
                  (157024*r3-131488*g3-25536*b3+45940035)/4)/357510);
      }
    }
  } else if (chroma_format == TH_PF_444) {
    for(y = 0; y < h; y++) {
      for(x = 0; x < w; x++) {
        uint8 r = TIFFGetR(raster[(h-y)*w + x]);
        uint8 g = TIFFGetG(raster[(h-y)*w + x]);
        uint8 b = TIFFGetB(raster[(h-y)*w + x]);

        yuv_y[x + y * yuv_w] = clamp((65481*r+128553*g+24966*b+4207500)/255000);
        yuv_u[x + y * yuv_w] = clamp((-33488*r-65744*g+99232*b+29032005)/225930);
        yuv_v[x + y * yuv_w] = clamp((157024*r-131488*g-25536*b+45940035)/357510);
      }
    }
  } else {  /* TH_PF_422 */
    for(y = 0; y < h; y += 1) {
      for(x = 0; x < w; x += 2) {
        x1=x+(x+1<w);
        uint8 r0 = TIFFGetR(raster[(h-y)*w + x]);
        uint8 g0 = TIFFGetG(raster[(h-y)*w + x]);
        uint8 b0 = TIFFGetB(raster[(h-y)*w + x]);
        uint8 r1 = TIFFGetR(raster[(h-y)*w + x1]);
        uint8 g1 = TIFFGetG(raster[(h-y)*w + x1]);
        uint8 b1 = TIFFGetB(raster[(h-y)*w + x1]);

        yuv_y[x  + y * yuv_w] = clamp((65481*r0+128553*g0+24966*b0+4207500)/255000);
        yuv_y[x1 + y * yuv_w] = clamp((65481*r1+128553*g1+24966*b1+4207500)/255000);

        yuv_u[(x >> 1) + y * ycbcr[1].stride] =
          clamp( ((-33488*r0-65744*g0+99232*b0+29032005)/2 +
                  (-33488*r1-65744*g1+99232*b1+29032005)/2)/225930);
        yuv_v[(x >> 1) + y * ycbcr[2].stride] =
          clamp( ((157024*r0-131488*g0-25536*b0+45940035)/2 +
                  (157024*r1-131488*g1-25536*b1+45940035)/2)/357510);
      }
    }
  }

}

static int
tiff_read(const char *pathname,
 unsigned int *w, unsigned int *h, th_ycbcr_buffer ycbcr)
{
  TIFF *tiff;
  uint32 width;
  uint32 height;
  size_t pixels;
  uint32 *raster;
  unsigned long yuv_w;
  unsigned long yuv_h;

  tiff = TIFFOpen(pathname, "r");
  if(!tiff) {
    fprintf(stderr, "%s: error: couldn't open as a tiff file.\n",
     pathname);
    return 1;
  }

  TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
  pixels = width*height;
  raster = malloc(pixels*sizeof(uint32));
  if(!raster) {
    fprintf(stderr, "%s: error: couldn't allocate storage for tiff raster.\n",
     pathname);
    TIFFClose(tiff);
    return 1;
  }
  if(!TIFFReadRGBAImage(tiff, width, height, raster, 1)) {
    fprintf(stderr, "%s: error: couldn't read tiff data.\n", pathname);
    free(raster);
    TIFFClose(tiff);
    return 1;
  }

  *w = width;
  *h = height;
  /* Must hold: yuv_w >= w */
  yuv_w = (*w + 15) & ~15;
  /* Must hold: yuv_h >= h */
  yuv_h = (*h + 15) & ~15;

  /* Do we need to allocate a buffer */
  if (!ycbcr[0].data){
    ycbcr[0].width = yuv_w;
    ycbcr[0].height = yuv_h;
    ycbcr[0].stride = yuv_w;
    ycbcr[1].width = (chroma_format == TH_PF_444) ? yuv_w : (yuv_w >> 1);
    ycbcr[1].stride = ycbcr[1].width;
    ycbcr[1].height = (chroma_format == TH_PF_420) ? (yuv_h >> 1) : yuv_h;
    ycbcr[2].width = ycbcr[1].width;
    ycbcr[2].stride = ycbcr[1].stride;
    ycbcr[2].height = ycbcr[1].height;

    ycbcr[0].data = malloc(ycbcr[0].stride * ycbcr[0].height);
    ycbcr[1].data = malloc(ycbcr[1].stride * ycbcr[1].height);
    ycbcr[2].data = malloc(ycbcr[2].stride * ycbcr[2].height);
  } else {
    if ((ycbcr[0].width != yuv_w) || (ycbcr[0].height != yuv_h)){
      fprintf(stderr, "Input size %lux%lu does not match %dx%d\n", yuv_w,yuv_h,ycbcr[0].width,ycbcr[0].height);
      exit(1);
    }
  }

  rgb_to_yuv(raster, ycbcr, *w, *h);

  _TIFFfree(raster);
  TIFFClose(tiff);

  return 0;
}

static int include_files (const struct dirent *de)
{
  char name[1024];
  int number = -1;
  sscanf(de->d_name, input_filter, &number);
  sprintf(name, input_filter, number);
  return !strcmp(name, de->d_name);
}

static int ilog(unsigned _v){
  int ret;
  for(ret=0;_v;ret++)_v>>=1;
  return ret;
}

int
main(int argc, char *argv[])
{
  int c,long_option_index;
  int i, n;
  char *input_mask;
  char *input_directory;
  char *scratch;
  th_comment       tc;
  struct dirent **files;
  int soft_target=0;
  int ret;

  while(1) {

    c=getopt_long(argc,argv,optstring,options,&long_option_index);
    if(c == EOF)
      break;

    switch(c) {
      case 'h':
        usage();
        break;
      case 'o':
        option_output = optarg;
        break;;
      case 'v':
        video_quality=rint(atof(optarg)*6.3);
        if(video_quality<0 || video_quality>63){
          fprintf(stderr,"Illegal video quality (choose 0 through 10)\n");
          exit(1);
        }
        video_rate=0;
        break;
      case 'V':
        video_rate=rint(atof(optarg)*1000);
        if(video_rate<1){
          fprintf(stderr,"Illegal video bitrate (choose > 0 please)\n");
          exit(1);
        }
        video_quality=0;
       break;
    case '\1':
      soft_target=1;
      break;
    case 'c':
      vp3_compatible=1;
      break;
    case 'k':
      keyframe_frequency=rint(atof(optarg));
      if(keyframe_frequency<1 || keyframe_frequency>2147483647){
        fprintf(stderr,"Illegal keyframe frequency\n");
        exit(1);
      }
      break;

    case 'd':
      buf_delay=atoi(optarg);
      if(buf_delay<=0){
        fprintf(stderr,"Illegal buffer delay\n");
        exit(1);
      }
      break;
     case 's':
       video_aspect_numerator=rint(atof(optarg));
       break;
     case 'S':
       video_aspect_denominator=rint(atof(optarg));
       break;
     case 'f':
       video_fps_numerator=rint(atof(optarg));
       break;
     case 'F':
       video_fps_denominator=rint(atof(optarg));
       break;
     case '\5':
       chroma_format=TH_PF_444;
       break;
     case '\6':
       chroma_format=TH_PF_422;
       break;
    case '\2':
      twopass=3; /* perform both passes */
      twopass_file=tmpfile();
      if(!twopass_file){
        fprintf(stderr,"Unable to open temporary file for twopass data\n");
        exit(1);
      }
      break;
    case '\3':
      twopass=1; /* perform first pass */
      twopass_file=fopen(optarg,"wb");
      if(!twopass_file){
        fprintf(stderr,"Unable to open \'%s\' for twopass data\n",optarg);
        exit(1);
      }
      break;
    case '\4':
      twopass=2; /* perform second pass */
      twopass_file=fopen(optarg,"rb");
      if(!twopass_file){
        fprintf(stderr,"Unable to open twopass data file \'%s\'",optarg);
        exit(1);
      }
      break;
     default:
        usage();
        break;
      }
  }

  if(argc < 3) {
    usage();
  }

  if(soft_target){
    if(video_rate<=0){
      fprintf(stderr,"Soft rate target (--soft-target) requested without a bitrate (-V).\n");
      exit(1);
    }
    if(video_quality==-1)
      video_quality=0;
  }else{
    if(video_rate>0)
      video_quality=0;
    if(video_quality==-1)
      video_quality=48;
  }

  if(keyframe_frequency<=0){
    /*Use a default keyframe frequency of 64 for 1-pass (streaming) mode, and
       256 for two-pass mode.*/
    keyframe_frequency=twopass?256:64;
  }

  input_mask = argv[optind];
  if (!input_mask) {
    fprintf(stderr, "no input files specified; run with -h for help.\n");
    exit(1);
  }
  /* dirname and basename must operate on scratch strings */
  scratch = strdup(input_mask);
  input_directory = strdup(dirname(scratch));
  free(scratch);
  scratch = strdup(input_mask);
  input_filter = strdup(basename(scratch));
  free(scratch);

#ifdef DEBUG
  fprintf(stderr, "scanning %s with filter '%s'\n",
  input_directory, input_filter);
#endif
  n = scandir (input_directory, &files, include_files, alphasort);

  if (!n) {
    fprintf(stderr, "no input files found; run with -h for help.\n");
    exit(1);
  }

  ogg_fp = fopen(option_output, "wb");
  if(!ogg_fp) {
    fprintf(stderr, "%s: error: %s\n",
      option_output, "couldn't open output file");
    return 1;
  }

  srand(time(NULL));
  if(ogg_stream_init(&ogg_os, rand())) {
    fprintf(stderr, "%s: error: %s\n",
      option_output, "couldn't create ogg stream state");
    return 1;
  }

  for(passno=(twopass==3?1:twopass);passno<=(twopass==3?2:twopass);passno++){
    unsigned int w;
    unsigned int h;
    char input_path[1024];
    th_ycbcr_buffer ycbcr;

    ycbcr[0].data = 0;
    int last = 0;

    snprintf(input_path, 1023,"%s/%s", input_directory, files[0]->d_name);
    if(tiff_read(input_path, &w, &h, ycbcr)) {
      fprintf(stderr, "could not read %s\n", input_path);
      exit(1);
    }

    if (passno!=2) fprintf(stderr,"%d frames, %dx%d\n",n,w,h);

    /* setup complete.  Raw processing loop */
    switch(passno){
    case 0: case 2:
      fprintf(stderr,"\rCompressing....                                          \n");
      break;
    case 1:
      fprintf(stderr,"\rScanning first pass....                                  \n");
      break;
    }

    fprintf(stderr, "%s\n", input_path);

    th_info_init(&ti);
    ti.frame_width = ((w + 15) >>4)<<4;
    ti.frame_height = ((h + 15)>>4)<<4;
    ti.pic_width = w;
    ti.pic_height = h;
    ti.pic_x = 0;
    ti.pic_y = 0;
    ti.fps_numerator = video_fps_numerator;
    ti.fps_denominator = video_fps_denominator;
    ti.aspect_numerator = video_aspect_numerator;
    ti.aspect_denominator = video_aspect_denominator;
    ti.colorspace = TH_CS_UNSPECIFIED;
    ti.pixel_fmt = chroma_format;
    ti.target_bitrate = video_rate;
    ti.quality = video_quality;
    ti.keyframe_granule_shift=ilog(keyframe_frequency-1);

    td=th_encode_alloc(&ti);
    th_info_clear(&ti);
    /* setting just the granule shift only allows power-of-two keyframe
       spacing.  Set the actual requested spacing. */
    ret=th_encode_ctl(td,TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE,
     &keyframe_frequency,sizeof(keyframe_frequency-1));
    if(ret<0){
      fprintf(stderr,"Could not set keyframe interval to %d.\n",(int)keyframe_frequency);
    }
    if(vp3_compatible){
      ret=th_encode_ctl(td,TH_ENCCTL_SET_VP3_COMPATIBLE,&vp3_compatible,
       sizeof(vp3_compatible));
      if(ret<0||!vp3_compatible){
        fprintf(stderr,"Could not enable strict VP3 compatibility.\n");
        if(ret>=0){
          fprintf(stderr,"Ensure your source format is supported by VP3.\n");
          fprintf(stderr,
           "(4:2:0 pixel format, width and height multiples of 16).\n");
        }
      }
    }
    if(soft_target){
      /* reverse the rate control flags to favor a 'long time' strategy */
      int arg = TH_RATECTL_CAP_UNDERFLOW;
      ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_FLAGS,&arg,sizeof(arg));
      if(ret<0)
        fprintf(stderr,"Could not set encoder flags for --soft-target\n");
      /* Default buffer control is overridden on two-pass */
      if(!twopass&&buf_delay<0){
        if((keyframe_frequency*7>>1) > 5*video_fps_numerator/video_fps_denominator)
          arg=keyframe_frequency*7>>1;
        else
          arg=5*video_fps_numerator/video_fps_denominator;
        ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_BUFFER,&arg,sizeof(arg));
        if(ret<0)
          fprintf(stderr,"Could not set rate control buffer for --soft-target\n");
      }
    }
    /* set up two-pass if needed */
    if(passno==1){
      unsigned char *buffer;
      int bytes;
      bytes=th_encode_ctl(td,TH_ENCCTL_2PASS_OUT,&buffer,sizeof(buffer));
      if(bytes<0){
        fprintf(stderr,"Could not set up the first pass of two-pass mode.\n");
        fprintf(stderr,"Did you remember to specify an estimated bitrate?\n");
        exit(1);
      }
      /*Perform a seek test to ensure we can overwrite this placeholder data at
         the end; this is better than letting the user sit through a whole
         encode only to find out their pass 1 file is useless at the end.*/
      if(fseek(twopass_file,0,SEEK_SET)<0){
        fprintf(stderr,"Unable to seek in two-pass data file.\n");
        exit(1);
      }
      if(fwrite(buffer,1,bytes,twopass_file)<bytes){
        fprintf(stderr,"Unable to write to two-pass data file.\n");
        exit(1);
      }
      fflush(twopass_file);
    }
    if(passno==2){
      /*Enable the second pass here.
        We make this call just to set the encoder into 2-pass mode, because
         by default enabling two-pass sets the buffer delay to the whole file
         (because there's no way to explicitly request that behavior).
        If we waited until we were actually encoding, it would overwite our
         settings.*/
      if(th_encode_ctl(td,TH_ENCCTL_2PASS_IN,NULL,0)<0){
        fprintf(stderr,"Could not set up the second pass of two-pass mode.\n");
        exit(1);
      }
      if(twopass==3){
        if(fseek(twopass_file,0,SEEK_SET)<0){
          fprintf(stderr,"Unable to seek in two-pass data file.\n");
          exit(1);
        }
      }
    }
    /*Now we can set the buffer delay if the user requested a non-default one
       (this has to be done after two-pass is enabled).*/
    if(passno!=1&&buf_delay>=0){
      ret=th_encode_ctl(td,TH_ENCCTL_SET_RATE_BUFFER,
       &buf_delay,sizeof(buf_delay));
      if(ret<0){
        fprintf(stderr,"Warning: could not set desired buffer delay.\n");
      }
    }
    /* write the bitstream header packets with proper page interleave */
    th_comment_init(&tc);
    /* first packet will get its own page automatically */
    if(th_encode_flushheader(td,&tc,&op)<=0){
      fprintf(stderr,"Internal Theora library error.\n");
      exit(1);
    }
    th_comment_clear(&tc);
    if(passno!=1){
      ogg_stream_packetin(&ogg_os,&op);
      if(ogg_stream_pageout(&ogg_os,&og)!=1){
        fprintf(stderr,"Internal Ogg library error.\n");
        exit(1);
      }
      fwrite(og.header,1,og.header_len,ogg_fp);
      fwrite(og.body,1,og.body_len,ogg_fp);
    }
    /* create the remaining theora headers */
    for(;;){
      ret=th_encode_flushheader(td,&tc,&op);
      if(ret<0){
        fprintf(stderr,"Internal Theora library error.\n");
        exit(1);
      }
      else if(!ret)break;
      if(passno!=1)ogg_stream_packetin(&ogg_os,&op);
    }
    /* Flush the rest of our headers. This ensures
       the actual data in each stream will start
       on a new page, as per spec. */
    if(passno!=1){
      for(;;){
        int result = ogg_stream_flush(&ogg_os,&og);
        if(result<0){
          /* can't get here */
          fprintf(stderr,"Internal Ogg library error.\n");
          exit(1);
        }
        if(result==0)break;
        fwrite(og.header,1,og.header_len,ogg_fp);
        fwrite(og.body,1,og.body_len,ogg_fp);
      }
    }

    i=0; last=0;
    do {
      if(i >= n-1) last = 1;
      if(theora_write_frame(ycbcr, last)) {
          fprintf(stderr,"Encoding error.\n");
        exit(1);
      }

      i++;
      if (!last) {
        snprintf(input_path, 1023,"%s/%s", input_directory, files[i]->d_name);
        if(tiff_read(input_path, &w, &h, ycbcr)) {
          fprintf(stderr, "could not read %s\n", input_path);
          exit(1);
        }
       fprintf(stderr, "%s\n", input_path);
      }
    } while (!last);

    if(passno==1){
      /* need to read the final (summary) packet */
      unsigned char *buffer;
      int bytes = th_encode_ctl(td, TH_ENCCTL_2PASS_OUT, &buffer, sizeof(buffer));
      if(bytes<0){
        fprintf(stderr,"Could not read two-pass summary data from encoder.\n");
        exit(1);
      }
      if(fseek(twopass_file,0,SEEK_SET)<0){
        fprintf(stderr,"Unable to seek in two-pass data file.\n");
        exit(1);
      }
      if(fwrite(buffer,1,bytes,twopass_file)<bytes){
        fprintf(stderr,"Unable to write to two-pass data file.\n");
        exit(1);
      }
      fflush(twopass_file);
    }
    th_encode_free(td);
    free(ycbcr[0].data);
    free(ycbcr[1].data);
    free(ycbcr[2].data);
  }

  if(ogg_stream_flush(&ogg_os, &og)) {
    fwrite(og.header, og.header_len, 1, ogg_fp);
    fwrite(og.body, og.body_len, 1, ogg_fp);
  }

  free(input_directory);
  free(input_filter);

  while (n--) free(files[n]);
  free(files);

  if(ogg_fp){
    fflush(ogg_fp);
    if(ogg_fp!=stdout)fclose(ogg_fp);
  }

  ogg_stream_clear(&ogg_os);
  if(twopass_file)fclose(twopass_file);
  fprintf(stderr,"\r   \ndone.\n\n");

  return 0;
}
