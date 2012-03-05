/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Daemon GPL Source Code (Daemon Source Code).  

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the Daemon 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		tr_image.c
 *
 * desc:
 *
*/

#include "tr_local.h"

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#define JPEG_INTERNALS
#include "../../libs/jpeg/jpeglib.h"
#include <png.h>


static void     LoadBMP(const char *name, byte ** pic, int *width, int *height);
static void     LoadTGA(const char *name, byte ** pic, int *width, int *height);
static void     LoadJPG(const char *name, byte ** pic, int *width, int *height);
static void     LoadPNG(const char *name, byte ** pic, int *width, int *height, byte alphaByte);
static void 	LoadDDS(const char *name, byte ** pic, int *width, int *height);

static byte     s_intensitytable[256];
static unsigned char s_gammatable[256];

int             gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int             gl_filter_max = GL_LINEAR;

float           gl_anisotropy = 1.0;

image_t *r_imageHashTable[IMAGE_FILE_HASH_SIZE];

// Ridah, in order to prevent zone fragmentation, all images will
// be read into this buffer. In order to keep things as fast as possible,
// we'll give it a starting value, which will account for the majority of
// images, but allow it to grow if the buffer isn't big enough
#define R_IMAGE_BUFFER_SIZE     ( 512 * 512 * 4 )	// 512 x 512 x 32bit

typedef enum
{
	BUFFER_IMAGE,
	BUFFER_SCALED,
	BUFFER_RESAMPLED,
	BUFFER_MAX_TYPES
} bufferMemType_t;

int             imageBufferSize[BUFFER_MAX_TYPES] = { 0, 0, 0 };
void           *imageBufferPtr[BUFFER_MAX_TYPES] = { NULL, NULL, NULL };

void           *R_GetImageBuffer(int size, bufferMemType_t bufferType)
{
	if(imageBufferSize[bufferType] < R_IMAGE_BUFFER_SIZE && size <= imageBufferSize[bufferType])
	{
		imageBufferSize[bufferType] = R_IMAGE_BUFFER_SIZE;
		imageBufferPtr[bufferType] = malloc(imageBufferSize[bufferType]);
//DAJ TEST      imageBufferPtr[bufferType] = Z_Malloc( imageBufferSize[bufferType] );
	}
	if(size > imageBufferSize[bufferType])
	{							// it needs to grow
		if(imageBufferPtr[bufferType])
		{
			free(imageBufferPtr[bufferType]);
		}
//DAJ TEST      Z_Free( imageBufferPtr[bufferType] );
		imageBufferSize[bufferType] = size;
		imageBufferPtr[bufferType] = malloc(imageBufferSize[bufferType]);
//DAJ TEST      imageBufferPtr[bufferType] = Z_Malloc( imageBufferSize[bufferType] );
	}

	return imageBufferPtr[bufferType];
}

void R_FreeImageBuffer(void)
{
	int             bufferType;

	for(bufferType = 0; bufferType < BUFFER_MAX_TYPES; bufferType++)
	{
		if(!imageBufferPtr[bufferType])
		{
			return;
		}
		free(imageBufferPtr[bufferType]);
//DAJ TEST      Z_Free( imageBufferPtr[bufferType] );
		imageBufferSize[bufferType] = 0;
		imageBufferPtr[bufferType] = NULL;
	}
}

/*
** R_GammaCorrect
*/
void R_GammaCorrect(byte * buffer, int bufSize)
{
	int             i;

	for(i = 0; i < bufSize; i++)
	{
		buffer[i] = s_gammatable[buffer[i]];
	}
}

typedef struct
{
	char           *name;
	int             minimize, maximize;
} textureMode_t;

