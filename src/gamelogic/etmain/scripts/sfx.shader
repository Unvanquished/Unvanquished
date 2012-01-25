// sfx.shader

textures/sfx/beam_dusty2
{
	qer_editorimage textures/sfx/beam_1.tga
	cull none
	nocompress
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm pointlight
	surfaceparm trans
	{
		map textures/sfx/beam_1.tga
		blendFunc add
	}
}

// ydnar: haloed style 
textures/sfx/construction 
{ 
     cull none 
     deformVertexes wave 1 sin -0.5 0 0 1
     noPicmip 
     surfaceparm trans 
     { 
          map textures/sfx/construction.tga 
          blendFunc GL_SRC_ALPHA GL_ONE 
          rgbGen const ( 0.25 0.25 0.25 )
          tcGen environment 
          tcMod scroll 0.025 -0.07625 
     } 
}

textures/sfx/fan
{
	cull none
	nopicmip
	surfaceparm nomarks
	surfaceparm trans
	{
		clampmap textures/sfx/fan.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen identity
		tcMod rotate 256
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
		rgbGen identity
	}
}

textures/sfx/fan_static
{
	
	qer_editorimage textures/sfx/fan.tga
	cull none
	nopicmip
	surfaceparm nomarks
	surfaceparm trans
	{
		clampmap textures/sfx/fan.tga
		blendFunc GL_ONE GL_ZERO
		alphaFunc GE128
		rgbGen identity
	}
	{
		map $lightmap
		blendFunc GL_DST_COLOR GL_ZERO
		depthFunc equal
		rgbGen identity
	}
}

textures/sfx/fog_water
{
	qer_editorimage textures/sfx/fog_grey1.tga
	q3map_globaltexture
	q3map_surfacelight 0
	fogparms ( .05 .10 .13 ) 128
	surfaceparm fog
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm pointlight
	surfaceparm trans
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
		tcmod scale -.05 -.05
		tcmod scroll .01 -.01
	}
	{
		map textures/liquids/kc_fogcloud3.tga
		blendfunc gl_dst_color gl_zero
		rgbgen identity
		tcmod scale .05 .05
		tcmod scroll .01 -.01
	}
}

textures/sfx/glass
{
	qer_editorimage textures/common/dirtymirror.tga
	qer_trans 0.5
	cull none
	surfaceparm glass
	surfaceparm trans
	{
		map textures/common/dirtymirror.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
		tcgen environment
	}
	{
		map $lightmap
		blendFunc filter
		rgbGen identity
	}
}

textures/sfx/tramglass2
{
	qer_editorimage textures/common/dirtymirror2.tga
	qer_trans 0.5
	surfaceparm alphashadow
	surfaceparm glass
	surfaceparm pointlight
	surfaceparm trans
	{
		map textures/common/dirtymirror2.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

textures/sfx/safety_glass
{
	qer_trans 0.5
	qer_editorimage textures/common/s_glass.tga
	cull disable
	nomipmaps
	nopicmip
	surfaceparm glass
	surfaceparm alphashadow
	surfaceparm trans
	{
		map textures/common/s_glass.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
	}
}

textures/sfx/lightmap
{
	fogparms ( .7 .1 .1 ) 64
	surfaceparm fog
	surfaceparm nodrop
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm pointlight
	surfaceparm trans
	{
		map $lightmap
		blendFunc GL_dst_color GL_one
		tcmod scale 1 .01
		tcMod scroll 1 -2
	}
}

textures/sfx/wilsflame1
{
	qer_editorimage textures/sfx/flame1.tga
	q3map_surfacelight 1482
	cull none
	nofog
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm pointlight
	surfaceparm trans
	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
	}
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}
	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6
	}
}

textures/sfx/wilsflame2
{
	qer_editorimage textures/sfx/flame1.tga
	cull none
	nofog
	surfaceparm nomarks
	surfaceparm nonsolid
	surfaceparm pointlight
	surfaceparm trans
	{
		animMap 10 textures/sfx/flame1.tga textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave inverseSawtooth 0 1 0 10
	}
	{
		animMap 10 textures/sfx/flame2.tga textures/sfx/flame3.tga textures/sfx/flame4.tga textures/sfx/flame5.tga textures/sfx/flame6.tga textures/sfx/flame7.tga textures/sfx/flame8.tga textures/sfx/flame1.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sawtooth 0 1 0 10
	}
	{
		map textures/sfx/flameball.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave sin .6 .2 0 .6
	}
}
