gfx/colors/white
{
	cull none
	{
		map $whiteimage
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}

gfx/colors/plain
{
	cull disable
	{
		map $whiteimage
		rgbGen entity
		alphaGen entity
		blendFunc GL_SRC_ALPHA GL_ONE
	}
}

gfx/colors/backtile
{
	nopicmip
	nomipmaps
	{
		map $blackimage
	}
}
