// required by engine
white
{
	cull none
	{
		map $whiteimage
		blendfunc	GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
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

// console font fallback
gfx/2d/bigchars
{
	nopicmip
	nomipmaps
	{
		map gfx/2d/bigchars
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}
