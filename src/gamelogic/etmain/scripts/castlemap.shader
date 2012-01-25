//Castlemap  Models


models/castlemap/castlemap_bottle
{ 
qer_editorimage models/castlemap/bottle_castlemap
cull twosided 
q3map_forceMeta 
q3map_nonplanar 
q3map_shadeAngle 179
q3map_lightmapSampleOffset 8.0
implicitMap models/castlemap/bottle_castlemap 
}

models/castlemap/castlemap_crate
{
	qer_editorimage models/castlemap/castlemap_crate
	surfaceparm wood
	implicitMap models/castlemap/castlemap_crate
}

models/castlemap/castlemap_crate_cull
{
	qer_editorimage models/castlemap/castlemap_crate
	cull twosided 
	surfaceparm wood
	implicitMap models/castlemap/castlemap_crate
}

models/castlemap/castlemap_light
{
	qer_editorimage models/castlemap/castlemap_light
	//q3map_lightimage models/castlemap/castlemap_light
	//q3map_surfacelight 5000
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map models/castlemap/castlemap_light
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map models/castlemap/castlemap_light_blend
		blendFunc GL_ONE GL_ONE
	}
}

models/castlemap/castlemap_light_blue
{
	qer_editorimage models/castlemap/castlemap_light_bblend
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map models/castlemap/castlemap_light
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
	{
		map models/castlemap/castlemap_light_bblend
		blendFunc GL_ONE GL_ONE
	}
}

models/castlemap/megaphone
{ 
qer_editorimage models/castlemap/mega1
q3map_forceMeta 
q3map_nonplanar 
q3map_shadeAngle 180
q3map_lightmapSampleOffset 8.0
implicitMap models/castlemap/mega1
}

models/castlemap/wolfstatue
{ 
qer_editorimage models/castlemap/wolfstatue
q3map_forceMeta 
q3map_nonplanar 
q3map_shadeAngle 180 
q3map_lightmapSampleOffset 8.0
implicitMap models/castlemap/wolfstatue 
}

models/castlemap/skull4
{
	qer_editorimage models/castlemap/skull_text

	{
		map textures/effects/envmap_slate
		tcGen environment
	}
	{
		map models/castlemap/skull_text2 
		blendFunc GL_ONE GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map models/castlemap/skull_blend 
		blendfunc add
		rgbGen wave triangle .5 .2 .5 3.5
	}
}

textures/sfx/roq_candle_flame
{
	cull none
	q3map_surfacelight 1000
	//nofog
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm pointlight
	surfaceparm trans
	
	qer_editorimage textures/sfx/roq_candle_flame
	{
		videomap video/roq_candle_flame.roq
		blendfunc add
		rgbGen identity
	}
}

//********************************************
//***************ZEPPELIN*********************
//********************************************

models/castlemap/balloon
{
	qer_editorimage models/castlemap/balloon
	q3map_nonplanar
	q3map_forceMeta
	q3map_shadeangle 45
	{
		map $lightmap
		rgbGen identity
	}
	{
		map models/castlemap/balloon
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

models/castlemap/cabine
{
	qer_editorimage models/castlemap/cabine
	q3map_forcemeta
	q3map_lightmapSampleOffset 8.0
	q3map_nonplanar
	implicitMap models/castlemap/cabine
	surfaceparm pointlight
	q3map_shadeangle 90
 
}

models/castlemap/banner1
{
	qer_editorimage models/castlemap/banner1
	q3map_nonplanar
	q3map_forceMeta
	q3map_shadeangle 45
	{
		map $lightmap
		rgbGen identity
	}
	{
		map models/castlemap/banner1
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

models/castlemap/inside
{
	qer_editorimage models/castlemap/inside
	q3map_forcemeta
	q3map_lightmapSampleOffset 8.0
	q3map_nonplanar
	implicitMap models/castlemap/inside
	surfaceparm pointlight
	q3map_shadeangle 90
 
}



models/castlemap/prop
{
	qer_editorimage models/castlemap/propz
	cull none
	nopicmip
	surfaceparm nomarks
	surfaceparm trans
	

{
		
		clampMap models/castlemap/propz
		blendFunc blend
		rgbGen identity
		tcMod rotate 512
	}
{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
		rgbGen identity
	}
}



models/castlemap/roof
{
	qer_editorimage textures/xlab_floor/xfloor_c01
	q3map_nonplanar
	q3map_forceMeta
	q3map_shadeangle 45
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/xlab_floor/xfloor_c01
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

models/castlemap/floor
{
	qer_editorimage textures/metal_misc/diamond_c_01b
	q3map_nonplanar
	q3map_forceMeta
	q3map_shadeangle 45
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/metal_misc/diamond_c_01b
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

//********************************************
//***************END_____ZEPPELIN*************
//********************************************

