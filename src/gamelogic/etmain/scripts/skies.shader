textures/skies/sd_goldrush
{
	qer_editorimage textures/skies/sky_8.tga
	q3map_skylight 65 3
	q3map_sun 0.3 0.3 0.45 60 35 45
	nocompress
	skyparms - 200 -
	sunshader textures/skies_sd/full_moon2
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	
	skyparms textures/skies_sd/wurzburg_env/sky 512 -
 
	
	{	fog off
		map textures/skies_sd/goldrush_clouds.tga
		tcMod scale 5 5
		tcMod scroll 0.0015 -0.003
		rgbGen identityLighting
	}


	{	fog off
		map textures/skies/nightsky1.jpg
		tcMod scale 10 10
		blendfunc add
		rgbGen identityLighting
	}

	{
		fog off
		clampmap textures/skies_sd/goldrush_mask.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcMod scale 0.956 0.956
		tcMod transform 1 0 0 1 -1 -1
		// rgbGen identityLighting
		//rgbGen const ( 0.6 0.6 0.6 ) 
		rgbGen const ( 0.4 0.4 0.4 ) 
	}
}

textures/skies/sd_railgun
{
	qer_editorimage textures/skies/sky_6.tga
	q3map_globaltexture
	q3map_lightsubdivide 1024
	q3map_sun 0.55 0.55 0.55 90 220 60
	q3map_surfacelight 90
	skyparms - 200 -
	surfaceparm nodlight
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
}

// -----------------------------------------------------------------------------------
// these should be deleted at some point ... the new "battery" equivalents are below
textures/skies/sd_seawallfog
{
	qer_editorimage textures/sfx/fog_grey1.tga
	
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm fog
	
	fogparms ( 0.4 0.4 0.4 ) 8192
}

textures/skies/sd_seawallsky
{
	nocompress
	qer_editorimage textures/skies/topclouds.tga
	q3map_lightrgb 0.8 0.9 1.0
	q3map_sun 1 .96 .87 140 140 8
	q3map_skylight 60 3
	q3map_nofog
	
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	surfaceparm sky
	
	skyparms textures/skies_sd/wurzburg_env/sky 512 -
 
	sunshader textures/skies_sd/seawallsunfog
 
	
	{	fog off
		map textures/skies_sd/seawall_clouds.tga
		tcMod scale 2.5 2.5
		tcMod scroll 0.0015 -0.003
		rgbGen identityLighting
	}

	{
		fog off
		clampmap textures/skies_sd/seawall_mask_ydnar.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcMod scale 0.956 0.956
		tcMod transform 1 0 0 1 -1 -1
		// rgbGen identityLighting
		//rgbGen const ( 0.6 0.6 0.6 ) 
		rgbGen const ( 0.4 0.4 0.4 ) 
	}
}

textures/skies/sd_seawallunderseasky
{
	qer_editorimage textures/skies_sd/white.tga
	
	{ 
		fog off
		map $whiteimage
		rgbGen const ( 0.4 0.4 0.4 )
	}	
	
}
// -----------------------------------------------------------------------------------

textures/skies/sd_batteryfog
{
	qer_editorimage textures/sfx/fog_grey1.tga
	
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm fog
	
	fogparms ( 0.4 0.4 0.4 ) 8192
}

textures/skies/sd_batterysky
{
	nocompress
	qer_editorimage textures/skies/topclouds.tga
	q3map_lightrgb 0.8 0.9 1.0
	q3map_sun 1 .96 .87 140 140 8
	q3map_skylight 60 3
	q3map_nofog
	
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	surfaceparm sky
	
	skyparms textures/skies_sd/wurzburg_env/sky 512 -
 
	sunshader textures/skies_sd/batterysunfog
 
	
	{	fog off
		map textures/skies_sd/battery_clouds.tga
		tcMod scale 2.5 2.5
		tcMod scroll 0.0015 -0.003
		rgbGen identityLighting
	}

	{
		fog off
		clampmap textures/skies_sd/battery_mask_ydnar.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcMod scale 0.956 0.956
		tcMod transform 1 0 0 1 -1 -1
		// rgbGen identityLighting
		//rgbGen const ( 0.6 0.6 0.6 ) 
		rgbGen const ( 0.4 0.4 0.4 ) 
	}
}

textures/skies/sd_batteryunderseasky
{
	qer_editorimage textures/skies_sd/white.tga
	
	{ 
		fog off
		map $whiteimage
		rgbGen const ( 0.4 0.4 0.4 )
	}	
	
}

textures/skies/sd_siwa
{
	qer_editorimage textures/skies_sd/nero_bluelight.tga
	q3map_globaltexture
	q3map_lightimage textures/skies_sd/nero_bluelight.tga
	q3map_lightsubdivide 768
	q3map_sun 0.75 0.70 0.6 120 220 45
	q3map_surfacelight 25
	skyparms - 200 -
	surfaceparm nodlight
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	{
		map textures/skies_sd/siwa_clouds.tga
		tcMod scroll -0.001 -0.0025
		tcMod scale 3 3
	}
}

textures/skies/sd_siwafog
{
	qer_editorimage textures/sfx/fog_grey1.tga
	
	surfaceparm nodraw
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm fog
	
	fogparms ( 0.77 0.64 0.46 ) 20480
}

textures/skies/sd_wurzburgsky
{
	nocompress
	qer_editorimage textures/skies/sky_8.tga
	q3map_lightimage textures/skies/n_blue2.tga
	q3map_nofog
	q3map_globaltexture
	q3map_lightsubdivide 256 
	q3map_sun 0.130 0.080 0.020 20 165 5
	q3map_sun 0.281 0.288 0.370 80 35 40
	q3map_sun 0.281 0.288 0.370 15 215 60
	q3map_sun 0.281 0.288 0.370 10 35 50
	q3map_sun 0.281 0.288 0.370 10 35 45
	q3map_sun 0.281 0.288 0.370 10 35 35
	q3map_sun 0.281 0.288 0.370 10 35 30
	q3map_sun 0.281 0.288 0.370 10 40 40
	q3map_sun 0.281 0.288 0.370 10 45 40
	q3map_sun 0.281 0.288 0.370 10 30 40
	q3map_sun 0.281 0.288 0.370 10 25 40
	q3map_surfacelight 30

	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky

	skyparms - 512 -
	sunshader textures/skies_sd/small_moon

	{
		map textures/skies_sd/wurzburg_clouds.tga
		tcMod scale 2.5 2.5
		tcMod scroll 0.003 -0.0015
		rgbGen identityLighting
	}
	{
		map textures/skies_sd/ydnar_lightning.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave triangle -8 9 0 0.137
		alphaGen wave noise -3 4 0 2.37
		tcMod scale 3 3
		tcMod scroll 0.003 -0.0015
	}
	{
		clampmap textures/skies_sd/wurzburg_fogmask.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcMod scale 0.956 0.956
		tcMod transform 1 0 0 1 -1 -1
		rgbGen identitylighting
	}
}

textures/skies/sky_goldrush
{
	qer_editorimage textures/skies/sky_8.tga
	q3map_globaltexture
	q3map_lightimage textures/skies/n_blue2.tga
	q3map_lightsubdivide 512
	q3map_sun 0.3 0.3 0.45 30 35 45
	q3map_surfacelight 27
	nocompress
	skyparms - 200 -
	sunshader sun
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	{
		map textures/skies/nightsky1.tga
		tcMod scale 16.0 16.0
	}
	{
		map textures/skies/vil_clouds1.tga
		blendfunc blend
		tcMod scroll 0.001 0.00
		tcMod scale 2 1
	}
}

