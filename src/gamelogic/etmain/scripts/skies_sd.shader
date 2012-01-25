textures/skies_sd/full_moon
{
	cull none
	nofog
	{
		map textures/skies_sd/full_moon.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen identityLighting
	}
}

textures/skies_sd/full_moon2
{
	cull none
	nofog
	{
		map textures/skies_sd/full_moon2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen identityLighting
	}
}

textures/skies_sd/small_moon
{
	cull none
	nofog
	{
		map textures/skies_sd/small_moon.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen identityLighting
	}
}

// -----------------------------------------------------------------------------------
// this should be deleted at some point ... the new "battery" equivalent is below
textures/skies_sd/seawallsunfog
{
	cull none
	q3map_nofog
	nofog
	{
		map textures/skies_sd/seawallsunfog.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen identityLighting
	}
}
// -----------------------------------------------------------------------------------

textures/skies_sd/batterysunfog
{
	cull none
	q3map_nofog
	nofog
	{
		map textures/skies_sd/batterysunfog.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen identityLighting
	}
}

textures/skies_sd/sd_siwasky
{
	nocompress
	qer_editorimage textures/skies_sd/nero_bluelight.tga
	q3map_lightimage textures/skies_sd/siwa_clouds.tga
	
	q3map_sun 0.75 0.70 0.6 135 199 49
	q3map_skylight 75 3
	sunshader textures/skies_sd/siwasunbright
	
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm nodlight
	surfaceparm sky
	
	skyparms textures/skies_sd/wurzburg_env/sky 512 -
 
	{
		fog off
		clampmap textures/skies_sd/siwa_mask.tga
		tcMod scale 0.956 0.956
		tcMod transform 1 0 0 1 -1 -1
		rgbGen identityLighting

	}
}

textures/skies_sd/siwasunbright
{
	nocompress
	cull none
	q3map_nofog
	
	{
		map textures/skies_sd/siwasun.tga
		blendFunc blend
		//blendFunc GL_SRC_ALPHA GL_ONE
		//rgbGen const ( 1.0 0.95 0.9 )
		rgbGen identityLighting
		//rgbGen identity
	}
}