textureMode_t   modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
================
return a hash value for the filename
================
*/
long GenerateImageHashValue(const char *fname)
{
	int             i;
	long            hash;
	char            letter;

	hash = 0;
	i = 0;
	while(fname[i] != '\0')
	{
		letter = tolower(fname[i]);
		if(letter == '.')
		{
			break;				// don't include extension
		}
		if(letter == '\\')
		{
			letter = '/';		// damn path names
		}
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash &= (IMAGE_FILE_HASH_SIZE - 1);
	return hash;
}

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode(const char *string)
{
	int             i;
	image_t        *glt;

	for(i = 0; i < 6; i++)
	{
		if(!Q_stricmp(modes[i].name, string))
		{
			break;
		}
	}

	// hack to prevent trilinear from being set on voodoo,
	// because their driver freaks...
	if(i == 5 && glConfig.hardwareType == GLHW_3DFX_2D3D)
	{
		ri.Printf(PRINT_ALL, "Refusing to set trilinear on a voodoo.\n");
		i = 3;
	}


	if(i == 6)
	{
		ri.Printf(PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for(i = 0; i < tr.numImages; i++)
	{
		glt = tr.images[i];
		GL_Bind(glt);
		// ydnar: for allowing lightmap debugging
		if(glt->mipmap)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
		else
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAnisotropy
===============
*/
void GL_TextureAnisotropy(float anisotropy)
{
	int             i;
	image_t        *glt;

	if(r_ext_texture_filter_anisotropic->integer == 1)
	{
		if(anisotropy < 1.0 || anisotropy > glConfig.maxAnisotropy)
		{
			ri.Printf(PRINT_ALL, "anisotropy out of range\n");
			return;
		}
	}

	gl_anisotropy = anisotropy;

	if(!glConfig.anisotropicAvailable)
	{
		return;
	}

	// change all the existing texture objects
	for(i = 0; i < tr.numImages; i++)
	{
		glt = tr.images[i];
		GL_Bind(glt);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropy);
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages(void)
{
	int             total;
	int             i, fc = (tr.frameCount - 1);

	total = 0;
	for(i = 0; i < tr.numImages; i++)
	{
		if(tr.images[i]->frameUsed == fc)
		{
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f(void)
{
	int             i;
	image_t        *image;
	int             texels;
	const char     *yesno[] = {
		"no ", "yes"
	};

	ri.Printf(PRINT_ALL, "\n      -w-- -h-- -mm- -TMU- GLname -if-- wrap --name-------\n");
	texels = 0;

	for(i = 0; i < tr.numImages; i++)
	{
		image = tr.images[i];

		texels += image->uploadWidth * image->uploadHeight;
		ri.Printf(PRINT_ALL, "%4i: %4i %4i  %s   %d   %5d ",
				  i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap], image->TMU, image->texnum);
		switch (image->internalFormat)
		{
			case 1:
				ri.Printf(PRINT_ALL, "I    ");
				break;
			case 2:
				ri.Printf(PRINT_ALL, "IA   ");
				break;
			case 3:
				ri.Printf(PRINT_ALL, "RGB  ");
				break;
			case 4:
				ri.Printf(PRINT_ALL, "RGBA ");
				break;
#ifndef IPHONE				
			case GL_RGBA8:
				ri.Printf(PRINT_ALL, "RGBA8");
				break;
			case GL_RGB8:
				ri.Printf(PRINT_ALL, "RGB8 ");
				break;
#endif // !IPHONE				
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				ri.Printf(PRINT_ALL, "DXT3 ");
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				ri.Printf(PRINT_ALL, "DXT5 ");
				break;
			case GL_RGB4_S3TC:
				ri.Printf(PRINT_ALL, "S3TC4");
				break;
#ifndef IPHONE				
			case GL_RGBA4:
				ri.Printf(PRINT_ALL, "RGBA4");
				break;
			case GL_RGB5:
				ri.Printf(PRINT_ALL, "RGB5 ");
				break;
#endif // !IPHONE				
			default:
				ri.Printf(PRINT_ALL, "???? ");
		}

		switch (image->wrapClampMode)
		{
			case GL_REPEAT:
				ri.Printf(PRINT_ALL, "rept ");
				break;
			case GL_CLAMP:
				ri.Printf(PRINT_ALL, "clmp ");
				break;
			default:
				ri.Printf(PRINT_ALL, "%4i ", image->wrapClampMode);
				break;
		}

		ri.Printf(PRINT_ALL, " %s\n", image->imgName);
	}
	ri.Printf(PRINT_ALL, " ---------\n");
	ri.Printf(PRINT_ALL, " %i total texels (not including mipmaps)\n", texels);
	ri.Printf(PRINT_ALL, " %i total images\n\n", tr.numImages);
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
before or after.
================
*/
static void ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight)
{
	int             i, j;
	unsigned       *inrow, *inrow2;
	unsigned        frac, fracstep;
	unsigned        p1[2048], p2[2048];
	byte           *pix1, *pix2, *pix3, *pix4;

	if(outwidth > 2048)
	{
		ri.Error(ERR_DROP, "ResampleTexture: max width");
	}

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep >> 2;
	for(i = 0; i < outwidth; i++)
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}
	frac = 3 * (fracstep >> 2);
	for(i = 0; i < outwidth; i++)
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	for(i = 0; i < outheight; i++, out += outwidth)
	{
		inrow = in + inwidth * (int)((i + 0.25) * inheight / outheight);
		inrow2 = in + inwidth * (int)((i + 0.75) * inheight / outheight);
		frac = fracstep >> 1;
		for(j = 0; j < outwidth; j++)
		{
			pix1 = (byte *) inrow + p1[j];
			pix2 = (byte *) inrow + p2[j];
			pix3 = (byte *) inrow2 + p1[j];
			pix4 = (byte *) inrow2 + p2[j];
			((byte *) (out + j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
			((byte *) (out + j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
			((byte *) (out + j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
			((byte *) (out + j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
		}
	}
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture(unsigned *in, int inwidth, int inheight, qboolean only_gamma)
{
	if(only_gamma)
	{
		if(!glConfig.deviceSupportsGamma)
		{
			int             i, c;
			byte           *p;

			p = (byte *) in;

			c = inwidth * inheight;
			for(i = 0; i < c; i++, p += 4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int             i, c;
		byte           *p;

		p = (byte *) in;

		c = inwidth * inheight;

		if(glConfig.deviceSupportsGamma)
		{
			// raynorpat: small optimization
			if(r_intensity->value != 1.0f)
			{
				for(i = 0; i < c; i++, p += 4)
				{
					p[0] = s_intensitytable[p[0]];
					p[1] = s_intensitytable[p[1]];
					p[2] = s_intensitytable[p[2]];
				}
			}
		}
		else
		{
			for(i = 0; i < c; i++, p += 4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}


/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2(unsigned *in, int inWidth, int inHeight)
{
	int             i, j, k;
	byte           *outpix;
	int             inWidthMask, inHeightMask;
	int             total;
	int             outWidth, outHeight;
	unsigned       *temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = ri.Hunk_AllocateTempMemory(outWidth * outHeight * 4);

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for(i = 0; i < outHeight; i++)
	{
		for(j = 0; j < outWidth; j++)
		{
			outpix = (byte *) (temp + i * outWidth + j);
			for(k = 0; k < 4; k++)
			{
				total =
					1 * ((byte *) & in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
					2 * ((byte *) & in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
					2 * ((byte *) & in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
					1 * ((byte *) & in[((i * 2 - 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +
					2 * ((byte *) & in[((i * 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
					4 * ((byte *) & in[((i * 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
					4 * ((byte *) & in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
					2 * ((byte *) & in[((i * 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +
					2 * ((byte *) & in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
					4 * ((byte *) & in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
					4 * ((byte *) & in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
					2 * ((byte *) & in[((i * 2 + 1) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k] +
					1 * ((byte *) & in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 - 1) & inWidthMask)])[k] +
					2 * ((byte *) & in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2) & inWidthMask)])[k] +
					2 * ((byte *) & in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 1) & inWidthMask)])[k] +
					1 * ((byte *) & in[((i * 2 + 2) & inHeightMask) * inWidth + ((j * 2 + 2) & inWidthMask)])[k];
				outpix[k] = total / 36;
			}
		}
	}

	memcpy(in, temp, outWidth * outHeight * 4);
	ri.Hunk_FreeTempMemory(temp);
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap(byte * in, int width, int height)
{
	int             i, j;
	byte           *out;
	int             row;

	if(!r_simpleMipMaps->integer)
	{
		R_MipMap2((unsigned *)in, width, height);
		return;
	}

	if(width == 1 && height == 1)
	{
		return;
	}

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if(width == 0 || height == 0)
	{
		width += height;		// get largest
		for(i = 0; i < width; i++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4]) >> 1;
			out[1] = (in[1] + in[5]) >> 1;
			out[2] = (in[2] + in[6]) >> 1;
			out[3] = (in[3] + in[7]) >> 1;
		}
		return;
	}

	for(i = 0; i < height; i++, in += row)
	{
		for(j = 0; j < width; j++, out += 4, in += 8)
		{
			out[0] = (in[0] + in[4] + in[row + 0] + in[row + 4]) >> 2;
			out[1] = (in[1] + in[5] + in[row + 1] + in[row + 5]) >> 2;
			out[2] = (in[2] + in[6] + in[row + 2] + in[row + 6]) >> 2;
			out[3] = (in[3] + in[7] + in[row + 3] + in[row + 7]) >> 2;
		}
	}
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
#if 0							// rain - unused
static float R_RMSE(byte * in, int width, int height)
{
	int             i, j;
	float           out, rmse, rtemp;
	int             row;

	rmse = 0.0f;

	if(width <= 32 || height <= 32)
	{
		return 9999.0f;
	}

	row = width * 4;

	width >>= 1;
	height >>= 1;

	for(i = 0; i < height; i++, in += row)
	{
		for(j = 0; j < width; j++, out += 4, in += 8)
		{
			out = (in[0] + in[4] + in[row + 0] + in[row + 4]) >> 2;
			rtemp = ((Q_fabs(out - in[0]) + Q_fabs(out - in[4]) + Q_fabs(out - in[row + 0]) + Q_fabs(out - in[row + 4])));
			rtemp = rtemp * rtemp;
			rmse += rtemp;
			out = (in[1] + in[5] + in[row + 1] + in[row + 5]) >> 2;
			rtemp = ((Q_fabs(out - in[1]) + Q_fabs(out - in[5]) + Q_fabs(out - in[row + 1]) + Q_fabs(out - in[row + 5])));
			rtemp = rtemp * rtemp;
			rmse += rtemp;
			out = (in[2] + in[6] + in[row + 2] + in[row + 6]) >> 2;
			rtemp = ((Q_fabs(out - in[2]) + Q_fabs(out - in[6]) + Q_fabs(out - in[row + 2]) + Q_fabs(out - in[row + 6])));
			rtemp = rtemp * rtemp;
			rmse += rtemp;
			out = (in[3] + in[7] + in[row + 3] + in[row + 7]) >> 2;
			rtemp = ((Q_fabs(out - in[3]) + Q_fabs(out - in[7]) + Q_fabs(out - in[row + 3]) + Q_fabs(out - in[row + 7])));
			rtemp = rtemp * rtemp;
			rmse += rtemp;
		}
	}
	rmse = sqrt(rmse / (height * width * 4));
	return rmse;
}
#endif

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture(byte * data, int pixelCount, byte blend[4])
{
	int             i;
	int             inverseAlpha;
	int             premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for(i = 0; i < pixelCount; i++, data += 4)
	{
		data[0] = (data[0] * inverseAlpha + premult[0]) >> 9;
		data[1] = (data[1] * inverseAlpha + premult[1]) >> 9;
		data[2] = (data[2] * inverseAlpha + premult[2]) >> 9;
	}
}

byte            mipBlendColors[16][4] = {
	{0, 0, 0, 0}
	,
	{255, 0, 0, 128}
	,
	{0, 255, 0, 128}
	,
	{0, 0, 255, 128}
	,
	{255, 0, 0, 128}
	,
	{0, 255, 0, 128}
	,
	{0, 0, 255, 128}
	,
	{255, 0, 0, 128}
	,
	{0, 255, 0, 128}
	,
	{0, 0, 255, 128}
	,
	{255, 0, 0, 128}
	,
	{0, 255, 0, 128}
	,
	{0, 0, 255, 128}
	,
	{255, 0, 0, 128}
	,
	{0, 255, 0, 128}
	,
	{0, 0, 255, 128}
	,
};


/*
===============
Upload32

===============
*/
static void Upload32(unsigned *data,
					 int width, int height,
					 qboolean mipmap,
					 qboolean picmip, qboolean lightMap, int *format, int *pUploadWidth, int *pUploadHeight, qboolean noCompress)
{
	int             samples;
	int             scaled_width, scaled_height;
	unsigned       *scaledBuffer = NULL;
	unsigned       *resampledBuffer = NULL;
	int             i, c;
	byte           *scan;
	GLenum          internalFormat = GL_RGB;

// rain - unused
//  static      int rmse_saved = 0;

	// do the root mean square error stuff first
/*	if (r_rmse->value) {
		while (R_RMSE((byte *)data, width, height) < r_rmse->value) {
			rmse_saved += (height*width*4)-((width>>1)*(height>>1)*4);
			resampledBuffer = R_GetImageBuffer( (width>>1) * (height>>1) * 4, BUFFER_RESAMPLED );
			ResampleTexture (data, width, height, resampledBuffer, width>>1, height>>1);
			data = resampledBuffer;
			width = width>>1;
			height = height>>1;
			ri.Printf (PRINT_ALL, "r_rmse of %f has saved %dkb\n", r_rmse->value, (rmse_saved/1024));
		}
	}*/
	//
	// convert to exact power of 2 sizes
	//
	for(scaled_width = 1; scaled_width < width; scaled_width <<= 1)
		;
	for(scaled_height = 1; scaled_height < height; scaled_height <<= 1)
		;
	if(r_roundImagesDown->integer && scaled_width > width)
	{
		scaled_width >>= 1;
	}
	if(r_roundImagesDown->integer && scaled_height > height)
	{
		scaled_height >>= 1;
	}

	if(scaled_width != width || scaled_height != height)
	{
		//resampledBuffer = ri.Hunk_AllocateTempMemory( scaled_width * scaled_height * 4 );
		resampledBuffer = R_GetImageBuffer(scaled_width * scaled_height * 4, BUFFER_RESAMPLED);
		ResampleTexture(data, width, height, resampledBuffer, scaled_width, scaled_height);
		data = resampledBuffer;
		width = scaled_width;
		height = scaled_height;
	}

	//
	// perform optional picmip operation
	//
	if(picmip)
	{
		scaled_width >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
	}

	//
	// clamp to minimum size
	//
	if(scaled_width < 1)
	{
		scaled_width = 1;
	}
	if(scaled_height < 1)
	{
		scaled_height = 1;
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while(scaled_width > glConfig.maxTextureSize || scaled_height > glConfig.maxTextureSize)
	{
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	//scaledBuffer = ri.Hunk_AllocateTempMemory( sizeof( unsigned ) * scaled_width * scaled_height );
	scaledBuffer = R_GetImageBuffer(sizeof(unsigned) * scaled_width * scaled_height, BUFFER_SCALED);

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width * height;
	scan = ((byte *) data);
	samples = 3;
	if(!lightMap)
	{
		for(i = 0; i < c; i++)
		{
			if(scan[i * 4 + 3] != 255)
			{
				samples = 4;
				break;
			}
		}
		// select proper internal format
		if(samples == 3)
		{
#ifdef IPHONE		
			internalFormat = GL_RGBA;
#else		
			if(!noCompress && glConfig.textureCompression == TC_EXT_COMP_S3TC)
			{
				// TODO: which format is best for which textures?
				//internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
			else if(!noCompress && glConfig.textureCompression == TC_S3TC)
			{
				internalFormat = GL_RGB4_S3TC;
			}
			else if(r_texturebits->integer == 16)
			{
				internalFormat = GL_RGB5;
			}
			else if(r_texturebits->integer == 32)
			{
				internalFormat = GL_RGB8;
			}
			else
			{
				internalFormat = 3;
			}
#endif // IPHONE			
		}
		else if(samples == 4)
		{
#ifdef IPHONE
			internalFormat = GL_RGBA;
#else		
			if(!noCompress && glConfig.textureCompression == TC_EXT_COMP_S3TC)
			{
				// TODO: which format is best for which textures?
				//internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
			else if(r_texturebits->integer == 16)
			{
				internalFormat = GL_RGBA4;
			}
			else if(r_texturebits->integer == 32)
			{
				internalFormat = GL_RGBA8;
			}
			else
			{
				internalFormat = 4;
			}
#endif // IPHONE			
		}
	}
	else
	{
#ifdef IPHONE
		internalFormat = GL_RGBA;
#else	
		internalFormat = 3;
#endif // IPHONE		
	}
	// copy or resample data as appropriate for first MIP level
	if((scaled_width == width) && (scaled_height == height))
	{
		if(!mipmap)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}
		memcpy(scaledBuffer, data, width * height * 4);
	}
	else
	{
		// use the normal mip-mapping function to go down from here
		while(width > scaled_width || height > scaled_height)
		{
			R_MipMap((byte *) data, width, height);
			width >>= 1;
			height >>= 1;
			if(width < 1)
			{
				width = 1;
			}
			if(height < 1)
			{
				height = 1;
			}
		}
		memcpy(scaledBuffer, data, width * height * 4);
	}

	R_LightScaleTexture(scaledBuffer, scaled_width, scaled_height, !mipmap);

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);

	if(mipmap)
	{
		int             miplevel;

		miplevel = 0;
		while(scaled_width > 1 || scaled_height > 1)
		{
			R_MipMap((byte *) scaledBuffer, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if(scaled_width < 1)
			{
				scaled_width = 1;
			}
			if(scaled_height < 1)
			{
				scaled_height = 1;
			}
			miplevel++;

			if(r_colorMipLevels->integer)
			{
				R_BlendOverTexture((byte *) scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel]);
			}

			glTexImage2D(GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
						  scaledBuffer);
		}
	}
  done:

	if(mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		// ydnar: for allowing lightmap debugging
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	if(glConfig.anisotropicAvailable)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_anisotropy);
	}

	GL_CheckErrors();

	//if ( scaledBuffer != 0 )
	//  ri.Hunk_FreeTempMemory( scaledBuffer );
	//if ( resampledBuffer != 0 )
	//  ri.Hunk_FreeTempMemory( resampledBuffer );
}

/*
================
R_CreateImage

This is the only way any image_t are created
================
*/
image_t        *R_CreateImage(const char *name, const byte * pic, int width, int height,
							  qboolean mipmap, qboolean allowPicmip, int glWrapClampMode)
{
	image_t        *image;
	qboolean        isLightmap = qfalse;
	long            hash;
	qboolean        noCompress = qfalse;

	if(strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "R_CreateImage: \"%s\" is too long\n", name);
	}
	if(!strncmp(name, "*lightmap", 9))
	{
		isLightmap = qtrue;
		noCompress = qtrue;
	}
	if(!noCompress && strstr(name, "skies"))
	{
		noCompress = qtrue;
	}
	if(!noCompress && strstr(name, "weapons"))
	{							// don't compress view weapon skins
		noCompress = qtrue;
	}
	// RF, if the shader hasn't specifically asked for it, don't allow compression
	if(r_ext_compressed_textures->integer == 2 && (tr.allowCompress != qtrue))
	{
		noCompress = qtrue;
	}
	else if(r_ext_compressed_textures->integer == 1 && (tr.allowCompress < 0))
	{
		noCompress = qtrue;
	}
#if __MACOS__
	// LBO 2/8/05. Work around apparent bug in OSX. Some mipmap textures draw incorrectly when
	// texture compression is enabled. Examples include brick edging on fueldump level appearing
	// bluish-green from a distance.
	else if(mipmap)
	{
		noCompress = qtrue;
	}
#endif
	// ydnar: don't compress textures smaller or equal to 128x128 pixels
	else if((width * height) <= (128 * 128))
	{
		noCompress = qtrue;
	}

	if(tr.numImages == MAX_DRAWIMAGES)
	{
		ri.Error(ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n");
	}

	// Ridah
	image = tr.images[tr.numImages] = R_CacheImageAlloc(sizeof(image_t));

	// ydnar: not exactly sure why this mechanism is here at all, but it's generating
	// bad texture names (not that the rest of the code is a saint, but hey...)
	//% image->texnum = 1024 + tr.numImages;

	// Ridah
	//% if (r_cacheShaders->integer) {
	//%     R_FindFreeTexnum(image);
	//% }
	// done.

	// ydnar: ok, let's try the recommended way
	glGenTextures(1, &image->texnum);

	tr.numImages++;

	image->mipmap = mipmap;
	image->allowPicmip = allowPicmip;

	strcpy(image->imgName, name);

	image->width = width;
	image->height = height;
	image->wrapClampMode = glWrapClampMode;

	// lightmaps are always allocated on TMU 1
	if(glActiveTextureARB && isLightmap)
	{
		image->TMU = 1;
	}
	else
	{
		image->TMU = 0;
	}

	if(glActiveTextureARB)
	{
		GL_SelectTexture(image->TMU);
	}

	GL_Bind(image);

	Upload32((unsigned *)pic,
			 image->width, image->height,
			 image->mipmap,
			 allowPicmip, isLightmap, &image->internalFormat, &image->uploadWidth, &image->uploadHeight, noCompress);

	// ydnar: opengl 1.2 GL_CLAMP_TO_EDGE SUPPORT
	// only 1.1 headers, joy
#define GL_CLAMP_TO_EDGE    0x812F
	if(r_clampToEdge->integer && glWrapClampMode == GL_CLAMP)
	{
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	}

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapClampMode);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapClampMode);

	glBindTexture(GL_TEXTURE_2D, 0);

	if(image->TMU == 1)
	{
		GL_SelectTexture(0);
	}

	hash = GenerateImageHashValue(name);
	image->next = r_imageHashTable[hash];
	r_imageHashTable[hash] = image;

	// Ridah
	image->hash = hash;

	return image;
}

/*
=========================================================

BMP LOADING

=========================================================
*/
typedef struct
{
	char            id[2];
	unsigned long   fileSize;
	unsigned long   reserved0;
	unsigned long   bitmapDataOffset;
	unsigned long   bitmapHeaderSize;
	unsigned long   width;
	unsigned long   height;
	unsigned short  planes;
	unsigned short  bitsPerPixel;
	unsigned long   compression;
	unsigned long   bitmapDataSize;
	unsigned long   hRes;
	unsigned long   vRes;
	unsigned long   colors;
	unsigned long   importantColors;
	unsigned char   palette[256][4];
} BMPHeader_t;

static void LoadBMP(const char *name, byte ** pic, int *width, int *height)
{
	int             columns, rows, numPixels;
	byte           *pixbuf;
	int             row, column;
	byte           *buf_p;
	byte           *buffer;
	int             length;
	BMPHeader_t     bmpHeader;
	byte           *bmpRGBA;

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_ReadFile((char *)name, (void **)&buffer);
	if(!buffer)
	{
		return;
	}

	buf_p = buffer;

	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.reserved0 = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.bitmapDataOffset = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.width = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.height = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.planes = LittleShort(*(short *)buf_p);
	buf_p += 2;
	bmpHeader.bitsPerPixel = LittleShort(*(short *)buf_p);
	buf_p += 2;
	bmpHeader.compression = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.bitmapDataSize = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.hRes = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.vRes = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.colors = LittleLong(*(long *)buf_p);
	buf_p += 4;
	bmpHeader.importantColors = LittleLong(*(long *)buf_p);
	buf_p += 4;

	memcpy(bmpHeader.palette, buf_p, sizeof(bmpHeader.palette));

	if(bmpHeader.bitsPerPixel == 8)
	{
		buf_p += 1024;
	}

	if(bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M')
	{
		ri.Error(ERR_DROP, "LoadBMP: only Windows-style BMP files supported (%s)\n", name);
	}
	if(bmpHeader.fileSize != length)
	{
		ri.Error(ERR_DROP, "LoadBMP: header size does not match file size (%lu vs. %d) (%s)\n", bmpHeader.fileSize, length, name);
	}
	if(bmpHeader.compression != 0)
	{
		ri.Error(ERR_DROP, "LoadBMP: only uncompressed BMP files supported (%s)\n", name);
	}
	if(bmpHeader.bitsPerPixel < 8)
	{
		ri.Error(ERR_DROP, "LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name);
	}

	columns = bmpHeader.width;
	rows = bmpHeader.height;
	if(rows < 0)
	{
		rows = -rows;
	}
	numPixels = columns * rows;

	if(width)
	{
		*width = columns;
	}
	if(height)
	{
		*height = rows;
	}

	bmpRGBA = R_GetImageBuffer(numPixels * 4, BUFFER_IMAGE);

	*pic = bmpRGBA;


	for(row = rows - 1; row >= 0; row--)
	{
		pixbuf = bmpRGBA + row * columns * 4;

		for(column = 0; column < columns; column++)
		{
			unsigned char   red, green, blue, alpha;
			int             palIndex;
			unsigned short  shortPixel;

			switch (bmpHeader.bitsPerPixel)
			{
				case 8:
					palIndex = *buf_p++;
					*pixbuf++ = bmpHeader.palette[palIndex][2];
					*pixbuf++ = bmpHeader.palette[palIndex][1];
					*pixbuf++ = bmpHeader.palette[palIndex][0];
					*pixbuf++ = 0xff;
					break;
				case 16:
					shortPixel = *(unsigned short *)pixbuf;
					pixbuf += 2;
					*pixbuf++ = (shortPixel & (31 << 10)) >> 7;
					*pixbuf++ = (shortPixel & (31 << 5)) >> 2;
					*pixbuf++ = (shortPixel & (31)) << 3;
					*pixbuf++ = 0xff;
					break;

				case 24:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alpha = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alpha;
					break;
				default:
					ri.Error(ERR_DROP, "LoadBMP: illegal pixel_size '%d' in file '%s'\n", bmpHeader.bitsPerPixel, name);
					break;
			}
		}
	}

	ri.FS_FreeFile(buffer);

}


/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
#define DECODEPCX( b, d, r ) d = *b++; if ( ( d & 0xC0 ) == 0xC0 ) {r = d & 0x3F; d = *b++;} else {r = 1;}

static void LoadPCX(const char *filename, byte ** pic, byte ** palette, int *width, int *height)
{
	byte           *raw;
	pcx_t          *pcx;
	int             x, y, lsize;
	int             len;
	int             dataByte, runLength;
	byte           *out, *pix;
	int             xmax, ymax;

	*pic = NULL;
	*palette = NULL;
	runLength = 0;

	//
	// load the file
	//
	len = ri.FS_ReadFile((char *)filename, (void **)&raw);
	if(!raw)
	{
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *) raw;
	raw = &pcx->data;

	xmax = LittleShort(pcx->xmax);
	ymax = LittleShort(pcx->ymax);

	if(pcx->manufacturer != 0x0a
	   || pcx->version != 5 || pcx->encoding != 1 || pcx->bits_per_pixel != 8 || xmax >= 1024 || ymax >= 1024)
	{
		ri.Printf(PRINT_ALL, "Bad pcx file %s (%i x %i) (%i x %i)\n", filename, xmax + 1, ymax + 1, pcx->xmax, pcx->ymax);
		return;
	}

	out = R_GetImageBuffer((ymax + 1) * (xmax + 1), BUFFER_IMAGE);

	*pic = out;

	pix = out;

	if(palette)
	{
		*palette = ri.Z_Malloc(768);
		memcpy(*palette, (byte *) pcx + len - 768, 768);
	}

	if(width)
	{
		*width = xmax + 1;
	}
	if(height)
	{
		*height = ymax + 1;
	}
// FIXME: use bytes_per_line here?

	// Arnout: this doesn't work for all pcx files
	/*for (y=0 ; y<=ymax ; y++, pix += xmax+1)
	   {
	   for (x=0 ; x<=xmax ; )
	   {
	   dataByte = *raw++;

	   if((dataByte & 0xC0) == 0xC0)
	   {
	   runLength = dataByte & 0x3F;
	   dataByte = *raw++;
	   }
	   else
	   runLength = 1;

	   while(runLength-- > 0)
	   pix[x++] = dataByte;
	   }

	   } */

	lsize = pcx->color_planes * pcx->bytes_per_line;

	// go scanline by scanline
	for(y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1)
	{
		// do a scanline
		for(x = 0; x <= pcx->xmax;)
		{
			DECODEPCX(raw, dataByte, runLength);
			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

		// discard any other data
		while(x < lsize)
		{
			DECODEPCX(raw, dataByte, runLength);
			x++;
		}
		while(runLength-- > 0)
			x++;
	}

	if(raw - (byte *) pcx > len)
	{
		ri.Printf(PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		ri.Free(*pic);
		*pic = NULL;
	}

	ri.FS_FreeFile(pcx);
}


/*
==============
LoadPCX32
==============
*/
static void LoadPCX32(const char *filename, byte ** pic, int *width, int *height)
{
	byte           *palette;
	byte           *pic8;
	int             i, c, p;
	byte           *pic32;

	LoadPCX(filename, &pic8, &palette, width, height);
	if(!pic8)
	{
		*pic = NULL;
		return;
	}

	c = (*width) * (*height);
	pic32 = *pic = R_GetImageBuffer(4 * c, BUFFER_IMAGE);
	for(i = 0; i < c; i++)
	{
		p = pic8[i];
		pic32[0] = palette[p * 3];
		pic32[1] = palette[p * 3 + 1];
		pic32[2] = palette[p * 3 + 2];
		pic32[3] = 255;
		pic32 += 4;
	}

	ri.Free(pic8);
	ri.Free(palette);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

/*
=============
LoadTGA
=============
*/
void LoadTGA(const char *name, byte ** pic, int *width, int *height)
{
	int             columns, rows, numPixels;
	byte           *pixbuf;
	int             row, column;
	byte           *buf_p;
	byte           *buffer;
	TargaHeader     targa_header;
	byte           *targa_rgba;

	*pic = NULL;

	//
	// load the file
	//
	ri.FS_ReadFile((char *)name, (void **)&buffer);
	if(!buffer)
	{
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	targa_header.colormap_index = LittleShort(*(short *)buf_p);
	buf_p += 2;
	targa_header.colormap_length = LittleShort(*(short *)buf_p);
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort(*(short *)buf_p);
	buf_p += 2;
	targa_header.y_origin = LittleShort(*(short *)buf_p);
	buf_p += 2;
	targa_header.width = LittleShort(*(short *)buf_p);
	buf_p += 2;
	targa_header.height = LittleShort(*(short *)buf_p);
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if(targa_header.image_type != 2 && targa_header.image_type != 10 && targa_header.image_type != 3)
	{
		ri.Error(ERR_DROP, "LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n");
	}

	if(targa_header.colormap_type != 0)
	{
		ri.Error(ERR_DROP, "LoadTGA: colormaps not supported\n");
	}

	if((targa_header.pixel_size != 32 && targa_header.pixel_size != 24) && targa_header.image_type != 3)
	{
		ri.Error(ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if(width)
	{
		*width = columns;
	}
	if(height)
	{
		*height = rows;
	}

	targa_rgba = R_GetImageBuffer(numPixels * 4, BUFFER_IMAGE);
	*pic = targa_rgba;

	if(targa_header.id_length != 0)
	{
		buf_p += targa_header.id_length;	// skip TARGA image comment

	}
	if(targa_header.image_type == 2 || targa_header.image_type == 3)
	{
		// Uncompressed RGB or gray scale image
		for(row = rows - 1; row >= 0; row--)
		{
			pixbuf = targa_rgba + row * columns * 4;
			for(column = 0; column < columns; column++)
			{
				unsigned char   red, green, blue, alphabyte;

				switch (targa_header.pixel_size)
				{

					case 8:
						blue = *buf_p++;
						green = blue;
						red = blue;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
						break;

					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						break;
					default:
						ri.Error(ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name);
						break;
				}
			}
		}
	}
	else if(targa_header.image_type == 10)
	{							// Runlength encoded RGB images
		unsigned char   red, green, blue, alphabyte, packetHeader, packetSize, j;

		red = 0;
		green = 0;
		blue = 0;
		alphabyte = 0xff;

		for(row = rows - 1; row >= 0; row--)
		{
			pixbuf = targa_rgba + row * columns * 4;
			for(column = 0; column < columns;)
			{
				packetHeader = *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if(packetHeader & 0x80)
				{				// run-length packet
					switch (targa_header.pixel_size)
					{
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							break;
						default:
							ri.Error(ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name);
							break;
					}

					for(j = 0; j < packetSize; j++)
					{
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if(column == columns)
						{		// run spans across rows
							column = 0;
							if(row > 0)
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else
				{				// non run-length packet
					for(j = 0; j < packetSize; j++)
					{
						switch (targa_header.pixel_size)
						{
							case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							default:
								ri.Error(ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size,
										 name);
								break;
						}
						column++;
						if(column == columns)
						{		// pixel packet run spans across rows
							column = 0;
							if(row > 0)
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
		  breakOut:;
		}
	}

	ri.FS_FreeFile(buffer);
}

static void R_JPGErrorExit(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];
  
  (*cinfo->err->format_message) (cinfo, buffer);
  
  /* Let the memory manager delete any temp files before we die */
  jpeg_destroy(cinfo);
  
  ri.Error(ERR_FATAL, "%s", buffer);
}

static void R_JPGOutputMessage(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];
  
  /* Create the message */
  (*cinfo->err->format_message) (cinfo, buffer);
  
  /* Send it to stderr, adding a newline */
  ri.Printf(PRINT_ALL, "%s\n", buffer);
}

void LoadJPG(const char *filename, unsigned char **pic, int *width, int *height)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo = {NULL};
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  JSAMPARRAY buffer;		/* Output row buffer */
  unsigned int row_stride;	/* physical row width in output buffer */
  unsigned int pixelcount, memcount;
  unsigned int sindex, dindex;
  byte *out;
  int len;
	union {
		byte *b;
		void *v;
	} fbuffer;
  byte  *buf;

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  len = ri.FS_ReadFile ( ( char * ) filename, &fbuffer.v);
  if (!fbuffer.b || len < 0) {
	return;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = R_JPGErrorExit;
  cinfo.err->output_message = R_JPGOutputMessage;

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_mem_src(&cinfo, fbuffer.b, len);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /*
   * Make sure it always converts images to RGB color space. This will
   * automatically convert 8-bit greyscale images to RGB as well.
   */
  cinfo.out_color_space = JCS_RGB;

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */

  pixelcount = cinfo.output_width * cinfo.output_height;

  if(!cinfo.output_width || !cinfo.output_height
      || ((pixelcount * 4) / cinfo.output_width) / 4 != cinfo.output_height
      || pixelcount > 0x1FFFFFFF || cinfo.output_components != 3
    )
  {
    // Free the memory to make sure we don't leak memory
    ri.FS_FreeFile (fbuffer.v);
    jpeg_destroy_decompress(&cinfo);
  
    ri.Error(ERR_DROP, "LoadJPG: %s has an invalid image format: %dx%d*4=%d, components: %d", filename,
		    cinfo.output_width, cinfo.output_height, pixelcount * 4, cinfo.output_components);
  }

  memcount = pixelcount * 4;
  row_stride = cinfo.output_width * cinfo.output_components;

  out = R_GetImageBuffer(memcount, BUFFER_IMAGE);

  *width = cinfo.output_width;
  *height = cinfo.output_height;

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
	buf = ((out+(row_stride*cinfo.output_scanline)));
	buffer = &buf;
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
  }
  
  buf = out;

  // Expand from RGB to RGBA
  sindex = pixelcount * cinfo.output_components;
  dindex = memcount;

  do
  {	
    buf[--dindex] = 255;
    buf[--dindex] = buf[--sindex];
    buf[--dindex] = buf[--sindex];
    buf[--dindex] = buf[--sindex];
  } while(sindex);

  *pic = out;

  /* Step 7: Finish decompression */

  jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  ri.FS_FreeFile (fbuffer.v);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
}



/* Expanded data destination object for stdio output */

typedef struct
{
	struct jpeg_destination_mgr pub;	/* public fields */

	byte           *outfile;	/* target stream */
	int             size;
} my_destination_mgr;

typedef my_destination_mgr *my_dest_ptr;


/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

void init_destination(j_compress_ptr cinfo)
{
	my_dest_ptr     dest = (my_dest_ptr) cinfo->dest;

	dest->pub.next_output_byte = dest->outfile;
	dest->pub.free_in_buffer = dest->size;
}


/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */

boolean empty_output_buffer(j_compress_ptr cinfo)
{
	return TRUE;
}

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

static int      hackSize;

void term_destination(j_compress_ptr cinfo)
{
	my_dest_ptr     dest = (my_dest_ptr) cinfo->dest;
	size_t          datacount = dest->size - dest->pub.free_in_buffer;

	hackSize = datacount;
}


/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */

void jpegDest(j_compress_ptr cinfo, byte * outfile, int size)
{
	my_dest_ptr     dest;

	/* The destination object is made permanent so that multiple JPEG images
	 * can be written to the same file without re-executing jpeg_stdio_dest.
	 * This makes it dangerous to use this manager and a different destination
	 * manager serially with the same JPEG object, because their private object
	 * sizes may be different.  Caveat programmer.
	 */
	if(cinfo->dest == NULL)
	{							/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_destination_mgr));
	}

	dest = (my_dest_ptr) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outfile = outfile;
	dest->size = size;
}

/*
=================
SaveJPGToBuffer

Encodes JPEG from image in image_buffer and writes to buffer.
Expects RGB input data
=================
*/
int SaveJPGToBuffer(byte * buffer, size_t bufSize, int quality, int image_width, int image_height, byte * image_buffer)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW        row_pointer[1];	/* pointer to JSAMPLE row[s] */
	my_dest_ptr     dest;
	int             row_stride;	/* physical row width in image buffer */
	size_t          outcount;

	/* Step 1: allocate and initialize JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = R_JPGErrorExit;
	cinfo.err->output_message = R_JPGOutputMessage;

	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */
	jpegDest(&cinfo, buffer, bufSize);

	/* Step 3: set parameters for compression */
	cinfo.image_width = image_width;	/* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 3;	/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB;	/* colorspace of input image */

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */ );
	/* If quality is set high, disable chroma subsampling */
	if(quality >= 85)
	{
		cinfo.comp_info[0].h_samp_factor = 1;
		cinfo.comp_info[0].v_samp_factor = 1;
	}

	/* Step 4: Start compressor */
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */
	row_stride = image_width * cinfo.input_components;// + padding;	/* JSAMPLEs per row in image_buffer */

	while(cinfo.next_scanline < cinfo.image_height)
	{
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could pass
		 * more than one scanline at a time if that's more convenient.
		 */
		row_pointer[0] = &image_buffer[((cinfo.image_height - 1) * row_stride) - cinfo.next_scanline * row_stride];
		(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	/* Step 6: Finish compression */
	jpeg_finish_compress(&cinfo);

	dest = (my_dest_ptr) cinfo.dest;
	outcount = dest->size - dest->pub.free_in_buffer;

	/* Step 7: release JPEG compression object */
	jpeg_destroy_compress(&cinfo);

	/* And we're done! */
	return outcount;
}

void SaveJPG(char *filename, int quality, int image_width, int image_height, byte * image_buffer)
{
	byte           *out;
	size_t          bufSize;

	bufSize = image_width * image_height * 3;
	out = ri.Hunk_AllocateTempMemory(bufSize);

	bufSize = SaveJPGToBuffer(out, bufSize, quality, image_width, image_height, image_buffer);
	ri.FS_WriteFile(filename, out, bufSize);

	ri.Hunk_FreeTempMemory(out);
}

//===================================================================

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage(const char *name, byte ** pic, int *width, int *height)
{
	const char	*ext;
	int             len;

	*pic = NULL;
	*width = 0;
	*height = 0;

	len = strlen(name);
	if(len < 5)
	{
		return;
	}

	ext = COM_GetExtension(name);

#ifdef USE_WEBP
	if(!Q_stricmp(ext, "webp"))
	{
		LoadWEBP(name, pic, width, height);
	}
#endif
	if(!Q_stricmp(ext, "dds"))
	{
		LoadDDS(name, pic, width, height);
	}

	if(!Q_stricmp(ext, "tga"))
	{
		LoadTGA(name, pic, width, height);	// try tga first
	}
	else if(!Q_stricmp(ext, "pcx"))
	{
		LoadPCX32(name, pic, width, height);
	}
	else if(!Q_stricmp(ext, "bmp"))
	{
		LoadBMP(name, pic, width, height);
	}
	else if(!Q_stricmp(ext, "jpg"))
	{
		LoadJPG(name, pic, width, height);
	}
	else if(!Q_stricmp(ext, "jpeg"))
	{
		LoadJPG(name, pic, width, height);
	}
	else if(!Q_stricmp(ext, "png"))
	{
		LoadPNG(name, pic, width, height, 0xff);
	}

	if( !*pic ) {
		char	filename[MAX_QPATH];

		COM_StripExtension3(name, filename, MAX_QPATH);
#ifdef USE_WEBP
		LoadWEBP(va("%s.%s", filename, "webp"), pic, width, height);
		if( *pic ) return;
#endif
		LoadDDS(va("%s.%s", filename, "dds"), pic, width, height);
		if( *pic ) return;
		LoadTGA(va("%s.%s", filename, "tga"), pic, width, height);
		if( *pic ) return;
		LoadPCX32(va("%s.%s", filename, "pcx"), pic, width, height);
		if( *pic ) return;
		LoadBMP(va("%s.%s", filename, "bmp"), pic, width, height);
		if( *pic ) return;
		LoadJPG(va("%s.%s", filename, "jpg"), pic, width, height);
		if( *pic ) return;
		LoadJPG(va("%s.%s", filename, "jpeg"), pic, width, height);
		if( *pic ) return;
		LoadPNG(va("%s.%s", filename, "png"), pic, width, height, 0xff);
		if( *pic ) return;
	}
}


/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t        *R_FindImageFile(const char *name, qboolean mipmap, qboolean allowPicmip, int glWrapClampMode, qboolean lightmap)
{
	image_t        *image;
	int             width, height;
	byte           *pic;
	long            hash;
	qboolean        allowCompress = qfalse;


	if(!name)
	{
		return NULL;
	}

	hash = GenerateImageHashValue(name);

	// Ridah, caching
	if(r_cacheGathering->integer)
	{
		ri.Cmd_ExecuteText(EXEC_NOW, va("cache_usedfile image %s %i %i %i\n", name, mipmap, allowPicmip, glWrapClampMode));
	}

	//
	// see if the image is already loaded
	//
	for(image = r_imageHashTable[hash]; image; image = image->next)
	{
		if(!strcmp(name, image->imgName))
		{
			// the white image can be used with any set of parms, but other mismatches are errors
			if(strcmp(name, "*white"))
			{
				if(image->mipmap != mipmap)
				{
					ri.Printf(PRINT_DEVELOPER, "WARNING: reused image %s with mixed mipmap parm\n", name);
				}
				if(image->allowPicmip != allowPicmip)
				{
					ri.Printf(PRINT_DEVELOPER, "WARNING: reused image %s with mixed allowPicmip parm\n", name);
				}
				if(image->wrapClampMode != glWrapClampMode)
				{
					ri.Printf(PRINT_ALL, "WARNING: reused image %s with mixed glWrapClampMode parm\n", name);
				}
			}
			return image;
		}
	}

	// Ridah, check the cache
	// TTimo: assignment used as truth value
	// ydnar: don't do this for lightmaps
	if(!lightmap)
	{
		image = R_FindCachedImage(name, hash);
		if(image != NULL)
		{
			return image;
		}
	}

	//
	// load the pic from disk
	//
	R_LoadImage(name, &pic, &width, &height);
	if(pic == NULL)
	{							// if we dont get a successful load
// TTimo: Duane changed to _DEBUG in all cases
// I'd still want that code in the release builds on linux
// (possibly for mod authors)
// /me maintained off for win32, using otherwise but printing diagnostics as developer
#if !defined( _WIN32 )
		char            altname[MAX_QPATH];	// copy the name
		int             len;	//

		strcpy(altname, name);	//
		len = strlen(altname);	//
		altname[len - 3] = toupper(altname[len - 3]);	// and try upper case extension for unix systems
		altname[len - 2] = toupper(altname[len - 2]);	//
		altname[len - 1] = toupper(altname[len - 1]);	//
		ri.Printf(PRINT_DEVELOPER, "trying %s...", altname);	//
		R_LoadImage(altname, &pic, &width, &height);	//
		if(pic == NULL)
		{						// if that fails
			ri.Printf(PRINT_DEVELOPER, "no\n");	//
			return NULL;		// bail
		}						//
		ri.Printf(PRINT_DEVELOPER, "yes\n");	//
#else
		return NULL;
#endif
	}

	// Arnout: apply lightmap colouring
	if(lightmap)
	{
		R_ProcessLightmap(&pic, 4, width, height, &pic);

		// ydnar: no texture compression
		if(lightmap)
		{
			allowCompress = tr.allowCompress;
		}
		tr.allowCompress = -1;
	}

//#ifdef _DEBUG
//#define CHECKPOWEROF2
//#endif // _DEBUG

#ifdef CHECKPOWEROF2
	if(((width - 1) & width) || ((height - 1) & height))
	{
		Com_Printf("^1Image not power of 2 scaled: %s\n", name);
		return NULL;
	}
#endif							// CHECKPOWEROF2

	image = R_CreateImage((char *)name, pic, width, height, mipmap, allowPicmip, glWrapClampMode);

	//ri.Free( pic );

	// ydnar: no texture compression
	if(lightmap)
	{
		tr.allowCompress = allowCompress;
	}

	return image;
}


/*
================
R_CreateDlightImage
================
*/
#define DLIGHT_SIZE 16
static void R_CreateDlightImage(void)
{
	int             x, y;
	byte            data[DLIGHT_SIZE][DLIGHT_SIZE][4];
	int             b;

	// make a centered inverse-square falloff blob for dynamic lighting
	for(x = 0; x < DLIGHT_SIZE; x++)
	{
		for(y = 0; y < DLIGHT_SIZE; y++)
		{
			float           d;

			d = (DLIGHT_SIZE / 2 - 0.5f - x) * (DLIGHT_SIZE / 2 - 0.5f - x) +
				(DLIGHT_SIZE / 2 - 0.5f - y) * (DLIGHT_SIZE / 2 - 0.5f - y);
			b = 4000 / d;
			if(b > 255)
			{
				b = 255;
			}
			else if(b < 75)
			{
				b = 0;
			}
			data[y][x][0] = data[y][x][1] = data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}
	tr.dlightImage = R_CreateImage("*dlight", (byte *) data, DLIGHT_SIZE, DLIGHT_SIZE, qfalse, qfalse, GL_CLAMP);
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable(void)
{
	int             i;
	float           d;
	float           exp;

	exp = 0.5;

	for(i = 0; i < FOG_TABLE_SIZE; i++)
	{
		d = pow((float)i / (FOG_TABLE_SIZE - 1), exp);

		// ydnar: changed to linear fog
		tr.fogTable[i] = d;
		//% tr.fogTable[ i ] = (i / 255.0f);
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float R_FogFactor(float s, float t)
{
	float           d;

	s -= 1.0 / 512;
	if(s < 0)
	{
		return 0;
	}
	if(t < 1.0 / 32)
	{
		return 0;
	}
	if(t < 31.0 / 32)
	{
		s *= (t - 1.0f / 32.0f) / (30.0f / 32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if(s > 1.0)
	{
		s = 1.0;
	}

	d = tr.fogTable[(int)(s * (FOG_TABLE_SIZE - 1))];

	return d;
}


void            SaveTGAAlpha(char *name, byte ** pic, int width, int height);

/*
================
R_CreateFogImage
================
*/
#define FOG_S       16
#define FOG_T       16			// ydnar: used to be 32
						// arnout: yd changed it to 256, changing to 16
static void R_CreateFogImage(void)
{
	int             x, y, alpha;
	byte           *data;

	//float d;
#ifndef IPHONE	
	float           borderColor[4];
#endif //IPHONE	


	// allocate table for image
	data = ri.Hunk_AllocateTempMemory(FOG_S * FOG_T * 4);

	// ydnar: old fog texture generating algo

	// S is distance, T is depth
	/*for (x=0 ; x<FOG_S ; x++) {
	   for (y=0 ; y<FOG_T ; y++) {
	   d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

	   data[(y*FOG_S+x)*4+0] =
	   data[(y*FOG_S+x)*4+1] =
	   data[(y*FOG_S+x)*4+2] = 255;
	   data[(y*FOG_S+x)*4+3] = 255 * d;
	   }
	   } */

	//% SaveTGAAlpha( "fog_q3.tga", &data, FOG_S, FOG_T );

	// ydnar: new, linear fog texture generating algo for GL_CLAMP_TO_EDGE (OpenGL 1.2+)

	// S is distance, T is depth
	for(x = 0; x < FOG_S; x++)
	{
		for(y = 0; y < FOG_T; y++)
		{
			alpha = 270 * ((float)x / FOG_S) * ((float)y / FOG_T);	// need slop room for fp round to 0
			if(alpha < 0)
			{
				alpha = 0;
			}
			else if(alpha > 255)
			{
				alpha = 255;
			}

			// ensure edge/corner cases are fully transparent (at 0,0) or fully opaque (at 1,N where N is 0-1.0)
			if(x == 0)
			{
				alpha = 0;
			}
			else if(x == (FOG_S - 1))
			{
				alpha = 255;
			}

			data[(y * FOG_S + x) * 4 + 0] = data[(y * FOG_S + x) * 4 + 1] = data[(y * FOG_S + x) * 4 + 2] = 255;
			data[(y * FOG_S + x) * 4 + 3] = alpha;	//%   255*d;
		}
	}

	//% SaveTGAAlpha( "fog_yd.tga", &data, FOG_S, FOG_T );

	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", (byte *) data, FOG_S, FOG_T, qfalse, qfalse, GL_CLAMP);
	ri.Hunk_FreeTempMemory(data);
	
	// ydnar: the following lines are unecessary for new GL_CLAMP_TO_EDGE fog
#ifndef IPHONE
	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
#endif // !IPHONE	
}

/*
==================
R_CreateDefaultImage
==================
*/
#define DEFAULT_SIZE    16
static void R_CreateDefaultImage(void)
{
	int             x, y;
	byte            data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	memset(data, 0, sizeof(data));
	for(x = 0; x < DEFAULT_SIZE; x++)
	{
		for(y = 0; y < 2; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 128;
			data[y][x][2] = 0;
			data[y][x][3] = 255;

			data[x][y][0] = 255;
			data[x][y][1] = 128;
			data[x][y][2] = 0;
			data[x][y][3] = 255;

			data[DEFAULT_SIZE - 1 - y][x][0] = 255;
			data[DEFAULT_SIZE - 1 - y][x][1] = 128;
			data[DEFAULT_SIZE - 1 - y][x][2] = 0;
			data[DEFAULT_SIZE - 1 - y][x][3] = 255;

			data[x][DEFAULT_SIZE - 1 - y][0] = 255;
			data[x][DEFAULT_SIZE - 1 - y][1] = 128;
			data[x][DEFAULT_SIZE - 1 - y][2] = 0;
			data[x][DEFAULT_SIZE - 1 - y][3] = 255;
		}
	}
	tr.defaultImage = R_CreateImage("*default", (byte *) data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qfalse, GL_REPEAT);
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages(void)
{
	int             x, y;
	byte            data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	memset(data, 255, sizeof(data));
	tr.whiteImage = R_CreateImage("*white", (byte *) data, 8, 8, qfalse, qfalse, GL_REPEAT);

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for(x = 0; x < DEFAULT_SIZE; x++)
	{
		for(y = 0; y < DEFAULT_SIZE; y++)
		{
			data[y][x][0] = data[y][x][1] = data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;
		}
	}

	tr.identityLightImage = R_CreateImage("*identityLight", (byte *) data, 8, 8, qfalse, qfalse, GL_REPEAT);


	for(x = 0; x < 32; x++)
	{
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[x] = R_CreateImage("*scratch", (byte *) data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qtrue, GL_CLAMP);
	}

	R_CreateDlightImage();
	R_CreateFogImage();
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings(void)
{
	int             i, j;
	float           g;
	int             inf;
	int             shift;

	// setup the overbright lighting
	tr.overbrightBits = r_overBrightBits->integer;
	if(!glConfig.deviceSupportsGamma)
	{
		tr.overbrightBits = 0;	// need hardware gamma for overbright
	}

	// never overbright in windowed mode
	if(!glConfig.isFullscreen)
	{
		tr.overbrightBits = 0;
	}

	// allow 2 overbright bits in 24 bit, but only 1 in 16 bit
	if(glConfig.colorBits > 16)
	{
		if(tr.overbrightBits > 2)
		{
			tr.overbrightBits = 2;
		}
	}
	else
	{
		if(tr.overbrightBits > 1)
		{
			tr.overbrightBits = 1;
		}
	}
	if(tr.overbrightBits < 0)
	{
		tr.overbrightBits = 0;
	}

	tr.identityLight = 1.0f / (1 << tr.overbrightBits);
	tr.identityLightByte = 255 * tr.identityLight;


	if(r_intensity->value <= 1)
	{
		ri.Cvar_Set("r_intensity", "1");
	}

#ifdef __linux__
	if(r_gamma->value != -1)
	{
#endif

		if(r_gamma->value < 0.5f)
		{
			ri.Cvar_Set("r_gamma", "0.5");
		}
		else if(r_gamma->value > 3.0f)
		{
			ri.Cvar_Set("r_gamma", "3.0");
		}
		g = r_gamma->value;

#ifdef __linux__
	}
	else
	{
		g = 1.0f;
	}
#endif

	shift = tr.overbrightBits;

	for(i = 0; i < 256; i++)
	{
		if(g == 1)
		{
			inf = i;
		}
		else
		{
			inf = 255 * pow(i / 255.0f, 1.0f / g) + 0.5f;
		}
		inf <<= shift;
		if(inf < 0)
		{
			inf = 0;
		}
		if(inf > 255)
		{
			inf = 255;
		}
		s_gammatable[i] = inf;
	}

	for(i = 0; i < 256; i++)
	{
		j = i * r_intensity->value;
		if(j > 255)
		{
			j = 255;
		}
		s_intensitytable[i] = j;
	}

	if(glConfig.deviceSupportsGamma)
	{
		GLimp_SetGamma(s_gammatable, s_gammatable, s_gammatable);
	}
}

/*
===============
R_InitImages
===============
*/
void R_InitImages(void)
{
	memset(r_imageHashTable, 0, sizeof(r_imageHashTable));

	// Ridah, caching system
	//% R_InitTexnumImages(qfalse);
	// done.

	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();

	// Ridah, load the cache media, if they were loaded previously, they'll be restored from the backupImages
	R_LoadCacheImages();
	// done.
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures(void)
{
	int             i;

	for(i = 0; i < tr.numImages; i++)
	{
		glDeleteTextures(1, &tr.images[i]->texnum);
	}
	memset(tr.images, 0, sizeof(tr.images));
	// Ridah
	//% R_InitTexnumImages(qtrue);
	// done.

	memset(glState.currenttextures, 0, sizeof(glState.currenttextures));
	if ( GLEW_ARB_multitexture )
	{
		if(GLEW_ARB_multitexture)
		{
			GL_SelectTexture(1);
			glBindTexture(GL_TEXTURE_2D, 0);
			GL_SelectTexture(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
}


// Ridah, utility for automatically cropping and numbering a bunch of images in a directory
/*
=============
SaveTGA

  saves out to 24 bit uncompressed format (no alpha)
=============
*/
void SaveTGA(char *name, byte ** pic, int width, int height)
{
	byte           *inpixel, *outpixel;
	byte           *outbuf, *b;

	outbuf = ri.Hunk_AllocateTempMemory(width * height * 4 + 18);
	b = outbuf;

	memset(b, 0, 18);
	b[2] = 2;					// uncompressed type
	b[12] = width & 255;
	b[13] = width >> 8;
	b[14] = height & 255;
	b[15] = height >> 8;
	b[16] = 24;					// pixel size

	{
		int             row, col;
		int             rows, cols;

		rows = (height);
		cols = (width);

		outpixel = b + 18;

		for(row = (rows - 1); row >= 0; row--)
		{
			inpixel = ((*pic) + (row * cols) * 4);

			for(col = 0; col < cols; col++)
			{
				*outpixel++ = *(inpixel + 2);	// blue
				*outpixel++ = *(inpixel + 1);	// green
				*outpixel++ = *(inpixel + 0);	// red
				//*outpixel++ = *(inpixel + 3); // alpha

				inpixel += 4;
			}
		}
	}

	ri.FS_WriteFile(name, outbuf, (int)(outpixel - outbuf));

	ri.Hunk_FreeTempMemory(outbuf);

}

/*
=============
SaveTGAAlpha

  saves out to 32 bit uncompressed format (with alpha)
=============
*/
void SaveTGAAlpha(char *name, byte ** pic, int width, int height)
{
	byte           *inpixel, *outpixel;
	byte           *outbuf, *b;

	outbuf = ri.Hunk_AllocateTempMemory(width * height * 4 + 18);
	b = outbuf;

	memset(b, 0, 18);
	b[2] = 2;					// uncompressed type
	b[12] = width & 255;
	b[13] = width >> 8;
	b[14] = height & 255;
	b[15] = height >> 8;
	b[16] = 32;					// pixel size

	{
		int             row, col;
		int             rows, cols;

		rows = (height);
		cols = (width);

		outpixel = b + 18;

		for(row = (rows - 1); row >= 0; row--)
		{
			inpixel = ((*pic) + (row * cols) * 4);

			for(col = 0; col < cols; col++)
			{
				*outpixel++ = *(inpixel + 2);	// blue
				*outpixel++ = *(inpixel + 1);	// green
				*outpixel++ = *(inpixel + 0);	// red
				*outpixel++ = *(inpixel + 3);	// alpha

				inpixel += 4;
			}
		}
	}

	ri.FS_WriteFile(name, outbuf, (int)(outpixel - outbuf));

	ri.Hunk_FreeTempMemory(outbuf);

}

/*
=========================================================

PNG LOADING

=========================================================
*/
static void png_read_data(png_structp png, png_bytep data, png_size_t length)
{
	byte *io_ptr = png_get_io_ptr(png);
	Com_Memcpy(data, io_ptr, length);
	png_init_io(png, (png_FILE_p)(io_ptr + length));
}

static void png_user_warning_fn(png_structp png_ptr, png_const_charp warning_message)
{
	ri.Printf(PRINT_WARNING, "libpng warning: %s\n", warning_message);
}

static void png_user_error_fn(png_structp png_ptr, png_const_charp error_message)
{
	ri.Printf(PRINT_ERROR, "libpng error: %s\n", error_message);
	longjmp(png_jmpbuf(png_ptr), 0);
}

static void LoadPNG(const char *name, byte ** pic, int *width, int *height, byte alphaByte)
{
	int             bit_depth;
	int             color_type;
	png_uint_32     w;
	png_uint_32     h;
	unsigned int    row;
	size_t          rowbytes;
	png_infop       info;
	png_structp     png;
	png_bytep      *row_pointers;
	byte           *data;
	byte           *out;
	int             size;

	// load png
	size = ri.FS_ReadFile(name, (void **)&data);

	if(!data)
		return;

	//png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, png_user_error_fn, png_user_warning_fn);

	if(!png)
	{
		ri.Printf(PRINT_WARNING, "LoadPNG: png_create_write_struct() failed for (%s)\n", name);
		ri.FS_FreeFile(data);
		return;
	}

	// allocate/initialize the memory for image information.  REQUIRED
	info = png_create_info_struct(png);
	if(!info)
	{
		ri.Printf(PRINT_WARNING, "LoadPNG: png_create_info_struct() failed for (%s)\n", name);
		ri.FS_FreeFile(data);
		png_destroy_read_struct(&png, (png_infopp) NULL, (png_infopp) NULL);
		return;
	}

	/*
	 * Set error handling if you are using the setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in the png_create_read_struct() earlier.
	 */
	if(setjmp(png_jmpbuf(png)))
	{
		// if we get here, we had a problem reading the file
		ri.Printf(PRINT_WARNING, "LoadPNG: first exception handler called for (%s)\n", name);
		ri.FS_FreeFile(data);
		png_destroy_read_struct(&png, (png_infopp) & info, (png_infopp) NULL);
		return;
	}

	//png_set_write_fn(png, buffer, png_write_data, png_flush_data);
	png_set_read_fn(png, data, png_read_data);

	png_set_sig_bytes(png, 0);

	// The call to png_read_info() gives us all of the information from the
	// PNG file before the first IDAT (image data chunk).  REQUIRED
	png_read_info(png, info);

	// get picture info
	png_get_IHDR(png, info, (png_uint_32 *) & w, (png_uint_32 *) & h, &bit_depth, &color_type, NULL, NULL, NULL);

	// tell libpng to strip 16 bit/color files down to 8 bits/color
	png_set_strip_16(png);

	// expand paletted images to RGB triplets
	if(color_type & PNG_COLOR_MASK_PALETTE)
		png_set_expand(png);

	// expand gray-scaled images to RGB triplets
	if(!(color_type & PNG_COLOR_MASK_COLOR))
		png_set_gray_to_rgb(png);

	// expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel
	//if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
	//  png_set_gray_1_2_4_to_8(png);

	// expand paletted or RGB images with transparency to full alpha channels
	// so the data will be available as RGBA quartets
	if(png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	// if there is no alpha information, fill with alphaByte
	if(!(color_type & PNG_COLOR_MASK_ALPHA))
		png_set_filler(png, alphaByte, PNG_FILLER_AFTER);

	// expand pictures with less than 8bpp to 8bpp
	if(bit_depth < 8)
		png_set_packing(png);

	// update structure with the above settings
	png_read_update_info(png, info);

	// allocate the memory to hold the image
	*width = w;
	*height = h;
	*pic = out = (byte *) R_GetImageBuffer(w * h * 4, BUFFER_IMAGE);

	row_pointers = (png_bytep *) ri.Hunk_AllocateTempMemory(sizeof(png_bytep) * h);

	// set a new exception handler
	if(setjmp(png_jmpbuf(png)))
	{
		ri.Printf(PRINT_WARNING, "LoadPNG: second exception handler called for (%s)\n", name);
		ri.Hunk_FreeTempMemory(row_pointers);
		ri.FS_FreeFile(data);
		png_destroy_read_struct(&png, (png_infopp) & info, (png_infopp) NULL);
		return;
	}

	rowbytes = png_get_rowbytes(png, info);

	for(row = 0; row < h; row++)
		row_pointers[row] = (png_bytep) (out + (row * 4 * w));

	// read image data
	png_read_image(png, row_pointers);

	// read rest of file, and get additional chunks in info
	png_read_end(png, info);

	// clean up after the read, and free any memory allocated
	png_destroy_read_struct(&png, &info, (png_infopp) NULL);

	ri.Hunk_FreeTempMemory(row_pointers);
	ri.FS_FreeFile(data);
}

/*
=========================================================

PNG SAVING

=========================================================
*/
static int      png_compressed_size;

static void png_write_data(png_structp png, png_bytep data, png_size_t length)
{
	byte *io_ptr = png_get_io_ptr(png);
	Com_Memcpy(io_ptr, data, length);
	png_init_io(png, (png_FILE_p)(io_ptr + length));
	png_compressed_size += length;
}

static void png_flush_data(png_structp png)
{
}

void SavePNG(const char *name, const byte * pic, int width, int height, int numBytes, qboolean flip)
{
	png_structp     png;
	png_infop       info;
	int             i;
	int             row_stride;
	byte           *buffer;
	byte           *row;
	png_bytep      *row_pointers;

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if(!png)
		return;

	// Allocate/initialize the image information data
	info = png_create_info_struct(png);
	if(!info)
	{
		png_destroy_write_struct(&png, (png_infopp) NULL);
		return;
	}

	png_compressed_size = 0;
	buffer = ri.Hunk_AllocateTempMemory(width * height * numBytes);

	// set error handling
	if(setjmp(png_jmpbuf(png)))
	{
		ri.Hunk_FreeTempMemory(buffer);
		png_destroy_write_struct(&png, &info);
		return;
	}

	png_set_write_fn(png, buffer, png_write_data, png_flush_data);

	if(numBytes == 4)
	{
		png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT);
	}
	else
	{
		// should be 3
		png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
	}

	// write the file header information
	png_write_info(png, info);

	row_pointers = ri.Hunk_AllocateTempMemory(height * sizeof(png_bytep));

	if(setjmp(png_jmpbuf(png)))
	{
		ri.Hunk_FreeTempMemory(row_pointers);
		ri.Hunk_FreeTempMemory(buffer);
		png_destroy_write_struct(&png, &info);
		return;
	}

	row_stride = width * numBytes;
	row = (byte *)pic + (height - 1) * row_stride;

	if(flip)
	{
		for(i = height - 1; i >= 0; i--)
		{
			row_pointers[i] = row;
			row -= row_stride;
		}
	}
	else
	{
		for(i = 0; i < height; i++)
		{
			row_pointers[i] = row;
			row -= row_stride;
		}
	}

	png_write_image(png, row_pointers);
	png_write_end(png, info);

	// clean up after the write, and free any memory allocated
	png_destroy_write_struct(&png, &info);

	ri.Hunk_FreeTempMemory(row_pointers);

	ri.FS_WriteFile(name, buffer, png_compressed_size);

	ri.Hunk_FreeTempMemory(buffer);

	if ( ri.Cvar_Get( "developer", "", 0 )) {
#if defined (USE_HTTP)
		ri.HTTP_PostBug(name);
#endif
	}

}

 /*
=========================================================

DDS LOADING

=========================================================
*/

/* -----------------------------------------------------------------------------

DDS Library 

Based on code from Nvidia's DDS example:
http://www.nvidia.com/object/dxtc_decompression_code.html

Copyright (c) 2003 Randy Reddig
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the names of the copyright holders nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------------- */

/* dds definition */
typedef enum
{
	DDS_PF_ARGB8888,
	DDS_PF_DXT1,
	DDS_PF_DXT2,
	DDS_PF_DXT3,
	DDS_PF_DXT4,
	DDS_PF_DXT5,
	DDS_PF_UNKNOWN
}
ddsPF_t;


/* 16bpp stuff */
#define DDS_LOW_5		0x001F;
#define DDS_MID_6		0x07E0;
#define DDS_HIGH_5		0xF800;
#define DDS_MID_555		0x03E0;
#define DDS_HI_555		0x7C00;


/* structures */
typedef struct ddsColorKey_s
{
	unsigned int    colorSpaceLowValue;
	unsigned int    colorSpaceHighValue;
}
ddsColorKey_t;


typedef struct ddsCaps_s
{
	unsigned int    caps1;
	unsigned int    caps2;
	unsigned int    caps3;
	unsigned int    caps4;
}
ddsCaps_t;


typedef struct ddsMultiSampleCaps_s
{
	unsigned short  flipMSTypes;
	unsigned short  bltMSTypes;
}
ddsMultiSampleCaps_t;


typedef struct ddsPixelFormat_s
{
	unsigned int    size;
	unsigned int    flags;
	unsigned int    fourCC;
	union
	{
		unsigned int    rgbBitCount;
		unsigned int    yuvBitCount;
		unsigned int    zBufferBitDepth;
		unsigned int    alphaBitDepth;
		unsigned int    luminanceBitCount;
		unsigned int    bumpBitCount;
		unsigned int    privateFormatBitCount;
	};
	union
	{
		unsigned int    rBitMask;
		unsigned int    yBitMask;
		unsigned int    stencilBitDepth;
		unsigned int    luminanceBitMask;
		unsigned int    bumpDuBitMask;
		unsigned int    operations;
	};
	union
	{
		unsigned int    gBitMask;
		unsigned int    uBitMask;
		unsigned int    zBitMask;
		unsigned int    bumpDvBitMask;
		ddsMultiSampleCaps_t multiSampleCaps;
	};
	union
	{
		unsigned int    bBitMask;
		unsigned int    vBitMask;
		unsigned int    stencilBitMask;
		unsigned int    bumpLuminanceBitMask;
	};
	union
	{
		unsigned int    rgbAlphaBitMask;
		unsigned int    yuvAlphaBitMask;
		unsigned int    luminanceAlphaBitMask;
		unsigned int    rgbZBitMask;
		unsigned int    yuvZBitMask;
	};
}
ddsPixelFormat_t;


typedef struct ddsBuffer_s
{
	/* magic: 'dds ' */
	char            magic[4];

	/* directdraw surface */
	unsigned int    size;
	unsigned int    flags;
	unsigned int    height;
	unsigned int    width;
	union
	{
		int             pitch;
		unsigned int    linearSize;
	};
	unsigned int    backBufferCount;
	union
	{
		unsigned int    mipMapCount;
		unsigned int    refreshRate;
		unsigned int    srcVBHandle;
	};
	unsigned int    alphaBitDepth;
	unsigned int    reserved;
	void           *surface;
	union
	{
		ddsColorKey_t   ckDestOverlay;
		unsigned int    emptyFaceColor;
	};
	ddsColorKey_t   ckDestBlt;
	ddsColorKey_t   ckSrcOverlay;
	ddsColorKey_t   ckSrcBlt;
	union
	{
		ddsPixelFormat_t pixelFormat;
		unsigned int    fvf;
	};
	ddsCaps_t       ddsCaps;
	unsigned int    textureStage;

	/* data (Varying size) */
	unsigned char   data[4];
}
ddsBuffer_t;


typedef struct ddsColorBlock_s
{
	unsigned short  colors[2];
	unsigned char   row[4];
}
ddsColorBlock_t;


typedef struct ddsAlphaBlockExplicit_s
{
	unsigned short  row[4];
}
ddsAlphaBlockExplicit_t;


typedef struct ddsAlphaBlock3BitLinear_s
{
	unsigned char   alpha0;
	unsigned char   alpha1;
	unsigned char   stuff[6];
}
ddsAlphaBlock3BitLinear_t;


typedef struct ddsColor_s
{
	unsigned char   r, g, b, a;
}
ddsColor_t;

/*
DDSDecodePixelFormat()
determines which pixel format the dds texture is in
*/

static void DDSDecodePixelFormat(ddsBuffer_t * dds, ddsPF_t * pf)
{
	unsigned int    fourCC;


	/* dummy check */
	if(dds == NULL || pf == NULL)
		return;

	/* extract fourCC */
	fourCC = dds->pixelFormat.fourCC;

	/* test it */
	if(fourCC == 0)
		*pf = DDS_PF_ARGB8888;
	else if(fourCC == *((unsigned int *)"DXT1"))
		*pf = DDS_PF_DXT1;
	else if(fourCC == *((unsigned int *)"DXT2"))
		*pf = DDS_PF_DXT2;
	else if(fourCC == *((unsigned int *)"DXT3"))
		*pf = DDS_PF_DXT3;
	else if(fourCC == *((unsigned int *)"DXT4"))
		*pf = DDS_PF_DXT4;
	else if(fourCC == *((unsigned int *)"DXT5"))
		*pf = DDS_PF_DXT5;
	else
		*pf = DDS_PF_UNKNOWN;
}



/*
DDSGetInfo()
extracts relevant info from a dds texture, returns 0 on success
*/

int DDSGetInfo(ddsBuffer_t * dds, int *width, int *height, ddsPF_t * pf)
{
	/* dummy test */
	if(dds == NULL)
		return -1;

	/* test dds header */
	if(*((int *)dds->magic) != *((int *)"DDS "))
		return -1;
	if(LittleLong(dds->size) != 124)
		return -1;

	/* extract width and height */
	if(width != NULL)
		*width = LittleLong(dds->width);
	if(height != NULL)
		*height = LittleLong(dds->height);

	/* get pixel format */
	DDSDecodePixelFormat(dds, pf);

	/* return ok */
	return 0;
}



/*
DDSGetColorBlockColors()
extracts colors from a dds color block
*/

static void DDSGetColorBlockColors(ddsColorBlock_t * block, ddsColor_t colors[4])
{
	unsigned short  word;


	/* color 0 */
	word = LittleShort(block->colors[0]);
	colors[0].a = 0xff;

	/* extract rgb bits */
	colors[0].b = (unsigned char)word;
	colors[0].b <<= 3;
	colors[0].b |= (colors[0].b >> 5);
	word >>= 5;
	colors[0].g = (unsigned char)word;
	colors[0].g <<= 2;
	colors[0].g |= (colors[0].g >> 5);
	word >>= 6;
	colors[0].r = (unsigned char)word;
	colors[0].r <<= 3;
	colors[0].r |= (colors[0].r >> 5);

	/* same for color 1 */
	word = LittleShort(block->colors[1]);
	colors[1].a = 0xff;

	/* extract rgb bits */
	colors[1].b = (unsigned char)word;
	colors[1].b <<= 3;
	colors[1].b |= (colors[1].b >> 5);
	word >>= 5;
	colors[1].g = (unsigned char)word;
	colors[1].g <<= 2;
	colors[1].g |= (colors[1].g >> 5);
	word >>= 6;
	colors[1].r = (unsigned char)word;
	colors[1].r <<= 3;
	colors[1].r |= (colors[1].r >> 5);

	/* use this for all but the super-freak math method */
	if(block->colors[0] > block->colors[1])
	{
		/* four-color block: derive the other two colors.    
		   00 = color 0, 01 = color 1, 10 = color 2, 11 = color 3
		   these two bit codes correspond to the 2-bit fields 
		   stored in the 64-bit block. */

		word = ((unsigned short)colors[0].r * 2 + (unsigned short)colors[1].r) / 3;
		/* no +1 for rounding */
		/* as bits have been shifted to 888 */
		colors[2].r = (unsigned char)word;
		word = ((unsigned short)colors[0].g * 2 + (unsigned short)colors[1].g) / 3;
		colors[2].g = (unsigned char)word;
		word = ((unsigned short)colors[0].b * 2 + (unsigned short)colors[1].b) / 3;
		colors[2].b = (unsigned char)word;
		colors[2].a = 0xff;

		word = ((unsigned short)colors[0].r + (unsigned short)colors[1].r * 2) / 3;
		colors[3].r = (unsigned char)word;
		word = ((unsigned short)colors[0].g + (unsigned short)colors[1].g * 2) / 3;
		colors[3].g = (unsigned char)word;
		word = ((unsigned short)colors[0].b + (unsigned short)colors[1].b * 2) / 3;
		colors[3].b = (unsigned char)word;
		colors[3].a = 0xff;
	}
	else
	{
		/* three-color block: derive the other color.
		   00 = color 0, 01 = color 1, 10 = color 2,  
		   11 = transparent.
		   These two bit codes correspond to the 2-bit fields 
		   stored in the 64-bit block */

		word = ((unsigned short)colors[0].r + (unsigned short)colors[1].r) / 2;
		colors[2].r = (unsigned char)word;
		word = ((unsigned short)colors[0].g + (unsigned short)colors[1].g) / 2;
		colors[2].g = (unsigned char)word;
		word = ((unsigned short)colors[0].b + (unsigned short)colors[1].b) / 2;
		colors[2].b = (unsigned char)word;
		colors[2].a = 0xff;

		/* random color to indicate alpha */
		colors[3].r = 0x00;
		colors[3].g = 0xff;
		colors[3].b = 0xff;
		colors[3].a = 0x00;
	}
}



/*
DDSDecodeColorBlock()
decodes a dds color block
fixme: make endian-safe
*/

static void DDSDecodeColorBlock(unsigned int *pixel, ddsColorBlock_t * block, int width, unsigned int colors[4])
{
	int             r, n;
	unsigned int    bits;
	unsigned int    masks[] = { 3, 12, 3 << 4, 3 << 6 };	/* bit masks = 00000011, 00001100, 00110000, 11000000 */
	int             shift[] = { 0, 2, 4, 6 };


	/* r steps through lines in y */
	for(r = 0; r < 4; r++, pixel += (width - 4))	/* no width * 4 as unsigned int ptr inc will * 4 */
	{
		/* width * 4 bytes per pixel per line, each j dxtc row is 4 lines of pixels */

		/* n steps through pixels */
		for(n = 0; n < 4; n++)
		{
			bits = block->row[r] & masks[n];
			bits >>= shift[n];

			switch (bits)
			{
				case 0:
					*pixel = colors[0];
					pixel++;
					break;

				case 1:
					*pixel = colors[1];
					pixel++;
					break;

				case 2:
					*pixel = colors[2];
					pixel++;
					break;

				case 3:
					*pixel = colors[3];
					pixel++;
					break;

				default:
					/* invalid */
					pixel++;
					break;
			}
		}
	}
}



/*
DDSDecodeAlphaExplicit()
decodes a dds explicit alpha block
*/

static void DDSDecodeAlphaExplicit(unsigned int *pixel, ddsAlphaBlockExplicit_t * alphaBlock, int width, unsigned int alphaZero)
{
	int             row, pix;
	unsigned short  word;
	ddsColor_t      color;


	/* clear color */
	color.r = 0;
	color.g = 0;
	color.b = 0;

	/* walk rows */
	for(row = 0; row < 4; row++, pixel += (width - 4))
	{
		word = LittleShort(alphaBlock->row[row]);

		/* walk pixels */
		for(pix = 0; pix < 4; pix++)
		{
			/* zero the alpha bits of image pixel */
			*pixel &= alphaZero;
			color.a = word & 0x000F;
			color.a = color.a | (color.a << 4);
			*pixel |= *((unsigned int *)&color);
			word >>= 4;			/* move next bits to lowest 4 */
			pixel++;			/* move to next pixel in the row */

		}
	}
}



/*
DDSDecodeAlpha3BitLinear()
decodes interpolated alpha block
*/

static void DDSDecodeAlpha3BitLinear(unsigned int *pixel, ddsAlphaBlock3BitLinear_t * alphaBlock, int width,
									 unsigned int alphaZero)
{

	int             row, pix;
	unsigned int    stuff;
	unsigned char   bits[4][4];
	unsigned short  alphas[8];
	ddsColor_t      aColors[4][4];


	/* get initial alphas */
	alphas[0] = alphaBlock->alpha0;
	alphas[1] = alphaBlock->alpha1;

	/* 8-alpha block */
	if(alphas[0] > alphas[1])
	{
		/* 000 = alpha_0, 001 = alpha_1, others are interpolated */
		alphas[2] = (6 * alphas[0] + alphas[1]) / 7;	/* bit code 010 */
		alphas[3] = (5 * alphas[0] + 2 * alphas[1]) / 7;	/* bit code 011 */
		alphas[4] = (4 * alphas[0] + 3 * alphas[1]) / 7;	/* bit code 100 */
		alphas[5] = (3 * alphas[0] + 4 * alphas[1]) / 7;	/* bit code 101 */
		alphas[6] = (2 * alphas[0] + 5 * alphas[1]) / 7;	/* bit code 110 */
		alphas[7] = (alphas[0] + 6 * alphas[1]) / 7;	/* bit code 111 */
	}

	/* 6-alpha block */
	else
	{
		/* 000 = alpha_0, 001 = alpha_1, others are interpolated */
		alphas[2] = (4 * alphas[0] + alphas[1]) / 5;	/* bit code 010 */
		alphas[3] = (3 * alphas[0] + 2 * alphas[1]) / 5;	/* bit code 011 */
		alphas[4] = (2 * alphas[0] + 3 * alphas[1]) / 5;	/* bit code 100 */
		alphas[5] = (alphas[0] + 4 * alphas[1]) / 5;	/* bit code 101 */
		alphas[6] = 0;			/* bit code 110 */
		alphas[7] = 255;		/* bit code 111 */
	}

	/* decode 3-bit fields into array of 16 bytes with same value */

	/* first two rows of 4 pixels each */
	stuff = *((unsigned int *)&(alphaBlock->stuff[0]));

	bits[0][0] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[0][1] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[0][2] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[0][3] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[1][0] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[1][1] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[1][2] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[1][3] = (unsigned char)(stuff & 0x00000007);

	/* last two rows */
	stuff = *((unsigned int *)&(alphaBlock->stuff[3]));	/* last 3 bytes */

	bits[2][0] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[2][1] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[2][2] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[2][3] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[3][0] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[3][1] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[3][2] = (unsigned char)(stuff & 0x00000007);
	stuff >>= 3;
	bits[3][3] = (unsigned char)(stuff & 0x00000007);

	/* decode the codes into alpha values */
	for(row = 0; row < 4; row++)
	{
		for(pix = 0; pix < 4; pix++)
		{
			aColors[row][pix].r = 0;
			aColors[row][pix].g = 0;
			aColors[row][pix].b = 0;
			aColors[row][pix].a = (unsigned char)alphas[bits[row][pix]];
		}
	}

	/* write out alpha values to the image bits */
	for(row = 0; row < 4; row++, pixel += width - 4)
	{
		for(pix = 0; pix < 4; pix++)
		{
			/* zero the alpha bits of image pixel */
			*pixel &= alphaZero;

			/* or the bits into the prev. nulled alpha */
			*pixel |= *((unsigned int *)&(aColors[row][pix]));
			pixel++;
		}
	}
}



/*
DDSDecompressDXT1()
decompresses a dxt1 format texture
*/

static int DDSDecompressDXT1(ddsBuffer_t * dds, int width, int height, unsigned char *pixels)
{
	int             x, y, xBlocks, yBlocks;
	unsigned int   *pixel;
	ddsColorBlock_t *block;
	ddsColor_t      colors[4];


	/* setup */
	xBlocks = width / 4;
	yBlocks = height / 4;

	/* walk y */
	for(y = 0; y < yBlocks; y++)
	{
		/* 8 bytes per block */
		block = (ddsColorBlock_t *)((unsigned long)dds->data + y * xBlocks * 8);

		/* walk x */
		for(x = 0; x < xBlocks; x++, block++)
		{
			DDSGetColorBlockColors(block, colors);
			pixel = (unsigned int *)(pixels + x * 16 + (y * 4) * width * 4);
			DDSDecodeColorBlock(pixel, block, width, (unsigned int *)colors);
		}
	}

	/* return ok */
	return 0;
}



/*
DDSDecompressDXT3()
decompresses a dxt3 format texture
*/

static int DDSDecompressDXT3(ddsBuffer_t * dds, int width, int height, unsigned char *pixels)
{
	int             x, y, xBlocks, yBlocks;
	unsigned int   *pixel, alphaZero;
	ddsColorBlock_t *block;
	ddsAlphaBlockExplicit_t *alphaBlock;
	ddsColor_t      colors[4];


	/* setup */
	xBlocks = width / 4;
	yBlocks = height / 4;

	/* create zero alpha */
	colors[0].a = 0;
	colors[0].r = 0xFF;
	colors[0].g = 0xFF;
	colors[0].b = 0xFF;
	alphaZero = *((unsigned int *)&colors[0]);

	/* walk y */
	for(y = 0; y < yBlocks; y++)
	{
		/* 8 bytes per block, 1 block for alpha, 1 block for color */
		block = (ddsColorBlock_t *) ((unsigned long)dds->data + y * xBlocks * 16);

		/* walk x */
		for(x = 0; x < xBlocks; x++, block++)
		{
			/* get alpha block */
			alphaBlock = (ddsAlphaBlockExplicit_t *) block;

			/* get color block */
			block++;
			DDSGetColorBlockColors(block, colors);

			/* decode color block */
			pixel = (unsigned int *)(pixels + x * 16 + (y * 4) * width * 4);
			DDSDecodeColorBlock(pixel, block, width, (unsigned int *)colors);

			/* overwrite alpha bits with alpha block */
			DDSDecodeAlphaExplicit(pixel, alphaBlock, width, alphaZero);
		}
	}

	/* return ok */
	return 0;
}



/*
DDSDecompressDXT5()
decompresses a dxt5 format texture
*/

static int DDSDecompressDXT5(ddsBuffer_t * dds, int width, int height, unsigned char *pixels)
{
	int             x, y, xBlocks, yBlocks;
	unsigned int   *pixel, alphaZero;
	ddsColorBlock_t *block;
	ddsAlphaBlock3BitLinear_t *alphaBlock;
	ddsColor_t      colors[4];


	/* setup */
	xBlocks = width / 4;
	yBlocks = height / 4;

	/* create zero alpha */
	colors[0].a = 0;
	colors[0].r = 0xFF;
	colors[0].g = 0xFF;
	colors[0].b = 0xFF;
	alphaZero = *((unsigned int *)&colors[0]);

	/* walk y */
	for(y = 0; y < yBlocks; y++)
	{
		/* 8 bytes per block, 1 block for alpha, 1 block for color */
		block = (ddsColorBlock_t *) ((unsigned long)dds->data + y * xBlocks * 16);

		/* walk x */
		for(x = 0; x < xBlocks; x++, block++)
		{
			/* get alpha block */
			alphaBlock = (ddsAlphaBlock3BitLinear_t *) block;

			/* get color block */
			block++;
			DDSGetColorBlockColors(block, colors);

			/* decode color block */
			pixel = (unsigned int *)(pixels + x * 16 + (y * 4) * width * 4);
			DDSDecodeColorBlock(pixel, block, width, (unsigned int *)colors);

			/* overwrite alpha bits with alpha block */
			DDSDecodeAlpha3BitLinear(pixel, alphaBlock, width, alphaZero);
		}
	}

	/* return ok */
	return 0;
}



/*
DDSDecompressDXT2()
decompresses a dxt2 format texture (fixme: un-premultiply alpha)
*/

static int DDSDecompressDXT2(ddsBuffer_t * dds, int width, int height, unsigned char *pixels)
{
	int             r;


	/* decompress dxt3 first */
	r = DDSDecompressDXT3(dds, width, height, pixels);

	/* return to sender */
	return r;
}



/*
DDSDecompressDXT4()
decompresses a dxt4 format texture (fixme: un-premultiply alpha)
*/

static int DDSDecompressDXT4(ddsBuffer_t * dds, int width, int height, unsigned char *pixels)
{
	int             r;


	/* decompress dxt5 first */
	r = DDSDecompressDXT5(dds, width, height, pixels);

	/* return to sender */
	return r;
}



/*
DDSDecompressARGB8888()
decompresses an argb 8888 format texture
*/

static int DDSDecompressARGB8888(ddsBuffer_t * dds, int width, int height, unsigned char *pixels)
{
	int             x, y;
	unsigned char  *in, *out;


	/* setup */
	in = dds->data;
	out = pixels;

	/* walk y */
	for(y = 0; y < height; y++)
	{
		/* walk x */
		for(x = 0; x < width; x++)
		{
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
		}
	}

	/* return ok */
	return 0;
}



/*
DDSDecompress()
decompresses a dds texture into an rgba image buffer, returns 0 on success
*/

int DDSDecompress(ddsBuffer_t * dds, unsigned char *pixels)
{
	int             width, height, r;
	ddsPF_t         pf;


	/* get dds info */
	r = DDSGetInfo(dds, &width, &height, &pf);
	if(r)
		return r;

	/* decompress */
	switch (pf)
	{
		case DDS_PF_ARGB8888:
			/* fixme: support other [a]rgb formats */
			r = DDSDecompressARGB8888(dds, width, height, pixels);
			break;

		case DDS_PF_DXT1:
			r = DDSDecompressDXT1(dds, width, height, pixels);
			break;

		case DDS_PF_DXT2:
			r = DDSDecompressDXT2(dds, width, height, pixels);
			break;

		case DDS_PF_DXT3:
			r = DDSDecompressDXT3(dds, width, height, pixels);
			break;

		case DDS_PF_DXT4:
			r = DDSDecompressDXT4(dds, width, height, pixels);
			break;

		case DDS_PF_DXT5:
			r = DDSDecompressDXT5(dds, width, height, pixels);
			break;

		default:
		case DDS_PF_UNKNOWN:
			memset(pixels, 0xFF, width * height * 4);
			r = -1;
			break;
	}

	/* return to sender */
	return r;
}

/*
=============
LoadDDS
loads a dxtc (1, 3, 5) dds buffer into a valid rgba image
=============
*/
static void LoadDDS(const char *name, byte ** pic, int *width, int *height)
{
	int             w, h;
	ddsPF_t         pf;
	byte           *buffer;

	*pic = NULL;

	// load the file
	ri.FS_ReadFile((char *)name, (void **)&buffer);
	if(!buffer)
	{
		return;
	}

	// null out
	*pic = 0;
	*width = 0;
	*height = 0;

	// get dds info
	if(DDSGetInfo((ddsBuffer_t *) buffer, &w, &h, &pf))
	{
		ri.Error(ERR_DROP, "LoadDDS: Invalid DDS texture '%s'\n", name);
		return;
	}

	// only certain types of dds textures are supported
	if(pf != DDS_PF_ARGB8888 && pf != DDS_PF_DXT1 && pf != DDS_PF_DXT3 && pf != DDS_PF_DXT5)
	{
		ri.Error(ERR_DROP, "LoadDDS: Only DDS texture formats ARGB8888, DXT1, DXT3, and DXT5 are supported (%d) '%s'\n", pf,
				 name);
		return;
	}

	// create image pixel buffer
	*width = w;
	*height = h;
	*pic = R_GetImageBuffer(w * h * 4, BUFFER_IMAGE);

	// decompress the dds texture
	DDSDecompress((ddsBuffer_t *) buffer, *pic);

	ri.FS_FreeFile(buffer);
}


/*
==============
R_CropImage
==============
*/
#define CROPIMAGES_ENABLED
//#define FUNNEL_HACK
#define RESIZE
//#define QUICKTIME_BANNER
#define TWILTB2_HACK

qboolean R_CropImage(char *name, byte ** pic, int border, int *width, int *height, int lastBox[2])
{
	return qtrue;				// shutup the compiler
}


/*
===============
R_CropAndNumberImagesInDirectory
===============
*/
void R_CropAndNumberImagesInDirectory(char *dir, char *ext, int maxWidth, int maxHeight, int withAlpha)
{
}

/*
==============
R_CropImages_f
==============
*/
void R_CropImages_f(void)
{
}

// done.

//==========================================================================================
// Ridah, caching system

static int      numBackupImages = 0;
static image_t *backupHashTable[IMAGE_FILE_HASH_SIZE];

//% static image_t  *texnumImages[MAX_DRAWIMAGES*2];

/*
===============
R_CacheImageAlloc

  this will only get called to allocate the image_t structures, not that actual image pixels
===============
*/
void           *R_CacheImageAlloc(int size)
{
	if(r_cache->integer && r_cacheShaders->integer)
	{
//      return ri.Z_Malloc( size );
		return malloc(size);	// ri.Z_Malloc causes load times about twice as long?... Gordon
//DAJ TEST      return ri.Z_Malloc( size ); //DAJ was CO
	}
	else
	{
		return ri.Hunk_Alloc(size, h_low);
	}
}

/*
===============
R_CacheImageFree
===============
*/
void R_CacheImageFree(void *ptr)
{
	if(r_cache->integer && r_cacheShaders->integer)
	{
//      ri.Free( ptr );
		free(ptr);
//DAJ TEST      ri.Free( ptr ); //DAJ was CO
	}
}

/*
===============
R_TouchImage

  remove this image from the backupHashTable and make sure it doesn't get overwritten
===============
*/
qboolean R_TouchImage(image_t * inImage)
{
	image_t        *bImage, *bImagePrev;
	int             hash;
	char           *name;

	if(inImage == tr.dlightImage || inImage == tr.whiteImage || inImage == tr.defaultImage || inImage->imgName[0] == '*')
	{							// can't use lightmaps since they might have the same name, but different maps will have different actual lightmap pixels
		return qfalse;
	}

	hash = inImage->hash;
	name = inImage->imgName;

	bImage = backupHashTable[hash];
	bImagePrev = NULL;
	while(bImage)
	{

		if(bImage == inImage)
		{
			// add it to the current images
			if(tr.numImages == MAX_DRAWIMAGES)
			{
				ri.Error(ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n");
			}

			tr.images[tr.numImages] = bImage;

			// remove it from the backupHashTable
			if(bImagePrev)
			{
				bImagePrev->next = bImage->next;
			}
			else
			{
				backupHashTable[hash] = bImage->next;
			}

			// add it to the hashTable
			bImage->next = r_imageHashTable[hash];
			r_imageHashTable[hash] = bImage;

			// get the new texture
			tr.numImages++;

			return qtrue;
		}

		bImagePrev = bImage;
		bImage = bImage->next;
	}

	return qtrue;
}

/*
===============
R_PurgeImage
===============
*/
void R_PurgeImage(image_t * image)
{

	//% texnumImages[image->texnum - 1024] = NULL;

	glDeleteTextures(1, &image->texnum);

	R_CacheImageFree(image);

	memset(glState.currenttextures, 0, sizeof(glState.currenttextures));
	if ( GLEW_ARB_multitexture )
	{
		if(glActiveTextureARB)
		{
			GL_SelectTexture(1);
			glBindTexture(GL_TEXTURE_2D, 0);
			GL_SelectTexture(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
}


/*
===============
R_PurgeBackupImages

  Can specify the number of Images to purge this call (used for background purging)
===============
*/
void R_PurgeBackupImages(int purgeCount)
{
	int             i, cnt;
	static int      lastPurged = 0;
	image_t        *image;

	if(!numBackupImages)
	{
		// nothing to purge
		lastPurged = 0;
		return;
	}

	R_SyncRenderThread();

	cnt = 0;
	for(i = lastPurged; i < IMAGE_FILE_HASH_SIZE;)
	{
		lastPurged = i;
		// TTimo: assignment used as truth value
		if((image = backupHashTable[i]))
		{
			// kill it
			backupHashTable[i] = image->next;
			R_PurgeImage(image);
			cnt++;

			if(cnt >= purgeCount)
			{
				return;
			}
		}
		else
		{
			i++;				// no images in this slot, so move to the next one
		}
	}

	// all done
	numBackupImages = 0;
	lastPurged = 0;
}

/*
===============
R_BackupImages
===============
*/
void R_BackupImages(void)
{

	if(!r_cache->integer)
	{
		return;
	}
	if(!r_cacheShaders->integer)
	{
		return;
	}

	// backup the hashTable
	memcpy(backupHashTable, r_imageHashTable, sizeof(backupHashTable));

	// pretend we have cleared the list
	numBackupImages = tr.numImages;
	tr.numImages = 0;

	memset(glState.currenttextures, 0, sizeof(glState.currenttextures));
	if ( GLEW_ARB_multitexture )
	{
		if(glActiveTextureARB)
		{
			GL_SelectTexture(1);
			glBindTexture(GL_TEXTURE_2D, 0);
			GL_SelectTexture(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
}

/*
=============
R_FindCachedImage
=============
*/
image_t        *R_FindCachedImage(const char *name, int hash)
{
	image_t        *bImage, *bImagePrev;

	if(!r_cacheShaders->integer)
	{
		return NULL;
	}

	if(!numBackupImages)
	{
		return NULL;
	}

	bImage = backupHashTable[hash];
	bImagePrev = NULL;
	while(bImage)
	{

		if(!Q_stricmp(name, bImage->imgName))
		{
			// add it to the current images
			if(tr.numImages == MAX_DRAWIMAGES)
			{
				ri.Error(ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit\n");
			}

			R_TouchImage(bImage);
			return bImage;
		}

		bImagePrev = bImage;
		bImage = bImage->next;
	}

	return NULL;
}

//bani
/*
R_GetTextureId
*/
int R_GetTextureId(const char *name)
{
	int             i;

//  ri.Printf( PRINT_ALL, "R_GetTextureId [%s].\n", name );

	for(i = 0; i < tr.numImages; i++)
	{
		if(!strcmp(name, tr.images[i]->imgName))
		{
//          ri.Printf( PRINT_ALL, "Found textureid %d\n", i );
			return i;
		}
	}

//  ri.Printf( PRINT_ALL, "Image not found.\n" );
	return -1;
}

// ydnar: glGenTextures, sir...

#if 0
/*
===============
R_InitTexnumImages
===============
*/
static int      last_i;
void R_InitTexnumImages(qboolean force)
{
	if(force || !numBackupImages)
	{
		memset(texnumImages, 0, sizeof(texnumImages));
		last_i = 0;
	}
}

/*
============
R_FindFreeTexnum
============
*/
void R_FindFreeTexnum(image_t * inImage)
{
	int             i, max;
	image_t       **image;

	max = (MAX_DRAWIMAGES * 2);
	if(last_i && !texnumImages[last_i + 1])
	{
		i = last_i + 1;
	}
	else
	{
		i = 0;
		image = (image_t **) & texnumImages;
		while(i < max && *(image++))
		{
			i++;
		}
	}

	if(i < max)
	{
		if(i < max - 1)
		{
			last_i = i;
		}
		else
		{
			last_i = 0;
		}
		inImage->texnum = 1024 + i;
		texnumImages[i] = inImage;
	}
	else
	{
		ri.Error(ERR_DROP, "R_FindFreeTexnum: MAX_DRAWIMAGES hit\n");
	}
}
#endif



/*
===============
R_LoadCacheImages
===============
*/
void R_LoadCacheImages(void)
{
	int             len;
	void           *buf;
	char           *token, *pString;
	char            name[MAX_QPATH];
	int             parms[4], i;

	if(numBackupImages)
	{
		return;
	}

	len = ri.FS_ReadFile("image.cache", NULL);

	if(len <= 0)
	{
		return;
	}

	buf = ri.Hunk_AllocateTempMemory(len);
	ri.FS_ReadFile("image.cache", &buf);
	pString = buf;

	while((token = COM_ParseExt(&pString, qtrue)) && token[0])
	{
		Q_strncpyz(name, token, sizeof(name));
		for(i = 0; i < 4; i++)
		{
			token = COM_ParseExt(&pString, qfalse);
			parms[i] = atoi(token);
		}
		R_FindImageFile(name, parms[0], parms[1], parms[2], parms[3]);
	}

	ri.Hunk_FreeTempMemory(buf);
}

// done.
//==========================================================================================
