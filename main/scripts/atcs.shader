textures/atcs/skybox_s
{
	qer_editorimage textures/atcs/mars
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	q3map_lightimage textures/atcs/skylight
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
		map textures/atcs/sparkle_red
		blendFunc add
		alphaGen	vertex
	}
}

textures/atcs/burst_blue_s
{
	entityMergable
	cull none
	{
		map textures/atcs/sparkle_blue
		blendFunc add
		alphaGen	vertex
	}
}

textures/atcs/force_field_s
	{
        qer_editorimage textures/atcs/force_field_gtk
	surfaceparm trans
        surfaceparm nomarks
	surfaceparm nolightmap
	cull none
	{
		map textures/atcs/force_field
		tcMod Scroll .1 0
		blendFunc add
	}
	{
		map textures/atcs/force_grid
		tcMod Scroll -.01 0
		blendFunc add
		rgbgen wave sin .2 .2 0 .4
	}
}

textures/atcs/bulb_red_s
{
	qer_editorimage textures/atcs/bulb_red
	surfaceparm nomarks
	surfaceparm trans
	cull disable
	qer_trans 0.5
	{
		map textures/atcs/bulb_red
		blendfunc gl_dst_color gl_src_alpha
	}
	{
		map textures/atcs/bulb_red
		blendfunc gl_dst_color gl_src_alpha
	}
}

textures/atcs/eq2_bounce
{
	qer_editorimage textures/atcs/eq2_bounce

	{
		map textures/atcs/eq2_bounce
		rgbGen identity
	}
	{
		clampmap textures/atcs/eq2_bouncefan
		tcMod rotate 512
		blendFunc blend
		depthWrite
		rgbGen identity

	}
	{
		map textures/atcs/eq2_bounce
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
	q3map_lightimage textures/atcs/cubelight_32_blue.blend
	qer_editorimage textures/atcs/cubelight_32_blue.blend
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
	q3map_lightimage textures/atcs/cubelight_32_red.blend
	qer_editorimage textures/atcs/cubelight_32_red.blend
}

textures/atcs/cubelight_32_white_s_20k
{
	surfaceparm nomarks
	q3map_surfacelight 20000
	q3map_lightimage textures/atcs/cubelight_32_white.blend
	qer_editorimage textures/atcs/cubelight_32_white
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_white
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_white.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_blue_s_20k
{
	surfaceparm nomarks
	q3map_surfacelight 20000
	q3map_lightimage textures/atcs/cubelight_32_blue.blend
	qer_editorimage textures/atcs/cubelight_32_blue
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_blue_s_15k
{
	surfaceparm nomarks
	q3map_surfacelight 15000
	q3map_lightimage textures/atcs/cubelight_32_blue.blend
	qer_editorimage textures/atcs/cubelight_32_blue
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_blue_s_10k
{
	surfaceparm nomarks
	q3map_surfacelight 10000
	q3map_lightimage textures/atcs/cubelight_32_blue.blend
	qer_editorimage textures/atcs/cubelight_32_blue
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_blue.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_red_s_20k
{
	surfaceparm nomarks
	q3map_surfacelight 20000
	q3map_lightimage textures/atcs/cubelight_32_red.blend
	qer_editorimage textures/atcs/cubelight_32_red
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_red_s_15k
{
	surfaceparm nomarks
	q3map_surfacelight 15000
	q3map_lightimage textures/atcs/cubelight_32_red.blend
	qer_editorimage textures/atcs/cubelight_32_red
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/cubelight_32_red_s_10k
{
	surfaceparm nomarks
	q3map_surfacelight 10000
	q3map_lightimage textures/atcs/cubelight_32_red.blend
	qer_editorimage textures/atcs/cubelight_32_red
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/cubelight_32_red.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_blue_s_3000
{
	surfaceparm nomarks
	q3map_surfacelight 3000
	q3map_lightimage textures/atcs/eq2_baselt03b_blue.blend
	qer_editorimage textures/atcs/eq2_baselt03b_blue
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_blue_s_2000
{
	surfaceparm nomarks
	q3map_surfacelight 2000
	q3map_lightimage textures/atcs/eq2_baselt03b_blue.blend
	qer_editorimage textures/atcs/eq2_baselt03b_blue
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_blue_s_1500
{
	surfaceparm nomarks
	q3map_surfacelight 1500
	q3map_lightimage textures/atcs/eq2_baselt03b_blue.blend
	qer_editorimage textures/atcs/eq2_baselt03b_blue
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b_blue.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_s_2000
{
	surfaceparm nomarks
	q3map_surfacelight 2000
	q3map_lightimage textures/atcs/eq2_baselt03b.blend
	qer_editorimage textures/atcs/eq2_baselt03b
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_baselt03b_s_1500
{
	surfaceparm nomarks
	q3map_surfacelight 1500
	q3map_lightimage textures/atcs/eq2_baselt03b.blend
	qer_editorimage textures/atcs/eq2_baselt03b
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03b.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2_baselt03_blue_s_5000
{
	surfaceparm nomarks
	q3map_surfacelight 5000
	q3map_lightimage textures/atcs/eq2_baselt03_blue.blend
	qer_editorimage textures/atcs/eq2_baselt03_blue
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03_blue
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03_blue.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2_baselt03_blue_s_3000
{
	surfaceparm nomarks
	q3map_surfacelight 3000
	q3map_lightimage textures/atcs/eq2_baselt03_blue.blend
	qer_editorimage textures/atcs/eq2_baselt03_blue
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03_blue
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_baselt03_blue.blend
		blendfunc GL_ONE GL_ONE
	}
}

textures/atcs/eq2lt_bmtl03light_300
{
	surfaceparm nomarks
	q3map_surfacelight 300
	qer_editorimage textures/atcs/eq2_bmtl_03_light
	q3map_lightimage textures/atcs/eq2_bmtl_03_light.blend

	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/atcs/eq2_bmtl_03_light
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map textures/atcs/eq2_bmtl_03_light.blend
		blendfunc GL_ONE GL_ONE
	}
}
