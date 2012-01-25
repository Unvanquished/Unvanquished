// *********************************************************
// *********************************************************
// OUT OF DATE - - MAKE FUTURE CHANGES TO "battery.shader"
// *********************************************************
// *********************************************************


// ocean fog water
textures/mp_seawall/fog_water
{
	qer_editorimage textures/common/fog_water
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	surfaceparm water
}

// abstract shader for subclassed shaders
textures/mp_seawall/ocean_base
{
	qer_editorimage textures/liquids_sd/seawall_ocean
	qer_trans 0.75
	q3map_tcGen ivector ( 512 0 0 ) ( 0 512 0 )
	q3map_globalTexture
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm lightfilter
	surfaceparm pointlight
}

// nonsolid terrain shader
textures/mp_seawall/ocean_terrain
{
	qer_editorimage textures/common/terrain_nonsolid
	q3map_terrain
	qer_trans .5
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	
}

// subclassed ocean shaders
textures/mp_seawall/ocean_0
{
	q3map_baseshader textures/mp_seawall/ocean_base
	cull none
	deformVertexes wave 1317 sin 0 12.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30
	
	{
		map textures/liquids_sd/seawall_specular
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
		depthWrite
	}
		
	{ 
		map textures/liquids_sd/sea_bright_na
		blendFunc blend
		rgbGen identity
		alphaGen const 0.8
		tcmod scroll 0.005 0.03
	}
	
}

textures/mp_seawall/ocean_1
{
	q3map_baseshader textures/mp_seawall/ocean_base
	cull none
	deformVertexes wave 1317 sin 0 12.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30
	
	{
		map textures/liquids_sd/seawall_specular
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
		depthWrite
	}
	
	{ 
		map textures/liquids_sd/sea_bright_na
		blendFunc blend
		rgbGen identity
		alphaGen const .8
		tcmod scroll 0.005 0.03

	}
	{
		map textures/liquids_sd/seawall_foam
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave sin 0.2 0.1 0 0.2
		alphaGen vertex
		tcMod turb 0 0.05 0 0.15
		tcmod scroll -0.01 0.08
	}
	{ 
		map textures/liquids_sd/seawall_foam
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave sin 0.15 0.1 0.1 0.15
		alphaGen vertex
		tcMod turb 0 0.05 0.5 0.15
		tcmod scroll 0.01 0.09
	}
	
}

textures/mp_seawall/ocean_0to1
{
	q3map_baseshader textures/mp_seawall/ocean_base
	cull none
	deformVertexes wave 1317 sin 0 12.5 0 0.15
 	deformVertexes wave 317 sin 0 1.5 0 0.30
	
	{
		map textures/liquids_sd/seawall_specular
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
		tcGen environment
		depthWrite
	}
	
	{ 
		map textures/liquids_sd/sea_bright_na
		blendFunc blend
		rgbGen identity
		alphaGen const .8
		tcmod scroll 0.005 0.03
	}
	{
		map textures/liquids_sd/seawall_foam
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave sin 0.2 0.1 0 0.2
		alphaGen vertex
		tcMod turb 0 0.05 0 0.15
		tcmod scroll -0.01 0.08
	}
	{ 
		map textures/liquids_sd/seawall_foam
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen wave sin 0.15 0.1 0.1 0.15
		alphaGen vertex
		tcMod turb 0 0.05 0.5 0.15
		tcmod scroll 0.01 0.09
	}
	
}

textures/mp_seawall/rock_graynoise
{
    q3map_nonplanar
    q3map_shadeangle 180
    //q3map_tcmodscale 1.5 1.5
    qer_editorimage textures/temperate_sd/rock_grayvar
    {
        map $lightmap
        rgbGen identity
    }
    {
        map textures/temperate_sd/rock_grayvar
        blendFunc filter
    }
    {
    	map textures/detail_sd/sanddetail
    	blendFunc GL_DST_COLOR GL_SRC_COLOR
    	detail
    	tcMod scale 6 6
    }
}

