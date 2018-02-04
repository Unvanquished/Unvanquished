console
{
	nopicmip
	nomipmaps
	{
		map gfx/colors/black
	}
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

white
{
	cull none
	{
		map *white
		blendfunc	GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbgen vertex
	}
}
