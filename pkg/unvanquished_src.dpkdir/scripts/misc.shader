gfx/misc/detail
{
	nopicmip
	{
		map gfx/misc/detail
		blendFunc GL_DST_COLOR GL_SRC_COLOR
		rgbgen identity
	}
}

gfx/misc/greenbuild
{
	{
		map gfx/misc/greenbuild
		blendfunc add
		rgbGen identity
	}
}

gfx/misc/redbuild
{
	{
		map gfx/misc/redbuild
		blendfunc add
		rgbGen identity
	}
}

gfx/misc/tracer
{
	cull none
	{
		map gfx/sprites/spark
		blendFunc blend
	}
}

console
{
	nopicmip
	nomipmaps
	{
		map gfx/colors/black
	}
}

creep
{
	nopicmip
	polygonoffset
	twoSided
	{
		clampmap gfx/creep/creep
		blendfunc blend
		blend diffusemap
		rgbGen identity
		alphaGen vertex
		alphaFunc GE128
	}
	specularMap gfx/creep/creep_s
	normalMap gfx/creep/creep_n
}

outline
{
	cull none
	nopicmip
	nomipmaps
	{
		map gfx/2d/outline
		blendfunc	GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

scope
{
	{
		clampmap gfx/2d/scope/zoom
		alphaGen vertex
		blend blend
	}
	{
		map gfx/2d/scope/bg
		alphaGen vertex
		blend blend
	}
	{
		map gfx/2d/scope/circle1
		alphaGen vertex
		blend blend
	}
	{
		clampmap gfx/2d/scope/circle2
		alphaGen vertex
		blend blend
		tcMod rotate 45
	}
	{
		clampmap gfx/2d/scope/crosshair
		alphaGen vertex
		blend blend
	}
}

white
{
	cull none
	{
		map *white
		blendfunc	GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}
