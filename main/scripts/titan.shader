textures/titan/grates_alpha
{
	surfaceparm	metalsteps
	cull none

	// A GRATE OR GRILL THAT CAN BE SEEN FROM BOTH SIDES
	{
		map textures/titan/grates_alpha.tga
		tcMod scale 2 2
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		depthWrite
		rgbGen identity
	}
	{
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}
}

textures/titan/asteroid_skybox
{
      qer_editorimage textures/titan/Asteroid
      surfaceparm noimpact
      surfaceparm nolightmap
      surfaceparm sky
     // q3map_sun   .5 .37 .19 90 170 33
      q3map_surfacelight 15

      skyparms env/Asteroid - -
}
