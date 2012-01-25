//======================================================================
// Fueldump.shader
// Last edit: 26/04/03 Sock
//
//======================================================================
// q3map_sun <red> <green> <blue> <intensity> <degrees> <elevation>
textures/fueldump/fueldumpsky
{
	qer_editorimage textures/skies/fueldump_clouds
	q3map_lightrgb 0.8 0.9 1.0
	q3map_skylight 85 3
	q3map_sun 1 .95 .9 200 210 28
	skyparms - 200 -
	surfaceparm nodlight
	surfaceparm noimpact
	surfaceparm nolightmap
	surfaceparm sky
	{
		map textures/skies/fueldump_clouds
		rgbGen identity
	}
	{
		map textures/skies/fueldump_clouds
		blendfunc blend
		rgbGen identity
		tcMod scroll 0.0005 0.00
		tcMod scale 2 1
	}
}

// Not used because of performance reasons
//textures/fueldump/fueldumpfog
//{
//	qer_editorimage textures/sfx/fog_grey1
//	fogparms ( 0.535 0.574 0.613 ) 9216
//	surfaceparm fog
//	surfaceparm nodraw
//	surfaceparm nonsolid
//	surfaceparm trans
//}

//textures/fueldump/fueldumpwaterfog
//{
//	qer_editorimage textures/sfx/fog_grey1
//	fogparms ( 0.24 0.28 0.28 ) 32
//	surfaceparm fog
//	surfaceparm nodraw
//	surfaceparm nonsolid
//	surfaceparm trans
//}

//======================================================================
// Base for metashaders
//======================================================================
textures/fueldump/terrain_base
{
	q3map_lightmapMergable
	q3map_lightmapaxis z
	q3map_lightmapsize 512 512
	q3map_normalimage textures/sd_bumpmaps/normalmap_terrain
	q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_tcMod rotate 37
	q3map_tcMod scale 1 1
	surfaceparm landmine
	surfaceparm snowsteps
}