textures/mp_seawall/sand_disturb
{
    q3map_nonplanar
    q3map_shadeangle 180
    //q3map_tcmodscale 1.5 1.5
    qer_editorimage textures/temperate_sd/sand_disturb_bright
    surfaceparm landmine
    surfaceparm gravelsteps
    
    {
        map $lightmap
        rgbGen identity
    }
    {
		map textures/temperate_sd/sand_disturb_bright
        blendFunc filter
    }
    {
    	map textures/detail_sd/sanddetail
    	blendFunc GL_DST_COLOR GL_SRC_COLOR
    	detail
    	tcMod scale 6 6
    }
}

textures/mp_seawall/terrain_base
{
	q3map_tcGen ivector ( 256 0 0 ) ( 0 256 0 )
	q3map_lightmapsize 512 512
	q3map_lightmapMergable
	q3map_lightmapsamplesize 16
	q3map_lightmapaxis z
	q3map_shadeangle 179
}

textures/mp_seawall/terrain_0
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	
	{

		map textures/temperate_sd/sand_disturb_bright
		rgbgen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}
}

textures/mp_seawall/terrain_1
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_wave_bright
		rgbgen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}
}

textures/mp_seawall/terrain_2
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}
}

textures/mp_seawall/terrain_3
{
	q3map_baseshader textures/mp_seawall/terrain_base
	{
		map textures/temperate_sd/rock_graynoise
		rgbgen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}
}

textures/mp_seawall/terrain_4
{
	q3map_baseshader textures/mp_seawall/terrain_base
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}
}

textures/mp_seawall/terrain_5
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_patchnoise
		rgbgen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}
}


textures/mp_seawall/terrain_0to1
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_disturb_bright
		rgbgen identity
		alphaGen vertex
		
	}
	{
		map textures/temperate_sd/sand_wave_bright
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}
}

textures/mp_seawall/terrain_0to2
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_disturb_bright
		rgbgen identity
		alphaGen vertex
		
	}
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}
}

textures/mp_seawall/terrain_0to3
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_disturb_bright
		rgbgen identity
		alphaGen vertex
		
	}
	{
		map textures/temperate_sd/rock_graynoise
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}

	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_0to4
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_disturb_bright
		rgbgen identity
		alphaGen vertex
		
	}
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}

	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_0to5
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_disturb_bright
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/sand_patchnoise
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}


textures/mp_seawall/terrain_1to2
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_wave_bright
		rgbgen identity
		alphaGen vertex

	}
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_1to3
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_wave_bright
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/rock_graynoise
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_1to4
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_wave_bright
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_1to5
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/sand_wave_bright
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/sand_patchnoise
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}



textures/mp_seawall/terrain_2to3
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/rock_graynoise
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_2to4
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_2to5
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/sand_patchnoise
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_3to4
{
	q3map_baseshader textures/mp_seawall/terrain_base
	{
		map textures/temperate_sd/rock_graynoise
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_3to5
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/rock_graynoise
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/sand_patchnoise
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/terrain_3to6
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/rock_graynoise
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/sand_disturb_medium
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}



textures/mp_seawall/terrain_4to5
{
	q3map_baseshader textures/mp_seawall/terrain_base
	surfaceparm landmine
	surfaceparm gravelsteps
	{
		map textures/temperate_sd/rock_grayvar
		rgbgen identity
		alphaGen vertex
	}
	{
		map textures/temperate_sd/sand_patchnoise
		rgbgen identity
		alphaGen vertex
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
	}
	{
		map textures/detail_sd/sanddetail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		detail
		tcMod scale 4 4
	}

}

textures/mp_seawall/water_nodraw
{
	qer_editorimage textures/common/nodraw
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	surfaceparm water
}

textures/mp_seawall/water_fog
{
	qer_editorimage textures/common/fog_water
	surfaceparm nodraw
  	surfaceparm nonsolid
  	surfaceparm trans
  	surfaceparm fog
  	
  	fogparms ( 0.3137 0.36 0.4039 ) 256
}
