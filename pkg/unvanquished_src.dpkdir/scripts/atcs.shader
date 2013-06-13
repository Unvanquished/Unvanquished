textures/atcs/skybox_s
{
	qer_editorimage textures/atcs/mars.jpg
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_lightimage textures/atcs/skylight.tga
//	q3map_sun 0.95 0.95 1.0 150 120 25	//lilacisles
//	q3map_sun 1.00 1.00 0.965 75 90 30 	//siege
//	q3map_sun 1.00 0.90 0.80 110 180 35	//orangecream
//	q3map_sun 1.00 0.80 0.70 100 0 20	//cottoncandy
//	q3map_sun 0.934 0.835 1.00 75 230 35	//desertsky
//	q3map_sun 1.00 1.00 1.00 50 90 90	//comablack
//	q3map_sun 4 3 3 150 135 50		//mars
//	q3map_sun 1.00 0.949 0.977 100 200 45	//desert
	q3map_sun 1.00 0.949 0.977 150 135 45
	q3map_skylight 250 3
	q3map_globaltexture
	skyparms env/atcs/mars - -
}

textures/atcs/burst_red_s
{
	entityMergable
	cull none
	{
		map textures/atcs/sparkle_red.tga
		blendFunc add
		alphaGen	vertex
	}
}

textures/atcs/burst_blue_s
{
	entityMergable
	cull none
	{
		map textures/atcs/sparkle_blue.tga
		blendFunc add
		alphaGen	vertex
	}
}

textures/atcs/force_field_s
	{
        qer_editorimage textures/atcs/force_field_gtk.tga
	surfaceparm trans
        surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	{
		map textures/atcs/force_field.tga
		tcMod Scroll .1 0
		blendFunc add
	}
	{
		map textures/atcs/force_grid.tga
		tcMod Scroll -.01 0
		blendFunc add
		rgbgen wave sin .2 .2 0 .4
	}
}

textures/atcs/bulb_red_s
{
	qer_editorimage textures/atcs/bulb_red.tga
	surfaceparm nomarks
	surfaceparm trans
	cull disable
	qer_trans 0.5
	{
		map textures/atcs/bulb_red.tga
		blendfunc gl_dst_color gl_src_alpha
	}
	{
		map textures/atcs/bulb_red.tga
		blendfunc gl_dst_color gl_src_alpha
	}
}

textures/atcs/eq2_bounce
{
	qer_editorimage textures/atcs/eq2_bounce.tga

	{
		map textures/atcs/eq2_bounce.tga
		rgbGen identity
	}
	{
		clampmap textures/atcs/eq2_bouncefan.tga
		tcMod rotate 512
		blendFunc blend
		depthWrite
		rgbGen identity

	}
	{
		map textures/atcs/eq2_bounce.tga
		blendfunc blend
		rgbGen identity
	}
		{
		map $lightmap
		rgbGen identity
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
	}

}

textures/atcs/cubelight_32_blue_invis_s_15k
{
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm noimpact
	q3map_surfacelight 15000
	q3map_lightimage textures/atcs/cubelight_32_blue.blend.tga
	qer_editorimage textures/atcs/cubelight_32_blue.blend.tga
}

textures/atcs/cubelight_32_red_invis_s_15k
{
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm noimpact
	q3map_surfacelight 15000
	q3map_lightimage textures/atcs/cubelight_32_red.blend.tga
	qer_editorimage textures/atcs/cubelight_32_red.blend.tga
}

textures/atcs/cubelight_32_white_s_20k
{
	surfaceparm nomarks
	q3map_surfacelight 20000
	q3map_lightimage textures/atcs/cubelight_32_white.blend.tga
	qer_editorimage textures/atcs/cubelight_32_white.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_white.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_white.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_blue_s_20k
{
	surfaceparm nomarks
	q3map_surfacelight 20000
	q3map_lightimage textures/atcs/cubelight_32_blue.blend.tga
	qer_editorimage textures/atcs/cubelight_32_blue.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_blue_s_15k
{
	surfaceparm nomarks
	q3map_surfacelight 15000
	q3map_lightimage textures/atcs/cubelight_32_blue.blend.tga
	qer_editorimage textures/atcs/cubelight_32_blue.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_blue_s_10k
{
	surfaceparm nomarks
	q3map_surfacelight 10000
	q3map_lightimage textures/atcs/cubelight_32_blue.blend.tga
	qer_editorimage textures/atcs/cubelight_32_blue.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_red_s_20k
{
	surfaceparm nomarks
	q3map_surfacelight 20000
	q3map_lightimage textures/atcs/cubelight_32_red.blend.tga
	qer_editorimage textures/atcs/cubelight_32_red.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_red_s_15k
{
	surfaceparm nomarks
	q3map_surfacelight 15000
	q3map_lightimage textures/atcs/cubelight_32_red.blend.tga
	qer_editorimage textures/atcs/cubelight_32_red.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_red_s_10k
{
	surfaceparm nomarks
	q3map_surfacelight 10000
	q3map_lightimage textures/atcs/cubelight_32_red.blend.tga
	qer_editorimage textures/atcs/cubelight_32_red.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_blue_s_3000
{
	surfaceparm nomarks
	q3map_surfacelight 3000
	q3map_lightimage textures/atcs/eq2_baselt03b_blue.blend.tga
	qer_editorimage textures/atcs/eq2_baselt03b_blue.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_blue_s_2000
{
	surfaceparm nomarks
	q3map_surfacelight 2000
	q3map_lightimage textures/atcs/eq2_baselt03b_blue.blend.tga
	qer_editorimage textures/atcs/eq2_baselt03b_blue.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_blue_s_1500
{
	surfaceparm nomarks
	q3map_surfacelight 1500
	q3map_lightimage textures/atcs/eq2_baselt03b_blue.blend.tga
	qer_editorimage textures/atcs/eq2_baselt03b_blue.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_s_2000
{
	surfaceparm nomarks
	q3map_surfacelight 2000
	q3map_lightimage textures/atcs/eq2_baselt03b.blend.tga
	qer_editorimage textures/atcs/eq2_baselt03b.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_s_1500
{
	surfaceparm nomarks
	q3map_surfacelight 1500
	q3map_lightimage textures/atcs/eq2_baselt03b.blend.tga
	qer_editorimage textures/atcs/eq2_baselt03b.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2_baselt03_blue_s_5000
{
	surfaceparm nomarks
	q3map_surfacelight 5000
	q3map_lightimage textures/atcs/eq2_baselt03_blue.blend.tga
	qer_editorimage textures/atcs/eq2_baselt03_blue.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03_blue.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03_blue.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2_baselt03_blue_s_3000
{
	surfaceparm nomarks
	q3map_surfacelight 3000
	q3map_lightimage textures/atcs/eq2_baselt03_blue.blend.tga
	qer_editorimage textures/atcs/eq2_baselt03_blue.tga
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03_blue.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03_blue.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_bmtl03light_300
{
	surfaceparm nomarks
	q3map_surfacelight 300
	qer_editorimage textures/atcs/eq2_bmtl_03_light.tga
	q3map_lightimage textures/atcs/eq2_bmtl_03_light.blend.tga

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_bmtl_03_light.tga
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_bmtl_03_light.blend.tga
		blendfunc GL_ONE GL_ONE
	}
}