//======================================================================
// Metashader for both sides of the map
// _0: Slighty dirty snow
// _1: Mud/dirt snow
// _2: Clean white snow
// _3: Snow crispy rock
//======================================================================
textures/fueldump/terrain1_0
{
        qer_editorimage textures/stone/mxsnow2
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/stone/mxsnow2
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_1
{
        qer_editorimage textures/snow_sd/mxrock4b_snow
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/snow_sd/mxrock4b_snow
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_2
{
        qer_editorimage textures/stone/mxsnow3
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/stone/mxsnow3
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_3
{
        qer_editorimage textures/stone/mxrock3h_snow
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/stone/mxrock3h_snow
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_0to1
{
        qer_editorimage textures/stone/mxsnow2
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/stone/mxsnow2
		rgbgen identity
	}
	{
		map textures/snow_sd/mxrock4b_snow
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_0to2
{
        qer_editorimage textures/stone/mxsnow2
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/stone/mxsnow2
		rgbgen identity
	}
	{
		map textures/stone/mxsnow3
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_0to3
{
        qer_editorimage textures/stone/mxsnow2
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/stone/mxsnow2
		rgbgen identity
	}
	{
		map textures/stone/mxrock3h_snow
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_1to2
{
        qer_editorimage textures/snow_sd/mxrock4b_snow
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/snow_sd/mxrock4b_snow
		rgbgen identity
	}
	{
		map textures/stone/mxsnow3
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_1to3
{
        qer_editorimage textures/snow_sd/mxrock4b_snow
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/snow_sd/mxrock4b_snow
		rgbgen identity
	}
	{
		map textures/stone/mxrock3h_snow
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

textures/fueldump/terrain1_2to3
{
        qer_editorimage textures/stone/mxsnow3
	q3map_baseshader textures/fueldump/terrain_base
	{
		map textures/stone/mxsnow3
		rgbgen identity
	}
	{
		map textures/stone/mxrock3h_snow
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen identity
		alphaGen vertex
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 5 5
		detail
	}
}

//===========================================================================
// Floor and wall textures for the cave system + hong phong goodness
//===========================================================================
textures/fueldump/cave_dark
{
	q3map_nonplanar
	q3map_shadeangle 60
	qer_editorimage textures/stone/mxrock3_a
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/stone/mxrock3_a
		blendFunc filter
	}
}

textures/fueldump/cave_floor
{
	surfaceparm gravelsteps
	q3map_nonplanar
	q3map_shadeangle 60
	qer_editorimage textures/stone/mxrock1aa
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/stone/mxrock1aa
		blendFunc filter
	}
}

textures/fueldump/snow_floor
{
	surfaceparm gravelsteps
	q3map_nonplanar
	q3map_shadeangle 179
	qer_editorimage textures/snow/s_dirt_m03i_2_phong
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/snow/s_dirt_m03i_2
		blendFunc filter
	}
}

textures/terrain/dirt_m03i
{
	{
		map textures/terrain/dirt_m03i
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 1 1
		detail
	}
}

//==========================================================================
// Terrain/tunnel blend textures
//==========================================================================
textures/snow_sd/snow_road01
{
	q3map_nonplanar
	q3map_shadeangle 179
	surfaceparm snowsteps
	{
		map textures/snow_sd/snow_road01
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 1 1
		detail
	}
}

textures/snow_sd/snow_path01
{
	q3map_nonplanar
	q3map_shadeangle 179
	surfaceparm snowsteps
	{
		map textures/snow_sd/snow_path01
		rgbgen identity
	}
	{
		lightmap $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		rgbgen identity
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 1 1
		detail
	}
}

//==========================================================================
// Misc stuff for the central building in the axis base
//==========================================================================
// Comms tower
textures/fueldump/atruss_m06a
{
	qer_alphafunc gequal 0.5
	qer_editorimage textures/assault/atruss_m06a
	cull disable
	nomipmaps
	nopicmip
	surfaceparm alphashadow
	surfaceparm roofsteps
	surfaceparm trans
	implicitMask textures/assault/atruss_m06a
}

textures/awf/awf_w_m11
{
	qer_editorimage textures/awf/awf_w_m11
	q3map_lightimage textures/awf/awf_w_m11_g
	q3map_surfacelight 200
	surfaceparm nomarks
	implicitMask textures/awf/awf_w_m11
}

textures/awf/awf_w_m11_nlm
{
	qer_editorimage textures/awf/awf_w_m11_nlm
	surfaceparm nomarks
	implicitMask textures/awf/awf_w_m11
}

// This is more or less similiar to clipmissile
textures/alpha/fence_c11fd
{
	qer_trans 0.85
	qer_editorimage textures/alpha/fence_c11
	cull disable
	nomipmaps
	nopicmip

	surfaceparm clipmissile
	surfaceparm nomarks
	surfaceparm alphashadow
	surfaceparm playerclip
	surfaceparm metalsteps
	surfaceparm pointlight
	surfaceparm trans

	implicitMask textures/alpha/fence_c11
}

//==========================================================================
// Various terrain decals textures
//==========================================================================

// ydnar: nuked unnecessary alphaGen vertex part & added surfaceparm trans & pointlight
textures/fueldump/cave_floorblend
{
	qer_editorimage textures/snow/s_dirt_m03i_alphadir
	q3map_nonplanar
	q3map_shadeangle 60
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm gravelsteps
	surfaceparm pointlight
	polygonOffset

	{
		map textures/snow/s_dirt_m03i_alpha
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}


textures/fueldump/alphatree
{
	qer_editorimage textures/snow/s_dirt_m03i_alphatree
	q3map_nonplanar 
	q3map_shadeangle 120 
	surfaceparm trans 
	surfaceparm nonsolid 
	surfaceparm pointlight
	surfaceparm nomarks
	polygonOffset
	{
		map textures/snow/s_dirt_m03i_alphatree
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

//==========================================================================
// Various metal surfaceparm textures
//==========================================================================
textures/fueldump/door_m01asml
{
	qer_editorimage textures/fueldump_sd/door_m01asml
	surfaceparm metalsteps
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/fueldump_sd/door_m01asml
		blendFunc filter
	}
}

textures/fueldump/door_m01asml_axis
{
	qer_editorimage textures/fueldump_sd/door_m01asml_axis
	surfaceparm metalsteps
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/fueldump_sd/door_m01asml_axis
		blendFunc filter
	}
}

//==========================================================================
// Ice Lake
//==========================================================================
textures/fueldump/icelake_top
{
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake3
	sort seethrough
	surfaceparm slick
	surfaceparm trans
	surfaceparm glass
	
	tesssize 256

	{
		map textures/effects/envmap_ice2
		tcgen environment
		blendfunc blend
	}
	{
		map textures/snow_sd/icelake3
		blendfunc blend
	}
	{
		map $lightmap
		blendfunc filter
		rgbGen identity
		depthWrite
	}
	{
		map textures/detail_sd/snowdetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
		tcMod scale 4 4
		detail
	}
}

textures/fueldump/icelake_bottom
{
	qer_trans 0.80
	qer_editorimage textures/snow_sd/icelake3
	sort seethrough
	surfaceparm trans
	cull disable
	
	{
		map textures/snow_sd/icelake3
		blendfunc filter
	}
}

textures/fueldump/riverbed
{
	qer_editorimage textures/stone/mxdebri0_riverbed

	{
		map textures/stone/mxdebri0_riverbed
		rgbGen vertex
	}	
}
